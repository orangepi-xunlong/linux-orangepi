/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_saradc_ioctl.h
 * Description:
 */

#ifndef __CVI_WIEGAND_IOCTL_H__
#define __CVI_WIEGAND_IOCTL_H__

struct wgn_tx_cfg {
	uint32_t tx_lowtime;
	uint32_t tx_hightime;
	uint32_t tx_bitcount;
	uint32_t tx_msb1st;
	uint32_t tx_opendrain;
};

struct wgn_rx_cfg {
	uint32_t rx_debounce;
	uint32_t rx_idle_timeout;
	uint32_t rx_bitcount;
	uint32_t rx_msb1st;
};

#define IOCTL_WGN_BASE			'W'
#define IOCTL_WGN_SET_TX_CFG		_IO(IOCTL_WGN_BASE, 1)
#define IOCTL_WGN_SET_RX_CFG		_IO(IOCTL_WGN_BASE, 2)
#define IOCTL_WGN_GET_TX_CFG		_IO(IOCTL_WGN_BASE, 3)
#define IOCTL_WGN_GET_RX_CFG		_IO(IOCTL_WGN_BASE, 4)
#define IOCTL_WGN_TX			_IO(IOCTL_WGN_BASE, 5)
#define IOCTL_WGN_RX			_IO(IOCTL_WGN_BASE, 6)
#define IOCTL_WGN_GET_VAL		_IO(IOCTL_WGN_BASE, 7)

#endif // __CVI_WIEGAND_IOCTL_H__
