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
#include <linux/bitops.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/io.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)
#include <drm/drm_hdcp.h>
#include <drm/drm_scdc_helper.h>
#else
#include <drm/display/drm_hdcp.h>
#include <drm/display/drm_hdmi_helper.h>
#include <drm/display/drm_scdc_helper.h>
#endif

#include "sunxi_hdmi.h"

struct sunxi_hdmi_vic_mode {
	char name[25];
	int  vic_code;
};

struct sunxi_hdmi_plat_s sun8i_hdmi = {
	.version = HDMI_SUN8I_W20_P1,
	.use_top_phy            = 0x0,
	.phy_func.phy_init      = aw_phy_init,
	.phy_func.phy_config    = aw_phy_config,
	.phy_func.phy_resume    = aw_phy_resume,
	.phy_func.phy_reset     = aw_phy_reset,
	.phy_func.phy_read      = aw_phy_read,
	.phy_func.phy_write     = aw_phy_write,
};

struct sunxi_hdmi_plat_s sun50i_hdmi = {
	.version = HDMI_SUN50I_W9_P1,
	.use_top_phy            = 0x0,
	.phy_func.phy_init      = snps_phy_init,
	.phy_func.phy_config    = snps_phy_config,
	.phy_func.phy_disconfig = snps_phy_disconfig,
	.phy_func.phy_read      = snps_phy_read,
	.phy_func.phy_write     = snps_phy_write,
	.phy_func.phy_dump      = snps_phy_dump,
};

struct sunxi_hdmi_plat_s sun55i_hdmi = {
	.version = HDMI_SUN55I_W3_P1,
	.use_top_phy            = 0x0,
	.phy_func.phy_init      = inno_phy_init,
	.phy_func.phy_config    = inno_phy_config,
	.phy_func.phy_read      = inno_phy_read,
	.phy_func.phy_write     = inno_phy_write,
	.phy_func.phy_dump      = inno_phy_dump,
};

struct sunxi_hdmi_plat_s sun60i_hdmi = {
	.version = HDMI_SUN60I_W2_P1,
	.use_top_phy            = SUNXI_HDMI_ENABLE,
	.phy_func.phy_init      = snps_phy_init,
	.phy_func.phy_config    = snps_phy_config,
	.phy_func.phy_disconfig = snps_phy_disconfig,
	.phy_func.phy_read      = snps_phy_read,
	.phy_func.phy_write     = snps_phy_write,
	.phy_func.phy_dump      = snps_phy_dump,

	/* config sun60i res cal and register and bitmask */
	.need_res_cal        = SUNXI_HDMI_ENABLE,
	.rescal_regs.rescal_ctrl_pa   = 0x03000160,
	.rescal_regs.bit_hdmi_res_sel = (u32)BIT(12),
	.rescal_regs.bit_rescal_mode  = (u32)BIT(2),
	.rescal_regs.bit_cal_ana_en   = (u32)BIT(1),
	.rescal_regs.bit_cal_en       = (u32)BIT(0),
	.rescal_regs.res0_ctrl_pa        = 0x03000164,
	.rescal_regs.res0_ctrl_bitmask   = (u32)GENMASK(31, 24),
};

struct sunxi_hdmi_s      *sunxi_hdmi;

static int _shdmi_is_use_hfvsif(u32 vic)
{
	switch (vic) {
	case HDMI_VIC_3840x2160P50:
	case HDMI_VIC_3840x2160P60:
	case HDMI_VIC_4096x2160P50:
	case HDMI_VIC_4096x2160P60:
		return 1;
	default:
		return 0;
	}
}

char *sunxi_hdmi_color_format_string(enum disp_csc_type format)
{
	char *string[] = {"rgb", "yuv444", "yuv422", "yuv420"};

	return string[format];
}

char *sunxi_hdmi_color_depth_string(enum disp_data_bits bits)
{
	char *string[] = {"8bits", "10bits", "12bits", "16bits"};

	return string[bits];
}

/*******************************************************************************
 * sunxi hdmi core register write and read function
 ******************************************************************************/
void sunxi_hdmi_ctrl_write(uintptr_t addr, u32 data)
{
	dw_write(addr, data);
}

u32 sunxi_hdmi_ctrl_read(uintptr_t addr)
{
	return (u32)dw_read(addr);
}

/*******************************************************************************
 * sunxi hdmi core scdc write and read function
 ******************************************************************************/
void sunxi_hdmi_scdc_write(u8 addr, u8 data)
{
	int ret = 0;

	ret = drm_scdc_writeb(sunxi_hdmi->connect->ddc, addr, data);
	if (ret != 0)
		hdmi_wrn("hdmi scdc byte write 0x%x = 0x%x failed\n", addr, data);
}

u8 sunxi_hdmi_scdc_read(u8 addr)
{
	u8 data = 0;
	int ret = 0;

	ret = drm_scdc_readb(sunxi_hdmi->connect->ddc, addr, &data);
	if (ret != 0) {
		hdmi_wrn("hdmi scdc byte read 0x%x failed\n", addr);
		data = 0;
	}

	return data;
}
/*******************************************************************************
 * sunxi hdmi core phy function
 ******************************************************************************/
