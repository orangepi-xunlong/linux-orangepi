/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <linux/delay.h>
#include "dw_dev.h"
#include "dw_mc.h"
#include "dw_cec.h"

#define DW_CEC_DATA_SIZE  16

int dw_cec_receive_msg(u8 *buf, u8 buf_size)
{
	u8 ret = 0x0, i = 0x0;

	/* get buffer lock */
	ret = dw_read_mask(CEC_LOCK, CEC_LOCK_LOCKED_BUFFER_MASK);
	if (!ret)
		return -1;

	ret = (u8)dw_read_mask(CEC_RX_CNT, CEC_RX_CNT_CEC_RX_CNT_MASK);
	ret = (ret > buf_size) ? buf_size : ret;
	if (ret > DW_CEC_DATA_SIZE)
		return -1;

	for (i = 0; i < ret; i++)
		*buf++ = (u8)dw_read(CEC_RX_DATA + (i));

	/* clear buffer lock */
	dw_write_mask(CEC_LOCK, CEC_LOCK_LOCKED_BUFFER_MASK, 0x0);
	return ret;
}

void dw_cec_send_msg(u8 *buf, u8 len, u8 type)
{
	int i = 0;

	if (len > DW_CEC_DATA_SIZE)
		return;

	for (i = 0; i < len; i++)
		dw_write(CEC_TX_DATA + i, *buf++);

	dw_write(CEC_TX_CNT, len);

	dw_write_mask(CEC_CTRL, CEC_CTRL_FRAME_TYP_MASK, type);

	dw_write_mask(CEC_CTRL, CEC_CTRL_SEND_MASK, CEC_CTRL_SEND_MASK);
}

void dw_cec_set_logical_addr(u16 addr)
{
	dw_write(CEC_ADDR_H, ((addr >> 8) & 0xFF));
	dw_write(CEC_ADDR_L, ((addr >> 0) & 0xFF));
}

unsigned int dw_cec_get_logic_addr(void)
{
	unsigned int reg, addr = 16, i;

	reg = dw_read(CEC_ADDR_H);
	reg = (reg << 8) | dw_read(CEC_ADDR_L);

	for (i = 0; i < 16; i++) {
		if ((reg >> i) & 0x01)
			addr = i;
	}

	return addr;
}

int dw_cec_set_enable(u8 enable)
{
	u8 mask = 0x0;
	if (enable == 0x1) {
		/* enable cec module clock */
		dw_mc_set_clk(DW_MC_CLK_CEC, DW_HDMI_ENABLE);

		udelay(20);

		/* clear control info */
		dw_write(CEC_CTRL, 0x0);

		/* clearr lock */
		dw_write_mask(CEC_LOCK, CEC_LOCK_LOCKED_BUFFER_MASK, 0x0);

		/* set logical addr */
		dw_cec_set_logical_addr(0x0);

		/* clear all interrupt state */
		dw_write(CEC_MASK, ~0);

		/* enable interrupt */
		mask |= CEC_MASK_DONE_MASK;
		mask |= CEC_MASK_EOM_MASK;
		mask |= CEC_MASK_NACK_MASK;
		mask |= CEC_MASK_ARB_LOST_MASK;
		mask |= CEC_MASK_ERROR_FLOW_MASK;
		mask |= CEC_MASK_ERROR_INITIATOR_MASK;
		dw_write(CEC_MASK, dw_read(CEC_MASK) & ~mask);

		/* unmute irq */
		dw_mc_irq_unmute(DW_MC_IRQ_CEC);
	} else {
		/* mask interrupt */
		mask |= CEC_MASK_DONE_MASK;
		mask |= CEC_MASK_EOM_MASK;
		mask |= CEC_MASK_NACK_MASK;
		mask |= CEC_MASK_ARB_LOST_MASK;
		mask |= CEC_MASK_ERROR_FLOW_MASK;
		mask |= CEC_MASK_ERROR_INITIATOR_MASK;
		dw_write(CEC_MASK, dw_read(CEC_MASK) | mask);

		/* mute irq */
		dw_mc_irq_mute(DW_MC_IRQ_CEC);

		udelay(20);
		/* disable cec module clock */
		dw_mc_set_clk(DW_MC_CLK_CEC, DW_HDMI_DISABLE);
	}
	return 0;
}
