#ifndef AUDIO_H_
#define AUDIO_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"
#include "core/audio.h"

typedef enum {
	INTERFACE_NOT_DEFINED = -1, I2S = 0, SPDIF, HBR, GPA, DMA
} interface_type_t;

typedef enum {
	PACKET_NOT_DEFINED = -1, AUDIO_SAMPLE = 1, HBR_STREAM
} packet_t;

typedef enum {
	CODING_NOT_DEFINED = -1,
	PCM = 1,
	AC3,
	MPEG1,
	MP3,
	MPEG2,
	AAC,
	DTS,
	ATRAC,
	ONE_BIT_AUDIO,
	DOLBY_DIGITAL_PLUS,
	DTS_HD,
	MAT,
	DST,
	WMAPRO
} coding_type_t;

typedef enum {
	DMA_NOT_DEFINED = -1,
	DMA_4_BEAT_INCREMENT = 0,
	DMA_8_BEAT_INCREMENT,
	DMA_16_BEAT_INCREMENT,
	DMA_UNUSED_BEAT_INCREMENT,
	DMA_UNSPECIFIED_INCREMENT
} dma_increment_t;

/* Supplementary Audio type, table 8-14 HDMI 2.0 Spec. pg 79 */
typedef enum {
	RESERVED = 0,
	AUDIO_FOR_VIS_IMP_NARR,
	AUDIO_FOR_VIS_IMP_SPOKEN,
	AUDIO_FOR_HEAR_IMPAIRED,
	ADDITIONAL_AUDIO
} suppl_a_type_t;

/* Audio Metadata Packet Header, table 8-4 HDMI 2.0 Spec. pg 71 */
typedef struct {
	u8 m3d_audio;
	u8 mnum_views;
	u8 mnum_audio_streams;
} audio_metadata_header_t;

/* Audio Metadata Descriptor, table 8-13 HDMI 2.0 Spec. pg 78 */
typedef struct {
	u8 mmultiview_right_left;
	u8 mlc_valid;
	u8 msuppl_a_valid;
	u8 msuppl_a_mixed;
	suppl_a_type_t msuppl_a_type;
	u8 mlanguage_code[3];    /*ISO 639.2 alpha-3 code, examples: eng,fre,spa,por,jpn,chi */

} audio_metadata_descriptor_t;

typedef struct {
	audio_metadata_header_t maudio_metadata_header;
	audio_metadata_descriptor_t maudio_metadata_descriptor[4];
} audio_metadata_packet_t;

/**
 * For detailed handling of this structure,
 * refer to documentation of the functions
 */
typedef struct {
	bool audio_en;

	u64 start_addr;

	u64 stop_addr;

	interface_type_t minterface_type;

	coding_type_t mcoding_type; /** (audio_params_t *params, see InfoFrame) */

	int mchannel_allocation;    /** channel allocation (audio_params_t *params, see InfoFrame) */

	u8 msample_size;            /**  sample size (audio_params_t *params, 16 to 24) */

	u32 msampling_frequency;    /** sampling frequency (audio_params_t *params, kHz) */

	u8 mLevel_shift_value;      /** level shift value (audio_params_t *params, see InfoFrame) */

	u8 mdown_mix_inhibit_flag;  /** down-mix inhibit flag (audio_params_t *params, see InfoFrame) */

	u8 miec_copyright;          /** IEC copyright */

	u8 miec_cgms_a;             /** IEC CGMS-A */

	u8 miec_pcm_mode;           /** IEC PCM mode */

	u8 miec_category_code;      /** IEC category code */

	u8 miec_source_number;      /** IEC source number */

	u8 miec_clock_accuracy;     /** IEC clock accuracy */

	packet_t mpacket_type;      /** packet type. currently only Audio Sample (AUDS)
	                                and High Bit Rate (HBR) are supported */

	u16 mclock_fsfactor;        /** Input audio clock Fs factor used at the audio
	                                packetizer to calculate the CTS value and ACR packet
	                                insertion rate */

	dma_increment_t mdma_beat_increment; /** Incremental burst modes: unspecified
	                                        lengths (upper limit is 1kB boundary) an
	                                        INCR4, INCR8, and INCR16 fixed-beat burst */

	u8 mdma_threshold;          /** When the number of samples in the Audio FIFO is lower
	                                than the threshold, the DMA engine requests a new burst
	                                request to the AHB master interface */

	u8 mdma_hlock;              /** Master burst lock mechanism */

	u8 mgpa_insert_pucv;        /* discard incoming (Parity, Channel status, User bit,
	                               Valid and B bit) data and insert data configured in
	                               controller instead */
	audio_metadata_packet_t maudio_metadata_packet; /** Audio Multistream variables, to be written to the Audio Metadata Packet */
} audio_params_t;

/**
 * Initial set up of package and prepare it to be configured. Set audio mute to on.
 * @param baseAddr base Address of controller
 * @return TRUE if successful
 */
int audio_Initialize(hdmi_tx_dev_t *dev);

/**
 * Configure hardware modules corresponding to user requirements to start transmitting audio packets.
 * @param baseAddr base Address of controller
 * @param params: audio parameters
 * @param pixelClk: pixel clock [0.01 MHz]
 * @param ratioClk: ratio clock (TMDS / Pixel) [0.01]
 * @return TRUE if successful
 */
int audio_configure(hdmi_tx_dev_t *dev, audio_params_t * params);

/**
 * Mute audio.
 * Stop sending audio packets
 * @param baseAddr base Address of controller
 * @param state:  1 to enable/0 to disable the mute
 * @return TRUE if successful
 */
int audio_mute(hdmi_tx_dev_t *dev, u8 state);

u32 hdmi_compute_n(u32 freq, u32 pixel_clk);
u32 hdmi_compute_cts(u32 n, u32 pixel_clk, u32 sample_rate);

/**
 * This method reset the parameters structure to a known state
 * SPDIF 16bits 32Khz Linear PCM
 * It is recommended to call this method before setting any parameters
 * to start from a stable and known condition avoid overflows.
 * @param params pointer to the audio parameters structure
 */
void audio_reset(hdmi_tx_dev_t *dev, audio_params_t * params);

/**
 * @param params pointer to the audio parameters structure
 * @return number of audio channels transmitted -1
 */
u8 audio_channel_count(hdmi_tx_dev_t *dev, audio_params_t * params);

/**
 * @param params pointer to the audio parameters structure
 */
u8 audio_iec_original_sampling_freq(hdmi_tx_dev_t *dev, audio_params_t * params);

/**
 * @param params pointer to the audio parameters structure
 */
u8 audio_iec_sampling_freq(hdmi_tx_dev_t *dev, audio_params_t * params);

/**
 * @param params pointer to the audio parameters structure
 */
u8 audio_iec_word_length(hdmi_tx_dev_t *dev, audio_params_t * params);

/**
 * return if channel is enabled or not using the user's channel allocation
 * code
 * @param params pointer to the audio parameters structure
 * @param channel in question -1
 * @return 1 if channel is to be enabled, 0 otherwise
 */
u8 audio_is_channel_en(hdmi_tx_dev_t *dev, audio_params_t * params, u8 channel);


#endif /* AUDIO_H_ */