#ifndef __DRV_DISPLAY_LEGACY_H__
#define __DRV_DISPLAY_LEGACY_H__

#define __bool signed char

typedef struct {__u8  alpha;__u8 red;__u8 green; __u8 blue; }__disp_color_t;
typedef struct {__s32 x; __s32 y; __u32 width; __u32 height;}__disp_rect_t;
typedef struct {__u32 width;__u32 height;                   }__disp_rectsz_t;
typedef struct {__s32 x; __s32 y;                           }__disp_pos_t;

typedef enum
{
	LCD_CMAP_B0	= 0x0,
	LCD_CMAP_G0	= 0x1,
	LCD_CMAP_R0	= 0x2,
	LCD_CMAP_B1	= 0x4,
	LCD_CMAP_G1	= 0x5,
	LCD_CMAP_R1	= 0x6,
	LCD_CMAP_B2	= 0x8,
	LCD_CMAP_G2	= 0x9,
	LCD_CMAP_R2	= 0xa,
	LCD_CMAP_B3	= 0xc,
	LCD_CMAP_G3	= 0xd,
	LCD_CMAP_R3	= 0xe,
}__lcd_cmap_color;



typedef enum
{
    DISP_FORMAT_1BPP        =0x0,
    DISP_FORMAT_2BPP        =0x1,
    DISP_FORMAT_4BPP        =0x2,
    DISP_FORMAT_8BPP        =0x3,
    DISP_FORMAT_RGB655      =0x4,
    DISP_FORMAT_RGB565      =0x5,
    DISP_FORMAT_RGB556      =0x6,
    DISP_FORMAT_ARGB1555    =0x7,
    DISP_FORMAT_RGBA5551    =0x8,
    DISP_FORMAT_ARGB888     =0x9,//alpha padding to 0xff
    DISP_FORMAT_ARGB8888    =0xa,
    DISP_FORMAT_RGB888      =0xb,
    DISP_FORMAT_ARGB4444    =0xc,

    DISP_FORMAT_YUV444      =0x10,
    DISP_FORMAT_YUV422      =0x11,
    DISP_FORMAT_YUV420      =0x12,
    DISP_FORMAT_YUV411      =0x13,
    DISP_FORMAT_CSIRGB      =0x14,
}__disp_pixel_fmt_t;


typedef enum
{
    DISP_MOD_INTERLEAVED        =0x1,   //interleaved,1个地址
    DISP_MOD_NON_MB_PLANAR      =0x0,   //无宏块平面模式,3个地址,RGB/YUV每个channel分别存放
    DISP_MOD_NON_MB_UV_COMBINED =0x2,   //无宏块UV打包模式,2个地址,Y和UV分别存放
    DISP_MOD_MB_PLANAR          =0x4,   //宏块平面模式,3个地址,RGB/YUV每个channel分别存放
    DISP_MOD_MB_UV_COMBINED     =0x6,   //宏块UV打包模式 ,2个地址,Y和UV分别存放
}__disp_pixel_mod_t;

typedef enum
{
//for interleave argb8888
    DISP_SEQ_ARGB   =0x0,//A在高位
    DISP_SEQ_BGRA   =0x2,
    
//for nterleaved yuv422
    DISP_SEQ_UYVY   =0x3,  
    DISP_SEQ_YUYV   =0x4,
    DISP_SEQ_VYUY   =0x5,
    DISP_SEQ_YVYU   =0x6,
    
//for interleaved yuv444
    DISP_SEQ_AYUV   =0x7,  
    DISP_SEQ_VUYA   =0x8,
    
//for uv_combined yuv420
    DISP_SEQ_UVUV   =0x9,  
    DISP_SEQ_VUVU   =0xa,
    
//for 16bpp rgb
    DISP_SEQ_P10    = 0xd,//p1在高位
    DISP_SEQ_P01    = 0xe,//p0在高位
    
//for planar format or 8bpp rgb
    DISP_SEQ_P3210  = 0xf,//p3在高位
    DISP_SEQ_P0123  = 0x10,//p0在高位
    
//for 4bpp rgb
    DISP_SEQ_P76543210  = 0x11,
    DISP_SEQ_P67452301  = 0x12,
    DISP_SEQ_P10325476  = 0x13,
    DISP_SEQ_P01234567  = 0x14,
    
//for 2bpp rgb
    DISP_SEQ_2BPP_BIG_BIG       = 0x15,//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    DISP_SEQ_2BPP_BIG_LITTER    = 0x16,//12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3
    DISP_SEQ_2BPP_LITTER_BIG    = 0x17,//3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12
    DISP_SEQ_2BPP_LITTER_LITTER = 0x18,//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    
//for 1bpp rgb
    DISP_SEQ_1BPP_BIG_BIG       = 0x19,//31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    DISP_SEQ_1BPP_BIG_LITTER    = 0x1a,//24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7
    DISP_SEQ_1BPP_LITTER_BIG    = 0x1b,//7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24
    DISP_SEQ_1BPP_LITTER_LITTER = 0x1c,//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
}__disp_pixel_seq_t;

typedef enum
{
    DISP_3D_SRC_MODE_TB = 0x0,//top bottom
    DISP_3D_SRC_MODE_FP = 0x1,//frame packing
    DISP_3D_SRC_MODE_SSF = 0x2,//side by side full
    DISP_3D_SRC_MODE_SSH = 0x3,//side by side half
    DISP_3D_SRC_MODE_LI = 0x4,//line interleaved
}__disp_3d_src_mode_t;

typedef enum
{
    DISP_3D_OUT_MODE_CI_1 = 0x5,//column interlaved 1
    DISP_3D_OUT_MODE_CI_2 = 0x6,//column interlaved 2
    DISP_3D_OUT_MODE_CI_3 = 0x7,//column interlaved 3
    DISP_3D_OUT_MODE_CI_4 = 0x8,//column interlaved 4
    DISP_3D_OUT_MODE_LIRGB = 0x9,//line interleaved rgb

    DISP_3D_OUT_MODE_TB = 0x0,//top bottom
    DISP_3D_OUT_MODE_FP = 0x1,//frame packing
    DISP_3D_OUT_MODE_SSF = 0x2,//side by side full
    DISP_3D_OUT_MODE_SSH = 0x3,//side by side half
    DISP_3D_OUT_MODE_LI = 0x4,//line interleaved
    DISP_3D_OUT_MODE_FA = 0xa,//field alternative
}__disp_3d_out_mode_t;

