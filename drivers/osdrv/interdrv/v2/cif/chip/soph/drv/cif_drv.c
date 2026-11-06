#include <linux/types.h>
#include <linux/delay.h>
#include "reg.h"
#include "inc/cif_reg.h"
#include "cif_drv.h"

/* SubLVDS Normal Sync code */
#define SLVDS_SYNC_CODE_1ST		0xFFFu
#define SLVDS_SYNC_CODE_2ND		0u
#define SLVDS_SYNC_CODE_3RD		0u
#define SLVDS_SYNC_CODE_NORM_BK_SAV	0xAB0u
#define SLVDS_SYNC_CODE_NORM_BK_EAV	0xB60u
#define SLVDS_SYNC_CODE_NORM_SAV	0x800u
#define SLVDS_SYNC_CODE_NORM_EAV	0x9D0u
#define SLVDS_SYNC_CODE_N0_BK_SAV	0x2B0u
#define SLVDS_SYNC_CODE_N0_BK_EAV	0x360u
#define SLVDS_SYNC_CODE_N1_BK_SAV	0x6B0u
#define SLVDS_SYNC_CODE_N1_BK_EAV	0x760u

/* 10 bit SubLVDS HDR-realted Sync code */
#define SLVDS_SYNC_CODE_N0_LEF_SAV	0x004u
#define SLVDS_SYNC_CODE_N0_LEF_EAV	0x1D4u
#define SLVDS_SYNC_CODE_N0_SEF_SAV	0x008u
#define SLVDS_SYNC_CODE_N0_SEF_EAV	0x1D8u
#define SLVDS_SYNC_CODE_N1_LEF_SAV	0x404u
#define SLVDS_SYNC_CODE_N1_LEF_EAV	0x5D4u
#define SLVDS_SYNC_CODE_N1_SEF_SAV	0x408u
#define SLVDS_SYNC_CODE_N1_SEF_EAV	0x5D8u
#define SLVDS_SYNC_CODE_N0_LSEF_SAV	0x00Cu
#define SLVDS_SYNC_CODE_N0_LSEF_EAV	0x1DCu
#define SLVDS_SYNC_CODE_N1_LSEF_SAV	0x40Cu
#define SLVDS_SYNC_CODE_N1_LSEF_EAV	0x5DCu

/* HiSPi PKT-SP HDR-realted Sync code */
#define HISPI_SYNC_CODE_T1_SOL		0x800u
#define HISPI_SYNC_CODE_T1_EOL		0xA00u
#define HISPI_SYNC_CODE_T2_SOL		0x820u
#define HISPI_SYNC_CODE_T2_EOL		0xA20u
#define HISPI_SYNC_CODE_T1_SOF		0xC00u
#define HISPI_SYNC_CODE_T1_EOF		0xE00u
#define HISPI_SYNC_CODE_T2_SOF		0xC20u
#define HISPI_SYNC_CODE_T2_EOF		0xE20u
#define HISPI_SYNC_CODE_VSYNC_GEN	0xC00u

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t m_cif_mac_phys_base_list[MAX_LINK_NUM]
	[CIF_MAC_BLK_ID_MAX];
static uintptr_t m_cif_macvi_phys_base_list[MAX_LINK_NUM]
	[CIF_MAC_VI_BLK_ID_MAX];
static uintptr_t mac_reg_base[MAX_LINK_NUM];
static uintptr_t wrap_reg_base[MAX_LINK_NUM];

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void cif_set_base_addr(uint32_t link, void *mac_base, void *wrap_base)
{
	uintptr_t *addr = cif_get_mac_phys_reg_bases(link);
	int i = 0;

	if (link < CIF_MAC_VI_0) {
		for (i = 0; i < CIF_MAC_BLK_ID_MAX; ++i) {
			addr[i] -= mac_reg_base[link];
			addr[i] += (uintptr_t)mac_base;
		}
		mac_reg_base[link] = (uintptr_t)mac_base;
	} else if (link < CIF_MAC_NUM) {
		for (i = 0; i < CIF_MAC_VI_BLK_ID_MAX; ++i) {
			addr[i] -= mac_reg_base[link];
			addr[i] += (uintptr_t)mac_base;
		}
		mac_reg_base[link] = (uintptr_t)mac_base;
	}

	addr = cif_get_wrap_phys_reg_bases(link);

	for (i = 0; i < CIF_WRAP_BLK_ID_MAX; ++i) {
		addr[i] -= wrap_reg_base[link];
		addr[i] += (uintptr_t)wrap_base;
	}
	wrap_reg_base[link] = (uintptr_t)wrap_base;
}

void cif_init(struct cif_ctx *ctx)
{
}

void cif_uninit(struct cif_ctx *ctx)
{
}

void cif_reset(struct cif_ctx *ctx)
{
}

static void _cif_sublvds_config(struct cif_ctx *ctx,
			       struct param_sublvds *param)
{
	uintptr_t wrap;
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];
	struct sync_code_s *sc = &param->sync_code;

	if (ctx->mac_num == CIF_MAC_0) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
	} else if (ctx->mac_num == CIF_MAC_1) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
	} else if (ctx->mac_num == CIF_MAC_2) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
	} else if (ctx->mac_num == CIF_MAC_3) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
	} else if (ctx->mac_num == CIF_MAC_4) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
	} else if (ctx->mac_num == CIF_MAC_5) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
	}

	/* Config the sync code */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_18, slvds_n0_lef_sav,
			       sc->slvds.n0_lef_sav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_1c, slvds_n0_lef_eav,
			       sc->slvds.n0_lef_eav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_1c, slvds_n0_sef_sav,
			       sc->slvds.n0_sef_sav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_20, slvds_n0_sef_eav,
			       sc->slvds.n0_sef_eav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_20, slvds_n1_lef_sav,
			       sc->slvds.n1_lef_sav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_24, slvds_n1_lef_eav,
			       sc->slvds.n1_lef_eav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_24, slvds_n1_sef_sav,
			       sc->slvds.n1_sef_sav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_28, slvds_n1_sef_eav,
			       sc->slvds.n1_sef_eav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_50, slvds_n0_lsef_sav,
			       sc->slvds.n0_lsef_sav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_50, slvds_n0_lsef_eav,
			       sc->slvds.n0_lsef_eav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_54, slvds_n1_lsef_sav,
			       sc->slvds.n1_lsef_sav);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_54, slvds_n1_lsef_eav,
			       sc->slvds.n1_lsef_eav);

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sensor_mac_mode,
			     2);
	/* invert the HS/VS/HDR */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_vs_inv,
			     1);
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_hs_inv,
			     1);
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_hdr_inv,
			     1);
	/* subLVDS controller enable */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_ctrl_enable,
			     1);
	/* disable HiSPi mode */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_60, hispi_mode,
			       0);
	/* Config the lane enable */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_enable,
			       (1 << param->lane_num) - 1);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_30, slvds_lane_mode,
			       param->lane_num - 1);
	/* Config the raw format. */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_bit_mode,
			       param->fmt);
	/* Config the endian. */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_data_reverse,
			       param->endian == CIF_SLVDS_ENDIAN_LSB);
	/* DPHY sensor mode select */
	CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			     reg_00, sensor_mode,
			     1);
	/* DPHY bit mode select */
	CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			     reg_20, slvds_bit_mode,
			     param->fmt);
	/* DPHY endian mode select */
	CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			     reg_20, slvds_inv_en,
			     param->wrap_endian == CIF_SLVDS_ENDIAN_MSB);
}

