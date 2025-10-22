/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sound\soc\sunxi\snd_sunxi_jack.h
 * (C) Copyright 2022-2027
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_JACK_H
#define __SND_SUNXI_JACK_H

#include <sound/jack.h>
#include "snd_sunxi_common.h"
#include "snd_sunxi_adapter.h"

/* mode -> codec */
struct sunxi_jack_sdbp_hrtime {
	struct hrtimer timer;
	ktime_t kt;
	struct work_struct hrtime_work;
};

enum SDBP_METHOD {
	SDBP_NONE = 0,
	SDBP_IRQ = 1,
	SDBP_SCAN,
	SDBP_METHOD_CNT,
};

struct sunxi_jack_sdbp {
	/* sdbp - headset detective based on headphone.
	 *
	 * 1. dst attribute - "jack-sdbp-method".
	 * 1.1 1 - Keep detecting by enable hmbias to trigger interupt.
	 * 1.2 2 - Keep detecting by delayed work queue to read mic-pin voltage.
	 *
	 * 2. dst attribute - "jack-sdbp-scan-single-time".(if "jack-sdbp-method" == SDBP_SCAN).
	 * 2.1 The unit is ms.
	 * 2.2 The length of time between each detection.
	 * 2.3 The value must greater than 1000.
	 */
	enum SDBP_METHOD jack_sdbp_method;
	unsigned int jack_sdbp_scan_single_time;

	int (*jack_sdbp_irq_init)(void *);
	void (*jack_sdbp_irq_exit)(void *);
	void (*jack_sdbp_irq_clean)(void *);
	void (*jack_sdbp_irq_work)(void *, enum snd_jack_types *);

	int (*jack_sdbp_scan_init)(void *);
	void (*jack_sdbp_scan_exit)(void *);
	void (*jack_sdbp_scan_work)(void *, enum snd_jack_types *);

	bool is_working;
	spinlock_t sdbp_lock;
	struct work_struct sdbp_irq_work;
	struct work_struct sdbp_scan_work;
	struct sunxi_jack_sdbp_hrtime sdbp_scan_hrt;
};

enum JACK_DET_METHOD {
	JACK_DET_NONE = 0,
	JACK_DET_CODEC,
	JACK_DET_EXTCON,
	JACK_DET_GPIO,
	JACK_DET_ADV,
	JACK_DET_CNT,
};

struct sunxi_jack_codec {
	struct platform_device *pdev;

	void *data;
	int (*jack_init)(void *);
	void (*jack_exit)(void *);
	int (*jack_suspend)(void *);
	int (*jack_resume)(void *);

	void (*jack_irq_clean)(void *);
	void (*jack_det_irq_work)(void *, enum snd_jack_types *);
	void (*jack_det_scan_work)(void *, enum snd_jack_types *);

	int (*jack_status_sync)(void *data, enum snd_jack_types);

	struct sunxi_jack_sdbp jack_sdbp;
};

/* mode -> extcon */
enum JACK_PLUG_STA {
	JACK_PLUG_STA_OUT = 0x0,
	JACK_PLUG_STA_IN,
};

enum sunxi_jack_modes {
	SND_JACK_MODE_NULL = -1,
	SND_JACK_MODE_OFF = 0,
	SND_JACK_MODE_USB,
	SND_JACK_MODE_HP,
	SND_JACK_MODE_MICN,
	SND_JACK_MODE_MICI,
	SND_JACK_MODE_CNT,
};

/* for jack pin config */
struct sunxi_jack_pins {
	uint32_t pin;
	bool used;
};

/* for jack mode config */
struct sunxi_jack_modes_map {
	enum sunxi_jack_modes type;
	uint32_t *map_value;
};

enum jack_system_sta {
	JACK_SYS_STA_INIT = 0,
	JACK_SYS_STA_RESUME,
	JACK_SYS_STA_NORMAL,
};

struct sunxi_jack_typec_cfg {
	uint32_t sw_pin_max;

	/* config for jack pin */
	struct sunxi_jack_pins *jack_pins;

	/* truth table for jack mode */
	struct sunxi_jack_modes_map *modes_map;
};

/* note:
 * "jack-swpin-max = <n>" - the num of pins what need to be controlled
 * "jack-swpin-0" - The pins that need to be controlled
 * "jack-swpin-1"
 * "jack-swpin-..."
 * "jack-swpin-n"
 *
 * The following is the truth table:
 * 1 - high level, 0 - low level, 0xf - no operation
 * "jack-mode-off" - The status of the above pins when the mode if off
 * "jack-mode-usb" - The status of the above pins when the mode if usb
 * "jack-mode-hp" - The status of the above pins when the mode if hp
 * "jack-mode-micn" - The status of the above pins when the mic insert
 * "jack-mode-mici" - The status of the above pins when the mic reverse insertion
 *
 * EX:
 * jack-swpin-max	= <3>;
 * jack-swpin-0		= <&pio PH 9 GPIO_ACTIVE_HIGH>;
 * jack-swpin-1		= <&pio PH 10 GPIO_ACTIVE_HIGH>;
 * jack-swpin-2		= <&pio PH 11 GPIO_ACTIVE_HIGH>;
 * jack-mode-off	= <0xf 1 0>;
 * jack-mode-usb	= <0xf 0 1>;
 * jack-mode-hp		= <0xf 0 0>;
 * jack-mode-micn	= <1 0xf 0xf>;
 * jack-mode-mici	= <0 0xf 0xf>;
 */