int sunxi_hdmi_phy_write(u8 addr, u32 data)
{
	int ret = 0;

	if (sunxi_hdmi->dw_hdmi.phy_ext->phy_write)
		ret = sunxi_hdmi->dw_hdmi.phy_ext->phy_write(addr, (void *)&data);

	return ret;
}

int sunxi_hdmi_phy_read(u8 addr, u32 *data)
{
	int ret = 0;
	if (sunxi_hdmi->dw_hdmi.phy_ext->phy_read)
		ret = sunxi_hdmi->dw_hdmi.phy_ext->phy_read(addr, (void *)data);
	return ret;
}

int sunxi_hdmi_phy_config(void)
{
	if (sunxi_hdmi->dw_hdmi.phy_ext->phy_config)
		sunxi_hdmi->dw_hdmi.phy_ext->phy_config();
	return 0;
}

void sunxi_hdmi_phy_reset(void)
{
	if (sunxi_hdmi->dw_hdmi.phy_ext->phy_reset)
		sunxi_hdmi->dw_hdmi.phy_ext->phy_reset();
}

int sunxi_hdmi_phy_resume(void)
{
	if (sunxi_hdmi->dw_hdmi.phy_ext->phy_resume)
		return sunxi_hdmi->dw_hdmi.phy_ext->phy_resume();
	return 0;
}

int sunxi_hdmi_i2cm_set_ddc(u32 mode, u32 rate)
{
	return dw_i2cm_set_ddc(mode, rate);
}

int sunxi_hdmi_i2cm_xfer(struct i2c_msg *msgs, int num)
{
	return dw_i2cm_xfer(msgs, num);
}

int sunxi_hdmi_edid_parse(u8 *buffer)
{
	u8 temp_edid[EDID_BLOCK_SIZE] = {0x0};
	int edid_ext_cnt = 0, i = 0, ret = 0;

	if (IS_ERR_OR_NULL(buffer)) {
		shdmi_err(buffer);
		return -1;
	}

	memcpy(temp_edid, buffer, EDID_BLOCK_SIZE);

	dw_edid_reset_sink();

	ret = dw_edid_parse_info((u8 *)temp_edid);
	if (ret != 0) {
		hdmi_err("hdmi edid parse block0 failed\n");
		return -1;
	}
	hdmi_inf("sunxi hdmi edid parse block0 finish\n");
	edid_ext_cnt = temp_edid[126];

	if (edid_ext_cnt == 0x0) {
		hdmi_inf("hdmi edid only has block0 and parse finish\n");
		return 0;
	}

	for (i = 0; i < edid_ext_cnt; i++) {
		memcpy(temp_edid, buffer + (EDID_BLOCK_SIZE * (i + 1)), EDID_BLOCK_SIZE);
		ret = dw_edid_parse_info((u8 *)temp_edid);
		if (ret != 0) {
			hdmi_err("hdmi edid parse block%d failed\n", i + 1);
			continue;
		}
		hdmi_inf("sunxi hdmi edid parse block%d finish\n", i + 1);
	}

	return 0;
}

/*******************************************************************************
 * sunxi hdmi core hdcp function
 ******************************************************************************/
int sunxi_hdcp1x_get_sink_cap(void)
{
	int ret = 0;
	u8 offset = DRM_HDCP_DDC_BCAPS & 0xff, data = 0x0;
	struct i2c_msg msgs[2];

	msgs[0].addr  = DRM_HDCP_DDC_ADDR;
	msgs[0].flags = 0;
	msgs[0].len   = 0x1;
	msgs[0].buf   = &offset;

	msgs[1].addr  = DRM_HDCP_DDC_ADDR;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len   = 0x1;
	msgs[1].buf   = &data;

	ret = i2c_transfer(sunxi_hdmi->i2c_adap, msgs, ARRAY_SIZE(msgs));
	if (ret == ARRAY_SIZE(msgs)) {
		hdmi_inf("sunxi hdmi get sink support hdcp1x\n");
		return 1;
	}

	hdmi_inf("sunxi hdmi get sink unsupport hdcp1x\n");
	return 0;
}

int sunxi_hdcp2x_get_sink_cap(void)
{
	int ret = 0x0;
	u8 offset = (HDCP_2_2_HDMI_REG_VER_OFFSET & 0xff), data = 0x0;
	struct i2c_msg msgs[2];

	msgs[0].addr  = DRM_HDCP_DDC_ADDR;
	msgs[0].flags = 0;
	msgs[0].len   = 0x1;
	msgs[0].buf   = &offset;

	msgs[1].addr  = DRM_HDCP_DDC_ADDR;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len   = 0x1;
	msgs[1].buf   = &data;

	ret = i2c_transfer(sunxi_hdmi->i2c_adap, msgs, ARRAY_SIZE(msgs));
	if (ret == ARRAY_SIZE(msgs)) {
		if (data & HDCP_2_2_HDMI_SUPPORT_MASK) {
			hdmi_inf("sunxi hdmi get sink support hdcp2x\n");
			return 1;
		}
	}

	hdmi_inf("sunxi hdmi get sink unsupport hdcp2x\n");
	return 0;
}

