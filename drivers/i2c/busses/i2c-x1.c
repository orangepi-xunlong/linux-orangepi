// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/scatterlist.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>
#include <linux/of_device.h>
#include <linux/rpmsg.h>

#include "i2c-x1.h"

#ifdef CONFIG_SOC_KY_X1

#define STARTUP_MSG		"startup"
#define IRQUP_MSG		"irqon"

struct instance_data {
	struct rpmsg_device *rpdev;
	struct ky_i2c_dev *ky_i2c;
};

static unsigned long long private_data[2];
static const struct of_device_id r_ky_i2c_dt_match[];
#endif

static inline u32 ky_i2c_read_reg(struct ky_i2c_dev *ky_i2c, int reg)
{
	return readl(ky_i2c->mapbase + reg);
}

static inline void
ky_i2c_write_reg(struct ky_i2c_dev *ky_i2c, int reg, u32 val)
{
	writel(val, ky_i2c->mapbase + reg);
}

static void ky_i2c_enable(struct ky_i2c_dev *ky_i2c)
{
	ky_i2c_write_reg(ky_i2c, REG_CR,
	ky_i2c_read_reg(ky_i2c, REG_CR) | CR_IUE);
}

static void ky_i2c_disable(struct ky_i2c_dev *ky_i2c)
{
	ky_i2c->i2c_ctrl_reg_value = ky_i2c_read_reg(ky_i2c, REG_CR) & ~CR_IUE;
	ky_i2c_write_reg(ky_i2c, REG_CR, ky_i2c->i2c_ctrl_reg_value);
}

static void ky_i2c_flush_fifo_buffer(struct ky_i2c_dev *ky_i2c)
{
	/* flush REG_WFIFO_WPTR and REG_WFIFO_RPTR */
	ky_i2c_write_reg(ky_i2c, REG_WFIFO_WPTR, 0);
	ky_i2c_write_reg(ky_i2c, REG_WFIFO_RPTR, 0);

	/* flush REG_RFIFO_WPTR and REG_RFIFO_RPTR */
	ky_i2c_write_reg(ky_i2c, REG_RFIFO_WPTR, 0);
	ky_i2c_write_reg(ky_i2c, REG_RFIFO_RPTR, 0);
}

static void ky_i2c_controller_reset(struct ky_i2c_dev *ky_i2c)
{
	/* i2c controller reset */
	ky_i2c_write_reg(ky_i2c, REG_CR, CR_UR);
	udelay(5);
	ky_i2c_write_reg(ky_i2c, REG_CR, 0);

	/* set load counter register */
	if (ky_i2c->i2c_lcr)
		ky_i2c_write_reg(ky_i2c, REG_LCR, ky_i2c->i2c_lcr);

	/* set wait counter register */
	if (ky_i2c->i2c_wcr)
		ky_i2c_write_reg(ky_i2c, REG_WCR, ky_i2c->i2c_wcr);
}

static void ky_i2c_bus_reset(struct ky_i2c_dev *ky_i2c)
{
	int clk_cnt = 0;
	u32 bus_status;

	/* if bus is locked, reset unit. 0: locked */
	bus_status = ky_i2c_read_reg(ky_i2c, REG_BMR);
	if (!(bus_status & BMR_SDA) || !(bus_status & BMR_SCL)) {
		ky_i2c_controller_reset(ky_i2c);
		usleep_range(10, 20);

		/* check scl status again */
		bus_status = ky_i2c_read_reg(ky_i2c, REG_BMR);
		if (!(bus_status & BMR_SCL))
			dev_alert(ky_i2c->dev, "unit reset failed\n");
	}

	while (clk_cnt < 9) {
		/* check whether the SDA is still locked by slave */
		bus_status = ky_i2c_read_reg(ky_i2c, REG_BMR);
		if (bus_status & BMR_SDA)
			break;

		/* if still locked, send one clk to slave to request release */
		ky_i2c_write_reg(ky_i2c, REG_RST_CYC, 0x1);
		ky_i2c_write_reg(ky_i2c, REG_CR, CR_RSTREQ);
		usleep_range(20, 30);
		clk_cnt++;
	}

	bus_status = ky_i2c_read_reg(ky_i2c, REG_BMR);
	if (clk_cnt >= 9 && !(bus_status & BMR_SDA))
		dev_alert(ky_i2c->dev, "bus reset clk reaches the max 9-clocks\n");
	else
		dev_alert(ky_i2c->dev, "bus reset, send clk: %d\n", clk_cnt);
}

static void ky_i2c_reset(struct ky_i2c_dev *ky_i2c)
{
	ky_i2c_controller_reset(ky_i2c);
}

static int ky_i2c_recover_bus_busy(struct ky_i2c_dev *ky_i2c)
{
	int timeout;
	int cnt, ret = 0;

	if (ky_i2c->high_mode)
		timeout = 1000; /* 1000us */
	else
		timeout = 1500; /* 1500us  */

	cnt = KY_I2C_BUS_RECOVER_TIMEOUT / timeout;

	if (likely(!(ky_i2c_read_reg(ky_i2c, REG_SR) & (SR_UB | SR_IBB))))
		return 0;

	/* wait unit and bus to recover idle */
	while (unlikely(ky_i2c_read_reg(ky_i2c, REG_SR) & (SR_UB | SR_IBB))) {
		if (cnt-- <= 0)
			break;

		usleep_range(timeout / 2, timeout);
	}

	if (unlikely(cnt <= 0)) {
		/* reset controller */
		ky_i2c_reset(ky_i2c);
		ret = -EAGAIN;
	}

	return ret;
}

static void ky_i2c_check_bus_release(struct ky_i2c_dev *ky_i2c)
{
	/* in case bus is not released after transfer completes */
	if (unlikely(ky_i2c_read_reg(ky_i2c, REG_SR) & SR_EBB)) {
		ky_i2c_bus_reset(ky_i2c);
		usleep_range(90, 150);
	}
}

static void ky_i2c_unit_init(struct ky_i2c_dev *ky_i2c)
{
	u32 cr_val = 0;

	/*
	 * Unmask interrupt bits for all xfer mode:
	 * bus error, arbitration loss detected.
	 * For transaction complete signal, we use master stop
	 * interrupt, so we don't need to unmask CR_TXDONEIE.
	 */
	cr_val |= CR_BEIE | CR_ALDIE;

	if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_INTERRUPT))
		/*
		 * Unmask interrupt bits for interrupt xfer mode:
		 * DBR rx full.
		 * For tx empty interrupt CR_DTEIE, we only
		 * need to enable when trigger byte transfer to start
		 * data sending.
		 */
		cr_val |= CR_DRFIE;
	else if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_FIFO))
		/* enable i2c FIFO mode*/
		cr_val |= CR_FIFOEN;
	else if (ky_i2c->xfer_mode == KY_I2C_MODE_DMA)
		/* enable i2c DMA mode*/
		cr_val |= CR_DMAEN | CR_FIFOEN;

	/* set speed bits */
	if (ky_i2c->fast_mode)
		cr_val |= CR_MODE_FAST;
	if (ky_i2c->high_mode)
		cr_val |= CR_MODE_HIGH | CR_GPIOEN;

	/* disable response to general call */
	cr_val |= CR_GCD;

	/* enable SCL clock output */
	cr_val |= CR_SCLE;

	/* enable master stop detected */
	cr_val |= CR_MSDE | CR_MSDIE;

	/* disable int to use pio xfer mode*/
	if (unlikely(ky_i2c->xfer_mode == KY_I2C_MODE_PIO))
		cr_val &= ~(CR_ALDIE | CR_BEIE | CR_MSDIE | CR_DTEIE);
	ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);
}

static void ky_i2c_trigger_byte_xfer(struct ky_i2c_dev *ky_i2c)
{
	u32 cr_val = ky_i2c_read_reg(ky_i2c, REG_CR);

	/* send start pulse */
	cr_val &= ~CR_STOP;
	if (ky_i2c->xfer_mode == KY_I2C_MODE_PIO)
		cr_val |= CR_START | CR_TB;
	else
		cr_val |= CR_START | CR_TB | CR_DTEIE;
	ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);
}

static inline void
ky_i2c_clear_int_status(struct ky_i2c_dev *ky_i2c, u32 mask)
{
	ky_i2c_write_reg(ky_i2c, REG_SR, mask & KY_I2C_INT_STATUS_MASK);
}

static bool ky_i2c_is_last_byte_to_send(struct ky_i2c_dev *ky_i2c)
{
	return (ky_i2c->tx_cnt == ky_i2c->cur_msg->len &&
		ky_i2c->msg_idx == ky_i2c->num - 1) ? true : false;
}

static bool ky_i2c_is_last_byte_to_receive(struct ky_i2c_dev *ky_i2c)
{
	/*
	 * if the message length is received from slave device,
	 * should at least to read out the length byte from slave.
	 */
	if (unlikely((ky_i2c->cur_msg->flags & I2C_M_RECV_LEN) &&
		!ky_i2c->smbus_rcv_len)) {
		return false;
	} else {
		return (ky_i2c->rx_cnt == ky_i2c->cur_msg->len - 1 &&
			ky_i2c->msg_idx == ky_i2c->num - 1) ? true : false;
	}
}

static void ky_i2c_mark_rw_flag(struct ky_i2c_dev *ky_i2c)
{
	if (ky_i2c->cur_msg->flags & I2C_M_RD) {
		ky_i2c->is_rx = true;
		ky_i2c->slave_addr_rw =
			((ky_i2c->cur_msg->addr & 0x7f) << 1) | 1;
	} else {
		ky_i2c->is_rx = false;
		ky_i2c->slave_addr_rw = (ky_i2c->cur_msg->addr & 0x7f) << 1;
	}
}

