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
#include "system_i2c.h"
#include "armcb_isp.h"
#include "system_logger.h"
#include <linux/version.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

/**
 * armcb_i2c_clear_bus_hold - Clear bus hold bit
 * @id:	Pointer to driver data struct
 *
 * Helper to clear the controller's bus hold bit.
 */
static void armcb_i2c_clear_bus_hold(struct armcb_i2c *id)
{
	u32 reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);

	if (reg & ARMCB_I2C_CR_HOLD)
		armcb_i2c_writereg(reg & ~ARMCB_I2C_CR_HOLD,
				   ARMCB_I2C_CR_OFFSET);
}

static inline bool armcb_is_holdquirk(struct armcb_i2c *id, bool hold_wrkaround)
{
	return (hold_wrkaround &&
		(id->curr_recv_count == ARMCB_I2C_FIFO_DEPTH + 1));
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static void armcb_i2c_set_mode(enum armcb_i2c_mode mode, struct armcb_i2c *id)
{
	u32 reg;

	/* Disable all interrupts */
	armcb_i2c_writereg(ARMCB_I2C_IXR_ALL_INTR_MASK, ARMCB_I2C_IDR_OFFSET);

	/* Clear FIFO and transfer size */
	armcb_i2c_writereg(ARMCB_I2C_CR_CLR_FIFO, ARMCB_I2C_CR_OFFSET);

	/* Update device mode and state */
	id->dev_mode = mode;
	id->slave_state = ARMCB_I2C_SLAVE_STATE_IDLE;

	switch (mode) {
	case ARMCB_I2C_MODE_MASTER:
		/* Enable i2c master */
		armcb_i2c_writereg(id->ctrl_reg, ARMCB_I2C_CR_OFFSET);
		break;
	case ARMCB_I2C_MODE_SLAVE:
		/* Enable i2c slave */
		reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
		reg &= ~ARMCB_I2C_CR_SLAVE_EN_MASK;
		armcb_i2c_writereg(reg, ARMCB_I2C_CR_OFFSET);

		/* Setting slave address */
		armcb_i2c_writereg(id->slave->addr & ARMCB_I2C_ADDR_MASK,
				   ARMCB_I2C_ADDR_OFFSET);

		/* Enable slave send/receive interrupts */
		armcb_i2c_writereg(ARMCB_I2C_IXR_SLAVE_INTR_MASK,
				   ARMCB_I2C_IER_OFFSET);
		break;
	}
}

static void armcb_i2c_slave_rcv_data(struct armcb_i2c *id)
{
	u8 bytes;
	unsigned char data;

	/* Prepare backend for data reception */
	if (id->slave_state == ARMCB_I2C_SLAVE_STATE_IDLE) {
		id->slave_state = ARMCB_I2C_SLAVE_STATE_RECV;
		i2c_slave_event(id->slave, I2C_SLAVE_WRITE_REQUESTED, NULL);
	}

	/* Fetch number of bytes to receive */
	bytes = armcb_i2c_readreg(ARMCB_I2C_XFER_SIZE_OFFSET);

	/* Read data and send to backend */
	while (bytes--) {
		data = armcb_i2c_readreg(ARMCB_I2C_DATA_OFFSET);
		i2c_slave_event(id->slave, I2C_SLAVE_WRITE_RECEIVED, &data);
	}
}

static void armcb_i2c_slave_send_data(struct armcb_i2c *id)
{
	u8 data;

	/* Prepare backend for data transmission */
	if (id->slave_state == ARMCB_I2C_SLAVE_STATE_IDLE) {
		id->slave_state = ARMCB_I2C_SLAVE_STATE_SEND;
		i2c_slave_event(id->slave, I2C_SLAVE_READ_REQUESTED, &data);
	} else {
		i2c_slave_event(id->slave, I2C_SLAVE_READ_PROCESSED, &data);
	}

	/* Send data over bus */
	armcb_i2c_writereg(data, ARMCB_I2C_DATA_OFFSET);
}

/**
 * armcb_i2c_slave_isr - Interrupt handler for the I2C device in slave role
 * @ptr:	   Pointer to I2C device private data
 *
 * This function handles the data interrupt and transfer complete interrupt of
 * the I2C device in slave role.
 *
 * Return: IRQ_HANDLED always
 */
static irqreturn_t armcb_i2c_slave_isr(void *ptr)
{
	struct armcb_i2c *id = ptr;
	unsigned int isr_status, i2c_status;

	/* Fetch the interrupt status */
	isr_status = armcb_i2c_readreg(ARMCB_I2C_ISR_OFFSET);
	armcb_i2c_writereg(isr_status, ARMCB_I2C_ISR_OFFSET);

	/* Ignore masked interrupts */
	isr_status &= ~armcb_i2c_readreg(ARMCB_I2C_IMR_OFFSET);

	/* Fetch transfer mode (send/receive) */
	i2c_status = armcb_i2c_readreg(ARMCB_I2C_SR_OFFSET);

	/* Handle data send/receive */
	if (i2c_status & ARMCB_I2C_SR_RXRW) {
		/* Send data to master */
		if (isr_status & ARMCB_I2C_IXR_DATA)
			armcb_i2c_slave_send_data(id);

		if (isr_status & ARMCB_I2C_IXR_COMP) {
			id->slave_state = ARMCB_I2C_SLAVE_STATE_IDLE;
			i2c_slave_event(id->slave, I2C_SLAVE_STOP, NULL);
		}
	} else {
		/* Receive data from master */
		if (isr_status & ARMCB_I2C_IXR_DATA)
			armcb_i2c_slave_rcv_data(id);

		if (isr_status & ARMCB_I2C_IXR_COMP) {
			armcb_i2c_slave_rcv_data(id);
			id->slave_state = ARMCB_I2C_SLAVE_STATE_IDLE;
			i2c_slave_event(id->slave, I2C_SLAVE_STOP, NULL);
		}
	}

	/* Master indicated xfer stop or fifo underflow/overflow */
	if (isr_status & (ARMCB_I2C_IXR_NACK | ARMCB_I2C_IXR_RX_OVF |
			  ARMCB_I2C_IXR_RX_UNF | ARMCB_I2C_IXR_TX_OVF)) {
		id->slave_state = ARMCB_I2C_SLAVE_STATE_IDLE;
		i2c_slave_event(id->slave, I2C_SLAVE_STOP, NULL);
		armcb_i2c_writereg(ARMCB_I2C_CR_CLR_FIFO, ARMCB_I2C_CR_OFFSET);
	}

	return IRQ_HANDLED;
}
#endif

/**
 * armcb_i2c_master_isr - Interrupt handler for the I2C device in master role
 * @ptr:	   Pointer to I2C device private data
 *
 * This function handles the data interrupt, transfer complete interrupt and
 * the error interrupts of the I2C device in master role.
 *
 * Return: IRQ_HANDLED always
 */
static irqreturn_t armcb_i2c_master_isr(void *ptr)
{
	unsigned int isr_status, avail_bytes, updatetx;
	unsigned int bytes_to_send;
	bool hold_quirk;
	struct armcb_i2c *id = ptr;
	/* Signal completion only after everything is updated */
	int done_flag = 0;
	irqreturn_t status = IRQ_NONE;

	isr_status = armcb_i2c_readreg(ARMCB_I2C_ISR_OFFSET);
	armcb_i2c_writereg(isr_status, ARMCB_I2C_ISR_OFFSET);

	/* Handling nack and arbitration lost interrupt */
	if (isr_status & (ARMCB_I2C_IXR_NACK | ARMCB_I2C_IXR_ARB_LOST)) {
		done_flag = 1;
		status = IRQ_HANDLED;
	}

	/*
	 * Check if transfer size register needs to be updated again for a
	 * large data receive operation.
	 */
	updatetx = 0;
	if (id->recv_count > id->curr_recv_count)
		updatetx = 1;

	hold_quirk = (id->quirks & ARMCB_I2C_BROKEN_HOLD_BIT) && updatetx;

	/* When receiving, handle data interrupt and completion interrupt */
	if (id->p_recv_buf && ((isr_status & ARMCB_I2C_IXR_COMP) ||
				   (isr_status & ARMCB_I2C_IXR_DATA))) {
		/* Read data if receive data valid is set */
		while (armcb_i2c_readreg(ARMCB_I2C_SR_OFFSET) &
			   ARMCB_I2C_SR_RXDV) {
		/*
		 * Clear hold bit that was set for FIFO control if
		 * RX data left is less than FIFO depth, unless
		 * repeated start is selected.
		 */
			if ((id->recv_count < ARMCB_I2C_FIFO_DEPTH) &&
				!id->bus_hold_flag)
				armcb_i2c_clear_bus_hold(id);

			*(id->p_recv_buf)++ =
				armcb_i2c_readreg(ARMCB_I2C_DATA_OFFSET);
			id->recv_count--;
			id->curr_recv_count--;

			if (armcb_is_holdquirk(id, hold_quirk))
				break;
		}

<<<<<<< HEAD   (28289f DPTSW-7267: Fix isp driver build warning)
		/*
		* The controller sends NACK to the slave when transfer size
		* register reaches zero without considering the HOLD bit.
		* For large data transfers to maintain transfer size non-zero
		* while performing a large receive operation.
		*/
=======
	/*
	 * The controller sends NACK to the slave when transfer size
	 * register reaches zero without considering the HOLD bit.
	 * This workaround is implemented for large data transfers to
	 * maintain transfer size non-zero while performing a large
	 * receive operation.
	 */
>>>>>>> CHANGE (3bd1a5 DPTSW-12398: fix the errors and warnings of code style check)
		if (armcb_is_holdquirk(id, hold_quirk)) {
			/* wait while fifo is full */
			while (armcb_i2c_readreg(ARMCB_I2C_XFER_SIZE_OFFSET) !=
				   (id->curr_recv_count - ARMCB_I2C_FIFO_DEPTH))
				;

			/*
			 * Check number of bytes to be received against maximum
			 * transfer size and update register accordingly.
			 */
			if (((int)(id->recv_count) - ARMCB_I2C_FIFO_DEPTH) >
				ARMCB_I2C_TRANSFER_SIZE) {
				armcb_i2c_writereg(ARMCB_I2C_TRANSFER_SIZE,
						   ARMCB_I2C_XFER_SIZE_OFFSET);
				id->curr_recv_count = ARMCB_I2C_TRANSFER_SIZE +
							  ARMCB_I2C_FIFO_DEPTH;
			} else {
				armcb_i2c_writereg(id->recv_count -
							   ARMCB_I2C_FIFO_DEPTH,
						   ARMCB_I2C_XFER_SIZE_OFFSET);
				id->curr_recv_count = id->recv_count;
			}
		} else if (id->recv_count && !hold_quirk &&
			   !id->curr_recv_count) {
			/* Set the slave address in address register*/
			armcb_i2c_writereg(id->p_msg->addr &
						   ARMCB_I2C_ADDR_MASK,
					   ARMCB_I2C_ADDR_OFFSET);

			if (id->recv_count > ARMCB_I2C_TRANSFER_SIZE) {
				armcb_i2c_writereg(ARMCB_I2C_TRANSFER_SIZE,
						   ARMCB_I2C_XFER_SIZE_OFFSET);
				id->curr_recv_count = ARMCB_I2C_TRANSFER_SIZE;
			} else {
				armcb_i2c_writereg(id->recv_count,
						   ARMCB_I2C_XFER_SIZE_OFFSET);
				id->curr_recv_count = id->recv_count;
			}
		}

		/* Clear hold (if not repeated start) and signal completion */
		if ((isr_status & ARMCB_I2C_IXR_COMP) && !id->recv_count) {
			if (!id->bus_hold_flag)
				armcb_i2c_clear_bus_hold(id);
			done_flag = 1;
		}

		status = IRQ_HANDLED;
	}

	/* When sending, handle transfer complete interrupt */
	if ((isr_status & ARMCB_I2C_IXR_COMP) && !id->p_recv_buf) {
	/*
	 * If there is more data to be sent, calculate the
	 * space available in FIFO and fill with that many bytes.
	 */
		if (id->send_count) {
			avail_bytes =
				ARMCB_I2C_FIFO_DEPTH -
				armcb_i2c_readreg(ARMCB_I2C_XFER_SIZE_OFFSET);
			if (id->send_count > avail_bytes)
				bytes_to_send = avail_bytes;
			else
				bytes_to_send = id->send_count;

			while (bytes_to_send--) {
				armcb_i2c_writereg((*(id->p_send_buf)++),
						   ARMCB_I2C_DATA_OFFSET);
				id->send_count--;
			}
		} else {
	  /*
	   * Signal the completion of transaction and
	   * clear the hold bus bit if there are no
	   * further messages to be processed.
	   */
			done_flag = 1;
		}
		if (!id->send_count && !id->bus_hold_flag)
			armcb_i2c_clear_bus_hold(id);

		status = IRQ_HANDLED;
	}

	/* Handling Slave monitor mode interrupt */
	if (isr_status & ARMCB_I2C_IXR_SLV_RDY) {
		unsigned int ctrl_reg;
		/* Read control register */
		ctrl_reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);

		/* Disable slave monitor mode */
		ctrl_reg &= ~ARMCB_I2C_CR_SLVMON;
		armcb_i2c_writereg(ctrl_reg, ARMCB_I2C_CR_OFFSET);

		/* Clear interrupt flag for slvmon mode */
		armcb_i2c_writereg(ARMCB_I2C_IXR_SLV_RDY, ARMCB_I2C_IDR_OFFSET);

		done_flag = 1;
		status = IRQ_HANDLED;
	}

	/* Update the status for errors */
	id->err_status = isr_status & ARMCB_I2C_IXR_ERR_INTR_MASK;
	if (id->err_status)
		status = IRQ_HANDLED;

	if (done_flag)
		complete(&id->xfer_done);

	return status;
}

