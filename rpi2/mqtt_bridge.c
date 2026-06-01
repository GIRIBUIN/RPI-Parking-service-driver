#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mosquitto.h>
#include "config.h"

static struct mosquitto *mosq = NULL;
static int device_fd = -1;

static void on_connect(struct mosquitto *m, void *userdata, int result) {
  if (result == 0) {
    printf("[MQTT] Connected successfully\n");
    mosquitto_subscribe(m, NULL, TOPIC_SUB_CAPACITY, 1);
    printf("[MQTT] Subscribed: %s\n", TOPIC_SUB_CAPACITY);
  } else {
    printf("[MQTT] Connection failed (code: %d)\n", result);
  }
}

static void on_disconnect(struct mosquitto *m, void *userdata, int result) {
  if (result == 0) {
    printf("[MQTT] Disconnected cleanly\n");
  } else {
    printf("[MQTT] Unexpected disconnect (code: %d)\n", result);
  }
}

static void on_message(struct mosquitto *m, void *userdata,
                       const struct mosquitto_message *msg) {
  char payload[256];
  int n;

  if (msg->payloadlen > 0) {
    n = snprintf(payload, sizeof(payload) - 1, "%s", (char *)msg->payload);
    payload[n] = '\0';
  } else {
    payload[0] = '\0';
  }

  printf("[MQTT] Topic: %s, Payload: %s\n", msg->topic, payload);

  // /dev/gate_node에 쓰기
  if (device_fd >= 0) {
    if (write(device_fd, payload, strlen(payload)) < 0) {
      perror("write to /dev/gate_node failed");
    }
  }
}

int main(void) {
  int ret;

  printf("MQTT Bridge Starting\n");

  // /dev/gate_node 오픈
  device_fd = open("/dev/gate_node", O_WRONLY);
  if (device_fd < 0) {
    perror("Failed to open /dev/gate_node");
    fprintf(stderr,
            "Make sure the gate_node module is loaded: sudo insmod gate_node.ko\n");
    return 1;
  }

  printf("[DEVICE] Opened /dev/gate_node\n");

  // Mosquitto 초기화
  mosquitto_lib_init();

  mosq = mosquitto_new(MQTT_CLIENT_ID, true, NULL);
  if (!mosq) {
    fprintf(stderr, "Failed to create mosquitto instance\n");
    close(device_fd);
    return 1;
  }

  // 콜백 설정
  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_disconnect_callback_set(mosq, on_disconnect);
  mosquitto_message_callback_set(mosq, on_message);

  // MQTT 브로커 연결
  printf("[MQTT] Connecting to %s:%d\n", MQTT_HOST, MQTT_PORT);
  ret = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE);
  if (ret != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "Failed to connect: %s\n", mosquitto_strerror(ret));
    mosquitto_destroy(mosq);
    close(device_fd);
    return 1;
  }

  // 비동기 루프 시작
  ret = mosquitto_loop_start(mosq);
  if (ret != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "Failed to start loop: %s\n", mosquitto_strerror(ret));
    mosquitto_destroy(mosq);
    close(device_fd);
    return 1;
  }

  printf("[MQTT] Connected and listening\n");

  // 메인 루프 (Ctrl+C로 종료)
  while (1) {
    sleep(1);
  }

  // 정리
  mosquitto_loop_stop(mosq, true);
  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  if (device_fd >= 0) {
    close(device_fd);
  }

  printf("MQTT Bridge Stopped\n");
  return 0;
}
