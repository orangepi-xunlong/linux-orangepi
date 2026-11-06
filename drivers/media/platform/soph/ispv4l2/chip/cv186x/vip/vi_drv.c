// SPDX-License-Identifier: MIT
#include <vip/vi_drv.h>
#include <cmdq.h>

#define LUMA_MAP_W_BIT	4
#define LUMA_MAP_H_BIT	4
#define MANR_W_BIT	3
#define MANR_H_BIT	3

#define RGBMAP_MAX_BIT		3
#define SLICE_MAX_GRID_SIZE	5

/****************************************************************************
 * Global parameters
 ****************************************************************************/
u8 g_w_bit[ISP_PRERAW_MAX], g_h_bit[ISP_PRERAW_MAX];
u8 g_rgbmap_chg_pre[ISP_PRERAW_MAX][2];

/****************************************************************************
 * LMAP_CONFIG
 ****************************************************************************/
struct lmap_cfg g_lmp_cfg[ISP_PRERAW_MAX];

/****************************************************************************
 * SLICE_BUFFER_CONFIG
 ****************************************************************************/
struct slice_buf_s slc_b_cfg = {
	.line_delay		= 512,
	.buffer			= 16,
	.main_max_grid_size	= 32,
	.sub_max_grid_size	= 8,
	.min_r_thshd		= 1,
};

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_be_online(struct isp_ctx *ctx)
{
	if (!ctx->is_offline_be && ctx->is_offline_postraw) //fe->be->dram->post
		return 1;
	return 0;
}

u32 _is_be_post_online(struct isp_ctx *ctx)
{
	if (ctx->is_offline_be && !ctx->is_offline_postraw) //fe->dram->be->post
		return 1;
	return 0;
}

u32 _is_all_online(struct isp_ctx *ctx)
{
	if (!ctx->is_offline_be && !ctx->is_offline_postraw)
		return 1;
	return 0;
}

u32 _is_post_sclr_online(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler)
		return 1;
	return 0;
}

u32 _is_right_tile(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	if (ctx->isp_pipe_cfg[raw_num].is_tile && raw_num == ISP_PRERAW1)
		return 1;
	return 0;
}

/****************************************************************************
 *  Interfaces
 ****************************************************************************/
int vi_get_dev_num_by_raw(struct isp_ctx *ctx, u8 raw_num)
{
	int dev_num = ISP_PRERAW_MAX;

	if (raw_num < VI_MAX_CHN_NUM) {
		if (ctx->isp_bind_info[raw_num].is_bind)
			dev_num = ctx->isp_bind_info[raw_num].bind_dev_num;
	}

	if (dev_num >= VI_MAX_CHN_NUM)
		dev_num = 0;

	return dev_num;
}

int vi_get_raw_num_by_dev(struct isp_ctx *ctx, u8 dev_num)
{
	int raw_num = ISP_PRERAW_MAX;
	int i = ISP_PRERAW0;
	raw_num = ctx->isp_bind_info[dev_num].bind_fe_num;

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)
		return dev_num;

	if (dev_num < ISP_PRERAW_MAX) {
		for (i = ISP_PRERAW0; i < ISP_PRERAW_MAX; i++) {
			if ((ctx->isp_bind_info[i].is_bind) &&
			    (ctx->isp_bind_info[i].bind_dev_num == dev_num)) {
				raw_num = ctx->isp_bind_info[i].bind_fe_num;
				break;
			}
		}
	}

	if (raw_num == ISP_PRERAW_MAX)
		raw_num = ISP_PRERAW0;

	return raw_num;
}

int vi_get_first_raw_num(struct isp_ctx *ctx)
{
	int raw_num = -1;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (ctx->isp_pipe_enable[raw_num])
			break;
	}

	if (raw_num == ISP_PRERAW_MAX)
		raw_num = ISP_PRERAW0;

	return raw_num;
}

void vi_set_base_addr(void *base)
{
	uintptr_t *addr = isp_get_phys_reg_bases();
	int i = 0;

	for (i = 0; i < ISP_BLK_ID_MAX; ++i)
		addr[i] += (uintptr_t)base;
}

