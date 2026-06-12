    #include <linux/init.h>
    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/fs.h>
    #include <linux/gpio.h>
    #include <linux/interrupt.h>
    #include <linux/timer.h>
    #include <linux/delay.h>

    MODULE_LICENSE("GPL");

    #define SENSOR1 17
    #define LED1 5

    struct led_timer_info {
        struct timer_list timer;
        long delay_jiffies;
        int data;
    };

    static struct led_timer_info led_timer;
    static spinlock_t spinlock;
    static int irq_num;

    static irqreturn_t detect_handler(int irq, void* dev_id) {
        unsigned long flags;

        spin_lock_irqsave(&spinlock, flags);
        printk("ch7: Detect\n");
        if (led_timer.data == 1){
            gpio_set_value(LED1, 0);
            mdelay(300);
        }
        gpio_set_value(LED1, 1);
        led_timer.data = 1;
        mod_timer(&led_timer.timer, jiffies + led_timer.delay_jiffies);
        spin_unlock_irqrestore(&spinlock, flags);

        return IRQ_HANDLED;
    }

    static void led_timer_func(struct timer_list *t) {
        unsigned long flags;
        spin_lock_irqsave(&spinlock, flags);
        gpio_set_value(LED1, 0);
        led_timer.data = 0;
        spin_unlock_irqrestore(&spinlock, flags);
    }

    static int __init ch7_init(void) {
        int ret = 0;

        printk("ch7: Init module\n");

        spin_lock_init(&spinlock);

        led_timer.delay_jiffies = msecs_to_jiffies(10000);
        led_timer.data = 0;
        timer_setup(&led_timer.timer, led_timer_func, 0);

        gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "LED1");
        gpio_request_one(SENSOR1, GPIOF_IN, "SENSOR1");
        irq_num = gpio_to_irq(SENSOR1);
        ret = request_irq(irq_num, detect_handler, IRQF_TRIGGER_RISING, "detect_handler", NULL);
        if (ret) {
            printk("ch7: Unable to request IRQ : %d\n", irq_num);
            free_irq(irq_num, NULL);
        }

        return 0;
    }

    static void __exit ch7_exit(void){
        printk("ch7: Exit module \n");
        disable_irq(irq_num);
        free_irq(irq_num, NULL);
        del_timer_sync(&led_timer.timer);

        gpio_set_value(LED1, 0);
        gpio_free(SENSOR1);
        gpio_free(LED1);

    }

    module_init(ch7_init);
    module_exit(ch7_exit);