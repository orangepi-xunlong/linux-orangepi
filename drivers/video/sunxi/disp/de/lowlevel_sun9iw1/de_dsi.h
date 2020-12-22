#ifndef __de_dsi_h__
#define __de_dsi_h__

struct dsi_init_para
{
	s32 (*delay_ms)(u32 ms);
	s32 (*delay_us)(u32 us);
};

typedef void (*dsi_print)(const char *c);
typedef void (*dsi_print_val)(u32 val);

extern int dsi_set_print_func(dsi_print print);
extern int dsi_set_print_val_func(dsi_print_val print_val);
s32 dsi_init(struct dsi_init_para* para);

s32 dsi_set_reg_base(u32 sel, u32 base);
u32 dsi_get_reg_base(u32 sel);
u32 dsi_irq_query(u32 sel,__dsi_irq_id_t id);
s32 dsi_cfg(u32 sel,disp_panel_para * panel);
s32 dsi_exit(u32 sel);
s32 dsi_open(u32 sel,disp_panel_para * panel);
s32 dsi_close(u32 sel);
s32 dsi_inst_busy(u32 sel);
s32 dsi_tri_start(u32 sel);
s32 dsi_dcs_wr(u32 sel,__u32 cmd,u8* para_p,u32 para_num);
s32 dsi_dcs_wr_index(u32 sel,u8 index);
s32 dsi_dcs_wr_data(u32 sel,u8 data);
u32 dsi_get_start_delay(u32 sel);
u32 dsi_get_cur_line(u32 sel);
u32 dsi_io_open(u32 sel,disp_panel_para * panel);
u32 dsi_io_close(u32 sel);
s32 dsi_clk_enable(u32 sel, u32 en);

__s32 dsi_dcs_wr_0para(__u32 sel,__u8 cmd);
__s32 dsi_dcs_wr_1para(__u32 sel,__u8 cmd,__u8 para);
__s32 dsi_dcs_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
__s32 dsi_dcs_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
__s32 dsi_dcs_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
__s32 dsi_dcs_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);

__s32 dsi_gen_wr_0para(__u32 sel,__u8 cmd);
__s32 dsi_gen_wr_1para(__u32 sel,__u8 cmd,__u8 para);
__s32 dsi_gen_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
__s32 dsi_gen_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
__s32 dsi_gen_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
__s32 dsi_gen_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);

#endif
