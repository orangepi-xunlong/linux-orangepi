#ifndef _CIF_H_
#define _CIF_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <media/media-entity.h>
#include <media/v4l2-mc.h>
#include <linux/videodev2.h>
#include <linux/irq.h>
#include <linux/reset.h>
#include <generated/compile.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/defines.h>
#ifdef __CV181X__
#include "pinctrl-cv181x.h"
#elif defined(__CV180X__)
#include "pinctrl-cv180x.h"
#endif
#include <linux/ctype.h>
#include <linux/version.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#endif

//#include "linux/vi_snsr.h"
#include "drv/cif_drv.h"
#include <linux/miscdevice.h>
//#include <vip_common.h>
//#include <base_cb.h>
//#include <cif_cb.h>

#define CIF_MAX_CSI_NUM		8


#define MIPI_LANE_NUM	8
#define WDR_VC_NUM	2
#define SYNC_CODE_NUM	4
#define BT_DEMUX_NUM	4
#define MIPI_DEMUX_NUM	4
#define CIF_PAD_MAX 2
#define CIF_SRC_PADS 4
#define CIF_EVENT_ELEMS 4
#define MAX_PAD_NUM			28

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#define VIP_SYS_ADDRESS 0x680BE000//vip_sys_address
//MAC_CLK_DIV_VAL
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC0   0x44
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC0_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC0_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC0_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC1   0x48
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC1_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC1_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC1_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC2   0x4c
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC2_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC2_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC2_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC3   0x50
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC3_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC3_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC3_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC4   0x54
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC4_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC4_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC4_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC5   0x58
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC5_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC5_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC5_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC6   0x5c
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC6_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC6_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC6_BITS   0x5
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC7   0x60
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC7_OFFSET 16
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC7_MASK   0x1f0000
#define  MAC_CLK_NORM_DIV_VAL_CSI_MAC7_BITS   0x5
//MAC_CLK_ENABLE
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC0   0x38
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC0_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC0_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC0_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC1   0x40
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC1_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC1_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC1_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC2   0x4c
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC2_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC2_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC2_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC3   0x50
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC3_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC3_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC3_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC4   0x54
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC4_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC4_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC4_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC5   0x58
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC5_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC5_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC5_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC6   0x5c
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC6_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC6_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC6_BITS   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC7   0x60
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC7_OFFSET 0
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC7_MASK   0x1
#define  MAC_CLK_NORM_DIV_EN_CSI_MAC7_BITS   0x1
//MAC_CLK_UPDATE
#define  MAC_CLK_UPDATE_SEL_CSI_MAC0   0x38
#define  MAC_CLK_UPDATE_SEL_CSI_MAC0_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC0_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC0_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC1   0x40
#define  MAC_CLK_UPDATE_SEL_CSI_MAC1_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC1_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC1_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC2   0x4c
#define  MAC_CLK_UPDATE_SEL_CSI_MAC2_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC2_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC2_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC3   0x50
#define  MAC_CLK_UPDATE_SEL_CSI_MAC3_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC3_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC3_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC4   0x54
#define  MAC_CLK_UPDATE_SEL_CSI_MAC4_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC4_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC4_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC5   0x58
#define  MAC_CLK_UPDATE_SEL_CSI_MAC5_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC5_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC5_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC6   0x5c
#define  MAC_CLK_UPDATE_SEL_CSI_MAC6_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC6_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC6_BITS   0x1
#define  MAC_CLK_UPDATE_SEL_CSI_MAC7   0x60
#define  MAC_CLK_UPDATE_SEL_CSI_MAC7_OFFSET 2
#define  MAC_CLK_UPDATE_SEL_CSI_MAC7_MASK   0x4
#define  MAC_CLK_UPDATE_SEL_CSI_MAC7_BITS   0x1
//MAC CLK SRC
#define  MAC_CLK_CLK_CSI_MAC0_SRC_SEL   0x24
#define  MAC_CLK_CLK_CSI_MAC0_SRC_SEL_OFFSET 0
#define  MAC_CLK_CLK_CSI_MAC0_SRC_SEL_MASK   0x3
#define  MAC_CLK_CLK_CSI_MAC0_SRC_SEL_BITS   0x2
#define  MAC_CLK_CLK_CSI_MAC1_SRC_SEL   0x24
#define  MAC_CLK_CLK_CSI_MAC1_SRC_SEL_OFFSET 2
#define  MAC_CLK_CLK_CSI_MAC1_SRC_SEL_MASK   0xc
#define  MAC_CLK_CLK_CSI_MAC1_SRC_SEL_BITS   0x2
#define  MAC_CLK_CLK_CSI_MAC2_SRC_SEL   0x24
#define  MAC_CLK_CLK_CSI_MAC2_SRC_SEL_OFFSET 4
#define  MAC_CLK_CLK_CSI_MAC2_SRC_SEL_MASK   0x30
#define  MAC_CLK_CLK_CSI_MAC2_SRC_SEL_BITS   0x2
#define  MAC_CLK_CLK_CSI_MAC3_SRC_SEL   0x24
#define  MAC_CLK_CLK_CSI_MAC3_SRC_SEL_OFFSET 6
#define  MAC_CLK_CLK_CSI_MAC3_SRC_SEL_MASK   0xc0
#define  MAC_CLK_CLK_CSI_MAC3_SRC_SEL_BITS   0x2
#define  MAC_CLK_CLK_CSI_MAC4_SRC_SEL   0x24
#define  MAC_CLK_CLK_CSI_MAC4_SRC_SEL_OFFSET 8
#define  MAC_CLK_CLK_CSI_MAC4_SRC_SEL_MASK   0x300
#define  MAC_CLK_CLK_CSI_MAC4_SRC_SEL_BITS   0x2
#define  MAC_CLK_CLK_CSI_MAC5_SRC_SEL   0x24
#define  MAC_CLK_CLK_CSI_MAC5_SRC_SEL_OFFSET 10
#define  MAC_CLK_CLK_CSI_MAC5_SRC_SEL_MASK   0xc00
#define  MAC_CLK_CLK_CSI_MAC5_SRC_SEL_BITS   0x2

