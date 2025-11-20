/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2018 Allwinner Technology Limited. All rights reserved.
 *      http://www.allwinnertech.com
 *
 * Author : Albert Yu <yuxyun@allwinnertech.com>
 *	   Lewis <liuyu@allwinnertech.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif

#define HEXADECIMAL	(0x10)
#define REG_INTERVAL	(0x04)
#define REG_CL		(0x0c)

#define RESULT_COMPLETE	1
#define RESULT_ERR	2

#define SUNXI_LEDC_REG_BASE_ADDR 0x06700000

#define SUNXI_LEDC_MAX_LED_COUNT 1024

#define SUNXI_LEDC_DEFAULT_LED_COUNT 8

#define SUNXI_LEDC_RESET_TIME_MIN_NS 84
#define SUNXI_LEDC_RESET_TIME_MAX_NS 327000

#define SUNXI_LEDC_TIMEREG_CONSTANT 42

#define SUNXI_LEDC_DEFAULT_RESET_NS 84
#define SUNXI_LEDC_DEFAULT_T1H_NS 800
#define SUNXI_LEDC_DEFAULT_T1L_NS 320
#define SUNXI_LEDC_DEFAULT_T0H_NS 300
#define SUNXI_LEDC_DEFAULT_T0L_NS 800
#define SUNXI_LEDC_DEFAULT_WAIT_TIMA0_NS 84
#define SUNXI_LEDC_DEFAULT_WAIT_TIME1_NS 84
#define SUNXI_LEDC_DEFAULT_WAIT_DATA_TIME_NS 600000

#define SUNXI_LEDC_T1H_MIN_NS 84
#define SUNXI_LEDC_T1H_MAX_NS 2560

#define SUNXI_LEDC_T1L_MIN_NS 84
#define SUNXI_LEDC_T1L_MAX_NS 1280

#define SUNXI_LEDC_T0H_MIN_NS 84
#define SUNXI_LEDC_T0H_MAX_NS 1280

#define SUNXI_LEDC_T0L_MIN_NS 84
#define SUNXI_LEDC_T0L_MAX_NS 2560

#define SUNXI_LEDC_WAIT_TIME0_MIN_NS 84
#define SUNXI_LEDC_WAIT_TIME0_MAX_NS 10000

#define SUNXI_LEDC_WAIT_TIME1_MIN_NS 84
#define SUNXI_LEDC_WAIT_TIME1_MAX_NS 85000000000

#define SUNXI_LEDC_WAIT_DATA_TIME_MIN_NS 84
#define SUNXI_LEDC_WAIT_DATA_TIME_MAX_NS_IC 655000
#define SUNXI_LEDC_WAIT_DATA_TIME_MAX_NS_FPGA 20000000

#define SUNXI_LEDC_FIFO_DEPTH 32 /* 32 * 4 bytes */
#define SUNXI_LEDC_FIFO_TRIG_LEVEL 15

#define SUNXI_LEDC_T1H_TIME_MASK	0x3F
#define SUNXI_LEDC_T1H_TIME_SHIFT	21

#define SUNXI_LEDC_T1L_TIME_MASK	0x1F
#define SUNXI_LEDC_T1L_TIME_SHIFT	16

#define SUNXI_LEDC_T0H_TIME_MASK	0x1F
#define SUNXI_LEDC_T0H_TIME_SHIFT	6

#define SUNXI_LEDC_T0L_TIME_MASK	0x3F

#define SUNXI_LEDC_RESET_TIME_MASK	0x1FFF
#define SUNXI_LEDC_RESET_TIME_SHIFT	16

#define SUNXI_LEDC_LENGTH_MASK	0x1FFF
#define SUNXI_LEDC_LENGTH_SHIFT	16
#define SUNXI_LEDC_LEDNUMS_MASK	0x3FF

#define SUNXI_LEDC_OUTPUT_MODE_MASK	0x7
#define SUNXI_LEDC_OUTPUT_MODE_SHIFT	6

#define SUNXI_LEDC_DMA_ENABLE	1
#define SUNXI_LEDC_DME_ENABLE_SHIFT	5

#if IS_ENABLED(AW_FPGA_BOARD)
#define SUNXI_FPGA_LEDC
#endif

enum sunxi_ledc_output_mode_val {
	SUNXI_LEDC_OUTPUT_GRB = 0b000 << 6,
	SUNXI_LEDC_OUTPUT_GBR = 0b001 << 6,
	SUNXI_LEDC_OUTPUT_RGB = 0b010 << 6,
	SUNXI_LEDC_OUTPUT_RBG = 0b011 << 6,
	SUNXI_LEDC_OUTPUT_BGR = 0b100 << 6,
	SUNXI_LEDC_OUTPUT_BRG = 0b101 << 6
};

struct sunxi_ledc_output_mode {
	char *str;
	enum sunxi_ledc_output_mode_val val;
};

enum sunxi_ledc_trans_mode_val {
	LEDC_TRANS_CPU_MODE,
	LEDC_TRANS_DMA_MODE
};

enum sunxi_ledc_reg {
	LEDC_CTRL_REG_OFFSET              = 0x00,
	LED_T01_TIMING_CTRL_REG_OFFSET    = 0x04,
	LEDC_DATA_FINISH_CNT_REG_OFFSET   = 0x08,
	LED_RESET_TIMING_CTRL_REG_OFFSET  = 0x0c,
	LEDC_WAIT_TIME0_CTRL_REG          = 0x10,
	LEDC_DATA_REG_OFFSET              = 0x14,
	LEDC_DMA_CTRL_REG                 = 0x18,
	LEDC_INT_CTRL_REG_OFFSET          = 0x1c,
	LEDC_INT_STS_REG_OFFSET           = 0x20,
	LEDC_WAIT_TIME1_CTRL_REG          = 0x28,
	LEDC_VER_NUM_REG                  = 0x2c,
	LEDC_FIFO_DATA                    = 0x30,
	LEDC_TOTAL_REG_SIZE = LEDC_FIFO_DATA + SUNXI_LEDC_FIFO_DEPTH
};

enum sunxi_ledc_irq_ctrl_reg {
	LEDC_TRANS_FINISH_INT_EN     = (1 << 0),
	LEDC_FIFO_CPUREQ_INT_EN      = (1 << 1),
	LEDC_WAITDATA_TIMEOUT_INT_EN = (1 << 3),
	LEDC_FIFO_OVERFLOW_INT_EN    = (1 << 4),
	LEDC_GLOBAL_INT_EN           = (1 << 5),
};

enum sunxi_ledc_irq_status_reg {
	LEDC_TRANS_FINISH_INT     = (1 << 0),
	LEDC_FIFO_CPUREQ_INT      = (1 << 1),
	LEDC_WAITDATA_TIMEOUT_INT = (1 << 3),
	LEDC_FIFO_OVERFLOW_INT    = (1 << 4),
	LEDC_FIFO_FULL            = (1 << 16),
	LEDC_FIFO_EMPTY           = (1 << 17),
};

enum sunxi_ledc_type {
	LED_TYPE_R,
	LED_TYPE_G,
	LED_TYPE_B
};

struct sunxi_ledc_info {
	enum sunxi_ledc_type type;
	struct led_classdev cdev;
};

struct sunxi_ledc_classdev_group {
	u32 led_num;
	struct sunxi_ledc_info r;
	struct sunxi_ledc_info g;
	struct sunxi_ledc_info b;
	void *private;
};

static u32 sunxi_ledc_regs_offset[] = {
	LEDC_CTRL_REG_OFFSET,
	LED_T01_TIMING_CTRL_REG_OFFSET,
	LED_RESET_TIMING_CTRL_REG_OFFSET,
	LEDC_WAIT_TIME0_CTRL_REG,
	LEDC_DMA_CTRL_REG,
	LEDC_WAIT_TIME1_CTRL_REG,
	LEDC_INT_CTRL_REG_OFFSET,
#ifndef SUNXI_FPGA_LEDC
	LEDC_DATA_FINISH_CNT_REG_OFFSET,
#endif
};

