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

#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>

#include "esm_lib/include/ESMHost.h"
#include "dw_fc.h"
#include "dw_mc.h"
#include "dw_hdcp.h"
#include "dw_hdcp22.h"

#define ESM_REG_BASE_OFFSET    (0x10000)
#define DW_ESM_FW_SIZE         (256 * 1024)
#define DW_ESM_DATA_SIZE       (128 * 1024)
#define DW_ESM_PAIRDATA_SIZE   (300)

static struct dw_hdcp_s  *hdcp;
static esm_instance_t  *esm;

static inline void _dw_hdcp2x_int_mute(u8 value)
{
	dw_write(HDCP22REG_MUTE, value);
}

/**
 * @desc: chose which way to enable hdcp22
 * @enable: 0 - chose to enable hdcp22 by dwc_hdmi inner signal ist_hdcp_capable
 *          1 - chose to enable hdcp22 by hdcp22_ovr_val bit
 */
static void _dw_hdcp2x_ovr_set_avmute(u8 ovr_val, u8 ovr_en)
{
	dw_write_mask(HDCP22REG_CTRL1,
			HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_VAL_MASK, ovr_val);
	dw_write_mask(HDCP22REG_CTRL1,
			HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_EN_MASK, ovr_en);
}

/**
 * @desc chose what place hdcp22 hpd come from and config hdcp22 hpd enable or disable
 * @val: 0 - hdcp22 hpd come from phy: phy_stat0.HPD
 *       1 - hdcp22 hpd come from hpd_ovr_val
 * @enable:hpd_ovr_val
 */
static void _dw_hdcp2x_ovr_set_hpd(u8 ovr_val, u8 ovr_en)
{
	dw_write_mask(HDCP22REG_CTRL, HDCP22REG_CTRL_HPD_OVR_VAL_MASK, ovr_val);
	dw_write_mask(HDCP22REG_CTRL, HDCP22REG_CTRL_HPD_OVR_EN_MASK, ovr_en);
}

void _dw_hdcp2x_data_enable(u8 enable)
{
	if (enable == DW_HDMI_ENABLE) {
		_dw_hdcp2x_ovr_set_avmute(DW_HDMI_DISABLE, DW_HDMI_ENABLE);
		dw_mc_set_clk(DW_MC_CLK_HDCP, DW_HDMI_ENABLE);
		return;
	}

	dw_mc_set_clk(DW_MC_CLK_HDCP, DW_HDMI_DISABLE);
	_dw_hdcp2x_ovr_set_avmute(DW_HDMI_DISABLE, DW_HDMI_ENABLE);
}

/**
 * @path: 0 - hdcp14 path
 *        1 - hdcp22 path
 */
void dw_hdcp2x_ovr_set_path(u8 ovr_val, u8 ovr_en)
{
	dw_write_mask(HDCP22REG_CTRL, HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK, ovr_val);
	dw_write_mask(HDCP22REG_CTRL, HDCP22REG_CTRL_HDCP22_OVR_EN_MASK, ovr_en);
}

u8 dw_hdcp2x_get_path(void)
{
	return dw_read_mask(HDCP22REG_STS, HDCP22REG_STS_HDCP22_SWITCH);
}

static int _esm_hpi_read(void *instance, uint32_t offset, uint32_t *data)
{
	unsigned long addr =  esm->driver->hpi_base + offset;

	*data = *((volatile u32 *)addr);
	mdelay(1);
	return 0;
}

static int _esm_hpi_write(void *instance, uint32_t offset, uint32_t data)
{
	unsigned long addr = esm->driver->hpi_base + offset;
	*((volatile u32 *)addr) = data;
	mdelay(1);
	return 0;
}

static int _esm_data_read(void *instance, uint32_t offset,
		uint8_t *dest_buf, uint32_t nbytes)
{
	memcpy(dest_buf, (u8 *)(esm->driver->vir_data_base + offset), nbytes);
	return 0;
}

