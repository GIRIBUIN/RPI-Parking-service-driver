#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <mosquitto.h>
#include "config.h"

static struct mosquitto *mosq = NULL;
static int device_fd = -1;

static volatile int polling_running = 1;
static pthread_t poll_thread;
static char last_state[64] = "UNKNOWN";
static pthread_mutex_t state_str_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static const char *state_to_gate(const char *s) {
  return (strcmp(s, "IDLE") == 0) ? "CLOSED" : "OPEN";
}

static void transition_to_event(const char *prev, const char *next,
                                char *ev, char *reason) {
  ev[0] = reason[0] = '\0';
  if (strcmp(prev, "IDLE") == 0 && strcmp(next, "ENTRY_DETECTED") == 0) {
    strcpy(ev, "ENTRY_OPEN");
    strcpy(reason, "vehicle_detected");
  } else if (strcmp(prev, "IDLE") == 0 && strcmp(next, "EXIT_REQUESTED") == 0) {
    strcpy(ev, "EXIT_OPEN");
    strcpy(reason, "switch_pressed");
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

static void *poll_gate_state(void *arg) {
  char cur[64], prev[64], ev[64], reason[64], payload[256];
  int fd;
  ssize_t n;
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 200 * 1000 * 1000};

  pthread_mutex_lock(&state_str_mutex);
  strncpy(prev, last_state, sizeof(prev) - 1);
  pthread_mutex_unlock(&state_str_mutex);

  while (polling_running) {
    nanosleep(&ts, NULL);

    fd = open("/dev/gate_node", O_RDONLY);
    if (fd < 0) {
      perror("[POLL] open failed");
      continue;
    }

    n = read(fd, cur, sizeof(cur) - 1);
    close(fd);

    if (n <= 0)
      continue;

    cur[n] = '\0';
    while (n > 0 && (cur[n - 1] == '\n' || cur[n - 1] == ' ')) {
      cur[--n] = '\0';
    }

    if (strcmp(cur, prev) == 0)
      continue;

    printf("[POLL] %s -> %s\n", prev, cur);

    const char *gate = state_to_gate(cur);
    mosquitto_publish(mosq, NULL, TOPIC_PUB_GATE, (int)strlen(gate), gate, 1,
                      true);

    transition_to_event(prev, cur, ev, reason);
    if (ev[0] != '\0') {
      snprintf(payload, sizeof(payload),
               "{\"event\":\"%s\",\"reason\":\"%s\"}", ev, reason);
      mosquitto_publish(mosq, NULL, TOPIC_PUB_EVENT, (int)strlen(payload),
                        payload, 1, false);
    }

    pthread_mutex_lock(&state_str_mutex);
    strncpy(last_state, cur, sizeof(last_state) - 1);
    pthread_mutex_unlock(&state_str_mutex);
    strncpy(prev, cur, sizeof(prev) - 1);
  }

  printf("[POLL] Polling thread stopped\n");
  return NULL;
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

  // 폴링 스레드 시작
  if (pthread_create(&poll_thread, NULL, poll_gate_state, NULL) != 0) {
    perror("Failed to create poll thread");
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    close(device_fd);
    return 1;
  }

  printf("[MQTT] Polling thread started\n");

  // 메인 루프 (Ctrl+C로 종료)
  while (1) {
    sleep(1);
  }

  // 폴링 스레드 정지
  polling_running = 0;
  pthread_join(poll_thread, NULL);

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
