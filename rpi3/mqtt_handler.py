import json
import os

import paho.mqtt.client as mqtt

from db import (
    init_db,
    insert_event_log,
    insert_gate_log,
    insert_lot_log,
    insert_slot_log,
)


MQTT_HOST = os.environ.get("MQTT_HOST", "localhost")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
MQTT_TOPIC = "parking/#"


def handle_slot_message(topic, payload):
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        insert_event_log(
            topic=topic,
            event_type="invalid_slot_payload",
            reason="json_decode_error",
            payload=payload,
        )
        return

    insert_slot_log(
        slot1_state=data.get("slot1"),
        slot2_state=data.get("slot2"),
    )


def handle_distance_message(topic, payload):
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        insert_event_log(
            topic=topic,
            event_type="invalid_distance_payload",
            reason="json_decode_error",
            payload=payload,
        )
        return

    insert_slot_log(
        slot1_distance=data.get("slot1"),
        slot2_distance=data.get("slot2"),
    )


def handle_event_message(topic, payload):
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        insert_event_log(
            topic=topic,
            event_type="invalid_event_payload",
            reason="json_decode_error",
            payload=payload,
        )
        return

    insert_event_log(
        topic=topic,
        event_type=data.get("event"),
        reason=data.get("reason"),
        payload=payload,
    )


def handle_message(topic, payload):
    if topic == "parking/rpi1/slot":
        handle_slot_message(topic, payload)
        return

    if topic == "parking/rpi1/distance":
        handle_distance_message(topic, payload)
        return

    if topic == "parking/rpi1/lot":
        insert_lot_log(payload)
        return

    if topic == "parking/rpi2/gate":
        insert_gate_log(payload)
        return

    if topic == "parking/rpi2/event":
        handle_event_message(topic, payload)
        return

    insert_event_log(
        topic=topic,
        event_type="unknown_topic",
        reason="unsupported_topic",
        payload=payload,
    )


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