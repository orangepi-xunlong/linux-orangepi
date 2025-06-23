// SPDX-License-Identifier: GPL-2.0
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
#include "system_spi.h"
#include "armcb_isp.h"
#include "system_logger.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

/**************************** Global variables ******************************/

struct system_spi_info *p_spi_info;

/* Macros for the SPI controller read/write */
static inline u32 armcb_spi_read(struct armcb_spi *xspi, u32 offset)
{
	return readl_relaxed(xspi->regs + offset);
}

static inline void armcb_spi_write(struct armcb_spi *xspi, u32 offset, u32 val)
{
	writel_relaxed(val, xspi->regs + offset);
}

/**
 * armcb_spi_init_hw - Initialize the hardware and configure the SPI controller
 * @xspi:	Pointer to the armcb_spi structure
 *
 * On reset the SPI controller is configured to be in master mode, baud rate
 * divisor is set to 4, threshold value for TX FIFO not full interrupt is set
 * to 1 and size of the word to be transferred as 8 bit.
 * This function initializes the SPI controller to disable and clear all the
 * interrupts, enable manual slave select and manual start, deselect all the
 * chip select lines, and enable the SPI controller.
 */
static void armcb_spi_init_hw(struct armcb_spi *xspi)
{
	u32 ctrl_reg = ARMCB_SPI_CR_DEFAULT;

	if (xspi->is_decoded_cs)
		ctrl_reg |= ARMCB_SPI_CR_PERI_SEL;

	armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_DISABLE);
	armcb_spi_write(xspi, ARMCB_SPI_IDR, ARMCB_SPI_IXR_ALL);

	/* Clear the RX FIFO */
	while (armcb_spi_read(xspi, ARMCB_SPI_ISR) & ARMCB_SPI_IXR_RXNEMTY)
		armcb_spi_read(xspi, ARMCB_SPI_RXD);

	armcb_spi_write(xspi, ARMCB_SPI_ISR, ARMCB_SPI_IXR_ALL);
	armcb_spi_write(xspi, ARMCB_SPI_CR, ctrl_reg);
	armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_ENABLE);
}

/**
 * armcb_spi_chipselect - Select or deselect the chip select line
 * @spi:	Pointer to the spi_device structure
 * @is_high:	Select(0) or deselect (1) the chip select line
 */
static void armcb_spi_chipselect(struct spi_device *spi, bool is_high)
{
	struct armcb_spi *xspi = spi_master_get_devdata(spi->master);
	u32 ctrl_reg;

	ctrl_reg = armcb_spi_read(xspi, ARMCB_SPI_CR);

	if (is_high) {
		/* Deselect the slave */
		ctrl_reg |= ARMCB_SPI_CR_SSCTRL;
	} else {
		/* Select the slave */
		ctrl_reg &= ~ARMCB_SPI_CR_SSCTRL;
		if (!(xspi->is_decoded_cs))
			ctrl_reg |= ((~(ARMCB_SPI_SS0 << spi->chip_select))
					 << ARMCB_SPI_SS_SHIFT) &
					ARMCB_SPI_CR_SSCTRL;
		else
			ctrl_reg |= (spi->chip_select << ARMCB_SPI_SS_SHIFT) &
					ARMCB_SPI_CR_SSCTRL;
	}
	armcb_spi_write(xspi, ARMCB_SPI_CR, ctrl_reg);
}

/**
 * armcb_spi_config_clock_mode - Sets clock polarity and phase
 * @spi:	Pointer to the spi_device structure
 *
 * Sets the requested clock polarity and phase.
 */
