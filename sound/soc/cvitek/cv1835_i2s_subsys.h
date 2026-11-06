/* SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _CV1835_I2S_SUBSYS_H_
#define _CV1835_I2S_SUBSYS_H_

//dw_i2s
#define DWI2S_MODE_REG_VAL 0xf0
#define DWI2S_CLK_CTRL0_VAL 0x140
#define DWI2S_SLAVEMODE_SOURCE 0x0

#define SUBSYS_I2S0     (0x1 << 0)
#define SUBSYS_I2S1     (0x1 << 1)
#define SUBSYS_I2S2     (0x1 << 2)
#define SUBSYS_I2S3     (0x1 << 3)

#define SCLK_IN_SEL				0x00
#define FS_IN_SEL				0x04
#define SDI_IN_SEL				0x08
#define SDO_OUT_SEL				0x0C
#define MULTI_SYNC				0x20
#define BCLK_OEN_SEL			0x30
#define BCLK_OUT_CTRL			0x34
#define AUDIO_PDM_CTRL			0x40
#define AUDIO_PHY_BYPASS1		0x50
#define AUDIO_PHY_BYPASS2		0x54
#define SYS_CLK_CTRL			0x70
// #define I2S0_MASTER_CLK_CTRL0	0x80
// #define I2S0_MASTER_CLK_CTRL1	0x84
// #define I2S1_MASTER_CLK_CTRL0	0x90
// #define I2S1_MASTER_CLK_CTRL1	0x94
// #define I2S2_MASTER_CLK_CTRL0	0xA0
// #define I2S2_MASTER_CLK_CTRL1	0xA4
// #define I2S3_MASTER_CLK_CTRL0	0xB0
// #define I2S3_MASTER_CLK_CTRL1	0xB4
#define SYS_LRCK_CTRL			0xC0

#define I2S_FRAME_SETTING_REG	0x04
#define I2S_ENABLE_REG			0x18
#define I2S_LCRK_MASTER_REG		0x2C
#define I2S_CLK_CTRL0_REG		0x60
#define I2S_CLK_CTRL1_REG		0x64
#define I2S_RESET_REG			0x1C
#define I2S_TX_STATUS_REG		0x48

#if defined(CONFIG_ARCH_CV186X)
//#define MULTI_SYNC_0				0x20
#define MULTI_SYNC_1				0x24

#define CODEC_SEL_BYPASS			0x58
#define I2S_SYS_INT_EN				0x60
#define I2S_SYS_INTS				0x64

#define I2S0_MASTER_CLK_CTRL0	0x80
#define I2S0_MASTER_CLK_CTRL1	0x84
#define I2S1_MASTER_CLK_CTRL0	0x88
#define I2S1_MASTER_CLK_CTRL1	0x8c
#define I2S2_MASTER_CLK_CTRL0	0x90
#define I2S2_MASTER_CLK_CTRL1	0x94
#define I2S3_MASTER_CLK_CTRL0	0x98
#define I2S3_MASTER_CLK_CTRL1	0x9c
#define I2S4_MASTER_CLK_CTRL0	0xa0
#define I2S4_MASTER_CLK_CTRL1	0xa4
#define I2S5_MASTER_CLK_CTRL0	0xa8
#define I2S5_MASTER_CLK_CTRL1	0xac

#define DW_I2S_MODE_REG			0x0c4
#define DW_I2S_SLAVEMODE_SOURCE	0x0c8
#define DW_I2S_CLK_CTRL0		0x0d0
#define DW_I2S_CLK_CTRL1		0x0d4

#else

#define I2S0_MASTER_CLK_CTRL0	0x80
#define I2S0_MASTER_CLK_CTRL1	0x84
#define I2S1_MASTER_CLK_CTRL0	0x90
#define I2S1_MASTER_CLK_CTRL1	0x94
#define I2S2_MASTER_CLK_CTRL0	0xA0
#define I2S2_MASTER_CLK_CTRL1	0xA4
#define I2S3_MASTER_CLK_CTRL0	0xB0
#define I2S3_MASTER_CLK_CTRL1	0xB4
#endif

#ifdef CONFIG_PM_SLEEP
struct subsys_reg_context {
	u32 sclk_in_sel;
	u32 fs_in_sel;
	u32 sdi_in_sel;
	u32 sdo_out_sel;
	u32 multi_sync;
	u32 bclk_oen_sel;
	u32 pdm_ctrl;
};
#endif

struct cvi_i2s_subsys_dev {
	void __iomem *subsys_base;
	struct device *dev;
	u32 master_id;
	u32 master_base;
#ifdef CONFIG_PM_SLEEP
	struct subsys_reg_context *reg_ctx;
#endif
};

#define	CVI_16384_MHZ	16384000   /* 16.384 Mhz */
#define	CVI_22579_MHZ	22579200   /* 22.5792 Mhz */
#define	CVI_24576_MHZ	24576000   /* 24.576 Mhz */

u32 i2s_subsys_query_master(void);
void i2s_master_clk_switch_on(bool on);
void i2s_set_master_clk(u32 clk_ctrl1);
void i2s_set_master_frame_setting(u32 frame_format);
void cv1835_set_mclk(u32 freq);
void cv182x_reset_dac(void);
void cv182x_reset_adc(void);
void cv182xa_reset_dac(void);
void cv182xa_reset_adc(void);

void dwi2s_get_subsys(void);
void dwi2s_set_mclk(u32 dwi2s_mode, u32 slave_source, u32 ctl0, u32 ctl1);

void aud_clk_enable(void);
void aud_clk_disable(void);
#endif
