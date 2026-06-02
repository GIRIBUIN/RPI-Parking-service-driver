#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/time64.h>
#include "config.h"
#include "gate_node.h"

#define DEVICE_NAME "gate_node"
#define CLASS_NAME "gate"
#define DEVICE_BUF_SIZE 256

// 디바이스 드라이버 데이터
static dev_t dev_num;
static struct cdev gate_cdev;
static struct class *gate_class;
static struct device *gate_device;

// 상태 머신 및 플래그
static DEFINE_MUTEX(state_mutex);
static SystemState current_state = STATE_IDLE;
static int capacity_full = 0;

// 상태 머신 스레드
static struct task_struct *state_thread;

// 마지막 이벤트 시간 (초 단위)
static time64_t last_vehicle_detect = 0;
static time64_t exit_request_time = 0;

// 부저 페이즈 (0~9 순환, 0~4:on, 5~9:off)
static int buzzer_phase = 0;

SystemState gate_get_state(void) {
  SystemState state;

  mutex_lock(&state_mutex);
  state = current_state;
  mutex_unlock(&state_mutex);

  return state;
}

void gate_set_state(SystemState state) {
  mutex_lock(&state_mutex);
  current_state = state;
  mutex_unlock(&state_mutex);
}

int get_capacity_full(void) {
  int full;

  mutex_lock(&state_mutex);
  full = capacity_full;
  mutex_unlock(&state_mutex);

  return full;
}

void set_capacity_full(int full) {
  mutex_lock(&state_mutex);
  capacity_full = full;
  if (full) {
    full_led_set(1);
  } else {
    full_led_set(0);
  }
  mutex_unlock(&state_mutex);

  pr_info("Capacity %s\n", full ? "FULL" : "AVAILABLE");
}

void on_switch_press(void) {
  SystemState state = gate_get_state();

  if (state == STATE_IDLE) {
    pr_info("[SWITCH] 출차 요청 — 게이트 OPEN, 10초 타이머 시작\n");
    gate_open();
    exit_request_time = ktime_get_seconds();
    gate_set_state(STATE_EXIT_REQUESTED);
  }
}

static int state_machine_thread_fn(void *data) {
  int distance_cm;
  time64_t now;
  int elapsed;
  SystemState state;

  pr_info("State machine thread started\n");

  while (!kthread_should_stop()) {  // BUG-5: kthread_should_stop() 사용
    // Critical-1: 스위치 폴링 (IRQ 핸들러는 atomic 플래그만 세팅)
    if (switch_was_pressed()) {
      on_switch_press();
    }

    state = gate_get_state();

    distance_cm = ultrasonic_measure_cm();
    now = ktime_get_seconds();

    switch (state) {
    case STATE_IDLE:
      if (distance_cm > 0 && distance_cm <= VEHICLE_DETECT_CM) {
        pr_info("[%d cm] 입차 차량 감지 — 게이트 OPEN\n", distance_cm);
        gate_set_state(STATE_ENTRY_DETECTED);  // BUG-4: 상태 먼저 변경
        gate_open();
        last_vehicle_detect = now;
      } else if (distance_cm > 0) {
        pr_debug("[IDLE] 거리: %d cm\n", distance_cm);
      }
      break;

    case STATE_ENTRY_DETECTED:
      if (distance_cm > 0 && distance_cm <= VEHICLE_DETECT_CM) {
        pr_debug("[ENTRY] 거리: %d cm\n", distance_cm);
        last_vehicle_detect = now;
      } else if (distance_cm > VEHICLE_DETECT_CM) {
        pr_info("[%d cm] 차량 사라짐 — 5초 타이머 시작\n", distance_cm);
        last_vehicle_detect = now;
        gate_set_state(STATE_ENTRY_WAITING);
      }
      break;

    case STATE_ENTRY_WAITING:
      elapsed = now - last_vehicle_detect;
      if (elapsed >= ENTRY_CLOSE_DELAY_SEC) {
        pr_info("5초 경과 — 게이트 닫음\n");
        gate_close();
        gate_set_state(STATE_IDLE);
      } else if (distance_cm > 0 && distance_cm <= VEHICLE_DETECT_CM) {
        pr_info("[%d cm] 재진입 감지 — 게이트 열림 상태 유지\n", distance_cm);
        last_vehicle_detect = now;
        gate_set_state(STATE_ENTRY_DETECTED);
      }
      break;

    case STATE_EXIT_REQUESTED:
      if (distance_cm > 0 && distance_cm <= VEHICLE_DETECT_CM) {
        pr_info("[%d cm] 출차 차량 감지\n", distance_cm);
        gate_set_state(STATE_EXIT_VEHICLE_DETECTED);  // BUG-4: 상태 먼저 변경
        last_vehicle_detect = now;
      } else {
        elapsed = now - exit_request_time;
        if (elapsed >= EXIT_TIMEOUT_SEC) {
          pr_info("10초 경과 (차량 미감지) — 게이트 닫음 (타임아웃)\n");
          gate_close();
          gate_set_state(STATE_IDLE);  // BUG-4: 상태 먼저 변경
        }
      }
      break;

    case STATE_EXIT_VEHICLE_DETECTED:
      if (distance_cm > 0 && distance_cm <= VEHICLE_DETECT_CM) {
        pr_debug("[EXIT] 거리: %d cm\n", distance_cm);
        last_vehicle_detect = now;
      } else if (distance_cm > VEHICLE_DETECT_CM) {
        pr_info("[%d cm] 차량 빠져나감 — 출차 완료, 게이트 닫음\n", distance_cm);
        gate_close();
        gate_set_state(STATE_IDLE);
      }

      elapsed = now - exit_request_time;
      if (elapsed >= EXIT_TIMEOUT_SEC) {
        pr_info("10초 경과 (차량 미감지) — 게이트 닫음 (타임아웃)\n");
        gate_close();
        gate_set_state(STATE_IDLE);
      }
      break;
    }

    // 부저 패턴 처리 (상태에 따라 0.5초 on/off 반복 또는 OFF)
    // LED도 동시에 깜빡임 (완벽한 동기화)
    state = gate_get_state();
    if (state != STATE_IDLE) {
      buzzer_phase = (buzzer_phase + 1) % 10;
      if (buzzer_phase < 5) {
        buzzer_on();
        entry_led_set(1);
      } else {
        buzzer_off();
        entry_led_set(0);
      }
    } else {
      buzzer_phase = 0;
      buzzer_off();
      entry_led_set(0);
    }

    msleep(100);  // 100ms 주기로 상태 체크
  }

  buzzer_off();
  entry_led_set(0);
  pr_info("State machine thread stopped\n");
  return 0;
}

