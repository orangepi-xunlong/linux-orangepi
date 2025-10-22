/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Based on the tcs4838 driver and the previous tcs4838 driver
 */

#ifndef __LINUX_MFD_BMU_EXT_H
#define __LINUX_MFD_BMU_EXT_H

#include <linux/regmap.h>
#include <power/pmic-debug.h>
#include <linux/thermal.h>
#include <linux/power_supply.h>

enum {
	ETA6973_ID = 0,
	AXP519_ID,
	AXP2601_ID,
	NR_BMU_EXT_VARIANTS,
};

/* List of registers for eta6973 */
#define CHARGsE_STATE_DONE 0x03
/*
* mask usage:
* such as mask 0x80 for 8 bit reg0:1000 0000
* 0x80: 1000 0000
* 0x60: 0110 0000
* 0x1f: 0001 1111
*/

/* Register 00h */
#define ETA6973_REG_00						0x00
#define ETA6973_REG00_ENHIZ_MASK			0x80
#define ETA6973_REG00_ENHIZ_SHIFT			7
#define	ETA6973_REG00_HIZ_ENABLE			1
#define	ETA6973_REG00_HIZ_DISABLE			0

#define	ETA6973_REG00_STAT_CTRL_MASK		0x60
#define ETA6973_REG00_STAT_CTRL_SHIFT		5
#define	ETA6973_REG00_STAT_CTRL_ENABLE		0x00
#define	ETA6973_REG00_STAT_CTRL_DISABLE		0x03

#define ETA6973_REG00_IINLIM_MASK			0x1F
#define ETA6973_REG00_IINLIM_SHIFT			0
#define	ETA6973_REG00_IINLIM_LSB			100
#define	ETA6973_REG00_IINLIM_BASE			100

/* Register 01h */
#define ETA6973_REG_01				0x01
#define ETA6973_REG01_PFM_DISABLE_MASK		0x80
#define	ETA6973_REG01_PFM_DISABLE_SHIFT		7
#define	ETA6973_REG01_PFM_ENABLE			0
#define	ETA6973_REG01_PFM_DISABLE			1

#define ETA6973_REG01_WDT_RESET_MASK		0x40
#define ETA6973_REG01_WDT_RESET_SHIFT		6
#define ETA6973_REG01_WDT_RESET				1

#define	ETA6973_REG01_OTG_CONFIG_MASK		0x20
#define	ETA6973_REG01_OTG_CONFIG_SHIFT		5
#define	ETA6973_REG01_OTG_ENABLE			1
#define	ETA6973_REG01_OTG_DISABLE			0

#define ETA6973_REG01_CHG_CONFIG_MASK		0x10
#define ETA6973_REG01_CHG_CONFIG_SHIFT		4
#define ETA6973_REG01_CHG_DISABLE			0
#define ETA6973_REG01_CHG_ENABLE			1

#define ETA6973_REG01_SYS_MINV_MASK			0x0E
#define ETA6973_REG01_SYS_MINV_SHIFT		1

#define	ETA6973_REG01_MIN_VBAT_SEL_MASK		0x01
#define	ETA6973_REG01_MIN_VBAT_SEL_SHIFT	0
#define	ETA6973_REG01_MIN_VBAT_2P8V			0
#define	ETA6973_REG01_MIN_VBAT_2P5V			1


/* Register 0x02*/
#define ETA6973_REG_02				0x02
#define	ETA6973_REG02_BOOST_LIM_MASK		0x80
#define	ETA6973_REG02_BOOST_LIM_SHIFT		7
#define	ETA6973_REG02_BOOST_LIM_0P5A		0
#define	ETA6973_REG02_BOOST_LIM_1P2A		1

#define	ETA6973_REG02_Q1_FULLON_MASK		0x40
#define	ETA6973_REG02_Q1_FULLON_SHIFT		6
#define	ETA6973_REG02_Q1_FULLON_ENABLE		1
#define	ETA6973_REG02_Q1_FULLON_DISABLE		0

#define ETA6973_REG02_ICHG_MASK				0x3F
#define ETA6973_REG02_ICHG_SHIFT			0
#define ETA6973_REG02_ICHG_BASE				0
#define ETA6973_REG02_ICHG_LSB				60

