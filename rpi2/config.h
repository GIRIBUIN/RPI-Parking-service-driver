#ifndef CONFIG_H
#define CONFIG_H

// GPIO 핀 (BCM)
#define ENTRY_LED_PIN        24
#define SWITCH_PIN           23
#define ULTRASONIC_TRIG_PIN  17
#define ULTRASONIC_ECHO_PIN  27
#define BUZZER_PIN           22
#define FULL_LED_PIN         25

// 스테퍼 모터 (28BYJ-48 + ULN2003)
#define STEPPER_IN1          5    // GPIO 5 (핀 29)
#define STEPPER_IN2          6    // GPIO 6 (핀 31)
#define STEPPER_IN3          13   // GPIO 13 (핀 33)
#define STEPPER_IN4          19   // GPIO 19 (핀 35)
#define STEPPER_STEP_DELAY_US 2000  // 스텝 간격 2ms
#define STEPPER_GATE_STEPS   1024  // 90도 회전 (4096 = 360도)

// MQTT
#define MQTT_HOST       "127.0.0.1"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "rpi2_gate"
#define MQTT_KEEPALIVE  60

// #define TOPIC_SUB_CAPACITY   "parking/rpi3/capacity"
#define TOPIC_PUB_GATE       "parking/rpi2/gate"   // [검토] 게이트 상태 발행 topic
#define TOPIC_SUB_CAPACITY   "parking/rpi1/lot"  // [검토] topic 수정
#define TOPIC_PUB_EVENT      "parking/rpi2/event"  // [검토] 진입/출차 이벤트 발행 topic

// 디바운싱 및 타이머
#define DEBOUNCE_MS          300
#define ENTRY_CLOSE_DELAY_SEC 5
#define EXIT_TIMEOUT_SEC     10

// 초음파 임계값 (cm)
#define VEHICLE_DETECT_CM    50

#endif
