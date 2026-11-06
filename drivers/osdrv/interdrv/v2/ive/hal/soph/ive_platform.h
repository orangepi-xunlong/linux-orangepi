/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: ive_platform.h
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef _IVE_PLATFORM_H_
#define _IVE_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */
#include "linux/comm_ive.h"
#include "ive_interface.h"
#include "linux/ive_uapi.h"
#include "ive_reg.h"
#include "reg.h"

enum filterop_mod {
	MOD_BYP = 0,
	MOD_FILTER3CH = 1,
	MOD_DILA = 2,
	MOD_ERO = 3,
	MOD_CANNY = 4,
	MOD_STBOX = 5,
	MOD_GRADFG = 6,
	MOD_MAG = 7,
	MOD_NORMG = 8,
	MOD_SOBEL = 9,
	MOD_STCANDI = 10,
	MOD_MAP = 11,
	MOD_GMM = 12, //9c2fe24
	MOD_BGM = 13, //9c2fe24
	MOD_BGU = 14, //9c2fe24
	MOD_HIST,
	MOD_NCC,
	MOD_INTEG,
	MOD_DMA,
	MOD_SAD,
	MOD_ADD, //20
	MOD_AND,
	MOD_OR,
	MOD_SUB,
	MOD_XOR,
	MOD_THRESH,
	MOD_THRS16,
	MOD_THRU16,
	MOD_16To8,
	MOD_CSC,
	MOD_FILTERCSC, //30
	MOD_ORDSTAFTR,
	MOD_RESIZE,
	MOD_LBP,
	MOD_GMM2,
	MOD_BERNSEN,
	MOD_MD,
	MOD_CMDQ,
	MOD_ED,
	MOD_TEST,
	MOD_RESET, //40
	MOD_DUMP,
	MOD_QUERY,
	MOD_CCL,
	MOD_ALL, //44
};

enum dma_name {
	RDMA_IMG_IN = 0,
	RDMA_IMG1,
	RDMA_EIGVAL,
	RDMA_RADFG,
	RDMA_MM_FACTOR,
	RDMA_MM_MOD = 5,
	RDMA_GMODEL_0,
	RDMA_GMODEL_1,
	RDMA_GFLAG,
	RDMA_DMA,
	WDMA_DMA,
	WDMA_ODMA,
	WDMA_Y,
	WDMA_C,
	WDMA_HIST,
	WDMA_INTEG,
	WDMA_SAD,
	WDMA_SAD_THR,
	WDMA_GMM_MATCH,
	WDMA_GMM_MOD,
	WDMA_CHG,
	WDMA_BGMODEL_0,
	WDMA_BGMODEL_1,
	WDMA_FG,
	DMA_ALL,
};

#ifdef DEBUG
static char IveRegister[53][100] = {
	"IVE_TOP                 ", /* NULL if not initialized. */
	"IMG_IN                  ", /* NULL if not initialized. */
	"RDMA_IMG1               ", /* NULL if not initialized. */
	"MAP                     ", /* NULL if not initialized. */
	"HIST                    ", /* NULL if not initialized. */
	"HIST_WDMA               ", /* NULL if not initialized. */
	"INTG                    ", /* NULL if not initialized. */
	"INTG_WDMA               ", /* NULL if not initialized. */
	"SAD                     ", /* NULL if not initialized. */
	"SAD_WDMA                ", /* NULL if not initialized. */
	"SAD_WDMA_THR            ", /* NULL if not initialized. */
	"NCC                     ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_0        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_1        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_2        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_3        ", /* NULL if not initialized. */
	"GMM_MODEL_RDMA_4        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_0        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_1        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_2        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_3        ", /* NULL if not initialized. */
	"GMM_MODEL_WDMA_4        ", /* NULL if not initialized. */
	"GMM_MATCH_WDMA          ", /* NULL if not initialized. */
	"GMM                     ", /* NULL if not initialized. */
	"GMM_FACTOR_RDMA         ", /* NULL if not initialized. */
	"BG_MATCH_FGFLAG_RDMA    ", /* NULL if not initialized. */
	"BG_MATCH_BGMODEL_0_RDMA ", /* NULL if not initialized. */
	"BG_MATCH_BGMODEL_1_RDMA ", /* NULL if not initialized. */
	"BG_MATCH_DIFFFG_WDMA    ", /* NULL if not initialized. */
	"BG_MATCH_IVE_MATCH_BG   ", /* NULL if not initialized. */
	"BG_UPDATE_FG_WDMA       ", /* NULL if not initialized. */
	"BG_UPDATE_BGMODEL_0_WDMA", /* NULL if not initialized. */
	"BG_UPDATE_BGMODEL_1_WDMA", /* NULL if not initialized. */
	"BG_UPDATE_CHG_WDMA      ", /* NULL if not initialized. */
	"BG_UPDATE_UPDATE_BG     ", /* NULL if not initialized. */
	"FILTEROP_RDMA           ", /* NULL if not initialized. */
	"FILTEROP_WDMA_Y         ", /* NULL if not initialized. */
	"FILTEROP_WDMA_C         ", /* NULL if not initialized. */
	"FILTEROP                ", /* NULL if not initialized. */
	"CCL                     ", /* NULL if not initialized. */
	"CCL_SRC_RDMA            ", /* NULL if not initialized. */
	"CCL_DST_WDMA            ", /* NULL if not initialized. */
	"CCL_REGION_WDMA         ", /* NULL if not initialized. */
	"CCL_SRC_RDMA_RELABEL    ", /* NULL if not initialized. */
	"CCL_DST_WDMA_RELABEL    ", /* NULL if not initialized. */
	"DMAF                    ", /* NULL if not initialized. */
	"DMAF_WDMA               ", /* NULL if not initialized. */
	"DMAF_RDMA               ", /* NULL if not initialized. */
	"LK                      ", /* NULL if not initialized. */
	"RDMA_EIGVAL             ", /* NULL if not initialized. */
	"WDMA                    ", /* NULL if not initialized. */
	"RDMA                    ", /* NULL if not initialized. */
	"CMDQ                    " /* NULL if not initialized. */
};
#endif

