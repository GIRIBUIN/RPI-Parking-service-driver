#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "config.h"

static pthread_mutex_t buzzer_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void buzzer_init(void) {
  export_gpio(BUZZER_PIN);
  set_gpio_direction(BUZZER_PIN, "out");
  set_gpio_value(BUZZER_PIN, 0);
  printf("부저 초기화 완료\n");
}

void buzzer_on(void) {
  pthread_mutex_lock(&buzzer_mutex);
  set_gpio_value(BUZZER_PIN, 1);
  pthread_mutex_unlock(&buzzer_mutex);
}

void buzzer_off(void) {
  pthread_mutex_lock(&buzzer_mutex);
  set_gpio_value(BUZZER_PIN, 0);
  pthread_mutex_unlock(&buzzer_mutex);
}

void buzzer_cleanup(void) {
  buzzer_off();
  unexport_gpio(BUZZER_PIN);
  printf("부저 정리 완료\n");
}
