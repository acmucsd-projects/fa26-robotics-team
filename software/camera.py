
import cv2
import os
from rfdetr import RFDETR

MODEL_PATH = os.path.abspath("checkpoint_best_total.pth")

# Load checkpoint using absolute path
model = RFDETR.from_checkpoint(MODEL_PATH)
cap = cv2.VideoCapture(0)

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    detections = model.predict(frame_rgb, threshold=0.75)

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

    cv2.imshow("RF-DETR Live Feed", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()