/**
 * armcb_i2c_isr - Interrupt handler for the I2C device
 * @irq:	irq number for the I2C device
 * @ptr:	void pointer to armcb_i2c structure
 *
 * This function passes the control to slave/master based on current role of
 * i2c controller.
 *
 * Return: IRQ_HANDLED always
 */
static irqreturn_t armcb_i2c_isr(int irq, void *ptr)
{
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	struct armcb_i2c *id = ptr;

	switch (id->dev_mode) {
	case ARMCB_I2C_MODE_SLAVE:
		LOG(LOG_DEBUG, "slave interrupt");
		armcb_i2c_slave_isr(ptr);
		break;
	case ARMCB_I2C_MODE_MASTER:
		LOG(LOG_DEBUG, "master interrupt");
		armcb_i2c_master_isr(ptr);
		break;
	default:
		LOG(LOG_DEBUG, "undefined interrupt");
		break;
	}
#else
	armcb_i2c_master_isr(ptr);
#endif
	return IRQ_HANDLED;
}

/**
 * armcb_i2c_mrecv - Prepare and start a master receive operation
 * @id:		pointer to the i2c device structure
 */
static void armcb_i2c_mrecv(struct armcb_i2c *id)
{
	unsigned int ctrl_reg;
	unsigned int isr_status;

	id->p_recv_buf = id->p_msg->buf;
	id->recv_count = id->p_msg->len;

	/* Put the controller in master receive mode and clear the FIFO */
	ctrl_reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
	ctrl_reg |= ARMCB_I2C_CR_RW | ARMCB_I2C_CR_CLR_FIFO;

	if (id->p_msg->flags & I2C_M_RECV_LEN)
		id->recv_count = I2C_SMBUS_BLOCK_MAX + 1;

	id->curr_recv_count = id->recv_count;

	/*
	 * Check for the message size against FIFO depth and set the
	 * 'hold bus' bit if it is greater than FIFO depth.
	 */
	if (id->recv_count > ARMCB_I2C_FIFO_DEPTH)
		ctrl_reg |= ARMCB_I2C_CR_HOLD;

	armcb_i2c_writereg(ctrl_reg, ARMCB_I2C_CR_OFFSET);

	/* Clear the interrupts in interrupt status register */
	isr_status = armcb_i2c_readreg(ARMCB_I2C_ISR_OFFSET);
	armcb_i2c_writereg(isr_status, ARMCB_I2C_ISR_OFFSET);

	/*
	 * The no. of bytes to receive is checked against the limit of
	 * max transfer size. Set transfer size register with no of bytes
	 * receive if it is less than transfer size and transfer size if
	 * it is more. Enable the interrupts.
	 */
	if (id->recv_count > ARMCB_I2C_TRANSFER_SIZE) {
		armcb_i2c_writereg(ARMCB_I2C_TRANSFER_SIZE,
				   ARMCB_I2C_XFER_SIZE_OFFSET);
		id->curr_recv_count = ARMCB_I2C_TRANSFER_SIZE;
	} else {
		armcb_i2c_writereg(id->recv_count, ARMCB_I2C_XFER_SIZE_OFFSET);
	}

	/* Set the slave address in address register - triggers operation */
	armcb_i2c_writereg(ARMCB_I2C_ENABLED_INTR_MASK, ARMCB_I2C_IER_OFFSET);
	armcb_i2c_writereg(id->p_msg->addr & ARMCB_I2C_ADDR_MASK,
			   ARMCB_I2C_ADDR_OFFSET);
	/* Clear the bus hold flag if bytes to receive is less than FIFO size */
	if (!id->bus_hold_flag &&
		((id->p_msg->flags & I2C_M_RECV_LEN) != I2C_M_RECV_LEN) &&
		(id->recv_count <= ARMCB_I2C_FIFO_DEPTH))
		armcb_i2c_clear_bus_hold(id);
}

