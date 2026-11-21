import math
import queue
import threading
import time
import traceback
import urllib.request
from pathlib import Path

import cv2
import mediapipe as mp
import paho.mqtt.client as mqtt
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from rfdetr import RFDETR
from gimbal_kinematics import GimbalKinematics


# Model paths and detection settings
BASE_DIR = Path(__file__).resolve().parent
MODEL_PATH = BASE_DIR / 'Arnavrfdetr_2.pth'
HAND_MODEL_PATH = BASE_DIR / 'hand_landmarker.task'

PERSON_CLASS_ID = 0
DETECTION_THRESHOLD = 0.25
ROI_W = ROI_H = 385
MAX_OCCLUSION_S = 1.2
FULL_FRAME_AFTER_S = 0.35
MAX_VELOCITY_SAMPLE_GAP_S = 0.25
REACQUIRE_RADIUS_PX = 100

# Set these to the FOV of the camera stream used for detection.
CAMERA_HORIZONTAL_FOV_DEG = 60.0  # Example value. Replace with your camera's FOV.
CAMERA_VERTICAL_FOV_DEG = None  # Infer from square pixels unless measured.
kinematics = GimbalKinematics(CAMERA_HORIZONTAL_FOV_DEG, CAMERA_VERTICAL_FOV_DEG)


# MQTT settings for sending target coordinates to the gimbal
BROKER = 'broker.emqx.io'
PORT = 1883
TOPIC = 'gimbal/commands'

# Load RF-DETR once instead of reloading it for every frame
model = RFDETR.from_checkpoint(str(MODEL_PATH))
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(BROKER, PORT, keepalive=60)
client.loop_start()

# future iterations using 2D inverse kinematics will be send over the form f'angle for bottom motor, angle for top motor'
def send(center, width, height):
    if center[0] != width//2 and center[1] != height//2:
        if center[0] < width//2 and center[1] < height//2:
            client.publish(TOPIC, f'up,left', qos=1)
            print(f"Move Gimbal Up Left: {center[0]},{center[1]}")

        elif center[0] > width//2 and center[1] > height//2:
            client.publish(TOPIC, f'down,right', qos=1)
            print(f"Move Gimbal Down Right: {center[0]},{center[1]}")

        elif center[0] < width//2 and center[1] > height//2:
            client.publish(TOPIC, f'down,left', qos=1)
            print(f"Move Gimbal Down Left: {center[0]},{center[1]}")

        elif center[0] > width//2 and center[1] < height//2:
            client.publish(TOPIC, f'up,right', qos=1)
            print(f"Move Gimbal Up Right: {center[0]},{center[1]}")

    # # Send the target center without waiting for the MQTT acknowledgment
    # info = client.publish(TOPIC, f'{center[0]},{center[1]}', qos=1)
    # # info.wait_for_publish()
    # print(f'Sent center point: {center[0]},{center[1]}')

def send_kin(center, width, height):
    # Wire order is bottom motor (yaw), then top motor (pitch).
    # Values are camera-relative corrections in degrees, positive right and up.
    yaw_deg, pitch_deg = kinematics.angles_for_pixel(center, width, height)
    client.publish(TOPIC, f'{yaw_deg:.3f},{pitch_deg:.3f}', qos=1)

def is_open_raised_hand(lm):
    # Check that all four fingers are extended upward
    fingers = ((5, 6, 7, 8), (9, 10, 11, 12),
               (13, 14, 15, 16), (17, 18, 19, 20))
    extended = all(lm[t].y < lm[d].y < lm[p].y < lm[m].y
                   for m, p, d, t in fingers)

    # Check that the thumb is extended away from the palm
    thumb = math.dist((lm[4].x, lm[4].y), (lm[17].x, lm[17].y)) > \
            math.dist((lm[2].x, lm[2].y), (lm[17].x, lm[17].y))
    return extended and thumb


def get_palm_center(lm, width, height):
    # Average the wrist and finger base positions to find the palm center
    points = (0, 5, 9, 13, 17)
    return (int(sum(lm[i].x for i in points) * width / 5),
            int(sum(lm[i].y for i in points) * height / 5))


