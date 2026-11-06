#ifndef __VI_DRV_H__
#define __VI_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/delay.h>
#include <linux/types.h>

#include <isp_reg.h>
#include <vi_reg_blocks.h>
#include <vi_reg_fields.h>
#include <vi_vreg_blocks.h>
#include <vi_tun_cfg.h>
#include <vi_ctx.h>
#include <vi_common.h>
#include <vi_sys.h>

#ifndef _OFST
#define _OFST(_BLK_T, _REG)       ((uintptr_t)&(((struct _BLK_T *)0)->_REG))
#endif

#define GMS_SEC_SIZE			(((1024 >> 1) << 5) * 3)
/*ae dma size ae_dma_counts + hist_dma_counts + faceae_dma_counts*/
#define AE_DMA_SIZE			(0x21C0 + 0x2000 + 0x80)
/*dci 256 * data_size*/
#define DCI_DMA_SIZE			(0x400)
/*af size = (block_num_x * block_num_y) << 5*/
#define AF_DMA_SIZE			((17 * 15) << 5)

#define ISP_RD_REG_BA(_BA) \
	(_reg_read(_BA))

#define ISP_RD_REG(_BA, _BLK_T, _REG) \
	(_reg_read(_BA+_OFST(_BLK_T, _REG)))

#define ISP_RD_BITS(_BA, _BLK_T, _REG, _FLD) \
	({\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_OFST(_BLK_T, _REG));\
		_r.bits._FLD;\
	})

#define ISP_WR_REG(_BA, _BLK_T, _REG, _V) \
	(_reg_write((_BA+_OFST(_BLK_T, _REG)), _V))

#define ISP_WR_REG_OFT(_BA, _BLK_T, _REG, _OFT, _V) \
	(_reg_write((_BA+_OFST(_BLK_T, _REG) + _OFT), _V))

#define ISP_WR_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_OFST(_BLK_T, _REG));\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define ISP_WO_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = 0;\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define ISP_WR_REGS_BURST(_BA, _BLK_T, _REG, _SIZE, _STR)\
	do {\
		u32 k = 0;\
		uintptr_t ofst = _OFST(_BLK_T, _REG);\
		for (; k < sizeof(_SIZE) / 0x4; k++) {\
			u32 val = (&_STR + k)->raw;\
			_reg_write((_BA + ofst + (k * 0x4)), val);\
		} \
	} while (0)

#define ISP_WR_REG_LOOP_SHFT(_BA, _BLK_T, _REG, _TOTAL_SIZE, _SFT_SIZE, _LUT, _SFT_BIT) \
	do {\
		u16 i = 0, j = 0;\
		u32 val = 0;\
		for (; i < _TOTAL_SIZE / _SFT_SIZE; i++) {\
			val = 0;\
			for (j = 0; j < _SFT_SIZE; j++) {\
				val += (_LUT[(i * _SFT_SIZE) + j] << (_SFT_BIT * j));\
			} \
			_reg_write((_BA + _OFST(_BLK_T, _REG) + (i * 0x4)), val);\
		} \
	} while (0)

#define REG_ARRAY_UPDATE2_SIZE(addr, array, size)		\
	do {							\
		u16 i;					\
		for (i = 0; i < size; i += 2) {			\
			val = array[i];				\
			if ((i + 1) < size)			\
				val |= (array[i+1] << 16);	\
			_reg_write(addr + (i << 1), val);	\
		}						\
	} while (0)

#define REG_ARRAY_UPDATE2(addr, array)				\
	REG_ARRAY_UPDATE2_SIZE(addr, array, ARRAY_SIZE(array))

#define REG_ARRAY_UPDATE4(addr, array)				\
	do {							\
		u16 i;					\
		for (i = 0; i < ARRAY_SIZE(array); i += 4) {	\
			val = array[i];				\
			if ((i + 1) < ARRAY_SIZE(array))	\
				val |= (array[i+1] << 8);	\
			if ((i + 2) < ARRAY_SIZE(array))	\
				val |= (array[i+2] << 16);	\
			if ((i + 3) < ARRAY_SIZE(array))	\
				val |= (array[i+3] << 24);	\
			_reg_write(addr + i, val);		\
		}						\
	} while (0)

#define LTM_REG_ARRAY_UPDATE11(addr, array)                                   \
	do {                                                                  \
		u32 val;                                                 \
		val = array[0] | (array[1] << 5) | (array[2] << 10) |         \
		      (array[3] << 15) | (array[4] << 20) | (array[5] << 25); \
		_reg_write(addr, val);                                        \
		val = array[6] | (array[7] << 5) | (array[8] << 10) |         \
		      (array[9] << 15) | (array[10] << 20);                   \
		_reg_write(addr + 4, val);                                    \
	} while (0)

#define LTM_REG_ARRAY_UPDATE30(addr, array)                                   \
	do {                                                                  \
		u8 i, j;                                                 \
		u32 val;                                                 \
		for (i = 0, j = 0; i < ARRAY_SIZE(array); i += 6, j++) {      \
			val = array[i] | (array[i + 1] << 5) |                \
			      (array[i + 2] << 10) | (array[i + 3] << 15) |   \
			      (array[i + 4] << 20) | (array[i + 5] << 25);    \
			_reg_write(addr + j * 4, val);                        \
		}                                                             \
	} while (0)

