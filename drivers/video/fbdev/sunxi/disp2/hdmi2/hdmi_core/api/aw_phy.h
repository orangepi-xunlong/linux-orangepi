/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef AW_PHY_H_
#define AW_PHY_H_

#include "hdmitx_dev.h"
#include "core/video.h"
#include "core/main_controller.h"
#include "log.h"
#include "access.h"

#define PHY_I2C_SLAVE_ADDR 0x69
#define AW_PHY_TIMEOUT	1000
#define LOCK_TIMEOUT	100
#define PHY_REG_OFFSET	0x10000

/* allwinner phy register offset */
#define HDMI_PHY_CTL0				0x40
#define HDMI_PHY_CTL1				0x44
#define HDMI_PHY_CTL2				0x48
#define HDMI_PHY_CTL3				0x4C
#define HDMI_PHY_CTL4				0x50
#define HDMI_PHY_CTL5				0x54
#define HDMI_PLL_CTL0				0x58
#define HDMI_PLL_CTL1				0x5C
#define HDMI_AFIFO_CFG				0x60
#define HDMI_MODULATOR_CFG0			0x64
#define HDMI_MODULATOR_CFG1			0x68
#define HDMI_PHY_INDEB_CTRL			0x6C
#define HDMI_PHY_INDBG_TXD0			0x70
#define HDMI_PHY_INDBG_TXD1			0x74
#define HDMI_PHY_INDBG_TXD2			0x78
#define HDMI_PHY_INDBG_TXD3			0x7C
#define HDMI_PHY_PLL_STS			0x80
#define HDMI_PRBS_CTL				0x84
#define HDMI_PRBS_SEED_GEN			0x88
#define HDMI_PRBS_SEED_CHK			0x8C
#define HDMI_PRBS_SEED_NUM			0x90
#define HDMI_PRBS_CYCLE_NUM			0x94
#define HDMI_PHY_PLL_ODLY_CFG			0x98
#define HDMI_PHY_CTL6				0x9C
#define HDMI_PHY_CTL7				0xA0

typedef union {
	u32 dwval;
	struct {
		u32 sda_en		:1;    // Default: 0;
		u32 scl_en		:1;    // Default: 0;
		u32 hpd_en		:1;    // Default: 0;
		u32 res0		:1;    // Default: 0;
		u32 reg_ck_sel		:1;    // Default: 1;
		u32 reg_ck_test_sel	:1;    // Default: 1;
		u32 reg_csmps		:2;    // Default: 0;
		u32 reg_den		:4;    // Default: F;
		u32 reg_plr		:4;    // Default: 0;
		u32 enck		:1;    // Default: 1;
		u32 enldo_fs		:1;    // Default: 1;
		u32 enldo		:1;    // Default: 1;
		u32 res1		:1;    // Default: 1;
		u32 enbi		:4;    // Default: F;
		u32 entx		:4;    // Default: F;
		u32 async_fifo_autosync_disable	:1;    // Default: 0;
		u32 async_fifo_workc_enable	:1;    // Default: 1;
		u32 phy_pll_lock_mode		:1;    // Default: 1;
		u32 phy_pll_lock_mode_man	:1;    // Default: 1;
	} bits;
} HDMI_PHY_CTL0_t;      //===========================    0x0040

typedef union {
	u32 dwval;
	struct {
		u32 reg_sp2_0				:   4  ;    // Default: 0;
		u32 reg_sp2_1				:   4  ;    // Default: 0;
		u32 reg_sp2_2				:   4  ;    // Default: 0;
		u32 reg_sp2_3				:   4  ;    // Default: 0;
		u32 reg_bst0				:   2  ;    // Default: 3;
		u32 reg_bst1				:   2  ;    // Default: 3;
		u32 reg_bst2				:   2  ;    // Default: 3;
		u32 res0				:   2  ;    // Default: 0;
		u32 reg_svr				:   2  ;    // Default: 2;
		u32 reg_swi				:   1  ;    // Default: 0;
		u32 res_scktmds				:   1  ;    // Default: 0;
		u32 res_res_s				:   2  ;    // Default: 3;
		u32 phy_rxsense_mode			:   1  ;    // Default: 0;
		u32 res_rxsense_mode_man		:   1  ;    // Default: 0;
	} bits;
} HDMI_PHY_CTL1_t;      //=====================================================    0x0044

