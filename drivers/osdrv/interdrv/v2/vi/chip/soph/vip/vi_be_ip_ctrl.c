#include <vip/vi_drv.h>

/*******************************************************************************
 *	BE IPs config
 ******************************************************************************/
void ispblk_rgbir_config(struct isp_ctx *ctx, enum isp_raw_path_e path, bool enable)
{
	uintptr_t rgbir = (path == ISP_RAW_PATH_LE)
			? ctx->phys_regs[ISP_BLK_ID_RGBIR0]
			: ctx->phys_regs[ISP_BLK_ID_RGBIR1];

	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_ctrl, rgbir2rggb_enable, enable);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_ctrl, rgbir2rggb_comp_enable, 0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_ctrl, rgbir2rggb_ir_wdma_mode, 0); //0: 12bit, 1: 8bit

	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_1, rgbir2rggb_rec_gbr_gain, 0x393);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_1, rgbir2rggb_rec_gbr_offset, 0x0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_2, rgbir2rggb_rec_gir_gain, 0x38a);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_2, rgbir2rggb_rec_gir_offset, 0x0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_3, rgbir2rggb_rec_rg_gain, 0x3e0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_3, rgbir2rggb_rec_rg_offset, 0x0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_4, rgbir2rggb_rec_bg_gain, 0x3c4);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_4, rgbir2rggb_rec_bg_offset, 0x0);

	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_comp_gain, rgbir2rggb_g_comp_gain, 0x0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_comp_gain, rgbir2rggb_r_comp_gain, 0x0);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_comp_gain, rgbir2rggb_b_comp_gain, 0x0);

	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rbgir_wdma_ctl, rgbir2rggb_dma_enable, enable);
}

void ispblk_dpc_config(struct isp_ctx *ctx, enum isp_raw_path_e path, bool enable, u8 test_case)
{
	uintptr_t dpc = (path == ISP_RAW_PATH_LE)
			? ctx->phys_regs[ISP_BLK_ID_DPC0]
			: ctx->phys_regs[ISP_BLK_ID_DPC1];
	union reg_isp_dpc_2 reg_2;

	reg_2.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, dpc_2);
	reg_2.bits.dpc_enable = enable;
	reg_2.bits.ge_enable = enable;
	reg_2.bits.dpc_dynamicbpc_enable = enable;
	reg_2.bits.dpc_staticbpc_enable = enable;
	ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_2, reg_2.raw);

	if (test_case == 1) { // test static dpc
		reg_2.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, dpc_2);
		reg_2.bits.dpc_dynamicbpc_enable = 0;
		reg_2.bits.dpc_staticbpc_enable = 1;
		ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_2, reg_2.raw);
	} else if (test_case == 2) { // test dynamic dpc
		reg_2.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, dpc_2);
		reg_2.bits.dpc_dynamicbpc_enable = 1;
		reg_2.bits.dpc_staticbpc_enable = 0;
		ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_2, reg_2.raw);
	}
}

/**
 * ispblk_dpc_set_static - set defect pixels for static dpc.
 *
 * @param ctx: global settings
 * @param offset: mem-offset for 4k tile
 * @param bps: array of defect pixels. [23:12]-row, [11:0]-col.
 * @param count: number of defect pixels.
 */

void ispblk_dpc_set_static(struct isp_ctx *ctx, enum isp_raw_path_e path,
			     u16 offset, u32 *bps, u16 count)
{
	uintptr_t dpc = (path == ISP_RAW_PATH_LE)
			? ctx->phys_regs[ISP_BLK_ID_DPC0]
			: ctx->phys_regs[ISP_BLK_ID_DPC1];
	u16 i = 0;

	ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_17, dpc_mem_prog_mode, 1);

	ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_st_addr, 0x80000000 | offset);

	for (i = 0; (i < count) && (i < 4096); ++i)
		ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_w0, 0x80000000 | *(bps + i));

	// write 1 3-fff-fff to end
	ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_w0, 0x83ffffff);
	ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_17, dpc_mem_prog_mode, 0);
}

void ispblk_af_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
	int numx = 17, numy = 15;

	ISP_WR_BITS(sts, reg_isp_af_t, kickoff, af_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, dmi_enable, dmi_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_horizon_0_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_horizon_1_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_vertical_0_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_hlc_enable, 1);
	ISP_WR_REG(sts, reg_isp_af_t, bypass, !enable);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(sts, reg_isp_af_t, offset_x, af_offset_x, 0x8);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(sts, reg_isp_af_t, offset_x, af_offset_y, 0x2);
	// block_width >= 15
	ISP_WR_REG(sts, reg_isp_af_t, block_width, (ctx->img_width - 16) / numx);
	// block_height >= 15
	ISP_WR_REG(sts, reg_isp_af_t, block_height, (ctx->img_height - 4) / numy);
	ISP_WR_REG(sts, reg_isp_af_t, block_num_x, 17);
	ISP_WR_REG(sts, reg_isp_af_t, block_num_y, 15);

	ISP_WR_REG(sts, reg_isp_af_t, high_y_thre, 0x258);

	ISP_WR_REG(sts, reg_isp_af_t, image_width, ctx->img_width - 1);
	ISP_WR_BITS(sts, reg_isp_af_t, mxn_image_width_m1, af_mxn_image_width, ctx->img_width - 1);
	ISP_WR_BITS(sts, reg_isp_af_t, mxn_image_width_m1, af_mxn_image_height, ctx->img_height - 1);
}
