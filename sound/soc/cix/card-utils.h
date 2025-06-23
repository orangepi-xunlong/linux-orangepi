/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Cix Technology Group Co., Ltd. */

#ifndef __CIX_CARD_H__
#define __CIX_CARD_H__
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <sound/soc.h>
#include <linux/workqueue.h>
#include <linux/gpio/consumer.h>

enum jack_det {
	JACK_DPIN = 0x0,
	JACK_DPOUT,
	JACK_HP, /* headphone or headset */

	JACK_CNT
};

#define JACK_MASK_DPIN   (0x1 << JACK_DPIN)
#define JACK_MASK_DPOUT  (0x1 << JACK_DPOUT)
#define JACK_MASK_HP     (0x1 << JACK_HP)

struct dai_link_info {

	unsigned int mclk_fs;

	unsigned int tx_mask;
	unsigned int rx_mask;
	unsigned int slots;
	unsigned int slot_width;

	struct snd_soc_jack	jack[JACK_CNT];
	struct snd_soc_jack_pin	jack_pin[JACK_CNT];
	unsigned int jack_det_mask;
};

struct cix_asoc_card {
	struct snd_soc_card *card;
	struct dai_link_info *link_info;

	/* pa, now support for alc1019 */
	struct gpio_desc *pdb0_gpiod; /* for chip power */
	struct gpio_desc *pdb1_gpiod;
	struct gpio_desc *pdb2_gpiod;
	struct gpio_desc *pdb3_gpiod;
	struct gpio_desc *beep_gpiod; /* for beep sound */

	/* codec, now support for alc5682 */
	struct gpio_desc *codec_gpiod; /* load switch for codec */
	struct gpio_desc *i2sint_gpiod; /* codec int */
	struct gpio_desc *hpmicdet_gpiod; /* mic detect */

	struct gpio_desc *mclk_gpiod;
};

int cix_card_parse_of(struct cix_asoc_card *priv);
int cix_card_parse_acpi(struct cix_asoc_card *priv);
#endif
