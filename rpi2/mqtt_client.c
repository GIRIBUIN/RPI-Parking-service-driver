#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <mosquitto.h>
#include "config.h"
#include "mqtt_client.h"

#define MAX_PAYLOAD 256

static struct mosquitto *mosq = NULL;
static MqttCapacityCallback mqtt_capacity_callback = NULL;
static int is_mqtt_connected = 0;
static pthread_mutex_t mqtt_mutex = PTHREAD_MUTEX_INITIALIZER;

static void on_connect(struct mosquitto *mosq, void *obj, int result) {
  if (result == 0) {
    printf("MQTT 연결 성공\n");
    pthread_mutex_lock(&mqtt_mutex);
    is_mqtt_connected = 1;
    pthread_mutex_unlock(&mqtt_mutex);

    mosquitto_subscribe(mosq, NULL, TOPIC_SUB_CAPACITY, 1);
    printf("구독: %s\n", TOPIC_SUB_CAPACITY);
  } else {
    printf("MQTT 연결 실패 (코드: %d)\n", result);
    pthread_mutex_lock(&mqtt_mutex);
    is_mqtt_connected = 0;
    pthread_mutex_unlock(&mqtt_mutex);
  }
}

static void on_disconnect(struct mosquitto *mosq, void *obj, int result) {
  printf("MQTT 연결 끊김\n");
  pthread_mutex_lock(&mqtt_mutex);
  is_mqtt_connected = 0;
  pthread_mutex_unlock(&mqtt_mutex);
}

static void on_message(struct mosquitto *mosq, void *obj,
                       const struct mosquitto_message *msg) {
  char payload[MAX_PAYLOAD] = {0};

  if (msg->payloadlen > 0) {
    strncpy(payload, (char *)msg->payload,
            msg->payloadlen < MAX_PAYLOAD - 1 ? msg->payloadlen : MAX_PAYLOAD - 1);
  }

  if (strcmp(msg->topic, TOPIC_SUB_CAPACITY) == 0) {
    if (mqtt_capacity_callback) {
      mqtt_capacity_callback(payload);
    }
  }
}

void mqtt_client_init(MqttCapacityCallback on_capacity) {
  mqtt_capacity_callback = on_capacity;

  mosquitto_lib_init();

  mosq = mosquitto_new(MQTT_CLIENT_ID, true, NULL);
  if (!mosq) {
    printf("MQTT 클라이언트 생성 실패\n");
    return;
  }

  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_disconnect_callback_set(mosq, on_disconnect);
  mosquitto_message_callback_set(mosq, on_message);
}

void mqtt_connect(void) {
  if (!mosq) {
    printf("MQTT 클라이언트 초기화 필요\n");
    return;
  }

  int rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE);

  if (rc == MOSQ_ERR_SUCCESS) {
    mosquitto_loop_start(mosq);
    printf("MQTT 연결 시도\n");
  } else {
    printf("MQTT 연결 실패: %s\n", mosquitto_strerror(rc));
  }
}

void mqtt_publish_gate_state(const char *state) {
  if (!mqtt_is_connected()) {
    return;
  }

  mosquitto_publish(mosq, NULL, TOPIC_PUB_GATE_STATE,
                   strlen(state), (void *)state, 1, true);
}

void mqtt_publish_event(const char *event) {
  if (!mqtt_is_connected()) {
    return;
  }

  char payload[MAX_PAYLOAD];
  time_t now = time(NULL);

  snprintf(payload, sizeof(payload),
          "{\"event\": \"%s\", \"timestamp\": %ld}",
          event, (long)now);

  mosquitto_publish(mosq, NULL, TOPIC_PUB_EVENT,
                   strlen(payload), (void *)payload, 1, false);
}

int mqtt_is_connected(void) {
  pthread_mutex_lock(&mqtt_mutex);
  int connected = is_mqtt_connected;
  pthread_mutex_unlock(&mqtt_mutex);
  return connected;
}

void mqtt_client_cleanup(void) {
  if (mosq) {
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    mosq = NULL;
  }
}
