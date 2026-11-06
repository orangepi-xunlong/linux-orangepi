// SPDX-License-Identifier:	GPL-2.0-only
/*
 * Created on Thu May 25 2023
 *
 * Copyright (c) 2023 CVITEK
 */

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/device/class.h>
#include <linux/input/matrix_keypad.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/reset.h>
#include <linux/platform_device.h>

/* #include "pinctrl-cv186x.h" */

#define MAX_KEY_NUM	0xf
#define KEYSCAN_CONFIG0		 0x0
#define KEYSCAN_CONFIG1		 0x4
#define KEYSCAN_CONFIG2		 0x8
#define KEYSCAN_CONFIG3		 0xc
#define KEYSCAN_SNAPSHOT_ARRAY_L 0x14
#define KEYSCAN_SNAPSHOT_ARRAY_H 0x18
#define KEYSCAN_SNAPSHOT_TRIG	 0x1c
#define KEYSCAN_FIFO_STATUS	 0x20
#define KEYSCAN_FIFO		 0x24
#define KEYSCAN_IRQ_ENABLE	 0x28
#define KEYSCAN_IRQ_FLAG	 0x2c
#define KEYSCAN_IRQ_CLEAR	 0x30

#define KEYSCAN_FIFO_DATA_MASk 0x7f
#define KEYSCAN_FIFO_KEY_MASk  0x3f
#define KEYSCAN_FIFO_PRE_MASk  0x40

#define KEYSCAN_CLR_IRQ 0x11
#define DEFAULT_MASK 0xf0f0

static struct class *keyscan_class;
static dev_t	     keyscan_dev_t;

enum mode { FIFO, SNAPSHOT };

struct cvi_keyscan {
	void __iomem	 *base;
	struct reset_control *rst_keyscan;
	struct cdev	  cdev;
	int		  irq;
	struct clk	 *clk;
	struct input_dev *input_dev;
	unsigned int	  n_rows, n_cols, row_shift;
	unsigned long long last_array;
	enum mode	  m;
};

static void keyscan_init(struct cvi_keyscan *keypad)
{
	unsigned int val;

    /* PINMUX_CONFIG(UART2_CTS, KEY_COL3, G12); */
    /* PINMUX_CONFIG(UART4_CTS, KEY_ROW3, G12); */

	// Scan frequency = IP clock freq / ( (reg_slow_div+1) * (9+reg_wait_count+1))
	writel(0xff, keypad->base + KEYSCAN_CONFIG1);	// reg_slow_div = 0xff
	writel(0x64, keypad->base + KEYSCAN_CONFIG2);	// reg_db_col = 0x64
	writel(0xff, keypad->base + KEYSCAN_CONFIG3);	// reg_wait_cntr = 0x70

	// umask
	val = readl(keypad->base + KEYSCAN_CONFIG0);
	val &= 0xf0000;
	// mask row4~7, col4~7
	val |= DEFAULT_MASK;
	writel(val, keypad->base + KEYSCAN_CONFIG0);

	// init array val
	keypad->last_array = 0UL - 1;
}

