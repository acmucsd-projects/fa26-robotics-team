import threading
import time

import cv2
import uvicorn
from fastapi import FastAPI
from fastapi.responses import StreamingResponse

app = FastAPI(title="Gimbal Tracker")

_state = None
_latest_jpeg = None
_jpeg_lock = threading.Lock()
_viewers = 0  # only encode JPEGs while someone is watching


def publish_frame(frame):
    """Called from the processing thread with the annotated frame."""
    global _latest_jpeg
    if _viewers == 0:
        return
    ok, buf = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 70])
    if ok:
        with _jpeg_lock:
            _latest_jpeg = buf.tobytes()


def _mjpeg():
    global _viewers
    _viewers += 1
    try:
        last = None
        while _state is not None and _state.running:
            with _jpeg_lock:
                jpeg = _latest_jpeg
            if jpeg is None or jpeg is last:
                time.sleep(0.01)
                continue
            last = jpeg
            yield (
                b"--frame\r\nContent-Type: image/jpeg\r\n\r\n"
                + jpeg + b"\r\n"
            )
    finally:
        _viewers -= 1


@app.get("/status")
def status():
    c = _state.center_point
    return {
        "running": _state.running,
        "locked": c is not None,
        "center": None if c is None else {"x": c[0], "y": c[1]},
    }


@app.get("/video")
def video():
    return StreamingResponse(
        _mjpeg(),
        media_type="multipart/x-mixed-replace; boundary=frame",
    )


@app.post("/reset")
def reset():
    _state.reset_requested = True
    return {"ok": True}


@app.post("/stop")
def stop():
    _state.running = False
    return {"ok": True}


def start_server(global_state, host="0.0.0.0", port=8000):
    global _state
    _state = global_state

    config = uvicorn.Config(app, host=host, port=port, log_level="warning")
    server = uvicorn.Server(config)

    thread = threading.Thread(target=server.run, daemon=True)
    thread.start()
    return server  # set server.should_exit = True to shut down