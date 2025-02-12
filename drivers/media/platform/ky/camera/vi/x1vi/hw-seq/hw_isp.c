// SPDX-License-Identifier: GPL-2.0
/*
 * hw_isp.c - isp top hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "hw_reg.h"
#include "hw_isp.h"
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>

#define CAM_ALIGN(a, b)		({ \
		unsigned int ___tmp1 = (a); \
		unsigned int ___tmp2 = (b); \
		unsigned int ___tmp3 = ___tmp1 % ___tmp2; \
		___tmp1 /= ___tmp2; \
		if (___tmp3) \
			___tmp1++; \
		___tmp1 *= ___tmp2; \
		___tmp1; \
	})

#define ISP_TOP_REG		TOP_REG

#define PIPE_ID(sc_block)	({ \
		unsigned int ___tmp1 = 0; \
		unsigned long ___addr = (sc_block)->base_addr; \
		___addr &= 0x0000ffff; \
		if (___addr >= KY_ISP_TOP1_OFFSET) \
			___tmp1 = 1; \
		___tmp1; \
	})

void hw_isp_top_set_idi_linebuf(struct spm_camera_block *sc_block,
				unsigned int fifo_depth,
				unsigned int line_depth, unsigned int pix_depth)
{
	union isp_top_reg_3 reg_3;
	union isp_top_reg_4 reg_4;

	reg_3.value = reg_4.value = 0;

	reg_3.field.idi_fifo_depth = fifo_depth;
	reg_3.field.idi_line_depth = line_depth;
	write32(ISP_TOP_REG(3), reg_3.value);
	reg_4.field.idi_pix_depth = pix_depth;
	reg_4.field.idi_fifo_line_th = 2;
	write32(ISP_TOP_REG(4), reg_4.value);
}

unsigned int hw_isp_top_get_idi_fifo_depth(struct spm_camera_block *sc_block)
{
	union isp_top_reg_3 reg_3;

	reg_3.value = read32(ISP_TOP_REG(3));
	return reg_3.field.idi_fifo_depth;
}

static void hw_isp_top_set_idi_input_crop(struct spm_camera_block *sc_block,
					  unsigned int crop_width_offset,
					  unsigned int crop_height_offset,
					  unsigned int crop_width,
					  unsigned int crop_height)
{
	union isp_top_reg_93 reg_93;
	union isp_top_reg_94 reg_94;

	reg_93.value = 0;
	reg_93.field.m_nwidth_offset = crop_width_offset;
	reg_93.field.m_nheight_offset = crop_height_offset;
	write32(ISP_TOP_REG(93), reg_93.value);
	reg_94.value = 0;
	reg_94.field.m_ncrop_width = crop_width;
	reg_94.field.m_ncrop_height = crop_height;
	write32(ISP_TOP_REG(94), reg_94.value);
}

void hw_isp_top_set_cfg_rdy(struct spm_camera_block *sc_block, unsigned int ready)
{
	union isp_top_reg_1 reg_1;

	reg_1.value = read32(ISP_TOP_REG(1));
	reg_1.field.m_cfg_ready = ready;
	write32(ISP_TOP_REG(1), reg_1.value);
}

int hw_isp_top_set_idi_online_input_fmt(struct spm_camera_block *sc_block,
					unsigned int width,
					unsigned int height, int cfa_pattern)
{
	union isp_top_reg_0 reg_0;
	union isp_top_reg_1 reg_1;

	reg_0.value = 0;
	reg_0.field.m_nwidth = width;
	reg_0.field.m_nheight = height;
	write32(ISP_TOP_REG(0), reg_0.value);
	hw_isp_top_set_idi_input_crop(sc_block, 0, 0, width, height);

	reg_1.value = read32(ISP_TOP_REG(1));
	if (cfa_pattern != CFA_IGNR)
		reg_1.field.m_ncfapattern = cfa_pattern;
	//reg_1.field.m_cfg_ready = 1;
	write32(ISP_TOP_REG(1), reg_1.value);

	return 0;
}

int hw_isp_top_set_idi_offline_input_fmt(struct spm_camera_block *sc_block,
					 unsigned int rdma_ch,
					 unsigned int width,
					 unsigned int height,
					 int cfa_pattern, unsigned int bit_depth)
{
	//union isp_top_reg_0 reg_0;
	//union isp_top_reg_1 reg_1;
	union isp_top_reg_21 reg_21;
	union isp_top_reg_22 reg_22;
	unsigned int pixel_align = 128 / bit_depth;
	unsigned int pipe_idx = PIPE_ID(sc_block);

	//reg_0.value = 0;
	//reg_0.field.m_nwidth = width;
	//reg_0.field.m_nheight = height;
	//write32(ISP_TOP_REG(0), reg_0.value);

	hw_isp_top_set_idi_input_crop(sc_block, 0, 0, width, height);

	//reg_1.value = read32(ISP_TOP_REG(1));
	//if (cfa_pattern != CFA_IGNR)
	//	reg_1.field.m_ncfapattern = cfa_pattern;
	//write32(ISP_TOP_REG(1), reg_1.value);

	reg_21.value = 0;
	reg_21.field.idi_ch0_rd_burst_len_sel = 1;
	reg_21.field.idi_ch0_rd_fifo_depth = 0;
	reg_21.field.idi_ch0_dma_sync_fifo_th = 4;
	reg_21.field.idi_ch0_img_rd_width_byte = (CAM_ALIGN(width, pixel_align) / pixel_align) * 16;
	write32(ISP_TOP_REG(21 + rdma_ch * 2) - pipe_idx * KY_PIPE_OFFSET, reg_21.value);

	reg_22.value = 0;
	reg_22.field.idi_ch0_img_rd_raw1214_type = (bit_depth == 12) ? 1 : 0;
	reg_22.field.idi_ch0_img_rd_height = height;
	reg_22.field.idi_ch0_img_rd_raw10_type = (bit_depth == 10) ? 1 : 0;
	reg_22.field.idi_ch0_img_rd_raw8_type = (bit_depth == 8) ? 1 : 0;
	reg_22.field.idi_ch0_img_rd_width_pix = width;
	write32(ISP_TOP_REG(22 + rdma_ch * 2) - pipe_idx * KY_PIPE_OFFSET, reg_22.value);
	return 0;
}

void hw_isp_top_set_idi_dummyline(struct spm_camera_block *sc_block,
				  unsigned int idi_insert_dummy_line)
{
	union isp_top_reg_14 reg_14;

	reg_14.value = read32(ISP_TOP_REG(14));
	//reg_14.field.idi_gap_value = 7 * width / 93;
	reg_14.field.idi_gap_value = 0xc8;
	//reg_14.field.idi_fifo_pix_th = 0;
	reg_14.field.idi_insert_dummy_line = idi_insert_dummy_line;
	write32(ISP_TOP_REG(14), reg_14.value);
}

void hw_isp_top_enable_hw_gap(struct spm_camera_block *sc_block, int pipe_id,
			      unsigned int enable)
{
	union isp_top_reg_14 reg_14;
	union isp_top_reg_76 reg_76;

	if (pipe_id == 0) {
		reg_14.value = read32(ISP_TOP_REG(14));
		reg_14.field.gap_hardware_mode = enable;
		write32(ISP_TOP_REG(14), reg_14.value);
	} else {
		reg_76.value = read32(ISP_TOP_REG(76));
		reg_76.field.pip1_gap_hardware_mode = enable;
		write32(ISP_TOP_REG(76), reg_76.value);
	}
}

void hw_isp_top_enable_vsync_pass_through(struct spm_camera_block *sc_block,
					  int pipe_id, unsigned int enable)
{
	union isp_top_reg_25 reg_25;

	reg_25.value = read32(ISP_TOP_REG(25));
	if (pipe_id == 0)
		reg_25.field.pip0_vsync_pass_through = enable;
	else
		reg_25.field.pip1_vsync_pass_through = enable;
	write32(ISP_TOP_REG(25), reg_25.value);
}

void hw_isp_top_set_vsync2href_dly_cnt(struct spm_camera_block *sc_block, int pipe_id,
				       unsigned int dly_cnt)
{
	union isp_top_reg_26 reg_26;

	reg_26.value = read32(ISP_TOP_REG(26));
	if (pipe_id == 0)
		reg_26.field.pip0_vsync2href_dly_cnt = dly_cnt;
	else
		reg_26.field.pip1_vsync2href_dly_cnt = dly_cnt;
	write32(ISP_TOP_REG(26), reg_26.value);
}

/*
void hw_isp_top_set_vsync2href_dly_cnt(struct spm_camera_block *sc_block, unsigned int p0_dly_cnt, unsigned int p1_dly_cnt)
{
	union isp_top_reg_26 reg_26;
	reg_26.value = 0;
	reg_26.field.pip0_vsync2href_dly_cnt = p0_dly_cnt;
	reg_26.field.pip1_vsync2href_dly_cnt = p1_dly_cnt;
	write32(ISP_TOP_REG(26), reg_26.value);
}
*/
void hw_isp_top_set_rawdump_fmt(struct spm_camera_block *sc_block,
				unsigned int idi_wdma_ch,
				unsigned int width,
				unsigned int height, unsigned int bit_depth)
{
	union isp_top_reg_17 reg_17;
	union isp_top_reg_18 reg_18;
	unsigned int n = 128 / bit_depth;

	reg_17.value = read32(ISP_TOP_REG(17 + idi_wdma_ch * 2));
	reg_17.field.idi_ch0_wr_burst_len_sel = 4;	//0:64byte 1:128byte 2:256byte 3:512byte 4:1024byte
	reg_17.field.idi_ch0_wr_14bit_en = (bit_depth == 14) ? 1 : 0;
	reg_17.field.idi_ch0_img_wr_width_byte =
	    CAM_ALIGN((((width % n) * bit_depth) >> 3) + ((width / n) << 4), 16);
	write32(ISP_TOP_REG(17 + idi_wdma_ch * 2), reg_17.value);

	reg_18.value = 0;
	reg_18.field.idi_ch0_img_wr_height = height;
	reg_18.field.idi_ch0_img_wr_width_pix = width;
	write32(ISP_TOP_REG(18 + idi_wdma_ch * 2), reg_18.value);
}

