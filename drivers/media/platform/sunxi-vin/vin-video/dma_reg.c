/*
 ******************************************************************************
 *
 * dma_reg.c
 *
 * CSIC - dma_reg.c module
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
#include "dma_reg_i.h"
#include "dma_reg.h"

#include "../utility/vin_io.h"

#define ADDR_BIT_R_SHIFT 2

volatile void __iomem *csic_dma_base[MAX_CSIC_DMA_NUM];

int csic_dma_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSIC_DMA_NUM - 1)
		return -1;
	csic_dma_base[sel] = (volatile void __iomem *)addr;

	return 0;
}

/* open module */
void csic_dma_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_EN_REG_OFF,
			CSIC_DMA_EN_MASK, 1 << CSIC_DMA_EN);
}

void csic_dma_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_EN_REG_OFF,
			CSIC_DMA_EN_MASK, 0 << CSIC_DMA_EN);
}
void csic_dma_clk_cnt_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_EN_REG_OFF,
			CSIC_CLK_CNT_EN_MASK, 1 << CSIC_CLK_CNT_EN);

}
void csic_dma_clk_cnt_sample(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_EN_REG_OFF,
			CSIC_CLK_CNT_SPL_MASK, 1 << CSIC_CLK_CNT_SPL);

}

void csic_dma_output_fmt_cfg(unsigned int sel, enum output_fmt fmt)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			OUTPUT_FMT_MASK, fmt << OUTPUT_FMT);

}
void csic_dma_flip_en(unsigned int sel, struct csic_dma_flip *flip)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			HFLIP_EN_MASK, flip->hflip_en << HFLIP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			VFLIP_EN_MASK, flip->vflip_en << VFLIP_EN);
}
void csic_dma_config(unsigned int sel, struct csic_dma_cfg *cfg)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			MIN_SDR_WR_SIZE_MASK, cfg->block << MIN_SDR_WR_SIZE);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			FPS_DS_MASK, cfg->ds << FPS_DS);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			FIELD_SEL_MASK,	cfg->field << FIELD_SEL);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			HFLIP_EN_MASK, cfg->flip.hflip_en << HFLIP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			VFLIP_EN_MASK, cfg->flip.vflip_en << VFLIP_EN);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_CFG_REG_OFF,
			OUTPUT_FMT_MASK, cfg->fmt << OUTPUT_FMT);

}
void csic_dma_output_size_cfg(unsigned int sel, struct dma_output_size *size)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_HSIZE_REG_OFF,
			HOR_START_MASK,	size->hor_start << HOR_START);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_HSIZE_REG_OFF,
			HOR_LEN_MASK, size->hor_len << HOR_LEN);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_VSIZE_REG_OFF,
			VER_START_MASK, size->ver_start << VER_START);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_VSIZE_REG_OFF,
			VER_LEN_MASK, size->ver_len << VER_LEN);
}
void csic_dma_buffer_address(unsigned int sel, enum fifo_buf_sel buf,
				unsigned long addr)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_F0_BUFA_REG_OFF + buf * 4,
			F0_BUFA_MASK, addr >> ADDR_BIT_R_SHIFT);
}
void csic_dma_buffer_length(unsigned int sel, struct dma_buf_len *buf_len)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_BUF_LEN_REG_OFF,
			BUF_LEN_MASK, buf_len->buf_len_y << BUF_LEN);
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_BUF_LEN_REG_OFF,
			BUF_LEN_C_MASK,	buf_len->buf_len_c << BUF_LEN_C);

}
void csic_dma_flip_size(unsigned int sel, struct dma_flip_size *flip_size)
{
	vin_reg_clr_set(csic_dma_base[sel] + CSIC_DMA_FLIP_SIZE_REG_OFF,
			VALID_LEN_MASK, flip_size->hor_len << VALID_LEN);
	vin_reg_clr_set(csic_dma_base[sel]+CSIC_DMA_FLIP_SIZE_REG_OFF,
			VER_LEN_MASK, flip_size->ver_len << VER_LEN);
}

void csic_dma_cap_status(unsigned int sel, struct dma_capture_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_CAP_STA_REG_OFF);
	status->scap_sta = (reg_val & SCAP_STA_MASK) >> SCAP_STA;
	status->vcap_sta = (reg_val & VCAP_STA_MASK) >> VCAP_STA;
	status->field_sta = (reg_val & FIELD_STA_MASK) >> FIELD_STA;
}

void csic_dma_int_enable(unsigned int sel,	enum dma_int_sel interrupt)
{
	vin_reg_set(csic_dma_base[sel] + CSIC_DMA_INT_EN_REG_OFF, interrupt);
}
void csic_dma_int_disable(unsigned int sel, enum dma_int_sel interrupt)
{
	vin_reg_clr(csic_dma_base[sel] + CSIC_DMA_INT_EN_REG_OFF, interrupt);
}
void csic_dma_int_get_status(unsigned int sel, struct dma_int_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_dma_base[sel] + CSIC_DMA_INT_STA_REG_OFF);
	status->capture_done = (reg_val & CD_PD_MASK) >> CD_PD;
	status->frame_done = (reg_val & FD_PD) >> FD_PD_MASK;
	status->buf_0_overflow = (reg_val & FIFO0_OF_PD_MASK) >> FIFO0_OF_PD;
	status->buf_1_overflow = (reg_val & FIFO1_OF_PD_MASK) >> FIFO1_OF_PD;
	status->buf_2_overflow = (reg_val & FIFO2_OF_PD_MASK) >> FIFO2_OF_PD;
	status->line_cnt_flag = (reg_val & LC_INT_EN_MASK) >> LC_INT_EN;
	status->hblank_overflow = (reg_val & HB_OF_INT_EN_MASK) >> HB_OF_INT_EN;
	status->vsync_trig = (reg_val & VS_INT_EN_MASK) >> VS_INT_EN;
}
void csic_dma_int_clear_status(unsigned int sel, enum dma_int_sel interrupt)
{
	vin_reg_writel(csic_dma_base[sel] + CSIC_DMA_INT_STA_REG_OFF, interrupt);
}