static int _esm_data_write(void *instance, uint32_t offset,
		uint8_t *src_buf, uint32_t nbytes)
{
	memcpy((u8 *)(esm->driver->vir_data_base + offset), src_buf, nbytes);
	return 0;
}

static int _esm_data_set(void *instance, uint32_t offset,
		uint8_t data, uint32_t nbytes)
{
	memset((u8 *)(esm->driver->vir_data_base + offset), data, nbytes);
	return 0;
}

static int _esm_address_dump(void)
{
	hdcp_log("[esm address]\n");
	hdcp_log(" - code physical addr: 0x%x, virtual addr: 0x%x\n",
		esm->driver->code_base, (u32)esm->driver->vir_code_base);
	hdcp_log(" - data physical addr: 0x%x, virtual addr: 0x%x\n",
		esm->driver->data_base, (u32)esm->driver->vir_data_base);
	return 0;
}

static int _dw_hdcp2x_start_auth(void)
{
	int ret = 0;

	mutex_lock(&hdcp->lock_esm_auth);
	ret = (ESM_Authenticate(esm, 1, 1, 0) == ESM_HL_SUCCESS) ? 0 : -1;
	mutex_unlock(&hdcp->lock_esm_auth);

	hdmi_inf("dw esm auth: %s\n", ret == 0 ? "success" : "failed");
	return ret;
}

static void _dw_hdcp2x_auth_work(struct work_struct *work)
{
	int ret = 0;

	if (hdcp->esm_auth_done)
		return;

	/* wait 2s */
	msleep(2000);

	ret = _dw_hdcp2x_start_auth();
	if (ret != 0) {
		hdmi_err("dw hdcp2x set authenticate failed\n");
		return;
	}

	hdcp->esm_auth_done = DW_HDMI_ENABLE;
}

/**
 * @desc: hdcp2x get state by esm hardware
 * @return: -1 - state failed
 *           0 - state success
 *           1 - state processing
 */
static int _dw_hdcp2x_get_encry_state(void)
{
	esm_status_t Status = {0, 0, 0, 0};
	uint32_t state = 0;
	ESM_STATUS ret = ESM_HL_SUCCESS;

	ret = ESM_GetState(esm, &state, &Status);
	if (ret != ESM_HL_SUCCESS)
		return -1;

	if (Status.esm_sync_lost) {
		hdmi_err("[hdcp2.2-error]esm sync lost!\n");
		return -1;
	}

	if (Status.esm_exception) {
		/* Got an exception. can check */
		/* bits for more detail */
		if (Status.esm_exception & 0x80000000)
			hdmi_inf("hardware exception\n");
		else
			hdmi_inf("solfware exception\n");

		hdmi_inf(" - line number:%d\n",
					(Status.esm_exception >> 10) & 0xfffff);
		hdmi_inf(" - flag:%d\n",
				(Status.esm_exception >> 1) & 0x1ff);
		hdmi_inf(" - Type:%s\n",
				(Status.esm_exception & 0x1) ? "notify" : "abort");
		if (((Status.esm_exception >> 1) & 0x1ff) != 109)
			return -1;

		if (hdcp->esm_pairdata) {
			memset(hdcp->esm_pairdata, 0, DW_ESM_PAIRDATA_SIZE);
			ret = ESM_SavePairing(esm, hdcp->esm_pairdata,
					&esm->esm_pair_size);
			if (ret != ESM_HL_SUCCESS)
				hdmi_err("ESM_SavePairing failed\n");
		}

		return 1;
	}

	if (Status.esm_auth_fail) {
		hdmi_err("esm status check result,failed:%d\n", Status.esm_auth_fail);
		return -1;
	}

	if (Status.esm_auth_pass) {
		memset(hdcp->esm_pairdata, 0, DW_ESM_PAIRDATA_SIZE);
		ret = ESM_SavePairing(esm, hdcp->esm_pairdata, &esm->esm_pair_size);
		if (ret != ESM_HL_SUCCESS)
			hdmi_err("ESM_SavePairing failed\n");
		return 0;
	}

	return 2;
}

