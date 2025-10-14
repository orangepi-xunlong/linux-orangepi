/* SPDX-License-Identifier: GPL-2.0*/
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

int cix_hdcp2_ioctl_get_rx_state(struct cix_hdcp *hdcp, void *kdata);
int cix_hdcp2_ioctl_timer_start(struct cix_hdcp *hdcp, void *kdata);
int cix_hdcp2_ioctl_timer_stop(struct cix_hdcp *hdcp);
int cix_hdcp2_ioctl_dpcd_access(struct cix_hdcp *hdcp, void *kdata);
int cix_hdcp2_ioctl_hw_init(struct cix_hdcp *hdcp);
int cix_hdcp2_ioctl_cipher_enable(struct cix_hdcp *hdcp, void *kdata);
int cix_hdcp2_cix_ioctl_cipher_disable(struct cix_hdcp *hdcp);
int cix_hdcp2_ioctl_get_kd(struct cix_hdcp *hdcp, void *kdata);
int cix_hdcp2_ioctl_get_dkey2(struct cix_hdcp *hdcp, void *kdata);
