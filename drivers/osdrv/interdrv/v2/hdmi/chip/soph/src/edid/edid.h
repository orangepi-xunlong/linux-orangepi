#ifndef _EDID_H_
#define _EDID_H_

#include "core/hdmitx_dev.h"
#include "desc.h"
#include "hdmivsdb.h"
#include "data_block.h"
#include "util/util.h"

typedef enum {
	EDID_ERROR = 0, EDID_IDLE, EDID_READING, EDID_DONE
} edid_status_t;

#define EDID_LENGTH		128
#define DDC_ADDR		0x50

#define CEA_EXT			0x02
#define VTB_EXT			0x10
#define DI_EXT			0x40
#define LS_EXT			0x50
#define MI_EXT			0x60

#define EDID_DETAIL_EST_TIMINGS		0xf7
#define EDID_DETAIL_CVT_3BYTE		0xf8
#define EDID_DETAIL_COLOR_MGMT_DATA	0xf9
#define EDID_DETAIL_STD_MODES		0xfa
#define EDID_DETAIL_MONITOR_CPDATA	0xfb
#define EDID_DETAIL_MONITOR_NAME	0xfc
#define EDID_DETAIL_MONITOR_RANGE	0xfd
#define EDID_DETAIL_MONITOR_STRING	0xfe
#define EDID_DETAIL_MONITOR_SERIAL	0xff

struct est_timings {
	u8 t1;
	u8 t2;
	u8 mfg_rsvd;
} __attribute__((packed));

struct std_timing {
	u8 hsize; /* need to multiply by 8 then add 248 */
	u8 vfreq_aspect;
} __attribute__((packed));

/* If detailed data is pixel timing */
struct detailed_pixel_timing {
	u8 hactive_lo;
	u8 hblank_lo;
	u8 hactive_hblank_hi;
	u8 vactive_lo;
	u8 vblank_lo;
	u8 vactive_vblank_hi;
	u8 hsync_offset_lo;
	u8 hsync_pulse_width_lo;
	u8 vsync_offset_pulse_width_lo;
	u8 hsync_vsync_offset_pulse_width_hi;
	u8 width_mm_lo;
	u8 height_mm_lo;
	u8 width_height_mm_hi;
	u8 hborder;
	u8 vborder;
	u8 misc;
} __attribute__((packed));

/* If it's not pixel timing, it'll be one of the below */
struct detailed_data_string {
	u8 str[13];
} __attribute__((packed));

struct detailed_data_monitor_range {
	u8 min_vfreq;
	u8 max_vfreq;
	u8 min_hfreq_khz;
	u8 max_hfreq_khz;
	u8 pixel_clock_mhz; /* need to multiply by 10 */
	u8 flags;
	union {
		struct {
			u8 reserved;
			u8 hfreq_start_khz; /* need to multiply by 2 */
			u8 c; /* need to divide by 2 */
			u16 m;
			u8 k;
			u8 j; /* need to divide by 2 */
		} __attribute__((packed)) gtf2;
		struct {
			u8 version;
			u8 data1; /* high 6 bits: extra clock resolution */
			u8 data2; /* plus low 2 of above: max hactive */
			u8 supported_aspects;
			u8 flags; /* preferred aspect and blanking support */
			u8 supported_scalings;
			u8 preferred_refresh;
		} __attribute__((packed)) cvt;
	} formula;
} __attribute__((packed));

struct detailed_data_wpindex {
	u8 white_yx_lo; /* Lower 2 bits each */
	u8 white_x_hi;
	u8 white_y_hi;
	u8 gamma; /* need to divide by 100 then add 1 */
} __attribute__((packed));

struct detailed_data_color_point {
	u8 windex1;
	u8 wpindex1[3];
	u8 windex2;
	u8 wpindex2[3];
} __attribute__((packed));

struct cvt_timing {
	u8 code[3];
} __attribute__((packed));

struct detailed_non_pixel {
	u8 pad1;
	u8 type; /* ff=serial, fe=string, fd=monitor range, fc=monitor name
		    fb=color point data, fa=standard timing data,
		    f9=undefined, f8=mfg. reserved */
	u8 pad2;
	union {
		struct detailed_data_string str;
		struct detailed_data_monitor_range range;
		struct detailed_data_wpindex color;
		struct std_timing timings[6];
		struct cvt_timing cvt[4];
	} data;
} __attribute__((packed));

struct detailed_timing {
	u16 pixel_clock; /* need to multiply by 10 KHz */
	union {
		struct detailed_pixel_timing pixel_data;
		struct detailed_non_pixel other_data;
	} data;
} __attribute__((packed));

