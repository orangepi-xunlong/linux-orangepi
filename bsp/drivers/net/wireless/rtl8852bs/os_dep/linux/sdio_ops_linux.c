/******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#define _SDIO_OPS_LINUX_C_

#include <drv_types.h>

static bool rtw_sdio_claim_host_needed(struct sdio_func *func)
{
	struct dvobj_priv *dvobj = sdio_get_drvdata(func);
	struct sdio_data *sdio_data = dvobj_to_sdio(dvobj);

	if (sdio_data->sys_sdio_irq_thd && sdio_data->sys_sdio_irq_thd == current)
		return _FALSE;
	return _TRUE;
}

/*#define RTW_SDIO_DUMP*/
#ifdef RTW_SDIO_DUMP
#define DUMP_LEN_LMT	0	/* buffer dump size limit */
				/* unit: byte, 0 for no limit */
#else
#define DUMP_LEN_LMT	32
#endif
#define GET_DUMP_LEN(len)	(DUMP_LEN_LMT ? rtw_min(len, DUMP_LEN_LMT) : len)

#ifdef DBG_SDIO
#if (DBG_SDIO >= 1)
static void sdio_dump_reg_by_cmd52(struct dvobj_priv *d,
				   u32 addr, size_t len, u8 *buf)
{
	struct sdio_func *func;
	size_t i;
	u8 val;
	u8 str[80], used = 0;
	u8 read_twice = 0;
	int error;


	if (buf)
		_rtw_memset(buf, 0xAE, len);
	func = dvobj_to_sdio_func(d);
	/*
	 * When register is WLAN IOREG,
	 * read twice to guarantee the result is correct.
	 */
	if (addr & 0x10000)
		read_twice = 1;

	_rtw_memset(str, 0, 80);
	used = 0;
	if (addr & 0xF) {
		used += snprintf(str+used, (80-used), "0x%02x:\t", addr&~0xF);
		used += snprintf(str+used, (80-used), "%*s", (addr&0xF)*5, "");
	}
	for (i = 0; i < len; i++, addr++) {
		val = sdio_readb(func, addr, &error);
		if (read_twice)
			val = sdio_readb(func, addr, &error);
		if (error)
			break;

		if (buf)
			buf[i] = val;

		if (!(addr & 0xF))
			used += snprintf(str+used, (80-used), "0x%02x:\t", addr&~0xF);
		used += snprintf(str+used, (80-used), "%02x ", val);
		if (((i + 1) < len) && ((addr + 1) & 0xF) == 0) {
			dev_err(&func->dev, "%s", str);
			_rtw_memset(str, 0, 80);
			used = 0;
		}
	}

	if (used) {
		dev_err(&func->dev, "%s", str);
		_rtw_memset(str, 0, 80);
		used = 0;
	}

	if (error)
		dev_err(&func->dev, "rtw_sdio_dbg: READ 0x%02x FAIL!", addr);
}

static void sdio_dump_cia(struct dvobj_priv *d, u32 addr, size_t len, u8 *buf)
{
	struct sdio_func *func;
	size_t i;
	u8 val;
	u8 str[80], used = 0;
	int error;


	if (buf)
		_rtw_memset(buf, 0xAE, len);
	func = dvobj_to_sdio_func(d);

	_rtw_memset(str, 0, 80);
	used = 0;
	if (addr & 0xF) {
		used += snprintf(str+used, (80-used), "0x%02x:\t", addr&~0xF);
		used += snprintf(str+used, (80-used), "%*s", (addr&0xF)*5, "");
	}
	for (i = 0; i < len; i++, addr++) {
		val = sdio_f0_readb(func, addr, &error);
		if (error)
			break;

		if (buf)
			buf[i] = val;

		if (!(addr & 0xF))
			used += snprintf(str+used, (80-used), "0x%02x:\t", addr&~0xF);
		used += snprintf(str+used, (80-used), "%02x ", val);
		if (((i + 1) < len) && ((addr + 1) & 0xF) == 0) {
			dev_err(&func->dev, "%s", str);
			_rtw_memset(str, 0, 80);
			used = 0;
		}
	}

	if (used) {
		dev_err(&func->dev, "%s", str);
		_rtw_memset(str, 0, 80);
		used = 0;
	}

	if (error)
		dev_err(&func->dev, "rtw_sdio_dbg: READ CIA 0x%02x FAIL!",
			addr);
}

#if (DBG_SDIO >= 2)
void rtw_sdio_dbg_reg_alloc(struct dvobj_priv *d);
#endif /* DBG_SDIO >= 2 */