struct sunxi_ledc {
	u32 reset_ns;
	u32 t1h_ns;
	u32 t1l_ns;
	u32 t0h_ns;
	u32 t0l_ns;
	u32 wait_time0_ns;
	unsigned long long wait_time1_ns;
	u32 wait_data_time_ns;
	u32 irqnum;
	u32 led_count;
	u32 *data;
	u32 length;
	u8 result;
	spinlock_t lock;
	struct mutex mutex_lock;
	struct platform_device *pdev;
	struct device *dev;
	dma_addr_t src_dma;
	struct dma_chan *dma_chan;
	wait_queue_head_t wait;
	struct timespec64 start_time;
	struct clk *clk_ledc;
	struct clk *clk_cpuapb;
	struct pinctrl *pctrl;
	void __iomem *base_addr;
	struct resource	*res;
	struct sunxi_ledc_output_mode output_mode;
	struct sunxi_ledc_classdev_group *pcdev_group;
	struct dentry *debugfs_dir;
	char regulator_id[16];
	struct regulator *regulator;
	struct reset_control *reset;
	u32 regs_backup[ARRAY_SIZE(sunxi_ledc_regs_offset)];
#if IS_ENABLED(CONFIG_ARCH_SUN8IW22)
	enum led_brightness saved_r;
	enum led_brightness saved_g;
	enum led_brightness saved_b;
#endif
};

static struct class *led_class;

#define sunxi_slave_id(d, s) (((d)<<16) | (s))

#if IS_ENABLED(CONFIG_PM)
static int sunxi_ledc_power_on(struct sunxi_ledc *led);
static int sunxi_led_power_off(struct sunxi_ledc *led);
#endif

static void sunxi_ledc_dump_reg(struct sunxi_ledc *led, u32 offset, u32 len)
{
	u32 i;
	u8 buf[64], cnt = 0;

	for (i = 0; i < len; i = i + REG_INTERVAL) {
		if (i % HEXADECIMAL == 0)
			cnt += sprintf(buf + cnt, "0x%08x:", (u32)(led->res->start + offset + i));

		cnt += sprintf(buf + cnt, "%08x ", readl(led->base_addr + offset + i));

		if (i % HEXADECIMAL == REG_CL) {
			pr_warn("%s\n", buf);
			cnt = 0;
		}
	}
}

static void sunxi_ledc_enable_irq(u32 mask, struct sunxi_ledc *led)
{
	u32 reg_val = 0;

	reg_val |= mask;
	writel(reg_val, led->base_addr + LEDC_INT_CTRL_REG_OFFSET);
}

static void sunxi_ledc_disable_irq(u32 mask, struct sunxi_ledc *led)
{
	u32 reg_val = 0;

	reg_val = readl(led->base_addr + LEDC_INT_CTRL_REG_OFFSET);
	reg_val &= ~mask;
	writel(reg_val, led->base_addr + LEDC_INT_CTRL_REG_OFFSET);
}

static void sunxi_ledc_enable(struct sunxi_ledc *led)
{
	u32 reg_val;

	reg_val = readl(led->base_addr + LEDC_CTRL_REG_OFFSET);
	reg_val |=  BIT(0);
	writel(reg_val, led->base_addr + LEDC_CTRL_REG_OFFSET);
}

static int sunxi_ledc_clk_get(struct sunxi_ledc *led)
{
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	led->reset = devm_reset_control_get(led->dev, NULL);
	if (IS_ERR_OR_NULL(led->reset)) {
		sunxi_err(led->dev, "request reset failed\n");
		return -EINVAL;
	}

	led->clk_ledc = of_clk_get(np, 0);
	if (IS_ERR(led->clk_ledc)) {
		sunxi_err(led->dev, "failed to get clk_ledc!\n");
		return -EINVAL;
	}

	led->clk_cpuapb = of_clk_get(np, 1);
	if (IS_ERR(led->clk_cpuapb)) {
		clk_put(led->clk_ledc);
		sunxi_err(led->dev, "failed to get clk_cpuapb!\n");
		return -EINVAL;
	}

	return 0;
}

static void sunxi_ledc_clk_put(struct sunxi_ledc *led)
{
	clk_put(led->clk_cpuapb);
	clk_put(led->clk_ledc);
}

static int sunxi_ledc_clk_enable(struct sunxi_ledc *led)
{
	int ret;

	ret = reset_control_deassert(led->reset);
	if (ret) {
		sunxi_err(led->dev, "deassert clk error, ret:%d\n", ret);
		return -EINVAL;
	}

	ret = clk_prepare_enable(led->clk_ledc);
	if (ret) {
		sunxi_err(led->dev, "enable clk_ledc error, ret:%d\n", ret);
		goto err0;
	}

	ret = clk_prepare_enable(led->clk_cpuapb);
	if (ret) {
		sunxi_err(led->dev, "enable clk_cpuapb error, ret:%d\n", ret);
		goto err1;
	}

	return 0;

err1:
	clk_disable_unprepare(led->clk_ledc);
err0:
	reset_control_assert(led->reset);
	return -EINVAL;
}

static void sunxi_ledc_clk_disable(struct sunxi_ledc *led)
{
	clk_disable_unprepare(led->clk_cpuapb);
	clk_disable_unprepare(led->clk_ledc);
	reset_control_assert(led->reset);
}

