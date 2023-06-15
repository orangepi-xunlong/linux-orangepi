/*
 * RK29 ebook control driver rk29_ebc.h
 *
 * Copyright (C) 2010 RockChip, Inc.
 * Author: <Dai Lunxue> dlx@rock-chips.com
 *	   <Hunag Lin> hl@rock-chips.com
 *	   <Yang Kuankuan> ykk@rock-chips.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef RK29_EBC_H
#define RK29_EBC_H

#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>


#define RKEBC_DRV_VERSION		"2.00"

#define EBC_SUCCESS			(0)
#define EBC_ERROR			(-1)

//#define DIRECT_MODE
#define AUTO_MODE_ENABLE

/* Select WAVEFORM from nand or spi flash */
#define NAND_WAVEFORM			(0)
#define SPI_WAVEFORM			(1)

/*SET END DISPLAY*/
#define END_RESET			(0)
#define END_PICTURE			(1)

// ebc ioctl command
#define GET_EBC_BUFFER (0x7000)
#define SET_EBC_SEND_BUFFER (0x7001)
#define GET_EBC_DRIVER_SN (0x7002)
#define GET_EBC_BUFFER_INFO (0x7003)

#define EBC_DRIVER_SN "RK29_EBC_DRIVER_VERSION_1.00"
#define EBC_DRIVER_SN_LEN sizeof(EBC_DRIVER_SN)

#define ebc_printk(dir_of_file, lev, fmt) ebc_dbg_printk(dir_of_file, lev, fmt)

/*
 * IMPORTANT: Those values is corresponding to android hardware program,
 * so *FORBID* to changes bellow values, unless you know what you're doing.
 * And if you want to add new refresh modes, please appended to the tail.
 */
enum epd_refresh_mode {
	EPD_AUTO	= 0,
	EPD_FULL	= 1,
	EPD_A2		= 2,
	EPD_PART	= 3,
	EPD_FULL_DITHER = 4,
	EPD_RESET	= 5,
	EPD_BLACK_WHITE = 6,
	EPD_TEXT	= 7,
	EPD_BLOCK	= 8,
	EPD_FULL_WIN	= 9,
	EPD_OED_PART	= 10,
	EPD_DIRECT_PART	= 11,
	EPD_DIRECT_A2	= 12,
};

#define EBC_OFF      (0)
#define EBC_ON        (1)
struct logo_info {
	int logo_pic_offset;
	int logo_end_offset;
	int logo_power_pic_offset;
};

//ebc panel info
struct ebc_panel {
	int    width;
	int    height;
	int    hsync_len;//refer to eink spec LSL
	int    hstart_len;//refer to eink spec LBL
	int    vsync_len;//refer to eink spec FSL
	int    vend_len; //refer to eink spec FEL
	int    frame_rate;
	int    vir_width;
	int    vir_height;
	int	refcount;//fixme
	int   fb_width;
	int   fb_height;
	int   color_panel;
	int   rotate;
	int   hend_len;//refer to eink spec LEL
	int   vstart_len; //refer to eink spec FBL
	int   gdck_sta;//refer to eink spec GDCK_STA
	int   lgonl;//refer to eink spec LGONL
};
/*struct*/
//ebc clocks info
struct ebc_clk {
	struct clk      *dclk;            //ebc dclk
	struct clk      *dclk_parent;     //ebc dclk divider frequency source
	int dclk_status;
	int dclk_parent_status;

	struct clk	*hclk;
	struct clk	*hclk_lcdc;
	struct clk	*aclk_lcdc;
	struct clk      *hclk_disp_matrix;
	int hclk_status;
	int hclk_lcdc_status;
	int aclk_lcdc_status;
	int hclk_disp_matrix_status;

	struct clk	*aclk_ddr_lcdc;   //DDR LCDC AXI clock disable.
	struct clk	*aclk_disp_matrix;	//DISPLAY matrix AXI clock disable.
	struct clk	*hclk_cpu_display;	//CPU DISPLAY AHB bus clock disable.
	struct clk	*pd_display;		// display power domain
	int aclk_ddr_lcdc_status;
	int aclk_disp_matrix_status;
	int hclk_cpu_display_status;
	int pd_display_status;

	int pixclock;
};

struct ebc_platform_data {
	int (*io_init)(void);
	int (*io_deinit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	int (*vcom_power_on)(void);
	int (*vcom_power_off)(void);
	int (*suspend)(struct ebc_platform_data *ebc_data);
	int (*resume)(struct ebc_platform_data *ebc_data);
	const char *regulator;
};
struct ebc_pwr_ops {
	int (*power_on)(void);
	int (*power_down)(void);
};
struct ebc_temperateure_ops {
	int (*temperature_get)(int *temp);
};


/*android use struct*/
struct ebc_buf_info {
	int offset;
	int epd_mode;
	int height;
	int width;
	int vir_height;
	int vir_width;
	int fb_width;
	int fb_height;
	int color_panel;
	int win_x1;
	int win_y1;
	int win_x2;
	int win_y2;
	int rotate;
};

// ebc sn
struct ebc_sn_info {
	u32 key;
	u32 sn_len;
	char cip_sn[EBC_DRIVER_SN_LEN];
};

/* modiy for open ebc ioctl */
extern int rkebc_register_notifier(struct notifier_block *nb);
extern int rkebc_unregister_notifier(struct notifier_block *nb);
#ifdef CONFIG_SOFTWARE_EBC
extern void ebc_init_lcdc(struct rk29fb_screen *screen);
#endif
extern int register_ebc_pwr_ops(struct ebc_pwr_ops *ops);
//extern int register_ebc_temp_ops(struct ebc_temperateure_ops *ops);
extern int tps65185_temperature_get(int *temp);

extern int rkebc_notify(unsigned long event);
extern int rk29ebc_notify(unsigned long event);
extern long ebc_io_ctl(struct file *file, unsigned int cmd, unsigned long arg);
extern int ebc_sn_encode(char *sn, char *cip_sn, int sn_len, int key);


/* public function */
extern void set_epd_info(struct ebc_panel *panel, struct ebc_clk *epd_clk, int *height, int *width);
extern int set_logo_info(struct logo_info *plogo_info);

/* hooks for customs */
void ebc_io_ctl_hook(unsigned int cmd, struct ebc_buf_info *info);

extern int support_pvi_waveform(void);
extern int get_lut_position(void);
extern int set_end_display(void);
extern int get_bootup_logo_cycle(void);
extern int is_bootup_ani_loop(void);
extern int is_need_show_lowpower_pic(void);
extern int support_bootup_ani(void);
extern int support_double_thread_calcu(void);
extern int get_bootup_ani_mode(void);
extern int support_tps_3v3_always_alive(void);

#endif
