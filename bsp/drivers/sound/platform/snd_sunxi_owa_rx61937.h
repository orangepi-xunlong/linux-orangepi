/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_OWA_RX61937_H
#define __SND_SUNXI_OWA_RX61937_H

#define	SUNXI_OWA_EXP_CTL	0x40
#define	SUNXI_OWA_EXP_ISTA	0x44
#define	SUNXI_OWA_EXP_INFO0	0x48
#define	SUNXI_OWA_EXP_INFO1	0x4C
#define	SUNXI_OWA_EXP_DBG0	0x50
#define	SUNXI_OWA_EXP_DBG1	0x54


/* SUNXI_OWA_EXP_CTL register */
#define INSET_DET_NUM		0
#define INSET_DET_EN		8
#define SYNCW_BIT_EN		9
#define DATA_TYPE_BIT_EN	10
#define DATA_LEG_BIT_EN		11
#define AUDIO_DATA_BIT_EN	12
#define RX_MODE			13
#define RX_MODE_MAN		14
#define UNIT_SEL		15
#define RPOTBF_NUM		16
#define BURST_DATA_OUT_SEL	30

/* SUNXI_OWA_EXP_ISTA register */
#define INSET_INT		0
#define PAPB_CAP_INT		1
#define PCPD_CAP_INT		2
#define RPDB_ERR_INT		3
#define PC_DTYOE_CH_INT		4
#define PC_ERR_FLAG_INT		5
#define PC_BIT_CH_INT		6
#define PC_PAUSE_STOP_INT	7
#define PD_CHAN_INT		8
#define INSET_INT_EN		16
#define PAPB_CAP_INT_EN		17
#define PCPD_CAP_INT_EN		18
#define RPDB_ERR_INT_EN		19
#define PC_DTYOE_CH_INT_EN	20
#define PC_ERR_FLAG_INT_EN	21
#define PC_BIT_CH_INT_EN	22
#define PC_PAUSE_STOP_INT_EN	23
#define PD_CHAN_INT_EN		24

/* SUNXI_OWA_EXP_INFO0 register */
#define PD_DATA_INFO		0
#define PC_DATA_INFO		16

/* SUNXI_OWA_EXP_INFO1 register */
#define SAMPLE_RATE_VAL		0
#define RPOTBF_VAL		16

/* SUNXI_OWA_EXP_DBG0 register */
#define RE_DATA_COUNT_VAL	0
#define DATA_CAP_STA_MACHE	16

/* SUNXI_OWA_EXP_DBG1 register */
#define SAMPLE_RATE_COUNT	0
#define RPOTBF_COUNT		16

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_OWA_RXIEC61937)
int sunxi_add_rx_raw_controls(struct snd_soc_component *component);
#else
static inline int sunxi_add_rx_raw_controls(struct snd_soc_component *component)
{
	return 0;
}
#endif

#endif /* __SND_SUNXI_OWA_RX61937_H */