static int sunxi_ledc_set_reset_ns(struct sunxi_ledc *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_LEDC_RESET_TIME_MIN_NS;
	u32 max = SUNXI_LEDC_RESET_TIME_MAX_NS;

	if (led->reset_ns < min || led->reset_ns > max) {
		sunxi_err(led->dev, "invalid parameter, reset_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

	n = (led->reset_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT; /* tr_time = 42ns * (N + 1) */
	reg_val = readl(led->base_addr + LED_RESET_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(SUNXI_LEDC_RESET_TIME_MASK << SUNXI_LEDC_RESET_TIME_SHIFT);
	reg_val |= (n << SUNXI_LEDC_RESET_TIME_SHIFT);
	writel(reg_val, led->base_addr + LED_RESET_TIMING_CTRL_REG_OFFSET);

	return 0;
}

static int sunxi_ledc_set_t1h_ns(struct sunxi_ledc *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_LEDC_T1H_MIN_NS;
	u32 max = SUNXI_LEDC_T1H_MAX_NS;

	if (led->t1h_ns < min || led->t1h_ns > max) {
		sunxi_err(led->dev, "invalid parameter, t1h_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

	n = (led->t1h_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT; /* t1h_time = 42ns * (N + 1) */
	reg_val = readl(led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(SUNXI_LEDC_T1H_TIME_MASK << SUNXI_LEDC_T1H_TIME_SHIFT);
	reg_val |= n << SUNXI_LEDC_T1H_TIME_SHIFT ;
	writel(reg_val, led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);

	return 0;
}

static int sunxi_ledc_set_t1l_ns(struct sunxi_ledc *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_LEDC_T1L_MIN_NS;
	u32 max = SUNXI_LEDC_T1L_MAX_NS;

	if (led->t1l_ns < min || led->t1l_ns > max) {
		sunxi_err(led->dev, "invalid parameter, t1l_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

	n = (led->t1l_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT; /* t1l_time = 42ns * (N + 1) */
	reg_val = readl(led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(SUNXI_LEDC_T1L_TIME_MASK << SUNXI_LEDC_T1L_TIME_SHIFT);
	reg_val |= n << SUNXI_LEDC_T1L_TIME_SHIFT;
	writel(reg_val, led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);

	return 0;
}

static int sunxi_ledc_set_t0h_ns(struct sunxi_ledc *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_LEDC_T0H_MIN_NS;
	u32 max = SUNXI_LEDC_T0H_MAX_NS;

	if (led->t0h_ns < min || led->t0h_ns > max) {
		sunxi_err(led->dev, "invalid parameter, t0h_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

	n = (led->t0h_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT; /* t0h_time = 42ns * (N + 1) */
	reg_val = readl(led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(SUNXI_LEDC_T0H_TIME_MASK << SUNXI_LEDC_T0H_TIME_SHIFT);
	reg_val |= n << SUNXI_LEDC_T0H_TIME_SHIFT;
	writel(reg_val, led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);

	return 0;
}

static int sunxi_ledc_set_t0l_ns(struct sunxi_ledc *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_LEDC_T0L_MIN_NS;
	u32 max = SUNXI_LEDC_T0L_MAX_NS;

	if (led->t0l_ns < min || led->t0l_ns > max) {
		sunxi_err(led->dev, "invalid parameter, t0l_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

	n = (led->t0l_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT;
	reg_val = readl(led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~SUNXI_LEDC_T0L_TIME_MASK;
	reg_val |= n;
	writel(reg_val, led->base_addr + LED_T01_TIMING_CTRL_REG_OFFSET);

	return 0;
}

static int sunxi_ledc_set_wait_time0_ns(struct sunxi_ledc *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_LEDC_WAIT_TIME0_MIN_NS;
	u32 max = SUNXI_LEDC_WAIT_TIME0_MAX_NS;

	if (led->wait_time0_ns < min || led->wait_time0_ns > max) {
		sunxi_err(led->dev, "invalid parameter, wait_time0_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

	n = (led->wait_time0_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT; /* wait_time0 = 42ns * (N + 1) */
	reg_val = (1 << 8) | n;
	writel(reg_val, led->base_addr + LEDC_WAIT_TIME0_CTRL_REG);

	return 0;
}

static int sunxi_ledc_set_wait_time1_ns(struct sunxi_ledc *led)
{
	unsigned long long tmp, max = SUNXI_LEDC_WAIT_TIME1_MAX_NS;
	u32 min = SUNXI_LEDC_WAIT_TIME1_MIN_NS;
	u32 n, reg_val;

	if (led->wait_time1_ns < min || led->wait_time1_ns > max) {
		sunxi_err(led->dev, "invalid parameter, wait_time1_ns should be %u-%llu!\n", min, max);
		return -EINVAL;
	}

	tmp = led->wait_time1_ns;
	n = div_u64(tmp, 42);
	n -= 1;
	reg_val = (1 << 31) | n;
	writel(reg_val, led->base_addr + LEDC_WAIT_TIME1_CTRL_REG);

	return 0;
}

static int sunxi_ledc_set_wait_data_time_ns(struct sunxi_ledc *led)
{
	u32 min, max;
#if IS_ENABLED(AW_IC_BOARD)
	u32 mask = 0x1FFF, shift = 16, reg_val = 0, n;
#endif
	min = SUNXI_LEDC_WAIT_DATA_TIME_MIN_NS;
#if IS_ENABLED(AW_FPGA_BOARD)
	/*
	 * For FPGA platforms, it is easy to meet wait data timeout for
	 * the obvious latency of task which is because of less cpu cores
	 * and lower cpu frequency compared with IC platforms, so here we
	 * permit long enough time latency.
	 */
	max = SUNXI_LEDC_WAIT_DATA_TIME_MAX_NS_FPGA;
#else /* SUNXI_FPGA_LEDC */
	max = SUNXI_LEDC_WAIT_DATA_TIME_MAX_NS_IC;
#endif /* SUNXI_FPGA_LEDC */

	if (led->wait_data_time_ns < min || led->wait_data_time_ns > max) {
		sunxi_err(led->dev, "invalid parameter, wait_data_time_ns should be %u-%u!\n", min, max);
		return -EINVAL;
	}

#if IS_ENABLED(AW_IC_BOARD)
	n = (led->wait_data_time_ns - SUNXI_LEDC_TIMEREG_CONSTANT) / SUNXI_LEDC_TIMEREG_CONSTANT;
	reg_val &= ~(mask << shift);
	reg_val |= (n << shift);
	writel(reg_val, led->base_addr + LEDC_DATA_FINISH_CNT_REG_OFFSET);
#endif /* SUNXI_FPGA_LEDC */

	return 0;
}

static void sunxi_ledc_set_time(struct sunxi_ledc *led)
{
	sunxi_ledc_set_reset_ns(led);
	sunxi_ledc_set_t1h_ns(led);
	sunxi_ledc_set_t1l_ns(led);
	sunxi_ledc_set_t0h_ns(led);
	sunxi_ledc_set_t0l_ns(led);
	sunxi_ledc_set_wait_time0_ns(led);
	sunxi_ledc_set_wait_time1_ns(led);
	sunxi_ledc_set_wait_data_time_ns(led);
}

static int sunxi_ledc_set_length(struct sunxi_ledc *led)
{
	u32 reg_val;

	if ((led->length == 0) || (led->length > led->led_count))
		return -EINVAL;

	reg_val = readl(led->base_addr + LEDC_CTRL_REG_OFFSET);
	reg_val &= ~(SUNXI_LEDC_LENGTH_MASK << SUNXI_LEDC_LENGTH_SHIFT);
	reg_val |=  led->length << SUNXI_LEDC_LENGTH_SHIFT;
	writel(reg_val, led->base_addr + LEDC_CTRL_REG_OFFSET);

	reg_val = readl(led->base_addr + LED_RESET_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~SUNXI_LEDC_LEDNUMS_MASK;
	reg_val |= led->length - 1;
	writel(reg_val, led->base_addr + LED_RESET_TIMING_CTRL_REG_OFFSET);

	return 0;
}

static void sunxi_ledc_set_output_mode(struct sunxi_ledc *led, const char *str)
{
	u32 val;
	u32 reg_val = readl(led->base_addr + LEDC_CTRL_REG_OFFSET);

	if (str != NULL) {
		if (!strncmp(str, "GRB", 3))
			val = SUNXI_LEDC_OUTPUT_GRB;
		else if (!strncmp(str, "GBR", 3))
			val = SUNXI_LEDC_OUTPUT_GBR;
		else if (!strncmp(str, "RGB", 3))
			val = SUNXI_LEDC_OUTPUT_RGB;
		else if (!strncmp(str, "RBG", 3))
			val = SUNXI_LEDC_OUTPUT_RBG;
		else if (!strncmp(str, "BGR", 3))
			val = SUNXI_LEDC_OUTPUT_BGR;
		else if (!strncmp(str, "BRG", 3))
			val = SUNXI_LEDC_OUTPUT_BRG;
		else
			return;
	} else {
		val = led->output_mode.val;
	}

	reg_val &= ~(SUNXI_LEDC_OUTPUT_MODE_MASK << SUNXI_LEDC_OUTPUT_MODE_SHIFT);
	reg_val |= val;

	writel(reg_val, led->base_addr + LEDC_CTRL_REG_OFFSET);

	if (str != NULL) {
		if (strncmp(str, led->output_mode.str, 3))
			memcpy(led->output_mode.str, str, 3);
	}

	if (val != led->output_mode.val)
		led->output_mode.val = val;
}

static void sunxi_ledc_reset(struct sunxi_ledc *led)
{
	u32 reg_val = readl(led->base_addr + LEDC_CTRL_REG_OFFSET);

	sunxi_ledc_disable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_FIFO_CPUREQ_INT_EN
			| LEDC_WAITDATA_TIMEOUT_INT_EN | LEDC_FIFO_OVERFLOW_INT_EN
			| LEDC_GLOBAL_INT_EN, led);

	dev_dbg(led->dev, "dump reg:\n");
	sunxi_ledc_dump_reg(led, 0, 0x30);

	reg_val |= 1 << 1;
	writel(reg_val, led->base_addr + LEDC_CTRL_REG_OFFSET);
}

static ssize_t sunxi_ledc_reset_ns_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_RESET_TIME_MIN_NS;
	max = SUNXI_LEDC_RESET_TIME_MAX_NS;

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->reset_ns = val;
	sunxi_ledc_set_reset_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, reset_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_reset_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->reset_ns);

}

static ssize_t sunxi_ledc_t1h_ns_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_T1H_MIN_NS;
	max = SUNXI_LEDC_T1H_MAX_NS;

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t1h_ns = val;

	sunxi_ledc_set_t1h_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, t1h_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_t1h_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->t1h_ns);
}

static ssize_t sunxi_ledc_t1l_ns_store(struct device *dev, struct device_attribute *attr,
			const char __user *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_T1L_MIN_NS;
	max = SUNXI_LEDC_T1L_MAX_NS;

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t1l_ns = val;
	sunxi_ledc_set_t1l_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, t1l_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_t1l_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->t1l_ns);
}

static ssize_t sunxi_ledc_t0h_ns_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_T0H_MIN_NS;
	max = SUNXI_LEDC_T0H_MAX_NS;

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t0h_ns = val;
	sunxi_ledc_set_t0h_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, t0h_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_t0h_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->t0h_ns);
}

static ssize_t sunxi_ledc_t0l_ns_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_T0L_MIN_NS;
	max = SUNXI_LEDC_T0L_MAX_NS;

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t0l_ns = val;
	sunxi_ledc_set_t0l_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, t0l_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_t0l_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->t0l_ns);
}

static ssize_t sunxi_ledc_wait_time0_ns_store(struct device *dev, struct device_attribute *attr,
				const char __user *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_WAIT_TIME0_MIN_NS;
	max = SUNXI_LEDC_WAIT_TIME0_MAX_NS;

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_time0_ns = val;
	sunxi_ledc_set_wait_time0_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, wait_time0_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_wait_time0_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->wait_time0_ns);
}

static ssize_t sunxi_ledc_wait_time1_ns_store(struct device *dev, struct device_attribute *attr,
				const char __user *buf, size_t count)
{
	int err;
	u32 min;
	unsigned long long max;
	unsigned long long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_WAIT_TIME1_MIN_NS;
	max = SUNXI_LEDC_WAIT_TIME1_MAX_NS;

	err = kstrtoull(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_time1_ns = val;
	sunxi_ledc_set_wait_time1_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, wait_time1_ns should be %u-%lld!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_wait_time1_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%lld\n", led->wait_time1_ns);
}

static ssize_t sunxi_ledc_wait_data_time_ns_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	u32 min, max;
	unsigned long val;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	min = SUNXI_LEDC_WAIT_DATA_TIME_MIN_NS;
#ifdef SUNXI_FPGA_LEDC
	max = SUNXI_LEDC_WAIT_DATA_TIME_MAX_NS_FPGA;
#else
	max = SUNXI_LEDC_WAIT_DATA_TIME_MAX_NS_IC;
#endif

	err = kstrtoul(buf, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_data_time_ns = val;
	sunxi_ledc_set_wait_data_time_ns(led);

	return count;

err_out:
	sunxi_err(led->dev, "invalid parameter, wait_data_time_ns should be %u-%u!\n", min, max);

	return -EINVAL;
}

static ssize_t sunxi_ledc_wait_data_time_ns_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", led->wait_data_time_ns);
}

static ssize_t sunxi_ledc_output_mode_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	sunxi_ledc_set_output_mode(led, buf);

	return count;
}

static ssize_t sunxi_ledc_output_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%s\n", led->output_mode.str);
}

static void sunxi_ledc_set_dma_mode(struct sunxi_ledc *led)
{
	u32 reg_val = 0;

	reg_val |= SUNXI_LEDC_DMA_ENABLE << SUNXI_LEDC_DME_ENABLE_SHIFT;
	writel(reg_val, led->base_addr + LEDC_DMA_CTRL_REG);

	sunxi_ledc_disable_irq(LEDC_FIFO_CPUREQ_INT_EN, led);
}

static void sunxi_ledc_set_cpu_mode(struct sunxi_ledc *led)
{
	u32 reg_val = 0;

	reg_val &= ~(SUNXI_LEDC_DMA_ENABLE << SUNXI_LEDC_DME_ENABLE_SHIFT);
	writel(reg_val, led->base_addr + LEDC_DMA_CTRL_REG);

	sunxi_ledc_enable_irq(LEDC_FIFO_CPUREQ_INT_EN, led);
}

static void sunxi_ledc_dma_callback(void *param)
{
	struct sunxi_ledc *led = (struct sunxi_ledc *)param;
	sunxi_info(led->dev, "dma xfer finish!\n");
}

static void sunxi_ledc_trans_data(struct sunxi_ledc *led)
{
	u32 i;
	int err;
	size_t size;
	unsigned long flags;
	phys_addr_t dst_addr;
	struct dma_slave_config slave_config;
	struct device *dev = led->dev;
	struct dma_async_tx_descriptor *dma_desc;

	if (led->length <= SUNXI_LEDC_FIFO_DEPTH) {
		sunxi_info(led->dev, "cpu xfer\n");
		ktime_get_coarse_real_ts64(&(led->start_time));
		sunxi_ledc_set_time(led);
		sunxi_ledc_set_output_mode(led, led->output_mode.str);
		sunxi_ledc_set_cpu_mode(led);
		sunxi_ledc_set_length(led);

		sunxi_ledc_enable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_WAITDATA_TIMEOUT_INT_EN
				| LEDC_FIFO_OVERFLOW_INT_EN | LEDC_GLOBAL_INT_EN, led);

		sunxi_ledc_enable(led);

		for (i = 0; i < led->length; i++)
			writel(led->data[i], led->base_addr + LEDC_DATA_REG_OFFSET);

	} else {
		sunxi_err(led->dev, "dma xfer\n");

		if (!led->dma_chan) {
			sunxi_err(led->dev, "led dma_chain is NULL");
			return;
		}

		size = led->length * 4;
		led->src_dma = dma_map_single(dev, led->data,
					size, DMA_TO_DEVICE);
		if (dma_mapping_error(led->dev, led->src_dma)) {
			dma_unmap_single(led->dev, led->src_dma, size, DMA_TO_DEVICE);
			sunxi_err(led->dev, "dma mapping err\n");
			return;
		}

		dst_addr = led->res->start + LEDC_DATA_REG_OFFSET;
		flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK;

		slave_config.direction = DMA_MEM_TO_DEV;
		slave_config.src_addr = led->src_dma;
		slave_config.dst_addr = dst_addr;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_maxburst = 4;
		slave_config.dst_maxburst = 4;

		err = dmaengine_slave_config(led->dma_chan, &slave_config);
		if (err) {
			sunxi_err(led->dev, "dmaengine_slave_config failed!\n");
			return;
		}

		dma_desc = dmaengine_prep_slave_single(led->dma_chan,
							led->src_dma,
							size,
							DMA_MEM_TO_DEV,
							flags);
		if (!dma_desc) {
			sunxi_err(led->dev, "dmaengine_prep_slave_single failed!\n");
			return;
		}

		dma_desc->callback = sunxi_ledc_dma_callback;

		dma_desc->callback_param = led;

		dmaengine_submit(dma_desc);
		dma_async_issue_pending(led->dma_chan);
		ktime_get_coarse_real_ts64(&(led->start_time));
		sunxi_ledc_set_time(led);
		sunxi_ledc_set_output_mode(led, led->output_mode.str);
		sunxi_ledc_set_dma_mode(led);
		sunxi_ledc_set_length(led);
		sunxi_ledc_enable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_WAITDATA_TIMEOUT_INT_EN
				| LEDC_FIFO_OVERFLOW_INT_EN | LEDC_GLOBAL_INT_EN, led);
		sunxi_ledc_enable(led);
	}
}

static inline void sunxi_ledc_clear_all_irq(struct sunxi_ledc *led)
{
	u32 reg_val = readl(led->base_addr + LEDC_INT_STS_REG_OFFSET);

	reg_val &= ~0x1F;
	writel(reg_val, led->base_addr + LEDC_INT_STS_REG_OFFSET);
}

static inline void sunxi_ledc_clear_irq(enum sunxi_ledc_irq_status_reg irq, struct sunxi_ledc *led)
{
	u32 reg_val = readl(led->base_addr + LEDC_INT_STS_REG_OFFSET);

	reg_val &= ~irq;
	writel(reg_val, led->base_addr + LEDC_INT_STS_REG_OFFSET);
}

static void sunxi_ledc_dma_terminate(struct sunxi_ledc *led)
{
	if (led->dma_chan) {
		dmaengine_terminate_all(led->dma_chan);
		dma_unmap_single(led->dev, led->src_dma, led->length * 4, DMA_TO_DEVICE);
	}
}

static int sunxi_ledc_complete(struct sunxi_ledc *led)
{
	unsigned long flags = 0;
	unsigned long timeout = 0;
	u32 reg_val;

	timeout = wait_event_timeout(led->wait, led->result, 5*HZ);

	/* dynamic close dma transmission */
	sunxi_ledc_dma_terminate(led);

	if (timeout == 0) {
		reg_val = readl(led->base_addr + LEDC_INT_STS_REG_OFFSET);
		sunxi_err(led->dev, "LEDC INTERRUPT STATUS REG IS %x", reg_val);
		sunxi_err(led->dev, "led xfer timeout\n");
		reg_val = readl(led->base_addr + LEDC_INT_STS_REG_OFFSET);
		sunxi_err(led->dev, "LEDC INTERRUPT STATUS REG IS %x", reg_val);
		return -ETIME;
	}

	if (led->result == RESULT_ERR) {
		sunxi_err(led->dev, "led xfer error\n");
		return -ECOMM;
	}

	sunxi_info(led->dev, "xfer complete\n");

	spin_lock_irqsave(&led->lock, flags);
	led->result = 0;
	spin_unlock_irqrestore(&led->lock, flags);

	return 0;
}

static irqreturn_t sunxi_ledc_irq_handler(int irq, void *dev_id)
{
	long delta_time_ns;
	u32 irq_status, max_ns;
	struct sunxi_ledc *led = (struct sunxi_ledc *)dev_id;
	struct timespec64 current_time;

	irq_status = readl(led->base_addr + LEDC_INT_STS_REG_OFFSET);

	sunxi_ledc_clear_all_irq(led);

	if (irq_status & LEDC_TRANS_FINISH_INT) {
		sunxi_ledc_reset(led);
		led->length = 0;
		led->result = RESULT_COMPLETE;
		wake_up(&led->wait);
		goto out;
	}

	if (irq_status & LEDC_WAITDATA_TIMEOUT_INT) {
		ktime_get_coarse_real_ts64(&current_time);
		delta_time_ns = current_time.tv_sec - led->start_time.tv_sec;
		delta_time_ns *= 1000 * 1000 * 1000;
		delta_time_ns += current_time.tv_nsec - led->start_time.tv_nsec;

		max_ns = led->wait_data_time_ns;

		if (delta_time_ns <= max_ns) {
			return IRQ_HANDLED;
		}

		sunxi_ledc_reset(led);

		if (delta_time_ns <= max_ns * 2) {
			sunxi_ledc_dma_terminate(led);
			sunxi_ledc_trans_data(led);
		} else {
			sunxi_err(led->dev, "wait time is more than %d ns, going to reset ledc and drop this operation!\n", max_ns);
			led->result = RESULT_ERR;
			wake_up(&led->wait);
			led->length = 0;
		}

		goto out;
	}

	if (irq_status & LEDC_FIFO_OVERFLOW_INT) {
		sunxi_err(led->dev, "there exists fifo overflow issue, irq_status=0x%x!\n", irq_status);
		sunxi_ledc_reset(led);
		led->result = RESULT_ERR;
		wake_up(&led->wait);
		led->length = 0;
		goto out;
	}

	sunxi_ledc_clear_all_irq(led);

out:
	sunxi_ledc_clear_all_irq(led);
	return IRQ_HANDLED;
}

static int sunxi_ledc_irq_init(struct sunxi_ledc *led)
{
	int err;
	struct device *dev = led->dev;
	unsigned long flags = 0;
	const char *name = "ledcirq";
	struct platform_device *pdev;

	pdev = container_of(dev, struct platform_device, dev);

	spin_lock_init(&led->lock);

	led->irqnum = platform_get_irq(pdev, 0);
	if (led->irqnum < 0)
		sunxi_err(led->dev, "failed to get ledc irq!\n");

	err = devm_request_irq(led->dev, led->irqnum, sunxi_ledc_irq_handler, flags, name, led);
	if (err) {
		sunxi_err(led->dev, "failed to install IRQ handler for irqnum %d\n", led->irqnum);
		return -EPERM;
	}

	return 0;
}

static void sunxi_ledc_irq_deinit(struct sunxi_ledc *led)
{
	devm_free_irq(led->dev, led->irqnum, led);
	sunxi_ledc_disable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_FIFO_CPUREQ_INT_EN
			| LEDC_WAITDATA_TIMEOUT_INT_EN | LEDC_FIFO_OVERFLOW_INT_EN
			| LEDC_GLOBAL_INT_EN, led);
}

static int sunxi_ledc_regulator_request(struct sunxi_ledc *led)
{
	if (led->regulator)
		return 0;

	led->regulator = regulator_get(led->dev, "ledc");
	if (IS_ERR(led->regulator)) {
		sunxi_err(led->dev, "get supply failed");
		return -1;
	}

	return 0;
}

static int sunxi_ledc_regulator_release(struct sunxi_ledc *led)
{
	if (led->regulator == NULL)
		return 0;

	regulator_put(led->regulator);
	led->regulator = NULL;

	return 0;
}

static int sunxi_ledc_dma_get(struct sunxi_ledc *led)
{
	if (led->dma_chan == NULL) {
		led->dma_chan = dma_request_chan(led->dev, "tx");
		if (IS_ERR(led->dma_chan)) {
			sunxi_err(led->dev, "failed to get the DMA channel!\n");
			return -EFAULT;
		}
	}
	return 0;
}

static void sunxi_ledc_dma_put(struct sunxi_ledc *led)
{
	if (led->dma_chan) {
		dma_release_channel(led->dma_chan);
		led->dma_chan = NULL;
	}
}

static int sunxi_ledc_set_led_brightness(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	unsigned long flags;
	u32 r, g, b, shift, old_data, new_data, length;
	struct sunxi_ledc *led;
	struct sunxi_ledc_info *pinfo;
	struct sunxi_ledc_classdev_group *pcdev_group;
	int err;

	pinfo = container_of(led_cdev, struct sunxi_ledc_info, cdev);

	switch (pinfo->type) {
	case LED_TYPE_G:
		pcdev_group = container_of(pinfo, struct sunxi_ledc_classdev_group, g);
		g = value;
		shift = 16;
		break;
	case LED_TYPE_R:
		pcdev_group = container_of(pinfo, struct sunxi_ledc_classdev_group, r);
		r = value;
		shift = 8;
		break;

	case LED_TYPE_B:
		pcdev_group = container_of(pinfo, struct sunxi_ledc_classdev_group, b);
		b = value;
		shift = 0;
		break;
	}

	led = pcdev_group->private;

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
	spin_unlock_irqrestore(&led->lock, flags);

	/* prepare for dma xfer, dynamic apply dma channel */
	if (led->length > SUNXI_LEDC_FIFO_DEPTH) {
		err = sunxi_ledc_dma_get(led);
		if (err)
			return err;
	}

	sunxi_ledc_trans_data(led);
	sunxi_info(led->dev, "dump reg:\n");
	sunxi_ledc_dump_reg(led, 0, 0x30);

	sunxi_ledc_complete(led);

	/* dynamic release dma chan, release at the end of a transmission */
	if (led->length > SUNXI_LEDC_FIFO_DEPTH)
		sunxi_ledc_dma_put(led);

	sunxi_debug(led->dev, "num = %03u\n", length);

	return 0;
}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW22)
static int sunxi_ledc_set_brightness_with_source(struct led_classdev *led_cdev,
			enum led_brightness value, bool from_resume)
{
	struct sunxi_ledc_info *pinfo;
	struct sunxi_ledc_classdev_group *pcdev_group;
	struct sunxi_ledc *led;
	u32 r = 0, g = 0, b = 0, new_data, shift;
	unsigned long flags;

	pinfo = container_of(led_cdev, struct sunxi_ledc_info, cdev);
	pcdev_group = NULL;

	switch (pinfo->type) {
	case LED_TYPE_G:
		pcdev_group = container_of(pinfo, struct sunxi_ledc_classdev_group, g);
		g = value;
		shift = 16;
		break;
	case LED_TYPE_R:
		pcdev_group = container_of(pinfo, struct sunxi_ledc_classdev_group, r);
		r = value;
		shift = 8;
		break;
	case LED_TYPE_B:
		pcdev_group = container_of(pinfo, struct sunxi_ledc_classdev_group, b);
		b = value;
		shift = 0;
		break;
	}

	if (!pcdev_group)
		return -EINVAL;

	led = pcdev_group->private;

	if (!from_resume) {
		if (pinfo->type != LED_TYPE_R)
			r = pcdev_group->r.cdev.brightness;
		if (pinfo->type != LED_TYPE_G)
			g = pcdev_group->g.cdev.brightness;
		if (pinfo->type != LED_TYPE_B)
			b = pcdev_group->b.cdev.brightness;
	}

	new_data = (g << 16) | (r << 8) | b;

	spin_lock_irqsave(&led->lock, flags);

	if (((led->data[pcdev_group->led_num] >> shift) & 0xFF) == value) {
		spin_unlock_irqrestore(&led->lock, flags);
		return 0;
	}

	led->data[pcdev_group->led_num] = (led->data[pcdev_group->led_num] & ~(0xFF << shift)) | (value << shift);
	led->length = pcdev_group->led_num + 1;

	spin_unlock_irqrestore(&led->lock, flags);

	sunxi_ledc_trans_data(led);
	sunxi_ledc_complete(led);

	return 0;
}
#endif

static int sunxi_ledc_register_led_classdev(struct sunxi_ledc *led)
{
	u32 i;
	int err;
	size_t size;
	struct device *dev = led->dev;
	struct led_classdev *pcdev_RGB;

	sunxi_info(led->dev, "led_classdev start\n");
	if (!led->led_count)
		led->led_count = SUNXI_LEDC_DEFAULT_LED_COUNT;

	size = sizeof(*led->pcdev_group) * led->led_count;
	led->pcdev_group = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!led->pcdev_group)
		return -ENOMEM;

	for (i = 0; i < led->led_count; i++) {
		led->pcdev_group[i].r.type = LED_TYPE_R;
		pcdev_RGB = &led->pcdev_group[i].r.cdev;
		pcdev_RGB->name = devm_kzalloc(dev, 16, GFP_KERNEL);
		if (!pcdev_RGB->name)
			return -ENOMEM;
		sprintf((char *)pcdev_RGB->name, "sunxi_led%dr", i);
		pcdev_RGB->brightness = LED_OFF;
		pcdev_RGB->brightness_set_blocking = sunxi_ledc_set_led_brightness;
		pcdev_RGB->dev = dev;
		err = led_classdev_register(dev, pcdev_RGB);
		if (err) {
			sunxi_err(led->dev, "led_classdev_register %s failed!\n",
				pcdev_RGB->name);
			return err;
		}

		led->pcdev_group[i].g.type = LED_TYPE_G;
		pcdev_RGB = &led->pcdev_group[i].g.cdev;
		pcdev_RGB->name = devm_kzalloc(dev, 16, GFP_KERNEL);
		if (!pcdev_RGB->name)
			return -ENOMEM;
		sprintf((char *)pcdev_RGB->name, "sunxi_led%dg", i);
		pcdev_RGB->brightness = LED_OFF;
		pcdev_RGB->brightness_set_blocking = sunxi_ledc_set_led_brightness;
		pcdev_RGB->dev = dev;
		err = led_classdev_register(dev, pcdev_RGB);
		if (err) {
			sunxi_err(led->dev, "led_classdev_register %s failed!\n",
			pcdev_RGB->name);
			return err;
		}

		led->pcdev_group[i].b.type = LED_TYPE_B;
		pcdev_RGB = &led->pcdev_group[i].b.cdev;
		pcdev_RGB->name = devm_kzalloc(dev, 16, GFP_KERNEL);
		if (!pcdev_RGB->name)
			return -ENOMEM;
		sprintf((char *)pcdev_RGB->name, "sunxi_led%db", i);
		pcdev_RGB->brightness = LED_OFF;
		pcdev_RGB->brightness_set_blocking = sunxi_ledc_set_led_brightness;
		pcdev_RGB->dev = dev;
		err = led_classdev_register(dev, pcdev_RGB);
		if (err) {
			sunxi_err(led->dev, "led_classdev_register %s failed!\n", pcdev_RGB->name);
			return err;
		}
		led->pcdev_group[i].led_num = i;
		led->pcdev_group[i].private = led;
	}
	size = sizeof(u32) * led->led_count;
	led->data = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!led->data)
		return -ENOMEM;

	return 0;
}

static void sunxi_ledc_unregister_led_classdev(struct sunxi_ledc *led)
{
	u32 i;

	for (i = 0; i < led->led_count; i++) {
		led_classdev_unregister(&led->pcdev_group[i].b.cdev);
		led_classdev_unregister(&led->pcdev_group[i].g.cdev);
		led_classdev_unregister(&led->pcdev_group[i].r.cdev);
	}
}

static void sunxi_ledc_get_para_from_dts(struct sunxi_ledc *led)
{
	int err;
	const char *str;
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	led->led_count = 32;
	err = of_property_read_u32(np, "led_count", &led->led_count);

	err = of_property_read_string(np, "output_mode", &str);
	if (!err)
		if (!strncmp(str, "BRG", 3) ||
			!strncmp(str, "GBR", 3) ||
			!strncmp(str, "RGB", 3) ||
			!strncmp(str, "RBG", 3) ||
			!strncmp(str, "BGR", 3))
			memcpy(led->output_mode.str, str, 3);
	memcpy(led->output_mode.str, "GRB", 3);
	led->output_mode.val = SUNXI_LEDC_OUTPUT_GRB;

	err = of_property_read_string(np, "led_regulator", &str);
	if (!err) {
		if (strlen(str) >= sizeof(led->regulator_id))
			sunxi_err(led->dev, "illegal regulator id\n");
		else {
			strcpy(led->regulator_id, str);
			sunxi_info(led->dev, "led_regulator: %s\n", led->regulator_id);
		}
	}

	led->reset_ns = SUNXI_LEDC_DEFAULT_RESET_NS;
	err = of_property_read_u32(np, "reset_ns", &led->reset_ns);

	led->t1h_ns = SUNXI_LEDC_DEFAULT_T1H_NS;
	err = of_property_read_u32(np, "t1h_ns", &led->t1h_ns);

	led->t1l_ns = SUNXI_LEDC_DEFAULT_T1L_NS;
	err = of_property_read_u32(np, "t1l_ns", &led->t1l_ns);

	led->t0h_ns = SUNXI_LEDC_DEFAULT_T0H_NS;
	err = of_property_read_u32(np, "t0h_ns", &led->t0h_ns);

	led->t0l_ns = SUNXI_LEDC_DEFAULT_T0L_NS;
	err = of_property_read_u32(np, "t0l_ns", &led->t0l_ns);

	led->wait_time0_ns = SUNXI_LEDC_DEFAULT_WAIT_TIMA0_NS;
	err = of_property_read_u32(np, "wait_time0_ns", &led->wait_time0_ns);

	led->wait_time1_ns = SUNXI_LEDC_DEFAULT_WAIT_TIME1_NS;
	err = of_property_read_u64(np, "wait_time1_ns", &led->wait_time1_ns);

	led->wait_data_time_ns = SUNXI_LEDC_DEFAULT_WAIT_DATA_TIME_NS;
	err = of_property_read_u32(np, "wait_data_time_ns", &led->wait_data_time_ns);
}

static void sunxi_ledc_set_all(struct sunxi_ledc *led, u8 channel, enum led_brightness value)
{
	u32 i;
	struct led_classdev *led_cdev;

	if (channel%3 == 0) {
		for (i = 0; i < led->led_count; i++) {
			led_cdev = &led->pcdev_group[i].r.cdev;
			mutex_lock(&led_cdev->led_access);
			sunxi_ledc_set_led_brightness(led_cdev, value);
			mutex_unlock(&led_cdev->led_access);
		}
	} else if (channel%3 == 1) {
		for (i = 0; i < led->led_count; i++) {
			led_cdev = &led->pcdev_group[i].g.cdev;
			mutex_lock(&led_cdev->led_access);
			sunxi_ledc_set_led_brightness(led_cdev, value);
			mutex_unlock(&led_cdev->led_access);
		}
	} else {
		for (i = 0; i < led->led_count; i++) {
			led_cdev = &led->pcdev_group[i].b.cdev;
			mutex_lock(&led_cdev->led_access);
			sunxi_ledc_set_led_brightness(led_cdev, value);
			mutex_unlock(&led_cdev->led_access);
		}
	}
}

static ssize_t sunxi_ledc_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	sunxi_ledc_set_all(led, 0, 0);
	sunxi_ledc_set_all(led, 1, 0);
	sunxi_ledc_set_all(led, 2, 0);

	sunxi_ledc_set_all(led, 0, 20);
	msleep(500);
	sunxi_ledc_set_all(led, 1, 20);
	msleep(500);
	sunxi_ledc_set_all(led, 2, 20);
	msleep(500);

	sunxi_ledc_set_all(led, 0, 0);
	sunxi_ledc_set_all(led, 1, 0);
	sunxi_ledc_set_all(led, 2, 0);

	return 0;
}

static ssize_t sunxi_ledc_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t len)
{
	u32 i = 0, ret = 0;
	u32 g = 0, r = 0, b = 0;
	struct sunxi_ledc *led = dev_get_drvdata(dev);

	if (len < 3 || len > led->led_count * 3) {
		sunxi_err(led->dev, "unexpected ledc len:%zu\n", len);
		return -EINVAL;
	}

	/* This mutex is used to avoid concurrency problems when multiple user processes call led_store() at the same time. */
	mutex_lock(&led->mutex_lock);

	for (i = 0; i < len/3; i++) {
		r = buf[i * 3];
		g = buf[i * 3 + 1];
		b = buf[i * 3 + 2];
		led->data[i] = (g << 16) | (r << 8) | b;
	}

	led->length = len/3;

	sunxi_ledc_trans_data(led);
	ret = sunxi_ledc_complete(led);
	mutex_unlock(&led->mutex_lock);

	return ret;
};

static struct device_attribute sunxi_led_debug_attr[] = {
	__ATTR(reset_ns, S_IRUGO | S_IWUSR, sunxi_ledc_reset_ns_show, sunxi_ledc_reset_ns_store),
	__ATTR(t1h_ns, S_IRUGO | S_IWUSR, sunxi_ledc_t1h_ns_show, sunxi_ledc_t1h_ns_store),
	__ATTR(t1l_ns, S_IRUGO | S_IWUSR, sunxi_ledc_t1l_ns_show, sunxi_ledc_t1l_ns_store),
	__ATTR(t0h_ns, S_IRUGO | S_IWUSR, sunxi_ledc_t0h_ns_show, sunxi_ledc_t0h_ns_store),
	__ATTR(t0l_ns, S_IRUGO | S_IWUSR, sunxi_ledc_t0l_ns_show, sunxi_ledc_t0l_ns_store),
	__ATTR(wait_time0_ns, S_IRUGO | S_IWUSR, sunxi_ledc_wait_time0_ns_show, sunxi_ledc_wait_time0_ns_store),
	__ATTR(wait_time1_ns, S_IRUGO | S_IWUSR, sunxi_ledc_wait_time1_ns_show, sunxi_ledc_wait_time1_ns_store),
	__ATTR(wait_data_time_ns, S_IRUGO | S_IWUSR, sunxi_ledc_wait_data_time_ns_show, sunxi_ledc_wait_data_time_ns_store),
	__ATTR(output_mode, S_IRUGO | S_IWUSR, sunxi_ledc_output_mode_show, sunxi_ledc_output_mode_store),
	__ATTR(light, S_IRUGO | S_IWUSR, sunxi_ledc_show, sunxi_ledc_store)

};

static void sunxi_ledc_create_sysfs(struct platform_device *pdev)
{
	u32 i;
	for (i = 0; i < ARRAY_SIZE(sunxi_led_debug_attr); i++)
		device_create_file(&pdev->dev, &sunxi_led_debug_attr[i]);
}

static void sunxi_ledc_remove_sysfs(struct platform_device *pdev)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(sunxi_led_debug_attr); i++)
		device_remove_file(&pdev->dev, &sunxi_led_debug_attr[i]);
}

static int sunxi_ledc_resource_get(struct sunxi_ledc *led)
{
	int err;

	led->res = platform_get_resource(led->pdev, IORESOURCE_MEM, 0);
	if (!led->res) {
		sunxi_err(led->dev, "failed to get MEM res\n");
		return -ENXIO;
	}

	led->base_addr = devm_ioremap_resource(led->dev, led->res);
	if (IS_ERR(led->base_addr)) {
		sunxi_err(led->dev, "unable to ioremap\n");
		return PTR_ERR(led->base_addr);
	}

	led->output_mode.str = devm_kzalloc(led->dev, 3, GFP_KERNEL);
	if (!led->output_mode.str) {
		return -ENOMEM;
	}

	err = sunxi_ledc_clk_get(led);
	if (err) {
		sunxi_err(led->dev, "ledc clk get failed!\n");
		return -EINVAL;
	}

	sunxi_ledc_get_para_from_dts(led);

	err = sunxi_ledc_regulator_request(led);
	if (err) {
		sunxi_err(led->dev, "request regulator failed!\n");
		return -EINVAL;
	}

	/* prepare for dma xfer, dynamic apply dma channel */
	if (led->led_count > SUNXI_LEDC_FIFO_DEPTH) {
		err = sunxi_ledc_dma_get(led);
		if (err) {
			sunxi_err(led->dev, "request dma failed!\n");
			return -EINVAL;
		}
	}
	return 0;
}

static void sunxi_ledc_resource_put(struct sunxi_ledc *led)
{
	sunxi_ledc_dma_put(led);
	sunxi_ledc_regulator_release(led);
	sunxi_ledc_clk_put(led);
}

static int sunxi_ledc_hw_init(struct sunxi_ledc *led)
{
	u32 ret;

#if IS_ENABLED(CONFIG_PM)
	ret = sunxi_ledc_power_on(led);
	if (ret) {
		sunxi_err(led->dev, "failed to power on\n");
		return -EINVAL;
	}
#endif

	sunxi_ledc_set_time(led);

	ret = sunxi_ledc_clk_enable(led);
	if (ret) {
		sunxi_err(led->dev, "failed to enable clk\n");
		return -EINVAL;
	}

	ret = sunxi_ledc_irq_init(led);
	if (ret) {
		sunxi_err(led->dev, "failed to init irq\n");
		goto err0;
	}

	led->pctrl = devm_pinctrl_get_select_default(led->dev);
	if (IS_ERR_OR_NULL(led->pctrl)) {
		sunxi_err(led->dev, "devm_pinctrl_get_select_default failed!\n");
		ret = -EINVAL;
		goto err1;
	}

	return 0;

err1:
	sunxi_ledc_irq_deinit(led);
err0:
	sunxi_ledc_clk_disable(led);

	return ret;
}

static void sunxi_ledc_hw_exit(struct sunxi_ledc *led)
{
	sunxi_ledc_irq_deinit(led);
	sunxi_ledc_clk_disable(led);
#if IS_ENABLED(CONFIG_PM)
	sunxi_led_power_off(led);
#endif
}

static int sunxi_ledc_probe(struct platform_device *pdev)
{
	struct sunxi_ledc *led;
	int ret;

	led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
	if (IS_ERR_OR_NULL(led))
		return -ENOMEM;

	led->pdev = pdev;
	led->dev = &pdev->dev;

	ret = sunxi_ledc_resource_get(led);
	if (ret) {
		sunxi_err(led->dev, "LED failed to get resource\n");
		goto err0;
	}

	ret = sunxi_ledc_register_led_classdev(led);
	if (ret) {
		sunxi_err(led->dev, "failed to register led classdev\n");
	}

	ret = sunxi_ledc_hw_init(led);
	if (ret) {
		sunxi_err(led->dev, "sunxi ledc hw_init failed\n");
		goto err1;
	}

	init_waitqueue_head(&led->wait);

	sunxi_ledc_create_sysfs(led->pdev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	led_class = class_create(THIS_MODULE, "led");
#else
	led_class = class_create("led");
#endif
	if (IS_ERR(led_class)) {
		sunxi_err(led->dev, "class_register err\n");
		class_destroy(led_class);
		ret = -EFAULT;
		goto err1;
	}

	platform_set_drvdata(pdev, led);

	mutex_init(&led->mutex_lock);

	sunxi_info(led->dev, "ledc probe success\n");
	return 0;

err1:
	sunxi_ledc_resource_put(led);
err0:
	return ret;
}

static int sunxi_ledc_remove(struct platform_device *pdev)
{
	struct sunxi_ledc *led = platform_get_drvdata(pdev);

	mutex_destroy(&led->mutex_lock);

	class_destroy(led_class);

	sunxi_ledc_remove_sysfs(led->pdev);

	sunxi_ledc_hw_exit(led);

	sunxi_ledc_unregister_led_classdev(led);

	sunxi_ledc_resource_put(led);

	sunxi_info(led->dev, "ledc remove finish\n");

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static inline void sunxi_ledc_save_regs(struct sunxi_ledc *led)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(sunxi_ledc_regs_offset); i++)
		led->regs_backup[i] = readl(led->base_addr + sunxi_ledc_regs_offset[i]);
}

static inline void sunxi_ledc_restore_regs(struct sunxi_ledc *led)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(sunxi_ledc_regs_offset); i++)
		writel(led->regs_backup[i], led->base_addr + sunxi_ledc_regs_offset[i]);
}

