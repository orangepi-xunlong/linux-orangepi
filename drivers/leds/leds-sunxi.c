/*
 * drivers/leds/leds-sunxi.c - Allwinner RGB LED Driver
 *
 * Copyright (C) 2018 Allwinner Technology Limited. All rights reserved.
 * Albert Yu <yuxyun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

#include "leds-sunxi.h"

static struct sunxi_led *sunxi_led;

static void sunxi_ledc_trans_data(struct sunxi_led *led);
static void sunxi_ledc_set_trans_mode(struct sunxi_led *led, const char *mode);

static void sunxi_clk_get(struct sunxi_led *led)
{
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	led->clk_ledc = of_clk_get(np, 0);
	if (IS_ERR(led->clk_ledc))
		dev_err(dev, "failed to get clk_ledc!\n");

	led->clk_cpuapb = of_clk_get(np, 1);
	if (IS_ERR(led->clk_cpuapb))
		dev_err(dev, "failed to get clk_cpuapb!\n");
}

static void sunxi_clk_put(struct sunxi_led *led)
{
	clk_put(led->clk_ledc);
	clk_put(led->clk_cpuapb);
}

static void sunxi_clk_enable(struct sunxi_led *led)
{
	clk_prepare_enable(led->clk_ledc);
	clk_prepare_enable(led->clk_cpuapb);
}

static void sunxi_clk_disable(struct sunxi_led *led)
{
	clk_disable_unprepare(led->clk_cpuapb);
	clk_disable_unprepare(led->clk_ledc);
}

static void sunxi_clk_init(struct sunxi_led *led)
{
	sunxi_clk_get(led);
	sunxi_clk_enable(led);
}

static void sunxi_clk_deinit(struct sunxi_led *led)
{
	sunxi_clk_disable(led);
	sunxi_clk_put(led);
}

static u32 sunxi_get_reg(int offset)
{
	struct sunxi_led *led = sunxi_led;
	u32 value = ioread32(((u8 *)led->iomem_reg_base) + offset);

	return value;
}

static void sunxi_set_reg(int offset, u32 value)
{
	struct sunxi_led *led = sunxi_led;

	iowrite32(value, ((u8 *)led->iomem_reg_base) + offset);
}

static inline void sunxi_set_reset_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x1FFF;
	u32 min = SUNXI_RESET_TIME_MIN_NS;
	u32 max = SUNXI_RESET_TIME_MAX_NS;

	if (led->reset_ns < min || led->reset_ns > max) {
		dev_err(led->dev,
				"invalid parameter, reset_ns should be %d-%d!\n",
				min, max);
		goto out;
	}

	n = (led->reset_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_RESET_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << 16);
	reg_val |= (n << 16);
	sunxi_set_reg(LED_RESET_TIMING_CTRL_REG_OFFSET, reg_val);

out:
	reg_val = sunxi_get_reg(LED_RESET_TIMING_CTRL_REG_OFFSET);
	n = (reg_val >> 16) & mask;
	led->reset_ns = 42 * (n + 1);
}

static inline void sunxi_set_t1h_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x3F;
	u32 shift = 21;
	u32 min = SUNXI_T1H_MIN_NS;
	u32 max = SUNXI_T1H_MAX_NS;

	if (led->t1h_ns < min || led->t1h_ns > max) {
		dev_err(led->dev,
				"invalid parameter, t1h_ns should be %d-%d!\n",
				min, max);
		goto out;
	}

	n = (led->t1h_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << shift);
	reg_val |= n << shift;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);

out:
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	n = (reg_val >> shift) & mask;
	led->t1h_ns = 42 * (n + 1);
}

static inline void sunxi_set_t1l_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x1F;
	u32 shift = 16;
	u32 min = SUNXI_T1L_MIN_NS;
	u32 max = SUNXI_T1L_MAX_NS;

	if (led->t1l_ns < min || led->t1l_ns > max) {
		dev_err(led->dev,
				"invalid parameter, t1l_ns should be %d-%d!\n",
				min, max);
		goto out;
	}

	n = (led->t1l_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << shift);
	reg_val |= n << shift;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);

out:
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	n = (reg_val >> shift) & mask;
	led->t1l_ns = 42 * (n + 1);

}

static inline void sunxi_set_t0h_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x1F;
	u32 shift = 6;
	u32 min = SUNXI_T0H_MIN_NS;
	u32 max = SUNXI_T0H_MAX_NS;

	if (led->t0h_ns < min || led->t0h_ns > max) {
		dev_err(led->dev,
			"invalid parameter, t0h_ns should be %d-%d!\n",
			min, max);
		goto out;
	}

	n = (led->t0h_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << shift);
	reg_val |= n << shift;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);

out:
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	n = (reg_val >> shift) & mask;
	led->t0h_ns = 42 * (n + 1);
}

static inline void sunxi_set_t0l_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x3F;
	u32 min = SUNXI_T0L_MIN_NS;
	u32 max = SUNXI_T0L_MAX_NS;

	if (led->t0l_ns < min || led->t0l_ns > max) {
		dev_err(led->dev,
				"invalid parameter, t0l_ns should be %d-%d!\n",
				min, max);
		goto out;
	}

	n = (led->t0l_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~0x3F;
	reg_val |= n;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);

out:
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	n = reg_val & mask;
	led->t0l_ns = 42 * (n + 1);
}

static inline void sunxi_set_wait_time0_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0xFF;
	u32 min = SUNXI_WAIT_TIME0_MIN_NS;
	u32 max = SUNXI_WAIT_TIME0_MAX_NS;

	if (led->wait_time0_ns < min || led->wait_time0_ns > max) {
		dev_err(led->dev,
				"invalid parameter, wait_time0_ns should be %d-%d!\n",
				min, max);
		goto out;
	}

	n = (led->wait_time0_ns - 42) / 42;
	reg_val = (1 << 8) | n;
	sunxi_set_reg(LEDC_WAIT_TIME0_CTRL_REG, reg_val);

out:
	reg_val = sunxi_get_reg(LEDC_WAIT_TIME0_CTRL_REG);
	n = reg_val & mask;
	led->wait_time0_ns = 42 * (n + 1);
}

static inline void sunxi_set_wait_time1_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x7FFFFFFF;
	u32 min = SUNXI_WAIT_TIME1_MIN_NS;
	long long max = SUNXI_WAIT_TIME1_MAX_NS;

	if (led->wait_time1_ns < min || led->wait_time1_ns > max) {
		dev_err(led->dev,
			"invalid parameter, wait_time1_ns should be %u-%lld!\n",
			min, max);
		goto out;
	}

	n = (led->wait_time1_ns - 42) / 42;
	reg_val = (1 << 31) | n;
	sunxi_set_reg(LEDC_WAIT_TIME1_CTRL_REG, reg_val);

out:
	reg_val = sunxi_get_reg(LEDC_WAIT_TIME1_CTRL_REG);
	n = reg_val & mask;
	led->wait_time1_ns =  42 * (n + 1);
}

static inline void sunxi_set_wait_data_time_ns(struct sunxi_led *led)
{
	u32 mask = 0x1FFF;
	u32 shift = 16;
	u32 reg_val = 0;
	u32 n, min, max;

	min = SUNXI_WAIT_DATA_TIME_MIN_NS;
#ifdef SUNXI_FPGA_LEDC
	/*
	 * For FPGA platforms, it is easy to meet wait data timeout for
	 * the obvious latency of task which is because of less cpu cores
	 * and lower cpu frequency compared with IC platforms, so here we
	 * permit long enough time latency.
	 */
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_FPGA;
#else /* SUNXI_FPGA_LEDC */
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_IC;
#endif /* SUNXI_FPGA_LEDC */

	if (led->wait_data_time_ns < min || led->wait_data_time_ns > max) {
		dev_err(led->dev,
			"invalid parameter, wait_data_time_ns should be %d-%d!\n",
			min, max);
		goto out;
	}

