
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/sys.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <linux/clk-provider.h>

#include <linux/common.h>
#include <linux/defines.h>
#include <linux/comm_buffer.h>

#include <proc/vo_proc.h>
#include <proc/vo_disp_proc.h>
#include "vo.h"
#include "vo_interfaces.h"
#include "pinctrl-cv186x.h"
#include "disp.h"
#include "vpss_cb.h"
#include "vo_cb.h"
#include "ldc_cb.h"
#include "rgn_cb.h"
#include "vo_rgn_ctrl.h"
#include "reg.h"

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define WAIT_TIMEOUT_MS  200
#define VO_PROFILE
/*******************************************************
 *  Global variables
 ******************************************************/
int vo_log_lv = DBG_WARN;
//to do: smooth from uboot
int smooth[VO_MAX_DEV_NUM];
int debug;
static bool hide_vo;

module_param(vo_log_lv, int, 0644);
module_param(hide_vo, bool, 0444);

struct _vo_gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

struct vo_stitch_cb_data {
	wait_queue_head_t wait;
	u8 flag;
};

#define IS_VB_OFFSET_INVALID(buf) \
	((buf).offset_left < 0 || (buf).offset_right < 0 || \
	 (buf).offset_top < 0 || (buf).offset_bottom < 0 || \
	 ((u32)((buf).offset_left + (buf).offset_right) > (buf).size.width) || \
	 ((u32)((buf).offset_top + (buf).offset_bottom) > (buf).size.height))

#define FRC_INVALID(frame_rate_ctrl)	\
	(frame_rate_ctrl.dst_frame_rate <= 0 || frame_rate_ctrl.src_frame_rate <= 0 ||	\
		frame_rate_ctrl.dst_frame_rate >= frame_rate_ctrl.src_frame_rate)

struct vo_ctx *g_vo_ctx;
struct vo_core_dev *g_core_dev;
static atomic_t  dev_open_cnt;
extern const char *const disp_irq_name[DISP_MAX_INST];

//update proc info
static void _update_vo_real_frame_rate(struct timer_list *timer);
DEFINE_TIMER(vo_timer_proc, _update_vo_real_frame_rate);

void _disp_sel_remux(vo_dev VoDev, const struct vo_d_remap *pins, unsigned int pin_num)
{
	int i = 0;

	for (i = 0; i < pin_num; ++i) {
		if (VoDev == 0){
			switch (pins[i].sel) {
				case VO_MIPI0_TXP0:
					PINMUX_CONFIG(PAD_MIPI0_TX0P, VO0_D14, PHY);
				break;
				case VO_MIPI0_TXN0:
					PINMUX_CONFIG(PAD_MIPI0_TX0N, VO0_D15, PHY);
				break;
				case VO_MIPI0_TXP1:
					PINMUX_CONFIG(PAD_MIPI0_TX1P, VO0_D16, PHY);
				break;
				case VO_MIPI0_TXN1:
					PINMUX_CONFIG(PAD_MIPI0_TX1N, VO0_D17, PHY);
				break;
				case VO_MIPI0_TXP2:
					PINMUX_CONFIG(PAD_MIPI0_TX2P, VO0_D18, PHY);
				break;
				case VO_MIPI0_TXN2:
					PINMUX_CONFIG(PAD_MIPI0_TX2N, VO0_D19, PHY);
				break;
				case VO_MIPI0_TXP3:
					PINMUX_CONFIG(PAD_MIPI0_TX3P, VO0_D20, PHY);
				break;
				case VO_MIPI0_TXN3:
					PINMUX_CONFIG(PAD_MIPI0_TX3N, VO0_D21, PHY);
				break;
				case VO_MIPI0_TXP4:
					PINMUX_CONFIG(PAD_MIPI0_TX4P, VO0_D22, PHY);
				break;
				case VO_MIPI0_TXN4:
					PINMUX_CONFIG(PAD_MIPI0_TX4N, VO0_D23, PHY);
				break;
				case VO_MIPI1_TXP0:
					PINMUX_CONFIG(PAD_MIPI1_TX0P, VO0_D24, PHY);
				break;
				case VO_MIPI1_TXN0:
					PINMUX_CONFIG(PAD_MIPI1_TX0N, VO0_D25, PHY);
				break;
				case VO_MIPI1_TXP1:
					PINMUX_CONFIG(PAD_MIPI1_TX1P, VO0_D26, PHY);
				break;
				case VO_MIPI1_TXN1:
					PINMUX_CONFIG(PAD_MIPI1_TX1N, VO0_D0, PHY);
				break;
				case VO_MIPI1_TXP2:
					PINMUX_CONFIG(PAD_MIPI1_TX2P, VO0_D1, PHY);
				break;
				case VO_MIPI1_TXN2:
					PINMUX_CONFIG(PAD_MIPI1_TX2N, VO0_D2, PHY);
				break;
				case VO_MIPI1_TXP3:
					PINMUX_CONFIG(PAD_MIPI1_TX3P, VO0_D3, PHY);
				break;
				case VO_MIPI1_TXN3:
					PINMUX_CONFIG(PAD_MIPI1_TX3N, VO0_D4, PHY);
				break;
				case VO_MIPI1_TXP4:
					PINMUX_CONFIG(PAD_MIPI1_TX4P, VO0_D5, PHY);
				break;
				case VO_MIPI1_TXN4:
					PINMUX_CONFIG(PAD_MIPI1_TX4N, VO0_D6, PHY);
				break;
				case VO_VIVO_CLK:
					PINMUX_CONFIG(PAD_VIVO_CLK, VO0_CLK, G5);
				break;
				case VO_VIVO0_D10:
					PINMUX_CONFIG(PAD_VIVO0_D10, VO0_D7, G5);
				break;
				case VO_VIVO0_D11:
					PINMUX_CONFIG(PAD_VIVO0_D11, VO0_D8, G5);
				break;
				case VO_VIVO0_D12:
					PINMUX_CONFIG(PAD_VIVO0_D12, VO0_D9, G5);
				break;
				case VO_VIVO0_D13:
					PINMUX_CONFIG(PAD_VIVO0_D13, VO0_D10, G5);
				break;
				case VO_VIVO0_D14:
					PINMUX_CONFIG(PAD_VIVO0_D14, VO0_D11, G5);
				break;
				case VO_VIVO0_D15:
					PINMUX_CONFIG(PAD_VIVO0_D15, VO0_D12, G5);
				break;
				case VO_VIVO0_D16:
					PINMUX_CONFIG(PAD_VIVO0_D16, VO0_D13, G5);
				break;
				default:
				break;
			}
		}else if(VoDev == 1) {
			switch (pins[i].sel) {
				case VO_MIPI0_TXP0:
					PINMUX_CONFIG(PAD_MIPI0_TX0P, VO1_D14, PHY);
				break;
				case VO_MIPI0_TXN0:
					PINMUX_CONFIG(PAD_MIPI0_TX0N, VO1_D15, PHY);
				break;
				case VO_MIPI0_TXP1:
					PINMUX_CONFIG(PAD_MIPI0_TX1P, VO1_D16, PHY);
				break;
				case VO_MIPI0_TXN1:
					PINMUX_CONFIG(PAD_MIPI0_TX1N, VO1_D17, PHY);
				break;
				case VO_MIPI0_TXP2:
					PINMUX_CONFIG(PAD_MIPI0_TX2P, VO1_D18, PHY);
				break;
				case VO_MIPI0_TXN2:
					PINMUX_CONFIG(PAD_MIPI0_TX2N, VO1_D19, PHY);
				break;
				case VO_MIPI0_TXP3:
					PINMUX_CONFIG(PAD_MIPI0_TX3P, VO1_D20, PHY);
				break;
				case VO_MIPI0_TXN3:
					PINMUX_CONFIG(PAD_MIPI0_TX3N, VO1_D21, PHY);
				break;
				case VO_MIPI0_TXP4:
					PINMUX_CONFIG(PAD_MIPI0_TX4P, VO1_D22, PHY);
				break;
				case VO_MIPI0_TXN4:
					PINMUX_CONFIG(PAD_MIPI0_TX4N, VO1_D23, PHY);
				break;
				case VO_MIPI1_TXP0:
					PINMUX_CONFIG(PAD_MIPI1_TX0P, VO1_D24, PHY);
				break;
				case VO_MIPI1_TXN0:
					PINMUX_CONFIG(PAD_MIPI1_TX0N, VO1_D25, PHY);
				break;
				case VO_MIPI1_TXP1:
					PINMUX_CONFIG(PAD_MIPI1_TX1P, VO1_D26, PHY);
				break;
				case VO_MIPI1_TXN1:
					PINMUX_CONFIG(PAD_MIPI1_TX1N, VO1_D0, PHY);
				break;
				case VO_MIPI1_TXP2:
					PINMUX_CONFIG(PAD_MIPI1_TX2P, VO1_D1, PHY);
				break;
				case VO_MIPI1_TXN2:
					PINMUX_CONFIG(PAD_MIPI1_TX2N, VO1_D2, PHY);
				break;
				case VO_MIPI1_TXP3:
					PINMUX_CONFIG(PAD_MIPI1_TX3P, VO1_D3, PHY);
				break;
				case VO_MIPI1_TXN3:
					PINMUX_CONFIG(PAD_MIPI1_TX3N, VO1_D4, PHY);
				break;
				case VO_MIPI1_TXP4:
					PINMUX_CONFIG(PAD_MIPI1_TX4P, VO1_D5, PHY);
				break;
				case VO_MIPI1_TXN4:
					PINMUX_CONFIG(PAD_MIPI1_TX4N, VO1_D6, PHY);
				break;
				case VO_VIVO_CLK:
					PINMUX_CONFIG(PAD_VIVO_CLK, VO1_CLK, G5);
				break;
				case VO_VIVO0_D10:
					PINMUX_CONFIG(PAD_VIVO0_D10, VO1_D7, G5);
				break;
				case VO_VIVO0_D11:
					PINMUX_CONFIG(PAD_VIVO0_D11, VO1_D8, G5);
				break;
				case VO_VIVO0_D12:
					PINMUX_CONFIG(PAD_VIVO0_D12, VO1_D9, G5);
				break;
				case VO_VIVO0_D13:
					PINMUX_CONFIG(PAD_VIVO0_D13, VO1_D10, G5);
				break;
				case VO_VIVO0_D14:
					PINMUX_CONFIG(PAD_VIVO0_D14, VO1_D11, G5);
				break;
				case VO_VIVO0_D15:
					PINMUX_CONFIG(PAD_VIVO0_D15, VO1_D12, G5);
				break;
				case VO_VIVO0_D16:
					PINMUX_CONFIG(PAD_VIVO0_D16, VO1_D13, G5);
				break;
				default:
				break;
			}
		}

		disp_vo_mux_sel(VoDev,  pins[i].sel, pins[i].mux);
	}

}

static void _disp_sel_pinmux(vo_dev VoDev, enum vo_disp_intf intf_type, void *param)
{
	if (intf_type == VO_DISP_INTF_BT656 || intf_type == VO_DISP_INTF_BT1120) {
		struct bt_intf_cfg *cfg = param;
		_disp_sel_remux(VoDev, cfg->pins.d_pins, cfg->pins.pin_num);
	}
}

const struct disp_pattern patterns[VO_PAT_MAX] = {
	{.type = PAT_TYPE_OFF,	.color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_SNOW, .color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_AUTO, .color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_RED},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_GREEN},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_BLUE},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_BAR},
	{.type = PAT_TYPE_H_GRAD, .color = PAT_COLOR_WHITE},
	{.type = PAT_TYPE_V_GRAD, .color = PAT_COLOR_WHITE},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_USR,
	.rgb = {0, 0, 0} },
};

/*******************************************************
 *  Internal APIs
 ******************************************************/
static int _vo_stitch_call_vpss(struct vpss_stitch_cfg *stitch_cfg)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = E_MODULE_VPSS;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = VPSS_CB_STITCH;
	exe_cb.data   = (void *)stitch_cfg;

	return base_exe_module_cb(&exe_cb);
}

int _vo_create_proc(struct vo_ctx *ctx)
{
	int ret = 0;

	if (vo_proc_init(ctx) < 0) {
		TRACE_VO(DBG_ERR, "vo proc init failed\n");
		return -EAGAIN;
	}

	if (vo_disp_proc_init() < 0) {
		TRACE_VO(DBG_ERR, "proc init failed\n");
		return -EAGAIN;
	}
	return ret;
}

void _vo_destroy_proc(void)
{
	vo_disp_proc_remove();
	vo_proc_remove();
}

