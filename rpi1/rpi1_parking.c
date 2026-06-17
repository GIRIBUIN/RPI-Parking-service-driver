#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/delay.h>

MODULE_LICENSE("GPL");

#define TRIG1   17
#define ECHO1   18
#define TRIG2   22
#define ECHO2   23
#define LED1     5
#define LED2     6

#define NUM_SPACES  2

#define OCCUPIED_THRESHOLD_CM   5
#define MEASURE_INTERVAL_MS     500

#define PARKING_IOC_MAGIC   'z'
#define PARKING_IOC_GET_STATUS  _IOR(PARKING_IOC_MAGIC, 1, struct parking_status)

#define DEV_NAME	"parking"

struct parking_status {
    int occupied[NUM_SPACES];   // 1=점유, 0=비어있음 
    int distance_cm[NUM_SPACES];
};

enum echo_state {
    ECHO_IDLE = 0,
    ECHO_WAIT_HIGH,     // TRIG 쏘고 echo 올라오길 기다림 
    ECHO_WAIT_LOW,      // echo HIGH -> LOW 기다림 
    ECHO_DONE,          // 측정 완료, workqueue 처리 대기 
};

static struct class *parking_class;
static struct device *parking_device;

struct sensor_dev {
    int     trig_pin;
    int     echo_pin;
    int     led_pin;
    int     irq_num;

    // ISR에서 쓰고 workqueue에서 읽음 -> spinlock 보호 
    spinlock_t          lock;
    enum echo_state     state;
    ktime_t             echo_start;
    ktime_t             echo_stop;

    // workqueue
    struct work_struct  work;

    // 최종 결과 (workqueue write, ioctl read)
    // atomic으로 lock-free 접근
    atomic_t    occupied;
    atomic_t    distance_cm;
};

static struct sensor_dev sensors[NUM_SPACES];

// 상태 변경 알림용 waitqueue (blocking read 지원) 
static wait_queue_head_t parking_wq;
static atomic_t          status_changed;    // 1이면 변경 있음

// char device 
static dev_t        dev_num;
static struct cdev *cd_cdev;

// 측정 주기 타이머 
static struct timer_list measure_timer;

// workqueue (dedicated: 순서 보장, 동시 실행 방지) 
static struct workqueue_struct *parking_wq_struct;

// GPIO 리소스 할당 테이블 (init/exit에서 일관성 유지) 
static const struct gpio gpio_list[] = {
    { TRIG1, GPIOF_OUT_INIT_LOW, "TRIG1" },
    { ECHO1, GPIOF_IN,           "ECHO1" },
    { LED1,  GPIOF_OUT_INIT_LOW, "LED1"  },
    { TRIG2, GPIOF_OUT_INIT_LOW, "TRIG2" },
    { ECHO2, GPIOF_IN,           "ECHO2" },
    { LED2,  GPIOF_OUT_INIT_LOW, "LED2"  },
};

// helper function
static void fire_trigger(int trig_pin)
{
    gpio_set_value(trig_pin, 1);
    udelay(10);
    gpio_set_value(trig_pin, 0);
}

// timer callback function
static void measure_timer_cb(struct timer_list *t)
{
    int i;
    unsigned long flags;

    for (i = 0; i < NUM_SPACES; i++) {
        spin_lock_irqsave(&sensors[i].lock, flags);

        if (sensors[i].state == ECHO_IDLE) {
            sensors[i].state = ECHO_WAIT_HIGH;
            spin_unlock_irqrestore(&sensors[i].lock, flags);

            fire_trigger(sensors[i].trig_pin);
        } else {
            spin_unlock_irqrestore(&sensors[i].lock, flags);
        }
    }

    // 타이머 재등록 
    mod_timer(&measure_timer,
              jiffies + msecs_to_jiffies(MEASURE_INTERVAL_MS));
}

