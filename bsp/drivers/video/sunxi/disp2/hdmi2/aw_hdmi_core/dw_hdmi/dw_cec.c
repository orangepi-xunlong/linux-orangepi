/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/delay.h>
#include "dw_access.h"
#include "dw_mc.h"
#include "dw_cec.h"

extern u32 bHdmi_drv_enable;

/**
 * clear locked status
 * @param dev: address and device information
 * @return LOCKED status
 */
static int _dw_cec_clear_lock(dw_hdmi_dev_t *dev)
{
	dev_write(dev, CEC_LOCK, dev_read(dev, CEC_LOCK) &
			~CEC_LOCK_LOCKED_BUFFER_MASK);
	return true;
}

/**
 * Enable interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to enable
 * @return error code
 */
static void _dw_cec_interrupt_enable(dw_hdmi_dev_t *dev, unsigned char mask)
{
	dev_write(dev, CEC_MASK, dev_read(dev, CEC_MASK) & ~mask);
}

/**
 * Disable interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to disable
 * @return error code
 */
static void _dw_cec_interrupt_disable(dw_hdmi_dev_t *dev, unsigned char mask)
{
	dev_write(dev, CEC_MASK, dev_read(dev, CEC_MASK) | mask);
}

/**
 * Clear interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to clear
 * @return error code
 */
/*static void _dw_cec_interrupt_clear(dw_hdmi_dev_t *dev, unsigned char mask)
{
	dev_write(dev, CEC_MASK, mask);
}*/

/**
 * Read interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to read
 * @return INT content
 */
int dw_cec_interrupt_get_state(dw_hdmi_dev_t *dev)
{
	return dev_read(dev, IH_CEC_STAT0);
}

/**
 * Clear interrupts state
 * @param dev:  address and device information
 * @param mask: interrupt mask to clear
 * @return INT content
 */
int dw_cec_interrupt_clear_state(dw_hdmi_dev_t *dev, unsigned state)
{
	/* write 1 to clear */
	dev_write(dev, IH_CEC_STAT0, state);
	return 0;
}

/**
 * Set cec logical address
 * @param dev:  address and device information
 * @param addr: logical address
 * @return INT content
 * @return error code or bytes configured
 */
int dw_cec_set_logical_addr(dw_hdmi_dev_t *dev, unsigned int addr)
{
	LOG_TRACE1(addr);
	dev_write(dev, CEC_ADDR_L, (addr & 0xff));
	dev_write(dev, CEC_ADDR_H, (addr >> 8));
	return 0;
}

/**
 * Get cec logical address
 * @param dev:  address and device information
 * @return error code or bytes configured
 */
int dw_cec_get_logical_addr(dw_hdmi_dev_t *dev)
{
	return ((dev_read(dev, CEC_ADDR_H) << 8) | dev_read(dev, CEC_ADDR_L));
}

/**
 * Write transmission buffer
 * @param dev:  address and device information
 * @param buf:  data to transmit
 * @param size: data length [byte]
 * @return error code or bytes configured
 */
int dw_cec_send_frame(dw_hdmi_dev_t *dev, char *buf, unsigned size, unsigned frame_type)
{
	unsigned i;
	unsigned char data;

	dev_write(dev, CEC_TX_CNT, size);
	for (i = 0; i < size; i++)
		dev_write(dev, CEC_TX_DATA + (i * 4), *buf++);

	data = dev_read(dev, CEC_CTRL) & (~(CEC_CTRL_FRAME_TYP_MASK));
	data |= frame_type | CEC_CTRL_SEND_MASK;
	dev_write(dev, CEC_CTRL, data);

	return 0;
}

/**
 * Read reception buffer
 * @param dev:  address and device information
 * @param buf:  buffer to hold receive data
 * @return size: reception data length [byte]
 */
int dw_cec_receive_frame(dw_hdmi_dev_t *dev, char *buf, unsigned *size)
{
	unsigned i;
	unsigned char cnt;

	cnt = dev_read(dev, CEC_RX_CNT);   /* mask 7-5? */
	cnt = (cnt > CEC_RX_DATA_SIZE) ? CEC_RX_DATA_SIZE : cnt;

	for (i = 0; i < cnt; i++)
		*buf++ = dev_read(dev, CEC_RX_DATA + (i * 4));

	_dw_cec_clear_lock(dev);

	*size = cnt;
	return 0;
}

/**
 * Open a CEC controller
 * @warning Execute before start using a CEC controller
 * @param dev:  address and device information
 * @return error code
 */
int dw_cec_enable(dw_hdmi_dev_t *dev)
{
	unsigned char mask = 0x0;

	if (dev == NULL) {
		hdmi_err("%s: param dev is null!!!\n", __func__);
		return -1;
	}

	/* clear ctrl */
	dev_write(dev, CEC_CTRL, 0);

	/* clear lock */
	dev_write(dev, CEC_LOCK, 0);

	/* clear cec logic address */
	dw_cec_set_logical_addr(dev, 0);

	/* TODO */
	_dw_cec_interrupt_disable(dev, CEC_MASK_WAKEUP_MASK);
	dev_write(dev, CEC_WAKEUPCTRL, 0);  /* disable wakeup */

	/* clear all interrupt state */
	dw_cec_interrupt_clear_state(dev, ~0);

	/* enable irq */
	mask |= CEC_MASK_DONE_MASK;
	mask |= CEC_MASK_EOM_MASK;
	mask |= CEC_MASK_NACK_MASK;
	mask |= CEC_MASK_ARB_LOST_MASK;
	mask |= CEC_MASK_ERROR_FLOW_MASK;
	mask |= CEC_MASK_ERROR_INITIATOR_MASK;
	_dw_cec_interrupt_enable(dev, mask);

	/* unmute irq */
	dw_mc_irq_unmute_source(dev, IRQ_CEC);

	return 0;
}

/**
 * Close a CEC controller
 * @warning Execute before stop using a CEC controller
 * @param dev:    address and device information
 * @return error code
 */
int dw_cec_disable(dw_hdmi_dev_t *dev)
{
	unsigned char mask = 0x0;

	if (dev == NULL) {
		hdmi_err("%s: param dev is null!!!\n", __func__);
		return -1;
	}

	/* mask interrupt */
	mask |= CEC_MASK_DONE_MASK;
	mask |= CEC_MASK_EOM_MASK;
	mask |= CEC_MASK_NACK_MASK;
	mask |= CEC_MASK_ARB_LOST_MASK;
	mask |= CEC_MASK_ERROR_FLOW_MASK;
	mask |= CEC_MASK_ERROR_INITIATOR_MASK;
	_dw_cec_interrupt_disable(dev, mask);

	/* mute irq */
	dw_mc_irq_mute_source(dev, IRQ_CEC);

	/* TODO: support wakeup */
	/* _dw_cec_interrupt_clear(CEC_MASK_WAKEUP_MASK); */
	/* _dw_cec_interrupt_enable(CEC_MASK_WAKEUP_MASK); */
	/* _dw_cec_set_standby_mode(1); */

	return 0;
}