static void armcb_spi_config_clock_mode(struct spi_device *spi)
{
	struct armcb_spi *xspi = spi_master_get_devdata(spi->master);
	u32 ctrl_reg, new_ctrl_reg;

	new_ctrl_reg = armcb_spi_read(xspi, ARMCB_SPI_CR);
	ctrl_reg = new_ctrl_reg;
	/* Set the SPI clock phase and clock polarity */
	new_ctrl_reg &= ~(ARMCB_SPI_CR_CPHA | ARMCB_SPI_CR_CPOL);
	if (spi->mode & SPI_CPHA)
		new_ctrl_reg |= ARMCB_SPI_CR_CPHA;
	if (spi->mode & SPI_CPOL)
		new_ctrl_reg |= ARMCB_SPI_CR_CPOL;

	if (new_ctrl_reg != ctrl_reg) {
<<<<<<< HEAD   (28289f DPTSW-7267: Fix isp driver build warning)
		/*
		* toggle the ER/CR register.
		*/
=======
	/*
	 * Just writing the CR register does not seem to apply the clock
	 * setting changes. This is problematic when changing the clock
	 * polarity as it will cause the SPI slave to see spurious clock
	 * transitions. To workaround the issue toggle the ER register.
	 */
>>>>>>> CHANGE (3bd1a5 DPTSW-12398: fix the errors and warnings of code style check)
		armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_DISABLE);
		armcb_spi_write(xspi, ARMCB_SPI_CR, new_ctrl_reg);
		armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_ENABLE);
	}
}

/**
 * armcb_spi_config_clock_freq - Sets clock frequency
 * @spi:	Pointer to the spi_device structure
 * @transfer:	Pointer to the spi_transfer structure which provides
 *		information about next transfer setup parameters
 *
 * Sets the requested clock frequency.
 * Note: If the requested frequency is not an exact match with what can be
 * obtained using the prescalar value the driver sets the clock frequency which
 * is lower than the requested frequency (maximum lower) for the transfer. If
 * the requested frequency is higher or lower than that is supported by the SPI
 * controller the driver will set the highest or lowest frequency supported by
 * controller.
 */
static void armcb_spi_config_clock_freq(struct spi_device *spi,
					struct spi_transfer *transfer)
{
	struct armcb_spi *xspi = spi_master_get_devdata(spi->master);
	u32 ctrl_reg, baud_rate_val;
	unsigned long frequency;

	frequency = clk_get_rate(xspi->ref_clk);

	ctrl_reg = armcb_spi_read(xspi, ARMCB_SPI_CR);

	/* Set the clock frequency */
	if (xspi->speed_hz != transfer->speed_hz) {
		/* first valid value is 1 */
		baud_rate_val = ARMCB_SPI_BAUD_DIV_MIN;
		while ((baud_rate_val < ARMCB_SPI_BAUD_DIV_MAX) &&
			   (frequency / (2 << baud_rate_val)) > transfer->speed_hz)
			baud_rate_val++;

		ctrl_reg &= ~ARMCB_SPI_CR_BAUD_DIV;
		ctrl_reg |= baud_rate_val << ARMCB_SPI_BAUD_DIV_SHIFT;

		xspi->speed_hz = frequency / (2 << baud_rate_val);
	}
	armcb_spi_write(xspi, ARMCB_SPI_CR, ctrl_reg);
}

/**
 * armcb_spi_setup_transfer - Configure SPI controller for specified transfer
 * @spi:	Pointer to the spi_device structure
 * @transfer:	Pointer to the spi_transfer structure which provides
 *		information about next transfer setup parameters
 *
 * Sets the operational mode of SPI controller for the next SPI transfer and
 * sets the requested clock frequency.
 *
 * Return:	Always 0
 */
static int armcb_spi_setup_transfer(struct spi_device *spi,
					struct spi_transfer *transfer)
{
	struct armcb_spi *xspi = spi_master_get_devdata(spi->master);

	armcb_spi_config_clock_freq(spi, transfer);

	LOG(LOG_DEBUG, "mode %d, %u bits/w, %u clock speed", spi->mode,
		spi->bits_per_word, xspi->speed_hz);

	return 0;
}

/**
 * armcb_spi_fill_tx_fifo - Fills the TX FIFO with as many bytes as possible
 * @xspi:	Pointer to the armcb_spi structure
 */