#ifndef SUNXI_FPGA_LEDC
	n = (led->wait_data_time_ns - 42) / 42;
	reg_val &= ~(mask << shift);
	reg_val |= (n << shift);
	sunxi_set_reg(LEDC_DATA_FINISH_CNT_REG_OFFSET, reg_val);
#endif /* SUNXI_FPGA_LEDC */

out:
#ifdef SUNXI_FPGA_LEDC
	if (led->wait_data_time_ns <= SUNXI_WAIT_DATA_TIME_MAX_NS_IC)
#endif /* SUNXI_FPGA_LEDC */
	{
		reg_val = sunxi_get_reg(LEDC_DATA_FINISH_CNT_REG_OFFSET);
		n = (reg_val >> shift) & mask;
		led->wait_data_time_ns =  42 * (n + 1);
	}
}

static void sunxi_ledc_set_time(struct sunxi_led *led)
{
	sunxi_set_reset_ns(led);
	sunxi_set_t1h_ns(led);
	sunxi_set_t1l_ns(led);
	sunxi_set_t0h_ns(led);
	sunxi_set_t0l_ns(led);
	sunxi_set_wait_time0_ns(led);
	sunxi_set_wait_time1_ns(led);
	sunxi_set_wait_data_time_ns(led);
}

static void sunxi_ledc_set_length(struct sunxi_led *led)
{
	u32 reg_val;
	u32 length = led->length;

	if (length == 0)
		goto err_out;

	if (length > led->led_count)
		goto err_out;

	reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);
	reg_val &= ~(0x1FF << 16);
	reg_val |=  length << 16;
	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);

	reg_val = sunxi_get_reg(LED_RESET_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~0x3FF;
	reg_val |= length - 1;
	sunxi_set_reg(LED_RESET_TIMING_CTRL_REG_OFFSET, reg_val);

	return;

err_out:
	led->length = 0;
}

