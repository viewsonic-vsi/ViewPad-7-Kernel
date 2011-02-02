/*
 *  linux/fs/proc/eventlog.c
 *
 *  Copyright (C) 2009  by FIHTDC JamesKCTung
 *
 *  Modfify by : JamesKCTung
 *  Modfify date : 2009, 12, 17
 *  Description : Add write interface
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/eventlog.h>

#include "internal.h"

#ifdef __FIH_EVENT_LOG__
extern wait_queue_head_t eventlog_wait;
extern int do_eventlog(int type, char __user *bug, int count);

static int eventlog_open(struct inode * inode, struct file * file)
{
	int ret = 0;
	
	ret = do_eventlog(1,NULL,0);
	return ret;
}

static int eventlog_release(struct inode * inode, struct file * file)
{
	(void) do_eventlog(0,NULL,0);

	return 0;
}

static ssize_t eventlog_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	int ret = 0;
	
	if ((file->f_flags & O_NONBLOCK) && !do_eventlog(9, NULL, 0))
		return -EAGAIN;
	ret = do_eventlog(2, buf, count);

	return ret;
}

static ssize_t eventlog_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	int ret;
	char * data;
	
	if (count <= 0)
		return -EINVAL;

	data = kmalloc(count, GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	ret = -EFAULT;
	if (copy_from_user(data, buf, count))
		goto out;

//	printk("[%s]James - %s\n", __func__, data);
	ret = eventlog("%s", data);

//	printk("[%s]James - ret=%d count=%d\n", __func__, ret, count);
	if(ret <= 0)
		ret = -EFAULT;
	
out:
	kfree(data);
	return ret;
}

static unsigned int eventlog_poll(struct file *file, poll_table *wait)
{
	int mask=0;

	poll_wait(file, &eventlog_wait, wait);
	if (do_eventlog(9, NULL, 0))
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

const struct file_operations proc_eventlog_operations = {
	.read		= eventlog_read,
	.write		= eventlog_write,
	.poll			= eventlog_poll,
	.open		= eventlog_open,
	.release		= eventlog_release,
};
#endif

static int __init proc_eventlog_init(void)
{
#ifdef __FIH_EVENT_LOG__
	proc_create("eventlog", S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, NULL, &proc_eventlog_operations);
#endif
	return 0;
}
module_init(proc_eventlog_init);