uintptr_t *isp_get_phys_reg_bases(void)
{
	static uintptr_t m_isp_phys_base_list[ISP_BLK_ID_MAX] = {
		[ISP_BLK_ID_PRE_RAW_FE0]		= (ISP_BLK_BA_PRE_RAW_FE0),
		[ISP_BLK_ID_CSIBDG0]			= (ISP_BLK_BA_CSIBDG0),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG1),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG2]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG2),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG3]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG3),
		[ISP_BLK_ID_PRE_RAW_FE0_BLC0]		= (ISP_BLK_BA_PRE_RAW_FE0_BLC0),
		[ISP_BLK_ID_PRE_RAW_FE0_BLC1]		= (ISP_BLK_BA_PRE_RAW_FE0_BLC1),
		[ISP_BLK_ID_RGBMAP_FE0_LE]		= (ISP_BLK_BA_RGBMAP_FE0_LE),
		[ISP_BLK_ID_RGBMAP_WBG0]		= (ISP_BLK_BA_RGBMAP_WBG0),
		[ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE]	= (ISP_BLK_BA_DMA_CTL_FE0_RGBMAP_LE),
		[ISP_BLK_ID_RGBMAP_FE0_SE]		= (ISP_BLK_BA_RGBMAP_FE0_SE),
		[ISP_BLK_ID_RGBMAP_WBG1]		= (ISP_BLK_BA_RGBMAP_WBG1),
		[ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE]	= (ISP_BLK_BA_DMA_CTL_FE0_RGBMAP_SE),

		[ISP_BLK_ID_PRE_RAW_FE1]		= (ISP_BLK_BA_PRE_RAW_FE1),
		[ISP_BLK_ID_CSIBDG1]			= (ISP_BLK_BA_CSIBDG1),
		[ISP_BLK_ID_DMA_CTL_CSI1_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI1_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI1_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI1_BDG1),
		[ISP_BLK_ID_DMA_CTL_CSI1_BDG2]		= (ISP_BLK_BA_DMA_CTL_CSI1_BDG2),
		[ISP_BLK_ID_DMA_CTL_CSI1_BDG3]		= (ISP_BLK_BA_DMA_CTL_CSI1_BDG3),
		[ISP_BLK_ID_PRE_RAW_FE1_BLC0]		= (ISP_BLK_BA_PRE_RAW_FE1_BLC0),
		[ISP_BLK_ID_PRE_RAW_FE1_BLC1]		= (ISP_BLK_BA_PRE_RAW_FE1_BLC1),
		[ISP_BLK_ID_RGBMAP_FE1_LE]		= (ISP_BLK_BA_RGBMAP_FE1_LE),
		[ISP_BLK_ID_RGBMAP_WBG2]		= (ISP_BLK_BA_RGBMAP_WBG2),
		[ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_LE]	= (ISP_BLK_BA_DMA_CTL_FE1_RGBMAP_LE),
		[ISP_BLK_ID_RGBMAP_FE1_SE]		= (ISP_BLK_BA_RGBMAP_FE1_SE),
		[ISP_BLK_ID_RGBMAP_WBG3]		= (ISP_BLK_BA_RGBMAP_WBG3),
		[ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_SE]	= (ISP_BLK_BA_DMA_CTL_FE1_RGBMAP_SE),

		[ISP_BLK_ID_PRE_RAW_FE2]		= (ISP_BLK_BA_PRE_RAW_FE2),
		[ISP_BLK_ID_CSIBDG2]			= (ISP_BLK_BA_CSIBDG2),
		[ISP_BLK_ID_DMA_CTL_CSI2_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI2_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI2_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI2_BDG1),
		[ISP_BLK_ID_PRE_RAW_FE2_BLC0]		= (ISP_BLK_BA_PRE_RAW_FE2_BLC0),
		[ISP_BLK_ID_PRE_RAW_FE2_BLC1]		= (ISP_BLK_BA_PRE_RAW_FE2_BLC1),
		[ISP_BLK_ID_RGBMAP_FE2_LE]		= (ISP_BLK_BA_RGBMAP_FE2_LE),
		[ISP_BLK_ID_RGBMAP_WBG4]		= (ISP_BLK_BA_RGBMAP_WBG4),
		[ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_LE]	= (ISP_BLK_BA_DMA_CTL_FE2_RGBMAP_LE),
		[ISP_BLK_ID_RGBMAP_FE2_SE]		= (ISP_BLK_BA_RGBMAP_FE2_SE),
		[ISP_BLK_ID_RGBMAP_WBG5]		= (ISP_BLK_BA_RGBMAP_WBG5),
		[ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_SE]	= (ISP_BLK_BA_DMA_CTL_FE2_RGBMAP_SE),

		[ISP_BLK_ID_PRE_RAW_FE3]		= (ISP_BLK_BA_PRE_RAW_FE3),
		[ISP_BLK_ID_CSIBDG3]			= (ISP_BLK_BA_CSIBDG3),
		[ISP_BLK_ID_DMA_CTL_CSI3_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI3_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI3_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI3_BDG1),
		[ISP_BLK_ID_PRE_RAW_FE3_BLC0]		= (ISP_BLK_BA_PRE_RAW_FE3_BLC0),
		[ISP_BLK_ID_PRE_RAW_FE3_BLC1]		= (ISP_BLK_BA_PRE_RAW_FE3_BLC1),
		[ISP_BLK_ID_RGBMAP_FE3_LE]		= (ISP_BLK_BA_RGBMAP_FE3_LE),
		[ISP_BLK_ID_RGBMAP_WBG6]		= (ISP_BLK_BA_RGBMAP_WBG6),
		[ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_LE]	= (ISP_BLK_BA_DMA_CTL_FE3_RGBMAP_LE),
		[ISP_BLK_ID_RGBMAP_FE3_SE]		= (ISP_BLK_BA_RGBMAP_FE3_SE),
		[ISP_BLK_ID_RGBMAP_WBG7]		= (ISP_BLK_BA_RGBMAP_WBG7),
		[ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_SE]	= (ISP_BLK_BA_DMA_CTL_FE3_RGBMAP_SE),

		[ISP_BLK_ID_PRE_RAW_FE4]		= (ISP_BLK_BA_PRE_RAW_FE4),
		[ISP_BLK_ID_CSIBDG4]			= (ISP_BLK_BA_CSIBDG4),
		[ISP_BLK_ID_DMA_CTL_CSI4_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI4_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI4_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI4_BDG1),
		[ISP_BLK_ID_PRE_RAW_FE4_BLC0]		= (ISP_BLK_BA_PRE_RAW_FE4_BLC0),
		[ISP_BLK_ID_PRE_RAW_FE4_BLC1]		= (ISP_BLK_BA_PRE_RAW_FE4_BLC1),
		[ISP_BLK_ID_RGBMAP_FE4_LE]		= (ISP_BLK_BA_RGBMAP_FE4_LE),
		[ISP_BLK_ID_RGBMAP_WBG8]		= (ISP_BLK_BA_RGBMAP_WBG8),
		[ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_LE]	= (ISP_BLK_BA_DMA_CTL_FE4_RGBMAP_LE),
		[ISP_BLK_ID_RGBMAP_FE4_SE]		= (ISP_BLK_BA_RGBMAP_FE4_SE),
		[ISP_BLK_ID_RGBMAP_WBG9]		= (ISP_BLK_BA_RGBMAP_WBG9),
		[ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_SE]	= (ISP_BLK_BA_DMA_CTL_FE4_RGBMAP_SE),

		[ISP_BLK_ID_PRE_RAW_FE5]		= (ISP_BLK_BA_PRE_RAW_FE5),
		[ISP_BLK_ID_CSIBDG5]			= (ISP_BLK_BA_CSIBDG5),
		[ISP_BLK_ID_DMA_CTL_CSI5_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI5_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI5_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI5_BDG1),
		[ISP_BLK_ID_PRE_RAW_FE5_BLC0]		= (ISP_BLK_BA_PRE_RAW_FE5_BLC0),
		[ISP_BLK_ID_PRE_RAW_FE5_BLC1]		= (ISP_BLK_BA_PRE_RAW_FE5_BLC1),
		[ISP_BLK_ID_RGBMAP_FE5_LE]		= (ISP_BLK_BA_RGBMAP_FE5_LE),
		[ISP_BLK_ID_RGBMAP_WBG10]		= (ISP_BLK_BA_RGBMAP_WBG10),
		[ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_LE]	= (ISP_BLK_BA_DMA_CTL_FE5_RGBMAP_LE),
		[ISP_BLK_ID_RGBMAP_FE5_SE]		= (ISP_BLK_BA_RGBMAP_FE5_SE),
		[ISP_BLK_ID_RGBMAP_WBG11]		= (ISP_BLK_BA_RGBMAP_WBG11),
		[ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_SE]	= (ISP_BLK_BA_DMA_CTL_FE5_RGBMAP_SE),

		[ISP_BLK_ID_PRE_RAW_BE]			= (ISP_BLK_BA_PRE_RAW_BE),
		[ISP_BLK_ID_BE_CROP_LE]			= (ISP_BLK_BA_BE_CROP_LE),
		[ISP_BLK_ID_BE_CROP_SE]			= (ISP_BLK_BA_BE_CROP_SE),
		[ISP_BLK_ID_PRE_RAW_BE_BLC0]		= (ISP_BLK_BA_PRE_RAW_BE_BLC0),
		[ISP_BLK_ID_PRE_RAW_BE_BLC1]		= (ISP_BLK_BA_PRE_RAW_BE_BLC1),
		[ISP_BLK_ID_AF]				= (ISP_BLK_BA_AF),
		[ISP_BLK_ID_DMA_CTL_AF_W]		= (ISP_BLK_BA_DMA_CTL_AF_W),
		[ISP_BLK_ID_DPC0]			= (ISP_BLK_BA_DPC0),
		[ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE]	= (ISP_BLK_BA_DMA_CTL_PRE_RAW_BE_LE),
		[ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE]	= (ISP_BLK_BA_DMA_CTL_PRE_RAW_BE_SE),
		[ISP_BLK_ID_PRE_WDMA]			= (ISP_BLK_BA_PRE_WDMA),
		[ISP_BLK_ID_PCHK0]			= (ISP_BLK_BA_PCHK0),
		[ISP_BLK_ID_PCHK1]			= (ISP_BLK_BA_PCHK1),
		[ISP_BLK_ID_RGBIR0]			= (ISP_BLK_BA_RGBIR0),
		[ISP_BLK_ID_DMA_CTL_RGBIR_LE]		= (ISP_BLK_BA_DMA_CTL_RGBIR_LE),
		[ISP_BLK_ID_DPC1]			= (ISP_BLK_BA_DPC1),
		[ISP_BLK_ID_RGBIR1]			= (ISP_BLK_BA_RGBIR1),
		[ISP_BLK_ID_DMA_CTL_RGBIR_SE]		= (ISP_BLK_BA_DMA_CTL_RGBIR_SE),

		[ISP_BLK_ID_WDMA_CORE0]			= (ISP_BLK_BA_WDMA_CORE0),
		[ISP_BLK_ID_WDMA_CORE1]			= (ISP_BLK_BA_WDMA_CORE1),
		[ISP_BLK_ID_WDMA_CORE2]			= (ISP_BLK_BA_WDMA_CORE2),
		[ISP_BLK_ID_WDMA_CORE3]			= (ISP_BLK_BA_WDMA_CORE3),

		[ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE0_WDMA_LE),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE0_WDMA_SE),
		[ISP_BLK_ID_SPLT_FE0_WDMA]		= (ISP_BLK_BA_SPLT_FE0_WDMA),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE0_RDMA_LE),
		[ISP_BLK_ID_SPLT_FE0_RDMA_LE]		= (ISP_BLK_BA_SPLT_FE0_RDMA_LE),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE0_RDMA_SE),
		[ISP_BLK_ID_SPLT_FE0_RDMA_SE]		= (ISP_BLK_BA_SPLT_FE0_RDMA_SE),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_LE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE1_WDMA_LE),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_SE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE1_WDMA_SE),
		[ISP_BLK_ID_SPLT_FE1_WDMA]		= (ISP_BLK_BA_SPLT_FE1_WDMA),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_LE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE1_RDMA_LE),
		[ISP_BLK_ID_SPLT_FE1_RDMA_LE]		= (ISP_BLK_BA_SPLT_FE1_RDMA_LE),
		[ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_SE]	= (ISP_BLK_BA_DMA_CTL_SPLT_FE1_RDMA_SE),
		[ISP_BLK_ID_SPLT_FE1_RDMA_SE]		= (ISP_BLK_BA_SPLT_FE1_RDMA_SE),
		[ISP_BLK_ID_SPLT]			= (ISP_BLK_BA_SPLT),

		[ISP_BLK_ID_RAWTOP]			= (ISP_BLK_BA_RAWTOP),
		[ISP_BLK_ID_CFA0]			= (ISP_BLK_BA_CFA0),
		[ISP_BLK_ID_LSC0]			= (ISP_BLK_BA_LSC0),
		[ISP_BLK_ID_DMA_CTL_LSC_LE]		= (ISP_BLK_BA_DMA_CTL_LSC_LE),
		[ISP_BLK_ID_GMS]			= (ISP_BLK_BA_GMS),
		[ISP_BLK_ID_DMA_CTL_GMS]		= (ISP_BLK_BA_DMA_CTL_GMS),
		[ISP_BLK_ID_AE_HIST0]			= (ISP_BLK_BA_AE_HIST0),
		[ISP_BLK_ID_DMA_CTL_AE_HIST_LE]		= (ISP_BLK_BA_DMA_CTL_AE_HIST_LE),
		[ISP_BLK_ID_AE_HIST1]			= (ISP_BLK_BA_AE_HIST1),
		[ISP_BLK_ID_DMA_CTL_AE_HIST_SE]		= (ISP_BLK_BA_DMA_CTL_AE_HIST_SE),
		[ISP_BLK_ID_DMA_CTL_RAW_RDMA0]		= (ISP_BLK_BA_DMA_CTL_RAW_RDMA0),
		[ISP_BLK_ID_RAW_RDMA0]			= (ISP_BLK_BA_RAW_RDMA0),
		[ISP_BLK_ID_DMA_CTL_RAW_RDMA1]		= (ISP_BLK_BA_DMA_CTL_RAW_RDMA1),
		[ISP_BLK_ID_RAW_RDMA1]			= (ISP_BLK_BA_RAW_RDMA1),
		[ISP_BLK_ID_CFA1]			= (ISP_BLK_BA_CFA1),
		[ISP_BLK_ID_LSC1]			= (ISP_BLK_BA_LSC1),
		[ISP_BLK_ID_DMA_CTL_LSC_SE]		= (ISP_BLK_BA_DMA_CTL_LSC_SE),
		[ISP_BLK_ID_LMAP1]			= (ISP_BLK_BA_LMAP1),
		[ISP_BLK_ID_DMA_CTL_LMAP_SE]		= (ISP_BLK_BA_DMA_CTL_LMAP_SE),
		[ISP_BLK_ID_BNR0]			= (ISP_BLK_BA_BNR0),
		[ISP_BLK_ID_BNR1]			= (ISP_BLK_BA_BNR1),
		[ISP_BLK_ID_RAW_CROP_LE]		= (ISP_BLK_BA_RAW_CROP_LE),
		[ISP_BLK_ID_RAW_CROP_SE]		= (ISP_BLK_BA_RAW_CROP_SE),
		[ISP_BLK_ID_LMAP0]			= (ISP_BLK_BA_LMAP0),
		[ISP_BLK_ID_DMA_CTL_LMAP_LE]		= (ISP_BLK_BA_DMA_CTL_LMAP_LE),
		[ISP_BLK_ID_RAW_WBG0]			= (ISP_BLK_BA_RAW_WBG0),
		[ISP_BLK_ID_RAW_WBG1]			= (ISP_BLK_BA_RAW_WBG1),
		[ISP_BLK_ID_PCHK2]			= (ISP_BLK_BA_PCHK2),
		[ISP_BLK_ID_PCHK3]			= (ISP_BLK_BA_PCHK3),
		[ISP_BLK_ID_LCAC0]			= (ISP_BLK_BA_LCAC0),
		[ISP_BLK_ID_RGBCAC0]			= (ISP_BLK_BA_RGBCAC0),
		[ISP_BLK_ID_LCAC1]			= (ISP_BLK_BA_LCAC1),
		[ISP_BLK_ID_RGBCAC1]			= (ISP_BLK_BA_RGBCAC1),

		[ISP_BLK_ID_RGBTOP]			= (ISP_BLK_BA_RGBTOP),
		[ISP_BLK_ID_CCM0]			= (ISP_BLK_BA_CCM0),
		[ISP_BLK_ID_CCM1]			= (ISP_BLK_BA_CCM1),
		[ISP_BLK_ID_RGBGAMMA]			= (ISP_BLK_BA_RGBGAMMA),
		[ISP_BLK_ID_YGAMMA]			= (ISP_BLK_BA_YGAMMA),
		[ISP_BLK_ID_MMAP]			= (ISP_BLK_BA_MMAP),
		[ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R]	= (ISP_BLK_BA_DMA_CTL_MMAP_PRE_LE_R),
		[ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R]	= (ISP_BLK_BA_DMA_CTL_MMAP_PRE_SE_R),
		[ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R]	= (ISP_BLK_BA_DMA_CTL_MMAP_CUR_LE_R),
		[ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R]	= (ISP_BLK_BA_DMA_CTL_MMAP_CUR_SE_R),
		[ISP_BLK_ID_DMA_CTL_MMAP_IIR_R]		= (ISP_BLK_BA_DMA_CTL_MMAP_IIR_R),
		[ISP_BLK_ID_DMA_CTL_MMAP_IIR_W]		= (ISP_BLK_BA_DMA_CTL_MMAP_IIR_W),
		[ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP]	= (ISP_BLK_BA_DMA_CTL_MMAP_AI_ISP),
		[ISP_BLK_ID_CLUT]			= (ISP_BLK_BA_CLUT),
		[ISP_BLK_ID_DEHAZE]			= (ISP_BLK_BA_DEHAZE),
		[ISP_BLK_ID_CSC]			= (ISP_BLK_BA_CSC),
		[ISP_BLK_ID_RGB_DITHER]			= (ISP_BLK_BA_RGB_DITHER),
		[ISP_BLK_ID_PCHK4]			= (ISP_BLK_BA_PCHK4),
		[ISP_BLK_ID_PCHK5]			= (ISP_BLK_BA_PCHK5),
		[ISP_BLK_ID_HIST_EDGE_V]		= (ISP_BLK_BA_HIST_EDGE_V),
		[ISP_BLK_ID_DMA_CTL_HIST_EDGE_V]	= (ISP_BLK_BA_DMA_CTL_HIST_EDGE_V),
		[ISP_BLK_ID_FUSION]			= (ISP_BLK_BA_FUSION),
		[ISP_BLK_ID_LTM]			= (ISP_BLK_BA_LTM),
		[ISP_BLK_ID_DMA_CTL_LTM_LE]		= (ISP_BLK_BA_DMA_CTL_LTM_LE),
		[ISP_BLK_ID_DMA_CTL_LTM_SE]		= (ISP_BLK_BA_DMA_CTL_LTM_SE),

		[ISP_BLK_ID_YUVTOP]			= (ISP_BLK_BA_YUVTOP),
		[ISP_BLK_ID_TNR]			= (ISP_BLK_BA_TNR),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_MO]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_MO),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_MO]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_MO),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_Y]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_Y),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_C]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_C),
		[ISP_BLK_ID_FBCE]			= (ISP_BLK_BA_FBCE),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_Y]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_Y),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_C]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_C),
		[ISP_BLK_ID_FBCD]			= (ISP_BLK_BA_FBCD),
		[ISP_BLK_ID_YUV_DITHER]			= (ISP_BLK_BA_YUV_DITHER),
		[ISP_BLK_ID_CA]				= (ISP_BLK_BA_CA),
		[ISP_BLK_ID_CA_LITE]			= (ISP_BLK_BA_CA_LITE),
		[ISP_BLK_ID_YNR]			= (ISP_BLK_BA_YNR),
		[ISP_BLK_ID_CNR]			= (ISP_BLK_BA_CNR),
		[ISP_BLK_ID_EE_POST]			= (ISP_BLK_BA_EE_POST),
		[ISP_BLK_ID_YCURVE]			= (ISP_BLK_BA_YCURVE),
		[ISP_BLK_ID_DCI]			= (ISP_BLK_BA_DCI),
		[ISP_BLK_ID_DMA_CTL_DCI]		= (ISP_BLK_BA_DMA_CTL_DCI),
		[ISP_BLK_ID_DCI_GAMMA]			= (ISP_BLK_BA_DCI_GAMMA),
		[ISP_BLK_ID_YUV_CROP_Y]			= (ISP_BLK_BA_YUV_CROP_Y),
		[ISP_BLK_ID_DMA_CTL_YUV_CROP_Y]		= (ISP_BLK_BA_DMA_CTL_YUV_CROP_Y),
		[ISP_BLK_ID_YUV_CROP_C]			= (ISP_BLK_BA_YUV_CROP_C),
		[ISP_BLK_ID_DMA_CTL_YUV_CROP_C]		= (ISP_BLK_BA_DMA_CTL_YUV_CROP_C),
		[ISP_BLK_ID_LDCI]			= (ISP_BLK_BA_LDCI),
		[ISP_BLK_ID_DMA_CTL_LDCI_W]		= (ISP_BLK_BA_DMA_CTL_LDCI_W),
		[ISP_BLK_ID_DMA_CTL_LDCI_R]		= (ISP_BLK_BA_DMA_CTL_LDCI_R),
		[ISP_BLK_ID_EE_PRE]			= (ISP_BLK_BA_EE_PRE),
		[ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y]	= (ISP_BLK_BA_DMA_CTL_AI_ISP_RDMA_Y),
		[ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U]	= (ISP_BLK_BA_DMA_CTL_AI_ISP_RDMA_U),
		[ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V]	= (ISP_BLK_BA_DMA_CTL_AI_ISP_RDMA_V),

		[ISP_BLK_ID_ISPTOP]			= (ISP_BLK_BA_ISPTOP),
		[ISP_BLK_ID_RDMA_CORE0]			= (ISP_BLK_BA_RDMA_CORE0),
		[ISP_BLK_ID_RDMA_CORE1]			= (ISP_BLK_BA_RDMA_CORE1),
		[ISP_BLK_ID_CSIBDG0_LITE]		= (ISP_BLK_BA_CSIBDG0_LITE),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE0]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE0),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE1]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE1),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE2]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE2),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE3]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE3),
		[ISP_BLK_ID_CSIBDG1_LITE]		= (ISP_BLK_BA_CSIBDG1_LITE),
		[ISP_BLK_ID_DMA_CTL_BT1_LITE0]		= (ISP_BLK_BA_DMA_CTL_BT1_LITE0),
		[ISP_BLK_ID_DMA_CTL_BT1_LITE1]		= (ISP_BLK_BA_DMA_CTL_BT1_LITE1),
		[ISP_BLK_ID_DMA_CTL_BT1_LITE2]		= (ISP_BLK_BA_DMA_CTL_BT1_LITE2),
		[ISP_BLK_ID_DMA_CTL_BT1_LITE3]		= (ISP_BLK_BA_DMA_CTL_BT1_LITE3),
		[ISP_BLK_ID_PRE_RAW_VI_SEL]		= (ISP_BLK_BA_PRE_RAW_VI_SEL),
		[ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE]	= (ISP_BLK_BA_DMA_CTL_PRE_RAW_VI_SEL_LE),
		[ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE]	= (ISP_BLK_BA_DMA_CTL_PRE_RAW_VI_SEL_SE),
		[ISP_BLK_ID_CMDQ]			= (ISP_BLK_BA_CMDQ),
	};
	return m_isp_phys_base_list;
}

void isp_debug_dump(struct isp_ctx *ctx)
{
}

void isp_intr_status(
	struct isp_ctx *ctx,
	union reg_isp_top_int_event0 *s0,
	union reg_isp_top_int_event1 *s1,
	union reg_isp_top_int_event2 *s2,
	union reg_isp_top_int_event0_fe345 *s0_fe345,
	union reg_isp_top_int_event1_fe345 *s1_fe345,
	union reg_isp_top_int_event2_fe345 *s2_fe345)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	s0->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event0);
	//clear isp top event0 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event0, s0->raw);

	s1->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event1);
	//clear isp top event1 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event1, s1->raw);

	s2->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event2);
	//clear isp top event2 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event2, s2->raw);

	s0_fe345->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event0_fe345);
	//clear isp top event0_fe345 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event0_fe345, s0_fe345->raw);

	s1_fe345->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event1_fe345);
	//clear isp top event1_fe345 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event1_fe345, s1_fe345->raw);

	s2_fe345->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event2_fe345);
	//clear isp top event2_fe345 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event2_fe345, s2_fe345->raw);
}

void isp_csi_intr_status(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	union reg_isp_csi_bdg_interrupt_status_0 *s0,
	union reg_isp_csi_bdg_interrupt_status_1 *s1)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t ba = ctx->phys_regs[id];

	if (ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
		s0->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_0);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_0, s0->raw);

		s1->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_1);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_1, s1->raw);
	} else {
		s0->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_t, interrupt_status_0);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_t, interrupt_status_0, s0->raw);

		s1->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_t, interrupt_status_1);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_t, interrupt_status_1, s1->raw);
	}
}

void isp_init(struct isp_ctx *ctx)
{
	u8 i = 0;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		g_w_bit[i] = MANR_W_BIT;
		g_h_bit[i] = MANR_H_BIT;
		g_rgbmap_chg_pre[i][0] = false;
		g_rgbmap_chg_pre[i][1] = false;

		g_lmp_cfg[i].pre_chg[0] = false;
		g_lmp_cfg[i].pre_chg[1] = false;
		g_lmp_cfg[i].pre_w_bit = LUMA_MAP_W_BIT;
		g_lmp_cfg[i].pre_h_bit = LUMA_MAP_H_BIT;

		g_lmp_cfg[i].post_w_bit = LUMA_MAP_W_BIT;
		g_lmp_cfg[i].post_h_bit = LUMA_MAP_H_BIT;
	}
}

