#include "drv_hdmi_i.h"
#include "hdmi_hal.h"
#include "dev_hdmi.h"
#include <linux/regulator/consumer.h>
#include <linux/clk-private.h>

static struct semaphore *run_sem = NULL;
static struct task_struct * HDMI_task;
static char hdmi_power[25];
static bool hdmi_power_used;
static bool hdmi_used;
#if defined(CONFIG_COMMON_CLK)
static struct clk *hdmi_clk = NULL;
static struct clk *hdmi_ddc_clk = NULL;
#endif
static __u32 power_enable_count = 0;
static __u32 clk_enable_count = 0;
static struct mutex mlock;
static bool audio_enable = false;
static bool b_hdmi_suspend;
static bool b_hdmi_suspend_pre;
__s32 Hdmi_suspend(void);
__s32 Hdmi_resume(void);

void hdmi_delay_ms(__u32 t)
{
	__u32 timeout = t*HZ/1000;
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(timeout);
}

void hdmi_delay_us(unsigned long us)
{
	udelay(us);
}

#if defined(CONFIG_COMMON_CLK)
static void hdmi_clk_init(void)
{
	struct clk* hdmi_pll = NULL;
	hdmi_clk = clk_get(NULL, "hdmi");
	if(IS_ERR(hdmi_clk) || (hdmi_clk == NULL)) {
		hdmi_clk = NULL;
		__wrn("fail get hdmi clk\n");
		return ;
	}
	hdmi_ddc_clk = clk_get(NULL, "hdmi_slow");
	if(IS_ERR(hdmi_ddc_clk) || (hdmi_ddc_clk == NULL)) {
		hdmi_ddc_clk = NULL;
		__wrn("fail get hdmi ddc clk\n");
		clk_put(hdmi_clk);
		hdmi_clk = NULL;
		return;
	}
	hdmi_pll = clk_get(NULL, "pll8");
	if(IS_ERR(hdmi_pll) || (hdmi_pll == NULL)) {
		hdmi_pll = NULL;
		__wrn("fail get pll7 clk\n");
	}
	if((NULL != hdmi_clk) && (NULL != hdmi_pll)) {
		if(0 != clk_set_parent(hdmi_clk, hdmi_pll)) {
			__wrn("fail to set parent %s for clk %s\n", "pll8", "hdmi");
		}
	}
	clk_enable_count = hdmi_clk->enable_count;

	clk_put(hdmi_pll);
}

static void hdmi_clk_exit(void)
{
	if(hdmi_clk)
		clk_put(hdmi_clk);
	if(hdmi_ddc_clk)
		clk_put(hdmi_ddc_clk);
	hdmi_clk = NULL;
	hdmi_ddc_clk = NULL;
}

static void hdmi_clk_enable(void)
{
	if(hdmi_clk)
		clk_prepare_enable(hdmi_clk);
	if(hdmi_ddc_clk)
		clk_prepare_enable(hdmi_ddc_clk);
}

static void hdmi_clk_disable(void)
{
	if(hdmi_clk)
		clk_disable(hdmi_clk);
	if(hdmi_ddc_clk)
		clk_disable(hdmi_ddc_clk);
}

static void hdmi_clk_config(u32 vic)
{
	int index = 0;

	index = Hdmi_hal_get_video_info(vic);
	if(hdmi_clk)
		clk_set_rate(hdmi_clk, video_timing[index].pixel_clk);
}
#else
static void hdmi_clk_init(void){}
static void hdmi_clk_exit(void){}
static void hdmi_clk_enable(void){}
static void hdmi_clk_disable(void){}
static void hdmi_clk_config(u32 vic){}
#endif

#ifdef CONFIG_AW_AXP
static int hdmi_power_enable(char *name)
{
	struct regulator *regu= NULL;
	int ret = 0;
	regu= regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		pr_err("%s: some error happen, fail to get regulator %s\n", __func__, name);
		goto exit;
	}

	//enalbe regulator
	ret = regulator_enable(regu);
	if (0 != ret) {
		pr_err("%s: some error happen, fail to enable regulator %s!\n", __func__, name);
		goto exit1;
	} else {
		__inf("suceess to enable regulator %s!\n", name);
	}

