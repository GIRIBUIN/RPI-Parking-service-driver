#ifndef CONFIG_H
#define CONFIG_H

// GPIO 핀 (BCM)
#define SERVO_PIN       18
#define LED_PIN         24
#define SWITCH_PIN      23

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

#define TOPIC_SUB_GATE_CMD   "parking/control/gate"
#define TOPIC_PUB_GATE_STATE "parking/rpi2/gate"
#define TOPIC_PUB_EVENT      "parking/rpi2/event"

// 디바운싱 및 Failsafe
#define DEBOUNCE_MS     300
#define FAILSAFE_SEC    10

#endif
