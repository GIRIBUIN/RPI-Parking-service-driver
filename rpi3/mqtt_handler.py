import json
import os
import threading
import time
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
HEARTBEAT_TIMEOUT_SEC = int(os.environ.get("RPI3_HEARTBEAT_TIMEOUT_SEC", "5"))
STATUS_REPORT_INTERVAL_SEC = int(os.environ.get("RPI3_STATUS_REPORT_INTERVAL_SEC", "1"))

node_last_seen = {
    "rpi1": None,
    "rpi2": None,
}
node_last_payload = {
    "rpi1": "-",
    "rpi2": "-",
}
node_reported_status = {
    "rpi1": "unknown",
    "rpi2": "unknown",
}
last_handler_status = "starting"
last_handler_update = None
status_lock = threading.Lock()


def write_kernel_status(message):
    try:
        STATUS_DEVICE.write_text(f"{message}\n", encoding="utf-8")
    except OSError:
        pass


def build_kernel_status():
    now = time.time()

    with status_lock:
        nodes = {
            node: {
                "last_seen": last_seen,
                "last_payload": node_last_payload[node],
                "reported_status": node_reported_status[node],
            }
            for node, last_seen in node_last_seen.items()
        }
        handler_status = last_handler_status
        handler_update = last_handler_update

    lines = [
        "RPI3 Node Monitor",
        f"heartbeat_timeout_sec: {HEARTBEAT_TIMEOUT_SEC}",
    ]

    for node in ("rpi1", "rpi2"):
        last_seen = nodes[node]["last_seen"]

        if last_seen is None:
            state = "unknown"
            age = "-"
        else:
            age_sec = int(now - last_seen)
            if nodes[node]["reported_status"] == "offline":
                state = "offline"
            else:
                state = "online" if age_sec <= HEARTBEAT_TIMEOUT_SEC else "offline"
            age = str(age_sec)

        lines.extend([
            f"{node}: {state}",
            f"{node}_last_seen_sec: {age}",
            f"{node}_last_payload: {nodes[node]['last_payload']}",
        ])

    handler_age = "-" if handler_update is None else str(int(now - handler_update))
    lines.extend([
        f"mqtt_handler: {handler_status}",
        f"mqtt_handler_last_update_sec: {handler_age}",
    ])

    return "\n".join(lines)


def publish_kernel_status():
    write_kernel_status(build_kernel_status())


def update_kernel_status(message):
    global last_handler_status, last_handler_update

    with status_lock:
        last_handler_status = message
        last_handler_update = time.time()

    publish_kernel_status()


def handle_heartbeat_message(topic, payload):
    node = topic.split("/")[1]
    reported_status = "online"

    try:
        data = json.loads(payload)
        reported_status = data.get("status", "online")
    except json.JSONDecodeError:
        reported_status = "unknown"

    with status_lock:
        node_last_seen[node] = time.time()
        node_last_payload[node] = payload
        node_reported_status[node] = reported_status

    publish_kernel_status()


def status_report_loop():
    while True:
        publish_kernel_status()
        time.sleep(STATUS_REPORT_INTERVAL_SEC)


def start_status_reporter():
    thread = threading.Thread(target=status_report_loop, daemon=True)
    thread.start()


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
    if topic in ("parking/rpi1/heartbeat", "parking/rpi2/heartbeat"):
        handle_heartbeat_message(topic, payload)
        return

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
    start_status_reporter()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_HOST, MQTT_PORT)
    client.loop_forever()


if __name__ == "__main__":
    main()
