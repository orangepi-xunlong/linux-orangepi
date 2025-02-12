// SPDX-License-Identifier: GPL-2.0
/*
 * hw_dma.c - isp front end dma hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "hw_reg.h"
#include "hw_dma.h"
#include <linux/delay.h>

#define DMA_REG		TOP_REG

void hw_dma_set_wdma_pitch(struct spm_camera_block *sc_block,
			   unsigned int wdma_ch,
			   unsigned int num_plane,
			   unsigned int p0_pitch, unsigned int p1_pitch)
{
	union dma_reg_38 reg_38;
	reg_38.value = 0;

	reg_38.field.ch0_wr_pitch0 = p0_pitch;
	if (num_plane > 1)
		reg_38.field.ch0_wr_pitch1 = p1_pitch;
	write32(DMA_REG(38 + wdma_ch), reg_38.value);
}

void hw_dma_set_rdma_pitch(struct spm_camera_block *sc_block,
			   unsigned int rdma_ch, unsigned int pitch)
{
	union dma_reg_79 reg_79;

	reg_79.value = read32(DMA_REG(79 + rdma_ch));
	reg_79.field.ch0_rd_pitch = pitch;
	reg_79.field.ch0_rd_trigger = 0;
	write32(DMA_REG(79 + rdma_ch), reg_79.value);
}

void hw_dma_update_rdma_address(struct spm_camera_block *sc_block,
				unsigned int rdma_ch, uint64_t buf_addr)
{
	unsigned int low = 0, high = 0;

	low = (unsigned int)(buf_addr & 0xffffffffUL);
	high = (unsigned int)((buf_addr >> 32) & 0x3UL);
	write32(DMA_REG(75 + rdma_ch), low);
	write32(DMA_REG(124 + rdma_ch), high);
}

/*
void hw_dma_set_rdma_weight(struct spm_camera_block *sc_block,
							unsigned int rdma_ch,
							unsigned int weight)
{
	union dma_reg_79 reg_79;

	reg_79.value = read32(DMA_REG(79 + rdma_ch));
	reg_79.field.ch0_rd_weight = weight;
	reg_79.field.ch0_rd_trigger = 0;
	write32(DMA_REG(79 + rdma_ch), reg_79.value);
}
*/
void hw_dma_rdma_trigger(struct spm_camera_block *sc_block, unsigned int rdma_ch)
{
	union dma_reg_79 reg_79;

	reg_79.value = read32(DMA_REG(79 + rdma_ch));
	reg_79.field.ch0_rd_trigger = 1;
	reg_79.field.ch0_rd_weight = 0;
	write32(DMA_REG(79 + rdma_ch), reg_79.value);
}

static void hw_dma_set_wdma_burst_length(struct spm_camera_block *sc_block,
					 unsigned char burst_len)
{
	union dma_reg_68 reg_68;

	reg_68.value = read32(DMA_REG(68));
	reg_68.field.wr_burst_length = burst_len;
	write32(DMA_REG(68), reg_68.value);
}

void hw_dma_reset(struct spm_camera_block *sc_block)
{
	hw_dma_set_wdma_burst_length(sc_block, 0x30);
}

void hw_dma_enable_rawdump(struct spm_camera_block *sc_block, int rawdump_id,
			   unsigned int enable)
{
	union dma_reg_83 reg_83;

	reg_83.value = read32(DMA_REG(83));
	if (rawdump_id == 0)
		reg_83.field.rawdump0_enable = enable;
	else
		reg_83.field.rawdump1_enable = enable;
	write32(DMA_REG(83), reg_83.value);
}

void hw_dma_enable_afbc(struct spm_camera_block *sc_block, int afbc_id,
			unsigned int enable)
{
	union dma_reg_83 reg_83;

	reg_83.value = read32(DMA_REG(83));
	if (afbc_id == 0)
		reg_83.field.afbc0_enable = enable;
	else
		reg_83.field.afbc1_enable = enable;
	write32(DMA_REG(83), reg_83.value);
}

void hw_dma_set_wdma_weight(struct spm_camera_block *sc_block, unsigned int wdma_ch,
			    unsigned int wr_weight)
{
	union dma_reg_52 reg_52;

	reg_52.value = read32(DMA_REG(52 + wdma_ch));
	reg_52.field.ch0_wr_weight = wr_weight;
	write32(DMA_REG(52 + wdma_ch), reg_52.value);
}