/* Register 0x03*/
#define ETA6973_REG_03					0x03
#define ETA6973_REG03_IPRECHG_MASK			0xF0
#define ETA6973_REG03_IPRECHG_SHIFT			4
#define ETA6973_REG03_IPRECHG_BASE			60
#define ETA6973_REG03_IPRECHG_LSB			60

#define ETA6973_REG03_ITERM_MASK			0x0F
#define ETA6973_REG03_ITERM_SHIFT			0
#define ETA6973_REG03_ITERM_BASE			60
#define ETA6973_REG03_ITERM_LSB				60


/* Register 0x04*/
#define ETA6973_REG_04				0x04
#define ETA6973_REG04_VREG_MASK				0xF8
#define ETA6973_REG04_VREG_SHIFT			3
#define ETA6973_REG04_VREG_BASE				3856
#define ETA6973_REG04_VREG_LSB				32

#define	ETA6973_REG04_TOPOFF_TIMER_MASK		0x06
#define	ETA6973_REG04_TOPOFF_TIMER_SHIFT	1
#define	ETA6973_REG04_TOPOFF_TIMER_DISABLE	0
#define	ETA6973_REG04_TOPOFF_TIMER_15M		1
#define	ETA6973_REG04_TOPOFF_TIMER_30M		2
#define	ETA6973_REG04_TOPOFF_TIMER_45M		3


#define ETA6973_REG04_VRECHG_MASK			0x01
#define ETA6973_REG04_VRECHG_SHIFT			0
#define ETA6973_REG04_VRECHG_100MV			0
#define ETA6973_REG04_VRECHG_200MV			1

/* Register 0x05*/
#define ETA6973_REG_05				0x05
#define ETA6973_REG05_EN_TERM_MASK			0x80
#define ETA6973_REG05_EN_TERM_SHIFT			7
#define ETA6973_REG05_TERM_ENABLE			1
#define ETA6973_REG05_TERM_DISABLE			0

#define ETA6973_REG05_WDT_MASK				0x30
#define ETA6973_REG05_WDT_SHIFT				4
#define ETA6973_REG05_WDT_DISABLE			0
#define ETA6973_REG05_WDT_40S				1
#define ETA6973_REG05_WDT_80S				2
#define ETA6973_REG05_WDT_160S				3
#define ETA6973_REG05_WDT_BASE				0
#define ETA6973_REG05_WDT_LSB				40

#define ETA6973_REG05_EN_TIMER_MASK			0x08
#define ETA6973_REG05_EN_TIMER_SHIFT		3
#define ETA6973_REG05_CHG_TIMER_ENABLE		1
#define ETA6973_REG05_CHG_TIMER_DISABLE		0

#define ETA6973_REG05_CHG_TIMER_MASK		0x04
#define ETA6973_REG05_CHG_TIMER_SHIFT		2
#define ETA6973_REG05_CHG_TIMER_5HOURS		0
#define ETA6973_REG05_CHG_TIMER_10HOURS		1

#define	ETA6973_REG05_TREG_MASK				0x02
#define	ETA6973_REG05_TREG_SHIFT			1
#define	ETA6973_REG05_TREG_90C				0
#define	ETA6973_REG05_TREG_110C				1

#define ETA6973_REG05_JEITA_ISET_MASK		0x01
#define ETA6973_REG05_JEITA_ISET_SHIFT		0
#define ETA6973_REG05_JEITA_ISET_50PCT		0
#define ETA6973_REG05_JEITA_ISET_20PCT		1


/* Register 0x06*/
#define ETA6973_REG_06				0x06
#define	ETA6973_REG06_OVP_MASK				0xC0
#define	ETA6973_REG06_OVP_SHIFT				0x6
#define	ETA6973_REG06_OVP_5P5V				0
#define	ETA6973_REG06_OVP_6P2V				1
#define	ETA6973_REG06_OVP_10P5V				2
#define	ETA6973_REG06_OVP_14P3V				3

#define	ETA6973_REG06_BOOSTV_MASK			0x30
#define	ETA6973_REG06_BOOSTV_SHIFT			4
#define	ETA6973_REG06_BOOSTV_4P85V			0
#define	ETA6973_REG06_BOOSTV_5V				1
#define	ETA6973_REG06_BOOSTV_5P15V			2
#define	ETA6973_REG06_BOOSTV_5P3V			3

