import queue

import cv2

from global_state import Global
from input_process import start_input_thread
from processing import start_process_thread
from output_process import start_output_thread


def main():
    global_state = Global()
    global_state.running = True
    process_thread = start_process_thread(global_state)
    output_thread = start_output_thread(global_state)
    input_thread = start_input_thread(global_state)

    try:
        while (
            input_thread.is_alive()
            or process_thread.is_alive()
            or not global_state.output_queue.empty()
        ):
            try:
                _, frame_rgb = global_state.output_queue.get(timeout=0.01)
                frame_bgr = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
                cv2.imshow("RF-DETR Live Feed", frame_bgr)
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