exit1:
	//put regulater, when module exit
	regulator_put(regu);
exit:
	return ret;
}

static int hdmi_power_disable(char *name)
{
	struct regulator *regu= NULL;
	int ret = 0;
	regu= regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		__wrn("%s: some error happen, fail to get regulator %s\n", __func__, name);
		goto exit;
	}

	//disalbe regulator
	ret = regulator_disable(regu);
	if (0 != ret) {
		__wrn("%s: some error happen, fail to disable regulator %s!\n", __func__, name);
		goto exit1;
	} else {
		__inf("suceess to disable regulator %s!\n", name);
	}

exit1:
	//put regulater, when module exit
	regulator_put(regu);
exit:
	return ret;
}
#else
int hdmi_power_enable(char *name){return 0;}
int hdmi_power_disable(char *name){return 0;}
#endif

__s32 Hdmi_open(void)
{
	__inf("[Hdmi_open]\n");

	mutex_lock(&mlock);
	if(1 != ghdmi.bopen) {
		if(0 == Hdmi_get_sw_init_flag())
		{
			hdmi_clk_config(ghdmi.mode);
		}

		Hdmi_hal_video_enable(1);
		ghdmi.bopen = 1;
	}
	mutex_unlock(&mlock);
	return 0;
}

__s32 Hdmi_close(void)
{
	__inf("[Hdmi_close]\n");

	mutex_lock(&mlock);
	if(0 != ghdmi.bopen) {
		Hdmi_hal_video_enable(0);
		ghdmi.bopen = 0;
	}
	mutex_unlock(&mlock);
	return 0;
}

static struct disp_hdmi_mode hdmi_mode_tbl[] = {
	{DISP_TV_MOD_480I,                HDMI1440_480I,     },
	{DISP_TV_MOD_576I,                HDMI1440_576I,     },
	{DISP_TV_MOD_480P,                HDMI480P,          },
	{DISP_TV_MOD_576P,                HDMI576P,          },
	{DISP_TV_MOD_720P_50HZ,           HDMI720P_50,       },
	{DISP_TV_MOD_720P_60HZ,           HDMI720P_60,       },
	{DISP_TV_MOD_1080I_50HZ,          HDMI1080I_50,      },
	{DISP_TV_MOD_1080I_60HZ,          HDMI1080I_60,      },
	{DISP_TV_MOD_1080P_24HZ,          HDMI1080P_24,      },
	{DISP_TV_MOD_1080P_50HZ,          HDMI1080P_50,      },
	{DISP_TV_MOD_1080P_60HZ,          HDMI1080P_60,      },
	{DISP_TV_MOD_1080P_25HZ,          HDMI1080P_25,      },
	{DISP_TV_MOD_1080P_30HZ,          HDMI1080P_30,      },
	{DISP_TV_MOD_1080P_24HZ_3D_FP,    HDMI1080P_24_3D_FP,},
	{DISP_TV_MOD_720P_50HZ_3D_FP,     HDMI720P_50_3D_FP, },
	{DISP_TV_MOD_720P_60HZ_3D_FP,     HDMI720P_60_3D_FP, },
	{DISP_TV_MOD_3840_2160P_30HZ,     HDMI3840_2160P_30, },
	{DISP_TV_MOD_3840_2160P_25HZ,     HDMI3840_2160P_25, },
};

__s32 Hdmi_set_display_mode(disp_tv_mode mode)
{
	__u32 hdmi_mode;
	__u32 i;
	bool find = false;

	__inf("[Hdmi_set_display_mode],mode:%d\n",mode);

	for(i=0; i<sizeof(hdmi_mode_tbl)/sizeof(struct disp_hdmi_mode); i++)
	{
		if(hdmi_mode_tbl[i].mode == mode) {
			hdmi_mode = hdmi_mode_tbl[i].hdmi_mode;
			find = true;
			break;
		}
	}

	if(find) {
		ghdmi.mode = hdmi_mode;
		return Hdmi_hal_set_display_mode(hdmi_mode);
	} else {
		__wrn("unsupported video mode %d when set display mode\n", mode);
		return -1;
	}

}

