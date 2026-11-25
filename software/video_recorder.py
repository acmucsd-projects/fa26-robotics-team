from datetime import datetime
from pathlib import Path

import cv2
import numpy as np
import threading


class VideoRecorder:
    def __init__(self):
        self._lock = threading.Lock()
        self._writer = None
        self._state = "stopped"

    def start(self):
        with self._lock:
            if self._state == "stopped":
                self._state = "recording"

    def toggle_pause(self):
        with self._lock:
            if self._state == "recording":
                self._state = "paused"
            elif self._state == "paused":
                self._state = "recording"

    def stop(self):
        with self._lock:
            self._state = "stopped"
            if self._writer is not None:
                self._writer.release()
                self._writer = None

    def write(self, frame, fps):
        with self._lock:
            if self._state != "recording":
                return

            if self._writer is None:
                folder = Path("software/recordings")
                folder.mkdir(exist_ok=True)
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
                path = folder / f"recording_{timestamp}.mp4"

                height, width = frame.shape[:2]
                writer = cv2.VideoWriter(
                    str(path),
                    cv2.VideoWriter_fourcc(*"mp4v"),
                    fps,
                    (width, height),
                )

                if not writer.isOpened():
                    writer.release()
                    self._state = "stopped"
                    print(f"Could not open video file: {path}")
                    return

                self._writer = writer
                print(f"Recording to {path.resolve()}")

            self._writer.write(frame)

