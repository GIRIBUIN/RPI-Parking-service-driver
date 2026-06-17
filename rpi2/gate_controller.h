#ifndef GATE_CONTROLLER_H
#define GATE_CONTROLLER_H

typedef enum {
  GATE_OPEN,
  GATE_CLOSED
} GateState;

typedef int (*detect_fn_t)(void);

// GPIO 및 PWM 초기화
void gate_controller_init(void);

// 게이트 제어
void gate_open(void);
void gate_close(void);
int gate_close_interruptible(detect_fn_t check);

// LED 제어
void entry_led_on(void);
void entry_led_off(void);
void full_led_on(void);
void full_led_off(void);

// 정리
void gate_controller_cleanup(void);

#endif
