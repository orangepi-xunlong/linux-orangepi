/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SYSTEM_I2C_H__
#define __SYSTEM_I2C_H__

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

/* Register offsets for the I2C device. */
#define ARMCB_I2C_CR_OFFSET 0x00 /* Control Register, RW */
#define ARMCB_I2C_SR_OFFSET 0x04 /* Status Register, RO */
#define ARMCB_I2C_ADDR_OFFSET 0x08 /* I2C Address Register, RW */
#define ARMCB_I2C_DATA_OFFSET 0x0C /* I2C Data Register, RW */
#define ARMCB_I2C_ISR_OFFSET 0x10 /* IRQ Status Register, RW */
#define ARMCB_I2C_XFER_SIZE_OFFSET 0x14 /* Transfer Size Register, RW */
#define ARMCB_I2C_SLV_PAUSE_OFFSET 0x18 /* Transfer Size Register, RW */
#define ARMCB_I2C_TIME_OUT_OFFSET 0x1C /* Time Out Register, RW */
#define ARMCB_I2C_IMR_OFFSET 0x20 /* IRQ Mask Register, RO */
#define ARMCB_I2C_IER_OFFSET 0x24 /* IRQ Enable Register, WO */
#define ARMCB_I2C_IDR_OFFSET 0x28 /* IRQ Disable Register, WO */

/* Control Register Bit mask definitions */
#define ARMCB_I2C_CR_SLVMON BIT(5) /* Slave monitor mode bit */
#define ARMCB_I2C_CR_HOLD BIT(4) /* Hold Bus bit */
#define ARMCB_I2C_CR_ACK_EN BIT(3)
#define ARMCB_I2C_CR_NEA BIT(2)
#define ARMCB_I2C_CR_MS BIT(1)
/* Read or Write Master transfer 0 = Transmitter, 1 = Receiver */
#define ARMCB_I2C_CR_RW BIT(0)
/* 1 = Auto init FIFO to zeroes */
#define ARMCB_I2C_CR_CLR_FIFO BIT(6)
#define ARMCB_I2C_CR_DIVA_SHIFT 14
#define ARMCB_I2C_CR_DIVA_MASK (3 << ARMCB_I2C_CR_DIVA_SHIFT)
#define ARMCB_I2C_CR_DIVB_SHIFT 8
#define ARMCB_I2C_CR_DIVB_MASK (0x3f << ARMCB_I2C_CR_DIVB_SHIFT)

#define ARMCB_I2C_CR_SLAVE_EN_MASK                                        \
	(ARMCB_I2C_CR_CLR_FIFO | ARMCB_I2C_CR_NEA | ARMCB_I2C_CR_ACK_EN | \
	 ARMCB_I2C_CR_MS)

/* Status Register Bit mask definitions */
#define ARMCB_I2C_SR_BA BIT(8)
#define ARMCB_I2C_SR_TXDV BIT(6)
#define ARMCB_I2C_SR_RXDV BIT(5)
#define ARMCB_I2C_SR_RXRW BIT(3)

/*
 * I2C Address Register Bit mask definitions
 * Normal addressing mode uses [6:0] bits. Extended addressing mode uses [9:0]
 * bits. A write access to this register always initiates a transfer if the I2C
 * is in master mode.
 */
#define ARMCB_I2C_ADDR_MASK 0x000003FF /* I2C Address Mask */

/*
 * I2C Interrupt Registers Bit mask definitions
 * All the four interrupt registers (Status/Mask/Enable/Disable) have the same
 * bit definitions.
 */
#define ARMCB_I2C_IXR_ARB_LOST BIT(9)
#define ARMCB_I2C_IXR_RX_UNF BIT(7)
#define ARMCB_I2C_IXR_TX_OVF BIT(6)
#define ARMCB_I2C_IXR_RX_OVF BIT(5)
#define ARMCB_I2C_IXR_SLV_RDY BIT(4)
#define ARMCB_I2C_IXR_TO BIT(3)
#define ARMCB_I2C_IXR_NACK BIT(2)
#define ARMCB_I2C_IXR_DATA BIT(1)
#define ARMCB_I2C_IXR_COMP BIT(0)

#define ARMCB_I2C_IXR_ALL_INTR_MASK                                            \
	(ARMCB_I2C_IXR_ARB_LOST | ARMCB_I2C_IXR_RX_UNF |                       \
	 ARMCB_I2C_IXR_TX_OVF | ARMCB_I2C_IXR_RX_OVF | ARMCB_I2C_IXR_SLV_RDY | \
	 ARMCB_I2C_IXR_TO | ARMCB_I2C_IXR_NACK | ARMCB_I2C_IXR_DATA |          \
	 ARMCB_I2C_IXR_COMP)

#define ARMCB_I2C_IXR_ERR_INTR_MASK                      \
	(ARMCB_I2C_IXR_ARB_LOST | ARMCB_I2C_IXR_RX_UNF | \
	 ARMCB_I2C_IXR_TX_OVF | ARMCB_I2C_IXR_RX_OVF | ARMCB_I2C_IXR_NACK)

#define ARMCB_I2C_ENABLED_INTR_MASK                                         \
	(ARMCB_I2C_IXR_ARB_LOST | ARMCB_I2C_IXR_RX_UNF |                    \
	 ARMCB_I2C_IXR_TX_OVF | ARMCB_I2C_IXR_RX_OVF | ARMCB_I2C_IXR_NACK | \
	 ARMCB_I2C_IXR_DATA | ARMCB_I2C_IXR_COMP)

