// SPDX-License-Identifier: GPL-2.0
/*
 * hw_isp.c - isp top hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "hw_reg.h"
#include "hw_ccic.h"

void hw_ccic_set_irq_enable(struct spm_camera_block *sc_block, unsigned int enable, unsigned int disable)
{
	unsigned int value = 0;

	value = read32(sc_block->base_addr + REG_IRQMASK);
	value &= ~disable;
	value |= enable;
	write32(sc_block->base_addr + REG_IRQMASK, value);
}

unsigned int hw_ccic_get_irq_status(struct spm_camera_block *sc_block)
{
	unsigned int value = 0;

	value = read32(sc_block->base_addr + REG_IRQSTAT);
	return value;
}

void hw_ccic_clr_irq_status(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(sc_block->base_addr + REG_IRQSTAT, clr);
}

void hw_ccic_set_trig_line_num(struct spm_camera_block *sc_block, unsigned int trig_line_num)
{
	unsigned int value = 0;

	value = read32(sc_block->base_addr + REG_IDI_TRIG_LINE_NUM);
	value &= ~IDI_LINE_NUM_MASK;
	trig_line_num &= IDI_LINE_NUM_MASK;
	value |= trig_line_num;
	write32(sc_block->base_addr + REG_IDI_TRIG_LINE_NUM, value);
}

void hw_ccic_set_trig_src(struct spm_camera_block *sc_block, int src)
{
	unsigned int value = 0;

	value = read32(sc_block->base_addr + REG_IDI_TRIG_LINE_NUM);
	if (src)
		value |= IDI_LINE_TRIG_SRC;
	else
		value &= ~IDI_LINE_TRIG_SRC;
	write32(sc_block->base_addr + REG_IDI_TRIG_LINE_NUM, value);
}
