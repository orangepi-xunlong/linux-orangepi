/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Based on the tcs4838 driver and the previous tcs4838 driver
 */

#ifndef __LINUX_MFD_PMU_EXT_H
#define __LINUX_MFD_PMU_EXT_H

#include "sunxi-power-mfd.h"

enum {
	TCS4838_ID = 0,
	SY8827G_ID,
	AXP1530_ID,
	AW37501_ID,
	OCP2131_ID,
	NR_PMU_EXT_VARIANTS,
};

/* For AXP1530 */
#define AXP1530_ON_INDICATE          (0x00)
#define AXP1530_OFF_INDICATE         (0x01)
#define AXP1530_IC_TYPE              (0x03)
#define AXP1530_OUTPUT_CONTROL       (0x10)
#define AXP1530_DCDC_DVM_PWM         (0x12)
#define AXP1530_DCDC1_CONRTOL        (0x13)
#define AXP1530_DCDC2_CONRTOL        (0x14)
#define AXP1530_DCDC3_CONRTOL        (0x15)
#define AXP1530_ALDO1_CONRTOL        (0x16)
#define AXP1530_DLDO1_CONRTOL        (0x17)
#define AXP1530_POWER_STATUS         (0x1A)
#define AXP1530_PWROK_SET            (0x1B)
#define AXP1530_WAKEUP_CONRTOL       (0x1C)
#define AXP1530_OUTOUT_MONITOR       (0x1D)
#define AXP1530_POK_CONRTOL          (0x1E)
#define AXP1530_IRQ_ENABLE1          (0x20)
#define AXP1530_IRQ_STATUS1          (0x21)
#define AXP1530_LOCK_REG71           (0x70)
#define AXP1530_EPROM_SET            (0x71)
#define AXP1530_DCDC12_DEFAULT       (0x80)
#define AXP1530_DCDC3_A1D1_DEFAULT   (0x81)
#define AXP1530_STARTUP_SEQ          (0x82)
#define AXP1530_STARTUP_RTCLDO       (0x83)
#define AXP1530_BIAS_I2C_ADDR        (0x84)
#define AXP1530_VREF_VRPN            (0x85)
#define AXP1530_VREF_VOL             (0x86)
#define AXP1530_FREQUENCY            (0x87)
#define AXP1530_END                  (0xFF)

/* List of registers for tcs4838 */
#define TCS4838_VSEL0		0x00
#define TCS4838_VSEL1		0x01
#define TCS4838_CTRL		0x02
#define TCS4838_ID1		    0x03
#define TCS4838_ID2		    0x04
#define TCS4838_PGOOD		0x05

/* List of registers for sy8827g */
#define SY8827G_VSEL0		0x00
#define SY8827G_VSEL1		0x01
#define SY8827G_CTRL		0x02
#define SY8827G_ID1		    0x03
#define SY8827G_ID2		    0x04
#define SY8827G_PGOOD		0x05

/* List of registers for aw37501 */
#define AW37501_OUTPUT_EN	0x03
#define AW37501_WRITE_PROTECT	0x21

#define OCP2131_POS_OUTPUT_AVDD		(0x00)
#define OCP2131_NEG_OUTPUT_AVEE		(0x01)

enum {
	AXP1530_DCDC1 = 0,
	AXP1530_DCDC2,
	AXP1530_DCDC3,
	AXP1530_LDO1,  /* RTCLDO */
	AXP1530_LDO2,  /* RTCLDO1 */
	AXP1530_EXT_REG_ID_MAX,
};
/*
 * struct tcs4838 - state holder for the tcs4838 driver
 *
 * Device data may be used to access the tcs4838 chip
 */
struct pmu_ext_dev {
	struct device 				*dev;
	struct regmap				*regmap;
	long		 				variant;
	int 						nr_cells;
	struct mfd_cell				*cells;
	const struct regmap_config	*regmap_cfg;
	void (*dts_parse)(struct pmu_ext_dev *);
};

enum {
	TCS4838_DCDC0 = 0,
	TCS4838_DCDC1,
	TCS4838_REG_ID_MAX,
};

enum {
	SY8827G_DCDC0 = 0,
	SY8827G_DCDC1,
	SY8827G_REG_ID_MAX,
};

enum {
	AW37501_LDO = 0,
	AW37501_REG_ID_MAX,
};

enum {
	OCP2131_AVDD = 0,
	OCP2131_AVEE,
	OCP2131_REG_ID_MAX,
};

int pmu_ext_match_device(struct pmu_ext_dev *ext);
int pmu_ext_device_init(struct pmu_ext_dev *ext);
int pmu_ext_device_exit(struct pmu_ext_dev *ext);

#endif /*  __LINUX_MFD_TCS4838_H */
