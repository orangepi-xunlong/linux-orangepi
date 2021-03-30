/*
 ******************************************************************************
 *
 * csi_reg.c
 *
 * Hawkview ISP - csi_reg.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    		Description
 *
 *   2.0		  Yang Feng   	2014/07/15	      Second Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>
#include "csi_reg_i.h"
#include "csi_reg.h"

#include "../utility/vin_io.h"


#if defined CONFIG_ARCH_SUN8IW11P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 0	/*0:RISING, 1:FAILING*/

volatile void __iomem *csi_base_addr[2];
enum csi_input_fmt input_fmt;

int csi_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSI - 1)
		return -1;
	csi_base_addr[sel] = (volatile void __iomem *)addr;

	return 0;
}

/* open module */
void csi_enable(unsigned int sel)
{
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_EN, 0x1);
}

void csi_disable(unsigned int sel)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_REG_EN, 0X1 << 0);
}

/* configure */
void csi_if_cfg(unsigned int sel, struct csi_if_cfg *csi_if_cfg)
{
	if (csi_if_cfg->interface == CSI_IF_CCIR656_16BIT) {
		input_fmt = CSI_YUV422_16;
	} else if (csi_if_cfg->interface == CSI_IF_CCIR656_1CH) {
		input_fmt = CSI_CCIR656;
	} else if (csi_if_cfg->interface == CSI_IF_CCIR656_2CH) {
		input_fmt = CSI_CCIR656_2CH;
	} else if (csi_if_cfg->interface == CSI_IF_CCIR656_4CH) {
		input_fmt = CSI_CCIR656_4CH;
	}
	vin_reg_clr_set(csi_base_addr[sel] + CSI_REG_CONF,
				0x7 << 20,
				input_fmt << 20);/*[22:20]*/
}

void csi_timing_cfg(unsigned int sel, struct csi_timing_cfg *csi_tmg_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_REG_CONF, 0x7,
			csi_tmg_cfg->vref << 2 |	/* [2] */
			csi_tmg_cfg->href << 1 |	/* [1] */
			(csi_tmg_cfg->sample == CLK_POL));	/* [0] */
}

void csi_fmt_cfg(unsigned int sel, unsigned int ch,
		 struct csi_fmt_cfg *csi_fmt_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_REG_CONF,
			0x7 << 20 | 0xf << 16 | 0x3 << 10 | 0x3 << 8,
			csi_fmt_cfg->input_fmt << 20 |	/* [21:20] */
			csi_fmt_cfg->output_fmt << 16 |	/* [18:16] */
			csi_fmt_cfg->field_sel << 10 |	/* [11:10] */
			csi_fmt_cfg->input_seq << 8);	/* [9:8] */
	input_fmt = csi_fmt_cfg->input_fmt;
}

/* buffer */
void csi_set_buffer_address(unsigned int sel, unsigned int ch,
			    enum csi_buf_sel buf, u64 addr)
{
	/*bufer0a +4 = buffer0b, bufer0a +8 = buffer1a*/
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_BUF_0_A + (buf << 2), addr);
}

u64 csi_get_buffer_address(unsigned int sel, unsigned int ch,
			   enum csi_buf_sel buf)
{
	u32 t;
	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_BUF_0_A + (buf << 2));
	return t;
}

/* capture */
void csi_capture_start(unsigned int sel, unsigned int ch_total_num,
		       enum csi_cap_mode csi_cap_mode)
{
	if (CSI_VCAP == csi_cap_mode)
		vin_reg_set(csi_base_addr[sel] + CSI_REG_CTRL, 0X1 << 1);
	else
		vin_reg_set(csi_base_addr[sel] + CSI_REG_CTRL, 0X1 << 0);
}

void csi_capture_stop(unsigned int sel, unsigned int ch_total_num,
		      enum csi_cap_mode csi_cap_mode)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_REG_CTRL, 0X3);
}