/**
 * armcb_i2c_msend - Prepare and start a master send operation
 * @id:		pointer to the i2c device
 */
static void armcb_i2c_msend(struct armcb_i2c *id)
{
	unsigned int avail_bytes;
	unsigned int bytes_to_send;
	unsigned int ctrl_reg;
	unsigned int isr_status;

	id->p_recv_buf = NULL;
	id->p_send_buf = id->p_msg->buf;
	id->send_count = id->p_msg->len;

	/* Set the controller in Master transmit mode and clear the FIFO. */
	ctrl_reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
	ctrl_reg &= ~ARMCB_I2C_CR_RW;
	ctrl_reg |= ARMCB_I2C_CR_CLR_FIFO;

	/*
	 * Check for the message size against FIFO depth and set the
	 * 'hold bus' bit if it is greater than FIFO depth.
	 */
	if (id->send_count > ARMCB_I2C_FIFO_DEPTH)
		ctrl_reg |= ARMCB_I2C_CR_HOLD;
	armcb_i2c_writereg(ctrl_reg, ARMCB_I2C_CR_OFFSET);

	/* Clear the interrupts in interrupt status register. */
	isr_status = armcb_i2c_readreg(ARMCB_I2C_ISR_OFFSET);
	armcb_i2c_writereg(isr_status, ARMCB_I2C_ISR_OFFSET);

	/*
	 * Calculate the space available in FIFO. Check the message length
	 * against the space available, and fill the FIFO accordingly.
	 * Enable the interrupts.
	 */
	avail_bytes = ARMCB_I2C_FIFO_DEPTH -
			  armcb_i2c_readreg(ARMCB_I2C_XFER_SIZE_OFFSET);

	if (id->send_count > avail_bytes)
		bytes_to_send = avail_bytes;
	else
		bytes_to_send = id->send_count;

	while (bytes_to_send--) {
		armcb_i2c_writereg((*(id->p_send_buf)++),
				   ARMCB_I2C_DATA_OFFSET);
		id->send_count--;
	}

	/*
	 * Clear the bus hold flag if there is no more data
	 * and if it is the last message.
	 */
	if (!id->bus_hold_flag && !id->send_count)
		armcb_i2c_clear_bus_hold(id);
	/* Set the slave address in address register - triggers operation. */
	armcb_i2c_writereg(ARMCB_I2C_ENABLED_INTR_MASK, ARMCB_I2C_IER_OFFSET);
	armcb_i2c_writereg(id->p_msg->addr & ARMCB_I2C_ADDR_MASK,
			   ARMCB_I2C_ADDR_OFFSET);
}