#define	ETA6973_REG06_VINDPM_MASK			0x0F
#define	ETA6973_REG06_VINDPM_SHIFT			0
#define	ETA6973_REG06_VINDPM_BASE			3900
#define	ETA6973_REG06_VINDPM_LSB			100

/* Register 0x07*/
#define ETA6973_REG_07				0x07
#define ETA6973_REG07_FORCE_DPDM_MASK		0x80
#define ETA6973_REG07_FORCE_DPDM_SHIFT		7
#define ETA6973_REG07_FORCE_DPDM			1

#define ETA6973_REG07_TMR2X_EN_MASK			0x40
#define ETA6973_REG07_TMR2X_EN_SHIFT		6
#define ETA6973_REG07_TMR2X_ENABLE			1
#define ETA6973_REG07_TMR2X_DISABLE			0

#define ETA6973_REG07_BATFET_DIS_MASK		0x20
#define ETA6973_REG07_BATFET_DIS_SHIFT		5
#define ETA6973_REG07_BATFET_OFF			1
#define ETA6973_REG07_BATFET_ON				0

#define ETA6973_REG07_JEITA_VSET_MASK		0x10
#define ETA6973_REG07_JEITA_VSET_SHIFT		4
#define ETA6973_REG07_JEITA_VSET_4100		0
#define ETA6973_REG07_JEITA_VSET_VREG		1

#define	ETA6973_REG07_BATFET_DLY_MASK		0x08
#define	ETA6973_REG07_BATFET_DLY_SHIFT		3
#define	ETA6973_REG07_BATFET_DLY_0S			0
#define	ETA6973_REG07_BATFET_DLY_10S		1

#define	ETA6973_REG07_BATFET_RST_EN_MASK	0x04
#define	ETA6973_REG07_BATFET_RST_EN_SHIFT	2
#define	ETA6973_REG07_BATFET_RST_DISABLE	0
#define	ETA6973_REG07_BATFET_RST_ENABLE		1

#define	ETA6973_REG07_VDPM_BAT_TRACK_MASK	0x03
#define	ETA6973_REG07_VDPM_BAT_TRACK_SHIFT	0
#define	ETA6973_REG07_VDPM_BAT_TRACK_DISABLE	0
#define	ETA6973_REG07_VDPM_BAT_TRACK_200MV	1
#define	ETA6973_REG07_VDPM_BAT_TRACK_250MV	2
#define	ETA6973_REG07_VDPM_BAT_TRACK_300MV	3

/* Register 0x08*/
#define ETA6973_REG_08			  0x08
#define ETA6973_REG08_VBUS_STAT_MASK	  0xE0
#define ETA6973_REG08_VBUS_STAT_SHIFT	  5
#define ETA6973_REG08_VBUS_TYPE_NONE	  0
#define ETA6973_REG08_VBUS_TYPE_OTG		  7

/* ETA6973 vbus stat */
#define ETA6973_REG08_VBUS_TYPE_USB		  1
#define ETA6973_REG08_VBUS_TYPE_ADAPTER	  3

/* ETA6974 vbus stat */
#define ETA6974_REG08_VBUS_TYPE_SDP			  1
#define ETA6974_REG08_VBUS_TYPE_CDP			  2
#define ETA6974_REG08_VBUS_TYPE_DCP			  3
#define ETA6974_REG08_VBUS_TYPE_UNKNOW		  5
#define ETA6974_REG08_VBUS_TYPE_NON_STANDER	  6

#define ETA6973_REG08_CHRG_STAT_MASK	  0x18
#define ETA6973_REG08_CHRG_STAT_SHIFT	  3
#define ETA6973_REG08_CHRG_STAT_IDLE	  0
#define ETA6973_REG08_CHRG_STAT_PRECHG	  1
#define ETA6973_REG08_CHRG_STAT_FASTCHG	  2
#define ETA6973_REG08_CHRG_STAT_CHGDONE	  3

#define ETA6973_REG08_PG_STAT_MASK		  0x04
#define ETA6973_REG08_PG_STAT_SHIFT		  2
#define ETA6973_REG08_POWER_GOOD		  1

#define ETA6973_REG08_THERM_STAT_MASK	  0x02
#define ETA6973_REG08_THERM_STAT_SHIFT	  1

#define ETA6973_REG08_VSYS_STAT_MASK	  0x01
#define ETA6973_REG08_VSYS_STAT_SHIFT	  0
#define ETA6973_REG08_IN_VSYS_STAT		  1