#define SET_BIT(data, bit, value) ((data) = (((data) & ~(1U << (bit))) | ((value) << (bit))))
#define MAC_NORM_CLK_RATIO_MASK(CLK_NAME) MAC_CLK_NORM_DIV_##CLK_NAME##_MASK
#define MAC_NORM_CLK_RATIO_OFFSET(CLK_NAME) MAC_CLK_NORM_DIV_##CLK_NAME##_OFFSET
#define MAC_NORM_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
		cif_ops_reg_write_mask(MAC_CLK_NORM_DIV_##CLK_NAME + VIP_SYS_ADDRESS, \
			MAC_NORM_CLK_RATIO_MASK(CLK_NAME), \
			RATIO << MAC_NORM_CLK_RATIO_OFFSET(CLK_NAME))

#define MAC_UPDATE_CLK_RATIO_MASK(CLK_NAME) MAC_CLK_UPDATE_##CLK_NAME##_MASK
#define MAC_UPDATE_CLK_RATIO_OFFSET(CLK_NAME) MAC_CLK_UPDATE_##CLK_NAME##_OFFSET
#define MAC_UPDATE_CLK_RATIO(CLK_NAME) \
	cif_ops_reg_write_mask(MAC_CLK_UPDATE_##CLK_NAME + VIP_SYS_ADDRESS, \
		MAC_UPDATE_CLK_RATIO_MASK(CLK_NAME), \
		1 << MAC_UPDATE_CLK_RATIO_OFFSET(CLK_NAME))

struct img_size_s {
	unsigned int	width;
	unsigned int	height;
};

extern int cif_log_lv;

enum {
	CIF_NOTICE,
	CIF_INFO,
	CIF_DEBUG,
	CIF_WARNING,
	CIF_ERROR,
};

