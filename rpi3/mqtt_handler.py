import json
import os

import paho.mqtt.client as mqtt

from db import init_db, insert_event_log, insert_gate_log, insert_parking_log


MQTT_HOST = os.environ.get("MQTT_HOST", "localhost")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
MQTT_TOPIC = "parking/#"


def handle_message(topic, payload):
    if topic == "parking/rpi1/status":
        insert_parking_log(parking_state=payload)
        return

    if topic == "parking/rpi1/risk":
        insert_parking_log(risk_state=payload)
        return

    if topic == "parking/rpi1/distance":
        try:
            distances = json.loads(payload)
        except json.JSONDecodeError:
            insert_event_log(topic, "invalid_distance_payload", payload)
            return

        insert_parking_log(
            front_distance=distances.get("front"),
            rear_distance=distances.get("rear"),
            side_distance=distances.get("side"),
        )
        return

    if topic == "parking/rpi2/gate":
        insert_gate_log(payload)
        return

    if topic in ("parking/rpi1/event", "parking/rpi2/event"):
        insert_event_log(topic, payload, payload)
        return

    insert_event_log(topic, "unknown_topic", payload)


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe(MQTT_TOPIC)
        print(f"Subscribed to {MQTT_TOPIC}")
    else:
        print(f"MQTT connection failed: {rc}")


def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8", errors="replace")
    print(msg.topic, payload)
    handle_message(msg.topic, payload)


def main():
    init_db()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_HOST, MQTT_PORT)
    client.loop_forever()


if __name__ == "__main__":
    main()
