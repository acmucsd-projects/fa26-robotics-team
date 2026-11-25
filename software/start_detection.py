import queue
import cv2

from global_state import Global
from input_process import start_input_thread
# from processing import start_process_thread
from processing_with_tracking import start_process_thread
from output_process import start_output_thread
from video_recorder import VideoRecorder

class Main():
    def __init__(self):
        self.global_state = Global()
        self.global_state.running = True
        self.process_thread = start_process_thread(self.global_state)
        # self.output_thread = start_output_thread(self.global_state)
        self.input_thread = start_input_thread(self.global_state)

    def run(self):
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
            # self.output_thread.join()
            cv2.destroyAllWindows()
        print(f"Output queue size: {self.global_state.data_queue.qsize()}")
        
def main():
    global_state = Global()
    global_state.running = True
    recorder = VideoRecorder()

    process_thread = start_process_thread(global_state)
    # output_thread = start_output_thread(global_state)
    input_thread = start_input_thread(global_state, recorder)

    win_name = "RF-DETR Live Feed"
    cv2.namedWindow(win_name, cv2.WINDOW_NORMAL)  # Allows resizing and maximizing
    is_fullscreen = False

    def toggle_fullscreen(event, x, y, flags, param):
        global is_fullscreen
        # Double-click or middle-click to toggle fullscreen
        if event == cv2.EVENT_LBUTTONDBLCLK or event == cv2.EVENT_MBUTTONDOWN:
            is_fullscreen = not is_fullscreen
            prop = cv2.WINDOW_FULLSCREEN if is_fullscreen else cv2.WINDOW_NORMAL
            cv2.setWindowProperty(win_name, cv2.WND_PROP_FULLSCREEN, prop)

    cv2.setMouseCallback(win_name, toggle_fullscreen)


    try:
        while (
            global_state.running
            or input_thread.is_alive()
            or process_thread.is_alive()
            or not global_state.output_stack.empty()
        ):
            try:
                # Get frame with RF-DETR boxes already drawn
                frame_number, frame_rgb = (
                    global_state.output_stack.get(timeout=0.01)
                )

                global_state.output_stack.task_done()

                # Convert RGB to BGR for OpenCV
                frame_bgr = cv2.cvtColor(
                    frame_rgb,
                    cv2.COLOR_RGB2BGR
                )

                cv2.imshow(win_name, frame_bgr)

            except queue.Empty:
                pass

            # Handle keyboard input on every iteration
            # Optional keyboard toggle with 'f'

            key = cv2.waitKey(1) & 0xFF

            if key == ord("r"):
                recorder.start()
            elif key == ord("p"):
                recorder.toggle_pause()
            elif key == ord("s"):
                recorder.stop()
            elif key == ord("q"):
                global_state.running = False
                break

            if cv2.waitKey(1) & 0xFF == ord("f"):
                is_fullscreen = not is_fullscreen
                prop = cv2.WINDOW_FULLSCREEN if is_fullscreen else cv2.WINDOW_NORMAL
                cv2.setWindowProperty(win_name, cv2.WND_PROP_FULLSCREEN, prop)
            # if cv2.waitKey(1) & 0xFF == ord("q"):
            #     global_state.running = False
            #     break

    finally:
        global_state.running = False
        recorder.stop()


        input_thread.join()
        process_thread.join()
        # output_thread.join()

        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

