#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/timekeeping.h>

/*
 * 공유 자원:
 * - rpi3_status 구조체(msg_count, err_count, last_topic, last_event 등)
 *
 * 접근 하는 프로세스:
 * - mqtt_handler.py: /dev/rpi3_status에 write해서 status 갱신
 * - cat/monitoring process: /proc/rpi3_status를 read해서 status 확인
 *
 * lock 방식, 선택 이유:
 * - mutex 사용
 * - /dev write와 /proc read는 process context --> sleep 해도 되니까 spinlock말고 mutex
 */
MODULE_LICENSE("GPL");

#define DEV_NAME "rpi3_status"
#define PROC_NAME "rpi3_status"
#define BUF_SIZE 256
#define FIELD_SIZE 128

struct parking_status {
    unsigned long msg_count;
    unsigned long err_count;
    char status[FIELD_SIZE];
    char last_topic[FIELD_SIZE];
    char last_event[FIELD_SIZE];
    char last_error[FIELD_SIZE];
    time64_t last_update;
};

static struct parking_status rpi3_status = {
    .status = "booting",
    .last_topic = "-",
    .last_event = "-",
    .last_error = "-",
};

static struct proc_dir_entry *status_proc;
static DEFINE_MUTEX(status_mutex);

static void save_str(char *dst, const char *src) {
    if (src && src[0])
        strncpy(dst, src, FIELD_SIZE - 1);
    else
        strncpy(dst, "-", FIELD_SIZE - 1);

    dst[FIELD_SIZE - 1] = '\0';
}

static void update_time(void) {
    rpi3_status.last_update = ktime_get_real_seconds();
}

// mqtt_handler.py가 /dev/rpi3_status에 쓴 문자열을 파싱
static void update_status(char *buf) {
    char *cmd;
    char *topic;
    char *data;

    strim(buf);
    cmd = strsep(&buf, " \t\n");
    if (!cmd || !cmd[0])
        return;

    mutex_lock(&status_mutex);

    if (strcmp(cmd, "MQTT_OK") == 0) {
        save_str(rpi3_status.status, "mqtt_connected");
        update_time();
    }
    else if (strcmp(cmd, "MQTT_ERROR") == 0) {
        rpi3_status.err_count++;
        save_str(rpi3_status.status, "mqtt_error");
        save_str(rpi3_status.last_error, buf);
        update_time();
    }
    else if (strcmp(cmd, "MSG") == 0) {
        topic = strsep(&buf, " \t\n");
        data = strsep(&buf, "\n");

        rpi3_status.msg_count++;
        save_str(rpi3_status.status, "running");
        save_str(rpi3_status.last_topic, topic);
        if (data && data[0])
            save_str(rpi3_status.last_event, data);
        update_time();
    }
    else if (strcmp(cmd, "ERROR") == 0) {
        rpi3_status.err_count++;
        save_str(rpi3_status.status, "handler_error");
        save_str(rpi3_status.last_error, buf);
        update_time();
    }

    mutex_unlock(&status_mutex);
}

// /dev/rpi3_status write
static ssize_t status_dev_write(struct file *file, const char __user *ubuf,
                                size_t ubuf_len, loff_t *pos) {
    char buf[BUF_SIZE];
    size_t copy_len = (ubuf_len >= BUF_SIZE) ? BUF_SIZE - 1 : ubuf_len;

    if (copy_from_user(buf, ubuf, copy_len))
        return -1;

    buf[copy_len] = '\0';
    update_status(buf);

    return ubuf_len;
}

// /proc/rpi3_status read
static int status_proc_show(struct seq_file *seq, void *v) {
    mutex_lock(&status_mutex);

    seq_printf(seq, "주차장 서버 상태\n");
    seq_printf(seq, "현재 상태: %s\n", rpi3_status.status);
    seq_printf(seq, "메시지 수신 카운트: %lu\n", rpi3_status.msg_count);
    seq_printf(seq, "오류 카운트: %lu\n", rpi3_status.err_count);
    seq_printf(seq, "마지막 토픽: %s\n", rpi3_status.last_topic);
    seq_printf(seq, "마지막 이벤트: %s\n", rpi3_status.last_event);
    seq_printf(seq, "마지막 오류: %s\n", rpi3_status.last_error);
    seq_printf(seq, "마지막 갱신 시간(epoch): %lld\n", rpi3_status.last_update);

    mutex_unlock(&status_mutex);
    return 0;
}

static int status_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, status_proc_show, NULL);
}

static const struct file_operations status_dev_fops = {
    .owner = THIS_MODULE,
    .write = status_dev_write,
};

static const struct proc_ops status_proc_fops = {
    .proc_open    = status_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static struct miscdevice status_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &status_dev_fops,
    .mode = 0666,
};

static int __init rpi3_status_init(void) {
    int ret;

    printk("rpi3_status: 모듈 로드\n");

    update_time();

    status_proc = proc_create(PROC_NAME, 0444, NULL, &status_proc_fops);
    if (!status_proc)
        return -ENOMEM;

    ret = misc_register(&status_dev);
    if (ret) {
        proc_remove(status_proc);
        return ret;
    }

    printk("rpi3_status: /dev/%s, /proc/%s 준비 완료\n", DEV_NAME, PROC_NAME);
    return 0;
}

static void __exit rpi3_status_exit(void) {
    printk("rpi3_status: 모듈 제거\n");

    misc_deregister(&status_dev);
    proc_remove(status_proc);
}

module_init(rpi3_status_init);
module_exit(rpi3_status_exit);