void isp_streaming(struct isp_ctx *ctx, u32 on, enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t splt = ctx->phys_regs[ISP_BLK_ID_SPLT];
	union reg_isp_top_sw_ctrl_0 sw_ctrl_0;
	union reg_isp_top_sw_ctrl_1 sw_ctrl_1;
	union reg_isp_top_sw_ctrl_0_fe345 sw_ctrl_0_fe345;
	union reg_isp_top_sw_ctrl_1_fe345 sw_ctrl_1_fe345;
	union reg_isp_csi_bdg_top_ctrl csibdg_topctl;
	union reg_isp_csi_bdg_lite_bdg_top_ctrl csibdg_lite_topctl;

	sw_ctrl_0.raw = sw_ctrl_1.raw = 0;
	sw_ctrl_0_fe345.raw = sw_ctrl_1_fe345.raw = 0;

	if (raw_num >= ISP_PRERAW_LITE0 && raw_num <= ISP_PRERAW_LITE1) {
		csibdg_lite_topctl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl);
		csibdg_lite_topctl.bits.csi_up_reg = on;
		csibdg_lite_topctl.bits.mcsi_enable = on;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl, csibdg_lite_topctl.raw);
		return;
	}

	if (on) {
		if (raw_num == ISP_PRERAW0) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_fe0	= 3;
				sw_ctrl_1.bits.pq_up_fe0	= 3;
			} else {
				sw_ctrl_0.bits.shaw_up_fe0	= 1;
				sw_ctrl_1.bits.pq_up_fe0	= 1;
			}
		} else if (raw_num == ISP_PRERAW1) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_fe1	= 3;
				sw_ctrl_1.bits.pq_up_fe1	= 3;
			} else {
				sw_ctrl_0.bits.shaw_up_fe1	= 1;
				sw_ctrl_1.bits.pq_up_fe1	= 1;
			}
		} else if (raw_num == ISP_PRERAW2) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_fe2	= 3;
				sw_ctrl_1.bits.pq_up_fe2	= 3;
			} else {
				sw_ctrl_0.bits.shaw_up_fe2	= 1;
				sw_ctrl_1.bits.pq_up_fe2	= 1;
			}
		} else if (raw_num == ISP_PRERAW3) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0_fe345.bits.shaw_up_fe3 = 3;
				sw_ctrl_1_fe345.bits.pq_up_fe3   = 3;
			} else {
				sw_ctrl_0_fe345.bits.shaw_up_fe3 = 1;
				sw_ctrl_1_fe345.bits.pq_up_fe3   = 1;
			}
		} else if (raw_num == ISP_PRERAW4) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0_fe345.bits.shaw_up_fe4 = 3;
				sw_ctrl_1_fe345.bits.pq_up_fe4   = 3;
			} else {
				sw_ctrl_0_fe345.bits.shaw_up_fe4 = 1;
				sw_ctrl_1_fe345.bits.pq_up_fe4   = 1;
			}
		} else if (raw_num == ISP_PRERAW5) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0_fe345.bits.shaw_up_fe5 = 3;
				sw_ctrl_1_fe345.bits.pq_up_fe5   = 3;
			} else {
				sw_ctrl_0_fe345.bits.shaw_up_fe5 = 1;
				sw_ctrl_1_fe345.bits.pq_up_fe5   = 1;
			}
		}

		sw_ctrl_0.bits.shaw_up_be	= 1;
		sw_ctrl_1.bits.pq_up_be		= 1;
		sw_ctrl_0.bits.shaw_up_raw	= 1;
		sw_ctrl_1.bits.pq_up_raw	= 1;
		sw_ctrl_0.bits.shaw_up_post	= 1;
		sw_ctrl_1.bits.pq_up_post	= 1;

		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1_fe345, sw_ctrl_1_fe345.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0_fe345, sw_ctrl_0_fe345.raw);

		csibdg_topctl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
			csibdg_topctl.bits.csi_up_reg = 0;
			csibdg_topctl.bits.mcsi_enable = 0;
			csibdg_topctl.bits.tgen_enable = 0;
		} else {
			csibdg_topctl.bits.csi_up_reg = 1;
			csibdg_topctl.bits.mcsi_enable = 1;

			if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
				csibdg_topctl.bits.tgen_enable = 1;
			else
				csibdg_topctl.bits.tgen_enable = 0;
		}

		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, csibdg_topctl.raw);
	} else {
		csibdg_topctl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
		csibdg_topctl.bits.mcsi_enable = 0;
		csibdg_topctl.bits.tgen_enable = 0;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, csibdg_topctl.raw);

		ISP_WR_BITS(splt, reg_isp_line_spliter_t, enable, line_spliter_enable, 0);
	}
}

void isp_splt_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t splt = ctx->phys_regs[ISP_BLK_ID_SPLT];
	union reg_isp_line_spliter_frame_vld_ctrl frame_vld_ctrl;
	union reg_isp_line_spliter_vs_sw_ctrl vs_sw_ctrl;

	if (raw_num != ISP_PRERAW0) {
		vi_pr(VI_ERR, "only support fe_%d\n", ISP_PRERAW0);
		return;
	}

	vi_pr(VI_DBG, "trigger splt_%d frame_vld\n", raw_num);

	// spliter always need to set frame vld for receive vsync
	frame_vld_ctrl.raw = ISP_RD_REG(splt, reg_isp_line_spliter_t, frame_vld_ctrl);
	frame_vld_ctrl.bits.spliter_frame_vld_fe0 = 1;
	frame_vld_ctrl.bits.spliter_frame_vld_fe1 = ctx->isp_pipe_cfg[raw_num].is_tile;
	ISP_WR_REG(splt, reg_isp_line_spliter_t, frame_vld_ctrl, frame_vld_ctrl.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) { //dram->fe
		vs_sw_ctrl.raw = ISP_RD_REG(splt, reg_isp_line_spliter_t, vs_sw_ctrl);
		vs_sw_ctrl.bits.fe0_sw_vs_w1p = 1;
		vs_sw_ctrl.bits.fe1_sw_vs_w1p = ctx->isp_pipe_cfg[raw_num].is_tile;
		ISP_WR_REG(splt, reg_isp_line_spliter_t, vs_sw_ctrl, vs_sw_ctrl.raw);
	}
}

void isp_pre_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num, const u8 chn_num)
{
	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) { //dram->be
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union reg_isp_top_sw_ctrl_0 sw_ctrl_0;
		union reg_isp_top_sw_ctrl_1 sw_ctrl_1;

		if (ctx->is_suspend) {
			vi_pr(VI_ERR, "already trig preraw\n");
			return;
		}

		sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			sw_ctrl_0.bits.trig_str_be	= 0x3;
			sw_ctrl_1.bits.pq_up_be		= 0x3;
			sw_ctrl_0.bits.shaw_up_be	= 0x3;
			sw_ctrl_0.bits.trig_str_raw	= 0x1;
			sw_ctrl_0.bits.shaw_up_raw	= 0x1;
			sw_ctrl_1.bits.pq_up_raw	= 0x1;
			sw_ctrl_0.bits.trig_str_post	= 0x1;
			sw_ctrl_0.bits.shaw_up_post	= 0x1;
			sw_ctrl_1.bits.pq_up_post	= 0x1;
		} else {
			sw_ctrl_0.bits.trig_str_be	= 0x1;
			sw_ctrl_1.bits.pq_up_be		= 0x1;
			sw_ctrl_0.bits.shaw_up_be	= 0x1;
			sw_ctrl_0.bits.trig_str_raw	= 0x1;
			sw_ctrl_0.bits.shaw_up_raw	= 0x1;
			sw_ctrl_1.bits.pq_up_raw	= 0x1;
			sw_ctrl_0.bits.trig_str_post	= 0x1;
			sw_ctrl_0.bits.shaw_up_post	= 0x1;
			sw_ctrl_1.bits.pq_up_post	= 0x1;
		}
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);

		vi_pr(VI_DBG, "Raw replay trigger fe_%d\n", raw_num);
	} else if (ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
		int id = csibdg_find_hwid(raw_num);
		uintptr_t csibdg_lite = ctx->phys_regs[id];

		vi_pr(VI_DBG, "trigger csibdg_lite_%d chn_num_%d frame_vld\n", raw_num, chn_num);

		switch (chn_num) {
		case ISP_FE_CH0:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch0, 1);
			break;
		case ISP_FE_CH1:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch1, 1);
			break;
		case ISP_FE_CH2:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch2, 1);
			break;
		case ISP_FE_CH3:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch3, 1);
			break;
		default:
			break;
		}
	} else { // patgen or sensor->fe
		int id = fe_find_hwid(raw_num);
		uintptr_t fe = ctx->phys_regs[id];

		vi_pr(VI_DBG, "trigger fe_%d chn_num_%d frame_vld\n", raw_num, chn_num);

		switch (chn_num) {
		case ISP_FE_CH0:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch0, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch0, 1);
			break;
		case ISP_FE_CH1:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch1, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch1, 1);
			break;
		case ISP_FE_CH2:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch2, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch2, 1);
			break;
		case ISP_FE_CH3:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch3, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch3, 1);
			break;
		default:
			break;
		}
	}
}

static void _isp_cmdq_post_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num, u32 sw_ctrl_1, u32 sw_ctrl_0)
{
	uintptr_t cmqd = ctx->phys_regs[ISP_BLK_ID_CMDQ];
	u32 isptop_phy_reg = ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_ISPTOP;
	u16 cmd_idx = ctx->isp_pipe_cfg[raw_num].cmdq_buf.cmd_idx;
	union cmdq_set *cmd_start = (union cmdq_set *)ctx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr;

	vi_ion_cache_invalidate(ctx->isp_pipe_cfg[raw_num].cmdq_buf.phy_addr,
				  ctx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr,
				  ctx->isp_pipe_cfg[raw_num].cmdq_buf.buf_size);

	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 isptop_phy_reg + _OFST(reg_isp_top_t, sw_ctrl_1), sw_ctrl_1);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 isptop_phy_reg + _OFST(reg_isp_top_t, sw_ctrl_0), sw_ctrl_0);

	cmd_start[cmd_idx - 1].reg.intr_end = 1;
	cmd_start[cmd_idx - 1].reg.intr_last = 1;

	vi_ion_cache_flush(ctx->isp_pipe_cfg[raw_num].cmdq_buf.phy_addr,
			     ctx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr,
			     ctx->isp_pipe_cfg[raw_num].cmdq_buf.buf_size);

	cmdq_intr_ctrl(cmqd, 0x1F);
	cmdq_engine(cmqd, (uintptr_t)ctx->isp_pipe_cfg[raw_num].cmdq_buf.phy_addr,
		    (ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CMDQ) >> 22, true, false, cmd_idx);
	//reset cmd_idx
	ctx->isp_pipe_cfg[raw_num].cmdq_buf.cmd_idx = 0;
}

void isp_post_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	//uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	union reg_isp_top_sw_ctrl_0 sw_ctrl_0;
	union reg_isp_top_sw_ctrl_1 sw_ctrl_1;

	sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

	if (ctx->is_suspend) {
		vi_pr(VI_ERR, "already trig postraw\n");
		return;
	}

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
		vi_pr(VI_DBG, "dram->post trig raw_num(%d), is_slice_buf_on(%d)\n",
				raw_num, ctx->is_slice_buf_on);
		sw_ctrl_0.bits.shaw_up_raw	= 1;
		sw_ctrl_0.bits.trig_str_raw	= 1;
		sw_ctrl_1.bits.pq_up_raw	= 1;
		sw_ctrl_0.bits.shaw_up_post	= 1;
		sw_ctrl_0.bits.trig_str_post	= 1;
		sw_ctrl_1.bits.pq_up_post	= 1;

		// ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		// ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);

		_isp_cmdq_post_trig(ctx, raw_num, sw_ctrl_1.raw, sw_ctrl_0.raw);
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		vi_pr(VI_DBG, "dram->be post trig raw_num(%d), is_hdr_on(%d)\n",
				raw_num, ctx->isp_pipe_cfg[raw_num].is_hdr_on);

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_be	= 3;
				sw_ctrl_0.bits.trig_str_be	= 3;
				sw_ctrl_1.bits.pq_up_be		= 3;
				sw_ctrl_0.bits.shaw_up_raw	= 1;
				sw_ctrl_0.bits.trig_str_raw	= 1;
				sw_ctrl_1.bits.pq_up_raw	= 1;
				sw_ctrl_0.bits.shaw_up_post	= 1;
				sw_ctrl_0.bits.trig_str_post	= 1;
				sw_ctrl_1.bits.pq_up_post	= 1;
			} else {
				sw_ctrl_0.bits.shaw_up_be	= 1;
				sw_ctrl_0.bits.trig_str_be	= 1;
				sw_ctrl_1.bits.pq_up_be		= 1;
				sw_ctrl_0.bits.shaw_up_raw	= 1;
				sw_ctrl_0.bits.trig_str_raw	= 1;
				sw_ctrl_1.bits.pq_up_raw	= 1;
				sw_ctrl_0.bits.shaw_up_post	= 1;
				sw_ctrl_0.bits.trig_str_post	= 1;
				sw_ctrl_1.bits.pq_up_post	= 1;
			}
		} else {
			sw_ctrl_0.bits.shaw_up_raw	= 1;
			sw_ctrl_0.bits.trig_str_raw	= 1;
			sw_ctrl_1.bits.pq_up_raw	= 1;
			sw_ctrl_0.bits.shaw_up_post	= 1;
			sw_ctrl_0.bits.trig_str_post	= 1;
			sw_ctrl_1.bits.pq_up_post	= 1;
		}

		// ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		// ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);
		_isp_cmdq_post_trig(ctx, raw_num, sw_ctrl_1.raw, sw_ctrl_0.raw);

	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { //slice buffer path
		vi_pr(VI_DBG, "dram->post trig raw_num(%d), is_slice_buf_on(%d)\n",
				raw_num, ctx->is_slice_buf_on);

		isp_slice_buf_trig(ctx, ISP_PRERAW0);
	}
}

/*********************************************************************************
 *	Common IPs for subsys
 ********************************************************************************/
struct isp_grid_s_info ispblk_lmap_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	struct isp_grid_s_info dummy = {0};
	return dummy;
}

u64 ispblk_dma_getaddr(struct isp_ctx *ctx, u32 dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u64 addr_h = ISP_RD_BITS(dmab, reg_isp_dma_ctl_t, sys_control, baseh);

	return ((u64)ISP_RD_REG(dmab, reg_isp_dma_ctl_t, base_addr) | (addr_h << 32));
}