/* Register 0x09*/
#define ETA6973_REG_09				0x09
#define ETA6973_REG09_FAULT_WDT_MASK	  0x80
#define ETA6973_REG09_FAULT_WDT_SHIFT	  7
#define ETA6973_REG09_FAULT_WDT			  1

#define ETA6973_REG09_FAULT_BOOST_MASK	  0x40
#define ETA6973_REG09_FAULT_BOOST_SHIFT	  6

#define ETA6973_REG09_FAULT_CHRG_MASK	  0x30
#define ETA6973_REG09_FAULT_CHRG_SHIFT	  4
#define ETA6973_REG09_FAULT_CHRG_NORMAL	  0
#define ETA6973_REG09_FAULT_CHRG_INPUT	  1
#define ETA6973_REG09_FAULT_CHRG_THERMAL  2
#define ETA6973_REG09_FAULT_CHRG_TIMER	  3

#define ETA6973_REG09_FAULT_BAT_MASK	  0x08
#define ETA6973_REG09_FAULT_BAT_SHIFT	  3
#define	ETA6973_REG09_FAULT_BAT_OVP		1

#define ETA6973_REG09_FAULT_NTC_MASK	  0x07
#define ETA6973_REG09_FAULT_NTC_SHIFT	  0
#define	ETA6973_REG09_FAULT_NTC_NORMAL		0
#define ETA6973_REG09_FAULT_NTC_WARM	  2
#define ETA6973_REG09_FAULT_NTC_COOL	  3
#define ETA6973_REG09_FAULT_NTC_COLD	  5
#define ETA6973_REG09_FAULT_NTC_HOT		  6


/* Register 0x0A */
#define ETA6973_REG_0A				0x0A
#define	ETA6973_REG0A_VBUS_GD_MASK			0x80
#define	ETA6973_REG0A_VBUS_GD_SHIFT			7
#define	ETA6973_REG0A_VBUS_GD				1

#define	ETA6973_REG0A_VINDPM_STAT_MASK		0x40
#define	ETA6973_REG0A_VINDPM_STAT_SHIFT		6
#define	ETA6973_REG0A_VINDPM_ACTIVE			1

#define	ETA6973_REG0A_IINDPM_STAT_MASK		0x20
#define	ETA6973_REG0A_IINDPM_STAT_SHIFT		5
#define	ETA6973_REG0A_IINDPM_ACTIVE			1

#define	ETA6973_REG0A_TOPOFF_ACTIVE_MASK	0x08
#define	ETA6973_REG0A_TOPOFF_ACTIVE_SHIFT	3
#define	ETA6973_REG0A_TOPOFF_ACTIVE			1

#define	ETA6973_REG0A_ACOV_STAT_MASK		0x04
#define	ETA6973_REG0A_ACOV_STAT_SHIFT		2
#define	ETA6973_REG0A_ACOV_ACTIVE			1

#define	ETA6973_REG0A_VINDPM_INT_MASK		0x02
#define	ETA6973_REG0A_VINDPM_INT_SHIFT		1
#define	ETA6973_REG0A_VINDPM_INT_ENABLE		0
#define	ETA6973_REG0A_VINDPM_INT_DISABLE	1

#define	ETA6973_REG0A_IINDPM_INT_MASK		0x01
#define	ETA6973_REG0A_IINDPM_INT_SHIFT		0
#define	ETA6973_REG0A_IINDPM_INT_ENABLE		0
#define	ETA6973_REG0A_IINDPM_INT_DISABLE	1

#define	ETA6973_REG0A_INT_MASK_MASK			0x03
#define	ETA6973_REG0A_INT_MASK_SHIFT		0


#define	ETA6973_REG_0B				0x0B
#define	ETA6973_REG0B_REG_RESET_MASK		0x80
#define	ETA6973_REG0B_REG_RESET_SHIFT		7
#define	ETA6973_REG0B_REG_RESET				1

#define ETA6973_REG0B_PN_MASK				0x78
#define ETA6973_REG0B_PN_SHIFT				3

#define ETA6973_REG0B_DEV_REV_MASK			0x03
#define ETA6973_REG0B_DEV_REV_SHIFT			0

