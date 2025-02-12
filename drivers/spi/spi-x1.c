// SPDX-License-Identifier: GPL-2.0
/*
 * Support for Ky x1 spi controller
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/acpi.h>

#include "spi-x1.h"

#define TIMOUT_DFLT		3000
#define TIMOUT_DFLT_SLAVE	0x40000

//#define CONFIG_X1_SSP_DEBUG	1

static bool x1_spi_txfifo_full(const struct spi_driver_data *drv_data)
{
	return !(x1_spi_read(drv_data, STATUS) & STATUS_TNF);
}

static u32 x1_configure_topctrl(const struct spi_driver_data *drv_data, u8 bits)
{
	/*
	 * set Motorola Frame Format
	 * set DSS
	 */
	return TOP_FRF_Motorola | TOP_DSS(bits);
}

static void set_dvfm_constraint(struct spi_driver_data *drv_data)
{
#if 0
	if (drv_data->qos_idle_value != PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE)
		freq_qos_update_request(&drv_data->qos_idle,
				drv_data->qos_idle_value);
#endif
}

static void unset_dvfm_constraint(struct spi_driver_data *drv_data)
{
#if 0
	if (drv_data->qos_idle_value != PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE)
		freq_qos_update_request(&drv_data->qos_idle,
					PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
#endif
}

static void init_dvfm_constraint(struct spi_driver_data *drv_data)
{
#if 0
#ifdef CONFIG_PM
	struct freq_constraints *idle_qos;

	idle_qos = cpuidle_get_constraints();

	freq_qos_add_request(idle_qos, &drv_data->qos_idle, FREQ_QOS_MAX,
			     PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
#endif
#endif
}

static void deinit_dvfm_constraint(struct spi_driver_data *drv_data)
{
#if 0
#ifdef CONFIG_PM
	freq_qos_remove_request(&drv_data->qos_idle);
#endif
#endif
}

static void cs_assert(struct spi_driver_data *drv_data)
{
	struct chip_data *chip = drv_data->cur_chip;

	if (chip->cs_control) {
		chip->cs_control(X1_CS_ASSERT);
		return;
	}

	if (gpio_is_valid(chip->gpio_cs)) {
		gpio_set_value(chip->gpio_cs, chip->gpio_cs_inverted);
		return;
	}
}

static void cs_deassert(struct spi_driver_data *drv_data)
{
	struct chip_data *chip = drv_data->cur_chip;

	if (chip->cs_control) {
		chip->cs_control(X1_CS_DEASSERT);
		return;
	}

	if (gpio_is_valid(chip->gpio_cs)) {
		gpio_set_value(chip->gpio_cs, !chip->gpio_cs_inverted);
		return;
	}
}

/* clear all rx fifo useless data */
int x1_spi_flush(struct spi_driver_data *drv_data)
{
	unsigned long limit = loops_per_jiffy << 1;

	do {
		while (x1_spi_read(drv_data, STATUS) & STATUS_RNE)
			x1_spi_read(drv_data, DATAR);
	} while ((x1_spi_read(drv_data, STATUS) & STATUS_BSY) && --limit);
	x1_spi_write(drv_data, STATUS, STATUS_ROR);

	return limit;
}

static int null_writer(struct spi_driver_data *drv_data)
{
	u8 n_bytes = drv_data->n_bytes;

	if (x1_spi_txfifo_full(drv_data)
		|| (drv_data->tx == drv_data->tx_end))
		return 0;

	x1_spi_write(drv_data, DATAR, 0);
	drv_data->tx += n_bytes;

	return 1;
}

static int null_reader(struct spi_driver_data *drv_data)
{
	u8 n_bytes = drv_data->n_bytes;

	while ((x1_spi_read(drv_data, STATUS) & STATUS_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		x1_spi_read(drv_data, DATAR);
		drv_data->rx += n_bytes;
	}

	return drv_data->rx == drv_data->rx_end;
}

static int u8_writer(struct spi_driver_data *drv_data)
{
	if (x1_spi_txfifo_full(drv_data)
		|| (drv_data->tx == drv_data->tx_end))
		return 0;

	x1_spi_write(drv_data, DATAR, *(u8 *)(drv_data->tx));
	++drv_data->tx;

	return 1;
}

static int u8_reader(struct spi_driver_data *drv_data)
{
	while ((x1_spi_read(drv_data, STATUS) & STATUS_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u8 *)(drv_data->rx) = x1_spi_read(drv_data, DATAR);
		++drv_data->rx;
	}

	return drv_data->rx == drv_data->rx_end;
}

static int u16_writer(struct spi_driver_data *drv_data)
{
	if (x1_spi_txfifo_full(drv_data)
		|| (drv_data->tx == drv_data->tx_end))
		return 0;

	x1_spi_write(drv_data, DATAR, *(u16 *)(drv_data->tx));
	drv_data->tx += 2;

	return 1;
}

static int u16_reader(struct spi_driver_data *drv_data)
{
	while ((x1_spi_read(drv_data, STATUS) & STATUS_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u16 *)(drv_data->rx) = x1_spi_read(drv_data, DATAR);
		drv_data->rx += 2;
	}

	return drv_data->rx == drv_data->rx_end;
}

static int u32_writer(struct spi_driver_data *drv_data)
{
	if (x1_spi_txfifo_full(drv_data)
		|| (drv_data->tx == drv_data->tx_end))
		return 0;

	x1_spi_write(drv_data, DATAR, *(u32 *)(drv_data->tx));
	drv_data->tx += 4;

	return 1;
}

static int u32_reader(struct spi_driver_data *drv_data)
{
	while ((x1_spi_read(drv_data, STATUS) & STATUS_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u32 *)(drv_data->rx) = x1_spi_read(drv_data, DATAR);
		drv_data->rx += 4;
	}

	return drv_data->rx == drv_data->rx_end;
}

void *x1_spi_next_transfer(struct spi_driver_data *drv_data)
{
	struct spi_message *msg = drv_data->cur_msg;
	struct spi_transfer *trans = drv_data->cur_transfer;

	/* Move to next transfer */
	if (trans->transfer_list.next != &msg->transfers) {
		drv_data->cur_transfer =
			list_entry(trans->transfer_list.next,
					struct spi_transfer,
					transfer_list);
		return RUNNING_STATE;
	} else
		return DONE_STATE;
}

/* caller already set message->status; dma and pio irqs are blocked */
static void giveback(struct spi_driver_data *drv_data)
{
	struct spi_transfer* last_transfer;
	struct spi_message *msg;

	msg = drv_data->cur_msg;
	drv_data->cur_msg = NULL;
	drv_data->cur_transfer = NULL;

	last_transfer = list_last_entry(&msg->transfers, struct spi_transfer,
					transfer_list);

	/* Delay if requested before any change in chip select */
	spi_transfer_delay_exec(last_transfer);

	/* Drop chip select UNLESS cs_change is true or we are returning
	 * a message with an error, or next message is for another chip
	 */
	if (!last_transfer->cs_change)
		cs_deassert(drv_data);
	else {
		struct spi_message *next_msg;

		/* Holding of cs was hinted, but we need to make sure
		 * the next message is for the same chip.  Don't waste
		 * time with the following tests unless this was hinted.
		 *
		 * We cannot postpone this until pump_messages, because
		 * after calling msg->complete (below) the driver that
		 * sent the current message could be unloaded, which
		 * could invalidate the cs_control() callback...
		 */

		/* get a pointer to the next message, if any */
		next_msg = spi_get_next_queued_message(drv_data->master);

		/* see if the next and current messages point
		 * to the same chip
		 */
		if (next_msg && next_msg->spi != msg->spi)
			next_msg = NULL;
		if (!next_msg || msg->state == ERROR_STATE)
			cs_deassert(drv_data);
	}

	drv_data->cur_chip = NULL;
	spi_finalize_current_message(drv_data->master);
	unset_dvfm_constraint(drv_data);

	if (drv_data->slave_mode)
		del_timer(&drv_data->slave_rx_timer);
	complete(&drv_data->cur_msg_completion);
}

static void reset_fifo_ctrl(struct spi_driver_data *drv_data)
{
	struct chip_data *chip = drv_data->cur_chip;
	u32 fifo_ctrl = 0;

	fifo_ctrl |= chip->threshold;
	x1_spi_write(drv_data, FIFO_CTRL, fifo_ctrl);
}

static void reset_int_en(struct spi_driver_data *drv_data)
{
	u32 int_en = 0;

	int_en = x1_spi_read(drv_data, INT_EN);
	int_en &= ~drv_data->int_cr;
	x1_spi_write(drv_data, INT_EN, int_en);
}

static void int_error_stop(struct spi_driver_data *drv_data, const char* msg)
{
	/* Stop and reset SSP */
	x1_spi_write(drv_data, STATUS, drv_data->clear_sr);
	reset_fifo_ctrl(drv_data);
	reset_int_en(drv_data);
	x1_spi_write(drv_data, TO, 0);
	x1_spi_flush(drv_data);
	x1_spi_write(drv_data, TOP_CTRL,
			 x1_spi_read(drv_data, TOP_CTRL) & ~(TOP_SSE | TOP_HOLD_FRAME_LOW));
	dev_err(&drv_data->pdev->dev, "%s\n", msg);

	drv_data->cur_msg->state = ERROR_STATE;
	queue_work(system_wq, &drv_data->pump_transfers);
}

static void int_transfer_complete(struct spi_driver_data *drv_data)
{
	/* Stop SSP */
	x1_spi_write(drv_data, STATUS, drv_data->clear_sr);
	reset_fifo_ctrl(drv_data);
	reset_int_en(drv_data);
	x1_spi_write(drv_data, TO, 0);

	/* Update total byte transferred return count actual bytes read */
	drv_data->cur_msg->actual_length += drv_data->len -
				(drv_data->rx_end - drv_data->rx);

	/* Transfer delays and chip select release are
	 * handled in pump_transfers or giveback
	 */

	/* Move to next transfer */
	drv_data->cur_msg->state = x1_spi_next_transfer(drv_data);

	/* Schedule transfer tasklet */
	queue_work(system_wq, &drv_data->pump_transfers);
}

static irqreturn_t interrupt_transfer(struct spi_driver_data *drv_data)
{
	u32 irq_mask = (x1_spi_read(drv_data, INT_EN) & INT_EN_TIE) ?
		       drv_data->mask_sr : drv_data->mask_sr & ~STATUS_TFS;

	u32 irq_status = x1_spi_read(drv_data, STATUS) & irq_mask;

	if (irq_status & STATUS_ROR) {
		int_error_stop(drv_data, "interrupt_transfer: fifo overrun");
		return IRQ_HANDLED;
	}

	if (irq_status & STATUS_TINT) {
		x1_spi_write(drv_data, STATUS, STATUS_TINT);
		if (drv_data->read(drv_data)) {
			int_transfer_complete(drv_data);
			return IRQ_HANDLED;
		}
	}

	/* Drain rx fifo, Fill tx fifo and prevent overruns */
	do {
		if (drv_data->read(drv_data)) {
			int_transfer_complete(drv_data);
			return IRQ_HANDLED;
		}
	} while (drv_data->write(drv_data));

	if (drv_data->read(drv_data)) {
		int_transfer_complete(drv_data);
		return IRQ_HANDLED;
	}

	if (drv_data->tx == drv_data->tx_end) {
		u32 int_en;

		int_en = x1_spi_read(drv_data, INT_EN);
		int_en &= ~INT_EN_TIE;

		x1_spi_write(drv_data, INT_EN, int_en);
	}

	/* We did something */
	return IRQ_HANDLED;
}

static irqreturn_t ssp_int(int irq, void *dev_id)
{
	struct spi_driver_data *drv_data = dev_id;
	u32 int_en;
	u32 mask = drv_data->mask_sr;
	u32 int_status;

	/*
	 * The IRQ might be shared with other peripherals so we must first
	 * check that are we RPM suspended or not. If we are we assume that
	 * the IRQ was not for us (we shouldn't be RPM suspended when the
	 * interrupt is enabled).
	 */
	if (pm_runtime_suspended(&drv_data->pdev->dev))
		return IRQ_NONE;

	/*
	 * If the device is not yet in RPM suspended state and we get an
	 * interrupt that is meant for another device, check if status bits
	 * are all set to one. That means that the device is already
	 * powered off.
	 */
	int_status = x1_spi_read(drv_data, STATUS);
	if (int_status == ~0)
		return IRQ_NONE;

	int_en = x1_spi_read(drv_data, INT_EN);

	/* Ignore possible writes if we don't need to write */
	if (!(int_en & INT_EN_TIE))
		mask &= ~STATUS_TFS;

	/* Ignore RX timeout interrupt if it is disabled */
	if (!(int_en & INT_EN_TINTE))
		mask &= ~STATUS_TINT;

	if (!(int_status & mask))
		return IRQ_NONE;

	if (!drv_data->cur_msg) {

		x1_spi_write(drv_data, TOP_CTRL,
				 x1_spi_read(drv_data, TOP_CTRL)
				 & ~(TOP_SSE | TOP_HOLD_FRAME_LOW));
		x1_spi_write(drv_data, INT_EN,
				 x1_spi_read(drv_data, INT_EN)
				 & ~drv_data->int_cr);
		x1_spi_write(drv_data, TO, 0);
		x1_spi_write(drv_data, STATUS, drv_data->clear_sr);

		dev_err(&drv_data->pdev->dev,
			"bad message state in interrupt handler\n");

		/* Never fail */
		return IRQ_HANDLED;
	}

	return drv_data->transfer_handler(drv_data);
}

static void slave_rx_timer_expired(struct timer_list *t) {
	struct spi_driver_data *drv_data = from_timer(drv_data, t, slave_rx_timer);
#ifdef CONFIG_X1_SSP_DEBUG
	pr_err("%s\n", __func__);
	pr_err("spi top = 0x%x\n", x1_spi_read(drv_data, TOP_CTRL));
	pr_err("fifo = 0x%x\n", x1_spi_read(drv_data, FIFO_CTRL));
	pr_err("int_en = 0x%x\n", x1_spi_read(drv_data, INT_EN));
	pr_err("to = 0x%x\n", x1_spi_read(drv_data, TO));
#endif
	x1_spi_slave_sw_timeout_callback(drv_data);
}

static void pump_transfers(struct work_struct *work)
{
	struct spi_driver_data *drv_data = container_of(work, struct spi_driver_data, pump_transfers);
	struct spi_message *message = NULL;
	struct spi_transfer *transfer = NULL;
	struct spi_transfer *previous = NULL;
	struct chip_data *chip = NULL;
	u8 bits = 0;
	u32 top_ctrl;
	u32 fifo_ctrl;
	u32 int_en = 0;
	u32 dma_thresh = drv_data->cur_chip->dma_threshold;
	u32 dma_burst = drv_data->cur_chip->dma_burst_size;

	if (drv_data->slave_mode)
		mod_timer(&drv_data->slave_rx_timer,
				jiffies + msecs_to_jiffies(1000));

	/* Get current state information */
	message = drv_data->cur_msg;
	transfer = drv_data->cur_transfer;
	chip = drv_data->cur_chip;

	/* Handle for abort */
	if (message->state == ERROR_STATE) {
		message->status = -EIO;
		giveback(drv_data);
		return;
	}

	/* Handle end of message */
	if (message->state == DONE_STATE) {
		message->status = 0;
		giveback(drv_data);
		return;
	}

	/* Delay if requested at end of transfer before CS change */
	if (message->state == RUNNING_STATE) {
		previous = list_entry(transfer->transfer_list.prev,
					struct spi_transfer,
					transfer_list);
		spi_transfer_delay_exec(previous);

		/* Drop chip select only if cs_change is requested */
		if (previous->cs_change)
			cs_deassert(drv_data);
	}

	/* Check if we can DMA this transfer */
	if (!x1_spi_dma_is_possible(transfer->len) && chip->enable_dma) {
		/* reject already-mapped transfers; PIO won't always work */
		if (message->is_dma_mapped
				|| transfer->rx_dma || transfer->tx_dma) {
			dev_err(&drv_data->pdev->dev,
				"pump_transfers: mapped transfer length of "
				"%u is greater than %d\n",
				transfer->len, MAX_DMA_LEN);
			message->status = -EINVAL;
			giveback(drv_data);
			return;
		}

		/* warn ... we force this to PIO mode */
		dev_warn_ratelimited(&message->spi->dev,
				     "pump_transfers: DMA disabled for transfer length %ld "
				     "greater than %d\n",
				     (long)drv_data->len, MAX_DMA_LEN);
	}

	/* Setup the transfer state based on the type of transfer */
	if (x1_spi_flush(drv_data) == 0) {
		dev_err(&drv_data->pdev->dev, "pump_transfers: flush failed\n");
		message->status = -EIO;
		giveback(drv_data);
		return;
	}
	drv_data->n_bytes = chip->n_bytes;
	drv_data->tx = (void *)transfer->tx_buf;
	drv_data->tx_end = drv_data->tx + transfer->len;
	drv_data->rx = transfer->rx_buf;
	drv_data->rx_end = drv_data->rx + transfer->len;
	drv_data->rx_dma = transfer->rx_dma;
	drv_data->tx_dma = transfer->tx_dma;
	drv_data->len = transfer->len;
	drv_data->write = drv_data->tx ? chip->write : null_writer;
	drv_data->read = drv_data->rx ? chip->read : null_reader;

	/* Change speed and bit per word on a per transfer */
	bits = transfer->bits_per_word;

	if (bits <= 8) {
		drv_data->n_bytes = 1;
		drv_data->read = drv_data->read != null_reader ?
					u8_reader : null_reader;
		drv_data->write = drv_data->write != null_writer ?
					u8_writer : null_writer;
	} else if (bits <= 16) {
		drv_data->n_bytes = 2;
		drv_data->read = drv_data->read != null_reader ?
					u16_reader : null_reader;
		drv_data->write = drv_data->write != null_writer ?
					u16_writer : null_writer;
	} else if (bits <= 32) {
		drv_data->n_bytes = 4;
		drv_data->read = drv_data->read != null_reader ?
					u32_reader : null_reader;
		drv_data->write = drv_data->write != null_writer ?
					u32_writer : null_writer;
	}
	/*
	 * if bits/word is changed in dma mode, then must check the
	 * thresholds and burst also
	 */
	if (chip->enable_dma) {
		if (x1_spi_set_dma_burst_and_threshold(chip,
						message->spi,
						bits, &dma_burst,
						&dma_thresh))
			dev_warn_ratelimited(&message->spi->dev,
					     "pump_transfers: DMA burst size reduced to match bits_per_word\n");
	}

	top_ctrl = x1_configure_topctrl(drv_data, bits);
		dev_dbg(&message->spi->dev, "%u Hz, %s\n",
			drv_data->master->max_speed_hz,
			chip->enable_dma ? "DMA" : "PIO");
	top_ctrl |= chip->top_ctrl;
	fifo_ctrl = chip->fifo_ctrl;

	if (drv_data->ssp_enhancement) {
		/*
		 * If transfer length is times of 4, then use
		 * 32 bit fifo width with endian swap support
		 */
		if (drv_data->len % 4 == 0 && transfer->bits_per_word <= 16) {
			if (transfer->bits_per_word <= 8)
				fifo_ctrl |=  FIFO_WR_ENDIAN_8BITS |
					FIFO_RD_ENDIAN_8BITS;
			else if (transfer->bits_per_word <= 16)
				fifo_ctrl |= FIFO_WR_ENDIAN_16BITS |
					FIFO_RD_ENDIAN_16BITS;
			bits = 32;
			drv_data->n_bytes = 4;
			if(transfer->rx_buf)
				drv_data->read = u32_reader;
			if(transfer->tx_buf)
				drv_data->write = u32_writer;

			if (chip->enable_dma) {
				if (x1_spi_set_dma_burst_and_threshold(chip,
							message->spi,
							bits, &dma_burst,
							&dma_thresh))
					dev_warn_ratelimited(&message->spi->dev,
							"pump_transfers:"
							"DMA burst size reduced to"
							"match bits_per_word\n");
			}

			top_ctrl &= ~TOP_DSS_MASK;
			top_ctrl |= TOP_DSS(32);
		}
	}

	message->state = RUNNING_STATE;

	drv_data->dma_mapped = 0;
	if (x1_spi_dma_is_possible(drv_data->len))
		drv_data->dma_mapped = x1_spi_map_dma_buffers(drv_data);
	if (drv_data->dma_mapped) {
		/* Ensure we have the correct interrupt handler */
		drv_data->transfer_handler = x1_spi_dma_transfer;

		x1_spi_dma_prepare(drv_data, dma_burst);

		/* Clear status and start DMA engine */
		fifo_ctrl |= chip->fifo_ctrl | dma_thresh | drv_data->dma_fifo_ctrl;
		top_ctrl |= chip->top_ctrl | drv_data->dma_top_ctrl;
		x1_spi_write(drv_data, STATUS, drv_data->clear_sr);
		x1_spi_dma_start(drv_data);
		int_en = x1_spi_read(drv_data, INT_EN) | drv_data->dma_cr;
	} else {
		/* Ensure we have the correct interrupt handler	*/
		drv_data->transfer_handler = interrupt_transfer;

		fifo_ctrl = fifo_ctrl | chip->fifo_ctrl | chip->threshold;
		int_en = x1_spi_read(drv_data, INT_EN) | drv_data->int_cr;
		x1_spi_write(drv_data, STATUS, drv_data->clear_sr);
	}

	x1_spi_write(drv_data, TO, chip->timeout);

	cs_assert(drv_data);

	/*
	 * TODO: refine these logic
	 * x1_spi_get_ssrc1_change_mask
	 * if ((x1_spi_read(drv_data, SSCR0) != cr0)
	 * cs_assert(drv_data);
	 * x1_spi_write(drv_data, SSCR1, cr1);
	 */

	set_dvfm_constraint(drv_data);	/*disable system to idle while DMA */
	if (drv_data->slave_mode)
		top_ctrl |= TOP_SSE | TOP_SCLKDIR | TOP_SFRMDIR;
	else
		top_ctrl |= TOP_HOLD_FRAME_LOW;
	/*
	 * This part changed the logic
	 * 1. clear SSE
	 * 2. write TOP_CTRL and other register
	 * 3. set SSE in the end of this function
	 */
	top_ctrl &= ~TOP_SSE;
	x1_spi_write(drv_data, TOP_CTRL, top_ctrl);
	x1_spi_write(drv_data, FIFO_CTRL, fifo_ctrl);
	x1_spi_write(drv_data, INT_EN, int_en);
	top_ctrl |= TOP_SSE;
#ifdef CONFIG_X1_SSP_DEBUG
	dev_err(&message->spi->dev, "spi top = 0x%x\n", top_ctrl);
	dev_err(&message->spi->dev, "fifo = 0x%x\n", x1_spi_read(drv_data, FIFO_CTRL));
	dev_err(&message->spi->dev, "int_en = 0x%x\n", x1_spi_read(drv_data, INT_EN));
	dev_err(&message->spi->dev, "to = 0x%x\n", x1_spi_read(drv_data, TO));
#endif
	x1_spi_write(drv_data, TOP_CTRL, top_ctrl);
}

static int x1_spi_transfer_one_message(struct spi_master *master,
					   struct spi_message *msg)
{
	struct spi_driver_data *drv_data = spi_master_get_devdata(master);

	drv_data->cur_msg = msg;
	/* Initial message state*/
	drv_data->cur_msg->state = START_STATE;
	drv_data->cur_transfer = list_entry(drv_data->cur_msg->transfers.next,
						struct spi_transfer,
						transfer_list);

	/*
	 * prepare to setup the SSP, in pump_transfers, using the per
	 * chip configuration
	 */
	drv_data->cur_chip = spi_get_ctldata(drv_data->cur_msg->spi);

	if (master->max_speed_hz != drv_data->cur_transfer->speed_hz) {
		master->max_speed_hz = drv_data->cur_transfer->speed_hz;
		clk_set_rate(drv_data->clk, master->max_speed_hz);
	}

	reinit_completion(&drv_data->cur_msg_completion);
	/* Mark as busy and launch transfers */
	queue_work(system_wq, &drv_data->pump_transfers);
	wait_for_completion(&drv_data->cur_msg_completion);

	return 0;
}

static int x1_spi_unprepare_transfer(struct spi_master *master)
{
	struct spi_driver_data *drv_data = spi_master_get_devdata(master);

	/* Disable the SSP now */
	x1_spi_write(drv_data, TOP_CTRL,
			 x1_spi_read(drv_data, TOP_CTRL) & ~(TOP_SSE | TOP_HOLD_FRAME_LOW));

	return 0;
}

static int setup_cs(struct spi_device *spi, struct chip_data *chip)
{
	int err = 0;

	if (chip == NULL)
		return 0;
	return err;
}

static int setup(struct spi_device *spi)
{
	struct chip_data *chip;
	struct spi_driver_data *drv_data = spi_master_get_devdata(spi->master);
	uint tx_thres, tx_hi_thres, rx_thres;

	tx_thres = TX_THRESH_DFLT;
	tx_hi_thres = 0;
	rx_thres = RX_THRESH_DFLT;

	/* Only alloc on first setup */
	chip = spi_get_ctldata(spi);
	if (!chip) {
		chip = devm_kzalloc(&spi->master->dev, sizeof(struct chip_data),
				GFP_KERNEL);
		if (!chip)
			return -ENOMEM;

		chip->gpio_cs = -1;
		chip->enable_dma = 0;
		chip->timeout =
			drv_data->slave_mode ? TIMOUT_DFLT_SLAVE : TIMOUT_DFLT;
	}

	chip->top_ctrl = 0;
	chip->fifo_ctrl = 0;

	chip->enable_dma = drv_data->master_info->enable_dma;
	if (drv_data->slave_mode)
		chip->dma_burst_size = 32;

	if (chip->enable_dma) {
		/* set up legal burst and threshold for dma */
		if (x1_spi_set_dma_burst_and_threshold(chip, spi,
						spi->bits_per_word,
						&chip->dma_burst_size,
						&chip->dma_threshold)) {
			dev_warn(&spi->dev,
					"in setup: DMA burst size reduced to match bits_per_word\n");
		}
	}
	chip->threshold = (FIFO_RxTresh(rx_thres) & FIFO_RFT) |
		(FIFO_TxTresh(tx_thres) & FIFO_TFT);

	chip->top_ctrl &= ~(TOP_SPO | TOP_SPH);
	chip->top_ctrl |= (((spi->mode & SPI_CPHA) != 0) ? TOP_SPH : 0)
			| (((spi->mode & SPI_CPOL) != 0) ? TOP_SPO : 0);

	if (spi->mode & SPI_LOOP)
		chip->top_ctrl |= TOP_LBM;

	/* Enable rx fifo auto full control */
	if (drv_data->ssp_enhancement)
		chip->fifo_ctrl |= FIFO_RXFIFO_AUTO_FULL_CTRL;

	if (spi->bits_per_word <= 8) {
		chip->n_bytes = 1;
		chip->read = u8_reader;
		chip->write = u8_writer;
	} else if (spi->bits_per_word <= 16) {
		chip->n_bytes = 2;
		chip->read = u16_reader;
		chip->write = u16_writer;
	} else if (spi->bits_per_word <= 32) {
		chip->n_bytes = 4;
		chip->read = u32_reader;
		chip->write = u32_writer;
	}

	if (spi->master->max_speed_hz != spi->max_speed_hz) {
		spi->master->max_speed_hz = spi->max_speed_hz;
		clk_set_rate(drv_data->clk, spi->master->max_speed_hz);
	}

	spi_set_ctldata(spi, chip);

	return setup_cs(spi, chip);
}

static void cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	if (!chip)
		return;

	if (gpio_is_valid(chip->gpio_cs))
		gpio_free(chip->gpio_cs);

	devm_kfree(&spi->dev, chip);
}

static const struct of_device_id x1_spi_dt_ids[] = {
	{ .compatible = "ky,x1-spi", .data = (void *) X1_SSP },
	{}
};
MODULE_DEVICE_TABLE(of, x1_spi_dt_ids);

static int x1_spi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct x1_spi_master *platform_info;
	struct spi_master *master = NULL;
	struct spi_driver_data *drv_data = NULL;
	struct device_node *np = dev->of_node;
	const struct of_device_id *id =
		of_match_device(of_match_ptr(x1_spi_dt_ids), dev);
	struct resource *iores;
	u32 bus_num;
#if 0
	const __be32 *prop;
	unsigned int proplen;
#endif
    int status;
	u32 tmp;

	platform_info = dev_get_platdata(dev);
	if (!platform_info) {
		platform_info = devm_kzalloc(dev, sizeof(*platform_info),
				GFP_KERNEL);
		if (!platform_info)
			return -ENOMEM;
		platform_info->num_chipselect = 1;
		/* TODO: NO DMA on FPGA yet */
		if (of_get_property(np, "x1,ssp-disable-dma", NULL))
			platform_info->enable_dma = 0;
		else
			platform_info->enable_dma = 1;
	}

	master = spi_alloc_master(dev, sizeof(struct spi_driver_data));
	if (!master) {
		dev_err(&pdev->dev, "cannot alloc spi_master\n");
		return -ENOMEM;
	}
	drv_data = spi_master_get_devdata(master);

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (iores == NULL) {
		dev_err(dev, "no memory resource defined\n");
		status = -ENODEV;
		goto out_error_master_alloc;
	}

	drv_data->ioaddr = devm_ioremap_resource(dev, iores);
	if (drv_data->ioaddr == NULL) {
		dev_err(dev, "failed to ioremap() registers\n");
		status = -ENODEV;
		goto out_error_master_alloc;
	}

	drv_data->irq = platform_get_irq(pdev, 0);
	if (drv_data->irq < 0) {
		dev_err(dev, "no IRQ resource defined\n");
		status = -ENODEV;
		goto out_error_master_alloc;
	}

	/* Receive FIFO auto full ctrl enable */
	if (of_get_property(np, "x1,ssp-enhancement", NULL))
		drv_data->ssp_enhancement = 1;

	if (of_get_property(np, "x1,ssp-slave-mode", NULL)) {
		drv_data->slave_mode = 1;
		dev_warn(&pdev->dev, "slave mode\n");
		timer_setup(&drv_data->slave_rx_timer,
				slave_rx_timer_expired, 0);
	}

#if 0
	prop = of_get_property(dev->of_node, "x1,ssp-lpm-qos", &proplen);
	if (!prop) {
		dev_err(&pdev->dev, "lpm-qos for spi is not defined!\n");
		status = -EINVAL;
		goto out_error_master_alloc;
	} else
		drv_data->qos_idle_value = be32_to_cpup(prop);
#endif

	init_dvfm_constraint(drv_data);

	master->dev.of_node = dev->of_node;
	drv_data->ssp_type = (uintptr_t) id->data;
	if (!of_property_read_u32(np, "x1,ssp-id", &bus_num))
		master->bus_num = bus_num;
	drv_data->ssdr_physical = iores->start + DATAR;

	drv_data->clk = devm_clk_get(dev, NULL);
	if (IS_ERR_OR_NULL(drv_data->clk)) {
		dev_err(&pdev->dev, "cannot get clk\n");
		status = -ENODEV;
		goto out_error_clk_check;
	}

	drv_data->reset = devm_reset_control_get_optional(dev, NULL);
	if (IS_ERR_OR_NULL(drv_data->reset)) {
		dev_err(&pdev->dev, "Failed to get spi's reset\n");
		status = -ENODEV;
		goto out_error_clk_check;
	}

	drv_data->master = master;
	drv_data->master_info = platform_info;
	drv_data->pdev = pdev;

	master->dev.parent = &pdev->dev;
	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LOOP;

	master->dma_alignment = DMA_ALIGNMENT;
	master->cleanup = cleanup;
	master->setup = setup;
	master->transfer_one_message = x1_spi_transfer_one_message;
	master->unprepare_transfer_hardware = x1_spi_unprepare_transfer;
	master->auto_runtime_pm = true;

	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(4, 32);
	drv_data->int_cr = INT_EN_TIE | INT_EN_RIE | INT_EN_TINTE; /* INT_EN */
	drv_data->dma_cr = (drv_data->slave_mode) ? INT_EN_TINTE : 0;
	drv_data->clear_sr = STATUS_ROR | STATUS_TINT;
	drv_data->mask_sr = STATUS_TINT | STATUS_RFS | STATUS_TFS | STATUS_ROR;
	drv_data->dma_top_ctrl = DEFAULT_DMA_TOP_CTRL;
	drv_data->dma_fifo_ctrl = DEFAULT_DMA_FIFO_CTRL;

	status = devm_request_irq(&pdev->dev, drv_data->irq, ssp_int, IRQF_SHARED, dev_name(dev),
			drv_data);
	if (status < 0) {
		dev_err(&pdev->dev, "cannot get IRQ %d\n", drv_data->irq);
		goto out_error_master_alloc;
	}

	/* Setup DMA if requested */
	if (platform_info->enable_dma) {
		status = x1_spi_dma_setup(drv_data);
		if (status) {
			dev_dbg(dev, "no DMA channels available, using PIO\n");
			platform_info->enable_dma = false;
		}
	}

	status = of_property_read_u32(np, "x1,ssp-clock-rate", &master->max_speed_hz);
	if (status < 0) {
		dev_err(&pdev->dev, "cannot get clock-rate from DT file\n");
		goto out_error_master_alloc;
	}

	clk_set_rate(drv_data->clk, master->max_speed_hz);
	master->max_speed_hz = clk_get_rate(drv_data->clk);
	clk_prepare_enable(drv_data->clk);
    	reset_control_deassert(drv_data->reset);

	/* Load default SSP configuration */
	x1_spi_write(drv_data, TOP_CTRL, 0);
	x1_spi_write(drv_data, FIFO_CTRL, 0);
	tmp = FIFO_RxTresh(RX_THRESH_DFLT) |
	      FIFO_TxTresh(TX_THRESH_DFLT);
	x1_spi_write(drv_data, FIFO_CTRL, tmp);
	tmp = TOP_FRF_Motorola | TOP_DSS(8);
	x1_spi_write(drv_data, TOP_CTRL, tmp);
	x1_spi_write(drv_data, TO, 0);

	x1_spi_write(drv_data, PSP_CTRL, 0);

	master->num_chipselect = platform_info->num_chipselect;

	INIT_WORK(&drv_data->pump_transfers, pump_transfers);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	init_completion(&drv_data->cur_msg_completion);

	/* Register with the SPI framework */
	platform_set_drvdata(pdev, drv_data);
	status = devm_spi_register_master(&pdev->dev, master);
	if (status != 0) {
		dev_err(&pdev->dev, "problem registering spi master\n");
		goto out_error_clock_enabled;
	}

	return status;

out_error_clock_enabled:
    	reset_control_assert(drv_data->reset);
	clk_disable_unprepare(drv_data->clk);
	x1_spi_dma_release(drv_data);
	free_irq(drv_data->irq, drv_data);
out_error_clk_check:
	deinit_dvfm_constraint(drv_data);
out_error_master_alloc:
	spi_master_put(master);
	return status;
}

static int x1_spi_remove(struct platform_device *pdev)
{
	struct spi_driver_data *drv_data = platform_get_drvdata(pdev);

	if (!drv_data)
		return 0;

	pm_runtime_get_sync(&pdev->dev);

	/* Disable the SSP at the peripheral and SOC level */
	x1_spi_write(drv_data, TOP_CTRL, 0);
	x1_spi_write(drv_data, FIFO_CTRL, 0); /* whether need this line? */

    	reset_control_assert(drv_data->reset);
	clk_disable_unprepare(drv_data->clk);

	/* Release DMA */
	if (drv_data->master_info->enable_dma)
		x1_spi_dma_release(drv_data);

	pm_runtime_put_noidle(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	/* Release IRQ */
	free_irq(drv_data->irq, drv_data);

	deinit_dvfm_constraint(drv_data);
	return 0;
}

static void x1_spi_shutdown(struct platform_device *pdev)
{
	int status = 0;

	if ((status = x1_spi_remove(pdev)) != 0)
		dev_err(&pdev->dev, "shutdown failed with %d\n", status);
}

#ifdef CONFIG_PM_SLEEP
static int x1_spi_suspend(struct device *dev)
{
	struct spi_driver_data *drv_data = dev_get_drvdata(dev);
	int status = 0;

	pm_runtime_get_sync(dev);
	status = spi_master_suspend(drv_data->master);
	if (status != 0)
		return status;
	x1_spi_write(drv_data, TOP_CTRL, 0);
	x1_spi_write(drv_data, FIFO_CTRL, 0); /* whether need this line? */

	status = pm_runtime_force_suspend(dev);

	return status;
}

static int x1_spi_resume(struct device *dev)
{
	struct spi_driver_data *drv_data = dev_get_drvdata(dev);
	int status = 0;

	/* Enable the SSP clock */
	status = pm_runtime_force_resume(dev);
	if (status) {
		dev_err(dev, "failed to resume pm_runtime (%d)\n", status);
		return status;
	}

	/* Start the queue running */
	status = spi_master_resume(drv_data->master);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	if (status != 0) {
		dev_err(dev, "problem starting queue (%d)\n", status);
		return status;
	}

	return 0;
}
#endif

#ifdef CONFIG_PM
/** static int x1_spi_runtime_suspend(struct device *dev)
 * {
 *	struct spi_driver_data *drv_data = dev_get_drvdata(dev);
 *
 *	reset_control_assert(drv_data->reset);
 *	clk_disable_unprepare(drv_data->clk);
 *
 *	return 0;
 *}
 */

/**
 * static int x1_spi_runtime_resume(struct device *dev)
 * {
 *	struct spi_driver_data *drv_data = dev_get_drvdata(dev);
 *
 *	clk_prepare_enable(drv_data->clk);
 *	reset_control_deassert(drv_data->reset);
 *	return 0;
 *}
 */
#endif

static const struct dev_pm_ops x1_spi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(x1_spi_suspend, x1_spi_resume)
	/**
	 * SET_RUNTIME_PM_OPS(x1_spi_runtime_suspend,
	 *		   x1_spi_runtime_resume, NULL)
	 */
};

static struct platform_driver driver = {
	.driver = {
		.name	= "x1-spi",
		.pm	= &x1_spi_pm_ops,
		.of_match_table = x1_spi_dt_ids,
	},
	.probe = x1_spi_probe,
	.remove = x1_spi_remove,
	.shutdown = x1_spi_shutdown,
};

static int __init x1_spi_init(void)
{
	return platform_driver_register(&driver);
}
module_init(x1_spi_init);

static void __exit x1_spi_exit(void)
{
	platform_driver_unregister(&driver);
}
module_exit(x1_spi_exit);

MODULE_AUTHOR("Ky");
MODULE_DESCRIPTION("Ky x1 spi controller driver");
MODULE_LICENSE("GPL v2");