void hw_isp_top_set_idi_input_source(struct spm_camera_block *sc_block, int source)
{
	union isp_top_reg_13 reg_13;

	reg_13.value = read32(ISP_TOP_REG(13));
	if (source >= SENSOR0_CH0 && source <= OFFLINE_CH1) {
		reg_13.field.idi_src_sel = 1 << source;
		reg_13.field.idi_online_ena = (source >= SENSOR0_CH0 && source <= SENSOR1_CH3) ? 1 : 0;
		reg_13.field.idi_offline_ena = (reg_13.field.idi_online_ena) ? 0 : 1;
	} else {
		reg_13.field.idi_src_sel = 0;
		//reg_13.field.idi_online_ena = 1;
		//reg_13.field.idi_offline_ena = 0;
	}
	write32(ISP_TOP_REG(13), reg_13.value);
}

void hw_isp_top_enable_rawdump(struct spm_camera_block *sc_block,
			       int enable, int rawdump_only)
{
	union isp_top_reg_13 reg_13;

	reg_13.value = read32(ISP_TOP_REG(13));
	if (enable) {
		reg_13.field.idi_rdp_ena = 1;
		if (rawdump_only) {
			reg_13.field.idi_offline_ena = 1;
			reg_13.field.idi_online_ena = 0;
		}
	} else {
		reg_13.field.idi_rdp_ena = 0;
		if (rawdump_only) {
			reg_13.field.idi_offline_ena = 0;
			reg_13.field.idi_online_ena = 0;
		}
	}
	write32(ISP_TOP_REG(13), reg_13.value);
}