static void _cif_hispi_config(struct cif_ctx *ctx,
			     struct param_hispi *param)
{
	uintptr_t wrap;
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	if (ctx->mac_num == CIF_MAC_0) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
	} else if (ctx->mac_num == CIF_MAC_1) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
	} else if (ctx->mac_num == CIF_MAC_2) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
	} else if (ctx->mac_num == CIF_MAC_3) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
	} else if (ctx->mac_num == CIF_MAC_4) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
	} else if (ctx->mac_num == CIF_MAC_5) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
	}

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sensor_mac_mode,
			     2);
	/* invert the HS/VS */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_vs_inv,
			     1);
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_hs_inv,
			     1);
	/* subLVDS controller enable */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sublvds_ctrl_enable,
			     1);
	/* Config the raw format. */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_bit_mode,
			       param->fmt);
	/* Enable HiSPi mode */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_60, hispi_mode,
			       1);
	/* Config the lane enable */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_enable,
			       (1 << param->lane_num) - 1);
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_30, slvds_lane_mode,
			       param->lane_num - 1);
	/* Config the endian. */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_data_reverse,
			       param->endian);
	/* Config the HiSPi mode*/
	if (param->mode == CIF_HISPI_MODE_PKT_SP) {
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_60, hispi_use_hsize,
				       0);
	} else {
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_58, slvds_hdr_p2_hsize,
				       param->h_size/param->lane_num);
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_60, hispi_use_hsize,
				       1);
	}
	/* DPHY sensor mode select */
	CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			     reg_00, sensor_mode,
			     1);
	/* DPHY bit mode select */
	CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			     reg_20, slvds_bit_mode,
			     param->fmt);
	/* DPHY endian mode select */
	CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			     reg_20, slvds_inv_en,
			     param->wrap_endian == CIF_SLVDS_ENDIAN_MSB);
}

static void _cif_ttl_config(struct cif_ctx *ctx, struct param_ttl *param)
{
	uintptr_t mac_top = (ctx->mac_num < CIF_MAC_VI_0) ? ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP] :
												ctx->mac_phys_regs[CIF_MAC_VI_BLK_TOP];

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sensor_mac_mode,
			     3);
	/* Config TTL sensor format */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_10, ttl_sensor_bit,
			     param->sensor_fmt);
	/* Config TTL clock invert */
	if (param->vi_from == FROM_VI0) {
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, pad_vi0_clk_inv,
				     param->clk_inv);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_from,
				     0);
	// to do :in FPGA ,need VI_CLK_INV, maybe set
	} else if (param->vi_from == FROM_VI1) {
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, pad_vi1_clk_inv,
				     param->clk_inv);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_from,
				     1);
	// to do :in FPGA ,need VI_CLK_INV, maybe set
	} else {
		return;
	}

	switch (param->fmt) {
	case TTL_SYNC_PAT_17B_BT1120:
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_fmt_in,
				     TTL_SYNC_PAT_17B_BT1120);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_bt_fmt_out,
				     param->fmt_out);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_14, ttl_vs_bp,
				     param->v_bp);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_14, ttl_hs_bp,
				     param->h_bp);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_wd,
				     param->width);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_ht,
				     param->height);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_1c, ttl_sync_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_1c, ttl_sync_1,
				     0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_20, ttl_sync_2,
				     0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_24, ttl_sav_vld,
				     0x8000);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_24, ttl_sav_blk,
				     0xab00);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_28, ttl_eav_vld,
				     0x9d00);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_28, ttl_eav_blk,
				     0xb600);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_sel,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_v_sel_vs,
				     1);
		break;
	case TTL_VSDE_11B_BT601:
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
		     reg_10, ttl_fmt_in,
		     TTL_VSDE_11B_BT601);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_bt_fmt_out,
				     param->fmt_out);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_vs_inv,
				     0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_hs_inv,
				     0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_wd,
				     param->width);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_ht,
				     param->height);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_sel,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_v_sel_vs,
				     1);
		break;
	case TTL_VHS_19B_BT601:
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_fmt_in,
				     TTL_VHS_19B_BT601);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_bt_fmt_out,
				     param->fmt_out);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_vs_inv,
				     1);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_hs_inv,
				     1);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_14, ttl_vs_bp,
				     param->v_bp);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_14, ttl_hs_bp,
				     param->h_bp);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_wd,
				     param->width);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_ht,
				     param->height);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_1c, ttl_sync_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_sel,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_v_sel_vs,
				     1);
		break;
	case TTL_SYNC_PAT_9B_BT656:
		if (ctx->mac_num < CIF_MAC_VI_0) {
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_10, ttl_fmt_in,
					     TTL_SYNC_PAT_9B_BT656);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_10, ttl_bt_fmt_out,
					     param->fmt_out);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_14, ttl_vs_bp,
					     param->v_bp);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_14, ttl_hs_bp,
					     param->h_bp);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_18, ttl_img_wd,
					     param->width);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_18, ttl_img_ht,
					     param->height);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_1c, ttl_sync_0,
					     0xFFFF);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_1c, ttl_sync_1,
					     0x0);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_20, ttl_sync_2,
					     0x0);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_24, ttl_sav_vld,
					     0x8000);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_24, ttl_sav_blk,
					     0xAB00);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_28, ttl_eav_vld,
					     0x9D00);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_28, ttl_eav_blk,
					     0xB600);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_30, vi_sel,
					     param->vi_sel);
		} else if (ctx->mac_num < CIF_MAC_NUM) {
			/*to do:add 656_ddr config*/
		}
		break;
	case TTL_CUSTOM_0:
		/* Config TTL format */
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_fmt_in,
				     TTL_VHS_19B_BT601);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_bt_fmt_out,
				     param->fmt_out);
		/* Config TTL format */
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_bt_fmt_out,
				     TTL_BT_FMT_OUT_CBYCRY);
		/* Config HV inverse */
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_vs_inv,
				     1);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_hs_inv,
				     1);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_14, ttl_vs_bp,
				     param->v_bp);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_14, ttl_hs_bp,
				     param->h_bp);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_wd,
				     param->width);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_18, ttl_img_ht,
				     param->height);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_1c, ttl_sync_0,
				     0xFFFF);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_1c, ttl_sync_1,
				     0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_20, ttl_sync_2,
				     0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_24, ttl_sav_vld,
				     0x8000);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_24, ttl_sav_blk,
				     0xab00);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_28, ttl_eav_vld,
				     0x9d00);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_28, ttl_eav_blk,
				     0xb600);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_sel,
				     param->vi_sel);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_v_sel_vs,
				     1);
		break;
	default:
		/* Config TTL format */
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_10, ttl_fmt_in,
				     param->fmt);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_30, vi_sel,
				     param->vi_sel);
		break;
	}
}

static void _cif_btdemux_config(struct cif_ctx *ctx,
				struct param_btdemux *param)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_VI_BLK_TOP];

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_00, bt_demux_enable,
			     1);

	/* config demux channels */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_20, bt_demux_ch,
			     param->demux);
	/* config bt fp  */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_30, bt_vs_fp_m1,
			     param->v_fp);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_30, bt_hs_fp_m1,
			     param->h_fp);
	/* config bt bp  */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_2c, bt_vs_bp_m1,
			     param->v_bp);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_2c, bt_hs_bp_m1,
			     param->h_bp);
	/* config bt sync code  */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_34, bt_sync_0,
			     param->sync_code_part_a[0]);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_34, bt_sync_1,
			     param->sync_code_part_a[1]);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_34, bt_sync_2,
			     param->sync_code_part_a[2]);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_38, bt_sav_vld_0,
			     param->sync_code_part_b[0].sav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_38, bt_sav_blk_0,
			     param->sync_code_part_b[0].sav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_38, bt_eav_vld_0,
			     param->sync_code_part_b[0].eav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_38, bt_eav_blk_0,
			     param->sync_code_part_b[0].eav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_3c, bt_sav_vld_1,
			     param->sync_code_part_b[1].sav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_3c, bt_sav_blk_1,
			     param->sync_code_part_b[1].sav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_3c, bt_eav_vld_1,
			     param->sync_code_part_b[1].eav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_3c, bt_eav_blk_1,
			     param->sync_code_part_b[1].eav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_40, bt_sav_vld_2,
			     param->sync_code_part_b[2].sav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_40, bt_sav_blk_2,
			     param->sync_code_part_b[2].sav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_40, bt_eav_vld_2,
			     param->sync_code_part_b[2].eav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_40, bt_eav_blk_2,
			     param->sync_code_part_b[2].eav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_44, bt_sav_vld_3,
			     param->sync_code_part_b[3].sav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_44, bt_sav_blk_3,
			     param->sync_code_part_b[3].sav_blk);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_44, bt_eav_vld_3,
			     param->sync_code_part_b[3].eav_vld);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_44, bt_eav_blk_3,
			     param->sync_code_part_b[3].eav_blk);
	/* config bt clk inv  */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_00, pad_clk_inv,
			     param->clk_inv);
	/* config bt yc exchange  */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_48, bt_yc_inv,
			     param->yc_exchg);
	/* config bt image size  */
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_28, bt_img_wd_m1,
			     param->width);
	CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
			     reg_28, bt_img_ht_m1,
			     param->height);
}

