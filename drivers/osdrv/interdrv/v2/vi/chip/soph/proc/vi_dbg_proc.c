#include <linux/version.h>
#include <proc/vi_dbg_proc.h>
#include <vi_isp_buf_ctrl.h>

#define VI_DBG_PROC_NAME	"soph/vi_dbg"

extern int sop_isp_rdy_buf_empty(struct sop_vi_dev *vdev, const u8 chn_num);
extern struct isp_queue splt_out_q[][ISP_SPLT_CHN_MAX],
			pre_fe_in_q[][ISP_SPLT_CHN_MAX],
			pre_fe_out_q[][ISP_FE_CHN_MAX],
			pre_be_in_q, pre_be_in_se_q[],
			pre_be_out_q[],
			postraw_in_q[];

/* Switch the output of proc.
 *
 * 0 ~ ISP_BLK_ID_MAX: block reg-dump
 * 255: vi debug info
 */
static int proc_isp_mode = 255;

/*************************************************************************
 *	Proc functions
 *************************************************************************/
static inline void _vi_dbg_reg_dump(struct isp_ctx *ctx, struct seq_file *m, int blk)
{
	static u8 init_done;
	static struct isp_dump_info m_block[ISP_BLK_ID_MAX] = {0};

	if (init_done)
		goto DUMP;

#define BLK_INFO(_para, _name, _struct) \
	do {\
		_para[ISP_BLK_ID_##_name].phy_base = ISP_BLK_BA_##_name + ISP_TOP_PHY_REG_BASE;\
		_para[ISP_BLK_ID_##_name].reg_base = ctx->phys_regs[ISP_BLK_ID_##_name];\
		_para[ISP_BLK_ID_##_name].blk_size = sizeof(struct _struct) / 4;\
	} while (0)

	BLK_INFO(m_block, PRE_RAW_FE0, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG0, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG2, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG3, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE0_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_FE0_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, RGBMAP_FE0_LE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG0, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE0_RGBMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RGBMAP_FE0_SE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG1, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE0_RGBMAP_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE1, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG1, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG2, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG3, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE1_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_FE1_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, RGBMAP_FE1_LE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG2, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE1_RGBMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RGBMAP_FE1_SE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG3, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE1_RGBMAP_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE2, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG2, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI2_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI2_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE2_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_FE2_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, RGBMAP_FE2_LE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG4, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE2_RGBMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RGBMAP_FE2_SE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG5, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE2_RGBMAP_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE3, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG3, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI3_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI3_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE3_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_FE3_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, RGBMAP_FE3_LE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG6, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE3_RGBMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RGBMAP_FE3_SE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG7, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE3_RGBMAP_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE4, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG4, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI4_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI4_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE4_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_FE4_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, RGBMAP_FE4_LE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG8, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE4_RGBMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RGBMAP_FE4_SE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG9, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE4_RGBMAP_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE5, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG5, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI5_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI5_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE5_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_FE5_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, RGBMAP_FE5_LE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG10, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE5_RGBMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RGBMAP_FE5_SE, reg_isp_rgbmap_t);
	BLK_INFO(m_block, RGBMAP_WBG11, reg_isp_wbg_t);
	BLK_INFO(m_block, DMA_CTL_FE5_RGBMAP_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_BE, reg_pre_raw_be_t);
	BLK_INFO(m_block, BE_CROP_LE, reg_crop_t);
	BLK_INFO(m_block, BE_CROP_SE, reg_crop_t);
	BLK_INFO(m_block, PRE_RAW_BE_BLC0, reg_isp_blc_t);
	BLK_INFO(m_block, PRE_RAW_BE_BLC1, reg_isp_blc_t);
	BLK_INFO(m_block, AF, reg_isp_af_t);
	BLK_INFO(m_block, DMA_CTL_AF_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DPC0, reg_isp_dpc_t);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_BE_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_BE_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_WDMA, reg_pre_wdma_ctrl_t);
	// BLK_INFO(m_block, PCHK0, );
	// BLK_INFO(m_block, PCHK1, );
	BLK_INFO(m_block, RGBIR0, reg_isp_rgbir_t);
	BLK_INFO(m_block, DMA_CTL_RGBIR_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DPC1, reg_isp_dpc_t);
	BLK_INFO(m_block, RGBIR1, reg_isp_rgbir_t);
	BLK_INFO(m_block, DMA_CTL_RGBIR_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, WDMA_CORE0, reg_wdma_core_t);
	BLK_INFO(m_block, WDMA_CORE1, reg_wdma_core_t);
	BLK_INFO(m_block, WDMA_CORE2, reg_wdma_core_t);
	BLK_INFO(m_block, WDMA_CORE3, reg_wdma_core_t);

	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_WDMA_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_WDMA_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, SPLT_FE0_WDMA, reg_pre_wdma_ctrl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_RDMA_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, SPLT_FE0_RDMA_LE, reg_raw_rdma_ctrl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_RDMA_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, SPLT_FE0_RDMA_SE, reg_raw_rdma_ctrl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_WDMA_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_WDMA_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, SPLT_FE1_WDMA, reg_pre_wdma_ctrl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_RDMA_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, SPLT_FE1_RDMA_LE, reg_raw_rdma_ctrl_t);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_RDMA_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, SPLT_FE1_RDMA_SE, reg_raw_rdma_ctrl_t);
	BLK_INFO(m_block, SPLT, reg_isp_line_spliter_t);

	BLK_INFO(m_block, RAWTOP, reg_raw_top_t);
	BLK_INFO(m_block, CFA0, reg_isp_cfa_t);
	BLK_INFO(m_block, LSC0, reg_isp_lsc_t);
	BLK_INFO(m_block, DMA_CTL_LSC_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, GMS, reg_isp_gms_t);
	BLK_INFO(m_block, DMA_CTL_GMS, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, AE_HIST0, reg_isp_ae_hist_t);
	BLK_INFO(m_block, DMA_CTL_AE_HIST_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, AE_HIST1, reg_isp_ae_hist_t);
	BLK_INFO(m_block, DMA_CTL_AE_HIST_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_RAW_RDMA0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RAW_RDMA0, reg_raw_rdma_ctrl_t);
	BLK_INFO(m_block, DMA_CTL_RAW_RDMA1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RAW_RDMA1, reg_raw_rdma_ctrl_t);
	BLK_INFO(m_block, CFA1, reg_isp_cfa_t);
	BLK_INFO(m_block, LSC1, reg_isp_lsc_t);
	BLK_INFO(m_block, DMA_CTL_LSC_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, LMAP1, reg_isp_lmap_t);
	BLK_INFO(m_block, DMA_CTL_LMAP_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, BNR0, reg_isp_bnr_t);
	BLK_INFO(m_block, BNR1, reg_isp_bnr_t);
	BLK_INFO(m_block, RAW_CROP_LE, reg_crop_t);
	BLK_INFO(m_block, RAW_CROP_SE, reg_crop_t);
	BLK_INFO(m_block, LMAP0, reg_isp_lmap_t);
	BLK_INFO(m_block, DMA_CTL_LMAP_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, RAW_WBG0, reg_isp_wbg_t);
	BLK_INFO(m_block, RAW_WBG1, reg_isp_wbg_t);
	// BLK_INFO(m_block, PCHK2, );
	// BLK_INFO(m_block, PCHK3, );
	BLK_INFO(m_block, LCAC0, reg_isp_lcac_t);
	BLK_INFO(m_block, RGBCAC0, reg_isp_rgbcac_t);
	BLK_INFO(m_block, LCAC1, reg_isp_lcac_t);
	BLK_INFO(m_block, RGBCAC1, reg_isp_rgbcac_t);

	BLK_INFO(m_block, RGBTOP, reg_isp_rgb_top_t);
	BLK_INFO(m_block, CCM0, reg_isp_ccm_t);
	BLK_INFO(m_block, CCM1, reg_isp_ccm_t);
	BLK_INFO(m_block, RGBGAMMA, reg_isp_gamma_t);
	BLK_INFO(m_block, YGAMMA, reg_ygamma_t);
	BLK_INFO(m_block, MMAP, reg_isp_mmap_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_PRE_LE_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_PRE_SE_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_CUR_LE_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_CUR_SE_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_IIR_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_IIR_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_MMAP_AI_ISP, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, CLUT, reg_isp_clut_t);
	BLK_INFO(m_block, DEHAZE, reg_isp_dehaze_t);
	BLK_INFO(m_block, CSC, reg_isp_csc_t);
	BLK_INFO(m_block, RGB_DITHER, reg_isp_rgb_dither_t);
	// BLK_INFO(m_block, PCHK4, );
	// BLK_INFO(m_block, PCHK5, );
	BLK_INFO(m_block, HIST_EDGE_V, reg_isp_hist_edge_v_t);
	BLK_INFO(m_block, DMA_CTL_HIST_EDGE_V, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, FUSION, reg_fusion_t);
	BLK_INFO(m_block, LTM, reg_ltm_t);
	BLK_INFO(m_block, DMA_CTL_LTM_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_LTM_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, YUVTOP, reg_yuv_top_t);
	BLK_INFO(m_block, TNR, reg_isp_444_422_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_MO, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_MO, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_C, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, FBCE, reg_fbce_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_C, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, FBCD, reg_fbcd_t);
	BLK_INFO(m_block, YUV_DITHER, reg_isp_yuv_dither_t);
	BLK_INFO(m_block, CA, reg_ca_t);
	BLK_INFO(m_block, CA_LITE, reg_ca_lite_t);
	BLK_INFO(m_block, YNR, reg_isp_ynr_t);
	BLK_INFO(m_block, CNR, reg_isp_cnr_t);
	BLK_INFO(m_block, EE_POST, reg_isp_ee_t);
	BLK_INFO(m_block, YCURVE, reg_isp_ycurv_t);
	BLK_INFO(m_block, DCI, reg_isp_dci_t);
	BLK_INFO(m_block, DMA_CTL_DCI, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DCI_GAMMA, reg_isp_gamma_t);
	BLK_INFO(m_block, YUV_CROP_Y, reg_crop_t);
	BLK_INFO(m_block, DMA_CTL_YUV_CROP_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, YUV_CROP_C, reg_crop_t);
	BLK_INFO(m_block, DMA_CTL_YUV_CROP_C, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, LDCI, reg_isp_ldci_t);
	BLK_INFO(m_block, DMA_CTL_LDCI_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_LDCI_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, EE_PRE, reg_isp_preyee_t);
	BLK_INFO(m_block, DMA_CTL_AI_ISP_RDMA_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_AI_ISP_RDMA_U, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_AI_ISP_RDMA_V, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, ISPTOP, reg_isp_top_t);
	BLK_INFO(m_block, RDMA_CORE0, reg_rdma_core_t);
	BLK_INFO(m_block, RDMA_CORE1, reg_rdma_core_t);
	BLK_INFO(m_block, CSIBDG0_LITE, reg_isp_csi_bdg_lite_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE2, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE3, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, CSIBDG1_LITE, reg_isp_csi_bdg_lite_t);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE2, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE3, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_VI_SEL, reg_pre_raw_vi_sel_t);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_VI_SEL_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_VI_SEL_SE, reg_isp_dma_ctl_t);
	// BLK_INFO(m_block, CMDQ, );

	init_done = true;

