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
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>

#include "esm_lib/include/ESMHost.h"
#include "dw_hdcp.h"
#include "dw_hdcp22_tx.h"

#define PAIRDATA_SIZE    300

/* static u8 set_cap; */
static struct mutex authen_mutex;
static struct mutex esm_ctrl;
static u8 esm_on;
static u8 esm_enable;
static u8 esm_set_cap;
static u8 authenticate_state;
static u8 pairdata[PAIRDATA_SIZE];
static wait_queue_head_t               esm_wait;
static struct workqueue_struct 		   *hdcp22_workqueue;
static struct work_struct			   hdcp22_work;
static esm_instance_t *esm;

static int _esm_hpi_read(void *instance, uint32_t offset, uint32_t *data)
{
	unsigned long addr =  esm->driver->hpi_base + offset;

	*data = *((volatile u32 *)addr);
	/* hdmi_inf("hpi_read: offset:%x  data:%x\n", offset, *data); */
	udelay(1000);
	return 0;
}

static int _esm_hpi_write(void *instance, uint32_t offset, uint32_t data)
{
	unsigned long addr =  esm->driver->hpi_base + offset;

	*((volatile u32 *)addr) = data;
	/* hdmi_inf("hpi_write: offset:%x  data:%x\n", offset, data); */
	udelay(1000);
	return 0;
}

static int _esm_data_read(void *instance, uint32_t offset, uint8_t *dest_buf, uint32_t nbytes)
{
	memcpy(dest_buf, (u8 *)(esm->driver->vir_data_base + offset), nbytes);
	return 0;
}

static int _esm_data_write(void *instance, uint32_t offset, uint8_t *src_buf, uint32_t nbytes)
{
	memcpy((u8 *)(esm->driver->vir_data_base + offset), src_buf, nbytes);
	return 0;
}

/* make a length of memory set a data */
static int _esm_data_set(void *instance, uint32_t offset, uint8_t data, uint32_t nbytes)
{
	memset((u8 *)(esm->driver->vir_data_base + offset), data, nbytes);
	return 0;
}

/* static int esm_tx_kill(void)
{
	return ESM_Kill(esm);
} */

static int _dw_esm_driver_init(unsigned long esm_hpi_base,
		u32 esm_code_base,
		unsigned long esm_vir_code_base,
		u32 esm_code_size,
		u32 esm_data_base,
		unsigned long esm_vir_data_base,
		u32 esm_data_size)
{
	if (esm == NULL) {
		hdmi_err("%s: pointer esm is null!!!\n", __func__);
		return -1;
	}

	esm->driver = kmalloc(sizeof(esm_host_driver_t), GFP_KERNEL | __GFP_ZERO);
	if (esm->driver == NULL) {
		hdmi_err("%s: esm host driver alloc failed!\n", __func__);
		return -1;
	}

	esm->driver->hpi_base      = esm_hpi_base;
	esm->driver->code_base     = esm_code_base;
	esm->driver->code_size     = esm_code_size;
	esm->driver->data_base     = esm_data_base;
	esm->driver->data_size     = esm_data_size;
	esm->driver->vir_code_base = esm_vir_code_base;
	esm->driver->vir_data_base = esm_vir_data_base;

	esm->driver->hpi_read   = _esm_hpi_read;
	esm->driver->hpi_write  = _esm_hpi_write;
	esm->driver->data_read  = _esm_data_read;
	esm->driver->data_write = _esm_data_write;
	esm->driver->data_set   = _esm_data_set;

	esm->driver->instance = NULL;
	esm->driver->idle     = NULL;

	return 0;
}

static void _dw_esm_driver_exit(void)
{
	if (esm->driver != NULL)
		kfree(esm->driver);
}

void _dw_esm_check_print(u32 code_base,
		unsigned long vir_code_base,
		u32 code_size,
		u32 data_base,
		unsigned long vir_data_base,
		u32 data_size)
{
	u32 i;

	HDCP_INF("physical code base: 0x%x, code size: %d\n",
			code_base, code_size);
	HDCP_INF("physical data base: 0x%x, data size: %d\n",
			data_base, data_size);
	HDCP_INF("virtual  code base: 0x%lx, data base: 0x%lx\n",
			vir_code_base, vir_data_base);

	for (i = 0; i < 10; i++)
		HDCP_INF("esm firmware %d: 0x%x\n", i,
				*((u8 *)(vir_code_base + i)));
}

