/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "../utility/vin_log.h"
#include <linux/kernel.h>
#include <linux/module.h>

#include "vin_rp.h"
#include "../vin-isp/sunxi_isp.h"
#include "../platform/platform_cfg.h"

extern struct isp_dev *glb_isp[VIN_MAX_ISP];

/*isp rpbuf*/
static void isp_rpbuf_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
}

static int isp_rpbuf_rx_cb(struct rpbuf_buffer *buffer,
				    void *data, int data_len, void *priv)
{
	struct isp_dev *isp = priv;
	void *buf_va = rpbuf_buffer_va(buffer);

	if (data_len > ISP_LOAD_DRAM_SIZE) {
		vin_err("melis ask for 0x%x data, it more than isp load_data 0x%x\n", data_len, ISP_LOAD_DRAM_SIZE);
		return -EINVAL;
	}

	if (data_len == ISP_SAVE_LOAD_STATISTIC_SIZE) {
		memcpy(isp->isp_save_load.vir_addr + ISP_SAVE_LOAD_REG_SIZE, buf_va, data_len);
		vin_log(VIN_LOG_RP, "isp%d receive save_load buffer from melis\n", isp->id);
		return 0;
	}

	isp->load_flag = 1;
	memcpy(&isp->load_shadow[0], buf_va, data_len);

	vin_log(VIN_LOG_RP, "isp%d frame number %d: receive load buffer from melis\n", isp->id, isp->isp_frame_number);

	return 0;
}

static int isp_rpbuf_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	return 0;
}

static const struct rpbuf_buffer_cbs isp_rpbuf_cbs = {
	.available_cb = isp_rpbuf_available_cb,
	.rx_cb = isp_rpbuf_rx_cb,
	.destroyed_cb = isp_rpbuf_destroyed_cb,
};

int isp_save_rpbuf_send(struct isp_dev *isp, void *vir_buf)
{
	int ret;
	const char *buf_name;
	void *buf_va;
	int buf_len, offset;
	int data_len = ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_STATISTIC_SIZE;

	if (!isp->save_buffer) {
		vin_err("save rpbuf is null\n");
		return -ENOENT;
	}

	buf_name = rpbuf_buffer_name(isp->save_buffer);
	buf_va = rpbuf_buffer_va(isp->save_buffer);
	buf_len = rpbuf_buffer_len(isp->save_buffer);

	if (buf_len > roundup(data_len, 0x1000)) {
		vin_err("%s: data size(0x%x) out of buffer length(0x%x)\n",
			buf_name, roundup(data_len, 0x1000), buf_len);
		return -EINVAL;
	}

	if (!rpbuf_buffer_is_available(isp->save_buffer)) {
		vin_err("%s not available\n", buf_name);
		return -EACCES;
	}

	memcpy(buf_va, vir_buf, ISP_STAT_TOTAL_SIZE);
	memcpy(buf_va + ISP_STAT_TOTAL_SIZE, isp->isp_save_load.vir_addr + ISP_SAVE_LOAD_REG_SIZE, ISP_SAVE_LOAD_STATISTIC_SIZE);

	offset = 0;
	ret = rpbuf_transmit_buffer(isp->save_buffer, offset, data_len);
	if (ret < 0) {
		vin_err("%s: rpbuf_transmit_buffer failed\n", buf_name);
		return ret;
	}

	vin_log(VIN_LOG_RP, "isp%d send save buffer to melis\n", isp->id);

	return data_len;
}

int vin_isp_get_hist(struct isp_dev *isp, unsigned int *hist_buf)
{
	void *buf_va;
	const char *buf_name;
	int hist_len = ISP_STAT_HIST0_MEM_SIZE;
	int hist_offset = ISP_STAT_HIST0_MEM_OFS;
	struct isp_stat *stat = &isp->h3a_stat;

	if (!isp->save_buffer) {
		vin_err("save rpbuf is null\n");
		return -ENOENT;
	}

	buf_name = rpbuf_buffer_name(isp->save_buffer);
	buf_va = rpbuf_buffer_va(isp->save_buffer);

	if (!rpbuf_buffer_is_available(isp->save_buffer)) {
		vin_err("%s not available\n", buf_name);
		return -EACCES;
	}

	mutex_lock(&stat->ioctl_lock);
	memcpy(hist_buf, buf_va + hist_offset, hist_len);
	mutex_unlock(&stat->ioctl_lock);

	vin_log(VIN_LOG_RP, "isp%d save hist buffer to user\n", isp->id);

	return hist_len;
}

