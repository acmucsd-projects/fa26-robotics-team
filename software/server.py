
from flask import Flask, Response, request, render_template_string
import cv2
import numpy as np
import threading
import time

app = Flask(__name__)

latest_frame = None
frame_lock = threading.Lock()

HTML = """
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport"
          content="width=device-width, initial-scale=1">
    <title>iPhone Camera Stream</title>
    <style>
        body {
            background: #111;
            color: white;
            text-align: center;
            font-family: Arial;
        }
        video {
            width: 100%;
            max-width: 640px;
            border-radius: 10px;
        }
        button {
            padding: 15px 25px;
            font-size: 18px;
            margin: 15px;
        }
    </style>
</head>
<body>
    <h2>iPhone Camera Stream</h2>
    <video id="camera" autoplay muted playsinline></video>
    <br>
    <button onclick="startCamera()">Start Camera</button>
    <button onclick="stopCamera()">Stop Camera</button>
    <p id="status">Camera stopped</p>

    <canvas id="canvas" style="display:none"></canvas>

    <script>
        const video = document.getElementById("camera");
        const canvas = document.getElementById("canvas");
        const status = document.getElementById("status");

        let stream = null;
        let running = false;

        async function startCamera() {
            if (running) return;

            try {
                stream = await navigator.mediaDevices.getUserMedia({
                    audio: false,
                    video: {
                        facingMode: "environment",
                        width: { ideal: 640 },
                        height: { ideal: 480 },
                        frameRate: { ideal: 15 }
                    }
                });

                video.srcObject = stream;
                await video.play();

                canvas.width = 640;
                canvas.height = 480;

                running = true;
                status.innerText = "Streaming...";
                sendFrames();

            } catch (err) {
                status.innerText = "Camera error: " + err.message;
            }
        }

        function stopCamera() {
            running = false;

            if (stream) {
                stream.getTracks().forEach(track => track.stop());
                stream = null;
            }

            video.srcObject = null;
            status.innerText = "Camera stopped";
        }

        async function sendFrames() {
            const ctx = canvas.getContext("2d");

            while (running) {
                const start = performance.now();

                if (video.readyState >= 2) {
                    ctx.drawImage(video, 0, 0, 640, 480);

                    const blob = await new Promise(resolve =>
                        canvas.toBlob(resolve, "image/jpeg", 0.65)
                    );

                    if (!running) break;

                    if (blob) {
                        try {
                            const response = await fetch("/upload", {
                                method: "POST",
                                headers: {
                                    "Content-Type": "image/jpeg"
                                },
                                body: blob
                            });

                            if (!response.ok) {
                                throw new Error("Upload failed");
                            }
                        } catch (err) {
                            status.innerText = "Upload error: " + err.message;
                            stopCamera();
                            break;
                        }
                    }
                }

                // Target 15 FPS, without overlapping uploads.
                const elapsed = performance.now() - start;
                await new Promise(resolve =>
                    setTimeout(resolve, Math.max(0, 67 - elapsed))
                );
            }
        }

        document.addEventListener("visibilitychange", () => {
            if (document.hidden && running) {
                stopCamera();
            }
        });
    </script>
</body>
</html>
"""


@app.route("/")
def index():
    return render_template_string(HTML)


@app.route("/upload", methods=["POST"])
def upload():
    global latest_frame

    image_data = np.frombuffer(request.data, dtype=np.uint8)
    frame = cv2.imdecode(image_data, cv2.IMREAD_COLOR)

    if frame is None:
        return "Invalid frame", 400

    with frame_lock:
        latest_frame = frame

    return "OK", 200


def generate_frames():
    while True:
        with frame_lock:
            frame = None if latest_frame is None else latest_frame.copy()

        if frame is None:
            time.sleep(0.05)
            continue

        success, buffer = cv2.imencode(
            ".jpg", frame,
            [cv2.IMWRITE_JPEG_QUALITY, 75]
        )

        if not success:
            continue

        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n"
            + buffer.tobytes()
            + b"\r\n"
        )

        time.sleep(1 / 15)


@app.route("/video")
def video():
    return Response(
        generate_frames(),
        mimetype="multipart/x-mixed-replace; boundary=frame"
    )


if __name__ == "__main__":
    print("Starting iPhone camera server...")
    print("Local OpenCV URL: http://127.0.0.1:5000/video")

    app.run(
        host="0.0.0.0",
        port=5000,
        threaded=True
    )
