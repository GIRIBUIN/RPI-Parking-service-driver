import json
import os
import signal
import time

import paho.mqtt.client as mqtt


BROKER_HOST = os.getenv("MQTT_HOST", "192.168.4.1")
BROKER_PORT = int(os.getenv("MQTT_PORT", "1883"))
TOPIC = "parking/rpi1/heartbeat"
PAYLOAD = {"node": "rpi1", "status": "online"}
INTERVAL_SECONDS = 2

running = True


def handle_signal(signum, frame):
    global running
    running = False


def main():
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    client = mqtt.Client()
    client.connect(BROKER_HOST, BROKER_PORT, 60)
    client.loop_start()

    try:
        while running:
            client.publish(TOPIC, json.dumps(PAYLOAD), qos=1)
            time.sleep(INTERVAL_SECONDS)
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
