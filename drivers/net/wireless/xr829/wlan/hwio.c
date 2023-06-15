/*
 * Hardware I/O implementation for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>

#include "xradio.h"
#include "hwio.h"
#include "sbus.h"


static int __xradio_read(struct xradio_common *hw_priv, u16 addr,
			 void *buf, size_t buf_len, int buf_id)
{
	u16 addr_sdio;
	u32 sdio_reg_addr_17bit ;

#if (CHECK_ADDR_LEN)
	/* Check if buffer is aligned to 4 byte boundary */
	if (SYS_WARN(((unsigned long)buf & 3) && (buf_len > 4))) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: buffer is not aligned.\n",
			    __func__);
		return -EINVAL;
	}
#endif

	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);
	SYS_BUG(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_data_read(hw_priv->sbus_priv,
						 sdio_reg_addr_17bit,
						 buf, buf_len);
}

static int __xradio_write(struct xradio_common *hw_priv, u16 addr,
			      const void *buf, size_t buf_len, int buf_id)
{
	u16 addr_sdio;
	u32 sdio_reg_addr_17bit ;

#if (CHECK_ADDR_LEN)
	/* Check if buffer is aligned to 4 byte boundary */
	if (SYS_WARN(((unsigned long)buf & 3) && (buf_len > 4))) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: buffer is not aligned.\n",
			    __func__);
		return -EINVAL;
	}
#endif

	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);

	SYS_BUG(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_data_write(hw_priv->sbus_priv,
						  sdio_reg_addr_17bit,
						  buf, buf_len);
}

static inline int __xradio_read_reg32(struct xradio_common *hw_priv,
				       u16 addr, u32 *val)
{
	int ret = 0;

	*hw_priv->sbus_priv->val32_r = 0;
	ret = __xradio_read(hw_priv, addr, hw_priv->sbus_priv->val32_r, sizeof(u32), 0);
	*val = *hw_priv->sbus_priv->val32_r;

	return ret;
}

static inline int __xradio_write_reg32(struct xradio_common *hw_priv,
					u16 addr, u32 val)
{
	int ret = 0;

	*hw_priv->sbus_priv->val32_w = val;
	ret = __xradio_write(hw_priv, addr, hw_priv->sbus_priv->val32_w, sizeof(u32), 0);
	*hw_priv->sbus_priv->val32_w = 0;

	return ret;
}