struct _IVE_ADDR_S {
	char addr_name[16];
	bool addr_en;
	int addr_l;
	int addr_h;
};

struct _IVE_MODE_S {
	bool op_en;
	int op_sel;
};

struct _IVE_DEBUG_INFO_S {
	char op_name[16];
	int src_w;
	int src_h;
	int src_fmt;
	int dst_fmt;
	struct _IVE_MODE_S op[2];
	struct _IVE_ADDR_S addr[24];
};

struct _IVE_IP_BLOCK_S {
	void __iomem *IVE_TOP; /* NULL if not initialized. */
	void __iomem *IMG_IN; /* NULL if not initialized. */
	void __iomem *RDMA_IMG1; /* NULL if not initialized. */
	void __iomem *MAP; /* NULL if not initialized. */
	void __iomem *HIST; /* NULL if not initialized. */
	void __iomem *HIST_WDMA; /* NULL if not initialized. */
	void __iomem *INTG; /* NULL if not initialized. */
	void __iomem *INTG_WDMA; /* NULL if not initialized. */
	void __iomem *SAD; /* NULL if not initialized. */
	void __iomem *SAD_WDMA; /* NULL if not initialized. */
	void __iomem *SAD_WDMA_THR; /* NULL if not initialized. */
	void __iomem *NCC; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_0; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_1; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_2; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_3; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_4; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_0; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_1; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_2; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_3; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_4; /* NULL if not initialized. */
	void __iomem *GMM_MATCH_WDMA; /* NULL if not initialized. */
	void __iomem *GMM; /* NULL if not initialized. */
	void __iomem *GMM_FACTOR_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_FGFLAG_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_BGMODEL_0_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_BGMODEL_1_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_DIFFFG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_IVE_MATCH_BG; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_FG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_BGMODEL_0_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_BGMODEL_1_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_CHG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_UPDATE_UPDATE_BG; /* NULL if not initialized. */
	void __iomem *FILTEROP_RDMA; /* NULL if not initialized. */
	void __iomem *FILTEROP_WDMA_Y; /* NULL if not initialized. */
	void __iomem *FILTEROP_WDMA_C; /* NULL if not initialized. */
	void __iomem *FILTEROP; /* NULL if not initialized. */

	void __iomem *CCL; /* NULL if not initialized. */
	void __iomem *CCL_SRC_RDMA; /* NULL if not initialized. */
	void __iomem *CCL_DST_WDMA; /* NULL if not initialized. */
	void __iomem *CCL_REGION_WDMA; /* NULL if not initialized. */
	void __iomem *CCL_SRC_RDMA_RELABEL; /* NULL if not initialized. */
	void __iomem *CCL_DST_WDMA_RELABEL; /* NULL if not initialized. */

	void __iomem *DMAF; /* NULL if not initialized. */
	void __iomem *DMAF_WDMA; /* NULL if not initialized. */
	void __iomem *DMAF_RDMA; /* NULL if not initialized. */
	void __iomem *LK; /* NULL if not initialized. */
	void __iomem *RDMA_EIGVAL; /* NULL if not initialized. */
	void __iomem *WDMA; /* NULL if not initialized. */
	void __iomem *RDMA; /* NULL if not initialized. */
	void __iomem *CMDQ; /* NULL if not initialized. */
};

