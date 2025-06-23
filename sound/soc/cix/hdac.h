/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Cix Technology Group Co., Ltd. */

#ifndef __HDAC_H__
#define __HDAC_H__

#include <linux/regmap.h>
#include <sound/hdaudio.h>
#include <sound/hdaudio_ext.h>
#include <sound/hda_codec.h>
#include <sound/hda_register.h>
#include <linux/gpio/consumer.h>

struct hdac;
typedef unsigned int (*azx_get_pos_callback_t)(struct hdac *, struct hdac_stream *);
typedef int (*azx_get_delay_callback_t)(struct hdac *, struct hdac_stream *, unsigned int pos);

struct hdac {
	struct device *dev;
	//struct hdac_bus bus;
	struct hda_bus hbus;

	struct regmap *regmap;
	void __iomem *remap_addr;
	void __iomem *regs;

	int irq;

	struct snd_card *card;
	int dev_index;
	const char *modelname;

	/* chip type specific */
	int driver_type;
	unsigned int driver_caps;
	int playback_streams;
	int playback_index_offset;
	int capture_streams;
	int capture_index_offset;
	int num_streams;
	int jackpoll_interval; /* jack poll interval in jiffies */

	/* Register interaction. */
	const struct hda_controller_ops *ops;

	/* position adjustment callbacks */
	azx_get_pos_callback_t get_position[2];
	azx_get_delay_callback_t get_delay[2];

	/* locks */
	struct mutex open_mutex; /* Prevents concurrent open/close operations */

	/* PCM */
	struct list_head pcm_list; /* azx_pcm list */

	/* HD codec */
	int  codec_probe_mask; /* copied from probe_mask option */
	unsigned int beep_mode;

	/* flags */
	int bdl_pos_adj;
	unsigned int running:1;
	unsigned int fallback_to_single_cmd:1;
	unsigned int single_cmd:1;
	unsigned int probing:1; /* codec probing phase */
	unsigned int snoop:1;
	unsigned int uc_buffer:1; /* non-cached pages for stream buffers */
	unsigned int align_buffer_size:1;
	unsigned int region_requested:1;
	unsigned int disabled:1; /* disabled by vga_switcheroo */
	unsigned int pm_prepared:1;

	/* assigned PCMs */
	DECLARE_BITMAP(pcm_dev_bits, SNDRV_PCM_DEVICES);

	/* misc op flags */
	unsigned int allow_bus_reset:1;	/* allow bus reset at fatal error */
	/* status for codec/controller */
	unsigned int shutdown :1;	/* being unloaded */
	unsigned int response_reset:1;	/* controller was reset */
	unsigned int in_reset:1;	/* during reset operation */
	unsigned int no_response_fallback:1; /* don't fallback at RIRB error */
	unsigned int bus_probing :1;	/* during probing process */
	unsigned int keep_power:1;	/* keep power up for notification */

	int primary_dig_out_type;	/* primary digital out PCM type */
	unsigned int mixer_assigned;	/* codec addr for mixer name */
};

/* Both the CPU DAI and platform drivers will access this data */
struct hda_ipbloq {
	struct device *dev;

	const struct snd_soc_component_driver *comp_drv;
	const struct regmap_config *regmap_cfg;
	struct regmap *cru_regmap;
	struct reset_control *hda_rst;

	struct clk *sysclk;
	struct clk *clk48m;
	struct clk *clkrst;

	struct hdac hdac;

	/* hda dma access ddr by remapped,
	 * fixed up dma address for hda registers
	 */
	unsigned int remap_offset;

	/* for codec daguther board power */
	struct gpio_desc *pdb_gpiod;
};

#define hdac_bus(hdac)	(&(hdac)->hbus.core)
#define bus_to_hdac(_bus)	container_of(_bus, struct hdac, hbus.core)

#define hdac_writel(hdac, reg, value) \
        snd_hdac_chip_writel(hdac_bus(hdac), reg, value)
#define hdac_readl(hdac, reg) \
        snd_hdac_chip_readl(hdac_bus(hdac), reg)
#define hdac_writew(hdac, reg, value) \
        snd_hdac_chip_writew(hdac_bus(hdac), reg, value)
#define hdac_readw(hdac, reg) \
        snd_hdac_chip_readw(hdac_bus(hdac), reg)
#define hdac_writeb(hdac, reg, value) \
        snd_hdac_chip_writeb(hdac_bus(hdac), reg, value)
#define hdac_readb(chhdacip, reg) \
        snd_hdac_chip_readb(hdac_bus(hdac), reg)

int cix_init_streams(struct hdac *hdac);
void cix_free_streams(struct hdac *hdac);

void hdac_clear_irq(struct hdac *hdac);
int hdac_acquire_irq(struct hdac *hdac);

int hdac_bus_stream_fixedup_remap(struct hdac_bus *bus, unsigned int remap_offset);
int hdac_pcm_stream_fixedup_remap(unsigned int *addr, unsigned int remap_offset);

struct hdac_bus *get_ipb_hda_bus(void);
struct snd_card *get_hda_card(void);
#endif