#define CIF_PR(level, fmt, args...) \
do { \
	if (level >= cif_log_lv) { \
		printk(fmt, ##args); \
	} \
} while(0)

enum cif_pads {
	CIF_PAD_SNS0  = 0, //sink
	CIF_PAD_SNS1,
	CIF_PAD_SNS2,
	CIF_PAD_SNS3,
	CIF_PAD_SNS4,
	CIF_PAD_SNS5,
	CIF_PAD_SNS6,
	CIF_PAD_SNS7,
	CIF_PAD_SNS8,
	CIF_PAD_SNS9,
	CIF_PAD_SNS10,
	CIF_PAD_SNS11,
	CIF_PAD_ISP, //source
	CIF_PAD_NUM,
};

enum rx_mac_clk_e {
	RX_MAC_CLK_150M = 0,
	RX_MAC_CLK_200M,
	RX_MAC_CLK_300M,
	RX_MAC_CLK_400M,
	RX_MAC_CLK_500M,
	RX_MAC_CLK_600M,
	RX_MAC_CLK_900M,
	RX_MAC_CLK_BUTT,
};

enum cam_pll_freq_e {
	CAMPLL_FREQ_NONE = 0,
	CAMPLL_FREQ_37P125M,
	CAMPLL_FREQ_25M,
	CAMPLL_FREQ_27M,
	CAMPLL_FREQ_24M,
	CAMPLL_FREQ_26M,
	CAMPLL_FREQ_NUM
};

struct mclk_pll_s {
	unsigned int		cam;
	enum cam_pll_freq_e	freq;
};

struct dphy_s {
	unsigned char		enable;
	unsigned char		hs_settle;
};

enum {
	LANE_SKEW_CROSS_CLK,
	LANE_SKEW_CROSS_DATA_NEAR,
	LANE_SKEW_CROSS_DATA_FAR,
	LANE_SKEW_CLK,
	LANE_SKEW_DATA,
	LANE_SKEW_NUM,
};

enum lane_divide_mode_e {
	LANE_DIVIDE_MODE_0 = 0,
	LANE_DIVIDE_MODE_1,
	LANE_DIVIDE_MODE_2,
	LANE_DIVIDE_MODE_3,
	LANE_DIVIDE_MODE_4,
	LANE_DIVIDE_MODE_5,
	LANE_DIVIDE_MODE_6,
	LANE_DIVIDE_MODE_7,
	LANE_DIVIDE_MODE_BUTT
};

enum input_mode_e {
	INPUT_MODE_MIPI = 0,
	INPUT_MODE_SUBLVDS,
	INPUT_MODE_HISPI,
	INPUT_MODE_CMOS,
	INPUT_MODE_BT1120,
	INPUT_MODE_BT601,
	INPUT_MODE_BT656_9B,
	INPUT_MODE_BT656_9B_DDR,
	INPUT_MODE_CUSTOM_0,
	INPUT_MODE_BT_DEMUX,
	INPUT_MODE_BUTT
};

enum raw_data_type_e {
	RAW_DATA_8BIT = 0,
	RAW_DATA_10BIT,
	RAW_DATA_12BIT,
	RAW_DATA_16BIT,
	YUV422_8BIT,	/* MIPI-CSI only */
	YUV422_10BIT,   /* MIPI-CSI only*/
	RAW_DATA_BUTT
};

enum mipi_wdr_mode_e {
	MIPI_WDR_MODE_NONE = 0,
	MIPI_WDR_MODE_VC,
	MIPI_WDR_MODE_DT,
	MIPI_WDR_MODE_DOL,
	MIPI_WDR_MODE_MANUAL,  /* SOI case */
	MIPI_WDR_MODE_BUTT
};

enum wdr_mode_e {
	CIF_WDR_MODE_NONE = 0,
	CIF_WDR_MODE_2F,
	CIF_WDR_MODE_3F,
	CIF_WDR_MODE_DOL_2F,
	CIF_WDR_MODE_DOL_3F,
	CIF_WDR_MODE_DOL_BUTT
};

enum lvds_sync_mode_e {
	LVDS_SYNC_MODE_SOF = 0,
	LVDS_SYNC_MODE_SAV,
	LVDS_SYNC_MODE_BUTT
};

enum lvds_bit_endian {
	LVDS_ENDIAN_LITTLE = 0,
	LVDS_ENDIAN_BIG,
	LVDS_ENDIAN_BUTT
};

enum lvds_vsync_type_e {
	LVDS_VSYNC_NORMAL = 0,
	LVDS_VSYNC_SHARE,
	LVDS_VSYNC_HCONNECT,
	LVDS_VSYNC_BUTT
};

enum lvds_fid_type_e {
	LVDS_FID_NONE = 0,
	LVDS_FID_IN_SAV,
	LVDS_FID_BUTT
};

struct lvds_fid_type_s {
	enum lvds_fid_type_e		fid;
};

struct lvds_vsync_type_s {
	enum lvds_vsync_type_e	sync_type;
	unsigned short			hblank1;
	unsigned short			hblank2;
};

struct lvds_dev_attr_s {
	enum wdr_mode_e			wdr_mode;
	enum lvds_sync_mode_e		sync_mode;
	enum raw_data_type_e		raw_data_type;
	enum lvds_bit_endian		data_endian;
	enum lvds_bit_endian		sync_code_endian;
	short				lane_id[MIPI_LANE_NUM+1];
	short		sync_code[MIPI_LANE_NUM][WDR_VC_NUM+1][SYNC_CODE_NUM];
/*
 * sublvds:
 * sync_code[x][0][0] sync_code[x][0][1] sync_code[x][0][2] sync_code[x][0][3]
 *	n0_lef_sav	   n0_lef_eav	      n1_lef_sav	 n1_lef_eav
 * sync_code[x][1][0] sync_code[x][1][1] sync_code[x][1][2] sync_code[x][1][3]
 *	n0_sef_sav	   n0_sef_eav	      n1_sef_sav	 n1_sef_eav
 * sync_code[x][2][0] sync_code[x][2][1] sync_code[x][2][2] sync_code[x][2][3]
 *	n0_lsef_sav	   n0_lsef_eav	      n1_lsef_sav	 n1_lsef_eav
 *
 * hispi:
 * sync_code[x][0][0] sync_code[x][0][1] sync_code[x][0][2] sync_code[x][0][3]
 *	t1_sol		   tl_eol	      t1_sof		 t1_eof
 * sync_code[x][1][0] sync_code[x][1][1] sync_code[x][1][2] sync_code[x][1][3]
 *	t2_sol		   t2_eol	      t2_sof		 t2_eof
 */
	struct lvds_vsync_type_s	vsync_type;
	struct lvds_fid_type_s		fid_type;
	char				pn_swap[MIPI_LANE_NUM+1];
};

struct mipi_demux_info_s {
	unsigned int			demux_en;
	unsigned char			vc_mapping[MIPI_DEMUX_NUM];
};

struct mipi_dev_attr_s {
	enum raw_data_type_e		raw_data_type;
	short				lane_id[MIPI_LANE_NUM+1];
	enum mipi_wdr_mode_e		wdr_mode;
	short				data_type[WDR_VC_NUM];
	char				pn_swap[MIPI_LANE_NUM+1];
	struct dphy_s			dphy;
	struct mipi_demux_info_s	demux;
};

struct manual_wdr_attr_s {
	unsigned int			manual_en;
	unsigned short			l2s_distance;
	unsigned short			lsef_length;
	unsigned int			discard_padding_lines;
	unsigned int			update;
};

enum ttl_pin_func_e {
	TTL_PIN_FUNC_VS,
	TTL_PIN_FUNC_HS,
	TTL_PIN_FUNC_VDE,
	TTL_PIN_FUNC_HDE,
	TTL_PIN_FUNC_D0,
	TTL_PIN_FUNC_D1,
	TTL_PIN_FUNC_D2,
	TTL_PIN_FUNC_D3,
	TTL_PIN_FUNC_D4,
	TTL_PIN_FUNC_D5,
	TTL_PIN_FUNC_D6,
	TTL_PIN_FUNC_D7,
	TTL_PIN_FUNC_D8,
	TTL_PIN_FUNC_D9,
	TTL_PIN_FUNC_D10,
	TTL_PIN_FUNC_D11,
	TTL_PIN_FUNC_D12,
	TTL_PIN_FUNC_D13,
	TTL_PIN_FUNC_D14,
	TTL_PIN_FUNC_D15,
	TTL_PIN_FUNC_NUM,
};

enum ttl_src_e {
	TTL_VI_SRC_VI0 = 0,
	TTL_VI_SRC_VI1,
	TTL_VI_SRC_VI2,		/* BT demux */
	TTL_VI_SRC_NUM
};

enum ttl_fmt_e {
	TTL_SYNC_PAT = 0,
	TTL_VHS_11B,
	TTL_VHS_19B,
	TTL_VDE_11B,
	TTL_VDE_19B,
	TTL_VSDE_11B,
	TTL_VSDE_19B,
};

enum bt_demux_mode_e {
	BT_DEMUX_DISABLE = 0,
	BT_DEMUX_2,
	BT_DEMUX_3,
	BT_DEMUX_4,
};

struct bt_demux_sync_s {
	unsigned char		sav_vld;
	unsigned char		sav_blk;
	unsigned char		eav_vld;
	unsigned char		eav_blk;
};

struct bt_demux_attr_s {
	signed char			func[TTL_PIN_FUNC_NUM];
	unsigned short			v_fp;
	unsigned short			h_fp;
	unsigned short			v_bp;
	unsigned short			h_bp;
	enum bt_demux_mode_e		mode;
	unsigned char			sync_code_part_A[3];	/* sync code 0~2 */
	struct bt_demux_sync_s		sync_code_part_B[BT_DEMUX_NUM];	/* sync code 3 */
	char				yc_exchg;
};

struct ttl_dev_attr_s {
	enum ttl_src_e			vi;
	enum ttl_fmt_e			ttl_fmt;
	enum raw_data_type_e		raw_data_type;
	signed char			func[TTL_PIN_FUNC_NUM];
	unsigned short			v_bp;
	unsigned short			h_bp;
};

struct combo_dev_attr_s {
	enum input_mode_e		input_mode;
	enum rx_mac_clk_e		mac_clk;
	struct mclk_pll_s		mclk;
	union {
		struct mipi_dev_attr_s	mipi_attr;
		struct lvds_dev_attr_s	lvds_attr;
		struct ttl_dev_attr_s	ttl_attr;
		struct bt_demux_attr_s	bt_demux_attr;
	};
	unsigned int			devno;
	unsigned int			cif_mode;
	struct img_size_s		img_size;
	struct manual_wdr_attr_s	wdr_manu;
};

enum clk_edge_e {
	CLK_UP_EDGE = 0,
	CLK_DOWN_EDGE,
	CLK_EDGE_BUTT
};

struct clk_edge_s {
	unsigned int			devno;
	enum clk_edge_e			edge;
};

enum output_msb_e {
	OUTPUT_NORM_MSB = 0,
	OUTPUT_REVERSE_MSB,
	OUTPUT_MSB_BUTT
};

struct msb_s {
	unsigned int			devno;
	enum output_msb_e		msb;
};

struct crop_top_s {
	unsigned int			devno;
	unsigned int			crop_top;
	unsigned int			update;
};

struct manual_wdr_s {
	unsigned int			devno;
	struct manual_wdr_attr_s	attr;
};

struct vsync_gen_s {
	unsigned int			devno;
	unsigned int			distance_fp;
};

enum bt_fmt_out_e {
	BT_FMT_OUT_CBYCRY,
	BT_FMT_OUT_CRYCBY,
	BT_FMT_OUT_YCBYCR,
	BT_FMT_OUT_YCRYCB,
};

struct bt_fmt_out_s {
	unsigned int			devno;
	enum bt_fmt_out_e		fmt_out;
};

struct cif_crop_win_s {
	unsigned int			devno;
	unsigned int			enable;
	unsigned int			x;
	unsigned int			y;
	unsigned int			w;
	unsigned int			h;
};

struct cif_yuv_swap_s {
	unsigned int			devno;
	unsigned int			uv_swap;
	unsigned int			yc_swap;
};

struct cvi_csi_status {
	unsigned int			errcnt_ecc;
	unsigned int			errcnt_crc;
	unsigned int			errcnt_hdr;
	unsigned int			errcnt_wc;
	unsigned int			fifo_full;
};

struct cvi_lvds_status {
	unsigned int			fifo_full;
};

struct cvi_link {
	struct cif_ctx			cif_ctx;
	int				irq_num;
	struct reset_control		*phy_reset;
	struct reset_control		*phy_apb_reset;
	unsigned int			is_on;
	struct cif_param		param;
	struct combo_dev_attr_s		attr;
	enum clk_edge_e			clk_edge;
	enum output_msb_e		msb;
	unsigned int			crop_top;
	unsigned int			distance_fp;
	int				snsr_rst_pin;
	enum of_gpio_flags		snsr_rst_pol;
	union {
		struct cvi_csi_status	sts_csi;
		struct cvi_lvds_status	sts_lvds;
	};
	struct device			*dev;
	enum rx_mac_clk_e		mac_clk;
	enum ttl_bt_fmt_out		bt_fmt_out;
};

struct cvi_cam_clk {
	int				is_on;
	struct clk			*clk_o;
};

struct cif_sensor_info {
	struct v4l2_subdev *sd;
	struct v4l2_mbus_config mbus;
	int lanes;
};

struct cvi_cif_dev {
	struct device				*dev;
	struct v4l2_subdev			sd;
	struct v4l2_subdev			*isp_sd;
	struct media_pad			pads[CIF_PAD_NUM];
	struct v4l2_device			v4l2_dev;
	struct media_device 		media_dev;
	spinlock_t					lock;
	struct mutex				mutex;
	struct cvi_link				link[MAX_LINK_NUM];
	bool						sink_linked[CIF_PAD_NUM - 1];
	struct cif_sensor_info		sensors[MAX_CHN_NUM];
	int							num_sensors;
	struct v4l2_subdev			*src_sd;
	struct v4l2_async_notifier	notifier;
	struct cvi_cam_clk			clk_cam0;
	struct cvi_cam_clk			clk_cam1;
	struct cvi_cam_clk			clk_cam2;
	struct cvi_cam_clk			clk_cam3;
	struct cvi_cam_clk			clk_cam4;
	struct cvi_cam_clk			clk_cam5;
	struct cvi_cam_clk			clk_cam6;
	struct cvi_cam_clk			vip_sys2;
	struct cvi_cam_clk			clk_mipimpll; /* mipipll */
	struct cvi_cam_clk			clk_disppll; /* disppll */
	struct cvi_cam_clk			clk_fpll; /* fpll */
	unsigned int				max_mac_clk;
	void						*pad_ctrl;
	struct miscdevice	miscdev;
};
struct cif_attr_s {
	unsigned int	devno;
	unsigned int	stagger_vsync;
};
enum CIF_CB_CMD {
	CIF_CB_RESET_LVDS,
	CIF_CB_GET_CIF_ATTR,
	CIF_CB_MAX
};
enum sns_rst_active_e {
	RST_ACTIVE_LOW,
	RST_ACTIVE_HIGH,
	RST_ACTIVE_BUFF,
};
typedef struct sns_rst_config {
	unsigned int			devno;
	unsigned int			gpio_pin;
	enum sns_rst_active_e	gpio_active;
} SNS_RST_CONFIG;

#define MIPI_IOC_MAGIC		'm'

/* Support commands */
#define MIPI_SET_DEV_ATTR		_IOW(MIPI_IOC_MAGIC, \
						0x01, struct combo_dev_attr_s)
