#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

typedef void (*MqttCapacityCallback)(const char *status);

// MQTT 클라이언트 초기화
void mqtt_client_init(MqttCapacityCallback on_capacity);

// MQTT 브로커 연결
void mqtt_connect(void);

// 연결 상태 확인
int mqtt_is_connected(void);

// [검토] 게이트 상태를 RPI3/Dashboard로 발행. state: "OPEN" 또는 "CLOSED"
int mqtt_publish_gate_state(const char *state);

// [검토] 게이트 open 원인을 RPI3/Dashboard로 발행.
int mqtt_publish_gate_event(const char *event, const char *reason);

// 정리
void mqtt_client_cleanup(void);

#endif
