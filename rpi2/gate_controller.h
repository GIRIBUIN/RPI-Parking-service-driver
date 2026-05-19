#ifndef GATE_CONTROLLER_H
#define GATE_CONTROLLER_H

typedef enum {
  GATE_OPEN,
  GATE_CLOSED
} GateState;

// GPIO 및 PWM 초기화
void gate_controller_init(void);

// 게이트 제어
void gate_open(void);
void gate_close(void);

// 상태 조회
GateState gate_get_state(void);

// 정리
void gate_controller_cleanup(void);

#endif