int sunxi_hdcp2x_fw_state(void)
{
	return dw_hdcp2x_firmware_state();
}

int sunxi_hdcp2x_fw_loading(const u8 *data, size_t size)
{
	return dw_hdcp2x_firmware_update(data, size);
}

int sunxi_hdcp1x_config(u8 state)
{
	if (state)
		return dw_hdcp1x_enable();
	else
		return dw_hdcp1x_disable();
}

int sunxi_hdcp2x_config(u8 state)
{
	if (state)
		return dw_hdcp2x_enable();
	else
		return dw_hdcp2x_disable();
}

u8 sunxi_hdcp_get_state(void)
{
	int ret = 0;

	ret = dw_hdcp_get_state();

	if (ret == DW_HDCP_ING)
		return SUNXI_HDCP_ING;

	if (ret == DW_HDCP_SUCCESS)
		return SUNXI_HDCP_SUCCESS;

	if (ret == DW_HDCP_FAILED)
		return SUNXI_HDCP_FAILED;

	return SUNXI_HDCP_DISABLE;
}

void sunxi_hdcp_set_path(u8 path)
{
	dw_hdcp2x_ovr_set_path(path, DW_HDMI_ENABLE);
}

/*******************************************************************************
 * sunxi hdmi core cec function
 ******************************************************************************/
void sunxi_cec_msg_dump(u8 *msg, u8 len)
{
	char buf[256];
	int n = 0, i = 0;

	n += sprintf(buf + n, "[msg]: ");
	for (i = 0; (i < len) && (n < 200); i++)
		n += sprintf(buf + n, "0x%02x ", *(msg + i));

	hdmi_trace("%s\n", buf);
}

void sunxi_cec_enable(u8 state)
{
	dw_cec_set_enable(state == SUNXI_HDMI_ENABLE ? 0x1 : 0x0);
}

int sunxi_cec_message_receive(u8 *buf)
{
	int size = sizeof(buf);

	return dw_cec_receive_msg(buf, size);
}

void sunxi_cec_message_send(u8 *buf, u8 len, u8 times)
{
	u8 type = SUNXI_CEC_WAIT_NULL;

	switch (times) {
	case SUNXI_CEC_WAIT_3BIT:
		type = DW_CEC_WAIT_3BIT;
		break;
	case SUNXI_CEC_WAIT_5BIT:
		type = DW_CEC_WAIT_5BIT;
		break;
	case SUNXI_CEC_WAIT_7BIT:
		type = DW_CEC_WAIT_7BIT;
		break;
	default:
		type = DW_CEC_WAIT_NULL;
		break;
	}
	sunxi_cec_msg_dump(buf, len);
	dw_cec_send_msg(buf, len, type);
}

void sunxi_cec_set_logic_addr(u16 addr)
{
	dw_cec_set_logical_addr(addr);
}

u8 sunxi_cec_get_irq_state(void)
{
	u8 state = dw_mc_irq_get_state(DW_MC_IRQ_CEC);

	if (!state)
		return SUNXI_CEC_IRQ_NULL;

	dw_mc_irq_clear_state(DW_MC_IRQ_CEC, state);

	switch (state) {
	case IH_CEC_STAT0_DONE_MASK:
	  return SUNXI_CEC_IRQ_DONE;
	case IH_CEC_STAT0_EOM_MASK:
	  return SUNXI_CEC_IRQ_EOM;
	case IH_CEC_STAT0_NACK_MASK:
	  return SUNXI_CEC_IRQ_NACK;
	case IH_CEC_STAT0_ARB_LOST_MASK:
	  return SUNXI_CEC_IRQ_ARB;
	case IH_CEC_STAT0_ERROR_INITIATOR_MASK:
	  return SUNXI_CEC_IRQ_ERR_INITIATOR;
	case IH_CEC_STAT0_ERROR_FOLLOW_MASK:
	  return SUNXI_CEC_IRQ_ERR_FOLLOW;
	case IH_CEC_STAT0_WAKEUP_MASK:
	  return SUNXI_CEC_IRQ_WAKEUP;
	default:
	  return SUNXI_CEC_IRQ_NULL;
	}
}
/*******************************************************************************
 * sunxi hdmi core audio function
 ******************************************************************************/
