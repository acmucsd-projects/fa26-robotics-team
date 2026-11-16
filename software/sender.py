import paho.mqtt.client as mqtt
import time

BROKER = "broker.emqx.io"
PORT = 1883
TOPIC = "gimbal/commands"

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code != 0:
        print(f"Failed to connect, reason code: {reason_code}")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect

client.connect(BROKER, PORT, keepalive=60)
client.loop_start()

message = "Arnav"
while True:
    info = client.publish(TOPIC, message, qos=1)
    info.wait_for_publish()
    print(f"Sent {message}")
    time.sleep(1)

client.loop_stop()
client.disconnect()