static void armcb_spi_fill_tx_fifo(struct armcb_spi *xspi)
{
	unsigned long trans_cnt = 0;

	while ((trans_cnt < ARMCB_SPI_FIFO_DEPTH) && (xspi->tx_bytes > 0)) {
		if (xspi->txbuf)
			armcb_spi_write(xspi, ARMCB_SPI_TXD, *xspi->txbuf++);
		else
			armcb_spi_write(xspi, ARMCB_SPI_TXD, 0);

		xspi->tx_bytes--;
		trans_cnt++;
	}
}

/**
 * armcb_spi_irq - Interrupt service routine of the SPI controller
 * @irq:	IRQ number
 * @dev_id:	Pointer to the xspi structure
 *
 * This function handles TX empty and Mode Fault interrupts only.
 * On TX empty interrupt this function reads the received data from RX FIFO and
 * fills the TX FIFO if there is any data remaining to be transferred.
 * On Mode Fault interrupt this function indicates that transfer is completed,
 * the SPI subsystem will identify the error as the remaining bytes to be
 * transferred is non-zero.
 *
 * Return:	IRQ_HANDLED when handled; IRQ_NONE otherwise.
 */
static irqreturn_t armcb_spi_irq(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct armcb_spi *xspi = spi_master_get_devdata(master);
	u32 intr_status = 0, status = IRQ_NONE;
	u8 data = 0;
	unsigned long trans_cnt = 0;

	intr_status = armcb_spi_read(xspi, ARMCB_SPI_ISR);
	armcb_spi_write(xspi, ARMCB_SPI_ISR, intr_status);
	if (intr_status & ARMCB_SPI_IXR_MODF) {
	/* Indicate that transfer is completed, the SPI subsystem will
	 * identify the error as the remaining bytes to be
	 * transferred is non-zero
	 */
		armcb_spi_write(xspi, ARMCB_SPI_IDR, ARMCB_SPI_IXR_DEFAULT);
		spi_finalize_current_transfer(master);
		status = IRQ_HANDLED;
	} else if (intr_status & ARMCB_SPI_IXR_TXOW) {
		trans_cnt = xspi->rx_bytes - xspi->tx_bytes;
		/* Read out the data from the RX FIFO */
		while (trans_cnt) {
			data = armcb_spi_read(xspi, ARMCB_SPI_RXD);
			if (xspi->rxbuf)
				*xspi->rxbuf++ = data;

			xspi->rx_bytes--;
			trans_cnt--;
		}

		if (xspi->tx_bytes) {
			/* There is more data to send */
			armcb_spi_fill_tx_fifo(xspi);
		} else {
			/* Transfer is completed */
			armcb_spi_write(xspi, ARMCB_SPI_IDR,
					ARMCB_SPI_IXR_DEFAULT);
			spi_finalize_current_transfer(master);
		}
		status = IRQ_HANDLED;
	}

	return status;
}

static int armcb_prepare_message(struct spi_master *master,
				 struct spi_message *msg)
{
	armcb_spi_config_clock_mode(msg->spi);
	return 0;
}

/**
 * armcb_transfer_one - Initiates the SPI transfer
 * @master:	Pointer to spi_master structure
 * @spi:	Pointer to the spi_device structure
 * @transfer:	Pointer to the spi_transfer structure which provides
 *		information about next transfer parameters
 *
 * This function fills the TX FIFO, starts the SPI transfer and
 * returns a positive transfer count so that core will wait for completion.
 *
 * Return:	Number of bytes transferred in the last transfer
 */
static int armcb_transfer_one(struct spi_master *master, struct spi_device *spi,
				  struct spi_transfer *transfer)
{
	struct armcb_spi *xspi = spi_master_get_devdata(master);

	xspi->txbuf = transfer->tx_buf;
	xspi->rxbuf = transfer->rx_buf;
	xspi->tx_bytes = transfer->len;
	xspi->rx_bytes = transfer->len;

	armcb_spi_setup_transfer(spi, transfer);

