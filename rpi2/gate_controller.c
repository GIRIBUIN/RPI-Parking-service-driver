#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "config.h"
#include "gate_controller.h"

#define GPIO_PATH "/sys/class/gpio"
#define PWM_PATH "/sys/class/pwm/pwmchip0"

static GateState current_state = GATE_OPEN;
static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;

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

// PWM 설정
static int setup_pwm(void) {
  char path[256];
  int fd;

  // PWM0 export
  snprintf(path, sizeof(path), "%s/export", PWM_PATH);
  fd = open(path, O_WRONLY);
  if (fd >= 0) {
    write(fd, "0", 1);
    close(fd);
    usleep(100000);
  }

  // PWM period 설정 (50Hz = 20ms = 20000000ns)
  snprintf(path, sizeof(path), "%s/pwm0/period", PWM_PATH);
  fd = open(path, O_WRONLY);
  if (fd < 0) {
    perror("PWM period 설정 실패");
    return -1;
  }
  dprintf(fd, "20000000");
  close(fd);

  // PWM duty cycle 초기값 (OPEN)
  snprintf(path, sizeof(path), "%s/pwm0/duty_cycle", PWM_PATH);
  fd = open(path, O_WRONLY);
  if (fd < 0) {
    perror("PWM duty_cycle 설정 실패");
    return -1;
  }
  dprintf(fd, "%d", PWM_DUTY_OPEN_NS);
  close(fd);

  // PWM 활성화
  snprintf(path, sizeof(path), "%s/pwm0/enable", PWM_PATH);
  fd = open(path, O_WRONLY);
  if (fd >= 0) {
    write(fd, "1", 1);
    close(fd);
  }

  return 0;
}

static int set_pwm_duty(int duty_ns) {
  char path[256];
  snprintf(path, sizeof(path), "%s/pwm0/duty_cycle", PWM_PATH);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return -1;
  }
  dprintf(fd, "%d", duty_ns);
  close(fd);
  return 0;
}

void gate_controller_init(void) {
  // LED GPIO export 및 설정
  export_gpio(LED_PIN);
  set_gpio_direction(LED_PIN, "out");
  set_gpio_value(LED_PIN, 0);

  // 서보 PWM 설정
  setup_pwm();

  current_state = GATE_OPEN;
}

void gate_open(void) {
  pthread_mutex_lock(&gate_mutex);

  if (current_state == GATE_OPEN) {
    pthread_mutex_unlock(&gate_mutex);
    return;
  }

  // PWM duty cycle 설정 (0.5ms)
  set_pwm_duty(PWM_DUTY_OPEN_NS);
  usleep(SERVO_DELAY_MS * 1000);

  // PWM 신호 차단 (duty_cycle = 0)
  set_pwm_duty(0);

  current_state = GATE_OPEN;
  set_gpio_value(LED_PIN, 0);

  pthread_mutex_unlock(&gate_mutex);
}

void gate_close(void) {
  pthread_mutex_lock(&gate_mutex);

  if (current_state == GATE_CLOSED) {
    pthread_mutex_unlock(&gate_mutex);
    return;
  }

  // PWM duty cycle 설정 (2.4ms)
  set_pwm_duty(PWM_DUTY_CLOSED_NS);
  usleep(SERVO_DELAY_MS * 1000);

  // PWM 신호 차단 (duty_cycle = 0)
  set_pwm_duty(0);

  current_state = GATE_CLOSED;
  set_gpio_value(LED_PIN, 1);

  pthread_mutex_unlock(&gate_mutex);
}

GateState gate_get_state(void) {
  pthread_mutex_lock(&gate_mutex);
  GateState state = current_state;
  pthread_mutex_unlock(&gate_mutex);
  return state;
}

void gate_controller_cleanup(void) {
  // PWM 비활성화
  char path[256];
  snprintf(path, sizeof(path), "%s/pwm0/enable", PWM_PATH);
  int fd = open(path, O_WRONLY);
  if (fd >= 0) {
    write(fd, "0", 1);
    close(fd);
  }

  // PWM0 unexport
  snprintf(path, sizeof(path), "%s/unexport", PWM_PATH);
  fd = open(path, O_WRONLY);
  if (fd >= 0) {
    write(fd, "0", 1);
    close(fd);
  }

  // LED GPIO unexport
  set_gpio_value(LED_PIN, 0);
  unexport_gpio(LED_PIN);
}
