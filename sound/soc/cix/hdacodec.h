/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Cix Technology Group Co., Ltd. */
#ifndef __HDACODEC_H__
#define __HDACODEC_H__

#include <sound/hdaudio.h>
#include <sound/hdaudio_ext.h>
#include <sound/hda_codec.h>
#include <sound/hda_register.h>

#include <linux/platform_device.h>

#define HDA_CODEC_DRV_NAME "hda-audio-codec"

#define HDA_CODEC_FMT (SNDRV_PCM_FMTBIT_S16_LE)

struct hda_codec_ext_ops {
	int (*startup)(struct hda_codec *codec);
	void (*shutdown)(struct hda_codec *codec);
	int (*hw_params)(struct hda_codec *codec, int stream);
	int (*hw_free)(struct hda_codec *codec, int stream);
	int (*prepare)(struct hda_codec *codec, int stream);
	int (*check_pm)(struct hda_codec *codec, int system_pm);
};

/* hda codec data */
struct hda_codec_pdata {
	const struct hda_codec_ext_ops *ops;

	void *data;
};

struct hda_codec_bus_pdata {
	void *bus;
};

struct hda_codec_priv {
	struct device *dev;
	struct hdac_bus *bus;
	struct hda_codec_bus_pdata hcbdata;
	struct hda_codec_pdata *codec_pdata; //for codec callback
	struct hda_codec *codec;
	struct snd_pcm_hw_params *codec_params[SNDRV_PCM_STREAM_LAST + 1];
	struct hdac_driver *hdac_driver;
	struct delayed_work probe_work;
};

int hda_codec_driver_probe(struct device *dev);
int hda_codec_driver_remove(struct device *dev);

int hdacodec_dev_probe(struct hdac_device *hdev);
int hdacodec_dev_remove(struct hdac_device *hdev);
int hdacodec_dev_match(struct hdac_device *dev, struct hdac_driver *drv);

int hda_codec_init(struct hdac_bus *bus);

int hda_codec_probe(struct platform_device *pdev);
int hda_codec_remove(struct platform_device *pdev);

int hda_codec_suspend(struct device *dev);
int hda_codec_resume(struct device *dev);

int snd_hda_get_devices(struct hda_codec *codec, hda_nid_t nid,
	u8 *dev_list, int max_devices);
int snd_hda_get_dev_select(struct hda_codec *codec, hda_nid_t nid);
int snd_hda_set_dev_select(struct hda_codec *codec, hda_nid_t nid, int dev_id);
unsigned int snd_hda_get_num_devices(struct hda_codec *codec, hda_nid_t nid);

int __maybe_unused hda_codec_runtime_suspend(struct device *dev);
int __maybe_unused hda_codec_runtime_resume(struct device *dev);

static const struct dev_pm_ops hda_codec_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(hda_codec_suspend, hda_codec_resume)
	SET_RUNTIME_PM_OPS(hda_codec_runtime_suspend, hda_codec_runtime_resume, NULL)
};

#define HDA_CODEC_DRIVER_REGISTER(of_id, acpi_id) \
static struct platform_driver hda_codec_driver = { \
	.driver = { \
		.name = HDA_CODEC_DRV_NAME, \
		.of_match_table = of_match_ptr(of_id), \
		.acpi_match_table = ACPI_PTR(acpi_id), \
		.pm = &hda_codec_pm_ops, \
	}, \
	.probe = hda_codec_probe, \
	.remove = hda_codec_remove, \
}; \
module_platform_driver(hda_codec_driver);

#endif