struct cmdq_adma {
	uint64_t addr;
	uint32_t size;
	uint32_t flags_end : 1;
	uint32_t rsv : 2;
	uint32_t flags_link : 1;
	uint32_t rsv2 : 28;
};

enum {
	CMDQ_SET_REG,
	CMDQ_SET_WAIT_TIMER,
	CMDQ_SET_WAIT_FLAG,
};

struct cmdq_set_reg {
	uint32_t data;
	uint32_t addr : 20;
	uint32_t byte_mask : 4;
	uint32_t intr_end : 1;
	uint32_t intr_int : 1;
	uint32_t intr_last : 1;
	uint32_t intr_rsv : 1;
	uint32_t action : 4;  // 0 for this case
};

struct cmdq_set_wait_timer {
	uint32_t counter;
	uint32_t rsv : 24;
	uint32_t intr_end : 1;
	uint32_t intr_int : 1;
	uint32_t intr_last : 1;
	uint32_t intr_rsv : 1;
	uint32_t action : 4;  // 1 for this case
};

struct cmdq_set_wait_flags {
	uint32_t flag_num;   // 0 ~ 15, depending on each module
	uint32_t rsv : 24;
	uint32_t intr_end : 1;
	uint32_t intr_int : 1;
	uint32_t intr_last : 1;
	uint32_t intr_rsv : 1;
	uint32_t action : 4;  // 2 for this case
};

union cmdq_set {
	struct cmdq_set_reg reg;
	struct cmdq_set_wait_timer wait_timer;
	struct cmdq_set_wait_flags wait_flags;
};


s32 ive_go(struct ive_device *ndev, ive_top_c *ive_top_c,
		   bool instant, int done_mask, int optype, int dev_id);

s32 assign_ive_block_addr(void __iomem *ive_phy_base, int dev_id);

irqreturn_t platform_ive_irq(struct ive_device *ndev, int dev_id);

s32 ive_dump_hw_flow(void);
s32 ive_dump_op1_op2_info(void);
s32 ive_dump_reg_state(bool enable);
s32 ive_set_dma_dump(bool enable);
s32 ive_set_reg_dump(bool enable);
s32 ive_set_img_dump(bool enable);
void stcandicorner_workaround(struct ive_device *ndev);

s32 _ive_reset(struct ive_device *ndev, int select, int dev_id);

s32 ive_test(struct ive_device *ndev, char *addr, u16 *w,
		     u16 *h);

s32 ive_cmdq(struct ive_device *ndev, int dev_id);

s32 ive_query(struct ive_device *ndev, bool *pbFinish,
		      bool bBlock, s32 dev_id);

s32 ive_dma(struct ive_device *ndev, ive_data_s *pstSrc,
		    ive_data_s *pstDst, ive_dma_ctrl_s *pstDmaCtrl,
		    bool instant, int dev_id);

s32 ive_and(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		    ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
		    bool instant, int dev_id);

s32 ive_or(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		   ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
		   bool instant, int dev_id);

s32 ive_xor(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		    ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
		    bool instant, int dev_id);

s32 ive_add(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		    ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
		    ive_add_ctrl_s *pstAddCtrl, bool instant, int dev_id);

s32 ive_sub(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		    ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
		    ive_sub_ctrl_s *pstSubCtrl, bool instant, int dev_id);

s32 ive_thresh(struct ive_device *ndev, ive_src_image_s *pstSrc,
		       ive_dst_image_s *pstDst, ive_thresh_ctrl_s *pstThrCtrl,
		       bool instant, int dev_id);

s32 ive_dilate(struct ive_device *ndev, ive_src_image_s *pstSrc,
		       ive_dst_image_s *pstDst,
		       ive_dilate_ctrl_s *pstDilateCtrl, bool instant, int dev_id);

s32 ive_erode(struct ive_device *ndev, ive_src_image_s *pstSrc,
		      ive_dst_image_s *pstDst, ive_erode_ctrl_s *pstErodeCtrl,
		      bool instant, int dev_id);

s32 ive_match_bg_model(struct ive_device *ndev,
			     ive_src_image_s *pstCurImg, ive_data_s *pstBgModel,
			     ive_image_s *pstFgFlag, ive_dst_image_s *pstDiffFg,
			     ive_bg_stat_data_s *pstStatData,
			     ive_match_bg_model_ctrl_s *pstMatchBgModelCtrl,
			     bool instant, int dev_id);

