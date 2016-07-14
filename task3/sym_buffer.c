#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

static DECLARE_WAIT_QUEUE_HEAD(write_qh);
static DECLARE_WAIT_QUEUE_HEAD(read_qh);

static int sbd_count = 1;
static dev_t sbd_dev = MKDEV(201, 201);
#define BUF_SIZE 128

#define PREV_WROTE 0
#define PREV_READ 1

static struct cdev sbd_cdev;
static struct sym_buffer
{
	char buf[BUF_SIZE];
	char *read;
	char *write;
	int status;
} sym_buf;

static char * get_next_pos (char * p) 
{
	if (p == sym_buf.buf+BUF_SIZE) {
		return sym_buf.buf;
	} else {
		return p + 1;
	}
}

static int get_available (char * p) 
{
	//printk(KERN_INFO "get avail r-%p w-%p b-%p\n", sym_buf.read, sym_buf.write, sym_buf.buf);
	if (p == sym_buf.read && p < sym_buf.write) {
		return sym_buf.write - p - 1;
	} else if (p == sym_buf.write && p < sym_buf.read) {
		return sym_buf.read - p - 1;
	} else {
		return BUF_SIZE - (p - sym_buf.buf);
	}
	
}

static ssize_t sbd_read(struct file *file, char __user * buf, size_t count, loff_t * ppos)
{
	int available;

	//printk(KERN_INFO "Read %d %d\n", (int) count, sym_buf.status);

	if (sym_buf.status != PREV_WROTE) {
		wait_event_interruptible(read_qh, sym_buf.status == PREV_WROTE);
	}

	available = get_available(sym_buf.read);
	//printk(KERN_INFO "Available to read %d\n", (int) available);
	if (available == 0) {

		wait_event_interruptible(read_qh, (available = get_available(sym_buf.read)) > 0);
	}

	if (count < available) {
		available = count;
	}

	//printk(KERN_INFO "Available again to read %d\n", (int) available);

	/*char test[BUF_SIZE+1];
	strncpy(test, sym_buf.read, available);
	test[available] = '\0';
	printk(KERN_INFO "String:%s\n", test);*/

	copy_to_user(buf, sym_buf.read, available);
	sym_buf.read += available;
	sym_buf.read = get_next_pos(sym_buf.read);
	sym_buf.status = PREV_READ;

	wake_up_interruptible(&write_qh);

	return available;
}

static ssize_t sbd_write(struct file *file, const char __user *buf, size_t count,
	loff_t *ppos)
{
	int available;

	//printk(KERN_INFO "Write %d %d\n", (int) count, sym_buf.status);

	if (sym_buf.status != PREV_READ) {
		wait_event_interruptible(write_qh, sym_buf.status == PREV_READ);
	}

	available = get_available(sym_buf.write);
	//printk(KERN_INFO "Available to write %d\n", (int) available);
	if (available == 0) {
		wait_event_interruptible(write_qh, (available = get_available(sym_buf.write)) > 0);
	}

	if (count < available) {
		available = count;
	}
	//printk(KERN_INFO "Available again to write %d\n", (int) available);


	copy_from_user(sym_buf.write, buf, available);

	/*char test[BUF_SIZE+1];
	strncpy(test, sym_buf.write, available);
	test[available] = '\0';
	printk(KERN_INFO "writed string:%s\n", test);*/

	sym_buf.write += available;
	sym_buf.write = get_next_pos(sym_buf.write);
	sym_buf.status = PREV_WROTE;

	wake_up_interruptible(&read_qh);

	return available;
}

static int sbd_open(struct inode *a, struct file *b)
{
	return 0;
}

static int sbd_release (struct inode *a, struct file *b)
{
	return 0;
}

struct file_operations sbd_fops = {
	.owner = THIS_MODULE,
	.read = sbd_read,
	.write = sbd_write,
	.release = sbd_release,
	.open = sbd_open,
};

static int __init sbd_init(void)
{
	int err;
	
	if (register_chrdev_region(sbd_dev, sbd_count, "rtsoftdev")) {
		err = -ENODEV;
	}

	cdev_init(&sbd_cdev, &sbd_fops);

	if (cdev_add(&sbd_cdev, sbd_dev, sbd_count)) {
		err = -ENODEV;
	}

	sym_buf.read = sym_buf.write = sym_buf.buf;
	sym_buf.status = PREV_READ;

	printk(KERN_INFO "Symbol block driver started\n");

	return 0;

}

static void __exit sbd_exit(void)
{
	printk(KERN_INFO "Symbol block driver exit\n");
	cdev_del(&sbd_cdev);
	unregister_chrdev_region(sbd_dev, sbd_count);
}

module_init(sbd_init);
module_exit(sbd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Symbol block driver");
MODULE_AUTHOR("Alguien");