#ifndef _FC_H_
#define _FC_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"
#include "core/video.h"
#include "core/audio.h"

#define ACP_TX		0
#define ISRC1_TX	1
#define ISRC2_TX	2
#define SPD_TX		4
#define VSD_TX		3

#define FC_GMD_PB_SIZE	28

typedef struct fc_spd_info {
	const u8 * vname;
	u8 vlength;
	const u8 * pname;
	u8 plength;
	u8 code;
	u8 auto_send;
}fc_spd_info_t;

void fc_packets_metadata_config(hdmi_tx_dev_t *dev);

void fc_packets_auto_send(hdmi_tx_dev_t *dev, u8 enable, u8 mask);

void fc_packets_manual_send(hdmi_tx_dev_t *dev, u8 mask);

void fc_packets_disable_all(hdmi_tx_dev_t *dev);

int fc_video_config(hdmi_tx_dev_t *dev, video_params_t *video);

void fc_video_hdcp_keepout(hdmi_tx_dev_t *dev, u8 bit);

int fc_spd_config(hdmi_tx_dev_t *dev, fc_spd_info_t *spd_data);

void fc_gamut_enable_tx(hdmi_tx_dev_t *dev, u8 enable);

void fc_gamut_config(hdmi_tx_dev_t *dev);

void fc_gamut_packet_config(hdmi_tx_dev_t *dev, const u8 * gbd_content, u8 length);

void fc_force_output(hdmi_tx_dev_t *dev, int enable);

void fc_avi_config(hdmi_tx_dev_t *dev, video_params_t * video_params);

void fc_audio_config(hdmi_tx_dev_t *dev, audio_params_t * audio);

void fc_audio_mute(hdmi_tx_dev_t *dev);

void fc_audio_unmute(hdmi_tx_dev_t *dev);

void fc_audio_info_config(hdmi_tx_dev_t *dev, audio_params_t * audio);

void fc_acp_type(hdmi_tx_dev_t *dev, u8 type);

void fc_acp_type_dependent_fields(hdmi_tx_dev_t *dev, u8 * fields, u8 fields_length);
/*
 * Configure the 24 bit IEEE Registration Identifier
 * @param baseAddr Block base address
 * @param id vendor unique identifier
 */
void fc_vsd_vendor_oui(hdmi_tx_dev_t *dev, u32 id);

/*
 * Configure the Vendor Payload to be carried by the InfoFrame
 * @param info array
 * @param length of the array
 * @return 0 when successful and 1 on error
 */
u8 fc_vsd_vendor_payload(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length);

/*
 * Configure the ISRC packet status
 * @param code
 * 001 - Starting Position
 * 010 - Intermediate Position
 * 100 - Ending Position
 * @param baseAddr block base address
 */
void fc_isrc_status(hdmi_tx_dev_t *dev, u8 code);

/*
 * Configure the validity bit in the ISRC packets
 * @param validity: 1 if valid
 * @param baseAddr block base address
 */
void fc_isrc_valid(hdmi_tx_dev_t *dev, u8 validity);

/*
 * Configure the cont bit in the ISRC1 packets
 * When a subsequent ISRC2 Packet is transmitted, the ISRC_Cont field shall be set and shall be clear otherwise.
 * @param isContinued 1 when set
 * @param baseAddr block base address
 */
void fc_isrc_cont(hdmi_tx_dev_t *dev, u8 is_continued);

/*
 * Configure the ISRC 1 Codes
 * @param codes
 * @param length
 * @param baseAddr block base address
 */
void fc_isrc_isrc1_codes(hdmi_tx_dev_t *dev, u8 * codes, u8 length);

/*
 * Configure the ISRC 2 Codes
 * @param codes
 * @param length
 * @param baseAddr block base address
 */
void fc_isrc_isrc2_codes(hdmi_tx_dev_t *dev, u8 * codes, u8 length);


void csc_config(hdmi_tx_dev_t *dev, video_params_t * params,
				unsigned interpolation, unsigned decimation, unsigned color_depth);

#endif				/* HALFRAMECOMPOSERVSD_H_ */