static void _cif_csi_config(struct cif_ctx *ctx,
			   struct param_csi *param)
{
	uintptr_t wrap;
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	if (ctx->mac_num == CIF_MAC_0) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
	} else if (ctx->mac_num == CIF_MAC_1) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
	} else if (ctx->mac_num == CIF_MAC_2) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
	} else if (ctx->mac_num == CIF_MAC_3) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
	} else if (ctx->mac_num == CIF_MAC_4) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
	} else if (ctx->mac_num == CIF_MAC_5) {
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
	}

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, sensor_mac_mode,
			     1);
	/* invert the HS/VS */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, csi_vs_inv,
			     1);
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, csi_hs_inv,
			     1);
	/* CSI controller enable */
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_00, csi_ctrl_enable,
			     1);
	/* Config the format */
	//CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
	//		     REG_00, CSI_FORMAT_SET,
	//		     1 << param->fmt);
	/* Config the lane enable */
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_00, csi_lane_mode,
			     param->lane_num - 1);
	/* Config the VS gen mode */
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_70, csi_vs_gen_mode,
			     param->vs_gen_mode);
#ifdef FPGA_PORTING
	/*delay for virtual channel ,depend on sensor speed*/
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_70, CSI_VS_DELAY_SEL,
			     0X0);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_70, CSI_HS_DELAY_SEL,
			     0X0);
#endif
	/* [Note] disable auto_ignore and auto sync by default. */
	if (ctx->mac_num == CIF_MAC_0) {
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				     reg_10, auto_ignore,
				     0);
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				     reg_10, auto_sync,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				     reg_00, sensor_mode,
				     0);
	} else if (ctx->mac_num == CIF_MAC_1) {
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_10, auto_ignore,
				     0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_10, auto_sync,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_00, sensor_mode,
				     0);
	} else if (ctx->mac_num == CIF_MAC_2) {
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_10, auto_ignore,
				     0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_10, auto_sync,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_00, sensor_mode,
				     0);
	} else if (ctx->mac_num == CIF_MAC_3) {
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				     reg_10, auto_ignore,
				     0);
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				     reg_10, auto_sync,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				     reg_00, sensor_mode,
				     0);
	} else if (ctx->mac_num == CIF_MAC_4) {
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				     reg_10, auto_ignore,
				     0);
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				     reg_10, auto_sync,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				     reg_00, sensor_mode,
				     0);
	} else if (ctx->mac_num == CIF_MAC_5) {
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_10, auto_ignore,
				     0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_10, auto_sync,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				     reg_00, sensor_mode,
				     0);
	}
	/* Config csi vc mapping */
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_18, csi_vc_map_ch00,
			     param->vc_mapping[0]);
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_18, csi_vc_map_ch01,
			     param->vc_mapping[1]);
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_18, csi_vc_map_ch10,
			     param->vc_mapping[2]);
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_18, csi_vc_map_ch11,
			     param->vc_mapping[3]);
}

void cif_crop_info_line(struct cif_ctx *ctx, uint32_t line_num, uint32_t sw_up)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	/* Config the info line strip for HDR pattern 2 */
	if (line_num) {
		CIF_WR_BITS_GRP2(mac_top, reg_sensor_mac_t,
				     reg_48,
				     sensor_mac_info_line_num,
				     line_num,
				     sensor_mac_rm_info_line,
				     1);

		if (sw_up) {
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_00, sw_up,
					     1);
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_00, sw_up,
					     1);
		}
	} else {
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_48,
				     sensor_mac_rm_info_line,
				     0);
	}
}

void cif_set_crop(struct cif_ctx *ctx, struct cif_crop_s *crop)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	if (!crop->w || !crop->h)
		return;

	CIF_WR_BITS_GRP2(mac_top, reg_sensor_mac_t,
		reg_b4,
		sensor_mac_crop_start_y,
		crop->y,
		sensor_mac_crop_end_y,
		(crop->y + crop->h));
	CIF_WR_BITS_GRP3(mac_top, reg_sensor_mac_t,
		reg_b0,
		sensor_mac_crop_start_x,
		crop->x,
		sensor_mac_crop_end_x,
		(crop->x + crop->w),
		sensor_mac_crop_en,
		crop->enable);
}

int cif_swap_yuv(struct cif_ctx *ctx, uint8_t uv_swap, uint8_t yc_swap)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	CIF_WR_BITS_GRP2(mac_top, reg_sensor_mac_t,
		reg_b8,
		sensor_mac_swapuv_en,
		!!uv_swap,
		sensor_mac_swapyc_en,
		!!yc_swap);

	return 0;
}

void cif_set_bt_fmt_out(struct cif_ctx *ctx, enum ttl_bt_fmt_out fmt_out)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			     reg_10, ttl_bt_fmt_out,
			     fmt_out);
}

void cif_set_ttl_pinmux(struct cif_ctx *ctx,
			enum ttl_vi_from_e vi, enum ttl_vi_func_e func, uint32_t pad)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uint32_t offset;
	uint32_t value;
	uint32_t reg, mask;

	if (vi == FROM_VI2) {
		offset = (func - VI_FUNC_D0) * 4;
		value = pad << offset;
		mask = 0x7 << offset;
		reg = CIF_RD_REG(mac_top, reg_sensor_mac_t, reg_74);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, reg_sensor_mac_t,
				    reg_74,
				    reg);
		return;
	}

	offset = (func & 0x3) * 8;
	value = (vi << 5 | pad) << offset;
	mask = 0x3f << offset;

	if (func <= VI_FUNC_HDE) {
		reg = CIF_RD_REG(mac_top, reg_sensor_mac_t, reg_60);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, reg_sensor_mac_t,
				    reg_60,
				    reg);
	} else if (func <= VI_FUNC_D3) {
		reg = CIF_RD_REG(mac_top, reg_sensor_mac_t, reg_64);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, reg_sensor_mac_t,
				    reg_64,
				    reg);
	} else if (func <= VI_FUNC_D7) {
		reg = CIF_RD_REG(mac_top, reg_sensor_mac_t, reg_68);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, reg_sensor_mac_t,
				    reg_68,
				    reg);
	} else if (func <= VI_FUNC_D11) {
		reg = CIF_RD_REG(mac_top, reg_sensor_mac_t, reg_6c);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, reg_sensor_mac_t,
				    reg_6c,
				    reg);
	} else {
		reg = CIF_RD_REG(mac_top, reg_sensor_mac_t, reg_70);
		reg &= ~mask;
		reg |= value;
		CIF_WR_REG(mac_top, reg_sensor_mac_t,
				    reg_70,
				    reg);
	}
}