enum isp_rgb_prob_out_e {
	ISP_RGB_PROB_OUT_CFA = 0,
	ISP_RGB_PROB_OUT_RGBEE,
	ISP_RGB_PROB_OUT_CCM,
	ISP_RGB_PROB_OUT_GMA,
	ISP_RGB_PROB_OUT_DHZ,
	ISP_RGB_PROB_OUT_HSV,
	ISP_RGB_PROB_OUT_RGBDITHER,
	ISP_RGB_PROB_OUT_CSC,
	ISP_RGB_PROB_OUT_MAX,
};

enum isp_raw_path_e {
	ISP_RAW_PATH_LE = 0,
	ISP_RAW_PATH_SE,
	ISP_RAW_PATH_MAX,
};

enum isp_splt_num_e {
	ISP_SPLT0 = 0,
	ISP_SPLT1,
	ISP_SPLT_MAX,
};

enum isp_splt_ch_num_e {
	ISP_SPLT_CHN0 = 0,
	ISP_SPLT_CHN1,
	ISP_SPLT_CHN_MAX,
};

enum isp_tnr_type_e {
	ISP_TNR_TYPE_BYPASS_MODE = 0,
	ISP_TNR_TYPE_OLD_MODE,
	ISP_TNR_TYPE_NEW_MODE,
	ISP_TNR_TYPE_MAX,
};

/*
 * To indicate the 1st two pixel in the bayer_raw.
 */
enum isp_bayer_type_e {
	ISP_BAYER_TYPE_BG = 0,
	ISP_BAYER_TYPE_GB,
	ISP_BAYER_TYPE_GR,
	ISP_BAYER_TYPE_RG,
	//for RGBIR
	ISP_BAYER_TYPE_GRGBI = 8,
	ISP_BAYER_TYPE_RGBGI = 9,
	ISP_BAYER_TYPE_GBGRI = 10,
	ISP_BAYER_TYPE_BGRGI = 11,
	ISP_BAYER_TYPE_IGRGB = 12,
	ISP_BAYER_TYPE_IRGBG = 13,
	ISP_BAYER_TYPE_IBGRG = 14,
	ISP_BAYER_TYPE_IGBGR = 15,
//	ISP_BAYER_TYPE_MAX,
};

enum isp_bnr_out_e {
	ISP_BNR_OUT_BYPASS = 0,
	ISP_BNR_OUT_B_DELAY,
	ISP_BNR_OUT_FACTOR,
	ISP_BNR_OUT_B_NL,
	ISP_BNR_OUT_RESV_0,
	ISP_BNR_OUT_RESV_1,
	ISP_BNR_OUT_RESV_2,
	ISP_BNR_OUT_RESV_3,
	ISP_BNR_OUT_B_OUT,
	ISP_BNR_OUT_INTENSITY,
	ISP_BNR_OUT_DELTA,
	ISP_BNR_OUT_NOT_SM,
	ISP_BNR_OUT_FLAG_V,
	ISP_BNR_OUT_FLAG_H,
	ISP_BNR_OUT_FLAG_D45,
	ISP_BNR_OUT_FLAG_D135,
	ISP_BNR_OUT_MAX,
};

enum isp_ynr_out_e {
	ISP_YNR_OUT_BYPASS = 0,
	ISP_YNR_OUT_Y_DELAY,
	ISP_YNR_OUT_FACTOR,
	ISP_YNR_OUT_ALPHA,
	ISP_YNR_OUT_Y_BF,
	ISP_YNR_OUT_Y_NL,
	ISP_YNR_OUT_RESV_0,
	ISP_YNR_OUT_RESV_1,
	ISP_YNR_OUT_Y_OUT,
	ISP_YNR_OUT_INTENSITY,
	ISP_YNR_OUT_DELTA,
	ISP_YNR_OUT_NOT_SM,
	ISP_YNR_OUT_FLAG_V,
	ISP_YNR_OUT_FLAG_H,
	ISP_YNR_OUT_FLAG_D45,
	ISP_YNR_OUT_FLAG_D135,
	ISP_YNR_OUT_MAX,
};

enum isp_fs_out_e {
	ISP_FS_OUT_FS = 0,
	ISP_FS_OUT_LONG,
	ISP_FS_OUT_SHORT,
	ISP_FS_OUT_SHORT_EX,
	ISP_FS_OUT_MOTION_PXL,
	ISP_FS_OUT_LE_BLD_WHT,
	ISP_FS_OUT_SE_BLD_WHT,
	ISP_FS_OUT_MOTION_LUT,
	ISP_FS_OUT_AC_FS,
	ISP_FS_OUT_DELTA_LE,
	ISP_FS_OUT_DELTA_SE,
	ISP_FS_OUT_MAX,
};

enum isp_yuv_scene_e {
	ISP_YUV_SCENE_BYPASS = 0,
	ISP_YUV_SCENE_ONLINE,
	ISP_YUV_SCENE_ISP,
	ISP_YUV_SCENE_MAX,
};

enum raw_ai_isp_ap_e {
	RAW_AI_ISP_BYPASS = 0,
	RAW_AI_ISP_SPLT,
	RAW_AI_ISP_FE,
	RAW_AI_ISP_MAX,
};

