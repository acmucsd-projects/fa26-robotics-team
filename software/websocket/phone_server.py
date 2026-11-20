import ipaddress
import socket
import time
from datetime import datetime, timedelta, timezone
from itertools import count
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse

from software.websocket.web_input_process import jpeg_queue, put_latest


app = FastAPI()
frame_numbers = count(1)

PHONE_HTML = """
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Phone Camera</title>
</head>
<body>
  <video id="video" autoplay playsinline muted style="width: 100%; max-width: 480px"></video>
  <p id="status">Requesting camera access…</p>

  <script>
    const video = document.getElementById("video");
    const status = document.getElementById("status");
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");
    let ws;

    function connect() {
      ws = new WebSocket(`wss://${location.host}/ws`);
      ws.onopen = () => { status.textContent = "Streaming"; };
      ws.onclose = () => {
        status.textContent = "Reconnecting…";
        setTimeout(connect, 500);
      };
    }

    function scheduleFrame() {
      if ("requestVideoFrameCallback" in video) {
        video.requestVideoFrameCallback(sendFrame);
      } else {
        requestAnimationFrame(sendFrame);
      }
    }

    function sendFrame() {
      const connection = ws;
      if (connection?.readyState !== WebSocket.OPEN ||
          connection.bufferedAmount !== 0) {
        scheduleFrame();
        return;
      }

      ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
      canvas.toBlob((jpeg) => {
        if (jpeg && connection.readyState === WebSocket.OPEN) {
          connection.send(jpeg);
        }
        scheduleFrame();
      }, "image/jpeg", 0.6);
    }

    async function start() {
      try {
        video.srcObject = await navigator.mediaDevices.getUserMedia({
          video: {
            facingMode: "environment",
            width: { ideal: 640 },
            height: { ideal: 480 },
            frameRate: { ideal: 30 }
          },
          audio: false
        });

        await video.play();
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;
        connect();
        scheduleFrame();
      } catch (error) {
        status.textContent = error.message;
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


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()

    try:
        while True:
            jpeg_bytes = await websocket.receive_bytes()
            if jpeg_bytes:
                # This is where a phone frame enters jpeg_queue.
                put_latest(
                    jpeg_queue,
                    (next(frame_numbers), time.monotonic(), jpeg_bytes),
                )
    except WebSocketDisconnect:
        pass


def get_local_ip():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        try:
            sock.connect(("8.8.8.8", 80))
            return sock.getsockname()[0]
        except OSError:
            return "127.0.0.1"


def ensure_self_signed_cert(cert_file="cert.pem", key_file="key.pem"):
    cert_path = Path(cert_file)
    key_path = Path(key_file)

    if cert_path.exists() and key_path.exists():
        return

    ip = get_local_ip()
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    subject = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, ip)])
    now = datetime.now(timezone.utc)

    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(subject)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now)
        .not_valid_after(now + timedelta(days=365))
        .add_extension(
            x509.SubjectAlternativeName([
                x509.IPAddress(ipaddress.ip_address(ip)),
                x509.DNSName("localhost"),
            ]),
            critical=False,
        )
        .add_extension(
            x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),
            critical=False,
        )
        .sign(key, hashes.SHA256())
    )

    key_path.write_bytes(
        key.private_bytes(
            serialization.Encoding.PEM,
            serialization.PrivateFormat.PKCS8,
            serialization.NoEncryption(),
        )
    )
    cert_path.write_bytes(cert.public_bytes(serialization.Encoding.PEM))