/*
 * Dump register when CMD53 fail
 */
static void sdio_dump_dbg_reg(struct dvobj_priv *d, u8 write,
			      unsigned int addr, size_t len)
{
	struct sdio_data *sdio;
	struct sdio_func *func;
	u8 *buf = NULL;
#if (DBG_SDIO >= 2)
	u8 *msg;
#endif /* DBG_SDIO >= 2 */


	sdio = dvobj_to_sdio(d);
	if (sdio->reg_dump_mark)
		return;
	func = dvobj_to_sdio_func(d);

	sdio->reg_dump_mark = sdio->cmd53_err_cnt;

#if (DBG_SDIO >= 2)
	if (!sdio->dbg_msg) {
		msg = rtw_zmalloc(80);
		if (msg) {
			sdio->dbg_msg = msg;
			sdio->dbg_msg_size = 80;
		}
	}
	if (sdio->dbg_msg_size) {
		snprintf(sdio->dbg_msg, sdio->dbg_msg_size,
			 "CMD53 %s 0x%05x, %zu bytes FAIL "
			 "at err_cnt=%d",
			 write?"WRITE":"READ",
			 addr, len, sdio->reg_dump_mark);
	}

	rtw_sdio_dbg_reg_alloc(d);
#endif /* DBG_SDIO >= 2 */

	/* MAC register */
	dev_err(&func->dev, "MAC register:");
#if (DBG_SDIO >= 2)
	buf = sdio->reg_mac;
#endif /* DBG_SDIO >= 2 */
	sdio_dump_reg_by_cmd52(d, 0x10000, 0x800, buf);
	dev_err(&func->dev, "MAC Extend register:");
#if (DBG_SDIO >= 2)
	buf = sdio->reg_mac_ext;
#endif /* DBG_SDIO >= 2 */
	sdio_dump_reg_by_cmd52(d, 0x11000, 0x800, buf);

	/* SDIO local register */
	dev_err(&func->dev, "SDIO Local register:");
#if (DBG_SDIO >= 2)
	buf = sdio->reg_local;
#endif /* DBG_SDIO >= 2 */
	sdio_dump_reg_by_cmd52(d, 0x0, 0x100, buf);

	/* F0 */
	dev_err(&func->dev, "f0 register:");
#if (DBG_SDIO >= 2)
	buf = sdio->reg_cia;
#endif /* DBG_SDIO >= 2 */
	sdio_dump_cia(d, 0x0, 0x200, buf);
}
#endif /* DBG_SDIO >= 1 */
#endif /* DBG_SDIO */

/**
 *	Returns driver error code,
 *	0	no error
 *	-1	Level 1 error, critical error and can't be recovered
 *	-2	Level 2 error, normal error, retry to recover is possible
 */
static int linux_io_err_to_drv_err(int err)
{
	if (!err)
		return 0;

	/* critical error */
	if ((err == -ESHUTDOWN) ||
	    (err == -ENODEV) ||
	    (err == -ENOMEDIUM))
		return -1;

	/* other error */
	return -2;
}

/**
 *	rtw_sdio_raw_read - Read from SDIO device
 *	@d: driver object private data
 *	@addr: address to read
 *	@buf: buffer to store the data
 *	@len: number of bytes to read
 *	@fixed:
 *
 *	Reads from the address space of a SDIO device.
 *	Return value indicates if the transfer succeeded or not.
 */