static int _dw_hdcp2x_open(void)
{
	ESM_STATUS err;
	esm_config_t esm_config;

	if (hdcp->esm_open) {
		hdmi_inf("esm has been booted\n");
		goto set_capability;
	}

	if (IS_ERR_OR_NULL(esm->driver)) {
		shdmi_err(esm->driver);
		return -1;
	}

	if (esm->driver->vir_data_base == 0) {
		hdmi_err("esm driver virtual address not set!\n");
		return -1;
	}

	memset((void *)esm->driver->vir_data_base, 0, esm->driver->data_size);
	memset(&esm_config, 0, sizeof(esm_config_t));

	_esm_address_dump();

	err = ESM_Initialize(esm,
			esm->driver->code_base,
			esm->driver->code_size,
			0, esm->driver, &esm_config);
	if (err != ESM_SUCCESS) {
		hdmi_err("esm boots fail!\n");
		return -1;
	}
	hdcp_log("esm boots successfully\n");

	if (ESM_LoadPairing(esm, hdcp->esm_pairdata, esm->esm_pair_size) < 0)
		hdmi_inf("ESM Load Pairing failed\n");
	hdcp->esm_open = DW_HDMI_ENABLE;

set_capability:
	if (hdcp->esm_cap_done)
		return 0;

	ESM_Reset(esm);
	/* Enable logging */
	ESM_LogControl(esm, 1, 0);
	/* ESM_EnableLowValueContent(esm); */
	err = ESM_SetCapability(esm);
	if (err != ESM_HL_SUCCESS) {
		hdmi_err("dw hdcp2x esm set capability failed!\n");
		return -1;
	}
	hdcp->esm_cap_done = DW_HDMI_ENABLE;

	msleep(50);
	queue_work(hdcp->hdcp2x_workqueue, &hdcp->hdcp2x_work);
	return 0;
}

int dw_hdcp2x_get_encrypt_state(void)
{
	int ret = 0;

	if (!hdcp->esm_auth_done)
		return DW_HDCP_DISABLE;

	mutex_lock(&hdcp->lock_esm_handler);

	ret = _dw_hdcp2x_get_encry_state();
	if (ret == 0) {
		_dw_hdcp2x_data_enable(DW_HDMI_ENABLE);
		hdcp->esm_auth_state = DW_HDCP_SUCCESS;
		hdmi_inf("dw hdcp2x enable data encry\n");
		goto esm_exit;
	} else if (ret == -1) {
		_dw_hdcp2x_data_enable(DW_HDMI_DISABLE);
		msleep(25);
		if (!_dw_hdcp2x_start_auth()) {
			hdcp->esm_auth_state = DW_HDCP_ING;
			hdmi_inf("dw hdcp2x state failed start re-atuh\n");
			goto esm_exit;
		}
		/* re-auth failed and set failed */
		hdcp->esm_auth_state = DW_HDCP_FAILED;
		hdmi_inf("dw hdcp2x start re-atuh failed\n");
		goto esm_exit;
	} else if (ret == 1) {
		hdcp->esm_auth_state = DW_HDCP_ING;
		hdmi_inf("dw hdcp2x authing\n");
		goto esm_exit;
	}

esm_exit:
	mutex_unlock(&hdcp->lock_esm_handler);
	return hdcp->esm_auth_state;
}