static void ky_i2c_byte_xfer_send_master_code(struct ky_i2c_dev *ky_i2c)
{
	u32 cr_val = ky_i2c_read_reg(ky_i2c, REG_CR);

	ky_i2c->phase = KY_I2C_XFER_MASTER_CODE;

	ky_i2c_write_reg(ky_i2c, REG_DBR, ky_i2c->master_code);

	cr_val &= ~(CR_STOP | CR_ALDIE);

	/* high mode: enable gpio to let I2C core generates SCL clock */
	cr_val |= CR_GPIOEN | CR_START | CR_TB | CR_DTEIE;
	ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);
}

static void ky_i2c_byte_xfer_send_slave_addr(struct ky_i2c_dev *ky_i2c)
{
	ky_i2c->phase = KY_I2C_XFER_SLAVE_ADDR;

	/* write slave address to DBR for interrupt mode */
	ky_i2c_write_reg(ky_i2c, REG_DBR, ky_i2c->slave_addr_rw);

	ky_i2c_trigger_byte_xfer(ky_i2c);
}

static int ky_i2c_byte_xfer(struct ky_i2c_dev *ky_i2c);
static int ky_i2c_byte_xfer_next_msg(struct ky_i2c_dev *ky_i2c);

static int ky_i2c_byte_xfer_body(struct ky_i2c_dev *ky_i2c)
{
	int ret = 0;
	u8  msglen = 0;
	u32 cr_val = ky_i2c_read_reg(ky_i2c, REG_CR);

	cr_val &= ~(CR_TB | CR_ACKNAK | CR_STOP | CR_START);
	ky_i2c->phase = KY_I2C_XFER_BODY;

	if (ky_i2c->i2c_status & SR_IRF) { /* i2c receive full */
		/* if current is transmit mode, ignore this signal */
		if (!ky_i2c->is_rx)
			return 0;

		/*
		 * if the message length is received from slave device,
		 * according to i2c spec, we should restrict the length size.
		 */
		if (unlikely((ky_i2c->cur_msg->flags & I2C_M_RECV_LEN) &&
				!ky_i2c->smbus_rcv_len)) {
			ky_i2c->smbus_rcv_len = true;
			msglen = (u8)ky_i2c_read_reg(ky_i2c, REG_DBR);
			if ((msglen == 0) ||
				(msglen > I2C_SMBUS_BLOCK_MAX)) {
				dev_err(ky_i2c->dev,
						"SMbus len out of range\n");
				*ky_i2c->msg_buf++ = 0;
				ky_i2c->rx_cnt = ky_i2c->cur_msg->len;
				cr_val |= CR_STOP | CR_ACKNAK;
				cr_val |= CR_ALDIE | CR_TB;
				ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);

				return 0;
			} else {
				*ky_i2c->msg_buf++ = msglen;
				ky_i2c->cur_msg->len = msglen + 1;
				ky_i2c->rx_cnt++;
			}
		} else {
			if (ky_i2c->rx_cnt < ky_i2c->cur_msg->len) {
				*ky_i2c->msg_buf++ =
					ky_i2c_read_reg(ky_i2c, REG_DBR);
				ky_i2c->rx_cnt++;
			}
		}
		/* if transfer completes, ISR will handle it */
		if (ky_i2c->i2c_status & (SR_MSD | SR_ACKNAK))
			return 0;

		/* trigger next byte receive */
		if (ky_i2c->rx_cnt < ky_i2c->cur_msg->len) {
			/* send stop pulse for last byte of last msg */
			if (ky_i2c_is_last_byte_to_receive(ky_i2c))
				cr_val |= CR_STOP | CR_ACKNAK;

			cr_val |= CR_ALDIE | CR_TB;
			ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);
		} else if (ky_i2c->msg_idx < ky_i2c->num - 1) {
			ret = ky_i2c_byte_xfer_next_msg(ky_i2c);
		} else {
			/*
			 * For this branch, we do nothing, here the receive
			 * transfer is already done, the master stop interrupt
			 * should be generated to complete this transaction.
			*/
		}
	} else if (ky_i2c->i2c_status & SR_ITE) { /* i2c transmit empty */
		/* MSD comes with ITE */
		if (ky_i2c->i2c_status & SR_MSD)
			return ret;

		if (ky_i2c->i2c_status & SR_RWM) { /* receive mode */
			/* if current is transmit mode, ignore this signal */
			if (!ky_i2c->is_rx)
				return 0;

			if (ky_i2c_is_last_byte_to_receive(ky_i2c))
				cr_val |= CR_STOP | CR_ACKNAK;

			/* trigger next byte receive */
			cr_val |= CR_ALDIE | CR_TB;

			/*
			 * Mask transmit empty interrupt to avoid useless tx
			 * interrupt signal after switch to receive mode, the
			 * next expected is receive full interrupt signal.
			 */
			cr_val &= ~CR_DTEIE;
			ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);
		} else { /* transmit mode */
			/* if current is receive mode, ignore this signal */
			if (ky_i2c->is_rx)
				return 0;

			if (ky_i2c->tx_cnt < ky_i2c->cur_msg->len) {
				ky_i2c_write_reg(ky_i2c, REG_DBR,
						*ky_i2c->msg_buf++);
				ky_i2c->tx_cnt++;

				/* send stop pulse for last byte of last msg */
				if (ky_i2c_is_last_byte_to_send(ky_i2c))
					cr_val |= CR_STOP;

				cr_val |= CR_ALDIE | CR_TB;
				ky_i2c_write_reg(ky_i2c, REG_CR, cr_val);
			} else if (ky_i2c->msg_idx < ky_i2c->num - 1) {
				ret = ky_i2c_byte_xfer_next_msg(ky_i2c);
			} else {
				/*
				 * For this branch, we do nothing, here the
				 * sending transfer is already done, the master
				 * stop interrupt should be generated to
				 * complete this transaction.
				*/
			}
		}
	}

	return ret;
}

static int ky_i2c_byte_xfer_next_msg(struct ky_i2c_dev *ky_i2c)
{
	if (ky_i2c->msg_idx == ky_i2c->num - 1)
		return 0;

	ky_i2c->msg_idx++;
	ky_i2c->cur_msg = ky_i2c->msgs + ky_i2c->msg_idx;
	ky_i2c->msg_buf = ky_i2c->cur_msg->buf;
	ky_i2c->rx_cnt = 0;
	ky_i2c->tx_cnt = 0;
	ky_i2c->i2c_err = 0;
	ky_i2c->i2c_status = 0;
	ky_i2c->smbus_rcv_len = false;
	ky_i2c->phase = KY_I2C_XFER_IDLE;

	ky_i2c_mark_rw_flag(ky_i2c);

	return ky_i2c_byte_xfer(ky_i2c);
}

static void ky_i2c_fifo_xfer_fill_buffer(struct ky_i2c_dev *ky_i2c)
{
	int finish, count = 0, fill = 0;
	u32 data = 0;
	u32 data_buf[KY_I2C_TX_FIFO_DEPTH * 2];
	int data_cnt = 0, i;
	unsigned long flags;

	while (ky_i2c->msg_idx < ky_i2c->num) {
		ky_i2c_mark_rw_flag(ky_i2c);

		if (ky_i2c->is_rx)
			finish = ky_i2c->rx_cnt;
		else
			finish = ky_i2c->tx_cnt;

		/* write master code to fifo buffer */
		if (ky_i2c->high_mode && ky_i2c->is_xfer_start) {
			data = ky_i2c->master_code;
			data |= WFIFO_CTRL_TB | WFIFO_CTRL_START;
			data_buf[data_cnt++] = data;

			fill += 2;
			count = min_t(size_t, ky_i2c->cur_msg->len - finish,
					KY_I2C_TX_FIFO_DEPTH - fill);
		} else {
			fill += 1;
			count = min_t(size_t, ky_i2c->cur_msg->len - finish,
					KY_I2C_TX_FIFO_DEPTH - fill);
		}

		ky_i2c->is_xfer_start = false;
		fill += count;
		data = ky_i2c->slave_addr_rw;
		data |= WFIFO_CTRL_TB | WFIFO_CTRL_START;

		/* write slave address to fifo buffer */
		data_buf[data_cnt++] = data;

		if (ky_i2c->is_rx) {
			ky_i2c->rx_cnt += count;

			if (ky_i2c->rx_cnt == ky_i2c->cur_msg->len &&
				ky_i2c->msg_idx == ky_i2c->num - 1)
				count -= 1;

			while (count > 0) {
				data = *ky_i2c->msg_buf | WFIFO_CTRL_TB;
				data_buf[data_cnt++] = data;
				ky_i2c->msg_buf++;
				count--;
			}

			if (ky_i2c->rx_cnt == ky_i2c->cur_msg->len &&
				ky_i2c->msg_idx == ky_i2c->num - 1) {
				data = *ky_i2c->msg_buf++;
				data = ky_i2c->slave_addr_rw | WFIFO_CTRL_TB |
					WFIFO_CTRL_STOP | WFIFO_CTRL_ACKNAK;
				data_buf[data_cnt++] = data;
			}
		} else {
			ky_i2c->tx_cnt += count;
			if (ky_i2c_is_last_byte_to_send(ky_i2c))
				count -= 1;

			while (count > 0) {
				data = *ky_i2c->msg_buf | WFIFO_CTRL_TB;
				data_buf[data_cnt++] = data;
				ky_i2c->msg_buf++;
				count--;
			}
			if (ky_i2c_is_last_byte_to_send(ky_i2c)) {
				data = *ky_i2c->msg_buf | WFIFO_CTRL_TB |
						WFIFO_CTRL_STOP;
				data_buf[data_cnt++] = data;
			}
		}

		if (ky_i2c->tx_cnt == ky_i2c->cur_msg->len ||
			ky_i2c->rx_cnt == ky_i2c->cur_msg->len) {
			ky_i2c->msg_idx++;
			if (ky_i2c->msg_idx == ky_i2c->num)
				break;

			ky_i2c->cur_msg = ky_i2c->msgs + ky_i2c->msg_idx;
			ky_i2c->msg_buf = ky_i2c->cur_msg->buf;
			ky_i2c->rx_cnt = 0;
			ky_i2c->tx_cnt = 0;
		}

		if (fill == KY_I2C_TX_FIFO_DEPTH)
			break;
	}

	spin_lock_irqsave(&ky_i2c->fifo_lock, flags);
	for (i = 0; i < data_cnt; i++)
		ky_i2c_write_reg(ky_i2c, REG_WFIFO, data_buf[i]);
	spin_unlock_irqrestore(&ky_i2c->fifo_lock, flags);
}