int __must_check rtw_sdio_raw_read(struct dvobj_priv *d, unsigned int addr,
				   void *buf, size_t len, bool fixed)
{
	int error = -EPERM;
	bool f0, cmd52;
	struct sdio_func *func;
	bool claim_needed;
	u32 offset, i;
	struct sdio_data *sdio;
	u8 *tmpbuf = NULL;


	func = dvobj_to_sdio_func(d);
	claim_needed = rtw_sdio_claim_host_needed(func);
	f0 = RTW_SDIO_ADDR_F0_CHK(addr);
	cmd52 = RTW_SDIO_ADDR_CMD52_CHK(addr);
	sdio = dvobj_to_sdio(d);

	/*
	 * Mask addr to remove driver defined bit and
	 * make sure addr is in valid range
	 */
	if (f0)
		addr &= 0xFFF;
	else
		addr &= 0x1FFFF;

#ifdef RTW_SDIO_DUMP
	if (f0)
		dev_dbg(&func->dev, "RTW_SDIO: READ F0\n");
	else if (cmd52)
		dev_dbg(&func->dev, "RTW_SDIO: READ use CMD52\n");
	else
		dev_dbg(&func->dev, "RTW_SDIO: READ use CMD53\n");

	dev_dbg(&func->dev, "RTW_SDIO: READ from 0x%05x\n", addr);
#endif /* RTW_SDIO_DUMP */

	if (claim_needed)
		sdio_claim_host(func);

	if (f0) {
		offset = addr;
		for (i = 0; i < len; i++, offset++) {
			((u8 *)buf)[i] = sdio_f0_readb(func, offset, &error);
			if (error)
				break;
#if 0
			dev_info(&func->dev, "%s: sdio f0 read 52 addr 0x%x, byte 0x%02x\n",
				 __func__, offset, ((u8 *)buf)[i]);
#endif
		}
	} else {
		if (cmd52) {
#ifdef RTW_SDIO_IO_DBG
			dev_info(&func->dev, "%s: sdio read 52 addr 0x%x, %zu bytes\n",
				 __func__, addr, len);
#endif
			offset = addr;
			for (i = 0; i < len; i++) {
				((u8 *)buf)[i] = sdio_readb(func, offset, &error);
				if (error)
					break;
#if 0
				dev_info(&func->dev, "%s: sdio read 52 addr 0x%x, byte 0x%02x\n",
					 __func__, offset, ((u8 *)buf)[i]);
#endif
				if (!fixed)
					offset++;
			}
		} else {
#ifdef RTW_SDIO_IO_DBG
			dev_info(&func->dev, "%s: sdio read 53 addr 0x%x, %zu bytes\n",
				 __func__, addr, len);
#endif
			if (len <= sdio->tmpbuf_sz) {
				tmpbuf = buf;
				buf = sdio->tmpbuf;
			}
			if (fixed)
				error = sdio_readsb(func, buf, addr, len);
			else
				error = sdio_memcpy_fromio(func, buf, addr, len);
			if (!error && tmpbuf)
				_rtw_memcpy(tmpbuf, buf, len);
		}
	}

#ifdef DBG_SDIO
#if (DBG_SDIO >= 3)
	if (!error && !f0 && !cmd52
	    && (sdio->dbg_enable
		&& sdio->err_test && !sdio->err_test_triggered
		&& ((addr & 0x10000)
		    || (!(addr & 0xE000)
			&& !((addr >= 0x40) && (addr < 0x48)))))) {
		sdio->err_test_triggered = 1;
		error = -ETIMEDOUT;
		dev_warn(&func->dev, "Simulate error(%d) READ addr=0x%05x %zu bytes",
			 error, addr, len);
	}
#endif /* DBG_SDIO >= 3 */

	if (error) {
		if (f0 || cmd52) {
			sdio->cmd52_err_cnt++;
		} else {
			sdio->cmd53_err_cnt++;
#if (DBG_SDIO >= 1)
			sdio_dump_dbg_reg(d, 0, addr, len);
#endif /* DBG_SDIO >= 1 */
		}
	}
#endif /* DBG_SDIO */

	if (claim_needed)
		sdio_release_host(func);

#ifdef RTW_SDIO_DUMP
	print_hex_dump(KERN_DEBUG, "RTW_SDIO: READ ",
		       DUMP_PREFIX_OFFSET, 16, 1,
		       buf, GET_DUMP_LEN(len), false);
#endif /* RTW_SDIO_DUMP */

	if (WARN_ON(error)) {
		dev_err(&func->dev, "%s: sdio read failed (%d)\n", __func__, error);
#ifndef RTW_SDIO_DUMP
		if (f0)
			dev_err(&func->dev, "RTW_SDIO: READ F0\n");
		if (cmd52)
			dev_err(&func->dev, "RTW_SDIO: READ use CMD52\n");
		else
			dev_err(&func->dev, "RTW_SDIO: READ use CMD53\n");
		dev_err(&func->dev, "RTW_SDIO: READ from 0x%05x, %zu bytes\n", addr, len);
		print_hex_dump(KERN_ERR, "RTW_SDIO: READ ",
			       DUMP_PREFIX_OFFSET, 16, 1,
			       buf, GET_DUMP_LEN(len), false);
#endif /* !RTW_SDIO_DUMP */
	}

	return linux_io_err_to_drv_err(error);
}

/**
 *	rtw_sdio_raw_write - Write to SDIO device
 *	@d: driver object private data
 *	@addr: address to write
 *	@buf: buffer that contains the data to write
 *	@len: number of bytes to write
 *	@fixed: address is fixed(FIFO) or incremented
 *
 *	Writes to the address space of a SDIO device.
 *	Return value indicates if the transfer succeeded or not.
 */
