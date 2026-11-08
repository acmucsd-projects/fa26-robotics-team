import queue
import cv2
import threading
import os
from rfdetr import RFDETR
from global_state import Global

MODEL_PATH = os.path.abspath("software/Arnavrfdetr_2.pth")

# Load checkpoint using absolute path
model = RFDETR.from_checkpoint(MODEL_PATH)

def threadTarget(global_state):
    """
    This function is the target for the thread. It processes input data in a separate thread.
    """

    while global_state.running or not global_state.input_stack.empty():
        try:
            frame_number, frame = global_state.input_stack.get(timeout=0.1)
        except queue.Empty:
            continue

        detections = model.predict(frame, threshold=0.2)

        # boxes = []
        
        # Extract coordinates, confidence, and class IDs
        for xyxy, conf, class_id in zip(
            detections.xyxy, detections.confidence, detections.class_id
        ):
            x1, y1, x2, y2 = map(int, xyxy)
            label = f"Class {class_id}: {conf:.2f}"

            # Calculate center
            center_x = round((x1 + x2) / 2)
            center_y = round((y1 + y2) / 2)

            center_point = (center_x, center_y)

            # boxes.append((
            #         x1, y1, x2, y2,
            #         float(conf),
            #         int(class_id)
            # ))

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

            # Center point
            cv2.circle(
                frame,
                center_point,
                5,
                (255, 0, 0),
                -1
            )

            # Center coordinates
            cv2.putText(
                frame,
                f"({center_x}, {center_y})",
                (center_x + 10, center_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 0, 255),
                2
            )

        global_state.output_stack.put((frame_number, frame))
        global_state.data_stack.put((frame_number, detections))


def start_process_thread(global_state):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        # daemon=True,
    )
    thread.start()
    return thread