static void ky_i2c_fifo_xfer_copy_buffer(struct ky_i2c_dev *ky_i2c)
{
	int idx = 0, cnt = 0;
	struct i2c_msg *msg;

	/* copy the rx FIFO buffer to msg */
	while (idx < ky_i2c->num) {
		msg = ky_i2c->msgs + idx;
		if (msg->flags & I2C_M_RD) {
			cnt = msg->len;
			while (cnt > 0) {
				*(msg->buf + msg->len - cnt)
					= ky_i2c_read_reg(ky_i2c, REG_RFIFO);
				cnt--;
			}
		}
		idx++;
	}
}

static int ky_i2c_fifo_xfer(struct ky_i2c_dev *ky_i2c)
{
	int ret = 0;
	unsigned long time_left;

	ky_i2c_fifo_xfer_fill_buffer(ky_i2c);

	time_left = wait_for_completion_timeout(&ky_i2c->complete,
					ky_i2c->timeout);
	if (unlikely(time_left == 0)) {
		dev_alert(ky_i2c->dev, "fifo transfer timeout\n");
		ky_i2c_bus_reset(ky_i2c);
		ret = -ETIMEDOUT;
		goto err_out;
	}

	if (unlikely(ky_i2c->i2c_err)) {
		ret = -1;
		ky_i2c_flush_fifo_buffer(ky_i2c);
		goto err_out;
	}

	ky_i2c_fifo_xfer_copy_buffer(ky_i2c);

err_out:
	return ret;
}

static void ky_i2c_dma_copy_buffer(struct ky_i2c_dev *ky_i2c)
{
	int idx = 0, total = 0, i, cnt = 0;
	struct i2c_msg *cur_msg;

	/* calculate total rx bytes */
	while (idx < ky_i2c->num) {
		if ((ky_i2c->msgs + idx)->flags & I2C_M_RD)
			total += (ky_i2c->msgs + idx)->len;
		idx++;
	}

	idx = 0;
	total -= total % KY_I2C_RX_FIFO_DEPTH;
	while (idx < ky_i2c->num) {
		cur_msg = ky_i2c->msgs + idx;
		if (cur_msg->flags & I2C_M_RD) {
			for (i = 0; i < cur_msg->len; i++) {
				if (cnt < total) {
					*(cur_msg->buf + i) = ky_i2c->rx_dma_buf[cnt];
				} else {
					/* copy the rest bytes from FIFO  */
					*(cur_msg->buf + i) =
					ky_i2c_read_reg(ky_i2c, REG_RFIFO) &
					0xff;
				}
				cnt++;
			}
		}
		idx++;
	}
}

static void ky_i2c_dma_callback(void *data)
{
	return;
}

static int
ky_i2c_map_rx_sg(struct ky_i2c_dev *ky_i2c, int rx_nents, int *rx_total)
{
	int len;
	int rx_buf_start = *rx_total;

	*rx_total += ky_i2c->cur_msg->len;
	if (*rx_total < ky_i2c->rx_total) {
		len = ky_i2c->cur_msg->len;
	} else {
		len = ky_i2c->cur_msg->len - *rx_total + ky_i2c->rx_total;
		ky_i2c->rx_total = 0;
	}
	sg_set_buf(ky_i2c->rx_sg + rx_nents, &(ky_i2c->rx_dma_buf[rx_buf_start]), len);

	return dma_map_sg(ky_i2c->dev, ky_i2c->rx_sg + rx_nents,
			1, DMA_FROM_DEVICE);
}

static int ky_i2c_dma_xfer(struct ky_i2c_dev *ky_i2c)
{
	struct dma_async_tx_descriptor *tx_des = NULL, *rx_des = NULL;
	dma_cookie_t rx_ck = 0, tx_ck;
	u32 rx_nents = 0, tx_nents = 0, data;
	int ret = 0, idx = 0, count = 0, start = 0, i;
	unsigned long time_left;
	int rx_total = 0;
	int comp_timeout = 1000000; /* (us) */

	ky_i2c->rx_total -= ky_i2c->rx_total % KY_I2C_RX_FIFO_DEPTH;
	while (idx < ky_i2c->num) {
		ky_i2c->msg_idx = idx;
		ky_i2c->cur_msg = ky_i2c->msgs + idx;
		ky_i2c_mark_rw_flag(ky_i2c);

		if (idx == 0 && ky_i2c->high_mode) {
			/* fill master code */
			data = (ky_i2c->master_code & 0xff) |
				WFIFO_CTRL_TB | WFIFO_CTRL_START;
			*(ky_i2c->tx_dma_buf + count) = data;
			count++;
		}
		/* fill slave address */
		data = ky_i2c->slave_addr_rw |
				WFIFO_CTRL_TB | WFIFO_CTRL_START;
		*(ky_i2c->tx_dma_buf + count) = data;
		count++;

		if (ky_i2c->is_rx) {
			if (ky_i2c->rx_total) {
				ret = ky_i2c_map_rx_sg(ky_i2c,
						rx_nents, &rx_total);
				if (!ret) {
					dev_err(ky_i2c->dev,
						"failed to map scatterlist\n");
					ret = -EINVAL;
					goto err_map;
				}

				rx_nents++;
			}

			for (i = 0; i < ky_i2c->cur_msg->len - 1; i++) {
				data = ky_i2c->slave_addr_rw | WFIFO_CTRL_TB;
				*(ky_i2c->tx_dma_buf + count) = data;
				count++;
			}
			data = ky_i2c->slave_addr_rw | WFIFO_CTRL_TB;

			/* send nak and stop pulse for last msg */
			if (idx == ky_i2c->num - 1)
				data |= WFIFO_CTRL_ACKNAK | WFIFO_CTRL_STOP;
			*(ky_i2c->tx_dma_buf + count++) = data;
			start += ky_i2c->cur_msg->len;
		} else {
			for (i = 0; i < ky_i2c->cur_msg->len - 1; i++) {
				data = *(ky_i2c->cur_msg->buf + i) |
					WFIFO_CTRL_TB;
				*(ky_i2c->tx_dma_buf + count) = data;
				count++;
			}
			data = *(ky_i2c->cur_msg->buf + i) | WFIFO_CTRL_TB;

			/* send stop pulse for last msg */
			if (idx == ky_i2c->num - 1)
				data |= WFIFO_CTRL_STOP;
			*(ky_i2c->tx_dma_buf + count++) = data;
		}
		idx++;
	}

	sg_set_buf(ky_i2c->tx_sg, ky_i2c->tx_dma_buf,
			count * sizeof(ky_i2c->tx_dma_buf[0]));
	ret = dma_map_sg(ky_i2c->dev, ky_i2c->tx_sg, 1, DMA_TO_DEVICE);
	if (unlikely(!ret)) {
		dev_err(ky_i2c->dev, "failed to map scatterlist\n");
		ret = -EINVAL;
		goto err_map;
	}

	tx_nents++;
	tx_des = dmaengine_prep_slave_sg(ky_i2c->tx_dma, ky_i2c->tx_sg, 1,
				      DMA_MEM_TO_DEV,
				      DMA_PREP_INTERRUPT | DMA_PREP_FENCE);
	if (unlikely(!tx_des)) {
		dev_err(ky_i2c->dev, "failed to get dma tx descriptor\n");
		ret = -EINVAL;
		goto err_desc;
	}

	tx_des->callback = ky_i2c_dma_callback;
	tx_des->callback_param = ky_i2c;

	tx_ck = dmaengine_submit(tx_des);
	if (unlikely(dma_submit_error(tx_ck))) {
		ret = -EINVAL;
		goto err_desc;
	}

	if (likely(rx_nents)) {
		rx_des = dmaengine_prep_slave_sg(ky_i2c->rx_dma,
						ky_i2c->rx_sg,
						rx_nents, DMA_DEV_TO_MEM,
						DMA_PREP_INTERRUPT);
		if (unlikely(!rx_des)) {
			dev_err(ky_i2c->dev,
				"failed to get dma rx descriptor\n");
			ret = -EINVAL;
			goto err_desc;
		}

		rx_des->callback = ky_i2c_dma_callback;
		rx_des->callback_param = ky_i2c;
		rx_ck = dmaengine_submit(rx_des);
		if (unlikely(dma_submit_error(rx_ck))) {
			dev_err(ky_i2c->dev,
				"failed to submit rx channel\n");
			ret = -EINVAL;
			goto err_desc;
		}

		dma_async_issue_pending(ky_i2c->rx_dma);
	}

	dma_async_issue_pending(ky_i2c->tx_dma);

	time_left = wait_for_completion_timeout(&ky_i2c->complete,
						ky_i2c->timeout);
	if (unlikely(time_left == 0)) {
		dev_alert(ky_i2c->dev, "dma transfer timeout\n");
		ky_i2c_bus_reset(ky_i2c);
		ky_i2c_reset(ky_i2c);
		ret = -ETIMEDOUT;
		comp_timeout = 0;
		goto finish;
	}

	if (unlikely(ky_i2c->i2c_err)) {
		ret = -1;
		ky_i2c_flush_fifo_buffer(ky_i2c);
		comp_timeout = 0;
	}

finish:
	/*
	 * wait for the rx DMA to complete, for tx, we use the i2c
	 * TXDONE/STOP interrupt, here we already receive the
	 * TXDONE/STOP signal.
	 */
	if (unlikely(rx_nents && dma_async_is_tx_complete(ky_i2c->rx_dma,
				rx_ck, NULL, NULL) != DMA_COMPLETE)) {
		int timeout = comp_timeout;

		while (timeout > 0) {
			if (dma_async_is_tx_complete(ky_i2c->rx_dma,
				rx_ck, NULL, NULL) != DMA_COMPLETE) {
				usleep_range(2, 4);
				timeout -= 4;
			} else
				break;
		}
		if (timeout <= 0) {
			dmaengine_pause(ky_i2c->rx_dma);
			if (ret >= 0) {
				ret = -1;
				dev_err(ky_i2c->dev,
					"dma rx channel timeout\n");
			}
		}
	}

	if (likely(ret >= 0))
		ky_i2c_dma_copy_buffer(ky_i2c);

err_desc:
	dma_unmap_sg(ky_i2c->dev, ky_i2c->tx_sg, tx_nents, DMA_TO_DEVICE);
err_map:
	if (likely(rx_nents))
		dma_unmap_sg(ky_i2c->dev, ky_i2c->rx_sg,
				rx_nents, DMA_FROM_DEVICE);

	/* make sure terminate transfers and free descriptors */
	if (tx_des)
		dmaengine_terminate_all(ky_i2c->tx_dma);

	if (rx_des)
		dmaengine_terminate_all(ky_i2c->rx_dma);

	return ret < 0 ? ret : 0;
}