int __must_check rtw_sdio_raw_write(struct dvobj_priv *d, unsigned int addr,
				    void *buf, size_t len, bool fixed)
{
	int error = -EPERM;
	bool f0, cmd52;
	struct sdio_func *func;
	bool claim_needed;
	u32 offset, i;
	struct sdio_data *sdio;


	func = dvobj_to_sdio_func(d);
	claim_needed = rtw_sdio_claim_host_needed(func);
	f0 = RTW_SDIO_ADDR_F0_CHK(addr);
	cmd52 = RTW_SDIO_ADDR_CMD52_CHK(addr);
	sdio = dvobj_to_sdio(d);

	/*
	 * Mask addr to remove driver defined bit and
	 * make sure addr is in valid range
	 */
	if (f0)
		addr &= 0xFFF;
	else
		addr &= 0x1FFFF;

#ifdef RTW_SDIO_DUMP
	if (f0)
		dev_dbg(&func->dev, "RTW_SDIO: WRITE F0\n");
	else if (cmd52)
		dev_dbg(&func->dev, "RTW_SDIO: WRITE use CMD52\n");
	else
		dev_dbg(&func->dev, "RTW_SDIO: WRITE use CMD53\n");
	dev_dbg(&func->dev, "RTW_SDIO: WRITE to 0x%05x\n", addr);
	print_hex_dump(KERN_DEBUG, "RTW_SDIO: WRITE ",
		       DUMP_PREFIX_OFFSET, 16, 1,
		       buf, GET_DUMP_LEN(len), false);
#endif /* RTW_SDIO_DUMP */

	if (claim_needed)
		sdio_claim_host(func);

	if (f0) {
		offset = addr;
		for (i = 0; i < len; i++, offset++) {
			sdio_f0_writeb(func, ((u8 *)buf)[i], offset, &error);
			if (error)
				break;
#if 0
			dev_info(&func->dev, "%s: sdio f0 write 52 addr 0x%x, byte 0x%02x\n",
				 __func__, offset, ((u8 *)buf)[i]);
#endif
		}
	} else {
		if (cmd52) {
#ifdef RTW_SDIO_IO_DBG
			dev_info(&func->dev, "%s: sdio write 52 addr 0x%x, %zu bytes\n",
				 __func__, addr, len);
#endif
			offset = addr;
			for (i = 0; i < len; i++) {
				sdio_writeb(func, ((u8 *)buf)[i], offset, &error);
				if (error)
					break;
#if 0
				dev_info(&func->dev, "%s: sdio write 52 addr 0x%x, byte 0x%02x\n",
					 __func__, offset, ((u8 *)buf)[i]);
#endif
				if (!fixed)
					offset++;
			}
		} else {
#ifdef RTW_SDIO_IO_DBG
			dev_info(&func->dev, "%s: sdio write 53 addr 0x%x, %zu bytes\n",
				 __func__, addr, len);
#endif
			if (len <= sdio->tmpbuf_sz) {
				_rtw_memcpy(sdio->tmpbuf, buf, len);
				buf = sdio->tmpbuf;
			}
			if (fixed)
				error = sdio_writesb(func, addr, buf, len);
			else
				error = sdio_memcpy_toio(func, addr, buf, len);
		}
	}

#ifdef DBG_SDIO
	if (error) {
		if (f0 || cmd52) {
			sdio->cmd52_err_cnt++;
		} else {
			sdio->cmd53_err_cnt++;
#if (DBG_SDIO >= 1)
			sdio_dump_dbg_reg(d, 1, addr, len);
#endif /* DBG_SDIO >= 1 */
		}
	}
#endif /* DBG_SDIO */

	if (claim_needed)
		sdio_release_host(func);

	if (WARN_ON(error)) {
		dev_err(&func->dev, "%s: sdio write failed (%d)\n", __func__, error);
#ifndef RTW_SDIO_DUMP
		if (f0)
			dev_err(&func->dev, "RTW_SDIO: WRITE F0\n");
		if (cmd52)
			dev_err(&func->dev, "RTW_SDIO: WRITE use CMD52\n");
		else
			dev_err(&func->dev, "RTW_SDIO: WRITE use CMD53\n");
		dev_err(&func->dev, "RTW_SDIO: WRITE to 0x%05x, %zu bytes\n", addr, len);
		print_hex_dump(KERN_ERR, "RTW_SDIO: WRITE ",
			       DUMP_PREFIX_OFFSET, 16, 1,
			       buf, GET_DUMP_LEN(len), false);
#endif /* !RTW_SDIO_DUMP */
	}

	return linux_io_err_to_drv_err(error);
}