int ispblk_dma_buf_get_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid)
{
	u32 len = 0, num = 0, w;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	{
		/* csibdg */
		w = ctx->isp_pipe_cfg[raw_num].is_tile
			? (ctx->tile_cfg.r_out.end + 1)
			: ctx->isp_pipe_cfg[raw_num].crop.w;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG1:
	{
		/* csibdg */
		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE:
	case ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE:
	case ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_LE:
	case ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_SE:
	case ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_LE:
	case ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_SE:
	case ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_LE:
	case ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_SE:
	case ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_LE:
	case ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_SE:
	case ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_LE:
	case ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_SE:
	{
		//rgbmap max size
		if (ctx->is_rgbmap_sbm_on) {
			if (dmaid == ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE)
				len = slc_b_cfg.sub_path.le_buf_size;
			else if (dmaid == ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE)
				len = slc_b_cfg.sub_path.se_buf_size;
			num = 1;
		} else {
			u8 grid_size = RGBMAP_MAX_BIT;

			w = ctx->isp_pipe_cfg[raw_num].is_tile
				? (ctx->tile_cfg.r_out.end + 1)
				: ctx->isp_pipe_cfg[raw_num].crop.w;

			len = (((UPPER(w, grid_size)) * 6 + 15) >> 4) << 4;
			num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, grid_size);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE:
	{
		if (ctx->is_slice_buf_on) {
			if (dmaid == ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE)
				len = slc_b_cfg.main_path.le_buf_size;
			else
				len = slc_b_cfg.main_path.se_buf_size;
			num = 1;
		} else {
			u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;
			u32 w = 0;

			if (ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_be) {
				w = ctx->isp_pipe_cfg[ISP_PRERAW0].csibdg_width;
				num = ctx->isp_pipe_cfg[ISP_PRERAW0].csibdg_height;
				len = 3 * UPPER(w, 1);
			} else {
				w = ctx->img_width;
				num = ctx->img_height;
				len = 3 * UPPER(w, dpcm_on);
			}
		}
		break;
	}
	case ISP_BLK_ID_DMA_CTL_AF_W:
	{
		/* af */
		len = ctx->isp_pipe_cfg[raw_num].is_tile ? VI_ALIGN(AF_DMA_SIZE) + AF_DMA_SIZE : AF_DMA_SIZE;

		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_RGBIR_LE:
	case ISP_BLK_ID_DMA_CTL_RGBIR_SE:
	{
		/* ir wdma max size*/
		// default 12bit
		w = ctx->isp_pipe_cfg[raw_num].is_tile
			? (ctx->tile_cfg.r_out.end + 1) >> 1
			: ctx->isp_pipe_cfg[raw_num].crop.w >> 1;
		num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;
		len = 3 * UPPER(w, 1);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LSC_LE:
	case ISP_BLK_ID_DMA_CTL_LSC_SE:
	{
		/* lsc rdma */
		// fixed value for 37x37

		num = 0xde;
		len = 0x40;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_GMS:
	{
		/* gms */
		len = ctx->isp_pipe_cfg[raw_num].is_tile ? (VI_ALIGN(GMS_SEC_SIZE) + GMS_SEC_SIZE) : GMS_SEC_SIZE;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AE_HIST_LE:
	case ISP_BLK_ID_DMA_CTL_AE_HIST_SE:
	{
		len = ctx->isp_pipe_cfg[raw_num].is_tile ? (VI_ALIGN(AE_DMA_SIZE) + AE_DMA_SIZE) : AE_DMA_SIZE;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LMAP_LE:
	case ISP_BLK_ID_DMA_CTL_LMAP_SE:
	{
		// lmap max size
		w = ctx->isp_pipe_cfg[raw_num].is_tile
			? (ctx->tile_cfg.r_out.end + 1)
			: ctx->isp_pipe_cfg[raw_num].crop.w;
		len = (((UPPER(w, 3)) + 15) >> 4) << 4;
		num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, 3);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_MMAP_IIR_R:
	case ISP_BLK_ID_DMA_CTL_MMAP_IIR_W:
	case ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP:
	{
		/* manr rdma */
		u8 grid_size = RGBMAP_MAX_BIT;

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			w = (!ctx->isp_pipe_cfg[raw_num].is_work_on_r_tile)
				? (ctx->tile_cfg.l_in.end - ctx->isp_pipe_cfg[raw_num].crop.x + 1)
				: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start + 1);
		} else {
			w = ctx->isp_pipe_cfg[raw_num].crop.w;
		}

		len = (((UPPER(w, grid_size) << 4) + 127) >> 7) << 4;
		num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, grid_size);
		vi_pr(VI_DBG, "raw_%d len=%d num=%d\n", raw_num, len, num);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_HIST_EDGE_V:
	{
		len = 2048;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_C:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_C:
	{
		//TNR UV
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.c_buf_size;
			num = 1;
		} else {
			if (ctx->isp_pipe_cfg[raw_num].is_tile) {
				w = (!ctx->isp_pipe_cfg[raw_num].is_work_on_r_tile)
					? (ctx->tile_cfg.l_in.end - ctx->isp_pipe_cfg[raw_num].crop.x + 1)
					: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start + 1);
			} else {
				w = ctx->isp_pipe_cfg[raw_num].crop.w;
			}

			len = (((((w) << 3) + 127) >> 7) << 7) >> 3;
			num = (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp)
				? (ctx->isp_pipe_cfg[raw_num].crop.h)
				: (ctx->isp_pipe_cfg[raw_num].crop.h >> 1);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_Y:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_Y:
	{
		//TNR Y
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.y_buf_size;
			num = 1;
		} else {
			if (ctx->isp_pipe_cfg[raw_num].is_tile) {
				w = (!ctx->isp_pipe_cfg[raw_num].is_work_on_r_tile)
					? (ctx->tile_cfg.l_in.end - ctx->isp_pipe_cfg[raw_num].crop.x + 1)
					: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start + 1);
			} else {
				w = ctx->isp_pipe_cfg[raw_num].crop.w;
			}

			len = (((((w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[raw_num].crop.h;
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y:
	{
		//TNR AI ISP Y
		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		len = ((((w << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U:
	case ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V:
	{
		//TNR AI ISP U/V
		w = ctx->isp_pipe_cfg[raw_num].crop.w >> 1;
		len = ((((w << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MO:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_MO:
	{
		//TNR Mo
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			w = (!ctx->isp_pipe_cfg[raw_num].is_work_on_r_tile)
				? (ctx->tile_cfg.l_in.end - ctx->isp_pipe_cfg[raw_num].crop.x + 1)
				: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start + 1);
		} else {
			w = ctx->isp_pipe_cfg[raw_num].crop.w;
		}

		len = (((((w) << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_DCI:
	{
		// dci
		len = ctx->isp_pipe_cfg[raw_num].is_tile ? (VI_ALIGN(DCI_DMA_SIZE) + DCI_DMA_SIZE) : DCI_DMA_SIZE;
		num = 0x1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LDCI_W:
	case ISP_BLK_ID_DMA_CTL_LDCI_R:
	{
		len = 0x300;
		num = 0x1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_SE:
	{
		// only ai_isp_raw need write to dram, and always used compressless mode.
		u32 dpcm_on = 1;

		w = ctx->isp_pipe_cfg[raw_num].max_width;
		num = ctx->isp_pipe_cfg[raw_num].max_height;
		len = 3 * UPPER(w, dpcm_on);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_SE:
	{
		u32 dpcm_on = (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe && ctx->is_dpcm_on) ? 2 : 1;

		w = ctx->isp_pipe_cfg[raw_num].max_width;
		num = ctx->isp_pipe_cfg[raw_num].max_height;
		len = 3 * UPPER(w, dpcm_on);
		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	vi_pr(VI_DBG, "dmaid=%d, size=%d\n", dmaid, len * num);

	return len * num;
}

void ispblk_dma_setaddr(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, base_addr, (buf_addr & 0xFFFFFFFF));
	ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, sys_control, baseh, ((buf_addr >> 32) & 0xFFFFFFFF));
}

void ispblk_dma_set_sw_mode(struct isp_ctx *ctx, u32 dmaid, bool is_sw_mode)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	//SW mode: config by SW
	sys_ctrl.raw = ISP_RD_REG(dmab, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= is_sw_mode;
	sys_ctrl.bits.seglen_sel	= is_sw_mode;
	sys_ctrl.bits.segnum_sel	= is_sw_mode;
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);
}

void ispblk_rgbmap_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 grid_size = (1 << g_w_bit[raw_num]);
	u32 w = 0, stride = 0, seglen = 0, segnum = 0;

	w = ctx->isp_pipe_cfg[raw_num].crop.w;
	seglen = ((w + grid_size - 1) / grid_size) * 6;

	if (ctx->isp_pipe_cfg[raw_num].is_tile)
		w = ctx->tile_cfg.r_out.end + 1;

	stride = ((((w + grid_size - 1) / grid_size) * 6 + 15) / 16) * 16;
	segnum = ((ctx->isp_pipe_cfg[raw_num].crop.h + grid_size - 1) / grid_size);

	vi_pr(VI_DBG, "raw_%d seglen = %d, stride = %d, num = %d\n", raw_num, seglen, stride, segnum);

	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, stride);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_seglen, seglen);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_segnum, segnum);
}

void ispblk_mmap_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 grid_size = (1 << ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit);
	u32 w = ctx->isp_pipe_cfg[raw_num].crop.w, stride = 0;

	if (ctx->isp_pipe_cfg[raw_num].is_tile)
		w = ctx->isp_pipe_cfg[raw_num].max_width;

	stride = ((((w + grid_size - 1) / grid_size) * 6 + 15) / 16) * 16;

	vi_pr(VI_DBG, "raw_%d stride = %d\n", raw_num, stride);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, stride);
}

int ispblk_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid, u64 buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 w = 0, len = 0, stride = 0, num = 0;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE:
	{
		// preraw be read/write dma
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
			w = ctx->isp_pipe_cfg[raw_num].csibdg_width;
			num = ctx->isp_pipe_cfg[raw_num].csibdg_height;
			len = 3 * UPPER(w, 1);
			stride = len;
		} else {
			w = ctx->img_width;
			num = ctx->img_height;
			len = 3 * UPPER(w, dpcm_on);
			if (ctx->isp_pipe_cfg[raw_num].is_tile)
				stride = 3 * UPPER(ctx->tile_cfg.r_out.end + 1, 1);
			else
				stride = len;
		}

		vi_pr(VI_DBG, "vi_sel raw_%d w = %d, num = %d, stride = %d, len = %d\n",
			raw_num, w, num, stride, len);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG1:
	{
		/* csibdg */
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			w = ctx->isp_pipe_cfg[raw_num].crop.w;
			num = ctx->isp_pipe_cfg[raw_num].crop.h;
			len = 3 * UPPER(w, 1);
			stride = 3 * UPPER(ctx->tile_cfg.r_out.end + 1, 1);

			vi_pr(VI_DBG, "line spliter raw_%d w = %d, num = %d, stride = %d, len = %d\n",
				raw_num, w, num, stride, len);
		} else {
			w = ctx->isp_pipe_cfg[raw_num].crop.w;
			num = ctx->isp_pipe_cfg[raw_num].crop.h;
			if (ctx->is_dpcm_on)
				w >>= 1;

			len = 3 * UPPER(w, 1);
			stride = len;
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AF_W:
	{
		/* af */
		num = 1;
		len = AF_DMA_SIZE;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_RGBIR_LE:
	case ISP_BLK_ID_DMA_CTL_RGBIR_SE:
	{
		/* ir wdma */
		// default 12bit
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			w = (!ctx->is_work_on_r_tile)
				? (ctx->tile_cfg.l_in.end - ctx->isp_pipe_cfg[raw_num].crop.x + 1) >> 1
				: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start + 1) >> 1;

			num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;
			len = 3 * UPPER(w, 1);
			stride = 3 * UPPER((ctx->tile_cfg.r_out.end + 1) >> 1, 1);
		} else {
			w = ctx->isp_pipe_cfg[raw_num].crop.w >> 1;
			num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;
			len = 3 * UPPER(w, 1);
			stride = len;
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LSC_LE:
	case ISP_BLK_ID_DMA_CTL_LSC_SE:
	{
		/* lsc rdma */
		// fixed value for 37x37

		num = 0xde;
		len = 0x3a;
		stride = 0x40;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_GMS:
	{
		/* gms */
		uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_GMS];

		u32 x_sec_size = ISP_RD_REG(sts, reg_isp_gms_t, gms_x_sizem1);
		u32 y_sec_size = ISP_RD_REG(sts, reg_isp_gms_t, gms_y_sizem1);
		u32 sec_size = (x_sec_size >= y_sec_size) ? x_sec_size : y_sec_size;

		num = 1;
		len = (((sec_size + 1) >> 1) << 5) * 3;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AE_HIST_LE:
	case ISP_BLK_ID_DMA_CTL_AE_HIST_SE:
	{
		num = 1;
		len = AE_DMA_SIZE;
		stride = len;

		break;
	}

	case ISP_BLK_ID_DMA_CTL_RAW_RDMA0:
	case ISP_BLK_ID_DMA_CTL_RAW_RDMA1:
	{
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		len = 3 * UPPER(w, dpcm_on);
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R:
	case ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R:
	case ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R:
	case ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R:
	{
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MMAP];

		u16 w_bit = ISP_RD_BITS(blk, reg_isp_mmap_t, reg_60, rgbmap_w_bit);
		u16 h_bit = ISP_RD_BITS(blk, reg_isp_mmap_t, reg_60, rgbmap_h_bit);

		len = ((UPPER(ctx->img_width, w_bit) * 48 + 127) >> 7) << 4;
		num = UPPER(ctx->img_height, h_bit);

		if (ctx->isp_pipe_cfg[raw_num].is_tile)
			stride = ((UPPER(ctx->isp_pipe_cfg[raw_num].max_width, w_bit) * 48 + 127) >> 7) << 4;
		else
			stride = len;

		vi_pr(VI_DBG, "raw_%d mmap len=%d num=%d stride=%d addr=%llx\n",
			raw_num, len, num, stride, buf_addr);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_MMAP_IIR_R:
	case ISP_BLK_ID_DMA_CTL_MMAP_IIR_W:
	case ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP:
	{
		/* manr rdma */
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MMAP];

		u16 w_bit = ISP_RD_BITS(blk, reg_isp_mmap_t, reg_60, rgbmap_w_bit);
		u16 h_bit = ISP_RD_BITS(blk, reg_isp_mmap_t, reg_60, rgbmap_h_bit);

		len = (((UPPER(ctx->img_width, w_bit) << 4) + 127) >> 7) << 4;
		num = UPPER(ctx->img_height, h_bit);
		stride = len;

		vi_pr(VI_DBG, "raw_%d mmap iir len=%d num=%d stride=%d addr=%llx\n",
			raw_num, len, num, stride, buf_addr);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_C:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_C:
	{
		//TNR UV
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.c_bs_size;
			num = 1;
		} else {
			len = (((((ctx->img_width) << 3) + 127) >> 7) << 7) >> 3;
			num = (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp)
				? (ctx->img_height)
				: (ctx->img_height >> 1);
		}
		stride = len;

		vi_pr(VI_DBG, "raw_%d tnr c len=%d num=%d stride=%d buf_addr=%llx\n",
			raw_num, len, num, stride, buf_addr);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_Y:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_Y:
	{
		//TNR Y
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.y_bs_size;
			num = 1;
		} else {
			len = (((((ctx->img_width) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->img_height;
		}
		stride = len;

		vi_pr(VI_DBG, "raw_%d tnr y len=%d num=%d stride=%d buf_addr=%llx\n",
			raw_num, len, num, stride, buf_addr);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y:
	{
		//TNR AI ISP Y
		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		len = ((((w << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U:
	case ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V:
	{
		//TNR AI ISP U/V
		w = ctx->isp_pipe_cfg[raw_num].crop.w >> 1;
		len = ((((w << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MO:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_MO:
	{
		//TNR Mo
		len = (((((ctx->img_width) << 3) + 127) >> 7) << 7) >> 3;
		num = ctx->img_height;
		stride = len;

		vi_pr(VI_DBG, "raw_%d tnr mo len=%d num=%d stride=%d\n", raw_num, len, num, stride);
		break;
	}
	case ISP_BLK_ID_DMA_CTL_DCI:
	{
		// dci
		len = DCI_DMA_SIZE;
		num = 0x1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_Y:
	{
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			if (ctx->isp_pipe_cfg[raw_num].is_postout_crop) {
				if (!ctx->is_work_on_r_tile) { //left tile
					len = (ctx->isp_pipe_cfg[raw_num].postout_crop.x +
						ctx->isp_pipe_cfg[raw_num].postout_crop.w > ctx->tile_cfg.r_out.start)
						? ctx->tile_cfg.r_out.start - ctx->isp_pipe_cfg[raw_num].postout_crop.x
						: ctx->isp_pipe_cfg[raw_num].postout_crop.w;
				} else { //right tile
					len = (ctx->isp_pipe_cfg[raw_num].postout_crop.x < ctx->tile_cfg.r_out.start)
						? (ctx->isp_pipe_cfg[raw_num].postout_crop.x +
						ctx->isp_pipe_cfg[raw_num].postout_crop.w - ctx->tile_cfg.r_out.start)
						: ctx->isp_pipe_cfg[raw_num].postout_crop.w;
				}

				stride = ctx->isp_pipe_cfg[raw_num].postout_crop.w;
			} else {
				len = (!ctx->is_work_on_r_tile)
					? (ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start + 1)
					: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start + 1);
				//stride need 16byte align
				stride = ctx->isp_pipe_cfg[raw_num].max_width;
			}
		} else {
			//yuvtop y out
			len = (ctx->isp_pipe_cfg[raw_num].postout_crop.w) ?
				ctx->isp_pipe_cfg[raw_num].postout_crop.w : ctx->img_width;
			stride = len;
		}

		num = (ctx->isp_pipe_cfg[raw_num].postout_crop.h) ?
				ctx->isp_pipe_cfg[raw_num].postout_crop.h : ctx->img_height;

		vi_pr(VI_DBG, "cropout raw_num len=%d stride=%d buffer_addr=0x%llx\n", len, stride, buf_addr);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_C:
	{
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			if (ctx->isp_pipe_cfg[raw_num].is_postout_crop) {
				if (!ctx->is_work_on_r_tile) { //left tile
					len = (ctx->isp_pipe_cfg[raw_num].postout_crop.x
						+ ctx->isp_pipe_cfg[raw_num].postout_crop.w) > ctx->tile_cfg.r_out.start
						? ctx->tile_cfg.r_out.start - ctx->isp_pipe_cfg[raw_num].postout_crop.x
						: ctx->isp_pipe_cfg[raw_num].postout_crop.w;
				} else { //right tile
					len = (ctx->isp_pipe_cfg[raw_num].postout_crop.x < ctx->tile_cfg.r_out.start)
						? (ctx->isp_pipe_cfg[raw_num].postout_crop.x +
						ctx->isp_pipe_cfg[raw_num].postout_crop.w - ctx->tile_cfg.r_out.start)
						: ctx->isp_pipe_cfg[raw_num].postout_crop.w;
				}

				stride = ctx->isp_pipe_cfg[raw_num].postout_crop.w;
			} else {
				len = (!ctx->is_work_on_r_tile)
					? (ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start + 1)
					: (ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start + 1);

				stride = ctx->isp_pipe_cfg[raw_num].max_width;
			}
		} else {
			//yuvtop uv out
			len = (ctx->isp_pipe_cfg[raw_num].postout_crop.w) ?
				ctx->isp_pipe_cfg[raw_num].postout_crop.w : ctx->img_width;
			stride = len;
		}

		num = (ctx->isp_pipe_cfg[raw_num].postout_crop.h) ?
				(ctx->isp_pipe_cfg[raw_num].postout_crop.h >> 1) : (ctx->img_height >> 1);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LDCI_W:
	case ISP_BLK_ID_DMA_CTL_LDCI_R:
	{
		len = 0x300;
		num = 0x1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_SE:
	{
		// only ai_isp_raw need write to dram, and always used compressless mode.
		u32 dpcm_on = 1;

		w = ctx->isp_pipe_cfg[raw_num].csibdg_width;
		num = ctx->isp_pipe_cfg[raw_num].csibdg_height;
		len = 3 * UPPER(w, dpcm_on);

		if (ctx->isp_pipe_cfg[raw_num].is_tile)
			stride = 3 * UPPER(ctx->isp_pipe_cfg[raw_num].max_width, dpcm_on);
		else
			stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_LE:
	case ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_SE:
	{
		u32 dpcm_on = (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe && ctx->is_dpcm_on) ? 2 : 1;

		w = ctx->isp_pipe_cfg[raw_num].csibdg_width;
		num = ctx->isp_pipe_cfg[raw_num].csibdg_height;
		len = 3 * UPPER(w, dpcm_on);

		if (ctx->isp_pipe_cfg[raw_num].is_tile)
			stride = 3 * UPPER(ctx->isp_pipe_cfg[raw_num].max_width, dpcm_on);
		else
			stride = len;

		break;
	}
	default:
		break;
	}

	stride = VI_ALIGN(stride);

	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_seglen, len);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, stride);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_segnum, num);

	if (buf_addr)
		ispblk_dma_setaddr(ctx, dmaid, buf_addr);

	return len * num;
}

void ispblk_dma_enable(struct isp_ctx *ctx, u32 dmaid, u32 on, u8 dma_disable)
{
	uintptr_t srcb = 0;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_Y:
		/* yuvtop y crop4 */
		srcb = ctx->phys_regs[ISP_BLK_ID_YUV_CROP_Y];
		break;
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_C:
		/* yuvtop uv crop5 */
		srcb = ctx->phys_regs[ISP_BLK_ID_YUV_CROP_C];
		break;
	case ISP_BLK_ID_DMA_CTL_RAW_RDMA0:
		/* yuvtop uv crop5 */
		srcb = ctx->phys_regs[ISP_BLK_ID_RAW_CROP_LE];
		break;
	case ISP_BLK_ID_DMA_CTL_RAW_RDMA1:
		/* yuvtop uv crop5 */
		srcb = ctx->phys_regs[ISP_BLK_ID_RAW_CROP_SE];
		break;
	default:
		break;
	}

	if (srcb) {
		ISP_WR_BITS(srcb, reg_crop_t, reg_0, dma_enable, on);
		ISP_WR_BITS(srcb, reg_crop_t, debug, force_dma_disable, dma_disable);
	}
}

void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];

	ISP_WR_BITS(cropb, reg_crop_t, reg_0, crop_enable, en);
}

int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vi_rect crop)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];
	union reg_crop_1 reg1;
	union reg_crop_2 reg2;

	// crop out size
	reg1.bits.crop_start_y = crop.y;
	reg1.bits.crop_end_y = crop.y + crop.h - 1;
	reg2.bits.crop_start_x = crop.x;
	reg2.bits.crop_end_x = crop.x + crop.w - 1;
	ISP_WR_REG(cropb, reg_crop_t, reg_1, reg1.raw);
	ISP_WR_REG(cropb, reg_crop_t, reg_2, reg2.raw);
	ISP_WR_BITS(cropb, reg_crop_t, reg_0, crop_enable, true);

	return 0;
}

int bayer_type_mapping(enum isp_bayer_type_e bayer_type)
{
	int mapping = bayer_type;

	if (bayer_type == ISP_BAYER_TYPE_IGRGB || bayer_type == ISP_BAYER_TYPE_IGBGR)
		mapping = ISP_BAYER_TYPE_BG;
	else if (bayer_type == ISP_BAYER_TYPE_IRGBG || bayer_type == ISP_BAYER_TYPE_IBGRG)
		mapping = ISP_BAYER_TYPE_GB;
	else if (bayer_type == ISP_BAYER_TYPE_GRGBI || bayer_type == ISP_BAYER_TYPE_GBGRI)
		mapping = ISP_BAYER_TYPE_GR;
	else if (bayer_type == ISP_BAYER_TYPE_RGBGI || bayer_type == ISP_BAYER_TYPE_BGRGI)
		mapping = ISP_BAYER_TYPE_RG;

	return mapping;
}

int csibdg_find_hwid(enum sop_isp_raw raw_num)
{
	int csibdg_id = -1;

	switch (raw_num) {
	case ISP_PRERAW0:
		csibdg_id = ISP_BLK_ID_CSIBDG0;
		break;
	case ISP_PRERAW1:
		csibdg_id = ISP_BLK_ID_CSIBDG1;
		break;
	case ISP_PRERAW2:
		csibdg_id = ISP_BLK_ID_CSIBDG2;
		break;
	case ISP_PRERAW3:
		csibdg_id = ISP_BLK_ID_CSIBDG3;
		break;
	case ISP_PRERAW4:
		csibdg_id = ISP_BLK_ID_CSIBDG4;
		break;
	case ISP_PRERAW5:
		csibdg_id = ISP_BLK_ID_CSIBDG5;
		break;
	case ISP_PRERAW_LITE0:
		csibdg_id = ISP_BLK_ID_CSIBDG0_LITE;
		break;
	case ISP_PRERAW_LITE1:
		csibdg_id = ISP_BLK_ID_CSIBDG1_LITE;
		break;
	default:
		break;
	}

	return csibdg_id;
}

int fe_find_hwid(enum sop_isp_raw raw_num)
{
	int fe_id = -1;

	switch (raw_num) {
	case ISP_PRERAW0:
		fe_id = ISP_BLK_ID_PRE_RAW_FE0;
		break;
	case ISP_PRERAW1:
		fe_id = ISP_BLK_ID_PRE_RAW_FE1;
		break;
	case ISP_PRERAW2:
		fe_id = ISP_BLK_ID_PRE_RAW_FE2;
		break;
	case ISP_PRERAW3:
		fe_id = ISP_BLK_ID_PRE_RAW_FE3;
		break;
	case ISP_PRERAW4:
		fe_id = ISP_BLK_ID_PRE_RAW_FE4;
		break;
	case ISP_PRERAW5:
		fe_id = ISP_BLK_ID_PRE_RAW_FE5;
		break;
	default:
		break;
	}

	return fe_id;
}

int blc_find_hwid(int id)
{
	int blc_id = -1;

	switch (id) {
	case ISP_BLC_ID_FE0_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE0_BLC0;
		break;
	case ISP_BLC_ID_FE0_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE0_BLC1;
		break;
	case ISP_BLC_ID_FE1_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE1_BLC0;
		break;
	case ISP_BLC_ID_FE1_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE1_BLC1;
		break;
	case ISP_BLC_ID_FE2_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE2_BLC0;
		break;
	case ISP_BLC_ID_FE2_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE2_BLC1;
		break;
	case ISP_BLC_ID_FE3_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE3_BLC0;
		break;
	case ISP_BLC_ID_FE3_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE3_BLC1;
		break;
	case ISP_BLC_ID_FE4_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE4_BLC0;
		break;
	case ISP_BLC_ID_FE4_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE4_BLC1;
		break;
	case ISP_BLC_ID_FE5_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE5_BLC0;
		break;
	case ISP_BLC_ID_FE5_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_FE5_BLC1;
		break;
	case ISP_BLC_ID_BE_LE:
		blc_id = ISP_BLK_ID_PRE_RAW_BE_BLC0;
		break;
	case ISP_BLC_ID_BE_SE:
		blc_id = ISP_BLK_ID_PRE_RAW_BE_BLC1;
		break;
	default:
		break;
	}

	return blc_id;
}

int rgbmap_find_hwid(int id)
{
	int rgbmap_id = -1;

	switch (id) {
	case ISP_RGBMAP_ID_FE0_LE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE0_LE;
		break;
	case ISP_RGBMAP_ID_FE0_SE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE0_SE;
		break;
	case ISP_RGBMAP_ID_FE1_LE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE1_LE;
		break;
	case ISP_RGBMAP_ID_FE1_SE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE1_SE;
		break;
	case ISP_RGBMAP_ID_FE2_LE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE2_LE;
		break;
	case ISP_RGBMAP_ID_FE2_SE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE2_SE;
		break;
	case ISP_RGBMAP_ID_FE3_LE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE3_LE;
		break;
	case ISP_RGBMAP_ID_FE3_SE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE3_SE;
		break;
	case ISP_RGBMAP_ID_FE4_LE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE4_LE;
		break;
	case ISP_RGBMAP_ID_FE4_SE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE4_SE;
		break;
	case ISP_RGBMAP_ID_FE5_LE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE5_LE;
		break;
	case ISP_RGBMAP_ID_FE5_SE:
		rgbmap_id = ISP_BLK_ID_RGBMAP_FE5_SE;
		break;
	default:
		break;
	}

	return rgbmap_id;
}

int wbg_find_hwid(int id)
{
	int wbg_id = -1;

	switch (id) {
	case ISP_WBG_ID_FE0_LE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG0;
		break;
	case ISP_WBG_ID_FE0_SE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG1;
		break;
	case ISP_WBG_ID_FE1_LE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG2;
		break;
	case ISP_WBG_ID_FE1_SE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG3;
		break;
	case ISP_WBG_ID_FE2_LE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG4;
		break;
	case ISP_WBG_ID_FE2_SE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG5;
		break;
	case ISP_WBG_ID_FE3_LE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG6;
		break;
	case ISP_WBG_ID_FE3_SE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG7;
		break;
	case ISP_WBG_ID_FE4_LE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG8;
		break;
	case ISP_WBG_ID_FE4_SE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG9;
		break;
	case ISP_WBG_ID_FE5_LE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG10;
		break;
	case ISP_WBG_ID_FE5_SE:
		wbg_id = ISP_BLK_ID_RGBMAP_WBG11;
		break;
	case ISP_WBG_ID_RAW_TOP_LE:
		wbg_id = ISP_BLK_ID_RAW_WBG0;
		break;
	case ISP_WBG_ID_RAW_TOP_SE:
		wbg_id = ISP_BLK_ID_RAW_WBG1;
		break;
	default:
		break;
	}

	return wbg_id;
}

int csibdg_lite_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num)
{
	int dma_id = -1;

	switch (raw_num) {
	case ISP_PRERAW_LITE0:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_BT0_LITE0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_BT0_LITE1;
		else if (chn_num == ISP_FE_CH2)
			dma_id = ISP_BLK_ID_DMA_CTL_BT0_LITE2;
		else if (chn_num == ISP_FE_CH3)
			dma_id = ISP_BLK_ID_DMA_CTL_BT0_LITE3;
		break;
	case ISP_PRERAW_LITE1:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_BT1_LITE0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_BT1_LITE1;
		else if (chn_num == ISP_FE_CH2)
			dma_id = ISP_BLK_ID_DMA_CTL_BT1_LITE2;
		else if (chn_num == ISP_FE_CH3)
			dma_id = ISP_BLK_ID_DMA_CTL_BT1_LITE3;
		break;
	default:
		break;
	}

	return dma_id;
}


int csibdg_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num)
{
	int dma_id = -1;

	switch (raw_num) {
	case ISP_PRERAW0:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG1;
		else if (chn_num == ISP_FE_CH2)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG2;
		else if (chn_num == ISP_FE_CH3)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG3;
		break;
	case ISP_PRERAW1:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI1_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI1_BDG1;
		else if (chn_num == ISP_FE_CH2)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI1_BDG2;
		else if (chn_num == ISP_FE_CH3)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI1_BDG3;
		break;
	case ISP_PRERAW2:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI2_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI2_BDG1;
		break;
	case ISP_PRERAW3:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI3_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI3_BDG1;
		break;
	case ISP_PRERAW4:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI4_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI4_BDG1;
		break;
	case ISP_PRERAW5:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI5_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI5_BDG1;
		break;
	default:
		break;
	}

	return dma_id;
}

int rgbmap_dma_find_hwid(enum sop_isp_raw raw_num, enum isp_raw_path_e path)
{
	int dma_id = -1;

	switch (raw_num) {
	case ISP_PRERAW0:
		dma_id = (path == ISP_RAW_PATH_LE) ?
				ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE : ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE;
		break;
	case ISP_PRERAW1:
		dma_id = (path == ISP_RAW_PATH_LE) ?
				ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_LE : ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_SE;
		break;
	case ISP_PRERAW2:
		dma_id = (path == ISP_RAW_PATH_LE) ?
				ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_LE : ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_SE;
		break;
	case ISP_PRERAW3:
		dma_id = (path == ISP_RAW_PATH_LE) ?
				ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_LE : ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_SE;
		break;
	case ISP_PRERAW4:
		dma_id = (path == ISP_RAW_PATH_LE) ?
				ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_LE : ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_SE;
		break;
	case ISP_PRERAW5:
		dma_id = (path == ISP_RAW_PATH_LE) ?
				ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_LE : ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_SE;
		break;
	default:
		break;
	}

	return dma_id;
}

void ispblk_blc_set_offset(struct isp_ctx *ctx, int blc_id,
				u16 roffset, u16 groffset,
				u16 gboffset, u16 boffset)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_3, blc_offset_r, roffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_3, blc_offset_gr, groffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_4, blc_offset_gb, gboffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_4, blc_offset_b, boffset);
}

void ispblk_blc_set_2ndoffset(struct isp_ctx *ctx, int blc_id,
				u16 roffset, u16 groffset,
				u16 gboffset, u16 boffset)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_9, blc_2ndoffset_r, roffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_9, blc_2ndoffset_gr, groffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_a, blc_2ndoffset_gb, gboffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_a, blc_2ndoffset_b, boffset);
}

void ispblk_blc_set_gain(struct isp_ctx *ctx, int blc_id,
				u16 rgain, u16 grgain,
				u16 gbgain, u16 bgain)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_5, blc_gain_r, rgain);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_5, blc_gain_gr, grgain);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_6, blc_gain_gb, gbgain);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_6, blc_gain_b, bgain);
}

void ispblk_blc_enable(struct isp_ctx *ctx, int blc_id, bool en, bool bypass)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_0, blc_bypass, bypass);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_2, blc_enable, en);
}

int ispblk_blc_config(struct isp_ctx *ctx, u32 blc_id, bool en, bool bypass)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return -1;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_0, blc_bypass, bypass);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_2, blc_enable, en);

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_3, blc_offset_r, 511);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_3, blc_offset_gr, 511);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_4, blc_offset_gb, 511);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_4, blc_offset_b, 511);

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_5, blc_gain_r, 0x40f);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_5, blc_gain_gr, 0x419);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_6, blc_gain_gb, 0x419);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_6, blc_gain_b, 0x405);

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_9, blc_2ndoffset_r, 0);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_9, blc_2ndoffset_gr, 0);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_a, blc_2ndoffset_gb, 0);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_a, blc_2ndoffset_b, 0);

	return 0;
}