__s32 Hdmi_Audio_Enable(__u8 mode, __u8 channel)
{
	__inf("[Hdmi_Audio_Enable],ch:%d\n",channel);
	mutex_lock(&mlock);
	audio_enable = mode;
	mutex_unlock(&mlock);
	return Hdmi_hal_audio_enable(mode, channel);
}

__s32 Hdmi_Set_Audio_Para(hdmi_audio_t * audio_para)
{
	__inf("[Hdmi_Set_Audio_Para]\n");
	return Hdmi_hal_set_audio_para(audio_para);
}

__s32 Hdmi_Is_Playback(void)
{
	return Hdmi_hal_is_playback();
}

__s32 Hdmi_mode_support(disp_tv_mode mode)
{
	__u32 hdmi_mode;
	__u32 i;
	bool find = false;

	for(i=0; i<sizeof(hdmi_mode_tbl)/sizeof(struct disp_hdmi_mode); i++)
	{
		if(hdmi_mode_tbl[i].mode == mode) {
			hdmi_mode = hdmi_mode_tbl[i].hdmi_mode;
			find = true;
			break;
		}
	}

	if(find) {
		return Hdmi_hal_mode_support(hdmi_mode);
	} else {
		return 0;
	}
}

__s32 Hdmi_get_HPD_status(void)
{
	return Hdmi_hal_get_HPD();
}


__s32 Hdmi_set_pll(__u32 pll, __u32 clk)
{
	Hdmi_hal_set_pll(pll, clk);
	return 0;
}

__s32 Hdmi_dvi_enable(__u32 mode)
{
	return Hdmi_hal_cts_enable(mode);//Hdmi_hal_dvi_enable(mode);
}

__s32 Hdmi_dvi_support(void)
{
	return Hdmi_hal_dvi_support();
}

__u32 Hdmi_hdcp_enable(__u32 hdcp_en)
{
	return Hdmi_hal_hdcp_enable(hdcp_en);
}

__s32 Hdmi_get_hdcp_enable(void)
{
	return Hdmi_hal_get_hdcp_enable();
}

__s32 Hdmi_get_video_timming_info(disp_video_timing **video_info)
{
	*video_info = video_timing;
	return 0;
}

int Hdmi_get_video_info_index(u32 mode_id)
{
	u32 i;
	u32 vic = 0;
	u32 ret = 0;
	disp_video_timing *video_info;

	for(i = 0; i < sizeof(hdmi_mode_tbl) / sizeof(struct disp_hdmi_mode); i++) {
		if(hdmi_mode_tbl[i].mode == mode_id) {
			vic = hdmi_mode_tbl[i].hdmi_mode;
			ret = 1;
			break;
		}
	}

	if(ret == 0)
		return -1;

	video_info = video_timing;
	i = 0;

	while(video_info) {
		if(video_info->vic == vic)
			return i;

		i++;
		video_info = video_info + 1;
	}

	return -1;
}

__s32 Hdmi_get_input_csc(void)
{
	return Hmdi_hal_get_input_csc();
}
static int Hdmi_run_thread(void *parg)
{
	while (1) {
		if(kthread_should_stop()) {
			break;
		}

		mutex_lock(&mlock);
		if(false == b_hdmi_suspend) {
			/* normal state */
			if(clk_enable_count == 0) {
				hdmi_clk_enable();
				clk_enable_count ++;
			}
			if((hdmi_power_used) && (power_enable_count == 0)) {
				hdmi_power_enable(hdmi_power);
				power_enable_count ++;
			}
			b_hdmi_suspend_pre = b_hdmi_suspend;
			mutex_unlock(&mlock);
			Hdmi_hal_main_task();
		} else {
			/* suspend state */
			if(false == b_hdmi_suspend_pre) {
				/* first time after enter suspend state */
				Hdmi_hal_enter_lp();
			}
			if((false == audio_enable) && (clk_enable_count != 0)) {
				hdmi_clk_disable();
				clk_enable_count --;
			}
			if((hdmi_power_used) && (power_enable_count != 0)) {
				hdmi_power_disable(hdmi_power);
				power_enable_count --;
			}
			b_hdmi_suspend_pre = b_hdmi_suspend;
			mutex_unlock(&mlock);
		}
		if(Hdmi_get_hdcp_enable()==1)
			hdmi_delay_ms(100); //200
		else
			hdmi_delay_ms(200);
	}

	return 0;
}

