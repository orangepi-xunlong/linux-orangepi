/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/delay.h>
#include "cec.h"
#include "../api/core/irq.h"

static uintptr_t hdmi_base;
static u8 cec_active;

static int cec_CfgSignalFreeTime(int time);
static int cec_CfgStandbyMode(int enable);

static int cec_GetSend(void);
static int cec_SetSend(void);
static int cec_IntClear(unsigned char mask);

static void cec_wakeupctrl_clear(void);
static int cec_IntDisable(unsigned char mask);
static int cec_IntEnable(unsigned char mask);
static int cec_IntStatus(unsigned char mask);
static int cec_IntStatusClear(unsigned char mask);
static int cec_CfgTxBuf(char *buf, unsigned size);
static int cec_CfgRxBuf(char *buf, unsigned size);
static int cec_GetLocked(void);
static int cec_SetLocked(void);

void cec_set_base_addr(uintptr_t reg_base)
{
	hdmi_base = reg_base;
}

static void cec_write(uintptr_t addr, u32 data)
{
	asm volatile("dsb st");
	*((volatile u8 *)(hdmi_base + (addr >> 2))) = data;
}

static u32 cec_read(uintptr_t addr)
{
	return *((volatile u8 *)(hdmi_base + (addr >> 2)));
}

/*calculate valid bit account*/
static u8 lowest_bitset(u8 x)
{
	u8 result = 0;

	/* x=0 is not properly handled by while-loop */
	if (x == 0)
		return 0;

	while ((x & 1) == 0) {
		x >>= 1;
		result++;
	}

	return result;
}
static void cec_write_mask(u32 addr, u8 mask, u8 data)
{
	u8 temp = 0;
	u8 shift = lowest_bitset(mask);

	temp = cec_read(addr);
	temp &= ~(mask);
	temp |= (mask & (data << shift));
	cec_write(addr, temp);
}

void cec_set_active(u8 active)
{
	cec_active = active;
}

static void mc_cec_clock_enable(u8 bit)
{
	LOG_TRACE1(bit);
	cec_write_mask(MC_CLKDIS, MC_CLKDIS_CECCLK_DISABLE_MASK, bit);
}

/**
 * Wait for message (quit after a given period)
 * @param buf:     buffer to hold data
 * @param size:    maximum data length [byte]
 * @param timeout: time out period [ms]
 * @return error code or received bytes
 */
static int cec_msgRx(char *buf, unsigned size,
		 int timeout)
{
	int retval = -1;

	if (cec_GetLocked() != 0) {
		retval = cec_CfgRxBuf(buf, size);
		cec_IntStatusClear(CEC_MASK_EOM_MASK | CEC_MASK_ERROR_FLOW_MASK);
		cec_SetLocked();
	}
	return retval;
}

/**
 * Send a message (retry in case of failure)
 * @param buf:   data to send
 * @param size:  data length [byte]
 * @param retry: maximum number of retries
 * @return error code or transmitted bytes
 */
static int cec_msgTx(char *buf, unsigned size,
		 unsigned retry)
{
	int timeout = 10;
	u8 status = 0;
	int i, retval = -1;

	if (size > 0) {
		if (cec_CfgTxBuf(buf, size) == (int)size) {
			for (i = 0; i < (int)retry; i++) {
				if (cec_CfgSignalFreeTime(i ? 3 : 5))
					break;

				cec_IntEnable(CEC_MASK_DONE_MASK
						| CEC_MASK_NACK_MASK
						| CEC_MASK_ARB_LOST_MASK
						| CEC_MASK_ERROR_INITIATOR_MASK);
				cec_SetSend();
				msleep(1);

				timeout = 100;
				while (cec_GetSend() != 0 && timeout) {
					msleep(10);
					timeout--;
				}

				if (timeout == 0) {
					pr_err("[%d cec send time] timeout!!!\n",
						i);
					continue;
				}

				msleep(1);
				status = cec_read(IH_CEC_STAT0);
				cec_write(IH_CEC_STAT0, status);
				if ((status & CEC_MASK_ARB_LOST_MASK)
					|| (status & CEC_MASK_ERROR_INITIATOR_MASK)
					|| (status & CEC_MASK_ERROR_FLOW_MASK)) {
					CEC_INF("CEC Send error[%d time]: "
					      "IH Status: 0x%02x\n", i, status);
					continue;
				} else if (status & CEC_MASK_NACK_MASK) {
					retval = 1;
					break;
				} else if (status  & CEC_MASK_DONE_MASK) {
					retval = 0;
					break;
				}
			}
		}
	}

	return retval;
}