static int sunxi_ledc_gpio_state_select(struct sunxi_ledc *led, char *name)
{
	int err;
	struct pinctrl_state *pctrl_state;

	pctrl_state = pinctrl_lookup_state(led->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		sunxi_err(led->dev, "pinctrl_lookup_state(%s) failed! return %p\n",
				name, pctrl_state);
		return PTR_ERR(pctrl_state);
	}

	err = pinctrl_select_state(led->pctrl, pctrl_state);
	if (err) {
		sunxi_err(led->dev, "pinctrl_select_state(%s) failed! return %d\n",
				name, err);
		return err;
	}

	return 0;
}

static int sunxi_ledc_power_on(struct sunxi_ledc *led)
{
	int err;

	if (led->regulator == NULL)
		return 0;

	err = regulator_enable(led->regulator);
	if (err) {
		sunxi_err(led->dev, "enable regulator %s failed!\n", led->regulator_id);
		return err;
	}
	return 0;
}

static int sunxi_led_power_off(struct sunxi_ledc *led)
{
	int err;

	if (led->regulator == NULL)
		return 0;

	err = regulator_disable(led->regulator);
	if (err) {
		sunxi_err(led->dev, "disable regulator %s failed!\n", led->regulator_id);
		return err;
	}
	return 0;
}