static irqreturn_t keyscan_isr(int irq, void *dev_id)
{
	struct cvi_keyscan *keypad = dev_id;
	unsigned short	   *keycode = keypad->input_dev->keycode;
	unsigned int	    cnt, row, col, data;
	unsigned long long	array, change;
	int		    pressed, i;

	pr_debug("keyscan iqr handle! mode: %s, irq status reg: 0x%x\n",
		 keypad->m ? "SNAPSHOT" : "FIFO",
		 readl(keypad->base + KEYSCAN_IRQ_FLAG));
	// fifo	mode
	if (keypad->m == FIFO) {
		cnt = readl(keypad->base + KEYSCAN_FIFO_STATUS) & 0xf;
		for (i = 0; i < cnt; ++i) {
			data = readl(keypad->base + KEYSCAN_FIFO) &
			       KEYSCAN_FIFO_DATA_MASk;
			row = (data & KEYSCAN_FIFO_KEY_MASk) / 8;
			col = (data & KEYSCAN_FIFO_KEY_MASk) % 8;
			pressed = data & KEYSCAN_FIFO_PRE_MASk;

			pr_debug("row = %d, col = %d, code idx = %d\n", row, col,
				 MATRIX_SCAN_CODE(row, col, keypad->row_shift));
			input_report_key(keypad->input_dev,
					 keycode[MATRIX_SCAN_CODE(
						 row, col, keypad->row_shift)],
					 pressed);
		}
		input_sync(keypad->input_dev);
		if (readl(keypad->base + KEYSCAN_FIFO_STATUS) & 0x10) {
			pr_err("%s:	keyscan	fifo read unfinished!\n",
			       __func__);
			return IRQ_HANDLED;
		}
		writel(KEYSCAN_CLR_IRQ, keypad->base + KEYSCAN_IRQ_CLEAR);
		return IRQ_HANDLED;
	}
	// snapshot	mode
	writel(0x1, keypad->base + KEYSCAN_SNAPSHOT_TRIG);
	array = readl(keypad->base + KEYSCAN_SNAPSHOT_ARRAY_L);
	array |= (unsigned long long)readl(keypad->base + KEYSCAN_SNAPSHOT_ARRAY_H) << 32;
	pr_debug("array = 0x%llx, last_array = 0x%llx\n", array, keypad->last_array);
	change = array ^ keypad->last_array;

	// i = row * 8 + col; row = i / 8;
	// j = row * n_cols + col;
	// i - j = row * n_cols; j = i - row * n_cols= i - (i / 8) * n_cols;
	for_each_set_bit (i, (unsigned long *)&change, BITS_PER_LONG_LONG)
		input_report_key(keypad->input_dev,
				 keycode[(i - (i / 8) * keypad->n_cols)],
				 array & BIT(i));

	input_sync(keypad->input_dev);
	writel(KEYSCAN_CLR_IRQ, keypad->base + KEYSCAN_IRQ_CLEAR);
	keypad->last_array = array;

	return IRQ_HANDLED;
}

static void keyscan_start(struct cvi_keyscan *keypad)
{
	unsigned int val;

	// enable irq
	if (keypad->m == FIFO) {
		writel(0x1, keypad->base + KEYSCAN_IRQ_ENABLE);
	} else {
		writel(0x10, keypad->base + KEYSCAN_IRQ_ENABLE);
	}

	val = readl(keypad->base + KEYSCAN_CONFIG0);
	val |= BIT(16);
	writel(val, keypad->base + KEYSCAN_CONFIG0);
	pr_debug("%s: config0 val:0x%x\n", __func__, readl(keypad->base + KEYSCAN_CONFIG0));
}

static void keyscan_stop(struct cvi_keyscan *keypad)
{
	unsigned int val;
	val = readl(keypad->base + KEYSCAN_CONFIG0);
	val &= ~BIT(16);
	writel(val, keypad->base + KEYSCAN_CONFIG0);

	// disable irq
	writel(0x0, keypad->base + KEYSCAN_IRQ_ENABLE);
	pr_debug("%s: config0 val:0x%x\n", __func__, readl(keypad->base + KEYSCAN_CONFIG0));
}

static int keyscan_input_open(struct input_dev *dev)
{
	struct cvi_keyscan *keypad = input_get_drvdata(dev);
	keyscan_start(keypad);

	pr_debug("keyscan start\n");
	return 0;
}

static void keyscan_input_close(struct input_dev *dev)
{
	struct cvi_keyscan *keypad = input_get_drvdata(dev);

	keyscan_stop(keypad);
	pr_debug("keyscan stoped\n");
}

static int keypad_matrix_key_parse_dt(struct cvi_keyscan *keypad_data)
{
	struct device *dev = keypad_data->input_dev->dev.parent;
	int	       ret;

	ret = matrix_keypad_parse_properties(dev, &keypad_data->n_rows,
					     &keypad_data->n_cols);
	if (ret) {
		dev_err(dev, "failed to parse keypad params\n");
		return ret;
	}
    keypad_data->row_shift = get_count_order(keypad_data->n_cols);

	ret = device_property_read_u32(dev, "mode", &keypad_data->m);
	if (ret) {
		keypad_data->m = SNAPSHOT;
		dev_info(dev,
			 "not found mode setting in dts, set snapshot mode\n");
	}
	dev_dbg(dev, "n_rows=%d n_col=%d mode=%s\n", keypad_data->n_rows,
		keypad_data->n_cols, keypad_data->m ? "SNAPSHOT" : "FIFO");

	return 0;
}