s32 ive_update_bg_model(struct ive_device *ndev,
				  ive_src_image_s *pstCurImg,
			      ive_data_s *pstBgModel, ive_image_s *pstFgFlag,
			      ive_dst_image_s *pstBgImg,
			      ive_dst_image_s *pstChgSta,
			      ive_bg_stat_data_s *pstStatData,
			      ive_update_bg_model_ctrl_s *pstUpdateBgModelCtrl,
			      bool instant, int dev_id);

s32 ive_gmm(struct ive_device *ndev, ive_src_image_s *pstSrc,
		    ive_dst_image_s *pstFg, ive_dst_image_s *pstBg,
		    ive_mem_info_s *pstModel, ive_gmm_ctrl_s *pstGmmCtrl,
		    bool instant, int dev_id);

s32 ive_gmm2(struct ive_device *ndev, ive_src_image_s *pstSrc,
		     ive_src_image_s *pstFactor, ive_dst_image_s *pstFg,
		     ive_dst_image_s *pstBg, ive_dst_image_s *pstMatchModelInfo,
		     ive_mem_info_s *pstModel, ive_gmm2_ctrl_s *pstGmm2Ctrl,
		     bool instant, int dev_id);

s32 ive_bernsen(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstDst, ive_bernsen_ctrl_s *pstLbpCtrl,
			bool instant, int dev_id);

s32 ive_filter(struct ive_device *ndev, ive_src_image_s *pstSrc,
		       ive_dst_image_s *pstDst, ive_filter_ctrl_s *pstFltCtrl,
		       bool instant, int dev_id);

s32 ive_sobel(struct ive_device *ndev, ive_src_image_s *pstSrc,
		      ive_dst_image_s *pstDstH, ive_dst_image_s *pstDstV,
		      ive_sobel_ctrl_s *pstSobelCtrl, bool instant, int dev_id);

s32 ive_mag_and_ang(struct ive_device *ndev, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstDstMag,
			  ive_dst_image_s *pstDstAng,
			  ive_mag_and_ang_ctrl_s *pstMagAndAngCtrl,
			  bool instant, int dev_id);

s32 ive_csc(struct ive_device *ndev, ive_src_image_s *pstSrc,
		    ive_dst_image_s *pstDst, ive_csc_ctrl_s *pstCscCtrl,
		    bool instant, int dev_id);

s32 ive_filter_and_csc(struct ive_device *ndev,
			     ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
			     ive_filter_and_csc_ctrl_s *pstFltCscCtrl,
			     bool instant, int dev_id);

s32 ive_hist(struct ive_device *ndev, ive_src_image_s *pstSrc,
		     ive_dst_mem_info_s *pstDst, bool instant, int dev_id);

s32 ive_map(struct ive_device *ndev, ive_src_image_s *pstSrc,
		    u16 *pstMap, ive_dst_image_s *pstDst,
		    ive_map_ctrl_s *pstMapCtrl, bool instant, int dev_id);

s32 ive_ncc(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		    ive_src_image_s *pstSrc2, ive_ncc_dst_mem_s *pstDst,
		    bool instant, int dev_id);

s32 ive_integ(struct ive_device *ndev, ive_src_image_s *pstSrc,
		      ive_dst_mem_info_s *pstDst,
		      ive_integ_ctrl_s *pstIntegCtrl, bool instant, int dev_id);

s32 ive_lbp(struct ive_device *ndev, ive_src_image_s *pstSrc,
		    ive_dst_image_s *pstDst, ive_lbp_ctrl_s *pstLbpCtrl,
		    bool instant, int dev_id);

s32 ive_thresh_s16(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst,
			   ive_thresh_s16_ctrl_s *pstThrS16Ctrl,
			   bool instant, int dev_id);

s32 ive_thresh_u16(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst,
			   ive_thresh_u16_ctrl_s *pstThrU16Ctrl,
			   bool instant, int dev_id);

s32 ive_16bit_to_8bit(struct ive_device *ndev,
			    ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
			    ive_16bit_to_8bit_ctrl_s *pst16BitTo8BitCtrl,
			    bool instant, int dev_id);

s32 ive_ord_stat_filter(struct ive_device *ndev,
			      ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
			      ive_ord_stat_filter_ctrl_s *pstOrdStatFltCtrl,
			      bool instant, int dev_id);

s32 ive_canny_hys_edge(struct ive_device *ndev,
			     ive_src_image_s *pstSrc, ive_dst_image_s *pstEdge,
			     ive_dst_mem_info_s *pstStack,
			     ive_canny_hys_edge_ctrl_s *pstCannyHysEdgeCtrl,
			     bool instant, int dev_id);

