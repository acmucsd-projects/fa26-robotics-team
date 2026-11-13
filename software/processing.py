
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
from target_tracker import TargetTracker

# Model paths and detection settings
BASE_DIR = Path(__file__).resolve().parent
MODEL_PATH = BASE_DIR / 'Arnavrfdetr_2.pth'
HAND_MODEL_PATH = BASE_DIR / 'hand_landmarker.task'

PERSON_CLASS_ID = 0
DETECTION_THRESHOLD = 0.2
ROI_W = ROI_H = 385
MAX_MISSES = 5

# MQTT settings for sending target coordinates to the gimbal
BROKER = 'broker.emqx.io'
PORT = 1883
TOPIC = 'gimbal/commands'

# Load RF-DETR once instead of reloading it for every frame
model = RFDETR.from_checkpoint(str(MODEL_PATH))
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(BROKER, PORT, keepalive=60)
client.loop_start()


def send(center):
    # Send the target center without waiting for the MQTT acknowledgment
    info = client.publish(TOPIC, f'{center[0]},{center[1]}', qos=1)
    # info.wait_for_publish()
    print(f'Sent center point: {center[0]},{center[1]}')


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


def threadTarget(global_state):
    # Set up the tracker and initial detection state
    tracker = TargetTracker()
    hands = None
    acquisition_finished = False
    target_lost = False
    roi = None
    misses = 0
    start_time = time.monotonic()
    last_timestamp = -1

    try:
        # Initialize MediaPipe for detecting the initial raised hand
        download_hand_model()
        hands = vision.HandLandmarker.create_from_options(
            vision.HandLandmarkerOptions(
                base_options=python.BaseOptions(model_asset_path=str(HAND_MODEL_PATH)),
                running_mode=vision.RunningMode.VIDEO,
                num_hands=2,
                min_hand_detection_confidence=0.5,
                min_hand_presence_confidence=0.5,
                min_tracking_confidence=0.5,
            )
        )

        # Keep processing frames while the system is running or frames remain
        while global_state.running or not global_state.input_stack.empty():
            try:
                frame_number, frame = global_state.input_stack.get(timeout=0.1)
            except queue.Empty:
                continue

            # Drop older frames so inference works on the latest camera frame
            try:
                while True:
                    frame_number, frame = global_state.input_stack.get_nowait()
            except queue.Empty:
                pass

            height, width = frame.shape[:2]
            center = None

            # After locking, run detection only inside the current ROI
            if acquisition_finished and roi is not None:
                rx, ry, rw, rh = roi
                input_frame = frame[ry:ry + rh, rx:rx + rw]
                offset_x, offset_y = rx, ry
            else:
                # Use the full frame before locking or when the target is lost
                input_frame = frame
                offset_x = offset_y = 0

            # Run RF-DETR and keep only person detections
            detections = model.predict(input_frame, threshold=DETECTION_THRESHOLD)
            detections = detections[detections.class_id == PERSON_CLASS_ID]

            # Convert ROI-relative boxes back to full-frame coordinates
            if offset_x or offset_y:
                detections.xyxy[:, [0, 2]] += offset_x
                detections.xyxy[:, [1, 3]] += offset_y

            if not acquisition_finished:
                # MediaPipe needs timestamps that increase with every frame
                timestamp = max(int((time.monotonic() - start_time) * 1000),
                                last_timestamp + 1)
                last_timestamp = timestamp

                # Look for an open hand to identify which person to follow
                result = hands.detect_for_video(
                    mp.Image(image_format=mp.ImageFormat.SRGB, data=frame),
                    timestamp
                )

                for landmarks in result.hand_landmarks:
                    if is_open_raised_hand(landmarks):
                        center = tracker.lock_target(
                            detections, get_palm_center(landmarks, width, height)
                        )

                        if center is not None:
                            # Lock the person and start tracking inside the ROI
                            acquisition_finished = True
                            roi = make_roi(tracker.target_box, width, height)

                            # Hand detection is no longer needed after locking
                            hands.close()
                            hands = None
                            print('[PROCESSING] Target locked. MediaPipe stopped.')
                            break

            elif tracker.target_box is not None:
                # Match the current detections against the previous target
                center = tracker.update(detections)

                if center is not None:
                    # Move the ROI to follow the updated target position
                    roi = make_roi(tracker.target_box, width, height)
                    misses = 0
                else:
                    # Allow a few missed detections before declaring target loss
                    misses += 1
                    if misses >= MAX_MISSES:
                        print('[PROCESSING] Target lost.')
                        target_lost = True
                        roi = None
                        tracker.reset()  # TargetTracker.reset preserves center_point.

            elif target_lost:
                # Search the full frame for a person near the last known center
                center = tracker.relock_target(detections, tracker.center_point)

                if center is not None:
                    # Resume ROI tracking once the target has been found
                    print('[PROCESSING] Target relocked.')
                    target_lost = False
                    misses = 0
                    roi = make_roi(tracker.target_box, width, height)

            else:
                target_lost = True

            if center is not None:
                # Share the center with the gimbal and draw the locked target
                global_state.center_point = center
                send(center)
                draw_target(frame, tracker.target_box, center)
            else:
                global_state.center_point = None

            # Show all person detections before locking or during reacquisition
            if center is None and (not acquisition_finished or target_lost):
                draw_detections(frame, detections)

            # Draw the red ROI outline to show where inference is focused
            if roi is not None:
                rx, ry, rw, rh = roi
                cv2.rectangle(frame, (rx, ry), (rx + rw, ry + rh), (0, 0, 255), 1)

            # Send the annotated frame to the output thread
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
