import asyncio
from datetime import datetime, timedelta, timezone
import math
import os
import socket
import threading
import time

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
import cv2
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
import numpy as np
import uvicorn

app = FastAPI()

# Shared latest frame state (thread-safe for your RF-DETR loop)
frame_lock = threading.Lock()
latest_jpeg: bytes | None = None
latest_frame_id: int = 0

PHONE_HTML = """
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Phone Camera Streamer</title>
  <style>
    body { font-family: sans-serif; background: #111; color: #fff; text-align: center; margin: 0; padding: 20px; }
    video { width: 100%; max-width: 480px; border-radius: 12px; background: #000; }
    #status { margin-top: 12px; color: #4ade80; font-weight: bold; }
  </style>
</head>
<body>
  <h3>Phone Camera (Low-Latency WS)</h3>
  <video id="video" autoplay playsinline muted></video>
  <div id="status">Requesting camera access...</div>

  <script>
    const video = document.getElementById('video');
    const status = document.getElementById('status');
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    let ws;

    function connectWS() {
      ws = new WebSocket(`wss://${location.host}/ws`);
      ws.onopen = () => { status.style.color = '#4ade80'; status.textContent = 'Streaming live (WebSocket)...'; };
      ws.onclose = () => { status.style.color = '#facc15'; status.textContent = 'Reconnecting...'; setTimeout(connectWS, 500); };
    }

    async function start() {
      try {
        const stream = await navigator.mediaDevices.getUserMedia({
          video: { facingMode: 'environment', width: { ideal: 640 }, height: { ideal: 480 }, frameRate: { ideal: 30 } },
          audio: false
        });
        video.srcObject = stream;
        video.onloadedmetadata = () => {
          // Set canvas dimensions ONCE to avoid GPU buffer reallocation every frame
          canvas.width = video.videoWidth;
          canvas.height = video.videoHeight;
          connectWS();
          scheduleNextFrame();
        };
      } catch (err) {
        status.style.color = '#f87171';
        status.textContent = 'Error: ' + err.message;
      }
    }

    function scheduleNextFrame() {
      if ('requestVideoFrameCallback' in video) {
        video.requestVideoFrameCallback(processFrame);
      } else {
        requestAnimationFrame(processFrame);
      }
    }

    function processFrame() {
      // Only encode & send if WebSocket is open and has zero queued backlog
      if (ws && ws.readyState === WebSocket.OPEN && ws.bufferedAmount === 0) {
        ctx.drawImage(video, 0, 0);
        canvas.toBlob((blob) => {
          if (blob && ws.readyState === WebSocket.OPEN) {
            ws.send(blob);
          }
          scheduleNextFrame();
        }, 'image/jpeg', 0.6);
      } else {
        scheduleNextFrame();
      }
    }

    start();
  </script>
</body>
</html>
"""


@app.get("/", response_class=HTMLResponse)
async def index():
  return PHONE_HTML


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
  global latest_jpeg, latest_frame_id
  await websocket.accept()
  try:
    while True:
      data = await websocket.receive_bytes()
      with frame_lock:
        latest_jpeg = data
        latest_frame_id += 1
  except WebSocketDisconnect:
    pass


# --- YOUR RF-DETR + IK + MQTT LOOP ---
def vision_tracking_loop():
  # iPhone 15 1x Main camera constants at 640x480
  FX, FY = 490.0, 490.0
  CX, CY = 320.0, 240.0
  KP = 0.5  # Visual damping gain to prevent Wi-Fi lag overshoot

  current_pan = 0.0  # Updated via your MQTT 'gimbal/state' callback
  current_tilt = 0.0  # Updated via your MQTT 'gimbal/state' callback
  last_processed_id = -1

  while True:
    with frame_lock:
      if latest_jpeg is None or latest_frame_id == last_processed_id:
        jpeg_bytes = None
      else:
        jpeg_bytes = latest_jpeg
        last_processed_id = latest_frame_id

    if jpeg_bytes is None:
      time.sleep(0.002)  # Wait 2ms for a fresh frame
      continue

    # Decode directly from RAM (0ms VideoCapture buffer lag)
    frame = cv2.imdecode(
        np.frombuffer(jpeg_bytes, np.uint8), cv2.IMREAD_COLOR
    )
    if frame is None:
      continue

    # 1. Run RF-DETR inference on `frame` here -> get [x_min, y_min, x_max, y_max]
    # ...

    # 2. Example IK math once you have a person bounding box:
    # u = (x_min + x_max) / 2.0
    # v = y_min + 0.25 * (y_max - y_min)  # Upper-chest aim point
    # alpha_x = math.degrees(math.atan2(u - CX, FX))
    # alpha_y = math.degrees(math.atan2(v - CY, FY))
    # target_tilt = current_tilt - KP * alpha_y
    # target_pan = current_pan + KP * (alpha_x / math.cos(math.radians(current_tilt)))
    # mqtt_client.publish("gimbal/target", json.dumps({"pan": target_pan, "tilt": target_tilt}))


def ensure_self_signed_cert(cert_file="cert.pem", key_file="key.pem"):
  if os.path.exists(cert_file) and os.path.exists(key_file):
    return
  key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
  subject = issuer = x509.Name(
      [x509.NameAttribute(NameOID.COMMON_NAME, "localhost")]
  )
  now = datetime.now(timezone.utc)
  cert = (
      x509.CertificateBuilder()
      .subject_name(subject)
      .issuer_name(issuer)
      .public_key(key.public_key())
      .serial_number(x509.random_serial_number())
      .not_valid_before(now)
      .not_valid_after(now + timedelta(days=365))
      .sign(key, hashes.SHA256())
  )
  with open(key_file, "wb") as f:
    f.write(
        key.private_bytes(
            serialization.Encoding.PEM,
            serialization.PrivateFormat.TraditionalOpenSSL,
            serialization.NoEncryption(),
        )
    )
  with open(cert_file, "wb") as f:
    f.write(cert.public_bytes(serialization.Encoding.PEM))


def get_local_ip():
  s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  try:
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]
  except Exception:
    return "127.0.0.1"
  finally:
    s.close()


if __name__ == "__main__":
  ensure_self_signed_cert()
  ip = get_local_ip()
  port = 5050

  # Start RF-DETR tracking thread
  threading.Thread(target=vision_tracking_loop, daemon=True).start()

  print(f"\nOpen on your PHONE: https://{ip}:{port}\n")
  uvicorn.run(
      app, host="0.0.0.0", port=port, ssl_keyfile="key.pem", ssl_certfile="cert.pem"
  )