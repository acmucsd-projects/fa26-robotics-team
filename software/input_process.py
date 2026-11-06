import cv2
import threading

from global_state import Global, put_latest


def threadTarget(global_state: Global):
    """
    This function is the target for the thread. It processes input data in a separate thread.
    """
    global_state.running = True
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    print(f"fps: {cap.get(cv2.CAP_PROP_FPS)}")

    frame_number = 0
    while cap.isOpened() and global_state.running:
        ret, frame = cap.read()
        if not ret:
            break

        if frame_number % 2 == 0:
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            put_latest(global_state.input_queue, (frame_number, frame_rgb))

        frame_number += 1

    cap.release()
    global_state.running = False


def start_input_thread(global_state: Global) -> threading.Thread:
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        daemon=True,
    )
    thread.start()
    return thread