void cif_set_mac_vi_pinmux(struct cif_ctx *ctx,
			enum ttl_vi_from_e vi, enum ttl_vi_func_e func, uint32_t pad)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_VI_BLK_TOP];

	switch (func)
	{
	case VI_FUNC_D0:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_10, vi_bt_d0_sel, pad);
		break;
	case VI_FUNC_D1:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_10, vi_bt_d1_sel, pad);
		break;
	case VI_FUNC_D2:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_10, vi_bt_d2_sel, pad);
		break;
	case VI_FUNC_D3:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_10, vi_bt_d3_sel, pad);
		break;
	case VI_FUNC_D4:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_14, vi_bt_d4_sel, pad);
		break;
	case VI_FUNC_D5:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_14, vi_bt_d5_sel, pad);
		break;
	case VI_FUNC_D6:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_14, vi_bt_d6_sel, pad);
		break;
	case VI_FUNC_D7:
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				reg_14, vi_bt_d7_sel, pad);
		break;
	default:
		break;
	}
}

void cif_config(struct cif_ctx *ctx, struct cif_param *param)
{
	switch (param->type) {
	case CIF_TYPE_CSI:
		_cif_csi_config(ctx, &param->cfg.csi);
		break;
	case CIF_TYPE_SUBLVDS:
		_cif_sublvds_config(ctx, &param->cfg.sublvds);
		break;
	case CIF_TYPE_HISPI:
		_cif_hispi_config(ctx, &param->cfg.hispi);
		break;
	case CIF_TYPE_TTL:
		_cif_ttl_config(ctx, &param->cfg.ttl);
		break;
	case CIF_TYPE_BT_DMUX:
		_cif_btdemux_config(ctx, &param->cfg.btdemux);
		break;
	default:
		break;
	}
}

static void _cif_hdr_sublvds_enable(struct cif_ctx *ctx,
				    struct param_sublvds *param,
				    uint32_t on)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	if (!on) {
		/*[TODO] V FP recovery? info line strip? */
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_00, slvds_hdr_mode,
				       0);
		return;
	}

	/* Config the HDR. */
	switch (param->hdr_mode) {
	case CIF_SLVDS_HDR_PAT1:
		/* Config the sync code if raw 10, default is raw 12*/
		/* Select the HDR pattern */
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_00, slvds_hdr_pattern,
				       param->hdr_mode>>1);
		break;
	case CIF_SLVDS_HDR_PAT2:
		/* Config the HSIZE and HBlank per lane */
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_58, slvds_hdr_p2_hsize,
				       param->h_size/param->lane_num);
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_58, slvds_hdr_p2_hblank,
				       param->hdr_hblank[0]/param->lane_num);
		/* select the hdr pattern */
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_00, slvds_hdr_pattern,
				       param->hdr_mode>>1);
		break;
	}

	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_hdr_mode,
			       1);
}

static void _cif_hdr_csi_enable(struct cif_ctx *ctx,
				 struct param_csi *param,
				 uint32_t on)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	if (param->hdr_mode == CSI_HDR_MODE_VC) {
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_04, csi_hdr_mode,
				     0);
	} else if (param->hdr_mode == CSI_HDR_MODE_DT) {
		/* Enable dtat type mode. */
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_74, csi_hdr_dt_mode,
				     1);
		/* Program lef data type. */
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_74, csi_hdr_dt_lef,
				     param->data_type[0]);
		/* Program sef data type. */
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_74, csi_hdr_dt_sef,
				     param->data_type[1]);
		/* Program decode data type. */
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_74, csi_hdr_dt_format,
				     param->decode_type);
	} else if (param->hdr_mode == CSI_HDR_MODE_DOL) {
		/* Enable Sony DOL mode. */
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_04, csi_hdr_mode,
				     1);
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_04, csi_id_rm_else,
				     1);
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_04, csi_id_rm_ob,
				     1);
	} else {
		/* [TODO] */
		CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_04, csi_hdr_mode,
				     1);
	}
	/* CV181X not support invert the HDR */
	// CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
	// 		     REG_00, CSI_HDR_INV,
	// 		     1);
	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_04, csi_hdr_en,
			     !!on);
}

static void _cif_hdr_hispi_enable(struct cif_ctx *ctx,
				  struct param_hispi *param,
				  uint32_t on)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	if (param->mode == CIF_HISPI_MODE_PKT_SP) {
		CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
				       reg_60, hispi_hdr_psp_mode,
				       !!on);

	}
}

void cif_hdr_manual_config(struct cif_ctx *ctx,
			   struct cif_param *param,
			   uint32_t sw_up)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	if (!param->hdr_manual) {
		CIF_WR_BITS_GRP3(mac_top, reg_sensor_mac_t,
				 reg_40,
				 sensor_mac_hdr_en,
				 0,
				 sensor_mac_hdr_hdr0inv, // to-do
				 0,
				 sensor_mac_hdr_hdr1inv, // to-do
				 0);
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				 reg_40, sensor_mac_hdr_mode,
				 0);

		if (sw_up) {
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
					     reg_00, sw_up,
					     1);
		}
		return;
	}

	/* Config the HDR mode V size and T2 line shift */
	CIF_WR_BITS_GRP2(mac_top, reg_sensor_mac_t,
			 reg_44,
			 sensor_mac_hdr_vsize,
			 param->hdr_vsize,
			 sensor_mac_hdr_shift,
			 param->hdr_shift);

	CIF_WR_BITS_GRP3(mac_top, reg_sensor_mac_t,
			 reg_40,
			 sensor_mac_hdr_en,
			 1,
			 sensor_mac_hdr_hdr0inv, // to-do
			 0,
			 sensor_mac_hdr_hdr1inv, // to-do
			 0);
	CIF_WR_BITS(mac_top, reg_sensor_mac_t,
			 reg_40, sensor_mac_hdr_mode,
			 !!param->hdr_rm_padding);

	if (sw_up) {
		CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				     reg_00, sw_up,
				     1);
	}
}
EXPORT_SYMBOL_GPL(cif_hdr_manual_config);

void cif_hdr_enable(struct cif_ctx *ctx, struct cif_param *param, uint32_t on)
{
	if (param->hdr_manual)
		return;

	switch (param->type) {
	case CIF_TYPE_CSI:
		_cif_hdr_csi_enable(ctx, &param->cfg.csi, on);
		break;
	case CIF_TYPE_SUBLVDS:
		_cif_hdr_sublvds_enable(ctx, &param->cfg.sublvds, on);
		break;
	case CIF_TYPE_HISPI:
		_cif_hdr_hispi_enable(ctx, &param->cfg.hispi, on);
		break;
	default:
		break;
	}

}

static void csi_clear_lane_enable(struct cif_ctx *ctx)
{
	uintptr_t wrap;

	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				reg_0c, deskew_lane_en, 0);
		break;
	case CIF_MAC_1:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_0c, deskew_lane_en, 0);
		break;
	case CIF_MAC_2:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_0c, deskew_lane_en, 0);
		break;
	case CIF_MAC_3:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_0c, deskew_lane_en, 0);
		break;
	case CIF_MAC_4:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_0c, deskew_lane_en, 0);
		break;
	case CIF_MAC_5:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_0c, deskew_lane_en, 0);
		break;
	default:
		break;
	}
}

static void csi_lane_enable(struct cif_ctx *ctx, uint16_t lane_num)
{
	uintptr_t wrap;

	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				reg_0c, deskew_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_1:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_0c, deskew_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_2:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_0c, deskew_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_3:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_0c, deskew_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_4:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_0c, deskew_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_5:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_0c, deskew_lane_en, (1 << lane_num) - 1);
		break;
	default:
		break;
	}
}

