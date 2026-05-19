#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "MQTTClient.h"
#include "config.h"
#include "mqtt_client.h"

#define MAX_PAYLOAD 256
#define CONNECTION_TIMEOUT 10000  // ms
#define RECONNECT_INTERVAL 5      // sec

static MQTTClient client;
static MqttCommandCallback mqtt_cmd_callback = NULL;
static int is_mqtt_connected = 0;
static pthread_mutex_t mqtt_mutex = PTHREAD_MUTEX_INITIALIZER;

static void message_arrived(void *context, char *topic, int topicLen,
                           MQTTClient_message *message) {
  char payload[MAX_PAYLOAD] = {0};
  strncpy(payload, (char *)message->payload, message->payloadlen);

  if (strcmp(topic, TOPIC_SUB_GATE_CMD) == 0) {
    if (mqtt_cmd_callback) {
      mqtt_cmd_callback(payload);
    }
  }

  MQTTClient_freeMessage(&message);
}

static void connection_lost(void *context, char *cause) {
  printf("MQTT 연결 끊김: %s\n", cause);
  pthread_mutex_lock(&mqtt_mutex);
  is_mqtt_connected = 0;
  pthread_mutex_unlock(&mqtt_mutex);
}

void mqtt_client_init(MqttCommandCallback on_gate_cmd) {
  mqtt_cmd_callback = on_gate_cmd;

  char address[256];
  snprintf(address, sizeof(address), "tcp://%s:%d", MQTT_HOST, MQTT_PORT);

  MQTTClient_create(&client, address, MQTT_CLIENT_ID,
                   MQTTCLIENT_PERSISTENCE_NONE, NULL);

  MQTTClient_setCallbacks(client, NULL, connection_lost, message_arrived, NULL);
}

void mqtt_connect(void) {
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  conn_opts.keepAliveInterval = MQTT_KEEPALIVE;
  conn_opts.cleansession = 1;

  int rc = MQTTClient_connect(client, &conn_opts);

  if (rc == MQTTCLIENT_SUCCESS) {
    pthread_mutex_lock(&mqtt_mutex);
    is_mqtt_connected = 1;
    pthread_mutex_unlock(&mqtt_mutex);

    MQTTClient_subscribe(client, TOPIC_SUB_GATE_CMD, 1);
    printf("MQTT 연결 성공, 구독: %s\n", TOPIC_SUB_GATE_CMD);
  } else {
    printf("MQTT 연결 실패 (코드: %d)\n", rc);
    pthread_mutex_lock(&mqtt_mutex);
    is_mqtt_connected = 0;
    pthread_mutex_unlock(&mqtt_mutex);
  }
}

void mqtt_publish_gate_state(const char *state) {
  if (!mqtt_is_connected()) {
    return;
  }

  MQTTClient_publish(client, TOPIC_PUB_GATE_STATE, strlen(state),
                    (void *)state, 1, 1, NULL);
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

  MQTTClient_publish(client, TOPIC_PUB_EVENT, strlen(payload),
                    (void *)payload, 1, 0, NULL);
}

int mqtt_is_connected(void) {
  pthread_mutex_lock(&mqtt_mutex);
  int connected = is_mqtt_connected;
  pthread_mutex_unlock(&mqtt_mutex);
  return connected;
}

void mqtt_client_cleanup(void) {
  if (mqtt_is_connected()) {
    MQTTClient_disconnect(client, 10000);
  }
  MQTTClient_destroy(&client);
}
