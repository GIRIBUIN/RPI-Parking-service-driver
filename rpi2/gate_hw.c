#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include "config.h"
#include "gate_node.h"

// 스테퍼 모터 하프스텝 시퀀스 (28BYJ-48 + ULN2003)
static const int stepper_sequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1},
};

// 모든 GPIO 핀 배열 (Critical-3: GPIO 추적용)
static int all_pins[] = {
  STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4,
  ENTRY_LED_PIN, EXIT_LED_PIN, BUZZER_PIN, FULL_LED_PIN,
  ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN, SWITCH_PIN
};
static bool gpio_allocated[11] = {false};  // 각 핀 할당 여부 추적

static spinlock_t hw_lock;
static int switch_irq = -1;
static unsigned long last_switch_time = 0;
static atomic_t switch_pressed = ATOMIC_INIT(0);  // Critical-1: atomic 플래그

// 스테퍼 현재 단계 (0~7 범위)
static int stepper_step = 0;

static void stepper_set_pins(int step) {
  step = step % 8;
  if (step < 0) step = (step % 8) + 8;

  gpio_set_value(STEPPER_IN1, stepper_sequence[step][0]);
  gpio_set_value(STEPPER_IN2, stepper_sequence[step][1]);
  gpio_set_value(STEPPER_IN3, stepper_sequence[step][2]);
  gpio_set_value(STEPPER_IN4, stepper_sequence[step][3]);
}

void gate_open(void) {
  unsigned long flags;
  int i;

  spin_lock_irqsave(&hw_lock, flags);

  // 1024 스텝 회전 (90도)
  for (i = 0; i < STEPPER_GATE_STEPS; i++) {
    stepper_step = (stepper_step + 1) % 8;
    stepper_set_pins(stepper_step);
    spin_unlock_irqrestore(&hw_lock, flags);
    usleep_range(STEPPER_STEP_DELAY_US, STEPPER_STEP_DELAY_US + 100);
    spin_lock_irqsave(&hw_lock, flags);
  }

  spin_unlock_irqrestore(&hw_lock, flags);
}

static void stepper_deenergize(void) {
  gpio_set_value(STEPPER_IN1, 0);
  gpio_set_value(STEPPER_IN2, 0);
  gpio_set_value(STEPPER_IN3, 0);
  gpio_set_value(STEPPER_IN4, 0);
}

void gate_close(void) {
  unsigned long flags;
  int i;

  spin_lock_irqsave(&hw_lock, flags);

  // -1024 스텝 회전 (역방향 90도)
  for (i = 0; i < STEPPER_GATE_STEPS; i++) {
    stepper_step = (stepper_step - 1) % 8;
    if (stepper_step < 0) stepper_step = (stepper_step % 8) + 8;
    stepper_set_pins(stepper_step);
    spin_unlock_irqrestore(&hw_lock, flags);
    usleep_range(STEPPER_STEP_DELAY_US, STEPPER_STEP_DELAY_US + 100);
    spin_lock_irqsave(&hw_lock, flags);
  }

  stepper_deenergize();  // Critical-2: 모든 핀 0 출력으로 완전 디에너지나이즈
  spin_unlock_irqrestore(&hw_lock, flags);
}

void buzzer_on(void) {
  unsigned long flags;

  spin_lock_irqsave(&hw_lock, flags);
  gpio_set_value(BUZZER_PIN, 0);
  spin_unlock_irqrestore(&hw_lock, flags);
}

void buzzer_off(void) {
  unsigned long flags;

  spin_lock_irqsave(&hw_lock, flags);
  gpio_set_value(BUZZER_PIN, 1);
  spin_unlock_irqrestore(&hw_lock, flags);
}

void entry_led_set(int on) {
  unsigned long flags;

  spin_lock_irqsave(&hw_lock, flags);
  gpio_set_value(ENTRY_LED_PIN, on ? 1 : 0);
  spin_unlock_irqrestore(&hw_lock, flags);
}

void exit_led_set(int on) {
  unsigned long flags;

  spin_lock_irqsave(&hw_lock, flags);
  gpio_set_value(EXIT_LED_PIN, on ? 1 : 0);
  spin_unlock_irqrestore(&hw_lock, flags);
}

void full_led_set(int on) {
  unsigned long flags;

  spin_lock_irqsave(&hw_lock, flags);
  gpio_set_value(FULL_LED_PIN, on ? 1 : 0);
  spin_unlock_irqrestore(&hw_lock, flags);
}