int sunxi_hdmi_audio_set_info(hdmi_audio_t *info)
{
	int ret = 0;
	struct dw_audio_s data;

	if (IS_ERR_OR_NULL(info)) {
		shdmi_err(info);
		return -1;
	}

	data.mInterfaceType     = info->hw_intf;
	data.mCodingType        = info->data_raw;
	data.mSamplingFrequency = info->sample_rate;
	data.mChannelAllocation = info->ca;
	data.mChannelNum        = info->channel_num;
	data.mSampleSize        = info->sample_bit;

	ret = dw_audio_set_info((void *)&data);
	if (ret != 0) {
		hdmi_err("sunxi hdmi update audio param failed\n");
		return -1;
	}

	/* work, if set info, we will need reset fifo and reset i2s */
	return 0;
}

int sunxi_hdmi_audio_enable(void)
{
	int ret = 0;
	mutex_lock(&sunxi_hdmi->lock_config);
	ret = dw_audio_on();
	mutex_unlock(&sunxi_hdmi->lock_config);
	return ret;
}

/*******************************************************************************
 * sunxi hdmi core video info function
 ******************************************************************************/
u32 sunxi_hdmi_get_support_hdr_mode(void)
{
	u32 mode = 0;

	if (dw_sink_support_sdr())
		mode |= BIT(SHDMI_SDR);

	if (dw_sink_support_hdr10())
		mode |= BIT(SHDMI_HDR10);

	if (dw_sink_support_hlg())
		mode |= BIT(SHDMI_HLG);
	return mode;
}

int sunxi_hdmi_disp_select_eotf(struct disp_device_config *info)
{
	if (IS_ERR_OR_NULL(info)) {
		shdmi_err(info);
		return -1;
	}

	if (info->eotf == DISP_EOTF_GAMMA22) {
		hdmi_trace("hdmi check continue use sdr eotf\n");
		return 0;
	} else if (info->eotf == DISP_EOTF_SMPTE2084) {
		if (dw_sink_support_hdr10()) {
			hdmi_trace("hdmi check continue use hdr10 eotf\n");
			return 0;
		}
		hdmi_inf("hdmi check sink unsupport hdr10. switch to sdr\n");
		goto switch_hdr;
	} else if (info->eotf == DISP_EOTF_ARIB_STD_B67) {
		if (dw_sink_support_hlg()) {
			hdmi_trace("hdmi check continue use hlg eotf\n");
			return 0;
		}
		hdmi_inf("hdmi check sink unsupport hlg. switch to sdr\n");
		goto switch_hdr;
	}

	hdmi_inf("hdmi check drv unsupport eotf %d. default use sdr\n", info->eotf);

switch_hdr:
	info->eotf = DISP_EOTF_GAMMA22;
	return 1;
}

int sunxi_hdmi_disp_select_space(struct disp_device_config *info, u32 vic_code)
{
	if ((info->eotf == DISP_EOTF_SMPTE2084) ||
			(info->eotf == DISP_EOTF_ARIB_STD_B67)) {
		info->cs = DISP_BT2020NC;
		hdmi_trace("hdmi check is hdr or hlg. prefer select BT2020NC space\n");
		return 0;
	}

	if (info->eotf == DISP_EOTF_GAMMA22) {
		switch (vic_code) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 6:
		case 7:
		case 17:
		case 18:
			info->cs = DISP_BT601;
			hdmi_trace("hdmi check use BT601\n");
			return 0;
		default:
			info->cs = DISP_BT709;
			hdmi_trace("hdmi check use BT709\n");
			return 0;
		}
	}

	hdmi_inf("hdmi check eotf %d unsupport, default use BT709\n", info->eotf);
	info->cs = DISP_BT709;
	return 1;
}