int ispblk_wbg_config(struct isp_ctx *ctx, int wbg_id, u16 rgain, u16 ggain, u16 bgain)
{
	int id = wbg_find_hwid(wbg_id);
	uintptr_t wbg;

	if (id < 0)
		return -EINVAL;

	wbg = ctx->phys_regs[id];
	ISP_WR_BITS(wbg, reg_isp_wbg_t, wbg_4, wbg_rgain, rgain);
	ISP_WR_BITS(wbg, reg_isp_wbg_t, wbg_4, wbg_ggain, ggain);
	ISP_WR_BITS(wbg, reg_isp_wbg_t, wbg_5, wbg_bgain, bgain);

	return 0;
}

int ispblk_wbg_enable(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass)
{
	int id = wbg_find_hwid(wbg_id);
	uintptr_t wbg;

	if (id < 0)
		return -EINVAL;

	wbg = ctx->phys_regs[id];
	ISP_WR_BITS(wbg, reg_isp_wbg_t, wbg_0, wbg_bypass, bypass);
	ISP_WR_BITS(wbg, reg_isp_wbg_t, wbg_2, wbg_enable, enable);

	return 0;
}

/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/
void isp_first_frm_reset(struct isp_ctx *ctx, u8 reset)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	//reg_first_frame_sw
	//[0]: LTM
	//[1]: LDCI
	//[2]: TNR
	//[3]: MMAP
	ISP_WR_BITS(isptopb, reg_isp_top_t, first_frame, first_frame_sw,
				reset ? 0xF : (ctx->is_3dnr_old2new ? 0x4 : 0x0));

	//0: reg_first_frame_reset(in IP)
	//1: reg_first_frame_sw
	ISP_WR_BITS(isptopb, reg_isp_top_t, first_frame, first_frame_top, reset);

	vi_pr(VI_DBG, "is_3dnr_old2_new_%d reset_%d\n", ctx->is_3dnr_old2new, reset);
}


