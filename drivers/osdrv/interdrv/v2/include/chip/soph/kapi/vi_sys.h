#ifndef __VI_SYS_H__
#define __VI_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_vi_sys.h"


#define VI_SYS_NORM_CLK_RATIO_MASK(CLK_NAME) VI_SYS_REG_NORM_DIV_##CLK_NAME##_MASK
#define VI_SYS_NORM_CLK_RATIO_OFFSET(CLK_NAME) VI_SYS_REG_NORM_DIV_##CLK_NAME##_OFFSET
#define VI_SYS_NORM_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
		vi_sys_reg_write_mask(VI_SYS_REG_NORM_DIV_##CLK_NAME, \
			VI_SYS_NORM_CLK_RATIO_MASK(CLK_NAME), \
			RATIO << VI_SYS_NORM_CLK_RATIO_OFFSET(CLK_NAME))

#define VI_SYS_UPDATE_CLK_RATIO_MASK(CLK_NAME) VI_SYS_REG_UPDATE_##CLK_NAME##_MASK
#define VI_SYS_UPDATE_CLK_RATIO_OFFSET(CLK_NAME) VI_SYS_REG_UPDATE_##CLK_NAME##_OFFSET
#define VI_SYS_UPDATE_CLK_RATIO(CLK_NAME) \
	vi_sys_reg_write_mask(VI_SYS_REG_UPDATE_##CLK_NAME, \
		VI_SYS_UPDATE_CLK_RATIO_MASK(CLK_NAME), \
		1 << VI_SYS_UPDATE_CLK_RATIO_OFFSET(CLK_NAME))

union vi_sys_reset {
	struct {
		u32 rsv_b0 : 1;
		u32 isp_top : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 csi_mac2 : 1;
		u32 csi_mac3 : 1;
		u32 csi_mac4 : 1;
		u32 csi_mac5 : 1;
		u32 csi_mac6 : 1;
		u32 csi_mac7 : 1;
		u32 csi_phy0 : 1;
		u32 csi_be : 1;
		u32 isp_raw : 1;
		u32 rsv_b13 : 3;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 vpss2 : 1;
		u32 vpss3 : 1;
		u32 ldc0 : 1;
		u32 ldc1 : 1;
		u32 dwa0 : 1;
		u32 dwa1 : 1;
		u32 dpu : 1;
		u32 stitching : 1;
		u32 ive_top0 : 1;
		u32 ive_top1 : 1;
		u32 cdma : 1;
	} b;
	u32 raw;
};

union vi_sys_reset_apb {
	struct {
		u32 rsv_b0 : 1;
		u32 isp_top : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 csi_mac2 : 1;
		u32 csi_mac3 : 1;
		u32 csi_mac4 : 1;
		u32 csi_mac5 : 1;
		u32 csi_mac6 : 1;
		u32 csi_mac7 : 1;
		u32 csi_phy0 : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 vpss2 : 1;
		u32 vpss3 : 1;
		u32 ldc0 : 1;
		u32 ldc1 : 1;
		u32 dwa0 : 1;
		u32 dwa1 : 1;
		u32 dpu : 1;
		u32 stitching : 1;
		u32 ive_top0 : 1;
		u32 ive_top1 : 1;
		u32 cdma : 1;
	} b;
	u32 raw;
};

union vi_sys_intr {
	struct {
		u32 rsv_b0 : 1;
		u32 isp_top : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 csi_mac2 : 1;
		u32 csi_mac3 : 1;
		u32 csi_mac4 : 1;
		u32 csi_mac5 : 1;
		u32 csi_mac6 : 1;
		u32 csi_mac7 : 1;
		u32 rsv_b10 : 6;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 vpss2 : 1;
		u32 vpss3 : 1;
		u32 ldc0 : 1;
		u32 ldc1 : 1;
		u32 dwa0 : 1;
		u32 dwa1 : 1;
		u32 dpu : 1;
		u32 stitching : 1;
		u32 ive_top0 : 1;
		u32 ive_top1 : 1;
		u32 cdma : 1;
	} b;
	u32 raw;
};

union vi_sys_clk_lp0 {
	struct {
		u32 rsv_b0 : 1;
		u32 isp_top : 1;
		u32 csi_phy0 : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 csi_mac2 : 1;
		u32 csi_mac3 : 1;
		u32 csi_mac4 : 1;
		u32 csi_mac5 : 1;
		u32 csi_mac6 : 1;
		u32 csi_mac7 : 1;
		u32 rsv_b11 : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 vpss2 : 1;
		u32 vpss3 : 1;
		u32 ldc0 : 1;
		u32 ldc1 : 1;
		u32 dwa0 : 1;
		u32 dwa1 : 1;
		u32 dpu : 1;
		u32 stitching : 1;
		u32 ive0 : 1;
		u32 ive1 : 1;
		u32 reg_vi : 1;
		u32 cdma : 1;
	} b;
	u32 raw;
};

union vi_sys_clk_lp1 {
	struct {
		u32 isp : 1;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 vpss2 : 1;
		u32 vpss3 : 1;
		u32 ldc0 : 1;
		u32 ldc1 : 1;
		u32 dwa0 : 1;
		u32 dwa1 : 1;
		u32 ive0 : 1;
		u32 ive1 : 1;
		u32 dpu : 1;
		u32 stitching : 1;
	} b;
	u32 raw;
};