#define MIPI_SET_OUTPUT_CLK_EDGE	_IOW(MIPI_IOC_MAGIC, \
						0x02, struct clk_edge_s)
#define MIPI_RESET_SENSOR		_IOW(MIPI_IOC_MAGIC, \
						0x05, struct sns_rst_config)
#define MIPI_UNRESET_SENSOR		_IOW(MIPI_IOC_MAGIC, \
						0x06, struct sns_rst_config)
#define MIPI_RESET_MIPI			_IOW(MIPI_IOC_MAGIC, \
						0x07, unsigned int)
#define MIPI_ENABLE_SENSOR_CLOCK	_IOW(MIPI_IOC_MAGIC, \
						0x10, unsigned int)
#define MIPI_DISABLE_SENSOR_CLOCK	_IOW(MIPI_IOC_MAGIC, \
						0x11, unsigned int)
#define MIPI_SET_CROP_TOP		_IOW(MIPI_IOC_MAGIC, \
						0x20, struct crop_top_s)
#define MIPI_SET_WDR_MANUAL		_IOW(MIPI_IOC_MAGIC, \
						0x21, struct manual_wdr_s)
#define MIPI_SET_LVDS_FP_VS		_IOW(MIPI_IOC_MAGIC, \
						0x22, struct vsync_gen_s)