typedef struct {
	/** VIC code */
	u32 m_code;

	/** Identifies modes that ONLY can be displayed in YCC 4:2:0 */
	u8 m_limited_to_ycc420;

	/** Identifies modes that can also be displayed in YCC 4:2:0 */
	u8 m_ycc420;

	u16 m_pixel_repetition_input;

	/** In units of 1MHz */
	u32 m_pixel_clock;

	/** 1 for interlaced, 0 progressive */
	u8 m_interlaced;

	u16 m_hactive;

	u16 m_hblanking;

	u16 m_hborder;

	u16 m_himage_size;

	u16 m_hsync_offset;

	u16 m_hsync_pulse_width;

	/** 0 for Active low, 1 active high */
	u8 m_hsync_polarity;

	u16 m_vactive;

	u16 m_vblanking;

	u16 m_vborder;

	u16 m_vimage_size;

	u16 m_vsync_offset;

	u16 m_vsync_pulse_width;

	/** 0 for Active low, 1 active high */
	u8 m_vsync_polarity;

} dtd_t;

typedef struct supported_dtd{
	u32 refresh_rate;
	dtd_t dtd;
}supported_dtd_t;

struct edid {
	u8 header[8];
	/* Vendor & product info */
	u8 mfg_id[2];
	u8 prod_code[2];
	u32 serial; /* FIXME: byte order */
	u8 mfg_week;
	u8 mfg_year;
	/* EDID version */
	u8 version;
	u8 revision;
	/* Display info: */
	u8 input;
	u8 width_cm;
	u8 height_cm;
	u8 gamma;
	u8 features;
	/* Color characteristics */
	u8 red_green_lo;
	u8 black_white_lo;
	u8 red_x;
	u8 red_y;
	u8 green_x;
	u8 green_y;
	u8 blue_x;
	u8 blue_y;
	u8 white_x;
	u8 white_y;
	/* Est. timings and mfg rsvd timings*/
	struct est_timings established_timings;
	/* Standard timings 1-8*/
	struct std_timing standard_timings[8];
	/* Detailing timings 1-4 */
	struct detailed_timing detailed_timings[4];
	/* Number of 128 byte ext. blocks */
	u8 extensions;
	/* Checksum */
	u8 checksum;
} __attribute__((packed));


/* Short Audio Descriptor */
struct cea_sad {
	u8 format;
	u8 channels; /* max number of channels - 1 */
	u8 freq;
	u8 byte2; /* meaning depends on format */
};

typedef struct {
	/**
	 * Array to hold all the parsed Detailed Timing Descriptors.
	 */
	dtd_t edid_mdtd[32];

	unsigned int edid_mdtd_index;
	/**
	 * array to hold all the parsed Short Video Descriptors.
	 */
	short_video_desc_t edid_msvd[128];  //TODO: too big for stack

	short_video_desc_t tmp_svd;

	unsigned int edid_msvd_index;
	/**
	 * array to hold all the parsed Short Audio Descriptors.
	 */
	short_audio_desc_t edid_msad[128]; //TODO: too big for stack

	unsigned int edid_msad_index;
#if 1
	/**
	 * A string to hold the Monitor Name parsed from EDID.
	 */
	char edid_mmonitor_name[13];

	int edid_mycc444_support;

	int edid_mycc422_support;

	int edid_mycc420_support;

	int edid_mbasic_audio_support;

	int edid_munder_scan_support;
#endif
	/**
	 *  If Sink is HDMI 2.0
	 */
	int edid_m20_sink;

	hdmivsdb_t edid_mhdmivsdb;

	hdmiforumvsdb_t edid_mhdmi_forumvsdb;

	monitor_range_limits_t edid_mmonitor_range_limits;

	video_capability_datablock_t edid_mvideo_capability_datablock;

	colorimetry_datablock_t edid_mcolorimetry_datablock;

	speaker_allocation_datablock_t edid_mspeaker_allocation_datablock;
} edid_cea_ext_t;

/**
 * Parses the Detailed Timing Descriptor.
 * Encapsulating the parsing process
 * @param dtd pointer to dtd_t strucutute for the information to be save in
 * @param data a pointer to the 18-byte structure to be parsed.
 * @return TRUE if success
 */
int dtd_parse(hdmi_tx_dev_t *dev, dtd_t * dtd, u8 data[18]);

/**
 * @param dtd pointer to dtd_t strucutute for the information to be save in
 * @param code the CEA 861-D video code.
 * @param refreshRate the specified vertical refresh rate.
 * @return TRUE if success
 */
int dtd_fill(hdmi_tx_dev_t *dev, dtd_t * dtd, u8 code, u32 refresh_rate);

/**
 * @param dtd Pointer to the current DTD parameters
 * @return The refresh rate if DTD parameters are correct and 0 if not
 */
u32 dtd_get_refresh_rate(dtd_t *dtd);

/**
 * @param dtd Pointer to the current DTD parameters
 * @param tempDtd Pointer to the temp DTD parameters
 * @return The refresh rate if DTD parameters are correct and 0 if not
 */
void dtd_change_horiz_for_ycc420(hdmi_tx_dev_t *dev, dtd_t * temp_dtd);

dtd_t * find_dtd(u16 width, u16 height, u32 pclk);

int edid_extension_read(hdmi_tx_dev_t *dev, int block, u8 * edid_ext);

int edid_read(hdmi_tx_dev_t *dev, struct edid * edid);

#endif	/* _EDID_H_ */