void csi_capture_get_status(unsigned int sel, unsigned int ch,
			    struct csi_capture_status *status)
{
	u32 t;
	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_STATUS);
	status->picture_in_progress = t & 0x1;
	status->video_in_progress = (t >> 1) & 0x1;
}

/* size */
void csi_set_size(unsigned int sel, unsigned int ch, unsigned int length_h,
		  unsigned int length_v, unsigned int buf_length_h,
		  unsigned int buf_length_c)
{
	u32 t;

	switch (input_fmt) {
	case CSI_CCIR656:
	case CSI_CCIR656_2CH:
	case CSI_CCIR656_4CH:
	case CSI_YUV422_16:
	case CSI_YUV422:
		length_h = length_h * 2;
		break;
	default:
		break;
	}

	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_RESIZE_H + ch * CSI_CH_OFF);
	t = (t & 0x0000ffff) | (length_h << 16);
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_RESIZE_H + ch * CSI_CH_OFF, t);

	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_RESIZE_V + ch * CSI_CH_OFF);
	t = (t & 0x0000ffff) | (length_v << 16);
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_RESIZE_V + ch * CSI_CH_OFF, t);

	vin_reg_writel(csi_base_addr[sel] + CSI_REG_BUF_LENGTH + ch * CSI_CH_OFF, buf_length_h);

}

/* offset */
void csi_set_offset(unsigned int sel, unsigned int ch, unsigned int start_h,
		    unsigned int start_v)
{
	u32 t;

	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_RESIZE_H + ch * CSI_CH_OFF);
	t = (t & 0xffff0000) | start_h;
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_RESIZE_H + ch * CSI_CH_OFF, t);

	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_RESIZE_V + ch * CSI_CH_OFF);
	t = (t & 0xffff0000) | start_v;
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_RESIZE_V + ch * CSI_CH_OFF, t);
}

/* interrupt */
void csi_int_enable(unsigned int sel, unsigned int ch,
		    enum csi_int_sel interrupt)
{
	vin_reg_set(csi_base_addr[sel] + CSI_REG_INT_EN, interrupt);
}

void csi_int_disable(unsigned int sel, unsigned int ch,
		     enum csi_int_sel interrupt)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_REG_INT_EN, interrupt);
}

inline void csi_int_get_status(unsigned int sel, unsigned int ch,
			       struct csi_int_status *status)
{
	u32 t;
	t = vin_reg_readl(csi_base_addr[sel] + CSI_REG_INT_STATUS);

	status->capture_done = t & CSI_INT_CAPTURE_DONE;
	status->frame_done = t & CSI_INT_FRAME_DONE;
	status->buf_0_overflow = t & CSI_INT_BUF_0_OVERFLOW;
	status->buf_1_overflow = t & CSI_INT_BUF_1_OVERFLOW;
	status->buf_2_overflow = t & CSI_INT_BUF_2_OVERFLOW;
	status->protection_error = t & CSI_INT_PROTECTION_ERROR;
	status->hblank_overflow = t & CSI_INT_HBLANK_OVERFLOW;
	status->vsync_trig = t & CSI_INT_VSYNC_TRIG;
}

inline void csi_int_clear_status(unsigned int sel, unsigned int ch,
				 enum csi_int_sel interrupt)
{
	vin_reg_writel(csi_base_addr[sel] + CSI_REG_INT_STATUS, interrupt);
}

#else

#if defined CONFIG_ARCH_SUN8IW1P1
#define ADDR_BIT_R_SHIFT 0
#define CLK_POL 0
#elif defined CONFIG_ARCH_SUN8IW3P1
#define ADDR_BIT_R_SHIFT 0
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN9IW1P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW5P1
#define ADDR_BIT_R_SHIFT 0
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW6P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW7P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW8P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW9P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN50I
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#elif defined CONFIG_ARCH_SUN8IW10P1
#define ADDR_BIT_R_SHIFT 2
#define CLK_POL 1
#endif

volatile void __iomem *csi_base_addr[2];

int csi_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSI - 1)
		return -1;
	csi_base_addr[sel] = (volatile void __iomem *)addr;

	return 0;
}

