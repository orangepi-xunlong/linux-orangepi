// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <asm/dma.h>
#include "hdac.h"

static unsigned int azx_command_addr(u32 cmd)
{
	unsigned int addr = cmd >> 28;

	if (addr >= HDA_MAX_CODECS) {
		snd_BUG();
		addr = 0;
	}

	return addr;
}

/* receive a response */
static int azx_rirb_get_response(struct hdac_bus *bus, unsigned int addr,
				 unsigned int *res)
{
	struct hdac *hdac = bus_to_hdac(bus);
	int err;

 again:
	err = snd_hdac_bus_get_response(bus, addr, res);
	if (!err)
		return 0;

	if (hdac->no_response_fallback)
		return -EIO;

	if (!bus->polling_mode) {
		dev_warn(hdac->card->dev,
			 "hdac_get_response timeout, switching to polling mode: last cmd=0x%08x\n",
			 bus->last_cmd[addr]);
		bus->polling_mode = 1;
		goto again;
	}

	if (hdac->probing) {
		/* If this critical timeout happens during the codec probing
		 * phase, this is likely an access to a non-existing codec
		 * slot.  Better to return an error and reset the system.
		 */
		return -EIO;
	}

	/* no fallback mechanism? */
	if (!hdac->fallback_to_single_cmd)
		return -EIO;

	/* a fatal communication error; need either to reset or to fallback
	 * to the single_cmd mode
	 */
	if (hdac->allow_bus_reset && !hdac->response_reset && !hdac->in_reset) {
		hdac->response_reset = 1;
		dev_err(hdac->card->dev,
			"No response from codec, resetting bus: last cmd=0x%08x\n",
			bus->last_cmd[addr]);
		return -EAGAIN; /* give a chance to retry */
	}

	dev_err(hdac->card->dev,
		"hdac_get_response timeout, switching to single_cmd mode: last cmd=0x%08x\n",
		bus->last_cmd[addr]);
	hdac->single_cmd = 1;
	hdac->response_reset = 0;
	snd_hdac_bus_stop_cmd_io(bus);
	return -EIO;
}

/*
 * Use the single immediate command instead of CORB/RIRB for simplicity
 *
 * Note: according to Intel, this is not preferred use.  The command was
 *       intended for the BIOS only, and may get confused with unsolicited
 *       responses.  So, we shouldn't use it for normal operation from the
 *       driver.
 *       I left the codes, however, for debugging/testing purposes.
 */
//#define __FPGA_DEBUG__
/* receive a response */
static int azx_single_wait_for_response(struct hdac_bus *bus, unsigned int addr)
{
	int timeout = 500;

	while (timeout--) {
#ifdef __FPGA_DEBUG__
		udelay(100);
		pr_info("\tget rsp: IRS(0x68h)=0x%x\n",
			snd_hdac_chip_readw(bus, IRS));
#endif
		/* check IRV busy bit */
		if (snd_hdac_chip_readw(bus, IRS) & AZX_IRS_VALID) {
			/* reuse rirb.res as the response return value */
			bus->rirb.res[addr] = snd_hdac_chip_readl(bus, IR);
			return 0;
		}
		udelay(1);
	}

	if (printk_ratelimit())
		dev_info(bus->dev, "get_response timeout: IRS=0x%x, last cmd:0x%x\n",
			snd_hdac_chip_readw(bus, IRS), bus->last_cmd[addr]);
	bus->rirb.res[addr] = -1;

	return -EIO;
}

/* send a command */
static int azx_single_send_cmd(struct hdac_bus *bus, u32 val)
{
	unsigned int addr = azx_command_addr(val);
	int timeout = 50;

#ifdef __FPGA_DEBUG__
	pr_info("send verb cmd:0x%x, addr:0x%x\n", val, addr);
#endif

	bus->last_cmd[azx_command_addr(val)] = val;
	while (timeout--) {
		/* check ICB busy bit */
		if (!((snd_hdac_chip_readw(bus, IRS) & AZX_IRS_BUSY))) {

			snd_hdac_chip_writew(bus, IRS, snd_hdac_chip_readw(bus, IRS) |
									AZX_IRS_VALID);

			snd_hdac_chip_writel(bus, IC, val);

			snd_hdac_chip_writew(bus, IRS, snd_hdac_chip_readw(bus, IRS) |
									AZX_IRS_BUSY);

			return azx_single_wait_for_response(bus, addr);
		}
		udelay(1);
	}

	return -EIO;
}

/* receive a response */
static int azx_single_get_response(struct hdac_bus *bus, unsigned int addr,
				   unsigned int *res)
{
	if (res)
		*res = bus->rirb.res[addr];
	return 0;
}

/*
 * The below are the main callbacks from hda_codec.
 *
 * They are just the skeleton to call sub-callbacks according to the
 * current setting of hdac->single_cmd.
 */

/* send a command */
static int hdac_send_cmd(struct hdac_bus *bus, unsigned int val)
{
	struct hdac *hdac = bus_to_hdac(bus);

	if (hdac->single_cmd)
		return azx_single_send_cmd(bus, val);
	else
		return snd_hdac_bus_send_cmd(bus, val);
}

/* get a response */
static int hdac_get_response(struct hdac_bus *bus, unsigned int addr,
			    unsigned int *res)
{
	struct hdac *hdac = bus_to_hdac(bus);

	if (hdac->single_cmd)
		return azx_single_get_response(bus, addr, res);
	else
		return azx_rirb_get_response(bus, addr, res);
}