typedef enum
{
    DISP_BT601  = 0,
    DISP_BT709  = 1,
    DISP_YCC    = 2,
    DISP_VXYCC  = 3,
}__disp_cs_mode_t;

typedef enum
{
    DISP_COLOR_RANGE_16_255 = 0,
    DISP_COLOR_RANGE_0_255 = 1,
    DISP_COLOR_RANGE_16_235 = 2,
}__disp_color_range_t;

typedef enum
{
    DISP_OUTPUT_TYPE_NONE   = 0,
    DISP_OUTPUT_TYPE_LCD    = 1,
    DISP_OUTPUT_TYPE_TV     = 2,
    DISP_OUTPUT_TYPE_HDMI   = 4,
    DISP_OUTPUT_TYPE_VGA    = 8,
}__disp_output_type_t;

typedef enum
{
    DISP_TV_NONE    = 0,
    DISP_TV_CVBS    = 1,
    DISP_TV_YPBPR   = 2,
    DISP_TV_SVIDEO  = 4,
}__disp_tv_output_t;

typedef enum
{
    DISP_TV_MOD_480I                = 0,
    DISP_TV_MOD_576I                = 1,
    DISP_TV_MOD_480P                = 2,
    DISP_TV_MOD_576P                = 3,
    DISP_TV_MOD_720P_50HZ           = 4,
    DISP_TV_MOD_720P_60HZ           = 5,
    DISP_TV_MOD_1080I_50HZ          = 6,
    DISP_TV_MOD_1080I_60HZ          = 7,
    DISP_TV_MOD_1080P_24HZ          = 8,
    DISP_TV_MOD_1080P_50HZ          = 9,
    DISP_TV_MOD_1080P_60HZ          = 0xa,
    DISP_TV_MOD_1080P_24HZ_3D_FP    = 0x17,
    DISP_TV_MOD_720P_50HZ_3D_FP     = 0x18,
    DISP_TV_MOD_720P_60HZ_3D_FP     = 0x19,
    DISP_TV_MOD_1080P_25HZ          = 0x1a,
    DISP_TV_MOD_1080P_30HZ          = 0x1b,
    DISP_TV_MOD_PAL                 = 0xb,
    DISP_TV_MOD_PAL_SVIDEO          = 0xc,
    DISP_TV_MOD_NTSC                = 0xe,
    DISP_TV_MOD_NTSC_SVIDEO         = 0xf,
    DISP_TV_MOD_PAL_M               = 0x11,
    DISP_TV_MOD_PAL_M_SVIDEO        = 0x12,
    DISP_TV_MOD_PAL_NC              = 0x14,
    DISP_TV_MOD_PAL_NC_SVIDEO       = 0x15,
    DISP_TV_MODE_NUM               = 0x1c,
}__disp_tv_mode_t;

typedef enum
{
    DISP_TV_DAC_SRC_COMPOSITE = 0,
    DISP_TV_DAC_SRC_LUMA = 1,
    DISP_TV_DAC_SRC_CHROMA = 2,
    DISP_TV_DAC_SRC_Y = 4,
    DISP_TV_DAC_SRC_PB = 5,
    DISP_TV_DAC_SRC_PR = 6,
    DISP_TV_DAC_SRC_NONE = 7,
}__disp_tv_dac_source;

typedef enum
{
    DISP_VGA_H1680_V1050    = 0,
    DISP_VGA_H1440_V900     = 1,
    DISP_VGA_H1360_V768     = 2,
    DISP_VGA_H1280_V1024    = 3,
    DISP_VGA_H1024_V768     = 4,
    DISP_VGA_H800_V600      = 5,
    DISP_VGA_H640_V480      = 6,
    DISP_VGA_H1440_V900_RB  = 7,//not support yet
    DISP_VGA_H1680_V1050_RB = 8,//not support yet
    DISP_VGA_H1920_V1080_RB = 9,
    DISP_VGA_H1920_V1080    = 0xa,
    DISP_VGA_H1280_V720     = 0xb,
    DISP_VGA_MODE_NUM       = 0xc,
}__disp_vga_mode_t;


typedef enum
{
    DISP_LCDC_SRC_DE_CH1    = 0,
    DISP_LCDC_SRC_DE_CH2    = 1,
    DISP_LCDC_SRC_DMA888    = 2,
    DISP_LCDC_SRC_DMA565    = 3,    
    DISP_LCDC_SRC_WHITE     = 4,
    DISP_LCDC_SRC_BLACK     = 5,
    DISP_LCDC_SRC_BLUE      = 6,
}__disp_lcdc_src_t;

typedef enum
{
    DISP_LCD_BRIGHT_LEVEL0  = 0,
    DISP_LCD_BRIGHT_LEVEL1  = 1,
    DISP_LCD_BRIGHT_LEVEL2  = 2,
    DISP_LCD_BRIGHT_LEVEL3  = 3,
    DISP_LCD_BRIGHT_LEVEL4  = 4,
    DISP_LCD_BRIGHT_LEVEL5  = 5,
    DISP_LCD_BRIGHT_LEVEL6  = 6,
    DISP_LCD_BRIGHT_LEVEL7  = 7,
    DISP_LCD_BRIGHT_LEVEL8  = 8,
    DISP_LCD_BRIGHT_LEVEL9  = 9,
    DISP_LCD_BRIGHT_LEVEL10 = 0xa,
    DISP_LCD_BRIGHT_LEVEL11 = 0xb,
    DISP_LCD_BRIGHT_LEVEL12 = 0xc,
    DISP_LCD_BRIGHT_LEVEL13 = 0xd,
    DISP_LCD_BRIGHT_LEVEL14 = 0xe,
    DISP_LCD_BRIGHT_LEVEL15 = 0xf,
}__disp_lcd_bright_t;

typedef enum
{
    DISP_LAYER_WORK_MODE_NORMAL     = 0,    //normal work mode
    DISP_LAYER_WORK_MODE_PALETTE    = 1,    //palette work mode 
    DISP_LAYER_WORK_MODE_INTER_BUF  = 2,    //internal frame buffer work mode
    DISP_LAYER_WORK_MODE_GAMMA      = 3,    //gamma correction work mode
    DISP_LAYER_WORK_MODE_SCALER     = 4,    //scaler work mode
}__disp_layer_work_mode_t;