/**
 * armcb_i2c_slvmon - Handling Slav monitor mode feature
 * @id:		pointer to the i2c device
 */
static void armcb_i2c_slvmon(struct armcb_i2c *id)
{
	unsigned int ctrl_reg;
	unsigned int isr_status;

	id->p_recv_buf = NULL;
	id->p_send_buf = id->p_msg->buf;
	id->send_count = id->p_msg->len;

	/* Clear the interrupts in interrupt status register. */
	isr_status = armcb_i2c_readreg(ARMCB_I2C_ISR_OFFSET);
	armcb_i2c_writereg(isr_status, ARMCB_I2C_ISR_OFFSET);

	/* Enable slvmon control reg */
	ctrl_reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
	ctrl_reg |= ARMCB_I2C_CR_MS | ARMCB_I2C_CR_NEA | ARMCB_I2C_CR_SLVMON |
			ARMCB_I2C_CR_CLR_FIFO;
	ctrl_reg &= ~(ARMCB_I2C_CR_RW);
	armcb_i2c_writereg(ctrl_reg, ARMCB_I2C_CR_OFFSET);

	/* Initialize slvmon reg */
	armcb_i2c_writereg(0xF, ARMCB_I2C_SLV_PAUSE_OFFSET);

	/* Set the slave address to start the slave address transmission */
	armcb_i2c_writereg(id->p_msg->addr, ARMCB_I2C_ADDR_OFFSET);

	/* Setup slvmon interrupt flag */
	armcb_i2c_writereg(ARMCB_I2C_IXR_SLV_RDY, ARMCB_I2C_IER_OFFSET);
}

/**
 * armcb_i2c_master_reset - Reset the interface
 * @adap:	pointer to the i2c adapter driver instance
 *
 * This function cleanup the fifos, clear the hold bit and status
 * and disable the interrupts.
 */
static void armcb_i2c_master_reset(struct i2c_adapter *adap)
{
	struct armcb_i2c *id = adap->algo_data;
	u32 regval;

	/* Disable the interrupts */
	armcb_i2c_writereg(ARMCB_I2C_IXR_ALL_INTR_MASK, ARMCB_I2C_IDR_OFFSET);
	/* Clear the hold bit and fifos */
	regval = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
	regval &= ~(ARMCB_I2C_CR_HOLD | ARMCB_I2C_CR_SLVMON);
	regval |= ARMCB_I2C_CR_CLR_FIFO;
	armcb_i2c_writereg(regval, ARMCB_I2C_CR_OFFSET);
	/* Update the transfercount register to zero */
	armcb_i2c_writereg(0, ARMCB_I2C_XFER_SIZE_OFFSET);
	/* Clear the interrupt status register */
	regval = armcb_i2c_readreg(ARMCB_I2C_ISR_OFFSET);
	armcb_i2c_writereg(regval, ARMCB_I2C_ISR_OFFSET);
	/* Clear the status register */
	regval = armcb_i2c_readreg(ARMCB_I2C_SR_OFFSET);
	armcb_i2c_writereg(regval, ARMCB_I2C_SR_OFFSET);
}

static int armcb_i2c_process_msg(struct armcb_i2c *id, struct i2c_msg *msg,
				 struct i2c_adapter *adap)
{
	unsigned long time_left;
	u32 reg;

	id->p_msg = msg;
	id->err_status = 0;
	reinit_completion(&id->xfer_done);

	/* Check for the TEN Bit mode on each msg */
	reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
	if (msg->flags & I2C_M_TEN) {
		if (reg & ARMCB_I2C_CR_NEA)
			armcb_i2c_writereg(reg & ~ARMCB_I2C_CR_NEA,
					   ARMCB_I2C_CR_OFFSET);
	} else {
		if (!(reg & ARMCB_I2C_CR_NEA))
			armcb_i2c_writereg(reg | ARMCB_I2C_CR_NEA,
					   ARMCB_I2C_CR_OFFSET);
	}
	/* Check for zero length - Slave monitor mode */
	if (msg->len == 0)
		armcb_i2c_slvmon(id);
	/* Check for the R/W flag on each msg */
	else if (msg->flags & I2C_M_RD)
		armcb_i2c_mrecv(id);
	else
		armcb_i2c_msend(id);

	/* Wait for the signal of completion */
	time_left = wait_for_completion_timeout(&id->xfer_done, adap->timeout);
	if (time_left == 0) {
		i2c_recover_bus(adap);
		armcb_i2c_master_reset(adap);
		LOG(LOG_ERR, "timeout waiting on completion");
		return -ETIMEDOUT;
	}

	armcb_i2c_writereg(ARMCB_I2C_IXR_ALL_INTR_MASK, ARMCB_I2C_IDR_OFFSET);

