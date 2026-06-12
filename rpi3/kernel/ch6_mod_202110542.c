#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");

#define SWITCH 12
#define LED1 5

struct my_timer_info{
    struct timer_list timer;
    long delay_jiffies;
    int data;
    int prev;
};

static struct my_timer_info my_timer;

static void my_timer_func(struct timer_list *t){
    struct my_timer_info *info = from_timer(info, t, timer);
    my_timer.data = gpio_get_value(SWITCH);
    if (my_timer.data == 1){
        gpio_set_value(LED1, 1);
    }
    else{
        gpio_set_value(LED1, 0);
    }
    if (info->data != info->prev){
        printk("ch6: %d -> %d\n", info->prev, info->data);
        info->prev = info->data;
    }
    mod_timer(&my_timer.timer, jiffies + info->delay_jiffies);
}

static int __init ch6_init(void){
    printk("ch6: init module \n");
    gpio_request_one(SWITCH, GPIOF_IN, "SWITCH");
    gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "LED1");

    my_timer.delay_jiffies = msecs_to_jiffies(500);
    my_timer.data = 0;
    my_timer.prev = 0;
    timer_setup(&my_timer.timer, my_timer_func, 0);
    my_timer.timer.expires = jiffies + my_timer.delay_jiffies;
    add_timer(&my_timer.timer);

    return 0;
}

static void __exit ch6_exit(void){
    printk("ch6: exit module \n");
    gpio_set_value(LED1, 0);
    gpio_free(SWITCH);
    gpio_free(LED1);
    del_timer(&my_timer.timer);
}

module_init(ch6_init);
module_exit(ch6_exit);