enum isp_blc_id_e {
	ISP_BLC_ID_FE0_LE = 0,
	ISP_BLC_ID_FE0_SE,
	ISP_BLC_ID_FE1_LE,
	ISP_BLC_ID_FE1_SE,
	ISP_BLC_ID_FE2_LE,
	ISP_BLC_ID_FE2_SE,
	ISP_BLC_ID_FE3_LE,
	ISP_BLC_ID_FE3_SE,
	ISP_BLC_ID_FE4_LE,
	ISP_BLC_ID_FE4_SE,
	ISP_BLC_ID_FE5_LE,
	ISP_BLC_ID_FE5_SE,
	ISP_BLC_ID_BE_LE,
	ISP_BLC_ID_BE_SE,
	ISP_BLC_ID_MAX,
};

enum isp_wbg_id_e {
	ISP_WBG_ID_FE0_LE = 0,
	ISP_WBG_ID_FE0_SE,
	ISP_WBG_ID_FE1_LE,
	ISP_WBG_ID_FE1_SE,
	ISP_WBG_ID_FE2_LE,
	ISP_WBG_ID_FE2_SE,
	ISP_WBG_ID_FE3_LE,
	ISP_WBG_ID_FE3_SE,
	ISP_WBG_ID_FE4_LE,
	ISP_WBG_ID_FE4_SE,
	ISP_WBG_ID_FE5_LE,
	ISP_WBG_ID_FE5_SE,
	ISP_WBG_ID_RAW_TOP_LE,
	ISP_WBG_ID_RAW_TOP_SE,
	ISP_WBG_ID_MAX,
};

enum isp_rgbmap_id_e {
	ISP_RGBMAP_ID_FE0_LE = 0,
	ISP_RGBMAP_ID_FE0_SE,
	ISP_RGBMAP_ID_FE1_LE,
	ISP_RGBMAP_ID_FE1_SE,
	ISP_RGBMAP_ID_FE2_LE,
	ISP_RGBMAP_ID_FE2_SE,
	ISP_RGBMAP_ID_FE3_LE,
	ISP_RGBMAP_ID_FE3_SE,
	ISP_RGBMAP_ID_FE4_LE,
	ISP_RGBMAP_ID_FE4_SE,
	ISP_RGBMAP_ID_FE5_LE,
	ISP_RGBMAP_ID_FE5_SE,
	ISP_RGBMAP_ID_MAX,
};

enum isp_lscr_id_e {
	ISP_LSCR_ID_PRE0_FE_LE = 0,
	ISP_LSCR_ID_PRE0_FE_SE,
	ISP_LSCR_ID_PRE1_FE_LE,
	ISP_LSCR_ID_PRE1_FE_SE,
	ISP_LSCR_ID_PRE_BE_LE,
	ISP_LSCR_ID_PRE_BE_SE,
	ISP_LSCR_ID_MAX,
};

enum isp_pre_proc_id_e {
	ISP_IR_PRE_PROC_ID_LE,
	ISP_IR_PRE_PROC_ID_SE,
	ISP_IR_PRE_PROC_ID_MAX,
};

