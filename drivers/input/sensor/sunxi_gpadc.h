/*
 * drivers/input/sensor/sunxi_gpadc.h
 *
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC Controller Driver Header
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef SUNXI_GPADC_H
#define SUNXI_GPADC_H

#define GPADC_DEV_NAME		("sunxi-gpadc")

#define OSC_24MHZ		(24000000UL)
#define MAX_SR                  (100000UL)
#define MIN_SR                  (400UL)
#define DEFAULT_SR		(1000UL)
#define VOL_RANGE		(1800000UL) /* voltage range 0~2.3v, unit is uv */

/* GPADC register offset */
#define GP_SR_REG		(0x00) /* GPADC Sample Rate config register */
#define GP_CTRL_REG		(0x04) /* GPADC control register */
#define GP_CS_EN_REG		(0x08) /* GPADC compare and select enable register */
#define GP_FIFO_INTC_REG	(0x0c) /* GPADC FIFO interrupt config register */
#define GP_FIFO_INTS_REG	(0x10) /* GPADC FIFO interrupt status register */
#define GP_FIFO_DATA_REG	(0X14) /* GPADC FIFO data register */
#define GP_CB_DATA_REG		(0X18) /* GPADC calibration data register */
#define GP_DATAL_INTC_REG	(0x20)
#define GP_DATAH_INTC_REG	(0x24)
#define GP_DATA_INTC_REG	(0x28)
#define GP_DATAL_INTS_REG	(0x30)
#define GP_DATAH_INTS_REG	(0x34)
#define GP_DATA_INTS_REG	(0x38)
#define GP_CH0_CMP_DATA_REG	(0x40) /* GPADC channal 0 compare data register */
#define GP_CH1_CMP_DATA_REG	(0x44) /* GPADC channal 1 compare data register */
#define GP_CH2_CMP_DATA_REG	(0x48) /* GPADC channal 2 compare data register */
#define GP_CH3_CMP_DATA_REG 	(0x4c) /* GPADC channal 3 compare data register */
#define GP_CH0_DATA_REG		(0x80) /* GPADC channal 0 data register */
#define GP_CH1_DATA_REG		(0x84) /* GPADC channal 1 data register */
#define GP_CH2_DATA_REG		(0x88) /* GPADC channal 2 data register */
#define GP_CH3_DATA_REG		(0x8c) /* GPADC channal 3 data register */


/* GP_SR_REG default value: 0x01df_002f 50KHZ */
#define GP_SR_CON		(0xffff << 16) /* sample_rate = clk_in/(n+1) = 24MHZ/(0x1df + 1) = 50KHZ */

/* GP_CTRL_REG default value:0x0000_0000 */
#define GP_FIRST_CONCERT_DLY	(0xff<<24) /* the delay time of the first time */
#define GP_CALI_EN		(1 << 17) /* enable calibration */
#define GP_ADC_EN		(1 << 16) /* GPADC function enable, 0: disable, 1: enable */
#define GP_MODE_SELECT		(3 << 8) /*00:single conversion mode, 01:single-cycle conversion mode, 10:continuous mode, 11:burst mode*/
#define GP_CH3_CMP_EN		(1 << 19) /* channale 3 compare enable, 0:disable, 1:enable */
#define GP_CH2_CMP_EN		(1 << 18) /* channale 2 compare enable, 0:disable, 1:enable */
#define GP_CH1_CMP_EN		(1 << 17) /* channale 1 compare enable, 0:disable, 1:enable */
#define GP_CH0_CMP_EN		(1 << 16) /* channale 0 compare enable, 0:disable, 1:enable */
#define GP_CH3_SELECT		(1 << 3) /* channale 3 select enable,  0:disable, 1:enable */
#define GP_CH2_SELECT		(1 << 2) /* channale 2 select enable,  0:disable, 1:enable */
#define GP_CH1_SELECT		(1 << 1) /* channale 1 select enable,  0:disable, 1:enable */
#define GP_CH0_SELECT		(1 << 0) /* channale 0 select enable,  0:disable, 1:enable */

/* GP_FIFO_INTC_REG default value: 0x0000_0f00 */
#define FIFO_OVER_IRQ_EN	(1 << 17) /* fifo over run irq enable, 0:disable, 1:enable */
#define FIFO_DATA_IRQ_EN	(1 << 16) /* fifo data irq enable, 0:disable, 1:enable */
#define FIFO_FLUSH		(1 << 4) /* write 1 to flush TX FIFO, self clear to 0 */

/* GP_FIFO_INTS_REG default value: 0x0000_0000 */
#define FIFO_OVER_PEND		(1 << 17) /* fifo over pending flag, 0:no pending irq, 1: over pending, need write 1 to clear flag */
#define FIFO_DATA_PEND		(1 << 16) /* fifo data pending flag, 0:no pending irq, 1:data pending, need write 1 to clear flag */
#define FIFO_CNT		(0x3f << 8) /* the data count in fifo */

/* GP_FIFO_DATA_REG default value: 0x0000_0000 */
#define GP_FIFO_DATA		(0xfff << 0) /* GPADC data in fifo */

/* GP_CB_DATA_REG default value: 0x0000_0000 */
#define GP_CB_DATA		(0xfff << 0) /* GPADC calibration data */

/* GP_INTC_REG default value: 0x0000_0000 */
#define GP_CH3_LOW_IRQ_EN	(1 << 11) /* 0:disable, 1:enable */
#define GP_CH2_LOW_IRQ_EN	(1 << 10)
#define GP_CH1_LOW_IRQ_EN	(1 << 9)
#define GP_CH0_LOW_IRQ_EN	(1 << 8)
#define GP_CH3_HIG_IRQ_EN	(1 << 7)
#define GP_CH2_HIG_IRQ_EN	(1 << 6)
#define GP_CH1_HIG_IRQ_EN	(1 << 5)
#define GP_CH0_HIG_IRQ_EN	(1 << 4)
#define GP_CH3_DATA_IRQ_EN	(1 << 3)
#define GP_CH2_DATA_IRQ_EN	(1 << 2)
#define GP_CH1_DATA_IRQ_EN	(1 << 1)
#define GP_CH0_DATA_IRQ_EN	(1 << 0)

