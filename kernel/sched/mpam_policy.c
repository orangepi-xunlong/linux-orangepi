// SPDX-License-Identifier: GPL-2.0+
/*
 * Module interface to map PIDs to  MPAM PARTIDs
 *
 * Copyright (C) 2021 Arm Ltd.
 */

#define DEBUG

#define pr_fmt(fmt) "MPAM_policy: " fmt

#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/kernfs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/user_namespace.h>

#include <linux/mpam_policy.h>
#include "sched.h"
#include "../../drivers/android/mpam_arch.h"

/*
 * This presents a virtual fs that looks something like:
 *
 * /sys/fs/mpam
 *   partitions/
 *     0/
 *       tasks
 *     1/
 *       tasks
 *    ...
 *     $(PARTID_COUNT-1)/
 *       tasks
 */

#define MPAM_POLICY_SUPER_MAGIC		0x677388

struct mpam_partition {
	unsigned int partid;
};

static struct {
	struct mpam_partition *partitions;
	struct mutex lock;
	unsigned int partitions_count;
} mpam_fs = {
	.lock = __MUTEX_INITIALIZER(mpam_fs.lock),
};

static int mpam_init_fs_context(struct fs_context *fc);
static void mpam_kill_sb(struct super_block *sb);
static int mpam_get_tree(struct fs_context *fc);

static int mpam_policy_tasks_open(struct inode *inode, struct file *file);
static ssize_t mpam_policy_tasks_write(struct file *filp, const char __user *ubuf,
				       size_t cnt, loff_t *ppos);

static void mpam_kick_task(struct task_struct *p);

static const struct fs_context_operations mpam_fs_context_ops = {
	.get_tree = mpam_get_tree,
};

static struct file_system_type mpam_fs_type = {
	.owner           = THIS_MODULE,
	.name            = "mpam",
	.init_fs_context = mpam_init_fs_context,
	.kill_sb         = mpam_kill_sb,
};

static const struct file_operations mpam_fs_tasks_ops = {
	.owner   = THIS_MODULE,
	.open    = mpam_policy_tasks_open,
	.write   = mpam_policy_tasks_write,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static inline unsigned int mpam_get_task_partid(struct task_struct *p)
{
	return READ_ONCE(p->partid);
}

static inline void mpam_set_task_partid(struct task_struct *p, unsigned int partid)
{
	WRITE_ONCE(p->partid, partid);
	mpam_kick_task(p);
}

static int mpam_init_fs_context(struct fs_context *fc)
{
	fc->ops = &mpam_fs_context_ops;
	put_user_ns(fc->user_ns);
	fc->user_ns = get_user_ns(&init_user_ns);
	fc->global = true;
	return 0;
}

static void mpam_kill_sb(struct super_block *sb)
{
	kfree(mpam_fs.partitions);
	kill_litter_super(sb);
}

static ssize_t mpam_policy_tasks_write(struct file *filp, const char __user *ubuf,
				       size_t cnt, loff_t *ppos)
{
	struct mpam_partition *part;
	struct task_struct *p;
	struct inode *inode;
	/* PID limit is the millions, 7 chars + newline + \0 */
	char buf[9];
	int ret = 0;
	pid_t pid;

	if (cnt > ARRAY_SIZE(buf))
		cnt = ARRAY_SIZE(buf);

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;

	if (kstrtoint(strstrip(buf), 0, &pid) || pid < 0)
		return -EINVAL;

	inode = file_inode(filp);
	part = (struct mpam_partition *)inode->i_private;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (!p) {
		ret = -EINVAL;
		rcu_read_unlock();
		goto out;
	}

	get_task_struct(p);
	rcu_read_unlock();

	mutex_lock(&mpam_fs.lock);
	mpam_set_task_partid(p, part->partid);
	mutex_unlock(&mpam_fs.lock);

	put_task_struct(p);

	*ppos += cnt;
out:
	return ret ?: cnt;
}

static int mpam_policy_tasks_show(struct seq_file *s, void *v)
{
	struct mpam_partition *part = s->private;
	struct task_struct *p, *t;

	mutex_lock(&mpam_fs.lock);
	rcu_read_lock();
	for_each_process_thread(p, t) {
		if (mpam_get_task_partid(t) == part->partid)
			seq_printf(s, "%d\n", t->pid);
	}
	rcu_read_unlock();
	mutex_unlock(&mpam_fs.lock);

	return 0;
}

static int mpam_policy_tasks_open(struct inode *inode, struct file *file)
{
	return single_open(file, mpam_policy_tasks_show, inode->i_private);
}

static const struct super_operations mpam_fs_super_ops = {
	.statfs		= simple_statfs,
};

static struct inode *mpam_fs_create_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);

	if (ret) {
		ret->i_ino = get_next_ino();
		ret->i_mode = mode;
		ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);
	}
	return ret;
}

static struct dentry *mpam_fs_create_dir(struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return ERR_PTR(-ENOMEM);

	inode = mpam_fs_create_inode(parent->d_sb, S_IFDIR | 0444);
	if (!inode) {
		dput(dentry);
		return ERR_PTR(-ENOMEM);
	}

	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;
}

