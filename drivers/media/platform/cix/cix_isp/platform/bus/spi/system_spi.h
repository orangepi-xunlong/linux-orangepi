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

#ifndef __SYSTEM_SPI_H__
#define __SYSTEM_SPI_H__
#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/uaccess.h>

/* Name of this driver */
#define ARMCB_SPI_DRVNAME "armcb-spi"

/* Register offset definitions */
#define ARMCB_SPI_CR 0x00 /* Configuration  Register, RW */
#define ARMCB_SPI_ISR 0x04 /* Interrupt Status Register, RO */
#define ARMCB_SPI_IER 0x08 /* Interrupt Enable Register, WO */
#define ARMCB_SPI_IDR 0x0c /* Interrupt Disable Register, WO */
#define ARMCB_SPI_IMR 0x10 /* Interrupt Enabled Mask Register, RO */
#define ARMCB_SPI_ER 0x14 /* Enable/Disable Register, RW */
#define ARMCB_SPI_DR 0x18 /* Delay Register, RW */
#define ARMCB_SPI_TXD 0x1C /* Data Transmit Register, WO */
#define ARMCB_SPI_RXD 0x20 /* Data Receive Register, RO */
#define ARMCB_SPI_SICR 0x24 /* Slave Idle Count Register, RW */
#define ARMCB_SPI_THLD 0x28 /* Transmit FIFO Watermark Register,RW */

#define SPI_AUTOSUSPEND_TIMEOUT 3000
/*
 * SPI Configuration Register bit Masks
 *
 * This register contains various control bits that affect the operation
 * of the SPI controller
 */
#define ARMCB_SPI_CR_MANSTRT 0x00010000 /* Manual TX Start */
#define ARMCB_SPI_CR_CPHA 0x00000004 /* Clock Phase Control */
#define ARMCB_SPI_CR_CPOL 0x00000002 /* Clock Polarity Control */
#define ARMCB_SPI_CR_SSCTRL 0x00003C00 /* Slave Select Mask */
#define ARMCB_SPI_CR_PERI_SEL 0x00000200 /* Peripheral Select Decode */
#define ARMCB_SPI_CR_BAUD_DIV 0x00000038 /* Baud Rate Divisor Mask */
#define ARMCB_SPI_CR_MSTREN 0x00000001 /* Master Enable Mask */
#define ARMCB_SPI_CR_MANSTRTEN 0x00008000 /* Manual TX Enable Mask */
#define ARMCB_SPI_CR_SSFORCE 0x00004000 /* Manual SS Enable Mask */
#define ARMCB_SPI_CR_BAUD_DIV_4 0x00000008 /* Default Baud Div Mask */
#define ARMCB_SPI_CR_DEFAULT                                                \
	(ARMCB_SPI_CR_MSTREN | ARMCB_SPI_CR_SSCTRL | ARMCB_SPI_CR_SSFORCE | \
	 ARMCB_SPI_CR_BAUD_DIV_4)

/*
 * SPI Configuration Register - Baud rate and slave select
 *
 * These are the values used in the calculation of baud rate divisor and
 * setting the slave select.
 */

#define ARMCB_SPI_BAUD_DIV_MAX 7 /* Baud rate divisor maximum */
#define ARMCB_SPI_BAUD_DIV_MIN 1 /* Baud rate divisor minimum */
#define ARMCB_SPI_BAUD_DIV_SHIFT 3 /* Baud rate divisor shift in CR */
#define ARMCB_SPI_SS_SHIFT 10 /* Slave Select field shift in CR */
#define ARMCB_SPI_SS0 0x1 /* Slave Select zero */

/*
 * SPI Interrupt Registers bit Masks
 *
 * All the four interrupt registers (Status/Mask/Enable/Disable) have the same
 * bit definitions.
 */
#define ARMCB_SPI_IXR_TXOW 0x00000004 /* SPI TX FIFO Overwater */
#define ARMCB_SPI_IXR_MODF 0x00000002 /* SPI Mode Fault */
#define ARMCB_SPI_IXR_RXNEMTY 0x00000010 /* SPI RX FIFO Not Empty */
#define ARMCB_SPI_IXR_DEFAULT (ARMCB_SPI_IXR_TXOW | ARMCB_SPI_IXR_MODF)
#define ARMCB_SPI_IXR_TXFULL 0x00000008 /* SPI TX Full */
#define ARMCB_SPI_IXR_ALL 0x0000007F /* SPI all interrupts */

/*
 * SPI Enable Register bit Masks
 *
 * This register is used to enable or disable the SPI controller
 */
#define ARMCB_SPI_ER_ENABLE 0x00000001 /* SPI Enable Bit Mask */
#define ARMCB_SPI_ER_DISABLE 0x0 /* SPI Disable Bit Mask */

/* SPI FIFO depth in bytes */
#define ARMCB_SPI_FIFO_DEPTH 128

/* Default number of chip select lines */
#define ARMCB_SPI_DEFAULT_NUM_CS 4

/**
 * struct armcb_spi - This definition defines spi driver instance
 * @regs:     Virtual address of the SPI controller registers
 * @ref_clk:  Pointer to the peripheral clock
 * @pclk:     Pointer to the APB clock
 * @speed_hz: Current SPI bus clock speed in Hz
 * @txbuf:    Pointer	to the TX buffer
 * @rxbuf:    Pointer to the RX buffer
 * @tx_bytes: Number of bytes left to transfer
 * @rx_bytes: Number of bytes requested
 * @dev_busy: Device busy flag
 * @is_decoded_cs: Flag for decoder property set or not
 */
struct armcb_spi {
	void __iomem *regs;
	struct clk *ref_clk;
	struct clk *pclk;
	u32 speed_hz;
	const u8 *txbuf;
	u8 *rxbuf;
	int tx_bytes;
	int rx_bytes;
	u8 dev_busy;
	u32 is_decoded_cs;
};

struct armcb_spi_device_data {
	bool gpio_requested;
};

struct system_spi_info {
	void __iomem *spi_base_addr;
	u8 curChnl;
};

#ifdef ARMCB_CAM_KO
void *armcb_get_system_spi_driver_instance(void);
void armcb_system_spi_driver_destroy(void);
#endif

#endif
