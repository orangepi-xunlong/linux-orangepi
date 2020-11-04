/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _ESM_HDCP_HDMI_IMAGE_COMMON_H_
#define _ESM_HDCP_HDMI_IMAGE_COMMON_H_

#define ESM_GET_FW_VERSION_UPPER(__x__)   (((__x__) >> 16) & 0x0000FFFF)
#define ESM_GET_FW_VERSION_LOWER(__x__)   (((__x__)) & 0x0000FFFF)
#define ESM_GET_FULL_VERSION(__x__)       (((__x__) >> 24) & 0xff)
#define ESM_GET_SYSTEM_VERSION(__x__)     (((__x__) >> 16) & 0xff)
#define ESM_GET_BSP_VERSION(__x__)        (((__x__) >> 8) & 0xff)
#define ESM_GET_APP_ID(__x__)             ((__x__) & 0xff)
#define ESM_GET_CONFIG_VERSION(__x__)     (((__x__) >> 16) & 0xff)
#define ESM_GET_CONFIG_CTRL(__x__)        (((__x__) >> 24) & 0xff)


#define ELP_BYTE_PUT(__img__, __val_size__, __val__) elliptic_byte_put(__img__, __val_size__, __val__)
#define ELP_BYTE_GET(__img__, __val__)               elliptic_byte_get(__img__, __val__)


#endif /* _ESM_HDCP_HDMI_IMAGE_COMMON_H_ */
