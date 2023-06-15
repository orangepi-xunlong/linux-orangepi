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
#include "sunxi-ir-tx.h"

static u32 debug_mask = 1;
#define dprintk(level_mask, fmt, arg...)				\
do {									\
	if (unlikely(debug_mask & level_mask))				\
		pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg);	\
} while (0)

#define IRTX_ERR(fmt, arg...) pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)

struct ir_raw_buffer {
	unsigned int tx_dcnt;
	unsigned char tx_buf[IR_TX_RAW_BUF_SIZE];
};

struct sunxi_ir_tx_data {
	void __iomem *reg_base;
	struct platform_device	*pdev;
	struct rc_dev *rcdev;
	struct clk *bclk;
	struct clk *pclk;
	struct clk *mclk;
	struct reset_control *reset;
	struct regulator *suply;
	struct pinctrl *pctrl;
	unsigned int suply_vol;
	int irq_num;
};

static struct sunxi_ir_tx_data *ir_tx_data;
static struct ir_raw_buffer     ir_rawbuf;

/*
 * one pulse cycle(=10.666666us) is not an integer, so in order to
 * improve the accuracy, define the value of three pulse cycle here.
 */
static unsigned int three_pulse_cycle = 32;

static inline int ir_tx_fifo_empty(void)
{
	unsigned int reg_val;

	reg_val = readl(ir_tx_data->reg_base + IR_TX_TACR);
	dprintk(DEBUG_INFO, "%3u bytes fifo available\n", reg_val);

	return (reg_val == IR_TX_FIFO_SIZE);
}

static inline int ir_tx_fifo_full(void)
{
	unsigned int reg_val;

	reg_val = readl(ir_tx_data->reg_base + IR_TX_TACR);
	dprintk(DEBUG_INFO, "%3u bytes fifo available\n", reg_val);

	return (reg_val == 0);
}

static inline void ir_tx_reset_rawbuffer(void)
{
	int i;

	ir_rawbuf.tx_dcnt = 0;
	for (i = 0; i < IR_TX_RAW_BUF_SIZE; i++)
		ir_rawbuf.tx_buf[i] = 0;
}

/**
 * This function implements encoding flow of NEC protocol,
 * just used to test sunxi ir-tx's basic function.
 */
void ir_tx_packet_handler(unsigned char address, unsigned char command)
{
	unsigned int  i, j;
	unsigned int  count = 0;
	unsigned char buffer[256];
	unsigned char tx_code[4];

	ir_tx_reset_rawbuffer();

	tx_code[0] = address;
	tx_code[1] = ~address;
	tx_code[2] = command;
	tx_code[3] = ~command;

	dprintk(DEBUG_INFO, "addr: 0x%x  addr': 0x%x  cmd: 0x%x  cmd': 0x%x\n",
			tx_code[0], tx_code[1], tx_code[2], tx_code[3]);

	// go encoding
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
		ir_rawbuf.tx_buf[ir_rawbuf.tx_dcnt++] = buffer[i];

	dprintk(DEBUG_INFO, "tx_dcnt = %d\n", ir_rawbuf.tx_dcnt);
}

