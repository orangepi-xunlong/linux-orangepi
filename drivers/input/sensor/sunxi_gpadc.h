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
#define VOL_RANGE		(2300000UL) /* voltage range 0~2.3v, unit is uv */

/* GPADC register offset */
#define GP_SR_REG		(0x00) /* GPADC Sample Rate config register */
#define GP_CTRL_REG		(0x04) /* GPADC control register */
#define GP_FIFO_INTC_REG	(0x08) /* GPADC FIFO interrupt config register */
#define GP_FIFO_INTS_REG	(0x0c) /* GPADC FIFO interrupt status register */
#define GP_FIFO_DATA_REG	(0X10) /* GPADC FIFO data register */
#define GP_CB_DATA_REG		(0X14) /* GPADC calibration data register */
#define GP_INTC_REG		(0x30) /* GPADC interrupt config register */
#define GP_INTS_REG		(0x34) /* GPADC interrupt status register */
#define GP_CH0_CMP_DATA_REG 	(0x40) /* GPADC channal 0 compare data register */
#define GP_CH1_CMP_DATA_REG 	(0x44) /* GPADC channal 1 compare data register */
#define GP_CH2_CMP_DATA_REG 	(0x48) /* GPADC channal 2 compare data register */
#define GP_CH3_CMP_DATA_REG 	(0x4c) /* GPADC channal 3 compare data register */
#define GP_CH0_DATA_REG 	(0x50) /* GPADC channal 0 data register */
#define GP_CH1_DATA_REG 	(0x54) /* GPADC channal 1 data register */
#define GP_CH2_DATA_REG 	(0x58) /* GPADC channal 2 data register */
#define GP_CH3_DATA_REG 	(0x5c) /* GPADC channal 3 data register */


/* GP_SR_REG default value: 0x01df_002f 50KHZ */
#define GP_SR_CON		(0xffff << 16) /* sample_rate = clk_in/(n+1) = 24MHZ/(0x1df + 1) = 50KHZ */

/* GP_CTRL_REG default value:0x0000_0000 */
#define GP_FIRST_CONCERT_DLY	(0xff<<24) /* the delay time of the first time */
#define GP_CALI_EN		(1 << 17) /* enable calibration */
#define GP_ADC_EN		(1 << 16) /* GPADC function enable, 0: disable, 1: enable */
#define GP_MODE_SELECT		(3 << 8) /*00:single conversion mode, 01:single-cycle conversion mode, 10:continuous mode, 11:burst mode*/
#define GP_CH3_CMP_EN		(1 << 7) /* channale 3 compare enable, 0:disable, 1:enable */
#define GP_CH2_CMP_EN		(1 << 6) /* channale 2 compare enable, 0:disable, 1:enable */
#define GP_CH1_CMP_EN		(1 << 5) /* channale 1 compare enable, 0:disable, 1:enable */
#define GP_CH0_CMP_EN		(1 << 4) /* channale 0 compare enable, 0:disable, 1:enable */
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
#define GP_CH3_LOW_IRQ_PEND		(1 << 11) /* 0:no pending, 1:pending */
#define GP_CH2_LOW_IRQ_PEND		(1 << 10)
#define GP_CH1_LOW_IRQ_PEND		(1 << 9)
#define GP_CH0_LOW_IRQ_PEND		(1 << 8)
#define GP_CH3_HIG_IRQ_PEND		(1 << 7)
#define GP_CH2_HIG_IRQ_PEND		(1 << 6)
#define GP_CH1_HIG_IRQ_PEND		(1 << 5)
#define GP_CH0_HIG_IRQ_PEND		(1 << 4)
#define GP_CH3_DATA_IRQ_PEND		(1 << 3)
#define GP_CH2_DATA_IRQ_PEND		(1 << 2)
#define GP_CH1_DATA_IRQ_PEND		(1 << 1)
#define GP_CH0_DATA_IRQ_PEND		(1 << 0)

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



enum gp_select_mode {
	GP_SINGLE_MODE = 0,
	GP_SINGLE_CYCLE_MODE,
	GP_CONTINUOUS_MODE,
	GP_BURST_MODE,
	GP_NUM_MAX,
};

enum gp_channel_id {
	GP_CH_0 = 0,
	GP_CH_1,
	GP_CH_2,
	GP_CH_3,
	GP_CH_MAX,
};

struct sunxi_gpadc {
	struct platform_device	*pdev;
	struct clk *mclk;
	struct clk *pclk;
	void __iomem *reg_base;
	int	irq_num;
	spinlock_t gpadc_lock;
	int gpadc_en_cnt; /* couter of enable work */
	int ch_open_cnt[GP_NUM_MAX]; /* couter of channel opened */
	int ch_cmp_cnt[GP_NUM_MAX]; /* counter of channel compare */
};

#endif