void hw_isp_top_set_rawdump_source(struct spm_camera_block *sc_block,
				   unsigned int rawdump_idx, int source)
{
	union isp_top_reg_17 reg_17;

	reg_17.value = read32(ISP_TOP_REG(17 + rawdump_idx * 2));
	if (source >= SENSOR0_CH0 && source <= SENSOR1_CH3)
		reg_17.field.idi_wdma_ch0_src_sel = 1 << source;
	else
		reg_17.field.idi_wdma_ch0_src_sel = 0;
	write32(ISP_TOP_REG(17 + rawdump_idx * 2), reg_17.value);
}

void hw_isp_top_enable_hdr(struct spm_camera_block *sc_block, int hdr_mode)
{
	union isp_top_reg_76 reg_76;

	reg_76.value = read32(ISP_TOP_REG(76));
	reg_76.field.idi_online_hdr_ena = 0;
	reg_76.field.idi_offline_hdr_ena = 0;
	reg_76.field.idi_mix_hdr_ena = 0;
	reg_76.field.mix_hdr_rdp_ena = 0;
	if (hdr_mode == HDR_ONLINE) {
		reg_76.field.idi_online_hdr_ena = 1;
	} else if (hdr_mode == HDR_OFFLINE) {
		reg_76.field.idi_offline_hdr_ena = 1;
	} else if (hdr_mode == HDR_MIX) {
		reg_76.field.idi_mix_hdr_ena = 1;
		reg_76.field.mix_hdr_rdp_ena = 1;
	}
	write32(ISP_TOP_REG(76), reg_76.value);
}