/* List of registers for axp519 */
#define AXP519_CHIP_ID					0x01
#define AXP519_IRQ_EN0					0x02
#define AXP519_IRQ_EN1					0x03
#define AXP519_IRQ0						0x04
#define AXP519_IRQ1						0x05
#define AXP519_SYS_STATUS				0x06
#define AXP519_WORK_CFG					0x0d
#define AXP519_ADC_CFG					0x10
#define AXP519_ADC_H					0x11
#define AXP519_DISCHG_SET1				0x20
#define AXP519_CHG_SET1					0x30
#define AXP519_CHG_SET3					0x32
#define AXP519_VTERM_CFG_H				0x34
#define AXP519_OVP_SET					0x38
#define AXP519_LINLIM_SET				0x39
#define AXP519_VBATLIM_SET				0x3a
#define AXP519_NTC_SET1					0x43
#define AXP519_NTC_SET2					0x44
#define AXP519_END						0xff

/* List of registers for axp2601 */
#define AXP2601_CHIP_ID					0x00
#define AXP2601_GAUGE_BROM				0x01
#define AXP2601_RESET_CFG				0x02
#define AXP2601_GAUGE_CONFIG			0x03
#define AXP2601_VBAT_H					0x04
#define AXP2601_TM						0x06
#define AXP2601_GAUGE_SOC				0x08
#define AXP2601_T2E						0x0A
#define AXP2601_T2F						0x0C
#define AXP2601_LOWSOC					0x0E
#define AXP2601_IRQ						0x20
#define AXP2601_IRQ_EN					0x21
#define AXP2601_FWVER					0xC0
#define AXP2601_TRIM_EFUSE				0xC8
#define AXP2601_GAUGE_FG_ADDR			0xCD
#define AXP2601_GAUGE_FG_DATA_H			0xCE
#define AXP2601_END						0xFF

/* List of registers for axp2601 */

enum eta6973_irqs {
	/* ETA6973_REG08 */
	ETA6973_IRQ_VBUS_STAT = 1,
	ETA6973_IRQ_CHRG_STAT,
	ETA6973_IRQ_PG_STAT,
	ETA6973_IRQ_THERM_STAT,
	ETA6973_IRQ_VSYS_STAT,
	/* ETA6973_REG09 */
	ETA6973_IRQ_FAULT_WDT,
	ETA6973_IRQ_FAULT_BOOST,
	ETA6973_IRQ_FAULT_CHRG,
	ETA6973_IRQ_FAULT_BAT,
	ETA6973_IRQ_FAULT_NTC,

	/* ETA6973_REG0A */
	ETA6973_IRQ_VBUS_GD,
	ETA6973_IRQ_VINDPM_STAT,
	ETA6973_IRQ_IINDPM_STAT,
	ETA6973_IRQ_TOPOFF_ACTIVE,
	ETA6973_IRQ_ACOV_STAT,

};

enum axp519_irqs {
	/* irq0 */
	AXP519_IRQ_SOVP,
	AXP519_IRQ_COVT,
	AXP519_IRQ_CHGDN,
	AXP519_IRQ_B_INSERT,
	AXP519_IRQ_B_REMOVE,
	AXP519_IRQ_A_INSERT,
	AXP519_IRQ_A_REMOVE,
	/* irq1 */
	AXP519_IRQ_IC_WOT,
	AXP519_IRQ_NTC,
	AXP519_IRQ_VBUS_OV,
	AXP519_IRQ_BOVP,
	AXP519_IRQ_BUVP,
	AXP519_IRQ_VBUS_SC,
	AXP519_IRQ_VBUS_OL,
	AXP519_IRQ_SSCP,
};

enum axp2601_irqs {
	AXP2601_IRQ_WDT,
	AXP2601_IRQ_OT,
	AXP2601_IRQ_NEWSOC,
	AXP2601_IRQ_LOWSOC,
};

/*
 * struct tcs4838 - state holder for the tcs4838 driver
 *
 * Device data may be used to access the tcs4838 chip
 */
struct bmu_ext_dev {
	struct device				*dev;
	int							irq;
	struct regmap				*regmap;
	struct regmap_irq_chip_data	*regmap_irqc;
	long						variant;
	int							nr_cells;
	struct mfd_cell				*cells;
	const struct regmap_config	*regmap_cfg;
	const struct regmap_irq_chip	*regmap_irq_chip;
	void (*dts_parse)(struct bmu_ext_dev *);
};