static void _ispblk_isptop_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_scenarios_ctrl scene_ctrl;

	scene_ctrl.raw = ISP_RD_REG(isptopb, reg_isp_top_t, scenarios_ctrl);

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) {
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
			scene_ctrl.bits.raw2yuv_422_enable = 1;
			scene_ctrl.bits.dci_rgb0yuv1 = 1;
			scene_ctrl.bits.hdr_enable = 0;
		} else { //rgb sensor
			scene_ctrl.bits.raw2yuv_422_enable = 0;
			scene_ctrl.bits.dci_rgb0yuv1 = 0;
			scene_ctrl.bits.hdr_enable = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		}
	} else if (_is_be_post_online(ctx)) {
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //yuv sensor
			scene_ctrl.bits.raw2yuv_422_enable = 1;
			scene_ctrl.bits.pre2yuv_422_enable = 0;
			scene_ctrl.bits.dci_rgb0yuv1 = 1;
			scene_ctrl.bits.hdr_enable = 0;

			scene_ctrl.bits.be2raw_l_enable = 0;
			scene_ctrl.bits.be2raw_s_enable = 0;
			scene_ctrl.bits.be_rdma_l_enable = 0;
			scene_ctrl.bits.be_rdma_s_enable = 0;
		} else { //rgb sensor
			scene_ctrl.bits.raw2yuv_422_enable = 0;
			scene_ctrl.bits.dci_rgb0yuv1 = 0;
			scene_ctrl.bits.hdr_enable = ctx->isp_pipe_cfg[raw_num].is_hdr_on;

			scene_ctrl.bits.be2raw_l_enable = 1;
			scene_ctrl.bits.be2raw_s_enable = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
			scene_ctrl.bits.be_rdma_l_enable = 1;
			scene_ctrl.bits.be_rdma_s_enable = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		}
	}

	ISP_WR_REG(isptopb, reg_isp_top_t, scenarios_ctrl, scene_ctrl.raw);
}

void _ispblk_be_yuv_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	uintptr_t af = ctx->phys_regs[ISP_BLK_ID_AF];

	if (!_is_be_post_online(ctx))
		return;

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				return;
			}
		//Disable af dma
		ISP_WR_BITS(af, reg_isp_af_t, kickoff, af_enable, 0);
		ISP_WR_BITS(af, reg_isp_af_t, dmi_enable, dmi_enable, 0);
		//dpcm off
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 0);
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 0);
	} else { //RGB sensor
		if (ctx->is_dpcm_on) {
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 7);
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 8191);
		}
	}

	ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_1, frame_widthm1, ctx->img_width - 1);
	ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_1, frame_heightm1, ctx->img_height - 1);

	ISP_WR_BITS(preraw_be, reg_pre_raw_be_t, img_size_le, frame_widthm1, ctx->img_width - 1);
	ISP_WR_BITS(preraw_be, reg_pre_raw_be_t, img_size_le, frame_heightm1, ctx->img_height - 1);
}

void _ispblk_rawtop_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t raw_rdma = ctx->phys_regs[ISP_BLK_ID_RAW_RDMA0];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
		ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, ch_num, 0);
		ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, rdmi_en, 1);
		ISP_WO_BITS(rawtop, reg_raw_top_t, ctrl, ls_crop_dst_sel, 1);
		ISP_WO_BITS(rawtop, reg_raw_top_t, raw_4, yuv_in_mode, 1);

		ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_mode, 0);
		ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_xstr, 0);

		if (_is_be_post_online(ctx)) {
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, config, le_rdma_en, 1);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, config, se_rdma_en, 0);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, rdma_size, rdmi_widthm1, ctx->img_width - 1);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, rdma_size, rdmi_heightm1, ctx->img_height - 1);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_mode, 0);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_xstr, 0);
		}

	} else { //RGB sensor
		if (_is_be_post_online(ctx)) //fe->dram->be->post
			ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, rdmi_en, 0);
		else if (_is_fe_be_online(ctx)) {//fe->be->dram->post
			ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, ch_num, ctx->isp_pipe_cfg[raw_num].is_hdr_on);
			ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, rdmi_en, 1);

			if (ctx->is_dpcm_on) {
				ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_mode, 7);
				ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_xstr, 8191);
			} else {
				ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_mode, 0);
				ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_xstr, 0);
			}
		}

		ISP_WO_BITS(rawtop, reg_raw_top_t, ctrl, ls_crop_dst_sel, 0);
		ISP_WO_BITS(rawtop, reg_raw_top_t, raw_4, yuv_in_mode, 0);
	}

	ISP_WR_BITS(rawtop, reg_raw_top_t, rdma_size, rdmi_widthm1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, reg_raw_top_t, rdma_size, rdmi_heightm1, ctx->img_height - 1);
	ISP_WR_BITS(rawtop, reg_raw_top_t, raw_2, img_widthm1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, reg_raw_top_t, raw_2, img_heightm1, ctx->img_height - 1);
}

void _ispblk_rgbtop_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];

	u32 dma_enable;

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) { // hdr mode
			if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp)
				dma_enable = 0x80;
			else
				dma_enable = 0xa0;
		} else { // linear mode
			if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp)
				dma_enable = 0x8a;
			else
				dma_enable = 0xaa;
		}

		if (ctx->is_3dnr_on) {
			//Enable manr dma
			ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, force_dma_disable, dma_enable);
			//Manr bypass
			ISP_WR_BITS(manr, reg_isp_mmap_t, reg_00, bypass, 0);
			ISP_WR_BITS(manr, reg_isp_mmap_t, reg_d0, crop_enable_scalar, 1);
		}
	}

	ISP_WR_BITS(rgbtop, reg_isp_rgb_top_t, reg_9, rgbtop_imgw_m1, ctx->img_width - 1);
	ISP_WR_BITS(rgbtop, reg_isp_rgb_top_t, reg_9, rgbtop_imgh_m1, ctx->img_height - 1);
}

void _ispblk_yuvtop_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUV_DITHER];
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];
	uintptr_t pre_ee = ctx->phys_regs[ISP_BLK_ID_EE_PRE];
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE_POST];
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP) {
			//Disable 3DNR dma
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_8, force_dma_disable, 0x3f);
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_4, reg_422_444, 1);
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, tdnr_enable, 0);
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, force_mono_enable, 0);
			//Because the yuv sensor don't pass the 444->422 module
			//Therefore the format output from 3dnr is yvu, need to swap to yuv
			//workaround, hw yuvtop's bug, only support yuyv and yvyu
			//for uyvy and vyuy, should set csi_ctrl_top's csi_format_frc[1] and csi_format_set[raw_16]
			if (ctx->isp_pipe_cfg[raw_num].data_seq == VI_DATA_SEQ_UYVY ||
			    ctx->isp_pipe_cfg[raw_num].data_seq == VI_DATA_SEQ_YUYV)
				ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_4, swap, 3);
			else
				ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_4, swap, 0);

			//Disable yuv dither
			ISP_WR_BITS(dither, reg_isp_yuv_dither_t, y_dither, y_dither_enable, 0);
			ISP_WR_BITS(dither, reg_isp_yuv_dither_t, uv_dither, uv_dither_enable, 0);
			//Disable cnr
			ISP_WR_BITS(cnr, reg_isp_cnr_t, cnr_enable, cnr_enable, 0);
			ISP_WR_BITS(cnr, reg_isp_cnr_t, cnr_enable, pfc_enable, 0);
			//Disable ynr
			ISP_WR_REG(ynr, reg_isp_ynr_t, out_sel, ISP_YNR_OUT_Y_DELAY);
			//Disable pre_ee
			ISP_WR_BITS(pre_ee, reg_isp_preyee_t, reg_00, ee_enable, 0);
			//Disable ee
			ISP_WR_BITS(ee, reg_isp_ee_t, reg_00, ee_enable, 0);
			//Disable ycurv
			ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_ctrl, ycur_enable, 0);
			//Disable dci dma
			ISP_WR_BITS(dci, reg_isp_dci_t, dmi_enable, dmi_enable, 0);
			//Disable ldci
			ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_enable, ldci_enable, 0);
			ISP_WR_BITS(ldci, reg_isp_ldci_t, dmi_enable, dmi_enable, 0);
			//Disable cacp
			ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_enable, 0);
			//Disable ca_lite
			ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_00, ca_lite_enable, 0);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, true, false);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, true, false);
		} else {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, false, false);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, false, false);
		}
	} else { //RGB sensor
		//Enable 3DNR dma
		if (ctx->is_3dnr_on) {
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_8, force_dma_disable,
				(ctx->isp_pipe_cfg[raw_num].tnr_mode == ISP_TNR_TYPE_NEW_MODE) ? 0x0 :
				((ctx->isp_pipe_cfg[raw_num].tnr_mode == ISP_TNR_TYPE_OLD_MODE) ? 0x24 : 0x3f));
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_4, reg_422_444, 0);
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, tdnr_enable, ctx->isp_pipe_cfg[raw_num].tnr_mode);
			ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_4, swap, 0);
		}
	}

	ISP_WR_BITS(yuvtop, reg_yuv_top_t, imgw_m1, yuv_top_imgw_m1, ctx->img_width - 1);
	ISP_WR_BITS(yuvtop, reg_yuv_top_t, imgw_m1, yuv_top_imgh_m1, ctx->img_height - 1);

	//bypass_v = 1 -> 422P online to scaler
	ISP_WR_BITS(yuvtop, reg_yuv_top_t, yuv_ctrl, bypass_v, !ctx->isp_pipe_cfg[raw_num].is_offline_scaler);

}