static int ky_i2c_byte_xfer(struct ky_i2c_dev *ky_i2c)
{
	int ret = 0;

	/* i2c error occurs */
	if (unlikely(ky_i2c->i2c_err))
		return -1;

	if (ky_i2c->phase == KY_I2C_XFER_IDLE) {
		if (ky_i2c->high_mode && ky_i2c->is_xfer_start)
			ky_i2c_byte_xfer_send_master_code(ky_i2c);
		else
			ky_i2c_byte_xfer_send_slave_addr(ky_i2c);

		ky_i2c->is_xfer_start = false;
	} else if (ky_i2c->phase == KY_I2C_XFER_MASTER_CODE) {
		ky_i2c_byte_xfer_send_slave_addr(ky_i2c);
	} else {
		ret = ky_i2c_byte_xfer_body(ky_i2c);
	}

	return ret;
}

static void ky_i2c_print_msg_info(struct ky_i2c_dev *ky_i2c)
{
	int i, j, idx;
	char printbuf[512];

	idx = sprintf(printbuf, "msgs: %d, mode: %d", ky_i2c->num,
					ky_i2c->xfer_mode);
	for (i = 0; i < ky_i2c->num && i < sizeof(printbuf) / 128; i++) {
		u16 len = ky_i2c->msgs[i].len & 0xffff;

		idx += sprintf(printbuf + idx, ", addr: %02x",
			ky_i2c->msgs[i].addr);
		idx += sprintf(printbuf + idx, ", flag: %c, len: %d",
			ky_i2c->msgs[i].flags & I2C_M_RD ? 'R' : 'W', len);
		if (!(ky_i2c->msgs[i].flags & I2C_M_RD)) {
			idx += sprintf(printbuf + idx, ", data:");
			/* print at most ten bytes of data */
			for (j = 0; j < len && j < 10; j++)
				idx += sprintf(printbuf + idx, " %02x",
					ky_i2c->msgs[i].buf[j]);
		}
	}

}

static int ky_i2c_handle_err(struct ky_i2c_dev *ky_i2c)
{
	if (unlikely(ky_i2c->i2c_err)) {
		dev_dbg(ky_i2c->dev, "i2c error status: 0x%08x\n",
				ky_i2c->i2c_status);
		if (ky_i2c->i2c_err & (SR_BED  | SR_ALD))
			ky_i2c_reset(ky_i2c);

		/* try transfer again */
		if (ky_i2c->i2c_err & (SR_RXOV | SR_ALD)) {
			ky_i2c_flush_fifo_buffer(ky_i2c);
			return -EAGAIN;
		}
		return (ky_i2c->i2c_status & SR_ACKNAK) ? -ENXIO : -EIO;
	}

	return 0;
}

#ifdef CONFIG_I2C_SLAVE
static void ky_i2c_slave_handler(struct ky_i2c_dev *ky_i2c)
{
	u32 status = ky_i2c->i2c_status;
	u8 value;

	/* clear interrupt status bits[31:18]*/
	ky_i2c_clear_int_status(ky_i2c, status);

	if (unlikely(status & (SR_EBB | SR_BED))) {
		dev_err(ky_i2c->dev,"i2c slave bus error status = 0x%x, reset controller\n", status);
		/* controller reset */
		ky_i2c_controller_reset(ky_i2c);

		/* reinit ky i2c slave */
		ky_i2c_write_reg(ky_i2c, REG_CR, KY_I2C_SLAVE_CRINIT);
		return;
	}

	/* slave address detected */
	if (status & SR_SAD) {
		/* read or write request */
		if (status & SR_RWM) {
			i2c_slave_event(ky_i2c->slave, I2C_SLAVE_READ_REQUESTED, &value);
			ky_i2c_write_reg(ky_i2c, REG_DBR, value & 0xff);
		} else {
			i2c_slave_event(ky_i2c->slave, I2C_SLAVE_WRITE_REQUESTED, &value);
		}
		ky_i2c_write_reg(ky_i2c, REG_CR, CR_TB | ky_i2c_read_reg(ky_i2c, REG_CR));
	} else if (status & SR_SSD) { /* stop detect */
		i2c_slave_event(ky_i2c->slave, I2C_SLAVE_STOP, &value);
		ky_i2c_write_reg(ky_i2c, REG_SR, SR_SSD);
	} else if (status & SR_IRF) { /* master write to us */
		ky_i2c_write_reg(ky_i2c, REG_SR, SR_IRF);

		value = ky_i2c_read_reg(ky_i2c, REG_DBR);
		ky_i2c_write_reg(ky_i2c, REG_CR, CR_TB | ky_i2c_read_reg(ky_i2c, REG_CR));

		i2c_slave_event(ky_i2c->slave, I2C_SLAVE_WRITE_RECEIVED, &value);
	} else if (status & SR_ITE) { /* ITE tx empty */
		ky_i2c_write_reg(ky_i2c, REG_SR, SR_ITE);

		i2c_slave_event(ky_i2c->slave, I2C_SLAVE_READ_PROCESSED, &value);
		ky_i2c_write_reg(ky_i2c, REG_DBR, value & 0xff);

		ky_i2c_write_reg(ky_i2c, REG_CR, CR_TB | ky_i2c_read_reg(ky_i2c, REG_CR));
	} else
		dev_err(ky_i2c->dev,"unknown slave status 0x%x\n", status);

	return;
}
#endif

static irqreturn_t ky_i2c_int_handler(int irq, void *devid)
{
	struct ky_i2c_dev *ky_i2c = devid;
	u32 status, ctrl;
	int ret = 0;

	/* record i2c status */
	status = ky_i2c_read_reg(ky_i2c, REG_SR);
	ky_i2c->i2c_status = status;

	/* check if a valid interrupt status */
	if(!status) {
		/* nothing need be done */
		return IRQ_HANDLED;
	}

#ifdef CONFIG_I2C_SLAVE
	if (ky_i2c->slave) {
		ky_i2c_slave_handler(ky_i2c);
		return IRQ_HANDLED;
	}
#endif

	/* bus error, rx overrun, arbitration lost */
	ky_i2c->i2c_err = status & (SR_BED | SR_RXOV | SR_ALD);

	/* clear interrupt status bits[31:18]*/
	ky_i2c_clear_int_status(ky_i2c, status);

	/* i2c error happens */
	if (unlikely(ky_i2c->i2c_err))
		goto err_out;

	/* process interrupt mode */
	if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_INTERRUPT))
		ret = ky_i2c_byte_xfer(ky_i2c);

err_out:
	/*
	 * send transaction complete signal:
	 * error happens, detect master stop
	 */
	if (likely(ky_i2c->i2c_err || (ret < 0) || (status & SR_MSD))) {
		/*
		 * Here the transaction is already done, we don't need any
		 * other interrupt signals from now, in case any interrupt
		 * happens before ky_i2c_xfer to disable irq and i2c unit,
		 * we mask all the interrupt signals and clear the interrupt
		 * status.
		*/
		ctrl = ky_i2c_read_reg(ky_i2c, REG_CR);
		ctrl &= ~KY_I2C_INT_CTRL_MASK;
		ky_i2c_write_reg(ky_i2c, REG_CR, ctrl);

		ky_i2c_clear_int_status(ky_i2c, KY_I2C_INT_STATUS_MASK);

		complete(&ky_i2c->complete);
	}

	return IRQ_HANDLED;
}

