/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2013-2020 Allwinnertech Co., Ltd.
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
/* #define DEBUG */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <linux/reset.h>
#include <media/rc-core.h>
#include <asm/io.h>
#include <linux/regulator/consumer.h>
#include "sunxi-ir-tx.h"

static u32 sunxi_irtx_regs_offset[] = {
	IR_TX_MCR,
	IR_TX_CR,
	IR_TX_IDC_H,
	IR_TX_IDC_L,
	IR_TX_STAR,
	IR_TX_INTC,
	IR_TX_GLR,
};

struct ir_raw_buffer {
	unsigned int tx_dcnt;
	unsigned char tx_buf[IR_TX_RAW_BUF_SIZE];
};

struct sunxi_irtx {
	void __iomem *reg_base;
	struct platform_device	*pdev;
	struct rc_dev *rcdev;
	struct clk *bclk;
	struct clk *pclk;
	struct clk *mclk;
	struct reset_control *reset;
	struct regulator *supply;
	struct pinctrl *pctrl;
	unsigned int supply_vol;
	int irq_num;
	struct ir_raw_buffer ir_rawbuf;
	struct regulator *regulator;
	u32 regs_backup[ARRAY_SIZE(sunxi_irtx_regs_offset)];
};

static int sunxi_irtx_regulator_request(struct sunxi_irtx *chip)
{
	if (chip->regulator)
		return 0;

	chip->regulator = regulator_get(&chip->pdev->dev, "irtx");
	if (IS_ERR(chip->regulator)) {
		dev_err(&chip->pdev->dev, "get supply failed!\n");
		return -EPROBE_DEFER;
	}

	return 0;
}

static int sunxi_irtx_regulator_release(struct sunxi_irtx *chip)
{
	if (!chip->regulator)
		return 0;

	regulator_put(chip->regulator);

	chip->regulator = NULL;

	return 0;
}

static int sunxi_irtx_regulator_enable(struct sunxi_irtx *chip)
{
	if (!chip->regulator)
		return 0;

	if (regulator_enable(chip->regulator)) {
		dev_err(&chip->pdev->dev, "enable regulator failed!\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_irtx_regulator_disable(struct sunxi_irtx *chip)
{
	if (!chip->regulator)
		return 0;

	if (regulator_is_enabled(chip->regulator))
		regulator_disable(chip->regulator);

	return 0;
}

static inline void sunxi_irtx_save_regs(struct sunxi_irtx *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_irtx_regs_offset); i++)
		chip->regs_backup[i] = readl(chip->reg_base + sunxi_irtx_regs_offset[i]);
}

static inline void sunxi_irtx_restore_regs(struct sunxi_irtx *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_irtx_regs_offset); i++)
		writel(chip->regs_backup[i], chip->reg_base + sunxi_irtx_regs_offset[i]);
}

/*
 * one pulse cycle(=10.666666us) is not an integer, so in order to
 * improve the accuracy, define the value of three pulse cycle here.
 */
static unsigned int three_pulse_cycle = 32;

static inline bool irtx_fifo_empty(struct sunxi_irtx *chip)
{
	unsigned int reg_val;
	struct device *dev = &chip->pdev->dev;

	reg_val = readl(chip->reg_base + IR_TX_TACR);
	dev_dbg(dev, "%3u bytes fifo available\n", reg_val);

	return (reg_val == IR_TX_FIFO_SIZE);
}

static inline bool irtx_fifo_full(struct sunxi_irtx *chip)
{
	unsigned int reg_val;
	struct device *dev = &chip->pdev->dev;

	reg_val = readl(chip->reg_base + IR_TX_TACR);
	dev_dbg(dev, "%3u bytes fifo available\n", reg_val);

	return (reg_val == 0);
}

static inline void sunxi_irtx_reset_rawbuffer(struct sunxi_irtx *chip)
{
	int i;

	chip->ir_rawbuf.tx_dcnt = 0;
	for (i = 0; i < IR_TX_RAW_BUF_SIZE; i++)
		chip->ir_rawbuf.tx_buf[i] = 0;
}