	armcb_spi_fill_tx_fifo(xspi);

	armcb_spi_write(xspi, ARMCB_SPI_IER, ARMCB_SPI_IXR_DEFAULT);

	return transfer->len;
}

/**
 * armcb_prepare_transfer_hardware - Prepares hardware for transfer.
 * @master:	Pointer to the spi_master structure which provides
 *		information about the controller.
 *
 * This function enables SPI master controller.
 *
 * Return:	0 always
 */
static int armcb_prepare_transfer_hardware(struct spi_master *master)
{
	struct armcb_spi *xspi = spi_master_get_devdata(master);

	armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_ENABLE);

	return 0;
}

/**
 * armcb_unprepare_transfer_hardware - Relaxes hardware after transfer
 * @master:	Pointer to the spi_master structure which provides
 *		information about the controller.
 *
 * This function disables the SPI master controller.
 *
 * Return:	0 always
 */
static int armcb_unprepare_transfer_hardware(struct spi_master *master)
{
	struct armcb_spi *xspi = spi_master_get_devdata(master);

	armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_DISABLE);

	return 0;
}

static int armcb_spi_setup(struct spi_device *spi)
{
	int ret = -EINVAL, mode = 0;
	struct armcb_spi_device_data *armcb_spi_data = spi_get_ctldata(spi);
	/* this is a pin managed by the controller, leave it alone */
	if (spi->cs_gpiod == -ENOENT)
		return 0;

	/* this seems to be the first time we're here */
	if (!armcb_spi_data) {
		armcb_spi_data = kzalloc(sizeof(*armcb_spi_data), GFP_KERNEL);
		if (!armcb_spi_data)
			return -ENOMEM;
		armcb_spi_data->gpio_requested = false;
		spi_set_ctldata(spi, armcb_spi_data);
	}

	/* if we haven't done so, grab the gpio */
	if (!armcb_spi_data->gpio_requested && gpio_is_valid(spi->cs_gpiod)) {
		ret = gpio_request_one(spi->cs_gpiod,
					   (spi->mode & SPI_CS_HIGH) ?
						   GPIOF_OUT_INIT_LOW :
						   GPIOF_OUT_INIT_HIGH,
					   dev_name(&spi->dev));
		if (ret)
			LOG(LOG_ERR, "can't request chipselect gpio %d",
				spi->cs_gpiod);
		else
			armcb_spi_data->gpio_requested = true;
	} else {
		if (gpio_is_valid(spi->cs_gpiod)) {
			mode = ((spi->mode & SPI_CS_HIGH) ?
					GPIOF_OUT_INIT_LOW :
					GPIOF_OUT_INIT_HIGH);

			ret = gpio_direction_output(spi->cs_gpiod, mode);
			if (ret)
				LOG(LOG_ERR,
					"chipselect gpio %d setup failed (%d)",
					spi->cs_gpiod, ret);
		}
	}

	return ret;
}

static void armcb_spi_cleanup(struct spi_device *spi)
{
	struct armcb_spi_device_data *armcb_spi_data = spi_get_ctldata(spi);

	if (armcb_spi_data) {
		if (armcb_spi_data->gpio_requested)
			gpio_free(spi->cs_gpiod);
		kfree(armcb_spi_data);
		spi_set_ctldata(spi, NULL);
	}
}

/**
 * armcb_spi_probe - Probe method for the SPI driver
 * @pdev:	Pointer to the platform_device structure
 *
 * This function initializes the driver data structures and the hardware.
 *
 * Return:	0 on success and error value on error
 */