// Bottom-half
static void sensor_work_func(struct work_struct *work)
{
    struct sensor_dev *sensor =
        container_of(work, struct sensor_dev, work);

    unsigned long flags;
    ktime_t start, stop;
    s64 time_us;
    int cm;
    int occupied;
    int prev_occupied;

    // ISR이 기록해 둔 시간을 안전하게 복사 
    spin_lock_irqsave(&sensor->lock, flags);
    start = sensor->echo_start;
    stop  = sensor->echo_stop;
    sensor->state = ECHO_IDLE;  // 다음 측정 허용 
    spin_unlock_irqrestore(&sensor->lock, flags);

    // 거리 계산: time(us) / 58 = cm (음속 340m/s 기준)
    time_us = ktime_to_us(ktime_sub(stop, start));

    // 비정상 값 필터링 (초음파 최대 측정 400cm ≈ 23200 us) 
    if (time_us <= 0 || time_us > 23200) {
        // 측정 실패 -> 상태 유지, 재측정 대기
        return;
    }

    cm = (int)(time_us / 58);
    occupied = (cm <= OCCUPIED_THRESHOLD_CM) ? 1 : 0;

    prev_occupied = atomic_read(&sensor->occupied);

    // 상태 업데이트
    atomic_set(&sensor->distance_cm, cm);
    atomic_set(&sensor->occupied, occupied);

    // LED 제어: 점유=ON, 비어있음=OFF
    gpio_set_value(sensor->led_pin, occupied);

    // 상태가 바뀌었을 때만 유저스페이스 알림 
    if (prev_occupied != occupied) {
        atomic_set(&status_changed, 1);
        wake_up_interruptible(&parking_wq);
        printk(KERN_INFO "parking: space changed -> occupied=%d, dist=%dcm\n",
               occupied, cm);
    }
}

// Top-half ISR
static irqreturn_t echo_isr(int irq, void *dev_id)
{
    struct sensor_dev *sensor = (struct sensor_dev *)dev_id;
    ktime_t now = ktime_get();
    int pin_val = gpio_get_value(sensor->echo_pin);

    // spinlock: state/ktime 보호 
    spin_lock(&sensor->lock);

    switch (sensor->state) {
    case ECHO_WAIT_HIGH:
        if (pin_val == 1) {
            sensor->echo_start = now;
            sensor->state = ECHO_WAIT_LOW;
        }
        break;

    case ECHO_WAIT_LOW:
        if (pin_val == 0) {
            sensor->echo_stop = now;
            sensor->state = ECHO_DONE;
            // bottom-half 스케줄 
            queue_work(parking_wq_struct, &sensor->work);
        }
        break;

    default:
        // spurious interrupt
        break;
    }

    spin_unlock(&sensor->lock);
    return IRQ_HANDLED;
}

static int parking_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int parking_release(struct inode *inode, struct file *file)
{
    return 0;
}

// MQTT daemon blocking read(parking)
static ssize_t parking_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    char kbuf[128];
    int len;
    int ret;

	// wait state changes
    if (!(file->f_flags & O_NONBLOCK)) {
        ret = wait_event_interruptible(parking_wq,
                    atomic_read(&status_changed) != 0);
        if (ret)
            return -ERESTARTSYS;
    } else {
        if (!atomic_read(&status_changed))
            return -EAGAIN;
    }

    atomic_set(&status_changed, 0);

    len = snprintf(kbuf, sizeof(kbuf), "Slot1:%d Slot2:%d\nDistance1:%d Distance2:%d\n",
                   atomic_read(&sensors[0].occupied),
                   atomic_read(&sensors[1].occupied),
                   atomic_read(&sensors[0].distance_cm),
                   atomic_read(&sensors[1].distance_cm));

    if (count < (size_t)len)
        return -EINVAL;

    if (copy_to_user(buf, kbuf, len))
        return -EFAULT;

    return len;
}