void hw_isp_top_enable_rd_outstanding(struct spm_camera_block *sc_block, unsigned int enable)
{
	union isp_top_reg_76 reg_76;

	reg_76.value = read32(ISP_TOP_REG(76));
	reg_76.field.outstanding_read_en = enable;
	write32(ISP_TOP_REG(76), reg_76.value);
}

void hw_isp_top_set_mix_hdr_line(struct spm_camera_block *sc_block, unsigned int mix_hdr_line)
{
	union isp_top_reg_76 reg_76;

	mix_hdr_line = 4;
	reg_76.value = read32(ISP_TOP_REG(76));
	reg_76.field.idi_mix_hdr_line = mix_hdr_line;
	write32(ISP_TOP_REG(76), reg_76.value);
}

void hw_isp_top_set_ddr_wr_line(struct spm_camera_block *sc_block, unsigned int ddr_wr_line_cnt)
{
	union isp_top_reg_76 reg_76;

	reg_76.value = read32(ISP_TOP_REG(76));
	reg_76.field.idi_ddr_wr_line_cnt = ddr_wr_line_cnt;
	write32(ISP_TOP_REG(76), reg_76.value);
}

void hw_isp_pwr(unsigned int on)
{
}

unsigned int hw_isp_top_get_irq_status(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(ISP_TOP_REG(32));
	return val;
}

void hw_isp_top_clr_irq_status(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(ISP_TOP_REG(32), clr);
}

unsigned int hw_isp_top_get_irq_raw_status(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(ISP_TOP_REG(34));
	return val;
}

void hw_isp_top_set_irq_enable(struct spm_camera_block *sc_block, unsigned int enable,
			       unsigned int disable)
{
	unsigned int value = 0;

	value = read32(ISP_TOP_REG(33));
	value &= ~disable;
	value |= enable;
	write32(ISP_TOP_REG(33), value);
}

void hw_isp_top_set_gap_value(struct spm_camera_block *sc_block, unsigned int p0_gap,
			      unsigned int p1_gap, unsigned int p01_gap)
{
	union isp_top_reg_77 reg_77;
	union isp_top_reg_78 reg_78;

	reg_77.field.idi_pip0_gap_value = p0_gap;
	reg_77.field.idi_pip1_gap_value = p1_gap;
	reg_78.value = read32(ISP_TOP_REG(78));
	reg_78.field.idi_pip01_gap_value = p01_gap;
	write32(ISP_TOP_REG(77), reg_77.value);
	write32(ISP_TOP_REG(78), reg_78.value);
}

void hw_isp_top_set_idi_rd_burst_len(struct spm_camera_block *sc_block,
				     unsigned int rdma_ch,
				     unsigned int speed_cnt, unsigned int bstlen_sel)
{
	union isp_top_reg_82 reg_82;

	reg_82.value = read32(ISP_TOP_REG(82 + rdma_ch));
	reg_82.field.idi_pip0_rd_speed_end_cnt = speed_cnt;
	reg_82.field.idi_ch0_rd_bst_len_sel = bstlen_sel;
	write32(ISP_TOP_REG(82 + rdma_ch), reg_82.value);
}

void hw_isp_top_set_err0_irq_enable(struct spm_camera_block *sc_block,
				    unsigned int enable, unsigned int disable)
{
	unsigned int value = 0;

	value = read32(ISP_TOP_REG(39));
	value &= ~disable;
	value |= enable;
	write32(ISP_TOP_REG(39), value);
}

void hw_isp_top_set_err2_irq_enable(struct spm_camera_block *sc_block,
				    unsigned int enable, unsigned int disable)
{
	unsigned int value = 0;

	value = read32(ISP_TOP_REG(41));
	value &= ~disable;
	value |= enable;
	write32(ISP_TOP_REG(41), value);
}

unsigned int hw_isp_top_get_err0_irq_status(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(ISP_TOP_REG(36));
	return val;
}

void hw_isp_top_clr_err0_irq_status(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(ISP_TOP_REG(36), clr);
}

unsigned int hw_isp_top_get_err1_irq_status(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(ISP_TOP_REG(37));
	return val;
}

unsigned int hw_isp_top_get_err2_irq_status(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(ISP_TOP_REG(38));
	return val;
}

void hw_isp_top_clr_err1_irq_status(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(ISP_TOP_REG(37), clr);
}

void hw_isp_top_clr_err2_irq_status(struct spm_camera_block *sc_block, unsigned int clr)
{
	write32(ISP_TOP_REG(38), clr);
}