int vo_set_interface(vo_dev dev, struct vo_disp_intf_cfg *cfg)
{
	struct vo_core_dev *vdev = g_core_dev;

	// if (smooth[dev]) {
	//	TRACE_VO(DBG_DEBUG, "set_interface won't apply if smooth.\n");
	//	disp_reg_force_up(dev);
	//	vdev->vo_core[dev].disp_interface = cfg->intf_type;
	//	return 0;
	// }

	if (atomic_read(&vdev->vo_core[dev].disp_streamon) == 1) {
		TRACE_VO(DBG_INFO, "set_interface can't be control if streaming.\n");
		return 0;
	}

	if (cfg->intf_type == VO_DISP_INTF_DSI) {
		TRACE_VO(DBG_INFO, "MIPI use mipi_tx to control.\n");
		return 0;
	} else if (cfg->intf_type == VO_DISP_INTF_HDMI) {
		TRACE_VO(DBG_INFO, "HDMI use hdmi_tx to control.\n");
		return 0;
	} else if (cfg->intf_type == VO_DISP_INTF_LVDS) {
		int i = 0;
		union disp_lvdstx lvds_cfg;
		bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

		for (i = 0; i < LANE_MAX_NUM; i++) {
			if ((cfg->lvds_cfg.lane_id[i] < 0) ||
				(cfg->lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
				dphy_dsi_set_lane(dev, i, MIPI_TX_LANE_MAX, false, false);
				continue;
			}
			dphy_dsi_set_lane(dev, i, cfg->lvds_cfg.lane_id[i],
					  cfg->lvds_cfg.lane_pn_swap[i], false);
			if (cfg->lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
				data_en[cfg->lvds_cfg.lane_id[i] - 1] = true;
			}
		}
		dphy_dsi_lane_en(dev, true, data_en, false);

		disp_set_intf(dev, VO_DISP_INTF_LVDS);

		if (cfg->lvds_cfg.pixelclock == 0) {
			TRACE_VO(DBG_ERR, "lvds pixelclock 0 invalid\n");
			return -1;
		}
		lvds_cfg.b.out_bit = cfg->lvds_cfg.out_bits;
		lvds_cfg.b.vesa_mode = cfg->lvds_cfg.mode;
		if (cfg->lvds_cfg.chn_num == 1)
			lvds_cfg.b.dual_ch = 0;
		else if (cfg->lvds_cfg.chn_num == 2)
			lvds_cfg.b.dual_ch = 1;
		else {
			lvds_cfg.b.dual_ch = 0;
			TRACE_VO(DBG_ERR, "invalid lvds chn_num(%d). Use 1 instead."
				, cfg->lvds_cfg.chn_num);
		}
		lvds_cfg.b.vs_out_en = cfg->lvds_cfg.vs_out_en;
		lvds_cfg.b.hs_out_en = cfg->lvds_cfg.hs_out_en;
		lvds_cfg.b.hs_blk_en = cfg->lvds_cfg.hs_blk_en;
		lvds_cfg.b.ml_swap = cfg->lvds_cfg.msb_lsb_data_swap;
		lvds_cfg.b.ctrl_rev = cfg->lvds_cfg.serial_msb_first;
		lvds_cfg.b.oe_swap = cfg->lvds_cfg.even_odd_link_swap;
		lvds_cfg.b.en = cfg->lvds_cfg.enable;
		dphy_lvds_set_pll(dev, cfg->lvds_cfg.pixelclock, cfg->lvds_cfg.chn_num);
		disp_lvdstx_set(dev, lvds_cfg);
	} else if (cfg->intf_type == VO_DISP_INTF_BT656 || cfg->intf_type == VO_DISP_INTF_BT1120) {
		char fmt_sel = 0;
		char bt_mode;
		union disp_bt_enc enc;
		union disp_bt_sync_code sync;

		if (cfg->bt_cfg.mode == BT_MODE_1120) {
			disp_set_intf(dev, VO_DISP_INTF_BT1120);
			bt_mode = DISP_VO_SEL_BT1120;
		}else if (cfg->bt_cfg.mode == BT_MODE_656) {
			disp_set_intf(dev, VO_DISP_INTF_BT656);
			bt_mode = DISP_VO_SEL_BT656;
		}else if (cfg->bt_cfg.mode == BT_MODE_601) {
			disp_set_intf(dev, VO_DISP_INTF_BT601);
			bt_mode = DISP_VO_SEL_BT601;
		}else {
			TRACE_VO(DBG_ERR, "invalid bt-mode(%d)\n", cfg->bt_cfg.mode);
			return -1;
		}

		dphy_dsi_set_gp_drv_level(dev);

		if (cfg->bt_cfg.mode == BT_MODE_1120) {
			dphy_dsi_set_pll(dev, cfg->bt_cfg.pixelclock, 4, 24);
			dphy_dsi_clk_setting(dev, 0x10010);
		} else if (cfg->bt_cfg.mode == BT_MODE_656) {
			dphy_dsi_set_pll(dev, cfg->bt_cfg.pixelclock * 2, 4, 24);
			dphy_dsi_clk_setting(dev, 0x10000);
		} else if (cfg->bt_cfg.mode == BT_MODE_601) {
			dphy_dsi_set_pll(dev, cfg->bt_cfg.pixelclock * 2, 4, 24);
			dphy_dsi_clk_setting(dev, 0x10000);
		}

		if (cfg->bt_cfg.mode == BT_MODE_656)
			fmt_sel = 0;
		else if(cfg->bt_cfg.mode == BT_MODE_601)
			fmt_sel = 2;
		else if(cfg->bt_cfg.mode == BT_MODE_1120)
			fmt_sel = 1;

		//set csc value
		disp_set_out_csc(dev, DISP_CSC_601_FULL_RGB2YUV);
		disp_set_vo_type_sel(dev, bt_mode);
		_disp_sel_pinmux(dev, cfg->intf_type, &cfg->bt_cfg);

		enc.raw = 0;
		enc.b.fmt_sel  = fmt_sel;
		enc.b.data_seq = cfg->bt_cfg.data_seq;
		enc.b.clk_inv  = cfg->bt_cfg.bt_clk_inv;
		enc.b.hs_inv   = cfg->bt_cfg.bt_hs_inv;
		enc.b.vs_inv   = cfg->bt_cfg.bt_vs_inv;
		sync.b.sav_vld = 0x80;
		sync.b.sav_blk = 0xab;
		sync.b.eav_vld = 0x9d;
		sync.b.eav_blk = 0xb6;
		disp_bt_set(dev, enc, sync);
		disp_bt_en(dev);
	} else {
		TRACE_VO(DBG_ERR, "invalid disp-intf(%d)\n", cfg->intf_type);
		return -1;
	}

	disp_reg_force_up(dev);

	vdev->vo_core[dev].disp_interface = cfg->intf_type;

	return 0;
}


/*******************************************************
 *  File operations for core
 ******************************************************/
void vo_fill_disp_timing(struct disp_timing *timing,
		struct vo_bt_timings *bt_timing)
{

	timing->vtotal = VO_DV_BT_FRAME_HEIGHT(bt_timing) - 1;
	timing->htotal = VO_DV_BT_FRAME_WIDTH(bt_timing) - 1;
	timing->vsync_start = 0;
	timing->vsync_end = timing->vsync_start + bt_timing->vsync - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + bt_timing->vsync + bt_timing->vbackporch;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + bt_timing->height - 1;
	timing->hsync_start = 0;
	timing->hsync_end = timing->hsync_start + bt_timing->hsync - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + bt_timing->hsync + bt_timing->hbackporch;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + bt_timing->width - 1;
	timing->vsync_pol = bt_timing->polarities & VO_DV_VSYNC_POS_POL;
	timing->hsync_pol = bt_timing->polarities & VO_DV_HSYNC_POS_POL;
}


static void _vo_hw_enque(vo_dev dev, struct vo_layer_ctx *layer_ctx)
{
	struct vo_buffer *buf;
	struct disp_buffer *b = NULL;
	struct disp_buffer *work_buf = NULL;
	struct disp_cfg *cfg;
	int i = 0;
	unsigned long flags;

	spin_lock_irqsave(&layer_ctx->list_lock, flags);
	if (!list_empty(&layer_ctx->list_work)) {
		work_buf = list_first_entry(&layer_ctx->list_work,
					    struct disp_buffer, list);
		list_move_tail(&work_buf->list, &layer_ctx->list_done);
	}
	if (!list_empty(&layer_ctx->list_wait)) {
		b = list_first_entry(&layer_ctx->list_wait,
				     struct disp_buffer, list);
		list_move_tail(&b->list, &layer_ctx->list_work);
	}
	spin_unlock_irqrestore(&layer_ctx->list_lock, flags);

	if (b == NULL)
		return;

	buf = &b->buf;
	for (i = 0; i < 3; i++) {
		TRACE_VO(DBG_INFO, "b->buf.planes[%d].addr=%llx\n", i, b->buf.planes[i].addr);
	}

	disp_enable_window_bgcolor(dev, false);
	cfg = disp_get_cfg(dev);
	cfg->mem.addr0 = b->buf.planes[0].addr;
	cfg->mem.addr1 = b->buf.planes[1].addr;
	cfg->mem.addr2 = b->buf.planes[2].addr;
	cfg->mem.pitch_y = b->buf.planes[0].bytesused;
	cfg->mem.pitch_c = b->buf.planes[1].bytesused;
	disp_set_mem(dev, &cfg->mem);

	layer_ctx->display_pts = ((struct vb_s *)b->blk)->buf.pts;
}

static int simplify_rate(u32 dst_in, u32 src_in, u32 *dst_out, u32 *src_out)
{
	u32 i = 1;
	u32 a, b;

	while (i < dst_in + 1) {
		a = dst_in % i;
		b = src_in % i;
		if (a == 0 && b == 0) {
			dst_in = dst_in / i;
			src_in = src_in / i;
			i = 1;
		}
		i++;
	}
	*dst_out = dst_in;
	*src_out = src_in;
	return 0;
}

static u8 vo_frame_ctrl(u64 frame_index, frame_rate_ctrl_s *frame_rate_ctrl)
{
	u32 src_simp;
	u32 dst_simp;
	u32 index;
	u32 src_dur, dst_dur;
	u32 cur_indx, next_indx;

	simplify_rate(frame_rate_ctrl->dst_frame_rate, frame_rate_ctrl->src_frame_rate,
		      &dst_simp, &src_simp);

	index = frame_index % src_simp;
	if (index == 0)
		return true;

	src_dur = 100;
	dst_dur = (src_dur * src_simp) / dst_simp;
	cur_indx = (index - 1) * src_dur / dst_dur;
	next_indx = index * src_dur / dst_dur;

	if (next_indx == cur_indx)
		return false;

	return true;
}

static void vo_snap(mmf_chn_s chn, struct vb_jobs_t *jobs, vb_blk blk)
{
	struct vb_s *p = (struct vb_s *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		pr_err("handle snap fail, Null parameter\n");
		return;
	}

	if (!jobs->inited) {
		pr_err("handle snap fail, job not inited yet\n");
		return;
	}

	mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(1, &p->usr_cnt);
			atomic_long_fetch_or(BIT(ID_USER), &p->mod_ids);
			s->avail = true;
			wake_up(&s->cond_queue);
			mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = &jobs->doneq;
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			struct vb_s *vb = NULL;

			FIFO_POP(doneq, &vb);
			atomic_long_fetch_and(~BIT(chn.mod_id), &vb->mod_ids);
			vb_release_block((vb_blk)vb);
		}
		atomic_fetch_add(1, &p->usr_cnt);
		atomic_long_fetch_or(BIT(chn.mod_id), &p->mod_ids);
		FIFO_PUSH(doneq, p);
	}
	mutex_unlock(&jobs->dlock);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
static rect_s aspect_ratio_resize(size_s in, size_s out)
{
	rect_s rect;
	u32 scale = in.height * in.width;
	u32 ratio_int = MIN2(out.width * in.height, out.height * in.width);
	u64 height, width;

	height = (u64)in.height * ratio_int + scale / 2;
	do_div(height, scale);
	rect.height = (u32)height;

	width = (u64)in.width * ratio_int + scale / 2;
	do_div(width, scale);
	rect.width = (u32)width;

	rect.x = (out.width - rect.width) >> 1;
	rect.y = (out.height - rect.height) >> 1;

	return rect;
}