void hw_dma_set_wdma_source(struct spm_camera_block *sc_block,
			    unsigned int wdma_ch,
			    int source,
			    unsigned int wr_offset,
			    unsigned int wr_fifo_depth,
			    unsigned int wr_weight, unsigned int div_mode)
{
	union dma_reg_52 reg_52;
	reg_52.value = read32(DMA_REG(52 + wdma_ch));

	reg_52.field.ch0_wr_weight = wr_weight;
	reg_52.field.ch0_fifo_div_mode = div_mode;
	reg_52.field.ch0_wr_ready = 0;
	write32(DMA_REG(52 + wdma_ch), reg_52.value);
}

void hw_dma_set_wdma_ready(struct spm_camera_block *sc_block,
			   unsigned int wdma_ch, unsigned int ready)
{
	union dma_reg_52 reg_52;

	reg_52.value = read32(DMA_REG(52 + wdma_ch));
	reg_52.field.ch0_wr_ready = ready;
	write32(DMA_REG(52 + wdma_ch), reg_52.value);
}

void hw_dma_update_wdma_address(struct spm_camera_block *sc_block,
				unsigned int wdma_ch,
				uint64_t p0_addr, uint64_t p1_addr)
{
	unsigned int low = 0, high = 0;

	low = (unsigned int)(p0_addr & 0xffffffffUL);
	high = (unsigned int)((p0_addr >> 32) & 0x3UL);
	write32(DMA_REG(0 + wdma_ch * 2), low);
	write32(DMA_REG(86 + wdma_ch * 2), high);
	low = (unsigned int)(p1_addr & 0xffffffffUL);
	high = (unsigned int)((p1_addr >> 32) & 0x3UL);
	write32(DMA_REG(1 + wdma_ch * 2), low);
	write32(DMA_REG(87 + wdma_ch * 2), high);
}

unsigned int hw_dma_get_irq_status1(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(DMA_REG(69));
	return val;
}

void hw_dma_clr_irq_status1(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(DMA_REG(69), clr);
}

unsigned int hw_dma_get_irq_status2(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(DMA_REG(70));
	return val;
}

void hw_dma_clr_irq_status2(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(DMA_REG(70), clr);
}

unsigned int hw_dma_get_irq_raw_status1(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(DMA_REG(71));
	return val;
}

unsigned int hw_dma_get_irq_raw_status2(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(DMA_REG(72));
	return val;
}

void __hw_dma_set_irq_enable(int irq_src,
			     unsigned int enable,
			     unsigned int disable,
			     unsigned int *value1, unsigned int *value2)
{
	unsigned int val = 0;
	union dma_reg_69 *reg_1 = NULL;
	union dma_reg_70 *reg_2 = NULL;

	if (irq_src >= DMA_IRQ_SRC_WDMA_CH0 && irq_src <= DMA_IRQ_SRC_WDMA_CH9) {
		val = ~(disable << ((irq_src - DMA_IRQ_SRC_WDMA_CH0) * 3));
		*value1 &= val;
		val = (enable << ((irq_src - DMA_IRQ_SRC_WDMA_CH0) * 3));
		*value1 |= val;
	} else if (irq_src == DMA_IRQ_SRC_WDMA_CH10) {
		reg_1 = (union dma_reg_69 *)value1;
		reg_2 = (union dma_reg_70 *)value2;
		if (disable & DMA_IRQ_START)
			reg_1->field.ch10_wr_start = 0;
		if (disable & DMA_IRQ_DONE)
			reg_1->field.ch10_wr_done = 0;
		if (disable & DMA_IRQ_ERR)
			reg_2->field.ch10_wr_err = 0;
		if (enable & DMA_IRQ_START)
			reg_1->field.ch10_wr_start = 1;
		if (enable & DMA_IRQ_DONE)
			reg_1->field.ch10_wr_done = 1;
		if (enable & DMA_IRQ_ERR)
			reg_2->field.ch10_wr_err = 1;
	} else if (irq_src >= DMA_IRQ_SRC_WDMA_CH11 && irq_src <= DMA_IRQ_SRC_RDMA_CH2) {
		val = ~(disable << ((irq_src - DMA_IRQ_SRC_WDMA_CH11) * 3 + 1));
		*value2 &= val;
		val = (enable << ((irq_src - DMA_IRQ_SRC_WDMA_CH11) * 3 + 1));
		*value2 |= val;
	} else {
		pr_err ("set irq src %d no support", irq_src);
	}
}