struct sunxi_jack_extcon {
	struct platform_device *pdev;

	enum JACK_PLUG_STA jack_plug_sta;
	struct extcon_dev *extdev;
	struct notifier_block hp_nb;
	struct sunxi_jack_typec_cfg jack_typec_cfg;

	void *data;
	int (*jack_init)(void *);
	void (*jack_exit)(void *);
	int (*jack_suspend)(void *);
	int (*jack_resume)(void *);

	void (*jack_irq_clean)(void *);
	void (*jack_det_irq_work)(void *, enum snd_jack_types *);
	void (*jack_det_scan_work)(void *, enum snd_jack_types *);

	int (*jack_status_sync)(void *data, enum snd_jack_types);
};

/* mode -> gpio */
struct sunxi_jack_gpio {
	struct platform_device *pdev;

	bool det_level;
	unsigned int debounce_time;

	int det_gpio;
	struct gpio_desc *desc;

	void *data;
	int (*jack_status_sync)(void *data, enum snd_jack_types);
};

/* mode -> advance */
typedef irqreturn_t (*jack_irq_work)(int, void *);

struct sunxi_jack_adv {
	struct device *dev;

	enum JACK_PLUG_STA jack_plug_sta;
	struct extcon_dev *extdev;
	struct notifier_block hp_nb;
	struct sunxi_jack_typec_cfg jack_typec_cfg;
	struct power_supply *pmu_psy;
	bool typec;

	void *data;
	int (*jack_init)(void *);
	void (*jack_exit)(void *);
	int (*jack_suspend)(void *);
	int (*jack_resume)(void *);

	int (*jack_irq_requeset)(void *, jack_irq_work work);
	void (*jack_irq_free)(void *);
	void (*jack_irq_enable)(void *);
	void (*jack_irq_disable)(void *);
	void (*jack_irq_clean)(void *, int);

	void (*jack_det_irq_work)(void *, enum snd_jack_types *);
	void (*jack_det_scan_work)(void *, enum snd_jack_types *);

	int (*jack_status_sync)(void *data, enum snd_jack_types);
};

struct sunxi_jack_port {
	struct sunxi_jack_codec *jack_codec;
	struct sunxi_jack_extcon *jack_extcon;
	struct sunxi_jack_gpio *jack_gpio;
	struct sunxi_jack_adv *jack_adv;
};

/* common */
struct sunxi_jack {
	struct snd_soc_jack jack;
	struct snd_soc_card *card;

	unsigned int jack_irq;
	struct mutex det_mutex;
	struct work_struct det_irq_work;
	struct delayed_work det_sacn_work;

	/* snd_jack_report value */
	enum jack_system_sta system_sta;
	enum snd_jack_types type;
	enum snd_jack_types type_old;

	/* init by codec or platform */
	struct sunxi_jack_codec *jack_codec;	/* mode -> codec */
	struct sunxi_jack_extcon *jack_extcon;	/* mode -> extcon */
	struct sunxi_jack_gpio *jack_gpio;	/* mode -> gpio */
	struct sunxi_jack_adv *jack_adv;	/* mode -> advance */
};

/* jack codec */
int snd_sunxi_jack_codec_init(void *jack_data);
void snd_sunxi_jack_codec_exit(void *jack_data);
int snd_sunxi_jack_codec_register(struct snd_soc_card *card);
void snd_sunxi_jack_codec_unregister(struct snd_soc_card *card);

/* jack extcon */
int snd_sunxi_jack_extcon_init(void *jack_data);
void snd_sunxi_jack_extcon_exit(void *jack_data);
int snd_sunxi_jack_extcon_register(struct snd_soc_card *card);
void snd_sunxi_jack_extcon_unregister(struct snd_soc_card *card);

/* jack gpio */
int snd_sunxi_jack_gpio_init(void *jack_data);
void snd_sunxi_jack_gpio_exit(void *jack_data);
int snd_sunxi_jack_gpio_register(struct snd_soc_card *card);
void snd_sunxi_jack_gpio_unregister(struct snd_soc_card *card);

/* jack advance */
int snd_sunxi_jack_adv_init(void *jack_data);
void snd_sunxi_jack_adv_exit(void *jack_data);
int snd_sunxi_jack_adv_register(struct snd_soc_card *card);
void snd_sunxi_jack_adv_unregister(struct snd_soc_card *card);

/* jack api */
extern int snd_sunxi_jack_register(struct snd_soc_card *card,
				   enum JACK_DET_METHOD jack_support);
extern void snd_sunxi_jack_unregister(struct snd_soc_card *card,
				      enum JACK_DET_METHOD jack_support);
extern int snd_sunxi_jack_init(struct sunxi_jack_port *sunxi_jack_port);
extern int snd_sunxi_jack_exit(struct sunxi_jack_port *sunxi_jack_port);

void sunxi_jack_typec_mode_set(struct sunxi_jack_typec_cfg *jack_typec_cfg,
			       enum sunxi_jack_modes mode);

#endif /* __SND_SUNXI_JACK_H */
