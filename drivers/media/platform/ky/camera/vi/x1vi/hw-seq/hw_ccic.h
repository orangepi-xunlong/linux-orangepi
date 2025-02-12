// SPDX-License-Identifier: GPL-2.0
/*
 * hw_dma.h - isp front end dma hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _HW_CCIC_H_
#define _HW_CCIC_H_
#include "../../cam_block.h"

#define REG_IRQMASK		(0x2c)	/* IRQ mask - same bits as IRQSTAT */
#define REG_IRQSTAT		(0x30)	/* IRQ status / clear */
#define REG_IDI_TRIG_LINE_NUM	(0x330)
#define IRQ_IPE_IDI_PRO_LINE	(1 << 8)
#define IDI_LINE_TRIG_SRC	(1 << 15)
#define IDI_LINE_NUM_MASK	(IDI_LINE_TRIG_SRC - 1)

void hw_ccic_set_irq_enable(struct spm_camera_block *sc_block, unsigned int enable, unsigned int disable);
unsigned int hw_ccic_get_irq_status(struct spm_camera_block *sc_block);
void hw_ccic_set_trig_line_num(struct spm_camera_block *sc_block, unsigned int trig_line_num);
void hw_ccic_set_trig_src(struct spm_camera_block *sc_block, int src);
void hw_ccic_clr_irq_status(struct spm_camera_block *sc_block, unsigned int clr);

#endif
