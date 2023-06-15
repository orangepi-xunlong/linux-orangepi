/*
 * Firmware APIs for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FWIO_H_INCLUDED
#define FWIO_H_INCLUDED

#define XR829_HW_REV0       (8290)
#define FIRMWARE_BASE_PATH  ""

#define XR829_BOOTLOADER    (FIRMWARE_BASE_PATH "boot_xr829.bin")
#define XR829_FIRMWARE      (FIRMWARE_BASE_PATH "fw_xr829.bin")
#define XR829_SDD_FILE      (FIRMWARE_BASE_PATH "sdd_xr829.bin")

#define SDD_PTA_CFG_ELT_ID              0xEB
#define SDD_REFERENCE_FREQUENCY_ELT_ID  0xC5
#define SDD_MAX_OUTPUT_POWER_2G4_ELT_ID 0xE3
#define SDD_MAX_OUTPUT_POWER_5G_ELT_ID  0xE4

#define FIELD_OFFSET(type, field) ((u8 *)&((type *)0)->field - (u8 *)0)
#define FIND_NEXT_ELT(e) (struct xradio_sdd *)((u8 *)&e->data + e->length)
struct xradio_sdd {
	u8 id;
	u8 length;
	u8 data[];
};

struct xradio_common;
int xradio_load_firmware(struct xradio_common *hw_priv);
int xradio_dev_deinit(struct xradio_common *hw_priv);
int xradio_update_dpllctrl(struct xradio_common *hw_priv, u32 dpll_update);

#endif