int sunxi_hdmi_disp_select_format(struct disp_device_config *info, u32 vic_code)
{
	int ret = 0;

	/* if this timing only support 420, we need select 420-8bit */
	ret = dw_sink_support_only_yuv420(vic_code);
	if (ret) {
		info->format = DISP_CSC_TYPE_YUV420;
		if (info->bits != DISP_DATA_8BITS) {
			if (!dw_sink_support_yuv420_dc(info->bits)) {
				hdmi_inf("hdmi check vic %d yuv420-%s unsupport and switch to yuv420-8bits\n",
						vic_code, sunxi_hdmi_color_depth_string(info->bits));
				info->bits = DISP_DATA_8BITS;
			}
		}

		hdmi_inf("hdmi check vic %d only support 420-%s\n",
				vic_code, sunxi_hdmi_color_depth_string(info->bits));
		return 0;
	}

	/* if hdr mode. perfer use yuv422-10bits */
	if ((info->eotf == DISP_EOTF_SMPTE2084) ||
			(info->eotf == DISP_EOTF_ARIB_STD_B67)) {
		info->format = DISP_CSC_TYPE_YUV422;
		info->bits   = DISP_DATA_10BITS;
		hdmi_inf("hdmi check is hdr mode. default use yuv422-10bits\n");
		return 0;
	}

	/* if current format support, not-change return. */
	switch (info->format) {
	case DISP_CSC_TYPE_YUV420:
		if (dw_sink_support_yuv420(vic_code)) {
			if (info->bits == DISP_DATA_8BITS ||
					dw_sink_support_yuv420_dc((u8)info->bits))
				hdmi_trace("hdmi check continue use yuv420-%s\n",
					sunxi_hdmi_color_depth_string(info->bits));
				return 0;
		}
		break;
	case DISP_CSC_TYPE_YUV422:
		if (dw_sink_support_yuv422()) {
			if (info->bits == DISP_DATA_8BITS ||
					dw_sink_support_yuv422_dc(info->bits)) {
				hdmi_trace("hdmi check continue use yuv422-%s\n",
					sunxi_hdmi_color_depth_string(info->bits));
				return 0;
			}
		}
		break;
	case DISP_CSC_TYPE_YUV444:
		if (dw_sink_support_yuv444()) {
			if (info->bits == DISP_DATA_8BITS ||
					dw_sink_support_yuv444_dc(info->bits)) {
				hdmi_trace("hdmi check continue use yuv444-%s\n",
					sunxi_hdmi_color_depth_string(info->bits));
				return 0;
			}
		}
		break;
	default:
		info->format = DISP_CSC_TYPE_RGB;
		if (info->bits == DISP_DATA_8BITS ||
				dw_sink_support_rgb_dc(info->bits)) {
			hdmi_trace("hdmi check continue use rgb-%s\n",
					sunxi_hdmi_color_depth_string(info->bits));
			return 0;
		}
		break;
	}

	/* if format unsupport, select perfer format */
	ret = dw_sink_support_yuv444();
	info->format = (ret == 0x1) ? DISP_CSC_TYPE_YUV444 : DISP_CSC_TYPE_RGB;
	info->bits = DISP_DATA_8BITS;
	hdmi_inf("hdmi check switch use %s-8bits\n",
			sunxi_hdmi_color_format_string(info->format));

	return 1;
}

int sunxi_hdmi_video_check_tmds_clock(u8 format, u8 bits, u32 pixel_clk)
{
	u32 tmds_clk = pixel_clk, ref_clk = pixel_clk;

	if (format == DISP_CSC_TYPE_YUV422)
		goto check_clk;

	if (format == DISP_CSC_TYPE_YUV420)
		ref_clk /= 2;

	switch (bits) {
	case DISP_DATA_10BITS:
		tmds_clk = (ref_clk * 125 / 100);
		break;
	case DISP_DATA_12BITS:
		tmds_clk = (ref_clk * 3 / 2);
		break;
	default:
		tmds_clk = ref_clk;
		break;
	}

check_clk:
	return dw_sink_support_max_tmdsclk(tmds_clk);
}

struct disp_device_config *sunxi_hdmi_get_disp_info(void)
{
	struct disp_device_config *info = &sunxi_hdmi->disp_info;

	return info;
}

int sunxi_hdmi_set_disp_info(struct disp_device_config *info)
{
	u8 data_bit = 0;
	struct disp_device_config *save_info = sunxi_hdmi_get_disp_info();

	/* set encoding mode */
	dw_video_update_color_format((dw_color_format_t)info->format);

	/* set data bits */
	if (info->bits == DISP_DATA_16BITS)
		data_bit = 16;
	else
		data_bit = 8 + (2 * info->bits);
	dw_video_update_color_depth(data_bit);

	if (info->eotf == DISP_EOTF_SMPTE2084)
		dw_video_update_hdr_eotf(DW_EOTF_SMPTE2084);
	else if (info->eotf == DISP_EOTF_ARIB_STD_B67)
		dw_video_update_hdr_eotf(DW_EOTF_HLG);
	else
		dw_video_update_hdr_eotf(DW_EOTF_SDR);

	/* set color space */
	if (info->cs == DISP_BT2020NC)
		dw_video_update_color_metry(DW_METRY_EXTENDED, DW_METRY_EXT_BT2020_Y_CB_CR);
	else if (info->cs == DISP_BT709)
		dw_video_update_color_metry(DW_METRY_ITU709, DW_METRY_EXT_XV_YCC601);
	else
		dw_video_update_color_metry(DW_METRY_ITU601, DW_METRY_EXT_XV_YCC601);

	/* set clor range: defult/limited/full */
	if (info->range == DISP_COLOR_RANGE_0_255)
		dw_video_update_range(DW_RGB_RANGE_FULL);
	else if (info->range == DISP_COLOR_RANGE_16_235)
		dw_video_update_range(DW_RGB_RANGE_LIMIT);
	else
		dw_video_update_range(DW_RGB_RANGE_DEFAULT);

	/* set output mode: hdmi or dvi */
	dw_video_update_tmds_mode((info->dvi_hdmi == DISP_DVI) ?
			DW_TMDS_MODE_DVI : DW_TMDS_MODE_HDMI);

	/* set scan info */
	dw_video_update_scaninfo(info->scan);

	/* set aspect ratio */
	dw_video_update_ratio(info->aspect_ratio ? info->aspect_ratio : 0x8);

	dw_video_dump_disp_info();

	/* save current config info */
	memcpy(save_info, info, sizeof(struct disp_device_config));

	return 0;
}