def download_hand_model():
    # Only download the MediaPipe model if it isn't already saved
    if not HAND_MODEL_PATH.exists():
        urllib.request.urlretrieve(
            'https://storage.googleapis.com/mediapipe-models/'
            'hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task',
            str(HAND_MODEL_PATH)
        )


def make_roi(box, width, height):
    # Center a fixed 385x385 ROI around the tracked person
    x1, y1, x2, y2 = box

    # Keep the ROI inside the camera frame
    x = int(max(0, min((x1 + x2) / 2 - ROI_W / 2, width - ROI_W)))
    y = int(max(0, min((y1 + y2) / 2 - ROI_H / 2, height - ROI_H)))
    return x, y, ROI_W, ROI_H


def draw_detections(frame, detections):
    # Draw green boxes around detected people before locking or while searching
    for box, conf in zip(detections.xyxy, detections.confidence):
        x1, y1, x2, y2 = map(int, box)
        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.putText(frame, f'Person: {conf:.2f}',
                    (x1, max(y1 - 10, 15)), cv2.FONT_HERSHEY_SIMPLEX,
                    0.5, (0, 255, 0), 2)


def draw_target(frame, box, center):
    # Highlight the locked person and show the center used for gimbal control
    x1, y1, x2, y2 = map(int, box)
    cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 255, 0), 3)
    cv2.circle(frame, center, 7, (255, 0, 0), -1)
    cv2.putText(frame, 'TARGET LOCKED', (x1, max(y1 - 30, 20)),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)


def box_center(box):
    x1, y1, x2, y2 = box
    return (float(x1 + x2) / 2, float(y1 + y2) / 2)


def closest_box_to_center(boxes, center):
    closest = None
    closest_distance = math.inf
    for box in boxes:
        x1, y1, x2, y2 = box
        if x2 <= x1 or y2 <= y1:
            continue
        distance = math.dist(center, box_center(box))
        if distance <= REACQUIRE_RADIUS_PX and distance < closest_distance:
            closest = box
            closest_distance = distance
    return closest


class TargetMotion:
    def __init__(self):
        self.last_box = None
        self.last_seen_at = None
        self.velocity = None

    def observe(self, box, captured_at):
        if self.last_box is not None:
            dt = captured_at - self.last_seen_at
            if dt <= 0:
                return

            if dt <= MAX_VELOCITY_SAMPLE_GAP_S:
                previous = box_center(self.last_box)
                current = box_center(box)
                measured = tuple((new - old) / dt for old, new in zip(previous, current))
                if self.velocity is None:
                    self.velocity = measured
                else:
                    self.velocity = tuple(
                        0.6 * old + 0.4 * new
                        for old, new in zip(self.velocity, measured)
                    )
            else:
                self.velocity = None

        self.last_box = tuple(map(float, box))
        self.last_seen_at = captured_at

    def predict(self, captured_at):
        elapsed = captured_at - self.last_seen_at
        vx, vy = self.velocity or (0.0, 0.0)
        dx, dy = vx * elapsed, vy * elapsed
        x1, y1, x2, y2 = self.last_box
        return x1 + dx, y1 + dy, x2 + dx, y2 + dy

    def match(self, boxes, predicted_box, elapsed):
        predicted_center = box_center(predicted_box)
        predicted_height = predicted_box[3] - predicted_box[1]
        radius = min(150.0, 45.0 + 75.0 * elapsed)
        candidates = []
        for box in boxes:
            distance = math.dist(predicted_center, box_center(box))
            height_ratio = float(box[3] - box[1]) / predicted_height
            if distance <= radius and 0.5 <= height_ratio <= 2.0:
                score = distance + 30.0 * abs(math.log(height_ratio))
                candidates.append((score, box))

        candidates.sort(key=lambda candidate: candidate[0])
        if not candidates:
            return None
        if len(candidates) > 1 and candidates[1][0] - candidates[0][0] < 12:
            return None
        return candidates[0][1]


def open_hand_landmarker():
    download_hand_model()
    return vision.HandLandmarker.create_from_options(
        vision.HandLandmarkerOptions(
            base_options=python.BaseOptions(model_asset_path=str(HAND_MODEL_PATH)),
            running_mode=vision.RunningMode.VIDEO,
            num_hands=2,
            min_hand_detection_confidence=0.5,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
        )
    )


