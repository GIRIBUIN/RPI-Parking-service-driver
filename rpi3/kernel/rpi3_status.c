#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DEVICE_NAME "rpi3_status"
#define PROC_NAME "rpi3_status"
#define STATUS_BUF_SIZE 2048

static dev_t rpi3_status_dev;
static struct cdev rpi3_status_cdev;
static struct class *rpi3_status_class;
static struct device *rpi3_status_device;
static struct proc_dir_entry *rpi3_status_proc;
static DEFINE_MUTEX(status_lock);
static char status_buf[STATUS_BUF_SIZE] =
    "RPI3 Node Monitor\n"
    "rpi1: unknown\n"
    "rpi1_last_seen_sec: -\n"
    "rpi2: unknown\n"
    "rpi2_last_seen_sec: -\n";

static char *rpi3_status_devnode(struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;

    return NULL;
}

static ssize_t rpi3_status_write(struct file *file, const char __user *buf,
                                 size_t count, loff_t *ppos)
{
    size_t len = min(count, (size_t)(STATUS_BUF_SIZE - 1));

    mutex_lock(&status_lock);
    memset(status_buf, 0, sizeof(status_buf));

    if (copy_from_user(status_buf, buf, len)) {
        mutex_unlock(&status_lock);
        return -EFAULT;
    }

    status_buf[len] = '\0';
    mutex_unlock(&status_lock);

    return count;
}

static const struct file_operations rpi3_status_fops = {
    .owner = THIS_MODULE,
    .write = rpi3_status_write,
};

static int rpi3_status_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&status_lock);
    seq_printf(m, "%s", status_buf);
    mutex_unlock(&status_lock);

    return 0;
}

static int rpi3_status_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, rpi3_status_proc_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops rpi3_status_proc_ops = {
    .proc_open = rpi3_status_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations rpi3_status_proc_ops = {
    .owner = THIS_MODULE,
    .open = rpi3_status_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

static int __init rpi3_status_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&rpi3_status_dev, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&rpi3_status_cdev, &rpi3_status_fops);
    rpi3_status_cdev.owner = THIS_MODULE;

    ret = cdev_add(&rpi3_status_cdev, rpi3_status_dev, 1);
    if (ret)
        goto unregister_chrdev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    rpi3_status_class = class_create(DEVICE_NAME);
#else
    rpi3_status_class = class_create(THIS_MODULE, DEVICE_NAME);
#endif
    if (IS_ERR(rpi3_status_class)) {
        ret = PTR_ERR(rpi3_status_class);
        goto delete_cdev;
    }
    rpi3_status_class->devnode = rpi3_status_devnode;

    rpi3_status_device = device_create(rpi3_status_class, NULL,
                                       rpi3_status_dev, NULL, DEVICE_NAME);
    if (IS_ERR(rpi3_status_device)) {
        ret = PTR_ERR(rpi3_status_device);
        goto destroy_class;
    }

    rpi3_status_proc = proc_create(PROC_NAME, 0444, NULL, &rpi3_status_proc_ops);
    if (!rpi3_status_proc) {
        ret = -ENOMEM;
        goto destroy_device;
    }

    pr_info("rpi3_status loaded\n");
    return 0;

destroy_device:
    device_destroy(rpi3_status_class, rpi3_status_dev);
destroy_class:
    class_destroy(rpi3_status_class);
delete_cdev:
    cdev_del(&rpi3_status_cdev);
unregister_chrdev:
    unregister_chrdev_region(rpi3_status_dev, 1);
    return ret;
}

static void __exit rpi3_status_exit(void)
{
    proc_remove(rpi3_status_proc);
    device_destroy(rpi3_status_class, rpi3_status_dev);
    class_destroy(rpi3_status_class);
    cdev_del(&rpi3_status_cdev);
    unregister_chrdev_region(rpi3_status_dev, 1);
    pr_info("rpi3_status unloaded\n");
}

module_init(rpi3_status_init);
module_exit(rpi3_status_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("parking project");
MODULE_DESCRIPTION("RPI3 node heartbeat status interface");
