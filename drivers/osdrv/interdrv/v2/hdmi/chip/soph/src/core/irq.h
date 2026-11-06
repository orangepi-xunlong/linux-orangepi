#ifndef _IRQ_H_
#define _IRQ_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

#define ALL_IRQ_MASK 0xff

typedef enum irq_sources {
	AUDIO_PACKETS = 1,
	OTHER_PACKETS,
	PACKETS_OVERFLOW,
	AUDIO_SAMPLER,
	PHY,
	I2C_DDC,
	CEC,
	VIDEO_PACKETIZER,
	I2C_PHY,
	AUDIO_DMA,
}irq_sources_t;

int irq_read_stat(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 *stat);

int irq_clear_source(hdmi_tx_dev_t *dev, irq_sources_t irq_source);

int irq_clear_bit(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 bit_mask);

int irq_mute_source(hdmi_tx_dev_t *dev, irq_sources_t irq_source);

int irq_unmute_source(hdmi_tx_dev_t *dev, irq_sources_t irq_source);

void irq_mute(hdmi_tx_dev_t *dev);

void irq_unmute(hdmi_tx_dev_t *dev);

void irq_clear_all(hdmi_tx_dev_t *dev);

void irq_mask_all(hdmi_tx_dev_t *dev);

int irq_unmask_bit(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 bit_mask);

int irq_mask_bit(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 bit_mask);

void irq_hpd_sense_enable(hdmi_tx_dev_t *dev);

void irq_scdc_read_request(hdmi_tx_dev_t *dev, int enable);

/**
 * Decode functions
 */
u32 read_interrupt_decode(hdmi_tx_dev_t *dev);

int decode_is_fc_stat0(u32 decode);

int decode_is_fc_stat1(u32 decode);

int decode_is_fc_stat2_vp(u32 decode);

int decode_is_as_stat0(u32 decode);

int decode_is_phy(u32 decode);

int decode_is_phy_lock(u32 decode);

int decode_is_phy_hpd(u32 decode);

int decode_is_phy_rx_s0(u32 decode);

int decode_is_phy_rx_s1(u32 decode);

int decode_is_phy_rx_s2(u32 decode);

int decode_is_phy_rx_s3(u32 decode);

int decode_is_i2c_stat0(u32 decode);

int decode_is_cec_stat0(u32 decode);


#endif	/* _IRQ_H_ */
