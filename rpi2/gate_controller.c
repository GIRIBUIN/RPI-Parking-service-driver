#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "config.h"
#include "gate_controller.h"

#define GPIO_PATH "/sys/class/gpio"

static GateState current_state = GATE_CLOSED;
static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;

// 하프스텝 시퀀스 (8단계)
static const int stepper_sequence[8][4] = {
  {1, 0, 0, 0},  // 0
  {1, 1, 0, 0},  // 1
  {0, 1, 0, 0},  // 2
  {0, 1, 1, 0},  // 3
  {0, 0, 1, 0},  // 4
  {0, 0, 1, 1},  // 5
  {0, 0, 0, 1},  // 6
  {1, 0, 0, 1}   // 7
};

// GPIO export/unexport
static int export_gpio(int pin) {
  char path[256];
  snprintf(path, sizeof(path), "%s/export", GPIO_PATH);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    perror("GPIO export 실패");
    return -1;
  }
  dprintf(fd, "%d", pin);
  close(fd);
  usleep(100000);  // 100ms delay
  return 0;
}

static int unexport_gpio(int pin) {
  char path[256];
  snprintf(path, sizeof(path), "%s/unexport", GPIO_PATH);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return -1;
  }
  dprintf(fd, "%d", pin);
  close(fd);
  return 0;
}

// GPIO 방향 설정
static int set_gpio_direction(int pin, const char *dir) {
  char path[256];
  snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_PATH, pin);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    perror("GPIO direction 설정 실패");
    return -1;
  }
  write(fd, dir, strlen(dir));
  close(fd);
  return 0;
}

// GPIO 값 쓰기
static int set_gpio_value(int pin, int value) {
  char path[256];
  snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_PATH, pin);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return -1;
  }
  dprintf(fd, "%d", value);
  close(fd);
  return 0;
}

// 스테퍼 모터 핀 설정
static void stepper_set_pins(int a, int b, int c, int d) {
  set_gpio_value(STEPPER_IN1, a);
  set_gpio_value(STEPPER_IN2, b);
  set_gpio_value(STEPPER_IN3, c);
  set_gpio_value(STEPPER_IN4, d);
}

// 스테퍼 모터 이동 (양수: 정방향, 음수: 역방향)
static void stepper_move(int steps) {
  int direction = (steps > 0) ? 1 : -1;
  int abs_steps = (steps > 0) ? steps : -steps;
  static int current_step = 0;

  for (int i = 0; i < abs_steps; i++) {
    current_step = (current_step + direction + 8) % 8;
    stepper_set_pins(stepper_sequence[current_step][0],
                     stepper_sequence[current_step][1],
                     stepper_sequence[current_step][2],
                     stepper_sequence[current_step][3]);
    usleep(STEPPER_STEP_DELAY_US);
  }
}

// 스테퍼 모터 전원 차단 (발열 방지)
static void stepper_deenergize(void) {
  stepper_set_pins(0, 0, 0, 0);
}

void gate_controller_init(void) {
  // 스테퍼 모터 핀 export 및 설정
  export_gpio(STEPPER_IN1);
  export_gpio(STEPPER_IN2);
  export_gpio(STEPPER_IN3);
  export_gpio(STEPPER_IN4);

  set_gpio_direction(STEPPER_IN1, "out");
  set_gpio_direction(STEPPER_IN2, "out");
  set_gpio_direction(STEPPER_IN3, "out");
  set_gpio_direction(STEPPER_IN4, "out");

  stepper_set_pins(0, 0, 0, 0);

  // 입차 LED GPIO export 및 설정
  export_gpio(ENTRY_LED_PIN);
  set_gpio_direction(ENTRY_LED_PIN, "out");
  set_gpio_value(ENTRY_LED_PIN, 0);

  // 만차 LED GPIO export 및 설정
  export_gpio(FULL_LED_PIN);
  set_gpio_direction(FULL_LED_PIN, "out");
  set_gpio_value(FULL_LED_PIN, 0);

  // 초기 상태: CLOSED
  stepper_move(-STEPPER_GATE_STEPS);
  stepper_deenergize();
  current_state = GATE_CLOSED;
}

void gate_open(void) {
  pthread_mutex_lock(&gate_mutex);

  if (current_state == GATE_OPEN) {
    pthread_mutex_unlock(&gate_mutex);
    return;
  }

  stepper_move(STEPPER_GATE_STEPS);
  stepper_deenergize();
  current_state = GATE_OPEN;

  pthread_mutex_unlock(&gate_mutex);
}

void gate_close(void) {
  pthread_mutex_lock(&gate_mutex);

  if (current_state == GATE_CLOSED) {
    pthread_mutex_unlock(&gate_mutex);
    return;
  }

  stepper_move(-STEPPER_GATE_STEPS);
  stepper_deenergize();
  current_state = GATE_CLOSED;

  pthread_mutex_unlock(&gate_mutex);
}

int gate_close_interruptible(detect_fn_t check) {
  int done = 0;
  int steps;

  pthread_mutex_lock(&gate_mutex);
  if (current_state == GATE_CLOSED) {
    pthread_mutex_unlock(&gate_mutex);
    return 0;
  }
  pthread_mutex_unlock(&gate_mutex);

  while (done < STEPPER_GATE_STEPS) {
    steps = STEPPER_GATE_STEPS - done;
    if (steps > 64) steps = 64;
    stepper_move(-steps);
    done += steps;

    if (done < STEPPER_GATE_STEPS && check != NULL && check()) {
      stepper_move(done);
      stepper_deenergize();
      return -1;
    }
  }

  stepper_deenergize();
  pthread_mutex_lock(&gate_mutex);
  current_state = GATE_CLOSED;
  pthread_mutex_unlock(&gate_mutex);
  return 0;
}

void entry_led_on(void) {
  set_gpio_value(ENTRY_LED_PIN, 1);
}

void entry_led_off(void) {
  set_gpio_value(ENTRY_LED_PIN, 0);
}

void full_led_on(void) {
  set_gpio_value(FULL_LED_PIN, 1);
}

void full_led_off(void) {
  set_gpio_value(FULL_LED_PIN, 0);
}

void gate_controller_cleanup(void) {
  // 스테퍼 모터 전원 차단
  stepper_deenergize();

  // GPIO unexport
  unexport_gpio(STEPPER_IN1);
  unexport_gpio(STEPPER_IN2);
  unexport_gpio(STEPPER_IN3);
  unexport_gpio(STEPPER_IN4);

  entry_led_off();
  full_led_off();
  unexport_gpio(ENTRY_LED_PIN);
  unexport_gpio(FULL_LED_PIN);
}
