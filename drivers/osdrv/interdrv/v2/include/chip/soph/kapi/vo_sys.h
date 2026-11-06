#ifndef __VO_SYS_H__
#define __VO_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_vo_sys.h"

#define VO_SYS_CLK_RATIO_MASK(CLK_NAME) VO_SYS_REG_CK_COEF_##CLK_NAME##_MASK
#define VO_SYS_CLK_RATIO_OFFSET(CLK_NAME) VO_SYS_REG_CK_COEF_##CLK_NAME##_OFFSET
#define VO_SYS_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
		vo_sys_reg_write_mask(VO_SYS_REG_CK_COEF_##CLK_NAME, \
			VO_SYS_CLK_RATIO_MASK(CLK_NAME), \
			RATIO << VO_SYS_CLK_RATIO_OFFSET(CLK_NAME))


union vo_sys_reset {
	struct {
		u32 _2de0 : 1;
		u32 _2de1 : 1;
		u32 oenc0 : 1;
		u32 oenc1 : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 disp0 : 1;
		u32 disp1 : 1;
		u32 vo_mac0 : 1;
		u32 vo_mac1 : 1;
		u32 dsi_mac0 : 1;
		u32 dsi_mac1 : 1;
		u32 dsi_phy0 : 1;
		u32 dsi_phy1 : 1;
		u32 hdmi_mac : 1;
		u32 hdmi_phy : 1;
		u32 clk_bt_div_0 : 1;
		u32 clk_bt_div_1 : 1;
	} b;
	u32 raw;
};

union vo_sys_reset_apb {
	struct {
		u32 _2de0 : 1;
		u32 _2de1 : 1;
		u32 oenc0 : 1;
		u32 oenc1 : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 disp0 : 1;
		u32 disp1 : 1;
		u32 vo_mac0 : 1;
		u32 vo_mac1 : 1;
		u32 dsi_mac0 : 1;
		u32 dsi_mac1 : 1;
		u32 dsi_phy0 : 1;
		u32 dsi_phy1 : 1;
		u32 hdmi_mac : 1;
	} b;
	u32 raw;
};

union vo_sys_intr {
	struct {
		u32 _2de0 : 1;
		u32 _2de1 : 1;
		u32 oenc0_b0 : 1;
		u32 oenc0_b1 : 1;
		u32 oenc1_b0 : 1;
		u32 oenc1_b1 : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 disp0 : 1;
		u32 disp1 : 1;
		u32 vo0 : 1;
		u32 vo1 : 1;
		u32 hdmi_mac : 1;
		u32 hdmi_wakeup : 1;
	} b;
	u32 raw;
};

union vo_sys_clk_lp {
	struct {
		u32 _2de0 : 1;
		u32 _2de1 : 1;
		u32 oenc0 : 1;
		u32 oenc1 : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 disp0 : 1;
		u32 disp1 : 1;
		u32 dsi_mac0 : 1;
		u32 dsi_mac1 : 1;
		u32 dsi_phy0 : 1;
		u32 dsi_phy1 : 1;
		u32 hdmi_mac : 1;
		u32 vo_mac0 : 1;
		u32 vo_mac1 : 1;
		u32 vo_sys : 1;
		u32 auto_2de0 : 1;
		u32 auto_2de1 : 1;
		u32 auto_oenc0 : 1;
		u32 auto_oenc1 : 1;
		u32 auto_vpss0 : 1;
		u32 auto_vpss1 : 1;
		u32 auto_disp0 : 1;
		u32 auto_disp1 : 1;
		u32 auto_dsi_mac0 : 1;
		u32 auto_dsi_mac1 : 1;
		u32 auto_dsi_phy0 : 1;
		u32 auto_dsi_phy1 : 1;
		u32 auto_hdmi_mac : 1;
		u32 auto_vo_mac0 : 1;
		u32 auto_vo_mac1 : 1;
		u32 auto_vo_sys : 1;
	} b;
	u32 raw;
};

