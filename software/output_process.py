import queue
import cv2
import threading
import os
from global_state import Global


def threadTarget(global_state):
    """
    This function is the target for the thread. It processes input data in a separate thread.
    """

    while global_state.running or not global_state.data_stack.empty():
        try:
            frame_number, detections = global_state.data_stack.get(timeout=0.1)
        except queue.Empty:
            continue
        

        



def start_output_thread(global_state):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        # daemon=True,
    )
    thread.start()
    return thread