static int send_ir_code(void)
{
	unsigned int i, idle_threshold;
	unsigned int reg_val;

	dprintk(DEBUG_INFO, "enter\n");

	/* reset transmit and flush fifo */
	reg_val = readl(ir_tx_data->reg_base + IR_TX_GLR);
	reg_val |= 0x02;
	writel(reg_val, ir_tx_data->reg_base + IR_TX_GLR);

	/* get idle threshold */
	idle_threshold = (readl(ir_tx_data->reg_base + IR_TX_IDC_H) << 8)
		| readl(ir_tx_data->reg_base + IR_TX_IDC_L);
	dprintk(DEBUG_INFO, "idle_threshold = %d\n", idle_threshold);

	/* set transmit threshold */
	writel((ir_rawbuf.tx_dcnt - 1), ir_tx_data->reg_base + IR_TX_TR);

	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_GLR   = 0x%2x\n", IR_TX_GLR,
			readl(ir_tx_data->reg_base + IR_TX_GLR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_MCR   = 0x%2x\n", IR_TX_MCR,
			readl(ir_tx_data->reg_base + IR_TX_MCR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_CR    = 0x%2x\n", IR_TX_CR,
			readl(ir_tx_data->reg_base + IR_TX_CR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_IDC_H = 0x%2x\n", IR_TX_IDC_H,
			readl(ir_tx_data->reg_base + IR_TX_IDC_H));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_IDC_L = 0x%2x\n", IR_TX_IDC_L,
			readl(ir_tx_data->reg_base + IR_TX_IDC_L));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_ICR_H = 0x%2x\n", IR_TX_ICR_H,
			readl(ir_tx_data->reg_base + IR_TX_ICR_H));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_ICR_L = 0x%2x\n", IR_TX_ICR_L,
			readl(ir_tx_data->reg_base + IR_TX_ICR_L));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_TELR  = 0x%2x\n", IR_TX_TELR,
			readl(ir_tx_data->reg_base + IR_TX_TELR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_INTC  = 0x%2x\n", IR_TX_INTC,
			readl(ir_tx_data->reg_base + IR_TX_INTC));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_TACR  = 0x%2x\n", IR_TX_TACR,
			readl(ir_tx_data->reg_base + IR_TX_TACR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_STAR  = 0x%2x\n", IR_TX_STAR,
			readl(ir_tx_data->reg_base + IR_TX_STAR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_TR    = 0x%2x\n", IR_TX_TR,
			readl(ir_tx_data->reg_base + IR_TX_TR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_DMAC  = 0x%2x\n", IR_TX_DMAC,
			readl(ir_tx_data->reg_base + IR_TX_DMAC));

	if (ir_rawbuf.tx_dcnt <= IR_TX_FIFO_SIZE) {
		for (i = 0; i < ir_rawbuf.tx_dcnt; i++) {
			writeb(ir_rawbuf.tx_buf[i],
					ir_tx_data->reg_base + IR_TX_FIFO_DR);
		}
	} else {
		dprintk(DEBUG_INFO, "invalid packet\n");
		return -1;
	}

	reg_val = readl(ir_tx_data->reg_base + IR_TX_TACR);
	dprintk(DEBUG_INFO, "%3u bytes fifo available\n", reg_val);

	if (IR_TX_CYCLE_TYPE) {
		for (i = 0; i < ir_rawbuf.tx_dcnt; i++)
			dprintk(DEBUG_INFO, "%d, ir txbuffer code = 0x%x!\n",
					i, ir_rawbuf.tx_buf[i]);
		reg_val = readl(ir_tx_data->reg_base + IR_TX_CR);
		reg_val |= (0x01 << 7);
		writel(reg_val, ir_tx_data->reg_base + IR_TX_CR);
	} else {
		while (!ir_tx_fifo_empty()) {
			reg_val = readl(ir_tx_data->reg_base + IR_TX_TACR);
			dprintk(DEBUG_INFO,
				"fifo under run. %3u bytes fifo available\n",
				reg_val);
		}
	}

	/* wait idle finish */
	while ((readl(ir_tx_data->reg_base + IR_TX_ICR_H) << 8
				| readl(ir_tx_data->reg_base + IR_TX_ICR_L))
				< idle_threshold)
		dprintk(DEBUG_INFO, "wait idle\n");

	dprintk(DEBUG_INFO, "finish\n");

	return 0;
}

static inline unsigned int ir_tx_get_intsta(void)
{
	return readl(ir_tx_data->reg_base + IR_TX_STAR);
}

static inline void ir_tx_clr_intsta(unsigned int bitmap)
{
	unsigned int reg_val;

	reg_val = readl(ir_tx_data->reg_base + IR_TX_STAR);
	reg_val &= ~0xff;
	reg_val |= bitmap & 0xff;
	writel(reg_val, ir_tx_data->reg_base + IR_TX_STAR);
}

static irqreturn_t ir_tx_irq_service(int irqno, void *dev_id)
{
	unsigned int intsta;

	intsta = ir_tx_get_intsta();
	dprintk(DEBUG_INFO, "IR TX IRQ Serve %#x\n", intsta);

	ir_tx_clr_intsta(intsta);

	return IRQ_HANDLED;
}

static void ir_tx_reg_clear(struct sunxi_ir_tx_data *ir_tx_data)
{
	writel(0, ir_tx_data->reg_base + IR_TX_GLR);
}

static void ir_tx_reg_cfg(struct sunxi_ir_tx_data *ir_tx_data)
{
	writel(IR_TX_MC_VALUE, ir_tx_data->reg_base + IR_TX_MCR);
	writel(IR_TX_CLK_VALUE, ir_tx_data->reg_base + IR_TX_CR);
	writel(IR_TX_IDC_H_VALUE, ir_tx_data->reg_base + IR_TX_IDC_H);
	writel(IR_TX_IDC_L_VALUE, ir_tx_data->reg_base + IR_TX_IDC_L);
	writel(IR_TX_STA_VALUE, ir_tx_data->reg_base + IR_TX_STAR);
	writel(IR_TX_INT_C_VALUE, ir_tx_data->reg_base + IR_TX_INTC);
	writel(IR_TX_GL_VALUE, ir_tx_data->reg_base + IR_TX_GLR);

	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_GLR   = 0x%2x\n", IR_TX_GLR,
			readl(ir_tx_data->reg_base + IR_TX_GLR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_MCR   = 0x%2x\n", IR_TX_MCR,
			readl(ir_tx_data->reg_base + IR_TX_MCR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_CR    = 0x%2x\n", IR_TX_CR,
			readl(ir_tx_data->reg_base + IR_TX_CR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_IDC_H = 0x%2x\n", IR_TX_IDC_H,
			readl(ir_tx_data->reg_base + IR_TX_IDC_H));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_IDC_L = 0x%2x\n", IR_TX_IDC_L,
			readl(ir_tx_data->reg_base + IR_TX_IDC_L));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_ICR_H = 0x%2x\n", IR_TX_ICR_H,
			readl(ir_tx_data->reg_base + IR_TX_ICR_H));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_ICR_L = 0x%2x\n", IR_TX_ICR_L,
			readl(ir_tx_data->reg_base + IR_TX_ICR_L));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_TELR  = 0x%2x\n", IR_TX_TELR,
			readl(ir_tx_data->reg_base + IR_TX_TELR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_INTC  = 0x%2x\n", IR_TX_INTC,
			readl(ir_tx_data->reg_base + IR_TX_INTC));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_TACR  = 0x%2x\n", IR_TX_TACR,
			readl(ir_tx_data->reg_base + IR_TX_TACR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_STAR  = 0x%2x\n", IR_TX_STAR,
			readl(ir_tx_data->reg_base + IR_TX_STAR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_TR    = 0x%2x\n", IR_TX_TR,
			readl(ir_tx_data->reg_base + IR_TX_TR));
	dprintk(DEBUG_INFO, "Offset: 0x%2x IR_TX_DMAC  = 0x%2x\n", IR_TX_DMAC,
			readl(ir_tx_data->reg_base + IR_TX_DMAC));

}

static int ir_tx_clk_cfg(struct sunxi_ir_tx_data *ir_tx_data)
{
	unsigned long rate = 0;
	int ret = 0;

	ret = reset_control_deassert(ir_tx_data->reset);
	if (ret) {
		IRTX_ERR("deassert ir tx rst failed!\n");
		return ret;
	}

	rate = clk_get_rate(ir_tx_data->bclk);
	dprintk(DEBUG_INIT, "get ir bus clk rate %dHZ\n", (__u32)rate);

	rate = clk_get_rate(ir_tx_data->pclk);
	dprintk(DEBUG_INIT, "get ir parent clk rate %dHZ\n", (__u32)rate);

	ret = clk_set_parent(ir_tx_data->mclk, ir_tx_data->pclk);
	if (ret) {
		IRTX_ERR("set ir_clk parent failed!\n");
		return ret;
	}

	ret = clk_set_rate(ir_tx_data->mclk, IR_TX_CLK);
	if (ret) {
		IRTX_ERR("set ir clock freq to %d failed!\n", IR_TX_CLK);
		return ret;
	}
	dprintk(DEBUG_INIT, "set ir_clk rate %dHZ\n", IR_TX_CLK);

	rate = clk_get_rate(ir_tx_data->mclk);
	dprintk(DEBUG_INIT, "get ir_clk rate %dHZ\n", (__u32)rate);

	ret = clk_prepare_enable(ir_tx_data->bclk);
	if (ret) {
		IRTX_ERR("try to enable bus clk failed!\n");
		goto assert_reset;
	}

	ret = clk_prepare_enable(ir_tx_data->mclk);
	if (ret) {
		IRTX_ERR("try to enable ir_clk failed!\n");
		goto assert_reset;
	}

	return 0;

assert_reset:
	reset_control_assert(ir_tx_data->reset);

	return ret;
}

#ifdef CONFIG_PM
static void ir_tx_clk_uncfg(struct sunxi_ir_tx_data *ir_tx_data)
{
	if (IS_ERR_OR_NULL(ir_tx_data->mclk))
		IRTX_ERR("ir_clk handle is invalid, just return!\n");
	else {
		clk_disable_unprepare(ir_tx_data->mclk);
	}

	if (IS_ERR_OR_NULL(ir_tx_data->bclk))
		IRTX_ERR("bus clk handle is invalid, just return!\n");
	else {
		clk_disable_unprepare(ir_tx_data->bclk);
	}

	reset_control_assert(ir_tx_data->reset);
/*
	if (IS_ERR_OR_NULL(ir_tx_data->pclk))
		IRTX_ERR("ir_clk_source handle is invalid, just return!\n");
	else {
		clk_disable_unprepare(ir_tx_data->pclk);
	}
*/
}
#endif

static int ir_tx_select_gpio_state(struct pinctrl *pctrl, char *name)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		IRTX_ERR("IR_TX pinctrl_lookup_state(%s) failed! return %p \n",
				name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret) {
		IRTX_ERR("IR_TX pinctrl_select_state(%s) failed! return %d \n",
				name, ret);
		return ret;
	}

	return 0;
}