int isp_ldci_rpbuf_send(struct isp_dev *isp, void *vir_buf)
{
	int ret;
	const char *buf_name;
	void *buf_va;
	int buf_len, offset;
	int data_len = ISP_RPBUF_LDCI_LEN;

	if (!isp->ldci_buffer) {
		vin_err("ldci rpbuf is null\n");
		return -ENOENT;
	}

	buf_name = rpbuf_buffer_name(isp->ldci_buffer);
	buf_va = rpbuf_buffer_va(isp->ldci_buffer);
	buf_len = rpbuf_buffer_len(isp->ldci_buffer);

	if (buf_len > roundup(data_len, 0x1000)) {
		vin_err("%s: data size(0x%x) out of buffer length(0x%x)\n",
			buf_name, roundup(data_len, 0x1000), buf_len);
		return -EINVAL;
	}

	if (!rpbuf_buffer_is_available(isp->ldci_buffer)) {
		vin_err("%s not available\n", buf_name);
		return -EACCES;
	}

	memcpy(buf_va, vir_buf, ISP_RPBUF_LDCI_LEN);

	offset = 0;
	ret = rpbuf_transmit_buffer(isp->ldci_buffer, offset, data_len);
	if (ret < 0) {
		vin_err("%s: rpbuf_transmit_buffer failed\n", buf_name);
		return ret;
	}

	vin_log(VIN_LOG_RP, "isp%d send ldci buffer to melis\n", isp->id);

	return data_len;
}

int isp_rpbuf_create(struct isp_dev *isp)
{
	const char *load_name, *save_name, *ldci_name;
	int load_len, save_len, ldci_len;

	if (isp->controller == NULL) {
		vin_err("isp%d:rpbuf controller is NULL, can not to create rpbuf\n", isp->id);
		return -ENOENT;
	}

	if (isp->id == 0) {
		load_name = ISP0_RPBUF_LOAD_NAME;
		save_name = ISP0_RPBUF_SAVE_NAME;
		ldci_name = ISP0_RPBUF_LDCI_NAME;
	} else if (isp->id == 1) {
		load_name = ISP1_RPBUF_LOAD_NAME;
		save_name = ISP1_RPBUF_SAVE_NAME;
		ldci_name = ISP1_RPBUF_LDCI_NAME;
	} else if (isp->id == 2) {
		load_name = ISP2_RPBUF_LOAD_NAME;
		save_name = ISP2_RPBUF_SAVE_NAME;
		ldci_name = ISP2_RPBUF_LDCI_NAME;
	} else {
		vin_err("isp%d not support rpbuf\n", isp->id);
		return -ENOENT;
	}

	if (rpbuf_wait_controller_ready(isp->controller, 1000)) {
		vin_err("rpbuf controller wait ready timeout.\n");
		return -ENOENT;
	}

	load_len = roundup(ISP_RPBUF_LOAD_LEN, 0x1000);
	isp->load_buffer = rpbuf_alloc_buffer(isp->controller, load_name, load_len,
						NULL, &isp_rpbuf_cbs, (void *)isp);
	if (!isp->load_buffer) {
		vin_err("rpbuf_alloc_buffer load failed\n");
		return -ENOENT;
	}
	rpbuf_buffer_set_sync(isp->load_buffer, true);

	save_len = roundup(ISP_RPBUF_SAVE_LEN, 0x1000);
	isp->save_buffer = rpbuf_alloc_buffer(isp->controller, save_name, save_len,
						NULL, NULL, (void *)isp);
	if (!isp->save_buffer) {
		vin_err("rpbuf_alloc_buffer save failed\n");
		return -ENOENT;
	}
	rpbuf_buffer_set_sync(isp->save_buffer, true);

	ldci_len = roundup(ISP_RPBUF_LDCI_LEN, 0x1000);
	isp->ldci_buffer = rpbuf_alloc_buffer(isp->controller, ldci_name, ldci_len,
						NULL, NULL, (void *)isp);
	if (!isp->ldci_buffer) {
		vin_err("rpbuf_alloc_buffer ldci failed\n");
		return -ENOENT;
	}
	rpbuf_buffer_set_sync(isp->ldci_buffer, true);

	return 0;
}

int isp_rpbuf_destroy(struct isp_dev *isp)
{
	int ret;

	ret = rpbuf_free_buffer(isp->load_buffer);
	if (ret < 0) {
		vin_err("rpbuf_free_buffer load failed\n");
		return ret;
	}

	ret = rpbuf_free_buffer(isp->save_buffer);
	if (ret < 0) {
		vin_err("rpbuf_free_buffer save failed\n");
		return ret;
	}

	ret = rpbuf_free_buffer(isp->ldci_buffer);
	if (ret < 0) {
		vin_err("rpbuf_free_buffer ldci failed\n");
		return ret;
	}

	isp->load_buffer = NULL;
	isp->save_buffer = NULL;
	isp->ldci_buffer = NULL;

	return 0;
}