static void ky_i2c_choose_xfer_mode(struct ky_i2c_dev *ky_i2c)
{
	unsigned long timeout;
	int idx = 0, cnt = 0, freq;
	bool block = false;

	/* scan msgs */
	if (ky_i2c->high_mode)
		cnt++;
	ky_i2c->rx_total = 0;
	while (idx < ky_i2c->num) {
		cnt += (ky_i2c->msgs + idx)->len + 1;
		if ((ky_i2c->msgs + idx)->flags & I2C_M_RD)
			ky_i2c->rx_total += (ky_i2c->msgs + idx)->len;

		/*
		 * Some SMBus transactions require that
		 * we receive the transacttion length as the first read byte.
		 * force to use I2C_MODE_INTERRUPT
		 */
		if ((ky_i2c->msgs + idx)->flags & I2C_M_RECV_LEN) {
			block = true;
			cnt += I2C_SMBUS_BLOCK_MAX + 2;
		}
		idx++;
	}

	if (likely(ky_i2c->dma_disable) || block) {
		ky_i2c->xfer_mode = KY_I2C_MODE_INTERRUPT;

#ifdef CONFIG_DEBUG_FS
	} else if (unlikely(ky_i2c->dbgfs_mode != KY_I2C_MODE_INVALID)) {
		ky_i2c->xfer_mode = ky_i2c->dbgfs_mode;
		if (cnt > KY_I2C_TX_FIFO_DEPTH &&
				ky_i2c->xfer_mode == KY_I2C_MODE_FIFO)
			ky_i2c->xfer_mode = KY_I2C_MODE_DMA;

		/* flush fifo buffer */
		ky_i2c_flush_fifo_buffer(ky_i2c);
#endif
	} else {
		if (likely(cnt <= KY_I2C_TX_FIFO_DEPTH))
			ky_i2c->xfer_mode = KY_I2C_MODE_FIFO;
		else
			ky_i2c->xfer_mode = KY_I2C_MODE_DMA;

		/* flush fifo buffer */
		ky_i2c_flush_fifo_buffer(ky_i2c);
	}

	/*
	 * if total message length is too large to over the allocated MDA
	 * total buf length, use interrupt mode. This may happens in the
	 * syzkaller test.
	 */
	if (unlikely(cnt > (KY_I2C_MAX_MSG_LEN * KY_I2C_SCATTERLIST_SIZE) ||
		ky_i2c->rx_total > KY_I2C_DMA_RX_BUF_LEN))
		ky_i2c->xfer_mode = KY_I2C_MODE_INTERRUPT;

	/* calculate timeout */
	if (likely(ky_i2c->high_mode))
		freq = 1500000;
	else if (likely(ky_i2c->fast_mode))
		freq = 400000;
	else
		freq = 100000;

	timeout = cnt * 9 * USEC_PER_SEC / freq;

	if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_INTERRUPT ||
		ky_i2c->xfer_mode == KY_I2C_MODE_PIO))
		timeout += (cnt - 1) * 220;

	if (ky_i2c->xfer_mode == KY_I2C_MODE_INTERRUPT)
		ky_i2c->timeout = usecs_to_jiffies(timeout + 500000);
	else
		ky_i2c->timeout = usecs_to_jiffies(timeout + 100000);
}

static void ky_i2c_init_xfer_params(struct ky_i2c_dev *ky_i2c)
{
	/* initialize transfer parameters */
	ky_i2c->msg_idx = 0;
	ky_i2c->cur_msg = ky_i2c->msgs;
	ky_i2c->msg_buf = ky_i2c->cur_msg->buf;
	ky_i2c->rx_cnt = 0;
	ky_i2c->tx_cnt = 0;
	ky_i2c->i2c_err = 0;
	ky_i2c->i2c_status = 0;
	ky_i2c->phase = KY_I2C_XFER_IDLE;

	/* only send master code once for high speed mode */
	ky_i2c->is_xfer_start = true;
}

static int ky_i2c_pio_xfer(struct ky_i2c_dev *ky_i2c)
{
	int ret = 0, xfer_try = 0;
	u32 status;
	signed long timeout;

xfer_retry:
	/* calculate timeout */
	ky_i2c_choose_xfer_mode(ky_i2c);
	ky_i2c->xfer_mode = KY_I2C_MODE_PIO;
	timeout = jiffies_to_usecs(ky_i2c->timeout);

	if (!ky_i2c->clk_always_on)
		clk_enable(ky_i2c->clk);
	ky_i2c_controller_reset(ky_i2c);
	udelay(2);

	ky_i2c_unit_init(ky_i2c);

	ky_i2c_clear_int_status(ky_i2c, KY_I2C_INT_STATUS_MASK);

	ky_i2c_init_xfer_params(ky_i2c);

	ky_i2c_mark_rw_flag(ky_i2c);

	ky_i2c_enable(ky_i2c);

	ret = ky_i2c_byte_xfer(ky_i2c);
	if (unlikely(ret < 0)) {
		ret = -EINVAL;
		goto out;
	}

	while (ky_i2c->num > 0 && timeout > 0) {
		status = ky_i2c_read_reg(ky_i2c, REG_SR);
		ky_i2c_clear_int_status(ky_i2c, status);
		ky_i2c->i2c_status = status;

		/* bus error, arbitration lost */
		ky_i2c->i2c_err = status & (SR_BED | SR_ALD);
		if (unlikely(ky_i2c->i2c_err)) {
			ret = -1;
			break;
		}

		/* receive full */
		if (likely(status & SR_IRF)) {
			ret = ky_i2c_byte_xfer(ky_i2c);
			if (unlikely(ret < 0))
				break;
		}

		/* transmit empty */
		if (likely(status & SR_ITE)) {
			ret = ky_i2c_byte_xfer(ky_i2c);
			if (unlikely(ret < 0))
				break;
		}

		/* transaction done */
		if (likely(status & SR_MSD))
			break;

		udelay(10);
		timeout -= 10;
	}

	ky_i2c_disable(ky_i2c);

	if (!ky_i2c->clk_always_on)
		clk_disable(ky_i2c->clk);

	if (unlikely(timeout <= 0)) {
		dev_alert(ky_i2c->dev, "i2c pio transfer timeout\n");
		ky_i2c_print_msg_info(ky_i2c);
		ky_i2c_bus_reset(ky_i2c);
		udelay(100);
		ret = -ETIMEDOUT;
		goto out;
	}

	/* process i2c error */
	if (unlikely(ky_i2c->i2c_err)) {
		dev_dbg(ky_i2c->dev, "i2c pio error status: 0x%08x\n",
			ky_i2c->i2c_status);
		ky_i2c_print_msg_info(ky_i2c);

		/* try transfer again */
		if (ky_i2c->i2c_err & SR_ALD)
			ret = -EAGAIN;
		else
			ret = (ky_i2c->i2c_status & SR_ACKNAK) ? -ENXIO : -EIO;
	}

out:
	xfer_try++;
	/* retry i2c transfer 3 times for timeout and bus busy */
	if (unlikely((ret == -ETIMEDOUT || ret == -EAGAIN) &&
		xfer_try <= ky_i2c->drv_retries)) {
		dev_alert(ky_i2c->dev, "i2c pio retry %d, ret %d err 0x%x\n",
				xfer_try, ret, ky_i2c->i2c_err);
		udelay(150);
		ret = 0;
		goto xfer_retry;
	}

	return ret < 0 ? ret : ky_i2c->num;
}

static bool ky_i2c_restart_notify = false;
static bool ky_i2c_poweroff_notify = false;
struct sys_off_handler *i2c_poweroff_handler;

static int
ky_i2c_notifier_reboot_call(struct notifier_block *nb, unsigned long action, void *data)
{
	ky_i2c_restart_notify = true;
	return 0;
}

static int ky_i2c_notifier_poweroff_call(struct sys_off_data *data)
{
	ky_i2c_poweroff_notify = true;

	return NOTIFY_DONE;
}

static struct notifier_block ky_i2c_sys_nb = {
	.notifier_call  = ky_i2c_notifier_reboot_call,
	.priority   = 0,
};

static int
ky_i2c_xfer(struct i2c_adapter *adapt, struct i2c_msg msgs[], int num)
{
	struct ky_i2c_dev *ky_i2c = i2c_get_adapdata(adapt);
	int ret = 0, xfer_try = 0;
	unsigned long time_left;
	bool clk_directly = false;

#ifdef CONFIG_I2C_SLAVE
	if (ky_i2c->slave) {
		dev_err(ky_i2c->dev, "working as slave mode here\n");
		return -EBUSY;
	}
#endif

	/*
	 * at the end of system power off sequence, system will send
	 * software power down command to pmic via i2c interface
	 * with local irq disabled, so just enter PIO mode at once
	*/
	if (unlikely(ky_i2c_restart_notify == true ||
			ky_i2c_poweroff_notify == true
#ifdef CONFIG_DEBUG_FS
		|| ky_i2c->dbgfs_mode == KY_I2C_MODE_PIO
#endif
		)) {

		ky_i2c->msgs = msgs;
		ky_i2c->num = num;

		return ky_i2c_pio_xfer(ky_i2c);
	}

	mutex_lock(&ky_i2c->mtx);
	ky_i2c->msgs = msgs;
	ky_i2c->num = num;

	if (ky_i2c->shutdown) {
		mutex_unlock(&ky_i2c->mtx);
		return -ENXIO;
	}

	if (!ky_i2c->clk_always_on) {
		ret = pm_runtime_get_sync(ky_i2c->dev);
		if (unlikely(ret < 0)) {
			/*
			 * during system suspend_late to system resume_early stage,
			 * if PM runtime is suspended, we will get -EACCES return
			 * value, so we need to enable clock directly, and disable after
			 * i2c transfer is finished, if PM runtime is active, it will
			 * work normally. During this stage, pmic onkey ISR that
			 * invoked in an irq thread may use i2c interface if we have
			 * onkey press action
			 */
			if (likely(ret == -EACCES)) {
				clk_directly = true;
				clk_enable(ky_i2c->clk);
			} else {
				dev_err(ky_i2c->dev, "pm runtime sync error: %d\n",
					ret);
				goto err_runtime;
			}
		}
	}

xfer_retry:
	/* if unit keeps the last control status, don't need to do reset */
	if (unlikely(ky_i2c_read_reg(ky_i2c, REG_CR) != ky_i2c->i2c_ctrl_reg_value))
		/* i2c controller & bus reset */
		ky_i2c_reset(ky_i2c);

	/* choose transfer mode */
	ky_i2c_choose_xfer_mode(ky_i2c);

	/* i2c unit init */
	ky_i2c_unit_init(ky_i2c);

	/* clear all interrupt status */
	ky_i2c_clear_int_status(ky_i2c, KY_I2C_INT_STATUS_MASK);

	ky_i2c_init_xfer_params(ky_i2c);

	ky_i2c_mark_rw_flag(ky_i2c);

	reinit_completion(&ky_i2c->complete);

	ky_i2c_enable(ky_i2c);

	/* i2c wait for bus busy */
	ret = ky_i2c_recover_bus_busy(ky_i2c);
	if (unlikely(ret))
		goto timeout_xfex;

	/* i2c msg transmit */
	if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_INTERRUPT))
		ret = ky_i2c_byte_xfer(ky_i2c);
	else if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_FIFO))
		ret = ky_i2c_fifo_xfer(ky_i2c);
	else
		ret = ky_i2c_dma_xfer(ky_i2c);

	if (unlikely(ret < 0)) {
		dev_dbg(ky_i2c->dev, "i2c transfer error\n");
		/* timeout error should not be overrided, and the transfer
		 * error will be confirmed by err handle function latter,
		 * the reset should be invalid argument error. */
		if (ret != -ETIMEDOUT)
			ret = -EINVAL;
		goto err_xfer;
	}

	if (likely(ky_i2c->xfer_mode == KY_I2C_MODE_INTERRUPT)) {
		time_left = wait_for_completion_timeout(&ky_i2c->complete,
							ky_i2c->timeout);
		if (unlikely(time_left == 0)) {
			dev_alert(ky_i2c->dev, "msg completion timeout\n");
			ky_i2c_bus_reset(ky_i2c);
			ky_i2c_reset(ky_i2c);
			ret = -ETIMEDOUT;
			goto timeout_xfex;
		}
	}

