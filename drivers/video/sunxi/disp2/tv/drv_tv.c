#include "drv_tv_i.h"
#include "dev_tv.h"

#include <linux/regulator/consumer.h>
#include <linux/clk-private.h>

#define TV_SOURCE 	"pll_de"
#define TVE_CLK	 	"tve"

extern s32 disp_tv_register(disp_tv_func * func);

static bool g_used;
static bool g_suspend;
static struct mutex mlock;


static void tve_clk_enable(u32 sel);
static void tve_clk_init(u32 sel);
static void tve_clk_disable(u32 sel);
static void tve_clk_config(u32 sel, u32 tv_mode);

static disp_video_timings video_timing[] =
{
	/*vic         tv_mode          PCLK   AVI    x     y    HT  HBP HFP HST VT VBP VFP VST*/
	//{0,	DISP_TV_MOD_PAL,27000000, 0, 720, 576, 864, 137, 3,   2,  625, 20, 25, 2, 0, 0,1, 0, 0},
	//{0,	DISP_TV_MOD_NTSC,13500000, 0,  720,   480,   858,   57,   19,   62,  525,   15,  4,  3,  0,   0,   1,   0,   0},
	{0,   DISP_TV_MOD_NTSC,27000000,  0,  720,   480,   858,   60,   16,   62,  525,   30,  9,  6,  0,   0,   0,   0,   0},
	{0,   DISP_TV_MOD_PAL  ,27000000,  0,  720,   576,   864,   68,   12,   64,  625,   39,  5,  5,  0,   0,   0,   0,   0},
};

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)

static struct task_struct * g_tve_task;
static u32 g_tv_hpd = 0;

static struct switch_dev cvbs_switch_dev = {
	.name = "cvbs",
};

static struct switch_dev svideo_switch_dev = {
	.name = "svideo",
};

static struct switch_dev ypbpr_switch_dev = {
	.name = "ypbpr",
};

static struct switch_dev vga_switch_dev = {
	.name = "vga",
};

void tv_report_hpd_work(void)
{
	switch(g_tv_hpd) {

	case DISP_TV_NONE:
		switch_set_state(&cvbs_switch_dev, STATUE_CLOSE);
		switch_set_state(&svideo_switch_dev, STATUE_CLOSE);
		switch_set_state(&ypbpr_switch_dev, STATUE_CLOSE);
		switch_set_state(&vga_switch_dev, STATUE_CLOSE);
		break;

	case DISP_TV_CVBS:
		switch_set_state(&cvbs_switch_dev, STATUE_OPEN);
		break;

	case DISP_TV_SVIDEO:
		switch_set_state(&svideo_switch_dev, STATUE_OPEN);
		break;

	case DISP_TV_YPBPR:
		switch_set_state(&ypbpr_switch_dev, STATUE_OPEN);
		break;

	default:
		switch_set_state(&cvbs_switch_dev, STATUE_CLOSE);
		switch_set_state(&svideo_switch_dev, STATUE_CLOSE);
		switch_set_state(&ypbpr_switch_dev, STATUE_CLOSE);
		switch_set_state(&vga_switch_dev, STATUE_CLOSE);
		break;
	}
}