enum isp_dma_id_e {
	// wdma0 dma_group=0
	ISP_DMA_ID_CSI0_BDG0 = 0,
	ISP_DMA_ID_CSI0_BDG1,
	ISP_DMA_ID_CSI0_BDG2,
	ISP_DMA_ID_CSI0_BDG3,
	ISP_DMA_ID_CSI1_BDG0,
	ISP_DMA_ID_CSI1_BDG1,
	ISP_DMA_ID_CSI1_BDG2,
	ISP_DMA_ID_CSI1_BDG3,
	ISP_DMA_ID_CSI2_BDG0,
	ISP_DMA_ID_CSI2_BDG1,
	ISP_DMA_ID_CSI3_BDG0,
	ISP_DMA_ID_CSI3_BDG1,
	ISP_DMA_ID_CSI4_BDG0,
	ISP_DMA_ID_CSI4_BDG1,
	ISP_DMA_ID_CSI5_BDG0,
	ISP_DMA_ID_CSI5_BDG1,
	// wdma1 dma_group=1
	ISP_DMA_ID_BT0_LITE0 = 0,
	ISP_DMA_ID_BT0_LITE1,
	ISP_DMA_ID_BT0_LITE2,
	ISP_DMA_ID_BT0_LITE3,
	ISP_DMA_ID_BT1_LITE0,
	ISP_DMA_ID_BT1_LITE1,
	ISP_DMA_ID_BT1_LITE2,
	ISP_DMA_ID_BT1_LITE3,
	ISP_DMA_ID_RGBIR_LE,
	ISP_DMA_ID_RGBIR_SE,
	ISP_DMA_ID_PRE_RAW_BE_LE,
	ISP_DMA_ID_PRE_RAW_BE_SE,
	ISP_DMA_ID_AF_W,
	// wdma2 dma_group=2
	ISP_DMA_ID_FE0_RGBMAP_LE = 0,
	ISP_DMA_ID_FE0_RGBMAP_SE,
	ISP_DMA_ID_FE1_RGBMAP_LE,
	ISP_DMA_ID_FE1_RGBMAP_SE,
	ISP_DMA_ID_FE2_RGBMAP_LE,
	ISP_DMA_ID_FE2_RGBMAP_SE,
	ISP_DMA_ID_FE3_RGBMAP_LE,
	ISP_DMA_ID_FE3_RGBMAP_SE,
	ISP_DMA_ID_FE4_RGBMAP_LE,
	ISP_DMA_ID_FE4_RGBMAP_SE,
	ISP_DMA_ID_FE5_RGBMAP_LE,
	ISP_DMA_ID_FE5_RGBMAP_SE,
	ISP_DMA_ID_AE_HIST_LE,
	ISP_DMA_ID_AE_HIST_SE,
	ISP_DMA_ID_LMAP_LE,
	ISP_DMA_ID_LMAP_SE,
	// wdma3 dma_group=3
	ISP_DMA_ID_GMS = 0,
	ISP_DMA_ID_DCI,
	ISP_DMA_ID_HIST_EDGE_V,
	ISP_DMA_ID_LDCI_W,
	ISP_DMA_ID_YUV_CROP_Y,
	ISP_DMA_ID_YUV_CROP_C,
	ISP_DMA_ID_TNR_LD_Y,
	ISP_DMA_ID_TNR_LD_C,
	ISP_DMA_ID_TNR_LD_MO,
	ISP_DMA_ID_MMAP_IIR_W,
	ISP_DMA_ID_SPLT_FE0_WDMA_LE,
	ISP_DMA_ID_SPLT_FE0_WDMA_SE,
	ISP_DMA_ID_SPLT_FE1_WDMA_LE,
	ISP_DMA_ID_SPLT_FE1_WDMA_SE,
	// rdma0 dma_group=4
	ISP_DMA_ID_PRE_RAW_VI_SEL_LE = 0,
	ISP_DMA_ID_RAW_RDMA0,
	ISP_DMA_ID_RAW_RDMA1,
	ISP_DMA_ID_LSC_LE,
	ISP_DMA_ID_MMAP_AI_ISP,
	ISP_DMA_ID_MMAP_PRE_LE_R,
	ISP_DMA_ID_MMAP_PRE_SE_R,
	ISP_DMA_ID_MMAP_CUR_LE_R,
	ISP_DMA_ID_MMAP_CUR_SE_R,
	ISP_DMA_ID_MMAP_IIR_R,
	ISP_DMA_ID_TNR_ST_Y,
	ISP_DMA_ID_TNR_ST_C,
	ISP_DMA_ID_TNR_ST_MO,
	ISP_DMA_ID_LTM_LE,
	ISP_DMA_ID_LTM_SE,
	ISP_DMA_ID_PRE_RAW_VI_SEL_SE,
	// rdma1 dma_group=5
	ISP_DMA_ID_AI_ISP_RDMA_Y = 0,
	ISP_DMA_ID_AI_ISP_RDMA_U,
	ISP_DMA_ID_AI_ISP_RDMA_V,
	ISP_DMA_ID_LDCI_R,
	ISP_DMA_ID_SPLT_FE0_RDMA_LE,
	ISP_DMA_ID_SPLT_FE0_RDMA_SE,
	ISP_DMA_ID_SPLT_FE1_RDMA_LE,
	ISP_DMA_ID_SPLT_FE1_RDMA_SE,

	ISP_DMA_ID_MAX,
};

enum isp_rgbmap_chg_e {
	ISP_RGBMAP_CHG_IDLE = 0,
	ISP_RGBMAP_CHG_T0,
	ISP_RGBMAP_CHG_T1,
	ISP_RGBMAP_CHG_T2,
	ISP_RGBMAP_CHG_MAX
};

struct lmap_cfg {
	u8 pre_chg[2]; //le/se
	u8 pre_w_bit;
	u8 pre_h_bit;
	u8 post_w_bit;
	u8 post_h_bit;
};

struct isp_dump_info {
	u64 phy_base;
	u64 reg_base;
	u32 blk_size;
};

struct isp_cmdq_buf {
	u64 phy_addr;
	void *vir_addr;
	u32 buf_size;
	u16 cmd_idx;
};

struct isp_vblock_info {
	u32 block_id;
	u32 block_size;
	u64 reg_base;
};

struct tile {
	u16 start;
	u16 end;
};

struct isp_ccm_cfg {
	u16 coef[3][3];
};

struct _fe_dbg_i {
	u32		fe_idle_sts;
	u32		fe_done_sts;
};

struct _be_dbg_i {
	u32		be_done_sts;
	u32		be_dma_idle_sts;
};

struct _post_dbg_i {
	u32		top_sts_0;
	u32		top_sts_1;
};

struct _dma_dbg_i {
	u32		wdma_0_err_sts;
	u32		wdma_0_idle;
	u32		wdma_1_err_sts;
	u32		wdma_1_idle;
	u32		wdma_2_err_sts;
	u32		wdma_2_idle;
	u32		wdma_3_err_sts;
	u32		wdma_3_idle;
	u32		rdma_0_err_sts;
	u32		rdma_0_idle;
	u32		rdma_1_err_sts;
	u32		rdma_1_idle;
};

struct _csibdg_dbg_i {
	u32		dbg_0;
	u32		dbg_1;
	u32		dbg_2;
	u32		dbg_3;
};