static int armcb_spi_probe(struct platform_device *pdev)
{
	int ret = 0, irq;
#ifndef QEMU_ON_VEXPRESS
	struct spi_master *master;
	struct armcb_spi *xspi;
	struct resource *res;
	u32 num_cs = 0;

	master = spi_alloc_master(&pdev->dev, sizeof(*xspi));
	if (!master)
		return -ENOMEM;

	xspi = spi_master_get_devdata(master);
	master->dev.of_node = pdev->dev.of_node;
	platform_set_drvdata(pdev, master);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xspi->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(xspi->regs)) {
		ret = PTR_ERR(xspi->regs);
		goto remove_master;
	}

	xspi->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(xspi->pclk)) {
		LOG(LOG_ERR, "pclk clock not found.");
		ret = PTR_ERR(xspi->pclk);
		goto remove_master;
	}

	xspi->ref_clk = devm_clk_get(&pdev->dev, "ref_clk");
	if (IS_ERR(xspi->ref_clk)) {
		LOG(LOG_ERR, "ref_clk clock not found.");
		ret = PTR_ERR(xspi->ref_clk);
		goto remove_master;
	}

	ret = clk_prepare_enable(xspi->pclk);
	if (ret) {
		LOG(LOG_ERR, "Unable to enable APB clock.");
		goto remove_master;
	}

	ret = clk_prepare_enable(xspi->ref_clk);
	if (ret) {
		LOG(LOG_ERR, "Unable to enable device clock.");
		goto clk_dis_apb;
	}

	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, SPI_AUTOSUSPEND_TIMEOUT);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = of_property_read_u32(pdev->dev.of_node, "num-cs", &num_cs);
	if (ret < 0)
		master->num_chipselect = ARMCB_SPI_DEFAULT_NUM_CS;
	else
		master->num_chipselect = num_cs;

	ret = of_property_read_u32(pdev->dev.of_node, "is-decodes-cs",
				   &xspi->is_decoded_cs);
	if (ret < 0)
		xspi->is_decoded_cs = 0;

	/* SPI controller initializations */
	armcb_spi_init_hw(xspi);

	pm_runtime_mark_last_busy(&pdev->dev);
	pm_runtime_put_autosuspend(&pdev->dev);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		ret = -ENXIO;
		LOG(LOG_ERR, "irq number is invalid");
		goto clk_dis_all;
	}

	ret = devm_request_irq(&pdev->dev, irq, armcb_spi_irq, 0, pdev->name,
				   master);
	if (ret != 0) {
		ret = -ENXIO;
		LOG(LOG_ERR, "request_irq failed");
		goto clk_dis_all;
	}

	master->prepare_transfer_hardware = armcb_prepare_transfer_hardware;
	master->prepare_message = armcb_prepare_message;
	master->transfer_one = armcb_transfer_one;
	master->unprepare_transfer_hardware = armcb_unprepare_transfer_hardware;
	master->set_cs = armcb_spi_chipselect;
	master->setup = armcb_spi_setup;
	master->cleanup = armcb_spi_cleanup;
	master->auto_runtime_pm = true;
	master->mode_bits = SPI_CPOL | SPI_CPHA;

	/* Set to default valid value */
	master->max_speed_hz = clk_get_rate(xspi->ref_clk) / 4;
	xspi->speed_hz = master->max_speed_hz;

	master->bits_per_word_mask = SPI_BPW_MASK(8);

	ret = spi_register_master(master);
	if (ret) {
		LOG(LOG_ERR, "spi_register_master failed");
		goto clk_dis_all;
	}

	return ret;
clk_dis_all:
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	clk_disable_unprepare(xspi->ref_clk);
clk_dis_apb:
	clk_disable_unprepare(xspi->pclk);
remove_master:
	spi_master_put(master);
#endif
	return ret;
}

/**
 * armcb_spi_remove - Remove method for the SPI driver
 * @pdev:	Pointer to the platform_device structure
 *
 * This function is called if a device is physically removed from the system or
 * if the driver module is being unloaded. It frees all resources allocated to
 * the device.
 *
 * Return:	0 on success and error value on error
 */
static int armcb_spi_remove(struct platform_device *pdev)
{
#ifndef QEMU_ON_VEXPRESS
	struct spi_master *master = platform_get_drvdata(pdev);
	struct armcb_spi *xspi = spi_master_get_devdata(master);

	armcb_spi_write(xspi, ARMCB_SPI_ER, ARMCB_SPI_ER_DISABLE);

	clk_disable_unprepare(xspi->ref_clk);
	clk_disable_unprepare(xspi->pclk);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	spi_unregister_master(master);
#endif
	return 0;
}