static void _vo_gdc_callback(void *gdc_param, vb_blk blk)
{
	struct _vo_gdc_cb_param *cb_param = NULL;
	struct vo_chn_ctx *chn_ctx;
	mmf_chn_s chn;
	struct vb_jobs_t *jobs;
	struct vb_s *vb = (struct vb_s *)blk;

	if (!gdc_param)
		return;

	cb_param = (struct _vo_gdc_cb_param *)gdc_param;
	chn = cb_param->chn;
	chn_ctx = &g_vo_ctx->layer_ctx[chn.dev_id].chn_ctx[chn.chn_id];

	if (!chn_ctx->is_chn_enable || g_vo_ctx->suspend) {
		TRACE_VO(DBG_INFO, "layer(%d) chn(%d) disable.\n", chn.dev_id, chn.chn_id);
		mutex_unlock(&chn_ctx->gdc_lock);
		vb_release_block(blk);
		vfree(gdc_param);
		gdc_param = NULL;
		return;
	}

	if ((chn_ctx->pause && !chn_ctx->refresh) || chn_ctx->hide) {
		TRACE_VO(DBG_INFO, "layer(%d) chn(%d) pause/hide.\n", chn.dev_id, chn.chn_id);
		mutex_unlock(&chn_ctx->gdc_lock);
		vb_release_block(blk);
		vfree(gdc_param);
		gdc_param = NULL;
		return;
	}
	if (IS_VB_OFFSET_INVALID(vb->buf)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) vb offset (%d %d %d %d) invalid\n",
			 chn.dev_id, chn.chn_id,
			 vb->buf.offset_left, vb->buf.offset_right,
			 vb->buf.offset_top, vb->buf.offset_bottom);
		mutex_unlock(&chn_ctx->gdc_lock);
		vb_release_block(blk);
		vfree(gdc_param);
		gdc_param = NULL;
		return;
	}

	mutex_unlock(&chn_ctx->gdc_lock);

	jobs = &chn_ctx->chn_jobs;
	mutex_lock(&jobs->lock);
	if (!jobs->inited){
		mutex_unlock(&jobs->lock);
		TRACE_VO(DBG_NOTICE, "layer(%d) chn(%d) jobs not initialized yet.\n",
				chn.dev_id, chn.chn_id);
		return;
	}
	if (FIFO_FULL(&jobs->waitq)) {
		struct vb_s *vb_old = NULL;
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) waitq full, drop frame.\n",
			     chn.dev_id, chn.chn_id);
		FIFO_POP(&jobs->waitq, &vb_old);
		atomic_long_fetch_and(~BIT(chn.mod_id), &vb_old->mod_ids);
		vb_release_block((vb_blk)vb_old);
	} else {
		chn_ctx->frame_num++;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	mutex_unlock(&jobs->lock);
	TRACE_VO(DBG_INFO, "layer(%d) chn(%d) push vb(0x%llx).\n",
		 chn.dev_id, chn.chn_id, vb->phy_addr);

	atomic_long_fetch_or(BIT(chn.mod_id), &vb->mod_ids);
	g_vo_ctx->layer_ctx[chn.dev_id].is_layer_update = true;
	vfree(gdc_param);
}

static int _mesh_gdc_do_op_cb(enum gdc_usage usage, const void *usage_param,
			      struct vb_s *vb_in, pixel_format_e pixformat, u64 mesh_addr,
			      u8 sync_io, void *cb_param, u32 cb_param_size,
			      mod_id_e mod_id, rotation_e rotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.vb_in = vb_in;
	cfg.pix_format = pixformat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.cb_param = cb_param;
	cfg.cb_param_size = cb_param_size;
	cfg.rotation = rotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}


static int _vo_get_chn_buffers(struct vo_layer_ctx *layer_ctx, vb_blk *blk)
{
	vo_chn chn;
	struct vo_chn_ctx *chn_ctx;
	struct vb_jobs_t *jobs;
	struct vb_s *old_workq = NULL;
	struct vb_s *new_workq = NULL;
	struct vb_s *vb;
	int chn_num = 0;
	vo_dev dev = layer_ctx->bind_dev_id;
	mmf_chn_s mmf_chn = {.mod_id = ID_VO, .dev_id = dev, .chn_id = 0};

	for (chn = 0; chn < VO_MAX_CHN_NUM; ++chn) {
		chn_ctx = &layer_ctx->chn_ctx[chn];
		jobs = &chn_ctx->chn_jobs;

		if (!chn_ctx->is_chn_enable)
			continue;
		if (chn_ctx->hide)
			continue;
		if (chn_ctx->pause) {
			mutex_lock(&jobs->lock);
			if (!FIFO_EMPTY(&jobs->waitq) && chn_ctx->refresh) {
				if (!FIFO_EMPTY(&jobs->workq))
					FIFO_POP(&jobs->workq, &old_workq);

				FIFO_POP(&jobs->waitq, &vb);
				FIFO_PUSH(&jobs->workq, vb);

				FIFO_GET_FRONT(&jobs->workq, &new_workq);
				blk[chn] = (vb_blk)new_workq;
				if (old_workq) {
					vb_release_block((vb_blk)old_workq);
					old_workq = NULL;
				}
				chn_num++;
				chn_ctx->refresh = false;
			} else if (!FIFO_EMPTY(&jobs->workq)) {
				FIFO_GET_FRONT(&jobs->workq, &vb);
				blk[chn] = (vb_blk)vb;
				chn_num++;
			}
			mutex_unlock(&jobs->lock);
			continue;
		} else if (chn_ctx->step) {
			mutex_lock(&jobs->lock);
			if (!FIFO_EMPTY(&jobs->waitq) && chn_ctx->step_trigger) {
				if (!FIFO_EMPTY(&jobs->workq))
					FIFO_POP(&jobs->workq, &old_workq);

				FIFO_POP(&jobs->waitq, &vb);
				FIFO_PUSH(&jobs->workq, vb);

				FIFO_GET_FRONT(&jobs->workq, &new_workq);
				blk[chn] = (vb_blk)new_workq;
				if (old_workq) {
					vb_release_block((vb_blk)old_workq);
					old_workq = NULL;
				}
				chn_num++;
				chn_ctx->step_trigger = false;
			} else if (!FIFO_EMPTY(&jobs->workq)) {
				FIFO_GET_FRONT(&jobs->workq, &vb);
				blk[chn] = (vb_blk)vb;
				chn_num++;
			}
			mutex_unlock(&jobs->lock);
			continue;
		}

		mutex_lock(&jobs->lock);
		if (FIFO_EMPTY(&jobs->workq) && FIFO_EMPTY(&jobs->waitq)) {
			mutex_unlock(&jobs->lock);
			continue;
		}

		if (!FIFO_EMPTY(&jobs->waitq)) {
			if (!FIFO_EMPTY(&jobs->workq))
				FIFO_POP(&jobs->workq, &old_workq);

			FIFO_POP(&jobs->waitq, &vb);
			FIFO_PUSH(&jobs->workq, vb);
		}
		mutex_unlock(&jobs->lock);

		FIFO_GET_FRONT(&jobs->workq, &new_workq);
		blk[chn] = (vb_blk)new_workq;
		if (old_workq) {
			chn_ctx->predone_pts = old_workq->buf.pts;
			mmf_chn.chn_id = chn;
			if (chn_ctx->chn_attr.depth)
				vo_snap(mmf_chn, &chn_ctx->chn_jobs, (vb_blk)old_workq);
			vb_release_block((vb_blk)old_workq);
			old_workq = NULL;
		}
		chn_num++;
	}

	layer_ctx->is_layer_update = false;

	return chn_num;
}

void vo_stitch_wakeup(void *data)
{
	struct vo_stitch_cb_data *stitch_data = (struct vo_stitch_cb_data *)data;

	TRACE_VO(DBG_INFO, "vo stitch wakeup.\n");
	stitch_data->flag = 1;
	wake_up(&stitch_data->wait);
}

void vo_sort_chn_priority(int *priority, u32 length, int *index)
{
	int i, j, t1, t2;

	for (j = 0; j < length; j++) {
		for (i = 0; i < (length - 1 - j); i++) {
			if (priority[i] > priority[i + 1]) {
				t2 = priority[i];
				priority[i] = priority[i + 1];
				priority[i + 1] = t2;
				t1 = index[i];
				index[i] = index[i + 1];
				index[i + 1] = t1;
			}
		}
	}
}