struct _isp_dg_info {
	struct _fe_dbg_i	fe_sts;
	struct _be_dbg_i	be_sts;
	struct _post_dbg_i	post_sts;
	struct _dma_dbg_i	dma_sts;
	struct _csibdg_dbg_i	bdg_chn_debug[ISP_FE_CHN_MAX];
	u32			bdg_int_sts_0;
	u32			bdg_int_sts_1;
	u32			bdg_fifo_of_cnt;
	u8			bdg_w_gt_cnt[ISP_FE_CHN_MAX];
	u8			bdg_w_ls_cnt[ISP_FE_CHN_MAX];
	u8			bdg_h_gt_cnt[ISP_FE_CHN_MAX];
	u8			bdg_h_ls_cnt[ISP_FE_CHN_MAX];
};

struct isp_grid_s_info {
	u8 w_bit;
	u8 h_bit;
};

struct _isp_cfg {
	u32			csibdg_width;
	u32			csibdg_height;
	u32			max_width;
	u32			max_height;
	u32			post_img_w;
	u32			post_img_h;
	u32			drop_ref_frm_num;
	u32			drop_frm_cnt;
	u32			isp_reset_frm;
	u32			first_frm_cnt;
	u32			raw_ai_isp_frm_cnt;
	u32			raw_ai_isp_ap;
	struct vi_rect		crop;
	struct vi_rect		crop_se;
	struct vi_rect		rawdump_crop;
	struct vi_rect		rawdump_crop_se;
	struct vi_rect		postout_crop;
	struct _isp_dg_info	dg_info;
	struct isp_grid_s_info	rgbmap_i;
	struct isp_grid_s_info	lmap_i;
	enum isp_bayer_type_e	rgb_color_mode;
	enum isp_yuv_scene_e	yuv_scene_mode;
	enum isp_tnr_type_e	tnr_mode;
	enum isp_rgbmap_chg_e	rgbmap_chg_state;
	enum _vi_intf_mode_e	inf_mode;
	enum _vi_work_mode_e	mux_mode;
	enum _vi_yuv_data_seq_e data_seq;
	struct isp_cmdq_buf	cmdq_buf;
	struct timespec64	ts;

	u32			is_patgen_en		: 1;
	u32			is_raw_replay_fe	: 1;
	u32			is_raw_replay_be	: 1;
	u32			is_yuv_sensor		: 1;
	u32			is_422_to_420		: 1;
	u32			is_bt_demux		: 1;
	u32			is_hdr_on		: 1;
	u32			is_fbc_on		: 1;
	u32			is_rgbir_sensor		: 1;
	u32			is_offline_scaler	: 1;
	u32			is_stagger_vsync	: 1;
	u32			is_slice_buf_on		: 1;
	u32			is_drop_next_frame	: 1;
	u32			is_ctrl_inited		: 1;
	u32			is_fake_splt_wdma	: 1;
	u32			is_tnr_ai_isp		: 1;
	u32			is_tnr_ai_isp_rdy	: 1;
	u32			is_tile			: 1;
	u32			is_work_on_r_tile	: 1;
	u32			is_postout_crop		: 1;
	u32			is_frist_frm_done	: 1;
};

struct _isp_bind_info {
	bool			is_bind;
	u8			bind_dev_num;
	u8			bind_fe_num;
	u8			bind_mipi_dev;
	u8			hw_chn;
};
/*
 * @src_width: width of original image
 * @src_height: height of original image
 * @img_width: width of image after crop
 * @img_height: height of image after crop
 * @pyhs_regs: index by enum isp_blk_id_t, always phys reg
 * @vreg_bases: index by enum isp_blk_id_t
 * @vreg_bases_pa: index by enum isp_blk_id_t
 *
 * @rgb_color_mode: bayer_raw type after crop could change
 *
 * @cam_id: preraw(0,1)
 * @is_offline_preraw: preraw src offline(from dram)
 * @is_offline_postraw: postraw src offline(from dram)
 */
struct isp_ctx {
	u32			src_width;
	u32			src_height;
	u32			img_width;
	u32			img_height;
	u32			crop_x;
	u32			crop_y;
	u32			crop_se_x;
	u32			crop_se_y;
	struct _tile_cfg {
		struct tile l_in;
		struct tile l_out;
		struct tile r_in;
		struct tile r_out;
	} tile_cfg;

	uintptr_t		*phys_regs;

	struct _isp_bind_info	isp_bind_info[VI_MAX_PIPE_NUM];
	struct _isp_cfg		isp_pipe_cfg[ISP_PRERAW_MAX];
	atomic_t			isp_vitural_num[ISP_PRERAW_MAX];
	u8			isp_pipe_enable[VI_MAX_PIPE_NUM];
	u8			isp_pipe_offline_sc[VI_MAX_PIPE_NUM];
	enum isp_bayer_type_e	rgb_color_mode[ISP_PRERAW_MAX];
	u8			rgbmap_prebuf_idx;
	u8			mmap_grid_size[ISP_PRERAW_MAX];
	u8			raw_chnstr_num[VI_MAX_PIPE_NUM];
	u8			total_chn_num;
	u8			dev_num;
	atomic_t		is_post_done;

	u8			cam_id;
	u8			gamma_tbl_idx;
	u32			is_multi_sensor     : 1;
	u32			is_hdr_on           : 1;
	u32			is_3dnr_on          : 1;
	u32			is_dpcm_on          : 1;
	u32			is_offline_be       : 1;
	u32			is_offline_postraw  : 1;
	u32			is_tile             : 1;
	u32			is_work_on_r_tile   : 1;
	u32			is_sublvds_path     : 1;
	u32			is_fbc_on           : 1;
	u32			is_ctrl_inited      : 1;
	u32			is_slice_buf_on     : 1;
	u32			is_rgbmap_sbm_on    : 1;
	u32			is_3dnr_old2new     : 1;
	u32			is_suspend          : 1;
};

