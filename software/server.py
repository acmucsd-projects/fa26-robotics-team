import asyncio
from datetime import datetime, timedelta, timezone
import os
import socket

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
from fastapi import FastAPI, Request, Response
from fastapi.responses import HTMLResponse, StreamingResponse
import uvicorn

app = FastAPI()

latest_frame: bytes | None = None
frame_condition: asyncio.Condition | None = None


def get_condition() -> asyncio.Condition:
  global frame_condition
  if frame_condition is None:
    frame_condition = asyncio.Condition()
  return frame_condition


PHONE_HTML = """
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Phone Camera Streamer</title>
  <style>
    body { font-family: sans-serif; background: #111; color: #fff; text-align: center; margin: 0; padding: 20px; }
    video { width: 100%; max-width: 480px; border-radius: 12px; background: #000; }
    #status { margin-top: 12px; color: #4ade80; font-weight: bold; }
  </style>
</head>
<body>
  <h3>Phone Camera</h3>
  <video id="video" autoplay playsinline muted></video>
  <div id="status">Requesting camera access...</div>

  <script>
    const video = document.getElementById('video');
    const status = document.getElementById('status');
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');

    async function start() {
      try {
        const stream = await navigator.mediaDevices.getUserMedia({
          video: { facingMode: 'environment', width: { ideal: 640 }, height: { ideal: 480 } },
          audio: false
        });
        video.srcObject = stream;
        status.textContent = 'Streaming live to laptop...';
        sendFrame();
      } catch (err) {
        status.style.color = '#f87171';
        status.textContent = 'Error: ' + err.message;
      }
    }

    function sendFrame() {
      if (video.readyState === video.HAVE_ENOUGH_DATA) {
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;
        ctx.drawImage(video, 0, 0);
        canvas.toBlob(async (blob) => {
          if (blob) {
            try {
              await fetch('/upload', { method: 'POST', body: blob });
            } catch (e) {}
          }
          requestAnimationFrame(sendFrame);
        }, 'image/jpeg', 0.6);
      } else {
        requestAnimationFrame(sendFrame);
      }
    }

    start();
  </script>
</body>
</html>
"""


@app.get("/", response_class=HTMLResponse)
async def index():
  return PHONE_HTML


@app.post("/upload")
async def upload(request: Request):
  global latest_frame
  data = await request.body()
  cond = get_condition()
  async with cond:
    latest_frame = data
    cond.notify_all()
  return Response(status_code=204)


async def generate_mjpeg():
  cond = get_condition()
  while True:
    async with cond:
      await cond.wait()
      frame = latest_frame
    if frame:
      yield (
          b"--frame\r\n" b"Content-Type: image/jpeg\r\n\r\n" + frame + b"\r\n"
      )


@app.get("/stream")
async def stream():
  return StreamingResponse(
      generate_mjpeg(), media_type="multipart/x-mixed-replace; boundary=frame"
  )


def ensure_self_signed_cert(cert_file="cert.pem", key_file="key.pem"):
  if os.path.exists(cert_file) and os.path.exists(key_file):
    return
  key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
  subject = issuer = x509.Name(
      [x509.NameAttribute(NameOID.COMMON_NAME, "localhost")]
  )
  now = datetime.now(timezone.utc)
  cert = (
      x509.CertificateBuilder()
      .subject_name(subject)
      .issuer_name(issuer)
      .public_key(key.public_key())
      .serial_number(x509.random_serial_number())
      .not_valid_before(now)
      .not_valid_after(now + timedelta(days=365))
      .sign(key, hashes.SHA256())
  )
  with open(key_file, "wb") as f:
    f.write(
        key.private_bytes(
            serialization.Encoding.PEM,
            serialization.PrivateFormat.TraditionalOpenSSL,
            serialization.NoEncryption(),
        )
    )
  with open(cert_file, "wb") as f:
    f.write(cert.public_bytes(serialization.Encoding.PEM))


def get_local_ip():
  s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  try:
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]
  except Exception:
    return "127.0.0.1"
  finally:
    s.close()


if __name__ == "__main__":
  ensure_self_signed_cert()
  ip = get_local_ip()
  port = 5050
  print(f"\n1. Open on your PHONE:  https://{ip}:{port}")
  print(f"2. Open on your LAPTOP: https://localhost:{port}/stream\n")
  uvicorn.run(
      app, host="0.0.0.0", port=port, ssl_keyfile="key.pem", ssl_certfile="cert.pem"
  )