/* configure hdcp2.2 and enable hdcp2.2 encrypt */
int dw_hdcp2x_enable(void)
{
	u8 hdcp_mask = 0;

	/* 1 - set main controller hdcp clock disable */
	_dw_hdcp2x_data_enable(DW_HDMI_DISABLE);

	/* 2 - set hdcp keepout */
	dw_fc_video_set_hdcp_keepout(DW_HDMI_ENABLE);

	/* 3 - Select DVI or HDMI mode */
	dw_hdcp_sync_tmds_mode();

	/* 4 - Set the Data enable, Hsync, and VSync polarity */
	dw_hdcp_sync_data_polarity();

	dw_hdcp2x_ovr_set_path(DW_HDMI_ENABLE, DW_HDMI_ENABLE);

	_dw_hdcp2x_ovr_set_hpd(DW_HDMI_ENABLE, DW_HDMI_ENABLE);

	/* disable avmute overwrite */
	_dw_hdcp2x_ovr_set_avmute(DW_HDMI_DISABLE, DW_HDMI_DISABLE);

	/* mask the interrupt of hdcp22 event */
	hdcp_mask  = HDCP22REG_MUTE_CAPABLE_MASK;
	hdcp_mask |= HDCP22REG_MUTE_NOT_CAPABLE_MASK;
	hdcp_mask |= HDCP22REG_MUTE_AUTHEN_LOST_MASK;
	hdcp_mask |= HDCP22REG_MUTE_AUTHEN_MASK;
	hdcp_mask |= HDCP22REG_MUTE_AUTHEN_FAIL_MASK;
	hdcp_mask |= HDCP22REG_MUTE_DECRYP_CHG_MASK;
	_dw_hdcp2x_int_mute(hdcp_mask);

	if (_dw_hdcp2x_open() < 0)
		return -1;

	dw_hdcp_set_enable_type(DW_HDCP_TYPE_HDCP22);

	return 0;
}

int dw_hdcp2x_disable(void)
{
	dw_mc_set_clk(DW_MC_CLK_HDCP, DW_HDMI_DISABLE);

	dw_hdcp2x_ovr_set_path(DW_HDMI_DISABLE, DW_HDMI_ENABLE);

	_dw_hdcp2x_ovr_set_hpd(DW_HDMI_DISABLE, DW_HDMI_ENABLE);

	if (hdcp->esm_auth_done) {
		mutex_lock(&hdcp->lock_esm_auth);
		ESM_Authenticate(esm, 0, 0, 0);
		mutex_unlock(&hdcp->lock_esm_auth);
		msleep(20);
	}

	hdcp->esm_auth_done  = DW_HDMI_DISABLE;
	hdcp->esm_cap_done   = DW_HDMI_DISABLE;
	hdcp->esm_auth_state = DW_HDCP_DISABLE;

	dw_hdcp_set_enable_type(DW_HDCP_TYPE_NULL);
	hdcp_log("hdcp22 disconfig done!\n");
	return 0;
}

int dw_hdcp2x_firmware_state(void)
{
	return hdcp->esm_loading;
}

int dw_hdcp2x_firmware_update(const u8 *data, size_t size)
{
	char *esm_addr = (char *)esm->driver->vir_code_base;

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return -1;
	}

	if (IS_ERR_OR_NULL(esm_addr)) {
		shdmi_err(esm_addr);
		return -1;
	}

	memcpy(esm_addr, data, size);
	hdcp->esm_size    = (u32)size;
	hdcp->esm_loading = DW_HDMI_ENABLE;

	hdmi_inf("dw hdcp2x loading firmware size %d finish.\n", (u32)size);
	return 0;
}