void hw_isp_top_config_tpg(struct spm_camera_block *sc_block,
			   unsigned int pipe_id,
			   unsigned int rolling,
			   unsigned int dummy_line,
			   unsigned int hblank, unsigned int vblank)
{
	union isp_top_reg_15 reg_15;
	union isp_top_reg_16 reg_16;

	reg_15.value = 0;
	reg_15.field.sensor_timing_en = 0;
	reg_15.field.rolling_en = rolling;
	reg_15.field.tpg_select = pipe_id;
	reg_15.field.hblank = hblank;	//3218;
	reg_15.field.dummy_line = dummy_line;

	reg_16.value = 0;
	reg_16.field.vblank = vblank;
	reg_16.field.valid_blank = 0x0a;

	write32(ISP_TOP_REG(16), reg_16.value);
	write32(ISP_TOP_REG(15), reg_15.value);
}

void hw_isp_top_enable_tpg(struct spm_camera_block *sc_block, unsigned int enable)
{
	union isp_top_reg_15 reg_15;

	reg_15.value = read32(ISP_TOP_REG(15));
	reg_15.field.tpg_en = enable;

	write32(ISP_TOP_REG(15), reg_15.value);
}

void hw_isp_top_enable_debug_clk(struct spm_camera_block *sc_block, unsigned int enable)
{
	union isp_top_reg_75 reg_75;

	reg_75.value = read32(ISP_TOP_REG(75));
	reg_75.field.debug_clk_en = enable;

	write32(ISP_TOP_REG(75), reg_75.value);
}

void hw_isp_top_set_posterr_irq_enable(struct spm_camera_block *sc_block,
				       unsigned int enable, unsigned int disable)
{
	unsigned int value = 0;

	value = read32(ISP_TOP_REG(62));
	value &= ~disable;
	value |= enable;
	write32(ISP_TOP_REG(62), value);
}

unsigned int hw_isp_top_get_posterr_irq_status(struct spm_camera_block *sc_block)
{
	unsigned int val = 0;

	val = read32(ISP_TOP_REG(61));
	return val;
}

void hw_isp_top_clr_posterr_irq_status(struct spm_camera_block *sc_block,
				       unsigned int clr)
{
	write32(ISP_TOP_REG(61), clr);
}

void hw_isp_top_shadow_latch(struct spm_camera_block *sc_block)
{
	union isp_top_reg_1 reg_1;

	reg_1.value = read32(ISP_TOP_REG(1));
	reg_1.field.idi_reg_latch_trig_pip = 1;
	write32(ISP_TOP_REG(1), reg_1.value);
}

void hw_isp_top_set_rdp_cfg_rdy(struct spm_camera_block *sc_block,
				unsigned int rawdump_id, unsigned int ready)
{
	union isp_top_reg_17 reg_17;

	reg_17.value = read32(ISP_TOP_REG(17 + rawdump_id * 2));
	reg_17.field.rdp0_cfg_ready = ready;
	write32(ISP_TOP_REG(17 + rawdump_id * 2), reg_17.value);
}

void hw_isp_top_global_reset(struct spm_camera_block *sc_block)
{
	union isp_top_reg_86 reg_86;

	reg_86.value = read32(ISP_TOP_REG(86));
	reg_86.field.global_reset = 1;
	write32(ISP_TOP_REG(86), reg_86.value);
}

void hw_isp_top_pipe0_debug_dump(struct spm_camera_block *sc_block)
{
	union isp_top_reg_0 reg_0;
	union isp_top_reg_14 reg_14;

	reg_0.value = read32(ISP_TOP_REG(0));
	reg_14.value = read32(ISP_TOP_REG(14));
	cam_not("p0 regs dump: reg0.m_nWidth=%d reg0.m_nHeight=%d reg14.dummy_line=%d",
		reg_0.field.m_nwidth, reg_0.field.m_nheight,
		reg_14.field.idi_insert_dummy_line);
}

void hw_isp_top_pipe1_debug_dump(struct spm_camera_block *sc_block)
{
	union isp_top_reg_0 reg_0;

	reg_0.value = read32(ISP_TOP_REG(0));
	cam_not("p1 regs dump: reg0.m_nWidth=%d reg0.m_nHeight=%d",
		reg_0.field.m_nwidth, reg_0.field.m_nheight);
}
void hw_isp_top_set_speed_ctrl(struct spm_camera_block *sc_block, unsigned int speed_ctrl)
{
	union isp_top_reg_76 reg_76;

	reg_76.value = read32(ISP_TOP_REG(76));
	reg_76.field.send_speed_ctrl = speed_ctrl;
	write32(ISP_TOP_REG(76), reg_76.value);
}