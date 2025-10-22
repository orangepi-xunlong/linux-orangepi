/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/

#ifndef _SUNXI_HDMI_H_
#define _SUNXI_HDMI_H_

#include <drm/drm_modes.h>

#include <linux/platform_device.h>
#include <video/drv_hdmi.h>


#include "include.h"
#include "lowlevel_hdmi20/dw_dev.h"
#include "lowlevel_hdmi20/dw_i2cm.h"
#include "lowlevel_hdmi20/dw_mc.h"
#include "lowlevel_hdmi20/dw_avp.h"
#include "lowlevel_hdmi20/dw_fc.h"
#include "lowlevel_hdmi20/dw_phy.h"
#include "lowlevel_hdmi20/dw_edid.h"
#include "lowlevel_hdmi20/dw_hdcp.h"
#include "lowlevel_hdmi20/dw_hdcp22.h"
#include "lowlevel_hdmi20/dw_cec.h"
#include "lowlevel_hdmi20/phy_aw.h"
#include "lowlevel_hdmi20/phy_inno.h"
#include "lowlevel_hdmi20/phy_snps.h"
#include "lowlevel_hdmi20/phy_top.h"

#define SUNXI_HDMI_ENABLE   			(0x1)
#define SUNXI_HDMI_DISABLE  			(0x0)

enum shdmi_dynamic_range_e {
	SHDMI_SDR    = 0,
	SHDMI_HDR10  = 1,
	SHDMI_HDR10P = 2,
	SHDMI_HLG    = 3,
	SHDMI_DV     = 4,
	SHDMI_MAX_DR,
};

enum sunxi_hdmi_update_bits {
	SUNXI_HDMI_UPDATE_FAORMAT = (0),
	SUNXI_HDMI_UPDATE_BITS    = (1),
	SUNXI_HDMI_UPDATE_EOTF    = (2),
	SUNXI_HDMI_UPDATE_SPACE   = (3),
	SUNXI_HDMI_UPDATE_DVIHDMI = (4),
	SUNXI_HDMI_UPDATE_RANGE   = (5),
	SUNXI_HDMI_UPDATE_SCAN    = (6),
	SUNXI_HDMI_UPDATE_RATIO   = (7),
};

enum sunxi_hdmi_color_capability {
	SHDMI_RGB888_8BITS    = 0,
	SHDMI_YUV444_8BITS    = 1,
	SHDMI_YUV422_8BITS    = 2,
	SHDMI_YUV420_8BITS    = 3,
	SHDMI_RGB888_10BITS   = 4,
	SHDMI_YUV444_10BITS   = 5,
	SHDMI_YUV422_10BITS   = 6,
	SHDMI_YUV420_10BITS   = 7,
	SHDMI_RGB888_12BITS   = 8,
	SHDMI_YUV444_12BITS   = 9,
	SHDMI_YUV422_12BITS   = 10,
	SHDMI_YUV420_12BITS   = 11,
	SHDMI_RGB888_16BITS   = 12,
	SHDMI_YUV444_16BITS   = 13,
	SHDMI_YUV422_16BITS   = 14,
	SHDMI_YUV420_16BITS   = 15,
	SHDMI_MAX_MASK,
};

enum sunxi_cec_irq_state_e {
	SUNXI_CEC_IRQ_NULL				= 0x0,
	SUNXI_CEC_IRQ_DONE				= IH_CEC_STAT0_DONE_MASK,
	SUNXI_CEC_IRQ_EOM				= IH_CEC_STAT0_EOM_MASK,
	SUNXI_CEC_IRQ_NACK				= IH_CEC_STAT0_NACK_MASK,
	SUNXI_CEC_IRQ_ARB				= IH_CEC_STAT0_ARB_LOST_MASK,
	SUNXI_CEC_IRQ_ERR_INITIATOR		= IH_CEC_STAT0_ERROR_INITIATOR_MASK,
	SUNXI_CEC_IRQ_ERR_FOLLOW		= IH_CEC_STAT0_ERROR_FOLLOW_MASK,
	SUNXI_CEC_IRQ_WAKEUP			= IH_CEC_STAT0_WAKEUP_MASK,
};

enum sunxi_cec_wait_bit_times_e {
	SUNXI_CEC_WAIT_3BIT				= DW_CEC_WAIT_3BIT,
	SUNXI_CEC_WAIT_5BIT				= DW_CEC_WAIT_5BIT,
	SUNXI_CEC_WAIT_7BIT				= DW_CEC_WAIT_7BIT,
	SUNXI_CEC_WAIT_NULL				= DW_CEC_WAIT_NULL,
};