int dw_esm_open(void)
{
	ESM_STATUS err;
	esm_config_t esm_config;

	if (esm_on != 0) {
		hdmi_wrn("esm has been booted\n");
		goto set_capability;
	}
	esm_on = 1;

	if (esm->driver == NULL) {
		hdmi_err("%s: esm driver is null!!!\n", __func__);
		return -1;
	}

	if (esm->driver->vir_data_base == 0) {
		hdmi_err("%s: esm driver virtual address not set!\n", __func__);
		return -1;
	}

	memset((void *)esm->driver->vir_data_base, 0, esm->driver->data_size);
	memset(&esm_config, 0, sizeof(esm_config_t));

	_dw_esm_check_print(esm->driver->code_base, esm->driver->vir_code_base,
					esm->driver->code_size, esm->driver->data_base,
					esm->driver->vir_data_base, esm->driver->data_size);

	err = ESM_Initialize(esm,
			esm->driver->code_base,
			esm->driver->code_size,
			0, esm->driver, &esm_config);
	if (err != ESM_SUCCESS) {
		hdmi_err("%s: esm boots failed!\n", __func__);
		return -1;
	} else {
		HDCP_INF("esm boots successfully\n");
	}

	if (ESM_LoadPairing(esm, pairdata, esm->esm_pair_size) < 0)
		hdmi_wrn("ESM Load Pairing failed!\n");

set_capability:
	if (esm_set_cap)
		return 0;
	esm_set_cap = 1;
	ESM_Reset(esm);
	/* Enable logging */
	ESM_LogControl(esm, 1, 0);
	/* ESM_EnableLowValueContent(esm); */
	if (ESM_SetCapability(esm) != ESM_HL_SUCCESS) {
		hdmi_err("%s: esm set capability fail, maybe remote Rx is not 2.2 capable!\n", __func__);
		return -1;
	}
	msleep(50);
	return 0;
}

/* for:
	hdmi_plugin<--->hdmi_plugout
	hdmi_suspend<--->hdmi_resume
*/
void dw_esm_close(void)
{
	/* esm_tx_kill();
	dw_esm_disable();
	esm_enable = 0; */
	esm_enable = 0;
	esm_on = 0;
}

static int _dw_hdcp22_set_authenticate(void)
{
	ESM_STATUS err;

	mutex_lock(&authen_mutex);
	err = ESM_Authenticate(esm, 1, 1, 0);
	if (err != 0) {
		authenticate_state = 0;
		hdmi_err("%s: esm authenticate failed\n", __func__);
		mutex_unlock(&authen_mutex);
		return -1;
	} else {
		authenticate_state = 1;
		mutex_unlock(&authen_mutex);
		return 0;
	}
}

void dw_esm_disable(void)
{
	/* if (!esm_enable)
		return; */
	if (esm_enable) {
		ESM_Authenticate(esm, 0, 0, 0);
		msleep(20);
	}
	esm_enable = 0;
	esm_set_cap = 0;
	wake_up(&esm_wait);
}

static void dw_hdcp22_auth_work(struct work_struct *work)
{
	int wait_time = 0;

	LOG_TRACE();

	if (esm_enable)
		return;
	esm_enable = 1;

	HDCP_INF("Sleep to wait for hdcp2.2 authentication\n");
	wait_time = wait_event_interruptible_timeout(esm_wait, !esm_enable,
						       msecs_to_jiffies(3000));
	if (wait_time > 0) {
		hdmi_inf("HDCP2.2 Force to wake up, waiting time is less than 3s\n");
		return;
	}

	if (_dw_hdcp22_set_authenticate()) {
		hdmi_err("%s: hdcp2.2 set authenticate failed\n", __func__);
		return;
	} else {
		/* esm_status = dw_hdcp22_status_check_and_handle();
		if (esm_status != -1) {
			esm_enable = 1;
			return 0;
		} else {
			esm_enable = 1;
			return -1;
		} */
	}
	esm_enable = 1;
}

void dw_hdcp22_esm_start_auth(void)
{
	queue_work(hdcp22_workqueue, &hdcp22_work);
}

