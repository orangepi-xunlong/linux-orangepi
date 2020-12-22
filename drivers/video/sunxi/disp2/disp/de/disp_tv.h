
#ifndef __DISP_TV_H__
#define __DISP_TV_H__


#include "disp_private.h"

s32 disp_init_tv(void);//call by disp_display
s32 disp_tv_set_func(struct disp_device*  ptv, disp_tv_func * func);
s32 disp_tv_set_hpd(struct disp_device*  ptv, u32 state);

struct disp_device_private_data {
	__u32 enabled;
	bool suspended;

	disp_tv_mode tv_mode;

	disp_tv_func tv_func;

	disp_video_timings *video_info;

	struct disp_clk_info lcd_clk;

	char *clk;

	char *clk_parent;

	u32 irq_no;
};

s32 disp_tv_suspend(struct disp_device* ptv);
s32 disp_tv_resume(struct disp_device* ptv);


#endif
