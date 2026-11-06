/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _U_VO_DISP_H_
#define _U_VO_DISP_H_

#ifndef LANE_MAX_NUM
#define LANE_MAX_NUM   5
#endif

#include <linux/comm_vo.h>

enum vo_disp_intf {
	VO_DISP_INTF_DSI = 0,
	VO_DISP_INTF_LVDS,
	VO_DISP_INTF_HDMI,
	//not support yet
	VO_DISP_INTF_BT601,
	VO_DISP_INTF_BT656,
	VO_DISP_INTF_BT1120,
	VO_DISP_INTF_PARALLEL_RGB,
	VO_DISP_INTF_SERIAL_RGB,
	VO_DISP_INTF_I80,
	VO_DISP_INTF_HW_MCU,
	VO_DISP_INTF_MAX,
};

/*
 * @pixelclock: pixel clock in kHz
 */
struct lvds_intf_cfg {
	u32 pixelclock;
	vo_lvds_out_bit_e out_bits;
	vo_lvds_mode_e mode;
	u8 chn_num;
	u8 vs_out_en;
	u8 hs_out_en;
	u8 hs_blk_en;
	u8 msb_lsb_data_swap;
	u8 serial_msb_first;
	u8 even_odd_link_swap;
	u8 enable;
	s8 lane_id[LANE_MAX_NUM];
	u8 lane_pn_swap[LANE_MAX_NUM];
};

struct vo_d_remap {
	vo_mac_d_sel_e sel;
	__u32 mux;
};

struct vo_pins {
	unsigned char pin_num;
	struct vo_d_remap d_pins[MAX_VO_PINS];
};

enum bt_mode {
	BT_MODE_656 = 0,
	BT_MODE_1120,
	BT_MODE_601,
	BT_MODE_MAX,
};

struct bt_intf_cfg {
	__u32 pixelclock;
	u8 is_set_param;
	u8 bt_clk_inv;
	u8 bt_vs_inv;
	u8 bt_hs_inv;
	enum bt_mode mode;
	vo_bt_data_seq_e data_seq;
	struct vo_pins pins;
};

struct vo_disp_intf_cfg {
	enum vo_disp_intf intf_type;
	union {
		struct lvds_intf_cfg lvds_cfg;
		struct bt_intf_cfg bt_cfg;
		//to do: prgb/srgb i80(sw_i80/hw_mcu)
	};
};

enum vo_dv_pos_pol {
	VO_DV_VSYNC_POS_POL = 1,
	VO_DV_HSYNC_POS_POL,
};

struct vo_plane {
	u32 length;
	u64 addr;
	u32 bytesused;
	union {
	u32 offset;
	u64 userptr;
	} m;
};

/*
 * @index:
 * @length: length of planes
 * @planes: to describe buf
 * @reserved
 */
struct vo_buffer {
	u32 index;
	u32 length;
	struct vo_plane planes[3];
};

struct vo_wbc_buffer {
	u64 addr[3];
	u32 pitch_y;
	u32 pitch_c;
	u32 width;
	u32 height;
};

enum vo_colorspace {
	VO_COLORSPACE_SRGB = 0,
	VO_COLORSPACE_SMPTE170M,
	VO_COLORSPACE_MAX,
};

struct vo_fmt {
	u32 fourcc;
	u8 fmt;
	u8 buffers;
	u32 bit_depth[3];
	u8 plane_sub_h;
	u8 plane_sub_v;
};

struct vo_plane_info {
	u32 sizeimage;
	u16 bytesperline;
};

struct vo_video_format {
	u32 width;
	u32 height;
	u32 pixelformat;
	u32 field;
	u32 colorspace;
	struct vo_plane_info plane_info[3];
	u8 num_planes;
};

#endif	// _U_VO_DISP_H_
