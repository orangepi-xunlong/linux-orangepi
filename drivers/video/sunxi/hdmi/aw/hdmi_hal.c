#include "../hdmi_hal.h"
#include "hdmi_interface.h"
#include "hdmi_core.h"

volatile __u32 HDMI_BASE = 0;

extern __s32			hdmi_state;
extern bool		        video_enable;
extern __u32			video_mode;
extern HDMI_AUDIO_INFO  audio_info;
extern __u8				sw_init_flag;

void Hdmi_set_reg_base(__u32 base)
{
	HDMI_BASE = base;
}

void Hdmi_set_sw_init_flag(__u8 flag)
{
	sw_init_flag = flag;
}

__u8 Hdmi_get_sw_init_flag(void)
{
	return sw_init_flag;
}

__s32 Hdmi_hal_video_enable(bool enable)
{
	if((video_enable != enable) && (hdmi_state >= HDMI_State_Video_config))
		hdmi_state = HDMI_State_Video_config;

	if((enable == 0) && (hdmi_state >= HDMI_State_Wait_Video_config) )
	{
		hdmi_state = HDMI_State_Wait_Video_config;
	}

	video_enable = enable;
	return 0;
}

__s32 Hdmi_hal_set_display_mode(__u32 hdmi_mode)
{
	if(hdmi_mode != video_mode) {
		if(hdmi_state >= HDMI_State_Video_config)
			hdmi_state = HDMI_State_Video_config;

		video_mode = hdmi_mode;
	}
	return 0;
}


__s32 Hdmi_hal_audio_enable(__u8 mode, __u8 channel)
{
	audio_info.audio_en = (channel == 0)?0:1;
	return 0;
}

__s32 Hdmi_hal_set_audio_para(hdmi_audio_t * audio_para)
{
	if(!audio_para)
		return -1;

  __inf("sample_bit:%d in Hdmi_hal_set_audio_para, audio_para->sample_bit:%d\n", audio_info.sample_bit, audio_para->sample_bit);
  if(audio_para->sample_bit != audio_info.sample_bit) {
		audio_info.sample_bit   = audio_para->sample_bit;

		__inf("sample_bit:%d in Hdmi_hal_set_audio_para\n", audio_info.sample_bit);
  }

	if(audio_para->sample_rate != audio_info.sample_rate) {
		audio_info.sample_rate = audio_para->sample_rate;
		__inf("sample_rate:%d in Hdmi_hal_set_audio_para\n", audio_info.sample_rate);
	}

	if(audio_para->channel_num != audio_info.channel_num) {
		audio_info.channel_num = audio_para->channel_num;
		__inf("channel_num:%d in Hdmi_hal_set_audio_para\n", audio_info.channel_num);
	}
	if(audio_para->data_raw != audio_info.data_raw) {
		audio_info.data_raw = audio_para->data_raw;
		__inf("data_raw:%d in Hdmi_hal_set_audio_para\n", audio_info.data_raw);
	}
	if(audio_para->ca != audio_info.ca) {
		audio_info.ca = audio_para->ca;
		__inf("ca:%d in Hdmi_hal_set_audio_para\n", audio_info.ca);
	}
	if (hdmi_state >= HDMI_State_Audio_config) {
		hdmi_state = HDMI_State_Audio_config;
	}
	return 0;
}
__s32 Hdmi_hal_cts_enable(__u32 mode)
{
	if(mode) {
		cts_enable = 1;
	} else {
		cts_enable = 0;
	}
	return 0;
}
__s32 Hdmi_hal_hdcp_enable(__u32 mode)
{
	if(hdcp_enable != mode) {
		if(hdmi_state >= HDMI_State_Video_config)
			hdmi_state = HDMI_State_Video_config;
	}
	if(mode)
		hdcp_enable = 1;
	else
		hdcp_enable = 0;

	return 0;
}

__s32 Hdmi_hal_get_hdcp_enable(void)
{
	return hdcp_enable;
}

__s32 Hdmi_hal_dvi_support(void)
{
	if((!isHDMI) && (cts_enable ==1))
		return 1;
	else
		return 0;
}

__s32 Hdmi_hal_mode_support(__u32 mode)
{
	if(Hpd_Check() == 0)
		return 0;
	else {
		while(hdmi_state < HDMI_State_Wait_Video_config)
			hdmi_delay_ms(10);

		return Device_Support_VIC[mode];
	}
}
//0:rgb, 1:yuv
__s32 Hmdi_hal_get_input_csc(void)
{
	if(cts_enable &&(!YCbCr444_Support))
		return 0;
	else
		return 1;
}
__s32 Hdmi_hal_get_HPD(void)
{
	return Hpd_Check();
}

__s32 Hdmi_hal_get_state(void)
{
	return hdmi_state;
}

__s32 Hdmi_hal_set_state(__s32 state)
{
	hdmi_state = state;
	return 0;
}

__s32 Hdmi_hal_set_video_enable(__s32 en)
{
	video_enable = en;
	return 0;
}

__s32 Hdmi_hal_set_pll(__u32 pll, __u32 clk)
{
	hdmi_pll = pll;
	hdmi_clk = clk;
	return 0;
}

__s32 Hdmi_hal_main_task(void)
{
	hdmi_main_task_loop();
	return 0;
}

__s32 Hdmi_hal_get_video_info(__s32 vic)
{
	return get_video_info(vic);
}

__s32 Hdmi_hal_enter_lp(void)
{
	return video_enter_lp();
}

__s32 Hdmi_hal_init(void)
{
	//hdmi_audio_t audio_para;
	hdmi_core_initial();
	audio_info.channel_num  = 2;
	//for audio test
#if 0
	audio_para.ch0_en = 1;
	audio_para.sample_rate = 44100;
	Hdmi_hal_set_audio_para(&audio_para);
	Hdmi_hal_audio_enable(0, 1);
#endif
	return 0;
}

__s32 Hdmi_hal_exit(void)
{
	return 0;
}

__s32 Hdmi_hal_is_playback(void)
{
	if(hdmi_state == HDMI_State_Playback)
		return 1;
	else
		return 0;
}
