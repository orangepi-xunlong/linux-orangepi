/*
 ******************************************************************************
 *
 * parser_reg.c
 *
 * CSIC - parser_reg.c module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    	Description
 *
 *   1.0		  Zhao Wei   	2016/06/13	      First Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>
#include "parser_reg_i.h"
#include "parser_reg.h"

#include "../utility/vin_io.h"

volatile void __iomem *csic_prs_base[MAX_CSIC_PRS_NUM];

int csic_prs_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSIC_PRS_NUM - 1)
		return -1;
	csic_prs_base[sel] = (volatile void __iomem *)addr;

	return 0;
}

void csic_prs_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_EN_REG_OFF,
			PRS_EN_REG_PRS_EN_MASK, 1 << PRS_EN_REG_PRS_EN);
}

void csic_prs_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_EN_REG_OFF,
			PRS_EN_REG_PRS_EN_MASK, 0 << PRS_EN_REG_PRS_EN);
}

void csic_prs_mode(unsigned int sel, enum prs_mode mode)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_EN_REG_OFF,
			PRS_EN_REG_PRS_MODE_MASK, mode << PRS_EN_REG_PRS_MODE);
}

void csic_prs_pclk_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_EN_REG_OFF,
			PRS_EN_REG_PCLK_EN_MASK, en << PRS_EN_REG_PCLK_EN);
}

void csic_prs_ncsi_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_EN_REG_OFF,
			PRS_EN_REG_NCSIC_EN_MASK, en << PRS_EN_REG_NCSIC_EN);
}

void csic_prs_mcsi_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_EN_REG_OFF,
			PRS_EN_REG_MCSIC_EN_MASK, en << PRS_EN_REG_MCSIC_EN);
}

void csic_prs_ncsi_if_cfg(unsigned int sel, struct prs_ncsi_if_cfg *if_cfg)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_CSI_IF_MASK,
		if_cfg->intf << PRS_NCSIC_IF_CSI_IF);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_OUTPUT_MODE_MASK,
		if_cfg->mode << PRS_NCSIC_IF_OUTPUT_MODE);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_INPUT_SEQ_MASK,
		if_cfg->seq << PRS_NCSIC_IF_INPUT_SEQ);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_DATA_WIDTH_MASK,
		if_cfg->dw << PRS_NCSIC_IF_DATA_WIDTH);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_FIELD_DT_MASK,
		if_cfg->field_dt << PRS_NCSIC_IF_FIELD_DT);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_CLK_POL_MASK,
		if_cfg->clk << PRS_NCSIC_IF_CLK_POL);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_HREF_POL_MASK,
		if_cfg->href << PRS_NCSIC_IF_HREF_POL);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_VREF_POL_MASK,
		if_cfg->vref << PRS_NCSIC_IF_VREF_POL);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_FIELD_POL_MASK,
		if_cfg->field << PRS_NCSIC_IF_FIELD_POL);

	vin_reg_clr_set(csic_prs_base[sel] + PRS_NCSIC_IF_CFG_REG_OFF,
		PRS_NCSIC_IF_SRC_TYPE_MASK,
		if_cfg->type << PRS_NCSIC_IF_SRC_TYPE);
}

void csic_prs_mcsi_if_cfg(unsigned int sel, struct prs_mcsi_if_cfg *if_cfg)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_MCSIC_IF_CFG_REG_OFF,
		PRS_MCSIC_IF_INPUT_SEQ_MASK,
		if_cfg->input_seq << PRS_MCSIC_IF_INPUT_SEQ);
	vin_reg_clr_set(csic_prs_base[sel] + PRS_MCSIC_IF_CFG_REG_OFF,
		PRS_MCSIC_IF_OUTPUT_MODE_MASK,
		if_cfg->output_mode << PRS_MCSIC_IF_OUTPUT_MODE);
}

void csic_prs_capture(unsigned int sel, unsigned int ch,
			struct prs_cap_mode *mode)
{
	unsigned int reg_val = mode->mode << (CH0_SCAP_ON + ch * 8);
	reg_val = reg_val | (mode->cap_mask << (CH0_CAP_MASK + ch * 8));
	vin_reg_writel(csic_prs_base[sel] + PRS_CAP_REG_OFF, reg_val);
}

void csic_prs_signal_status(unsigned int sel, struct prs_signal_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_prs_base[sel] + PRS_SIGNAL_STA_REG_OFF);
	status->pclk_sta = (reg_val & PCLK_STA_MASK) >> PCLK_STA;
	status->data_sta = (reg_val & DATA_STA_MASK) >> DATA_STA;
}

void csic_prs_ncsi_bt656_header_cfg(unsigned int sel, struct prs_ncsi_bt656_header *header)
{
	unsigned int reg_val = (header->ch0_id << CH0_ID) |
				(header->ch1_id << CH1_ID) |
				(header->ch2_id << CH2_ID) |
				(header->ch3_id << CH3_ID);
	vin_reg_writel(csic_prs_base[sel] + PRS_NCSIC_BT656_HEAD_CFG_REG_OFF,
			reg_val);
}

void csic_prs_input_fmt_cfg(unsigned int sel, unsigned int ch,
				enum prs_input_fmt fmt)
{
	vin_reg_writel(csic_prs_base[sel] + PRS_CH0_INFMT_REG_OFF +
			ch * PARSER_CH_OFF, fmt);
}

void csic_prs_output_size_cfg(unsigned int sel, unsigned int ch,
			struct prs_output_size *size)
{
	vin_reg_clr_set(csic_prs_base[sel] + PRS_CH0_OUTPUT_HSIZE_REG_OFF +
		ch * PARSER_CH_OFF, PRS_CH0_HOR_START_MASK,
		size->hor_start << PRS_CH0_HOR_START);
	vin_reg_clr_set(csic_prs_base[sel] + PRS_CH0_OUTPUT_HSIZE_REG_OFF +
		ch * PARSER_CH_OFF, PRS_CH0_HOR_LEN_MASK,
		size->hor_len << PRS_CH0_HOR_LEN);
	vin_reg_clr_set(csic_prs_base[sel] + PRS_CH0_OUTPUT_VSIZE_REG_OFF +
		ch * PARSER_CH_OFF, PRS_CH0_VER_START_MASK,
		size->ver_start << PRS_CH0_VER_START);
	vin_reg_clr_set(csic_prs_base[sel] + PRS_CH0_OUTPUT_VSIZE_REG_OFF +
		ch * PARSER_CH_OFF, PRS_CH0_VER_LEN_MASK,
		size->ver_len << PRS_CH0_VER_LEN);
}


/* for debug */
void csic_prs_input_para_get(unsigned int sel, unsigned int ch,
				struct prs_input_para *para)
{

}
void csic_prs_int_enable(unsigned int sel, unsigned int ch,
				enum prs_int_sel interrupt)
{
	vin_reg_set(csic_prs_base[sel] + PRS_CH0_INT_EN_REG_OFF +
		    ch * PARSER_CH_OFF, interrupt);
}
void csic_prs_int_disable(unsigned int sel, unsigned int ch,
				enum prs_int_sel interrupt)
{
	vin_reg_clr(csic_prs_base[sel] + PRS_CH0_INT_EN_REG_OFF +
		    ch * PARSER_CH_OFF, interrupt);
}
void csic_prs_int_get_status(unsigned int sel, unsigned int ch,
				struct prs_int_status *status)
{
	unsigned int reg_val =
		vin_reg_readl(csic_prs_base[sel] + PRS_CH0_INT_STA_REG_OFF +
				ch * PARSER_CH_OFF);
	status->input_src_pd0 = (reg_val >> PRS_CH0_INPUT_SRC_PD0) & 0X01;
	status->input_src_pd1 = (reg_val >> PRS_CH0_INPUT_SRC_PD1) & 0X01;
	status->mul_err_pd = (reg_val >> PRS_CH0_MUL_ERR_PD) & 0X01;
}
void csic_prs_int_clear_status(unsigned int sel, unsigned int ch,
				enum prs_int_sel interrupt)
{
	vin_reg_writel(csic_prs_base[sel] + PRS_CH0_INT_STA_REG_OFF +
			ch * PARSER_CH_OFF, interrupt);
}

