#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>
#include "config.h"
#include "gate_controller.h"

static GateState current_state = GATE_OPEN;
static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;

void gate_controller_init(void) {
  wiringPiSetupGpio();

  // LED 설정
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 서보 PWM 설정 (BCM 18)
  pinMode(SERVO_PIN, PWM_OUTPUT);
  pwmSetMode(PWM_MODE_MS);
  pwmSetClock(PWM_CLOCK);
  pwmSetRange(PWM_RANGE);
  pwmWrite(SERVO_PIN, PWM_DUTY_OPEN);

  current_state = GATE_OPEN;
}

void gate_open(void) {
  pthread_mutex_lock(&gate_mutex);

  if (current_state == GATE_OPEN) {
    pthread_mutex_unlock(&gate_mutex);
    return;
  }

  pwmWrite(SERVO_PIN, PWM_DUTY_OPEN);
  usleep(SERVO_DELAY_MS * 1000);
  pwmWrite(SERVO_PIN, 0);

  current_state = GATE_OPEN;
  digitalWrite(LED_PIN, LOW);

  pthread_mutex_unlock(&gate_mutex);
}

void gate_close(void) {
  pthread_mutex_lock(&gate_mutex);

  if (current_state == GATE_CLOSED) {
    pthread_mutex_unlock(&gate_mutex);
    return;
  }

  pwmWrite(SERVO_PIN, PWM_DUTY_CLOSED);
  usleep(SERVO_DELAY_MS * 1000);
  pwmWrite(SERVO_PIN, 0);

  current_state = GATE_CLOSED;
  digitalWrite(LED_PIN, HIGH);

  pthread_mutex_unlock(&gate_mutex);
}

GateState gate_get_state(void) {
  pthread_mutex_lock(&gate_mutex);
  GateState state = current_state;
  pthread_mutex_unlock(&gate_mutex);
  return state;
}

void gate_controller_cleanup(void) {
  pwmWrite(SERVO_PIN, 0);
  digitalWrite(LED_PIN, LOW);
}
