#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "gate_controller.h"
#include "switch_handler.h"
#include "mqtt_client.h"

static int running = 1;
static time_t last_disconnect_time = 0;

static void on_gate_command(const char *cmd) {
  last_disconnect_time = 0;

  if (strcmp(cmd, "CLOSE") == 0) {
    gate_close();
    mqtt_publish_gate_state("CLOSED");
    printf("게이트 닫힘\n");
  } else if (strcmp(cmd, "OPEN") == 0) {
    gate_open();
    mqtt_publish_gate_state("OPEN");
    printf("게이트 열림\n");
  }
}

static void on_switch_press(void) {
  last_disconnect_time = 0;

  gate_open();
  mqtt_publish_gate_state("OPEN");
  mqtt_publish_event("exit_requested");
  printf("출차 요청\n");
}

static void failsafe_check(void) {
  if (!mqtt_is_connected()) {
    if (last_disconnect_time == 0) {
      last_disconnect_time = time(NULL);
    } else if (time(NULL) - last_disconnect_time >= FAILSAFE_SEC) {
      printf("MQTT 미연결 상태 %d초 이상 지속 → Failsafe: 게이트 OPEN\n",
            FAILSAFE_SEC);
      gate_open();
      last_disconnect_time = 0;
    }
  } else {
    last_disconnect_time = 0;
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

  // GPIO 초기화
  gate_controller_init();
  gate_open();
  printf("게이트 초기 상태: OPEN\n");

  // Switch 인터럽트 설정
  switch_handler_init(on_switch_press);
  printf("Switch 인터럽트 설정 완료\n");

  // MQTT 클라이언트 초기화 및 연결
  mqtt_client_init(on_gate_command);
  mqtt_connect();
  printf("MQTT 연결 시도\n");

  // 초기 상태 발행
  mqtt_publish_gate_state("OPEN");

  // 메인 루프
  while (running) {
    failsafe_check();
    sleep(1);
  }

  // 정리
  gate_controller_cleanup();
  switch_handler_cleanup();
  mqtt_client_cleanup();

  printf("종료 완료\n");
  return 0;
}