static int sunxi_ledc_suspend(struct device *dev)
{
	int err;
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_ledc *led = platform_get_drvdata(pdev);

	dev_dbg(led->dev, "[%s] enter standby\n", __func__);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW22)
	led->saved_r = led->pcdev_group[0].r.cdev.brightness;
	led->saved_g = led->pcdev_group[0].g.cdev.brightness;
	led->saved_b = led->pcdev_group[0].b.cdev.brightness;

	sunxi_ledc_set_brightness_with_source(&led->pcdev_group[0].r.cdev, LED_OFF, false);
	sunxi_ledc_set_brightness_with_source(&led->pcdev_group[0].g.cdev, LED_OFF, false);
	sunxi_ledc_set_brightness_with_source(&led->pcdev_group[0].b.cdev, LED_OFF, false);
#endif
	disable_irq_nosync(led->irqnum);

	sunxi_ledc_save_regs(led);

	err = sunxi_ledc_gpio_state_select(led, PINCTRL_STATE_SLEEP);
	if (err)
		return err;

	sunxi_ledc_clk_disable(led);

	err = sunxi_led_power_off(led);
	if (err)
		return err;

	return 0;
}

static int sunxi_ledc_resume(struct device *dev)
{
	int err;
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_ledc *led = platform_get_drvdata(pdev);

	dev_dbg(led->dev, "[%s] return from standby\n", __func__);

	err = sunxi_ledc_power_on(led);
	if (err)
		return err;

	err = sunxi_ledc_clk_enable(led);
	if (err) {
		sunxi_err(led->dev, "failed to enable clk\n");
		return -EINVAL;
	}

	err = sunxi_ledc_gpio_state_select(led, PINCTRL_STATE_DEFAULT);
	if (err)
		return err;

	sunxi_ledc_restore_regs(led);

	enable_irq(led->irqnum);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW22)
	sunxi_ledc_set_brightness_with_source(&led->pcdev_group[0].r.cdev, led->saved_r, true);
	sunxi_ledc_set_brightness_with_source(&led->pcdev_group[0].g.cdev, led->saved_g, true);
	sunxi_ledc_set_brightness_with_source(&led->pcdev_group[0].b.cdev, led->saved_b, true);