static int layer_process(struct vo_layer_ctx *layer_ctx)
{
	int ret = 0;
	vo_dev dev = layer_ctx->bind_dev_id;
	struct vo_chn_ctx *chn_ctx;
	struct vpss_stitch_cfg stitch_cfg;
	struct stitch_dst_cfg *dst_cfg = &stitch_cfg.dst_cfg;
	struct stitch_chn_cfg *chn_cfg = NULL;
	unsigned long timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS);
	u8 i, n = 0;
	u32 chn_num;
	struct vo_stitch_cb_data stitch_data;
	vb_blk blk_next = VB_INVALID_HANDLE;
	vb_blk blk_out = VB_INVALID_HANDLE;
	struct vb_s *vb;
	size_s out_size = layer_ctx->layer_attr.img_size;
	vb_blk blks[VO_MAX_CHN_NUM] = { [0 ... VO_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	struct disp_buffer *disp_buf;
	unsigned long flags;
	vb_cal_config_s vb_cal_config;
	mmf_chn_s chn = {.mod_id = ID_VO, .dev_id = dev, .chn_id = 0};
	struct timespec64 time;
	u64 pts;
	bool is_wbc_match = 0;
	frame_rate_ctrl_s layer_frame_ctrl;
	vo_chn_zoom_ratio_e zoom_ratio;
	rect_s zoom_rect;
	bool is_need_zoom;
	int priority[VO_MAX_CHN_NUM] = { 0 };
	int index[VO_MAX_CHN_NUM] = { 0 };
	u8 j = 0;

	layer_ctx->frame_index++;
	layer_ctx->src_frame_num++;

	layer_frame_ctrl.src_frame_rate = layer_ctx->src_frame_rate;
	layer_frame_ctrl.dst_frame_rate = layer_ctx->layer_attr.frame_rate;

	if ((((layer_ctx->display_pts - layer_ctx->predone_pts) / 1000)
		> layer_ctx->toleration) && layer_ctx->toleration > 0) {
		layer_ctx->frame_index = 0;
	}

	if (!FRC_INVALID(layer_frame_ctrl) && (!vo_frame_ctrl(layer_ctx->frame_index, &layer_frame_ctrl)))
		layer_ctx->is_drop = true;

	if (layer_ctx->is_drop) {
		layer_ctx->is_drop = false;
		return 0;
	}

	if (!list_empty(&layer_ctx->list_done)) {
		//get a new vb
		common_getpicbufferconfig(layer_ctx->layer_attr.img_size.width,
					  layer_ctx->layer_attr.img_size.height,
					  layer_ctx->layer_attr.pixformat,
					  DATA_BITWIDTH_8,
					  COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

		blk_next = vb_get_block_with_id(VB_INVALID_POOLID, vb_cal_config.vb_size, ID_VO);
		if (blk_next == VB_INVALID_HANDLE) {
			TRACE_VO(DBG_ERR, "get vb block fail.\n");
			return -1;
		}

		spin_lock_irqsave(&layer_ctx->list_lock, flags);
		disp_buf = list_first_entry(&layer_ctx->list_done,
					    struct disp_buffer, list);
		list_del_init(&disp_buf->list);
		spin_unlock_irqrestore(&layer_ctx->list_lock, flags);

		blk_out = disp_buf->blk;
		disp_buf->blk = blk_next;

		vb = (struct vb_s *)blk_out;
		vb->buf.dev_num = dev;
		vb->buf.frm_num = layer_ctx->done_cnt;
		layer_ctx->predone_pts = vb->buf.pts;

		//for wbc get layer frame
		for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
			if (g_vo_ctx->wbc_ctx[i].is_wbc_enable &&
			    g_vo_ctx->wbc_ctx[i].wbc_src.src_type == VO_WBC_SRC_VIDEO &&
			    g_vo_ctx->wbc_ctx[i].wbc_src.src_id == layer_ctx->bind_dev_id) {
				if (layer_ctx->layer_attr.depth) {
					vb_done_handler(chn, CHN_TYPE_OUT, &layer_ctx->layer_jobs, blk_out);
					g_vo_ctx->wbc_ctx[i].frame_num++;
				} else {
					vb_release_block(blk_out);
				}
				is_wbc_match = true;
			}
		}

		//for get screen frame
		if (!is_wbc_match) {
			if (layer_ctx->layer_attr.depth)
				vo_snap(chn, &layer_ctx->layer_jobs, blk_out);
			vb_release_block(blk_out);
		}

		layer_ctx->done_cnt++;

	} else {
		TRACE_VO(DBG_ERR, "layer(%d) list_done empty!.\n",
			 layer_ctx->layer);
		return -1;
	}

	mutex_lock(&layer_ctx->layer_lock);
	chn_num = _vo_get_chn_buffers(layer_ctx, blks);
	if (!chn_num) {
		mutex_unlock(&layer_ctx->layer_lock);
		goto err0;
	}

	chn_cfg = vmalloc(sizeof(*chn_cfg) * chn_num);
	if (!chn_cfg) {
		TRACE_VO(DBG_ERR, "vmalloc fail.\n");
		mutex_unlock(&layer_ctx->layer_lock);
		goto err0;
	}

	//Chn priority ctrl
	for (i = 0; i < VO_MAX_CHN_NUM; i++) {
		if (blks[i] == VB_INVALID_HANDLE)
			continue;
		chn_ctx = &layer_ctx->chn_ctx[i];
		priority[j] = chn_ctx->chn_attr.priority;
		index[j] = i;
		j++;
	}

	vo_sort_chn_priority(priority, chn_num, index);

	for (i = 0; i < chn_num; i++) {
		vb = (struct vb_s *)blks[index[i]];
		chn_ctx = &layer_ctx->chn_ctx[index[i]];

		TRACE_VO(DBG_INFO, "layer(%d) chn(%d) get vb(0x%llx) frm_num(%u).\n",
			 layer_ctx->layer, index[i], vb->phy_addr, vb->buf.frm_num);

		chn_ctx->display_pts = vb->buf.pts;

		chn_cfg[n].pixelformat = vb->buf.pixel_format;
		chn_cfg[n].bytesperline[0] = vb->buf.stride[0];
		chn_cfg[n].bytesperline[1] = vb->buf.stride[1];
		chn_cfg[n].addr[0] = vb->buf.phy_addr[0];
		chn_cfg[n].addr[1] = vb->buf.phy_addr[1];
		chn_cfg[n].addr[2] = vb->buf.phy_addr[2];
		chn_cfg[n].src_size.width = vb->buf.size.width;
		chn_cfg[n].src_size.height = vb->buf.size.height;
		chn_ctx->src_width = vb->buf.size.width;
		chn_ctx->src_height = vb->buf.size.height;

		//Zoom
		if (chn_ctx->chn_zoom_attr.zoom_type == VO_CHN_ZOOM_IN_RECT) {
			zoom_rect = chn_ctx->chn_zoom_attr.rect;
			is_need_zoom = zoom_rect.x || zoom_rect.y || zoom_rect.width || zoom_rect.height;
		} else if (chn_ctx->chn_zoom_attr.zoom_type == VO_CHN_ZOOM_IN_RATIO) {
			zoom_ratio = chn_ctx->chn_zoom_attr.zoom_ratio;
			is_need_zoom = zoom_ratio.x_ratio || zoom_ratio.y_ratio ||
				zoom_ratio.width_ratio || zoom_ratio.height_ratio;
		} else {
			is_need_zoom = false;
		}

		if (chn_ctx->chn_zoom_attr.zoom_type == VO_CHN_ZOOM_IN_RECT && is_need_zoom) {
			chn_cfg[n].rect_crop.left = vb->buf.offset_left + zoom_rect.x;
			chn_cfg[n].rect_crop.top = vb->buf.offset_top + zoom_rect.y;
			chn_cfg[n].rect_crop.width = zoom_rect.width;
			chn_cfg[n].rect_crop.height = zoom_rect.height;

			if (zoom_rect.x < 0) {
				chn_cfg[n].rect_crop.left = vb->buf.offset_left;
				chn_cfg[n].rect_crop.width = (zoom_rect.width - ABS(zoom_rect.x));
			}

			if (chn_cfg[n].rect_crop.left + zoom_rect.width > chn_cfg[n].src_size.width)
				chn_cfg[n].rect_crop.width = chn_cfg[n].src_size.width - chn_cfg[n].rect_crop.left;

			if (zoom_rect.y < 0) {
				chn_cfg[n].rect_crop.top = vb->buf.offset_top;
				chn_cfg[n].rect_crop.height = (zoom_rect.height - ABS(zoom_rect.y));
			}

			if (chn_cfg[n].rect_crop.top + zoom_rect.height > chn_cfg[n].src_size.height)
				chn_cfg[n].rect_crop.height = chn_cfg[n].src_size.height - chn_cfg[n].rect_crop.top;

			if (chn_cfg[n].rect_crop.width < VO_MIN_CHN_WIDTH ||
			    chn_cfg[n].rect_crop.height < VO_MIN_CHN_HEIGHT) {
				TRACE_VO(DBG_INFO, "rect_crop(%d %d) < 32 invalid.\n",
					 chn_cfg[n].rect_crop.width, chn_cfg[n].rect_crop.height);
					 chn_cfg[n].rect_crop.left = vb->buf.offset_left;
					 chn_cfg[n].rect_crop.top = vb->buf.offset_top;
					 chn_cfg[n].rect_crop.width = vb->buf.size.width -
					 vb->buf.offset_left - vb->buf.offset_right;
					 chn_cfg[n].rect_crop.height = vb->buf.size.height -
					 vb->buf.offset_top - vb->buf.offset_bottom;
			}
		} else if ((chn_ctx->chn_zoom_attr.zoom_type == VO_CHN_ZOOM_IN_RATIO) && is_need_zoom) {
			chn_cfg[n].rect_crop.left = vb->buf.offset_left +
				zoom_ratio.x_ratio * chn_cfg[n].src_size.width / 1000;
			chn_cfg[n].rect_crop.top = vb->buf.offset_top +
				zoom_ratio.y_ratio * chn_cfg[n].src_size.height / 1000;
			chn_cfg[n].rect_crop.width = (zoom_ratio.width_ratio *
				chn_cfg[n].src_size.width / 1000) & ~0x01;
			chn_cfg[n].rect_crop.height = (zoom_ratio.height_ratio *
				chn_cfg[n].src_size.height / 1000) & ~0x01;

			if (chn_cfg[n].rect_crop.left + chn_cfg[n].rect_crop.width > chn_cfg[n].src_size.width)
				chn_cfg[n].rect_crop.width = chn_cfg[n].src_size.width - chn_cfg[n].rect_crop.left;

			if (chn_cfg[n].rect_crop.top + chn_cfg[n].rect_crop.height > chn_cfg[n].src_size.height)
				chn_cfg[n].rect_crop.height = chn_cfg[n].src_size.height - chn_cfg[n].rect_crop.top;

			if (chn_cfg[n].rect_crop.width < VO_MIN_CHN_WIDTH ||
			    chn_cfg[n].rect_crop.height < VO_MIN_CHN_HEIGHT) {
				TRACE_VO(DBG_INFO, "rect_crop(%d %d) < 32 invalid.\n",
					 chn_cfg[n].rect_crop.width, chn_cfg[n].rect_crop.height);
					 chn_cfg[n].rect_crop.left = vb->buf.offset_left;
					 chn_cfg[n].rect_crop.top = vb->buf.offset_top;
					 chn_cfg[n].rect_crop.width = vb->buf.size.width -
					 vb->buf.offset_left - vb->buf.offset_right;
					 chn_cfg[n].rect_crop.height = vb->buf.size.height -
					 vb->buf.offset_top - vb->buf.offset_bottom;
			}
		} else {
			chn_cfg[n].rect_crop.left = vb->buf.offset_left;
			chn_cfg[n].rect_crop.top = vb->buf.offset_top;
			chn_cfg[n].rect_crop.width = vb->buf.size.width - vb->buf.offset_left - vb->buf.offset_right;
			chn_cfg[n].rect_crop.height = vb->buf.size.height - vb->buf.offset_top - vb->buf.offset_bottom;
		}

		//AspectRatio
		if (chn_ctx->chn_param.aspect_ratio.mode == ASPECT_RATIO_AUTO) {
			size_s in, out;
			rect_s rect;

			in.width = chn_cfg[n].rect_crop.width;
			in.height = chn_cfg[n].rect_crop.height;
			out.width = chn_ctx->chn_attr.rect.width;
			out.height = chn_ctx->chn_attr.rect.height;
			rect = aspect_ratio_resize(in, out);
			chn_cfg[n].window.rect_in.width = rect.width & ~0x01;
			chn_cfg[n].window.rect_in.height = rect.height & ~0x01;
			chn_cfg[n].window.rect_in.left = (chn_ctx->chn_attr.rect.x + rect.x) & ~0x01;
			chn_cfg[n].window.rect_in.top = (chn_ctx->chn_attr.rect.y + rect.y) & ~0x01;
			if (chn_cfg[n].window.rect_in.width < VO_MIN_CHN_WIDTH ||
			    chn_cfg[n].window.rect_in.height < VO_MIN_CHN_HEIGHT) {
				TRACE_VO(DBG_INFO, "rect_in(%d %d) < 32 invalid.\n",
					 chn_cfg[n].window.rect_in.width, chn_cfg[n].window.rect_in.height);
					 chn_cfg[n].window.rect_in.width = chn_ctx->chn_attr.rect.width & ~0x01;
					 chn_cfg[n].window.rect_in.height = chn_ctx->chn_attr.rect.height & ~0x01;
					 chn_cfg[n].window.rect_in.left = chn_ctx->chn_attr.rect.x & ~0x01;
					 chn_cfg[n].window.rect_in.top = chn_ctx->chn_attr.rect.y & ~0x01;
			}
		} else if (chn_ctx->chn_param.aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
			rect_s rect = chn_ctx->chn_param.aspect_ratio.video_rect;

			chn_cfg[n].window.rect_in.width = rect.width & ~0x01;
			chn_cfg[n].window.rect_in.height = rect.height & ~0x01;
			chn_cfg[n].window.rect_in.left = (chn_ctx->chn_attr.rect.x + rect.x) & ~0x01;
			chn_cfg[n].window.rect_in.top = (chn_ctx->chn_attr.rect.y + rect.y) & ~0x01;
			if (chn_cfg[n].window.rect_in.width < VO_MIN_CHN_WIDTH ||
			    chn_cfg[n].window.rect_in.height < VO_MIN_CHN_HEIGHT) {
				TRACE_VO(DBG_INFO, "rect_in(%d %d) < 32 invalid.\n",
					 chn_cfg[n].window.rect_in.width, chn_cfg[n].window.rect_in.height);
					 chn_cfg[n].window.rect_in.width = chn_ctx->chn_attr.rect.width & ~0x01;
					 chn_cfg[n].window.rect_in.height = chn_ctx->chn_attr.rect.height & ~0x01;
					 chn_cfg[n].window.rect_in.left = chn_ctx->chn_attr.rect.x & ~0x01;
					 chn_cfg[n].window.rect_in.top = chn_ctx->chn_attr.rect.y & ~0x01;
			}
		} else {
			chn_cfg[n].window.rect_in.width = chn_ctx->chn_attr.rect.width & ~0x01;
			chn_cfg[n].window.rect_in.height = chn_ctx->chn_attr.rect.height & ~0x01;
			chn_cfg[n].window.rect_in.left = chn_ctx->chn_attr.rect.x & ~0x01;
			chn_cfg[n].window.rect_in.top = chn_ctx->chn_attr.rect.y & ~0x01;
		}

		if (chn_ctx->chn_param.aspect_ratio.enable_bgcolor) {
			chn_cfg[n].window.bgcolor[0] = (chn_ctx->chn_param.aspect_ratio.bgcolor >> 16) & 0xff; //R
			chn_cfg[n].window.bgcolor[1] = (chn_ctx->chn_param.aspect_ratio.bgcolor >> 8) & 0xff; //G
			chn_cfg[n].window.bgcolor[2] = chn_ctx->chn_param.aspect_ratio.bgcolor & 0xff; //B
		} else {
			chn_cfg[n].window.bgcolor[0] = 0; //R
			chn_cfg[n].window.bgcolor[1] = 0; //G
			chn_cfg[n].window.bgcolor[2] = 0; //B
		}

		chn_cfg[n].window.rect_out.width = chn_ctx->chn_attr.rect.width & ~0x01;
		chn_cfg[n].window.rect_out.height = chn_ctx->chn_attr.rect.height & ~0x01;
		chn_cfg[n].window.rect_out.left = chn_ctx->chn_attr.rect.x & ~0x01;
		chn_cfg[n].window.rect_out.top = chn_ctx->chn_attr.rect.y & ~0x01;

		//Border
		if (chn_ctx->chn_border_attr.enable) {
			chn_cfg[n].window.top_width = chn_ctx->chn_border_attr.border.top_width;
			chn_cfg[n].window.bottom_width = chn_ctx->chn_border_attr.border.bottom_width;
			chn_cfg[n].window.left_width = chn_ctx->chn_border_attr.border.left_width;
			chn_cfg[n].window.right_width = chn_ctx->chn_border_attr.border.right_width;
			chn_cfg[n].window.border_color[0] = (chn_ctx->chn_border_attr.border.color >> 16) & 0xff; //R
			chn_cfg[n].window.border_color[1] = (chn_ctx->chn_border_attr.border.color >> 8) & 0xff; //G
			chn_cfg[n].window.border_color[2] = chn_ctx->chn_border_attr.border.color & 0xff; //B
		} else {
			chn_cfg[n].window.top_width = 0;
			chn_cfg[n].window.bottom_width = 0;
			chn_cfg[n].window.left_width = 0;
			chn_cfg[n].window.right_width = 0;
			chn_cfg[n].window.border_color[0] = 0; //R
			chn_cfg[n].window.border_color[1] = 0; //G
			chn_cfg[n].window.border_color[2] = 0; //B
		}

		chn_cfg[n].window.flip = chn_ctx->chn_mirror;

		TRACE_VO(DBG_INFO, "layer(%d) pixelformat(%d) bytesperline(%d %d) addr(%llx, %llx, %llx) "
			 "src_size(%d %d ) crop(%d %d %d %d) rect_dst_in(%d %d %d %d) rect_dst_out(%d %d %d %d) "
			 "border(%d %d %d %d) mirror(%d).\n",
			 layer_ctx->layer, chn_cfg[n].pixelformat,
			 chn_cfg[n].bytesperline[0], chn_cfg[n].bytesperline[1],
			 chn_cfg[n].addr[0], chn_cfg[n].addr[1], chn_cfg[n].addr[2],
			 chn_cfg[n].src_size.width, chn_cfg[n].src_size.height,
			 chn_cfg[n].rect_crop.left, chn_cfg[n].rect_crop.top,
			 chn_cfg[n].rect_crop.width, chn_cfg[n].rect_crop.height,
			 chn_cfg[n].window.rect_in.left, chn_cfg[n].window.rect_in.top,
			 chn_cfg[n].window.rect_in.width, chn_cfg[n].window.rect_in.height,
			 chn_cfg[n].window.rect_out.left, chn_cfg[n].window.rect_out.top,
			 chn_cfg[n].window.rect_out.width, chn_cfg[n].window.rect_out.height,
			 chn_cfg[n].window.top_width, chn_cfg[n].window.bottom_width,
			 chn_cfg[n].window.left_width, chn_cfg[n].window.right_width,
			 chn_cfg[n].window.flip);
		n++;
	}
	mutex_unlock(&layer_ctx->layer_lock);

	vb = (struct vb_s *)blk_next;
	base_get_frame_info(layer_ctx->layer_attr.pixformat
			   , out_size
			   , &vb->buf
			   , vb_handle2phys_addr(blk_next)
			   , DEFAULT_ALIGN);

	dst_cfg->bytesperline[0] = vb->buf.stride[0];
	dst_cfg->bytesperline[1] = vb->buf.stride[1];
	dst_cfg->addr[0] = vb->buf.phy_addr[0];
	dst_cfg->addr[1] = vb->buf.phy_addr[1];
	dst_cfg->addr[2] = vb->buf.phy_addr[2];
	dst_cfg->pixelformat = vb->buf.pixel_format;
	dst_cfg->color[0] = 0;
	dst_cfg->color[1] = 0;
	dst_cfg->color[2] = 0;
	dst_cfg->dst_size.width = out_size.width;
	dst_cfg->dst_size.height = out_size.height;

	TRACE_VO(DBG_INFO,
		 "Dst layer(%d) pixelformat(%d) bytesperline(%d %d)"
		 "addr(%llx, %llx, %llx) rect_dst_out(%d %d)",
		 layer_ctx->layer, dst_cfg->pixelformat,
		 dst_cfg->bytesperline[0], dst_cfg->bytesperline[1],
		 dst_cfg->addr[0], dst_cfg->addr[1], dst_cfg->addr[2],
		 dst_cfg->dst_size.width, dst_cfg->dst_size.height);

	init_waitqueue_head(&stitch_data.wait);
	stitch_data.flag = 0;

	stitch_cfg.num = chn_num;
	stitch_cfg.chn_cfg = chn_cfg;
	stitch_cfg.data = (void *)&stitch_data;
	stitch_cfg.job_cb = vo_stitch_wakeup;

	ret = _vo_stitch_call_vpss(&stitch_cfg);
	if (ret) {
		TRACE_VO(DBG_ERR, "_vo_stitch_call_vpss fail.\n");
		goto err1;
	}

	ret = wait_event_timeout(stitch_data.wait, stitch_data.flag, timeout);
	if (ret < 0) {
		TRACE_VO(DBG_ERR, "-ERESTARTSYS!.\n");
	} else if (ret == 0) {
		TRACE_VO(DBG_ERR, "dev(%d) stitch timeout.\n", dev);
	} else {

		for (i = 0; i < disp_buf->buf.length; i++) {
			disp_buf->buf.planes[i].addr = vb->buf.phy_addr[i];
			disp_buf->buf.planes[i].bytesused = vb->buf.stride[i];
		}

		ktime_get_ts64(&time);
		pts = timespec64_to_ns(&time);
		do_div(pts, 1000);
		vb->buf.pts = pts;

		spin_lock_irqsave(&layer_ctx->list_lock, flags);
		list_add_tail(&disp_buf->list, &layer_ctx->list_wait);
		spin_unlock_irqrestore(&layer_ctx->list_lock, flags);
		layer_ctx->frame_num++;

		TRACE_VO(DBG_INFO, "layer(%d) add buffer(0x%llx) to wait list.\n",
			 layer_ctx->layer, vb->phy_addr);

		vfree(chn_cfg);

		return 0;
	}

err1:
	vfree(chn_cfg);

err0:
	spin_lock_irqsave(&layer_ctx->list_lock, flags);
	list_add_tail(&disp_buf->list, &layer_ctx->list_done);
	spin_unlock_irqrestore(&layer_ctx->list_lock, flags);

	return -1;
}

//Wbc function
void vo_wbc_rdy_buf_queue(struct vo_wbc_ctx *wbc_ctx, struct wbc_buffer *qbuf)
{
	unsigned long flags;
	spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
	list_add_tail(&qbuf->list, &wbc_ctx->qbuf_list);
	++wbc_ctx->qbuf_num;
	spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);
}