/*
 * FIXME: The NEC coding process is wrong and needs to be fixed
 * This function implements encoding flow of NEC protocol,
 * just used to test sunxi ir-tx's basic function.
void irtx_packet_handler(unsigned char address, unsigned char command)
{
	unsigned int  i, j;
	unsigned int  count = 0;
	unsigned char buffer[256];
	unsigned char tx_code[4];

	sunxi_irtx_reset_rawbuffer();

	tx_code[0] = address;
	tx_code[1] = ~address;
	tx_code[2] = command;
	tx_code[3] = ~command;

	dev_dbg(chip->pdev->dev, "addr: 0x%x  addr': 0x%x  cmd: 0x%x  cmd': 0x%x\n",
			tx_code[0], tx_code[1], tx_code[2], tx_code[3]);

	if (IR_TX_CLK_Ts == 1) {
		buffer[count++] = 0xff;
		buffer[count++] = 0xff;
		buffer[count++] = 0xff;
		buffer[count++] = 0xa5;
		buffer[count++] = 0x7f;
		buffer[count++] = 0x54;
		for (j = 0; j < 4; j++) {
			for (i = 0; i < 8; i++) {
				if (tx_code[j] & 0x01) {
					buffer[count++] = 0x99;
					buffer[count++] = 0x4d;
				} else {
					buffer[count++] = 0x99;
					buffer[count++] = 0x19;
				}
				tx_code[j] = tx_code[j] >> 1;
			}
		}
		buffer[count++] = 0x99;
		buffer[count++] = 0x7f;
		buffer[count++] = 0x7f;
		buffer[count++] = 0x7f;
		buffer[count++] = 0x7f;
		buffer[count++] = 0x7f;
		buffer[count++] = 0x7f;
	} else {
		for (j = 0; j < 4; j++) {
			if (IR_TX_CYCLE_TYPE == 0 && j % 4 == 0) {
				buffer[count++] = 0xff;
				buffer[count++] = 0xff;
				buffer[count++] = 0xff;
				buffer[count++] = 0xab;
				buffer[count++] = 0x7f;
				buffer[count++] = 0x55;
			}
			for (i = 0; i < 8; i++) {
				if (tx_code[j] & 0x01) {
					buffer[count++] = 0x9a;
					buffer[count++] = 0x4e;
				} else {
					buffer[count++] = 0x9a;
					buffer[count++] = 0x1a;
				}
				tx_code[j] = tx_code[j] >> 1;
			}
		}
		if (IR_TX_CYCLE_TYPE == 0)
			buffer[count++] = 0x9a;
	}

	for (i = 0; i < count; i++)
		chip->ir_rawbuf.tx_buf[chip->ir_rawbuf.tx_dcnt++] = buffer[i];

	dev_dbg(chip->pdev->dev, "tx_dcnt = %d\n", chip->ir_rawbuf.tx_dcnt);
}
*/

static int sunxi_send_ir_code(struct sunxi_irtx *chip)
{
	unsigned int i, idle_threshold;
	unsigned int reg_val;
	struct device *dev = &chip->pdev->dev;

	dev_dbg(dev, "enter\n");

	/* reset transmit and flush fifo */
	reg_val = readl(chip->reg_base + IR_TX_GLR);
	reg_val |= BIT(1);
	writel(reg_val, chip->reg_base + IR_TX_GLR);

	/* get idle threshold */
	idle_threshold = (readl(chip->reg_base + IR_TX_IDC_H) << 8)
		| readl(chip->reg_base + IR_TX_IDC_L);
	dev_dbg(dev, "idle_threshold = %d\n", idle_threshold);

	/* set transmit threshold */
	writel((chip->ir_rawbuf.tx_dcnt - 1), chip->reg_base + IR_TX_TR);

	if (chip->ir_rawbuf.tx_dcnt > IR_TX_FIFO_SIZE) {
		dev_err(dev, "invalid packet\n");
		return -1;
	}
	for (i = 0; i < chip->ir_rawbuf.tx_dcnt; i++) {
		writeb(chip->ir_rawbuf.tx_buf[i],
				chip->reg_base + IR_TX_FIFO_DR);
	}

	reg_val = readl(chip->reg_base + IR_TX_TACR);
	dev_dbg(dev, "%3u bytes fifo available\n", reg_val);

	if (IR_TX_CYCLE_TYPE) {
		for (i = 0; i < chip->ir_rawbuf.tx_dcnt; i++)
			dev_dbg(dev, "%d, ir txbuffer code = 0x%x!\n",
					i, chip->ir_rawbuf.tx_buf[i]);
		reg_val = readl(chip->reg_base + IR_TX_CR);
		reg_val |= (0x01 << 7);
		writel(reg_val, chip->reg_base + IR_TX_CR);
	} else {
		while (!irtx_fifo_empty(chip)) {
			reg_val = readl(chip->reg_base + IR_TX_TACR);
			dev_dbg(dev, "fifo under run. %3u bytes fifo available\n",
				reg_val);
		}
	}

	/* wait idle finish */
	while ((readl(chip->reg_base + IR_TX_ICR_H) << 8
				| readl(chip->reg_base + IR_TX_ICR_L))
				< idle_threshold)
		dev_dbg(dev, "wait idle\n");

	dev_dbg(dev, "finish\n");

	return 0;
}

