import queue
import threading

import cv2
import numpy as np


# phone_server.py imports this queue and adds incoming JPEGs to it.
jpeg_queue = queue.Queue(maxsize=1)


def put_latest(target_queue, item):
    """Put an item in the queue, replacing the waiting item if full."""
    try:
        target_queue.put_nowait(item)
    except queue.Full:
        try:
            target_queue.get_nowait()
        except queue.Empty:
            # The consumer already took the waiting item.
            pass
        else:
            target_queue.task_done()

        target_queue.put_nowait(item)


def input_worker(global_state):
    while global_state.running:
        try:
            frame_number, received_at, jpeg_bytes = jpeg_queue.get(timeout=0.1)
        except queue.Empty:
            continue

        try:
            jpeg_array = np.frombuffer(jpeg_bytes, dtype=np.uint8)
            frame_bgr = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)

            if frame_bgr is None:
                continue

            frame_rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
        except cv2.error:
            continue
        finally:
            jpeg_queue.task_done()

        # The RF-DETR processing thread reads this tuple from input_stack.
        
        global_state.input_stack.put(
        (frame_number, received_at, frame_rgb))


def start_input_thread(global_state):
    thread = threading.Thread(
        target=input_worker,
        args=(global_state,),
        name="phone-camera-input",
    )
    thread.start()
    return thread