static int ir_tx_request_gpio(struct sunxi_ir_tx_data *ir_tx_data)
{
	ir_tx_data->pctrl = devm_pinctrl_get(&ir_tx_data->pdev->dev);
	if (IS_ERR(ir_tx_data->pctrl)) {
		IRTX_ERR("devm_pinctrl_get() failed!\n");
		return -1;
	}

	return ir_tx_select_gpio_state(ir_tx_data->pctrl,
			PINCTRL_STATE_DEFAULT);
}

static int ir_tx_setup(struct sunxi_ir_tx_data *ir_tx_data)
{
	int ret = 0;

	ret = ir_tx_request_gpio(ir_tx_data);
	if (ret) {
		IRTX_ERR("request gpio failed!\n");
		return ret;
	}

	ret = ir_tx_clk_cfg(ir_tx_data);
	if (ret) {
		IRTX_ERR("ir tx clk configure failed!\n");
		return ret;
	}

	ir_tx_reg_cfg(ir_tx_data);

	return 0;
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
static int sunxi_ir_tx_set_carrier(struct rc_dev *dev, u32 carrier_freq)
{
	unsigned int reg_val;
	unsigned int drmc = 1;

	if ((carrier_freq > 6000000) || (carrier_freq < 15000)) {
		IRTX_ERR("invalid frequency of carrier: %d\n", carrier_freq);
		return -EINVAL;
	}

	/* First, get the duty cycle of modulated carrier */
	reg_val = readl(ir_tx_data->reg_base + IR_TX_GLR);
	drmc = (reg_val >> 5) & 0x3;
	dprintk(DEBUG_INFO, "DRMC is %d\n", drmc);
	dprintk(DEBUG_INFO, "0: duty cycle 50%%\n"
						"1: duty cycle 33%%\n"
						"2: duty cycle 25%%\n");

	/* Then, calculate the value of N */
	reg_val = IR_TX_CLK / ((2 + drmc) * carrier_freq) - 1;
	reg_val &= 0xff;
	dprintk(DEBUG_INFO, "RFMC is %2x\n", reg_val);
	writel(reg_val, ir_tx_data->reg_base + IR_TX_MCR);

	return 0;
}

static int sunxi_ir_tx_set_duty_cycle(struct rc_dev *dev, u32 duty_cycle)
{
	unsigned int reg_val;

	if (duty_cycle > 100) {
		IRTX_ERR("invalid duty_cycle: %d\n", duty_cycle);
		return -EINVAL;
	}

	dprintk(DEBUG_INFO, "set duty cycle to %d\n", duty_cycle);
	reg_val = readl(ir_tx_data->reg_base + IR_TX_GLR);

	/* clear bit5 and bit6 */
	reg_val &= 0x9f;
	if (duty_cycle < 30) {
		reg_val |= 0x40; /* set bit6=1 */
		dprintk(DEBUG_INFO, "set duty cycle to 25%%\n");
	} else if (duty_cycle < 40) {
		reg_val |= 0x20; /* set bit5=1 */
		dprintk(DEBUG_INFO, "set duty cycle to 33%%\n");
	} else {
		/* do nothing, bit5 and bit6 are already cleared to 0 */
		dprintk(DEBUG_INFO, "set duty cycle to 50%%\n");
	}

	dprintk(DEBUG_INFO, "reg_val of IR_TX_GLR: %2x\n", reg_val);
	writel(reg_val, ir_tx_data->reg_base + IR_TX_GLR);

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
static unsigned int run_length_encode(unsigned int *raw_data,
										unsigned char *buf,
										unsigned int count)
{
	unsigned int is_high_level = 0;
	unsigned int num = 0; /* the number of pulse cycle */

	/* output high level or low level */
	is_high_level = (*raw_data >> 24) & 0x01;
	dprintk(DEBUG_INFO, "is high level: %d\n", is_high_level);

	/* calculate the number of pulse cycle */
	num = ((*raw_data & 0x00FFFFFF) * 3) / three_pulse_cycle;
	/* check number is over 127 or not */
	while (num > 0x7f) {
		buf[count] = (is_high_level << 7) | 0x7f;
		dprintk(DEBUG_INFO, "current buffer data is %2x\n", buf[count]);
		count++;
		num -= 0x7f;
	}

	buf[count] = (is_high_level << 7) | num;
	dprintk(DEBUG_INFO, "current buffer data is %2x\n", buf[count]);
	count++;

	return count;
}

static int sunxi_ir_tx_xmit(struct rc_dev *dev, unsigned int *txbuf,
							unsigned int count)
{
	int i, ret, num;
	int mark = 0;
	unsigned int index = 0;
	unsigned int *head_p, *data_p, *stop_p;

	if (unlikely(!dev)) {
		dprintk(DEBUG_INFO, "device is null\n");
		return -EINVAL;
	}

	if (unlikely(count > IR_TX_RAW_BUF_SIZE)) {
		dprintk(DEBUG_INFO, "too many raw data\n");
		return -EINVAL;
	}

	dprintk(DEBUG_INFO, "transmit %d raw data\n", count);

	ir_tx_reset_rawbuffer();

	/* encode the guide code */
	head_p = txbuf;
	dprintk(DEBUG_INFO, "head pulse: '%#x', head space: '%#x'\n",
				*head_p, *(head_p + 1));
	/* pull the level bit high */
	*head_p |= (1 << 24);
	index = run_length_encode(head_p, ir_rawbuf.tx_buf, index);
	head_p++;
	index = run_length_encode(head_p, ir_rawbuf.tx_buf, index);

	/* encode the payload: addr, ~addr, cmd, ~cmd */
	data_p = (++head_p);
	dprintk(DEBUG_INFO, "valid raw data is:\n");
	/* count = head(2) + payload(num) + end(2) */
	num = count - 4;
	for (i = 0; i < num; i++) {
		dprintk(DEBUG_INFO, "%#x ", *data_p);
		/* cycle the level bit up and down  */
		if (mark == 1) {
			index = run_length_encode(data_p, ir_rawbuf.tx_buf, index);
			mark = 0;
		} else if (mark == 0) {
			*data_p |= (1 << 24);
			index = run_length_encode(data_p, ir_rawbuf.tx_buf, index);
			mark = 1;
		}

		data_p++;
		if ((i + 1) % 8 == 0)
			dprintk(DEBUG_INFO, "\n");
	}
	dprintk(DEBUG_INFO, "\n");

	/* encode the end code */
	stop_p = data_p;
	dprintk(DEBUG_INFO, "stop pulse: '%#x', stop space: '%#x'\n",
				*stop_p, *(stop_p + 1));
	index = run_length_encode(stop_p, ir_rawbuf.tx_buf, index);
	stop_p++;
	/* pull the stop bit level high  */
	*stop_p |= (1 << 24);
	index = run_length_encode(stop_p, ir_rawbuf.tx_buf, index);

	/* update ir_rawbuf.tx_dcnt */
	ir_rawbuf.tx_dcnt = index;
	dprintk(DEBUG_INFO, "ir_rawbuf total count is %d\n", ir_rawbuf.tx_dcnt);

	/* send ir code to tx fifo */
	ret = send_ir_code();

	if (unlikely(ret)) {
		dprintk(DEBUG_INFO, "send ir code fail\n");
		ret = -EINVAL;
	} else {
		ret = count;
	}

	return ret;
}

static int sunxi_ir_tx_startup(struct platform_device *pdev,
				struct sunxi_ir_tx_data *ir_tx_data)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	ir_tx_data->reg_base = of_iomap(np, 0);
	if (ir_tx_data->reg_base == NULL) {
		IRTX_ERR("Failed to ioremap() io memory region.\n");
		return -EBUSY;
	}
	dprintk(DEBUG_INIT, "base: %p !\n", ir_tx_data->reg_base);

	ir_tx_data->irq_num = irq_of_parse_and_map(np, 0);
	if (ir_tx_data->irq_num == 0) {
		IRTX_ERR("Failed to map irq.\n");
		ret = -EBUSY;
		goto out_iounmap;

	}
	dprintk(DEBUG_INIT, "irq num: %d !\n", ir_tx_data->irq_num);

	ir_tx_data->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(ir_tx_data->reset)) {
		IRTX_ERR("Failed to get reset handle\n");
		ret = PTR_ERR(ir_tx_data->reset);
		goto out_dispose_mapping;
	}

	ir_tx_data->bclk = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR_OR_NULL(ir_tx_data->bclk)) {
		IRTX_ERR("Failed to get bus clk.\n");
		ret = -EBUSY;
		goto out_dispose_mapping;
	}

	ir_tx_data->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR_OR_NULL(ir_tx_data->pclk)) {
		IRTX_ERR("Failed to get parent clk.\n");
		ret = -EBUSY;
		goto out_dispose_mapping;
	}

	ir_tx_data->mclk = devm_clk_get(&pdev->dev, "mclk");
	if (IS_ERR_OR_NULL(ir_tx_data->mclk)) {
		IRTX_ERR("Failed to get ir tx clk.\n");
		ret = -EBUSY;
		goto out_dispose_mapping;
	}

	return 0;