struct wbc_buffer *vo_wbc_rdy_buf_next(struct vo_wbc_ctx *wbc_ctx)
{
	unsigned long flags;
	struct wbc_buffer *buf = NULL;

	spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
	if (!list_empty(&wbc_ctx->qbuf_list))
		buf = list_first_entry(&wbc_ctx->qbuf_list, struct wbc_buffer, list);
	spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);

	return buf;
}

int vo_wbc_rdy_buf_empty(struct vo_wbc_ctx *wbc_ctx)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
	empty = (wbc_ctx->qbuf_num == 0);
	spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);

	return empty;
}

void vo_wbc_rdy_buf_pop(struct vo_wbc_ctx *wbc_ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
	wbc_ctx->qbuf_num--;
	spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);
}

void vo_wbc_rdy_buf_remove(struct vo_wbc_ctx *wbc_ctx)
{
	unsigned long flags;
	struct wbc_buffer *buf = NULL;

	spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
	if (!list_empty(&wbc_ctx->qbuf_list)) {
		buf = list_first_entry(&wbc_ctx->qbuf_list, struct wbc_buffer, list);
		list_del_init(&buf->list);
		kfree(buf);
	}
	spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);
}

void vo_wbc_dqbuf_list(struct vo_wbc_ctx *wbc_ctx)
{
	unsigned long flags;
	struct wbc_buffer *buf;

	buf = kzalloc(sizeof(*buf), GFP_ATOMIC);
	if (buf == NULL) {
		TRACE_VO(DBG_ERR, "DQbuf kmalloc size(%zu) fail\n", sizeof(struct wbc_buffer));
		return;
	}

	spin_lock_irqsave(&wbc_ctx->dqbuf_lock, flags);
	list_add_tail(&buf->list, &wbc_ctx->dqbuf_list);
	spin_unlock_irqrestore(&wbc_ctx->dqbuf_lock, flags);
}

int vo_wbc_dqbuf(struct vo_wbc_ctx *wbc_ctx)
{
	unsigned long flags;
	struct wbc_buffer *buf = NULL;
	int ret = -1;

	spin_lock_irqsave(&wbc_ctx->dqbuf_lock, flags);
	if (!list_empty(&wbc_ctx->dqbuf_list)) {
		buf = list_first_entry(&wbc_ctx->dqbuf_list, struct wbc_buffer, list);
		list_del_init(&buf->list);
		kfree(buf);
		ret = 0;
	}
	spin_unlock_irqrestore(&wbc_ctx->dqbuf_lock, flags);

	return ret;
}

static void vo_wbc_submit(struct vo_wbc_ctx *wbc_ctx)
{
	struct wbc_buffer *wbc_qbuf = NULL;
	vo_dev dev;
	pixel_format_e pixformat;
	struct disp_odma_cfg *odma_cfg;
	struct vo_fmt *fmt;
	struct disp_cfg *disp_cfg;

	vo_wbc_rdy_buf_pop(wbc_ctx);
	wbc_qbuf = vo_wbc_rdy_buf_next(wbc_ctx);
	if (wbc_qbuf == NULL) {
		TRACE_VO(DBG_ERR, "vo_wbc_rdy_buf_next empty");
		return;
	}

	// TRACE_VO(DBG_INFO, "update wbc outbuf: 0x%llx\n",
	// 		b->buf.addr[0]);

	// submit to odma hw
	dev = wbc_ctx->wbc_src.src_id;
	odma_cfg = disp_odma_get_cfg(dev);
	pixformat = wbc_ctx->wbc_attr.pixformat;
	fmt = vo_sdk_get_format(pixformat);

	odma_cfg->fmt = fmt->fmt;
	odma_cfg->burst = 1;
	odma_cfg->mem.addr0 = wbc_qbuf->buf.addr[0];
	odma_cfg->mem.addr1 = wbc_qbuf->buf.addr[1];
	odma_cfg->mem.addr2 = wbc_qbuf->buf.addr[2];
	odma_cfg->mem.pitch_y = wbc_qbuf->buf.pitch_y;
	odma_cfg->mem.pitch_c = wbc_qbuf->buf.pitch_c;
	odma_cfg->mem.start_x = 0;
	odma_cfg->mem.start_y = 0;
	odma_cfg->mem.width = wbc_qbuf->buf.width;
	odma_cfg->mem.height = wbc_qbuf->buf.height;

	disp_cfg = disp_get_cfg(dev);

	if (disp_cfg[dev].out_csc >= DISP_CSC_601_LIMIT_RGB2YUV &&
	    disp_cfg[dev].out_csc <= DISP_CSC_709_FULL_RGB2YUV) {
		if (IS_YUV_FMT(odma_cfg->fmt)) {
			odma_cfg->csc = DISP_CSC_NONE;
		} else {
			odma_cfg->csc = disp_cfg[dev].out_csc - 4;
		}
	} else {
		if (IS_YUV_FMT(odma_cfg->fmt)) {
			//Do we need to open it up to customers
			odma_cfg->csc = DISP_CSC_601_FULL_RGB2YUV;
		} else {
			odma_cfg->csc = DISP_CSC_NONE;
		}
	}

	disp_odma_set_cfg(dev, odma_cfg);
}

static int wbc_process(struct vo_wbc_ctx *wbc_ctx)
{
	int ret = -1;
	vo_dev dev = wbc_ctx->wbc_src.src_id;
	mmf_chn_s chn = {.mod_id = ID_VO, .dev_id = dev, .chn_id = 0};
	vb_blk blk;
	struct vb_s *vb;
	struct timespec64 time;
	u64 pts;

	ret = vo_wbc_dqbuf(wbc_ctx);
	if (ret == -1) {
		TRACE_VO(DBG_ERR, "dev(%d) vo_wbc_dqbuf failed\n", dev);
		return ret;
	}

	// prev frame odma done
	ret = vb_dqbuf(chn, &wbc_ctx->wbc_jobs, &blk);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "dev(%d) wbc vb_dqbuf failed\n", dev);
		return ret;
	}

	vb = (struct vb_s *)blk;
	ktime_get_ts64(&time);
	pts = timespec64_to_ns(&time);
	do_div(pts, 1000);
	vb->buf.dev_num = dev;
	vb->buf.pts = pts;
	vb->buf.frm_num = wbc_ctx->done_cnt;

	wbc_ctx->done_cnt++;
	wbc_ctx->frame_num++;
	// push to vpss/venc/vo
	vb_done_handler(chn, CHN_TYPE_OUT, &wbc_ctx->wbc_jobs, blk);

	// get another vb for next frame
	ret = vo_wbc_qbuf(wbc_ctx);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "vo_wbc_qbuf error (%d)", ret);
		return ret;
	}

	return 0;
}

static int disp_event_handler(void *arg)
{
	struct vo_layer_ctx *layer_ctx = (struct vo_layer_ctx *)arg;
	unsigned long timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS);
	int ret;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(layer_ctx->wq, layer_ctx->event ||
						       kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret)
			continue;

		//TRACE_VO(DBG_INFO, "[%d] thread run.\n", layer_ctx->layer);
		layer_ctx->event = 0;
		if (layer_ctx->is_layer_update)
			layer_process(layer_ctx);
	}

	return 0;
}

static int wbc_event_handler(void *arg)
{
	struct vo_wbc_ctx *wbc_ctx = (struct vo_wbc_ctx *)arg;
	unsigned long timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS);
	int ret;


	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(wbc_ctx->wq, wbc_ctx->event ||
						       kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret)
			continue;

		wbc_ctx->event = 0;
		wbc_process(wbc_ctx);
	}

	return 0;
}

int vo_create_thread(vo_layer layer)
{
	struct sched_param param;
	char task_name[32];
	struct vo_layer_ctx *layer_ctx = &g_vo_ctx->layer_ctx[layer];

	snprintf(task_name, sizeof(task_name), "task_vo_layer%d", layer);

	layer_ctx->thread = kthread_create(disp_event_handler, (void *)layer_ctx, task_name);

	if (IS_ERR(layer_ctx->thread)) {
		TRACE_VO(DBG_ERR, "Unable to create %s.\n", task_name);
		return -1;
	}

	param.sched_priority = MAX_USER_RT_PRIO - 10;
	sched_setscheduler(layer_ctx->thread, SCHED_FIFO, &param);
	wake_up_process(layer_ctx->thread);

	TRACE_VO(DBG_INFO, "[layer%d]create thread.\n", layer);

	return 0;
}

int vo_destroy_thread(vo_layer layer)
{
	struct vo_layer_ctx *layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (kthread_stop(layer_ctx->thread))
		TRACE_VO(DBG_ERR, "fail to stop disp thread.\n");

	layer_ctx->thread = NULL;
	disp_enable_window_bgcolor(layer_ctx->bind_dev_id, true);
	TRACE_VO(DBG_INFO, "[layer%d]destroy thread.\n", layer);

	return 0;
}

int vo_wbc_create_thread(vo_wbc wbc_dev)
{
	struct sched_param param;
	char task_name[32];
	struct vo_wbc_ctx *wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];

	snprintf(task_name, sizeof(task_name), "task_vo_wbc%d", wbc_dev);

	wbc_ctx->thread = kthread_create(wbc_event_handler, (void *)wbc_ctx, task_name);

	if (IS_ERR(wbc_ctx->thread)) {
		TRACE_VO(DBG_ERR, "Unable to create %s.\n", task_name);
		return -1;
	}

	param.sched_priority = MAX_USER_RT_PRIO - 10;
	sched_setscheduler(wbc_ctx->thread, SCHED_FIFO, &param);
	wake_up_process(wbc_ctx->thread);

	TRACE_VO(DBG_INFO, "[wbc_dev%d]create thread.\n", wbc_dev);

	return 0;
}

