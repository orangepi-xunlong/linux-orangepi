/*
 * linux/sunxi-sid.h
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sunxi soc chip version and chip id manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_MACH_SUNXI_CHIP_H
#define __SUNXI_MACH_SUNXI_CHIP_H

#include <linux/types.h>


/* The key info in Efuse */

#define EFUSE_CHIPID_NAME            "chipid"
#define EFUSE_BROM_CONF_NAME         "brom_conf"
#define EFUSE_BROM_TRY_NAME          "brom_try"
#define EFUSE_THM_SENSOR_NAME        "thermal_sensor"
#define EFUSE_FT_ZONE_NAME           "ft_zone"
#define EFUSE_TV_OUT_NAME            "tvout"
#define EFUSE_TVE_NAME               "tve"
#define EFUSE_OEM_NAME               "oem"
#define EFUSE_ANTI_BLUSH_NAME        "anti_blushing"

#define EFUSE_PSENSOR_NAME           "psensor"
#define EFUSE_DDR_CFG_NAME           "ddr_cfg"
#define EFUSE_LDOA_NAME              "ldoa"
#define EFUSE_LDOB_NAME              "ldob"
#define EFUSE_AUDIO_BIAS_NAME        "audio_bias"
#define EFUSE_GAMMA_NAME             "gamma"
#define EFUSE_WR_PROTECT_NAME        "write_protect"
#define EFUSE_RD_PROTECT_NAME        "read_protect"
#define EFUSE_IN_NAME                "in"
#define EFUSE_ID_NAME                "id"
#define EFUSE_ROTPK_NAME             "rotpk"
#define EFUSE_SSK_NAME               "ssk"
#define EFUSE_RSSK_NAME              "rssk"
#define EFUSE_HDCP_HASH_NAME         "hdcp_hash"
#define EFUSE_HDCP_PKF_NAME          "hdcp_pkf"
#define EFUSE_HDCP_DUK_NAME          "hdcp_duk"
#define EFUSE_EK_HASH_NAME           "ek_hash"
#define EFUSE_SN_NAME                "sn"
#define EFUSE_NV1_NAME               "nv1"
#define EFUSE_NV2_NAME               "nv2"
#define EFUSE_BACKUP_KEY_NAME        "backup_key"
#define EFUSE_BACKUP_KEY2_NAME       "backup_key2"
#define EFUSE_TCON_PARM_NAME         "tcon_parm"
#define EFUSE_RSAKEY_HASH_NAME       "rsakey_hash"
#define EFUSE_RENEW_NAME             "renewability"
#define EFUSE_OPT_ID_NAME            "operator_id"
#define EFUSE_LIFE_CYCLE_NAME        "life_cycle"
#define EFUSE_JTAG_SECU_NAME         "jtag_security"
#define EFUSE_JTAG_ATTR_NAME         "jtag_attr"
#define EFUSE_CHIP_CONF_NAME         "chip_config"
#define EFUSE_RESERVED_NAME          "reserved"
#define EFUSE_RESERVED2_NAME         "reserved2"

#define SUNXI_KEY_NAME_LEN	32

#define SID_PRCTL			0x40
#define SID_RDKEY			0x60
#define SID_OP_LOCK			0xAC

#define SRAM_CTRL_BASE		"allwinner,sram_ctrl"
#define EFUSE_SID_BASE		"allwinner,sunxi-sid"

#define sunxi_efuse_read(key_name, read_buf) \
		sunxi_efuse_readn(key_name, read_buf, 1024)

/* The interface functions */

unsigned int sunxi_get_soc_ver(void);
int sunxi_get_soc_chipid(unsigned char *chipid);
int sunxi_get_soc_chipid_str(char *chipid);
int sunxi_get_soc_ft_zone_str(char *serial);
int sunxi_get_soc_rotpk_status_str(char *status);
int sunxi_get_pmu_chipid(unsigned char *chipid);
int sunxi_get_serial(unsigned char *serial);
unsigned int sunxi_get_soc_bin(void);
int sunxi_soc_is_secure(void);
s32 sunxi_get_platform(s8 *buf, s32 size);
s32 sunxi_efuse_readn(s8 *key_name, void *buf, u32 n);

#endif  /* __SUNXI_MACH_SUNXI_CHIP_H */
