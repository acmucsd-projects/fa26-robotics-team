import cv2
import threading
import time
from pathlib import Path
from global_state import Global, put_latest
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

from global_state import Global, put_latest
import urllib.request

import math


def distance(a, b):
    return math.hypot(a.x - b.x, a.y - b.y)


def is_open_raised_hand(landmarks, img_height):
    """
    Returns True when the hand is open, upright,
    and raised into the upper 30% of the frame.

    landmarks: 21 MediaPipe hand landmarks
    """

    # 1. Check that four fingers are extended
    # (MCP, PIP, DIP, TIP)
    fingers = [
        (5, 6, 7, 8),       # Index
        (9, 10, 11, 12),    # Middle
        (13, 14, 15, 16),   # Ring
        (17, 18, 19, 20),   # Pinky
    ]

    extended_count = 0

    for mcp, pip, dip, tip in fingers:
        # Fingertip must be above the joints
        extended = (
            landmarks[tip].y < landmarks[dip].y
            < landmarks[pip].y < landmarks[mcp].y
        )

        if extended:
            extended_count += 1

    # 2. Thumb must be extended away from pinky base
    thumb_tip = landmarks[4]
    thumb_base = landmarks[2]
    pinky_base = landmarks[17]

    thumb_extended = (
        distance(thumb_tip, pinky_base)
        > distance(thumb_base, pinky_base) * 1.0
    )

    return extended_count == 4 and thumb_extended


def threadTarget(global_state):
    MODEL_PATH = Path(__file__).resolve().parent / "hand_landmarker.task"

    if not MODEL_PATH.exists():
        print("Downloading MediaPipe hand landmark model...")

        urllib.request.urlretrieve(
            "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task",
            str(MODEL_PATH)
        )

    print("Model downloaded successfully.")
    """
    Detect a hand in the top third of the frame to initialize
    the gimbal target, then stream frames to the processing queue.
    """
    global_state.running = True

    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    print(f"FPS: {cap.get(cv2.CAP_PROP_FPS)}")

    found_hands = False

    options = vision.HandLandmarkerOptions(
            base_options=python.BaseOptions(
                model_asset_path=str(MODEL_PATH)
            ),
            running_mode=vision.RunningMode.VIDEO,
            num_hands=2,
            min_hand_detection_confidence=0.5,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
        )
    
    
    with vision.HandLandmarker.create_from_options(options) as hands:

        start_time = time.monotonic()
        last_timestamp = -1

        # Phase 1: Wait for hand initialization
        while not found_hands and cap.isOpened() and global_state.running:
            success, frame = cap.read()

            if not success:
                print("Ignoring empty camera frame.")
                continue

            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

            mp_image = mp.Image(
                image_format=mp.ImageFormat.SRGB,
                data=frame_rgb
            )

            timestamp_ms = int(
                (time.monotonic() - start_time) * 1000
            )
            timestamp_ms = max(timestamp_ms, last_timestamp + 1)
            last_timestamp = timestamp_ms

            results = hands.detect_for_video(
                mp_image,
                timestamp_ms
            )

            if results.hand_landmarks:
                img_height, img_width = frame.shape[:2]

                for hand_landmarks in results.hand_landmarks:
                    # Landmark 0 = wrist
                    wrist = hand_landmarks[0]

                    pixel_x = int(wrist.x * img_width)
                    pixel_y = int(wrist.y * img_height)

                    if is_open_raised_hand(hand_landmarks, img_height):
                        print("Hand is raised and open. Initializing gimbal target.")
                        print(
                            f"Pixel Coordinates -> "
                            f"X: {pixel_x}, Y: {pixel_y}"
                        )

                        global_state.center_point = (
                            pixel_x,
                            pixel_y
                        )

                        found_hands = True
                        break

            # Optional preview
            cv2.imshow("MediaPipe Hands", cv2.flip(frame, 1))

            if cv2.waitKey(5) & 0xFF == 27:
                global_state.running = False
                break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    global_state = Global()
    threadTarget(global_state)