	/* If it is bus arbitration error, try again */
	if (id->err_status & ARMCB_I2C_IXR_ARB_LOST)
		return -EAGAIN;

	return 0;
}

/**
 * armcb_i2c_master_xfer - The main i2c transfer function
 * @adap:	pointer to the i2c adapter driver instance
 * @msgs:	pointer to the i2c message structure
 * @num:	the number of messages to transfer
 *
 * Initiates the send/recv activity based on the transfer message received.
 *
 * Return: number of msgs processed on success, negative error otherwise
 */
static int armcb_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
				 int num)
{
	int ret, count;
	u32 reg;
	struct armcb_i2c *id = adap->algo_data;
	bool hold_quirk;
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	bool change_role = false;
#endif

	ret = pm_runtime_get_sync(id->dev);
	if (ret < 0)
		return ret;

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	/* Check i2c operating mode and switch if possible */
	if (id->dev_mode == ARMCB_I2C_MODE_SLAVE) {
		if (id->slave_state != ARMCB_I2C_SLAVE_STATE_IDLE)
			return -EAGAIN;

		/* Set mode to master */
		armcb_i2c_set_mode(ARMCB_I2C_MODE_MASTER, id);

		/* Mark flag to change role once xfer is completed */
		change_role = true;
	}
#endif

	/* Check if the bus is free */
	if (msgs->len)
		if (armcb_i2c_readreg(ARMCB_I2C_SR_OFFSET) & ARMCB_I2C_SR_BA) {
			ret = -EAGAIN;
			goto out;
		}

	hold_quirk = !!(id->quirks & ARMCB_I2C_BROKEN_HOLD_BIT);
	/*
	 * Set the flag to one when multiple messages are to be
	 * processed with a repeated start.
	 */
	if (num > 1) {
	/*
	 * This controller does not give completion interrupt after a
	 * master receive message if HOLD bit is set (repeated start),
	 * resulting in SW timeout. Hence, if a receive message is
	 * followed by any other message, an error is returned
	 * indicating that this sequence is not supported.
	 */
		for (count = 0; (count < num - 1 && hold_quirk); count++) {
			if (msgs[count].flags & I2C_M_RD) {
				dev_warn(
					adap->dev.parent,
					"Can't do repeated start after a receive message");
				ret = -EOPNOTSUPP;
				goto out;
			}
		}
		id->bus_hold_flag = 1;
		reg = armcb_i2c_readreg(ARMCB_I2C_CR_OFFSET);
		reg |= ARMCB_I2C_CR_HOLD;
		armcb_i2c_writereg(reg, ARMCB_I2C_CR_OFFSET);
	} else {
		id->bus_hold_flag = 0;
	}

	/* Process the msg one by one */
	for (count = 0; count < num; count++, msgs++) {
		if (count == (num - 1))
			id->bus_hold_flag = 0;

		ret = armcb_i2c_process_msg(id, msgs, adap);
		if (ret)
			goto out;

		/* Report the other error interrupts to application */
		if (id->err_status) {
			armcb_i2c_master_reset(adap);

			if (id->err_status & ARMCB_I2C_IXR_NACK) {
				ret = -ENXIO;
				goto out;
			}
			ret = -EIO;
			goto out;
		}
	}

	ret = num;

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	/* Switch i2c mode to slave */
	if (change_role)
		armcb_i2c_set_mode(ARMCB_I2C_MODE_SLAVE, id);
#endif

out:
	pm_runtime_mark_last_busy(id->dev);
	pm_runtime_put_autosuspend(id->dev);
	return ret;
}

/**
 * armcb_i2c_func - Returns the supported features of the I2C driver
 * @adap:	pointer to the i2c adapter structure
 *
 * Return: 32 bit value, each bit corresponding to a feature
 */