int vo_wbc_destroy_thread(vo_wbc wbc_dev)
{
	struct vo_wbc_ctx *wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];

	if (kthread_stop(wbc_ctx->thread))
		TRACE_VO(DBG_ERR, "fail to stop wbc thread.\n");

	wbc_ctx->thread = NULL;
	TRACE_VO(DBG_INFO, "[wbc_dev%d]destroy thread.\n", wbc_dev);

	return 0;
}

void vo_show_panttern(vo_dev dev)
{
	struct vo_core_dev *vdev = g_core_dev;

	if ((vdev->clk_vo[dev * 2]) && (!__clk_is_enabled(vdev->clk_vo[dev * 2])))
		clk_prepare_enable(vdev->clk_vo[dev * 2]);

	if(!(g_vo_ctx->dev_ctx[dev].pub_attr.intf_type & (VO_INTF_MIPI | VO_INTF_LVDS | VO_INTF_HDMI))) {
		if ((vdev->clk_vo[dev * 2 + 1]) && (!__clk_is_enabled(vdev->clk_vo[dev * 2 + 1])))
			clk_prepare_enable(vdev->clk_vo[dev * 2 + 1]);
	}

	if(g_vo_ctx->dev_ctx[dev].pub_attr.intf_type & VO_INTF_LVDS) {
		if ((vdev->clk_lvds[dev]) && (!__clk_is_enabled(vdev->clk_lvds[dev])))
			clk_prepare_enable(vdev->clk_lvds[dev]);
	}

	disp_tgen_enable(dev, true);

	return;
}

int vo_start_streaming(vo_dev dev)
{
	int rc = 0;
	struct vo_core_dev *vdev = g_core_dev;

	if ((vdev->clk_vo[dev * 2]) && (!__clk_is_enabled(vdev->clk_vo[dev * 2])))
		clk_prepare_enable(vdev->clk_vo[dev * 2]);

	if(!(g_vo_ctx->dev_ctx[dev].pub_attr.intf_type & (VO_INTF_MIPI | VO_INTF_LVDS | VO_INTF_HDMI))) {
		if ((vdev->clk_vo[dev * 2 + 1]) && (!__clk_is_enabled(vdev->clk_vo[dev * 2 + 1])))
			clk_prepare_enable(vdev->clk_vo[dev * 2 + 1]);
	}

	if(g_vo_ctx->dev_ctx[dev].pub_attr.intf_type & VO_INTF_LVDS) {
		if ((vdev->clk_lvds[dev]) && (!__clk_is_enabled(vdev->clk_lvds[dev])))
			clk_prepare_enable(vdev->clk_lvds[dev]);
	}

	disp_enable_window_bgcolor(dev, true);

	vdev->vo_core[dev].align = DISP_ALIGNMENT;
	vdev->vo_core[dev].frame_number = 0;

	disp_tgen_enable(dev, true);

	atomic_set(&vdev->vo_core[dev].disp_streamon, 1);
	TRACE_VO(DBG_INFO, "[dev%d]start streaming.\n", dev);

	rc = devm_request_irq(vdev->dev, vdev->vo_core[dev].irq_num, vo_irq_handler, 0,
			      disp_irq_name[dev], (void *)&vdev->vo_core[dev]);
	if (rc) {
		TRACE_VO(DBG_ERR, "Failed to vo request irq\n");
		return -EAGAIN;
	}

	return rc;
}

int vo_stop_streaming(vo_dev dev)
{
	int rc = 0;
	struct vo_core_dev *vdev = g_core_dev;

	disp_enable_window_bgcolor(dev, true);
	disp_tgen_enable(dev, false);

	atomic_set(&vdev->vo_core[dev].disp_streamon, 0);
	memset(&vdev->vo_core[dev].compose_out, 0, sizeof(vdev->vo_core[dev].compose_out));
	TRACE_VO(DBG_INFO, "[dev%d]stop streaming.\n", dev);
	TRACE_VO(DBG_DEBUG, "end...\n");

	devm_free_irq(vdev->dev, vdev->vo_core[dev].irq_num,
		      (void *)&vdev->vo_core[dev]);

	if ((vdev->clk_vo[dev * 2]) && __clk_is_enabled(vdev->clk_vo[dev * 2]))
		clk_disable_unprepare(vdev->clk_vo[dev * 2]);


	if(!(g_vo_ctx->dev_ctx[dev].pub_attr.intf_type & (VO_INTF_MIPI | VO_INTF_LVDS | VO_INTF_HDMI))) {
		if ((vdev->clk_vo[dev * 2 + 1]) && __clk_is_enabled(vdev->clk_vo[dev * 2 + 1]))
			clk_disable_unprepare(vdev->clk_vo[dev * 2 + 1]);
	}

	if(g_vo_ctx->dev_ctx[dev].pub_attr.intf_type & VO_INTF_LVDS) {
		if ((vdev->clk_lvds[dev]) && __clk_is_enabled(vdev->clk_lvds[dev]))
			clk_disable_unprepare(vdev->clk_lvds[dev]);
	}

	return rc;
}

