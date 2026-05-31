#ifndef CONFIG_H
#define CONFIG_H

// GPIO 핀 (BCM)
#define SERVO_PIN            18
#define ENTRY_LED_PIN        24
#define SWITCH_PIN           23
#define ULTRASONIC_TRIG_PIN  17
#define ULTRASONIC_ECHO_PIN  27
#define BUZZER_PIN           22
#define FULL_LED_PIN         25

// 서보 PWM (나노초 단위)
// 50Hz = 20ms = 20000000ns 주기
// OPEN (0도):   0.5ms = 500000ns
// CLOSED(180도): 2.4ms = 2400000ns
#define PWM_PERIOD_NS        20000000
#define PWM_DUTY_OPEN_NS     500000
#define PWM_DUTY_CLOSED_NS   2400000
#define SERVO_DELAY_MS       500

// MQTT
#define MQTT_HOST       "192.168.1.100"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "rpi2_gate"
#define MQTT_KEEPALIVE  60

#define TOPIC_SUB_CAPACITY   "parking/rpi3/capacity"
#define TOPIC_PUB_GATE_STATE "parking/rpi2/gate"
#define TOPIC_PUB_EVENT      "parking/rpi2/event"

// 디바운싱 및 타이머
#define DEBOUNCE_MS          300
#define ENTRY_CLOSE_DELAY_SEC 5
#define EXIT_TIMEOUT_SEC     10

// 초음파 임계값 (cm)
#define VEHICLE_DETECT_CM    50

#endif