s32 ive_norm_grad(struct ive_device *ndev, ive_src_image_s *pstSrc,
			 ive_dst_image_s *pstDstH, ive_dst_image_s *pstDstV,
			 ive_dst_image_s *pstDstHV,
			 ive_norm_grad_ctrl_s *pstNormGradCtrl,
			 bool instant, int dev_id);

s32 ive_grad_fg(struct ive_device *ndev,
		       ive_src_image_s *pstBgDiffFg,
		       ive_src_image_s *pstCurGrad, ive_src_image_s *pstBgGrad,
		       ive_dst_image_s *pstGradFg,
		       ive_grad_fg_ctrl_s *pstGradFgCtrl, bool instant, int dev_id);

s32 ive_sad(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		    ive_src_image_s *pstSrc2, ive_dst_image_s *pstSad,
		    ive_dst_image_s *pstThr, ive_sad_ctrl_s *pstSadCtrl,
		    bool instant, int dev_id);

s32 ive_resize(struct ive_device *ndev, ive_src_image_s *pstSrc,
		       ive_dst_image_s *pstDst,
		       ive_resize_ctrl_s *pstResizeCtrl, bool instant, int dev_id);

s32 ive_imgIn_to_odma(struct ive_device *ndev,
			    ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
			    ive_filter_ctrl_s *pstFltCtrl, bool instant, int dev_id);

s32 ive_rgbp_to_yuv_to_erode_to_dilate(struct ive_device *ndev,
					 ive_src_image_s *pstSrc,
					 ive_dst_image_s *pstDst,
					 ive_dst_image_s *pstDst2,
					 ive_filter_ctrl_s *pstFltCtrl,
					 bool instant, int dev_id);

s32 ive_stcandi_corner(struct ive_device *ndev,
			      ive_src_image_s *pstSrc,
			      ive_dst_image_s *pstCandiCorner,
			      ive_st_candi_corner_ctrl_s *pstStCandiCornerCtrl,
			      bool instant, int dev_id);

s32 ive_equalize_hist(struct ive_device *ndev,
			     ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
			     ive_equalize_hist_ctrl_s *pstEqualizeHistCtrl,
			     bool instant);

s32 ive_ccl(struct ive_device *ndev, ive_image_s *src_dst,
				ive_dst_mem_info_s *blob, ive_ccl_ctrl_s *ccl_ctrl,
				bool instant, s32 dev_id);

s32 mpi_ive_ccl(ive_handle *ive_handle, ive_image_s *pstSrcDst,
			ive_dst_mem_info_s *pstBlob, ive_ccl_ctrl_s *pstCclCtrl,
			bool instant);

s32 mpi_ive_canny_edge(ive_image_s *pstEdge, ive_mem_info_s *pstStack);

s32 mpi_ive_lk_optical_flow_pyr(
	ive_handle *ive_handle, ive_src_image_s astSrcPrevPyr[],
	ive_src_image_s astSrcNextPyr[], ive_src_mem_info_s *pstPrevPts,
	ive_mem_info_s *pstNextPts, ive_dst_mem_info_s *pstStatus,
	ive_dst_mem_info_s *pstErr,
	ive_lk_optical_flow_pyr_ctrl_s *pstLkOptiFlowPyrCtrl,
	bool instant);

s32
ive_frame_diff_motion(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			ive_frame_diff_motion_ctrl_s *ctrl, bool instant, int dev_id);

s32
mpi_ive_st_candi_corner(ive_handle *ive_handle, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstCandiCorner,
			  ive_st_candi_corner_ctrl_s *pstStCandiCornerCtrl,
			  bool instant);

s32 mpi_ive_st_corner(ive_src_image_s *pstCandiCorner,
			     ive_dst_mem_info_s *pstCorner,
			     ive_st_corner_ctrl_s *pstStCornerCtrl);

s32 mpi_ive_ann_mlp_load_model(const char *pchFileName,
				      ive_ann_mlp_model_s *pstAnnMlpModel);

void mpi_ive_ann_mlp_unload_model(ive_ann_mlp_model_s *pstAnnMlpModel);

s32 mpi_ive_ann_mlp_predict(ive_handle *ive_handle,
				    ive_src_data_s *pstSrc,
				    ive_look_up_table_s *pstActivFuncTab,
				    ive_ann_mlp_model_s *pstAnnMlpModel,
				    ive_dst_data_s *pstDst, bool instant);

s32 mpi_ive_query(ive_handle IveHandle, bool *pbFinish,
			  bool bBlock);

#ifdef __cplusplus
}
#endif
#endif
