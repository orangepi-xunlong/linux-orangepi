/*
 * drivers/media/rc/sunxi-ir-rx.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SUNXI_IR_RX_H
#define SUNXI_IR_RX_H
#include <linux/reset.h>

/* Registers */
#define IR_CTRL_REG		(0x00)	/* IR Control */
#define IR_RXCFG_REG		(0x10)	/* Rx Config */
#define IR_RXDAT_REG		(0x20)	/* Rx Data */
#define IR_RXINTE_REG		(0x2C)	/* Rx Interrupt Enable */
#define IR_RXINTS_REG		(0x30)	/* Rx Interrupt Status */
#define IR_SPLCFG_REG		(0x34)	/* IR Sample Config */

/* Config parameters */
#define IR_FIFO_SIZE		(64)	/* 64Bytes */

#define IR_SIMPLE_UNIT		(10700)		/* simple in ns */
#define IR_CLK			(24000000)	/* 24Mhz */
#define IR_SAMPLE_DEV		(0x2<<0)	/* 24MHz/256 =93750Hz (~10.7us) */

/* Active Threshold (1+1)*128clock*10.7us = 2.6ms */
#define IR_ACTIVE_T		((1&0xff)<<16)

/* Filter Threshold = 32*10.7us = 336us < 500us */
#define IR_RXFILT_VAL		(((32)&0x3f)<<2)

/* Filter Threshold = 22*21us = 336us < 500us */
#define IR_RXFILT_VAL_RC5	(((22)&0x3f)<<2)

/* Idle Threshold = (11+1)*128clock*10.7us = 16ms > 9ms */
#define IR_RXIDLE_VAL		(((11)&0xff)<<8)

/* Active Threshold (1+1)*128clock*10.7us = 2.6ms */
#define IR_ACTIVE_T_SAMPLE	((32&0xff)<<16)

#define IR_ACTIVE_T_C		(1<<23)		/* Active Threshold */
#define IR_CIR_MODE		(0x3<<4)	/* CIR mode enable */
#define IR_ENTIRE_ENABLE	(0x3<<0)	/* IR entire enable */
#define IR_FIFO_20		(((20)-1)<<8)
#define IR_IRQ_STATUS		((0x1<<4)|0x3)
#define IR_BOTH_PULSE		(0x1 << 6)
#define IR_LOW_PULSE		(0x2 << 6)
#define IR_HIGH_PULSE		(0x3 << 6)

/*Bit Definition of IR_RXINTS_REG Register*/
#define IR_RXINTS_RXOF		(0x1<<0)	/* Rx FIFO Overflow */
#define IR_RXINTS_RXPE		(0x1<<1)	/* Rx Packet End */
#define IR_RXINTS_RXDA		(0x1<<4)	/* Rx FIFO Data Available */

#define MAX_ADDR_NUM		(32)

#define RC_MAP_SUNXI "rc_map_sunxi"
#define SUNXI_IR_RX_VERSION "v1.0.0"

#define SUNXI_IR_DRIVER_NAME	"sunxi-rc-recv"
/*compatible*/
#ifdef CONFIG_ANDROID
#define SUNXI_IR_DEVICE_NAME	"sunxi-ir"
#else
#define SUNXI_IR_DEVICE_NAME	"sunxi_ir_recv"
#endif

#define RC5_UNIT		889000  /* ns */
#define NEC_UNIT		562500
#define NEC_BOOT_CODE		(16 * NEC_UNIT)
#define NEC			0x0
#define RC5			0x1
#define RC5ANDNEC		0x2

#define DEBUG

#define RTC_REG			0x07000120	/* ATF code store ir value */

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_ERR = 1U << 4,
};

enum ir_mode {
	CIR_MODE_ENABLE,
	IR_MODULE_ENABLE,
	IR_BOTH_PULSE_MODE, /* new feature to avoid noisy */
	IR_LOW_PULSE_MODE,
	IR_HIGH_PULSE_MODE,
};

enum ir_sample_config {
	IR_SAMPLE_REG_CLEAR,
	IR_CLK_SAMPLE,
	IR_FILTER_TH_NEC,
	IR_FILTER_TH_RC5,
	IR_IDLE_TH,
	IR_ACTIVE_TH,
	IR_ACTIVE_TH_SAMPLE,
};

enum ir_irq_config {
	IR_IRQ_STATUS_CLEAR,
	IR_IRQ_ENABLE,
	IR_IRQ_FIFO_SIZE,
};

enum {
	IR_SUPLY_DISABLE = 0,
	IR_SUPLY_ENABLE,
};

struct sunxi_ir_rx_data {
	void __iomem *reg_base;
	struct platform_device	*pdev;
	struct clk *bclk;
	struct clk *pclk;
	struct clk *mclk;
	struct reset_control *reset;
	struct rc_dev *rcdev;
	struct regulator *suply;
	struct pinctrl *pctrl;
	u32 suply_vol;
	int irq_num;
	u32 ir_protocols;
	u32 ir_addr_cnt;
	u32 ir_addr[MAX_ADDR_NUM];
	u32 ir_powerkey[MAX_ADDR_NUM];
	bool wakeup;
};

int init_sunxi_ir_map(void);
void exit_sunxi_ir_map(void);
#ifdef CONFIG_SUNXI_KEYMAPPING_SUPPORT
int init_sunxi_ir_map_ext(void *addr, int num);
#endif
#endif /* SUNXI_IR_RX_H */