void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	_ispblk_isptop_cfg_update(ctx, raw_num);
	//_ispblk_be_yuv_cfg_update(ctx, raw_num);
	_ispblk_rawtop_cfg_update(ctx, raw_num);
	_ispblk_rgbtop_cfg_update(ctx, raw_num);
	_ispblk_yuvtop_cfg_update(ctx, raw_num);
}

void _ispblk_lsc_cfg_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSC0];
	int width = (ctx->isp_pipe_cfg[raw_num].is_tile)
			? (ctx->tile_cfg.r_out.end + 1)
			: (ctx->img_width);
	int height = ctx->img_height;
	int mesh_num = 37;
	int InnerBlkX = mesh_num - 1 - 2;
	int InnerBlkY = mesh_num - 1 - 2;
	int mesh_x_coord_unit = (InnerBlkX * (1 << 15)) / width;
	int mesh_y_coord_unit = (InnerBlkY * (1 << 15)) / height;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;
	u32 reg_lsc_ystep = mesh_y_coord_unit + 1;

	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int image_h_in_mesh_unit = height * reg_lsc_ystep;
	int OuterBlkX = InnerBlkX + 2;
	int OuterBlkY = InnerBlkY + 2;
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << 15) - image_w_in_mesh_unit) / 2;
	u32 reg_lsc_imgy0 = (OuterBlkY * (1 << 15) - image_h_in_mesh_unit) / 2;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile)
		reg_lsc_imgx0 += reg_lsc_xstep * (ctx->tile_cfg.r_in.start - ctx->isp_pipe_cfg[raw_num].crop.x);

	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_xstep, reg_lsc_xstep);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_ystep, reg_lsc_ystep);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_imgx0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_imgy0, reg_lsc_imgy0);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_initx0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_inity0, reg_lsc_imgy0);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		union reg_isp_lsc_enable lsc0_enable;
		union reg_isp_lsc_enable lsc1_enable;

		lsc = ctx->phys_regs[ISP_BLK_ID_LSC0];
		lsc0_enable.raw = ISP_RD_REG(lsc, reg_isp_lsc_t, lsc_enable);
		lsc = ctx->phys_regs[ISP_BLK_ID_LSC1];
		lsc1_enable.raw = ISP_RD_REG(lsc, reg_isp_lsc_t, lsc_enable);

		if (lsc0_enable.bits.lsc_enable != lsc1_enable.bits.lsc_enable) {
			lsc = ctx->phys_regs[ISP_BLK_ID_LSC0];
			ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_enable, 0);
			lsc = ctx->phys_regs[ISP_BLK_ID_LSC1];
			ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_enable, 0);
			return;
		}

		ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_xstep, reg_lsc_xstep);
		ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_ystep, reg_lsc_ystep);
		ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_imgx0, reg_lsc_imgx0);
		ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_imgy0, reg_lsc_imgy0);
		ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_initx0, reg_lsc_imgx0);
		ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_inity0, reg_lsc_imgy0);
	}
}

void _ispblk_ltm_cfg_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_LTM];
	union reg_ltm_h8c reg_8c;

	reg_8c.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h8c);
	reg_8c.bits.lmap_w_bit = g_lmp_cfg[raw_num].post_w_bit;
	reg_8c.bits.lmap_h_bit = g_lmp_cfg[raw_num].post_h_bit;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h8c, reg_8c.raw);
}

void _ispblk_manr_cfg_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	union reg_isp_mmap_6c reg_6c;

	if (ctx->is_3dnr_on && !ctx->is_fbc_on) {
		reg_6c.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_6c);

		if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp &&
		    ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp_rdy) {
			reg_6c.bits.manr_debug |= (1 << 6);
			reg_6c.bits.force_dma_disable &= ~(1 << 5);
		} else {
			reg_6c.bits.manr_debug &= ~(1 << 6);
			reg_6c.bits.force_dma_disable |= (1 << 5);
		}

		ISP_WR_REG(manr, reg_isp_mmap_t, reg_6c, reg_6c.raw);
	}
}

void _ispblk_dci_cfg_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		if (!ctx->is_work_on_r_tile) {
			ISP_WR_BITS(dci, reg_isp_dci_t, dci_roi_start, dci_roi_start_x, 0);
			ISP_WR_BITS(dci, reg_isp_dci_t, dci_roi_geo, dci_roi_widthm1,
					ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start);
			ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_zeroing_enable, true);
		} else {
			ISP_WR_BITS(dci, reg_isp_dci_t, dci_roi_start, dci_roi_start_x,
					ctx->tile_cfg.r_out.start - ctx->tile_cfg.r_in.start);
			ISP_WR_BITS(dci, reg_isp_dci_t, dci_roi_geo, dci_roi_widthm1,
					ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start);
			ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_zeroing_enable, false);
		}

		ISP_WR_BITS(dci, reg_isp_dci_t, dci_roi_geo, dci_roi_heightm1, ctx->img_height - 1);
		ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_roi_enable, 1);
	} else {
		ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_roi_enable, 0);
	}
}

void _ispblk_ldci_cfg_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		//if (!ctx->is_work_on_r_tile) {
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_crop_size, block_crop_w_str, 0);
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_crop_size, block_crop_w_end, 7);
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_offset, block_x_offset1, 0);
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_offset, block_x_offset2, 0);
		//} else {
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_crop_size, block_crop_w_str, 8);
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_crop_size, block_crop_w_end, 15);
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_offset, block_x_offset1, 7);
		//	ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_x_offset, block_x_offset2, 7);
		//}

		//ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_y_crop_size, block_crop_h_str, 0);
		//ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_y_crop_size, block_crop_h_end, 11);
		//ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_crop_size, block_img_width_crop, 15);
		//ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_crop_size, block_img_height_crop, 11);

		//ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_crop_enable, block_crop_enable, 1);
	} else {
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

		if ((width % 16 == 0) && (height % 12 == 0)) {
			ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_enable,
				ldci_image_size_div_by_16x12, 1);
		} else {
			ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_enable,
				ldci_image_size_div_by_16x12, 0);
		}

		block_size_x = (width % 16 == 0) ?
			(width / 16) : (width / 16) + 1; // Width of one block
		block_size_y = (height % 12 == 0) ?
			(height / 12) : (height / 12) + 1; // Height of one block
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
		interp_norm_lr.bits.ldci_interp_norm_lr =
			(block_size_x == 0) ? 0 : (1 << 16) / block_size_x;
		interp_norm_lr.bits.ldci_interp_norm_ud =
			(block_size_y == 0) ? 0 : (1 << 16) / block_size_y;
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_interp_norm_lr, interp_norm_lr.raw);

		interp_norm_lr1.raw = 0;
		interp_norm_lr1.bits.ldci_interp_norm_lr1 =
			(block_size_x1 == 0) ? 0 : (1 << 16) / block_size_x1;
		interp_norm_lr1.bits.ldci_interp_norm_ud1 =
			(block_size_y1 == 0) ? 0 : (1 << 16) / block_size_y1;
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_interp_norm_lr1, interp_norm_lr1.raw);

		sub_interp_norm_lr.raw = 0;
		sub_interp_norm_lr.bits.ldci_sub_interp_norm_lr =
			(sub_block_size_x == 0) ? 0 : (1 << 16) / sub_block_size_x;
		sub_interp_norm_lr.bits.ldci_sub_interp_norm_ud =
			(sub_block_size_y == 0) ? 0 : (1 << 16) / sub_block_size_y;
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_sub_interp_norm_lr, sub_interp_norm_lr.raw);

		sub_interp_norm_lr1.raw = 0;
		sub_interp_norm_lr1.bits.ldci_sub_interp_norm_lr1 =
			(sub_block_size_x1 == 0) ? 0 : (1 << 16) / sub_block_size_x1;
		sub_interp_norm_lr1.bits.ldci_sub_interp_norm_ud1 =
			(sub_block_size_y1 == 0) ? 0 : (1 << 16) / sub_block_size_y1;
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_sub_interp_norm_lr1, sub_interp_norm_lr1.raw);

		mean_norm_x.raw = 0;
		mean_norm_x.bits.ldci_mean_norm_x = (1 << 14) / MAX(block_size_x, 1);
		mean_norm_x.bits.ldci_mean_norm_y = (1 << 13) / MAX(line_mean_num, 1);
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_mean_norm_x, mean_norm_x.raw);

		var_norm_y.raw = 0;
		var_norm_y.bits.ldci_var_norm_y = (1 << 13) / MAX(line_var_num, 1);
		ISP_WR_REG(ldci, reg_isp_ldci_t, ldci_var_norm_y, var_norm_y.raw);

		ISP_WR_BITS(ldci, reg_isp_ldci_t, ldci_block_crop_enable, block_crop_enable, 0);
	}
}

void ispblk_post_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	ispblk_raw_rdma_ctrl_config(ctx, raw_num, ISP_BLK_ID_RAW_RDMA0,
					ctx->is_offline_postraw);
	ispblk_raw_rdma_ctrl_config(ctx, raw_num, ISP_BLK_ID_RAW_RDMA1,
					ctx->is_offline_postraw &&
					ctx->isp_pipe_cfg[raw_num].is_hdr_on);

	ispblk_rawtop_config(ctx, raw_num);
	ispblk_rgbtop_config(ctx, raw_num);
	ispblk_yuvtop_config(ctx, raw_num);

	_ispblk_lsc_cfg_update(ctx, raw_num);
	_ispblk_ltm_cfg_update(ctx, raw_num);
	_ispblk_manr_cfg_update(ctx, raw_num);
	_ispblk_dci_cfg_update(ctx, raw_num);
	_ispblk_ldci_cfg_update(ctx, raw_num);
}

int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, u32 _w, u32 _h)
{
	u32 len = 0, num = 0, w;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG1:
	{
		w = _w;
		num = _h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	vi_pr(VI_INFO, "dmaid=%d, size=%d\n", dmaid, len * num);

	return len * num;
}

void ispblk_pre_be_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
	union reg_pre_raw_vi_sel_1 vi_sel_1;
	union reg_pre_raw_be_top_ctrl top_ctrl;
	union reg_pre_raw_be_img_size_le img_size;
	int numx = 17, numy = 15;

	vi_sel_1.raw = 0;
	vi_sel_1.bits.frame_widthm1 = ctx->img_width - 1;
	vi_sel_1.bits.frame_heightm1 = ctx->img_height - 1;
	ISP_WR_REG(vi_sel, reg_pre_raw_vi_sel_t, reg_1, vi_sel_1.raw);

	top_ctrl.raw = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, top_ctrl);
	top_ctrl.bits.bayer_type_le	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.bayer_type_se	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.rgbir_en		= ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	top_ctrl.bits.ch_num		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	top_ctrl.bits.post_rgbir_bayer_type_le	= bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	top_ctrl.bits.post_rgbir_bayer_type_se	= bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	ISP_WR_REG(preraw_be, reg_pre_raw_be_t, top_ctrl, top_ctrl.raw);

	img_size.raw = 0;
	img_size.bits.frame_widthm1 = ctx->img_width - 1;
	img_size.bits.frame_heightm1 = ctx->img_height - 1;
	ISP_WR_REG(preraw_be, reg_pre_raw_be_t, img_size_le, img_size.raw);

	// block_width >= 15
	ISP_WR_REG(sts, reg_isp_af_t, block_width, (ctx->img_width - 16) / numx);
	// block_height >= 15
	ISP_WR_REG(sts, reg_isp_af_t, block_height, (ctx->img_height - 4) / numy);

	ISP_WR_REG(sts, reg_isp_af_t, image_width, ctx->img_width - 1);
	ISP_WR_BITS(sts, reg_isp_af_t, mxn_image_width_m1, af_mxn_image_width, ctx->img_width - 1);
	ISP_WR_BITS(sts, reg_isp_af_t, mxn_image_width_m1, af_mxn_image_height, ctx->img_height - 1);
}

int isp_frm_err_handler(struct isp_ctx *ctx, const enum sop_isp_raw err_raw_num, const u8 step)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_sw_rst sw_rst;
	union reg_isp_top_sw_rst_fe345 sw_rst_fe345;
	union vi_sys_reset vi_rst;

	if (step == 1) {
		int id = -1;
		uintptr_t ba = 0;

		if (ctx->isp_pipe_cfg[err_raw_num].is_bt_demux) {
			id = csibdg_find_hwid(err_raw_num);
			ba = ctx->phys_regs[id];

			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch0, 0);
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch1, 0);
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch2, 0);
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch3, 0);
		} else {
			id = fe_find_hwid(err_raw_num);
			ba = ctx->phys_regs[id];

			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch0, 0);
			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch1, 0);
			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch2, 0);
			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch3, 0);
		}
	} else if (step == 3) {
		int id = csibdg_find_hwid(err_raw_num);
		uintptr_t ba = ctx->phys_regs[id];
		uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
		uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
		uintptr_t wdma_com_2 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE2];
		uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
		uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];
		uintptr_t rdma_com_1 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE1];
		u8 count = 10;

		if (ctx->isp_pipe_cfg[err_raw_num].is_bt_demux)
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl, abort, 1);
		else
			ISP_WR_BITS(ba, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, abort, 1);

		while (--count > 0) {
			if (ISP_RD_BITS(wdma_com_0, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(wdma_com_1, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(wdma_com_2, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(wdma_com_3, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(rdma_com_0, reg_rdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(rdma_com_1, reg_rdma_core_t, norm_status0, abort_done)) {
				vi_pr(VI_INFO, "W/RDMA_CORE abort done, count(%d)\n", count);
				break;
			}
			usleep_range(1, 5);
		}

		if (count == 0) {
			vi_pr(VI_ERR, "WDMA/RDMA_CORE abort fail\n");
			return -1;
		}
	} else if (step == 4) {
		sw_rst.raw = 0;
		sw_rst.bits.axi_rst = 1;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, sw_rst.raw);
		sw_rst.raw = 0x6FF;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, sw_rst.raw);
		sw_rst_fe345.raw = 0x7;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst_fe345, sw_rst_fe345.raw);

		vi_rst.raw = 0;
		vi_rst.b.isp_top = 1;
		sys_set_reset_vi(vi_rst);

		vi_pr(VI_INFO, "ISP and vip_sys isp rst pull up\n");
	} else if (step == 5) {
		vi_rst = sys_get_reset_vi();
		vi_rst.b.isp_top = 0;
		sys_set_reset_vi(vi_rst);

		sw_rst.raw = ISP_RD_REG(isptopb, reg_isp_top_t, sw_rst);
		sw_rst.bits.axi_rst = 0;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, sw_rst.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst_fe345, 0);

		vi_pr(VI_INFO, "ISP and vip_sys isp rst pull down\n");
	} else if (step == 6) {
		uintptr_t fe0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		uintptr_t fe1 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
		uintptr_t fe2 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE2];
		uintptr_t fe3 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE3];
		uintptr_t fe4 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE4];
		uintptr_t fe5 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE5];
		uintptr_t be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		u8 cnt = 10;

		vi_pr(VI_INFO, "Wait ISP idle\n");

		while (--cnt > 0) {
			if ((ISP_RD_REG(fe0, reg_pre_raw_fe_t, fe_idle_info) == 0x3F) &&
			    (ISP_RD_REG(fe1, reg_pre_raw_fe_t, fe_idle_info) == 0x3F) &&
			    (ISP_RD_REG(fe2, reg_pre_raw_fe_t, fe_idle_info) == 0x3F) &&
			    (ISP_RD_REG(fe3, reg_pre_raw_fe_t, fe_idle_info) == 0x3F) &&
			    (ISP_RD_REG(fe4, reg_pre_raw_fe_t, fe_idle_info) == 0x3F) &&
			    (ISP_RD_REG(fe5, reg_pre_raw_fe_t, fe_idle_info) == 0x3F) &&
			    ((ISP_RD_REG(be, reg_pre_raw_be_t, be_ip_idle_info) & 0x3F007F) == 0x3F007F) &&
			    (ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle)) == 0x3FF) {
				vi_pr(VI_INFO, "FE/BE/ISP idle done, count(%d)\n", cnt);
				break;
			}
			usleep_range(1, 5);
		}

		if (cnt == 0) {
			vi_pr(VI_ERR, "FE(0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x)/BE(0x%x)/ISP(0x%x) not idle.",
				ISP_RD_REG(fe0, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe1, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe2, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe3, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe4, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe5, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(be, reg_pre_raw_be_t, be_ip_idle_info),
				ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle));
			return -1;
		}
	}

	return 0;
}
/****************************************************************************
 *	YUV Bypass Control Flow Config
 ****************************************************************************/
