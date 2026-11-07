import queue

def put_latest(target_queue, item):
    try:
        target_queue.put_nowait(item)
    except queue.Full:
        try:
            target_queue.get_nowait()
        except queue.Empty:
            pass
        target_queue.put_nowait(item)


class Global:
    def __init__(self):
        self.input_queue = queue.Queue(maxsize=1)
        self.output_queue = queue.Queue(maxsize=1)
        self.data_queue = queue.Queue(maxsize=1)
        self.center_point = None
        self.hand_position = None
        self.running = False