int cec_send_poll(char src)
{
	int ret = -1;
	char buf[1];
	int timeout = 40;

	if (!cec_active) {
		pr_err("[CEC]cec_send_poll failed\n");
		return -1;
	}

	buf[0] = (src << 4) | src;
	if (cec_CfgTxBuf(buf, 1) == 1) {
		cec_CfgSignalFreeTime(5);
		cec_IntEnable(CEC_MASK_DONE_MASK
				| CEC_MASK_NACK_MASK
				| CEC_MASK_ARB_LOST_MASK
				| CEC_MASK_ERROR_INITIATOR_MASK);
		cec_SetSend();
		udelay(100);
		while (timeout) {
			if ((!cec_GetSend())
				&& cec_IntStatus(CEC_MASK_NACK_MASK)) {
				cec_IntStatusClear(CEC_MASK_NACK_MASK);
				ret = 0;
				break;
			}
			msleep(10);
			timeout--;
		}
	}

	return ret;
}

/**
 * Open a CEC controller
 * @warning Execute before start using a CEC controller
 * @return error code
 */
int cec_Init(void)
{
	mc_cec_clock_enable(0);
	udelay(200);
	/*
	 * This function is also called after wake up,
	 * so it must NOT reset logical address allocation
	 */
	cec_CfgStandbyMode(0);
	cec_write(IH_MUTE_CEC_STAT0, 0x00);
	cec_IntDisable(CEC_MASK_WAKEUP_MASK);
	cec_IntClear(CEC_MASK_WAKEUP_MASK);
	cec_wakeupctrl_clear();
	cec_IntEnable(CEC_MASK_DONE_MASK | CEC_MASK_EOM_MASK
					| CEC_MASK_NACK_MASK
					| CEC_MASK_ARB_LOST_MASK
					| CEC_MASK_ERROR_FLOW_MASK
					| CEC_MASK_ERROR_INITIATOR_MASK);

	return 0;
}

/**
 * Close a CEC controller
 * @warning Execute before stop using a CEC controller
 * @param wakeup: enable wake up mode (don't enable it in TX side)
 * @return error code
 */
int cec_Disable(int wakeup)
{
	if (!cec_active) {
		pr_err("[CEC]cec_Disable failed\n");
		return -1;
	}

	cec_IntDisable(CEC_MASK_DONE_MASK | CEC_MASK_EOM_MASK |
		CEC_MASK_NACK_MASK | CEC_MASK_ARB_LOST_MASK
		| CEC_MASK_ERROR_FLOW_MASK
		| CEC_MASK_ERROR_INITIATOR_MASK);
	if (wakeup) {
		cec_IntClear(CEC_MASK_WAKEUP_MASK);
		cec_IntEnable(CEC_MASK_WAKEUP_MASK);
		cec_CfgStandbyMode(1);
	} else
		cec_CfgLogicAddr(BCST_ADDR, 1);

	mc_cec_clock_enable(1);
	return 0;
}

/**
 * Configure logical address
 * @param addr:   logical address
 * @param enable: alloc/free address
 * @return error code
 */