// Character Device: write (mqtt_bridge → /dev/gate_node)
static ssize_t gate_device_write(struct file *filp, const char __user *buf,
                                 size_t len, loff_t *off) {
  char local_buf[DEVICE_BUF_SIZE];
  size_t copy_len = min(len, sizeof(local_buf) - 1);

  if (copy_from_user(local_buf, buf, copy_len)) {
    return -EFAULT;
  }

  local_buf[copy_len] = '\0';

  // 공백 및 개행 제거
  while (copy_len > 0 && (local_buf[copy_len - 1] == '\n' ||
                          local_buf[copy_len - 1] == ' ')) {
    copy_len--;
    local_buf[copy_len] = '\0';
  }

  if (strcmp(local_buf, "FULL") == 0) {
    set_capacity_full(1);
  } else if (strcmp(local_buf, "AVAILABLE") == 0) {
    set_capacity_full(0);
  } else {
    pr_warn("Unknown command: %s\n", local_buf);
  }

  return len;
}

static const struct file_operations gate_fops = {
  .write = gate_device_write,
};

static int gate_node_init(void) {
  int ret;

  pr_info("Gate Node Module Loading\n");

  // Cdev 초기화
  ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
  if (ret < 0) {
    pr_err("Failed to allocate device number\n");
    return ret;
  }

  cdev_init(&gate_cdev, &gate_fops);
  ret = cdev_add(&gate_cdev, dev_num, 1);
  if (ret < 0) {
    pr_err("Failed to add cdev\n");
    unregister_chrdev_region(dev_num, 1);
    return ret;
  }

  // Device class 생성
  gate_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(gate_class)) {
    pr_err("Failed to create class\n");
    cdev_del(&gate_cdev);
    unregister_chrdev_region(dev_num, 1);
    return PTR_ERR(gate_class);
  }

  // Device 생성
  gate_device = device_create(gate_class, NULL, dev_num, NULL, DEVICE_NAME);
  if (IS_ERR(gate_device)) {
    pr_err("Failed to create device\n");
    class_destroy(gate_class);
    cdev_del(&gate_cdev);
    unregister_chrdev_region(dev_num, 1);
    return PTR_ERR(gate_device);
  }

  // 하드웨어 초기화
  ret = gate_hw_init();
  if (ret) {
    pr_err("Failed to initialize hardware\n");
    device_destroy(gate_class, dev_num);
    class_destroy(gate_class);
    cdev_del(&gate_cdev);
    unregister_chrdev_region(dev_num, 1);
    return ret;
  }

  // 상태 머신 스레드 생성
  state_thread = kthread_run(state_machine_thread_fn, NULL, "gate_state_machine");
  if (IS_ERR(state_thread)) {
    pr_err("Failed to create state machine thread\n");
    gate_hw_cleanup();
    device_destroy(gate_class, dev_num);
    class_destroy(gate_class);
    cdev_del(&gate_cdev);
    unregister_chrdev_region(dev_num, 1);
    return PTR_ERR(state_thread);
  }

  pr_info("Gate Node Module Loaded Successfully\n");
  pr_info("Device: /dev/%s, Major: %d, Minor: %d\n", DEVICE_NAME, MAJOR(dev_num),
          MINOR(dev_num));

  return 0;
}

static void gate_node_exit(void) {
  pr_info("Gate Node Module Unloading\n");

  // 상태 머신 스레드 중지 (BUG-5: kthread_should_stop 신호)
  if (state_thread && !IS_ERR(state_thread)) {
    kthread_stop(state_thread);
  }

  // 하드웨어 정리
  gate_hw_cleanup();

  // 디바이스 정리
  device_destroy(gate_class, dev_num);
  class_destroy(gate_class);
  cdev_del(&gate_cdev);
  unregister_chrdev_region(dev_num, 1);

  pr_info("Gate Node Module Unloaded\n");
}

module_init(gate_node_init);
module_exit(gate_node_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RPI Parking Service");
MODULE_DESCRIPTION("RPI2 Gate Control Module");