s32 tv_detect_thread(void *parg)
{
	s32 hpd;
	int i = 0;
	while(1) {
		if(kthread_should_stop()) {
			break;
		}
		if(!g_suspend) {
			for(i=0;i<SCREEN_COUNT;i++)
				hpd = tv_get_dac_hpd(i);
			if(hpd != g_tv_hpd) {
				g_tv_hpd = hpd;
				tv_report_hpd_work();
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(20);
	}
	return 0;
}

s32 tv_detect_enable(void)
{
	int i = 0;
	for(i=0; i<SCREEN_COUNT;i++)
		tve_low_dac_autocheck_enable(i, 0);

	g_tve_task = kthread_create(tv_detect_thread, (void*)0, "tve detect");
	if(IS_ERR(g_tve_task)) {
		s32 err = 0;
		err = PTR_ERR(g_tve_task);
		g_tve_task = NULL;
		return err;
	}
	else {
		pr_debug("g_tve_task is ok!\n");
	}
	wake_up_process(g_tve_task);
	return 0;
}

s32 tv_detect_disable(void)
{
	int i = 0;
	if(g_tve_task) {
		kthread_stop(g_tve_task);
		g_tve_task = NULL;
		for(i=0; i<SCREEN_COUNT;i++)
			tve_low_dac_autocheck_disable(i, 0);
	}
	return 0;
}
#else
void tv_report_hpd_work(void)
{
	pr_debug("there is null report hpd work,you need support the switch class!");
}

s32 tv_detect_thread(void *parg)
{
	pr_debug("there is null tv_detect_thread,you need support the switch class!");
	return -1;
}

s32 tv_detect_enable(void)
{
	pr_debug("there is null tv_detect_enable,you need support the switch class!");
	return -1;
}

s32 tv_detect_disable(void)
{
	pr_debug("there is null tv_detect_disable,you need support the switch class!");
    	return -1;
}
#endif

s32 tv_get_dac_hpd(u32 sel)
{
	u8 dac = 0;
	u32 ret = DISP_TV_NONE;

	if(SCREEN_COUNT-1 < sel && SCREEN_COUNT > 0)
		sel--;

	dac = tve_low_get_dac_status(sel);
	if(dac>1) {
		dac = 0;
	}
	if(g_tv_info.screen[sel].dac_source[0] == DISP_TV_DAC_SRC_COMPOSITE && dac == 1) {
		ret = DISP_TV_CVBS;
	}
	return  ret;
}

s32 tv_get_video_info(s32 mode)
{
	s32 i,count;
	count = sizeof(video_timing)/sizeof(disp_video_timings);
	for(i=0;i<count;i++) {
		if(mode == video_timing[i].tv_mode)
			return i;
	}
	return -1;
}

s32 tv_get_list_num(void)
{
	return sizeof(video_timing)/sizeof(disp_video_timings);

}

s32 tv_set_enhance_mode(u32 sel, u32 mode)
{
	return tve_low_enhance(sel, mode);
}

s32 tv_init(void)
{
	disp_tv_func disp_func;
	s32 i = 0, j = 0;
	u32 sid = 0;
	s32 sid_turn = 0;
	script_item_u   val;
	script_item_value_type_e  type;
	char sub_key[20];
	g_suspend = false;

	type = script_get_item("tv_para", "tv_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT == type)
		g_used = val.val;

	if(g_used) {
		unsigned int value, output_type0, output_mode0, output_type1, output_mode1;

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
		switch_dev_register(&cvbs_switch_dev);
		switch_dev_register(&ypbpr_switch_dev);
		switch_dev_register(&svideo_switch_dev);
		switch_dev_register(&vga_switch_dev);
#endif

		mutex_init(&mlock);
		value = disp_boot_para_parse();
		pr_debug("[TV]:value = %d", value);
		output_type0 = (value >> 8) & 0xff;
		output_mode0 = (value) & 0xff;
		output_type1 = (value >> 24)& 0xff;
		output_mode1 = (value >> 16) & 0xff;

		if((output_type0 == DISP_OUTPUT_TYPE_TV) ||(output_type1 == DISP_OUTPUT_TYPE_TV)) {
			g_tv_info.enable = 1;
			for(i=0; i<SCREEN_COUNT; i++) {
				g_tv_info.screen[i].tv_mode = (output_type0 == DISP_OUTPUT_TYPE_TV)?output_mode0:output_mode1;
			}
			pr_debug("[TV]:g_tv_info.screen[0].tv_mode = %d", g_tv_info.screen[0].tv_mode);
		}

		type = script_get_item("tv_para", "tv_cali_offset", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
			for(i=0; i<SCREEN_COUNT; i++) {
				g_tv_info.screen[i].cali_offset = val.val;
			}
		}

		for(i=0; i<SCREEN_COUNT; i++) {
			sid = tve_low_get_sid(0x10);
			if (0 == sid)
				g_tv_info.screen[i].sid = 0x200;
			else {
				if(sid & (1<<9))
					sid_turn = 0 + (sid & 0x1ff);
				else
					sid_turn = 0 - (sid & 0x1ff);

				sid_turn += 91;

				if(sid_turn >= 0)
					sid_turn = (1<<9) | sid_turn;
				else
					sid_turn = 0 - sid_turn;
				sid = (u32)sid_turn;

				g_tv_info.screen[i].sid = sid;
			}
		}

		for(i=0; i<SCREEN_COUNT; i++) {
			tve_low_set_reg_base(i, g_tv_info.screen[i].base_address);
			tve_clk_init(i);
			tve_clk_config(i, g_tv_info.screen[i].tv_mode);
			tve_clk_enable(i);
		}
		tv_detect_enable();

		for(i=0; i<4; i++) {
			sprintf(sub_key, "tv_dac_src%d", i);
			type = script_get_item("tv_para", sub_key, &val);
			if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
				for(j=0; j<SCREEN_COUNT; j++) {
					g_tv_info.screen[j].dac_source[i] = val.val;
				}
	        }
			else
				break;
		}

		memset(&disp_func, 0, sizeof(disp_tv_func));
		disp_func.tv_enable = tv_enable;
		disp_func.tv_disable = tv_disable;
		disp_func.tv_resume = tv_resume;
		disp_func.tv_suspend = tv_suspend;
		disp_func.tv_get_mode = tv_get_mode;
		disp_func.tv_set_mode = tv_set_mode;
		disp_func.tv_get_video_timing_info = tv_get_video_timing_info;
		disp_func.tv_get_input_csc = tv_get_input_csc;
		disp_func.tv_mode_support = tv_mode_support;
		disp_func.tv_hot_plugging_detect = tv_hot_plugging_detect;
		disp_func.tv_set_enhance_mode = tv_set_enhance_mode;
		disp_tv_register(&disp_func);
	}
	return 0;
}

s32 tv_exit(void)
{
	s32 i;
	mutex_lock(&mlock);
	tv_detect_disable();
	mutex_unlock(&mlock);
	for(i=0; i <SCREEN_COUNT; i++) {
		tv_disable(i);
	}
	return 0;
}


s32 tv_get_mode(u32 sel)
{
	if(SCREEN_COUNT-1 < sel && SCREEN_COUNT > 0)
		sel--;
	return g_tv_info.screen[sel].tv_mode;
}

s32 tv_set_mode(u32 sel, disp_tv_mode tv_mode)
{
	if(tv_mode >= DISP_TV_MODE_NUM) {
		return -1;
	}

	if(SCREEN_COUNT-1 < sel && SCREEN_COUNT > 0)
		sel--;

	mutex_lock(&mlock);
	g_tv_info.screen[sel].tv_mode = tv_mode;
	mutex_unlock(&mlock);
	return  0;
}

s32 tv_get_input_csc(void)
{
	return 1;   	//support yuv only
}

s32 tv_get_video_timing_info(u32 sel, disp_video_timings **video_info)
{
	disp_video_timings *info;
	int ret = -1;
	int i, list_num;
	info = video_timing;

	list_num = tv_get_list_num();
	if(SCREEN_COUNT-1 < sel && SCREEN_COUNT > 0)
		sel--;
	for(i=0; i<list_num; i++) {
		mutex_lock(&mlock);
		if(info->tv_mode == g_tv_info.screen[sel].tv_mode){
			*video_info = info;
			ret = 0;
			mutex_unlock(&mlock);
			break;
		}
		mutex_unlock(&mlock);
		info ++;
	}
	return ret;
}


s32 tv_enable(u32 sel)
{
	if(SCREEN_COUNT-1 < sel && SCREEN_COUNT > 0)
		sel--;

	if(!g_tv_info.enable) {
		tve_low_set_tv_mode(sel, g_tv_info.screen[sel].tv_mode, g_tv_info.screen[sel].sid);
		tve_low_open(sel);
		mutex_lock(&mlock);
		g_tv_info.enable = 1;
		mutex_unlock(&mlock);
	}
	return 0;
}

s32 tv_disable(u32 sel)
{
	if(SCREEN_COUNT-1 < sel && SCREEN_COUNT > 0)
		sel--;

	mutex_lock(&mlock);
	if(g_tv_info.enable) {
		tve_low_close(sel);
		g_tv_info.enable = 0;
	}
	mutex_unlock(&mlock);
	return 0;
}

s32 tv_suspend(void)
{
	int i = 0;

	mutex_lock(&mlock);
	if(g_used && (0 == g_suspend)) {
		g_suspend = true;
		tv_detect_disable();
		for(i=0; i<SCREEN_COUNT;i++)
			tve_clk_disable(i);
	}
	mutex_unlock(&mlock);

	return 0;
}

s32 tv_resume(void)
{
	int i = 0;

	mutex_lock(&mlock);
	if(g_used && (1 == g_suspend)) {
		g_suspend= false;
		for(i=0; i<SCREEN_COUNT;i++) {
			tve_clk_enable(i);
			tve_low_init(i, g_tv_info.screen[i].sid, g_tv_info.screen[i].cali_offset);

		}
		tv_detect_enable();
	}
	mutex_unlock(&mlock);

	return  0;
}

s32 tv_mode_support(disp_tv_mode mode)
{
	u32 i, list_num;
	disp_video_timings *info;


	info = video_timing;
	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		if(info->tv_mode == mode) {
			return 1;
		}
		info ++;
	}
	return 0;
}

s32 tv_hot_plugging_detect (u32 state)
{
	int i = 0;
	for(i=0; i<SCREEN_COUNT;i++) {
		if(state == STATUE_OPEN) {
			return tve_low_dac_autocheck_enable(i,0);
		}
		else if(state == STATUE_CLOSE){
			return tve_low_dac_autocheck_disable(i,0);
		}
	}
	return 0;
}


static void tve_clk_init(u32 sel)
{
	disp_sys_clk_set_parent("tve", TV_SOURCE);
}


static void tve_clk_enable(u32 sel)
{
	disp_sys_clk_enable("tve");
}

static void tve_clk_disable(u32 sel)
{
	disp_sys_clk_disable("tve");
}

static void tve_clk_config(u32 sel, u32 tv_mode)
{
	disp_sys_clk_set_rate("tve", 216000000);
}