typedef union {
	u32 dwval;
	struct {
		u32 reg_p2opt				:   4  ;    // Default: 0;
		u32 reg_sp1_0				:   5  ;    // Default: 0;
		u32 reg_sp1_1				:   5  ;    // Default: 0;
		u32 reg_sp1_2				:   5  ;    // Default: 0;
		u32 reg_sp1_3				:   5  ;    // Default: 0;
		u32 reg_resdi				:   6  ;    // Default: 18;
		u32 phy_hpdo_mode			:   1  ;    // Default: 0;
		u32 phy_hpdo_mode_man			:   1  ;    // Default: 0;
	} bits;
} HDMI_PHY_CTL2_t;      //=====================================================    0x0048



typedef union {
	u32 dwval;
	struct {
		u32 reg_mc0				:   4  ;    // Default: F;
		u32 reg_mc1				:   4  ;    // Default: F;
		u32 reg_mc2				:   4  ;    // Default: F;
		u32 reg_mc3				:   4  ;    // Default: F;
		u32 reg_p2_0				:   4  ;    // Default: F;
		u32 reg_p2_1				:   4  ;    // Default: F;
		u32 reg_p2_2				:   4  ;    // Default: F;
		u32 reg_p2_3				:   4  ;    // Default: F;
	} bits;
} HDMI_PHY_CTL3_t;      //=====================================================    0x004C



typedef union {
	u32 dwval;
	struct {
		u32 reg_p1_0				:   5  ;    // Default: 0x10;
		u32 res0				:   3  ;    // Default: 0;
		u32 reg_p1_1				:   5  ;    // Default: 0x10;
		u32 res1				:   3  ;    // Default: 0;
		u32 reg_p1_2				:   5  ;    // Default: 0x10;
		u32 res2				:   3  ;    // Default: 0;
		u32 reg_p1_3				:   5  ;    // Default: 0x10;
		u32 reg_slv				:   3  ;    // Default: 0;
	} bits;
} HDMI_PHY_CTL4_t;      //=====================================================    0x0050

typedef union {
	u32 dwval;
	struct {
		u32 encalog				:   1  ;    // Default: 0x1;
		u32 enib				:   1  ;    // Default: 0x1;
		u32 res0				:   2  ;    // Default: 0;
		u32 enp2s				:   4  ;    // Default: 0xF;
		u32 enrcal				:   1  ;    // Default: 0x1;
		u32 enres				:   1  ;    // Default: 1;
		u32 enresck				:   1  ;    // Default: 1;
		u32 reg_calsw				:   1  ;    // Default: 0;
		u32 reg_ckpdlyopt			:   1  ;    // Default: 0;
		u32 res1				:   3  ;    // Default: 0;
		u32 reg_p1opt				:   4  ;    // Default: 0;
		u32 res2				:  12  ;    // Default: 0;
	} bits;
} HDMI_PHY_CTL5_t;      //=====================================================    0x0054

typedef union {
	u32 dwval;
	struct {
		u32 prop_cntrl				:   3  ;    // Default: 0x7;
		u32 res0				:   1  ;    // Default: 0;
		u32 gmp_cntrl				:   2  ;    // Default: 1;
		u32 n_cntrl				:   2  ;    // Default: 0;
		u32 vcorange				:   1  ;    // Default: 0;
		u32 sdrven				:   1  ;    // Default: 0;
		u32 divx1				:   1  ;    // Default: 0;
		u32 res1				:   1  ;    // Default: 0;
		u32 div_pre				:   4  ;    // Default: 0;
		u32 div2_cktmds				:   1  ;    // Default: 1;
		u32 div2_ckbit				:   1  ;    // Default: 1;
		u32 cutfb				:   1  ;    // Default: 0;
		u32 res2				:   1  ;    // Default: 0;
		u32 clr_dpth				:   2  ;    // Default: 0;
		u32 bypass_clrdpth			:   1  ;    // Default: 0;
		u32 bcr					:   1  ;    // Default: 0;
		u32 slv					:   3  ;    // Default: 4;
		u32 res3				:   1  ;    // Default: 0;
		u32 envbs				:   1  ;    // Default: 0;
		u32 bypass_ppll				:   1  ;    // Default: 0;
		u32 cko_sel				:   2  ;    // Default: 0;
	} bits;
} HDMI_PLL_CTL0_t;      //=====================================================    0x0058