static long _vo_s_ctrl(struct vo_core_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->id;
	long rc = 0;

	switch (id) {
	case VO_IOCTL_SDK_CTRL: {
		rc = vo_sdk_ctrl(vdev, p);
	}
	break;

	case VO_IOCTL_START_STREAMING: {
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		rc = vo_start_streaming(dev);
		if (rc != 0)
			TRACE_VO(DBG_ERR, "Failed to vo start streaming\n");

	}
	break;

	case VO_IOCTL_STOP_STREAMING: {
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		rc = vo_stop_streaming(dev);
		if (rc != 0)
			TRACE_VO(DBG_ERR, "Failed to vo stop streaming\n");

	}
	break;

	case VO_IOCTL_SET_DV_TIMINGS: {
		struct vo_dv_timings dv_timings;
		struct disp_timing disp_timing;
		vo_dev dev = p->reserved[0];

		if (g_vo_ctx->dev_ctx[dev].pub_attr.intf_type == VO_INTF_MIPI) {
			TRACE_VO(DBG_INFO, "device(%d) timing should be setup by mipi-tx!\n", dev);
			break;
		}
		if (g_vo_ctx->dev_ctx[dev].pub_attr.intf_type == VO_INTF_HDMI) {
			TRACE_VO(DBG_INFO, "device(%d) timing should be setup by hdmi-tx!\n", dev);
			break;
		}

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		if (copy_from_user(&dv_timings, (void *)p->ptr, sizeof(struct vo_dv_timings))) {
			TRACE_VO(DBG_ERR, "Set DV timing copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		vdev->vo_core[dev].dv_timings = dv_timings;
		vdev->vo_core[dev].sink_rect.width = dv_timings.bt.width;
		vdev->vo_core[dev].sink_rect.height = dv_timings.bt.height;
		vdev->vo_core[dev].compose_out = vdev->vo_core[dev].sink_rect;
		TRACE_VO(DBG_DEBUG, "timing %d-%d\n", dv_timings.bt.width, dv_timings.bt.height);

		vo_fill_disp_timing(&disp_timing, &dv_timings.bt);
		disp_set_timing(dev, &disp_timing);
	}
	break;

	case VO_IOCTL_SEL_TGT_COMPOSE: {
		struct vo_rect area;
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		if (copy_from_user(&area, p->ptr, sizeof(area)) != 0) {
			TRACE_VO(DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		if (memcmp(&vdev->vo_core[dev].compose_out, &area, sizeof(area))) {
			struct disp_rect rect;

			rect.x = area.left;
			rect.y = area.top;
			rect.w = area.width;
			rect.h = area.height;

			TRACE_VO(DBG_DEBUG, "Compose Area (%d,%d,%d,%d)\n", rect.x, rect.y, rect.w, rect.h);
			rc = disp_set_rect(dev, rect);
		}
	}
	break;

	case VO_IOCTL_SEL_TGT_CROP: {
		struct disp_cfg *cfg;
		struct vo_rect area;
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		if (copy_from_user(&area, (void __user *)p->ptr, sizeof(struct vo_rect))) {
			TRACE_VO(DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		cfg = disp_get_cfg(dev);
		cfg->mem.start_x = area.left;
		cfg->mem.start_y = area.top;
		cfg->mem.width	 = area.width;
		cfg->mem.height  = area.height;

		TRACE_VO(DBG_INFO, "Crop Area (%d, %d, %d, %d)\n", cfg->mem.start_x,
			 cfg->mem.start_y, cfg->mem.width, cfg->mem.height);
		disp_set_mem(dev, &cfg->mem);
		vdev->vo_core[dev].crop_rect = area;
	}
	break;

	case VO_IOCTL_SET_CUSTOM_CSC: {
		struct disp_csc_matrix cfg;
		vo_layer layer = p->reserved[0];
		vo_dev dev = g_vo_ctx->layer_ctx[layer].bind_dev_id;

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		CHECK_STRUCT_SIZE(p->size, struct disp_csc_matrix);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct disp_csc_matrix))) {
			TRACE_VO(DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		_disp_set_in_csc(dev, &cfg);
	}
	break;

	case VO_IOCTL_PATTERN: {
		vo_layer dev = p->reserved[0];
		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		if (p->value >= VO_PAT_MAX) {
			TRACE_VO(DBG_ERR, "invalid disp-pattern(%d)\n", p->value);
			rc = -EFAULT;
			break;
		}

		disp_set_pattern(dev, patterns[p->value].type, patterns[p->value].color,
				 patterns[p->value].rgb);

		vo_show_panttern(dev);
	}
	break;

	case VO_IOCTL_FRAME_BGCOLOR: {
		u16 rgb[3];
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		if (copy_from_user(rgb, p->ptr, sizeof(rgb))) {
			TRACE_VO(DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		TRACE_VO(DBG_INFO, "Set Frame BG color (R,G,B) = (0x%x, 0x%x, 0x%x)\n",
			 rgb[1], rgb[2], rgb[3]);

		disp_set_frame_bgcolor(dev, rgb[0], rgb[1], rgb[2]);
	}
	break;

	case VO_IOCTL_WINDOW_BGCOLOR: {
		u16 rgb[3];
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		if (copy_from_user(rgb, p->ptr, sizeof(rgb))) {
			TRACE_VO(DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		TRACE_VO(DBG_INFO, "Set window BG color (R,G,B) = (0x%x, 0x%x, 0x%x)\n",
			 rgb[1], rgb[2], rgb[3]);

		disp_set_window_bgcolor(dev, rgb[0], rgb[1], rgb[2]);
	}
	break;

	case VO_IOCTL_ENABLE_WIN_BGCOLOR: {
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}
		vdev->vo_core[dev].bgcolor_enable = p->value;
		disp_enable_window_bgcolor(dev, p->value);
	}
	break;

	case VO_IOCTL_GAMMA_LUT_UPDATE: {
		int i = 0;
		struct disp_gamma_attr gamma_attr_sclr;
		vo_gamma_info_s gamma_attr;
		vo_dev dev = p->reserved[0];

		if (dev >= VO_MAX_DEV_NUM || dev < 0) {
			rc = -EINVAL;
			TRACE_VO(DBG_ERR, "Invalid vo device(%d)!\n", dev);
			break;
		}

		CHECK_STRUCT_SIZE(p->size, vo_gamma_info_s);

		if (copy_from_user(&gamma_attr, (void *)p->ptr, sizeof(gamma_attr))) {
			TRACE_VO(DBG_ERR, "gamma lut update copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		gamma_attr_sclr.enable = gamma_attr.enable;
		gamma_attr_sclr.pre_osd = gamma_attr.osd_apply;

		for (i = 0; i < DISP_GAMMA_NODE; ++i) {
			gamma_attr_sclr.table[i] = gamma_attr.value[i];
		}

		disp_gamma_ctrl(dev, gamma_attr_sclr.enable, gamma_attr_sclr.pre_osd);
		disp_gamma_lut_update(dev, gamma_attr_sclr.table, gamma_attr_sclr.table, gamma_attr_sclr.table);
	}
	break;

	default:
		break;
	}

	return rc;
}

static long _vo_g_ctrl(struct vo_core_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->id;
	long rc = 0;

	switch (id) {
	case VO_IOCTL_GET_DV_TIMINGS: {
		vo_dev dev = p->reserved[0];

		if (copy_to_user(p->ptr, &vdev->vo_core[dev].dv_timings, sizeof(struct vo_dv_timings))) {
			TRACE_VO(DBG_ERR, "VO_IOCTL_GET_DV_TIMINGS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GET_VLAYER_SIZE: {
		vo_dev dev = p->reserved[0];

		struct disp_timing *timing = disp_get_timing(dev);
		struct dsize {
			u32 width;
			u32 height;
		} vsize;

		vsize.width = timing->hfde_end - timing->hfde_start + 1;
		vsize.height = timing->vfde_end - timing->vfde_start + 1;

		if (copy_to_user(p->ptr, &vsize, sizeof(struct dsize))) {
			TRACE_VO(DBG_ERR, "VO_IOCTL_GET_VLAYER_SIZE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GET_INTF_TYPE: {
		vo_intf_type_e inft;
		vo_dev dev = p->reserved[0];

		inft = g_vo_ctx->dev_ctx[dev].pub_attr.intf_type;
		if (copy_to_user(p->ptr, &inft, sizeof(inft))) {
			TRACE_VO(DBG_ERR, "VO_IOCTL_GET_INTF_TYPE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GAMMA_LUT_READ: {
		int i = 0;
		vo_gamma_info_s gamma_attr;
		struct disp_gamma_attr gamma_attr_sclr;
		vo_dev dev = p->reserved[0];

		disp_gamma_lut_read(dev, &gamma_attr_sclr);

		gamma_attr.enable = gamma_attr_sclr.enable;
		gamma_attr.osd_apply = gamma_attr_sclr.pre_osd;
		gamma_attr.dev = dev;

		for (i = 0; i < DISP_GAMMA_NODE; ++i) {
			gamma_attr.value[i] = gamma_attr_sclr.table[i];
		}

		if (copy_to_user((void *)p->ptr, &gamma_attr, sizeof(vo_gamma_info_s))) {
			TRACE_VO(DBG_ERR, "VO_IOCTL_GAMMA_LUT_READ copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	default:
		break;
	}

	return rc;
}

long vo_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct vo_core_dev *vdev = file->private_data;
	int ret = 0;
	struct vo_ext_control p;

	CHECK_IOCTL_CMD(cmd, struct vo_ext_control);

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vo_ext_control)))
		return -EFAULT;

	switch (cmd) {
	case VO_IOC_G_CTRL:
	{
		ret = _vo_g_ctrl(vdev, &p);
		break;
	}
	case VO_IOC_S_CTRL:
	{
		ret = _vo_s_ctrl(vdev, &p);
		break;
	}
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct vo_ext_control)))
		return -EFAULT;

	return ret;
}


int vo_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	vo_dev dev;
	struct vo_core_dev *vdev;

	vdev = container_of(inode->i_cdev, struct vo_core_dev, cdev);
	file->private_data = vdev;
	if (!atomic_read(&dev_open_cnt)) {
		for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev) {
			atomic_set(&vdev->vo_core[dev].disp_streamon, 0);

			disp_reg_shadow_sel(dev, false);
			// if (!smooth[dev])
			disp_set_cfg(dev, disp_get_cfg(dev));
		}
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

void _vo_sdk_release(struct vo_core_dev *vdev)
{
	int i, j;

	for (i = 0; i < VO_MAX_WBC_NUM; i++) {
		if (g_vo_ctx->wbc_ctx[i].is_wbc_enable)
			vo_disable_wbc(i);
	}
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; i++) {
		if (g_vo_ctx->layer_ctx[i].is_layer_enable) {
			for (j = 0; j < VO_MAX_CHN_NUM; j++) {
				if (g_vo_ctx->layer_ctx[i].chn_ctx[j].is_chn_enable)
					vo_disable_chn(i, j);
			}
			vo_disablevideolayer(i);
		}
	}
	for (i = 0; i < VO_MAX_DEV_NUM; i++) {
		if (g_vo_ctx->dev_ctx[i].is_dev_enable)
			vo_disable(i);
	}
}

int vo_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	atomic_dec(&dev_open_cnt);
	if (!atomic_read(&dev_open_cnt)) {
		struct vo_core_dev *vdev;

		vdev = container_of(inode->i_cdev, struct vo_core_dev, cdev);
		_vo_sdk_release(vdev);
	}
	return ret;
}

u32 vo_poll(struct file *file, struct poll_table_struct *wait)
{
	return 0;
}

int vo_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	struct vo_core_dev *vdev = (struct vo_core_dev *)dev;
	int rc = -1;

	UNUSED(vdev);
	switch (cmd) {
	case VO_CB_GET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		vo_layer layer = attr->chn.dev_id;
		rgn_handle *handle = attr->hdls;
		rgn_type_e type = attr->type;

		TRACE_VO(DBG_INFO, "VO_CB_GET_RGN_HDLS\n");
		rc = vo_cb_get_rgn_hdls(layer, type, handle);
		if (rc != 0)
			TRACE_VO(DBG_ERR, "VO_CB_GET_RGN_HDLS failed.\n");

		break;
	}

	case VO_CB_SET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		vo_layer layer = attr->chn.dev_id;
		rgn_handle *handle = attr->hdls;
		rgn_type_e type = attr->type;

		TRACE_VO(DBG_INFO, "VO_CB_SET_RGN_HDLS\n");
		rc = vo_cb_set_rgn_hdls(layer, type, handle);
		if (rc != 0)
			TRACE_VO(DBG_ERR, "VO_CB_SET_RGN_HDLS failed.\n");

		break;
	}

	case VO_CB_SET_RGN_CFG:
	{
		struct _rgn_cfg_cb_param *attr = (struct _rgn_cfg_cb_param *)arg;
		struct rgn_cfg *rgn_cfg = &attr->rgn_cfg;
		vo_layer layer = attr->chn.dev_id;

		TRACE_VO(DBG_INFO, "VO_CB_SET_RGN_CFG\n");
		if (vo_cb_set_rgn_cfg(layer, rgn_cfg))
			TRACE_VO(DBG_ERR, "VO_CB_SET_RGN_CFG is failed.\n");

		rc = 0;
		break;
	}

	case VO_CB_GET_CHN_SIZE:
	{
		struct _rgn_chn_size_cb_param *param = (struct _rgn_chn_size_cb_param *)arg;
		vo_layer layer = param->chn.dev_id;

		TRACE_VO(DBG_INFO, "VO_CB_GET_CHN_SIZE\n");
		rc = vo_cb_get_chn_size(layer, &param->rect);
		if (rc != 0)
			TRACE_VO(DBG_ERR, "VO_CB_GET_CHN_SIZE failed\n");

		break;
	}

	case VO_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg = (struct ldc_op_done_cfg *)arg;

		_vo_gdc_callback(cfg->param, cfg->blk);
		rc = 0;
		break;
	}

	case VO_CB_QBUF_VO_GET_CHN_ROTATION:
	{
		struct vo_get_chnrotation_cfg *cfg = (struct vo_get_chnrotation_cfg *)arg;

		rc = vo_get_chnrotation(cfg->layer, cfg->chn, (rotation_e *)&cfg->rotation);
		break;
	}

	default:
		break;
	}

	return rc;
}

u64 timespec64_to_us(struct timespec64 *time_spec)
{
	return time_spec->tv_sec * 1000000L + time_spec->tv_nsec / 1000L;
}

void ddr_retrain(vo_dev dev, union disp_intr intr_status)
{
	#define DISP_FPS_CNT 10
	#define DISP_FPS_TABLE_CNT 6

	static struct timespec64 disp_frame_need_time[DISP_MAX_INST], last_overlap_time;
	static const u8 fps_table[DISP_FPS_TABLE_CNT] = {24, 25, 30, 48, 50, 60};
	static const u16 margin_table[DISP_FPS_TABLE_CNT] = {489, 460, 345, 129, 115, 57};
	static u8 disp_fps[DISP_MAX_INST], disp_fps_cnt[DISP_MAX_INST];
	static struct timespec64 disp_fps_time[DISP_MAX_INST];
	static u16 margin[DISP_MAX_INST];
	u64 disp_frame_need_time_us[DISP_MAX_INST], frame_gap_ms[DISP_MAX_INST];
	struct timespec64 cur_overlap_time;
	u8 i = 0;

	if (intr_status.b.disp_frame_end) {
		cur_overlap_time = disp_frame_need_time[dev] = ktime_to_timespec64(ktime_get());
		disp_frame_need_time_us[dev] = timespec64_to_us(&disp_frame_need_time[dev]);
		disp_frame_need_time_us[!dev] = timespec64_to_us(&disp_frame_need_time[!dev]);

		if (disp_fps_cnt[dev] < DISP_FPS_CNT) {
			if (!disp_fps_cnt[dev])
				disp_fps_time[dev] = disp_frame_need_time[dev];
			disp_fps_cnt[dev]++;
		} else if (disp_fps_cnt[dev] == DISP_FPS_CNT) {
			disp_fps_cnt[dev]++;
			frame_gap_ms[dev] = disp_frame_need_time_us[dev] / 1000L
							- timespec64_to_us(&disp_fps_time[dev]) / 1000L;
			disp_fps[dev] = (DISP_FPS_CNT * 10000 / frame_gap_ms[dev] + 5) / 10;
			pr_info("disp%d fps:%d\n", dev, disp_fps[dev]);
		}

		if (disp_frame_need_time_us[dev] > disp_frame_need_time_us[!dev]) {
			if (margin[dev] == 0 && disp_fps[!dev] != 0) {
				for (i = 0; i < DISP_FPS_TABLE_CNT; i++) {
					if (disp_fps[!dev] <= fps_table[i]
						|| (i == DISP_FPS_TABLE_CNT - 1)) {
						margin[dev] = margin_table[i];
						pr_info("disp%d ahead disp%d, margin time(%d)\n"
							, !dev, dev, margin[dev]);
						break;
					}
				}
			}
			if ((disp_frame_need_time_us[dev] - disp_frame_need_time_us[!dev]
				< margin[dev]) || !disp_check_tgen_enable(!dev)) {
				if (ddr_need_retrain())
					trigger_8051();
				if (timespec64_to_us(&cur_overlap_time)
					- timespec64_to_us(&last_overlap_time) > 900000L)
					pr_info("disp%d ahead disp%d, vblanking overlap. "
						"margin(%lld)us last overlap is (%lld)ms before.\n"
						, !dev, dev
						, disp_frame_need_time_us[dev] - disp_frame_need_time_us[!dev]
						, (timespec64_to_us(&cur_overlap_time)
							- timespec64_to_us(&last_overlap_time)) / 1000);
				last_overlap_time = cur_overlap_time;
			}
		}
	}
}
/*******************************************************
 *  Irq handlers
 ******************************************************/
static void irq_handler(struct vo_core *core, union disp_intr intr_status)
{
	vo_dev dev = core->core_id;
	union disp_dbg_status status = disp_get_dbg_status(dev, true);
	vo_layer layer = g_vo_ctx->dev_ctx[dev].bind_layer_id;
	struct vo_layer_ctx *layer_ctx;
	struct vo_wbc_ctx *wbc_ctx;
	int i;
	union disp_odma_intr_sel online_odma_mask;
	frame_rate_ctrl_s wbc_frame_ctrl;

	if (intr_status.b.disp_frame_end) {
		ddr_retrain(dev, intr_status);
	}

	if (atomic_read(&core->disp_streamon) == 0)
		return;

	if (intr_status.b.disp_frame_end) {
		++core->frame_number;

		layer_ctx = &g_vo_ctx->layer_ctx[layer];
		if (!layer_ctx->is_layer_enable)
			return;

		if (status.b.bw_fail){
			layer_ctx->bw_fail++;
			TRACE_VO(DBG_WARN, "dev(%d) disp bw failed at frame#%d\n", dev, core->frame_number);
		}
		if (status.b.osd_bw_fail){
			layer_ctx->vgop_bw_fail++;
			TRACE_VO(DBG_WARN, "dev(%d) osd bw failed at frame#%d\n", dev, core->frame_number);
		}


		if (!core->disp_online) {
			_vo_hw_enque(dev, layer_ctx);
			layer_ctx->event = 1;
			wake_up_interruptible(&layer_ctx->wq);
			//TRACE_VO(DBG_INFO, "wakeup thread-%d.\n", layer);
		}

		for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
			if (g_vo_ctx->wbc_ctx[i].is_wbc_enable &&
			    g_vo_ctx->wbc_ctx[i].wbc_src.src_type == VO_WBC_SRC_DEV &&
			    g_vo_ctx->wbc_ctx[i].wbc_src.src_id == dev) {
				wbc_ctx = &g_vo_ctx->wbc_ctx[i];

				if (intr_status.b.disp_odma_fifo_full_err)
					wbc_ctx->odma_fifofull++;

				wbc_frame_ctrl.src_frame_rate = g_vo_ctx->dev_ctx[dev].pub_attr.sync_info.frame_rate;
				wbc_frame_ctrl.dst_frame_rate = wbc_ctx->wbc_attr.frame_rate;

				if (!FRC_INVALID(wbc_frame_ctrl) &&
				    (!vo_frame_ctrl(core->frame_number, &wbc_frame_ctrl)))
					wbc_ctx->is_drop = true;

				if (wbc_ctx->is_drop) {
					wbc_ctx->is_drop = false;
					continue;
				}

				vo_wbc_rdy_buf_remove(wbc_ctx);
				vo_wbc_dqbuf_list(wbc_ctx);

				if (vo_wbc_rdy_buf_empty(wbc_ctx))
					TRACE_VO(DBG_ERR, "wbc_dev(%d) wbc outbuf is empty\n", i);
				else
					vo_wbc_submit(wbc_ctx);

				if (!wbc_ctx->is_odma_enable) {
					// odma enable
					disp_get_odma_intr_mask(dev, &online_odma_mask);
					online_odma_mask.b.disp_online_frame_end = true; //true means disable
					online_odma_mask.b.disp_odma_frame_end = false; //true means disable
					disp_set_odma_intr_mask(dev, online_odma_mask);
					disp_odma_enable(dev, true);
					wbc_ctx->is_odma_enable = true;
				}

				wbc_ctx->event = 1;
				wake_up_interruptible(&wbc_ctx->wq);
			}
		}
	}
}

irqreturn_t vo_irq_handler(int irq, void *data)
{
	struct vo_core *core = (struct vo_core *)data;
	union disp_intr intr_status;
	union disp_intr_clr intr_clr;
	vo_dev dev = core->core_id;

	if (core->irq_num != irq) {
		TRACE_VO(DBG_ERR, "irq(%d) Error.\n", irq);
		return IRQ_HANDLED;
	}

	intr_status = disp_intr_status(dev);
	intr_clr.b.disp_frame_end = intr_status.b.disp_frame_end;
	intr_clr.b.disp_frame_start = intr_status.b.disp_frame_start;
	intr_clr.b.disp_tgen_lite = intr_status.b.disp_tgen_lite;

	if(intr_status.b.disp_odma_fifo_full_err) {
		disp_odma_fifofull_clr(dev);
	}

	//IC bug, need to clear two times.
	disp_intr_clr(dev, intr_clr);
	disp_intr_clr(dev, intr_clr);

	irq_handler(core, intr_status);

	return IRQ_HANDLED;
}

int default_overlay_bind_priority[VO_MAX_DEV_NUM][VO_MAX_GRAPHIC_LAYER_IN_DEV] = {
	{VO_LAYER_G0, VO_LAYER_G2, VO_LAYER_G3},//dev0 bind G0 G2 G3
	{VO_LAYER_G1, VO_LAYER_G4, VO_LAYER_G5},//dev1 bind G1 G4 G5
};

static int _vo_init_param(struct vo_ctx *ctx)
{
	int ret = 0, i, j;
	u16 rgb[3] = {0, 0, 0};

	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		ctx->dev_ctx[i].bind_layer_id = i;
		if (hide_vo) {
			disp_set_pattern(i, PAT_TYPE_FULL, PAT_COLOR_USR, rgb);
			disp_set_frame_bgcolor(i, 0, 0, 0);
		}
		mutex_init(&ctx->dev_ctx[i].dev_lock);
	}

	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		ctx->layer_ctx[i].bind_dev_id = i;
		ctx->layer_ctx[i].layer = i;
		ctx->layer_ctx[i].display_buflen = 2;

		spin_lock_init(&ctx->layer_ctx[i].list_lock);
		INIT_LIST_HEAD(&ctx->layer_ctx[i].list_wait);
		INIT_LIST_HEAD(&ctx->layer_ctx[i].list_work);
		INIT_LIST_HEAD(&ctx->layer_ctx[i].list_done);
		mutex_init(&ctx->layer_ctx[i].layer_lock);

		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			mutex_init(&ctx->layer_ctx[i].chn_ctx[j].gdc_lock);
		}
	}

	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		for (j = 0; j < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++j) {
			ctx->dev_ctx[i].bind_overlay_id[j] = default_overlay_bind_priority[i][j];
			ctx->overlay_ctx[default_overlay_bind_priority[i][j] - VO_MAX_VIDEO_LAYER_NUM].bind_dev_id = i;
			ctx->overlay_ctx[default_overlay_bind_priority[i][j] - VO_MAX_VIDEO_LAYER_NUM].priority = 0;
		}
	}

	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_NUM; ++i) {
		for (j = 0; j < RGN_MAX_NUM_VO; ++j)
			ctx->overlay_ctx[i].rgn_handle[j] = RGN_INVALID_HANDLE;
	}

	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		ctx->wbc_ctx[i].is_wbc_enable = false;
		ctx->wbc_ctx[i].is_wbc_src_cfg = false;
		ctx->wbc_ctx[i].is_wbc_attr_cfg = false;
		ctx->wbc_ctx[i].wbc_src.src_type = VO_WBC_SRC_DEV;
		ctx->wbc_ctx[i].wbc_src.src_id = 0;
		ctx->wbc_ctx[i].wbc_attr.target_size.width = 0;
		ctx->wbc_ctx[i].wbc_attr.target_size.height = 0;
		ctx->wbc_ctx[i].wbc_attr.pixformat = PIXEL_FORMAT_NV21;
		ctx->wbc_ctx[i].wbc_attr.frame_rate = 0;
		ctx->wbc_ctx[i].wbc_attr.dynamic_range = DYNAMIC_RANGE_SDR8;
		ctx->wbc_ctx[i].wbc_attr.compress_mode = COMPRESS_MODE_NONE;
		ctx->wbc_ctx[i].wbc_mode = VO_WBC_MODE_NORM;
		ctx->wbc_ctx[i].depth = VO_WBC_DONEQ;
		ctx->wbc_ctx[i].done_cnt = 0;
		ctx->wbc_ctx[i].frame_num = 0;
		ctx->wbc_ctx[i].frame_rate = 0;
		mutex_init(&ctx->wbc_ctx[i].wbc_lock);
	}

	return ret;
}