void hw_dma_set_irq_enable(struct spm_camera_block *sc_block, int irq_src,
			   unsigned int enable, unsigned int disable)
{
	unsigned int value1 = 0, value1_old = 0, value2 = 0, value2_old = 0;
	if (irq_src < DMA_IRQ_SRC_ALL || irq_src > DMA_IRQ_SRC_RDMA_CH2)
		return;
	disable &= 0x07;
	enable &= 0x07;
	value1 = value1_old = read32(DMA_REG(73));
	value2 = value2_old = read32(DMA_REG(74));
	if (irq_src == DMA_IRQ_SRC_ALL)
		for (irq_src = DMA_IRQ_SRC_WDMA_CH0; irq_src <= DMA_IRQ_SRC_RDMA_CH2; irq_src++)
			__hw_dma_set_irq_enable(irq_src, enable, disable, &value1, &value2);
	else
		__hw_dma_set_irq_enable(irq_src, enable, disable, &value1, &value2);

	if (value1 || (value2 & ((1 << 19) - 1)))
		value2 |= (DMA_IRQ_OVERRUN | DMA_IRQ_OVERLAP);
	else
		value2 &= ~(DMA_IRQ_OVERRUN | DMA_IRQ_OVERLAP);
	if (value1 != value1_old)
		write32(DMA_REG(73), value1);
	if (value2 != value2_old)
		write32(DMA_REG(74), value2);
}

void hw_dma_set_fbc_irq_enable(struct spm_camera_block *sc_block, unsigned int enable,
			       unsigned int disable)
{
	unsigned int value = 0;

	enable &= (DMA_IRQ_FBC_ENC0 | DMA_IRQ_FBC_ENC1);
	disable &= (DMA_IRQ_FBC_ENC0 | DMA_IRQ_FBC_ENC1);
	value = read32(DMA_REG(74));
	value &= ~disable;
	value |= enable;
	write32(DMA_REG(74), value);
}

unsigned int hw_dma_irq_analyze(int irq_src, unsigned int status1, unsigned int status2)
{
	unsigned int val = 0;
	union dma_reg_69 *reg_1 = (union dma_reg_69 *)(&status1);
	union dma_reg_70 *reg_2 = (union dma_reg_70 *)(&status2);

	if (irq_src < DMA_IRQ_SRC_WDMA_CH0 || irq_src > DMA_IRQ_SRC_RDMA_CH2)
		return 0;
	if (irq_src >= DMA_IRQ_SRC_WDMA_CH0 && irq_src <= DMA_IRQ_SRC_WDMA_CH9) {
		val = status1 >> ((irq_src - DMA_IRQ_SRC_WDMA_CH0) * 3);
	} else if (irq_src == DMA_IRQ_SRC_WDMA_CH10) {
		val = reg_2->field.ch10_wr_err;
		val <<= 1;
		val += reg_1->field.ch10_wr_done;
		val <<= 1;
		val += reg_1->field.ch10_wr_start;
	} else if (irq_src >= DMA_IRQ_SRC_WDMA_CH11 && irq_src <= DMA_IRQ_SRC_RDMA_CH2) {
		val = status2 >> ((irq_src - DMA_IRQ_SRC_WDMA_CH11) * 3 + 1);
	}
	val &= 0x07;
	return val;
}

void hw_dma_trigger_pipe_tbu_load(struct spm_camera_block *sc_block, int pipe_id)
{
	union dma_reg_86 reg_86;

	reg_86.value = read32(DMA_REG(86));
	if (pipe_id == 0)
		reg_86.field.mmu_start_p0 = 1;
	else
		reg_86.field.mmu_start_p1 = 1;
	write32(DMA_REG(86), reg_86.value);
}

void hw_dma_trigger_rdp_tbu_load(struct spm_camera_block *sc_block, int rawdump_id)
{
	union dma_reg_86 reg_86;

	reg_86.value = read32(DMA_REG(86));
	if (rawdump_id == 0)
		reg_86.field.mmu_start_p0 = 1;
	else
		reg_86.field.mmu_start_p1 = 1;
	write32(DMA_REG(86), reg_86.value);
}

void hw_dma_dump_regs(struct spm_camera_block *sc_block)
{
	unsigned int val_69 = 0, val_70 = 0, val_71 = 0;
	unsigned int val_72 = 0, val_73 = 0, val_74 = 0;

	val_69 = read32(DMA_REG(69));
	val_70 = read32(DMA_REG(70));
	val_71 = read32(DMA_REG(71));
	val_72 = read32(DMA_REG(72));
	val_73 = read32(DMA_REG(73));
	val_74 = read32(DMA_REG(74));
	pr_info("cam_not: vi: dma reg_69=0x%08x reg_70=0x%08x reg_71=0x%08x reg_72=0x%08x reg_73=0x%08x reg_74=0x%08x\n",
		val_69, val_70, val_71, val_72, val_73, val_74);
}