// ioctl
static long parking_ioctl(struct file *file, unsigned int cmd,
                          unsigned long arg)
{
    struct parking_status status;
    int i;

    switch (cmd) {
    case PARKING_IOC_GET_STATUS:
        for (i = 0; i < NUM_SPACES; i++) {
            status.occupied[i]    = atomic_read(&sensors[i].occupied);
            status.distance_cm[i] = atomic_read(&sensors[i].distance_cm);
        }
        if (copy_to_user((void __user *)arg, &status, sizeof(status)))
            return -EFAULT;
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations parking_fops = {
    .owner          = THIS_MODULE,
    .open           = parking_open,
    .release        = parking_release,
    .read           = parking_read,
    .unlocked_ioctl = parking_ioctl,
};

static int __init parking_init(void)
{
    int ret;
    int i;

    printk(KERN_INFO "parking: Init module\n");

    // init sensor 0
    sensors[0].trig_pin = TRIG1;
    sensors[0].echo_pin = ECHO1;
    sensors[0].led_pin  = LED1;
    spin_lock_init(&sensors[0].lock);
    sensors[0].state    = ECHO_IDLE;
    atomic_set(&sensors[0].occupied,    0);
    atomic_set(&sensors[0].distance_cm, 999);
    INIT_WORK(&sensors[0].work, sensor_work_func);

    // init sensor 1
    sensors[1].trig_pin = TRIG2;
    sensors[1].echo_pin = ECHO2;
    sensors[1].led_pin  = LED2;
    spin_lock_init(&sensors[1].lock);
    sensors[1].state    = ECHO_IDLE;
    atomic_set(&sensors[1].occupied,    0);
    atomic_set(&sensors[1].distance_cm, 999);
    INIT_WORK(&sensors[1].work, sensor_work_func);

    // init waitqueue
    init_waitqueue_head(&parking_wq);
    atomic_set(&status_changed, 0);

    // request gpio
    ret = gpio_request_array(gpio_list, ARRAY_SIZE(gpio_list));
    if (ret) {
        printk(KERN_ERR "parking: gpio_request_array failed: %d\n", ret);
        return ret;
    }

    // create dedicated workqueue
    parking_wq_struct = create_singlethread_workqueue("parking_wq");
    if (!parking_wq_struct) {
        printk(KERN_ERR "parking: failed to create workqueue\n");
        ret = -ENOMEM;
        goto err_gpio;
    }

    // register IRQ
    for (i = 0; i < NUM_SPACES; i++) {
        sensors[i].irq_num = gpio_to_irq(sensors[i].echo_pin);
        ret = request_irq(sensors[i].irq_num,
                          echo_isr,
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                          "parking_echo",
                          &sensors[i]);
        if (ret) {
            printk(KERN_ERR "parking: request_irq failed for sensor %d: %d\n",
                   i, ret);
            // 이미 등록된 IRQ 해제 
            while (--i >= 0)
                free_irq(sensors[i].irq_num, &sensors[i]);
            goto err_wq;
        }
    }

    // alloc chrdev
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
    if (ret) {
        printk(KERN_ERR "parking: alloc_chrdev_region failed: %d\n", ret);
        goto err_irq;
    }

    cd_cdev = cdev_alloc();
    if (!cd_cdev) {
        ret = -ENOMEM;
        goto err_chrdev;
    }

    cdev_init(cd_cdev, &parking_fops);
    ret = cdev_add(cd_cdev, dev_num, 1);
    if (ret) {
        printk(KERN_ERR "parking: cdev_add failed: %d\n", ret);
        goto err_cdev;
    }
    
    parking_class = class_create(THIS_MODULE, "parking");
    
    if (IS_ERR(parking_class)) {
    	ret = PTR_ERR(parking_class);
    	goto err_cdev;
    }
    
    parking_device = device_create( 
    	parking_class,
    	NULL,
    	dev_num,
    	NULL,
    	"parking");
    	
    if (IS_ERR(parking_device)) {
    	ret = PTR_ERR(parking_device);
    	class_destroy(parking_class);
    	goto err_cdev;
    }

    // start timer
    timer_setup(&measure_timer, measure_timer_cb, 0);
    mod_timer(&measure_timer,
              jiffies + msecs_to_jiffies(MEASURE_INTERVAL_MS));

    printk(KERN_INFO "parking: ready. major=%d\n", MAJOR(dev_num));
    printk(KERN_INFO "parking: mknod /dev/%s c %d 0\n",
           DEV_NAME, MAJOR(dev_num));
    return 0;

	err_cdev:
		kfree(cd_cdev);
	err_chrdev:
		unregister_chrdev_region(dev_num, 1);
	err_irq:
		for (i = 0; i < NUM_SPACES; i++)
		    free_irq(sensors[i].irq_num, &sensors[i]);
	err_wq:
		destroy_workqueue(parking_wq_struct);
	err_gpio:
		gpio_free_array(gpio_list, ARRAY_SIZE(gpio_list));
    return ret;
}

static void __exit parking_exit(void)
{
    int i;

    printk(KERN_INFO "parking: Exit module\n");

    // 타이머 먼저 중지 (새 ISR 방지)
    del_timer_sync(&measure_timer);

    // IRQ 해제
    for (i = 0; i < NUM_SPACES; i++)
        free_irq(sensors[i].irq_num, &sensors[i]);

    // workqueue 완전히 비운 후 파괴
    flush_workqueue(parking_wq_struct);
    destroy_workqueue(parking_wq_struct);

    // 잠자는 read() 깨워서 종료 
    atomic_set(&status_changed, 1);
    wake_up_interruptible(&parking_wq);

	device_destroy(parking_class, dev_num);
	class_destroy(parking_class);
	
    // char device 해제 
    cdev_del(cd_cdev);
    unregister_chrdev_region(dev_num, 1);

    // LED 끄기 
    gpio_set_value(LED1, 0);
    gpio_set_value(LED2, 0);

    // GPIO 해제 
    gpio_free_array(gpio_list, ARRAY_SIZE(gpio_list));

    printk(KERN_INFO "parking: cleaned up\n");
}

module_init(parking_init);
module_exit(parking_exit);


