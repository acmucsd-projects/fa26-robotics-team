import queue
import cv2
import threading
import time
import math
import urllib.request
import traceback
from pathlib import Path

import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

from rfdetr import RFDETR
from global_state import Global
from target_tracker import TargetTracker


BASE_DIR = Path(__file__).resolve().parent
MODEL_PATH = BASE_DIR / "Arnavrfdetr_2.pth"
HAND_MODEL_PATH = BASE_DIR / "hand_landmarker.task"

PERSON_CLASS_ID = 0
DETECTION_THRESHOLD = 0.2

ROI_W = 385
ROI_H = 385
MAX_MISSES = 5

model = RFDETR.from_checkpoint(str(MODEL_PATH))


def distance(a, b):
    return math.hypot(a.x - b.x, a.y - b.y)


def is_open_raised_hand(landmarks):
    wrist = landmarks[0]

    # raised = wrist.y < 0.30
    # upright = landmarks[12].y < wrist.y

    fingers = [
        (5, 6, 7, 8),
        (9, 10, 11, 12),
        (13, 14, 15, 16),
        (17, 18, 19, 20),
    ]

    extended_count = 0

    for mcp, pip, dip, tip in fingers:
        if (
            landmarks[tip].y
            < landmarks[dip].y
            < landmarks[pip].y
            < landmarks[mcp].y
        ):
            extended_count += 1

    thumb_extended = (
        distance(landmarks[4], landmarks[17])
        > distance(landmarks[2], landmarks[17])
    )

    return extended_count == 4 and thumb_extended


def get_palm_center(landmarks, width, height):
    indices = [0, 5, 9, 13, 17]

    x = sum(landmarks[i].x for i in indices) / len(indices)
    y = sum(landmarks[i].y for i in indices) / len(indices)

    return int(x * width), int(y * height)


def download_hand_model():
    if HAND_MODEL_PATH.exists():
        return

    urllib.request.urlretrieve(
        "https://storage.googleapis.com/mediapipe-models/"
        "hand_landmarker/hand_landmarker/float16/1/"
        "hand_landmarker.task",
        str(HAND_MODEL_PATH)
    )

def make_roi(box, frame_width, frame_height):
    x1, y1, x2, y2 = box

    # Center of detected bounding box
    center_x = (x1 + x2) / 2
    center_y = (y1 + y2) / 2

    # Fixed ROI dimensions
    roi_width = ROI_W
    roi_height = ROI_H

    # Keep ROI within frame boundaries
    roi_x = int(max(
        0,
        min(center_x - roi_width / 2, frame_width - roi_width)
    ))

    roi_y = int(max(
        0,
        min(center_y - roi_height / 2, frame_height - roi_height)
    ))

    return (roi_x, roi_y, roi_width, roi_height)


