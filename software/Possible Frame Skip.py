import cv2
from ultralytics import YOLO

#model = YOLO("best.pt")
model = YOLO("/Users/gxh/ACM Robotics Fa 26/76percent.pt")
cap = cv2.VideoCapture(0)

frame_no = 0

while cap.isOpened():
    #success, frame = cap.read()

    skip_rate = 0.25 # 0.25 is the best number by far
    
    success = cap.grab()
    if not success:
        break



    if (frame_no % skip_rate == 0):
        ret, frame = cap.retrieve()

        if not ret:
            break

        # Run tracking/inference and get the annotated frame directly
        results = model.track(frame, persist=True, show=False)
        annotated_frame = results[0].plot()

    # Run tracking/inference and get the annotated frame directly
    # results = model.track(frame, persist=True, show=False)
    # annotated_frame = results[0].plot()

    cv2.imshow("YOLO Camera Test", annotated_frame)

    frame_no += 5

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()