static void sunxi_ledc_set_output_mode(struct sunxi_led *led, const char *str)
{
	u32 val;
	u32 mask = 0x7;
	u32 shift = 6;
	u32 reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);

	if (str != NULL) {
		if (!strncmp(led->output_mode.str, str, 3))
			return;

		if (!strncmp(str, "GRB", 3))
			val = SUNXI_OUTPUT_GRB;
		else if (!strncmp(str, "GBR", 3))
			val = SUNXI_OUTPUT_GBR;
		else if (!strncmp(str, "RGB", 3))
			val = SUNXI_OUTPUT_RGB;
		else if (!strncmp(str, "RBG", 3))
			val = SUNXI_OUTPUT_RBG;
		else if (!strncmp(str, "BGR", 3))
			val = SUNXI_OUTPUT_BGR;
		else if (!strncmp(str, "BRG", 3))
			val = SUNXI_OUTPUT_BRG;
		else
			return;

		memcpy(led->output_mode.str, str, 3);
	} else {
		val = led->output_mode.val;
	}

	reg_val &= ~(mask << shift);
	reg_val |= val;

	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);

	if (str)
		memcpy(led->output_mode.str, str, 3);

	if (val != led->output_mode.val)
		led->output_mode.val = val;
}

static void sunxi_ledc_set_trans_mode(struct sunxi_led *led, const char *str)
{
	u32 val, reg_val;

	if (str != NULL) {
		if (!strncmp(led->trans_mode.str, str, 3))
			return;

		if (!strncmp(str, "CPU", 3))
			val = LEDC_TRANS_CPU_MODE;
		else if (!strncmp(str, "DMA", 3))
			val = LEDC_TRANS_DMA_MODE;
		else
			return;

		memcpy(led->trans_mode.str, str, 3);
	} else {
		val = led->trans_mode.val;
	}

	reg_val = sunxi_get_reg(LEDC_DMA_CTRL_REG);
	if (val == LEDC_TRANS_DMA_MODE)
		reg_val |= 1 << 5;
	else
		reg_val &= ~(1 << 5);
	reg_val &= ~0x1F;
	reg_val |= SUNXI_LEDC_FIFO_TRIG_LEVEL;
	sunxi_set_reg(LEDC_DMA_CTRL_REG, reg_val);

	reg_val = sunxi_get_reg(LEDC_INT_CTRL_REG_OFFSET);
	if (val == LEDC_TRANS_DMA_MODE)
		reg_val &= ~(1 << 1);
	else
		reg_val |= 1 << 1;
	sunxi_set_reg(LEDC_INT_CTRL_REG_OFFSET, reg_val);

	if (val != led->trans_mode.val)
		led->trans_mode.val = val;
}

static bool sunxi_ledc_is_enabled(struct sunxi_led *led)
{
	u32 reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);

	return reg_val & 1;
}

static inline void sunxi_ledc_enable(struct sunxi_led *led)
{
	u32 reg_val;

	reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);
	reg_val |=  1;
	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_ledc_reset(struct sunxi_led *led)
{
	if (led->dma_chan)
		dmaengine_terminate_all(led->dma_chan);

	led->transmitted_data = 0;
	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, 1 << 1);
}

#ifdef CONFIG_DEBUG_FS
static ssize_t reset_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_RESET_TIME_MIN_NS;
	max = SUNXI_RESET_TIME_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->reset_ns = val;
	sunxi_set_reset_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, reset_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t reset_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->reset_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations reset_ns_fops = {
	.owner = THIS_MODULE,
	.write = reset_ns_write,
	.read  = reset_ns_read,
};

static ssize_t t1h_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_T1H_MIN_NS;
	max = SUNXI_T1H_MAX_NS;

	if (count >= sizeof(buffer))
		return -EINVAL;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		return -EINVAL;

	if (val < min || val > max)
		goto err_out;

	led->t1h_ns = val;

	sunxi_set_t1h_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, t1h_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t1h_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->t1h_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t1h_ns_fops = {
	.owner = THIS_MODULE,
	.write = t1h_ns_write,
	.read  = t1h_ns_read,
};

static ssize_t t1l_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_T1L_MIN_NS;
	max = SUNXI_T1L_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t1l_ns = val;
	sunxi_set_t1l_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, t1l_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t1l_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->t1l_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t1l_ns_fops = {
	.owner = THIS_MODULE,
	.write = t1l_ns_write,
	.read  = t1l_ns_read,
};

static ssize_t t0h_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_T0H_MIN_NS;
	max = SUNXI_T0H_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t0h_ns = val;
	sunxi_set_t0h_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, t0h_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t0h_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->t0h_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t0h_ns_fops = {
	.owner = THIS_MODULE,
	.write = t0h_ns_write,
	.read  = t0h_ns_read,
};

static ssize_t t0l_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_T0L_MIN_NS;
	max = SUNXI_T0L_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t0l_ns = val;
	sunxi_set_t0l_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, t0l_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t0l_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->t0l_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t0l_ns_fops = {
	.owner = THIS_MODULE,
	.write = t0l_ns_write,
	.read  = t0l_ns_read,
};

