#include <vip/vi_drv.h>

/****************************************************************************
 * FBC_CONFIG
 ****************************************************************************/
#define DEFAULT_K	2
#define CPLX_SHIFT	3
#define PEN_POS_SHIFT	4
#define TARGET_CR	43//, 55, 68, 80, 93, 100

struct vi_fbc_cfg fbc_cfg = {
	.cu_size	= 8,
	.target_cr	= TARGET_CR,
	.is_lossless	= 0,
	.y_bs_size	= 0,
	.c_bs_size	= 0,
	.y_buf_size	= 0,
	.c_buf_size	= 0,
};

/***************************************************************************
 * CA global setting
 ***************************************************************************/
u8 ca_y_lut[] = {
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};

u8 cp_y_lut[] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

u8 cp_u_lut[] = {
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
};

u8 cp_v_lut[] = {
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
};

/*******************************************************************************
 *	YUV IPs config
 ******************************************************************************/

/**
 * ispblk_yuvdither_config - setup yuv dither.
 *
 * @param ctx: global settings
 * @param sel: y(0)/uv(1)
 * @param en: dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
int ispblk_yuvdither_config(struct isp_ctx *ctx, u8 sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en)
{
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUV_DITHER];

	if (sel == 0) {
		union reg_isp_yuv_dither_y_dither reg;

		reg.raw = 0;
		reg.bits.y_dither_enable = en;
		reg.bits.y_dither_mod_enable = mod_en;
		reg.bits.y_dither_histidx_enable = histidx_en;
		reg.bits.y_dither_fmnum_enable = fmnum_en;
		reg.bits.y_dither_shdw_sel = 1;
		reg.bits.y_dither_widthm1 = ctx->img_width - 1;
		reg.bits.y_dither_heightm1 = ctx->img_height - 1;

		ISP_WR_REG(dither, reg_isp_yuv_dither_t, y_dither, reg.raw);
	} else if (sel == 1) {
		union reg_isp_yuv_dither_uv_dither reg;

		reg.raw = 0;
		reg.bits.uv_dither_enable = en;
		reg.bits.uv_dither_mod_enable = mod_en;
		reg.bits.uv_dither_histidx_enable = histidx_en;
		reg.bits.uv_dither_fmnum_enable = fmnum_en;
		reg.bits.uv_dither_widthm1 = (ctx->img_width >> 1) - 1;
		reg.bits.uv_dither_heightm1 = (ctx->img_height >> 1) - 1;

		ISP_WR_REG(dither, reg_isp_yuv_dither_t, uv_dither, reg.raw);
	}

	return 0;
}

int ispblk_pre_ee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE_PRE];

	union reg_isp_preyee_00 reg_0;
	reg_0.raw = ISP_RD_REG(ee, reg_isp_preyee_t, reg_00);
	reg_0.bits.ee_enable = en;
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_00, reg_0.raw);

	ISP_WR_REG(ee, reg_isp_preyee_t, reg_200, 128);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_204, 1024);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_208, 256);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_20c, 1024);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_210, 64);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_214, 512);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_218, 717);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_21c, 512);
	ISP_WR_REG(ee, reg_isp_preyee_t, reg_220, 0);

	return 0;
}

void ispblk_tnr_config(struct isp_ctx *ctx, bool en, u8 test_case)
{
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	//uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	union reg_isp_444_422_8 tnr_8;
	union reg_isp_444_422_13 tnr_13;
	union reg_isp_444_422_14 tnr_14;
	union reg_isp_444_422_15 tnr_15;
	enum sop_isp_raw first_raw_num = vi_get_first_raw_num(ctx);

	if (en) {
		tnr_13.raw = 0;
		tnr_13.bits.reg_3dnr_y_lut_in_0 = 0;
		tnr_13.bits.reg_3dnr_y_lut_in_1 = 255;
		tnr_13.bits.reg_3dnr_y_lut_in_2 = 255;
		tnr_13.bits.reg_3dnr_y_lut_in_3 = 255;
		ISP_WR_REG(tnr, reg_isp_444_422_t, reg_13, tnr_13.raw);

		tnr_14.raw = 0;
		tnr_14.bits.reg_3dnr_y_lut_out_0 = 0;
		tnr_14.bits.reg_3dnr_y_lut_out_1 = 255;
		tnr_14.bits.reg_3dnr_y_lut_out_2 = 255;
		tnr_14.bits.reg_3dnr_y_lut_out_3 = 255;
		ISP_WR_REG(tnr, reg_isp_444_422_t, reg_14, tnr_14.raw);

		tnr_15.raw = 0;
		tnr_15.bits.reg_3dnr_y_lut_slope_0 = 16;
		tnr_15.bits.reg_3dnr_y_lut_slope_1 = 16;
		ISP_WR_REG(tnr, reg_isp_444_422_t, reg_15, tnr_15.raw);

		ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_16, reg_3dnr_y_lut_slope_2, 16);

		if (test_case == 1) {
			//select not pixel mode
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_16, motion_sel, 0);
			//motion map output
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_8, tdnr_debug_sel, 1);

			//ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, manr_debug, 0x8); // show rgbmap
		}
	}

	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_4, reg_422_444, ctx->isp_pipe_cfg[first_raw_num].is_yuv_sensor);

	// [0]: y read
	// [1]: c read
	// [2]: motion read
	// [3]: y write
	// [4]: c write
	// [5]: motion write
	// reg_dma_enable = ~reg_force_dma_disable
	tnr_8.raw = ISP_RD_REG(tnr, reg_isp_444_422_t, reg_8);
	if (en) {
		if (ctx->isp_pipe_cfg[first_raw_num].tnr_mode == ISP_TNR_TYPE_NEW_MODE)
			tnr_8.bits.force_dma_disable = 0x0;
		else if (ctx->isp_pipe_cfg[first_raw_num].tnr_mode == ISP_TNR_TYPE_OLD_MODE)
			tnr_8.bits.force_dma_disable = 0x24;
		else
			tnr_8.bits.force_dma_disable = 0x3f;
	} else {
		tnr_8.bits.force_dma_disable = 0x3f;
	}
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_8, tnr_8.raw);

	// 0: disbale, 1:old mode, 2: new mode
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, tdnr_enable,
			((en) ? (ctx->isp_pipe_cfg[first_raw_num].tnr_mode) : 0));
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, force_mono_enable, false);
	// 0: yuv420 for normal, 1:yuv422 for ai_tnr
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_100, bypass_h, 0);
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_100, bypass_v, ctx->isp_pipe_cfg[first_raw_num].is_tnr_ai_isp);
}

void ispblk_fbc_clear_fbcd_ring_base(struct isp_ctx *ctx, u8 raw_num)
{
	uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];

	ISP_WR_REG(rdma_com_0, reg_rdma_core_t, up_ring_base,
				((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C)));
}

void ispblk_fbc_chg_to_sw_mode(struct isp_ctx *ctx, u8 raw_num)
{
	uintptr_t y_rdma = ctx->phys_regs[ISP_BLK_ID_DMA_CTL_TNR_ST_Y];
	uintptr_t uv_rdma = ctx->phys_regs[ISP_BLK_ID_DMA_CTL_TNR_ST_C];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	// Y/UV rdma config seglen to HW mode, other SW mode
	sys_ctrl.raw = ISP_RD_REG(y_rdma, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= true;
	sys_ctrl.bits.seglen_sel	= false;
	sys_ctrl.bits.segnum_sel	= true;
	ISP_WR_REG(y_rdma, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);

	sys_ctrl.raw = ISP_RD_REG(uv_rdma, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= true;
	sys_ctrl.bits.seglen_sel	= false;
	sys_ctrl.bits.segnum_sel	= true;
	ISP_WR_REG(uv_rdma, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);
}

void vi_fbc_calculate_size(struct isp_ctx *ctx, u8 raw_num)
{
	u32 img_w = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 img_h = ctx->isp_pipe_cfg[raw_num].crop.h;

	u32 max_cu_bit = fbc_cfg.is_lossless ? 65 : 67; // = CU_SIZE * 8 + cu_md_bit
	u32 line_cu_num = (img_w + fbc_cfg.cu_size - 1) / fbc_cfg.cu_size;
	u32 total_line_bit_budget = fbc_cfg.is_lossless ?
				(max_cu_bit * line_cu_num) : ((img_w * 8 * fbc_cfg.target_cr) / 100);
	u32 total_first_line_bit_budget = fbc_cfg.is_lossless ? total_line_bit_budget : (img_w * 8);

	u32 y_bs_size = VI_ALIGN(((total_line_bit_budget * (img_h - 1) + total_first_line_bit_budget) / 512) * 64);
	u32 uv_bs_size = VI_ALIGN((((total_line_bit_budget * (img_h / 2) - 1)
				+ total_first_line_bit_budget) / 512) * 64);
	u32 y_buf_size = VI_4K_ALIGN(y_bs_size);
	u32 uv_buf_size = VI_4K_ALIGN(uv_bs_size);

	fbc_cfg.y_bs_size = y_bs_size;
	fbc_cfg.c_bs_size = uv_bs_size;
	fbc_cfg.y_buf_size = y_buf_size;
	fbc_cfg.c_buf_size = uv_buf_size;
}

void ispblk_fbc_ring_buf_config(struct isp_ctx *ctx, u8 en)
{
	//WDMA_CORE, RDMA_CORE ring buffer ctrl
	uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
	uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];
	union reg_wdma_core_disable_seglen	disable_seglen;
	union reg_wdma_core_ring_buffer_en	ring_buf_en;
	enum sop_isp_raw first_raw_num = vi_get_first_raw_num(ctx);

	vi_fbc_calculate_size(ctx, first_raw_num);

	if (en) {
		disable_seglen.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, disable_seglen);
		disable_seglen.bits.seglen_disable |= ((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, disable_seglen, disable_seglen.raw);

		ring_buf_en.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable |= ((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en, ring_buf_en.raw);

		ring_buf_en.raw = ISP_RD_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable |= ((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C));
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en, ring_buf_en.raw);

		//WDMA ctrl cfg
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_size6, fbc_cfg.y_buf_size);
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_size7, fbc_cfg.c_buf_size);
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, up_ring_base,
					((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C)));

		//RDMA ctrl cfg
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size10, fbc_cfg.y_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size11, fbc_cfg.c_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, up_ring_base,
					((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C)));
	} else {
		disable_seglen.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, disable_seglen);
		disable_seglen.bits.seglen_disable &= ~((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, disable_seglen, disable_seglen.raw);

		ring_buf_en.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable &= ~((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en, ring_buf_en.raw);

		ring_buf_en.raw = ISP_RD_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable &= ~((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C));
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en, ring_buf_en.raw);
	}
}

void ispblk_fbcd_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbcd = ctx->phys_regs[ISP_BLK_ID_FBCD];
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union reg_fbcd_24	d_reg_24;
	union reg_fbcd_28	d_reg_28;
	union reg_fbce_10	reg_10;
	union reg_fbce_20	reg_20;

	if (en) {
		reg_10.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_10);
		reg_20.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_20);

		d_reg_24.raw = ISP_RD_REG(fbcd, reg_fbcd_t, reg_24);
		d_reg_24.bits.y_lossless		= reg_10.bits.y_lossless;
		d_reg_24.bits.y_base_qdpcm_q		= reg_10.bits.y_base_qdpcm_q;
		d_reg_24.bits.y_base_pcm_bd_minus2	= reg_10.bits.y_base_pcm_bd_minus2;
		d_reg_24.bits.y_default_gr_k		= DEFAULT_K;
		ISP_WR_REG(fbcd, reg_fbcd_t, reg_24, d_reg_24.raw);

		d_reg_28.raw = ISP_RD_REG(fbcd, reg_fbcd_t, reg_28);
		d_reg_28.bits.c_lossless		= reg_20.bits.c_lossless;
		d_reg_28.bits.c_base_qdpcm_q		= reg_20.bits.c_base_qdpcm_q;
		d_reg_28.bits.c_base_pcm_bd_minus2	= reg_20.bits.c_base_pcm_bd_minus2;
		d_reg_28.bits.c_default_gr_k		= DEFAULT_K;
		ISP_WR_REG(fbcd, reg_fbcd_t, reg_28, d_reg_28.raw);
	}

	ISP_WR_BITS(fbcd, reg_fbcd_t, reg_00, fbcd_en, en);
}

void ispblk_fbce_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union reg_fbce_10	reg_10;
	union reg_fbce_14	reg_14;
	union reg_fbce_1c	reg_1c;
	union reg_fbce_20	reg_20;
	union reg_fbce_24	reg_24;
	union reg_fbce_2c	reg_2c;
	union reg_fbce_30	reg_30;
	union reg_fbce_34	reg_34;
	enum sop_isp_raw raw_num = vi_get_first_raw_num(ctx);

	u32 img_w = ctx->isp_pipe_cfg[raw_num].crop.w;

	u32 cu_md_bit = fbc_cfg.is_lossless ? 1 : 3;
	u32 max_cu_bit = fbc_cfg.is_lossless ? 65 : 67; // = CU_SIZE * 8 + cu_md_bit
	u32 line_cu_num = (img_w + fbc_cfg.cu_size - 1) / fbc_cfg.cu_size;
	u32 total_line_bit_budget = fbc_cfg.is_lossless ?
				(max_cu_bit * line_cu_num) : ((img_w * 8 * fbc_cfg.target_cr) / 100);
	u32 total_first_line_bit_budget = fbc_cfg.is_lossless ? total_line_bit_budget : (img_w * 8);
	u32 cu_target_bit = total_line_bit_budget / line_cu_num;
	u32 base_dpcm_q = ((cu_target_bit <= 27) ? 1 : 0);
	u32 base_pcm_bd = (cu_target_bit - cu_md_bit) / fbc_cfg.cu_size;
	u32 min_cu_bit = fbc_cfg.is_lossless ? max_cu_bit : (base_pcm_bd * fbc_cfg.cu_size + cu_md_bit);

	if (base_pcm_bd < 2)
		base_pcm_bd = 2;
	else if (base_pcm_bd > 8)
		base_pcm_bd = 8;

	if (en) {
		reg_14.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_14);
		reg_14.bits.y_max_cu_bit		= max_cu_bit;
		reg_14.bits.y_min_cu_bit		= min_cu_bit;
		ISP_WR_REG(fbce, reg_fbce_t, reg_14, reg_14.raw);

		reg_1c.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_1c);
		reg_1c.bits.y_total_line_bit_budget	= total_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_1c, reg_1c.raw);

		reg_10.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_10);
		reg_10.bits.y_base_qdpcm_q		= base_dpcm_q;
		reg_10.bits.y_base_pcm_bd_minus2	= base_pcm_bd - 2;
		reg_10.bits.y_cplx_shift		= CPLX_SHIFT;
		reg_10.bits.y_pen_pos_shift		= PEN_POS_SHIFT;
		reg_10.bits.y_default_gr_k		= DEFAULT_K;
		reg_10.bits.y_lossless			= fbc_cfg.is_lossless;
		ISP_WR_REG(fbce, reg_fbce_t, reg_10, reg_10.raw);

		reg_30.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_30);
		reg_30.bits.y_total_first_line_bit_budget = total_first_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_30, reg_30.raw);

		reg_24.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_24);
		reg_24.bits.c_max_cu_bit		= max_cu_bit;
		reg_24.bits.c_min_cu_bit		= min_cu_bit;
		ISP_WR_REG(fbce, reg_fbce_t, reg_24, reg_24.raw);

		reg_2c.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_2c);
		reg_2c.bits.c_total_line_bit_budget	= total_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_2c, reg_2c.raw);

		reg_20.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_20);
		reg_20.bits.c_base_qdpcm_q		= 0;
		reg_20.bits.c_base_pcm_bd_minus2	= base_pcm_bd - 2;
		reg_20.bits.c_cplx_shift		= CPLX_SHIFT;
		reg_20.bits.c_pen_pos_shift		= PEN_POS_SHIFT;
		reg_20.bits.c_default_gr_k		= DEFAULT_K;
		reg_20.bits.c_lossless			= fbc_cfg.is_lossless;
		ISP_WR_REG(fbce, reg_fbce_t, reg_20, reg_20.raw);

		reg_34.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_34);
		reg_34.bits.c_total_first_line_bit_budget = total_first_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_34, reg_34.raw);
	}

	ISP_WR_BITS(fbce, reg_fbce_t, reg_00, fbce_en, en);
}

void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, u8 str_mode, u8 test_case)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	union reg_isp_cnr_enable reg_00;
	union reg_isp_cnr_strength_mode reg_01;
	union reg_isp_cnr_purple_th reg_02;
	union reg_isp_cnr_edge_scale reg_03;
	union reg_isp_cnr_edge_ratio_speed reg_04;

	// test_case = 0, for cnr_all_off and cnr_all_on, use default lut
	//   if cnr_all_off, en = 0, pfc_en = 0, str_mode = 255
	//   if cnr_all_on, en = 1, pfc_en = 1, str_mode = 255
	if (test_case == 0) {
		reg_00.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_enable);
		reg_00.bits.cnr_enable = en;
		reg_00.bits.pfc_enable = pfc_en;
		reg_00.bits.cnr_diff_shift_val = 255;
		reg_00.bits.cnr_ratio = 0;
		reg_00.bits.cnr_out_sel = 0;
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_enable, reg_00.raw);

		reg_01.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_strength_mode);
		reg_01.bits.cnr_strength_mode = str_mode;
		reg_01.bits.cnr_flag_neighbor_max_weight = 1;
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_strength_mode, reg_01.raw);

		reg_02.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_purple_th);
		reg_02.bits.cnr_purple_th = 85;
		reg_02.bits.cnr_correct_strength = 96;
		reg_02.bits.cnr_diff_gain = 4;
		reg_02.bits.cnr_motion_enable = 0;
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_purple_th, reg_02.raw);

		reg_03.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_edge_scale);
		reg_03.bits.cnr_edge_scale = 12;
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_edge_scale, reg_03.raw);

		reg_04.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_edge_ratio_speed);
		reg_04.bits.cnr_cb_str = 8;
		reg_04.bits.cnr_cr_str = 8;
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_edge_ratio_speed, reg_04.raw);
	} else if (test_case == 1) { // for cnr_set_lut, other registers with default values
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_0, 0x1e1e1e1e);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_4, 0x1e1e1e1e);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_8, 0x1e1e1e1e);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_12, 0x1e1e1e1e);

		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_0, 0xffffffff);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_4, 0xffffffff);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_8, 0xffffffff);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_12, 0xffffffff);

		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_edge_scale_lut_0, 0x20202020);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_edge_scale_lut_4, 0x20202020);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_edge_scale_lut_8, 0x20202020);
		ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_edge_scale_lut_12, 0x20202020);

		ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_00, 0x10101010);
		ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_04, 0x10101010);
		ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_08, 0x10101010);
		ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_12, 0x10101010);
	}
}


void ispblk_ynr_config(struct isp_ctx *ctx, enum isp_ynr_out_e out_sel, u8 ns_gain)
{
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];

	// depth =64
	u8 weight_lut[] = {
		   31,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	};

	u16 i = 0;

	ISP_WO_BITS(ynr, reg_isp_ynr_t, index_clr, ynr_index_clr, 1);

	for (i = 0; i < ARRAY_SIZE(weight_lut); ++i) {
		ISP_WR_REG(ynr, reg_isp_ynr_t, weight_lut, weight_lut[i]);
	}

	// ns0 luma th
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_luma_th_00, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_luma_th_01, 16);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_luma_th_02, 32);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_luma_th_03, 64);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_luma_th_04, 128);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_luma_th_05, 255);

	// ns0 slope
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_slope_00, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_slope_01, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_slope_02, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_slope_03, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_slope_04, 0);

	// ns0 offset
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_offset_00, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_offset_01, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_offset_02, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_offset_03, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_offset_04, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns0_offset_05, 255);

	// ns1 luma th
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_luma_th_00, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_luma_th_01, 16);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_luma_th_02, 32);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_luma_th_03, 64);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_luma_th_04, 128);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_luma_th_05, 255);

	// ns1 slope
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_slope_00, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_slope_01, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_slope_02, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_slope_03, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_slope_04, 0);

	// ns1 offset
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_offset_00, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_offset_01, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_offset_02, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_offset_03, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_offset_04, 255);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns1_offset_05, 255);

	// motion lut
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_00, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_01, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_02, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_03, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_04, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_05, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_06, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_07, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_08, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_09, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_10, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_11, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_12, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_13, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_14, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, motion_lut_15, 0);

	ISP_WR_REG(ynr, reg_isp_ynr_t, out_sel, ISP_YNR_OUT_BYPASS);

	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_intra_0, 6);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_intra_1, 1);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_intra_2, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_norm_1, 51);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_norm_2, 85);
	ISP_WR_REG(ynr, reg_isp_ynr_t, alpha_gain, 256);
	ISP_WR_REG(ynr, reg_isp_ynr_t, var_th, 64);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_sm, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_v, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_h, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_d45, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_d135, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, neighbor_max, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, res_k_smooth, 128);
	ISP_WR_REG(ynr, reg_isp_ynr_t, res_k_texture, 128);
	ISP_WR_REG(ynr, reg_isp_ynr_t, filter_mode_en, 0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, filter_mode_alpha, 128);

	ISP_WR_REG(ynr, reg_isp_ynr_t, out_sel, out_sel);
	ISP_WR_REG(ynr, reg_isp_ynr_t, ns_gain, ns_gain);
}

int ispblk_ee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE_POST];
	union reg_isp_ee_00  reg_0;

	reg_0.raw = ISP_RD_REG(ee, reg_isp_ee_t, reg_00);
	reg_0.bits.ee_enable = en;
	ISP_WR_REG(ee, reg_isp_ee_t, reg_00, reg_0.raw);

	return 0;
}

void ispblk_dci_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *lut, u8 test_case)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	uintptr_t dci_gamma = ctx->phys_regs[ISP_BLK_ID_DCI_GAMMA];
	union reg_isp_gamma_prog_ctrl dci_gamma_ctrl;
	union reg_isp_gamma_prog_data dci_gamma_data;
	u16 i = 0;

	ISP_WR_BITS(dci, reg_isp_dci_t, dci_enable, dci_enable, en);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_enable, dci_hist_enable, en);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_map_enable, test_case);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_per1sample_enable, en);
	ISP_WR_REG(dci, reg_isp_dci_t, dci_demo_mode, test_case);
	ISP_WR_BITS(dci, reg_isp_dci_t, dmi_enable, dmi_enable, en);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	dci_gamma_ctrl.bits.gamma_wsel = sel;
	dci_gamma_ctrl.bits.gamma_prog_en = 1;
	dci_gamma_ctrl.bits.gamma_prog_1to3_en = 1;
	ISP_WR_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		dci_gamma_data.bits.gamma_data_e = lut[i];
		dci_gamma_data.bits.gamma_data_o = lut[i + 1];
		dci_gamma_data.bits.gamma_w = 1;
		ISP_WR_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_data, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, sel);
	ISP_WR_BITS(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void ispblk_ldci_config(struct isp_ctx *ctx, bool en, u8 test_case)
{
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	union reg_isp_ldci_blk_size_x          blk_size_x;
	union reg_isp_ldci_blk_size_x1         blk_size_x1;
	union reg_isp_ldci_subblk_size_x       subblk_size_x;
	union reg_isp_ldci_subblk_size_x1      subblk_size_x1;
	union reg_isp_ldci_interp_norm_lr      interp_norm_lr;
	union reg_isp_ldci_interp_norm_lr1     interp_norm_lr1;
	union reg_isp_ldci_sub_interp_norm_lr  sub_interp_norm_lr;
	union reg_isp_ldci_sub_interp_norm_lr1 sub_interp_norm_lr1;
	union reg_isp_ldci_mean_norm_x         mean_norm_x;
	union reg_isp_ldci_var_norm_y          var_norm_y;

	u16 block_size_x, block_size_y, sub_block_size_x, sub_block_size_y;
	u16 block_size_x1, block_size_y1, sub_block_size_x1, sub_block_size_y1;
	u16 line_mean_num, line_var_num;
	u16 dW, dH;

	//hw bug use right/left tile size;
	u16 width = ctx->img_width;
	u16 height = ctx->img_height;

	if ((width % 16 == 0) && (height % 12 == 0))
		ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_enable, ldci_image_size_div_by_16x12, 1);
	else
		ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_enable, ldci_image_size_div_by_16x12, 0);

	block_size_x = (width % 16 == 0) ? (width / 16) : (width / 16) + 1; // Width of one block
	block_size_y = (height % 12 == 0) ? (height / 12) : (height / 12) + 1; // Height of one block
	sub_block_size_x = (block_size_x >> 1);
	sub_block_size_y = (block_size_y >> 1);
	line_mean_num = (block_size_y / 2) + (block_size_y % 2);
	line_var_num  = (block_size_y / 2);

	if (width % 16 == 0) {
		block_size_x1 = block_size_x;
		sub_block_size_x1 = width - block_size_x * (16 - 1) - sub_block_size_x;
	} else {
		dW = block_size_x * 16 - width;
		block_size_x1 = 2 * block_size_x - sub_block_size_x - dW;
		sub_block_size_x1 = 0;
	}

	if (height % 12 == 0) {
		block_size_y1 = block_size_y;
		sub_block_size_y1 = height - block_size_y * (12 - 1) - sub_block_size_y;
	} else {
		dH = block_size_y * 12 - height;
		block_size_y1 = 2 * block_size_y - sub_block_size_y - dH;
		sub_block_size_y1 = 0;
	}

	blk_size_x.raw = 0;
	blk_size_x.bits.ldci_blk_size_x = block_size_x;
	blk_size_x.bits.ldci_blk_size_y = block_size_y;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_blk_size_x, blk_size_x.raw);

	block_size_x1 = block_size_x;
	block_size_y1 = block_size_y;

	blk_size_x1.raw = 0;
	blk_size_x1.bits.ldci_blk_size_x1 = block_size_x1;
	blk_size_x1.bits.ldci_blk_size_y1 = block_size_y1;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_blk_size_x1, blk_size_x1.raw);

	subblk_size_x.raw = 0;
	subblk_size_x.bits.ldci_subblk_size_x = sub_block_size_x;
	subblk_size_x.bits.ldci_subblk_size_y = sub_block_size_y;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_subblk_size_x, subblk_size_x.raw);

	sub_block_size_x1 = sub_block_size_x;
	sub_block_size_y1 = sub_block_size_y;

	subblk_size_x1.raw = 0;
	subblk_size_x1.bits.ldci_subblk_size_x1 = sub_block_size_x1;
	subblk_size_x1.bits.ldci_subblk_size_y1 = sub_block_size_y1;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_subblk_size_x1, subblk_size_x1.raw);

	interp_norm_lr.raw = 0;
	interp_norm_lr.bits.ldci_interp_norm_lr = (block_size_x == 0) ? 0 : (1 << 16) / block_size_x;
	interp_norm_lr.bits.ldci_interp_norm_ud = (block_size_y == 0) ? 0 : (1 << 16) / block_size_y;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_interp_norm_lr, interp_norm_lr.raw);

	interp_norm_lr1.raw = 0;
	interp_norm_lr1.bits.ldci_interp_norm_lr1 = (block_size_x1 == 0) ? 0 : (1 << 16) / block_size_x1;
	interp_norm_lr1.bits.ldci_interp_norm_ud1 = (block_size_y1 == 0) ? 0 : (1 << 16) / block_size_y1;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_interp_norm_lr1, interp_norm_lr1.raw);

	sub_interp_norm_lr.raw = 0;
	sub_interp_norm_lr.bits.ldci_sub_interp_norm_lr = (sub_block_size_x == 0) ? 0 : (1 << 16) / sub_block_size_x;
	sub_interp_norm_lr.bits.ldci_sub_interp_norm_ud = (sub_block_size_y == 0) ? 0 : (1 << 16) / sub_block_size_y;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_sub_interp_norm_lr, sub_interp_norm_lr.raw);

	sub_interp_norm_lr1.raw = 0;
	sub_interp_norm_lr1.bits.ldci_sub_interp_norm_lr1 = (sub_block_size_x1 == 0) ? 0 : (1 << 16) / sub_block_size_x1;
	sub_interp_norm_lr1.bits.ldci_sub_interp_norm_ud1 = (sub_block_size_y1 == 0) ? 0 : (1 << 16) / sub_block_size_y1;
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_sub_interp_norm_lr1, sub_interp_norm_lr1.raw);

	mean_norm_x.raw = 0;
	mean_norm_x.bits.ldci_mean_norm_x = (1 << 14) / MAX(block_size_x, 1);
	mean_norm_x.bits.ldci_mean_norm_y = (1 << 13) / MAX(line_mean_num, 1);
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_mean_norm_x, mean_norm_x.raw);

	var_norm_y.raw = 0;
	var_norm_y.bits.ldci_var_norm_y = (1 << 13) / MAX(line_var_num, 1);
	ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_var_norm_y, var_norm_y.raw);

	if (test_case == 1) {
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_00, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_02, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_04, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_06, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_08, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_10, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_12, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_tone_curve_lut_p_14, (1023 << 16) | (1023));
	}

	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_enable, ldci_enable, en);
	ISP_WR_BITS(ldci, reg_isp_ldci_t, dmi_enable, dmi_enable, en ? 3 : 0);
}


void ispblk_ca_config(struct isp_ctx *ctx, bool en, u8 mode)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	u16 i = 0;
	union reg_ca_04 wdata;

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_enable, en);
	// 0 CA mode, 1 Cp mode
	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mode, mode);

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_iso_ratio, 64);

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 1);

	if (mode == 0) {
		for (i = 0; i < sizeof(ca_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = ca_y_lut[i];
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < sizeof(cp_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = ((cp_v_lut[i]) | (cp_u_lut[i] << 8) | (cp_y_lut[i] << 16));
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 0);
}

void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_00, ca_lite_enable, en);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_0, 0x0);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_1, 0x80);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_2, 0x100);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_3, 0x100);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_4, 0x100);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_5, 0x100);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_0, 0x100);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_1, 0x80);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_2, 0x40);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_3, 0x40);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_4, 0x40);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_5, 0x40);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_0, 0x0);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_1, 0x0);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_2, 0x0);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_3, 0x0);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_24, ca_lite_lut_slp_4, 0x0);
}

void ispblk_ycur_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	u16 i;
	union reg_isp_ycurv_ycur_prog_data reg_data;

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_prog_en, 1);

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_wsel, sel);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_addr, 0);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_w, 1);
	ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_max, data[64]);
	for (i = 0; i < 64; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.ycur_data_e = data[i];
		reg_data.bits.ycur_data_o = data[i + 1];
		reg_data.bits.ycur_w = 1;
		ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_data, reg_data.raw);
	}

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_rsel, sel);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_prog_en, 0);
}

void ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, u8 sel)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_ctrl, ycur_enable, enable);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_rsel, sel);
}

#ifdef PORTING_TEST
void ispblk_dci_restore_default_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	uintptr_t dci_gamma = ctx->phys_regs[ISP_BLK_ID_DCI_GAMMA];
	union reg_isp_gamma_prog_ctrl dci_gamma_ctrl;
	union reg_isp_gamma_prog_data dci_gamma_data;
	u16 i = 0;

	ISP_WR_BITS(dci, reg_isp_dci_t, dci_enable, dci_enable, en);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_enable, dci_hist_enable, en);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_map_enable, 0);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_per1sample_enable, en);
	ISP_WR_REG(dci, reg_isp_dci_t, dci_demo_mode, 0);
	ISP_WR_BITS(dci, reg_isp_dci_t, dmi_enable, dmi_enable, en);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	dci_gamma_ctrl.bits.gamma_wsel = 0;
	dci_gamma_ctrl.bits.gamma_prog_en = 0;
	dci_gamma_ctrl.bits.gamma_prog_1to3_en = 1;
	ISP_WR_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		ISP_WR_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_data, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, 1);
	ISP_WR_BITS(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}
#endif
