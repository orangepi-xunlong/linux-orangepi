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
#ifndef _AW_HDMI_DEV_H_
#define _AW_HDMI_DEV_H_

typedef struct {
	/* hdmi devices */
	dev_t          hdmi_devid;
	struct cdev   *hdmi_cdev;
	struct class  *hdmi_class;
	struct device *hdmi_device;
	/* cec devices */
	dev_t          cec_devid;
	struct cdev   *cec_cdev;
	struct class  *cec_class;
	struct device *cec_device;
} aw_device_t;

/* CEC config ioctl */
#define AW_IOCTL_CEC_S_PHYS_ADDR             _IOW( 'c', 1, __u16)
#define AW_IOCTL_CEC_G_PHYS_ADDR             _IOR( 'c', 2, __u16)
#define AW_IOCTL_CEC_S_LOG_ADDR              _IOW( 'c', 3, __u8)
#define AW_IOCTL_CEC_G_LOG_ADDR              _IOR( 'c', 4, __u8)
#define AW_IOCTL_CEC_TRANSMIT_MSG            _IOWR('c', 5, struct cec_msg)
#define AW_IOCTL_CEC_RECEIVE_MSG             _IOWR('c', 6, struct cec_msg)

enum aw_hdmi_ioctl_cmd_e {
	AW_IOCTL_HDMI_NULL            = 0,
	AW_IOCTL_HDMI_HDCP22_LOAD_FW  = 1,
	AW_IOCTL_HDMI_HDCP_ENABLE     = 2,
	AW_IOCTL_HDMI_HDCP_DISABLE    = 3,
	AW_IOCTL_HDMI_HDCP_INFO       = 4,
	AW_IOCTL_HDMI_GET_LOG_SIZE    = 5,
	AW_IOCTL_HDMI_GET_LOG         = 6,
	AW_IOCTL_HDMI_MAX_CMD,
};

enum aw_hdcp_debug_t {
	AW_HDCP_ENABLE_NORMAL   = 0,
	AW_HDCP_ENABLE_FORCE_14 = 1,
	AW_HDCP_ENABLE_FORCE_22 = 2,
};

#endif /* _AW_HDMI_DEV_H_ */