int cec_CfgLogicAddr(unsigned addr, int enable)
{
	unsigned int regs;

	if (!cec_active) {
		pr_err("[CEC]cec_CfgLogicAddr failed\n");
		return -1;
	}

	if (addr > BCST_ADDR) {
		pr_err("Error:invalid parameter\n");
		return -1;
	}

	cec_write(CEC_ADDR_L, 0x00);
	cec_write(CEC_ADDR_H, 0x00);

	if (addr == BCST_ADDR) {
		if (enable) {
			cec_write(CEC_ADDR_H, 0x80);
			cec_write(CEC_ADDR_L, 0x00);
			return 0;
		} else {
			pr_err("Error:cannot de-allocate broadcast logical address\n");
			return -1;
		}
	}
	regs = (addr > 7) ? CEC_ADDR_H : CEC_ADDR_L;
	addr = (addr > 7) ? (addr - 8) : addr;
	if (enable) {
		cec_write(CEC_ADDR_H, 0x00);
		cec_write(regs, cec_read(regs) |  (1 << addr));
	} else
		cec_write(regs, cec_read(regs) & ~(1 << addr));
	return 0;
}

unsigned int cec_get_log_addr(void)
{
	unsigned int reg, addr = 16, i;

	if (!cec_active) {
		pr_err("[CEC]cec_get_log_addr failed\n");
		return -1;
	}
	reg = cec_read(CEC_ADDR_H);
	reg = (reg << 8) | cec_read(CEC_ADDR_L);

	for (i = 0; i < 16; i++) {
		if ((reg >> i) & 0x01)
			addr = i;
	}

	return addr;
}


/**
 * Configure standby mode
 * @param enable: if true enable standby mode
 * @return error code
 */
static int cec_CfgStandbyMode(int enable)
{
	if (!cec_active) {
		pr_err("[CEC]cec_CfgStandbyMode failed\n");
		return -1;
	}
	if (enable)
		cec_write(CEC_CTRL, cec_read(CEC_CTRL)
						| CEC_CTRL_STANDBY_MASK);
	else
		cec_write(CEC_CTRL, cec_read(CEC_CTRL)
					& ~CEC_CTRL_STANDBY_MASK);
	return 0;
}



/**
 * Configure signal free time
 * @param time: time between attempts [nr of frames]
 * @return error code
 */
static int cec_CfgSignalFreeTime(int time)
{
	unsigned char data;

	data = cec_read(CEC_CTRL) & ~(CEC_CTRL_FRAME_TYP_MASK);
	if (time == 3)
		;                                  /* 0 */
	else if (time == 5)
		data |= FRAME_TYP0;                /* 1 */
	else if (time == 7)
		data |= FRAME_TYP1 | FRAME_TYP0;   /* 2 */
	else
		return -1;
	cec_write(CEC_CTRL, data);
	return 0;
}

/**
 * Read send status
 * @return SEND status
 */
static int cec_GetSend(void)
{
	return (cec_read(CEC_CTRL) & CEC_CTRL_SEND_MASK) != 0;
}

/**
 * Set send status
 * @return error code
 */
static int cec_SetSend(void)
{
	cec_write(CEC_CTRL, cec_read(CEC_CTRL) | CEC_CTRL_SEND_MASK);
	return 0;
}

/**
 * Clear interrupts
 * @param mask: interrupt mask to clear
 * @return error code
 */
static int cec_IntClear(unsigned char mask)
{
	cec_write(CEC_MASK, mask);
	return 0;
}

static void cec_wakeupctrl_clear(void)
{
	cec_write(CEC_WAKEUPCTRL, 0);
}

/**
 * Disable interrupts
 * @param mask: interrupt mask to disable
 * @return error code
 */
static int cec_IntDisable(unsigned char mask)
{
	cec_write(CEC_MASK, cec_read(CEC_MASK) | mask);
	return 0;
}

/**
 * Enable interrupts
 * @param mask: interrupt mask to enable
 * @return error code
 */