typedef union {
	u32 dwval;
	struct {
		u32 int_cntrl				:   3  ;    // Default: 0x0;
		u32 res0				:   1  ;    // Default: 0;
		u32 ref_cntrl				:   2  ;    // Default: 3;
		u32 gear_shift				:   1  ;    // Default: 0;
		u32 fast_tech				:   1  ;    // Default: 0;
		u32 drv_ana				:   1  ;    // Default: 1;
		u32 sckfb				:   1  ;    // Default: 0;
		u32 sckref				:   1  ;    // Default: 0;
		u32 reset				:   1  ;    // Default: 0;
		u32 pwron				:   1  ;    // Default: 0;
		u32 res1				:   3  ;    // Default: 0;
		u32 pixel_rep				:   2  ;    // Default: 0;
		u32 sdm_en				:   1  ;    // Default: 0;
		u32 pcnt_en				:   1  ;    // Default: 0;
		u32 pcnt_n				:   8  ;    // Default: 0xE;
		u32 res2				:   3  ;    // Default: 0;
		u32 ctrl_modle_clksrc			:   1  ;    // Default: 0;
	} bits;
} HDMI_PLL_CTL1_t;      //=====================================================    0x005C

typedef union {
	u32 dwval;
	struct {
		u32 hdmi_afifo_error	:   1  ;    // Default: 0x0;
		u32 hdmi_afifo_error_det	:   1  ;    // Default: 0x0;
		u32 res0			:  30  ;    // Default: 0;
	} bits;
} HDMI_AFIFO_CFG_t;      //=====================================================    0x0060

typedef union {
	u32 dwval;
	struct {
		u32 fnpll_mash_en		:   1  ;    // Default: 0x0;
		u32 fnpll_mash_mod		:   2  ;    // Default: 0x0;
		u32 fnpll_mash_stp		:   9  ;    // Default: 0x0;
		u32 fnpll_mash_m12		:   1  ;    // Default: 0x0;
		u32 fnpll_mash_frq		:   2  ;    // Default: 0x0;
		u32 fnpll_mash_bot		:  17  ;    // Default: 0x0;
	} bits;
} HDMI_MODULATOR_CFG0_t;      //=====================================================    0x0064

typedef union {
	u32 dwval;
	struct {
		u32 fnpll_mash_dth		:   1  ;    // Default: 0x0;
		u32 fnpll_mash_fen		:   1  ;    // Default: 0x0;
		u32 fnpll_mash_frc		:  17  ;    // Default: 0x0;
		u32 fnpll_mash_fnv		:   8  ;    // Default: 0x0;
		u32 res0			:   5  ;    // Default: 0x0;
	} bits;
} HDMI_MODULATOR_CFG1_t;      //=====================================================    0x0068