typedef enum
{
    DISP_VIDEO_NATUAL       = 0,
    DISP_VIDEO_SOFT         = 1,
    DISP_VIDEO_VERYSOFT     = 2,
    DISP_VIDEO_SHARP        = 3,
    DISP_VIDEO_VERYSHARP    = 4
}__disp_video_smooth_t;

typedef enum
{
    DISP_HWC_MOD_H32_V32_8BPP = 0,
    DISP_HWC_MOD_H64_V64_2BPP = 1,
    DISP_HWC_MOD_H64_V32_4BPP = 2,
    DISP_HWC_MOD_H32_V64_4BPP = 3,
}__disp_hwc_mode_t;

typedef enum
{
    DISP_EXIT_MODE_CLEAN_ALL    = 0,
    DISP_EXIT_MODE_CLEAN_PARTLY = 1,//only clean interrupt temply
}__disp_exit_mode_t;

typedef enum
{
    DISP_ENHANCE_MODE_RED       = 0x0,
    DISP_ENHANCE_MODE_GREEN     = 0x1,
    DISP_ENHANCE_MODE_BLUE      = 0x2,
    DISP_ENHANCE_MODE_CYAN      = 0x3,
    DISP_ENHANCE_MODE_MAGENTA   = 0x4,
    DISP_ENHANCE_MODE_YELLOW    = 0x5,
    DISP_ENHANCE_MODE_FLESH     = 0x6,
    DISP_ENHANCE_MODE_STANDARD  = 0x7,
    DISP_ENHANCE_MODE_VIVID     = 0x8,
    DISP_ENHANCE_MODE_SCENERY   = 0xa,
}__disp_enhance_mode_t;

typedef enum
{
    DISP_OUT_CSC_TYPE_LCD        = 0,
    DISP_OUT_CSC_TYPE_TV         = 1,
    DISP_OUT_CSC_TYPE_HDMI_YUV   = 2,
    DISP_OUT_CSC_TYPE_VGA        = 3,
    DISP_OUT_CSC_TYPE_HDMI_RGB   = 4,
}__disp_out_csc_type_t;

typedef enum
{
    DISP_MOD_FE0          = 0,
    DISP_MOD_FE1          = 1,
    DISP_MOD_BE0          = 2,
    DISP_MOD_BE1          = 3,
    DISP_MOD_LCD0         = 4,
    DISP_MOD_LCD1         = 5,
    DISP_MOD_TVE0         = 6,
    DISP_MOD_TVE1         = 7,
    DISP_MOD_CCMU         = 8,
    DISP_MOD_PIOC         = 9,
    DISP_MOD_PWM          = 10,
    DISP_MOD_DEU0         = 11,
    DISP_MOD_DEU1         = 12,
    DISP_MOD_CMU0         = 13,
    DISP_MOD_CMU1         = 14,
    DISP_MOD_DRC0         = 15,
    DISP_MOD_DRC1         = 16,
    DISP_MOD_DSI0         = 17,
    DISP_MOD_DSI0_DPHY    = 18,
    DISP_MOD_DSI1         = 19,
    DISP_MOD_DSI1_DPHY    = 20,
    DISP_MOD_HDMI         = 21,
    DISP_MOD_EDP          = 22,
    DISP_MOD_NUM          = 23,
}__disp_mod_id_t;



typedef enum
{
    LCD_IF_HV			= 0,
    LCD_IF_CPU			= 1,
    LCD_IF_LVDS			= 3,
    LCD_IF_DSI			= 4,
    LCD_IF_EDP          = 5,
    LCD_IF_EXT_DSI      = 6,
}__lcd_if_t;

typedef enum
{
    LCD_HV_IF_PRGB_1CYC		= 0,  //parallel hv
    LCD_HV_IF_SRGB_3CYC		= 8,  //serial hv
    LCD_HV_IF_DRGB_4CYC		= 10, //Dummy RGB
    LCD_HV_IF_RGBD_4CYC		= 11, //RGB Dummy
    LCD_HV_IF_CCIR656_2CYC	= 12, 
}__lcd_hv_if_t;

typedef enum
{
    LCD_HV_SRGB_SEQ_RGB_RGB	= 0,
    LCD_HV_SRGB_SEQ_RGB_BRG	= 1,
    LCD_HV_SRGB_SEQ_RGB_GBR	= 2,
    LCD_HV_SRGB_SEQ_BRG_RGB	= 4,
    LCD_HV_SRGB_SEQ_BRG_BRG	= 5,
    LCD_HV_SRGB_SEQ_BRG_GBR	= 6,
    LCD_HV_SRGB_SEQ_GRB_RGB	= 8,
    LCD_HV_SRGB_SEQ_GRB_BRG	= 9,
    LCD_HV_SRGB_SEQ_GRB_GBR	= 10,
}__lcd_hv_srgb_seq_t;

typedef enum
{
    LCD_HV_SYUV_SEQ_YUYV	= 0,
    LCD_HV_SYUV_SEQ_YVYU	= 1,
    LCD_HV_SYUV_SEQ_UYUV	= 2,
    LCD_HV_SYUV_SEQ_VYUY	= 3,
}__lcd_hv_syuv_seq_t;

typedef enum
{
    LCD_HV_SYUV_FDLY_0LINE	= 0,
    LCD_HV_SRGB_FDLY_2LINE	= 1, //ccir ntsc
    LCD_HV_SRGB_FDLY_3LINE	= 2, //ccir pal
}__lcd_hv_syuv_fdly_t;

typedef enum
{
    LCD_CPU_IF_RGB666_18PIN = 0,
    LCD_CPU_IF_RGB666_9PIN  = 10,
    LCD_CPU_IF_RGB666_6PIN  = 12,
    LCD_CPU_IF_RGB565_16PIN = 8,
    LCD_CPU_IF_RGB565_8PIN  = 14,
}__lcd_cpu_if_t;

typedef enum
{
    LCD_TE_DISABLE		= 0,
    LCD_TE_RISING		= 1,
    LCD_TE_FALLING      = 2,
}__lcd_te_t;

typedef enum
{
    LCD_LVDS_IF_SINGLE_LINK		= 0,
    LCD_LVDS_IF_DUAL_LINK		= 1,
}__lcd_lvds_if_t;

typedef enum
{
    LCD_LVDS_8bit		= 0,
    LCD_LVDS_6bit		= 1,
}__lcd_lvds_colordepth_t;

typedef enum
{
    LCD_LVDS_MODE_NS		= 0,
    LCD_LVDS_MODE_JEIDA		= 1,
}__lcd_lvds_mode_t;