enum sunxi_hdcp_state_e {
	SUNXI_HDCP_DISABLE				= DW_HDCP_DISABLE,
	SUNXI_HDCP_ING					= DW_HDCP_ING,
	SUNXI_HDCP_FAILED				= DW_HDCP_FAILED,
	SUNXI_HDCP_SUCCESS				= DW_HDCP_SUCCESS,
};

typedef enum {
	SUNXI_HDCP_TYPE_NULL			= DW_HDCP_TYPE_NULL,
	SUNXI_HDCP_TYPE_HDCP14			= DW_HDCP_TYPE_HDCP14,
	SUNXI_HDCP_TYPE_HDCP22			= DW_HDCP_TYPE_HDCP22,
} sunxi_hdcp_type_t;

#define HDMI_VIC_3D_OFFSET    (0x80)
enum HDMI_CEA_VIC {
	HDMI_VIC_NULL       = 0,
	HDMI_VIC_640x480P60 = 1,
	HDMI_VIC_720x480P60_4_3,
	HDMI_VIC_720x480P60_16_9,
	HDMI_VIC_1280x720P60,
	HDMI_VIC_1920x1080I60,
	HDMI_VIC_720x480I_4_3,
	HDMI_VIC_720x480I_16_9,
	HDMI_VIC_720x240P_4_3,
	HDMI_VIC_720x240P_16_9,
	HDMI_VIC_1920x1080P60 = 16,
	HDMI_VIC_720x576P_4_3,
	HDMI_VIC_720x576P_16_9,
	HDMI_VIC_1280x720P50,
	HDMI_VIC_1920x1080I50,
	HDMI_VIC_720x576I_4_3,
	HDMI_VIC_720x576I_16_9,
	HDMI_VIC_1920x1080P50 = 31,
	HDMI_VIC_1920x1080P24,
	HDMI_VIC_1920x1080P25,
	HDMI_VIC_1920x1080P30,
	HDMI_VIC_1280x720P24 = 60,
	HDMI_VIC_1280x720P25,
	HDMI_VIC_1280x720P30,
	HDMI_VIC_3840x2160P24 = 93,
	HDMI_VIC_3840x2160P25,
	HDMI_VIC_3840x2160P30,
	HDMI_VIC_3840x2160P50,
	HDMI_VIC_3840x2160P60,
	HDMI_VIC_4096x2160P24,
	HDMI_VIC_4096x2160P25,
	HDMI_VIC_4096x2160P30,
	HDMI_VIC_4096x2160P50,
	HDMI_VIC_4096x2160P60,
	HDMI_VIC_2560x1440P60 = 0x201,
	HDMI_VIC_1440x2560P70 = 0x202,
	HDMI_VIC_1080x1920P60 = 0x203,
	HDMI_VIC_1680x1050P60 = 0x204,
	HDMI_VIC_1600x900P60  = 0x205,
	HDMI_VIC_1280x1024P60 = 0x206,
	HDMI_VIC_1024x768P60  = 0x207,
	HDMI_VIC_800x600P60   = 0x208,
	HDMI_VIC_MAX,
};

/******************************************************************
 * @desc: sunxi hdmi level struct
 *****************************************************************/
struct sunxi_hdmi_rescal_s {
	unsigned long rescal_ctrl_pa;
	u32 bit_hdmi_res_sel;
	u32 bit_rescal_mode;
	u32 bit_cal_ana_en;
	u32 bit_cal_en;

	unsigned long res0_ctrl_pa;
	u32 res0_ctrl_bitmask;
};

struct sunxi_hdmi_plat_s {
	unsigned char version;
	unsigned char use_top_phy;
	/* resistance calibration */
	unsigned char need_res_cal;
	struct sunxi_hdmi_rescal_s  rescal_regs;

	struct dw_phy_ops_s  phy_func;
};

struct sunxi_hdmi_s {
	struct device         *dev;
	struct drm_connector  *connect;
	struct i2c_adapter    *i2c_adap;

	u8 smooth_boot;
	u8 clock_src;
	u8 resistor_src;
	uintptr_t reg_base;

	struct sunxi_hdmi_plat_s    *plat_data;
	struct disp_device_config	disp_info;
	struct dw_hdmi_dev_s    	dw_hdmi;

	struct mutex  lock_config;
};

/**
 * @desc: sunxi hdmi get color format string
 * @bits: disp define format
 * @return: format string
 */
char *sunxi_hdmi_color_format_string(enum disp_csc_type format);
/**
 * @desc: sunxi hdmi get color depth string
 * @bits: disp define data bits
 * @return: bits string
 */
char *sunxi_hdmi_color_depth_string(enum disp_data_bits bits);
/*******************************************************************************
 * sunxi hdmi core register write and read function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi contorl register write
 * @addr: write address
 * @data: write value
 */