#endif
	return 0;
}

static const struct dev_pm_ops sunxi_led_pm_ops = {
	.suspend = sunxi_ledc_suspend,
	.resume = sunxi_ledc_resume,
};

#define SUNXI_LED_PM_OPS (&sunxi_led_pm_ops)
#endif

static const struct of_device_id sunxi_led_dt_ids[] = {
	{.compatible = "allwinner,sunxi-leds"},
	{.compatible = "allwinner,sunxi-ledc"},
	{},
};

static struct platform_driver sunxi_ledc_driver = {
	.probe		= sunxi_ledc_probe,
	.remove		= sunxi_ledc_remove,
	.driver		= {
		.name	= "sunxi-leds",
		.owner	= THIS_MODULE,
#if IS_ENABLED(CONFIG_PM)
		.pm	= SUNXI_LED_PM_OPS,
#endif
		.of_match_table = sunxi_led_dt_ids,
	},
};

module_platform_driver(sunxi_ledc_driver);

MODULE_ALIAS("sunxi ledc dirver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.3.9");
MODULE_AUTHOR("liuyu <SWCliuyus@allwinnertech.com>");
MODULE_AUTHOR("danghao <danghao@allwinnertech.com>");
MODULE_AUTHOR("zhaiyaya <zhaiyaya@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner ledc-controller driver");
