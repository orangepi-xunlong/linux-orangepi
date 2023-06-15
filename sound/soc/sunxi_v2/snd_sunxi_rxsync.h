/*
 * sound\soc\sunxi\snd_sunxi_rxsync.h
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
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
	RX_SYNC_DSP_DOMAIN,
	RX_SYNC_DOMAIN_CNT
} rx_sync_domain_t;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_RXSYNC)
int sunxi_rx_sync_probe(rx_sync_domain_t domain);
void sunxi_rx_sync_remove(rx_sync_domain_t domain);
void sunxi_rx_sync_startup(rx_sync_domain_t domain, int id,
	void *data, void (*rx_enable)(void *data, bool enable));
void sunxi_rx_sync_shutdown(rx_sync_domain_t domain, int id);
void sunxi_rx_sync_control(rx_sync_domain_t domain, int id, bool enable);
#else
static inline int sunxi_rx_sync_probe(rx_sync_domain_t domain)
{
	return 0;
}
static inline void sunxi_rx_sync_remove(rx_sync_domain_t domain)
{
	return;
}
static inline void sunxi_rx_sync_startup(rx_sync_domain_t domain, int id,
	void *data, void (*rx_enable)(void *data, bool enable))
{
	return;
}
static inline void sunxi_rx_sync_shutdown(rx_sync_domain_t domain, int id)
{
	return;
}
static inline void sunxi_rx_sync_control(rx_sync_domain_t domain, int id, bool enable)
{
	return;
}
#endif

#endif /* __SND_SUNXI_RXSYNC_H */