// read	keyscan	mask
ssize_t keyscan_read(struct file *filp, char __user *buff, size_t count,
		     loff_t *offp)
{
	struct cvi_keyscan *keypad = filp->private_data;
	unsigned int	    val;
	char		    buf[8];

	if (*offp >= sizeof(buf)) {
		*offp = 0;
		return 0;
	}

	val = readl(keypad->base + KEYSCAN_CONFIG0) & 0xffff;
	scnprintf(buf, sizeof(buf), "0x%x\n", val);
	if (copy_to_user(buff, buf, strlen(buf))) {
		pr_err("%s:	keyscan read mask error\n", __func__);
		return -EIO;
	}
	*offp += sizeof(buf);
	return sizeof(buf);
}

// set keyscan mask
ssize_t keyscan_write(struct file *filp, const char __user *buff, size_t count,
		      loff_t *offp)
{
	struct cvi_keyscan *keypad = filp->private_data;
	char		    buf[8];
	unsigned int	    val;

	if (copy_from_user(buf, buff, count)) {
		pr_err("%s: keyscan write mask error\n", __func__);
		return -EIO;
	}
	pr_debug("%s: write mask val: %s\n", __func__, buf);
	sscanf(buf, "0x%x", &val);

	val &= 0xffff;
	val |= (readl(keypad->base + KEYSCAN_CONFIG0) & 0x10000);
	val |= DEFAULT_MASK;
	writel(val, keypad->base + KEYSCAN_CONFIG0);
	pr_debug("%s: keyscan mask val:0x%x\n", __func__, readl(keypad->base + KEYSCAN_CONFIG0));
	return count;
}

static int keyscan_open(struct inode *inode, struct file *filp)
{
	struct cvi_keyscan *keypad =
		container_of(inode->i_cdev, struct cvi_keyscan, cdev);

	filp->private_data = keypad;

	pr_debug("cvi-keyscan opened\n");

	return 0;
}

static int keyscan_close(struct inode *inode, struct file *filp)
{
	// struct cvi_keyscan *keypad =
	// container_of(inode->i_cdev, struct cvi_keyscan, cdev);

	filp->private_data =	NULL;

	pr_debug("cvi-keyscan closed\n");
	return 0;
}

static const struct file_operations keyscan_fops = {
	.owner = THIS_MODULE,
	.open = keyscan_open,
	.release = keyscan_close,
	.read = keyscan_read,
	.write = keyscan_write,
};