int dw_hdcp2x_init(void)
{
	struct dw_hdmi_dev_s  *hdmi = dw_get_hdmi();

	hdcp = &hdmi->hdcp_dev;

	hdcp->esm_open      = DW_HDMI_DISABLE;
	hdcp->esm_loading   = DW_HDMI_DISABLE;
	hdcp->esm_auth_done = DW_HDMI_DISABLE;
	hdcp->esm_cap_done  = DW_HDMI_DISABLE;

	shdmi_free_point(hdcp->esm_pairdata);
	hdcp->esm_pairdata = kzalloc(DW_ESM_PAIRDATA_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(hdcp->esm_pairdata)) {
		shdmi_err(hdcp->esm_pairdata);
		return -1;
	}

	shdmi_free_point(esm);
	esm = kzalloc(sizeof(esm_instance_t), GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(esm)) {
		shdmi_err(esm);
		return -1;
	}

	shdmi_free_point(esm->driver);
	esm->driver = kzalloc(sizeof(esm_host_driver_t), GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(esm->driver)) {
		shdmi_err(esm->driver);
		return -1;
	}

	mutex_init(&hdcp->lock_esm_auth);
	mutex_init(&hdcp->lock_esm_handler);

	hdcp->hdcp2x_workqueue = create_workqueue("hdcp2x-workqueue");
	INIT_WORK(&hdcp->hdcp2x_work, _dw_hdcp2x_auth_work);

	/* esm firmware dma address alloc */
	hdcp->esm_fw_size = DW_ESM_FW_SIZE;
	hdcp->esm_fw_addr_vir = (unsigned long)dma_alloc_coherent(hdmi->dev,
		hdcp->esm_fw_size, &hdcp->esm_fw_addr, GFP_KERNEL | __GFP_ZERO);
	/* hdcp->esm_fw_addr -= 0x40000000; */

	/* esm data dma address alloc */
	hdcp->esm_data_size = DW_ESM_DATA_SIZE;
	hdcp->esm_data_addr_vir = (unsigned long)dma_alloc_coherent(hdmi->dev,
		hdcp->esm_data_size, &hdcp->esm_data_addr, GFP_KERNEL | __GFP_ZERO);
	/* hdcp->esm_data_addr -= 0x40000000; */

	hdcp->esm_hpi_addr = (unsigned long)(hdmi->addr + ESM_REG_BASE_OFFSET);

	esm->driver->code_base     = (u32)hdcp->esm_fw_addr;
	esm->driver->code_size     = hdcp->esm_fw_size;
	esm->driver->vir_code_base = hdcp->esm_fw_addr_vir;

	esm->driver->data_base     = (u32)hdcp->esm_data_addr;
	esm->driver->data_size     = hdcp->esm_data_size;
	esm->driver->vir_data_base = hdcp->esm_data_addr_vir;

	esm->driver->hpi_base   = hdcp->esm_hpi_addr;
	esm->driver->hpi_read   = _esm_hpi_read;
	esm->driver->hpi_write  = _esm_hpi_write;
	esm->driver->data_read  = _esm_data_read;
	esm->driver->data_write = _esm_data_write;
	esm->driver->data_set   = _esm_data_set;

	esm->driver->instance = NULL;
	esm->driver->idle     = NULL;

	_esm_address_dump();

	return 0;
}

void dw_hdcp2x_exit(void)
{
	shdmi_free_point(esm->driver);

	shdmi_free_point(esm);

	shdmi_free_point(hdcp->hdcp2x_workqueue);

	shdmi_free_point(hdcp);
}

ssize_t dw_hdcp2x_dump(char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n, "\n[dw hdcp2x]\n");
	n += sprintf(buf + n, "|       |                  state               |                  esm firmware\n");
	n += sprintf(buf + n, "| name  |--------------------------------------+-----------------------------------------------------|\n");
	n += sprintf(buf + n, "|       | mode  |  path  | config | capability | booting | loader |  size  | code addr  | data addr  |\n");
	n += sprintf(buf + n, "|-------+-------+--------+--------+------------+---------+--------+--------+------------+------------|\n");
	n += sprintf(buf + n, "| state | %-5s | %-6s |  %-3s   |    %-4s    |   %-3s   |   %-3s  | %-6d | 0x%-8x | 0x%-8x |\n",
		dw_read_mask(A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK) ? "hdmi" : "dvi",
		dw_hdcp2x_get_path() ? "hdcp2x" : "hdcp1x",
		hdcp->esm_auth_done ? "yes" : "no",
		hdcp->esm_cap_done ? "pass" : "fail",
		hdcp->esm_open ? "on" : "off",
		hdcp->esm_loading ? "yes" : "no",
		hdcp->esm_size,
		esm->driver->code_base,
		esm->driver->data_base);
	return n;
}

