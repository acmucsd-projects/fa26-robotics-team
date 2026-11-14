from controller import Robot
import cv2
import numpy as np
import sys
from gimbal_global_state import Global
from gimbal_processing import start_process_thread
from gimbal_input_process import start_input_thread
import queue

global_state = Global()
global_state.running = True

process_thread = start_process_thread(global_state)
# output_thread = start_output_thread(global_state)
input_thread = start_input_thread(global_state)

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

            cv2.imshow("RF-DETR Live Feed", frame_bgr)

        except queue.Empty:
            pass

        # Handle keyboard input on every iteration
        if cv2.waitKey(1) & 0xFF == ord("q"):
            global_state.running = False
            break

finally:
    global_state.running = False

    input_thread.join()
    process_thread.join()
    # output_thread.join()

    cv2.destroyAllWindows()