static struct switch_dev hdmi_switch_dev = {
	.name = "hdmi",
};

static void hdmi_report_hpd_work(struct work_struct *work)
{
	if(Hdmi_get_HPD_status())	{
		if(hdmi_switch_dev.dev)
			switch_set_state(&hdmi_switch_dev, 1);
		__inf("switch_set_state 1\n");
	}	else {
		if(hdmi_switch_dev.dev)
			switch_set_state(&hdmi_switch_dev, 0);
		__inf("switch_set_state 0\n");
	}
}

__s32 hdmi_hpd_state(__u32 state)
{
	if(state == 0) {
		if(hdmi_switch_dev.dev)
			switch_set_state(&hdmi_switch_dev, 0);
	} else {
		if(hdmi_switch_dev.dev)
			switch_set_state(&hdmi_switch_dev, 1);
	}

	return 0;
}
/**
 * hdmi_hpd_report - report hdmi hot plug state to user space
 * @hotplug:	0: hdmi plug out;   1:hdmi plug in
 *
 * always return success.
 */
__s32 Hdmi_hpd_event(void)
{
	schedule_work(&ghdmi.hpd_work);
	return 0;
}

#if defined(CONFIG_SND_SUNXI_SOC_HDMIAUDIO)
extern void audio_set_hdmi_func(__audio_hdmi_func * hdmi_func);
#endif
extern __s32 disp_set_hdmi_func(u32 screen_id, disp_hdmi_func * func);
extern u32 disp_get_disp_rsl(void);

static __u32 hdmi_parse_disp_rsl(__u32 *hdmimode)
{
	__u32 type, mode;
	__u32 value = disp_get_disp_rsl();

	if(0 != ((value >> 24) & 0xFF)) {
		return 0;
	}
	type = (value >> 16) & 0xFF;
	if(type != DISP_OUTPUT_TYPE_HDMI) {
		return 0;
	}
	mode = (value >> 8) & 0xFF;
	if(mode >= DISP_TV_MODE_NUM) {
		return 0;
	}
	*hdmimode = mode;
	return 1;
}