struct vi_fbc_cfg {
	u8	cu_size;
	u8	target_cr; //compression ratio
	u8	is_lossless; // lossless or lossy
	u32	y_bs_size; //Y WDMA seglen
	u32	c_bs_size; //C WDMA seglen
	u32	y_buf_size; //total Y buf size
	u32	c_buf_size; //total C buf size
};

struct slc_cfg_s {
	u32 le_buf_size;
	u32 se_buf_size;
	u32 le_w_thshd;
	u32 se_w_thshd;
	u32 le_r_thshd;
	u32 se_r_thshd;
};

struct slice_buf_s {
	u16 line_delay; //sensor exposure ratio
	u16 buffer; //cover for read/write latency, axi latency..etc
	u8  main_max_grid_size; //main path rgbmap grid size
	u8  sub_max_grid_size; //sub path rgbmap grid size
	u8  min_r_thshd; // minimum read threshold
	struct slc_cfg_s main_path;
	struct slc_cfg_s sub_path;
};

extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_MAX];
extern struct vi_fbc_cfg fbc_cfg;
extern u8 g_rgbmap_chg_pre[ISP_PRERAW_MAX][2];
extern u8 g_w_bit[ISP_PRERAW_MAX], g_h_bit[ISP_PRERAW_MAX];

extern void sys_set_reset_vi(union vi_sys_reset reset);
extern union vi_sys_reset sys_get_reset_vi(void);
extern int32_t vi_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len);
extern int32_t vi_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len);

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_be_online(struct isp_ctx *ctx);
u32 _is_be_post_online(struct isp_ctx *ctx);
u32 _is_all_online(struct isp_ctx *ctx);
u32 _is_post_sclr_online(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
u32 _is_right_tile(struct isp_ctx *ctx, enum sop_isp_raw raw_num);

/****************************************************************************
 * Interfaces
 ****************************************************************************/
int vi_get_dev_num_by_raw(struct isp_ctx *ctx, u8 raw_num);
int vi_get_raw_num_by_dev(struct isp_ctx *ctx, u8 dev_num);
int vi_get_first_raw_num(struct isp_ctx *ctx);

void vi_set_base_addr(void *base);
uintptr_t *isp_get_phys_reg_bases(void);
void isp_debug_dump(struct isp_ctx *ctx);
/**
 * isp_init - setup isp
 *
 * @param :
 */
void isp_init(struct isp_ctx *ctx);


/**
 * isp_reset - do reset. This can be activated only if dma stop to avoid
 * hang fabric.
 *
 */
void isp_reset(struct isp_ctx *ctx);

/**
 * isp_stream_on - start/stop isp stream.
 *
 * @param on: 1 for stream start, 0 for stream stop
 */
void isp_streaming(struct isp_ctx *ctx, u32 on, enum sop_isp_raw raw_num);


struct isp_grid_s_info ispblk_rgbmap_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
struct isp_grid_s_info ispblk_lmap_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_splt_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool enable);
void ispblk_splt_wdma_ctrl_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool enable);
void ispblk_splt_rdma_ctrl_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool enable);
void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_preraw_vi_sel_config(struct isp_ctx *ctx);
void ispblk_pre_wdma_ctrl_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_preraw_be_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_raw_rdma_ctrl_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num,
					enum isp_blk_id_t blk_id, bool enable);
void ispblk_rawtop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_rgbtop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_yuvtop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_isptop_config(struct isp_ctx *ctx);

void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en);
int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vi_rect crop);

int bayer_type_mapping(enum isp_bayer_type_e bayer_type);
int csibdg_find_hwid(enum sop_isp_raw raw_num);
int fe_find_hwid(enum sop_isp_raw raw_num);
int blc_find_hwid(int id);
int rgbmap_find_hwid(int id);
int wbg_find_hwid(int id);
int csibdg_lite_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num);
int csibdg_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num);
int rgbmap_dma_find_hwid(enum sop_isp_raw raw_num, enum isp_raw_path_e path);

void ispblk_blc_set_offset(struct isp_ctx *ctx, int blc_id,
				u16 roffset, u16 groffset,
				u16 gboffset, u16 boffset);
void ispblk_blc_set_2ndoffset(struct isp_ctx *ctx, int blc_id,
				u16 roffset, u16 groffset,
				u16 gboffset, u16 boffset);
void ispblk_blc_set_gain(struct isp_ctx *ctx, int blc_id,
				u16 rgain, u16 grgain,
				u16 gbgain, u16 bgain);
