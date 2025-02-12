#include <linux/property.h>
#include <linux/delay.h>
#include <linux/usb.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>


struct ky_hub_priv {
	struct device *dev;
	bool is_hub_on;
	bool is_vbus_on;

	struct gpio_descs *hub_gpios;
	struct gpio_descs *vbus_gpios;
	bool hub_gpio_active_low;
	bool vbus_gpio_active_low;

	u32 hub_inter_delay_ms;
	u32 vbus_delay_ms;
	u32 vbus_inter_delay_ms;

	bool suspend_power_on;

	struct mutex hub_mutex;
};

static void ky_hub_enable(struct ky_hub_priv *ky, bool on);

static void ky_hub_vbus_enable(struct ky_hub_priv *ky,
					bool on);

static int ky_hub_enable_show(struct seq_file *s, void *unused)
{
	struct ky_hub_priv *ky = s->private;
	mutex_lock(&ky->hub_mutex);
	seq_puts(s, ky->is_hub_on ? "true\n" : "false\n");
	mutex_unlock(&ky->hub_mutex);
	return 0;
}

static ssize_t ky_hub_enable_write(struct file *file,
					   const char __user *ubuf, size_t count,
					   loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct ky_hub_priv *ky = s->private;
	bool on = false;
	char buf[32];

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if ((!strncmp(buf, "true", 4)) || (!strncmp(buf, "1", 1)))
		on = true;
	if ((!strncmp(buf, "false", 5)) || !strncmp(buf, "0", 1))
		on = false;

	mutex_lock(&ky->hub_mutex);
	if (on != ky->is_hub_on) {
		ky_hub_enable(ky, on);
	}
	mutex_unlock(&ky->hub_mutex);

	return count;
}

static int ky_hub_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, ky_hub_enable_show, inode->i_private);
}

struct file_operations ky_hub_enable_fops = {
	.open = ky_hub_enable_open,
	.write = ky_hub_enable_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ky_hub_vbus_show(struct seq_file *s, void *unused)
{
	struct ky_hub_priv *ky = s->private;
	mutex_lock(&ky->hub_mutex);
	seq_puts(s, ky->is_vbus_on ? "true\n" : "false\n");
	mutex_unlock(&ky->hub_mutex);
	return 0;
}

static ssize_t ky_hub_vbus_write(struct file *file,
					   const char __user *ubuf, size_t count,
					   loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct ky_hub_priv *ky = s->private;
	bool on = false;
	char buf[32];

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if ((!strncmp(buf, "true", 4)) || (!strncmp(buf, "1", 1)))
		on = true;
	if ((!strncmp(buf, "false", 5)) || !strncmp(buf, "0", 1))
		on = false;

	mutex_lock(&ky->hub_mutex);
	if (on != ky->is_vbus_on) {
		ky_hub_vbus_enable(ky, on);
	}
	mutex_unlock(&ky->hub_mutex);

	return count;
}

static int ky_hub_vbus_open(struct inode *inode, struct file *file)
{
	return single_open(file, ky_hub_vbus_show, inode->i_private);
}

struct file_operations ky_hub_vbus_fops = {
	.open = ky_hub_vbus_open,
	.write = ky_hub_vbus_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ky_hub_suspend_show(struct seq_file *s, void *unused)
{
	struct ky_hub_priv *ky = s->private;
	mutex_lock(&ky->hub_mutex);
	seq_puts(s, ky->suspend_power_on ? "true\n" : "false\n");
	mutex_unlock(&ky->hub_mutex);
	return 0;
}

static ssize_t ky_hub_suspend_write(struct file *file,
					   const char __user *ubuf, size_t count,
					   loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct ky_hub_priv *ky = s->private;
	bool on = false;
	char buf[32];

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if ((!strncmp(buf, "true", 4)) || (!strncmp(buf, "1", 1)))
		on = true;
	if ((!strncmp(buf, "false", 5)) || !strncmp(buf, "0", 1))
		on = false;

	mutex_lock(&ky->hub_mutex);
	ky->suspend_power_on = on;
	mutex_unlock(&ky->hub_mutex);

	return count;
}

static int ky_hub_suspend_open(struct inode *inode, struct file *file)
{
	return single_open(file, ky_hub_suspend_show, inode->i_private);
}

struct file_operations ky_hub_suspend_fops = {
	.open = ky_hub_suspend_open,
	.write = ky_hub_suspend_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void ky_hub_debugfs_init(struct ky_hub_priv *ky)
{
	struct dentry *root;

	root = debugfs_create_dir(dev_name(ky->dev), usb_debug_root);
	debugfs_create_file("vbus_on", 0644, root, ky,
				&ky_hub_vbus_fops);
	debugfs_create_file("hub_on", 0644, root, ky,
				&ky_hub_enable_fops);
	debugfs_create_file("suspend_power_on", 0644, root, ky,
				&ky_hub_suspend_fops);
}
