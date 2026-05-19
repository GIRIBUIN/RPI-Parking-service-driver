#ifndef CONFIG_H
#define CONFIG_H

// GPIO 핀 (BCM)
#define SERVO_PIN       18
#define LED_PIN         24
#define SWITCH_PIN      23

// 서보 PWM (wiringPi)
#define PWM_CLOCK       192
#define PWM_RANGE       2000
#define PWM_DUTY_OPEN   50    // 0.5ms (0도)
#define PWM_DUTY_CLOSED 240   // 2.4ms (~180도)
#define SERVO_DELAY_MS  500

// MQTT
#define MQTT_HOST       "192.168.1.100"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "rpi2_gate"
#define MQTT_KEEPALIVE  60

#define TOPIC_SUB_GATE_CMD   "parking/control/gate"
#define TOPIC_PUB_GATE_STATE "parking/rpi2/gate"
#define TOPIC_PUB_EVENT      "parking/rpi2/event"

// 디바운싱 및 Failsafe
#define DEBOUNCE_MS     300
#define FAILSAFE_SEC    10

#endif