struct bmu_platform_data {
	int current_limit;		/* mA */
	int weak_battery_voltage;	/* mV */
	int battery_regulation_voltage;	/* mV */
	int charge_current;		/* mA */
	int termination_current;	/* mA */
	int resistor_sense;		/* m ohm */
	const char *notify_device;	/* name */
};

enum bmu_fields {
	F_EN_HIZ, F_EN_ILIM, F_IILIM,					 /* Reg00 */
	F_BHOT, F_BCOLD, F_VINDPM_OFS,					 /* Reg01 */
	F_CONV_START, F_CONV_RATE, F_BOOSTF, F_ICO_EN,
	F_HVDCP_EN, F_MAXC_EN, F_FORCE_DPM, F_AUTO_DPDM_EN,		 /* Reg02 */
	F_BAT_LOAD_EN, F_WD_RST, F_OTG_CFG, F_CHG_CFG, F_SYSVMIN,	 /* Reg03 */
	F_PUMPX_EN, F_ICHG,						 /* Reg04 */
	F_IPRECHG, F_ITERM,						 /* Reg05 */
	F_VREG, F_BATLOWV, F_VRECHG,					 /* Reg06 */
	F_TERM_EN, F_STAT_DIS, F_WD, F_TMR_EN, F_CHG_TMR,
	F_JEITA_ISET,							 /* Reg07 */
	F_BATCMP, F_VCLAMP, F_TREG,					 /* Reg08 */
	F_FORCE_ICO, F_TMR2X_EN, F_BATFET_DIS, F_JEITA_VSET,
	F_BATFET_DLY, F_BATFET_RST_EN, F_PUMPX_UP, F_PUMPX_DN,		 /* Reg09 */
	F_BOOSTV, F_BOOSTI,						 /* Reg0A */
	F_VBUS_STAT, F_CHG_STAT, F_PG_STAT, F_SDP_STAT, F_VSYS_STAT, /* Reg0B */
	F_WD_FAULT, F_BOOST_FAULT, F_CHG_FAULT, F_BAT_FAULT,
	F_NTC_FAULT,							 /* Reg0C */
	F_FORCE_VINDPM, F_VINDPM,					 /* Reg0D */
	F_THERM_STAT, F_BATV,						 /* Reg0E */
	F_SYSV,								 /* Reg0F */
	F_TSPCT,							 /* Reg10 */
	F_VBUS_GD, F_VBUSV,						 /* Reg11 */
	F_ICHGR,							 /* Reg12 */
	F_VDPM_STAT, F_IDPM_STAT, F_IDPM_LIM,				 /* Reg13 */
	F_REG_RST, F_ICO_OPTIMIZED, F_PN, F_TS_PROFILE, F_DEV_REV,	 /* Reg14 */

	F_MAX_FIELDS
};

/* initial field values, converted to register values */
struct bmu_init_data {
	u8 ichg;	/* charge current		*/
	u8 vreg;	/* regulation voltage		*/
	u8 iterm;	/* termination current		*/
	u8 iprechg;	/* precharge current		*/
	u8 sysvmin;	/* minimum system voltage limit */
	u8 boostv;	/* boost regulation voltage	*/
	u8 boosti;	/* boost current limit		*/
	u8 boostf;	/* boost frequency		*/
	u8 ilim_en;	/* enable ILIM pin		*/
	u8 treg;	/* thermal regulation threshold */
};

struct bmu_state {
	u8 online;
	u8 chrg_status;
	u8 chrg_fault;
	u8 vsys_status;
	u8 boost_fault;
	u8 bat_fault;
};

struct bmu_device {
	struct i2c_client *client;
	struct device *dev;
	struct power_supply *charger;

	struct usb_phy *usb_phy;
	struct notifier_block usb_nb;
	struct work_struct usb_work;
	unsigned long usb_event;

	struct regmap *rmap;
	struct regmap_field *rmap_fields[F_MAX_FIELDS];

	int chip_id;
	struct bmu_init_data init_data;
	struct bmu_state state;

	struct mutex lock; /* protect state data */
};

int bmu_ext_match_device(struct bmu_ext_dev *ext);
int bmu_ext_device_init(struct bmu_ext_dev *ext);
int bmu_ext_device_exit(struct bmu_ext_dev *ext);

int bmu_ext_register_cooler(struct power_supply *psy);
void bmu_ext_unregister_cooler(struct power_supply *psy);

#endif /*  __LINUX_MFD_TCS4838_H */