typedef enum
{
    LCD_DSI_IF_VIDEO_MODE	= 0,
    LCD_DSI_IF_COMMAND_MODE	= 1,
    LCD_DSI_IF_BURST_MODE = 2,
}__lcd_dsi_if_t;

typedef enum
{
    LCD_DSI_1LANE			= 1,
    LCD_DSI_2LANE			= 2,
    LCD_DSI_3LANE			= 3,
    LCD_DSI_4LANE			= 4,
}__lcd_dsi_lane_t;

typedef enum
{
    LCD_DSI_FORMAT_RGB888	= 0,  
    LCD_DSI_FORMAT_RGB666	= 1,  
    LCD_DSI_FORMAT_RGB666P	= 2,  
    LCD_DSI_FORMAT_RGB565	= 3,  
}__lcd_dsi_format_t;


typedef enum
{
    LCD_FRM_BYPASS			= 0,
    LCD_FRM_RGB666			= 1,
    LCD_FRM_RGB565			= 2,
}__lcd_frm_t;


typedef struct
{
    __u32                   addr[3];    // frame buffer的内容地址，对于rgb类型，只有addr[0]有效
    __disp_rectsz_t         size;//单位是pixel
    __disp_pixel_fmt_t      format;
    __disp_pixel_seq_t      seq;
    __disp_pixel_mod_t      mode;
    __bool                  br_swap;    // blue red color swap flag, FALSE:RGB; TRUE:BGR,only used in rgb format
    __disp_cs_mode_t        cs_mode;    //color space 
    __bool                  b_trd_src; //if 3d source, used for scaler mode layer
    __disp_3d_src_mode_t    trd_mode; //source 3d mode, used for scaler mode layer
    __u32                   trd_right_addr[3];//used when in frame packing 3d mode
    __bool                  pre_multiply; //TRUE: pre-multiply fb
}__disp_fb_t;

typedef struct
{
    __disp_layer_work_mode_t    mode;       //layer work mode
    __bool                      b_from_screen;
    __u8                        pipe;       //layer pipe,0/1,if in scaler mode, scaler0 must be pipe0, scaler1 must be pipe1
    __u8                        prio;       //layer priority,can get layer prio,but never set layer prio,从底至顶,优先级由低至高
    __bool                      alpha_en;   //layer global alpha enable
    __u16                       alpha_val;  //layer global alpha value 
    __bool                      ck_enable;  //layer color key enable
    __disp_rect_t               src_win;    // framebuffer source window,only care x,y if is not scaler mode
    __disp_rect_t               scn_win;    // screen window
    __disp_fb_t                 fb;         //framebuffer
    __bool                      b_trd_out;  //if output 3d mode, used for scaler mode layer
    __disp_3d_out_mode_t        out_trd_mode; //output 3d mode, used for scaler mode layer
}__disp_layer_info_t;

typedef struct
{
    __disp_color_t   ck_max;
    __disp_color_t   ck_min;
    __u32             red_match_rule;//0/1:always match; 2:match if min<=color<=max; 3:match if color>max or color<min 
    __u32             green_match_rule;//0/1:always match; 2:match if min<=color<=max; 3:match if color>max or color<min 
    __u32             blue_match_rule;//0/1:always match; 2:match if min<=color<=max; 3:match if color>max or color<min 
}__disp_colorkey_t;

typedef struct
{
    __s32   id;
    __u32   addr[3];
    __u32   addr_right[3];//used when in frame packing 3d mode
    __bool  interlace;
    __bool  top_field_first;
    __u32   frame_rate; // *FRAME_RATE_BASE(现在定为1000)
    __u32   flag_addr;//dit maf flag address
    __u32   flag_stride;//dit maf flag line stride
    __bool  maf_valid;
    __bool  pre_frame_valid;
}__disp_video_fb_t;

typedef struct
{
    __bool maf_enable;
    __bool pre_frame_enable;
}__disp_dit_info_t;

typedef struct
{
    __disp_hwc_mode_t     pat_mode;
    __u32                 addr;
}__disp_hwc_pattern_t;

typedef struct
{
    __disp_fb_t     input_fb;
    __disp_rect_t   source_regn;
    __disp_fb_t     output_fb;
    __disp_rect_t   out_regn;
}__disp_scaler_para_t;

typedef struct
{
    __disp_fb_t       fb;
    __disp_rect_t   src_win;//source region,only care x,y because of not scaler
    __disp_rect_t   scn_win;// sceen region 
}__disp_sprite_block_para_t;

typedef struct
{
    __disp_rectsz_t screen_size;//used when the screen is not displaying on any output device(lcd/hdmi/vga/tv)
    __disp_fb_t     output_fb[3];
		__u32           buffer_num;//1,2,3
		__u32           mode;      //0:single,  1:continue
		__u32           fps;       //0:fps of lcd,  1:1/2 fps of lcd
		__disp_rect_t   capture_window;
		__disp_rect_t   output_window;

		__u32           cur_buffer_id;  //no need to care about it
		__u32           capture_request;
		__s32           scaler_id;
		__u32           got_frame;
}__disp_capture_screen_para_t;

typedef struct
{
    __s32 (*hdmi_open)(void);
    __s32 (*hdmi_close)(void);
    __s32 (*hdmi_set_mode)(__disp_tv_mode_t mode);
    __s32 (*hdmi_mode_support)(__disp_tv_mode_t mode);
    __s32 (*hdmi_get_HPD_status)(void);
    __s32 (*hdmi_set_pll)(__u32 pll, __u32 clk);
    __s32 (*hdmi_dvi_enable)(__u32 mode);
    __s32 (*hdmi_dvi_support)(void);
    __s32 (*hdmi_get_input_csc)(void);
    __s32 (*hdmi_suspend)(void);
    __s32 (*hdmi_resume)(void);
    __s32 (*hdmi_early_suspend)(void);
    __s32 (*hdmi_late_resume)(void);
}__disp_hdmi_func;

typedef struct
{
	__u32 lp_clk_div;
	__u32 hs_prepare;
	__u32 hs_trail;
	__u32 clk_prepare;
	__u32 clk_zero;
	__u32 clk_pre;
	__u32 clk_post;
	__u32 clk_trail;
	__u32 hs_dly_mode;
	__u32 hs_dly;
	__u32 lptx_ulps_exit;
	__u32 hstx_ana0;
	__u32 hstx_ana1;
}__disp_dsi_dphy_timing_t;


