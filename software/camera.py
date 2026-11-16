import cv2
import os
import numpy as np
import torch
from rfdetr import RFDETR

MODEL_PATH = os.path.abspath("checkpoint_best_total_v7.pth")
PERSON_CLASS = 0          
THRESH = 0.25
ROI_W, ROI_H = 385, 385
FRAME_W, FRAME_H = 640, 480   
MAX_MISSES = 5              

device = "cuda" if torch.cuda.is_available() else "cpu"
model = RFDETR.from_checkpoint(MODEL_PATH)
if hasattr(model, "optimize_for_inference"):
    try:
        model.optimize_for_inference()
    except Exception as e:
        print(f"optimize_for_inference skipped: {e}")
else:
    print("optimize_for_inference not available in this rfdetr version, continuing without it")

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_W)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_H)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  


def best_person(dets):
    """Highest-confidence person as (x1, y1, x2, y2, conf), or None."""
    if dets is None or len(dets.xyxy) == 0:
        return None
    keep = dets.class_id == PERSON_CLASS
    if not keep.any():
        return None
    boxes, confs = dets.xyxy[keep], dets.confidence[keep]
    i = int(np.argmax(confs))
    return (*boxes[i], float(confs[i]))


def make_roi(box, fw, fh):
    """ROI centered on the person; grows if the person is bigger than 385; clamped to frame."""
    x1, y1, x2, y2 = box
    w = min(max(ROI_W, int(x2 - x1)), fw)
    h = min(max(ROI_H, int(y2 - y1)), fh)
    cx, cy = (x1 + x2) / 2, (y1 + y2) / 2
    rx = int(np.clip(cx - w / 2, 0, fw - w))
    ry = int(np.clip(cy - h / 2, 0, fh - h))
    return rx, ry, w, h


roi = None
misses = 0

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break
    fh, fw = frame.shape[:2]

    # Detect on the ROI crop if we have one, otherwise on the full frame
    if roi is None:
        ox, oy = 0, 0
        inp = frame
    else:
        rx, ry, rw, rh = roi
        ox, oy = rx, ry
        inp = frame[ry:ry + rh, rx:rx + rw]      # always crop from the original frame

    dets = model.predict(cv2.cvtColor(inp, cv2.COLOR_BGR2RGB), threshold=THRESH)
    person = best_person(dets)

    if person is not None:
        x1, y1, x2, y2, conf = person
        # crop coords -> full-frame coords
        x1, x2 = x1 + ox, x2 + ox
        y1, y2 = y1 + oy, y2 + oy
        x1, y1 = max(0, x1), max(0, y1)
        x2, y2 = min(fw, x2), min(fh, y2)

        roi = make_roi((x1, y1, x2, y2), fw, fh)   # re-center ROI on the new box
        misses = 0

        print(f"BBox (full frame): [{int(x1)}, {int(y1)}, {int(x2)}, {int(y2)}] | ROI: {roi}")
        cv2.rectangle(frame, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 2)
        cv2.putText(frame, f"Person: {conf:.2f}", (int(x1), max(int(y1) - 10, 15)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
    else:
        misses += 1
        if misses >= MAX_MISSES:
            roi = None     # lost the person, search the full frame again

    # Draw the ROI window in blue
    if roi is not None:
        rx, ry, rw, rh = roi
        cv2.rectangle(frame, (rx, ry), (rx + rw, ry + rh), (255, 0, 0), 1)
        cv2.imshow("ROI crop", frame[ry:ry + rh, rx:rx + rw])

    cv2.imshow("RF-DETR Live Feed", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()