union vo_sys_clk_ctrl0 {
	struct {
		u32 bt_src_sel_0 : 2;
		u32 rsv_b2 : 2;
		u32 disp_sel_bt_div1_0 : 1;
		u32 disp_div_cnt_0 : 3;
		u32 dsi_mac_src_sel_0 : 1;
		u32 rsv_9 : 7;
		u32 disp_div_up_w1t_0 : 1;
	} b;
	u32 raw;
};

union vo_sys_clk_ctrl1 {
	struct {
		u32 bt_src_sel_1 : 2;
		u32 rsv_b2 : 2;
		u32 disp_sel_bt_div1_1 : 1;
		u32 disp_div_cnt_1 : 3;
		u32 dsi_mac_src_sel_1 : 1;
		u32 rsv_9 : 7;
		u32 disp_div_up_w1t_1 : 1;
		u32 hdmi_src_sel : 1;
	} b;
	u32 raw;
};

enum vo_sys_axi_bus {
	VO_SYS_AXI_BUS_VPSS0 = 0,
	VO_SYS_AXI_BUS_VPSS1,
	VO_SYS_AXI_BUS_MAX,
};

union top_reset {
	struct {
		u32 main_ap : 1;
		u32 second_ap : 1;
		u32 system : 1;
		u32 visys : 1;
		u32 vi_cam0 : 1;
		u32 vi_cam1 : 1;
		u32 vi_cam2 : 1;
		u32 vosys : 1;
		u32 vdsys0 : 1;
		u32 vdsys0_vd : 1;
		u32 vdsys0_vpss_0 : 1;
		u32 vdsys0_vpss_1 : 1;
		u32 vdsys1 : 1;
		u32 vdsys1_vd : 1;
		u32 vdsys1_vpss_0 : 1;
		u32 vdsys1_vpss_1 : 1;
		u32 vesys : 1;
		u32 vesys_ve : 1;
		u32 vesys_jpeg_0 : 1;
		u32 vesys_jpeg_1 : 1;
		u32 vesys_jpeg_2 : 1;
		u32 vesys_jpeg_3 : 1;
		u32 tpusys : 1;
		u32 tpu0 : 1;
		u32 tpu1 : 1;
	} b;
	u32 raw;
};

/********************************************************************
 *   APIs to replace bmtest's standard APIs
 ********************************************************************/
void vo_sys_set_base_addr(void *base);
union vo_sys_clk_lp vo_sys_get_clk_lp(void);
void vo_sys_set_clk_lp(union vo_sys_clk_lp clk);
union vo_sys_clk_ctrl0 vo_sys_get_clk_ctrl0(void);
void vo_sys_set_clk_ctrl0(union vo_sys_clk_ctrl0 cfg);
union vo_sys_clk_ctrl1 vo_sys_get_clk_ctrl1(void);
void vo_sys_set_clk_ctrl1(union vo_sys_clk_ctrl1 cfg);
union vo_sys_reset vo_sys_get_reset(void);
void vo_sys_set_reset(union vo_sys_reset reset);
void vo_sys_toggle_reset(union vo_sys_reset mask);
union vo_sys_reset_apb vo_sys_get_reset_apb(void);
void vo_sys_set_reset_apb(union vo_sys_reset_apb reset);
void vo_sys_toggle_reset_apb(union vo_sys_reset_apb mask);
union vo_sys_intr vo_sys_get_intr_status(void);
void vo_set_intr_mask(union vo_sys_intr mask);
unsigned int vo_sys_reg_read(uintptr_t addr);
void vo_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data);
void vo_sys_set_offline(enum vo_sys_axi_bus bus, bool offline);
void extern_axi_to_36bit(u32 high_bit);

union top_reset top_sys_get_reset(void);
void top_sys_set_reset(union top_reset reset);

#ifdef __cplusplus
}
#endif

#endif //__COMMON_VIP_H__