static int keyscan_register_cdev(struct cvi_keyscan *keypad)
{
	int ret;
	// int rc;

	keyscan_class = class_create(THIS_MODULE, "cvi-keyscan");
	if (IS_ERR(keyscan_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(keyscan_class);
	}

	ret = alloc_chrdev_region(&keyscan_dev_t, 0, 1, "cvi-keyscan");
	if (ret < 0) {
		pr_err("alloc chrdev failed\n");
		return ret;
	}

	cdev_init(&keypad->cdev, &keyscan_fops);
	keypad->cdev.owner = THIS_MODULE;
	cdev_add(&keypad->cdev, keyscan_dev_t, 1);

	device_create(keyscan_class, NULL, keyscan_dev_t, NULL, "%s%d",
		      "cvi-keyscan", 0);

	// rc =	sysfs_create_group(&ndev->dev->kobj, &tee_dev_group);
	// if (rc) {
	//	dev_err(ndev->dev,
	//		"failed	to create sysfs	attributes,	err=%d\n", rc);
	//	return rc;
	// }

	return 0;
};

static void keyscan_unregister_cdev(struct cvi_keyscan *keypad)
{
	device_destroy(keyscan_class, keyscan_dev_t);

	cdev_del(&keypad->cdev);

	unregister_chrdev_region(keyscan_dev_t, 1);

	class_destroy(keyscan_class);
}

static int keyscan_probe(struct platform_device *pdev)
{
	struct cvi_keyscan *keypad_data;
	struct input_dev   *input_dev;
	int		    ret;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "no	DT data	present\n");
		return -EINVAL;
	}

	keypad_data =
		devm_kzalloc(&pdev->dev, sizeof(*keypad_data), GFP_KERNEL);
	if (!keypad_data)
		return -ENOMEM;

	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!input_dev) {
		dev_err(&pdev->dev, "failed to allocate the input device\n");
		return -ENOMEM;
	}

	input_dev->name = pdev->name;
	input_dev->phys = "keyscan-keys/input0";
	input_dev->dev.parent = &pdev->dev;
	input_dev->open = keyscan_input_open;
	input_dev->close = keyscan_input_close;

	input_dev->id.bustype = BUS_HOST;

	keypad_data->input_dev = input_dev;

	ret = keypad_matrix_key_parse_dt(keypad_data);
	if (ret)
		return ret;

	ret = matrix_keypad_build_keymap(NULL, NULL, keypad_data->n_rows,
					 keypad_data->n_cols, NULL, input_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to build keymap\n");
		return ret;
	}

	input_set_drvdata(input_dev, keypad_data);

	keypad_data->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(keypad_data->base)) {
		dev_err(&pdev->dev, "failed to get keyscan res\n");
		return PTR_ERR(keypad_data->base);
	}

	keypad_data->rst_keyscan = devm_reset_control_get(&pdev->dev, "res_keyscan");
	if (IS_ERR(keypad_data->rst_keyscan)) {
		dev_err(&pdev->dev, "failed to	retrieve res_keyscan\n");
		keypad_data->rst_keyscan = NULL;
	}

	keypad_data->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(keypad_data->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		return PTR_ERR(keypad_data->clk);
	}

	ret	= clk_prepare_enable(keypad_data->clk);
	if (ret)	{
		dev_err(&pdev->dev, "failed to enable clock\n");
		return ret;
	}

	keypad_data->irq = platform_get_irq(pdev, 0);
	if (keypad_data->irq < 0)
		return -EINVAL;

	ret = devm_request_irq(&pdev->dev, keypad_data->irq, keyscan_isr, 0,
			       pdev->name, keypad_data);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
		return ret;
	}

	ret = keyscan_register_cdev(keypad_data);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register keyscan cdev\n");
		return ret;
	}

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register input device\n");
		goto err_out;
	}

	platform_set_drvdata(pdev, keypad_data);

	keyscan_init(keypad_data);

	// enable trigger wake-up event
	// device_set_wakeup_capable(&pdev->dev, 1);

	return 0;

err_out:
	keyscan_unregister_cdev(keypad_data);
	return ret;
}

static int keyscan_remove(struct platform_device *pdev)
{
	struct cvi_keyscan *keypad = platform_get_drvdata(pdev);

	keyscan_unregister_cdev(keypad);
	input_unregister_device(keypad->input_dev);
	clk_disable_unprepare(keypad->clk);
	pr_debug("cvi_keyscan_remove\n");
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int keyscan_suspend(struct device *dev)
{
	struct cvi_keyscan *keypad = dev_get_drvdata(dev);
	struct input_dev   *input = keypad->input_dev;

	mutex_lock(&input->mutex);

	if (device_may_wakeup(dev))
		enable_irq_wake(keypad->irq);
	else if (input->users) {
		keyscan_stop(keypad);
		clk_disable_unprepare(keypad->clk);
	}

	mutex_unlock(&input->mutex);
	return 0;
}

static int keyscan_resume(struct device *dev)
{
	struct cvi_keyscan *keypad = dev_get_drvdata(dev);
	struct input_dev   *input = keypad->input_dev;
	// int		    retval = 0;

	mutex_lock(&input->mutex);

	if (device_may_wakeup(dev))
		disable_irq_wake(keypad->irq);
	else if (input->users) {
		clk_prepare_enable(keypad->clk);
		keyscan_start(keypad);
	}

	mutex_unlock(&input->mutex);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(keyscan_dev_pm_ops, keyscan_suspend, keyscan_resume);

static const struct of_device_id keyscan_of_match[] = {
	{ .compatible = "cvitek,keyscan" },
	{},
};
MODULE_DEVICE_TABLE(of, keyscan_of_match);

static struct platform_driver
	keyscan_device_driver = { .probe = keyscan_probe,
				  .remove = keyscan_remove,
				  .driver = {
					  .name = "cvi-keyscan",
					  .pm = &keyscan_dev_pm_ops,
					  .of_match_table = of_match_ptr(
						  keyscan_of_match),
				  } };

module_platform_driver(keyscan_device_driver);

MODULE_AUTHOR("zixun.li	<zixun.li@sophgo.com>");
MODULE_DESCRIPTION("cvitek keyscan driver");
MODULE_LICENSE("GPL");
