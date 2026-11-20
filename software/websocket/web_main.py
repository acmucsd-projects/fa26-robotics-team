import queue
import sys
import threading
from pathlib import Path

import cv2
import uvicorn

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from software.global_state import Global
from software.websocket.web_input_process import start_input_thread
from software.websocket.phone_server import app, ensure_self_signed_cert, get_local_ip
from software.processing_with_tracking import start_process_thread


def main():
    port = 5050
    cert_file = "cert.pem"
    key_file = "key.pem"
    window_name = "RF-DETR Live Feed"

    ensure_self_signed_cert(cert_file, key_file)

    server = uvicorn.Server(
        uvicorn.Config(
            app,
            host="0.0.0.0",
            port=port,
            ssl_certfile=cert_file,
            ssl_keyfile=key_file,
        )
    )
    server_thread = threading.Thread(
        target=server.run,
        name="phone-camera-server",
    )

    state = Global()
    state.running = True
    input_thread = start_input_thread(state)
    process_thread = start_process_thread(state)

    preview_open = False
    try:
        # cv2.namedWindow(window_name)
        server_thread.start()
        print(f"Open on your iPhone: https://{get_local_ip()}:{port}")

        while state.running and server_thread.is_alive():
            try:
                _, frame_rgb = state.output_stack.get(timeout=0.01)
            except queue.Empty:
                pass
            else:
                state.output_stack.task_done()
                frame_bgr = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
                cv2.imshow(window_name, frame_bgr)
                preview_open = True

            if preview_open and cv2.waitKey(1) & 0xFF == ord("q"):
                break

    except KeyboardInterrupt:
        pass
    finally:
        state.running = False
        server.should_exit = True
        if server_thread.ident is not None:
            server_thread.join()
        input_thread.join()
        process_thread.join()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()