//TODO maybe use sw control
u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr,
				 const enum sop_isp_raw raw_num)
{
	// uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 len = 0, /* stride = 0, */ num = 0;

	switch (dmaid) {
	/* csibdg */
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI3_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI4_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI5_BDG1:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE0:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE1:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE2:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE3:
	case ISP_BLK_ID_DMA_CTL_BT1_LITE0:
	case ISP_BLK_ID_DMA_CTL_BT1_LITE1:
	case ISP_BLK_ID_DMA_CTL_BT1_LITE2:
	case ISP_BLK_ID_DMA_CTL_BT1_LITE3:
	/* preraw_be */
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE:
	/* raw_top */
	case ISP_BLK_ID_DMA_CTL_RAW_RDMA0:
	case ISP_BLK_ID_DMA_CTL_RAW_RDMA1:
	{
		/* csibdg */
		len = ctx->isp_pipe_cfg[raw_num].csibdg_width * 2;
		num = ctx->isp_pipe_cfg[raw_num].csibdg_height;

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	if (buf_addr)
		ispblk_dma_setaddr(ctx, dmaid, buf_addr);

	// maybe will use sw control
	// ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_seglen, len);
	// ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, stride);
	// ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_segnum, num);

	return len * num;
}

/****************************************************************************
 *	Slice buffer Control
 ****************************************************************************/
void vi_calculate_slice_buf_setting(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	u32 main_le_num = 0, main_se_num = 0;
	u32 main_le_size = 0, main_se_size = 0;
	u32 sub_le_num = 0, sub_se_num = 0;
	u32 sub_le_size = 0, sub_se_size = 0;
	u16 main_le_w_th = 0, main_le_r_th = 0;
	u16 main_se_w_th = 0, main_se_r_th = 0;
	u16 sub_le_w_th = 0, sub_le_r_th = 0;
	u16 sub_se_w_th = 0, sub_se_r_th = 0;

	u32 line_delay = slc_b_cfg.line_delay, buffer = slc_b_cfg.buffer;
	u32 main_max_grid_size = slc_b_cfg.main_max_grid_size;
	u32 sub_max_grid_size = slc_b_cfg.sub_max_grid_size;
	u32 min_r_th = slc_b_cfg.min_r_thshd;

	u32 w = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 h = ctx->isp_pipe_cfg[raw_num].crop.h;

	// Calculate the ring buffer line number
	main_le_num = line_delay + 2 * main_max_grid_size + buffer;
	main_le_size = VI_256_ALIGN(main_le_num * ((w * 3) / 2));
	sub_le_num  = (h + sub_max_grid_size - 1) / sub_max_grid_size + line_delay / sub_max_grid_size + buffer;
	sub_le_size = VI_256_ALIGN(sub_le_num *
			(((((w + sub_max_grid_size - 1) / sub_max_grid_size) * 48 + 127) >> 7) << 4));
	// Calculate the r/w threshold
	main_le_r_th = 2 * main_max_grid_size;
	main_le_w_th = main_le_num - 1;

	sub_le_r_th = min_r_th;
	sub_le_w_th = (line_delay / sub_max_grid_size) + buffer - 1;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		main_se_num = 2 * main_max_grid_size + buffer;
		main_se_size = VI_256_ALIGN(main_se_num * ((w * 3) / 2));
		sub_se_num  = (h + sub_max_grid_size - 1) / sub_max_grid_size + buffer;
		sub_se_size = VI_256_ALIGN(sub_se_num *
				(((((w + sub_max_grid_size - 1) / sub_max_grid_size) * 48 + 127) >> 7) << 4));
		main_se_r_th = 2 * main_max_grid_size;
		main_se_w_th = main_se_num - 1;

		sub_se_r_th = min_r_th;
		sub_se_w_th = buffer - 1;
	}

	slc_b_cfg.main_path.le_buf_size = main_le_size;
	slc_b_cfg.main_path.le_w_thshd  = main_le_w_th;
	slc_b_cfg.main_path.le_r_thshd  = main_le_r_th;

	slc_b_cfg.sub_path.le_buf_size  = sub_le_size;
	slc_b_cfg.sub_path.le_w_thshd   = sub_le_w_th;
	slc_b_cfg.sub_path.le_r_thshd   = sub_le_r_th;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		slc_b_cfg.main_path.se_buf_size = main_se_size;
		slc_b_cfg.main_path.se_w_thshd  = main_se_w_th;
		slc_b_cfg.main_path.se_r_thshd  = main_se_r_th;

		slc_b_cfg.sub_path.se_buf_size  = sub_se_size;
		slc_b_cfg.sub_path.se_w_thshd   = sub_se_w_th;
		slc_b_cfg.sub_path.se_r_thshd   = sub_se_r_th;
	}
}

void manr_clear_prv_ring_base(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];

	ISP_WR_REG(rdma_com_0, reg_rdma_core_t, up_ring_base, (1 << ISP_DMA_ID_MMAP_PRE_LE_R));
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, up_ring_base, (1 << ISP_DMA_ID_MMAP_PRE_SE_R));
}

void isp_slice_buf_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)
		ISP_WR_REG(isptopb, reg_isp_top_t, raw_frame_valid, 0x3);
}

void _ispblk_dma_slice_config(struct isp_ctx *ctx, int dmaid, int en)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, dma_slicesize, slice_size, 1);
	ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, sys_control, slice_enable, en);

	// rgbmap dma
	if (dmaid == ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE || dmaid == ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE) {
		if (en)
			ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, dma_dummy, perf_patch_enable, 0);
		else
			ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, dma_dummy, perf_patch_enable, 1);
	}
}

void ispblk_slice_buf_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, u8 en)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t wdma_com_2 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE2];
	uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];
	union reg_wdma_core_ring_buffer_en w_ring_buf_en_1;
	union reg_wdma_core_ring_buffer_en w_ring_buf_en_2;
	union reg_rdma_core_ring_buffer_en r_ring_buf_en_0;

	// Multi-sensor don't support slice buffer mode.
	if (en && _is_fe_be_online(ctx)) {
		union reg_isp_top_sclie_enable			slice_en;
		union reg_isp_top_w_slice_thresh_main		w_th_main;
		union reg_isp_top_w_slice_thresh_sub_curr	w_th_sub_cur;
		union reg_isp_top_w_slice_thresh_sub_prv	w_th_sub_prev;
		union reg_isp_top_r_slice_thresh_main		r_th_main;
		union reg_isp_top_r_slice_thresh_sub_curr	r_th_sub_cur;
		union reg_isp_top_r_slice_thresh_sub_prv	r_th_sub_prev;
		bool is_sub_slice_en = ctx->is_3dnr_on && ctx->is_rgbmap_sbm_on;

		slice_en.raw = 0;
		slice_en.bits.slice_enable_main_lexp = en;
		slice_en.bits.slice_enable_sub_lexp = en && is_sub_slice_en;

		w_th_main.raw = 0;
		w_th_main.bits.w_slice_thr_main_lexp = slc_b_cfg.main_path.le_w_thshd;

		w_th_sub_cur.raw = 0;
		w_th_sub_cur.bits.w_slice_thr_sub_cur_lexp = slc_b_cfg.sub_path.le_w_thshd;

		w_th_sub_prev.raw = 0;
		w_th_sub_prev.bits.w_slice_thr_sub_prv_lexp = slc_b_cfg.sub_path.le_w_thshd;

		r_th_main.raw = 0;
		r_th_main.bits.r_slice_thr_main_lexp = slc_b_cfg.main_path.le_r_thshd;

		r_th_sub_cur.raw = 0;
		r_th_sub_cur.bits.r_slice_thr_sub_cur_lexp = slc_b_cfg.sub_path.le_r_thshd;

		r_th_sub_prev.raw = 0;
		r_th_sub_prev.bits.r_slice_thr_sub_prv_lexp = slc_b_cfg.sub_path.le_r_thshd;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			slice_en.bits.slice_enable_main_sexp = en;
			slice_en.bits.slice_enable_sub_sexp = en && is_sub_slice_en;

			w_th_main.bits.w_slice_thr_main_sexp		= slc_b_cfg.main_path.se_w_thshd;
			w_th_sub_cur.bits.w_slice_thr_sub_cur_sexp	= slc_b_cfg.sub_path.se_w_thshd;
			w_th_sub_prev.bits.w_slice_thr_sub_prv_sexp	= slc_b_cfg.sub_path.se_w_thshd;
			r_th_main.bits.r_slice_thr_main_sexp		= slc_b_cfg.main_path.se_r_thshd;
			r_th_sub_cur.bits.r_slice_thr_sub_cur_sexp	= slc_b_cfg.sub_path.se_r_thshd;
			r_th_sub_prev.bits.r_slice_thr_sub_prv_sexp	= slc_b_cfg.sub_path.se_r_thshd;
		}

		ISP_WR_REG(isptopb, reg_isp_top_t, sclie_enable, slice_en.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, w_slice_thresh_main, w_th_main.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, w_slice_thresh_sub_curr, w_th_sub_cur.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, w_slice_thresh_sub_prv, w_th_sub_prev.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, r_slice_thresh_main, r_th_main.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, r_slice_thresh_sub_curr, r_th_sub_cur.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, r_slice_thresh_sub_prv, r_th_sub_prev.raw);

		// wdma/rdma core config
		w_ring_buf_en_1.raw = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en);
		w_ring_buf_en_1.raw |= (1 << ISP_DMA_ID_PRE_RAW_BE_LE);

		w_ring_buf_en_2.raw = ISP_RD_REG(wdma_com_2, reg_wdma_core_t, ring_buffer_en);
		w_ring_buf_en_2.raw |= ((is_sub_slice_en) ? (1 << ISP_DMA_ID_FE0_RGBMAP_LE) : 0);

		r_ring_buf_en_0.raw = ISP_RD_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en);
		r_ring_buf_en_0.raw |= (1 << ISP_DMA_ID_RAW_RDMA0);
		r_ring_buf_en_0.raw |= ((is_sub_slice_en)
					? ((1 << ISP_DMA_ID_MMAP_CUR_LE_R) | (1 << ISP_DMA_ID_MMAP_PRE_LE_R))
					: 0);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			w_ring_buf_en_1.raw |= (1 << ISP_DMA_ID_PRE_RAW_BE_SE);

			w_ring_buf_en_2.raw |= ((is_sub_slice_en) ? (1 << ISP_DMA_ID_FE0_RGBMAP_SE) : 0);

			r_ring_buf_en_0.raw |= (1 << ISP_DMA_ID_RAW_RDMA1);
			r_ring_buf_en_0.raw |= ((is_sub_slice_en)
						? ((1 << ISP_DMA_ID_MMAP_CUR_SE_R) | (1 << ISP_DMA_ID_MMAP_PRE_SE_R))
						: 0);
		}

		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en, w_ring_buf_en_1.raw);
		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_size10, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_size11, slc_b_cfg.main_path.se_buf_size);
		ISP_WR_REG(wdma_com_2, reg_wdma_core_t, ring_buffer_en, w_ring_buf_en_2.raw);
		ISP_WR_REG(wdma_com_2, reg_wdma_core_t, ring_buffer_size0, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(wdma_com_2, reg_wdma_core_t, ring_buffer_size1, slc_b_cfg.sub_path.se_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en, r_ring_buf_en_0.raw);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size1, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size2, slc_b_cfg.main_path.se_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size5, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size6, slc_b_cfg.sub_path.se_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size7, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_size8, slc_b_cfg.sub_path.se_buf_size);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA1, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R, false);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA0, true);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE, true);
		if (is_sub_slice_en) {
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R, true);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA1, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE, true);
			if (is_sub_slice_en) {
				_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE, true);
				_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R, true);
				_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R, true);
			}
		}
	} else if (!en || _is_be_post_online(ctx)) {
		w_ring_buf_en_1.raw = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en);
		w_ring_buf_en_1.raw &= ~(1 << ISP_DMA_ID_PRE_RAW_BE_LE);

		w_ring_buf_en_2.raw = ISP_RD_REG(wdma_com_2, reg_wdma_core_t, ring_buffer_en);
		w_ring_buf_en_2.raw &= ~((ctx->is_3dnr_on) ? (1 << ISP_DMA_ID_FE0_RGBMAP_LE) : 0);

		r_ring_buf_en_0.raw = ISP_RD_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en);
		r_ring_buf_en_0.raw &= ~(1 << ISP_DMA_ID_RAW_RDMA0);
		r_ring_buf_en_0.raw &= ~((ctx->is_3dnr_on)
					? ((1 << ISP_DMA_ID_MMAP_CUR_LE_R) | (1 << ISP_DMA_ID_MMAP_PRE_LE_R))
					: 0);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			w_ring_buf_en_1.raw &= ~(1 << ISP_DMA_ID_PRE_RAW_BE_SE);

			w_ring_buf_en_2.raw &= ~((ctx->is_3dnr_on) ? (1 << ISP_DMA_ID_FE0_RGBMAP_SE) : 0);

			r_ring_buf_en_0.raw &= ~(1 << ISP_DMA_ID_RAW_RDMA1);
			r_ring_buf_en_0.raw &= ~((ctx->is_3dnr_on)
						? ((1 << ISP_DMA_ID_MMAP_CUR_SE_R) | (1 << ISP_DMA_ID_MMAP_PRE_SE_R))
						: 0);
		}

		ISP_WR_REG(isptopb, reg_isp_top_t, sclie_enable, 0);
		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en, w_ring_buf_en_1.raw);
		ISP_WR_REG(wdma_com_2, reg_wdma_core_t, ring_buffer_en, w_ring_buf_en_2.raw);
		ISP_WR_REG(rdma_com_0, reg_rdma_core_t, ring_buffer_en, r_ring_buf_en_0.raw);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA0, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA1, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R, false);
	}
}

