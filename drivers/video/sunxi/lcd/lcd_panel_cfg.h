
#ifndef __LCD_PANNEL_CFG_H__
#define __LCD_PANNEL_CFG_H__

#include "dev_lcd.h"
#include <linux/gpio.h>
#include "panels/panels.h"

void LCD_set_panel_funs(void);

extern void LCD_OPEN_FUNC(u32 sel, LCD_FUNC func, u32 delay/*ms*/);
extern void LCD_CLOSE_FUNC(u32 sel, LCD_FUNC func, u32 delay/*ms*/);
extern void LCD_delay_ms(u32 ms) ;
extern void LCD_delay_us(u32 ns);
extern void TCON_open(u32 sel);
extern void TCON_close(u32 sel);
extern s32 LCD_PWM_EN(u32 sel, bool b_en);
extern s32 LCD_BL_EN(u32 sel, bool b_en);
extern s32 LCD_POWER_EN(u32 sel, bool b_en);
extern void LCD_CPU_register_irq(u32 sel, void (*Lcd_cpuisr_proc) (void));
extern void LCD_CPU_WR(u32 sel, u32 index, u32 data);
extern void LCD_CPU_WR_INDEX(u32 sel,u32 index);
extern void LCD_CPU_WR_DATA(u32 sel, u32 data);
extern void LCD_CPU_AUTO_FLUSH(u32 sel, bool en);
extern void pwm_clock_enable(u32 sel);
extern void pwm_clock_disable(u32 sel);
extern s32 LCD_POWER_ELDO3_EN(u32 sel, bool b_en, u32 voltage);
extern s32 LCD_POWER_DLDO1_EN(u32 sel, bool b_en, u32 voltage);

extern s32 lcd_iic_write(u8 slave_addr, u8 sub_addr, u8 value);
extern s32 lcd_iic_read(u8 slave_addr, u8 sub_addr, u8* value);

extern s32 lcd_get_panel_para(u32 sel,disp_panel_para * info);

extern s32 dsi_dcs_wr(u32 sel,u8 cmd,u8* para_p,u32 para_num);
extern s32 dsi_dcs_wr_0para(u32 sel,u8 cmd);
extern s32 dsi_dcs_wr_1para(u32 sel,u8 cmd,u8 para);
extern s32 dsi_dcs_wr_2para(u32 sel,u8 cmd,u8 para1,u8 para2);
extern s32 dsi_dcs_wr_3para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3);
extern s32 dsi_dcs_wr_4para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3,u8 para4);
extern s32 dsi_dcs_wr_5para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3,u8 para4,u8 para5);

extern __s32 dsi_gen_wr_0para(__u32 sel,__u8 cmd);
extern __s32 dsi_gen_wr_1para(__u32 sel,__u8 cmd,__u8 para);
extern __s32 dsi_gen_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
extern __s32 dsi_gen_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
extern __s32 dsi_gen_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
extern __s32 dsi_gen_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);

extern __s32 dsi_dcs_rd(__u32 sel,__u8	cmd,__u8* para_p,__u32*	num_p);

extern s32 LCD_GPIO_request(u32 sel, u32 io_index);
extern s32 LCD_GPIO_release(u32 sel,u32 io_index);
extern s32 LCD_GPIO_set_attr(u32 sel,u32 io_index, bool b_output);
extern s32 LCD_GPIO_read(u32 sel,u32 io_index);
extern s32 LCD_GPIO_write(u32 sel,u32 io_index, u32 data);

#define sys_get_wvalue(n)   (*((volatile u32 *)(n)))          /* word input */
#define sys_put_wvalue(n,c) (*((volatile u32 *)(n))  = (c))   /* word output */

#endif