typedef union {
	u32 dwval;
	struct {
		u32 txdata_debugmode		:   2  ;    // Default: 0x0;
		u32 res0			:  14  ;    // Default: 0x0;
		u32 ceci_debug			:   1  ;    // Default: 0x0;
		u32 ceci_debugmode		:   1  ;    // Default: 0x0;
		u32 res1			:   2  ;    // Default: 0x0;
		u32 sdai_debug			:   1  ;    // Default: 0x0;
		u32 sdai_debugmode		:   1  ;    // Default: 0x0;
		u32 res2			:   2  ;    // Default: 0x0;
		u32 scli_debug			:   1  ;    // Default: 0x0;
		u32 scli_debugmode		:   1  ;    // Default: 0x0;
		u32 res3			:   2  ;    // Default: 0x0;
		u32 hpdi_debug			:   1  ;    // Default: 0x0;
		u32 hpdi_debugmode		:   1  ;    // Default: 0x0;
		u32 res4			:   2  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_INDBG_CTRL_t;      //==================================================    0x006C

typedef union {
	u32 dwval;
	struct {
		u32 txdata0_debug_data	    :  32  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_INDBG_TXD0_t;      //==================================================    0x0070

typedef union {
	u32 dwval;
	struct {
		u32 txdata1_debug_data	    :  32  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_INDBG_TXD1_t;      //==================================================    0x0074

typedef union {
	u32 dwval;
	struct {
		u32 txdata2_debug_data	    :  32  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_INDBG_TXD2_t;      //==================================================    0x0078

typedef union {
	u32 dwval;
	struct {
		u32 txdata3_debug_data	    :  32  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_INDBG_TXD3_t;      //==================================================    0x007C

typedef union {
	u32 dwval;
	struct {
		u32 tx_ready_dly_status	:   1  ;    // Default: 0x0;
		u32 rxsense_dly_status		:   1  ;    // Default: 0x0;
		u32 res0			:   2  ;    // Default: 0x0;
		u32 pll_lock_status		:   1  ;    // Default: 0x0;
		u32 res1			:   3  ;    // Default: 0x0;
		u32 phy_resdo2d_status		:   6  ;    // Default: 0x0;
		u32 res2			:   2  ;    // Default: 0x0;
		u32 phy_rcalend2d_status	:   1  ;    // Default: 0x0;
		u32 phy_cout2d_status		:   1  ;    // Default: 0x0;
		u32 res3			:   2  ;    // Default: 0x0;
		u32 phy_ceco_status		:   1  ;    // Default: 0x0;
		u32 phy_sdao_status		:   1  ;    // Default: 0x0;
		u32 phy_sclo_status		:   1  ;    // Default: 0x0;
		u32 phy_hpdo_status		:   1  ;    // Default: 0x0;
		u32 phy_cdetn_status		:   3  ;    // Default: 0x0;
		u32 phy_cdetnck_status		:   1  ;    // Default: 0x0;
		u32 phy_cdetp_status		:   3  ;    // Default: 0x0;
		u32 phy_cdetpck_status		:   1  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_PLL_STS_t;      //=====================================================    0x0080

typedef union {
	u32 dwval;
	struct {
		u32 prbs_en				:   1  ;    // Default: 0x0;
		u32 prbs_start				:   1  ;    // Default: 0x0;
		u32 prbs_seq_gen			:   1  ;    // Default: 0x0;
		u32 prbs_seq_chk			:   1  ;    // Default: 0x0;
		u32 prbs_mode				:   4  ;    // Default: 0x0;
		u32 prbs_type				:   2  ;    // Default: 0x0;
		u32 prbs_clk_pol			:   1  ;    // Default: 0x0;
		u32 res0				:  21  ;    // Default: 0x0;
	} bits;
} HDMI_PRBS_CTL_t;      //=====================================================    0x0084

typedef union {
	u32 dwval;
	struct {
		u32 prbs_seed_gen			:  32  ;    // Default: 0x0;
	} bits;
} HDMI_PRBS_SEED_GEN_t;      //=================================================    0x0088

typedef union {
	u32 dwval;
	struct {
		u32 prbs_seed_chk			:  32  ;    // Default: 0x0;
	} bits;
} HDMI_PRBS_SEED_CHK_t;      //=================================================    0x008C

typedef union {
	u32 dwval;
	struct {
		u32 prbs_seed_num			:  32  ;    // Default: 0x0;
	} bits;
} HDMI_PRBS_SEED_NUM_t;      //=================================================    0x0090

typedef union {
	u32 dwval;
	struct {
		u32 prbs_cycle_num			:  32  ;    // Default: 0x0;
	} bits;
} HDMI_PRBS_CYCLE_NUM_t;      //=================================================    0x0094

typedef union {
	u32 dwval;
	struct {
		u32	tx_ready_dly_count		:  15  ;    // Default: 0x0;
		u32	tx_ready_dly_reset		:   1  ;    // Default: 0x0;
		u32	rxsense_dly_count		:  15  ;    // Default: 0x0;
		u32	rxsense_dly_reset		:   1  ;    // Default: 0x0;
	} bits;
} HDMI_PHY_PLL_ODLY_CFG_t;      //=================================================    0x0098

typedef union {
	u32 dwval;
	struct {
		u32	clk_greate0_340m		:  10  ;    // Default: 0x3FF;
		u32	clk_greate1_340m		:  10  ;    // Default: 0x3FF;
		u32	clk_greate2_340m		:  10  ;    // Default: 0x3FF;
		u32	en_ckdat				:   1  ;    // Default: 0x3FF;
		u32	switch_clkch_data_corresponding	:   1  ;    // Default: 0x3FF;
	} bits;
} HDMI_PHY_CTL6_t;      //=========================================================    0x009C

typedef union {
	u32 dwval;
	struct {
		u32	clk_greate3_340m		:  10  ;    // Default: 0x0;
		u32	res0				:   2  ;    // Default: 0x3FF;
		u32	clk_low_340m			:  10  ;    // Default: 0x3FF;
		u32	res1				:  10  ;    // Default: 0x3FF;
	} bits;
} HDMI_PHY_CTL7_t;      //=========================================================    0x00A0

struct __aw_phy_reg_t {
	u32 res[16];		/* 0x0 ~ 0x3c */
	HDMI_PHY_CTL0_t		phy_ctl0; /* 0x0040 */
	HDMI_PHY_CTL1_t		phy_ctl1; /* 0x0044 */
	HDMI_PHY_CTL2_t		phy_ctl2; /* 0x0048 */
	HDMI_PHY_CTL3_t		phy_ctl3; /* 0x004c */
	HDMI_PHY_CTL4_t		phy_ctl4; /* 0x0050 */
	HDMI_PHY_CTL5_t		phy_ctl5; /* 0x0054 */
	HDMI_PLL_CTL0_t		pll_ctl0; /* 0x0058 */
	HDMI_PLL_CTL1_t		pll_ctl1; /* 0x005c */
	HDMI_AFIFO_CFG_t	afifo_cfg; /* 0x0060 */
	HDMI_MODULATOR_CFG0_t	modulator_cfg0; /* 0x0064 */
	HDMI_MODULATOR_CFG1_t	modulator_cfg1; /* 0x0068 */
	HDMI_PHY_INDBG_CTRL_t	phy_indbg_ctrl;	/* 0x006c */
	HDMI_PHY_INDBG_TXD0_t	phy_indbg_txd0; /* 0x0070 */
	HDMI_PHY_INDBG_TXD1_t	phy_indbg_txd1; /* 0x0074 */
	HDMI_PHY_INDBG_TXD2_t	phy_indbg_txd2; /* 0x0078 */
	HDMI_PHY_INDBG_TXD3_t	phy_indbg_txd3; /* 0x007c */
	HDMI_PHY_PLL_STS_t	phy_pll_sts; /* 0x0080 */
	HDMI_PRBS_CTL_t		prbs_ctl;	/* 0x0084 */
	HDMI_PRBS_SEED_GEN_t	prbs_seed_gen;	/* 0x0088 */
	HDMI_PRBS_SEED_CHK_t	prbs_seed_chk;	/* 0x008c */
	HDMI_PRBS_SEED_NUM_t	prbs_seed_num;	/* 0x0090 */
	HDMI_PRBS_CYCLE_NUM_t	prbs_cycle_num;	/* 0x0094 */
	HDMI_PHY_PLL_ODLY_CFG_t	phy_pll_odly_cfg; /* 0x0098 */
	HDMI_PHY_CTL6_t		phy_ctl6;	/* 0x009c */
	HDMI_PHY_CTL7_t		phy_ctl7;	/* 0x00A0 */
};

/*****************************************************************************
 *                                                                           *
 *                        PHY Configuration Registers                        *
 *                                                                           *
 *****************************************************************************/

/* PHY Configuration Register This register holds the power down, data enable polarity, and interface control of the HDMI Source PHY control */
#define PHY_CONF0  0x0000C000
#define PHY_CONF0_SELDIPIF_MASK  0x00000001 /* Select interface control */
#define PHY_CONF0_SELDATAENPOL_MASK  0x00000002 /* Select data enable polarity */
#define PHY_CONF0_ENHPDRXSENSE_MASK  0x00000004 /* PHY ENHPDRXSENSE signal */
#define PHY_CONF0_TXPWRON_MASK  0x00000008 /* PHY TXPWRON signal */
#define PHY_CONF0_PDDQ_MASK  0x00000010 /* PHY PDDQ signal */
#define PHY_CONF0_SVSRET_MASK  0x00000020 /* Reserved as "spare" register with no associated functionality */
#define PHY_CONF0_SPARES_1_MASK  0x00000040 /* Reserved as "spare" register with no associated functionality */
#define PHY_CONF0_SPARES_2_MASK  0x00000080 /* Reserved as "spare" register with no associated functionality */

/* PHY Test Interface Register 0 PHY TX mapped test interface (control) */
#define PHY_TST0  0x0000C004
#define PHY_TST0_SPARE_0_MASK  0x00000001 /* Reserved as "spare" register with no associated functionality */
#define PHY_TST0_SPARE_1_MASK  0x0000000E /* Reserved as "spare" bit with no associated functionality */
#define PHY_TST0_SPARE_3_MASK  0x00000010 /* Reserved as "spare" register with no associated functionality */
#define PHY_TST0_SPARE_4_MASK  0x00000020 /* Reserved as "spare" register with no associated functionality */
#define PHY_TST0_SPARE_2_MASK  0x000000C0 /* Reserved as "spare" bit with no associated functionality */

/* PHY Test Interface Register 1 PHY TX mapped text interface (data in) */
#define PHY_TST1  0x0000C008
#define PHY_TST1_SPARE_MASK  0x000000FF /* Reserved as "spare" register with no associated functionality */

/* PHY Test Interface Register 2 PHY TX mapped text interface (data out) */
#define PHY_TST2  0x0000C00C
#define PHY_TST2_SPARE_MASK  0x000000FF /* Reserved as "spare" register with no associated functionality */

/* PHY RXSENSE, PLL Lock, and HPD Status Register This register contains the following active high packet sent status indications */
#define PHY_STAT0  0x0000C010
#define PHY_STAT0_TX_PHY_LOCK_MASK  0x00000001 /* Status bit */
#define PHY_STAT0_HPD_MASK  0x00000002 /* Status bit */
#define PHY_STAT0_RX_SENSE_0_MASK  0x00000010 /* Status bit */
#define PHY_STAT0_RX_SENSE_1_MASK  0x00000020 /* Status bit */
#define PHY_STAT0_RX_SENSE_2_MASK  0x00000040 /* Status bit */
#define PHY_STAT0_RX_SENSE_3_MASK  0x00000080 /* Status bit */

/* PHY RXSENSE, PLL Lock, and HPD Interrupt Register This register contains the interrupt indication of the PHY_STAT0 status interrupts */
#define PHY_INT0  0x0000C014
#define PHY_INT0_TX_PHY_LOCK_MASK  0x00000001 /* Interrupt indication bit */
#define PHY_INT0_HPD_MASK  0x00000002 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_0_MASK  0x00000010 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_1_MASK  0x00000020 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_2_MASK  0x00000040 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_3_MASK  0x00000080 /* Interrupt indication bit */

/* PHY RXSENSE, PLL Lock, and HPD Mask Register Mask register for generation of PHY_INT0 interrupts */
#define PHY_MASK0  0x0000C018
#define PHY_MASK0_TX_PHY_LOCK_MASK  0x00000001 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_HPD_MASK  0x00000002 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_0_MASK  0x00000010 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_1_MASK  0x00000020 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_2_MASK  0x00000040 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_3_MASK  0x00000080 /* Mask bit for PHY_INT0 */

/* PHY RXSENSE, PLL Lock, and HPD Polarity Register Polarity register for generation of PHY_INT0 interrupts */
#define PHY_POL0  0x0000C01C
#define PHY_POL0_TX_PHY_LOCK_MASK  0x00000001 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_HPD_MASK  0x00000002 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_0_MASK  0x00000010 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_1_MASK  0x00000020 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_2_MASK  0x00000040 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_3_MASK  0x00000080 /* Polarity bit for PHY_INT0 */

/* PHY I2C Slave Address Configuration Register */
#define PHY_I2CM_SLAVE  0x0000C080
#define PHY_I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F /* Slave address to be sent during read and write operations */

/* PHY I2C Address Configuration Register This register writes the address for read and write operations */
#define PHY_I2CM_ADDRESS  0x0000C084
#define PHY_I2CM_ADDRESS_ADDRESS_MASK  0x000000FF /* Register address for read and write operations */

/* PHY I2C Data Write Register 1 */
#define PHY_I2CM_DATAO_1  0x0000C088
#define PHY_I2CM_DATAO_1_DATAO_MASK  0x000000FF /* Data MSB (datao[15:8]) to be written on register pointed by phy_i2cm_address [7:0] */

/* PHY I2C Data Write Register 0 */
#define PHY_I2CM_DATAO_0  0x0000C08C
#define PHY_I2CM_DATAO_0_DATAO_MASK  0x000000FF /* Data LSB (datao[7:0]) to be written on register pointed by phy_i2cm_address [7:0] */

/* PHY I2C Data Read Register 1 */
#define PHY_I2CM_DATAI_1  0x0000C090
#define PHY_I2CM_DATAI_1_DATAI_MASK  0x000000FF /* Data MSB (datai[15:8]) read from register pointed by phy_i2cm_address[7:0] */

/* PHY I2C Data Read Register 0 */
#define PHY_I2CM_DATAI_0  0x0000C094
#define PHY_I2CM_DATAI_0_DATAI_MASK  0x000000FF /* Data LSB (datai[7:0]) read from register pointed by phy_i2cm_address[7:0] */

/* PHY I2C RD/RD_EXT/WR Operation Register This register requests read and write operations from the I2C Master PHY */
#define PHY_I2CM_OPERATION  0x0000C098
#define PHY_I2CM_OPERATION_RD_MASK  0x00000001 /* Read operation request */
#define PHY_I2CM_OPERATION_WR_MASK  0x00000010 /* Write operation request */

/* PHY I2C Done Interrupt Register This register contains and configures I2C master PHY done interrupt */
#define PHY_I2CM_INT  0x0000C09C
#define PHY_I2CM_INT_DONE_STATUS_MASK  0x00000001 /* Operation done status bit */
#define PHY_I2CM_INT_DONE_INTERRUPT_MASK  0x00000002 /* Operation done interrupt bit */
#define PHY_I2CM_INT_DONE_MASK_MASK  0x00000004 /* Done interrupt mask signal */
#define PHY_I2CM_INT_DONE_POL_MASK  0x00000008 /* Done interrupt polarity configuration */

/* PHY I2C error Interrupt Register This register contains and configures the I2C master PHY error interrupts */
#define PHY_I2CM_CTLINT  0x0000C0A0
#define PHY_I2CM_CTLINT_ARBITRATION_STATUS_MASK  0x00000001 /* Arbitration error status bit */
#define PHY_I2CM_CTLINT_ARBITRATION_INTERRUPT_MASK  0x00000002 /* Arbitration error interrupt bit {arbitration_interrupt = (arbitration_mask==0b) && (arbitration_status==arbitration_pol)} Note: This bit field is read by the sticky bits present on the ih_i2cmphy_stat0 register */
#define PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK  0x00000004 /* Arbitration error interrupt mask signal */
#define PHY_I2CM_CTLINT_ARBITRATION_POL_MASK  0x00000008 /* Arbitration error interrupt polarity configuration */
#define PHY_I2CM_CTLINT_NACK_STATUS_MASK  0x00000010 /* Not acknowledge error status bit */
#define PHY_I2CM_CTLINT_NACK_INTERRUPT_MASK  0x00000020 /* Not acknowledge error interrupt bit */
#define PHY_I2CM_CTLINT_NACK_MASK_MASK  0x00000040 /* Not acknowledge error interrupt mask signal */
#define PHY_I2CM_CTLINT_NACK_POL_MASK  0x00000080 /* Not acknowledge error interrupt polarity configuration */

/* PHY I2C Speed control Register This register wets the I2C Master PHY to work in either Fast or Standard mode */
#define PHY_I2CM_DIV  0x0000C0A4
#define PHY_I2CM_DIV_SPARE_MASK  0x00000007 /* Reserved as "spare" register with no associated functionality */
#define PHY_I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 /* Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode */

/* PHY I2C SW reset control register This register sets the I2C Master PHY software reset */
#define PHY_I2CM_SOFTRSTZ  0x0000C0A8
#define PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 /* I2C Master Software Reset */

/* PHY I2C Slow Speed SCL High Level Control Register 1 */
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR  0x0000C0AC
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK  0x000000FF /* PHY I2C Slow Speed SCL High Level Control Register 1 */

/* PHY I2C Slow Speed SCL High Level Control Register 0 */
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR  0x0000C0B0
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK  0x000000FF /* PHY I2C Slow Speed SCL High Level Control Register 0 */

/* PHY I2C Slow Speed SCL Low Level Control Register 1 */
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR  0x0000C0B4
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK  0x000000FF /* PHY I2C Slow Speed SCL Low Level Control Register 1 */

/* PHY I2C Slow Speed SCL Low Level Control Register 0 */
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR  0x0000C0B8
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK  0x000000FF /* PHY I2C Slow Speed SCL Low Level Control Register 0 */

/* PHY I2C Fast Speed SCL High Level Control Register 1 */
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR  0x0000C0BC
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK  0x000000FF /* PHY I2C Fast Speed SCL High Level Control Register 1 */

/* PHY I2C Fast Speed SCL High Level Control Register 0 */
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR  0x0000C0C0
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK  0x000000FF /* PHY I2C Fast Speed SCL High Level Control Register 0 */

/* PHY I2C Fast Speed SCL Low Level Control Register 1 */
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR  0x0000C0C4
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK  0x000000FF /* PHY I2C Fast Speed SCL Low Level Control Register 1 */

/* PHY I2C Fast Speed SCL Low Level Control Register 0 */
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR  0x0000C0C8
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK  0x000000FF /* PHY I2C Fast Speed SCL Low Level Control Register 0 */

/* PHY I2C SDA HOLD Control Register */
#define PHY_I2CM_SDA_HOLD  0x0000C0CC
#define PHY_I2CM_SDA_HOLD_OSDA_HOLD_MASK  0x000000FF /* Defines the number of SFR clock cycles to meet tHD:DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1/isfrclk_frequency)) */

/* PHY I2C/JTAG I/O Configuration Control Register */
#define JTAG_PHY_CONFIG  0x0000C0D0
#define JTAG_PHY_CONFIG_JTAG_TRST_N_MASK  0x00000001 /* Configures the JTAG PHY interface output pin JTAG_TRST_N when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_trst_n when PHY_EXTERNAL=1 */
#define JTAG_PHY_CONFIG_I2C_JTAGZ_MASK  0x00000010 /* Configures the JTAG PHY interface output pin I2C_JTAGZ to select the PHY configuration interface when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_i2c_jtagz when PHY_EXTERNAL=1 */

/* PHY JTAG Clock Control Register */
#define JTAG_PHY_TAP_TCK  0x0000C0D4
#define JTAG_PHY_TAP_TCK_JTAG_TCK_MASK  0x00000001 /* Configures the JTAG PHY interface pin JTAG_TCK when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tck when PHY_EXTERNAL=1 */

/* PHY JTAG TAP In Control Register */
#define JTAG_PHY_TAP_IN  0x0000C0D8
#define JTAG_PHY_TAP_IN_JTAG_TDI_MASK  0x00000001 /* Configures the JTAG PHY interface pin JTAG_TDI when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tdi when PHY_EXTERNAL=1 */
#define JTAG_PHY_TAP_IN_JTAG_TMS_MASK  0x00000010 /* Configures the JTAG PHY interface pin JTAG_TMS when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tms when PHY_EXTERNAL=1 */

/* PHY JTAG TAP Out Control Register */
#define JTAG_PHY_TAP_OUT  0x0000C0DC
#define JTAG_PHY_TAP_OUT_JTAG_TDO_MASK  0x00000001 /* Read JTAG PHY interface input pin JTAG_TDO when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_tdo when PHY_EXTERNAL=1 */
#define JTAG_PHY_TAP_OUT_JTAG_TDO_EN_MASK  0x00000010 /* Read JTAG PHY interface input pin JTAG_TDO_EN when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_tdo_en when PHY_EXTERNAL=1 */

/* PHY JTAG Address Control Register */
#define JTAG_PHY_ADDR  0x0000C0E0
#define JTAG_PHY_ADDR_JTAG_ADDR_MASK  0x000000FF /* Configures the JTAG PHY interface pin JTAG_ADDR[7:0] when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_addr[7:0] when PHY_EXTERNAL=1 */


struct aw_phy_config {
	u32			clock;/*phy clock: unit:kHZ*/
	pixel_repetition_t	pixel;
	color_depth_t		color;
	u32			phy_ctl1;
	u32			phy_ctl2;
	u32			phy_ctl3;
	u32			phy_ctl4;
	u32			pll_ctl0;
	u32			pll_ctl1;
};

/**
 * Initialize PHY and put into a known state
 * @param dev device structure
 * @param phy_model
 * return always TRUE
 */
int phy_initialize(hdmi_tx_dev_t *dev, u16 phy_model);

/**
 * Bring up PHY and start sending media for a specified pixel clock, pixel
 * repetition and color resolution (to calculate TMDS) - this fields must
 * be configured in the dev->snps_hdmi_ctrl variables
 * @param dev device structure
 * return TRUE if success, FALSE if not success and -1 if PHY configurations
 * are not supported.
 */
int phy_configure(hdmi_tx_dev_t *dev, u16 phy_model, encoding_t EncodingOut);

/**
 * Set PHY to standby mode - turn off all interrupts
 * @param dev device structure
 */
int phy_standby(hdmi_tx_dev_t *dev);

/**
 * Enable HPD sensing circuitry
 * @param dev device structure
 */
int phy_enable_hpd_sense(hdmi_tx_dev_t *dev);

/**
 * Disable HPD sensing circuitry
 * @param dev device structure
 */
int phy_disable_hpd_sense(hdmi_tx_dev_t *dev);

void aw_phy_read(u8 offset, u32 *value);
void aw_phy_write(u8 offset, u32 value);

u8 phy_hot_plug_state(hdmi_tx_dev_t *dev);

void phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit);
void phy_i2c_master_reset(hdmi_tx_dev_t *dev);

u8 phy_rxsense_state(hdmi_tx_dev_t *dev);
u8 phy_pll_lock_state(hdmi_tx_dev_t *dev);
u8 phy_power_state(hdmi_tx_dev_t *dev);

void phy_power_enable(hdmi_tx_dev_t *dev, u8 enable);
void phy_set_reg_base(uintptr_t base);
uintptr_t phy_get_reg_base(void);
void phy_reset(void);
int phy_config_resume(void);


#endif	/* AW_PHY_H_ */
