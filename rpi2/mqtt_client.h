#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

typedef void (*MqttCapacityCallback)(const char *status);

// MQTT 클라이언트 초기화
void mqtt_client_init(MqttCapacityCallback on_capacity);

// MQTT 브로커 연결
void mqtt_connect(void);

// 게이트 상태 발행
void mqtt_publish_gate_state(const char *state);

// 이벤트 발행
void mqtt_publish_event(const char *event);

// 연결 상태 확인
int mqtt_is_connected(void);

// 정리
void mqtt_client_cleanup(void);

#endif
