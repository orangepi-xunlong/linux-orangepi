// SPDX-License-Identifier: GPL-2.0
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <asm/ioctl.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include "cix_hdcp.h"
#include "cix_hdcp_ioctl_cmd.h"
#include "hdcp2_tx_tmr.h"

#define TR_DPTX_AUX_CMD_READ 0x0900ul
#define TR_DPTX_AUX_CMD_WRITE 0x0800ul

int cix_hdcp2_ioctl_get_rx_state(struct cix_hdcp *hdcp, void *kdata)
{
	TX2_STATE_T *state = kdata;
	memcpy(state, &hdcp->state, sizeof(TX2_STATE_T));

	return 0;
}

int cix_hdcp2_ioctl_timer_start(struct cix_hdcp *hdcp, void *kdata)
{
	uint32_t ms = *(uint32_t *)kdata;

	hdcp2_tx_tmr_start(hdcp, ms);

	return 0;
}

int cix_hdcp2_ioctl_timer_stop(struct cix_hdcp *hdcp)
{
	hdcp2_tx_tmr_stop(hdcp);

	return 0;
}

int cix_hdcp2_ioctl_dpcd_access(struct cix_hdcp *hdcp, void *kdata)
{
	int ret = 0;
	ssize_t bytes;
	uint8_t *udata = NULL;
	struct drm_dp_aux *aux = hdcp->aux;
	struct device *dev = aux->dev;
	dptx_aux_trxn_t *aux_trxn = kdata;

	if (aux_trxn->cmd != TR_DPTX_AUX_CMD_READ &&
		aux_trxn->cmd != TR_DPTX_AUX_CMD_WRITE) {
		dev_err(dev, "%s, %d is unsupported\n", __func__, aux_trxn->cmd);
		ret = -EINVAL;
		goto exit;
	}

	if (!aux_trxn->data) {
		dev_err(dev, "%s, aux_trxn->data is NULL\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	if (!aux_trxn->ct) {
		dev_err(dev, "%s, aux_trxn->ct is zero\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	dev_dbg(dev, "%s, \ndata=0x%px, addr=0x%x, cnt=0x%x\n", __func__,
			aux_trxn->data, aux_trxn->addr, aux_trxn->ct);

	udata = kmalloc(aux_trxn->ct, GFP_KERNEL | __GFP_ZERO);

	if (aux_trxn->cmd == TR_DPTX_AUX_CMD_READ) {
		bytes = drm_dp_dpcd_read(aux, aux_trxn->addr, udata, aux_trxn->ct);
		if (bytes < 0 || bytes < aux_trxn->ct) {
			dev_err(dev, "%s, failed to read dpcd %x, bytes=%lx\n",
					__func__, aux_trxn->addr, bytes);
			ret = -EINVAL;
			goto exit;
		}

		if (copy_to_user((void __user *)aux_trxn->data, udata, aux_trxn->ct) != 0) {
			dev_err(dev, "%s, copy dpcd data from userspace failed\n", __func__);
			ret = -EFAULT;
			goto exit;
		}
	} else {
		if (copy_from_user(udata, (void __user *)aux_trxn->data, aux_trxn->ct) != 0) {
			dev_err(dev, "%s, copy dpcd data to userspace failed\n", __func__);
			ret = -EFAULT;
			goto exit;
		}

		bytes = drm_dp_dpcd_write(aux, aux_trxn->addr, udata, aux_trxn->ct);
		if (bytes < 0 || bytes < aux_trxn->ct) {
			dev_err(dev, "%s, failed to write dpcd %x, bytes=%lx\n",
					__func__, aux_trxn->addr, bytes);
			ret = -EINVAL;
			goto exit;
		}
	}
exit:
	if (udata)
		kfree(udata);
	return ret;
}