err_xfer:
	if (likely(!ret))
		ky_i2c_check_bus_release(ky_i2c);

timeout_xfex:
	/* disable ky i2c */
	ky_i2c_disable(ky_i2c);

	/* print more message info when error or timeout happens */
	if (unlikely(ret < 0 || ky_i2c->i2c_err))
		ky_i2c_print_msg_info(ky_i2c);

	/* process i2c error */
	if (unlikely(ky_i2c->i2c_err))
		ret = ky_i2c_handle_err(ky_i2c);

	xfer_try++;
	/* retry i2c transfer 3 times for timeout and bus busy */
	if (unlikely((ret == -ETIMEDOUT || ret == -EAGAIN) &&
		xfer_try <= ky_i2c->drv_retries)) {
		dev_alert(ky_i2c->dev, "i2c transfer retry %d, ret %d mode %d err 0x%x\n",
				xfer_try, ret, ky_i2c->xfer_mode, ky_i2c->i2c_err);
		usleep_range(150, 200);
		ret = 0;
		goto xfer_retry;
	}

err_runtime:
	if (unlikely(clk_directly)) {
		/* if clock is enabled directly, here disable it */
		clk_disable(ky_i2c->clk);
	}

	if (!ky_i2c->clk_always_on) {
		pm_runtime_mark_last_busy(ky_i2c->dev);
		pm_runtime_put_autosuspend(ky_i2c->dev);
	}

	mutex_unlock(&ky_i2c->mtx);

	return ret < 0 ? ret : num;
}

static int ky_i2c_prepare_dma(struct ky_i2c_dev *ky_i2c)
{
	int ret = 0;
	struct dma_slave_config *rx_cfg = &ky_i2c->rx_dma_cfg;
	struct dma_slave_config *tx_cfg = &ky_i2c->tx_dma_cfg;

	if (ky_i2c->dma_disable)
		return 0;

	/* request dma channels */
	ky_i2c->rx_dma = dma_request_slave_channel(ky_i2c->dev, "rx");
	if (IS_ERR_OR_NULL(ky_i2c->rx_dma)) {
		ret = -1;
		dev_err(ky_i2c->dev, "failed to request rx dma channel\n");
		return ret;
	}

	ky_i2c->tx_dma = dma_request_slave_channel(ky_i2c->dev, "tx");
	if (IS_ERR_OR_NULL(ky_i2c->tx_dma)) {
		ret = -1;
		dev_err(ky_i2c->dev, "failed to request tx dma channel\n");
		goto err_rxch;
	}

	rx_cfg->direction = DMA_DEV_TO_MEM;
	rx_cfg->src_addr = ky_i2c->resrc.start + REG_RFIFO;
	rx_cfg->device_fc = true;
	rx_cfg->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_cfg->src_maxburst = KY_I2C_RX_FIFO_DEPTH * 1;

	ret = dmaengine_slave_config(ky_i2c->rx_dma, rx_cfg);
	if (ret) {
		dev_err(ky_i2c->dev, "failed to config rx channel\n");
		goto err_txch;
	}

	tx_cfg->direction = DMA_MEM_TO_DEV;
	tx_cfg->dst_addr = ky_i2c->resrc.start + REG_WFIFO;
	tx_cfg->device_fc = true;
	tx_cfg->dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	tx_cfg->dst_maxburst = KY_I2C_TX_FIFO_DEPTH * 1;

	ret = dmaengine_slave_config(ky_i2c->tx_dma, tx_cfg);
	if (ret) {
		dev_err(ky_i2c->dev, "failed to config tx channel\n");
		goto err_txch;
	}

	/* allocate scatter lists */
	ky_i2c->rx_sg = devm_kmalloc(ky_i2c->dev,
			sizeof(*ky_i2c->rx_sg) * KY_I2C_SCATTERLIST_SIZE,
			GFP_KERNEL);
	if (!ky_i2c->rx_sg) {
		ret = -ENOMEM;
		dev_err(ky_i2c->dev,
			"failed to allocate memory for rx scatterlist\n");
		goto err_txch;
	}
	sg_init_table(ky_i2c->rx_sg, KY_I2C_SCATTERLIST_SIZE);

	ky_i2c->tx_sg = devm_kmalloc(ky_i2c->dev,
				sizeof(*ky_i2c->tx_sg),
				GFP_KERNEL);
	if (!ky_i2c->tx_sg) {
		ret = -ENOMEM;
		dev_err(ky_i2c->dev,
			"failed to allocate memory for tx scatterlist\n");
		goto err_txch;
	}
	sg_init_table(ky_i2c->tx_sg, 1);

	/* allocate memory for tx */
	ky_i2c->tx_dma_buf = devm_kzalloc(ky_i2c->dev,
			sizeof(ky_i2c->tx_dma_buf[0]) * KY_I2C_DMA_TX_BUF_LEN,
			GFP_KERNEL);
	if (!ky_i2c->tx_dma_buf) {
		ret = -ENOMEM;
		dev_err(ky_i2c->dev,
			"failed to allocate memory for tx dma buffer\n");
		goto err_txch;
	}

	/* allocate memory for rx */
	ky_i2c->rx_dma_buf = devm_kzalloc(ky_i2c->dev,
			sizeof(ky_i2c->rx_dma_buf[0]) * KY_I2C_DMA_RX_BUF_LEN,
			GFP_KERNEL);
	if (!ky_i2c->rx_dma_buf) {
		ret = -ENOMEM;
		dev_err(ky_i2c->dev,
			"failed to allocate memory for rx dma buffer\n");
		goto err_txch;
	}

	/*
	 * DMA controller can access all 4G or higher 4G address space, set
	 * dma mask will avoid to use swiotlb, that will improve performance
	 * and also avoid panic if swiotlb is not initialized.
	 * Besides, device's coherent_dma_mask is set as DMA_BIT_MASK(32)
	 * in initialization, see of_dma_configure().
	 */
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	dma_set_mask(ky_i2c->dev, DMA_BIT_MASK(64));
#else
	dma_set_mask(ky_i2c->dev, ky_i2c->dev->coherent_dma_mask);
#endif

	return 0;

err_txch:
	dma_release_channel(ky_i2c->tx_dma);
err_rxch:
	dma_release_channel(ky_i2c->rx_dma);
	return ret;
}

static int ky_i2c_release_dma(struct ky_i2c_dev *ky_i2c)
{
	if (ky_i2c->dma_disable)
		return 0;

	if (!IS_ERR_OR_NULL(ky_i2c->rx_dma))
		dma_release_channel(ky_i2c->rx_dma);

	if (!IS_ERR_OR_NULL(ky_i2c->tx_dma))
		dma_release_channel(ky_i2c->tx_dma);

	return 0;
}

#ifdef CONFIG_DEBUG_FS

static ssize_t
ky_i2c_dbgfs_read(struct file *filp, char __user *user_buf,
	size_t size, loff_t *ppos)
{
	struct ky_i2c_dev *ky_i2c = filp->private_data;
	char buf[64];
	int ret, n, copy;

	n = min(sizeof(buf) - 1, size);
	switch (ky_i2c->xfer_mode) {
	case KY_I2C_MODE_INTERRUPT:
		copy = sprintf(buf, "%s: interrupt mode\n",
				ky_i2c->dbgfs_name);
		break;
	case KY_I2C_MODE_FIFO:
		copy = sprintf(buf, "%s: fifo mode\n", ky_i2c->dbgfs_name);
		break;
	case KY_I2C_MODE_DMA:
		copy = sprintf(buf, "%s: dma mode\n", ky_i2c->dbgfs_name);
		break;
	case KY_I2C_MODE_PIO:
		copy = sprintf(buf, "%s: pio mode\n", ky_i2c->dbgfs_name);
		break;
	default:
		copy = sprintf(buf, "%s: mode is invalid\n",
				ky_i2c->dbgfs_name);
		break;
	}

	copy  = min(n, copy);
	ret = simple_read_from_buffer(user_buf, size, ppos, buf, copy);

	return ret;
}

