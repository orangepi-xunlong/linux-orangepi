// SPDX-License-Identifier: GPL-2.0
/*
 * hw_postpipe.c - postpipe hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "hw_reg.h"
#include "hw_postpipe.h"

#define KY_POSTPIPE_SCL0_OFFSET	(0x100)
#define KY_POSTPIPE_SCL1_OFFSET	(0x200)
#define PP_REG		TOP_REG
#define SCL_REG(n)	(TOP_REG_ADDR(sc_block->base_addr + offset, (n)))

void hw_postpipe_set_formatter_format(struct spm_camera_block *sc_block,
				      unsigned int idx, int format)
{
	union pp_reg_2 reg_2;

	reg_2.value = read32(PP_REG(2 + idx));
	reg_2.field.fmt0_m_bSwitchYCFlag = 0;
	reg_2.field.fmt0_m_bConvertDithering = 1;
	reg_2.field.fmt0_m_bCompressDithering = 1;
	reg_2.field.fmt0_m_bSwitchUVFlag = 0;
	reg_2.field.fmt0_m_bCompress = 0;
	switch (format) {
	case NV12:
		reg_2.field.fmt0_m_nFormat = 1;
		break;
	case NV21:
		reg_2.field.fmt0_m_nFormat = 1;
		reg_2.field.fmt0_m_bSwitchUVFlag = 1;
		break;
	case RGB888:
		reg_2.field.fmt0_m_bSwitchUVFlag = 1;
		fallthrough;
	case P210:
	case Y210:
	case P010:
	case RGB565:
		reg_2.field.fmt0_m_nFormat = 2 + (format - P210);
		break;
	default:
		reg_2.field.fmt0_m_nFormat = 1;
	}
	write32(PP_REG(2 + idx), reg_2.value);
}

void hw_postpipe_dma_mux_enable(struct spm_camera_block *sc_block, int mux_select)
{
	union pp_reg_14 reg_14;

	reg_14.value = read32(PP_REG(14));
	switch (mux_select) {
	case MUX_SEL_FORMATTER2:
		reg_14.field.dma_mux_ctrl_6 = 0;
		break;
	case MUX_SEL_FORMATTER3:
		reg_14.field.dma_mux_ctrl_7 = 0;
		break;
	case MUX_SEL_FORMATTER4:
		reg_14.field.dma_mux_ctrl_0 = 0;
		break;
	case MUX_SEL_FORMATTER5:
		reg_14.field.dma_mux_ctrl_1 = 0;
		break;
	case MUX_SEL_DWT0_LAYER1:
		reg_14.field.dma_mux_ctrl_2 = 0;
		break;
	case MUX_SEL_DWT0_LAYER2:
		reg_14.field.dma_mux_ctrl_3 = 0;
		break;
	case MUX_SEL_DWT0_LAYER3:
		reg_14.field.dma_mux_ctrl_4 = 0;
		break;
	case MUX_SEL_DWT0_LAYER4:
		reg_14.field.dma_mux_ctrl_5 = 0;
		break;
	}

	write32(PP_REG(14), reg_14.value);
}

void hw_postpipe_enable_dwt(struct spm_camera_block *sc_block, unsigned int idx, int src, int enable)
{
	union pp_reg_8 reg_8;

	reg_8.value = read32(PP_REG(8 + idx));
	if (enable) {
		reg_8.field.dwt0_src_sel = src;
		if (src < DWT_SRC_SEL_FORMATTER2)
			reg_8.field.dwt0_mode_sel = 0;
		else
			reg_8.field.dwt0_mode_sel = 1;
	}
	reg_8.field.dwt0_ena = enable ? 1 : 0;
	write32(PP_REG(8 + idx), reg_8.value);
}

/*
void hw_postpipe_set_scaler(struct spm_camera_block *sc_block, unsigned int idx,
							unsigned int in_width, unsigned int in_height,
							unsigned int out_width, unsigned int out_height)
{
	union scl_reg_12 reg_12;
	union scl_reg_13 reg_13;
	unsigned long offset = 0;

	reg_12.value = 0;
	if (idx == 0)
		offset = KY_POSTPIPE_SCL0_OFFSET;
	else
		offset = KY_POSTPIPE_SCL1_OFFSET;
	reg_12.field.m_nintrim_out_width = in_width;
	reg_12.field.m_nintrim_out_height = in_height;
	write32(SCL_REG(12), reg_12.value);
	reg_13.value = 0;
	reg_13.field.m_nouttrim_out_width = in_width;
	reg_13.field.m_nouttrim_out_height = in_height;
	write32(SCL_REG(13), reg_13.value);
}
*/
void hw_postpipe_set_scaler_source(struct spm_camera_block *sc_block, unsigned int idx, int source)
{
	union scl_reg_11 reg_11;
	unsigned long offset = 0;

	reg_11.value = 0;
	if (idx == 0)
		offset = KY_POSTPIPE_SCL0_OFFSET;
	else
		offset = KY_POSTPIPE_SCL1_OFFSET;
	//disable scaler & in/out trim
	write32(SCL_REG(0), 0);
	reg_11.field.pipe_sel = source;
	write32(SCL_REG(11), reg_11.value);
}