void sunxi_hdmi_video_set_pattern(u8 bit, u32 value)
{
	dw_fc_video_force_value(bit, value);
}

u32 sunxi_hdmi_get_color_capality(u32 vic)
{
	u32 value = 0x0;

	/* RGB-Bits default support */
	value |= BIT(SHDMI_RGB888_8BITS);

	/* Check RGB DeepColor */
	if (dw_sink_support_rgb_dc(DISP_DATA_10BITS))
		value |= BIT(SHDMI_RGB888_10BITS);
	if (dw_sink_support_rgb_dc(DISP_DATA_12BITS))
		value |= BIT(SHDMI_RGB888_12BITS);
	if (dw_sink_support_rgb_dc(DISP_DATA_16BITS))
		value |= BIT(SHDMI_RGB888_16BITS);

	/* Check YUV444 DeepColor */
	if (dw_sink_support_yuv444()) {
		value |= BIT(SHDMI_YUV444_8BITS);
		if (dw_sink_support_yuv444_dc(DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV444_10BITS);
		if (dw_sink_support_yuv444_dc(DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV444_12BITS);
		if (dw_sink_support_yuv444_dc(DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV444_16BITS);
	}

	/* check yuv422 format and bits */
	if (dw_sink_support_yuv422()) {
		value |= BIT(SHDMI_YUV422_8BITS);
		if (dw_sink_support_yuv422_dc(DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV422_10BITS);
		if (dw_sink_support_yuv422_dc(DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV422_12BITS);
		if (dw_sink_support_yuv422_dc(DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV422_16BITS);
	}

	/* check yuv420 format and bits */
	if (dw_sink_support_only_yuv420(vic)) {
		value = BIT(SHDMI_YUV420_8BITS);
		if (dw_sink_support_yuv420_dc(DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV420_10BITS);
		if (dw_sink_support_yuv420_dc(DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV420_12BITS);
		if (dw_sink_support_yuv420_dc(DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV420_16BITS);
		goto exit_update;
	}

	if (dw_sink_support_yuv420(vic)) {
		value |= BIT(SHDMI_YUV420_8BITS);
		if (dw_sink_support_yuv420_dc(DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV420_10BITS);
		if (dw_sink_support_yuv420_dc(DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV420_12BITS);
		if (dw_sink_support_yuv420_dc(DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV420_16BITS);
	}

exit_update:
	hdmi_trace("sunxi hdmi get color capality: 0x%x\n", value);
	return value;
}

void sunxi_hdmi_select_output_packets(u8 flags)
{
	int ret = 0, hdmi_vic = 0;
	u32 dtd_code = dw_video_get_cea_vic();

	/* check is 3d flags */
	if ((flags & DRM_MODE_FLAG_3D_MASK) == DRM_MODE_FLAG_3D_FRAME_PACKING) {
		dw_video_set_vic_format(DW_VIDEO_FORMAT_3D, dtd_code);
		dw_video_use_hdmi14_vsif(DW_VIDEO_FORMAT_3D, 0x0);
		hdmi_inf("sunxi hdmi select vic %d 3d format struct\n", dtd_code);
		return;
	}

	hdmi_vic = dw_video_cea_to_hdmi_vic(dtd_code);
	if (hdmi_vic > 0) {
		dw_video_set_vic_format(DW_VIDEO_FORMAT_HDMI14_4K, hdmi_vic);
		dw_video_use_hdmi14_vsif(DW_VIDEO_FORMAT_HDMI14_4K, hdmi_vic);
		hdmi_inf("sunxi hdmi select vic %d to hdmi vic %d\n", dtd_code, hdmi_vic);
		return;
	}

	dw_video_set_vic_format(DW_VIDEO_FORMAT_NONE, dtd_code);
	/* select config hdmi20 vsif */
	ret = _shdmi_is_use_hfvsif(dtd_code);
	if (ret == 0x1)
		dw_video_use_hdmi20_vsif();
	else
		dw_video_use_hdmi14_vsif(DW_VIDEO_FORMAT_NONE, 0x0);

	hdmi_inf("sunxi hdmi select vic %d use %s vsif\n",
			dtd_code, ret ? "hdmi20" : "hdmi14");
}

u8 sunxi_hdmi_get_hpd(void)
{
	return dw_phy_get_hpd();
}

u8 sunxi_hdmi_get_loglevel(void)
{
	return dw_hdmi_get_loglevel();
}

int suxni_hdmi_set_loglevel(u8 level)
{
	dw_hdmi_set_loglevel(level);
	return 0;
}

/* first disable output, send mute */
int sunxi_hdmi_disconfig(void)
{
	struct disp_device_config *info = &sunxi_hdmi->disp_info;

	/* 1. send avmute */
	dw_avp_set_mute(0x1);

	/* 2. clear sink scdc info */
	if (!dw_phy_get_hpd())
		goto disconfig_exit;

	if (dw_fc_video_get_scramble() && dw_sink_support_scdc())
		dw_hdmi_scdc_set_scramble(0);

disconfig_exit:
	dw_phy_standby();

	dw_mc_clk_all_disable();

	dw_hdmi_ctrl_reset();

	memset(info, 0x0, sizeof(struct disp_device_config));
	return 0;
}

int sunxi_hdmi_config(void)
{
	struct dw_phy_ops_s *func = &sunxi_hdmi->plat_data->phy_func;
	int ret = 0;

	if (IS_ERR_OR_NULL(func)) {
		shdmi_err(func);
		return -1;
	}

	mutex_lock(&sunxi_hdmi->lock_config);
	dw_avp_set_mute(0x1);

	if (!IS_ERR_OR_NULL(func->phy_disconfig)) {
		ret = func->phy_disconfig();
		if (ret != 0) {
			hdmi_err("sunxi hdmi phy disconfig failed\n");
			goto exit;
		}
	}

	ret = dw_avp_config();
	if (ret != 0) {
		hdmi_err("sunxi hdmi avp config failed\n");
		goto exit;
	}

	if (sunxi_hdmi->plat_data->use_top_phy == 0x1) {
		ret = top_phy_config();
		if (ret != 0) {
			hdmi_err("sunxi hdmi top phy config failed\n");
			goto exit;
		}
	}

	if (!IS_ERR_OR_NULL(func->phy_config)) {
		ret = func->phy_config();
		if (ret != 0) {
			hdmi_err("sunxi hdmi ext phy config failed\n");
			goto exit;
		}
	}

	mdelay(50);
	dw_avp_set_mute(0x0);

exit:
	mutex_unlock(&sunxi_hdmi->lock_config);
	hdmi_trace("%s ret = %d\n", __func__, ret);
	return ret;
}

int sunxi_hdmi_smooth_config(void)
{
	int ret = 0;

	ret = dw_hdmi_ctrl_update();
	if (ret != 0) {
		hdmi_err("sunxi hdmi update control param failed\n");
		return -1;
	}

	ret = dw_infoframe_packet();
	if (ret != 0) {
		hdmi_err("sunxi hdmi smooth config infoframe failed\n");
		return -1;
	}
	return 0;
}

int sunxi_hdmi_set_disp_mode(struct drm_display_mode *mode)
{
	dw_dtd_t video;
	u32 rate = 0;

	memset(&video, 0x0, sizeof(dw_dtd_t));

	if (IS_ERR_OR_NULL(mode)) {
		shdmi_err(mode);
		goto ret_failed;
	}

	video.mCode = (u32)drm_match_cea_mode(mode);

	video.mPixelClock = mode->clock;
	video.mInterlaced = (mode->flags & DRM_MODE_FLAG_INTERLACE) ? 0x1 : 0x0;
	video.mPixelRepetitionInput = (mode->flags & DRM_MODE_FLAG_DBLCLK) ? 0x1 : 0x0;

	video.mHActive     = mode->hdisplay;
	video.mHBlanking   = mode->htotal - mode->hdisplay;
	video.mHSyncOffset = mode->hsync_start - mode->hdisplay;
	video.mHSyncPulseWidth = mode->hsync_end - mode->hdisplay - video.mHSyncOffset;
	video.mHSyncPolarity   = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0x1 : 0x0;

	video.mVActive     = mode->vdisplay;
	video.mVBlanking   = mode->vtotal - mode->vdisplay;
	video.mVSyncOffset = mode->vsync_start - mode->vdisplay;
	video.mVSyncPulseWidth = mode->vsync_end - mode->vdisplay - video.mVSyncOffset;
	video.mVSyncPolarity   = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0x1 : 0x0;

	if (video.mInterlaced) {
		video.mVActive     /= 2;
		video.mVBlanking   /= 2;
		video.mVSyncOffset /= 2;
		video.mVSyncPulseWidth /= 2;
	}

	switch (mode->picture_aspect_ratio) {
	case HDMI_PICTURE_ASPECT_4_3:
		video.mHImageSize = 4;
		video.mVImageSize = 3;
		break;
	case HDMI_PICTURE_ASPECT_16_9:
		video.mHImageSize = 16;
		video.mVImageSize = 9;
		break;
	case HDMI_PICTURE_ASPECT_64_27:
		video.mHImageSize = 64;
		video.mVImageSize = 27;
		break;
	case HDMI_PICTURE_ASPECT_256_135:
		video.mHImageSize = 256;
		video.mVImageSize = 135;
		break;
	default:
		video.mHImageSize = 16;
		video.mVImageSize = 9;
		break;
	}

	rate = drm_mode_vrefresh(mode);

	dw_video_filling_timing(&video, rate);

	return 0;
ret_failed:
	return -1;
}

static int _sunxi_hdmi_board_init(struct sunxi_hdmi_s *hdmi)
{
	u8 res_src   = hdmi->resistor_src;
	u8 rescal_en = hdmi->plat_data->need_res_cal;
	struct sunxi_hdmi_rescal_s *res_reg = &hdmi->plat_data->rescal_regs;

	void __iomem *reg_addr;

	if (!rescal_en) {
		hdmi_trace("this plat not need use resistance calibration\n");
		return 0;
	}

	if (res_src == 0x1) {
		reg_addr = ioremap(res_reg->rescal_ctrl_pa, 4);
		writel(readl(reg_addr) | res_reg->bit_hdmi_res_sel, reg_addr);
		iounmap(reg_addr);

		reg_addr = ioremap(res_reg->res0_ctrl_pa, 4);
		writel(readl(reg_addr) & (~res_reg->res0_ctrl_bitmask), reg_addr);
		iounmap(reg_addr);
	} else {
		reg_addr = ioremap(res_reg->rescal_ctrl_pa, 4);

		writel((readl(reg_addr) & (~res_reg->bit_cal_en)), reg_addr);
		writel((readl(reg_addr) & (~res_reg->bit_hdmi_res_sel)), reg_addr);
		writel((readl(reg_addr) | (res_reg->bit_cal_ana_en)), reg_addr);
		writel((readl(reg_addr) | (res_reg->bit_cal_en)), reg_addr);

		udelay(200);

		writel((readl(reg_addr) & (~res_reg->bit_cal_ana_en)), reg_addr);
		writel((readl(reg_addr) & (~res_reg->bit_cal_en)), reg_addr);

		iounmap(reg_addr);
	}
	return 0;
}

static const struct of_device_id hdmi_plat_match[] = {
	{ .compatible = "arm,sun8iw20p1", .data = &sun8i_hdmi },
	{ .compatible = "arm,sun50iw9p1", .data = &sun50i_hdmi },
	{ .compatible = "arm,sun55iw3p1", .data = &sun55i_hdmi },
	{ .compatible = "arm,sun60iw2p1", .data = &sun60i_hdmi },
	{ },
};

int sunxi_hdmi_init(struct sunxi_hdmi_s *hdmi)
{
	int ret = 0;
	struct device_node *root_node = NULL;
	const struct of_device_id *match = NULL;

	root_node = of_find_node_by_path("/");
	if (IS_ERR_OR_NULL(root_node)) {
		shdmi_err(root_node);
		return -1;
	}

	match = of_match_node(hdmi_plat_match, root_node);
	if (IS_ERR_OR_NULL(match)) {
		shdmi_err(match);
		return -1;
	}
	hdmi->plat_data = (struct sunxi_hdmi_plat_s *)match->data;

	if (IS_ERR_OR_NULL(hdmi->plat_data)) {
		shdmi_err(hdmi->plat_data);
		return -1;
	}

	mutex_init(&hdmi->lock_config);

	if (!hdmi->smooth_boot) {
		_sunxi_hdmi_board_init(hdmi);
		hdmi_trace("sunxi hdmi board init done\n");
	}

	mutex_lock(&hdmi->lock_config);
	/* bind phy ops */
	hdmi->dw_hdmi.phy_ext   = &hdmi->plat_data->phy_func;
	hdmi->dw_hdmi.dev       = hdmi->dev;
	hdmi->dw_hdmi.addr      = hdmi->reg_base;
	hdmi->dw_hdmi.i2c_adap  = hdmi->i2c_adap;
	hdmi->dw_hdmi.connect   = hdmi->connect;
	hdmi->dw_hdmi.clock_src = hdmi->clock_src;
	hdmi->dw_hdmi.plat_id   = hdmi->plat_data->version;
	hdmi->dw_hdmi.sw_init   = hdmi->smooth_boot;
	ret = dw_hdmi_init(&hdmi->dw_hdmi);
	if (ret != 0) {
		hdmi_err("dw dev init failed\n");
		goto exit;
	}

	if (hdmi->plat_data->use_top_phy) {
		ret = top_phy_init();
		if (ret != 0) {
			hdmi_err("top phy init failed\n");
			goto exit;
		}
	}

	sunxi_hdmi = hdmi;
exit:
	hdmi_trace("%s ret = %d\n", __func__, ret);
	mutex_unlock(&hdmi->lock_config);
	return ret;
}

void sunxi_hdmi_exit(void)
{
	dw_hdmi_exit();

	if (sunxi_hdmi) {
		kfree(sunxi_hdmi);
		sunxi_hdmi = NULL;
	}
}

ssize_t sunxi_hdmi_tx_dump(char *buf)
{
	int n = 0;

	n += dw_hdmi_dump(buf + n);

	if (sunxi_hdmi->plat_data->phy_func.phy_dump)
		n += sunxi_hdmi->plat_data->phy_func.phy_dump(buf + n);

	if (sunxi_hdmi->plat_data->use_top_phy) {
		n += top_phy_dump(buf + n);
	}

	return n;
}

ssize_t sunxi_hdmi_rx_dump(char *buf)
{
	int n = 0;

	n += dw_sink_dump(buf + n);

	return n;
}