out_dispose_mapping:
	irq_dispose_mapping(ir_tx_data->irq_num);
out_iounmap:
	iounmap(ir_tx_data->reg_base);

	return ret;
}

static ssize_t ir_tx_test_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (dev == NULL) {
		printk("%s: device is NULL!\n", __func__);
		return 0;
	}

	return sprintf(buf, "Usage: echo 0xyyzz > ir_tx_test\n"
					"yy - address\n"
					"zz - command\n");
}

static ssize_t ir_tx_test_store(struct device *dev,
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

	ir_tx_packet_handler(addr, cmd);
	ret = send_ir_code();
	if (ret == 0) {
		printk("send ir code success\n");
	}

	return count;
}
static DEVICE_ATTR(ir_tx_test, 0664, ir_tx_test_show, ir_tx_test_store);

static int sunxi_ir_tx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rc_dev *ir_tx_rcdev;
	int ret = 0;
	static char ir_tx_dev_name[] = "ir_tx";

	dprintk(DEBUG_INIT, "%s %s\n", SUNXI_IR_TX_DRIVER_NAME,
									SUNXI_IR_TX_VERSION);

	ir_tx_data = devm_kzalloc(dev, sizeof(*ir_tx_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ir_tx_data)) {
		IRTX_ERR("not enough memory for ir tx data\n");
		return -ENOMEM;
	}

	if (pdev->dev.of_node) {
		/* initialize hardware resource */
		ret = sunxi_ir_tx_startup(pdev, ir_tx_data);
		if (ret) {
			IRTX_ERR("initialize hardware resource fail\n");
			goto err_startup;
		}
	} else {
		IRTX_ERR("device tree err!\n");
		ret = -EBUSY;
		goto err_startup;
	}
	dprintk(DEBUG_INIT, "initialize hardware resource success\n");

	ir_tx_rcdev = devm_rc_allocate_device(dev, RC_DRIVER_IR_RAW);
	if (!ir_tx_rcdev) {
		IRTX_ERR("rc dev allocate fail!\n");
		ret = -ENOMEM;
		goto err_startup;
	}

	/* initialize ir_tx_rcdev*/
	ir_tx_rcdev->priv = ir_tx_data;
	ir_tx_rcdev->input_phys = SUNXI_IR_TX_DEVICE_NAME "/input0";
	ir_tx_rcdev->input_id.bustype = BUS_HOST;
	ir_tx_rcdev->input_id.vendor = 0x0001;
	ir_tx_rcdev->input_id.product = 0x0001;
	ir_tx_rcdev->input_id.version = 0x0100;
	ir_tx_rcdev->input_dev->dev.init_name = &ir_tx_dev_name[0];

	ir_tx_rcdev->dev.parent = dev;
	ir_tx_rcdev->map_name = RC_MAP_SUNXI;
	ir_tx_rcdev->device_name = SUNXI_IR_TX_DEVICE_NAME;
	ir_tx_rcdev->driver_name = SUNXI_IR_TX_DRIVER_NAME;
	ir_tx_rcdev->driver_type = RC_DRIVER_IR_RAW;

	ir_tx_rcdev->tx_ir = sunxi_ir_tx_xmit;
	ir_tx_rcdev->s_tx_carrier = sunxi_ir_tx_set_carrier;
	ir_tx_rcdev->s_tx_duty_cycle = sunxi_ir_tx_set_duty_cycle;

	ret = devm_rc_register_device(dev, ir_tx_rcdev);
	if (ret) {
		IRTX_ERR("failed to register rc device\n");
		goto err_startup;
	}
	dprintk(DEBUG_INIT, "register rc device success\n");

	ir_tx_data->rcdev = ir_tx_rcdev;
	ir_tx_data->pdev = pdev;

	ret = ir_tx_setup(ir_tx_data);
	if (ret) {
		IRTX_ERR("ir_tx_setup failed.\n");
		ret = -EIO;
		goto err_startup;
	}
	dprintk(DEBUG_INIT, "ir_tx_setup success\n");

	platform_set_drvdata(pdev, ir_tx_data);
	ret = request_irq(ir_tx_data->irq_num, ir_tx_irq_service, 0,
						"RemoteIR_TX", ir_tx_data);
	if (ret) {
		IRTX_ERR(" request irq fail.\n");
		ret = -EBUSY;
		goto err_request_irq;
	}

	device_create_file(dev, &dev_attr_ir_tx_test);

	dprintk(DEBUG_INIT, "probe success\n");

	return 0;