/* open module */
void csi_enable(unsigned int sel)
{
	vin_reg_set(csi_base_addr[sel] + CSI_EN_REG_OFF,
		    1 << CSI_EN_REG_CSI_EN);
}

void csi_disable(unsigned int sel)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_EN_REG_OFF,
		    1 << CSI_EN_REG_CSI_EN);
}

/* configure */
void csi_if_cfg(unsigned int sel, struct csi_if_cfg *csi_if_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_SRC_TYPE_MASK,
			csi_if_cfg->src_type << CSI_IF_CFG_REG_SRC_TYPE);

	if (csi_if_cfg->interface < 0x80)
		vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
				CSI_IF_CFG_REG_CSI_IF_MASK,
				csi_if_cfg->interface << CSI_IF_CFG_REG_CSI_IF);
	else
		vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
				CSI_IF_CFG_REG_MIPI_IF_MASK,
				1 << CSI_IF_CFG_REG_MIPI_IF);

	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_IF_DATA_WIDTH_MASK,
			csi_if_cfg->data_width << CSI_IF_CFG_REG_IF_DATA_WIDTH);
}

void csi_timing_cfg(unsigned int sel, struct csi_timing_cfg *csi_tmg_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_VREF_POL_MASK,
			csi_tmg_cfg->vref << CSI_IF_CFG_REG_VREF_POL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_HREF_POL_MASK,
			csi_tmg_cfg->href << CSI_IF_CFG_REG_HREF_POL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_CLK_POL_MASK,
			((csi_tmg_cfg->sample ==
			  CLK_POL) ? 1 : 0) << CSI_IF_CFG_REG_CLK_POL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_IF_CFG_REG_OFF,
			CSI_IF_CFG_REG_FIELD_MASK,
			csi_tmg_cfg->field << CSI_IF_CFG_REG_FIELD);
}

void csi_fmt_cfg(unsigned int sel, unsigned int ch,
		 struct csi_fmt_cfg *csi_fmt_cfg)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_INPUT_FMT_MASK,
			csi_fmt_cfg->input_fmt << CSI_CH_CFG_REG_INPUT_FMT);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_OUTPUT_FMT_MASK,
			csi_fmt_cfg->output_fmt << CSI_CH_CFG_REG_OUTPUT_FMT);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_FIELD_SEL_MASK,
			csi_fmt_cfg->field_sel << CSI_CH_CFG_REG_FIELD_SEL);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_CFG_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_CFG_REG_INPUT_SEQ_MASK,
			csi_fmt_cfg->input_seq << CSI_CH_CFG_REG_INPUT_SEQ);
}

/* buffer */
void csi_set_buffer_address(unsigned int sel, unsigned int ch,
			    enum csi_buf_sel buf, u64 addr)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_F0_BUFA_REG_OFF +
			ch * CSI_CH_OFF + (buf << 2), 0xffffffff,
			addr >> ADDR_BIT_R_SHIFT);
}

u64 csi_get_buffer_address(unsigned int sel, unsigned int ch,
			   enum csi_buf_sel buf)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_base_addr[sel] + CSI_CH_F0_BUFA_REG_OFF +
			  ch * CSI_CH_OFF + (buf << 2));
	return reg_val << ADDR_BIT_R_SHIFT;
}

/* capture */
void csi_capture_start(unsigned int sel, unsigned int ch_total_num,
		       enum csi_cap_mode csi_cap_mode)
{
	u32 reg_val = (((ch_total_num == 4) ? csi_cap_mode : 0) << 24) +
	    (((ch_total_num == 3) ? csi_cap_mode : 0) << 16) +
	    (((ch_total_num == 2) ? csi_cap_mode : 0) << 8) +
	    (((ch_total_num == 1) ? csi_cap_mode : 0));
	vin_reg_writel(csi_base_addr[sel] + CSI_CAP_REG_OFF, reg_val);
}