static irqreturn_t sunxi_irtx_isr(int irqno, void *dev_id)
{
	struct sunxi_irtx *chip = (struct sunxi_irtx *)dev_id;
	unsigned int intsta;
	struct device *dev = &chip->pdev->dev;

	/* Clear the interrupt */
	intsta = readl(chip->reg_base + IR_TX_STAR);
	dev_dbg(dev, "IR TX IRQ Serve %#x\n", intsta);

	intsta |= intsta & 0xff;
	writel(intsta, chip->reg_base + IR_TX_STAR);

	return IRQ_HANDLED;
}

static void sunxi_irtx_reg_clear(struct sunxi_irtx *chip)
{
	writel(0, chip->reg_base + IR_TX_GLR);
}

static void sunxi_irtx_reg_cfg(struct sunxi_irtx *chip)
{
	struct device *dev = &chip->pdev->dev;

	/* set reference frequency of modulated carrier */
	writel(IR_TX_MC_VALUE, chip->reg_base + IR_TX_MCR);

	/* reference clock select for cir transmit */
	writel(IR_TX_CLK_VALUE, chip->reg_base + IR_TX_CR);

	/* set idle duration counter threshold(high 4 bit) */
	writel(IR_TX_IDC_H_VALUE, chip->reg_base + IR_TX_IDC_H);

	/* set idle duration counter threshold(low 8 bit) */
	writel(IR_TX_IDC_L_VALUE, chip->reg_base + IR_TX_IDC_L);

	/* clear TX FIFO available interrupt flag */
	writel(IR_TX_STA_VALUE, chip->reg_base + IR_TX_STAR);

	/* set transmit packet end interrupt for cyclical pulse enbale
	 * and transmitter fifo under run interrupt enable for Non-cyclical
	 * pulse enable
	 * */
	writel(IR_TX_INT_C_VALUE, chip->reg_base + IR_TX_INTC);

	/* Enable cir transmitter
	 * Reset the cir transmit
	 * set low level is the two times of high level
	 * set the transmitting signal is modulated internally
	 * */
	writel(IR_TX_GL_VALUE, chip->reg_base + IR_TX_GLR);

	dev_dbg(dev, "Offset: 0x%2x IR_TX_GLR   = 0x%2x\n", IR_TX_GLR,
			readl(chip->reg_base + IR_TX_GLR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_MCR   = 0x%2x\n", IR_TX_MCR,
			readl(chip->reg_base + IR_TX_MCR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_CR    = 0x%2x\n", IR_TX_CR,
			readl(chip->reg_base + IR_TX_CR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_IDC_H = 0x%2x\n", IR_TX_IDC_H,
			readl(chip->reg_base + IR_TX_IDC_H));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_IDC_L = 0x%2x\n", IR_TX_IDC_L,
			readl(chip->reg_base + IR_TX_IDC_L));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_ICR_H = 0x%2x\n", IR_TX_ICR_H,
			readl(chip->reg_base + IR_TX_ICR_H));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_ICR_L = 0x%2x\n", IR_TX_ICR_L,
			readl(chip->reg_base + IR_TX_ICR_L));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_TELR  = 0x%2x\n", IR_TX_TELR,
			readl(chip->reg_base + IR_TX_TELR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_INTC  = 0x%2x\n", IR_TX_INTC,
			readl(chip->reg_base + IR_TX_INTC));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_TACR  = 0x%2x\n", IR_TX_TACR,
			readl(chip->reg_base + IR_TX_TACR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_STAR  = 0x%2x\n", IR_TX_STAR,
			readl(chip->reg_base + IR_TX_STAR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_TR    = 0x%2x\n", IR_TX_TR,
			readl(chip->reg_base + IR_TX_TR));
	dev_dbg(dev, "Offset: 0x%2x IR_TX_DMAC  = 0x%2x\n", IR_TX_DMAC,
			readl(chip->reg_base + IR_TX_DMAC));

}

static void sunxi_irtx_clk_uncfg(struct sunxi_irtx *chip)
{
	clk_disable_unprepare(chip->mclk);

	clk_disable_unprepare(chip->bclk);

	reset_control_assert(chip->reset);
}

static int irtx_clk_cfg(struct sunxi_irtx *chip)
{
	unsigned long rate = 0;
	int ret = 0;
	struct device *dev = &chip->pdev->dev;

	ret = reset_control_deassert(chip->reset);
	if (ret) {
		dev_err(dev, "deassert ir tx rst failed!\n");
		return ret;
	}

	rate = clk_get_rate(chip->bclk);
	dev_dbg(dev, "get ir bus clk rate %dHZ\n", (__u32)rate);

	rate = clk_get_rate(chip->pclk);
	dev_dbg(dev, "get ir parent clk rate %dHZ\n", (__u32)rate);

	ret = clk_set_parent(chip->mclk, chip->pclk);
	if (ret) {
		dev_err(dev, "set ir_clk parent failed!\n");
		goto assert_reset;
	}

	ret = clk_set_rate(chip->mclk, IR_TX_CLK);
	if (ret) {
		dev_err(dev, "set ir clock freq to %d failed!\n", IR_TX_CLK);
		goto assert_reset;
	}
	dev_dbg(dev, "set ir_clk rate %dHZ\n", IR_TX_CLK);

	rate = clk_get_rate(chip->mclk);
	dev_dbg(dev, "get ir_clk rate %dHZ\n", (__u32)rate);

	ret = clk_prepare_enable(chip->bclk);
	if (ret) {
		dev_err(dev, "try to enable bus clk failed!\n");
		goto assert_reset;
	}

	ret = clk_prepare_enable(chip->mclk);
	if (ret) {
		dev_err(dev, "try to enable ir_clk failed!\n");
		goto clk_unprepare;
	}

	return 0;

clk_unprepare:
	clk_disable_unprepare(chip->bclk);

assert_reset:
	reset_control_assert(chip->reset);

	return ret;
}

static int sunxi_irtx_select_pinctrl_state(char *name, struct sunxi_irtx *chip)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;
	struct device *dev = &chip->pdev->dev;

	pctrl_state = pinctrl_lookup_state(chip->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		dev_err(dev, "IR_TX pinctrl_lookup_state(%s) failed! return %p \n",
				name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(chip->pctrl, pctrl_state);
	if (ret) {
		dev_err(dev, "IR_TX pinctrl_select_state(%s) failed! return %d \n",
				name, ret);
		return ret;
	}

	return 0;
}

static int sunxi_irtx_hw_init(struct sunxi_irtx *chip)
{
	int ret = 0;
	struct device *dev = &chip->pdev->dev;

	ret = sunxi_irtx_select_pinctrl_state(PINCTRL_STATE_DEFAULT, chip);
	if (ret) {
		dev_err(dev, "request gpio failed!\n");
		return ret;
	}
	ret = sunxi_irtx_regulator_enable(chip);
	if (ret) {
		dev_err(dev, "enable regulator failed!\n");
		return ret;
	}

	ret = irtx_clk_cfg(chip);
	if (ret) {
		sunxi_irtx_regulator_disable(chip);
		dev_err(dev, "ir tx clk configure failed!\n");
		return ret;
	}

	sunxi_irtx_reg_cfg(chip);

	return 0;
}

static void sunxi_irtx_hw_exit(struct sunxi_irtx *chip)
{
	sunxi_irtx_reg_clear(chip);
	sunxi_irtx_clk_uncfg(chip);
	sunxi_irtx_regulator_disable(chip);
	sunxi_irtx_select_pinctrl_state(PINCTRL_STATE_SLEEP, chip);
}

/*
 * The relation of carrier frequency and IR TX CLK as follows:
 *    RFMC = FCLK / ((N + 1) * (DRMC + 2))
 *
 * @RFMC: reference frequency of modulated carrier(=carrier_freq).
 * @FCLK: IR TX CLK(=12000000).
 * @N: the low 8 bit value of IR_TX_MCR.
 * @DRMC: duty cycle of modulated carrier, its value can be 0,1 or 2
 *        which is decided by the bit6 and bit5 of TX MCR.
 */
static int sunxi_irtx_set_carrier(struct rc_dev *rcdev, u32 carrier_freq)
{
	unsigned int reg_val;
	unsigned int drmc = 1;
	struct sunxi_irtx *chip = rcdev->priv;
	struct device *dev = &chip->pdev->dev;

	if ((carrier_freq > 6000000) || (carrier_freq < 15000)) {
		dev_err(dev, "invalid frequency of carrier: %d\n", carrier_freq);
		return -EINVAL;
	}

	/* First, get the duty cycle of modulated carrier */
	reg_val = readl(chip->reg_base + IR_TX_GLR);
	drmc = (reg_val >> 5) & 0x3;
	dev_dbg(dev, "DRMC is %d\n", drmc);
	dev_dbg(dev, "0: duty cycle 50%%\n"
						"1: duty cycle 33%%\n"
						"2: duty cycle 25%%\n");

	/* Then, calculate the value of N */
	reg_val = IR_TX_CLK / ((2 + drmc) * carrier_freq) - 1;
	reg_val &= 0xff;
	dev_dbg(dev,  "RFMC is %2x\n", reg_val);
	writel(reg_val, chip->reg_base + IR_TX_MCR);

	return 0;
}

static int sunxi_irtx_set_duty_cycle(struct rc_dev *rcdev, u32 duty_cycle)
{
	unsigned int reg_val;
	struct sunxi_irtx *chip = rcdev->priv;
	struct device *dev = &chip->pdev->dev;

	if (duty_cycle > 100) {
		dev_err(dev, "invalid duty_cycle: %d\n", duty_cycle);
		return -EINVAL;
	}

	dev_dbg(dev, "set duty cycle to %d\n", duty_cycle);
	reg_val = readl(chip->reg_base + IR_TX_GLR);

	/* clear bit5 and bit6 */
	reg_val &= 0x9f;
	if (duty_cycle < 30) {
		reg_val |= 0x40; /* set bit6=1 */
		dev_dbg(dev, "set duty cycle to 25%%\n");
	} else if (duty_cycle < 40) {
		reg_val |= 0x20; /* set bit5=1 */
		dev_dbg(dev, "set duty cycle to 33%%\n");
	} else {
		/* do nothing, bit5 and bit6 are already cleared to 0 */
		dev_dbg(dev, "set duty cycle to 50%%\n");
	}

	dev_dbg(dev, "reg_val of IR_TX_GLR: %2x\n", reg_val);
	writel(reg_val, chip->reg_base + IR_TX_GLR);

	return 0;
}

/*
 * run_length_encode - encode raw data as run_length_encode format
 * @raw_data: raw data are from userspace, such as 0x01002328,
 *			  bit24=1 indicates the output wave is high level and
 *			  otherwise low level; the low 24 bits of raw data
 *            indicate the number of pulse cycle.
 * @buf: store run_length_encode data and will be written to tx fifo
 * @count: the current index of buf
 */
static unsigned int run_length_encode(unsigned int *raw_data, unsigned char *buf,
					unsigned int count, struct sunxi_irtx *chip)
{
	unsigned int is_high_level = 0;
	unsigned int num = 0; /* the number of pulse cycle */
	struct device *dev = &chip->pdev->dev;

	/* output high level or low level */
	is_high_level = (*raw_data >> 24) & 0x01;
	dev_dbg(dev, "is high level: %d\n", is_high_level);

	/* calculate the number of pulse cycle */
	num = ((*raw_data & 0x00FFFFFF) * 3) / three_pulse_cycle;
	/* check number is over 127 or not */
	while (num > 0x7f) {
		buf[count] = (is_high_level << 7) | 0x7f;
		dev_dbg(dev, "current buffer data is %2x\n", buf[count]);
		count++;
		num -= 0x7f;
	}

	buf[count] = (is_high_level << 7) | num;
	dev_dbg(dev, "current buffer data is %2x\n", buf[count]);
	count++;

	return count;
}

static int sunxi_irtx_xmit(struct rc_dev *rcdev, unsigned int *txbuf,
							unsigned int count)
{
	int i, ret, num;
	bool mark = 0;
	unsigned int index = 0;
	unsigned int *head_p, *data_p, *stop_p;
	struct sunxi_irtx *chip = rcdev->priv;
	struct device *dev = &chip->pdev->dev;

	if (unlikely(!rcdev)) {
		dev_dbg(dev, "device is null\n");
		return -EINVAL;
	}

	if (unlikely(count > IR_TX_RAW_BUF_SIZE)) {
		dev_dbg(dev, "too many raw data\n");
		return -EINVAL;
	}

	dev_dbg(dev, "transmit %d raw data\n", count);

	sunxi_irtx_reset_rawbuffer(chip);

	/* encode the guide code */
	head_p = txbuf;
	dev_dbg(dev, "head pulse: '%#x', head space: '%#x'\n",
				*head_p, *(head_p + 1));
	/* pull the level bit high */
	*head_p |= (1 << 24);
	index = run_length_encode(head_p, chip->ir_rawbuf.tx_buf, index, chip);
	head_p++;
	index = run_length_encode(head_p, chip->ir_rawbuf.tx_buf, index, chip);

	/* encode the payload: addr, ~addr, cmd, ~cmd */
	data_p = (++head_p);
	dev_dbg(dev, "valid raw data is:\n");
	/* count = head(2) + payload(num) + end(2) */
	num = count - 4;
	for (i = 0; i < num; i++) {
		dev_dbg(dev, "%#x ", *data_p);

		/* cycle the level bit up and down  */
		mark = !mark;
		*data_p |= (mark << 24);

		data_p++;
		if ((i + 1) % 8 == 0)
			dev_dbg(dev, "\n");
	}
	dev_dbg(dev, "\n");

	/* encode the end code */
	stop_p = data_p;
	dev_dbg(dev, "stop pulse: '%#x', stop space: '%#x'\n",
				*stop_p, *(stop_p + 1));
	index = run_length_encode(stop_p, chip->ir_rawbuf.tx_buf, index, chip);
	stop_p++;
	/* pull the stop bit level high  */
	*stop_p |= (1 << 24);
	index = run_length_encode(stop_p, chip->ir_rawbuf.tx_buf, index, chip);

	/* avoid the level being continuously pulled up */
	stop_p++;
	index = run_length_encode(stop_p, chip->ir_rawbuf.tx_buf, index, chip);

	/* update ir_rawbuf.tx_dcnt */
	chip->ir_rawbuf.tx_dcnt = index;
	dev_dbg(dev, "ir_rawbuf total count is %d\n", chip->ir_rawbuf.tx_dcnt);

	/* send ir code to tx fifo */
	ret = sunxi_send_ir_code(chip);
	if (unlikely(ret)) {
		dev_dbg(dev, "send ir code fail\n");
		ret = -EINVAL;
	} else {
		ret = count;
	}

	return ret;
}

static int sunxi_irtx_resource_get(struct sunxi_irtx *chip)
{
	struct resource *res;
	struct device *dev = &chip->pdev->dev;
	int err;

	res = platform_get_resource(chip->pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	chip->reg_base = devm_ioremap_resource(&chip->pdev->dev, res);
	if (IS_ERR(chip->reg_base)) {
		dev_err(&chip->pdev->dev, "fail to map IO resource\n");
		return PTR_ERR(chip->reg_base);
	}

	chip->irq_num = platform_get_irq(chip->pdev, 0);
	if (chip->irq_num < 0)
		return -EINVAL;

	chip->pctrl = devm_pinctrl_get(&chip->pdev->dev);
	if (IS_ERR(chip->pctrl)) {
		dev_err(&chip->pdev->dev, "devm_pinctrl_get() failed!\n");
		return PTR_ERR(chip->pctrl);
	}

	chip->reset = devm_reset_control_get(&chip->pdev->dev, NULL);
	if (IS_ERR(chip->reset)) {
		dev_err(&chip->pdev->dev, "Failed to get reset handle\n");
		return PTR_ERR(chip->reset);
	}

	chip->bclk = devm_clk_get(&chip->pdev->dev, "bus");
	if (!chip->bclk) {
		dev_err(&chip->pdev->dev, "Failed to get bus clk.\n");
		return -EBUSY;
	}

	chip->pclk = devm_clk_get(&chip->pdev->dev, "pclk");
	if (!chip->pclk) {
		dev_err(&chip->pdev->dev, "Failed to get parent clk.\n");
		return -EBUSY;
	}

	chip->mclk = devm_clk_get(&chip->pdev->dev, "mclk");
	if (!chip->mclk) {
		dev_err(&chip->pdev->dev, "Failed to get ir tx clk.\n");
		return -EBUSY;
	}

	err = sunxi_irtx_regulator_request(chip);
	if (err) {
		dev_err(&chip->pdev->dev, "request regulator failed!\n");
		return err;
	}

	return 0;
}

static int sunxi_irtx_rc_init(struct sunxi_irtx *chip)
{
	int ret;
	static char irtx_dev_name[] = "irtx";
	struct device *dev = &chip->pdev->dev;

	chip->rcdev = devm_rc_allocate_device(dev, RC_DRIVER_IR_RAW);
	if (!chip->rcdev) {
		dev_err(dev, "rc dev allocate fail!\n");
		return -ENOMEM;
	}

	/* initialize rcdev */
	chip->rcdev->priv = chip;
	chip->rcdev->input_phys = SUNXI_IR_TX_DEVICE_NAME"/input0";
	chip->rcdev->input_id.bustype = BUS_HOST;
	chip->rcdev->input_id.vendor = 0x0001;
	chip->rcdev->input_id.product = 0x0001;
	chip->rcdev->input_id.version = 0x0100;
	chip->rcdev->input_dev->dev.init_name = &irtx_dev_name[0];

	chip->rcdev->dev.parent = dev;
	chip->rcdev->map_name = RC_MAP_SUNXI;
	chip->rcdev->device_name = SUNXI_IR_TX_DEVICE_NAME;
	chip->rcdev->driver_name = SUNXI_IR_TX_DRIVER_NAME;
	chip->rcdev->driver_type = RC_DRIVER_IR_RAW;

	chip->rcdev->tx_ir = sunxi_irtx_xmit;
	chip->rcdev->s_tx_carrier = sunxi_irtx_set_carrier;
	chip->rcdev->s_tx_duty_cycle = sunxi_irtx_set_duty_cycle;

	ret = devm_rc_register_device(dev, chip->rcdev);
	if (ret) {
		dev_err(dev, "failed to register rc device\n");
		return ret;
	}

	return 0;
}

/*
static ssize_t irtx_test_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (dev == NULL) {
		printk("%s: device is NULL!\n", __func__);
		return 0;
	}

	return sprintf(buf, "Usage: echo 0xyyzz > irtx_test\n"
					"yy - address\n"
					"zz - command\n");
}

static ssize_t irtx_test_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	int ret = 0;
	unsigned int value = 0;
	unsigned char addr, cmd;

	if (dev == NULL) {
		printk("%s: device is NULL!\n", __func__);
		return count;
	}

	printk("%s: data is %s\n", __func__, buf);
	ret = kstrtoint(buf, 0, &value);
	if (ret) {
		printk("%s: kstrtou32 fail!\n", __func__);
		return ret;
	}

	printk("%s: value is 0x%#x\n", __func__, value);
	if ((value == 0) || (value > 0xffff)) {
		printk("%s: unvalid value!\n", __func__);
		return -1;
	}

	cmd = value & 0xff;
	addr = (value >> 8) & 0xff;

	irtx_packet_handler(addr, cmd);
	ret = sunxi_send_ir_code();
	if (ret == 0) {
		printk("send ir code success\n");
	}

	return count;
}
static DEVICE_ATTR(irtx_test, 0664, irtx_test_show, irtx_test_store);
*/

static int sunxi_irtx_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct sunxi_irtx *chip;

	dev_dbg(dev, "%s %s\n", SUNXI_IR_TX_DRIVER_NAME, SUNXI_IR_TX_VERSION);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (IS_ERR_OR_NULL(chip)) {
		dev_err(dev, "not enough memory for ir tx data\n");
		return -ENOMEM;
	}

	chip->pdev = pdev;

	/* initialize hardware resource */
	ret = sunxi_irtx_resource_get(chip);
	if (ret) {
		dev_err(dev, "ir-tx failed to get resource\n");
		goto err_startup;
	}

	ret = sunxi_irtx_rc_init(chip);
	if (ret) {
		dev_err(dev, "sunxi_irtx_rc_init failed.\n");
		goto err_startup;
	}

	ret = sunxi_irtx_hw_init(chip);
	if (ret) {
		dev_err(dev, "sunxi_irtx_hw_init failed.\n");
		goto err_register;
	}
	dev_dbg(dev, "sunxi_irtx_hw_init success\n");
	platform_set_drvdata(pdev, chip);

	ret = devm_request_irq(dev, chip->irq_num, sunxi_irtx_isr, 0,
						"RemoteIR_TX", chip);
	if (ret) {
		dev_err(dev, "request irq fail.\n");
		goto err_request_irq;
	}

	/* FIXME: need to fix */
	/* device_create_file(dev, &dev_attr_irtx_test); */

	dev_dbg(dev, "probe success\n");

	return 0;

err_request_irq:
	sunxi_irtx_hw_exit(chip);
err_register:
	rc_unregister_device(chip->rcdev);
err_startup:
	return ret;
}

static int sunxi_irtx_remove(struct platform_device *pdev)
{
	struct sunxi_irtx *chip = platform_get_drvdata(pdev);

	/* device_remove_file(&pdev->dev, &dev_attr_irtx_test); */
	sunxi_irtx_hw_exit(chip);
	rc_unregister_device(chip->rcdev);
	sunxi_irtx_regulator_release(chip);

	return 0;
}

static const struct of_device_id sunxi_irtx_of_match[] = {
	{ .compatible = "allwinner,irtx", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_irtx_of_match);

#if IS_ENABLED(CONFIG_PM)
static int sunxi_irtx_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_irtx *chip = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s enter\n", __func__);

	disable_irq_nosync(chip->irq_num);

	sunxi_irtx_save_regs(chip);

	sunxi_irtx_hw_exit(chip);

	return 0;
}

static int sunxi_irtx_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_irtx *chip = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s enter\n", __func__);

	sunxi_irtx_hw_init(chip);

	sunxi_irtx_restore_regs(chip);

	enable_irq(chip->irq_num);

	return 0;
}

static const struct dev_pm_ops sunxi_irtx_pm_ops = {
	.suspend        = sunxi_irtx_suspend,
	.resume         = sunxi_irtx_resume,
};
#endif

static struct platform_driver sunxi_irtx_driver = {
	.probe  = sunxi_irtx_probe,
	.remove = sunxi_irtx_remove,
	.driver = {
		.name   = SUNXI_IR_TX_DRIVER_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_irtx_of_match,
#if IS_ENABLED(CONFIG_PM)
		.pm	= &sunxi_irtx_pm_ops,
#endif
	},
};

module_platform_driver(sunxi_irtx_driver);
MODULE_AUTHOR("luruixiang <luruixiang@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Remote IR TX driver");
MODULE_VERSION("2.1.3");