/*static*/ const struct hdac_bus_ops bus_core_ops = {
	.command = hdac_send_cmd,
	.get_response = hdac_get_response,
};

/*
 * interrupt handler
 */
static void stream_update(struct hdac_bus *bus, struct hdac_stream *s)
{
	/* check whether this IRQ is really acceptable */
	spin_unlock(&bus->reg_lock);
	snd_pcm_period_elapsed(s->substream);
	spin_lock(&bus->reg_lock);
}

static irqreturn_t hdac_interrupt(int irq, void *dev_id)
{
	struct hdac *hdac = dev_id;
	struct hdac_bus *bus = hdac_bus(hdac);
	u32 status;
	bool active, handled = false;
	int repeat = 0; /* count for avoiding endless loop */

	spin_lock(&bus->reg_lock);

	do {
		status = hdac_readl(hdac, INTSTS);
		if (status == 0 || status == 0xffffffff)
			break;

		handled = true;
		active = false;
		if (snd_hdac_bus_handle_stream_irq(bus, status, stream_update))
			active = true;

		status = hdac_readb(hdac, RIRBSTS);
		if (status & RIRB_INT_MASK) {
			/*
			 * Clearing the interrupt status here ensures that no
			 * interrupt gets masked after the RIRB wp is read in
			 * snd_hdac_bus_update_rirb. This avoids a possible
			 * race condition where codec response in RIRB may
			 * remain unserviced by IRQ, eventually falling back
			 * to polling mode in azx_rirb_get_response.
			 */
			hdac_writeb(hdac, RIRBSTS, RIRB_INT_MASK);
			active = true;
			if (status & RIRB_INT_RESPONSE)
				snd_hdac_bus_update_rirb(bus);
		}
	} while (active && ++repeat < 10);

	spin_unlock(&bus->reg_lock);

	return IRQ_RETVAL(handled);
}

void hdac_clear_irq(struct hdac *hdac)
{
	free_irq(hdac->irq, hdac);
}
EXPORT_SYMBOL_GPL(hdac_clear_irq);

int hdac_acquire_irq(struct hdac *hdac)
{
	struct hdac_bus *bus = hdac_bus(hdac);

	if (request_irq(hdac->irq, hdac_interrupt,
					IRQF_SHARED,
					"hda irq", hdac)) {
		dev_err(bus->dev,
				"unable to grab IRQ %d, disabling device\n",
				hdac->irq);
		return -1;
	}

	bus->irq = hdac->irq;

	return 0;
}
EXPORT_SYMBOL_GPL(hdac_acquire_irq);

static int stream_direction(struct hdac *hdac, unsigned char index)
{
	if (index >= hdac->capture_index_offset &&
	    index < hdac->capture_index_offset + hdac->capture_streams)
		return SNDRV_PCM_STREAM_CAPTURE;
	return SNDRV_PCM_STREAM_PLAYBACK;
}

/* initialize SD streams */
int cix_init_streams(struct hdac *hdac)
{
	int i;

	/* initialize each stream (aka device)
	 * assign the starting bdl address to each stream (device)
	 * and initialize
	 */
	for (i = 0; i < hdac->num_streams; i++) {
		struct hdac_stream *hs = kzalloc(sizeof(*hs), GFP_KERNEL);
		int dir, tag;

		if (!hs)
			return -ENOMEM;

		dir = stream_direction(hdac, i);
		/* stream tag must be unique throughout
		 * the stream direction group,
		 * valid values 1...15
		 * use separate stream tag if the flag
		 * AZX_DCAPS_SEPARATE_STREAM_TAG is used
		 */
		tag = i + 1;
		snd_hdac_stream_init(hdac_bus(hdac), hs,
				     i, dir, tag);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(cix_init_streams);

void cix_free_streams(struct hdac *hdac)
{
	struct hdac_bus *bus = hdac_bus(hdac);
	struct hdac_stream *s;

	while (!list_empty(&bus->stream_list)) {
		s = list_first_entry(&bus->stream_list, struct hdac_stream, list);
		list_del(&s->list);
		kfree(s);
	}
}
EXPORT_SYMBOL_GPL(cix_free_streams);

static int hdac_fixedup_regmap_addr(unsigned int *addr, unsigned int remap_offset)
{
	/* check whether need to do fixed up regmap */
	if (*addr <= remap_offset) {
		return 0;
	}

	*addr -= remap_offset;

	return 0;
}

int hdac_bus_stream_fixedup_remap(struct hdac_bus *bus, unsigned int remap_offset)
{
	struct hdac_stream *s;

	list_for_each_entry(s, &bus->stream_list, list)
		if (s->bdl.addr)
			hdac_fixedup_regmap_addr((unsigned int *)&(s->bdl.addr), remap_offset);

	hdac_fixedup_regmap_addr((unsigned int *)&(bus->posbuf.addr), remap_offset);
	hdac_fixedup_regmap_addr((unsigned int *)&(bus->rb.addr), remap_offset);

	return 0;
}

int hdac_pcm_stream_fixedup_remap(unsigned int *addr, unsigned int remap_offset)
{
	return hdac_fixedup_regmap_addr(addr, remap_offset);
}
