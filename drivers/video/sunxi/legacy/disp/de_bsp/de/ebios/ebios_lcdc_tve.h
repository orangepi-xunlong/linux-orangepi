#ifndef __EBIOS_LCDC_TVE_H_
#define __EBIOS_LCDC_TVE_H_

#include "../../bsp_display.h"

typedef enum
{
    LCD_IRQ_TCON0_VBLK	= 15,
    LCD_IRQ_TCON1_VBLK = 14,
    LCD_IRQ_TCON0_LINE = 13,
    LCD_IRQ_TCON1_LINE = 12,
    LCD_IRQ_TCON0_TRIF = 11,
    LCD_IRQ_TCON0_CNTR = 10,
}__lcd_irq_id_t;

typedef enum
{
    DSI_IRQ_VIDEO_LINE	= 3,
    DSI_IRQ_VIDEO_VBLK  = 2,
    DSI_IRQ_INSTR_STEP  = 1,
    DSI_IRQ_INSTR_END   = 0,
}__dsi_irq_id_t;


typedef enum
{
    LCD_SRC_BE0 	= 0,
    LCD_SRC_BE1 	= 1,
    LCD_SRC_DMA888 	= 2,
    LCD_SRC_DMA565 	= 3,
    LCD_SRC_BLACK	= 4,
    LCD_SRC_WHITE	= 5,
    LCD_SRC_BLUE	= 6,
}__lcd_src_t;

typedef struct
{
	__u32	pixel_clk;
	__u16	hor_pixels;
	__u16	ver_pixels;
	__u16	hor_total_time;
	__u16	hor_front_porch;
	__u16	hor_sync_time;
	__u16	hor_back_time;
	__u16	ver_total_time;
	__u16	ver_front_porch;
	__u16	ver_sync_time;
	__u16	ver_back_porch;
	__bool	hor_sync_polarity;
	__bool	ver_sync_polarity;
	__bool  interlace;
    __u16   vactive_space;
    __bool  trd_mode;
}__disp_timing_t;


#define TVE_D0ActFlags  (0x01)
#define TVE_D1ActFlags  (0x01<<1)
#define TVE_D2ActFlags  (0x01<<2)
#define TVE_D3ActFlags  (0x01<<3)

typedef enum
{
    TVE_MODE_NTSC = 0, 
    TVE_MODE_PAL,
    TVE_MODE_480I,
    TVE_MODE_576I,
    TVE_MODE_480P,
    TVE_MODE_576P,
    TVE_MODE_720P_50HZ,
    TVE_MODE_720P_60HZ,
    TVE_MODE_1080I_50HZ,
    TVE_MODE_1080I_60HZ,
    TVE_MODE_1080P_50HZ,
    TVE_MODE_1080P_60HZ,
    TVE_MODE_VGA
}__tve_mode_t;

typedef enum tag_TVE_DAC
{
    DAC1 = 1, //bit0
    DAC2 = 2, //bit1
    DAC3 = 4  //bit2
}__tve_dac_t;

typedef enum tag_TVE_SRC
{
    CVBS = 0, 
    SVIDEO_Y = 1,
    SVIDEO_C = 2,
    COMPONENT_Y = 4,
    COMPONENT_PB = 5,
    COMPONENT_PR = 6,
    VGA_R = 4,
    VGA_G = 5,
    VGA_B = 6  
}__tve_src_t;

__s32 	hmid_src_sel(__u32 sel);
__s32 	dsi_src_sel(__u32 sel);
__s32   lvds_open(__u32 sel, __panel_para_t * panel);
__s32   lvds_close(__u32 sel);


__s32 	tcon_set_reg_base(__u32 sel, __u32 address);
__u32   tcon_get_reg_base(__u32 sel);
__s32   tcon_init(__u32 sel);
__s32   tcon_exit(__u32 sel);
__s32   tcon_get_timing(__u32 sel,__u32 index,__disp_tcon_timing_t* tt);
__u32 	tcon_irq_query(__u32 sel,__lcd_irq_id_t id);
__u32   tcon_get_start_delay(__u32 sel,__u32 tcon_index);
__u32   tcon_get_cur_line(__u32 sel, __u32 tcon_index);
__s32   tcon_gamma(__u32 sel, __u32 en,__u32 *gamma_tbl);
__s32 tcon_ceu(__u32 sel,__u32 mode,__s32 b,__s32 c,__s32 s,__s32 h);

__s32 	tcon0_cfg(__u32 sel, __panel_para_t * panel);
__s32   tcon0_src_select(__u32 sel, __lcd_src_t src);
__s32 tcon0_src_get(__u32 sel);
__s32 	tcon0_open(__u32 sel, __panel_para_t * panel);
__s32 	tcon0_close(__u32 sel);
__s32   tcon0_set_dclk_div(__u32 sel, __u8 div);
__u32   tcon0_get_dclk_div(__u32 sel);
__s32	tcon0_tri_busy(__u32 sel);
__s32   tcon0_tri_start(__u32 sel);
__s32	tcon0_cpu_wr_24b(__u32 sel, __u32 index, __u32 data);
__s32	tcon0_cpu_wr_24b_index(__u32 sel, __u32 index);
__s32	tcon0_cpu_wr_24b_data(__u32 sel, __u32 data);
__s32	tcon0_cpu_rd_24b(__u32 sel, __u32 index, __u32 *data);
__s32	tcon0_cpu_wr_16b(__u32 sel, __u32 index, __u32 data);
__s32	tcon0_cpu_wr_16b_index(__u32 sel, __u32 index);
__s32	tcon0_cpu_wr_16b_data(__u32 sel, __u32 data);
__s32	tcon0_cpu_rd_16b(__u32 sel, __u32 index, __u32 *data);
__u32 tcon0_cpu_busy(__u32 sel);
__u32 tcon0_cpu_16b_to_24b(__u32 value);
__u32 tcon0_cpu_24b_to_16b(__u32 value);

