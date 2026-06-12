#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

#define BUF_SIZE 51
static struct proc_dir_entry *parent_proc;

static int count = 0;
static char user_input[BUF_SIZE] = "";

static void count_spaces(void){
    // 공백 마다 카운트 하는 걸로 구현(시작, 끝, 공백 여러개 고려 안함)
    int tmp = 0;
    for(int i = 0; i < strlen(user_input); i++) {
        if(user_input[i] == ' ') {
            tmp++;
        }
    }
    count = tmp;
}

// str 개수랑, 공백 개수 보여주기
static int proc_info_show(struct seq_file *seq, void *v) {
    seq_printf(seq, "str(%2ld): %s\n", strlen(user_input), user_input);
    seq_printf(seq, "count  : %d\n", count);
    return 0;
}

// proc 파일이 open될 때 proc info show 호출
static int proc_info_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_info_show, NULL);
}

// proc 파일에 데이터를 쓸 때 호출되는 함수. user_input에 문자열 저장
static ssize_t proc_info_write(struct file *file, const char __user *ubuf, size_t ubuf_len, loff_t *pos) {
    char buf[BUF_SIZE];
    size_t copy_len = (ubuf_len >= BUF_SIZE) ? BUF_SIZE - 1 : ubuf_len; // 최대 문자열 50 btyes 제한

    if (copy_from_user(buf, ubuf, copy_len))
        return -1;
    
    buf[copy_len] = '\0';

    // buf[strcspn(buf, "\n")] = '\0'; // 강의자료 39개 문자열이라 널 문자 제거 안한 것 같아서 주석처리

    memset(user_input, 0, sizeof(user_input));
    strncpy(user_input, buf, sizeof(user_input) - 1);
    
    count_spaces();
    
    buf[strcspn(buf, "\n")] = '\0'; // 출력용으로
    printk("[ch11] user input: '%s', count: %d\n", buf, count);

    return ubuf_len;
}


static const struct proc_ops proc_info_fops = {
    .proc_open    = proc_info_open,
    .proc_read    = seq_read,
    .proc_write   = proc_info_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};


static int __init simple_proc_init(void) {
    parent_proc = proc_mkdir("ch11_proc_dir", NULL);
    proc_create("proc_info", 0666, parent_proc, &proc_info_fops);
    return 0;
}

static void __exit simple_proc_exit(void) {
    proc_remove(parent_proc);
}

module_init(simple_proc_init);
module_exit(simple_proc_exit);