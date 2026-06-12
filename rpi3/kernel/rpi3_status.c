#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "rpi3_status"
#define PROC_NAME "rpi3_status"
#define MAX_INPUT 256
#define MAX_FIELD 128

struct rpi3_status {
	unsigned long message_count;
	unsigned long error_count;
	char status[MAX_FIELD];
	char last_topic[MAX_FIELD];
	char last_event[MAX_FIELD];
	char last_error[MAX_FIELD];
	time64_t last_update;
};

static struct rpi3_status status_state = {
	.status = "booting",
	.last_topic = "-",
	.last_event = "-",
	.last_error = "-",
};
static DEFINE_MUTEX(status_lock);
static struct proc_dir_entry *status_proc;

static void set_field(char *dest, size_t size, const char *value)
{
	strscpy(dest, value && value[0] ? value : "-", size);
}

static void update_last_seen_locked(void)
{
	status_state.last_update = ktime_get_real_seconds();
}

static void handle_status_command(char *input)
{
	char *cmd;
	char *topic;
	char *event;
	char *error;

	strim(input);
	cmd = strsep(&input, " \t\n");
	if (!cmd || !cmd[0])
		return;

	mutex_lock(&status_lock);

	if (strcmp(cmd, "MQTT_OK") == 0) {
		set_field(status_state.status, sizeof(status_state.status), "mqtt_connected");
		update_last_seen_locked();
	} else if (strcmp(cmd, "MQTT_ERROR") == 0) {
		status_state.error_count++;
		set_field(status_state.status, sizeof(status_state.status), "mqtt_error");
		set_field(status_state.last_error, sizeof(status_state.last_error), input);
		update_last_seen_locked();
	} else if (strcmp(cmd, "MSG") == 0) {
		topic = strsep(&input, " \t\n");
		event = strsep(&input, "\n");

		status_state.message_count++;
		set_field(status_state.status, sizeof(status_state.status), "running");
		set_field(status_state.last_topic, sizeof(status_state.last_topic), topic);
		if (event && event[0])
			set_field(status_state.last_event, sizeof(status_state.last_event), event);
		update_last_seen_locked();
	} else if (strcmp(cmd, "ERROR") == 0) {
		error = input;

		status_state.error_count++;
		set_field(status_state.status, sizeof(status_state.status), "handler_error");
		set_field(status_state.last_error, sizeof(status_state.last_error), error);
		update_last_seen_locked();
	}

	mutex_unlock(&status_lock);
}

static ssize_t rpi3_status_write(struct file *file, const char __user *buf,
				 size_t len, loff_t *ppos)
{
	char *input;
	size_t copy_len;

	copy_len = min(len, (size_t)(MAX_INPUT - 1));
	input = kzalloc(MAX_INPUT, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	if (copy_from_user(input, buf, copy_len)) {
		kfree(input);
		return -EFAULT;
	}

	input[copy_len] = '\0';
	handle_status_command(input);
	kfree(input);

	return len;
}

static int rpi3_status_show(struct seq_file *m, void *v)
{
	mutex_lock(&status_lock);
	seq_puts(m, "RPI3 Parking Status\n");
	seq_printf(m, "status: %s\n", status_state.status);
	seq_printf(m, "message_count: %lu\n", status_state.message_count);
	seq_printf(m, "error_count: %lu\n", status_state.error_count);
	seq_printf(m, "last_topic: %s\n", status_state.last_topic);
	seq_printf(m, "last_event: %s\n", status_state.last_event);
	seq_printf(m, "last_error: %s\n", status_state.last_error);
	seq_printf(m, "last_update_epoch: %lld\n", status_state.last_update);
	mutex_unlock(&status_lock);

	return 0;
}

static int rpi3_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rpi3_status_show, NULL);
}

static const struct proc_ops rpi3_status_proc_ops = {
	.proc_open = rpi3_status_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct file_operations rpi3_status_fops = {
	.owner = THIS_MODULE,
	.write = rpi3_status_write,
};

static struct miscdevice rpi3_status_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &rpi3_status_fops,
	.mode = 0666,
};

static int __init rpi3_status_init(void)
{
	int ret;

	mutex_lock(&status_lock);
	update_last_seen_locked();
	mutex_unlock(&status_lock);

	status_proc = proc_create(PROC_NAME, 0444, NULL, &rpi3_status_proc_ops);
	if (!status_proc)
		return -ENOMEM;

	ret = misc_register(&rpi3_status_device);
	if (ret) {
		proc_remove(status_proc);
		return ret;
	}

	pr_info("rpi3_status loaded: /dev/%s, /proc/%s\n", DEVICE_NAME, PROC_NAME);
	return 0;
}

static void __exit rpi3_status_exit(void)
{
	misc_deregister(&rpi3_status_device);
	proc_remove(status_proc);
	pr_info("rpi3_status unloaded\n");
}

module_init(rpi3_status_init);
module_exit(rpi3_status_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RPI Parking Service");
MODULE_DESCRIPTION("RPI3 parking server status device");