static void sublvds_clear_lane_enable(struct cif_ctx *ctx)
{
	uintptr_t wrap;

	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				reg_20, slvds_lane_en, 0);
		break;
	case CIF_MAC_1:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_20, slvds_lane_en, 0);
		break;
	case CIF_MAC_2:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_20, slvds_lane_en, 0);
		break;
	case CIF_MAC_3:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_20, slvds_lane_en, 0);
		break;
	case CIF_MAC_4:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_20, slvds_lane_en, 0);
		break;
	case CIF_MAC_5:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_20, slvds_lane_en, 0);
		break;
	default:
		break;
	}
}

static void sublvds_lane_enable(struct cif_ctx *ctx, uint16_t lane_num)
{
	uintptr_t wrap;

	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
				reg_20, slvds_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_1:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_20, slvds_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_2:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_20, slvds_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_3:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_20, slvds_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_4:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
				reg_20, slvds_lane_en, (1 << lane_num) - 1);
		break;
	case CIF_MAC_5:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
				reg_20, slvds_lane_en, (1 << lane_num) - 1);
		break;
	default:
		break;
	}
}

void cif_stream_enable(struct cif_ctx *ctx, struct cif_param *param, uint32_t on)
{
	uintptr_t mac_top = (ctx->mac_num < CIF_MAC_VI_0) ? ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP] :
													ctx->mac_phys_regs[CIF_MAC_VI_BLK_TOP];
	//uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	union cif_cfg *cfg = &param->cfg;

	/* configure the phy termination only for serdes format. */
	//if (param->type <= CIF_TYPE_HISPI) {
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	    REG_00, MIPIRX_PD_IBIAS, 1);
		//CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		//	    REG_1C, EN_MIPI_LPRX, 0x3FFFF);
		// if (on) {
		// 	CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 		    REG_00, MIPIRX_PD_IBIAS, 0);
		// }
	//}

	switch (param->type) {
	case CIF_TYPE_CSI:
		/* clear the lane enable */
		csi_clear_lane_enable(ctx);
		if (on) {
			//CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			//	    REG_1C, EN_MIPI_LPRX, 0);
			udelay(20);
			/* lane enable */
			csi_lane_enable(ctx, cfg->csi.lane_num);
		}
		break;
	case CIF_TYPE_SUBLVDS:
		/* clear the lane enable */
		sublvds_clear_lane_enable(ctx);
		if (on) {
			udelay(20);
			/* lane enable */
			sublvds_lane_enable(ctx, cfg->sublvds.lane_num);
		}
		break;
	case CIF_TYPE_HISPI:
		/* clear the lane enable */
		sublvds_clear_lane_enable(ctx);

		if (on) {
			udelay(20);
			/* lane enable */
			sublvds_lane_enable(ctx, cfg->hispi.lane_num);
		}
		break;
	case CIF_TYPE_TTL:
		/* Enable TTL */
		if (ctx->mac_num < CIF_MAC_VI_0) {
			CIF_WR_BITS(mac_top, reg_sensor_mac_t,
				    reg_10, ttl_ip_en,
				    !!on);
		} else if (ctx->mac_num < CIF_MAC_NUM) {
			CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				    reg_20, bt_ip_en,
				    !!on);
		}
		break;
	case CIF_TYPE_BT_DMUX:
		/* Enable BT DEMUX */
		CIF_WR_BITS(mac_top, reg_sensor_mac_vi_t,
				     reg_20, bt_ip_en,
				     1);
		break;
	default:
		break;
	}

}

void cif_set_rx_bus_config(struct cif_ctx *ctx, enum lane_id_e lane, uint32_t select)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_10, pd_mipi_lane,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_10, pd_mipi_lane) & ~(1 << select));
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_08, en_rxbus_clk,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_08, en_rxbus_clk) | (1 << select));
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_04, en_clkrx_source,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_04, en_clkrx_source) & ~(1 << select));
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_10, pd_ref_lane, 0x0);
	if (ctx->cur_config->type == CIF_TYPE_CSI) {
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_1c, en_mipi_lprx,
				CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
					reg_1c, en_mipi_lprx) | 1 << select);
	}
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_20, en_demux,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_20, en_demux) | 1 << select);
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_24, en_preamp,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_24, en_preamp) | 1 << select);
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_28, en_vcm_det,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_28, en_vcm_det) | 1 << select);
	// CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
	// 		reg_2c, en_hvcmi,
	// 		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
	// 			reg_2c, en_hvcmi) | 1 << select);
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_2c, en_hvcmi,
			ctx->cur_config->type != CIF_TYPE_SUBLVDS ?
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_2c, en_hvcmi) & ~(1 << select) :
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_2c, en_hvcmi) | 1 << select);
	// CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
	// 		reg_40, en_mipi_drv,
	// 		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
	// 			reg_40, en_mipi_drv) | 1 << select);
	// CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
	// 		reg_44, en_mipi_ldo,
	// 		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
	// 			reg_44, en_mipi_ldo) | 1 << select);
	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_48, en_mipi_data_ser,
			CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_48, en_mipi_data_ser) | 1 << select);
	if (lane == CIF_LANE_CLK) {
		/* PHYA clock select */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_04, en_clkrx_source,
				CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
					reg_04, en_clkrx_source) | 1 << select);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_08, en_rxbus_clk,
				CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
					reg_08, en_rxbus_clk) & ~(1 << select));
	}
}

void lane_enable(struct cif_ctx *ctx, enum phy_lane_id_e lane_num)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_08, en_rxbus_clk,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_08, en_rxbus_clk) | (1 << lane_num));

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_1c, en_mipi_lprx,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_1c, en_mipi_lprx) | (1 << lane_num));

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_1c, en_mipi_lprx,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_1c, en_mipi_lprx) | (1 << lane_num));

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_20, en_demux,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_20, en_demux) | (1 << lane_num));

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_24, en_preamp,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_24, en_preamp) | (1 << lane_num));

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_28, en_vcm_det,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_28, en_vcm_det) | (1 << lane_num));

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		reg_48, en_mipi_data_ser,
		CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
			reg_48, en_mipi_data_ser) | (1 << lane_num));
}

void set_rx_frist_lane_enable(struct cif_ctx *ctx, int frist_lane_used)
{
	if (frist_lane_used == 1)
		return ;

	if (ctx->phy_mode == 0) {
		if (ctx->mac_num == 0)
			lane_enable(ctx, CIF_PHY_LANE_0);
	} else if (ctx->phy_mode == 1) {
		if (ctx->mac_num == 0)
			lane_enable(ctx, CIF_PHY_LANE_0);
		else if (ctx->mac_num == 3)
			lane_enable(ctx, CIF_PHY_LANE_9);
	} else if (ctx->phy_mode == 2) {
		if (ctx->mac_num == 0)
			lane_enable(ctx, CIF_PHY_LANE_0);
	} else if (ctx->phy_mode == 3) {
		if (ctx->mac_num == 0)
			lane_enable(ctx, CIF_PHY_LANE_0);
		else if (ctx->mac_num == 3)
			lane_enable(ctx, CIF_PHY_LANE_6);
		else if (ctx->mac_num == 4)
			lane_enable(ctx, CIF_PHY_LANE_12);
	} else if (ctx->phy_mode == 4) {
		if (ctx->mac_num == 0)
			lane_enable(ctx, CIF_PHY_LANE_0);
		if (ctx->mac_num == 3)
			lane_enable(ctx, CIF_PHY_LANE_6);
	} else if (ctx->phy_mode == 5) {
		if (ctx->mac_num == 0)
			lane_enable(ctx, CIF_PHY_LANE_0);
	}
}

