import json
import os
from pathlib import Path

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
STATUS_DEVICE = Path(os.environ.get("RPI3_STATUS_DEVICE", "/dev/rpi3_status"))


def update_kernel_status(message):
    try:
        STATUS_DEVICE.write_text(f"{message}\n", encoding="utf-8")
    except OSError:
        pass


def handle_slot_message(topic, payload):
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        update_kernel_status(f"ERROR invalid_slot_payload {topic}")
        insert_event_log(
            topic=topic,
            event_type="invalid_slot_payload",
            reason="json_decode_error",
            payload=payload,
        )
        return False

    insert_slot_log(
        slot1_state=data.get("slot1"),
        slot2_state=data.get("slot2"),
    )
    return True


def handle_distance_message(topic, payload):
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        update_kernel_status(f"ERROR invalid_distance_payload {topic}")
        insert_event_log(
            topic=topic,
            event_type="invalid_distance_payload",
            reason="json_decode_error",
            payload=payload,
        )
        return False

    insert_slot_log(
        slot1_distance=data.get("slot1"),
        slot2_distance=data.get("slot2"),
    )
    return True


def handle_event_message(topic, payload):
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        update_kernel_status(f"ERROR invalid_event_payload {topic}")
        insert_event_log(
            topic=topic,
            event_type="invalid_event_payload",
            reason="json_decode_error",
            payload=payload,
        )
        return False

    insert_event_log(
        topic=topic,
        event_type=data.get("event"),
        reason=data.get("reason"),
        payload=payload,
    )
    update_kernel_status(f"MSG {topic} {data.get('event', '-')}")
    return True


def handle_message(topic, payload):
    if topic == "parking/rpi1/slot":
        if handle_slot_message(topic, payload):
            update_kernel_status(f"MSG {topic}")
        return

    if topic == "parking/rpi1/distance":
        if handle_distance_message(topic, payload):
            update_kernel_status(f"MSG {topic}")
        return

    if topic == "parking/rpi1/lot":
        insert_lot_log(payload)
        update_kernel_status(f"MSG {topic} {payload}")
        return

    if topic == "parking/rpi2/gate":
        insert_gate_log(payload)
        update_kernel_status(f"MSG {topic} {payload}")
        return

    if topic == "parking/rpi2/event":
        handle_event_message(topic, payload)
        return

    update_kernel_status(f"ERROR unknown_topic {topic}")
    insert_event_log(
        topic=topic,
        event_type="unknown_topic",
        reason="unsupported_topic",
        payload=payload,
    )


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe(MQTT_TOPIC)
        update_kernel_status("MQTT_OK")
        print(f"Subscribed to {MQTT_TOPIC}")
    else:
        update_kernel_status(f"MQTT_ERROR connect_rc_{rc}")
        print(f"MQTT connection failed: {rc}")


def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8", errors="replace")
    print(msg.topic, payload)
    try:
        handle_message(msg.topic, payload)
    except Exception:
        update_kernel_status(f"ERROR handler_exception {msg.topic}")
        raise


def main():
    init_db()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_HOST, MQTT_PORT)
    client.loop_forever()


if __name__ == "__main__":
    main()
