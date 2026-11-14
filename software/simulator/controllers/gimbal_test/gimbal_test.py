import io
import json
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import numpy as np
import paho.mqtt.client as mqtt
from controller import Robot

try:
    import cv2
    HAS_CV2 = True
except ImportError:
    from PIL import Image
    HAS_CV2 = False

# --- EMQX Broker Configuration ---
BROKER = "broker.emqx.io"
PORT = 1883
TOPIC = "gimbal/commands"
USERNAME = ""
PASSWORD = ""

# --- Localhost Stream Configuration ---
STREAM_HOST = "0.0.0.0"
STREAM_PORT = 8080
JPEG_QUALITY = 80               # 1-100 (lower = lower latency & bandwidth)

# --- Step Size per Command (in radians) ---
YAW_STEP = 0.08                  # ~11.5 degrees per "left"/"right" message
PITCH_STEP = 0.08                # ~11.5 degrees per "up"/"down" message

# --- Target Motor Positions ---
target_yaw = 0.0
target_pitch = 0.0
position_updated = True

# --- Shared Frame Buffer for Video Stream ---
latest_jpeg = None
active_clients = 0
frame_condition = threading.Condition()

class MJPEGHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global active_clients
        self.send_response(200)
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        self.send_header("Pragma", "no-cache")
        self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=frame")
        self.end_headers()

        with frame_condition:
            active_clients += 1

        last_sent = None
        try:
            while True:
                with frame_condition:
                    frame_condition.wait(timeout=1.0)
                    frame = latest_jpeg

                if frame is None or frame is last_sent:
                    continue

                self.wfile.write(b"--frame\r\n")
                self.wfile.write(b"Content-Type: image/jpeg\r\n")
                self.wfile.write(f"Content-Length: {len(frame)}\r\n\r\n".encode("utf-8"))
                self.wfile.write(frame)
                self.wfile.write(b"\r\n")
                last_sent = frame
        except (BrokenPipeError, ConnectionResetError, OSError):
            pass
        finally:
            with frame_condition:
                active_clients = max(0, active_clients - 1)

    def log_message(self, format, *args):
        pass  # Suppress per-request HTTP logs

class ReusableThreadingHTTPServer(ThreadingHTTPServer):
    allow_reuse_address = True
    daemon_threads = True

def encode_jpeg(raw_bgra, width, height, quality):
    bgra = np.frombuffer(raw_bgra, dtype=np.uint8).reshape((height, width, 4))
    if HAS_CV2:
        ok, buf = cv2.imencode(".jpg", bgra[:, :, :3], [cv2.IMWRITE_JPEG_QUALITY, quality])
        return buf.tobytes() if ok else None
    else:
        rgb = bgra[:, :, [2, 1, 0]]
        buf = io.BytesIO()
        Image.fromarray(rgb).save(buf, format="JPEG", quality=quality)
        return buf.getvalue()

# --- 1. Initialize Webots Robot & Camera FIRST ---
robot = Robot()
timestep = int(robot.getBasicTimeStep())

camera = robot.getDevice("gimbal_camera")
camera.enable(timestep)
cam_width = camera.getWidth()
cam_height = camera.getHeight()

yaw_motor = robot.getDevice("yaw_motor")
pitch_motor = robot.getDevice("pitch_motor")

yaw_motor.setVelocity(1.0)
pitch_motor.setVelocity(1.0)

# Read Webots motor limits (in Webots, min == max == 0.0 means unlimited)
yaw_min, yaw_max = yaw_motor.getMinPosition(), yaw_motor.getMaxPosition()
pitch_min, pitch_max = pitch_motor.getMinPosition(), pitch_motor.getMaxPosition()
print(f"Yaw limits:   min={yaw_min:.2f}, max={yaw_max:.2f}")
print(f"Pitch limits: min={pitch_min:.2f}, max={pitch_max:.2f}")

def clamp(value, min_val, max_val):
    if max_val > min_val:  # Only clamp if Webots limits are actually set
        return max(min_val, min(max_val, value))
    return value

robot.step(timestep)

# --- 2. Start Localhost MJPEG Stream Server ---
http_server = ReusableThreadingHTTPServer((STREAM_HOST, STREAM_PORT), MJPEGHandler)
stream_thread = threading.Thread(target=http_server.serve_forever, daemon=True)
stream_thread.start()
print(f"Camera stream live at: http://{STREAM_HOST}:{STREAM_PORT}")

# --- 3. MQTT Callbacks ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"Connected to EMQX broker at {BROKER}:{PORT}")
        client.subscribe(TOPIC)
        print(f"Subscribed to topic: {TOPIC}")
    else:
        print(f"Failed to connect to EMQX, return code: {rc}")

def on_message(client, userdata, msg):
    global target_yaw, target_pitch, position_updated
    payload = msg.payload.decode("utf-8").strip()
    print(f"Received [{msg.topic}]: {payload}")

    # Optional JSON support for jumping directly to an angle: {"yaw": 1.0, "pitch": 0.5}
    if ":" in payload:
        try:
            data = json.loads(payload)
            if isinstance(data, dict):
                if "yaw" in data:
                    target_yaw = clamp(float(data["yaw"]), yaw_min, yaw_max)
                if "pitch" in data:
                    target_pitch = clamp(float(data["pitch"]), pitch_min, pitch_max)
                position_updated = True
                return
        except (json.JSONDecodeError, ValueError, TypeError) as e:
            print(f"Invalid JSON format: {e}")
            return

    # Parse "{left, up}" style incremental commands
    cleaned = payload.strip("{} ").lower()
    if not cleaned:
        return

    commands = [cmd.strip() for cmd in cleaned.split(",") if cmd.strip()]

    for cmd in commands:
        if cmd == "left":
            target_yaw = clamp(target_yaw + YAW_STEP, yaw_min, yaw_max)
        elif cmd == "right":
            target_yaw = clamp(target_yaw - YAW_STEP, yaw_min, yaw_max)
        elif cmd == "up":
            target_pitch = clamp(target_pitch + PITCH_STEP, pitch_min, pitch_max)
        elif cmd == "down":
            target_pitch = clamp(target_pitch - PITCH_STEP, pitch_min, pitch_max)
        elif cmd == "center":
            target_yaw = 0.0
            target_pitch = 0.0
        else:
            print(f"Unrecognized direction '{cmd}' in message: {payload}")
            continue
        position_updated = True

    print(f"Target -> yaw: {target_yaw:.2f} rad, pitch: {target_pitch:.2f} rad")

# --- 4. Connect to EMQX Asynchronously ---
try:
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
except AttributeError:
    mqtt_client = mqtt.Client()

if USERNAME and PASSWORD:
    mqtt_client.username_pw_set(USERNAME, PASSWORD)

mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect_async(BROKER, PORT, keepalive=60)
mqtt_client.loop_start()

print("--- Gimbal MQTT Controller Active ---")

# --- 5. Main Simulation Loop ---
try:
    while robot.step(timestep) != -1:
        if position_updated:
            yaw_motor.setPosition(target_yaw)
            pitch_motor.setPosition(target_pitch)
            position_updated = False

        # Only encode JPEG frames while at least one client is watching
        if active_clients > 0:
            raw_image = camera.getImage()
            if raw_image is not None:
                jpeg_bytes = encode_jpeg(raw_image, cam_width, cam_height, JPEG_QUALITY)
                if jpeg_bytes:
                    with frame_condition:
                        latest_jpeg = jpeg_bytes
                        frame_condition.notify_all()
finally:
    http_server.shutdown()
    http_server.server_close()
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
