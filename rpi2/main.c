#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "config.h"
#include "gate_controller.h"
#include "switch_handler.h"
#include "mqtt_client.h"
#include "ultrasonic.h"
#include "buzzer.h"

typedef enum {
  STATE_IDLE,
  STATE_ENTRY_DETECTED,
  STATE_ENTRY_WAITING,
  STATE_EXIT_REQUESTED,
  STATE_EXIT_VEHICLE_DETECTED
} SystemState;

static int running = 1;
static SystemState sys_state = STATE_IDLE;
static time_t state_timer = 0;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static int idle_log_count = 0;  // IDLE 상태에서 3초마다 거리 출력용

static void on_capacity(const char *status) {
  if (strcmp(status, "FULL") == 0) {
    printf("만차 신호 수신 → 만차 LED ON\n");
    full_led_on();
  } else if (strcmp(status, "AVAILABLE") == 0) {
    printf("만차 해제 신호 수신 → 만차 LED OFF\n");
    full_led_off();
  }
}

static void on_switch_press(void) {
  pthread_mutex_lock(&state_mutex);
  SystemState s = sys_state;
  pthread_mutex_unlock(&state_mutex);

  // 입차 처리 중 버튼 무시
  if (s == STATE_ENTRY_DETECTED || s == STATE_ENTRY_WAITING) {
    printf("입차 처리 중 — 출차 버튼 무시\n");
    return;
  }

  // STATE_IDLE에서만 출차 요청 처리
  if (s == STATE_IDLE) {
    pthread_mutex_lock(&state_mutex);
    sys_state = STATE_EXIT_REQUESTED;
    state_timer = time(NULL);
    pthread_mutex_unlock(&state_mutex);

    gate_open();
    entry_led_on();
    buzzer_on();
    mqtt_publish_gate_state("OPEN");
    mqtt_publish_event("exit_requested");
    printf("출차 요청 — 게이트 OPEN, LED ON, 부저 ON, 10초 타이머 시작\n");
  }
}

static void shutdown_handler(int signum) {
  printf("\n종료 중...\n");
  running = 0;
}

int main(void) {
  signal(SIGINT, shutdown_handler);
  signal(SIGTERM, shutdown_handler);

  printf("RPI2 바리게이트 제어 노드 시작\n");

  // 하드웨어 초기화
  gate_controller_init();
  ultrasonic_init();
  buzzer_init();

  // Switch 인터럽트 설정
  switch_handler_init(on_switch_press);
  printf("Switch 모니터링 시작\n");

  // MQTT 클라이언트 초기화 및 연결
  mqtt_client_init(on_capacity);
  mqtt_connect();
  printf("MQTT 연결 시도\n");

  // 초기 상태: 게이트 닫힘
  gate_close();
  entry_led_off();
  buzzer_off();
  mqtt_publish_gate_state("CLOSED");
  printf("게이트 초기 상태: CLOSED\n");

  // 메인 루프 — 100ms 폴링 (10Hz)
  while (running) {
    double dist = ultrasonic_measure_cm();

    pthread_mutex_lock(&state_mutex);
    SystemState s = sys_state;
    time_t now = time(NULL);
    pthread_mutex_unlock(&state_mutex);

    // State Machine
    switch (s) {
    case STATE_IDLE:
      // 3초마다 거리 출력 (센서 정상 작동 확인용)
      if (++idle_log_count >= 30) {  // 100ms × 30 = 3초
        printf("[IDLE] 거리: %.1f cm\n", dist);
        idle_log_count = 0;
      }

      if (dist > 0 && dist <= VEHICLE_DETECT_CM) {
        // 차량 감지 — 입차 시작
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_ENTRY_DETECTED;
        state_timer = now;
        pthread_mutex_unlock(&state_mutex);

        gate_open();
        entry_led_on();
        buzzer_on();
        mqtt_publish_gate_state("OPEN");
        mqtt_publish_event("entry_detected");
        printf("[%.1f cm] 입차 차량 감지 — 게이트 OPEN, LED ON, 부저 ON\n", dist);
      }
      break;

    case STATE_ENTRY_DETECTED:
      if (dist < 0 || dist > VEHICLE_DETECT_CM) {
        // 차량 사라짐 — 입차 대기로 전환
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_ENTRY_WAITING;
        state_timer = now;
        pthread_mutex_unlock(&state_mutex);

        buzzer_off();
        printf("[%.1f cm] 차량 사라짐 — 5초 타이머 시작\n", dist);
      }
      break;

    case STATE_ENTRY_WAITING:
      if (dist > 0 && dist <= VEHICLE_DETECT_CM) {
        // 차량 재감지 — 입차 상태로 복귀
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_ENTRY_DETECTED;
        state_timer = now;
        pthread_mutex_unlock(&state_mutex);

        buzzer_on();
        printf("[%.1f cm] 차량 재감지 — 입차 상태 복귀\n", dist);
      } else if (now - state_timer >= ENTRY_CLOSE_DELAY_SEC) {
        // 5초 경과 — 게이트 닫음
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_IDLE;
        pthread_mutex_unlock(&state_mutex);

        gate_close();
        entry_led_off();
        mqtt_publish_gate_state("CLOSED");
        printf("5초 경과 — 게이트 닫음, LED OFF\n");
      }
      break;

    case STATE_EXIT_REQUESTED:
      if (dist > 0 && dist <= VEHICLE_DETECT_CM) {
        // 차량 감지 — 출차 감지 상태로 전환
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_EXIT_VEHICLE_DETECTED;
        pthread_mutex_unlock(&state_mutex);

        printf("[%.1f cm] 출차 차량 감지\n", dist);
      } else if (now - state_timer >= EXIT_TIMEOUT_SEC) {
        // 10초 경과 — 타임아웃, 게이트 닫음
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_IDLE;
        pthread_mutex_unlock(&state_mutex);

        entry_led_off();
        buzzer_off();
        gate_close();
        mqtt_publish_gate_state("CLOSED");
        printf("10초 경과 (차량 미감지) — LED OFF, 부저 OFF, 게이트 닫음 (타임아웃)\n");
      }
      break;

    case STATE_EXIT_VEHICLE_DETECTED:
      if (dist < 0 || dist > VEHICLE_DETECT_CM) {
        // 차량 빠져나감 — 출차 완료
        pthread_mutex_lock(&state_mutex);
        sys_state = STATE_IDLE;
        pthread_mutex_unlock(&state_mutex);

        entry_led_off();
        buzzer_off();
        gate_close();
        mqtt_publish_gate_state("CLOSED");
        mqtt_publish_event("exit_completed");
        printf("[%.1f cm] 차량 빠져나감 — 출차 완료, LED OFF, 부저 OFF, 게이트 닫음\n", dist);
      }
      break;
    }

    usleep(100000);  // 100ms
  }

  // 정리
  gate_controller_cleanup();
  ultrasonic_cleanup();
  buzzer_cleanup();
  switch_handler_cleanup();
  mqtt_client_cleanup();

  printf("종료 완료\n");
  return 0;
}