int ultrasonic_measure_cm(void) {
  unsigned long flags;
  ktime_t start, end;
  s64 duration_us;
  int distance_cm;
  int timeout_loops = 0;
  const int max_timeout = 30000;  // 30ms 타임아웃

  spin_lock_irqsave(&hw_lock, flags);

  // 트리거 LOW
  gpio_set_value(ULTRASONIC_TRIG_PIN, 0);
  udelay(2);

  // 트리거 HIGH
  gpio_set_value(ULTRASONIC_TRIG_PIN, 1);
  udelay(10);

  // 트리거 LOW
  gpio_set_value(ULTRASONIC_TRIG_PIN, 0);

  spin_unlock_irqrestore(&hw_lock, flags);

  // ECHO rising 대기
  timeout_loops = 0;
  while (gpio_get_value(ULTRASONIC_ECHO_PIN) == 0 && timeout_loops < max_timeout) {
    udelay(1);
    timeout_loops++;
  }

  if (timeout_loops >= max_timeout) {
    return -1;  // 타임아웃
  }

  start = ktime_get();

  // ECHO falling 대기
  timeout_loops = 0;
  while (gpio_get_value(ULTRASONIC_ECHO_PIN) == 1 && timeout_loops < max_timeout) {
    udelay(1);
    timeout_loops++;
  }

  end = ktime_get();

  if (timeout_loops >= max_timeout) {
    return -1;  // 타임아웃
  }

  duration_us = ktime_us_delta(end, start);
  distance_cm = (int)(duration_us / 58);

  return (distance_cm > 0) ? distance_cm : -1;
}

static irqreturn_t switch_irq_handler(int irq, void *dev_id) {
  unsigned long now = jiffies;

  // 디바운싱: 300ms 이내에는 무시
  if (time_before(now, last_switch_time + msecs_to_jiffies(DEBOUNCE_MS))) {
    return IRQ_HANDLED;
  }

  last_switch_time = now;

  // Critical-1: IRQ 핸들러에서는 atomic 플래그만 세팅
  // on_switch_press()는 kthread에서 폴링
  atomic_set(&switch_pressed, 1);

  return IRQ_HANDLED;
}

// Critical-1: kthread에서 호출할 스위치 상태 확인 함수
int switch_was_pressed(void) {
  return atomic_xchg(&switch_pressed, 0);
}

int gate_hw_init(void) {
  int ret;
  int i;
  int out_pins[] = {
    STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4,
    ENTRY_LED_PIN, EXIT_LED_PIN, BUZZER_PIN, FULL_LED_PIN,
    ULTRASONIC_TRIG_PIN
  };

  spin_lock_init(&hw_lock);

  // Critical-3: GPIO 요청 및 할당 여부 추적
  for (i = 0; i < ARRAY_SIZE(all_pins); i++) {
    ret = gpio_request(all_pins[i], "gate_node");
    if (ret) {
      pr_err("Failed to request GPIO %d\n", all_pins[i]);
      goto cleanup;
    }
    gpio_allocated[i] = true;
  }

  // 출력 GPIO 설정

  for (i = 0; i < ARRAY_SIZE(out_pins); i++) {
    ret = gpio_direction_output(out_pins[i], 0);
    if (ret) {
      pr_err("Failed to set GPIO %d as output\n", out_pins[i]);
      goto cleanup;
    }
  }

  // 부저만 Active Low이므로 초기값 1(OFF)로 설정
  gpio_set_value(BUZZER_PIN, 1);

  // 입력 GPIO 설정
  ret = gpio_direction_input(ULTRASONIC_ECHO_PIN);
  if (ret) {
    pr_err("Failed to set ECHO_PIN as input\n");
    goto cleanup;
  }

  ret = gpio_direction_input(SWITCH_PIN);
  if (ret) {
    pr_err("Failed to set SWITCH_PIN as input\n");
    goto cleanup;
  }

  // 스위치 IRQ 설정 (FALLING edge)
  switch_irq = gpio_to_irq(SWITCH_PIN);
  if (switch_irq < 0) {
    pr_err("Failed to get IRQ for SWITCH_PIN\n");
    ret = switch_irq;
    goto cleanup;
  }

  ret = request_irq(switch_irq, switch_irq_handler, IRQF_TRIGGER_FALLING,
                    "gate_switch", NULL);
  if (ret) {
    pr_err("Failed to request IRQ\n");
    goto cleanup;
  }

  pr_info("Gate hardware initialized successfully\n");
  return 0;

cleanup:
  // Critical-3: 초기화 중 실패 시 cleanup도 gate_hw_cleanup에서 처리
  return ret;
}

void gate_hw_cleanup(void) {
  int i;

  if (switch_irq >= 0) {
    free_irq(switch_irq, NULL);
  }

  // BCM GPIO는 gpio_free() 이후에도 마지막 핀 상태를 유지함
  // gpio_free 전에 모든 출력 핀을 정리해야 함 (부저는 Active Low이므로 1)
  gpio_set_value(BUZZER_PIN, 1);
  gpio_set_value(ENTRY_LED_PIN, 0);
  gpio_set_value(EXIT_LED_PIN, 0);
  gpio_set_value(FULL_LED_PIN, 0);
  gpio_set_value(STEPPER_IN1, 0);
  gpio_set_value(STEPPER_IN2, 0);
  gpio_set_value(STEPPER_IN3, 0);
  gpio_set_value(STEPPER_IN4, 0);
  gpio_set_value(ULTRASONIC_TRIG_PIN, 0);

  // Critical-3: 할당된 GPIO만 해제
  for (i = 0; i < ARRAY_SIZE(all_pins); i++) {
    if (gpio_allocated[i]) {
      gpio_free(all_pins[i]);
      gpio_allocated[i] = false;
    }
  }

  pr_info("Gate hardware cleaned up\n");
}