/**
 * armcb_spi_suspend - Suspend method for the SPI driver
 * @dev:	Address of the platform_device structure
 *
 * This function disables the SPI controller and
 * changes the driver state to "suspend"
 *
 * Return:	0 on success and error value on error
 */
static int __maybe_unused armcb_spi_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = platform_get_drvdata(pdev);

	return spi_master_suspend(master);
}

/**
 * armcb_spi_resume - Resume method for the SPI driver
 * @dev:	Address of the platform_device structure
 *
 * This function changes the driver state to "ready"
 *
 * Return:	0 on success and error value on error
 */
static int __maybe_unused armcb_spi_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = platform_get_drvdata(pdev);
	struct armcb_spi *xspi = spi_master_get_devdata(master);

	armcb_spi_init_hw(xspi);
	return spi_master_resume(master);
}

/**
 * armcb_spi_runtime_resume - Runtime resume method for the SPI driver
 * @dev:	Address of the platform_device structure
 *
 * This function enables the clocks
 *
 * Return:	0 on success and error value on error
 */
static int __maybe_unused armcb_runtime_resume(struct device *dev)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct armcb_spi *xspi = spi_master_get_devdata(master);
	int ret;

	ret = clk_prepare_enable(xspi->pclk);
	if (ret) {
		LOG(LOG_ERR, "Cannot enable APB clock.");
		return ret;
	}

	ret = clk_prepare_enable(xspi->ref_clk);
	if (ret) {
		LOG(LOG_ERR, "Cannot enable device clock.");
		clk_disable(xspi->pclk);
		return ret;
	}
	return 0;
}

/**
 * armcb_spi_runtime_suspend - Runtime suspend method for the SPI driver
 * @dev:	Address of the platform_device structure
 *
 * This function disables the clocks
 *
 * Return:	Always 0
 */
static int __maybe_unused armcb_runtime_suspend(struct device *dev)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct armcb_spi *xspi = spi_master_get_devdata(master);

	clk_disable_unprepare(xspi->ref_clk);
	clk_disable_unprepare(xspi->pclk);

	return 0;
}

static const struct dev_pm_ops armcb_spi_dev_pm_ops = {
	SET_RUNTIME_PM_OPS(armcb_runtime_suspend, armcb_runtime_resume, NULL)
		SET_SYSTEM_SLEEP_PM_OPS(armcb_spi_suspend, armcb_spi_resume)
};

static const struct of_device_id armcb_spi_of_match[] = {
	{ .compatible = "armcb,armcb-spi" },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, armcb_spi_of_match);

/* armcb_spi_driver - This structure defines the SPI subsystem platform driver
 */
static struct platform_driver armcb_spi_driver = {
	.probe = armcb_spi_probe,
	.remove = armcb_spi_remove,
	.driver = {
		.name = ARMCB_SPI_DRVNAME,
		.of_match_table = armcb_spi_of_match,
		.pm = &armcb_spi_dev_pm_ops,
	},
};

#ifndef ARMCB_CAM_KO
module_platform_driver(armcb_spi_driver);

MODULE_AUTHOR("ARMCHINA, Inc.");
MODULE_DESCRIPTION("Cadence SPI driver");
MODULE_LICENSE("GPL");
#else
static void *g_instance;

void *armcb_get_system_spi_driver_instance(void)
{
	if (platform_driver_register(&armcb_spi_driver) < 0) {
		LOG(LOG_ERR, "register spi motor driver failed.\n");
		return NULL;
	}
	g_instance = (void *)&armcb_spi_driver;
	return g_instance;
}

void armcb_system_spi_driver_destroy(void)
{
	if (g_instance)
		platform_driver_unregister(
			(struct platform_driver *)g_instance);
}
#endif
