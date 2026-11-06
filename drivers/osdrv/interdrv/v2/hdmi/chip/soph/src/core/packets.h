#ifndef _PACKETS_H_
#define _PACKETS_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"
#include "core/video.h"
#include "core/audio.h"

#define PACKET_HDMIVIDEOFORMAT_MASK	0xE0
#define PACKET_3D_STRUCTURE_MASK	0xF0
#define PACKET_3D_EXT_DATA_MASK		0xF0
#define PACKET_HDMI_VIC_MASK		0xFF

/**
 * Initialize the packets package. Reset local variables.
 * @param dev Device structure
 * @return TRUE when successful
 */
int packets_initialize(hdmi_tx_dev_t *dev);

/**
 * Configure Source Product Description, Vendor Specific and Auxiliary
 * Video InfoFrames.
 * @param dev Device structure
 * @param video  Video Parameters to set up AVI InfoFrame (and all
 * other video parameters)
 * @return TRUE when successful
 */
int packets_configure(hdmi_tx_dev_t *dev, video_params_t * video);

/**
 * Configure Audio Content Protection packets.
 * @param type Content protection type (see HDMI1.3a Section 9.3)
 * @param fields  ACP Type Dependent Fields
 * @param length of the ACP fields
 * @param auto_send Send Packets Automatically
 */
void packets_audio_content_protection(hdmi_tx_dev_t *dev, u8 type, const u8 * fields,
				    u8 length, u8 auto_send);

/**
 * Configure ISRC 1 & 2 Packets
 * @param dev Device structure
 * @param initStatus Initial status which the packets are sent with (usually starting position)
 * @param codes ISRC codes array
 * @param length of the ISRC codes array
 * @param auto_send Send ISRC Automatically
 * @note Automatic sending does not change status automatically, it does the insertion of the packets in the data
 * islands.
 */
void packets_isrc_packets(hdmi_tx_dev_t *dev, u8 init_status, const u8 * codes,
			 u8 length, u8 auto_send);

/**
 * Send/stop sending AV Mute in the General Control Packet
 * @param dev Device structure
 * @param enable (TRUE) /disable (FALSE) the AV Mute
 */
void packets_av_mute(hdmi_tx_dev_t *dev, u8 enable);

/**
 * Set ISRC status that is changing during play back depending on position (see HDMI 1.3a Section 8.8)
 * @param dev Device structure
 * @param status the ISRC status code according to position of track
 */
void packets_isrc_status(hdmi_tx_dev_t *dev, u8 status);


/**
 * Stop sending ACP packets when in auto send mode
 * @param dev Device structure
 */
void packets_stop_send_acp(hdmi_tx_dev_t *dev);

/**
 * Stop sending ISRC 1 & 2 packets when in auto send mode (ISRC 2 packets cannot be send without ISRC 1)
 * @param dev Device structure
 */
void packets_stop_send_isrc1(hdmi_tx_dev_t *dev);

/**
 * Stop sending ISRC 2 packets when in auto send mode
 * @param dev Device structure
 */
void packets_stop_send_isrc2(hdmi_tx_dev_t *dev);

/**
 * Stop sending Source Product Description InfoFrame packets when in auto send mode
 * @param dev Device structure
 */
void packets_stop_send_spd(hdmi_tx_dev_t *dev);

/**
 * Stop sending Vendor Specific InfoFrame packets when in auto send mode
 * @param dev Device structure
 */
void packets_stop_send_vsd(hdmi_tx_dev_t *dev);

/**
 * Disable all metadata packets from being sent automatically. (ISRC 1& 2, ACP, VSD and SPD)
 * @param dev Device structure
 */
void packets_disable_all_packets(hdmi_tx_dev_t *dev);

/**
 * Configure Vendor Specific InfoFrames.
 * @param dev Device structure
 * @param oui Vendor Organisational Unique Identifier 24 bit IEEE
 * Registration Identifier
 * @param payload Vendor Specific Info Payload
 * @param length of the payload array
 * @param auto_send Start send Vendor Specific InfoFrame automatically
 */
int packets_vendor_specific_info_frame(hdmi_tx_dev_t *dev, u32 oui, const u8 * payload, u8 length, u8 auto_send);

/**
 * Configure Colorimetry packets
 * @param dev Device structure
 * @param video Video information structure
 */
void packets_colorimetry_config(hdmi_tx_dev_t *dev, video_params_t * video);

u8 vp_pixel_packing_phase(hdmi_tx_dev_t *dev);

void vp_color_depth(hdmi_tx_dev_t *dev, u8 value);

void vp_pixel_packing_default_phase(hdmi_tx_dev_t *dev, u8 bit);

void vp_pixel_repetition_factor(hdmi_tx_dev_t *dev, u8 value);

void vp_ycc422_remap_size(hdmi_tx_dev_t *dev, u8 value);

void vp_output_selector(hdmi_tx_dev_t *dev, u8 value);

#endif	/* PACKETS_H_ */
