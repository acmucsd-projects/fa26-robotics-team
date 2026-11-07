import queue
import cv2
import threading
import os
from rfdetr import RFDETR
from gimbal_global_state import Global, put_latest

# MODEL_PATH = os.path.abspath("software/simulator/controllers/gimbal_test/Arnavrfdetr_2.pth")
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODEL_PATH = os.path.join(BASE_DIR, "Arnavrfdetr_2.pth")


# Load checkpoint using absolute path
model = RFDETR.from_checkpoint(MODEL_PATH)

def threadTarget(global_state):
    """
    This function is the target for the thread. It processes input data in a separate thread.
    """

    while global_state.running or not global_state.input_queue.empty():
        try:
            frame_number, frame = global_state.input_queue.get(timeout=0.1)
        except queue.Empty:
            continue

        detections = model.predict(frame, threshold=0.2)
        
        # Extract coordinates, confidence, and class IDs
        for xyxy, conf, class_id in zip(
            detections.xyxy, detections.confidence, detections.class_id
        ):
            x1, y1, x2, y2 = map(int, xyxy)
            label = f"Class {class_id}: {conf:.2f}"

            # Draw box and label
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(
                frame,
                label,
                (x1, max(y1 - 10, 15)),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 255, 0),
                2,
            )

        put_latest(global_state.output_queue, (frame_number, frame))
        put_latest(global_state.data_queue, (frame_number, detections))


def start_process_thread(global_state):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        # daemon=True,
    )
    thread.start()
    return thread