static struct dentry *mpam_fs_create_file(struct dentry *parent,
			const char *name,
			const struct file_operations *fops,
			void *data,
			int mode)
{
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return ERR_PTR(-ENOMEM);

	inode = mpam_fs_create_inode(parent->d_sb, S_IFREG | mode);
	if (!inode) {
		dput(dentry);
		return ERR_PTR(-ENOMEM);
	}

	inode->i_fop = fops;
	inode->i_private = data;

	d_add(dentry, inode);
	return dentry;
}

static int mpam_fs_create_files(struct super_block *sb)
{
	struct dentry *parts_dir;
	/* Max PARTID is 16 bits aka 65535 */
	char dirname[6];
	int i;

	mpam_fs.partitions_count = mpam_get_partid_count();

	mpam_fs.partitions = kmalloc_array(
		mpam_fs.partitions_count, sizeof(*mpam_fs.partitions), GFP_KERNEL);
	if (!mpam_fs.partitions)
		return -ENOMEM;

	parts_dir = mpam_fs_create_dir(sb->s_root, "partitions");
	if (IS_ERR(parts_dir))
		return PTR_ERR(parts_dir);

	for (i = 0; i < mpam_get_partid_count(); i++) {
		struct mpam_partition *part = &mpam_fs.partitions[i];
		struct dentry *dir;
		struct dentry *file;

		part->partid = i;

		snprintf(dirname, sizeof(dirname),  "%d", i);
		dir = mpam_fs_create_dir(parts_dir, dirname);
		if (IS_ERR(dir))
			return PTR_ERR(dir);
		file = mpam_fs_create_file(dir, "tasks", &mpam_fs_tasks_ops, part, 0644);
		if (IS_ERR(file))
			return PTR_ERR(file);
	}
	return 0;
}

static int mpam_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct inode *root;

	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = MPAM_POLICY_SUPER_MAGIC;
	sb->s_op = &mpam_fs_super_ops;

	root = mpam_fs_create_inode(sb, S_IFDIR | 0444);
	if (!root)
		return -ENOMEM;

	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	sb->s_root = d_make_root(root);
	if (!sb->s_root)
		return -ENOMEM;

	return mpam_fs_create_files(sb);
}

static int mpam_get_tree(struct fs_context *fc)
{
	return get_tree_single(fc, mpam_fill_super);
}

static int __init mpam_policy_fs_init(void)
{
	int ret = 0;

	ret = sysfs_create_mount_point(fs_kobj, "mpam");
	if (ret)
		goto err;

	ret = register_filesystem(&mpam_fs_type);
	if (ret)
		goto err_mount;

	return ret;

err_mount:
	sysfs_remove_mount_point(fs_kobj, "mpam");
err:
	return ret;
}

static void mpam_policy_fs_exit(void)
{
	unregister_filesystem(&mpam_fs_type);
	sysfs_remove_mount_point(fs_kobj, "mpam");
}

/*
 * Sync @p's associated PARTID with this CPU's register.
 */
void mpam_sync_task(struct task_struct *p)
{
	mpam_write_partid(mpam_get_task_partid(p));
}
EXPORT_SYMBOL_GPL(mpam_sync_task);

/*
 * Same as mpam_sync_task(), with a pre-filter for the current task.
 */
static void mpam_sync_current(void *task)
{
	if (task && task != current)
		return;

	mpam_sync_task(current);
}

static bool __task_curr(struct task_struct *p)
{
	return cpu_curr(task_cpu(p)) == p;
}

static void mpam_kick_task(struct task_struct *p)
{
	/*
	 * If @p is no longer on the task_cpu(p) we see here when the smp_call
	 * actually runs, then it had a context switch, so it doesn't need the
	 * explicit update - no need to chase after it.
	 */
	if (__task_curr(p))
		smp_call_function_single(task_cpu(p), mpam_sync_current, p, 1);
}

void mpam_hook_fork(struct task_struct *p)
{
	/*
	 * The task isn't supposed to be runnable yet, so we don't have to issue
	 * an mpam_sync_task() here.
	 */
	if (p->sched_reset_on_fork)
		mpam_set_task_partid(p, MPAM_PARTID_DEFAULT);
}
EXPORT_SYMBOL_GPL(mpam_hook_fork);

/*
 * Default-0 is a sensible thing, and it avoids us having to do anything
 * to setup the task_struct vendor data field that serves as partid.
 * If it becomes different than zero, we need the following after registering
 * the sched_switch hook:
 * - a for_each_process_thread() loop, to initialize existing tasks
 * - a trace_task_newtask hook, to initialize tasks that are being
 *   forked and may not be covered by the above loop
 */
static_assert(MPAM_PARTID_DEFAULT == 0);

static int __init mpam_policy_init(void)
{
	int ret;

	ret = mpam_policy_fs_init();
	if (ret)
		mpam_policy_fs_exit();

	return ret;
}

module_init(mpam_policy_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Valentin Schneider <valentin.schneider@arm.com>");
MODULE_SOFTDEP("mpam_policy pre: mpam_arch");