__s32 	tcon1_open(__u32 sel);
__s32 	tcon1_close(__u32 sel);
__s32 	tcon1_set_hdmi_mode(__u32 sel,  __disp_tv_mode_t mode);
__s32 	tcon1_set_tv_mode(__u32 sel, __disp_tv_mode_t mode);
__s32   tcon1_set_vga_mode(__u32 sel,  __disp_vga_mode_t mode);
__s32   tcon1_src_select(__u32 sel, __lcd_src_t src);
__s32   tcon1_src_get(__u32 sel);
__s32 hmdi_src_sel(__u32 sel);
__s32 tcon_irq_enable(__u32 sel, __lcd_irq_id_t id);
__s32 tcon_irq_disable(__u32 sel, __lcd_irq_id_t id);
__s32 tcon1_cfg(__u32 sel,__disp_timing_t* timing);
__s32 tcon1_cfg_ex(__u32 sel, __panel_para_t * panel);

__s32   dsi_set_reg_base(__u32 sel, __u32 base);
__u32   dsi_get_reg_base(__u32 sel);
__u32 	dsi_irq_query(__u32 sel,__dsi_irq_id_t id);
__s32   dsi_cfg(__u32 sel,__panel_para_t * panel);
__s32   dsi_exit(__u32 sel);
__s32   dsi_open(__u32 sel,__panel_para_t * panel);
__s32   dsi_close(__u32 sel);
__s32   dsi_inst_busy(__u32 sel);
__s32   dsi_tri_start(__u32 sel);
__s32   dsi_dcs_wr(__u32 sel,__u8 cmd,__u8* para_p,__u32 para_num);
__s32   dsi_dcs_wr_index(__u32 sel,__u8 index);
__s32   dsi_dcs_wr_data(__u32 sel,__u8 data);
__u32 	dsi_get_start_delay(__u32 sel);
__u32 	dsi_get_cur_line(__u32 sel);
__u32   dsi_io_open(__u32 sel,__panel_para_t * panel);
__u32   dsi_io_close(__u32 sel);
__s32   dsi_clk_enable(__u32 sel, __u32 en);
__u32 dsi_dphy_get_reg_base(__u32 sel);
__s32 dsi_irq_enable(__u32 sel, __dsi_irq_id_t id);
__s32 dsi_irq_disable(__u32 sel, __dsi_irq_id_t id);
__s32 dsi_dcs_wr_0para(__u32 sel,__u8 cmd);
__s32 dsi_dcs_wr_1para(__u32 sel,__u8 cmd,__u8 para);
__s32 dsi_dcs_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
__s32 dsi_dcs_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
__s32 dsi_dcs_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
__s32 dsi_dcs_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);
__s32 dsi_dcs_wr_memory(__u32 sel,__u32* p_data,__u32 length);
__s32 dsi_dcs_rd(__u32 sel,__u8 cmd,__u8* para_p,__u32* num_p);
__s32 dsi_dcs_rd_memory(__u32 sel,__u32* p_data,__u32 length);

__s32   TVE_set_reg_base(__u32 sel,__u32 address);
__u32   TVE_get_reg_base(__u32 sel);
__s32   TVE_init(__u32 sel);
__s32   TVE_exit(__u32 sel);
__s32   TVE_open(__u32 sel);
__s32   TVE_close(__u32 sel);
__s32   TVE_set_vga_mode(__u32 sel);
__s32   TVE_set_tv_mode(__u32 sel, __u8 mode);
__u8    TVE_query_interface(__u32 sel,__u8 index);
__u8    TVE_query_int(__u32 sel);
__u8    TVE_clear_int (__u32 sel);
__u8    TVE_dac_int_enable(__u32 sel,__u8 index);
__u8    TVE_dac_int_disable(__u32 sel,__u8 index);
__u8    TVE_dac_autocheck_enable(__u32 sel,__u8 index);
__u8    TVE_dac_autocheck_disable(__u32 sel,__u8 index);
__u8    TVE_dac_enable(__u32 sel,__u8 index);
__u8    TVE_dac_disable(__u32 sel,__u8 index);
__u8    TVE_dac_set_de_bounce(__u32 sel,__u8 index,__u32 times);
__u8    TVE_dac_get_de_bounce(__u32 sel,__u8 index);
__s32   TVE_dac_set_source(__u32 sel,__u32 index,__u32 source);
__s32   TVE_dac_get_source(__u32 sel,__u32 index);
__s32   TVE_get_dac_status(__u32 index);
__u8 	TVE_csc_init(__u32 sel,__u8 type);
__s32   TVE_dac_sel(__u32 sel,__u32 dac, __u32 index);
__u8 TVE_csc_enable(__u32 sel);
__u8 TVE_csc_disable(__u32 sel);

#endif