static void cif_set_phy0_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_8l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];

	switch (lane) {
	case CIF_LANE_CLK:
		/* Enable phy mode */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_00, sensor_phy_mode,
				ctx->phy_mode);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_ck_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_ck_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_ck_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
			break;
	case CIF_LANE_0:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d0_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d0_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d0_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_1:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d1_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d1_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d1_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_2:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d2_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d2_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d2_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_3:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d3_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d3_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d3_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_4:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d4_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d4_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d4_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_5:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d5_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d5_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d4_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_6:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d6_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d6_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d6_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	case CIF_LANE_7:
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_04, csi_lane_d7_sel,
				select);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_08, csi_lane_d7_pnswap,
				pn_swap);
		CIF_WR_BITS(wrap_8l, reg_sensor_phy_8l_t,
				reg_14, csi_lane_d7_sel_h,
				select > CIF_PHY_LANE_15 ? 1 : 0);
		break;
	default:
		break;
	}
}

static void cif_set_phy1_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];

	switch (lane) {
	case CIF_LANE_CLK:
		/* Enable phy mode */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_00, sensor_phy_mode,
				ctx->phy_mode);
		/* PHYD clock select */
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_ck_sel,
				select % 3);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_ck_pnswap,
				pn_swap);
		break;
	case CIF_LANE_0:
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_04, csi_lane_d0_sel,
				select % 3);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_d0_pnswap,
				pn_swap);
		break;
	case CIF_LANE_1:
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_04, csi_lane_d1_sel,
				select % 3);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_d1_pnswap,
				pn_swap);
		break;
	default:
		break;
	}
}

static void cif_set_phy2_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];

	switch (lane) {
	case CIF_LANE_CLK:
		/* Enable phy mode */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_00, sensor_phy_mode,
				ctx->phy_mode);
		/* PHYD clock select */
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_ck_sel,
				select % 6);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_ck_pnswap,
				pn_swap);
		break;
	case CIF_LANE_0:
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_04, csi_lane_d0_sel,
				select % 6);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_d0_pnswap,
				pn_swap);
		break;
	case CIF_LANE_1:
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_04, csi_lane_d1_sel,
				select % 6);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_d1_pnswap,
				pn_swap);
		break;
	default:
		break;
	}
}

static void cif_set_phy3_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
	int offset;

	if (ctx->phy_mode == 1 || ctx->phy_mode == 2 || ctx->phy_mode == 5 || ctx->phy_mode == 6) {
		offset = 9;
	} else if (ctx->phy_mode == 3 || ctx->phy_mode == 4) {
		offset = 6;
	}

	switch (lane) {
	case CIF_LANE_CLK:
		/* Enable phy mode */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_00, sensor_phy_mode,
				ctx->phy_mode);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_ck_sel,
				select % offset);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_ck_pnswap,
				pn_swap);
		break;
	case CIF_LANE_0:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d0_sel,
				select % offset);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d0_pnswap,
				pn_swap);
		break;
	case CIF_LANE_1:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d1_sel,
				select % offset);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d1_pnswap,
				pn_swap);
		break;
	case CIF_LANE_2:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d2_sel,
				select % offset);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d2_pnswap,
				pn_swap);
		break;
	case CIF_LANE_3:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d3_sel,
				select % offset);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d3_pnswap,
				pn_swap);
		break;
	default:
		break;
	}
}

static void cif_set_phy4_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];

	switch (lane) {
	case CIF_LANE_CLK:
		/* Enable phy mode */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_00, sensor_phy_mode,
				ctx->phy_mode);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_ck_sel,
				select % 12);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_ck_pnswap,
				pn_swap);
		break;
	case CIF_LANE_0:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d0_sel,
				select % 12);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d0_pnswap,
				pn_swap);
		break;
	case CIF_LANE_1:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d1_sel,
				select % 12);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d1_pnswap,
				pn_swap);
		break;
	case CIF_LANE_2:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d2_sel,
				select % 12);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d2_pnswap,
				pn_swap);
		break;
	case CIF_LANE_3:
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_04, csi_lane_d3_sel,
				select % 12);
		CIF_WR_BITS(wrap_4l, reg_sensor_phy_4l_t,
				reg_08, csi_lane_d3_pnswap,
				pn_swap);
		break;
	default:
		break;
	}
}

static void cif_set_phy5_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];

	switch (lane) {
	case CIF_LANE_CLK:
		/* Enable phy mode */
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_00, sensor_phy_mode,
				ctx->phy_mode);
		/* PHYD clock select */
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_ck_sel,
				select % 15);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_ck_pnswap,
				pn_swap);
		break;
	case CIF_LANE_0:
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_04, csi_lane_d0_sel,
				select % 15);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_d0_pnswap,
				pn_swap);
		break;
	case CIF_LANE_1:
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_04, csi_lane_d1_sel,
				select % 15);
		CIF_WR_BITS(wrap_2l, reg_sensor_phy_2l_t,
				reg_08, csi_lane_d1_pnswap,
				pn_swap);
		break;
	default:
		break;
	}
}

void cif_streaming(struct cif_ctx *ctx, uint32_t on, uint32_t hdr)
{
	/* CIF OFF */
	cif_stream_enable(ctx, ctx->cur_config, 0);

	if (on) {
		cif_config(ctx, ctx->cur_config);
		cif_hdr_enable(ctx, ctx->cur_config, hdr);
		/* CIF ON */
		cif_stream_enable(ctx, ctx->cur_config, on);
	}
}

void cif_set_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	switch (ctx->mac_num) {
	case CIF_MAC_0:
		cif_set_phy0_lane_id(ctx, lane, select, pn_swap);
		break;
	case CIF_MAC_1:
		cif_set_phy1_lane_id(ctx, lane, select, pn_swap);
		break;
	case CIF_MAC_2:
		cif_set_phy2_lane_id(ctx, lane, select, pn_swap);
		break;
	case CIF_MAC_3:
		cif_set_phy3_lane_id(ctx, lane, select, pn_swap);
		break;
	case CIF_MAC_4:
		cif_set_phy4_lane_id(ctx, lane, select, pn_swap);
		break;
	case CIF_MAC_5:
		cif_set_phy5_lane_id(ctx, lane, select, pn_swap);
		break;
	default:
		break;
	}
}

// to-do: check with RD
void cif_set_clk_edge(struct cif_ctx *ctx,
		      enum phy_lane_id_e lane, enum cif_clk_edge_e edge)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
		      reg_c4, ad_clk_inv,
		      edge << lane);
}

void cif_set_group(struct cif_ctx *ctx, int gruop)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	if (ctx->cur_config->type == CIF_TYPE_CSI) {
		if (gruop == 0) {
			CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				      reg_10, pd_pll,
				      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_10, pd_pll) & 0x6);
		} else if (gruop == 1) {
			CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				      reg_10, pd_pll,
				      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_10, pd_pll) | 0x5);
		} else if (gruop == 2) {
			CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				      reg_10, pd_pll,
				      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_10, pd_pll) | 0x3);
		}
	}

}

void cif_set_pd(struct cif_ctx *ctx, short clock_lane)
{
	short frist_clock;
	short mask;
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	if (ctx->phy_mode == 0) {
		frist_clock = CIF_PHY_LANE_0;
	} else if (ctx->phy_mode == 1) {
		if (ctx->mac_num == 0) {
			frist_clock = CIF_PHY_LANE_0;
		} else if (ctx->mac_num == 3) {
			frist_clock = CIF_PHY_LANE_9;
		}
	} else if (ctx->phy_mode == 2) {
		if (ctx->mac_num == 0) {
			frist_clock = CIF_PHY_LANE_0;
		} else if (ctx->mac_num == 3) {
			frist_clock = CIF_PHY_LANE_9;
		}
	} else if (ctx->phy_mode == 3) {
		if (ctx->mac_num == 0) {
			frist_clock = CIF_PHY_LANE_0;
		} else if (ctx->mac_num == 3) {
			frist_clock = CIF_PHY_LANE_9;
		} else if (ctx->mac_num == 4) {
			frist_clock = CIF_PHY_LANE_12;
		}
	} else if (ctx->phy_mode == 4) {

		if (ctx->mac_num == 0) {
			frist_clock = CIF_PHY_LANE_0;
		} else if (ctx->mac_num == 3) {
			frist_clock = CIF_PHY_LANE_9;
		}

	} else if (ctx->phy_mode == 5) {
		if (ctx->mac_num == 0) {
			frist_clock = CIF_PHY_LANE_0;
		}
	}
	mask = ((1 << (clock_lane + 1)) - 1) & ~((1 << frist_clock) - 1);

	CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
				reg_10, pd_mipi_lane,
				CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_10, pd_mipi_lane) & ~mask);

}