#define MIPI_RESET_LVDS			_IOW(MIPI_IOC_MAGIC, \
						0x23, unsigned int)
#define MIPI_SET_BT_FMT_OUT		_IOW(MIPI_IOC_MAGIC, \
						0x24, struct bt_fmt_out_s)
#define MIPI_GET_CIF_ATTR		_IOWR(MIPI_IOC_MAGIC, \
						0x25, struct cif_attr_s)
#define MIPI_SET_SENSOR_CLOCK		_IOW(MIPI_IOC_MAGIC, \
						0x26, struct mclk_pll_s)
#define MIPI_SET_MAX_MAC_CLOCK		_IOW(MIPI_IOC_MAGIC, \
						0x27, unsigned int)
#define MIPI_SET_CROP_WINDOW		_IOW(MIPI_IOC_MAGIC, \
						0x28, struct cif_crop_win_s)
#define MIPI_SET_YUV_SWAP		_IOW(MIPI_IOC_MAGIC, \
						0x29, struct cif_yuv_swap_s)
/* Unsupport commands */
#define MIPI_SET_PHY_CMVMODE		_IOW(MIPI_IOC_MAGIC, \
						0x04, unsigned int)
#define MIPI_UNRESET_MIPI		_IOW(MIPI_IOC_MAGIC, \
						0x08, unsigned int)