DUMP:

#define BLK_DUMP(_para, _blk)\
	do {\
		int i = 0;\
		for (i = 0; i < _para[_blk].blk_size; i += 4) {\
			if (m == NULL) {\
				if (i == 0) {\
					vi_pr(VI_ERR, "%22X, %10X, %10X, %10X\n", 0x00, 0x04, 0x08, 0x0C);\
				} \
				vi_pr(VI_ERR, "0x%llx: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",\
					(_para[_blk].phy_base + i * 0x4),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 0) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 1) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 2) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 3) * 0x4)));\
			} else {\
				if (i == 0) {\
					seq_printf(m, "%22X, %10X, %10X, %10X\n", 0x00, 0x04, 0x08, 0x0C);\
				} \
				seq_printf(m, "0x%llx: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",\
					(_para[_blk].phy_base + i * 0x4),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 0) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 1) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 2) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 3) * 0x4)));\
			} \
		} \
	} while (0)

	if (blk >= 0 && blk < ISP_BLK_ID_MAX)
		BLK_DUMP(m_block, blk);
}

static inline void _vi_dbg_proc_show(struct seq_file *m, void *v)
{
	struct sop_vi_dev *vdev = m->private;
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;
	struct timespec64 ts1, ts2;
	u32 sofCnt1[ISP_PRERAW_MAX], sofCnt2[ISP_PRERAW_MAX];
	u32 frmCnt1[ISP_PRERAW_MAX], frmCnt2[ISP_PRERAW_MAX];
	u64 t2 = 0, t1 = 0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;
		sofCnt1[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) //YUV sensor
			frmCnt1[raw_num] = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
		else //RGB sensor
			frmCnt1[raw_num] = vdev->postraw_frame_number[raw_num];
	}

	ktime_get_real_ts64(&ts1);
	t1 = ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000;

	msleep(940);
	do {
		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			if (!vdev->ctx.isp_pipe_enable[raw_num])
				continue;
			sofCnt2[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
			if (vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) //YUV sensor
				frmCnt2[raw_num] = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
			else //RGB sensor
				frmCnt2[raw_num] = vdev->postraw_frame_number[raw_num];
		}

		ktime_get_real_ts64(&ts2);
		t2 = ts2.tv_sec * 1000000 + ts2.tv_nsec / 1000;
	} while ((t2 - t1) < 1000000);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (raw_num == vi_get_first_raw_num(ctx)) {
			seq_puts(m, "[VI BE_Dbg_Info]\n");
			seq_printf(m, "VIPreBEDoneSts\t\t:0x%x\t\tVIPreBEDmaIdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.be_sts.be_done_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.be_sts.be_dma_idle_sts);
			seq_puts(m, "[VI Post_Dbg_Info]\n");
			seq_printf(m, "VIIspTopStatus0\t\t:0x%x\t\tVIIspTopStatus1\t\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.post_sts.top_sts_0,
					ctx->isp_pipe_cfg[raw_num].dg_info.post_sts.top_sts_1);
			seq_puts(m, "[VI DMA_Dbg_Info]\n");
			seq_printf(m, "VIWdma0ErrStatus\t:0x%x\tVIWdma0IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_idle);
			seq_printf(m, "VIWdma1ErrStatus\t:0x%x\tVIWdma1IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_idle);
			seq_printf(m, "VIWdma2ErrStatus\t:0x%x\tVIWdma2IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_2_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_2_idle);
			seq_printf(m, "VIWdma3ErrStatus\t:0x%x\tVIWdma3IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_3_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_3_idle);
			seq_printf(m, "VIRdma0ErrStatus\t:0x%x\tVIRdma0IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_0_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_0_idle);
			seq_printf(m, "VIRdma1ErrStatus\t:0x%x\tVIRdma1IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_1_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_1_idle);
		}

		seq_printf(m, "[VI ISP_PIPE_%d FE_Dbg_Info]\n", raw_num);
		seq_printf(m, "VIPreFERawDbgSts\t:0x%x\t\tVIPreFEDbgInfo\t\t:0x%x\n",
				ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts.fe_idle_sts,
				ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts.fe_done_sts);

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			seq_printf(m, "[VI ISP_PIPE_%d %s]\n", raw_num,
				(!ctx->isp_pipe_cfg[raw_num].is_work_on_r_tile) ? "Left_Tile" : "Right_Tile");
		} else {
			seq_printf(m, "[VI ISP_PIPE_%d]\n", raw_num);
		}
		seq_printf(m, "VIOutImgWidth\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].post_img_w);
		seq_printf(m, "VIOutImgHeight\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].post_img_h);
		seq_printf(m, "VIInImgWidth\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].csibdg_width);
		seq_printf(m, "VIInImgHeight\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].csibdg_height);

		seq_printf(m, "VIDevFPS\t\t:%4d\n", sofCnt2[raw_num] - sofCnt1[raw_num]);
		seq_printf(m, "VIFPS\t\t\t:%4d\n", frmCnt2[raw_num] - frmCnt1[raw_num]);

		seq_printf(m, "VISofCh0Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
		    ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_1MULTIPLEX)
			seq_printf(m, "VISofCh1Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH1]);
		if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_2MULTIPLEX)
			seq_printf(m, "VISofCh2Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH2]);
		if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_3MULTIPLEX)
			seq_printf(m, "VISofCh3Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH3]);

		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
		    ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT ||
		    ctx->isp_pipe_cfg[raw_num].is_tile) {
			seq_printf(m, "VISpltWdmaCh0Cnt\t:%4d\n", vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH0]);
			seq_printf(m, "VISpltRdmaCh0Cnt\t:%4d\n", vdev->splt_rdma_frm_num[raw_num][ISP_FE_CH0]);
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				seq_printf(m, "VISpltWdmaCh1Cnt\t:%4d\n", vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH1]);
				seq_printf(m, "VISpltRdmaCh1Cnt\t:%4d\n", vdev->splt_rdma_frm_num[raw_num][ISP_FE_CH1]);
			}
		}

		seq_printf(m, "VIPreFECh0Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
		    ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_1MULTIPLEX)
			seq_printf(m, "VIPreFECh1Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);
		if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_2MULTIPLEX)
			seq_printf(m, "VIPreFECh2Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH2]);
		if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_3MULTIPLEX)
			seq_printf(m, "VIPreFECh3Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH3]);

		seq_printf(m, "VIPreBECh0Cnt\t\t:%4d\n", vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			seq_printf(m, "VIPreBECh1Cnt\t\t:%4d\n", vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);

		seq_printf(m, "VIPostCnt\t\t:%4d\n", vdev->postraw_frame_number[raw_num]);
		seq_printf(m, "VIDropCnt\t\t:%4d\n", vdev->drop_frame_number[raw_num]);
		seq_printf(m, "VIDumpCnt\t\t:%4d\n", vdev->dump_frame_number[raw_num]);

		seq_printf(m, "[VI ISP_PIPE_%d Csi_Dbg_Info]\n", raw_num);
		seq_printf(m, "VICsiIntStatus0\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_0);
		seq_printf(m, "VICsiIntStatus1\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_1);
		seq_printf(m, "VICsiOverFlowCnt\t:%4d\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt);

		for (chn_num = ISP_FE_CH0; chn_num < ISP_FE_CHN_MAX; chn_num++) {
			if (chn_num == ISP_FE_CH1) {
				if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
				    ctx->isp_pipe_cfg[raw_num].mux_mode < VI_WORK_MODE_2MULTIPLEX)
					break;
			} else if (chn_num == ISP_FE_CH2) {
				if (ctx->isp_pipe_cfg[raw_num].mux_mode < VI_WORK_MODE_3MULTIPLEX)
					break;
			} else if (chn_num == ISP_FE_CH3) {
				if (ctx->isp_pipe_cfg[raw_num].mux_mode < VI_WORK_MODE_4MULTIPLEX)
					break;
			}

			seq_printf(m, "VICsiCh%dDbg0\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_0);
			seq_printf(m, "VICsiCh%dDbg1\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_1);
			seq_printf(m, "VICsiCh%dDbg2\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_2);
			seq_printf(m, "VICsiCh%dDbg3\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_3);

			seq_printf(m, "VICsiCh%dWidthGTCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[chn_num]);
			seq_printf(m, "VICsiCh%dWidthLSCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[chn_num]);
			seq_printf(m, "VICsiCh%dHeightGTCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[chn_num]);
			seq_printf(m, "VICsiCh%dHeightLSCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[chn_num]);
		}

		seq_printf(m, "[VI ISP_PIPE_%d Buf_Dbg_Info]\n", raw_num);
		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
		    ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT ||
		    ctx->isp_pipe_cfg[raw_num].is_tile) {
			seq_printf(m, "VISpltCh0OutBufEmpty\t:%4d\n",
					isp_buf_empty(&splt_out_q[raw_num][ISP_SPLT_CHN0]));
			seq_printf(m, "VIPreFECh0InBufEmpty\t:%4d\n",
					isp_buf_empty(&pre_fe_in_q[raw_num][ISP_SPLT_CHN1]));
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				seq_printf(m, "VISpltCh1OutBufEmpty\t:%4d\n",
						isp_buf_empty(&splt_out_q[raw_num][ISP_SPLT_CHN0]));
				seq_printf(m, "VIPreFECh1InBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_fe_in_q[raw_num][ISP_SPLT_CHN1]));
			}
		}
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
		    ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			seq_printf(m, "VIYuvCh0OutBufEmpty\t:%4d\n",
				sop_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH0));
			if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_1MULTIPLEX)
				seq_printf(m, "VIYuvCh1OutBufEmpty\t:%4d\n",
					sop_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH1));
			if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_2MULTIPLEX)
				seq_printf(m, "VIYuvCh2OutBufEmpty\t:%4d\n",
					sop_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH2));
			if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_3MULTIPLEX)
				seq_printf(m, "VIYuvCh3OutBufEmpty\t:%4d\n",
					sop_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH3));
		} else {
			if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { // fe->be->dram->post
				seq_printf(m, "VIPreBECh0OutBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_be_out_q[ISP_FE_CH0]));
				seq_printf(m, "VIPostCh0InBufEmpty\t:%4d\n",
						isp_buf_empty(&postraw_in_q[ISP_FE_CH0]));
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					seq_printf(m, "VIPreBECh1OutBufEmpty\t:%4d\n",
							isp_buf_empty(&pre_be_out_q[ISP_FE_CH1]));
					seq_printf(m, "VIPostCh1InBufEmpty\t:%4d\n",
							isp_buf_empty(&postraw_in_q[ISP_FE_CH1]));
				}
			} else if (_is_be_post_online(ctx)) { // fe->dram->be->post
				seq_printf(m, "VIPreFECh0OutBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_fe_out_q[raw_num][ISP_FE_CH0]));
				seq_printf(m, "VIPreBECh0InBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_be_in_q));
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					seq_printf(m, "VIPreFECh1OutBufEmpty\t:%4d\n",
							isp_buf_empty(&pre_fe_out_q[raw_num][ISP_FE_CH1]));
					seq_printf(m, "VIPreBECh1InBufEmpty\t:%4d\n",
							isp_buf_empty(&pre_be_in_se_q[raw_num]));
				}
			}
			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
				seq_printf(m, "VIPostOutBufEmpty\t:%4d\n", sop_isp_rdy_buf_empty(vdev, raw_num));
			}
		}
	}
}

static int vi_dbg_proc_show(struct seq_file *m, void *v)
{
	struct sop_vi_dev *vdev = m->private;

	if (proc_isp_mode == 255)
		_vi_dbg_proc_show(m, v);
	else
		_vi_dbg_reg_dump(&vdev->ctx, m, proc_isp_mode);

	return 0;
}

static ssize_t vi_dbg_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(cProcInputdata)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(cProcInputdata, 10, &proc_isp_mode))
		proc_isp_mode = 255;

	return count;
}

static int vi_dbg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vi_dbg_proc_show, PDE_DATA(inode));
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops vi_dbg_proc_fops = {
	.proc_open = vi_dbg_proc_open,
	.proc_read = seq_read,
	.proc_write = vi_dbg_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vi_dbg_proc_fops = {
	.owner = THIS_MODULE,
	.open = vi_dbg_proc_open,
	.read = seq_read,
	.write = vi_dbg_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vi_dbg_proc_init(struct sop_vi_dev *_vdev)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VI_DBG_PROC_NAME, 0644, NULL, &vi_dbg_proc_fops, _vdev) == NULL) {
		pr_err("vi_dbg proc creation failed\n");
		rc = -EAGAIN;
	}

	return rc;
}

int vi_dbg_proc_remove(void)
{
	remove_proc_entry(VI_DBG_PROC_NAME, NULL);

	return 0;
}
