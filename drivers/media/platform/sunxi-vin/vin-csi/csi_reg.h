/*
 * sunxi csi register read/write interface header file
 * Author:raymonxiu
*/
#ifndef __CSI__REG__H__
#define __CSI__REG__H__

#include <linux/types.h>
#include <media/sunxi_camera_v2.h>

#define MAX_CSI 2

#if defined CONFIG_ARCH_SUN8IW11P1

#define CSI_REG_EN           (0x00)
#define CSI_REG_CONF         (0x04)
#define CSI_REG_CTRL         (0x08)
#define CSI_REG_SCALE        (0x0C)
#define CSI_REG_BUF_0_A      (0x10)
#define CSI_REG_BUF_0_B      (0x14)
#define CSI_REG_BUF_1_A      (0x18)
#define CSI_REG_BUF_1_B      (0x1C)
#define CSI_REG_BUF_2_A      (0x20)
#define CSI_REG_BUF_2_B      (0x24)
#define CSI_REG_BUF_CTRL     (0x28)
#define CSI_REG_STATUS       (0x2C)
#define CSI_REG_INT_EN       (0x30)
#define CSI_REG_INT_STATUS   (0x34)
#define CSI_REG_RESIZE_H     (0x40)
#define CSI_REG_RESIZE_V     (0x44)
#define CSI_REG_BUF_LENGTH   (0x48)

#define CSI_CH_OFF	(0x0100)

#define CSI_CH_0	(1 << 10)
#define CSI_CH_1	(1 << 11)
#define CSI_CH_2	(1 << 12)
#define CSI_CH_3	(1 << 13)

int csi_set_base_addr(unsigned int sel, unsigned long addr);
void csi_enable(unsigned int sel);
void csi_disable(unsigned int sel);
void csi_if_cfg(unsigned int sel, struct csi_if_cfg *csi_if_cfg);
void csi_timing_cfg(unsigned int sel, struct csi_timing_cfg *csi_tmg_cfg);
void csi_fmt_cfg(unsigned int sel, unsigned int ch,
		 struct csi_fmt_cfg *csi_fmt_cfg);
void csi_set_buffer_address(unsigned int sel, unsigned int ch,
			    enum csi_buf_sel buf, u64 addr);
u64 csi_get_buffer_address(unsigned int sel, unsigned int ch,
			   enum csi_buf_sel buf);
void csi_capture_start(unsigned int sel, unsigned int ch_total_num,
		       enum csi_cap_mode csi_cap_mode);
void csi_capture_stop(unsigned int sel, unsigned int ch_total_num,
		      enum csi_cap_mode csi_cap_mode);
void csi_capture_get_status(unsigned int sel, unsigned int ch,
			    struct csi_capture_status *status);
void csi_set_size(unsigned int sel, unsigned int ch, unsigned int length_h,
		  unsigned int length_v, unsigned int buf_length_y,
		  unsigned int buf_length_c);
void csi_set_offset(unsigned int sel, unsigned int ch, unsigned int start_h,
		    unsigned int start_v);
void csi_int_enable(unsigned int sel, unsigned int ch,
		    enum csi_int_sel interrupt);
void csi_int_disable(unsigned int sel, unsigned int ch,
		     enum csi_int_sel interrupt);
void csi_int_get_status(unsigned int sel, unsigned int ch,
			struct csi_int_status *status);
void csi_int_clear_status(unsigned int sel, unsigned int ch,
			  enum csi_int_sel interrupt);

#else

int csi_set_base_addr(unsigned int sel, unsigned long addr);
void csi_enable(unsigned int sel);
void csi_disable(unsigned int sel);
void csi_if_cfg(unsigned int sel, struct csi_if_cfg *csi_if_cfg);
void csi_timing_cfg(unsigned int sel, struct csi_timing_cfg *csi_tmg_cfg);
void csi_fmt_cfg(unsigned int sel, unsigned int ch,
		 struct csi_fmt_cfg *csi_fmt_cfg);
void csi_set_buffer_address(unsigned int sel, unsigned int ch,
			    enum csi_buf_sel buf, u64 addr);
u64 csi_get_buffer_address(unsigned int sel, unsigned int ch,
			   enum csi_buf_sel buf);
void csi_capture_start(unsigned int sel, unsigned int ch_total_num,
		       enum csi_cap_mode csi_cap_mode);
void csi_capture_stop(unsigned int sel, unsigned int ch_total_num,
		      enum csi_cap_mode csi_cap_mode);
void csi_capture_get_status(unsigned int sel, unsigned int ch,
			    struct csi_capture_status *status);
void csi_set_size(unsigned int sel, unsigned int ch, unsigned int length_h,
		  unsigned int length_v, unsigned int buf_length_y,
		  unsigned int buf_length_c);
void csi_set_offset(unsigned int sel, unsigned int ch, unsigned int start_h,
		    unsigned int start_v);
void csi_int_enable(unsigned int sel, unsigned int ch,
		    enum csi_int_sel interrupt);
void csi_int_disable(unsigned int sel, unsigned int ch,
		     enum csi_int_sel interrupt);
void csi_int_get_status(unsigned int sel, unsigned int ch,
			struct csi_int_status *status);
void csi_int_clear_status(unsigned int sel, unsigned int ch,
			  enum csi_int_sel interrupt);
#endif

#endif
