# pip install paho-mqtt
import paho.mqtt.client as mqtt

BROKER = "broker.emqx.io"
PORT = 1883
TOPIC = "gimbal/commands"

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected!")
        client.subscribe(TOPIC)
    else:
        print(f"Failed to connect, reason code: {reason_code}")


def on_message(client, userdata, msg):
    print(msg.payload.decode())

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, keepalive=60)
client.loop_start()