/*isp rpmsg*/
int isp_rpmsg_send(struct isp_dev *isp, void *data, int len)
{
	int ret = 0;

	if (!isp->rpmsg)
		return -1;

	vin_log(VIN_LOG_RP, "isp%d rpmsg_send, cmd is %d\n", isp->id, *((unsigned int *)data + 0));

	ret = rpmsg_send(isp->rpmsg->ept, data, len);
	if (ret) {
		vin_err("rpmsg_send failed, cmd is %d\n", *((unsigned int *)data + 0));
	}

	return ret;
}

int isp_rpmsg_trysend(struct isp_dev *isp, void *data, int len)
{
	int ret = 0;

	if (!isp->rpmsg)
		return -1;

	vin_log(VIN_LOG_RP, "isp%d rpmsg_send, cmd is %d\n", isp->id, *((unsigned int *)data + 0));

	ret = rpmsg_trysend(isp->rpmsg->ept, data, len);
	if (ret) {
		vin_err("rpmsg_send failed, cmd is %d\n", *((unsigned int *)data + 0));
	}

	return ret;
}

static int isp_rpmsg_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct isp_dev *isp = priv;
	unsigned int *p = data;
	vin_log(VIN_LOG_RP, "isp%d_rpmsg_cb, cmd is %d\n", isp->id, p[0]);

	switch (p[0]) {
	case ISP_REQUEST_SENSOR_INFO:
		isp_config_sensor_info(isp);
		break;
	case ISP_SET_SENSOR_EXP_GAIN:
		isp_sensor_set_exp_gain(isp, data);
		break;
	case ISP_SET_STAT_EN:
		isp_stat_enable(&isp->h3a_stat, p[1]);
		break;
	case ISP_SET_SAVE_AE:
		isp_save_ae(isp, p + 1);
		break;
	case ISP_SET_ENCPP_DATA:
		isp_set_encpp_cfg(isp, data);
		break;
	case ISP_SET_IR_STATUS:
		isp_set_ir_cfg(isp, data);
		break;
	case ISP_SET_ATTR_IOCTL:
		isp_update_isp_attr_cfg(isp, data);
		break;
	case ISP_REQUEST_SENSOR_STATE:
		isp_get_sensor_state(isp);
		break;
	case ISP_SET_GTM_TPYE:
		isp->gtm_type = p[1];
		vin_print("gtm type is %d\n", p[1]);
		break;
	case ISP_SET_SYS_RESET:
		isp->isp_server_reset = 1;
		vin_print("melis system reset, isp_server set reset\n");
		break;
	}

	return 0;
}

static int isp_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int isp_id;

	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;
	if (!strcmp(rpdev->id.name, ISP0_RPMSG_NAME)) {
		isp_id = 0;
		if (VIN_MAX_ISP <= isp_id) {
			vin_err("these is no isp%d, cannot to probe rpmsg\n", isp_id);
			return -1;
		}
		glb_isp[isp_id]->rpmsg = rpdev;
		rpdev->ept->priv = glb_isp[isp_id];
		vin_log(VIN_LOG_RP, "isp%d rpmsg probe!\n", isp_id);
	} else if (!strcmp(rpdev->id.name, ISP1_RPMSG_NAME)) {
		isp_id = 1;
		if (VIN_MAX_ISP <= isp_id) {
			vin_err("these is no isp%d, cannot to probe rpmsg\n", isp_id);
			return -1;
		}
		glb_isp[isp_id]->rpmsg = rpdev;
		rpdev->ept->priv = glb_isp[isp_id];
		vin_log(VIN_LOG_RP, "isp%d rpmsg probe!\n", isp_id);
	} else if (!strcmp(rpdev->id.name, ISP2_RPMSG_NAME)) {
		isp_id = 2;
		if (VIN_MAX_ISP <= isp_id) {
			vin_err("these is no isp%d, cannot to probe rpmsg\n", isp_id);
			return -1;
		}
		glb_isp[isp_id]->rpmsg = rpdev;
		rpdev->ept->priv = glb_isp[isp_id];
		vin_log(VIN_LOG_RP, "isp%d rpmsg probe!\n", isp_id);
	} else
		vin_err("isp rpmsg probe err!");

	return 0;
}

static void isp_rpmsg_remove(struct rpmsg_device *rpdev)
{
	glb_isp[0]->rpmsg = NULL;
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name	= ISP0_RPMSG_NAME },
	{ .name	= ISP1_RPMSG_NAME },
	{ .name	= ISP2_RPMSG_NAME },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

struct rpmsg_driver rpmsg_vin_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_sample_id_table,
	.probe		= isp_rpmsg_probe,
	.callback	= isp_rpmsg_cb,
	.remove 	= isp_rpmsg_remove,
};
EXPORT_SYMBOL(rpmsg_vin_client);
MODULE_AUTHOR("zhengzequn");