int vo_recv_frame(mmf_chn_s chn, vb_blk blk)
{
	int ret;
	struct vo_chn_ctx *chn_ctx;
	struct vb_jobs_t *jobs;
	struct vb_s *vb = (struct vb_s *)blk;
	frame_rate_ctrl_s chn_frame_ctrl;

	ret = check_vo_chn_valid(chn.dev_id, chn.chn_id);
	if (ret != 0)
		return ret;

	if (g_vo_ctx->suspend) {
		TRACE_VO(DBG_WARN, "vo suspend do not recv frame.\n");
		return -1;
	}

	chn_ctx = &g_vo_ctx->layer_ctx[chn.dev_id].chn_ctx[chn.chn_id];

	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_WARN, "layer(%d) chn(%d) disable.\n", chn.dev_id, chn.chn_id);
		return -1;
	}

	chn_ctx->frame_index++;
	chn_ctx->src_frame_num++;
	chn_frame_ctrl.src_frame_rate = chn_ctx->src_frame_rate;
	chn_frame_ctrl.dst_frame_rate = chn_ctx->frame_rate_user_set;
	chn_ctx->is_drop = false;

	if (!FRC_INVALID(chn_frame_ctrl) && (!vo_frame_ctrl(chn_ctx->frame_index, &chn_frame_ctrl)))
		chn_ctx->is_drop = true;

	if ((chn_ctx->pause && !chn_ctx->refresh) || chn_ctx->hide || chn_ctx->is_drop)
		return 0;

	if (IS_VB_OFFSET_INVALID(vb->buf)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) vb offset (%d %d %d %d) invalid\n",
			 chn.dev_id, chn.chn_id,
			 vb->buf.offset_left, vb->buf.offset_right,
			 vb->buf.offset_top, vb->buf.offset_bottom);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (chn_ctx->rotation != ROTATION_0) {
		if (mutex_trylock(&chn_ctx->gdc_lock)) {
			struct _vo_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};
			size_s size;

			if (vb->buf.size.width % LDC_ALIGN) {
				size.width = ALIGN(vb->buf.size.width, LDC_ALIGN);
				vb->buf.offset_left = 0;
				vb->buf.offset_right =
					size.width - vb->buf.size.width;
				vb->buf.size.width = size.width;
			}
			if (vb->buf.size.height % LDC_ALIGN) {
				size.height = ALIGN(vb->buf.size.height, LDC_ALIGN);
				vb->buf.offset_top = 0;
				vb->buf.offset_bottom =
					size.height - vb->buf.size.height;
				vb->buf.size.height = size.height;
			}
			TRACE_VO(DBG_INFO, "layer(%d) chn(%d) gdc vb offset(%d %d %d %d).\n",
				 chn.dev_id, chn.chn_id,
				 vb->buf.offset_left, vb->buf.offset_top,
				 vb->buf.offset_right, vb->buf.offset_bottom);

			atomic_fetch_add(1, &vb->usr_cnt);
			atomic_long_fetch_or(BIT(ID_GDC), &vb->mod_ids);

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, g_vo_ctx->layer_ctx[chn.dev_id].layer_attr.pixformat
				, chn_ctx->mesh.paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VO
				, chn_ctx->rotation) != 0) {
				mutex_unlock(&chn_ctx->gdc_lock);
				TRACE_VO(DBG_ERR, "gdc rotation failed.\n");
				return -1;
			}
			TRACE_VO(DBG_DEBUG, "layer(%d) chn(%d) push vb(0x%llx) to gdc job.\n",
				 chn.dev_id, chn.chn_id, vb->phy_addr);

			return 0;
		} else {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) drop frame due to gdc op blocked.\n",
				 chn.dev_id, chn.chn_id);
			return -1;
		}
	}

	jobs = &chn_ctx->chn_jobs;
	mutex_lock(&jobs->lock);
	if (!jobs->inited){
		mutex_unlock(&jobs->lock);
		TRACE_VO(DBG_NOTICE, "layer(%d) chn(%d) jobs not initialized yet.\n",
				chn.dev_id, chn.chn_id);
		return -1;
	}
	if (FIFO_FULL(&jobs->waitq)) {
		struct vb_s *vb_old = NULL;
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) waitq full, drop frame.\n",
			     chn.dev_id, chn.chn_id);
		FIFO_POP(&jobs->waitq, &vb_old);
		atomic_long_fetch_and(~BIT(chn.mod_id), &vb_old->mod_ids);
		vb_release_block((vb_blk)vb_old);
	} else {
		chn_ctx->frame_num++;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	mutex_unlock(&jobs->lock);

	atomic_fetch_add(1, &vb->usr_cnt);
	atomic_long_fetch_or(BIT(chn.mod_id), &vb->mod_ids);

	mutex_lock(&g_vo_ctx->layer_ctx[chn.dev_id].layer_lock);
	g_vo_ctx->layer_ctx[chn.dev_id].is_layer_update = true;
	mutex_unlock(&g_vo_ctx->layer_ctx[chn.dev_id].layer_lock);

	TRACE_VO(DBG_INFO, "layer(%d) chn(%d) push vb(0x%llx).\n",
		 chn.dev_id, chn.chn_id, vb->phy_addr);

	return ret;
}

static void _update_vo_real_frame_rate(struct timer_list *timer)
{
	int i, j;

	UNUSED(timer);

	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		if (!g_vo_ctx->layer_ctx[i].is_layer_enable)
			continue;
		g_vo_ctx->layer_ctx[i].frame_rate = g_vo_ctx->layer_ctx[i].frame_num;
		g_vo_ctx->layer_ctx[i].frame_num = 0;

		g_vo_ctx->layer_ctx[i].src_frame_rate = g_vo_ctx->layer_ctx[i].src_frame_num;
		g_vo_ctx->layer_ctx[i].src_frame_num = 0;

		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!g_vo_ctx->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			g_vo_ctx->layer_ctx[i].chn_ctx[j].frame_rate =
				g_vo_ctx->layer_ctx[i].chn_ctx[j].frame_num;
			g_vo_ctx->layer_ctx[i].chn_ctx[j].frame_num = 0;

			g_vo_ctx->layer_ctx[i].chn_ctx[j].src_frame_rate =
				g_vo_ctx->layer_ctx[i].chn_ctx[j].src_frame_num;
			g_vo_ctx->layer_ctx[i].chn_ctx[j].src_frame_num = 0;
		}
	}

	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		if (!g_vo_ctx->wbc_ctx[i].is_wbc_enable)
			continue;

		g_vo_ctx->wbc_ctx[i].frame_rate = g_vo_ctx->wbc_ctx[i].frame_num;
		g_vo_ctx->wbc_ctx[i].frame_num = 0;
	}


	mod_timer(&vo_timer_proc, jiffies + msecs_to_jiffies(1000));
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vo_create_instance(struct platform_device *pdev)
{
	int ret = 0;

	g_core_dev = dev_get_drvdata(&pdev->dev);
	if (!g_core_dev) {
		TRACE_VO(DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

	g_vo_ctx = kzalloc(sizeof(*g_vo_ctx), GFP_ATOMIC);
	if (!g_vo_ctx) {
		TRACE_VO(DBG_ERR, "g_vo_ctx alloc size(%ld) failed\n", sizeof(struct vo_ctx));
		return -ENOMEM;
	}
	_vo_init_param(g_vo_ctx);

	ret = _vo_create_proc(g_vo_ctx);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to create proc\n");
		return ret;
	}

	base_register_recv_cb(ID_VO, vo_recv_frame);

	add_timer(&vo_timer_proc);
	mod_timer(&vo_timer_proc, jiffies + msecs_to_jiffies(1000));

	return 0;
}

int vo_destroy_instance(struct platform_device *pdev)
{
	if (!g_core_dev) {
		TRACE_VO(DBG_ERR, "VO has been destroyed!\n");
		return -EINVAL;
	}

	del_timer_sync(&vo_timer_proc);
	base_unregister_recv_cb(ID_VO);
	_vo_destroy_proc();
	kfree(g_vo_ctx);
	g_vo_ctx = NULL;
	g_core_dev = NULL;

	return 0;
}