/* GP_INTS_REG default value: 0x0000_0000 */
#define GP_CH3_LOW		(1 << 3) /* 0:no pending, 1:pending */
#define GP_CH2_LOW		(1 << 2)
#define GP_CH1_LOW		(1 << 1)
#define GP_CH0_LOW		(1 << 0)
#define GP_CH3_HIG		(1 << 3)
#define GP_CH2_HIG		(1 << 2)
#define GP_CH1_HIG		(1 << 1)
#define GP_CH0_HIG		(1 << 0)
#define GP_CH3_DATA		(1 << 3)
#define GP_CH2_DATA		(1 << 2)
#define GP_CH1_DATA		(1 << 1)
#define GP_CH0_DATA		(1 << 0)

/* GP_CH0_CMP_DATA_REG default value 0x0bff_0400 */
#define GP_CH0_CMP_HIG_DATA		(0xfff << 16) /* channel 0 hight voltage data */
#define GP_CH0_CMP_LOW_DATA		(0xfff << 0) /* channel 0 low voltage data */
/* GP_CH1_CMP_DATA_REG default value 0x0bff_0400 */
#define GP_CH1_CMP_HIG_DATA		(0xfff << 16) /* channel 1 hight voltage data */
#define GP_CH1_CMP_LOW_DATA		(0xfff << 0) /* channel 1 low voltage data */
/* GP_CH2_CMP_DATA_REG default value 0x0bff_0400 */
#define GP_CH2_CMP_HIG_DATA		(0xfff << 16) /* channel 2 hight voltage data */
#define GP_CH2_CMP_LOW_DATA		(0xfff << 0) /* channel 2 low voltage data */
/* GP_CH3_CMP_DATA_REG default value 0x0bff_0400 */
#define GP_CH3_CMP_HIG_DATA		(0xfff << 16) /* channel 3 hight voltage data */
#define GP_CH3_CMP_LOW_DATA		(0xfff << 0) /* channel 3 low voltage data */

/* GP_CH0_DATA_REG default value:0x0000_0000 */
#define GP_CH0_DATA_MASK		(0xfff << 0) /* channel 0 data mask */
/* GP_CH1_DATA_REG default value:0x0000_0000 */
#define GP_CH1_DATA_MASK		(0xfff << 0) /* channel 1 data mask */
/* GP_CH2_DATA_REG default value:0x0000_0000 */
#define GP_CH2_DATA_MASK		(0xfff << 0) /* channel 2 data mask */
/* GP_CH3_DATA_REG default value:0x0000_0000 */
#define GP_CH3_DATA_MASK		(0xfff << 0) /* channel 3 data mask */
#define GP_CALIBRATION_ENABLE		(0x1 << 17)
#define CHANNEL_0_SELECT		0x01
#define CHANNEL_1_SELECT		0x02
#define CHANNEL_2_SELECT		0x04
#define CHANNEL_3_SELECT		0x08

#define CHANNEL_MAX_NUM			8
#define KEY_MAX_CNT			(13)
#define VOL_NUM				KEY_MAX_CNT
#define MAXIMUM_INPUT_VOLTAGE		1800
#define DEVIATION			100
#define SUNXIKEY_DOWN			(MAXIMUM_INPUT_VOLTAGE-DEVIATION)
#define SUNXIKEY_UP			SUNXIKEY_DOWN
#define MAXIMUM_SCALE			128
#define SCALE_UNIT			(MAXIMUM_INPUT_VOLTAGE/MAXIMUM_SCALE)
#define SAMPLING_FREQUENCY		10

enum {
	DEBUG_INFO = 1U << 0,
	DEBUG_RUN  = 1U << 1,
};

enum gp_select_mode {
	GP_SINGLE_MODE = 0,
	GP_SINGLE_CYCLE_MODE,
	GP_CONTINUOUS_MODE,
	GP_BURST_MODE,
};

enum gp_channel_id {
	GP_CH_0 = 0,
	GP_CH_1,
	GP_CH_2,
	GP_CH_3,
};

struct sunxi_config {
	u32 channel_select;
	u32 channel_data_select;
	u32 channel_compare_select;
	u32 channel_cld_select;
	u32 channel_chd_select;
	u32 channel_compare_lowdata[CHANNEL_MAX_NUM];
	u32 channel_compare_higdata[CHANNEL_MAX_NUM];
};

struct sunxi_gpadc {
	struct platform_device	*pdev;
	struct input_dev *input_gpadc[CHANNEL_MAX_NUM];
	struct sunxi_config gpadc_config;
	struct clk *mclk;
	struct clk *pclk;
	void __iomem *reg_base;
	int irq_num;
	u32 channel_num;
	u32 scankeycodes[KEY_MAX_CNT];
	u32 gpadc_sample_rate;
	char key_name[16];
	u32 key_num;
	u8 key_cnt;
	u8 compare_before;
	u8 compare_later;
	u8 key_code;
	u32 key_val;
};

struct status_reg {
	char *pst;
	char *ped;
	unsigned char channel;
	unsigned char val;
};

struct vol_reg {
	char *pst;
	char *ped;
	unsigned char index;
	unsigned long vol;
};

struct sr_reg {
	char *pst;
	unsigned long val;
};

struct filter_reg {
	char *pst;
	unsigned long val;
};

#endif