void csi_capture_stop(unsigned int sel, unsigned int ch_total_num,
		      enum csi_cap_mode csi_cap_mode)
{
	vin_reg_writel(csi_base_addr[sel] + CSI_CAP_REG_OFF, 0);
}

void csi_capture_get_status(unsigned int sel, unsigned int ch,
			    struct csi_capture_status *status)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_base_addr[sel] + CSI_CH_STA_REG_OFF +
			  ch * CSI_CH_OFF);
	status->picture_in_progress =
	    (reg_val >> CSI_CH_STA_REG_SCAP_STA) & 0x1;
	status->video_in_progress = (reg_val >> CSI_CH_STA_REG_VCAP_STA) & 0x1;
}

/* size */
void csi_set_size(unsigned int sel, unsigned int ch, unsigned int length_h,
		  unsigned int length_v, unsigned int buf_length_y,
		  unsigned int buf_length_c)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_HSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_HSIZE_REG_HOR_LEN_MASK,
			length_h << CSI_CH_HSIZE_REG_HOR_LEN);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_VSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_VSIZE_REG_VER_LEN_MASK,
			length_v << CSI_CH_VSIZE_REG_VER_LEN);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_BUF_LEN_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_BUF_LEN_REG_BUF_LEN_MASK,
			buf_length_y << CSI_CH_BUF_LEN_REG_BUF_LEN);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_BUF_LEN_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_BUF_LEN_REG_BUF_LEN_C_MASK,
			buf_length_c << CSI_CH_BUF_LEN_REG_BUF_LEN_C);
}

/* offset */
void csi_set_offset(unsigned int sel, unsigned int ch, unsigned int start_h,
		    unsigned int start_v)
{
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_HSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_HSIZE_REG_HOR_START_MASK,
			start_h << CSI_CH_HSIZE_REG_HOR_START);
	vin_reg_clr_set(csi_base_addr[sel] + CSI_CH_VSIZE_REG_OFF +
			ch * CSI_CH_OFF, CSI_CH_VSIZE_REG_VER_START_MASK,
			start_v << CSI_CH_VSIZE_REG_VER_START);
}

/* interrupt */
void csi_int_enable(unsigned int sel, unsigned int ch,
		    enum csi_int_sel interrupt)
{
	vin_reg_set(csi_base_addr[sel] + CSI_CH_INT_EN_REG_OFF +
		    ch * CSI_CH_OFF, interrupt);
}

void csi_int_disable(unsigned int sel, unsigned int ch,
		     enum csi_int_sel interrupt)
{
	vin_reg_clr(csi_base_addr[sel] + CSI_CH_INT_EN_REG_OFF +
		    ch * CSI_CH_OFF, interrupt);
}

void csi_int_get_status(unsigned int sel, unsigned int ch,
			struct csi_int_status *status)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_base_addr[sel] + CSI_CH_INT_STA_REG_OFF +
			  ch * CSI_CH_OFF);
	status->capture_done = (reg_val >> CSI_CH_INT_STA_REG_CD_PD) & 0x1;
	status->frame_done = (reg_val >> CSI_CH_INT_STA_REG_FD_PD) & 0x1;
	status->buf_0_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_FIFO0_OF_PD) & 0x1;
	status->buf_1_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_FIFO1_OF_PD) & 0x1;
	status->buf_2_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_FIFO2_OF_PD) & 0x1;
	status->protection_error =
	    (reg_val >> CSI_CH_INT_STA_REG_PRTC_ERR_PD) & 0x1;
	status->hblank_overflow =
	    (reg_val >> CSI_CH_INT_STA_REG_HB_OF_PD) & 0x1;
	status->vsync_trig = (reg_val >> CSI_CH_INT_STA_REG_VS_PD) & 0x1;
}

void csi_int_clear_status(unsigned int sel, unsigned int ch,
			  enum csi_int_sel interrupt)
{
	vin_reg_writel(csi_base_addr[sel] + CSI_CH_INT_STA_REG_OFF, interrupt);
}
#endif