void cif_set_clk_dir(struct cif_ctx *ctx, enum cif_clk_dir_e dir)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (dir) {
	case CIF_CLK_P02P1:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusl) | 0x1);
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	      REG_10, PD_REF_LANE,
		// 	      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T, REG_10, PD_REF_LANE) & 0x6);
		break;
	case CIF_CLK_P12P0:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusr) | 0x1);
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	      REG_10, PD_REF_LANE,
		// 	      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T, REG_10, PD_REF_LANE) & 0x6);
		break;
	case CIF_CLK_P12P2:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusr_to_extr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusr_to_extr) | 0x1);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_extl_to_clkbusl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_extl_to_clkbusl) | 0x2);
		break;
	case CIF_CLK_P22P1:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_extr_to_clkbusr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_extr_to_clkbusr) | 0x1);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusl_to_extl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusl_to_extl) | 0x2);
		break;
	case CIF_CLK_P22P3:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusl) | 0x2);
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	      REG_10, PD_REF_LANE,
		// 	      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T, REG_10, PD_REF_LANE) & 0x5);
		break;
	case CIF_CLK_P32P2:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusr) | 0x2);
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	      REG_10, PD_REF_LANE,
		// 	      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T, REG_10, PD_REF_LANE) & 0x5);
		break;
	case CIF_CLK_P32P4:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusr_to_extr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusr_to_extr) | 0x2);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_extl_to_clkbusl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_extl_to_clkbusl) | 0x4);
		break;
	case CIF_CLK_P42P3:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_extr_to_clkbusr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_extr_to_clkbusr) | 0x2);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusl_to_extl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusl_to_extl) | 0x4);
		break;
	case CIF_CLK_P42P5:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusl,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusl) | 0x4);
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	      REG_10, PD_REF_LANE,
		// 	      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T, REG_10, PD_REF_LANE) & 0x3);
		break;
	case CIF_CLK_P52P4:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_4c, en_clkbusr,
			      CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t, reg_4c, en_clkbusr) | 0x4);
		// CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
		// 	      REG_10, PD_REF_LANE,
		// 	      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T, REG_10, PD_REF_LANE) & 0x3);
		break;
	//case CIF_CLK_FREERUN:
	//	CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
	//		      REG_4C, MIPIRX_SEL_CLK_P0TOP1,
	//		      0);
	//	CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
	//		      REG_4C, MIPIRX_SEL_CLK_P1TOP0,
	//		      0);
	//	break;
	default:
		break;
	}
}

void cif_set_hs_settle(struct cif_ctx *ctx, uint8_t hs_settle)
{
	uintptr_t wrap;

	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			      reg_10, auto_ignore,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			      reg_10, auto_sync,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_8l_t,
			      reg_10, t_hs_settle,
			      hs_settle);
		break;
	case CIF_MAC_1:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, auto_ignore,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, auto_sync,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, t_hs_settle,
			      hs_settle);
		break;
	case CIF_MAC_2:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, auto_ignore,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, auto_sync,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, t_hs_settle,
			      hs_settle);
		break;
	case CIF_MAC_3:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
			      reg_10, auto_ignore,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
			      reg_10, auto_sync,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
			      reg_10, t_hs_settle,
			      hs_settle);
		break;
	case CIF_MAC_4:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
			      reg_10, auto_ignore,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
			      reg_10, auto_sync,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_4l_t,
			      reg_10, t_hs_settle,
			      hs_settle);
		break;
	case CIF_MAC_5:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, auto_ignore,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, auto_sync,
			      0);
		CIF_WR_BITS(wrap, reg_sensor_phy_2l_t,
			      reg_10, t_hs_settle,
			      hs_settle);
		break;
	default:
		break;
	}
}

uint8_t cif_get_lane_data(struct cif_ctx *ctx, enum phy_lane_id_e lane)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uint8_t value;

	switch (lane) {
	case CIF_PHY_LANE_0:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d0, ad_d0_data);
		break;
	case CIF_PHY_LANE_1:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d0, ad_d1_data);
		break;
	case CIF_PHY_LANE_2:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d0, ad_d2_data);
		break;
	case CIF_PHY_LANE_3:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d0, ad_d3_data);
		break;
	case CIF_PHY_LANE_4:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d4, ad_d4_data);
		break;
	case CIF_PHY_LANE_5:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d4, ad_d5_data);
		break;
	case CIF_PHY_LANE_6:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d4, ad_d6_data);
		break;
	case CIF_PHY_LANE_7:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d4, ad_d7_data);
		break;
	case CIF_PHY_LANE_8:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d8, ad_d8_data);
		break;
	case CIF_PHY_LANE_9:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d8, ad_d9_data);
		break;
	case CIF_PHY_LANE_10:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d8, ad_d10_data);
		break;
	case CIF_PHY_LANE_11:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_d8, ad_d11_data);
		break;
	case CIF_PHY_LANE_12:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_dc, ad_d12_data);
		break;
	case CIF_PHY_LANE_13:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_dc, ad_d13_data);
		break;
	case CIF_PHY_LANE_14:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_dc, ad_d14_data);
		break;
	case CIF_PHY_LANE_15:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_dc, ad_d15_data);
		break;
	case CIF_PHY_LANE_16:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_e0, ad_d16_data);
		break;
	case CIF_PHY_LANE_17:
		value = CIF_RD_BITS(wrap_top, reg_sensor_phy_top_t,
				    reg_e0, ad_d17_data);
		break;
	default:
		value = 0;
		break;
	}

	return value;
}

void cif_set_lane_deskew(struct cif_ctx *ctx,
		      enum phy_lane_id_e lane, uint8_t phase)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (lane) {
	case CIF_PHY_LANE_0:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f0, deskew_code0,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code0,
			      !!phase);
		break;
	case CIF_PHY_LANE_1:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f0, deskew_code1,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code1,
			      !!phase);
		break;
	case CIF_PHY_LANE_2:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f0, deskew_code2,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code2,
			      !!phase);
		break;
	case CIF_PHY_LANE_3:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f0, deskew_code3,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code3,
			      !!phase);
		break;
	case CIF_PHY_LANE_4:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f4, deskew_code4,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code4,
			      !!phase);
		break;
	case CIF_PHY_LANE_5:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f4, deskew_code5,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code5,
			      !!phase);
		break;
	case CIF_PHY_LANE_6:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f4, deskew_code6,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code6,
			      !!phase);
		break;
	case CIF_PHY_LANE_7:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f4, deskew_code7,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code7,
			      !!phase);
		break;
	case CIF_PHY_LANE_8:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f8, deskew_code8,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code8,
			      !!phase);
		break;
	case CIF_PHY_LANE_9:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f8, deskew_code9,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code9,
			      !!phase);
		break;
	case CIF_PHY_LANE_10:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f8, deskew_code10,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code10,
			      !!phase);
		break;
	case CIF_PHY_LANE_11:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_f8, deskew_code11,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code11,
			      !!phase);
		break;
	case CIF_PHY_LANE_12:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_fc, deskew_code12,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code12,
			      !!phase);
		break;
	case CIF_PHY_LANE_13:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_fc, deskew_code13,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code13,
			      !!phase);
		break;
	case CIF_PHY_LANE_14:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_fc, deskew_code14,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code14,
			      !!phase);
		break;
	case CIF_PHY_LANE_15:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_fc, deskew_code15,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code15,
			      !!phase);
		break;
	case CIF_PHY_LANE_16:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_100, deskew_code16,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code16,
			      !!phase);
		break;
	case CIF_PHY_LANE_17:
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_100, deskew_code17,
			      phase);
		CIF_WR_BITS(wrap_top, reg_sensor_phy_top_t,
			      reg_ec, force_deskew_code17,
			      !!phase);
		break;
	default:
		break;
	}
}