typedef struct
{
	__u32   lcd_gamma_tbl[256];
	__u32	lcd_cmap_tbl[2][3][4];
    __u32   lcd_bright_curve_tbl[256];
}__panel_extend_para_t;



typedef struct
{
	__lcd_if_t   			lcd_if;

	__lcd_hv_if_t		 	lcd_hv_if;
	__lcd_hv_srgb_seq_t  	lcd_hv_srgb_seq;
	__lcd_hv_syuv_seq_t  	lcd_hv_syuv_seq;
	__lcd_hv_syuv_fdly_t	lcd_hv_syuv_fdly;

	__lcd_lvds_if_t   		lcd_lvds_if;
	__lcd_lvds_colordepth_t	lcd_lvds_colordepth; //color depth, 0:8bit; 1:6bit
	__lcd_lvds_mode_t   	lcd_lvds_mode;
	__u32   				lcd_lvds_io_polarity;

	__lcd_cpu_if_t			lcd_cpu_if;
	__lcd_te_t			    lcd_cpu_te;

	__lcd_dsi_if_t			lcd_dsi_if;
	__lcd_dsi_lane_t		lcd_dsi_lane;
	__lcd_dsi_format_t		lcd_dsi_format;
	__u32					lcd_dsi_eotp;
    __u32					lcd_dsi_vc;
    __lcd_te_t              lcd_dsi_te;
    
	__u32						lcd_dsi_dphy_timing_en; //todo? maybe not used
	__disp_dsi_dphy_timing_t*	lcd_dsi_dphy_timing_p;

    __u32                  lcd_edp_tx_ic;   //0:anx9804;  1:anx6345
    __u32                  lcd_edp_tx_rate; //1(1.62G); 2(2.7G); 3(5.4G)
    __u32                  lcd_edp_tx_lane; //  1/2/4lane
    __u32                  lcd_edp_colordepth; //color depth, 0:8bit; 1:6bit

	__u32   lcd_dclk_freq;
	__u32   lcd_x; //horizontal resolution
	__u32   lcd_y; //vertical resolution
    __u32   lcd_width; //width of lcd in mm
    __u32   lcd_height;//height of lcd in mm
    __u32   lcd_xtal_freq;

	__u32   lcd_pwm_used;
	__u32   lcd_pwm_ch;
	__u32  	lcd_pwm_freq;
	__u32  	lcd_pwm_pol;

	__u32  	lcd_rb_swap;
	__u32  	lcd_rgb_endian;

	__u32   lcd_vt;
	__u32   lcd_ht;
	__u32   lcd_vbp;
	__u32   lcd_hbp;
	__u32   lcd_vspw;
	__u32   lcd_hspw;

	__u32   lcd_hv_clk_phase;
    __u32   lcd_hv_sync_polarity;

	__u32   lcd_frm;
	__u32   lcd_gamma_en;
	__u32 	lcd_cmap_en;
    __u32   lcd_bright_curve_en;
    __panel_extend_para_t lcd_extend_para;

    char    lcd_size[8]; //e.g. 7.9, 9.7
    char    lcd_model_name[32];

	__u32   tcon_index; //not need to config for user
	__u32	lcd_fresh_mode;//not need to config for user
	__u32   lcd_dclk_freq_original; //not need to config for user
}__panel_para_t;


typedef struct
{
	__u32	pixel_clk;//khz
	__u32	hor_pixels;
	__u32	ver_pixels;
	__u32	hor_total_time;
	__u32	hor_front_porch;
	__u32	hor_sync_time;	
	__u32	hor_back_porch;
	__u32	ver_total_time;
	__u32	ver_front_porch;
	__u32	ver_sync_time;
	__u32	ver_back_porch;
}__disp_tcon_timing_t;

typedef struct
{
	__u32 base_lcdc0;
	__u32 base_lcdc1;
	__u32 base_pioc;
	__u32 base_ccmu;
	__u32 base_pwm;
}__reg_bases_t;

typedef void (*LCD_FUNC) (__u32 sel);
typedef struct lcd_function
{
    LCD_FUNC func;
    __u32 delay;//ms
}__lcd_function_t;

typedef struct lcd_flow
{
    __lcd_function_t func[5];
    __u32 func_num;
    __u32 cur_step;
}__lcd_flow_t;

typedef struct
{
    void (*cfg_panel_info)(__panel_extend_para_t * info);
    __s32 (*cfg_open_flow)(__u32 sel);
    __s32 (*cfg_close_flow)(__u32 sel);
    __s32 (*lcd_user_defined_func)(__u32 sel, __u32 para1, __u32 para2, __u32 para3);
}__lcd_panel_fun_t;

typedef struct
{
    __bool enable;
    __u32 active_state;
    __u32 duty_ns;
    __u32 period_ns;
}__pwm_info_t;

struct sunxi_disp_source_ops
{
  __s32 (*sunxi_lcd_delay_ms)(__u32 ms);
  __s32 (*sunxi_lcd_delay_us)(__u32 us);
  __s32 (*sunxi_lcd_tcon_enable)(__u32 scree_id);
  __s32 (*sunxi_lcd_tcon_disable)(__u32 scree_id);
  __s32 (*sunxi_lcd_pwm_set_para)(__u32 pwm_channel, __pwm_info_t * pwm_info);
  __s32 (*sunxi_lcd_pwm_get_para)(__u32 pwm_channel, __pwm_info_t * pwm_info);
  __s32 (*sunxi_lcd_pwm_set_duty_ns)(__u32 pwm_channel, __u32 duty_ns);
  __s32 (*sunxi_lcd_pwm_enable)(__u32 pwm_channel);
  __s32 (*sunxi_lcd_pwm_disable)(__u32 pwm_channel);
  __s32 (*sunxi_lcd_cpu_write)(__u32 scree_id, __u32 command, __u32 *para, __u32 para_num);
  __s32 (*sunxi_lcd_cpu_write_index)(__u32 scree_id, __u32 index);
  __s32 (*sunxi_lcd_cpu_write_data)(__u32 scree_id, __u32 data);
  __s32 (*sunxi_lcd_dsi_write)(__u32 scree_id, __u8 command, __u8 *para, __u32 para_num);
  __s32 (*sunxi_lcd_pin_cfg)(__u32 screen_id, __u32 bon);
  __s32 (*sunxi_lcd_dsi_clk_enable)(__u32 screen_id, __u32 en);
  __s32 (*sunxi_disp_get_num_screens)(void);
  __s32 (*sunxi_lcd_backlight_enable)(__u32 screen_id);
  __s32 (*sunxi_lcd_backlight_disable)(__u32 screen_id);
  __s32 (*sunxi_lcd_power_enable)(__u32 screen_id, __u32 pwr_id);
  __s32 (*sunxi_lcd_power_disable)(__u32 screen_id, __u32 pwr_id);
  __s32 (*sunxi_lcd_get_driver_name)(__u32 screen_id, char *name);
  __s32 (*sunxi_lcd_set_panel_funs)(char *drv_name, __lcd_panel_fun_t * lcd_cfg);
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    FB_MODE_SCREEN0 = 0,
    FB_MODE_SCREEN1 = 1,
    FB_MODE_DUAL_SAME_SCREEN_TB = 2,//two screen, top buffer for screen0, bottom buffer for screen1
    FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS = 3,//two screen, they have same contents;
}__fb_mode_t;