#define ARMCB_I2C_IXR_SLAVE_INTR_MASK                                         \
	(ARMCB_I2C_IXR_RX_UNF | ARMCB_I2C_IXR_TX_OVF | ARMCB_I2C_IXR_RX_OVF | \
	 ARMCB_I2C_IXR_TO | ARMCB_I2C_IXR_NACK | ARMCB_I2C_IXR_DATA |         \
	 ARMCB_I2C_IXR_COMP)

#define ARMCB_I2C_TIMEOUT msecs_to_jiffies(1000)
/* timeout for pm runtime autosuspend */
#define ARMCB_I2C_PM_TIMEOUT 1000 /* ms */

#define ARMCB_I2C_FIFO_DEPTH 16
/* FIFO depth at which the DATA interrupt occurs */
#define ARMCB_I2C_DATA_INTR_DEPTH (ARMCB_I2C_FIFO_DEPTH - 2)
#define ARMCB_I2C_MAX_TRANSFER_SIZE 255
/* Transfer size in multiples of data interrupt depth */
#define ARMCB_I2C_TRANSFER_SIZE (ARMCB_I2C_MAX_TRANSFER_SIZE - 3)

#define ARMCB_I2C_DRVNAME "armcb-i2c"

#define ARMCB_I2C_SPEED_MAX 400000
#define ARMCB_I2C_SPEED_DEFAULT 100000

#define ARMCB_I2C_DIVA_MAX 4
#define ARMCB_I2C_DIVB_MAX 64

#define ARMCB_I2C_TIMEOUT_MAX 0xFF

#define ARMCB_I2C_BROKEN_HOLD_BIT BIT(0)

#define armcb_i2c_readreg(offset) readl_relaxed(id->membase + offset)
#define armcb_i2c_writereg(val, offset) \
	writel_relaxed(val, id->membase + offset)

#if IS_ENABLED(CONFIG_I2C_SLAVE)
/**
 * enum armcb_i2c_mode - I2C Controller current operating mode
 *
 * @ARMCB_I2C_MODE_SLAVE:       I2C controller operating in slave mode
 * @ARMCB_I2C_MODE_MASTER:      I2C Controller operating in master mode
 */
enum armcb_i2c_mode {
	ARMCB_I2C_MODE_SLAVE,
	ARMCB_I2C_MODE_MASTER,
};

/**
 * enum armcb_i2c_slave_mode - Slave state when I2C is operating in slave mode
 *
 * @ARMCB_I2C_SLAVE_STATE_IDLE: I2C slave idle
 * @ARMCB_I2C_SLAVE_STATE_SEND: I2C slave sending data to master
 * @ARMCB_I2C_SLAVE_STATE_RECV: I2C slave receiving data from master
 */
enum armcb_i2c_slave_state {
	ARMCB_I2C_SLAVE_STATE_IDLE,
	ARMCB_I2C_SLAVE_STATE_SEND,
	ARMCB_I2C_SLAVE_STATE_RECV,
};
#endif

/**
 * struct armcb_i2c - I2C device private data structure
 *
 * @dev:		Pointer to device structure
 * @membase:		Base address of the I2C device
 * @adap:		I2C adapter instance
 * @p_msg:		Message pointer
 * @err_status:		Error status in Interrupt Status Register
 * @xfer_done:		Transfer complete status
 * @p_send_buf:		Pointer to transmit buffer
 * @p_recv_buf:		Pointer to receive buffer
 * @send_count:		Number of bytes still expected to send
 * @recv_count:		Number of bytes still expected to receive
 * @curr_recv_count:	Number of bytes to be received in current transfer
 * @irq:		IRQ number
 * @input_clk:		Input clock to I2C controller
 * @i2c_clk:		Maximum I2C clock speed
 * @bus_hold_flag:	Flag used in repeated start for clearing HOLD bit
 * @clk:		Pointer to struct clk
 * @clk_rate_change_nb:	Notifier block for clock rate changes
 * @quirks:		flag for broken hold bit usage in r1p10
 * @ctrl_reg:		Cached value of the control register.
 * @rinfo:		Structure holding recovery information.
 * @pinctrl:		Pin control state holder.
 * @pinctrl_pins_default: Default pin control state.
 * @pinctrl_pins_gpio:	GPIO pin control state.
 * @slave:		Registered slave instance.
 * @dev_mode:		I2C operating role(master/slave).
 * @slave_state:	I2C Slave state(idle/read/write).
 */
struct armcb_i2c {
	struct device *dev;
	void __iomem *membase;
	struct i2c_adapter adap;
	struct i2c_msg *p_msg;
	int err_status;
	struct completion xfer_done;
	unsigned char *p_send_buf;
	unsigned char *p_recv_buf;
	unsigned int send_count;
	unsigned int recv_count;
	unsigned int curr_recv_count;
	int irq;
	unsigned long input_clk;
	unsigned int i2c_clk;
	unsigned int bus_hold_flag;
	struct clk *clk;
	struct notifier_block clk_rate_change_nb;
	u32 quirks;
	u32 ctrl_reg;
	struct i2c_bus_recovery_info rinfo;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_pins_default;
	struct pinctrl_state *pinctrl_pins_gpio;
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	struct i2c_client *slave;
	enum armcb_i2c_mode dev_mode;
	enum armcb_i2c_slave_state slave_state;
#endif
};

struct armcb_platform_data {
	u32 quirks;
};

#define to_armcb_i2c(_nb) \
	container_of(_nb, struct armcb_i2c, clk_rate_change_nb)

#ifdef ARMCB_CAM_KO
void *armcb_get_system_i2c_driver_instance(void);
void armcb_system_i2c_driver_detroy(void);
#endif
#endif //__SYSTEM_I2C_H__