static ssize_t
ky_i2c_dbgfs_write(struct file *filp, const char __user *user_buf,
	size_t size, loff_t *ppos)
{
	struct ky_i2c_dev *ky_i2c = filp->private_data;
	char buf[32];
	int buf_size, i = 0;

	buf_size = min(size, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	*(buf + buf_size) = '\0';
	while (*(buf + i) != '\n' && *(buf + i) != '\0')
		i++;
	*(buf + i) = '\0';

	i = 0;
	while (*(buf + i) == ' ')
		i++;

	if (!strncmp(buf + i, "pio", 3)) {
		ky_i2c->dbgfs_mode = KY_I2C_MODE_PIO;
	} else if (!strncmp(buf + i, "interrupt", 9)) {
		ky_i2c->dbgfs_mode = KY_I2C_MODE_INTERRUPT;
	} else if (!strncmp(buf + i, "fifo", 4)) {
		if (!ky_i2c->dma_disable)
			ky_i2c->dbgfs_mode = KY_I2C_MODE_FIFO;
		else
			goto err_out;
	} else if (!strncmp(buf + i, "dma", 3)) {
		if (!ky_i2c->dma_disable)
			ky_i2c->dbgfs_mode = KY_I2C_MODE_DMA;
		else
			goto err_out;
	} else {
		if (!ky_i2c->dma_disable)
			dev_err(ky_i2c->dev,
				"only accept: interrupt, fifo, dma, pio\n");
		else
			goto err_out;
	}

	return size;

err_out:
	ky_i2c->dbgfs_mode = KY_I2C_MODE_INTERRUPT;
	dev_err(ky_i2c->dev,
		"dma is disabled, only accept: interrupt, pio\n");
	return size;
}

static const struct file_operations ky_i2c_dbgfs_ops = {
	.open	= simple_open,
	.read	= ky_i2c_dbgfs_read,
	.write	= ky_i2c_dbgfs_write,
};
#endif /* CONFIG_DEBUG_FS */

#ifdef CONFIG_PM_SLEEP
/** static int ky_i2c_suspend(struct device *dev)
 * {
 *	struct ky_i2c_dev *ky_i2c = dev_get_drvdata(dev);
 *
 *	dev_dbg(ky_i2c->dev, "system suspend\n");
 *
 *	if (ky_i2c->clk_always_on)
 *		return 0;
 *
 *	// grab mutex to make sure the i2c transaction is over
 *	mutex_lock(&ky_i2c->mtx);
 *	if (!pm_runtime_status_suspended(dev)) {
 *		 // sync runtime pm and system pm states:
 *		 // prevent runtime pm suspend callback from being re-invoked
 *		pm_runtime_disable(dev);
 *		pm_runtime_set_suspended(dev);
 *		pm_runtime_enable(dev);
 *	}
 *	mutex_unlock(&ky_i2c->mtx);
 *
 *	return 0;
 * }
 *
 * static int ky_i2c_resume(struct device *dev)
 * {
 *	struct ky_i2c_dev *ky_i2c = dev_get_drvdata(dev);
 *
 *	dev_dbg(ky_i2c->dev, "system resume\n");
 *
 *	return 0;
 *}
 */
#endif /* CONFIG_PM_SLEEP */

/**
 * static const struct dev_pm_ops ky_i2c_pm_ops = {
 *	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(ky_i2c_suspend,
 *			ky_i2c_resume)
 *};
 */

static u32 ky_i2c_func(struct i2c_adapter *adap)
{
#ifdef CONFIG_I2C_SLAVE
	return I2C_FUNC_I2C | I2C_FUNC_SLAVE |
		(I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
#else
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
#endif
}

#ifdef CONFIG_I2C_SLAVE
static int ky_i2c_reg_slave(struct i2c_client *slave)
{
	struct ky_i2c_dev *ky_i2c = i2c_get_adapdata(slave->adapter);
	int ret = 0;

	if (ky_i2c->slave)
		return -EBUSY;

	if (slave->flags & I2C_CLIENT_TEN)
		return -EAFNOSUPPORT;

	if(!slave->addr) {
		dev_err(ky_i2c->dev, "have no slave address\n");
		return -EAFNOSUPPORT;
	}

	/* Keep device active for slave address detection logic */
	if (!ky_i2c->clk_always_on) {
		ret = pm_runtime_get_sync(ky_i2c->dev);
		if(unlikely(ret < 0)) {
			return ret;
		}
	}

	ky_i2c->slave = slave;

	ky_i2c_write_reg(ky_i2c, REG_SAR, slave->addr);
	ky_i2c_write_reg(ky_i2c, REG_CR, KY_I2C_SLAVE_CRINIT);

	return 0;
}

static int ky_i2c_unreg_slave(struct i2c_client *slave)
{
	struct ky_i2c_dev *ky_i2c = i2c_get_adapdata(slave->adapter);

	WARN_ON(!ky_i2c->slave);

	ky_i2c_write_reg(ky_i2c, REG_CR, 0);
	/* clear slave address */
	ky_i2c_write_reg(ky_i2c, REG_SAR, 0);

	if (!ky_i2c->clk_always_on)
		pm_runtime_put(ky_i2c->dev);

	ky_i2c->slave = NULL;

	return 0;
}
#endif

static const struct i2c_algorithm ky_i2c_algrtm = {
	.master_xfer	= ky_i2c_xfer,
	.functionality	= ky_i2c_func,
#ifdef CONFIG_I2C_SLAVE
	.reg_slave	= ky_i2c_reg_slave,
	.unreg_slave	= ky_i2c_unreg_slave,
#endif
};

/* i2c message limitation for DMA mode */
static struct i2c_adapter_quirks ky_i2c_quirks = {
	.max_num_msgs	= KY_I2C_SCATTERLIST_SIZE,
	.max_write_len	= KY_I2C_MAX_MSG_LEN,
	.max_read_len	= KY_I2C_MAX_MSG_LEN,
};

static int
ky_i2c_parse_dt(struct platform_device *pdev, struct ky_i2c_dev *ky_i2c)
{
	struct device_node *dnode = pdev->dev.of_node;
	int ret;

	/* enable fast speed mode */
	ky_i2c->fast_mode = of_property_read_bool(dnode, "ky,i2c-fast-mode");

	/* enable high speed mode */
	ky_i2c->high_mode = of_property_read_bool(dnode, "ky,i2c-high-mode");
	if (ky_i2c->high_mode) {
		/* get master code for high speed mode */
		ret = of_property_read_u8(dnode, "ky,i2c-master-code",
				&ky_i2c->master_code);
		if (ret) {
			ky_i2c->master_code = 0x0e;
			dev_warn(ky_i2c->dev,
			"failed to get i2c master code, use default: 0x0e\n");
		}
	}
	ret = of_property_read_u32(dnode, "ky,i2c-clk-rate",
			&ky_i2c->clk_rate);
	if (ret) {
		dev_err(ky_i2c->dev,
			"failed to get i2c clock rate\n");
		return ret;
	}

	ret = of_property_read_u32(dnode, "ky,i2c-lcr", &ky_i2c->i2c_lcr);
	if (ret) {
		dev_err(ky_i2c->dev, "failed to get i2c lcr\n");
		return ret;
	}

	ret = of_property_read_u32(dnode, "ky,i2c-wcr", &ky_i2c->i2c_wcr);
	if (ret) {
		dev_err(ky_i2c->dev, "failed to get i2c wcr\n");
		return ret;
	}

	/*
	 * adapter device id:
	 * assigned in dt node or alias name, or automatically allocated
	 * in i2c_add_numbered_adapter()
	 */
	ret = of_property_read_u32(dnode, "ky,adapter-id", &pdev->id);
	if (ret)
		pdev->id = -1;

	/* disable DMA transfer mode */
	ky_i2c->dma_disable = of_property_read_bool(dnode, "ky,dma-disable");

	/* default: interrupt mode */
	if (ky_i2c->dma_disable)
		ky_i2c->xfer_mode = KY_I2C_MODE_INTERRUPT;
	else
		ky_i2c->xfer_mode = KY_I2C_MODE_DMA;

	/* true: the clock will always on and not use runtime mechanism */
	ky_i2c->clk_always_on = of_property_read_bool(dnode, "ky,clk-always-on");

	/* apb clock: 26MHz or 52MHz */
	ret = of_property_read_u32(dnode, "ky,apb_clock", &ky_i2c->apb_clock);
	if (ret) {
		dev_err(ky_i2c->dev, "failed to get apb clock\n");
		return ret;
	} else if ((ky_i2c->apb_clock != KY_I2C_APB_CLOCK_26M) &&
			(ky_i2c->apb_clock != KY_I2C_APB_CLOCK_52M)) {
		dev_err(ky_i2c->dev, "the apb clock should be 26M or 52M\n");
		return -EINVAL;
	}

	return 0;
}

static int ky_i2c_probe(struct platform_device *pdev)
{
	struct ky_i2c_dev *ky_i2c;
	struct device_node *dnode = pdev->dev.of_node;
#ifdef CONFIG_SOC_KY_X1
	struct rpmsg_device *rpdev;
	struct instance_data *idata;
	const struct of_device_id *of_id;
	bool rcpu_i2c = false;
#endif
	int ret = 0;

	/* allocate memory */
	ky_i2c = devm_kzalloc(&pdev->dev,
				sizeof(struct ky_i2c_dev),
				GFP_KERNEL);
	if (!ky_i2c) {
		ret =  -ENOMEM;
		goto err_out;
	}

	ky_i2c->dev = &pdev->dev;
	platform_set_drvdata(pdev, ky_i2c);
	mutex_init(&ky_i2c->mtx);

	ky_i2c->resets = devm_reset_control_get_optional(&pdev->dev, NULL);
	if(IS_ERR(ky_i2c->resets)) {
		dev_err(&pdev->dev, "failed to get resets\n");
		goto err_out;
	}
	/* reset the i2c controller */
	reset_control_assert(ky_i2c->resets);
	udelay(200);
	reset_control_deassert(ky_i2c->resets);

	ret = ky_i2c_parse_dt(pdev, ky_i2c);
	if (ret)
		goto err_out;

	ret = of_address_to_resource(dnode, 0, &ky_i2c->resrc);
	if (ret) {
		dev_err(&pdev->dev, "failed to get resource\n");
		ret =  -ENODEV;
		goto err_out;
	}

	ky_i2c->mapbase = devm_ioremap_resource(ky_i2c->dev, &ky_i2c->resrc);
	if (IS_ERR(ky_i2c->mapbase)) {
		dev_err(&pdev->dev, "failed to do ioremap\n");
		ret =  PTR_ERR(ky_i2c->mapbase);
		goto err_out;
	}

#ifdef CONFIG_SOC_KY_X1
	if (of_get_property(pdev->dev.of_node, "rcpu-i2c", NULL)) {
		rcpu_i2c = true;
		of_id = of_match_device(r_ky_i2c_dt_match, &pdev->dev);
		if (!of_id) {
			pr_err("Unable to match OF ID\n");
			return -ENODEV;
		}

		idata = (struct instance_data *)((unsigned long long *)(of_id->data))[0];
		rpdev = idata->rpdev;
		idata->ky_i2c = ky_i2c;

		ret = rpmsg_send(rpdev->ept, STARTUP_MSG, strlen(STARTUP_MSG));
		if (ret) {
			dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
			return ret;
		}
	} else
#endif
	{
		ky_i2c->irq = platform_get_irq(pdev, 0);
		if (ky_i2c->irq < 0) {
			dev_err(ky_i2c->dev, "failed to get irq resource\n");
			ret = ky_i2c->irq;
			goto err_out;
		}

		ret = devm_request_irq(ky_i2c->dev, ky_i2c->irq, ky_i2c_int_handler,
				IRQF_NO_SUSPEND,
				dev_name(ky_i2c->dev), ky_i2c);
		if (ret) {
			dev_err(ky_i2c->dev, "failed to request irq\n");
			goto err_out;
		}
	}

	ret = ky_i2c_prepare_dma(ky_i2c);
	if (ret) {
		dev_err(&pdev->dev, "failed to request dma channels\n");
		goto err_out;
	}

	ky_i2c->clk = devm_clk_get(ky_i2c->dev, NULL);
	if (IS_ERR(ky_i2c->clk)) {
		dev_err(ky_i2c->dev, "failed to get clock\n");
		ret = PTR_ERR(ky_i2c->clk);
		goto err_dma;
	}
#ifdef CONFIG_SOC_KY_X1
	if (rcpu_i2c) {
		clk_set_rate(ky_i2c->clk, ky_i2c->clk_rate);
	}
#endif
	clk_prepare_enable(ky_i2c->clk);

	i2c_set_adapdata(&ky_i2c->adapt, ky_i2c);
	ky_i2c->adapt.owner = THIS_MODULE;
	ky_i2c->adapt.algo = &ky_i2c_algrtm;
	ky_i2c->adapt.dev.parent = ky_i2c->dev;
	ky_i2c->adapt.nr = pdev->id;
	/* retries used by i2c framework: 3 times */
	ky_i2c->adapt.retries = 3;
	/*
	 * retries used by i2c driver: 3 times
	 * this is for the very low occasionally PMIC i2c access failure.
	 */
	ky_i2c->drv_retries = 3;
	ky_i2c->adapt.dev.of_node = dnode;
	ky_i2c->adapt.algo_data = ky_i2c;
	strlcpy(ky_i2c->adapt.name, "ky-i2c-adapter",
		sizeof(ky_i2c->adapt.name));

	if (!ky_i2c->dma_disable)
		ky_i2c->adapt.quirks = &ky_i2c_quirks;

	init_completion(&ky_i2c->complete);
	spin_lock_init(&ky_i2c->fifo_lock);

	if (!ky_i2c->clk_always_on) {
		pm_runtime_set_autosuspend_delay(ky_i2c->dev, MSEC_PER_SEC);
		pm_runtime_use_autosuspend(ky_i2c->dev);
		pm_runtime_set_active(ky_i2c->dev);
		pm_suspend_ignore_children(&pdev->dev, 1);
		pm_runtime_enable(ky_i2c->dev);
	} else
		dev_dbg(ky_i2c->dev, "clock keeps always on\n");

	ky_i2c->dbgfs_mode = KY_I2C_MODE_INVALID;
	ky_i2c->shutdown = false;
	ret = i2c_add_numbered_adapter(&ky_i2c->adapt);
	if (ret) {
		dev_err(ky_i2c->dev, "failed to add i2c adapter\n");
		goto err_clk;
	}

#ifdef CONFIG_DEBUG_FS
	snprintf(ky_i2c->dbgfs_name, sizeof(ky_i2c->dbgfs_name),
			"ky-i2c-%d", ky_i2c->adapt.nr);
	ky_i2c->dbgfs = debugfs_create_file(ky_i2c->dbgfs_name, 0644,
					NULL, ky_i2c, &ky_i2c_dbgfs_ops);
	if (!ky_i2c->dbgfs) {
		dev_err(ky_i2c->dev, "failed to create debugfs\n");
		ret = -ENOMEM;
		goto err_adapt;
	}
#endif

	dev_dbg(ky_i2c->dev, "driver probe success with dma %s\n",
		ky_i2c->dma_disable ? "disabled" : "enabled");
	return 0;

#ifdef CONFIG_DEBUG_FS
err_adapt:
	i2c_del_adapter(&ky_i2c->adapt);
#endif
err_clk:
	if (!ky_i2c->clk_always_on) {
		pm_runtime_disable(ky_i2c->dev);
		pm_runtime_set_suspended(ky_i2c->dev);
	}
	clk_disable_unprepare(ky_i2c->clk);
err_dma:
	ky_i2c_release_dma(ky_i2c);
err_out:
	return ret;
}

static int ky_i2c_remove(struct platform_device *pdev)
{
	struct ky_i2c_dev *ky_i2c = platform_get_drvdata(pdev);

	if (!ky_i2c->clk_always_on) {
		pm_runtime_disable(ky_i2c->dev);
		pm_runtime_set_suspended(ky_i2c->dev);
	}

	debugfs_remove_recursive(ky_i2c->dbgfs);
	i2c_del_adapter(&ky_i2c->adapt);

	mutex_destroy(&ky_i2c->mtx);

	reset_control_assert(ky_i2c->resets);

	ky_i2c_release_dma(ky_i2c);

	clk_disable_unprepare(ky_i2c->clk);

	dev_dbg(ky_i2c->dev, "driver removed\n");
	return 0;
}

static void ky_i2c_shutdown(struct platform_device *pdev)
{
	/**
	 * we should using i2c to communicate with pmic to shutdown the system
	 * so we should not shutdown i2c
	 */
/**
 *	struct ky_i2c_dev *ky_i2c = platform_get_drvdata(pdev);
 *
 *	mutex_lock(&ky_i2c->mtx);
 *	ky_i2c->shutdown = true;
 *	mutex_unlock(&ky_i2c->mtx);
 */
}

static const struct of_device_id ky_i2c_dt_match[] = {
	{
		.compatible = "ky,x1-i2c",
	},
	{}
};

MODULE_DEVICE_TABLE(of, ky_i2c_dt_match);

static struct platform_driver ky_i2c_driver = {
	.probe  = ky_i2c_probe,
	.remove = ky_i2c_remove,
	.shutdown = ky_i2c_shutdown,
	.driver = {
		.name		= "i2c-ky-x1",
		/* .pm             = &ky_i2c_pm_ops, */
		.of_match_table	= ky_i2c_dt_match,
	},
};

static int __init ky_i2c_init(void)
{
	register_restart_handler(&ky_i2c_sys_nb);
	i2c_poweroff_handler = register_sys_off_handler(SYS_OFF_MODE_POWER_OFF,
			SYS_OFF_PRIO_HIGH,
			ky_i2c_notifier_poweroff_call,
			NULL);

	return platform_driver_register(&ky_i2c_driver);
}

static void __exit ky_i2c_exit(void)
{
	platform_driver_unregister(&ky_i2c_driver);
	unregister_restart_handler(&ky_i2c_sys_nb);
	unregister_sys_off_handler(i2c_poweroff_handler);
}

subsys_initcall(ky_i2c_init);
module_exit(ky_i2c_exit);

#ifdef CONFIG_SOC_KY_X1
static const struct of_device_id r_ky_i2c_dt_match[] = {
	{ .compatible = "ky,x1-i2c-rcpu", .data =(void *)&private_data[0] },
	{}
};

MODULE_DEVICE_TABLE(of, r_ky_i2c_dt_match);

static struct platform_driver r_ky_i2c_driver = {
	.probe  = ky_i2c_probe,
	.remove = ky_i2c_remove,
	.shutdown = ky_i2c_shutdown,
	.driver = {
		.name		= "ri2c-ky-x1",
		/* .pm             = &ky_i2c_pm_ops, */
		.of_match_table	= r_ky_i2c_dt_match,
	},
};

static struct rpmsg_device_id rpmsg_driver_i2c_id_table[] = {
	{ .name	= "i2c-service", .driver_data = 0 },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_i2c_id_table);

static int rpmsg_i2c_client_probe(struct rpmsg_device *rpdev)
{
	struct instance_data *idata;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
					rpdev->src, rpdev->dst);

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	dev_set_drvdata(&rpdev->dev, idata);
	idata->rpdev = rpdev;

	((unsigned long long *)(r_ky_i2c_dt_match[0].data))[0] = (unsigned long long)idata;

	return platform_driver_register(&r_ky_i2c_driver);
}

static int rpmsg_i2c_client_cb(struct rpmsg_device *rpdev, void *data,
		int len, void *priv, u32 src)
{
	int ret;
	struct instance_data *idata = dev_get_drvdata(&rpdev->dev);
	struct ky_i2c_dev *ky_i2c = idata->ky_i2c;

	ky_i2c_int_handler(0, (void *)ky_i2c);

        ret = rpmsg_send(rpdev->ept, IRQUP_MSG, strlen(IRQUP_MSG));
        if (ret) {
                dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
                return ret;
        }

	return 0;
}

static void rpmsg_i2c_client_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg i2c client driver is removed\n");

	platform_driver_unregister(&r_ky_i2c_driver);
}

static struct rpmsg_driver rpmsg_i2c_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_i2c_id_table,
	.probe		= rpmsg_i2c_client_probe,
	.callback	= rpmsg_i2c_client_cb,
	.remove		= rpmsg_i2c_client_remove,
};
module_rpmsg_driver(rpmsg_i2c_client);
#endif

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:i2c-ky-x1");
