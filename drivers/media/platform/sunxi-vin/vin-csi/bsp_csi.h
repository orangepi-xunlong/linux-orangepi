/*
 * sunxi csi bsp header file
 * Author:raymonxiu
*/
#ifndef __BSP__CSI__H__
#define __BSP__CSI__H__

#include "csi_reg.h"
#include "../utility/bsp_common.h"

#define MAX_CH_NUM 4

#define CSI_ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define CSI_ALIGN_32B(x) (((x) + (31)) & ~(31))
#define CSI_ALIGN_16B(x) (((x) + (15)) & ~(15))
#define CSI_ALIGN_8B(x) (((x) + (7)) & ~(7))

enum ref_pol {
	ACTIVE_LOW,		/* active low */
	ACTIVE_HIGH,		/* active high */
};

enum edge_pol {
	FALLING,		/* active falling */
	RISING,			/* active rising */
};

/*
 * the same define as enum csi_input_seq
 */
enum bus_fmt_seq {
	/* only when input is yuv422 */
	YUYV = 0,
	YVYU,
	UYVY,
	VYUY,

	/* only when input is byer */
	RGRG = 0,		/* first line sequence is RGRG... */
	GRGR,			/* first line sequence is GRGR... */
	BGBG,			/* first line sequence is BGBG... */
	GBGB,			/* first line sequence is GBGB... */
};

struct bus_timing {
	enum ref_pol href_pol;
	enum ref_pol vref_pol;
	enum edge_pol pclk_sample;
	enum ref_pol field_even_pol; /*field 0/1 0:odd 1:even*/
};

struct bus_info {
	enum v4l2_mbus_type bus_if;
	struct bus_timing bus_tmg;
	enum v4l2_mbus_pixelcode bus_ch_fmt[MAX_CH_NUM]; /* define the same as V4L2 */
	unsigned int ch_total_num;
};

struct frame_size {
	unsigned int width;	/* in pixel unit */
	unsigned int height;	/* in pixel unit */
};

struct frame_offset {
	unsigned int hoff;	/* in pixel unit */
	unsigned int voff;	/* in pixel unit */
};

/*
 * frame arrangement
 * Indicate that how the channel images are put together into one buffer
 */

struct frame_arrange {
	unsigned char column;
	unsigned char row;
};

struct frame_info {
	struct frame_arrange arrange;
	struct frame_size ch_size[MAX_CH_NUM];
	struct frame_offset ch_offset[MAX_CH_NUM];
	unsigned int pix_ch_fmt[MAX_CH_NUM];
	enum v4l2_field ch_field[MAX_CH_NUM]; /* define the same as V4L2 */
	unsigned int frm_byte_size;
};

extern int bsp_csi_set_base_addr(unsigned int sel, unsigned long addr);
extern void bsp_csi_enable(unsigned int sel);
extern void bsp_csi_disable(unsigned int sel);
extern void bsp_csi_reset(unsigned int sel);
extern int bsp_csi_set_fmt(unsigned int sel, struct bus_info *bus_info,
			   struct frame_info *frame_info);
extern int bsp_csi_set_size(unsigned int sel, struct bus_info *bus_info,
			    struct frame_info *frame_info);
void bsp_csi_set_addr(unsigned int sel, u64 buf_base_addr, u64 buf_base_addr_y,
		      u64 buf_base_addr_u, u64 buf_base_addr_v);
extern void bsp_csi_cap_start(unsigned int sel, unsigned int ch_total_num,
			      enum csi_cap_mode csi_cap_mode);
extern void bsp_csi_cap_stop(unsigned int sel, unsigned int ch_total_num,
			     enum csi_cap_mode csi_cap_mode);
extern void bsp_csi_int_enable(unsigned int sel, unsigned int ch,
			       enum csi_int_sel interrupt);
extern void bsp_csi_int_disable(unsigned int sel, unsigned int ch,
				enum csi_int_sel interrupt);
extern void bsp_csi_int_get_status(unsigned int sel, unsigned int ch,
				   struct csi_int_status *status);
extern void bsp_csi_int_clear_status(unsigned int sel, unsigned int ch,
				     enum csi_int_sel interrupt);

#endif /* __BSP__CSI__H__ */