typedef struct
{
	__fb_mode_t                 fb_mode;
	__disp_layer_work_mode_t    mode;
	__u32                       buffer_num;
	__u32                       width;
	__u32                       height;
	
	__u32                       output_width;//used when scaler mode 
	__u32                       output_height;//used when scaler mode
	
	__u32                       primary_screen_id;//used when FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS
	__u32                       aux_output_width;//used when FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS
	__u32                       aux_output_height;//used when FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS

//maybe not used anymore
	__u32                       line_length;//in byte unit
	__u32                       smem_len;
	__u32                       ch1_offset;//use when PLANAR or UV_COMBINED mode
	__u32                       ch2_offset;//use when PLANAR mode
}__disp_fb_create_para_t;

typedef enum
{
    DISP_INIT_MODE_SCREEN0 = 0,//fb0 for screen0
    DISP_INIT_MODE_SCREEN1 = 1,//fb0 for screen1
    DISP_INIT_MODE_TWO_DIFF_SCREEN = 2,//fb0 for screen0 and fb1 for screen1
    DISP_INIT_MODE_TWO_SAME_SCREEN = 3,//fb0(up buffer for screen0, down buffer for screen1)
    DISP_INIT_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS = 4,//fb0 for two different screen(screen0 layer is normal layer, screen1 layer is scaler layer);
}__disp_init_mode_t;


typedef struct
{
    __bool                  b_init;
    __disp_init_mode_t      disp_mode;//0:single screen0(fb0); 1:single screen1(fb0);  2:dual diff screen(fb0, fb1); 3:dual same screen(fb0 up and down); 4:dual diff screen same contents(fb0)

    //for screen0 and screen1
    __disp_output_type_t    output_type[2];
    __u32                   output_mode[2];

    //for fb0 and fb1
    __u32                   buffer_num[2];
    __bool                  scaler_mode[2];
    __disp_pixel_fmt_t      format[2];
    __disp_pixel_seq_t      seq[2];
    __bool                  br_swap[2];
    __u32                   fb_width[2];
    __u32                   fb_height[2];
}__disp_init_t;

typedef struct
{   
    int                 post2_layers;
    __disp_layer_info_t layer_info[8];
    __disp_rect_t       fb_scn_win;

    int                 primary_display_layer_num;
}setup_dispc_data_t;

