import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
from database import add_light_status


# BROKER = "localhost"  # Change if your broker is remote
BROKER = "172.20.10.3"
PORT = 1883

PIR_TOPIC = "building/rooms/A/PIR/+"
LED_TOPIC = "building/rooms/A/LED/+"

device_id_mapping = {
    "A": "LightNode1",
    "B": "LightNode2",
    "C": "LightNode3",
    "D": "LightNode4",
}


def on_connect(client, userdata, flags, rc, properties):
    if rc == 0:
        print("✅ Connected to broker")

        client.subscribe(PIR_TOPIC)
        print(f"📡 Subscribed to topic: {PIR_TOPIC}")

        client.subscribe(LED_TOPIC)
        print(f"📡 Subscribed to topic: {LED_TOPIC}")
    else:
        print(f"❌ Connection failed with code {rc}")


def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = msg.payload.decode()
        total_packet_size = estimate_packet_size(msg)

        print(f"Topic: [{topic}] Payload: {payload}")
        print(f"Packet Size: {total_packet_size}")

        topic_level = topic.split("/")[-2]
        print(f"Topic Level: {topic_level}")

        if topic_level == "LED":
            handle_led(topic, payload)
        elif topic_level == "PIR":
            handle_pir(topic, payload)
        else:
            print(f"Unexpected Topic Level: {topic_level}")

    except Exception as e:
        print("❌ Error processing message:", e)


def estimate_packet_size(msg):
    topic_length = len(msg.topic.encode())  # MQTT encodes topics as UTF-8
    payload_length = len(msg.payload)

    fixed_header = 2  # Typically 2 bytes for small messages
    topic_header = 2 + topic_length  # 2 bytes for topic length field
    packet_id = 2 if msg.qos > 0 else 0

    return fixed_header + topic_header + packet_id + payload_length


def handle_led(topic, payload):
    led_id = topic.split("/")[-1]
    device_id = device_id_mapping[led_id]

    device_state = int(payload)  # Parse from message

    add_light_status(device_id, device_state, "MQTT")


def handle_pir(topic, message):
    print("Handling PIR", topic, message)


client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(username="server", password="password")
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_forever()