err_request_irq:
	platform_set_drvdata(pdev, NULL);
	ir_tx_reg_clear(ir_tx_data);
	ir_tx_clk_uncfg(ir_tx_data);
	ir_tx_select_gpio_state(ir_tx_data->pctrl, PINCTRL_STATE_SLEEP);

err_startup:

	return ret;
}

static int sunxi_ir_tx_remove(struct platform_device *pdev)
{
	struct sunxi_ir_tx_data *ir_tx_data = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	device_remove_file(&pdev->dev, &dev_attr_ir_tx_test);
	free_irq(ir_tx_data->irq_num, &pdev->dev);
	ir_tx_reg_clear(ir_tx_data);
	ir_tx_clk_uncfg(ir_tx_data);
	ir_tx_select_gpio_state(ir_tx_data->pctrl, PINCTRL_STATE_SLEEP);

	return 0;
}

static const struct of_device_id sunxi_ir_tx_of_match[] = {
	{ .compatible = "allwinner,ir_tx", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_ir_tx_of_match);

#ifdef CONFIG_PM

static int ir_tx_resume_setup(struct sunxi_ir_tx_data *ir_tx_data)
{
	int ret;

	if (IS_ERR_OR_NULL(ir_tx_data->pctrl)) {
		pr_err("ir-tx pin config err\n");
		return -1;
	}

	ret = ir_tx_clk_cfg(ir_tx_data);
	if (ret) {
		pr_err("ir tx clk configure failed\n");
		return ret;
	}

	ir_tx_reg_cfg(ir_tx_data);

	ir_tx_select_gpio_state(ir_tx_data->pctrl, PINCTRL_STATE_DEFAULT);

	return 0;
}

static int ir_tx_suspend_setup(struct sunxi_ir_tx_data *ir_tx_data)
{
	ir_tx_select_gpio_state(ir_tx_data->pctrl, PINCTRL_STATE_SLEEP);

	ir_tx_reg_clear(ir_tx_data);

	ir_tx_clk_uncfg(ir_tx_data);

	return 0;
}

static int sunxi_ir_tx_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_ir_tx_data *ir_tx_data = platform_get_drvdata(pdev);
	int ret = 0;

	dprintk(DEBUG_SUSPEND, "enter\n");

	disable_irq_nosync(ir_tx_data->irq_num);

	ret = ir_tx_suspend_setup(ir_tx_data);
	if (ret) {
		pr_err("%s: ir tx suspend failed\n", __func__);
		return ret;
	}

	return 0;
}

static int sunxi_ir_tx_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_ir_tx_data *ir_tx_data = platform_get_drvdata(pdev);
	int ret = 0;

	dprintk(DEBUG_SUSPEND, "enter\n");

	ret = ir_tx_resume_setup(ir_tx_data);
	if (ret) {
		IRTX_ERR("ir_tx_setup failed!\n");
		return ret;
	}

	enable_irq(ir_tx_data->irq_num);

	return 0;
}

static const struct dev_pm_ops sunxi_ir_tx_pm_ops = {
	.suspend        = sunxi_ir_tx_suspend,
	.resume         = sunxi_ir_tx_resume,
};
#endif

static struct platform_driver sunxi_ir_tx_driver = {
	.probe  = sunxi_ir_tx_probe,
	.remove = sunxi_ir_tx_remove,
	.driver = {
		.name   = SUNXI_IR_TX_DRIVER_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_ir_tx_of_match,
#ifdef CONFIG_PM
		.pm	= &sunxi_ir_tx_pm_ops,
#endif
	},
};

module_platform_driver(sunxi_ir_tx_driver);
MODULE_AUTHOR("libohui <libohui@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Remote IR TX driver");
