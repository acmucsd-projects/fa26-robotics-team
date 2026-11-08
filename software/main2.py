import queue

import cv2

from global_state import Global
from input_process import start_input_thread
from processing import start_process_thread
from output_process import start_output_thread

class Main():
    def __init__(self):
        self.global_state = Global()
        self.global_state.running = True
        self.process_thread = start_process_thread(self.global_state)
        self.output_thread = start_output_thread(self.global_state)
        self.input_thread = start_input_thread(self.global_state)

    def run(self):
        latest_boxes = []
        latest_detection_frame = -1
        last_preview_frame = -1

        try:
            while (
                self.input_thread.is_alive()
                or self.process_thread.is_alive()
                or not self.global_state.output_queue.empty()
            ):
                try:
                    _, frame_rgb = self.global_state.output_queue.get(timeout=0.01)
                    frame_bgr = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
                    cv2.imshow("RF-DETR Live Feed", frame_bgr)
                except queue.Empty:
                    pass

                if cv2.waitKey(1) & 0xFF == ord("q"):
                    self.global_state.running = False
        finally:
            self.global_state.running = False
            self.input_thread.join()
            self.process_thread.join()
            self.output_thread.join()
            cv2.destroyAllWindows()
        print(f"Output queue size: {self.global_state.data_queue.qsize()}")
        
def main():
    latest_boxes = []
    latest_detection_frame = -1
    last_preview_frame = -1
    global_state = Global()
    global_state.running = True
    process_thread = start_process_thread(global_state)
    output_thread = start_output_thread(global_state)
    input_thread = start_input_thread(global_state)

    try:
        while global_state.running:

            # Get newest detections without blocking
            try:
                while True:
                    frame_num, boxes = (
                        global_state.output_stack.get_nowait()
                    )

                    if frame_num > latest_detection_frame:
                        latest_boxes = boxes
                        latest_detection_frame = frame_num

                    global_state.output_stack.task_done()

            except queue.Empty:
                pass

            # Get newest camera frame
            try:
                frame_number, frame = (
                    global_state.preview_stack.get(timeout=0.01)
                )
                
                frame_rgb = cv2.cvtColor(
                    frame,
                    cv2.COLOR_BGR2RGB
                )

                global_state.preview_stack.task_done()

                # Don't redraw the same camera frame repeatedly
                if frame_number != last_preview_frame:
                    last_preview_frame = frame_number

                    # Overlay latest RF-DETR boxes
                    for box in latest_boxes:
                        x1, y1, x2, y2, conf, class_id = box

                        cv2.rectangle(
                            frame_rgb,
                            (x1, y1),
                            (x2, y2),
                            (0, 255, 0),
                            2
                        )

                        cv2.putText(
                            frame_rgb,
                            f"Class {class_id}: {conf:.2f}",
                            (x1, max(y1 - 10, 15)),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.5,
                            (0, 255, 0),
                            2
                        )

                    cv2.imshow("RF-DETR Live Feed", frame_rgb)

            except queue.Empty:
                pass

            if cv2.waitKey(1) & 0xFF == ord("q"):
                global_state.running = False

    finally:
        global_state.running = False
        input_thread.join()
        process_thread.join()
        output_thread.join()
        cv2.destroyAllWindows()
    print(f"Output queue size: {global_state.data_queue.qsize()}")


if __name__ == "__main__":
    main()
