/**
 ******************************************************************************
 *
 * @file bl_v7.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#ifndef _BL_V7_H_
#define _BL_V7_H_

#include "bl_platform.h"

struct bl_device {
	const char *firmware;
	const struct bl_sdio_card_reg *reg;
	u8 *mp_regs;
	u8 max_ports;
	u8 mp_agg_pkt_limit;
	bool supports_sdio_new_mode;
	bool has_control_mask;
	bool supports_fw_dump;
	u16 tx_buf_size;
	u32 mp_tx_agg_buf_size;
	u32 mp_rx_agg_buf_size;
	u8 auto_tdls;
};

int bl_device_init(struct sdio_func *func, struct bl_plat **bl_plat);
void bl_device_deinit(struct bl_plat *bl_plat);

#endif /* _BL_V7_H_ */