void sunxi_hdmi_ctrl_write(uintptr_t addr, u32 data);
/**
 * @desc: sunxi hdmi contorl register read
 * @addr: read address
 * @return: read value
 */
u32 sunxi_hdmi_ctrl_read(uintptr_t addr);
/*******************************************************************************
 * sunxi hdmi core scdc write and read function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi scdc register write
 * @addr: write address
 * @data: write value
 */
void sunxi_hdmi_scdc_write(u8 addr, u8 data);
/**
 * @desc: sunxi hdmi scdc register read
 * @addr: read address
 * @return: read value
 */
u8 sunxi_hdmi_scdc_read(u8 addr);
/*******************************************************************************
 * sunxi hdmi core phy function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi phy register write
 * @addr: write address
 * @data: write value
 * @return: 0 - write success
 *         -1 - write failed
 */
int sunxi_hdmi_phy_write(u8 addr, u32 data);
/**
 * @desc: sunxi hdmi phy register read
 * @addr: read address
 * @data: point to read value
 * @return: 0 - read success
 *         -1 - read failed
 */
int sunxi_hdmi_phy_read(u8 addr, u32 *data);
/**
 * @desc: sunxi hdmi phy config
 * @return: 0 - config success
 *         -1 - config failed
 */
int sunxi_hdmi_phy_config(void);
/**
 * @desc: sunxi hdmi phy reset(Optional)
 */
void sunxi_hdmi_phy_reset(void);
/**
 * @desc: sunxi hdmi phy resume(Optional)
 * @return: 0 - resume success
 *         -1 - resume failed
 */
int sunxi_hdmi_phy_resume(void);
/**
 * @desc: sunxi hdmi i2c set mode and rate
 * @mode: i2c mode. standard or fast mode
 * @rate: i2c rate. (*100Hz)
 * @return: 0 - suaccess
 *         -1 - failed
 */
int sunxi_hdmi_i2cm_set_ddc(u32 mode, u32 rate);
/**
 * @desc: sunxi hdmi i2c master xfer, support send and receive
 * @msgs: buffer fot send or receive message
 * @num: send or receive message number
 */
int sunxi_hdmi_i2cm_xfer(struct i2c_msg *msgs, int num);
/**
 * @desc: sunxi hdmi edid parse
 * @buffer: point to edid buffer
 * @return: 0 - parse success
 *         -1 - parse failed
 */
int sunxi_hdmi_edid_parse(u8 *buffer);

/*******************************************************************************
 * sunxi hdmi core hdcp function
 ******************************************************************************/
/**
 * @desc: sunxi hdcp get sink is support hdcp1x
 * @return: 1 - sink support
 *          0 - sink unsupport
 */
int sunxi_hdcp1x_get_sink_cap(void);
/**
 * @desc: sunxi hdcp get sink is support hdcp2x
 * @return: 1 - sink support
 *          0 - sink unsupport
 */
int sunxi_hdcp2x_get_sink_cap(void);
/**
 * @desc: sunxi hdcp2x esm firmware state
 * @return: 1 - firmware valid
 *          0 - firmware invalid
 */
int sunxi_hdcp2x_fw_state(void);
/**
 * @desc: sunxi hdcp2x loading firmware
 * @data: point to firmware data buffer
 * @size: firmware size
 * @return: 0 - loading success
 *         -1 - loading failed
 */
int sunxi_hdcp2x_fw_loading(const u8 *data, size_t size);
/**
 * @desc: sunxi hdcp1x config
 * @state: 1 - enable hdcp1x
 *         0 - disable hdcp 1x
 * @return: 0 - enable/disable success
 *         -1 - enable/disable failed
 */
int sunxi_hdcp1x_config(u8 state);
/**
 * @desc: sunxi hdcp2x config
 * @state: 1 - enable hdcp1x
 *         0 - disable hdcp 1x
 * @return: 0 - enable/disable success
 *         -1 - enable/disable failed
 */
int sunxi_hdcp2x_config(u8 state);
/**
 * @desc: sunxi hdcp state
 * @return: SUNXI_HDCP_DISABLE - disable state
 *          SUNXI_HDCP_ING     - encrypting
 *          SUNXI_HDCP_FAILED  - encry failed
 *          SUNXI_HDCP_SUCCESS - encry success
 */
u8 sunxi_hdcp_get_state(void);
void sunxi_hdcp_set_path(u8 path);

/*******************************************************************************
 * sunxi hdmi core cec function
 ******************************************************************************/
/**
 * @desc: sunxi cec message dump
 * @msg: point to message buffer
 * @len: message length
 */
void sunxi_cec_msg_dump(u8 *msg, u8 len);
/**
 * @desc: sunxi cec enable/disable
 * @state: 1 - enable
 *         0 - disable
 */
