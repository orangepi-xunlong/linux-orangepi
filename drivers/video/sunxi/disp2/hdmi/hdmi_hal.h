#ifndef __HDMI_HAL_H__
#define __HDMI_HAL_H__

#include "drv_hdmi_i.h"

#define HDMI1440_480I		6
#define HDMI1440_576I		21
#define HDMI480P			2
#define HDMI576P			17
#define HDMI720P_50			19
#define HDMI720P_60 		4
#define HDMI1080I_50		20
#define HDMI1080I_60		5
#define HDMI1080P_50		31
#define HDMI1080P_60 		16
#define HDMI1080P_24 		32
#define HDMI1080P_25 		33
#define HDMI1080P_30 		34
#define HDMI800_480P            35
#define HDMI1024_768P           36
#define HDMI1280_1024P          37
#define HDMI1360_768P           38
#define HDMI1440_900P           39
#define HDMI1680_1050P          40
#define HDMI2048_1536P          41
#define HDMI1080P_24_3D_FP  (HDMI1080P_24 +0x80)
#define HDMI720P_50_3D_FP   (HDMI720P_50  +0x80)
#define HDMI720P_60_3D_FP   (HDMI720P_60  +0x80)
#define HDMI3840_2160P_30   (1+0x100)
#define HDMI3840_2160P_25   (2+0x100)
#define HDMI3840_2160P_24   (3+0x100)

#define HDMI_EDID_LEN 1024
#define HDMI_EDID            511

extern void hdmi_delay_ms(__u32 t);
extern void hdmi_delay_us(unsigned long us);
extern unsigned int hdmi_get_soc_version(void);

extern void  Hdmi_set_reg_base(__u32 base);
extern __u32 Hal_IsHdcpDriver(void);
extern __s32 Hdmi_hpd_event(void);
extern __s32 Hdmi_hal_init(bool sw_only);
extern __s32 Hdmi_hal_exit(void);
extern __s32 Hdmi_hal_video_enable(bool enable);
extern __s32 Hdmi_hal_set_display_mode(__u32 hdmi_mode);
extern __s32 Hdmi_hal_audio_enable(__u8 mode, __u8 channel);
extern __s32 Hdmi_hal_set_audio_para(hdmi_audio_t * audio_para);
extern __s32 Hdmi_hal_mode_support(__u32 mode);
extern __s32 Hdmi_hal_get_HPD(void);
extern __s32 Hmdi_hal_get_input_csc(void);
extern __s32 Hdmi_hal_get_state(void);
extern __s32 Hdmi_hal_main_task(void);
extern __s32 Hdmi_hal_set_pll(__u32 pll, __u32 clk);
//extern __s32 Hdmi_hal_dvi_enable(__u32 mode);
extern __s32 Hdmi_hal_cts_enable(__u32 mode);
extern __s32 Hdmi_hal_dvi_support(void);
extern __s32 Hdmi_hal_hdcp_enable(__u32 mode);
extern __s32 Hdmi_hal_get_hdcp_enable(void);
extern __s32 Hdmi_hal_suspend(void);
extern __s32 Hdmi_hal_resume(void);
extern __s32 Hdmi_hal_get_video_info(__s32 vic);
extern __s32 Hdmi_hal_get_list_num(void);
extern __s32 Hdmi_hal_enter_lp(void);
extern __s32 Hdmi_hal_exit_lp(void);
extern __s32 Hdmi_hal_is_playback(void);
extern __s32 Hdmi_hal_get_edid(void);

#endif

