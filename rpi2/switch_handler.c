#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <wiringPi.h>
#include "config.h"
#include "switch_handler.h"

static SwitchCallback switch_callback = NULL;
static uint64_t last_press_time = 0;

static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

static void switch_isr(void) {
  uint64_t now = get_time_ms();
  uint64_t elapsed = now - last_press_time;

  if (elapsed < DEBOUNCE_MS) {
    return;
  }

  last_press_time = now;

  if (switch_callback) {
    switch_callback();
  }
}

void switch_handler_init(SwitchCallback on_press) {
  switch_callback = on_press;

  pinMode(SWITCH_PIN, INPUT);
  pullUpDnControl(SWITCH_PIN, PUD_UP);

  wiringPiISR(SWITCH_PIN, INT_EDGE_FALLING, switch_isr);
}

void switch_handler_cleanup(void) {
  // wiringPi ISR은 자동 정리됨
}