void sunxi_cec_enable(u8 state);
/**
 * @desc: sunxi cec receive message
 * @buf: point to message buffer
 * @return: -1 - failed
 *       other - receive message length
 */
int sunxi_cec_message_receive(u8 *buf);
/**
 * @desc: sunxi cec send message
 * @buf: point to send message buffer
 * @len: message length
 * @times: send retry count
 */
void sunxi_cec_message_send(u8 *buf, u8 len, u8 times);
/**
 * @desc: sunxi cec config logical address
 * @addr: logical address
 */
void sunxi_cec_set_logic_addr(u16 addr);
/**
 * @desc: sunxi cec irq state
 * @return: irq state
 */
u8 sunxi_cec_get_irq_state(void);
/*******************************************************************************
 * sunxi hdmi core audio function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi audio set info params
 * @snd_param: sunxi snd params
 * @return: 0 - set info success
 *         -1 - set info failed
 */
int sunxi_hdmi_audio_set_info(hdmi_audio_t *snd_param);
/**
 * @desc: sunxi hdmi audio enable
 * @return: 0 - enable success
 *         -1 - enable failed
 */
int sunxi_hdmi_audio_enable(void);
/*******************************************************************************
 * sunxi hdmi core video info function
 ******************************************************************************/
u32 sunxi_hdmi_get_support_hdr_mode(void);
/**
 * @desc: sunxi hdmi get useing disp info
 * @return: disp info struct
 */
struct disp_device_config *sunxi_hdmi_get_disp_info(void);
/**
 * @desc: check sink is support current eotf
 * @info: point to config
 */
int sunxi_hdmi_disp_select_eotf(struct disp_device_config *info);
/**
 * @desc: check sink is support current color space
 * @info: point to config
 * @vic_code: vic code
 */
int sunxi_hdmi_disp_select_space(struct disp_device_config *info, u32 vic_code);
/**
 * @desc: check sink is support current color format
 * @info: point to config
 * @vic_code: vic code
 */
int sunxi_hdmi_disp_select_format(struct disp_device_config *info, u32 vic_code);
/**
 * @desc: check current format,bits is sink support
 * @format: current use format
 * @bits: current use bits
 * @pixel_clk: current timing pixel clock
 */
int sunxi_hdmi_video_check_tmds_clock(u8 format, u8 bits, u32 pixel_clk);
/**
 * @desc: sunxi hdmi video set disp info
 * @disp_param: disp info
 */
int sunxi_hdmi_set_disp_info(struct disp_device_config *disp_param);
/**
 * @desc: sunxi hdmi video force patterm
 * @bit: force enable
 * @value: force value
 */
void sunxi_hdmi_video_set_pattern(u8 bit, u32 value);
/**
 * @desc: sunxi hdmi update sink color capality
 * @vic: current timing vic
 * @return: current timing support vic
 */
u32 sunxi_hdmi_get_color_capality(u32 vic);
/**
 * @desc: sunxi hdmi update send perfer video infoframe
 */
void sunxi_hdmi_select_output_packets(u8 flags);
/**
 * @desc: sunxi hdmi get hpd state
 * @return: 1 - hight
 *          0 - low
 */
u8 sunxi_hdmi_get_hpd(void);
/**
 * @desc: sunxi hdmi get lowlevel loglevel
 * @return: log level
 */
u8 sunxi_hdmi_get_loglevel(void);
/**
 * @desc: sunxi hdmi set log level
 * @level: log level
 */
int suxni_hdmi_set_loglevel(u8 level);
/**
 * @desc: sunxi hdmi disconfig
 * @return: 0 - success
 */
int sunxi_hdmi_disconfig(void);
/**
 * @desc: sunxi hdmi config
 * @return: 0 - success
 */
int sunxi_hdmi_config(void);
int sunxi_hdmi_smooth_config(void);
/**
 * @desc: sunxi hdmi mode set.
 * @mode: set mode
 * @return: 0 - success
 *         -1 - failed
 */
int sunxi_hdmi_set_disp_mode(struct drm_display_mode *mode);
/**
 * @desc: sunxi hdmi init
 * @hdmi: sunxi hdmi point
 * @return: 0 - init success
 */
int sunxi_hdmi_init(struct sunxi_hdmi_s *hdmi);
/**
 * @desc: sunxi hdmi exit
 */
void sunxi_hdmi_exit(void);
/**
 * @desc: sunxi hdmi tx info dump
 * @buf: point to save info buffer
 * @return: save buffer size
 */
ssize_t sunxi_hdmi_tx_dump(char *buf);
/**
 * @desc: sunxi hdmi rx info dump
 * @buf: point to save info buffer
 * @return: save buffer size
 */
ssize_t sunxi_hdmi_rx_dump(char *buf);

#endif /* _SUNXI_HDMI_H_ */
