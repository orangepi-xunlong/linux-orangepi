/*
 * sound\soc\sunxi\sunxi-snddmic.h
 * (C) Copyright 2014-2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__SUNXI_SNDDMIC_H_
#define	__SUNXI_SNDDMIC_H_

#ifdef CONFIG_SND_SUNXI_MAD
#include "sunxi-mad.h"
#endif

struct sunxi_snddmic_priv {
	struct snd_soc_card *card;
	struct platform_device *codec_device;

#ifdef CONFIG_SND_SUNXI_MAD
	struct sunxi_mad_priv mad_priv;
#endif

};

#endif
