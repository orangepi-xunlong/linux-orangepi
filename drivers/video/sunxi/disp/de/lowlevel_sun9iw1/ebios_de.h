#ifndef __EBIOS_DE_H__
#define __EBIOS_DE_H__

#include "../bsp_display.h"

#define DE_WB_END_IE    			(1<<7)      /*write back end interrupt */
#define DE_FE_IE_REG_LOAD_FINISH    10
#define DE_FE_INTEN_ALL             0x1ff     /*front-end all interrupt enable*/
#define DE_IMG_REG_LOAD_FINISH  (1<<1)

#define SCAL_WB_ERR_SYNC (1<<15) //sync reach flag when capture in process
#define SCAL_WB_ERR_LOSEDATA (1<<14) //lose data flag when capture in process
#define SCAL_WB_ERR_STATUS (1<<12) //unvalid write back

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
            (((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
            (((reg) & CLRMASK(width, shift)) | (val << (shift)))

typedef enum   		/*layer framebuffer format enum definition*/
{
	DE_MONO_1BPP=0,
	DE_MONO_2BPP,
	DE_MONO_4BPP,
	DE_MONO_8BPP,
	DE_COLOR_RGB655,
	DE_COLOR_RGB565,
	DE_COLOR_RGB556,
	DE_COLOR_ARGB1555,
	DE_COLOR_RGBA5551,
	DE_COLOR_RGB0888,
	DE_COLOR_ARGB8888,
	DE_COLOR_RGB888,
	DE_COLOR_ARGB4444,

}de_fbfmt_e;

typedef enum     		/*internal layer framebuffer format enum definition*/
{
	DE_IF1BPP=0,
	DE_IF2BPP,
	DE_IF4BPP,
	DE_IF8BPP
}de_inter_fbfmt_e;

typedef enum
{
  DE_H32_V32_8BPP,
  DE_H64_V64_2BPP,
  DE_H64_V32_4BPP,
  DE_H32_V64_4BPP
}de_hwc_mode_e;


typedef enum
{
   DE_N32PIXELS=0,
   DE_N64PIXELS
}de_pixels_num_t;


typedef enum __SCAL_PS
{
	DE_SCAL_BGRA=0,  //rgb
	DE_SCAL_ARGB=1,
	DE_SCAL_AYUV=0,
	DE_SCAL_VUYA=1,
	DE_SCAL_UVUV=0, //for uv combined
	DE_SCAL_VUVU=1,
	DE_SCAL_UYVY=0,
	DE_SCAL_YUYV=1,
	DE_SCAL_VYUY=2,
	DE_SCAL_YVYU=3,
	DE_SCAL_RGB565=0,
	DE_SCAL_BGR565=1,
	DE_SCAL_ARGB4444=0,
	DE_SCAL_BGRA4444=1,
	DE_SCAL_ARGB1555=0,
	DE_SCAL_BGRA5551=1
}__scal_ps_t;

typedef enum __SCAL_INMODE
{
	DE_SCAL_PLANNAR=0,
	DE_SCAL_INTERLEAVED,
	DE_SCAL_UVCOMBINED,
	DE_SCAL_PLANNARMB=4,
	DE_SCAL_UVCOMBINEDMB=6,
	DE_SCAL_UVCOMBINEDMB_128X32=7,
}__scal_inmode_t;


typedef enum __SCAL_INFMT
{
	DE_SCAL_INYUV444=0,
	DE_SCAL_INYUV422,
	DE_SCAL_INYUV420,
	DE_SCAL_INYUV411,
	DE_SCAL_INRGB565,  //new
	DE_SCAL_INRGB888,
	DE_SCAL_INRGB4444, //new
	DE_SCAL_INRGB1555  //new
}__scal_infmt_t;

typedef enum __SCAL_OUTFMT
{
	DE_SCAL_OUTPRGB888=0,
	DE_SCAL_OUTI0RGB888,
	DE_SCAL_OUTI1RGB888,
	DE_SCAL_OUTPYUV444=4,
	DE_SCAL_OUTPYUV420,
	DE_SCAL_OUTPYUV422,
	DE_SCAL_OUTPYUV411,
	DE_SCAL_OUTSPYUV420 = 0xd,
	DE_SCAL_OUTSPYUV422 = 0xe,
	DE_SCAL_OUTSPYUV411
}__scal_outfmt_t;
//for 3D inmod,  source mod must  be DE_SCAL_PLANNAR or DE_SCAL_UVCOMBINEDMB
//DE_SCAL_INTER_LEAVED and DE_SCAL_UVCOMBINED maybe supported in future====
typedef enum __SCAL_3D_INMODE
{
	DE_SCAL_3DIN_TB=0,
	DE_SCAL_3DIN_FP=1,
	DE_SCAL_3DIN_SSF=2,
	DE_SCAL_3DIN_SSH=3,
	DE_SCAL_3DIN_LI=4,
}__scal_3d_inmode_t;

typedef enum __SCAL_3D_OUTMODE
{
	DE_SCAL_3DOUT_CI_1=0,    //for lcd
	DE_SCAL_3DOUT_CI_2,
	DE_SCAL_3DOUT_CI_3,
	DE_SCAL_3DOUT_CI_4,
	DE_SCAL_3DOUT_LIRGB,
	DE_SCAL_3DOUT_HDMI_FPP,   //for hdmi
	DE_SCAL_3DOUT_HDMI_FPI,
	DE_SCAL_3DOUT_HDMI_TB,
	DE_SCAL_3DOUT_HDMI_FA,
	DE_SCAL_3DOUT_HDMI_SSH,
	DE_SCAL_3DOUT_HDMI_SSF,
	DE_SCAL_3DOUT_HDMI_LI,
}__scal_3d_outmode_t;

typedef enum
{
	IEP_DRC_MODE_UI,
	IEP_DRC_MODE_VIDEO,
}__iep_drc_mode_t;

typedef struct layer_input_src
{
   u8     format;
   u8     pixseq;
   u8     br_swap;
   u32    fb_width;
   u32    fb_addr;
   u32    offset_x;
   u32    offset_y;

   bool yuv_ch;
   bool pre_multiply;
}layer_src_t;

typedef struct dlcdp_src         /*direct lcd pipe input source definition */
{
   u8     format;
   u8     pixseq;
   u32    fb_width;
   u32    fb_addr;
   u32    offset_x;
   u32    offset_y;
}de_dlcdp_src_t;

typedef struct hwc_src
{
    u8    mode;
    u32   paddr;
}de_hwc_src_t;

typedef struct yuv_ch_src
{
   u8     format;
   u8     mode;
   u8     pixseq;
   u32    ch0_base;	//in bits
   u32    ch1_base;	//in bits
   u32    ch2_base;	//in bits
   u32    line_width;	//in bits
   u32    offset_x;
   u32    offset_y;
   u8     cs_mode;    //0:DISP_BT601; 1:DISP_BT709; 2:DISP_YCC; 3:DISP_VXYCC
}de_yuv_ch_src_t;

typedef struct sprite_src
{
	u8    pixel_seq;//0,1
	u8    format;//0:32bpp; 1:8bpp
   u32    offset_x;
   u32    offset_y;
   u32    fb_addr;
   u32    fb_width;
}de_sprite_src_t;


typedef struct __SCAL_SRC_TYPE
{
    u8    sample_method; //for yuv420, 0: uv_hphase=-0.25, uv_vphase=-0.25; other : uv_hphase = 0, uv_vphase = -0.25
    u8    byte_seq;  //0:byte0,byte1, byte2, byte3; 1: byte3, byte2, byte1, byte0
    u8    mod;       //0:plannar; 1: interleaved; 2: plannar uv combined; 4: plannar mb; 6: uv combined mb
    u8    fmt;       //0:yuv444; 1: yuv422; 2: yuv420; 3:yuv411; 4: csi rgb; 5:rgb888
    u8    ps;        //
    u8    br_swap;
}__scal_src_type_t;

typedef struct __SCAL_OUT_TYPE
{
    u8    byte_seq;  //0:byte0,byte1, byte2, byte3; 1: byte3, byte2, byte1, byte0
    u8    fmt;       //0:plannar rgb; 1: argb(byte0,byte1, byte2, byte3); 2:bgra; 4:yuv444; 5:yuv420; 6:yuv422; 7:yuv411

    bool  alpha_en;   //output alpha channel enable, valid when rgb888fmt
    u8   alpha_coef_type;  //0:soft type;  1: sharp type
}__scal_out_type_t;

typedef struct __SCAL_SRC_SIZE
{
    u32   src_width;
	u32   src_height;
    u32   x_off;
    u32   y_off;
    u32   scal_width;
    u32   scal_height;
}__scal_src_size_t;

typedef struct __SCAL_OUT_SIZE
{
    u32   width;
    u32   height;  //when ouput interlace enable,  the height is the 2x height of scale, for example, ouput is 480i, this value is 480
    u32   x_off;
    u32   y_off;
    u32   fb_width;
    u32   fb_height;
}__scal_out_size_t;

typedef struct _SCAL_BUF_ADDR
{
    u32   ch0_addr;   //
    u32   ch1_addr;
    u32   ch2_addr;
}__scal_buf_addr_t;

typedef struct _SCAL_SCAN_MOD
{
    u8    field;    //0:frame scan; 1:field scan
    u8    bottom;      //0:top field; 1:bottom field
}__scal_scan_mod_t;

typedef struct
{
	bool                  b_interlace_out;
	bool                  b_trd_out;
	disp_3d_out_mode      trd_out_mode;
	disp_size             in_size; //single size
	disp_size             out_size;//single size
	disp_size             disp_size;//full size out
	disp_cs_mode          csc_mode;
}disp_frame_info;

s32 DE_SCAL_Set_Reg_Base(u32 sel, u32 base);
u32 DE_SCAL_Get_Reg_Base(u8 sel);
s32 DE_SCAL_Config_Src(u8 sel, __scal_buf_addr_t *addr, __scal_src_size_t *size,
                         __scal_src_type_t *type, u8 field, u8 dien);
s32 DE_SCAL_Set_Fb_Addr(u8 sel, __scal_buf_addr_t *addr);
s32 DE_SCAL_Set_Init_Phase(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                             __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                             __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 dien);
s32 DE_SCAL_Agth_Config(u8 sel, __scal_src_type_t *in_type,__scal_src_size_t *in_size,__scal_out_size_t *out_size,
                          u8 dien,u8 trden,__scal_3d_outmode_t outmode);
s32 DE_SCAL_Set_Scaling_Factor(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                                 __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                                 __scal_out_size_t *out_size, __scal_out_type_t *out_type);
s32 DE_SCAL_Set_Scaling_Coef(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                               __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                               __scal_out_size_t *out_size, __scal_out_type_t *out_type, u8 smth_mode);
s32 DE_SCAL_Set_Scaling_Coef_for_video(u8 sel, __scal_scan_mod_t *in_scan, __scal_src_size_t *in_size,
                               __scal_src_type_t *in_type, __scal_scan_mod_t *out_scan,
                               __scal_out_size_t *out_size, __scal_out_type_t *out_type, u32 smth_mode)  ;
s32 DE_SCAL_Set_CSC_Coef(u8 sel, u8 in_csc_mode, u8 out_csc_mode, u8 incs, u8 outcs, u32  in_br_swap, u32 out_br_swap);
s32 DE_SCAL_Set_Out_Format(u8 sel, __scal_out_type_t *out_type);
s32 DE_SCAL_Set_Out_Size(u8 sel, __scal_scan_mod_t *out_scan, __scal_out_type_t *out_type, __scal_out_size_t *out_size);
s32 DE_SCAL_Set_Trig_Line(u8 sel, u32 line);
s32 DE_SCAL_Set_Int_En(u8 sel, u32 int_num);
s32 DE_SCAL_Set_Di_Ctrl(u8 sel, u8 en, u8 mode, u8 diagintp_en, u8 tempdiff_en);
s32 DE_SCAL_Set_Di_PreFrame_Addr(u8 sel, u32 luma_addr, u32 chroma_addr);
s32 DE_SCAL_Set_Di_MafFlag_Src(u8 sel, u32 cur_addr, u32 pre_addr, u32 stride);
s32 DE_SCAL_Set_Filtercoef_Ready(u8 sel);
s32 DE_SCAL_Output_Select(u8 sel, u8 out);
s32 DE_SCAL_Writeback_Enable(u8 sel);
s32 DE_SCAL_Writeback_Disable(u8 sel);
s32 DE_SCAL_Writeback_Linestride_Enable(u8 sel);
s32 DE_SCAL_Writeback_Linestride_Disable(u8 sel);
s32 DE_SCAL_Set_Writeback_Addr_ex(u32 sel, __scal_buf_addr_t *addr, __scal_out_size_t *size,__scal_out_type_t *type);
s32 DE_SCAL_Set_Writeback_Addr(u8 sel, __scal_buf_addr_t *addr);
u16 DE_SCAL_Get_Input_Width(u8 sel);
u16 DE_SCAL_Get_Input_Height(u8 sel);
s32 DE_SCAL_Set_CSC_Coef_Enhance(u8 sel, u8 in_csc_mode, u8 out_csc_mode, u8 incs, u8 outcs,
                                                   s32  bright, s32 contrast, s32 saturaion, s32 hue,
                                                   u32  in_br_swap, u32 out_br_swap);
s32 DE_SCAL_Get_3D_In_Single_Size(__scal_3d_inmode_t inmode, __scal_src_size_t *fullsize,__scal_src_size_t *singlesize);
s32 DE_SCAL_Get_3D_Out_Single_Size(__scal_3d_outmode_t outmode, __scal_out_size_t *singlesize,__scal_out_size_t *fullsize);
s32 DE_SCAL_Get_3D_Out_Full_Size(__scal_3d_outmode_t outmode, __scal_out_size_t *singlesize,__scal_out_size_t *fullsize);
s32 DE_SCAL_Set_3D_Fb_Addr(u8 sel, __scal_buf_addr_t *addr, __scal_buf_addr_t *addrtrd);
s32 DE_SCAL_Set_3D_Di_PreFrame_Addr(u8 sel, __scal_buf_addr_t *addr, __scal_buf_addr_t *addrtrd);
s32 DE_SCAL_Set_3D_Ctrl(u8 sel, u8 trden, __scal_3d_inmode_t inmode,
								__scal_3d_outmode_t outmode);
s32 DE_SCAL_Config_3D_Src(u8 sel, __scal_buf_addr_t *addr, __scal_src_size_t *size,
                           __scal_src_type_t *type, __scal_3d_inmode_t trdinmode, __scal_buf_addr_t *addrtrd);
s32 DE_SCAL_Input_Port_Select(u8 sel, u8 port);

s32 DE_SCAL_Vpp_Enable(u8 sel, u32 enable);
s32 DE_SCAL_Vpp_Set_Luma_Sharpness_Level(u8 sel, u32 level);
s32 DE_SCAL_Vpp_Set_Chroma_Sharpness_Level(u8 sel, u32 level);
s32 DE_SCAL_Vpp_Set_White_Level_Extension(u8 sel, u32 level);
s32 DE_SCAL_Vpp_Set_Black_Level_Extension(u8 sel, u32 level);
s32 DE_SCAL_Reset(u8 sel);
s32 DE_SCAL_Start(u8 sel);
s32 DE_SCAL_Set_Reg_Rdy(u8 sel);
s32 DE_SCAL_Enable(u8 sel);
s32 DE_SCAL_Disable(u8 sel);
s32 DE_SCAL_Get_Field_Status(u8 sel);
s32 DE_SCAL_EnableINT(u8 sel, u32 irqsrc);
s32 DE_SCAL_DisableINT(u8 sel, u32 irqsrc);
u32 DE_SCAL_QueryINT(u8 sel);
u32 DE_SCAL_ClearINT(u8 sel, u32 irqsrc);
s32 DE_SCAL_Input_Select(u8 sel, u32 source);
u8 DE_SCAL_Get_Input_Format(u8 sel);
u8 DE_SCAL_Get_Input_Mode(u8 sel);
u8 DE_SCAL_Get_Output_Format(u8 sel);
u16 DE_SCAL_Get_Input_Width(u8 sel);
u16 DE_SCAL_Get_Input_Height(u8 sel);
u16 DE_SCAL_Get_Output_Width(u8 sel);
u16 DE_SCAL_Get_Output_Height(u8 sel);
s32 DE_SCAL_Get_Start_Status(u8 sel);

u32 DE_SCAL_Get_Fb_Addr0(u8 sel);
u32 DE_SCAL_Set_Fb_Addr0(u8 sel, u32 *addr);

s32 DE_Set_Reg_Base(u32 sel, u32 address);
u32 DE_Get_Reg_Base(u32 sel);
u32 DE_BE_Reg_Init(u32 sel);
s32 DE_BE_Enable(u32 sel);
s32 DE_BE_Disable(u32 sel);
s32 DE_BE_Output_Select(u32 sel, u32 out_sel);
s32 DE_BE_Set_BkColor(u32 sel, disp_color_info bkcolor);
s32 DE_BE_Set_ColorKey(u32 sel, disp_color_info ck_max,disp_color_info  ck_min,u32 ck_red_match, u32 ck_green_match, u32 ck_blue_match);
s32 DE_BE_Set_SystemPalette(u32 sel, u32 * pbuffer, u32 offset,u32 size);
s32 DE_BE_Get_SystemPalette(u32 sel, u32 *pbuffer, u32 offset,u32 size);
s32 DE_BE_Cfg_Ready(u32 sel);
s32 DE_BE_EnableINT(u8 sel,u32 irqsrc);
s32 DE_BE_DisableINT(u8 sel, u32 irqsrc);
u32 DE_BE_QueryINT(u8 sel);
u32 DE_BE_ClearINT(u8 sel,u32 irqsrc);
s32 DE_BE_reg_auto_load_en(u32 sel, u32 en);

s32 DE_BE_Layer_Enable(u32 sel, u8 layidx, bool enable);
s32 DE_BE_Layer_Set_Format(u32 sel, u8 layidx,u8 format,bool br_swap,u8 order, bool pre_multiply);
s32 DE_BE_Layer_Set_Framebuffer(u32 sel, u8 layidx,layer_src_t *layer_fb);
s32 DE_BE_Layer_Set_Screen_Win(u32 sel, u8 layidx, disp_window * win);
s32 DE_BE_Layer_Video_Enable(u32 sel, u8 layidx,bool video_en);
s32 DE_BE_Layer_Video_Ch_Sel(u32 sel, u8 layidx,u8 scaler_index);
s32 DE_BE_Layer_Yuv_Ch_Enable(u32 sel, u8 layidx,bool yuv_en);
s32 DE_BE_Layer_Set_Prio(u32 sel, u8 layidx,u8 prio);
s32 DE_BE_Layer_Set_Pipe(u32 sel, u8 layidx,u8 pipe);
s32 DE_BE_Layer_Alpha_Enable(u32 sel, u8 layidx, bool enable);
s32 DE_BE_Layer_Set_Alpha_Value(u32 sel, u8 layidx,u8 alpha_val);
s32 DE_BE_Layer_Global_Pixel_Alpha_Enable(u32 sel, u8 layidx, bool enable);
s32 DE_BE_Layer_ColorKey_Enable(u32 sel, u8 layidx, bool enable);
s32 DE_BE_Layer_Set_Work_Mode(u32 sel, u8 layidx,u8 mode);

s32 DE_BE_YUV_CH_Enable(u32 sel, bool enable);
s32 DE_BE_YUV_CH_Set_Src(u32 sel, de_yuv_ch_src_t * in_src);

s32 DE_BE_HWC_Enable(u32 sel, bool enable);
s32 DE_BE_HWC_Set_Pos(u32 sel, disp_position * pos);
s32 DE_BE_HWC_Get_Pos(u32 sel, disp_position * pos);
s32 DE_BE_HWC_Set_Palette(u32 sel, u32 address,u32 offset,u32 size);
s32 DE_BE_HWC_Get_Format(void);
s32 DE_BE_HWC_Set_Src(u32 sel, de_hwc_src_t *hwc_pat);

s32 DE_BE_Sprite_Enable(u32 sel, bool enable);
s32 DE_BE_Sprite_Disable(u32 sel);
s32 DE_BE_Sprite_Set_Format(u32 sel, u8 pixel_seq,u8 format);
s32 DE_BE_Sprite_Global_Alpha_Enable(u32 sel, bool enable);
s32 DE_BE_Sprite_Set_Global_Alpha(u32 sel, u8 alpha_val);
s32 DE_BE_Sprite_Block_Set_Pos(u32 sel, u8 blk_idx,s16 x,s16 y);
s32 DE_BE_Sprite_Block_Set_Size(u32 sel, u8 blk_idx,u32 xsize,u32 ysize);
s32 DE_BE_Sprite_Block_Set_fb(u32 sel, u8 blk_idx,u32 addr, u32 line_width);
s32 DE_BE_Sprite_Block_Set_Next_Id(u32 sel, u8 blk_idx,u8 next_blk_id);
s32 DE_BE_Sprite_Set_Palette_Table(u32 sel, u32 address, u32 offset, u32 size);
s32 DE_BE_Set_Enhance(u8 sel,u32 brightness, u32 contrast, u32 saturaion, u32 hue);
s32 DE_BE_Set_Enhance_ex(u8 sel, u32 out_csc, u32 out_color_range, u32 enhance_en, u32 brightness, u32 contrast, u32 saturaion, u32 hue);
s32 DE_BE_enhance_enable(u32 sel, bool enable);
s32 DE_BE_set_display_size(u32 sel, u32 width, u32 height);
s32 DE_BE_get_display_width(u32 sel);
s32 DE_BE_get_display_height(u32 sel);
s32 DE_BE_deflicker_enable(u32 sel, bool enable);
s32 DE_BE_Set_Outitl_enable(u32 sel, bool enable);
s32 DE_BE_Format_To_Bpp(u32 sel, u8 format);
u32 DE_BE_Offset_To_Addr(u32 src_addr,u32 width,u32 x,u32 y,u32 bpp);
u32 DE_BE_Addr_To_Offset(u32 src_addr,u32 off_addr,u32 width,u32 bpp,disp_position *pos);

s32 IEP_Drc_Init(u32 sel);
s32 IEP_Drc_Exit(u32 sel);
s32 IEP_Drc_Enable(u32 sel, u32 en);
s32 IEP_Drc_Operation_In_Vblanking(u32 sel);
s32 IEP_Drc_Set_Reg_Base(u32 sel, u32 base);
s32 IEP_Drc_Get_Reg_Base(u32 sel);
s32 IEP_Drc_Set_Winodw(u32 sel, disp_window window);//full screen for default
s32 IEP_Drc_Set_Mode(u32 sel, __iep_drc_mode_t mode);
s32 iep_drc_early_suspend(u32 sel);//close clk
s32 iep_drc_suspend(u32 sel);//save register
s32 iep_drc_resume (u32 sel);//restore register
s32 iep_drc_late_resume(u32 sel);//open clk
s32 IEP_Drc_Set_Imgsize(u32 sel, u32 width, u32 height);
s32 IEP_Drc_Update_Backlight(u32 sel, u32 backlight);
s32 IEP_Drc_Get_Backlight_Dimming(u32 sel);

s32 IEP_CMU_Set_Reg_Base(u32 sel, u32 address);
s32 IEP_CMU_Reg_Init(u32 sel);
s32 IEP_CMU_Enable(u32 sel, u32 enable);
s32 IEP_CMU_Disable(u32 sel);
s32 IEP_CMU_Set_Data_Flow(__u8 sel, u32 data_flow);
s32 IEP_CMU_Set_Window(__u8 sel, disp_window *window);
s32 IEP_CMU_Set_Imgsize(__u8 sel, u32 width, u32 height);
s32 IEP_CMU_Set_Par(__u8 sel, u32 hue, u32 saturaion, u32 brightness, u32 mode);
s32 IEP_CMU_Operation_In_Vblanking(u32 sel);
s32 iep_cmu_early_suspend(u32 sel);
s32 iep_cmu_suspend(u32 sel);
s32 iep_cmu_resume(u32 sel);
s32 iep_cmu_late_resume(u32 sel);

s32 IEP_Deu_Enable(u32 sel, u32 enable);
s32 IEP_Deu_Set_Luma_Sharpness_Level(u32 sel, u32 level);
s32 IEP_Deu_Set_Chroma_Sharpness_Level(u32 sel, u32 level);
s32 IEP_Deu_Set_White_Level_Extension(u32 sel, u32 level);
s32 IEP_Deu_Set_Black_Level_Extension(u32 sel, u32 level);
s32 IEP_Deu_Set_Ready(u32 sel);
s32 IEP_Deu_Set_Reg_base(u32 sel, u32 base);
s32 IEP_Deu_Set_Winodw(u32 sel, disp_window *window);
s32 IEP_Deu_Output_Select(u32 sel, u32 be_ch);
s32 IEP_Deu_Init(u32 sel);
s32 IEP_Deu_Exit(u32 sel);
s32 IEP_Deu_Operation_In_Vblanking(u32 sel);
s32 IEP_Deu_Set_frameinfo(u32 sel, disp_frame_info frameinfo);

s32 de_top_set_reg_base(u32 reserve, u32 reg_base);
s32 de_top_clk_enable(u32 mod_id, u32 enable);
s32 de_top_clk_div(u32 mod_id, u32 div);

#endif  /* __EBIOS_DE_H__ */