static u32 armcb_i2c_func(struct i2c_adapter *adap)
{
	u32 func = I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR |
		   (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK) |
		   I2C_FUNC_SMBUS_BLOCK_DATA;

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	func |= I2C_FUNC_SLAVE;
#endif

	return func;
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static int armcb_reg_slave(struct i2c_client *slave)
{
	int ret;
	struct armcb_i2c *id =
		container_of(slave->adapter, struct armcb_i2c, adap);

	if (id->slave)
		return -EBUSY;

	if (slave->flags & I2C_CLIENT_TEN)
		return -EAFNOSUPPORT;

	ret = pm_runtime_get_sync(id->dev);
	if (ret < 0)
		return ret;

	/* Store slave information */
	id->slave = slave;

	/* Enable I2C slave */
	armcb_i2c_set_mode(ARMCB_I2C_MODE_SLAVE, id);

	return 0;
}

static int armcb_unreg_slave(struct i2c_client *slave)
{
	struct armcb_i2c *id =
		container_of(slave->adapter, struct armcb_i2c, adap);

	pm_runtime_put(id->dev);

	/* Remove slave information */
	id->slave = NULL;

	/* Enable I2C master */
	armcb_i2c_set_mode(ARMCB_I2C_MODE_MASTER, id);

	return 0;
}
#endif

static const struct i2c_algorithm armcb_i2c_algo = {
	.master_xfer = armcb_i2c_master_xfer,
	.functionality = armcb_i2c_func,
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	.reg_slave = armcb_reg_slave,
	.unreg_slave = armcb_unreg_slave,
#endif
};

/**
 * armcb_i2c_calc_divs - Calculate clock dividers
 * @f:		I2C clock frequency
 * @input_clk:	Input clock frequency
 * @a:		First divider (return value)
 * @b:		Second divider (return value)
 *
 * f is used as input and output variable. As input it is used as target I2C
 * frequency. On function exit f holds the actually resulting I2C frequency.
 *
 * Return: 0 on success, negative errno otherwise.
 */
static int armcb_i2c_calc_divs(unsigned long *f, unsigned long input_clk,
				   unsigned int *a, unsigned int *b)
{
	unsigned long fscl = *f, best_fscl = *f, actual_fscl, temp;
	unsigned int div_a, div_b, calc_div_a = 0, calc_div_b = 0;
	unsigned int last_error, current_error;

	/* calculate (divisor_a+1) x (divisor_b+1) */
	temp = input_clk / (22 * fscl);

	/*
	 * If the calculated value is negative or 0, the fscl input is out of
	 * range. Return error.
	 */
	if (!temp || (temp > (ARMCB_I2C_DIVA_MAX * ARMCB_I2C_DIVB_MAX)))
		return -EINVAL;

	last_error = -1;
	for (div_a = 0; div_a < ARMCB_I2C_DIVA_MAX; div_a++) {
		div_b = DIV_ROUND_UP(input_clk, 22 * fscl * (div_a + 1));

		if ((div_b < 1) || (div_b > ARMCB_I2C_DIVB_MAX))
			continue;
		div_b--;

		actual_fscl = input_clk / (22 * (div_a + 1) * (div_b + 1));

		if (actual_fscl > fscl)
			continue;

		current_error = ((actual_fscl > fscl) ? (actual_fscl - fscl) :
							(fscl - actual_fscl));

		if (last_error > current_error) {
			calc_div_a = div_a;
			calc_div_b = div_b;
			best_fscl = actual_fscl;
			last_error = current_error;
		}
	}

	*a = calc_div_a;
	*b = calc_div_b;
	*f = best_fscl;

	return 0;
}

/**
 * armcb_i2c_setclk - This function sets the serial clock rate for the I2C
 *device
 * @clk_in:	I2C clock input frequency in Hz
 * @id:		Pointer to the I2C device structure
 *
 * The device must be idle rather than busy transferring data before setting
 * these device options.
 * The data rate is set by values in the control register.
 * The formula for determining the correct register values is
 *	Fscl = Fpclk/(22 x (divisor_a+1) x (divisor_b+1))
 * See the hardware data sheet for a full explanation of setting the serial
 * clock rate. The clock can not be faster than the input clock divide by 22.
 * The two most common clock rates are 100KHz and 400KHz.
 *
 * Return: 0 on success, negative error otherwise
 */
static int armcb_i2c_setclk(unsigned long clk_in, struct armcb_i2c *id)
{
	unsigned int div_a, div_b;
	unsigned int ctrl_reg;
	int ret = 0;
	unsigned long fscl = id->i2c_clk;

	ret = armcb_i2c_calc_divs(&fscl, clk_in, &div_a, &div_b);
	if (ret)
		return ret;

	ctrl_reg = id->ctrl_reg;
	ctrl_reg &= ~(ARMCB_I2C_CR_DIVA_MASK | ARMCB_I2C_CR_DIVB_MASK);
	ctrl_reg |= ((div_a << ARMCB_I2C_CR_DIVA_SHIFT) |
			 (div_b << ARMCB_I2C_CR_DIVB_SHIFT));
	id->ctrl_reg = ctrl_reg;
	return 0;
}

/**
 * armcb_i2c_clk_notifier_cb - Clock rate change callback
 * @nb:		Pointer to notifier block
 * @event:	Notification reason
 * @data:	Pointer to notification data object
 *
 * This function is called when the armcb_i2c input clock frequency changes.
 * The callback checks whether a valid bus frequency can be generated after the
 * change. If so, the change is acknowledged, otherwise the change is aborted.
 * New dividers are written to the HW in the pre- or post change notification
 * depending on the scaling direction.
 *
 * Return:	NOTIFY_STOP if the rate change should be aborted, NOTIFY_OK
 *		to acknowledge the change, NOTIFY_DONE if the notification is
 *		considered irrelevant.
 */
static int armcb_i2c_clk_notifier_cb(struct notifier_block *nb,
					 unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct armcb_i2c *id = to_armcb_i2c(nb);

	if (pm_runtime_suspended(id->dev))
		return NOTIFY_OK;

	switch (event) {
	case PRE_RATE_CHANGE: {
		unsigned long input_clk = ndata->new_rate;
		unsigned long fscl = id->i2c_clk;
		unsigned int div_a, div_b;
		int ret;

		ret = armcb_i2c_calc_divs(&fscl, input_clk, &div_a, &div_b);
		if (ret) {
			dev_warn(id->adap.dev.parent,
				 "clock rate change rejected");
			return NOTIFY_STOP;
		}

		/* scale up */
		if (ndata->new_rate > ndata->old_rate)
			armcb_i2c_setclk(ndata->new_rate, id);

		return NOTIFY_OK;
	}
	case POST_RATE_CHANGE:
		id->input_clk = ndata->new_rate;
		/* scale down */
		if (ndata->new_rate < ndata->old_rate)
			armcb_i2c_setclk(ndata->new_rate, id);
		return NOTIFY_OK;
	case ABORT_RATE_CHANGE:
		/* scale up */
		if (ndata->new_rate > ndata->old_rate)
			armcb_i2c_setclk(ndata->old_rate, id);
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}
}

/**
 * armcb_i2c_runtime_suspend -  Runtime suspend method for the driver
 * @dev:	Address of the platform_device structure
 *
 * Put the driver into low power mode.
 *
 * Return: 0 always
 */
static int __maybe_unused armcb_i2c_runtime_suspend(struct device *dev)
{
	struct armcb_i2c *xi2c = dev_get_drvdata(dev);

	clk_disable(xi2c->clk);

	return 0;
}

/**
 * armcb_i2c_init -  Controller initialisation
 * @id:		Device private data structure
 *
 * Initialise the i2c controller.
 *
 */
static void armcb_i2c_init(struct armcb_i2c *id)
{
	armcb_i2c_writereg(id->ctrl_reg, ARMCB_I2C_CR_OFFSET);
  /*
   * Armcb I2C controller has a bug wherein it generates
   * invalid read transaction after HW timeout in master receiver mode.
   * HW timeout is not used by this driver and the interrupt is disabled.
   * But the feature itself cannot be disabled. Hence maximum value
   * is written to this register to reduce the chances of error.
   */
	armcb_i2c_writereg(ARMCB_I2C_TIMEOUT_MAX, ARMCB_I2C_TIME_OUT_OFFSET);
}

/**
 * armcb_i2c_runtime_resume - Runtime resume
 * @dev:	Address of the platform_device structure
 *
 * Runtime resume callback.
 *
 * Return: 0 on success and error value on error
 */
static int __maybe_unused armcb_i2c_runtime_resume(struct device *dev)
{
	struct armcb_i2c *xi2c = dev_get_drvdata(dev);
	int ret;

	ret = clk_enable(xi2c->clk);
	if (ret) {
		LOG(LOG_ERR, "Cannot enable clock.");
		return ret;
	}
	armcb_i2c_init(xi2c);

	return 0;
}

/**
 * armcb_i2c_prepare_recovery - Withold recovery state
 * @adapter:	Pointer to i2c adapter
 *
 * This function is called to prepare for recovery.
 * It changes the state of pins from SCL/SDA to GPIO.
 */
static void armcb_i2c_prepare_recovery(struct i2c_adapter *adapter)
{
	struct armcb_i2c *p_armcb_i2c;

	p_armcb_i2c = container_of(adapter, struct armcb_i2c, adap);

	/* Setting pin state as gpio */
	pinctrl_select_state(p_armcb_i2c->pinctrl,
				 p_armcb_i2c->pinctrl_pins_gpio);
}

/**
 * armcb_i2c_unprepare_recovery - Release recovery state
 * @adapter:	Pointer to i2c adapter
 *
 * This function is called on exiting recovery. It reverts
 * the state of pins from GPIO to SCL/SDA.
 */
static void armcb_i2c_unprepare_recovery(struct i2c_adapter *adapter)
{
	struct armcb_i2c *p_armcb_i2c;

	p_armcb_i2c = container_of(adapter, struct armcb_i2c, adap);

	/* Setting pin state to default(i2c) */
	pinctrl_select_state(p_armcb_i2c->pinctrl,
				 p_armcb_i2c->pinctrl_pins_default);
}

/**
 * armcb_i2c_init_recovery_info  - Initialize I2C bus recovery
 * @pid:		Pointer to armcb i2c structure
 * @pdev:	   Handle to the platform device structure
 *
 * This function does required initialization for i2c bus
 * recovery. It registers three functions for prepare,
 * recover and unprepare
 *
 * Return: 0 on Success, negative error otherwise.
 */
static int armcb_i2c_init_recovery_info(struct armcb_i2c *pid,
					struct platform_device *pdev)
{
	struct i2c_bus_recovery_info *rinfo = &pid->rinfo;

	pid->pinctrl_pins_default =
		pinctrl_lookup_state(pid->pinctrl, PINCTRL_STATE_DEFAULT);
	pid->pinctrl_pins_gpio = pinctrl_lookup_state(pid->pinctrl, "gpio");

/* Fetches GPIO pins */
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	rinfo->sda_gpiod = (struct gpio_desc *)of_get_named_gpio(
		pdev->dev.of_node, "sda-gpios", 0);
	rinfo->scl_gpiod = (struct gpio_desc *)of_get_named_gpio(
		pdev->dev.of_node, "scl-gpios", 0);
	/* if GPIO driver isn't ready yet, deffer probe */
	if ((int)rinfo->sda_gpiod == -EPROBE_DEFER ||
		(int)rinfo->scl_gpiod == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	/* Validates fetched information */
	if (!gpio_is_valid((int)rinfo->sda_gpiod) ||
		!gpio_is_valid((int)rinfo->scl_gpiod) ||
		IS_ERR(pid->pinctrl_pins_default) ||
		IS_ERR(pid->pinctrl_pins_gpio)) {
		LOG(LOG_DEBUG, "recovery information incomplete");
		return 0;
	}

	LOG(LOG_DEBUG, "using scl-gpio %d and sda-gpio %d for recovery",
		rinfo->sda_gpiod, rinfo->scl_gpiod);

	rinfo->prepare_recovery = armcb_i2c_prepare_recovery;
	rinfo->unprepare_recovery = armcb_i2c_unprepare_recovery;
	rinfo->recover_bus = i2c_generic_scl_recovery;
	pid->adap.bus_recovery_info = rinfo;
#else
	rinfo->sda_gpio = of_get_named_gpio(pdev->dev.of_node, "sda-gpios", 0);
	rinfo->scl_gpio = of_get_named_gpio(pdev->dev.of_node, "scl-gpios", 0);

	/* if GPIO driver isn't ready yet, deffer probe */
	if (rinfo->sda_gpio == -EPROBE_DEFER ||
		rinfo->scl_gpio == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	/* Validates fetched information */
	if (!gpio_is_valid(rinfo->sda_gpio) ||
		!gpio_is_valid(rinfo->scl_gpio) ||
		IS_ERR(pid->pinctrl_pins_default) ||
		IS_ERR(pid->pinctrl_pins_gpio)) {
		LOG(LOG_DEBUG, "recovery information incomplete");
		return 0;
	}

	LOG(LOG_DEBUG, "using scl-gpio %d and sda-gpio %d for recovery",
		rinfo->sda_gpio, rinfo->scl_gpio);

	rinfo->prepare_recovery = armcb_i2c_prepare_recovery;
	rinfo->unprepare_recovery = armcb_i2c_unprepare_recovery;
	rinfo->recover_bus = i2c_generic_gpio_recovery;
	pid->adap.bus_recovery_info = rinfo;
#endif

	return 0;
}

static const struct dev_pm_ops armcb_i2c_dev_pm_ops = {
#ifndef QEMU_ON_VEXPRESS
	SET_RUNTIME_PM_OPS(armcb_i2c_runtime_suspend, armcb_i2c_runtime_resume,
			   NULL)
#endif
};

static const struct armcb_platform_data armcb_i2c_def = {
	.quirks = ARMCB_I2C_BROKEN_HOLD_BIT,
};

static const struct of_device_id armcb_i2c_of_match[] = {
	{ .compatible = "armcb,sensor", .data = &armcb_i2c_def },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, armcb_i2c_of_match);

/**
 * armcb_i2c_probe - Platform registration call
 * @pdev:	Handle to the platform device structure
 *
 * This function does all the memory allocation and registration for the i2c
 * device. User can modify the address mode to 10 bit address mode using the
 * ioctl call with option I2C_TENBIT.
 *
 * Return: 0 on success, negative error otherwise
 */
static int armcb_i2c_probe(struct platform_device *pdev)
{
	int ret = 0;
#ifndef QEMU_ON_VEXPRESS
	struct resource *r_mem;
	struct armcb_i2c *id;
	const struct of_device_id *match;

	LOG(LOG_INFO, "+");
	id = devm_kzalloc(&pdev->dev, sizeof(*id), GFP_KERNEL);
	if (!id)
		return -ENOMEM;

	id->dev = &pdev->dev;
	platform_set_drvdata(pdev, id);

	match = of_match_node(armcb_i2c_of_match, pdev->dev.of_node);
	if (match && match->data) {
		const struct armcb_platform_data *data = match->data;

		id->quirks = data->quirks;
	}

	id->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (!IS_ERR(id->pinctrl)) {
		ret = armcb_i2c_init_recovery_info(id, pdev);
		if (ret)
			return ret;
	}

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	id->membase = devm_ioremap_resource(&pdev->dev, r_mem);
	if (IS_ERR(id->membase))
		return PTR_ERR(id->membase);

	id->irq = platform_get_irq(pdev, 0);
	id->adap.owner = THIS_MODULE;
	id->adap.dev.of_node = pdev->dev.of_node;
	id->adap.algo = &armcb_i2c_algo;
	id->adap.timeout = ARMCB_I2C_TIMEOUT;
	id->adap.retries = 3; /* Default retry value. */
	id->adap.algo_data = id;
	id->adap.dev.parent = &pdev->dev;
	init_completion(&id->xfer_done);
	snprintf(id->adap.name, sizeof(id->adap.name), "Armcb I2C at %08lx",
		 (unsigned long)r_mem->start);
	LOG(LOG_INFO, " start(%lx)", (unsigned long)r_mem->start);
	id->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(id->clk)) {
		LOG(LOG_ERR, "input clock not found.");
		return PTR_ERR(id->clk);
	}
	ret = clk_prepare_enable(id->clk);
	if (ret)
		LOG(LOG_ERR, "Unable to enable clock.");

	pm_runtime_set_autosuspend_delay(id->dev, ARMCB_I2C_PM_TIMEOUT);
	pm_runtime_use_autosuspend(id->dev);
	pm_runtime_set_active(id->dev);
	pm_runtime_enable(id->dev);

	id->clk_rate_change_nb.notifier_call = armcb_i2c_clk_notifier_cb;
	if (clk_notifier_register(id->clk, &id->clk_rate_change_nb))
		dev_warn(&pdev->dev, "Unable to register clock notifier.");
	id->input_clk = clk_get_rate(id->clk);
	LOG(LOG_DEBUG, " input_clk(%lx)", id->input_clk);

	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency",
				   &id->i2c_clk);
	if (ret || (id->i2c_clk > ARMCB_I2C_SPEED_MAX))
		id->i2c_clk = ARMCB_I2C_SPEED_DEFAULT;
	LOG(LOG_INFO, " i2c_clk(%d)", id->i2c_clk);

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	/* Set initial mode to master */
	id->dev_mode = ARMCB_I2C_MODE_MASTER;
	id->slave_state = ARMCB_I2C_SLAVE_STATE_IDLE;
#endif
	id->ctrl_reg = ARMCB_I2C_CR_ACK_EN | ARMCB_I2C_CR_NEA | ARMCB_I2C_CR_MS;

	ret = armcb_i2c_setclk(id->input_clk, id);
	if (ret) {
		LOG(LOG_ERR, "invalid SCL clock: %u Hz", id->i2c_clk);
		ret = -EINVAL;
		goto err_clk_dis;
	}

	ret = devm_request_irq(&pdev->dev, id->irq, armcb_i2c_isr, 0,
				   ARMCB_I2C_DRVNAME, id);
	if (ret) {
		LOG(LOG_ERR, "cannot get irq %d", id->irq);
		goto err_clk_dis;
	}

	LOG(LOG_DEBUG, " id->ctrl_reg(0x%x)", id->ctrl_reg);
	armcb_i2c_init(id);

	ret = i2c_add_adapter(&id->adap);
	if (ret < 0)
		goto err_clk_dis;

	LOG(LOG_INFO, " id->adap.nr(%d)", id->adap.nr);

	dev_info(&pdev->dev, "%u kHz mmio %08lx irq %d", id->i2c_clk / 1000,
		 (unsigned long)r_mem->start, id->irq);

	LOG(LOG_INFO, " scuess -");
	return ret;

err_clk_dis:
	clk_disable_unprepare(id->clk);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	LOG(LOG_ERR, "failed ret(%d) -", ret);
#endif
	return ret;
}

/**
 * armcb_i2c_remove - Unregister the device after releasing the resources
 * @pdev:	Handle to the platform device structure
 *
 * This function frees all the resources allocated to the device.
 *
 * Return: 0 always
 */
static int armcb_i2c_remove(struct platform_device *pdev)
{
#ifndef QEMU_ON_VEXPRESS
	struct armcb_i2c *id = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_dont_use_autosuspend(&pdev->dev);

	i2c_del_adapter(&id->adap);
	clk_notifier_unregister(id->clk, &id->clk_rate_change_nb);
	clk_disable_unprepare(id->clk);
#endif
	return 0;
}

static struct platform_driver armcb_i2c_drv = {
	.driver = {
		.name = ARMCB_I2C_DRVNAME,
		.of_match_table = armcb_i2c_of_match,
		.pm = &armcb_i2c_dev_pm_ops,
	},
	.probe = armcb_i2c_probe,
	.remove = armcb_i2c_remove,
};

#ifndef ARMCB_CAM_KO
static int __init armcb_i2c_bus_init(void)
{
	return platform_driver_register(&armcb_i2c_drv);
}

static void __exit armcb_i2c_bus_exit(void)
{
	platform_driver_unregister(&armcb_i2c_drv);
}

module_init(armcb_i2c_bus_init);
module_exit(armcb_i2c_bus_exit);

MODULE_AUTHOR("Armchina Inc.");
MODULE_DESCRIPTION("Armcb I2C bus driver");
MODULE_LICENSE("GPL v2");
#else
static void *g_instance;
void *armcb_get_system_i2c_driver_instance(void)
{
	if (platform_driver_register(&armcb_i2c_drv) < 0) {
		LOG(LOG_ERR, "register spi motor driver failed.\n");
		return NULL;
	}
	g_instance = (void *)&armcb_i2c_drv;
	return g_instance;
}

void armcb_system_i2c_driver_detroy(void)
{
	if (g_instance)
		platform_driver_unregister(
			(struct platform_driver *)g_instance);
}
#endif