union vi_sys_clk_lp2 {
	struct {
		u32 isp_top : 1;
		u32 rsv_b1 : 1;
		u32 csi_phy0 : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 csi_mac2 : 1;
		u32 csi_mac3 : 1;
		u32 csi_mac4 : 1;
		u32 csi_mac5 : 1;
		u32 csi_mac6 : 1;
		u32 csi_mac7 : 1;
		u32 rsv_b11 : 5;
		u32 vpss0 : 1;
		u32 vpss1 : 1;
		u32 vpss2 : 1;
		u32 vpss3 : 1;
		u32 ldc0 : 1;
		u32 ldc1 : 1;
		u32 dwa0 : 1;
		u32 dwa1 : 1;
		u32 dpu : 1;
		u32 stitching : 1;
		u32 ive0 : 1;
		u32 ive1 : 1;
		u32 reg_vi : 1;
		u32 cdma : 1;
	} b;
	u32 raw;
};

union vi_sys_clk_ctrl0 {
	struct {
		u32 vi0_clk0_src_sel : 1;
		u32 vi0_clk1_src_sel : 1;
		u32 vi1_clk_src_sel : 1;
		u32 vi2_clk_src_sel : 1;
		u32 vi_src_sel : 1;
	} b;
	u32 raw;
};

union vi_sys_clk_ctrl1 {
	struct {
		u32 csi_mac0_src_sel : 2;
		u32 csi_mac1_src_sel : 2;
		u32 csi_mac2_src_sel : 2;
		u32 csi_mac3_src_sel : 2;
		u32 csi_mac4_src_sel : 2;
		u32 csi_mac5_src_sel : 2;
		u32 csi_be_src_sel : 2;
		u32 raw_src_sel : 2;
		u32 isp_src_sel : 2;
		u32 vpss0_src_sel : 2;
		u32 vpss1_src_sel : 2;
		u32 vpss2_src_sel : 2;
		u32 vpss3_src_sel : 2;
		u32 ldc0_src_sel : 2;
	} b;
	u32 raw;
};

union vi_sys_clk_ctrl2 {
	struct {
		u32 ldc1_src_sel : 2;
		u32 dpu_src_sel : 2;
		u32 stitching_src_sel : 2;
		u32 ive0_src_sel : 2;
		u32 ive1_src_sel : 2;
		u32 dwa0_src_sel : 2;
		u32 dwa1_src_sel : 2;
	} b;
	u32 raw;
};

enum vi_sys_axi_bus {
	VI_SYS_AXI_BUS_VPSS0 = 0,
	VI_SYS_AXI_BUS_VPSS1,
	VI_SYS_AXI_BUS_VPSS2,
	VI_SYS_AXI_BUS_VPSS3,
	VI_SYS_AXI_BUS_ISP_RAW,
	VI_SYS_AXI_BUS_ISP_YUV,
	VI_SYS_AXI_BUS_MAX,
};

/********************************************************************
 *   APIs to replace bmtest's standard APIs
 ********************************************************************/
int vi_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb);
int vi_sys_cmm_cb_i2c(unsigned int cmd, void *arg);
int vi_sys_cmm_cb_ssp(unsigned int cmd, void *arg);

void vi_sys_set_base_addr(void *base);
union vi_sys_clk_lp0 vi_sys_get_clk_lp0(void);
void vi_sys_set_clk_lp0(union vi_sys_clk_lp0 clk);
union vi_sys_clk_lp1 vi_sys_get_clk_lp1(void);
void vi_sys_set_clk_lp1(union vi_sys_clk_lp1 clk);
union vi_sys_clk_lp2 vi_sys_get_clk_lp2(void);
void vi_sys_set_clk_lp2(union vi_sys_clk_lp2 clk);
union vi_sys_clk_ctrl0 vi_sys_get_clk_ctrl0(void);
void vi_sys_set_clk_ctrl0(union vi_sys_clk_ctrl0 cfg);
union vi_sys_clk_ctrl1 vi_sys_get_clk_ctrl1(void);
void vi_sys_set_clk_ctrl1(union vi_sys_clk_ctrl1 cfg);
union vi_sys_clk_ctrl2 vi_sys_get_clk_ctrl2(void);
void vi_sys_set_clk_ctrl2(union vi_sys_clk_ctrl2 cfg);
union vi_sys_reset vi_sys_get_reset(void);
void vi_sys_set_reset(union vi_sys_reset reset);
void vi_sys_toggle_reset(union vi_sys_reset mask);
union vi_sys_reset_apb vi_sys_get_reset_apb(void);
void vi_sys_set_reset_apb(union vi_sys_reset_apb reset);
void vi_sys_toggle_reset_apb(union vi_sys_reset_apb mask);
union vi_sys_intr vi_sys_get_intr_status(void);
void vi_set_intr_mask(union vi_sys_intr mask);
unsigned int vi_sys_reg_read(uintptr_t addr);
void vi_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data);
void vi_sys_set_offline(enum vi_sys_axi_bus bus, bool offline);


#ifdef __cplusplus
}
#endif

#endif //__COMMON_VIP_H__
