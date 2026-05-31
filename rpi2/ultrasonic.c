#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "config.h"

static pthread_mutex_t ultrasonic_mutex = PTHREAD_MUTEX_INITIALIZER;

static void export_gpio(int pin) {
  FILE *f = fopen("/sys/class/gpio/export", "w");
  if (f) {
    fprintf(f, "%d\n", pin);
    fclose(f);
  }
  usleep(100000);
}

static void unexport_gpio(int pin) {
  FILE *f = fopen("/sys/class/gpio/unexport", "w");
  if (f) {
    fprintf(f, "%d\n", pin);
    fclose(f);
  }
}

static void set_gpio_direction(int pin, const char *direction) {
  char path[64];
  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f, "%s\n", direction);
    fclose(f);
  }
}

static void set_gpio_value(int pin, int value) {
  char path[64];
  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f, "%d\n", value);
    fclose(f);
  }
}

static int get_gpio_value(int pin) {
  char path[64];
  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
  FILE *f = fopen(path, "r");
  if (!f) return -1;
  int val;
  fscanf(f, "%d", &val);
  fclose(f);
  return val;
}

void ultrasonic_init(void) {
  export_gpio(ULTRASONIC_TRIG_PIN);
  export_gpio(ULTRASONIC_ECHO_PIN);

  set_gpio_direction(ULTRASONIC_TRIG_PIN, "out");
  set_gpio_direction(ULTRASONIC_ECHO_PIN, "in");

  set_gpio_value(ULTRASONIC_TRIG_PIN, 0);
  printf("초음파 센서 초기화 완료\n");
}

double ultrasonic_measure_cm(void) {
  pthread_mutex_lock(&ultrasonic_mutex);

  // TRIG: LOW 2us → HIGH 10us → LOW
  set_gpio_value(ULTRASONIC_TRIG_PIN, 0);
  usleep(2);
  set_gpio_value(ULTRASONIC_TRIG_PIN, 1);
  usleep(10);
  set_gpio_value(ULTRASONIC_TRIG_PIN, 0);

  // ECHO가 HIGH로 변할 때까지 대기 (타임아웃 30ms)
  struct timespec ts_start, ts_now;
  clock_gettime(CLOCK_MONOTONIC, &ts_start);
  while (get_gpio_value(ULTRASONIC_ECHO_PIN) == 0) {
    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    long elapsed_us = (ts_now.tv_sec - ts_start.tv_sec) * 1000000LL
                    + (ts_now.tv_nsec - ts_start.tv_nsec) / 1000;
    if (elapsed_us > 30000) {
      pthread_mutex_unlock(&ultrasonic_mutex);
      return -1.0;
    }
  }

  // ECHO HIGH 진입 시각 기록
  clock_gettime(CLOCK_MONOTONIC, &ts_start);

  // ECHO가 LOW로 떨어질 때까지 대기 (타임아웃 30ms)
  struct timespec ts_end;
  while (get_gpio_value(ULTRASONIC_ECHO_PIN) == 1) {
    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    long elapsed_us = (ts_now.tv_sec - ts_start.tv_sec) * 1000000LL
                    + (ts_now.tv_nsec - ts_start.tv_nsec) / 1000;
    if (elapsed_us > 30000) {
      pthread_mutex_unlock(&ultrasonic_mutex);
      return -1.0;
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  // ECHO 펄스 폭을 마이크로초 단위로 계산
  long echo_us = (ts_end.tv_sec - ts_start.tv_sec) * 1000000LL
               + (ts_end.tv_nsec - ts_start.tv_nsec) / 1000;

  pthread_mutex_unlock(&ultrasonic_mutex);

  // 거리 = echo_us / 58.0 (음속 340m/s, 왕복 고려)
  return echo_us / 58.0;
}

void ultrasonic_cleanup(void) {
  unexport_gpio(ULTRASONIC_TRIG_PIN);
  unexport_gpio(ULTRASONIC_ECHO_PIN);
  printf("초음파 센서 정리 완료\n");
}