void ispblk_blc_enable(struct isp_ctx *ctx, int blc_id, bool en, bool bypass);
int ispblk_wbg_config(struct isp_ctx *ctx, int wbg_id, u16 rgain, u16 ggain, u16 bgain);
int ispblk_wbg_enable(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass);
void ispblk_lscr_set_lut(struct isp_ctx *ctx, int lscr_id, u16 *gain_lut, u8 lut_count);
void ispblk_lscr_config(struct isp_ctx *ctx, int lscr_id, bool en);
void ispblk_rgbmap_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid);
void ispblk_rgbmap_dma_mode(struct isp_ctx *ctx, u32 dmaid);
u64 ispblk_dma_getaddr(struct isp_ctx *ctx, u32 dmaid);
int ispblk_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid, u64 buf_addr);
void ispblk_dma_setaddr(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr);
void ispblk_dma_enable(struct isp_ctx *ctx, u32 dmaid, u32 on, u8 dma_disable);
int ispblk_dma_buf_get_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid);
void ispblk_dma_set_sw_mode(struct isp_ctx *ctx, u32 dmaid, bool is_sw_mode);

/****************************************************************************
 *	PRERAW FE SUBSYS
 ****************************************************************************/
void ispblk_csidbg_dma_wr_en(
	struct isp_ctx *ctx,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num,
	const u8 en);
void ispblk_csibdg_wdma_crop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, struct vi_rect crop, u8 en);
void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool en);
int ispblk_csibdg_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
int ispblk_rgbmap_config(struct isp_ctx *ctx, int map_id, bool en);
void ispblk_lmap_chg_size(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, const enum sop_isp_fe_chn_num chn_num);
void ispblk_lmap_config(struct isp_ctx *ctx, int map_id, bool en);

/****************************************************************************
 *	PRE BE SUBSYS
 ****************************************************************************/
void ispblk_rgbir_config(struct isp_ctx *ctx, enum isp_raw_path_e path, bool enable);
void ispblk_dpc_config(struct isp_ctx *ctx, enum isp_raw_path_e path, bool enable, u8 test_case);
void ispblk_dpc_set_static(struct isp_ctx *ctx, enum isp_raw_path_e path,
			   u16 offset, u32 *bps, u16 count);
void ispblk_af_config(struct isp_ctx *ctx, bool enable);

/****************************************************************************
 *	RAW TOP SUBSYS
 ****************************************************************************/
void ispblk_bnr_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, enum isp_bnr_out_e out_sel,
		       bool lsc_en, u8 ns_gain, u8 str);
void ispblk_cfa_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id);
void ispblk_aehist_reset(struct isp_ctx *ctx, int blk_id, enum sop_isp_raw raw_num);
void ispblk_aehist_config(struct isp_ctx *ctx, int blk_id, bool enable);
void ispblk_gms_config(struct isp_ctx *ctx, bool enable);
void ispblk_rgbcac_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, u8 test_case);
void ispblk_lcac_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, u8 test_case);

/****************************************************************************
 *	RGB TOP SUBSYS
 ****************************************************************************/
void ispblk_lsc_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en);
void ispblk_fusion_hdr_cfg(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_fusion_config(struct isp_ctx *ctx, bool enable, bool mc_enable, enum isp_fs_out_e out_sel);
void ispblk_ltm_d_lut(struct isp_ctx *ctx, u8 sel, u16 *data);
void ispblk_ltm_b_lut(struct isp_ctx *ctx, u8 sel, u16 *data);
void ispblk_ltm_g_lut(struct isp_ctx *ctx, u8 sel, u16 *data);
void ispblk_ltm_config(struct isp_ctx *ctx, u8 ltm_en, u8 dehn_en, u8 behn_en, u8 ee_en);
void ispblk_ccm_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, struct isp_ccm_cfg *cfg);
void ispblk_hist_v_config(struct isp_ctx *ctx, bool en, u8 test_case);
void ispblk_dhz_config(struct isp_ctx *ctx, bool en);
void ispblk_ygamma_config(struct isp_ctx *ctx, bool en,
				u8 sel, u16 *data, u8 inv, u8 test_case);
void ispblk_ygamma_enable(struct isp_ctx *ctx, bool enable);
void ispblk_gamma_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data, u8 inv);
void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable);
void ispblk_clut_config(struct isp_ctx *ctx, bool en,
				int16_t *r_lut, int16_t *g_lut, int16_t *b_lut);
void ispblk_clut_cmdq_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, bool en,
			int16_t *r_lut, int16_t *g_lut, int16_t *b_lut);
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en, bool histidx_en, bool fmnum_en);
void ispblk_csc_config(struct isp_ctx *ctx);
void ispblk_manr_config(struct isp_ctx *ctx, bool en);

/****************************************************************************
 *	YUV TOP SUBSYS
 ****************************************************************************/
int ispblk_pre_ee_config(struct isp_ctx *ctx, bool en);
int ispblk_yuvdither_config(struct isp_ctx *ctx, u8 sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en);
void ispblk_tnr_config(struct isp_ctx *ctx, bool en, u8 test_case);
void ispblk_fbc_clear_fbcd_ring_base(struct isp_ctx *ctx, u8 raw_num);
void ispblk_fbc_chg_to_sw_mode(struct isp_ctx *ctx, u8 raw_num);
void vi_fbc_calculate_size(struct isp_ctx *ctx, u8 raw_num);
void ispblk_fbc_ring_buf_config(struct isp_ctx *ctx, u8 en);
void ispblk_fbcd_config(struct isp_ctx *ctx, bool en);
void ispblk_fbce_config(struct isp_ctx *ctx, bool en);
void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, u8 str_mode, u8 test_case);
void ispblk_ynr_config(struct isp_ctx *ctx, enum isp_ynr_out_e out_sel, u8 ns_gain);
int ispblk_ee_config(struct isp_ctx *ctx, bool en);
void ispblk_dci_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *lut, u8 test_case);
void ispblk_ldci_config(struct isp_ctx *ctx, bool en, u8 test_case);
void ispblk_ca_config(struct isp_ctx *ctx, bool en, u8 mode);
void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en);
void ispblk_ycur_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data);
void ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, u8 sel);

