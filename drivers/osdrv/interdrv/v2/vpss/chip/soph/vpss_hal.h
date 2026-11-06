#ifndef __VPSS_HAL_H__
#define __VPSS_HAL_H__

#include "base_ctx.h"
#include <linux/list.h>
#include <linux/defines.h>

#include "vpss_core.h"
#include "vpss_cb.h"

#define YRATIO_SCALE         100
#define VPSS_CMDQ_BUF_SIZE (0x8000)
#define VPSS_WORK_MAX        8

enum sc_flip_mode {
	SC_FLIP_NO,
	SC_FLIP_HFLIP,
	SC_FLIP_VFLIP,
	SC_FLIP_HVFLIP,
	SC_FLIP_MAX
};

enum sc_quant_rounding {
	SC_QUANT_ROUNDING_TO_EVEN = 0,
	SC_QUANT_ROUNDING_AWAY_FROM_ZERO,
	SC_QUANT_ROUNDING_TRUNCATE,
	SC_QUANT_ROUNDING_MAX,
};

enum sc_scaling_coef {
	SC_SCALING_COEF_BICUBIC = 0,
	SC_SCALING_COEF_BILINEAR,
	SC_SCALING_COEF_NEAREST,
	SC_SCALING_COEF_BICUBIC_OPENCV,
	SC_SCALING_COEF_MAX,
};

enum job_state {
	JOB_WAIT,
	JOB_WORKING,
	JOB_HALF,
	JOB_END,
	JOB_INVALID,
};


/* struct sc_quant_param
 *   parameters for quantization, output format fo sc must be RGB/BGR.
 *
 * @sc_frac: fractional number of the scaling-factor. [13bits]
 * @sub: integer number of the means.
 * @sub_frac: fractional number of the means. [10bits]
 */
struct sc_quant_param {
	__u16 sc_frac[3];
	__u8  sub[3];
	__u16 sub_frac[3];
	enum sc_quant_rounding rounding;
	bool enable;
};

struct convertto_param {
	bool enable;
	__u32 a_frac[3];
	__u32 b_frac[3];
};

/* struct sc_border_param
 *   only work if sc offline and sc_output size < fmt's setting
 *
 * @bg_color: rgb format
 * @offset_x: offset of x
 * @offset_y: offset of y
 * @enable: enable or disable
 */
struct sc_border_param {
	__u32 bg_color[3];
	__u16 offset_x;
	__u16 offset_y;
	bool enable;
};

struct sc_border_vpp_param {
	bool enable;
	__u8 bg_color[3];
	struct vip_range inside;
	struct vip_range outside;
};

/* struct sc_mute
 *   cover sc with the specified rgb-color if enabled.
 *
 * @color: rgb format
 * @enable: enable or disable
 */
struct sc_mute {
	__u8 color[3];
	bool enable;
};

struct odma_sb_cfg {
	__u8 sb_mode;
	__u8 sb_size;
	__u8 sb_nb;
	__u8 sb_full_nb;
	__u8 sb_wr_ctrl_idx;
};

struct vpss_hal_chn_cfg {
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	__u32 y_ratio;
	struct vip_frmsize src_size;
	struct vip_rect crop;
	struct vip_rect dst_rect;
	struct vip_frmsize dst_size;
	struct rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct rgn_coverex_cfg rgn_coverex_cfg;
	struct rgn_mosaic_cfg rgn_mosaic_cfg;
	struct sc_quant_param quant_cfg;
	struct convertto_param convert_to_cfg;
	struct sc_border_param border_cfg;
	struct sc_border_vpp_param border_vpp_cfg[VPSS_RECT_NUM];
	struct sc_mute mute_cfg;
	struct csc_cfg csc_cfg;
	enum sc_flip_mode flip;
	enum sc_scaling_coef sc_coef;
};

struct vpss_hal_grp_cfg {
	bool fbd_enable;
	bool online_from_isp;
	bool upsample;
	bool bm_scene;
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[4];
	struct vip_frmsize src_size;
	struct vip_rect crop;
	struct csc_cfg csc_cfg;
};

struct vpss_online_cb {
	__u8 snr_num;
	__u8 is_tile;
	__u8 is_left_tile;
	struct vip_line l_in;
	struct vip_line l_out;
	struct vip_line r_in;
	struct vip_line r_out;
	struct timespec64 ts;
};

struct vpss_hw_cfg {
	__u8 chn_num;
	__u8 chn_enable[VPSS_MAX_CHN_NUM];
	struct vpss_hal_grp_cfg grp_cfg;
	struct vpss_hal_chn_cfg chn_cfg[VPSS_MAX_CHN_NUM];
};

typedef void (*vpss_job_cb)(void *data);

struct vpss_job {
	struct vpss_hw_cfg cfg;
	__u8 grp_id;
	__u8 working_mask;
	__u8 dev_idx_start;
	__u8 tile_mode;
	bool is_online;
	bool is_tile;
	bool is_work_on_r_tile;
	bool is_v_tile;
	struct vpss_online_cb online_param;
	vpss_job_cb job_cb;
	void *data;
	spinlock_t lock;
	atomic_t job_state;
	__u32 vpss_dev_mask;
	__u32 checksum[VPSS_MAX_CHN_NUM];
	struct list_head list;
	u32 hw_duration;
	struct work_struct job_work;
};

void vpss_irq_handler(struct vpss_core *vpss_dev);
int vpss_hal_init(struct vpss_device *dev);
void vpss_hal_deinit(void);

int vpss_hal_push_job(struct vpss_job *job);
int vpss_hal_push_online_job(struct vpss_job *job);
int vpss_hal_remove_job(struct vpss_job *job);
int vpss_hal_try_schedule(void);
int vpss_hal_direct_schedule(struct vpss_job *job);
int vpss_hal_online_run(struct vpss_online_cb *param);
void vpss_hal_online_release_dev(void);

int vpss_hal_stitch_schedule(struct vpss_stitch_cfg *cfg);
void vpss_cmdq_irq_handler(struct vpss_core *vpss_dev);

void vpss_hal_suspend(void);
void vpss_hal_resume(void);
bool vpss_check_suspend(void);

void vpss_clk_disable(void);
void vpss_clk_enable(void);

void vpss_hal_down_reg(unsigned char inst);

#endif // _U_SC_UAPI_H_
