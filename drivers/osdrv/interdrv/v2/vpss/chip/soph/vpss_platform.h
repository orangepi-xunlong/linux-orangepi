#ifndef __VPSS_PLATFORM_H__
#define __VPSS_PLATFORM_H__

#include "vpss_hal.h"

struct vpss_cmdq_buf {
	__u64 cmdq_phy_addr;
	void *cmdq_vir_addr;
	__u32 cmdq_buf_size;
};

void img_update(u8 dev_idx, bool is_master, const struct vpss_hal_grp_cfg *grp_cfg);
void sc_update(u8 dev_idx, const struct vpss_hal_chn_cfg *chn_cfg);
void top_update(u8 dev_idx, bool is_share, bool fbd_enable);
void img_start(u8 dev_idx, u8 chn_num);
void img_reset(u8 dev_idx);
bool img_left_tile_cfg(u8 dev_idx, u16 online_l_width);
bool img_right_tile_cfg(u8 dev_idx, u16 online_r_start, u16 online_r_end);
bool img_top_tile_cfg(u8 dev_idx, u8 is_right);
bool img_down_tile_cfg(u8 dev_idx, u8 is_right);

void vpss_stauts(u8 dev_idx);
void vpss_error_stauts(u8 dev_idx);

int vpss_stitch_platform(u8 dev_idx, struct vpss_cmdq_buf *cmdq_buf, struct vpss_stitch_cfg *cfg);

#endif