int xradio_reg_read(struct xradio_common *hw_priv, u16 addr,
		    void *buf, size_t buf_len)
{
	int ret;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	*hw_priv->sbus_priv->val32_r = 0;
	ret = __xradio_read(hw_priv, addr, hw_priv->sbus_priv->val32_r, buf_len, 0);
	*(u32 *)buf = *hw_priv->sbus_priv->val32_r;
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_reg_write(struct xradio_common *hw_priv, u16 addr,
		     const void *buf, size_t buf_len)
{
	int ret;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	*hw_priv->sbus_priv->val32_w = *(u32 *)buf;
	ret = __xradio_write(hw_priv, addr, hw_priv->sbus_priv->val32_w, buf_len, 0);
	*hw_priv->sbus_priv->val32_w = 0;
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_reg_bit_operate(struct xradio_common *hw_priv, u16 addr, u32 set, u32 clr)
{
	int ret;
	u32 val32;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	ret = __xradio_read_reg32(hw_priv, addr, &val32);

	if (ret < 0 && 1 == addr) { //means control reg read failed
		ret = __xradio_read_reg32(hw_priv, addr, &val32);
		printk(KERN_ERR"[SDIO-DCE] read control reg agian and val is 0x%x\n", val32);
	}

	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: Can't read reg.\n", __func__);
		goto out;
	}
	val32 &= ~clr;
	val32 |= set;
	ret = __xradio_write_reg32(hw_priv, addr, val32);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: Can't write reg.\n", __func__);
	}
out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_data_read(struct xradio_common *hw_priv, void *buf, size_t buf_len)
{
	int ret, retry = 1;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	{
		int buf_id_rx = hw_priv->buf_id_rx;
		while (retry <= MAX_RETRY) {
			ret = __xradio_read(hw_priv, HIF_IN_OUT_QUEUE_REG_ID, buf,
					    buf_len, buf_id_rx + 1);
			if (!ret) {
				buf_id_rx = (buf_id_rx + 1) & 3;
				hw_priv->buf_id_rx = buf_id_rx;
				break;
			} else {
				retry++;
				mdelay(1);
				sbus_printk(XRADIO_DBG_ERROR, "%s, error :[%d]\n",
					    __func__, ret);
			}
		}
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_data_write(struct xradio_common *hw_priv, const void *buf,
		      size_t buf_len)
{
	int ret, retry = 1;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	{
		int buf_id_tx = hw_priv->buf_id_tx;
		while (retry <= MAX_RETRY) {
			ret = __xradio_write(hw_priv, HIF_IN_OUT_QUEUE_REG_ID, buf,
					     buf_len, buf_id_tx);
			if (!ret) {
				buf_id_tx = (buf_id_tx + 1) & 31;
				hw_priv->buf_id_tx = buf_id_tx;
				break;
			} else {
				retry++;
				mdelay(1);
				sbus_printk(XRADIO_DBG_ERROR, "%s, error :[%d]\n",
					    __func__, ret);
			}
		}
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_indirect_read(struct xradio_common *hw_priv, u32 addr, void *buf,
			 size_t buf_len, u32 prefetch, u16 port_addr)
{
	u32 val32 = 0;
	int i, ret;

	if ((buf_len / 2) >= 0x1000) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't read more than 0xfff words.\n",
			    __func__);
		return -EINVAL;
		goto out;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	/* Write address */
	ret = __xradio_write_reg32(hw_priv, HIF_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write address register.\n", __func__);
		goto out;
	}

	/* Read CONFIG Register Value - We will read 32 bits */
	ret = __xradio_read_reg32(hw_priv, HIF_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't read config register.\n", __func__);
		goto out;
	}

	/* Set PREFETCH bit */
	ret = __xradio_write_reg32(hw_priv, HIF_CONFIG_REG_ID, val32 | prefetch);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write prefetch bit.\n", __func__);
		goto out;
	}

	/* Check for PRE-FETCH bit to be cleared */
	for (i = 0; i < 20; i++) {
		ret = __xradio_read_reg32(hw_priv, HIF_CONFIG_REG_ID, &val32);
		if (ret < 0) {
			sbus_printk(XRADIO_DBG_ERROR,
				    "%s: Can't check prefetch bit.\n", __func__);
			goto out;
		}
		if (!(val32 & prefetch))
			break;
		mdelay(i);
	}

	if (val32 & prefetch) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Prefetch bit is not cleared.\n", __func__);
		goto out;
	}

	/* Read data port */
	if (buf_len == sizeof(u32)) {
		*hw_priv->sbus_priv->val32_r = 0;
		ret = __xradio_read(hw_priv, port_addr, hw_priv->sbus_priv->val32_r, buf_len, 0);
		*(u32 *)buf = *hw_priv->sbus_priv->val32_r;
	} else {
		ret = __xradio_read(hw_priv, port_addr, buf, buf_len, 0);
	}
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't read data port.\n", __func__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_apb_write(struct xradio_common *hw_priv, u32 addr, const void *buf,
		     size_t buf_len)
{
	int ret;

	if ((buf_len / 2) >= 0x1000) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't wrire more than 0xfff words.\n", __func__);
		return -EINVAL;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);

	/* Write address */
	ret = __xradio_write_reg32(hw_priv, HIF_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write address register.\n", __func__);
		goto out;
	}

	/* Write data port */
	if (buf_len == sizeof(u32)) {
		memcpy(hw_priv->sbus_priv->val32_w, buf, sizeof(u32));
		ret = __xradio_write(hw_priv, HIF_SRAM_DPORT_REG_ID,
				hw_priv->sbus_priv->val32_w, buf_len, 0);
		memset(hw_priv->sbus_priv->val32_w, 0, sizeof(u32));
	} else {
		ret = __xradio_write(hw_priv, HIF_SRAM_DPORT_REG_ID, buf, buf_len, 0);
	}

	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: Can't write data port.\n",
			    __func__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_ahb_write(struct xradio_common *hw_priv, u32 addr, const void *buf,
		     size_t buf_len)
{
	int ret;

	if ((buf_len / 2) >= 0x1000) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't wrire more than 0xfff words.\n",
			     __func__);
		return -EINVAL;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);

	/* Write address */
	ret = __xradio_write_reg32(hw_priv, HIF_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write address register.\n", __func__);
		goto out;
	}

	/* Write data port */
	if (buf_len == sizeof(u32)) {
		memcpy(hw_priv->sbus_priv->val32_w, buf, sizeof(u32));
		ret = __xradio_write(hw_priv, HIF_AHB_DPORT_REG_ID,
				hw_priv->sbus_priv->val32_w, buf_len, 0);
		memset(hw_priv->sbus_priv->val32_w, 0, sizeof(u32));
	} else {
		ret = __xradio_write(hw_priv, HIF_AHB_DPORT_REG_ID, buf, buf_len, 0);
	}

	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write data port.\n", __func__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}
