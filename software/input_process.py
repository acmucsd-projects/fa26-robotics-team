import cv2
import threading
import queue
from global_state import Global

jpeg_queue = queue.Queue(maxsize=1)


def put_latest(target_queue, item):
    try:
        target_queue.put_nowait(item)
    except queue.Full:
        # Discard the waiting item if the consumer hasn't taken it yet.
        try:
            target_queue.get_nowait()
        except queue.Empty:
            pass
        else:
            target_queue.task_done()

        target_queue.put_nowait(item)

def threadTarget(global_state: Global):
    """
    Detect a hand in the top third of the frame to initialize
    the gimbal target, then stream frames to the processing queue.
    """

    frame_number = 0

   
    global_state.running = True

    # cap = cv2.VideoCapture("https://localhost:5050/stream")
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    print(f"FPS: {cap.get(cv2.CAP_PROP_FPS)}")

    try:
        while cap.isOpened() and global_state.running:
            ret, frame = cap.read()

            if not ret:
                break

            # global_state.preview_stack.put((frame_number, frame.copy()))

            if frame_number % 1 == 0:
                frame_rgb = cv2.cvtColor(
                    frame,
                    cv2.COLOR_BGR2RGB
                )

                global_state.input_stack.put((frame_number, frame_rgb))
                    
                

            frame_number += 1

    finally:
        cap.release()
        cv2.destroyAllWindows()
        global_state.running = False


def start_input_thread(global_state: Global):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        daemon=True,
    )

    thread.start()
    return thread

