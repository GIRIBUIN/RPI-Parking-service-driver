#ifndef GATE_NODE_H
#define GATE_NODE_H

#include <linux/types.h>
#include <linux/time64.h>

// 게이트 상태 머신
typedef enum {
  STATE_IDLE = 0,
  STATE_ENTRY_DETECTED,
  STATE_ENTRY_WAITING,
  STATE_EXIT_REQUESTED,
  STATE_EXIT_VEHICLE_DETECTED,
} SystemState;

// 초음파 센서 측정
int ultrasonic_measure_cm(void);

// 게이트 제어
void gate_open(void);
void gate_close(void);

// 부저 제어
void buzzer_on(void);
void buzzer_off(void);

// LED 제어
void entry_led_set(int on);
void full_led_set(int on);

// 하드웨어 초기화/정리
int gate_hw_init(void);
void gate_hw_cleanup(void);

// 상태 머신 접근
SystemState get_current_state(void);
void set_current_state(SystemState state);

// 만차 플래그
int get_capacity_full(void);
void set_capacity_full(int full);

// 스위치 핸들링 (state_machine_thread 에서 폴링)
int switch_was_pressed(void);
void on_switch_press(void);

#endif