def threadTarget(global_state):
    motion = None
    lost_target_center = None
    hands = None
    last_timestamp = -1

    try:
        while global_state.running or not global_state.input_stack.empty():
            try:
                packet = global_state.input_stack.get(timeout=0.1)
            except queue.Empty:
                continue

            try:
                while True:
                    packet = global_state.input_stack.get_nowait()
            except queue.Empty:
                pass

            if len(packet) == 3:
                frame_number, captured_at, frame = packet
            else:
                frame_number, frame = packet
                captured_at = time.monotonic()

            height, width = frame.shape[:2]
            elapsed = 0.0 if motion is None else captured_at - motion.last_seen_at

            if motion is not None and elapsed >= MAX_OCCLUSION_S:
                # Anchor long-gap recovery to the last detection, before extrapolation.
                lost_target_center = box_center(motion.last_box)
                motion = None
                print('[PROCESSING] Target lost. Searching near the last known center.')

            predicted_box = None
            roi = None
            input_frame = frame
            rx = ry = 0
            if motion is not None:
                predicted_box = motion.predict(captured_at)
                if elapsed < FULL_FRAME_AFTER_S and width >= ROI_W and height >= ROI_H:
                    roi = make_roi(predicted_box, width, height)
                    rx, ry, rw, rh = roi
                    input_frame = frame[ry:ry + rh, rx:rx + rw]

            detections = model.predict(input_frame, threshold=DETECTION_THRESHOLD)
            detections = detections[detections.class_id == PERSON_CLASS_ID]
            if rx or ry:
                detections.xyxy[:, [0, 2]] += rx
                detections.xyxy[:, [1, 3]] += ry

            matched_box = None
            if motion is not None:
                matched_box = motion.match(detections.xyxy, predicted_box, elapsed)
            elif lost_target_center is not None:
                matched_box = closest_box_to_center(detections.xyxy, lost_target_center)
                if matched_box is not None:
                    motion = TargetMotion()
                    lost_target_center = None
                    print('[PROCESSING] Target reacquired near the last known center.')
            else:
                if hands is None:
                    hands = open_hand_landmarker()

                timestamp = max(int(captured_at * 1000), last_timestamp + 1)
                last_timestamp = timestamp
                result = hands.detect_for_video(
                    mp.Image(image_format=mp.ImageFormat.SRGB, data=frame),
                    timestamp,
                )
                for landmarks in result.hand_landmarks:
                    if not is_open_raised_hand(landmarks):
                        continue

                    px, py = get_palm_center(landmarks, width, height)
                    containing = [
                        box for box in detections.xyxy
                        if box[2] > box[0] and box[3] > box[1]
                        and box[0] <= px <= box[2] and box[1] <= py <= box[3]
                    ]
                    if len(containing) == 1:
                        matched_box = containing[0]
                        motion = TargetMotion()
                        hands.close()
                        hands = None
                        print('[PROCESSING] Target locked. MediaPipe stopped.')
                        break

            if matched_box is not None:
                motion.observe(matched_box, captured_at)
                center = tuple(round(value) for value in box_center(matched_box))
                global_state.center_point = center
                # send(center, width, height)
                send_kin(center, width, height)
                draw_target(frame, matched_box, center)
            else:
                global_state.center_point = None
                if predicted_box is None:
                    draw_detections(frame, detections)
                else:
                    x1, y1, x2, y2 = map(round, predicted_box)
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 0, 255), 2)
                    cv2.putText(frame, 'PREDICTED', (max(0, x1), max(15, y1 - 8)),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 255), 2)

            if roi is not None:
                _, _, rw, rh = roi
                cv2.rectangle(frame, (rx, ry), (rx + rw - 1, ry + rh - 1), (0, 0, 255), 1)

            global_state.output_stack.put((frame_number, frame))

    except Exception:
        # Report errors and stop the processing loop
        print('[PROCESSING] Thread crashed:')
        traceback.print_exc()
        global_state.running = False

    finally:
        # Release MediaPipe resources and clear the shared target center
        if hands is not None:
            hands.close()
        global_state.center_point = None
        print('[PROCESSING] Thread exited.')


def start_process_thread(global_state):
    # Run processing separately from camera capture and display
    thread = threading.Thread(target=threadTarget, args=(global_state,), daemon=False)
    thread.start()
    return thread