void cif_set_lvds_endian(struct cif_ctx *ctx,
			 enum cif_endian mac, enum cif_endian wrap)
{
	uintptr_t wrap_phy;
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	/* Config the endian. */
	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_data_reverse,
			       mac == CIF_SLVDS_ENDIAN_LSB);
	/* DPHY endian mode select */
	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap_phy = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		CIF_WR_BITS(wrap_phy, reg_sensor_phy_8l_t,
				      reg_20, slvds_inv_en,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
		break;
	case CIF_MAC_1:
		wrap_phy = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		CIF_WR_BITS(wrap_phy, reg_sensor_phy_2l_t,
				      reg_20, slvds_inv_en,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
		break;
	case CIF_MAC_2:
		wrap_phy = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		CIF_WR_BITS(wrap_phy, reg_sensor_phy_2l_t,
				      reg_20, slvds_inv_en,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
		break;
	case CIF_MAC_3:
		wrap_phy = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		CIF_WR_BITS(wrap_phy, reg_sensor_phy_4l_t,
				      reg_20, slvds_inv_en,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
		break;
	case CIF_MAC_4:
		wrap_phy = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		CIF_WR_BITS(wrap_phy, reg_sensor_phy_4l_t,
				      reg_20, slvds_inv_en,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
		break;
	case CIF_MAC_5:
		wrap_phy = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		CIF_WR_BITS(wrap_phy, reg_sensor_phy_2l_t,
				      reg_20, slvds_inv_en,
				      wrap == CIF_SLVDS_ENDIAN_MSB);
		break;
	default:
		break;
	}
}

void cif_set_lvds_vsync_gen(struct cif_ctx *ctx, uint32_t fp)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	CIF_WR_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
			       reg_00, slvds_vfporch,
			       fp);
}

int cif_check_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
	uint32_t reg = CIF_RD_REG(mac_csi, reg_csi_ctrl_top_t, reg_60);

	return !!(reg & mask);
}

void cif_clear_csi_int_sts(struct cif_ctx *ctx)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_04, csi_intr_clr,
			     0xff);
}

void cif_mask_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_04, csi_intr_mask,
			     0xff);
}

void cif_unmask_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, reg_csi_ctrl_top_t,
			     reg_04, csi_intr_mask,
			     0x00);
}

int cif_check_csi_fifo_full(struct cif_ctx *ctx)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	return !!CIF_RD_BITS(mac_csi, reg_csi_ctrl_top_t,
				      reg_40, csi_fifo_full);
}

int cif_check_lvds_fifo_full(struct cif_ctx *ctx)
{
	uintptr_t mac_slvds = ctx->mac_phys_regs[CIF_MAC_BLK_ID_SLVDS];

	return !!CIF_RD_BITS(mac_slvds, reg_sublvds_ctrl_top_t,
					reg_40, slvds_fifo_full);
}

int cif_get_csi_decode_fmt(struct cif_ctx *ctx)
{
	int i;
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
	uint32_t value = CIF_RD_BITS(mac_csi, reg_csi_ctrl_top_t,
				     reg_40, csi_decode_format);

	for (i = 0; i < DEC_FMT_NUM; i++) {
		if (value & (1 << i))
			return i;
	}

	return i;
}

int cif_get_csi_phy_state(struct cif_ctx *ctx, union mipi_phy_state *state)
{
	uintptr_t wrap;

	switch (ctx->mac_num) {
	case CIF_MAC_0:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_8L_0];
		state->raw_ckhs = CIF_RD_REG(wrap, reg_sensor_phy_8l_t, dbg_90);
		state->raw_datahs = CIF_RD_REG(wrap, reg_sensor_phy_8l_t, dbg_94);
		break;
	case CIF_MAC_1:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_1];
		state->raw_ckhs = CIF_RD_REG(wrap, reg_sensor_phy_2l_t, dbg_90);
		state->raw_datahs = CIF_RD_REG(wrap, reg_sensor_phy_2l_t, dbg_94);
		break;
	case CIF_MAC_2:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_2];
		state->raw_ckhs = CIF_RD_REG(wrap, reg_sensor_phy_2l_t, dbg_90);
		state->raw_datahs = CIF_RD_REG(wrap, reg_sensor_phy_2l_t, dbg_94);
		break;
	case CIF_MAC_3:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_3];
		state->raw_ckhs = CIF_RD_REG(wrap, reg_sensor_phy_4l_t, dbg_90);
		state->raw_datahs = CIF_RD_REG(wrap, reg_sensor_phy_4l_t, dbg_94);
		break;
	case CIF_MAC_4:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L_4];
		state->raw_ckhs = CIF_RD_REG(wrap, reg_sensor_phy_4l_t, dbg_90);
		state->raw_datahs = CIF_RD_REG(wrap, reg_sensor_phy_4l_t, dbg_94);
		break;
	case CIF_MAC_5:
		wrap = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L_5];
		state->raw_ckhs = CIF_RD_REG(wrap, reg_sensor_phy_2l_t, dbg_90);
		state->raw_datahs = CIF_RD_REG(wrap, reg_sensor_phy_2l_t, dbg_94);
		break;
	default:
		break;
	}

	return 0;
}

uintptr_t *cif_get_mac_phys_reg_bases(uint32_t link)
{
	switch (link) {
	case CIF_MAC_0:
	case CIF_MAC_1:
	case CIF_MAC_2:
	case CIF_MAC_3:
	case CIF_MAC_4:
	case CIF_MAC_5:
		{
		m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_TOP] =
						(CIF_MAC_BLK_BA_TOP);
		m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_SLVDS] =
						(CIF_MAC_BLK_BA_SLVDS);
		m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_CSI] =
						(CIF_MAC_BLK_BA_CSI);

		return m_cif_mac_phys_base_list[link];
		}
	case CIF_MAC_VI_0:
	case CIF_MAC_VI_1:
		{
		m_cif_macvi_phys_base_list[link][CIF_MAC_VI_BLK_TOP] =
						(CIF_MAC_VI_BLK_BA_TOP);
		m_cif_macvi_phys_base_list[link][CIF_MAC_VI_BLK_ID_SLVDS] =
						(CIF_MAC_VI_BLK_BA_SLVDS);
		m_cif_macvi_phys_base_list[link][CIF_MAC_VI_BLK_ID_CSI] =
						(CIF_MAC_VI_BLK_BA_CSI);

		return m_cif_macvi_phys_base_list[link];
		}
	default:
		break;
	}
	return 0;
}

uintptr_t *cif_get_wrap_phys_reg_bases(uint32_t link)
{
	static uintptr_t m_cif_wrap_phys_base_list[MAX_LINK_NUM]
		[CIF_WRAP_BLK_ID_MAX];

	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_TOP] =
					(CIF_WRAP_BLK_BA_TOP);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_8L_0] =
					(CIF_WRAP_BLK_BA_8L_0);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_2L_1] =
					(CIF_WRAP_BLK_BA_2L_1);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_2L_2] =
					(CIF_WRAP_BLK_BA_2L_2);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_4L_3] =
					(CIF_WRAP_BLK_BA_4L_3);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_4L_4] =
					(CIF_WRAP_BLK_BA_4L_4);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_2L_5] =
					(CIF_WRAP_BLK_BA_2L_5);
	return m_cif_wrap_phys_base_list[link];
}