static int _dw_esm_encrypt_status_check(void)
{
	esm_status_t Status = {0, 0, 0, 0};
	uint32_t state = 0;

	/* Check to see if sync gets lost
		when running (i.e. lose authentication) */
	/* if (ESM_GetStatusRegister(esm, &Status, 1) == ESM_HL_SUCCESS) { */
	if (ESM_GetState(esm, &state, &Status) == ESM_HL_SUCCESS) {
		if (Status.esm_sync_lost) {
			hdmi_err("%s: esm sync lost!\n", __func__);
			return -1;
		}

		if (Status.esm_exception) {
			/* Got an exception. can check */
			/* bits for more detail */
			if (Status.esm_exception & 0x80000000)
				hdmi_inf("hardware exception:\n");
			else
				hdmi_inf("solfware exception:\n");

			hdmi_inf("exception line number:%d\n",
				       (Status.esm_exception >> 10) & 0xfffff);
			hdmi_inf("exception flag:%d\n",
					(Status.esm_exception >> 1) & 0x1ff);
			hdmi_inf("exception Type:%s\n",
					(Status.esm_exception & 0x1) ? "notify" : "abort");
			if (((Status.esm_exception >> 1) & 0x1ff) != 109) {
				hdmi_err("%s: esm_exception error! %d\n",
						__func__, ((Status.esm_exception >> 1) & 0x1ff));
				return -1;
			}

			memset(pairdata, 0, PAIRDATA_SIZE);
			if (ESM_SavePairing(esm, pairdata, &esm->esm_pair_size) != 0)
				hdmi_err("%s: ESM_SavePairing failed\n", __func__);

			return 0;
		}

		if (Status.esm_auth_fail) {
			hdmi_err("%s: esm status check result,failed:%d\n", __func__, Status.esm_auth_fail);
			return -1;
		}

		if (Status.esm_auth_pass) {
			memset(pairdata, 0, PAIRDATA_SIZE);
			if (ESM_SavePairing(esm, pairdata, &esm->esm_pair_size) != 0)
				hdmi_err("%s: ESM_SavePairing failed\n", __func__);
			return 0;
		}

		return -2;
	}
	return -1;
}

/* Check esm encrypt status and handle the status
 * return value: 1-indicate that esm is being in authenticate;
 *               0-indicate that esm authenticate is sucessful
 *              -1-indicate that esm authenticate is failed
 *              -2-indicate that esm authenticate is in idle state
 * */
int dw_hdcp22_status_check_and_handle(void)
{
	int encrypt_status = -1;

	if (!esm_enable)
		return -2;

	/* if (!authenticate_state)
		_dw_hdcp22_set_authenticate(); */
	mutex_lock(&esm_ctrl);
	encrypt_status = _dw_esm_encrypt_status_check();
	if (encrypt_status == -1) {
		dw_hdcp22_data_enable(0);
		msleep(25);
		if (!_dw_hdcp22_set_authenticate()) {
			mutex_unlock(&esm_ctrl);
			return 1;
		} else {
			mutex_unlock(&esm_ctrl);
			return -1;
		}
	} else if (encrypt_status == -2) {
		mutex_unlock(&esm_ctrl);
		return -2;
	} else if (encrypt_status == 0) {
		dw_hdcp22_data_enable(1);
		mutex_unlock(&esm_ctrl);
		return 0;
	}
	mutex_unlock(&esm_ctrl);
	return -2;
}

int dw_esm_tx_initial(unsigned long esm_hpi_base,
		u32 code_base,
		unsigned long vir_code_base,
		u32 code_size,
		u32 data_base,
		unsigned long vir_data_base,
		u32 data_size)
{
	int ret = -1;
	esm_on = 0;
	esm_enable = 0;
	esm_set_cap = 0;
	authenticate_state = 0;
	memset(pairdata, 0, PAIRDATA_SIZE);

	if (esm) {
		kfree(esm);
		esm = NULL;
	}

	esm = kmalloc(sizeof(esm_instance_t), GFP_KERNEL | __GFP_ZERO);
	if (esm == NULL) {
		hdmi_err("%s: esm instance alloc failed!!!\n", __func__);
		return -1;
	}

	_dw_esm_check_print(code_base, vir_code_base, code_size,
			data_base, vir_data_base, data_size);

	ret = _dw_esm_driver_init(esm_hpi_base, code_base, vir_code_base, code_size,
			data_base, vir_data_base, data_size);
	if (ret != 0) {
		hdmi_err("%s: esm driver init failed!\n", __func__);
		return -1;
	}

	mutex_init(&authen_mutex);
	mutex_init(&esm_ctrl);
	init_waitqueue_head(&esm_wait);
	hdcp22_workqueue = create_workqueue("hdcp22_workqueue");
	INIT_WORK(&hdcp22_work, dw_hdcp22_auth_work);
	return 0;
}

void dw_esm_tx_exit(void)
{
	if (hdcp22_workqueue != NULL)
		destroy_workqueue(hdcp22_workqueue);

	_dw_esm_driver_exit();

	if (esm != NULL)
		kfree(esm);
}

ssize_t dw_hdcp22_esm_dump(char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "     - esm %s and %s, capability: %s\n",
		esm_on ? "on" : "off",
		esm_enable ? "enable" : "disable",
		esm_set_cap ? "set" : "unset");
	n += sprintf(buf + n, "     - code addr: 0x%x, size: 0x%x\n",
		esm->driver->code_base, esm->driver->code_size);
	n += sprintf(buf + n, "     - data addr: 0x%x, size: 0x%x\n",
		esm->driver->data_base, esm->driver->data_size);

	return n;
}
