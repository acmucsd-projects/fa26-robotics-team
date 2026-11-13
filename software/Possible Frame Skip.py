import cv2
from ultralytics import YOLO

#model = YOLO("best.pt")
model = YOLO("/Users/gxh/ACM Robotics Fa 26/81percent_grace.pt")
cap = cv2.VideoCapture(0)

frame_no = 0

while cap.isOpened():
    #success, frame = cap.read()

    skip_rate = 80 # 0.25 is the best number by far
    
    success = cap.grab()
    if not success:
        break



    if (frame_no % skip_rate == 0):
        ret, frame = cap.retrieve()

        if not ret:
            break

        # Run tracking/inference and get the annotated frame directly
        results = model.track(frame, persist=True, show=False, conf = 0.2)

        # Find center point
        box = results[0].boxes # get the box

        xyxy = box.xyxy # get coordinates

        # Get middle coordinates
        if (len(box) > 0) :
            x_center, y_center, w, h = box.xywh[0].tolist()

            center_point = (round(x_center), round(y_center))

            annotated_frame = results[0][:1].plot()
            cv2.circle(annotated_frame, center = center_point, radius = 5, color = (0, 0, 255), thickness = -1)


    # Run tracking/inference and get the annotated frame directly
    # results = model.track(frame, persist=True, show=False)
    # annotated_frame = results[0].plot()

    cv2.imshow("YOLO Camera Test", annotated_frame)

    frame_no += 80

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()