static ssize_t wait_time0_ns_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_WAIT_TIME0_MIN_NS;
	max = SUNXI_WAIT_TIME0_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_time0_ns = val;
	sunxi_set_wait_time0_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, wait_time0_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t wait_time0_ns_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->wait_time0_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations wait_time0_ns_fops = {
	.owner = THIS_MODULE,
	.write = wait_time0_ns_write,
	.read  = wait_time0_ns_read,
};

static ssize_t wait_time1_ns_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min;
	long long max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_WAIT_TIME1_MIN_NS;
	max = SUNXI_WAIT_TIME1_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_time1_ns = val;
	sunxi_set_wait_time1_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, wait_time1_ns should be %u-%lld!\n",
		min, max);

	return -EINVAL;
}

static ssize_t wait_time1_ns_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%lld\n", led->wait_time1_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations wait_time1_ns_fops = {
	.owner = THIS_MODULE,
	.write = wait_time1_ns_write,
	.read  = wait_time1_ns_read,
};

static ssize_t wait_data_time_ns_write(struct file *filp,
				const char __user *buf,
				size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	min = SUNXI_WAIT_DATA_TIME_MIN_NS;
#ifdef SUNXI_FPGA_LEDC
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_FPGA;
#else
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_IC;
#endif

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_data_time_ns = val;
	sunxi_set_wait_data_time_ns(led);

	*offp += count;

	return count;

err_out:
	dev_err(dev,
		"invalid parameter, wait_data_time_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t wait_data_time_ns_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%u\n", led->wait_data_time_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations wait_data_time_ns_fops = {
	.owner = THIS_MODULE,
	.write = wait_data_time_ns_write,
	.read  = wait_data_time_ns_read,
};

static int data_show(struct seq_file *s, void *data)
{
	int i;
	struct sunxi_led *led = sunxi_led;

	for (i = 0; i < led->led_count; i++) {
		if (!(i % 4)) {
			if (i + 4 <= led->led_count)
				seq_printf(s, "%04d-%04d", i, i + 4);
			else
				seq_printf(s, "%04d-%04d", i, led->led_count);
		}
		seq_printf(s, " 0x%08x", led->data[i]);
		if (((i % 4) == 3) || (i == led->led_count - 1))
			seq_puts(s, "\n");
	}

	return 0;
}

static int data_open(struct inode *inode, struct file *file)
{
	return single_open(file, data_show, inode->i_private);
}

static const struct file_operations data_fops = {
	.owner = THIS_MODULE,
	.open  = data_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t output_mode_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	char buffer[64];
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	sunxi_ledc_set_output_mode(led, buffer);

	*offp += count;

	return count;

err_out:
	dev_err(dev, "invalid parameter!\n");

	return -EINVAL;
}

static ssize_t output_mode_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%s\n", led->output_mode.str);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations output_mode_fops = {
	.owner = THIS_MODULE,
	.write = output_mode_write,
	.read  = output_mode_read,
};

static ssize_t trans_mode_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	char buffer[64];
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	sunxi_ledc_set_trans_mode(led, buffer);

	*offp += count;

	return count;

err_out:
	dev_err(dev, "invalid parameter!\n");

	return -EINVAL;
}


static ssize_t trans_mode_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led;

	r = snprintf(buffer, 64, "%s\n", led->trans_mode.str);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations trans_mode_fops = {
	.owner = THIS_MODULE,
	.write = trans_mode_write,
	.read  = trans_mode_read,
};

static ssize_t hwversion_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	u32 reg_val, major_ver, minor_ver;

	reg_val = sunxi_get_reg(LEDC_VER_NUM_REG);
	major_ver = reg_val >> 16;
	minor_ver = reg_val & 0xF;

	r = snprintf(buffer, 64, "r%up%u\n", major_ver, minor_ver);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations hwversion_fops = {
	.owner = THIS_MODULE,
	.read  = hwversion_read,
};