__s32 Hdmi_init(void)
{
#if defined(CONFIG_SND_SUNXI_SOC_HDMIAUDIO)
	__audio_hdmi_func audio_func;
#endif
	disp_hdmi_func disp_func;
	__u32 output_mode;

	script_item_u   val;
	script_item_value_type_e  type;

	hdmi_used = 0;
	b_hdmi_suspend_pre = b_hdmi_suspend = false;
	hdmi_power_used = 0;

	type = script_get_item("hdmi_para", "hdmi_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
		hdmi_used = val.val;

		if(hdmi_used)
		{
			if(hdmi_parse_disp_rsl(&output_mode))
			{
				Hdmi_set_display_mode(output_mode);
				Hdmi_hal_set_video_enable(1);
				Hdmi_set_sw_init_flag(1);
			} else {
				Hdmi_set_sw_init_flag(0);
			}
			mutex_init(&mlock);
			hdmi_clk_init();
			mutex_lock(&mlock);
			hdmi_clk_enable(); //enable clk for hpd
			clk_enable_count ++;
			mutex_unlock(&mlock);
			INIT_WORK(&ghdmi.hpd_work, hdmi_report_hpd_work);
			switch_dev_register(&hdmi_switch_dev);

			type = script_get_item("hdmi_para", "hdmi_power", &val);
			if(SCIRPT_ITEM_VALUE_TYPE_STR == type) {
				hdmi_power_used = 1;
				if(hdmi_power_used) {
					memcpy((void*)hdmi_power, (void*)val.str, strlen(val.str)+1);
					mutex_lock(&mlock);
					hdmi_power_enable(hdmi_power);
					power_enable_count ++;
					mutex_unlock(&mlock);
				}
			}
			type = script_get_item("hdmi_para", "hdmi_cts_compatibility", &val);
			if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
				Hdmi_hal_cts_enable(val.val);
			}
			type = script_get_item("hdmi_para", "hdmi_hdcp_enable", &val);
			if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
				Hdmi_hal_hdcp_enable(val.val);
			}
			Hdmi_set_reg_base((__u32)ghdmi.base_hdmi);
			Hdmi_hal_init();

			run_sem = kmalloc(sizeof(struct semaphore),GFP_KERNEL | __GFP_ZERO);
			sema_init((struct semaphore*)run_sem,0);

			HDMI_task = kthread_create(Hdmi_run_thread, (void*)0, "hdmi proc");
			if(IS_ERR(HDMI_task)) {
				__s32 err = 0;
				__wrn("Unable to start kernel thread %s.\n","hdmi proc");
				err = PTR_ERR(HDMI_task);
				HDMI_task = NULL;
				return err;
			}
			wake_up_process(HDMI_task);

#if defined(CONFIG_SND_SUNXI_SOC_HDMIAUDIO)
			audio_func.hdmi_audio_enable = Hdmi_Audio_Enable;
			audio_func.hdmi_set_audio_para = Hdmi_Set_Audio_Para;
			audio_func.hdmi_is_playback = Hdmi_Is_Playback;
			audio_set_hdmi_func(&audio_func);
#endif
			memset(&disp_func, 0, sizeof(disp_hdmi_func));
			disp_func.hdmi_open = Hdmi_open;
			disp_func.hdmi_close = Hdmi_close;
			disp_func.hdmi_set_mode = Hdmi_set_display_mode;
			disp_func.hdmi_mode_support = Hdmi_mode_support;
			disp_func.hdmi_get_HPD_status = Hdmi_get_HPD_status;
			disp_func.hdmi_set_pll = Hdmi_set_pll;
			disp_func.hdmi_dvi_enable= Hdmi_dvi_enable;
			disp_func.hdmi_dvi_support= Hdmi_dvi_support;
			disp_func.hdmi_get_input_csc = Hdmi_get_input_csc;
			disp_func.hdmi_get_hdcp_enable = Hdmi_get_hdcp_enable;
			disp_func.hdmi_get_video_timing_info = Hdmi_get_video_timming_info;
			disp_func.hdmi_get_video_info_index = Hdmi_get_video_info_index;
			disp_func.hdmi_suspend = Hdmi_suspend;
			disp_func.hdmi_resume = Hdmi_resume;
			disp_set_hdmi_func(0, &disp_func);
			disp_set_hdmi_func(1, &disp_func);
			if(Hdmi_get_sw_init_flag())
			{
				Hdmi_open();
			}
			Hdmi_set_sw_init_flag(0);
		}
	}
	return 0;
}

__s32 Hdmi_exit(void)
{
	if(hdmi_used) {
		Hdmi_hal_exit();

		if(run_sem)	{
			kfree(run_sem);
		}

		run_sem = NULL;
		if(HDMI_task) {
			kthread_stop(HDMI_task);
			HDMI_task = NULL;
		}

		if((1 == hdmi_power_used) && (0 != power_enable_count)) {
			hdmi_power_disable(hdmi_power);
		}
		if(0 != clk_enable_count) {
			hdmi_clk_disable();
			clk_enable_count--;
		}
		hdmi_clk_exit();
	}

	return 0;
}

__s32 Hdmi_suspend(void)
{
	mutex_lock(&mlock);
	if(hdmi_used && (0 == b_hdmi_suspend)) {
		b_hdmi_suspend = true;
		pr_info("[HDMI]hdmi suspend\n");
	}
	mutex_unlock(&mlock);

	return 0;
}

__s32 Hdmi_resume(void)
{
	mutex_lock(&mlock);
	if(hdmi_used && (1 == b_hdmi_suspend)) {
		b_hdmi_suspend = false;
		pr_info("[HDMI]hdmi resume\n");
	}
	mutex_unlock(&mlock);

	return  0;
}