typedef enum tag_DISP_CMD
{
//----disp global----
    DISP_CMD_RESERVE0 = 0x00,
    DISP_CMD_RESERVE1 = 0x01,
    DISP_CMD_SET_BKCOLOR = 0x3f,
    DISP_CMD_GET_BKCOLOR = 0x03,
    DISP_CMD_SET_COLORKEY = 0x04,
    DISP_CMD_GET_COLORKEY = 0x05,
    DISP_CMD_SET_PALETTE_TBL = 0x06,
    DISP_CMD_GET_PALETTE_TBL = 0x07,
    DISP_CMD_SCN_GET_WIDTH = 0x08,
    DISP_CMD_SCN_GET_HEIGHT = 0x09,
    DISP_CMD_GET_OUTPUT_TYPE = 0x0a,
    DISP_CMD_SET_EXIT_MODE = 0x0c,
    DISP_CMD_SET_GAMMA_TABLE = 0x0d,
    DISP_CMD_GAMMA_CORRECTION_ON = 0x0e,
    DISP_CMD_GAMMA_CORRECTION_OFF = 0x0f,
    DISP_CMD_START_CMD_CACHE =0x10,
    DISP_CMD_EXECUTE_CMD_AND_STOP_CACHE = 0x11,
    DISP_CMD_SET_BRIGHT = 0x12,
    DISP_CMD_SET_CONTRAST = 0x13,
    DISP_CMD_SET_SATURATION = 0x14,
    DISP_CMD_GET_BRIGHT = 0x16,
    DISP_CMD_GET_CONTRAST = 0x17,
    DISP_CMD_GET_SATURATION = 0x18,
    DISP_CMD_ENHANCE_ON = 0x1a,
    DISP_CMD_ENHANCE_OFF = 0x1b,
    DISP_CMD_GET_ENHANCE_EN = 0x1c,
    DISP_CMD_CLK_ON = 0x1d,
    DISP_CMD_CLK_OFF = 0x1e,
    DISP_CMD_SET_SCREEN_SIZE = 0x1f,//when the screen is not used to display(lcd/tv/vga/hdmi) directly, maybe capture the screen and scaler to dram, or as a layer of another screen
    DISP_CMD_CAPTURE_SCREEN = 0x20,//caputre screen and scaler to dram
    DISP_CMD_DE_FLICKER_ON = 0x21,
    DISP_CMD_DE_FLICKER_OFF = 0x22,
    DISP_CMD_GET_DE_FLICKER_EN = 0x23,
    DISP_CMD_DRC_ON = 0x24,
    DISP_CMD_DRC_OFF = 0x25,
    DISP_CMD_GET_DRC_EN = 0x26,
    DISP_CMD_DE_FLICKER_SET_WINDOW = 0x27,
    DISP_CMD_DRC_SET_WINDOW = 0x28,
    DISP_CMD_SET_HUE = 0x29,
    DISP_CMD_GET_HUE = 0x2a,
    DISP_CMD_VSYNC_EVENT_EN = 0x2b,
    DISP_CMD_DRC_GET_WINDOW = 0x2c,
    DISP_CMD_SET_ENHANCE_MODE = 0x2d,      
    DISP_CMD_GET_ENHANCE_MODE = 0x2e,
    DISP_CMD_SET_ENHANCE_WINDOW = 0X2f,
    DISP_CMD_GET_ENHANCE_WINDOW = 0X30,
    DISP_CMD_SET_OVL_MODE = 0x31,
    DISP_CMD_BLANK = 0x32,
    DISP_CMD_CAPTURE_SCREEN_STOP = 0x33,//for continue mode

//----layer----
    DISP_CMD_LAYER_REQUEST = 0x40,
    DISP_CMD_LAYER_RELEASE = 0x41,
    DISP_CMD_LAYER_OPEN = 0x42,
    DISP_CMD_LAYER_CLOSE = 0x43,
    DISP_CMD_LAYER_SET_FB = 0x44,
    DISP_CMD_LAYER_GET_FB = 0x45,
    DISP_CMD_LAYER_SET_SRC_WINDOW = 0x46,
    DISP_CMD_LAYER_GET_SRC_WINDOW = 0x47,
    DISP_CMD_LAYER_SET_SCN_WINDOW = 0x48,
    DISP_CMD_LAYER_GET_SCN_WINDOW = 0x49,
    DISP_CMD_LAYER_SET_PARA = 0x4a,
    DISP_CMD_LAYER_GET_PARA = 0x4b,
    DISP_CMD_LAYER_ALPHA_ON = 0x4c,
    DISP_CMD_LAYER_ALPHA_OFF = 0x4d,
    DISP_CMD_LAYER_GET_ALPHA_EN = 0x4e,
    DISP_CMD_LAYER_SET_ALPHA_VALUE = 0x4f,
    DISP_CMD_LAYER_GET_ALPHA_VALUE = 0x50,
    DISP_CMD_LAYER_CK_ON = 0x51,
    DISP_CMD_LAYER_CK_OFF = 0x52,
    DISP_CMD_LAYER_GET_CK_EN = 0x53,
    DISP_CMD_LAYER_SET_PIPE = 0x54,
    DISP_CMD_LAYER_GET_PIPE = 0x55,
    DISP_CMD_LAYER_TOP = 0x56,
    DISP_CMD_LAYER_BOTTOM = 0x57,
    DISP_CMD_LAYER_GET_PRIO = 0x58,
    DISP_CMD_LAYER_SET_SMOOTH = 0x59,
    DISP_CMD_LAYER_GET_SMOOTH = 0x5a,
    DISP_CMD_LAYER_SET_BRIGHT = 0x5b,
    DISP_CMD_LAYER_SET_CONTRAST = 0x5c,
    DISP_CMD_LAYER_SET_SATURATION = 0x5d,
    DISP_CMD_LAYER_SET_HUE = 0x5e,
    DISP_CMD_LAYER_GET_BRIGHT = 0x5f,
    DISP_CMD_LAYER_GET_CONTRAST = 0x60,
    DISP_CMD_LAYER_GET_SATURATION = 0x61,
    DISP_CMD_LAYER_GET_HUE = 0x62,
    DISP_CMD_LAYER_ENHANCE_ON = 0x63,
    DISP_CMD_LAYER_ENHANCE_OFF = 0x64,
    DISP_CMD_LAYER_GET_ENHANCE_EN = 0x65,
    DISP_CMD_LAYER_VPP_ON = 0x67,
    DISP_CMD_LAYER_VPP_OFF = 0x68,
    DISP_CMD_LAYER_GET_VPP_EN = 0x69,
    DISP_CMD_LAYER_SET_LUMA_SHARP_LEVEL = 0x6a,
    DISP_CMD_LAYER_GET_LUMA_SHARP_LEVEL = 0x6b,
    DISP_CMD_LAYER_SET_CHROMA_SHARP_LEVEL = 0x6c,
    DISP_CMD_LAYER_GET_CHROMA_SHARP_LEVEL = 0x6d,
    DISP_CMD_LAYER_SET_WHITE_EXTEN_LEVEL = 0x6e,
    DISP_CMD_LAYER_GET_WHITE_EXTEN_LEVEL = 0x6f,
    DISP_CMD_LAYER_SET_BLACK_EXTEN_LEVEL = 0x70,
    DISP_CMD_LAYER_GET_BLACK_EXTEN_LEVEL = 0x71,
    DISP_CMD_LAYER_VPP_SET_WINDOW = 0X72,
    DISP_CMD_LAYER_VPP_GET_WINDOW = 0X73,
    DISP_CMD_LAYER_SET_ENHANCE_MODE = 0x74,      
    DISP_CMD_LAYER_GET_ENHANCE_MODE = 0x75,      
    DISP_CMD_LAYER_SET_ENHANCE_WINDOW = 0X76,
    DISP_CMD_LAYER_GET_ENHANCE_WINDOW = 0X77,

//----scaler----
    DISP_CMD_SCALER_REQUEST = 0x80,
    DISP_CMD_SCALER_RELEASE = 0x81,
    DISP_CMD_SCALER_EXECUTE = 0x82,
    DISP_CMD_SCALER_EXECUTE_EX = 0x83,

//----hwc----
    DISP_CMD_HWC_OPEN = 0xc0,
    DISP_CMD_HWC_CLOSE = 0xc1,
    DISP_CMD_HWC_SET_POS = 0xc2,
    DISP_CMD_HWC_GET_POS = 0xc3,
    DISP_CMD_HWC_SET_FB = 0xc4,
    DISP_CMD_HWC_SET_PALETTE_TABLE = 0xc5,

//----video----
    DISP_CMD_VIDEO_START = 0x100,
    DISP_CMD_VIDEO_STOP = 0x101,
    DISP_CMD_VIDEO_SET_FB = 0x102,
    DISP_CMD_VIDEO_GET_FRAME_ID = 0x103,
    DISP_CMD_VIDEO_GET_DIT_INFO = 0x104,

//----lcd----
    DISP_CMD_LCD_ON = 0x140,
    DISP_CMD_LCD_OFF = 0x141,
    DISP_CMD_LCD_SET_BRIGHTNESS = 0x142,
    DISP_CMD_LCD_GET_BRIGHTNESS = 0x143,
    DISP_CMD_LCD_CPUIF_XY_SWITCH = 0x146,
    DISP_CMD_LCD_CHECK_OPEN_FINISH = 0x14a,
    DISP_CMD_LCD_CHECK_CLOSE_FINISH = 0x14b,
    DISP_CMD_LCD_SET_SRC = 0x14c,
    DISP_CMD_LCD_USER_DEFINED_FUNC = 0x14d,
    DISP_CMD_LCD_BACKLIGHT_ON  = 0x14e,
    DISP_CMD_LCD_BACKLIGHT_OFF  = 0x14f,
    DISP_CMD_LCD_SET_FPS  = 0x150,
    DISP_CMD_LCD_GET_FPS  = 0x151,
    DISP_CMD_LCD_GET_SIZE = 0x152,
    DISP_CMD_LCD_GET_MODEL_NAME = 0x153,

//----tv----
    DISP_CMD_TV_ON = 0x180,
    DISP_CMD_TV_OFF = 0x181,
    DISP_CMD_TV_SET_MODE = 0x182,
    DISP_CMD_TV_GET_MODE = 0x183,
    DISP_CMD_TV_AUTOCHECK_ON = 0x184,
    DISP_CMD_TV_AUTOCHECK_OFF = 0x185,
    DISP_CMD_TV_GET_INTERFACE = 0x186,
    DISP_CMD_TV_SET_SRC = 0x187,
    DISP_CMD_TV_GET_DAC_STATUS = 0x188,
    DISP_CMD_TV_SET_DAC_SOURCE = 0x189,
    DISP_CMD_TV_GET_DAC_SOURCE = 0x18a,

//----hdmi----
    DISP_CMD_HDMI_ON = 0x1c0,
    DISP_CMD_HDMI_OFF = 0x1c1,
    DISP_CMD_HDMI_SET_MODE = 0x1c2,
    DISP_CMD_HDMI_GET_MODE = 0x1c3,
    DISP_CMD_HDMI_SUPPORT_MODE = 0x1c4,
    DISP_CMD_HDMI_GET_HPD_STATUS = 0x1c5,
	DISP_CMD_HDMI_SET_SRC = 0x1c6,

//----vga----
    DISP_CMD_VGA_ON = 0x200,
    DISP_CMD_VGA_OFF = 0x201,
    DISP_CMD_VGA_SET_MODE = 0x202,
    DISP_CMD_VGA_GET_MODE = 0x203,
	DISP_CMD_VGA_SET_SRC = 0x204,

//----sprite----
    DISP_CMD_SPRITE_OPEN = 0x240,
    DISP_CMD_SPRITE_CLOSE = 0x241,
    DISP_CMD_SPRITE_SET_FORMAT = 0x242,
    DISP_CMD_SPRITE_GLOBAL_ALPHA_ENABLE = 0x243,
    DISP_CMD_SPRITE_GLOBAL_ALPHA_DISABLE = 0x244,
    DISP_CMD_SPRITE_GET_GLOBAL_ALPHA_ENABLE = 0x252,
    DISP_CMD_SPRITE_SET_GLOBAL_ALPHA_VALUE = 0x245,
    DISP_CMD_SPRITE_GET_GLOBAL_ALPHA_VALUE = 0x253,
    DISP_CMD_SPRITE_SET_ORDER = 0x246,
    DISP_CMD_SPRITE_GET_TOP_BLOCK = 0x250,
    DISP_CMD_SPRITE_GET_BOTTOM_BLOCK = 0x251,
    DISP_CMD_SPRITE_SET_PALETTE_TBL = 0x247,
    DISP_CMD_SPRITE_GET_BLOCK_NUM = 0x259,
    DISP_CMD_SPRITE_BLOCK_REQUEST = 0x248,
    DISP_CMD_SPRITE_BLOCK_RELEASE = 0x249,
    DISP_CMD_SPRITE_BLOCK_OPEN = 0x257,
    DISP_CMD_SPRITE_BLOCK_CLOSE = 0x258,
    DISP_CMD_SPRITE_BLOCK_SET_SOURCE_WINDOW = 0x25a,
    DISP_CMD_SPRITE_BLOCK_GET_SOURCE_WINDOW = 0x25b,
    DISP_CMD_SPRITE_BLOCK_SET_SCREEN_WINDOW = 0x24a,
    DISP_CMD_SPRITE_BLOCK_GET_SCREEN_WINDOW = 0x24c,
    DISP_CMD_SPRITE_BLOCK_SET_FB = 0x24b,
    DISP_CMD_SPRITE_BLOCK_GET_FB = 0x24d,
    DISP_CMD_SPRITE_BLOCK_SET_PARA = 0x25c,
    DISP_CMD_SPRITE_BLOCK_GET_PARA = 0x25d,
    DISP_CMD_SPRITE_BLOCK_SET_TOP = 0x24e,
    DISP_CMD_SPRITE_BLOCK_SET_BOTTOM = 0x24f,
    DISP_CMD_SPRITE_BLOCK_GET_PREV_BLOCK = 0x254,
    DISP_CMD_SPRITE_BLOCK_GET_NEXT_BLOCK = 0x255,
    DISP_CMD_SPRITE_BLOCK_GET_PRIO = 0x256,

//----framebuffer----
	DISP_CMD_FB_REQUEST = 0x280,
	DISP_CMD_FB_RELEASE = 0x281,
	DISP_CMD_FB_GET_PARA = 0x282,
	DISP_CMD_GET_DISP_INIT_PARA = 0x283,
	
//--- memory --------	
	DISP_CMD_MEM_REQUEST = 0x2c0,
	DISP_CMD_MEM_RELEASE = 0x2c1,
	DISP_CMD_MEM_GETADR = 0x2c2,
	DISP_CMD_MEM_SELIDX = 0x2c3,
	
//---- for test	
	DISP_CMD_SUSPEND = 0x2d0,
	DISP_CMD_RESUME = 0x2d1,
	DISP_CMD_PRINT_REG = 0x2e0, 

//---pwm --------	
    DISP_CMD_PWM_SET_PARA = 0x300,
    DISP_CMD_PWM_GET_PARA = 0x301,
}__disp_cmd_t;

#define FBIOGET_LAYER_HDL_0 0x4700
#define FBIOGET_LAYER_HDL_1 0x4701

#define FBIO_CLOSE 0x4710
#define FBIO_OPEN 0x4711
#define FBIO_ALPHA_ON 0x4712
#define FBIO_ALPHA_OFF 0x4713
#define FBIOPUT_ALPHA_VALUE 0x4714

#define FBIO_DISPLAY_SCREEN0_ONLY 0x4720
#define FBIO_DISPLAY_SCREEN1_ONLY 0x4721
#define FBIO_DISPLAY_TWO_SAME_SCREEN_TB 0x4722
#define FBIO_DISPLAY_TWO_DIFF_SCREEN_SAME_CONTENTS 0x4723

#endif
