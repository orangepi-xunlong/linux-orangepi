#include <linux/slab.h>
#include <linux/version.h>
#include <proc/vi_isp_proc.h>

#define NO_PROC_ISP
#define ISP_PRC_NAME		"soph/isp"
#define PROC_READ_TIMEOUT	1000

struct isp_prc_ctrl {
	u8			*isp_buff_str;
	spinlock_t		isp_proc_lock;
	u32			isp_prc_int_flag;
	wait_queue_head_t	isp_prc_wait_q;
} isp_prc_unit;

/*************************************************************************
 *	ISP proc functions
 *************************************************************************/

int isp_proc_set_proc_content(void *buffer, size_t count)
{
	unsigned long flags;
	ssize_t rc = count;

	isp_prc_unit.isp_prc_int_flag = 1;
	spin_lock_irqsave(&isp_prc_unit.isp_proc_lock, flags);

	wake_up(&isp_prc_unit.isp_prc_wait_q);
	if (isp_prc_unit.isp_buff_str != NULL) {
		kfree(isp_prc_unit.isp_buff_str);
		isp_prc_unit.isp_buff_str = NULL;
	}
	isp_prc_unit.isp_buff_str = kmalloc(count, GFP_ATOMIC);
	memset(isp_prc_unit.isp_buff_str, 0, count);
	if (copy_from_user(isp_prc_unit.isp_buff_str, buffer, count) != 0)
		rc = -1;

	spin_unlock_irqrestore(&isp_prc_unit.isp_proc_lock, flags);

	return rc;
}

static int _isp_proc_show(struct seq_file *m, void *v)
{
	unsigned long flags;

	int ret = 0;

	ret = wait_event_timeout(isp_prc_unit.isp_prc_wait_q,
				isp_prc_unit.isp_prc_int_flag != 0,
				msecs_to_jiffies(PROC_READ_TIMEOUT));
	if (!ret) {
		pr_err("isp proc write timeout(%dms)\n", PROC_READ_TIMEOUT);
	} else {
		spin_lock_irqsave(&isp_prc_unit.isp_proc_lock, flags);

		seq_puts(m, "[ISP INFO]\n");
		seq_printf(m, "%s\n", isp_prc_unit.isp_buff_str);

		spin_unlock_irqrestore(&isp_prc_unit.isp_proc_lock, flags);
	}

	return 0;
}

static int _isp_proc_open(struct inode *inode, struct file *file)
{
	struct sop_vi_dev *vdev = PDE_DATA(inode);

	isp_prc_unit.isp_prc_int_flag = 0;
	vi_event_queue(vdev, VI_EVENT_ISP_PROC_READ, 0);

	return single_open(file, _isp_proc_show, PDE_DATA(inode));
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops _isp_proc_fops = {
	.proc_open = _isp_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _isp_proc_fops = {
	.owner = THIS_MODULE,
	.open = _isp_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif
int isp_proc_init(struct sop_vi_dev *_vdev)
{
	int rc = 0;
#ifndef NO_PROC_ISP
	/* create the /proc file */
	if (proc_create_data(ISP_PRC_NAME, 0644, NULL, &_isp_proc_fops, _vdev) == NULL) {
		pr_err("isp proc creation failed\n");
		rc = -1;
	}

	isp_prc_unit.isp_prc_int_flag = 0;
	init_waitqueue_head(&isp_prc_unit.isp_prc_wait_q);
	spin_lock_init(&isp_prc_unit.isp_proc_lock);
#endif
	return rc;
}

int isp_proc_remove(void)
{
#ifndef NO_PROC_ISP
	if (isp_prc_unit.isp_buff_str != NULL)
		kfree(isp_prc_unit.isp_buff_str);

	remove_proc_entry(ISP_PRC_NAME, NULL);
#endif
	return 0;
}