static void sunxi_led_create_debugfs(struct sunxi_led *led)
{
	struct dentry *debugfs_dir, *debugfs_file;
	struct device *dev = led->dev;

	debugfs_dir = debugfs_create_dir("sunxi_leds", NULL);
	if (IS_ERR_OR_NULL(debugfs_dir)) {
		dev_err(dev, "debugfs_create_dir failed!\n");
		return;
	}

	debugfs_file = debugfs_create_file("reset_ns", 0660,
				debugfs_dir, NULL, &reset_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for reset_ns failed!\n");

	debugfs_file = debugfs_create_file("t1h_ns", 0660,
				debugfs_dir, NULL, &t1h_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for t1h_ns failed!\n");

	debugfs_file = debugfs_create_file("t1l_ns", 0660,
				debugfs_dir, NULL, &t1l_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for t1l_ns failed!\n");

	debugfs_file = debugfs_create_file("t0h_ns", 0660,
				debugfs_dir, NULL, &t0h_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for t0h_ns failed!\n");

	debugfs_file = debugfs_create_file("t0l_ns", 0660,
				debugfs_dir, NULL, &t0l_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for t0l_ns failed!\n");

	debugfs_file = debugfs_create_file("wait_time0_ns", 0660,
				debugfs_dir, NULL, &wait_time0_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for wait_time0_ns failed!\n");

	debugfs_file = debugfs_create_file("wait_time1_ns", 0660,
				debugfs_dir, NULL, &wait_time1_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for wait_time1_ns failed!\n");

	debugfs_file = debugfs_create_file("wait_data_time_ns", 0660,
				debugfs_dir, NULL, &wait_data_time_ns_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for wait_data_time_ns failed!\n");

	debugfs_file = debugfs_create_file("data", 0440,
				debugfs_dir, NULL, &data_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for data failed!\n");

	debugfs_file = debugfs_create_file("output_mode", 0660,
				debugfs_dir, NULL, &output_mode_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for output_mode failed!\n");

	debugfs_file = debugfs_create_file("trans_mode", 0660,
				debugfs_dir, NULL, &trans_mode_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for trans_mode failed!\n");

	debugfs_file = debugfs_create_file("hwversion", 0440,
				debugfs_dir, NULL, &hwversion_fops);
	if (!debugfs_file)
		dev_err(dev, "debugfs_create_file for hwversion failed!\n");
}
#endif /* CONFIG_DEBUG_FS */

static void sunxi_ledc_dma_callback(void *param)
{
	struct sunxi_led *led = sunxi_led;

	dev_dbg(led->dev, "sunxi_ledc_dma_callback finish\n");
}

static void sunxi_ledc_trans_data(struct sunxi_led *led)
{
	int i, err;
	size_t size;
	u32 sub_length, delta_length;
	unsigned int slave_id;
	unsigned long flags;
	phys_addr_t dst_addr;
	struct dma_slave_config slave_config;
	struct device *dev = led->dev;
	struct dma_async_tx_descriptor *dma_desc;

	if (led->transmitted_data >= led->length)
		return;

	delta_length = led->length - led->transmitted_data;
	if (delta_length > SUNXI_LEDC_FIFO_TRIG_LEVEL)
		sub_length = SUNXI_LEDC_FIFO_TRIG_LEVEL;
	else
		sub_length = delta_length;

	switch (led->trans_mode.val) {
	case LEDC_TRANS_CPU_MODE:
		for (i = 0; i < sub_length; i++) {
			sunxi_set_reg(LEDC_DATA_REG_OFFSET,
				led->data[led->transmitted_data]);
			led->transmitted_data++;
		}

		break;

	case LEDC_TRANS_DMA_MODE:
		size = led->length * 4;
		led->src_dma = dma_map_single(dev, led->data,
					size, DMA_TO_DEVICE);

		dst_addr = SUNXI_LEDC_REG_BASE_ADDR + LEDC_DATA_REG_OFFSET;
		slave_id = sunxi_slave_id(DRQDST_LEDC, DRQSRC_SDRAM);
		flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK;

		slave_config.direction = DMA_MEM_TO_DEV;
		slave_config.src_addr = led->src_dma;
		slave_config.dst_addr = dst_addr;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_maxburst = 1;
		slave_config.dst_maxburst = 1;
		slave_config.slave_id = slave_id;
		err = dmaengine_slave_config(led->dma_chan, &slave_config);
		if (err < 0) {
			dev_err(dev, "dmaengine_slave_config failed!\n");
			dma_unmap_single(dev, led->src_dma,
					size, DMA_TO_DEVICE);
			return;
		}

		dma_desc = dmaengine_prep_slave_single(led->dma_chan,
							led->src_dma,
							size,
							DMA_MEM_TO_DEV,
							flags);
		if (!dma_desc) {
			dev_err(dev, "dmaengine_prep_slave_single failed!\n");
			dma_unmap_single(dev, led->src_dma,
					size, DMA_TO_DEVICE);
			return;
		}

		dma_desc->callback = sunxi_ledc_dma_callback;

		dmaengine_submit(dma_desc);
		dma_async_issue_pending(led->dma_chan);

		break;
	}

	if (!sunxi_ledc_is_enabled(led)) {
		sunxi_ledc_set_length(led);
		sunxi_ledc_enable(led);
	}
}

static inline void sunxi_ledc_clear_all_irq(void)
{
	u32 reg_val = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);

	reg_val &= ~0x1F;
	sunxi_set_reg(LEDC_INT_STS_REG_OFFSET, reg_val);
}

static inline void sunxi_ledc_clear_irq(enum sunxi_ledc_irq_status_reg irq)
{
	u32 reg_val = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);

	reg_val &= ~irq;
	sunxi_set_reg(LEDC_INT_STS_REG_OFFSET, reg_val);
}

static irqreturn_t sunxi_ledc_irq_handler(int irq, void *dev_id)
{
	unsigned long flags;
	long delta_time_ns;
	u32 irq_status, max_ns;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;
	struct timespec64 current_time;

	spin_lock_irqsave(&led->lock, flags);

	irq_status = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);

	sunxi_ledc_clear_all_irq();

	if (irq_status & LEDC_TRANS_FINISH_INT) {
		if (led->dma_chan)
			dma_unmap_single(dev,
					led->src_dma, led->length * 4,
					DMA_TO_DEVICE);
		sunxi_ledc_reset(led);
		led->length = 0;
		goto out;
	}

	if (irq_status & LEDC_WAITDATA_TIMEOUT_INT) {
		current_time = current_kernel_time64();
		delta_time_ns = current_time.tv_sec - led->start_time.tv_sec;
		delta_time_ns *= 1000 * 1000 * 1000;
		delta_time_ns += current_time.tv_nsec - led->start_time.tv_nsec;

		max_ns = led->wait_data_time_ns;

		if (delta_time_ns <= max_ns) {
			spin_unlock_irqrestore(&led->lock, flags);
			return IRQ_HANDLED;
		}

		if (led->dma_chan)
			dmaengine_terminate_all(led->dma_chan);

		sunxi_ledc_reset(led);

		if (delta_time_ns <= max_ns * 2) {
			sunxi_ledc_trans_data(led);
		} else {
			dev_err(dev,
				"wait time is more than %d ns, going to reset ledc and drop this operation!\n",
				max_ns);
			led->length = 0;
		}

		goto out;
	}

	if (irq_status & LEDC_FIFO_OVERFLOW_INT) {
		dev_err(dev,
			"there exists fifo overflow issue, irq_status=0x%x!\n",
			irq_status);

		sunxi_ledc_reset(led);
		sunxi_ledc_trans_data(led);

		goto out;
	}

	if (irq_status & LEDC_FIFO_CPUREQ_INT) {
		if (led->trans_mode.val == LEDC_TRANS_CPU_MODE
			&& led->transmitted_data <= led->length)
			sunxi_ledc_trans_data(led);
	}

out:
	spin_unlock_irqrestore(&led->lock, flags);

	return IRQ_HANDLED;
}

static int sunxi_ledc_irq_init(struct sunxi_led *led)
{
	int err;
	u32 reg_val = 0;
	struct device *dev = led->dev;
	unsigned long flags = 0;
	const char *name = "ledcirq";
	struct platform_device *pdev;

	pdev = container_of(dev, struct platform_device, dev);

	spin_lock_init(&led->lock);

	led->irqnum = platform_get_irq(pdev, 0);
	if (led->irqnum < 0)
		dev_err(dev, "failed to get ledc irq!\n");

	err = request_irq(led->irqnum, sunxi_ledc_irq_handler,
				flags, name, dev);
	if (err) {
		dev_err(dev,
			"failed to install IRQ handler for irqnum %d\n",
			led->irqnum);
		return -EPERM;
	}

	reg_val = sunxi_get_reg(LEDC_INT_CTRL_REG_OFFSET);
	reg_val |= LEDC_GLOBAL_INT_EN;
	reg_val |= LEDC_FIFO_OVERFLOW_INT_EN;
	reg_val |= LEDC_WAITDATA_TIMEOUT_INT_EN;
	if (led->trans_mode.val == LEDC_TRANS_CPU_MODE)
		reg_val |= LEDC_FIFO_CPUREQ_INT_EN;
	reg_val |= LEDC_TRANS_FINISH_INT_EN;

	sunxi_set_reg(LEDC_INT_CTRL_REG_OFFSET, reg_val);

	return 0;
}

static void sunxi_ledc_irq_deinit(struct sunxi_led *led)
{
	u32 reg_val;

	free_irq(led->irqnum, led->dev);

	reg_val = sunxi_get_reg(LEDC_INT_CTRL_REG_OFFSET);
	reg_val &= ~LEDC_TRANS_FINISH_INT_EN;
	reg_val &= ~LEDC_FIFO_CPUREQ_INT_EN;
	reg_val &= ~LEDC_WAITDATA_TIMEOUT_INT_EN;
	reg_val &= ~LEDC_FIFO_OVERFLOW_INT_EN;
	reg_val &= ~LEDC_GLOBAL_INT_EN;
	sunxi_set_reg(LEDC_INT_CTRL_REG_OFFSET, reg_val);
}

static void sunxi_ledc_pinctrl_init(struct sunxi_led *led)
{
	struct device *dev = led->dev;
	struct pinctrl *pinctrl = devm_pinctrl_get_select_default(dev);

	if (IS_ERR(pinctrl))
		dev_warn(dev, "devm_pinctrl_get_select_default failed!\n");
}

static int sunxi_set_led_brightness(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	unsigned long flags;
	u32 r, g, b, shift, old_data, new_data, length;
	struct sunxi_led_info *pinfo;
	struct sunxi_led_classdev_group *pcdev_group;
	struct sunxi_led *led = sunxi_led;

	pinfo = container_of(led_cdev, struct sunxi_led_info, cdev);

	switch (pinfo->type) {
	case LED_TYPE_R:
		pcdev_group = container_of(pinfo,
			struct sunxi_led_classdev_group, r);
		r = value;
		shift = 8;
		break;
	case LED_TYPE_G:
		pcdev_group = container_of(pinfo,
			struct sunxi_led_classdev_group, g);
		g = value;
		shift = 16;
		break;
	case LED_TYPE_B:
		pcdev_group = container_of(pinfo,
			struct sunxi_led_classdev_group, b);
		b = value;
		shift = 0;
		break;
	}

	old_data = led->data[pcdev_group->led_num];
	if (((old_data >> shift) & 0xFF) == value)
		return 0;

	if (pinfo->type != LED_TYPE_R)
		r = pcdev_group->r.cdev.brightness;
	if (pinfo->type != LED_TYPE_G)
		g = pcdev_group->g.cdev.brightness;
	if (pinfo->type != LED_TYPE_B)
		b = pcdev_group->b.cdev.brightness;

	/* LEDC treats input data as GRB by default */
	new_data = (g << 16) | (r << 8) | b;

	length = pcdev_group->led_num + 1;

	spin_lock_irqsave(&led->lock, flags);

	led->data[pcdev_group->led_num] = new_data;

	led->length = length;

	led->start_time = current_kernel_time64();

	sunxi_ledc_trans_data(led);

	spin_unlock_irqrestore(&led->lock, flags);

	return 0;
}

static int sunxi_register_led_classdev(struct sunxi_led *led)
{
	int i, err;
	size_t size;
	struct device *dev = led->dev;
	struct led_classdev *pcdev;

	if (!led->led_count)
		led->led_count = SUNXI_DEFAULT_LED_COUNT;

	size = sizeof(struct sunxi_led_classdev_group) * led->led_count;
	led->pcdev_group = kzalloc(size, GFP_KERNEL);
	if (!led->pcdev_group)
		return -ENOMEM;

	for (i = 0; i < led->led_count; i++) {
		led->pcdev_group[i].r.type = LED_TYPE_R;
		pcdev = &led->pcdev_group[i].r.cdev;
		pcdev->name = kzalloc(16, GFP_KERNEL);
		sprintf((char *)pcdev->name, "sunxi_led%dr", i);
		pcdev->brightness = LED_OFF;
		pcdev->brightness_set_blocking = sunxi_set_led_brightness;
		pcdev->dev = dev;
		err = led_classdev_register(dev, pcdev);
		if (err < 0) {
			dev_err(dev,
				"led_classdev_register %s failed!\n",
				pcdev->name);
			return err;
		}

		led->pcdev_group[i].g.type = LED_TYPE_G;
		pcdev = &led->pcdev_group[i].g.cdev;
		pcdev->name = kzalloc(16, GFP_KERNEL);
		sprintf((char *)pcdev->name, "sunxi_led%dg", i);
		pcdev->brightness = LED_OFF;
		pcdev->brightness_set_blocking = sunxi_set_led_brightness;
		pcdev->dev = dev;
		err = led_classdev_register(dev, pcdev);
		if (err < 0) {
			dev_err(dev,
			"led_classdev_register %s failed!\n",
			pcdev->name);
			return err;
		}

		led->pcdev_group[i].b.type = LED_TYPE_B;
		pcdev = &led->pcdev_group[i].b.cdev;
		pcdev->name = kzalloc(16, GFP_KERNEL);
		sprintf((char *)pcdev->name, "sunxi_led%db", i);
		pcdev->brightness = LED_OFF;
		pcdev->brightness_set_blocking = sunxi_set_led_brightness;
		pcdev->dev = dev;
		err = led_classdev_register(dev, pcdev);
		if (err < 0) {
			dev_err(dev,
				"led_classdev_register %s failed!\n",
				pcdev->name);
			return err;
		}

		led->pcdev_group[i].led_num = i;
	}

	size = sizeof(u32) * led->led_count;
	led->data = kzalloc(size, GFP_KERNEL);
	if (!led->data)
		return -ENOMEM;

	return 0;
}

static void sunxi_unregister_led_classdev(struct sunxi_led *led)
{
	int i;

	for (i = 0; i < led->led_count; i++) {
		kfree(led->pcdev_group[i].r.cdev.name);
		kfree(led->pcdev_group[i].g.cdev.name);
		kfree(led->pcdev_group[i].b.cdev.name);
		led_classdev_unregister(&led->pcdev_group[i].r.cdev);
		led_classdev_unregister(&led->pcdev_group[i].g.cdev);
		led_classdev_unregister(&led->pcdev_group[i].b.cdev);
	}

	kfree(led->pcdev_group);
	kfree(led->data);
}

static inline int sunxi_get_u32_of_property(const char *propname, int *val)
{
	int err;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	err = of_property_read_u32(np, propname, val);
	if (err < 0)
		dev_warn(dev,
			"failed to get the value of propname %s!\n",
			propname);

	return err;
}

static inline int sunxi_get_str_of_property(const char *propname,
					const char **out_string)
{
	int err;
	struct sunxi_led *led = sunxi_led;
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	err = of_property_read_string(np, propname, out_string);
	if (err < 0)
		dev_warn(dev,
			"failed to get the string of propname %s!\n",
			propname);

	return err;
}

static void sunxi_get_para_of_property(struct sunxi_led *led)
{
	int err;
	u32 val;
	const char *str;

	err = sunxi_get_u32_of_property("led_count", &val);
	if (!err)
		led->led_count = val;

	memcpy(led->output_mode.str, "GRB", 3);
	led->output_mode.val = SUNXI_OUTPUT_GRB;
	err = sunxi_get_str_of_property("output_mode", &str);
	if (!err)
		if (!strncmp(str, "BRG", 3) ||
			!strncmp(str, "GBR", 3) ||
			!strncmp(str, "RGB", 3) ||
			!strncmp(str, "RBG", 3) ||
			!strncmp(str, "BGR", 3) ||
			!strncmp(str, "BRG", 3))
			memcpy(led->output_mode.str, str, 3);

	memcpy(led->trans_mode.str, "DMA", 3);
	led->trans_mode.val = LEDC_TRANS_DMA_MODE;
	err = sunxi_get_str_of_property("trans_mode", &str);
	if (!err)
		if (!strncmp(str, "CPU", 3) || !strncmp(str, "DMA", 3))
			memcpy(led->trans_mode.str, str, 3);

	err = sunxi_get_u32_of_property("reset_ns", &val);
	if (!err)
		led->reset_ns = val;

	err = sunxi_get_u32_of_property("t1h_ns", &val);
	if (!err)
		led->t1h_ns = val;

	err = sunxi_get_u32_of_property("t1l_ns", &val);
	if (!err)
		led->t1l_ns = val;

	err = sunxi_get_u32_of_property("t0h_ns", &val);
	if (!err)
		led->t0h_ns = val;

	err = sunxi_get_u32_of_property("t0l_ns", &val);
	if (!err)
		led->t0l_ns = val;

	err = sunxi_get_u32_of_property("wait_time0_ns", &val);
	if (!err)
		led->wait_time0_ns = val;

	err = sunxi_get_u32_of_property("wait_time1_ns", &val);
	if (!err)
		led->wait_time1_ns = val;

	err = sunxi_get_u32_of_property("wait_data_time_ns", &val);
	if (!err)
		led->wait_data_time_ns = val;
}

static int sunxi_led_probe(struct platform_device *pdev)
{
	int err;
	dma_cap_mask_t mask;
	struct sunxi_led *led;
	struct device *dev = &pdev->dev;

	led = kzalloc(sizeof(struct sunxi_led), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	sunxi_led = led;

	led->dev = dev;

	led->output_mode.str = kzalloc(3, GFP_KERNEL);
	if (!led->output_mode.str)
		return -ENOMEM;

	led->trans_mode.str = kzalloc(3, GFP_KERNEL);
	if (!led->trans_mode.str)
		return -ENOMEM;

	sunxi_get_para_of_property(led);

	err = sunxi_register_led_classdev(led);
	if (err)
		return err;

	/* Registers initialization */
	led->iomem_reg_base = ioremap(SUNXI_LEDC_REG_BASE_ADDR,
					LEDC_TOTAL_REG_SIZE);
	sunxi_ledc_set_time(led);
	sunxi_ledc_set_trans_mode(led, NULL);
	sunxi_ledc_set_output_mode(led, NULL);

	sunxi_clk_init(led);

	err = sunxi_ledc_irq_init(led);
	if (err)
		return err;

	sunxi_ledc_pinctrl_init(led);

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	led->dma_chan = dma_request_channel(mask, NULL, NULL);
	if (!led->dma_chan) {
		dev_err(dev, "failed to get the DMA channel!\n");
		return 0;
	}

#ifdef CONFIG_DEBUG_FS
		sunxi_led_create_debugfs(led);
#endif /* CONFIG_DEBUG_FS */

	return 0;
}

static int sunxi_led_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_led *led = dev_get_drvdata(dev);

	sunxi_ledc_irq_deinit(led);

	sunxi_unregister_led_classdev(led);

	iounmap(led->iomem_reg_base);
	led->iomem_reg_base = NULL;

	sunxi_clk_deinit(led);

	if (led->dma_chan)
		dma_release_channel(led->dma_chan);

	kfree(led->output_mode.str);
	kfree(led->trans_mode.str);
	kfree(led);

	return 0;
}

static const struct of_device_id sunxi_led_dt_ids[] = {
	{.compatible = "allwinner,sunxi-leds"},
	{},
};

static struct platform_driver sunxi_led_driver = {
	.probe		= sunxi_led_probe,
	.remove		= sunxi_led_remove,
	.driver		= {
		.name	= "sunxi-leds",
		.of_match_table = sunxi_led_dt_ids,
	},
};

module_platform_driver(sunxi_led_driver);

MODULE_AUTHOR("Albert Yu <yuxyun@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner LED driver");
MODULE_LICENSE("GPL v2");