static int cec_IntEnable(unsigned char mask)
{
	cec_write(CEC_MASK, cec_read(CEC_MASK) & ~mask);
	return 0;
}

/**
 * Read interrupts
 * @param mask: interrupt mask to read
 * @return INT content
 */
static int cec_IntStatus(unsigned char mask)
{
	return cec_read(IH_CEC_STAT0) & mask;
}

static int cec_IntStatusClear(unsigned char mask)
{
	/* write 1 to clear */
	cec_write(IH_CEC_STAT0, mask);
	return 0;
}

/**
 * Write transmission buffer
 * @param buf:  data to transmit
 * @param size: data length [byte]
 * @return error code or bytes configured
 */
static int cec_CfgTxBuf(char *buf, unsigned size)
{
	unsigned i;
	if (size > CEC_TX_DATA_SIZE) {
		pr_info("invalid parameter\n");
		return -1;
	}
	cec_write(CEC_TX_CNT, size);   /* mask 7-5? */
	for (i = 0; i < size; i++)
		cec_write(CEC_TX_DATA + (i * 4), *buf++);
	return size;
}

/**
 * Read reception buffer
 * @param buf:  buffer to hold receive data
 * @param size: maximum data length [byte]
 * @return error code or received bytes
 */
static int cec_CfgRxBuf(char *buf, unsigned size)
{
	unsigned i;
	unsigned char cnt;

	cnt = cec_read(CEC_RX_CNT);   /* mask 7-5? */
	cnt = (cnt > size) ? size : cnt;
	for (i = 0; i < size; i++)
		*buf++ = cec_read(CEC_RX_DATA + (i * 4));

	if (cnt > CEC_RX_DATA_SIZE) {
		pr_err("Error: wrong byte count\n");
		return -1;
	}
	return cnt;
}

/**
 * Read locked status
 * @return LOCKED status
 */
static int cec_GetLocked(void)
{
	return (cec_read(CEC_LOCK) & CEC_LOCK_LOCKED_BUFFER_MASK) != 0;
}

/**
 * Set locked status
 * @return error code
 */
static int cec_SetLocked(void)
{
	cec_write(CEC_LOCK, cec_read(CEC_LOCK)
					& ~CEC_LOCK_LOCKED_BUFFER_MASK);
	return true;
}

/**
 * Attempt to receive data (quit after 1s)
 * @warning Caller context must allow sleeping
 * @param buf:  container for incoming data (first byte will be header block)
 * @param size: maximum data size [byte]
 * @return error code or received message size [byte]
 */
int cec_ctrlReceiveFrame(char *buf, unsigned size)
{
	if (buf == NULL) {
		pr_err("Error:invalid parameter\n");
		return -1;
	}

	if (!cec_active) {
		pr_err("[CEC]cec_ctrlReceiveFrame failed\n");
		return -1;
	}

	return cec_msgRx(buf, size, 100);
}

/**
 * Attempt to send data (retry 5 times)
 * @warning Doesn't check if source address is allocated
 * @param buf:  container of the outgoing data (without header block)
 * @param size: request size [byte] (must be less than 15)
 * @param src:  initiator logical address
 * @param dst:  destination logical address
 * @return error code:
 *    -1:ERROR     0: send OK     1: NACK
 */
int cec_ctrlSendFrame(char *buf, unsigned size,
		  unsigned src, unsigned dst)
{
	char tmp[100];
	int i;

	if (buf == NULL || size >= CEC_TX_DATA_SIZE ||
	    src > BCST_ADDR || dst > BCST_ADDR) {
		pr_err("invalid parameter\n");
		return -1;
	}

	if (!cec_active) {
		pr_err("[CEC]cec_ctrlSendFrame failed\n");
		return -1;
	}

	tmp[0] = src << 4 | dst;
	for (i = 0; i < (int)size; i++)
		tmp[i + 1] = buf[i];

	return cec_msgTx(tmp, size + 1, 5);
}