#define MIPI_RESET_SLVS			_IOW(MIPI_IOC_MAGIC, \
						0x09, unsigned int)
#define MIPI_UNRESET_SLVS		_IOW(MIPI_IOC_MAGIC, \
						0x0A, unsigned int)
#define MIPI_SET_HS_MODE		_IOW(MIPI_IOC_MAGIC, \
						0x0B, unsigned int)
#define MIPI_ENABLE_MIPI_CLOCK		_IOW(MIPI_IOC_MAGIC, \
						0x0C, unsigned int)
#define MIPI_DISABLE_MIPI_CLOCK		_IOW(MIPI_IOC_MAGIC, \
						0x0D, unsigned int)
#define MIPI_ENABLE_SLVS_CLOCK		_IOW(MIPI_IOC_MAGIC, \
						0x0E, unsigned int)
#define MIPI_DISABLE_SLVS_CLOCK		_IOW(MIPI_IOC_MAGIC, \
						0x0F, unsigned int)
#endif // _U_VIP_CIF_H_


int cif_start_stream(struct cvi_cif_dev *dev, struct combo_dev_attr_s *attr);
int _cif_enable_snsr_clk(struct cvi_cif_dev *cdev, uint32_t devno, uint8_t on);
int dbg_hdler(struct cvi_cif_dev *dev, char const *input);
int proc_cif_show(struct seq_file *m, void *v);
int cif_init_subdev(struct platform_device *pdev, struct cvi_cif_dev *dev);
int _cif_set_mac_clk(struct cvi_cif_dev *cdev, uint32_t devno, enum rx_mac_clk_e mac_clk);
void cif_ops_reg_write_mask(uintptr_t addr, u32 mask, u32 data);
const char *_to_string_input_mode(enum input_mode_e input_mode);
const char *_to_string_mipi_wdr_mode(enum mipi_wdr_mode_e wdr);
const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type);
const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk);
const char *_to_string_mclk(enum cam_pll_freq_e freq);
int cif_set_dev_attr(struct cvi_cif_dev *dev,struct combo_dev_attr_s *attr);
int cif_set_output_clk_edge(struct cvi_cif_dev *dev,struct clk_edge_s *clk_edge);
int cif_reset_mipi(struct cvi_cif_dev *dev, uint32_t devno);
int cif_set_crop_top(struct cvi_cif_dev *dev, struct crop_top_s *crop);
int cif_set_windowing(struct cvi_cif_dev *dev,struct cif_crop_win_s *win);
int cif_set_wdr_manual(struct cvi_cif_dev *dev,struct manual_wdr_s *manual);
int cif_set_lvds_fp_vs(struct cvi_cif_dev *dev,struct vsync_gen_s *vs);
int cif_bt_fmt_out(struct cvi_cif_dev *dev, struct bt_fmt_out_s *fmt_out);

