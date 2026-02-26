/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021
 * caiyongheng <caiyongheng@allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_RXSYNC_H
#define __SND_SUNXI_RXSYNC_H

typedef enum {
	RX_SYNC_SYS_DOMAIN,
	RX_SYNC_MCU_DOMAIN,
	RX_SYNC_DOMAIN_CNT
} rx_sync_domain_t;

int sunxi_rx_sync_probe(rx_sync_domain_t domain);
void sunxi_rx_sync_remove(rx_sync_domain_t domain);
int sunxi_rx_sync_register_cb(rx_sync_domain_t domain, int id,
			      void *data, void (*rx_enable)(void *data, bool enable));
void sunxi_rx_sync_unregister_cb(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_startup(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_shutdown(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_control(rx_sync_domain_t domain, int id, bool enable);

#endif /* __SND_SUNXI_RXSYNC_H */