def threadTarget(global_state):
    tracker = TargetTracker()
    hands = None
    acquisition_finished = False

    target_lost = False

    roi = None
    misses = 0

    start_time = time.monotonic()
    last_timestamp = -1

    try:
        download_hand_model()

        options = vision.HandLandmarkerOptions(
            base_options=python.BaseOptions(
                model_asset_path=str(HAND_MODEL_PATH)
            ),
            running_mode=vision.RunningMode.VIDEO,
            num_hands=2,
            min_hand_detection_confidence=0.5,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
        )

        hands = vision.HandLandmarker.create_from_options(options)

        while global_state.running or not global_state.input_stack.empty():
            try:
                frame_number, frame = global_state.input_stack.get(
                    timeout=0.1
                )
            except queue.Empty:
                continue

            height, width = frame.shape[:2]

            if target_lost:
                global_state.center_point = None

                global_state.output_stack.put(
                    (frame_number, frame)
                )

                continue

            center = None

            if acquisition_finished and roi is not None:
                rx, ry, rw, rh = roi

                input_frame = frame[
                    ry:ry + rh,
                    rx:rx + rw
                ]

                offset_x = rx
                offset_y = ry

            else:
                input_frame = frame
                offset_x = 0
                offset_y = 0

            detections = model.predict(
                input_frame,
                threshold=DETECTION_THRESHOLD
            )

            detections = detections[
                detections.class_id == PERSON_CLASS_ID
            ]

            if offset_x != 0 or offset_y != 0:
                detections.xyxy[:, [0, 2]] += offset_x
                detections.xyxy[:, [1, 3]] += offset_y

            if not acquisition_finished:
                mp_image = mp.Image(
                    image_format=mp.ImageFormat.SRGB,
                    data=frame
                )

                timestamp_ms = int(
                    (time.monotonic() - start_time) * 1000
                )
                timestamp_ms = max(
                    timestamp_ms,
                    last_timestamp + 1
                )
                last_timestamp = timestamp_ms

                results = hands.detect_for_video(
                    mp_image,
                    timestamp_ms
                )

                for landmarks in results.hand_landmarks:
                    if not is_open_raised_hand(landmarks):
                        continue

                    hand_point = get_palm_center(
                        landmarks, width, height
                    )

                    center = tracker.lock_target(
                        detections,
                        hand_point
                    )

                    if center is not None:
                        roi = make_roi(
                            tracker.target_box,
                            width,
                            height
                        )


                        acquisition_finished = True

                        hands.close()
                        hands = None

                        print("[PROCESSING] Target locked. MediaPipe stopped.")
                        break

            elif tracker.target_box is not None:
                center = tracker.update(detections)

                if center is not None:
                    roi = make_roi(
                        tracker.target_box,
                        width,
                        height
                    )

                    misses = 0

                else:
                    misses += 1

                    if misses >= MAX_MISSES:
                        print("[PROCESSING] Target lost.")

                        target_lost = True
                        roi = None
                        tracker.reset()
            else:
                target_lost = True

            global_state.center_point = center

            if tracker.target_box is not None:
                x1, y1, x2, y2 = map(int, tracker.target_box)

                cv2.rectangle(
                    frame,
                    (x1, y1),
                    (x2, y2),
                    (255, 255, 0),
                    3
                )

                cv2.circle(
                    frame,
                    center,
                    7,
                    (255, 0, 0),
                    -1
                )

                cv2.putText(
                    frame,
                    "TARGET LOCKED",
                    (x1, max(y1 - 30, 20)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (255, 255, 0),
                    2
                )
            else:
                for box, conf in zip(
                    detections.xyxy,
                    detections.confidence
                ):
                    x1, y1, x2, y2 = map(int, box)

                    cv2.rectangle(
                        frame,
                        (x1, y1),
                        (x2, y2),
                        (0, 255, 0),
                        2
                    )

                    cv2.putText(
                        frame,
                        f"Person: {conf:.2f}",
                        (x1, max(y1 - 10, 15)),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.5,
                        (0, 255, 0),
                        2
                    )

            if tracker.target_box is not None and center is not None:
                x1, y1, x2, y2 = map(int, tracker.target_box)

                cv2.rectangle(
                    frame,
                    (x1, y1),
                    (x2, y2),
                    (255, 255, 0),
                    3
                )

                cv2.circle(
                    frame,
                    center,
                    7,
                    (255, 0, 0),
                    -1
                )

                cv2.putText(
                    frame,
                    "TARGET LOCKED",
                    (x1, max(y1 - 30, 20)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (255, 255, 0),
                    2
                )

            if roi is not None:
                rx, ry, rw, rh = roi

                cv2.rectangle(
                    frame,
                    (rx, ry),
                    (rx + rw, ry + rh),
                    (0, 0, 255),
                    1
                )

            
            global_state.output_stack.put((frame_number, frame))
            global_state.data_stack.put((frame_number, detections))

    except Exception:
        print("[PROCESSING] Thread crashed:")
        traceback.print_exc()
        global_state.running = False

    finally:
        if hands is not None:
            hands.close()

        global_state.center_point = None
        print("[PROCESSING] Thread exited.")


def start_process_thread(global_state):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        daemon=False
    )

    thread.start()
    return thread
