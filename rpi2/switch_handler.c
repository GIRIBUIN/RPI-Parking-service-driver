#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include "config.h"
#include "switch_handler.h"

#define GPIO_PATH "/sys/class/gpio"

static SwitchCallback switch_callback = NULL;
static uint64_t last_press_time = 0;
static pthread_t switch_thread;
static int thread_running = 0;

static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

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

// GPIO 방향 설정 (pull-up)
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

// GPIO edge 감지 설정 (falling edge)
static int set_gpio_edge(int pin, const char *edge) {
  char path[256];
  snprintf(path, sizeof(path), "%s/gpio%d/edge", GPIO_PATH, pin);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    perror("GPIO edge 설정 실패");
    return -1;
  }
  write(fd, edge, strlen(edge));
  close(fd);
  return 0;
}

// poll을 사용한 GPIO 모니터링 스레드
static void* switch_monitor_thread(void *arg) {
  char path[256];
  char val[10];
  struct pollfd fds[1];

  snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_PATH, SWITCH_PIN);
  int gpio_fd = open(path, O_RDONLY);
  if (gpio_fd < 0) {
    perror("GPIO value 파일 열기 실패");
    return NULL;
  }

  fds[0].fd = gpio_fd;
  fds[0].events = POLLPRI;

  while (thread_running) {
    // edge 감지를 위해 값 읽기 (초기 상태 무시)
    lseek(gpio_fd, 0, SEEK_SET);
    read(gpio_fd, val, sizeof(val));

    // poll로 edge 대기 (timeout 500ms)
    int ret = poll(fds, 1, 500);

    if (ret > 0 && (fds[0].revents & POLLPRI)) {
      // edge 감지됨
      lseek(gpio_fd, 0, SEEK_SET);
      read(gpio_fd, val, sizeof(val));

      // 소프트웨어 디바운싱
      uint64_t now = get_time_ms();
      uint64_t elapsed = now - last_press_time;

      if (elapsed >= DEBOUNCE_MS) {
        last_press_time = now;
        if (switch_callback) {
          switch_callback();
        }
      }
    }
  }

  close(gpio_fd);
  return NULL;
}

void switch_handler_init(SwitchCallback on_press) {
  switch_callback = on_press;
  last_press_time = 0;

  // GPIO export 및 설정
  export_gpio(SWITCH_PIN);
  set_gpio_direction(SWITCH_PIN, "in");
  set_gpio_edge(SWITCH_PIN, "falling");

  // 모니터링 스레드 시작
  thread_running = 1;
  if (pthread_create(&switch_thread, NULL, switch_monitor_thread, NULL) != 0) {
    perror("Switch 모니터링 스레드 생성 실패");
    thread_running = 0;
  }
}

void switch_handler_cleanup(void) {
  thread_running = 0;
  if (pthread_join(switch_thread, NULL) != 0) {
    perror("Switch 스레드 종료 실패");
  }
  unexport_gpio(SWITCH_PIN);
}