void isp_splt_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void isp_pre_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num, const u8 chn_num);
void isp_post_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num);

void isp_intr_set_mask(struct isp_ctx *ctx);
void isp_intr_status(
	struct isp_ctx *ctx,
	union reg_isp_top_int_event0 *s0,
	union reg_isp_top_int_event1 *s1,
	union reg_isp_top_int_event2 *s2,
	union reg_isp_top_int_event0_fe345 *s0_fe345,
	union reg_isp_top_int_event1_fe345 *s1_fe345,
	union reg_isp_top_int_event2_fe345 *s2_fe345);
void isp_csi_intr_status(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	union reg_isp_csi_bdg_interrupt_status_0 *s0,
	union reg_isp_csi_bdg_interrupt_status_1 *s1);

void ispblk_tnr_rgbmap_chg(struct isp_ctx *ctx, enum sop_isp_raw raw_num, const u8 chn_num);
void ispblk_tnr_post_chg(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_mmap_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, int dmaid);
void ispblk_mmap_dma_mode(struct isp_ctx *ctx, u32 dmaid);

/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/
void isp_first_frm_reset(struct isp_ctx *ctx, u8 reset);
void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_post_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_pre_be_cfg_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, u32 _w, u32 _h);
struct _csibdg_dbg_i ispblk_csibdg_chn_dbg(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	enum sop_isp_fe_chn_num chn_num);
struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
struct _be_dbg_i ispblk_be_dbg_info(struct isp_ctx *ctx);
struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx);
struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx);
int isp_frm_err_handler(struct isp_ctx *ctx, const enum sop_isp_raw err_raw_num, const u8 step);

/****************************************************************************
 *	YUV Bypass Control Flow Config
 ****************************************************************************/
void ispblk_csibdg_lite_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr,
				 const enum sop_isp_raw raw_num);

/****************************************************************************
 *	Slice buffer Control
 ****************************************************************************/
void vi_calculate_slice_buf_setting(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void isp_slice_buf_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void manr_clear_prv_ring_base(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_slice_buf_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, u8 en);

/*******************************************************************************
 *	Tuning interfaces
 ******************************************************************************/
void vi_tuning_gamma_ips_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
void vi_tuning_dci_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
void vi_tuning_drc_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
void vi_tuning_clut_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
int vi_tuning_get_clut_tbl_idx(enum sop_isp_raw raw_num, int tun_idx);
int vi_tuning_sw_init(void);
int vi_tuning_buf_setup(struct isp_ctx *ctx);
void *vi_get_tuning_buf_addr(u32 *size);
void vi_tuning_buf_release(void);
void vi_tuning_buf_clear(void);

/*******************************************************************************
 *	Tuning modules update
 ******************************************************************************/
void pre_fe_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
void pre_be_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
void postraw_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);

void ispblk_blc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_blc_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_wbg_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_wbg_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_rgbir_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_rgbir_config *cfg,
	const enum sop_isp_raw raw_num);
/****************************************************************************
 *	Postraw Tuning Config
 ****************************************************************************/
void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ccm_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cacp_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ca2_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ygamma_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ygamma_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_gamma_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_demosiac_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lsc_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_bnr_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_clut_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_drc_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ynr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ynr_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cnr_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_tnr_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ee_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_pre_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_pre_ee_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_fswdr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_fswdr_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_fswdr_update_rpt(
	struct isp_ctx *ctx,
	struct sop_vip_isp_fswdr_report *cfg);
void ispblk_ldci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ldci_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ycur_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dci_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_dhz_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dhz_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_rgbcac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_rgbcac_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_cac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cac_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_lcac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lcac_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_csc_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dpc_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ae_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ge_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_af_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_hist_v_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_hist_v_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_gms_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_gms_config *cfg,
	const enum sop_isp_raw raw_num);
void ispblk_mono_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_mono_config *cfg,
	const enum sop_isp_raw raw_num);

/****************************************************************************
 *	Pre Be Tuning Config
 ****************************************************************************/
//void ispblk_lscr_tun_cfg(struct isp_ctx *ctx,
//	struct sop_vip_isp_lscr_config *cfg,
//	const enum sop_isp_raw raw_num);
//void ispblk_preproc_tun_cfg(
//	struct isp_ctx *ctx,
//	struct sop_vip_isp_preproc_config *cfg,
//	const enum sop_isp_raw raw_num);

#ifdef PORTING_TEST
void vi_ip_test_cases_init(struct isp_ctx *ctx);
void vi_ip_test_cases_uninit(struct isp_ctx *ctx);
void ispblk_patgen_config_pat(struct isp_ctx *ctx, enum sop_isp_raw raw_num, u8 test_case);
void ispblk_isptop_fpga_config(struct isp_ctx *ctx, u16 test_case);
void ispblk_dci_restore_default_config(struct isp_ctx *ctx, bool en);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VI_DRV_H__ */
