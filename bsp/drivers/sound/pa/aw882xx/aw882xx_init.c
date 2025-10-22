// SPDX-License-Identifier: GPL-2.0
/* aw_init.c   aw882xx codec module
 *
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 *
 * Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/* #define DEBUG */

#include <linux/module.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h>
#include <sound/control.h>
#include <linux/uaccess.h>

#include "aw882xx.h"
#include "aw882xx_init.h"
#include "aw882xx_log.h"


/******************************************************
 *
 * aw882xx_init common function
 *
 ******************************************************/

/* [7 : 4]: -6DB ; [3 : 0]: 0.5DB  real_value = value * 2 : 0.5db --> 1 */
static unsigned int aw_6_0P5_reg_val_to_db(unsigned int value)
{
	return ((value >> 4) * AW_6_0P5_VOL_STEP_DB + (value & 0x0f));
}

/* [7 : 4]: -6DB ; [3 : 0]: -0.5DB reg_value = value / step << 4 + value % step ; step = 6 * 2 */
static unsigned int aw_6_0P5_db_val_to_reg(unsigned int value)
{
	return (((value / AW_6_0P5_VOL_STEP_DB) << 4) + (value % AW_6_0P5_VOL_STEP_DB));
}

/* [9 : 6]: -6DB ; [5 : 0]: 0.125DB  real_value = value * 8 : 0.125db --> 1 */
static unsigned int aw_6_0P125_reg_val_to_db(unsigned int value)
{
	return ((value >> 6) * AW_6_0P125_VOL_STEP_DB + (value & 0x3f));
}

/* [9 : 6]: -6DB ; [5 : 0]: 0.125DB reg_value = value / step << 6 + value % step ; step = 6 * 8 */
static unsigned int aw_6_0P125_db_val_to_reg(unsigned int value)
{
	return (((value / AW_6_0P125_VOL_STEP_DB) << 6) + (value % AW_6_0P125_VOL_STEP_DB));
}

/* [9 : 0]: -0.0940625DB*/
static unsigned int aw_direct_reg_val_to_db(unsigned int value)
{
	return value;
}
/* [9 : 0]: -0.0940625DB*/
static unsigned int aw_direct_db_val_to_reg(unsigned int value)
{
	return value;
}

static unsigned int aw_get_irq_type(struct aw_device *aw_dev,
					unsigned int value)
{
	unsigned int ret = INT_TYPE_NONE;

	/* UVL0 */
	if (value & (~AW_UVLI_MASK)) {
		aw_dev_info(aw_dev->dev, "UVLO: occur");
		ret |= INT_TYPE_UVLO;
	}

	/* BSTOCM */
	if (value & (~AW_BSTOCI_MASK)) {
		aw_dev_info(aw_dev->dev, "BSTOCI: occur");
		ret |= INT_TYPE_BSTOC;
	}

	/* OCDI */
	if (value & (~AW_OCDI_MASK)) {
		aw_dev_info(aw_dev->dev, "OCDI: occur");
		ret |= INT_TYPE_OCDI;
	}

	/* OTHI */
	if (value & (~AW_OTHI_MASK)) {
		aw_dev_info(aw_dev->dev, "OTHI: occur");
		ret |= INT_TYPE_OTHI;
	}

	return ret;
}

static unsigned int aw_get_irq_nobst_type(struct aw_device *aw_dev,
					unsigned int value)
{
	unsigned int ret = INT_TYPE_NONE;

	/* UVL0 */
	if (value & (~AW_UVLI_MASK)) {
		aw_dev_info(aw_dev->dev, "UVLO: occur");
		ret |= INT_TYPE_UVLO;
	}

	/* BSTOCM */
	if (value & (~AW_BSTOCI_MASK)) {
		aw_dev_info(aw_dev->dev, "BSTOCI: occur");
		ret |= INT_TYPE_BSTOC;
	}

	/* OCDI */
	if (value & (~AW_OCDI_MASK)) {
		aw_dev_info(aw_dev->dev, "OCDI: occur");
		ret |= INT_TYPE_OCDI;
	}

	/* OTHI */
	if (value & (~AW_OTHI_MASK)) {
		aw_dev_info(aw_dev->dev, "OTHI: occur");
		ret |= INT_TYPE_OTHI;
	}

	return ret;
}


/******************************************************
 *
 * aw882xx_init pid 1852
 *
 ******************************************************/
static void aw_pid_1852_dev_init(struct aw_device *aw_pa)
{
	aw_pa->reg_num = AW_PID_1852_REG_MAX;
	/* call aw device init func */
	memcpy(aw_pa->monitor_name, AW_PID_1852_MONITOR_FILE, strlen(AW_PID_1852_MONITOR_FILE));
	aw_pa->vol_step = AW_6_0P5_VOL_STEP_DB;

	aw_pa->ops.aw_reg_val_to_db = aw_6_0P5_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_6_0P5_db_val_to_reg;

	aw_pa->mute_desc.reg = AW_PID_1852_SYSCTRL2_REG;
	aw_pa->mute_desc.mask = AW_PID_1852_HMUTE_MASK;
	aw_pa->mute_desc.enable = AW_PID_1852_HMUTE_ENABLE_VALUE;
	aw_pa->mute_desc.disable = AW_PID_1852_HMUTE_DISABLE_VALUE;
	aw_pa->mute_desc.name = "btn 1852 hmue";

	aw_pa->uls_hmute_desc.reg = AW_REG_NONE;

	aw_pa->txen_desc.reg = AW_PID_1852_I2SCFG1_REG;
	aw_pa->txen_desc.mask = AW_PID_1852_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_1852_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_1852_I2STXEN_DISABLE_VALUE;

	aw_pa->vcalb_desc.vcalb_reg = AW_PID_1852_VTMCTRL3_REG;
	aw_pa->vcalb_desc.vcal_factor = AW_PID_1852_VCAL_FACTOR;
	aw_pa->vcalb_desc.cabl_base_value = AW_PID_1852_CABL_BASE_VALUE;

	aw_pa->vcalb_desc.icalk_value_factor = AW_PID_1852_ICABLK_FACTOR;
	aw_pa->vcalb_desc.icalk_reg = AW_PID_1852_EFRM1_REG;
	aw_pa->vcalb_desc.icalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.icalk_reg_mask = AW_PID_1852_EF_ISN_GESLP_MASK;
	aw_pa->vcalb_desc.icalk_sign_mask = AW_PID_1852_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_neg_mask = AW_PID_1852_EF_ISN_GESLP_NEG;

	aw_pa->vcalb_desc.vcalk_reg = AW_PID_1852_EFRH_REG;
	aw_pa->vcalb_desc.vcalk_reg_mask = AW_PID_1852_EF_VSN_GESLP_MASK;
	aw_pa->vcalb_desc.vcalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.vcalk_sign_mask = AW_PID_1852_EF_VSN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_neg_mask = AW_PID_1852_EF_VSN_GESLP_NEG;
	aw_pa->vcalb_desc.vcalk_value_factor = AW_PID_1852_VCABLK_FACTOR;


	aw_pa->cco_mux_desc.reg = AW_PID_1852_PLLCTRL1_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_1852_I2S_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_1852_I2S_CCO_MUX_8_16_32KHZ_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_1852_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;

	aw_pa->voltage_desc.reg = AW_PID_1852_VBAT_REG;

	aw_pa->temp_desc.reg = AW_PID_1852_TEMP_REG;

	aw_pa->ipeak_desc.reg = AW_PID_1852_SYSCTRL2_REG;
	aw_pa->ipeak_desc.mask = AW_PID_1852_BST_IPEAK_MASK;

	aw_pa->volume_desc.reg = AW_PID_1852_HAGCCFG4_REG;
	aw_pa->volume_desc.mask = AW_PID_1852_VOL_MASK;
	aw_pa->volume_desc.shift = AW_PID_1852_VOL_START_BIT;
	aw_pa->volume_desc.mute_volume = AW_PID_1852_MUTE_VOL;
	aw_pa->volume_desc.ctl_volume = AW_PID_1852_VOL_DEFAULT_VALUE;

	aw_pa->bop_desc.reg = AW_REG_NONE;

	aw_pa->dither_desc.reg = AW_PID_1852_TESTCTRL2_REG;
	aw_pa->dither_desc.mask = AW_PID_1852_DITHER_MASK;
	aw_pa->dither_desc.enable = AW_PID_1852_DITHER_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_1852_DITHER_DISABLE_VALUE;
	aw_pa->dither_desc.name = "btn 1852 dither";

	aw_pa->noise_gate_desc.reg = AW_REG_NONE;

	aw_pa->ef_desc.count = 1;
	aw_pa->ef_desc.sequence[0].reg = AW_PID_1852_EFRH_REG;
	aw_pa->ef_desc.sequence[0].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[0].check_val = AW_EF_LOCK_ENABLE_VALUE;
}

/******************************************************
 *
 * aw882xx_init pid 2013
 *
 ******************************************************/

static void aw_pid_2013_efver_check(struct aw_device *aw_dev)
{
	unsigned int reg_val = 0;
	unsigned int efverh = 0;
	unsigned int efverl = 0;

	aw_dev->ops.aw_i2c_read(aw_dev->i2c,
			AW_PID_2013_EFRM1_REG, &reg_val);

	efverh = (((reg_val & (~AW_PID_2013_EFVERH_MASK)) >>
			AW_PID_2013_EFVERH_START_BIT) ^
			AW_PID_2013_EFVER_CHECK);
	efverl = (((reg_val & (~AW_PID_2013_EFVERL_MASK)) >>
			AW_PID_2013_EFVERL_START_BIT) ^
			AW_PID_2013_EFVER_CHECK);

	aw_dev_dbg(aw_dev->dev, "efverh: 0x%0x, efverl: 0x%0x", efverh, efverl);

	if (efverh && efverl) {
		aw_dev_info(aw_dev->dev, "A2013 EFVER A");
	} else if (efverh && !efverl) {
		aw_dev->profctrl_desc.reg = AW_REG_NONE;
		aw_dev->bstctrl_desc.reg = AW_REG_NONE;
		aw_dev_info(aw_dev->dev, "A2013 EFVER B");
	} else {
		aw_dev->profctrl_desc.reg = AW_REG_NONE;
		aw_dev->bstctrl_desc.reg = AW_REG_NONE;
		aw_dev_info(aw_dev->dev, "unsupport A2013 EFVER");
	}
}

static void aw_pid_2013_dev_init(struct aw_device *aw_pa)
{
	aw_pa->reg_num = AW_PID_2013_REG_MAX;
	/*call aw device init func*/
	memcpy(aw_pa->monitor_name, AW_PID_2013_MONITOR_FILE, strlen(AW_PID_2013_MONITOR_FILE));

	aw_pa->vol_step = AW_6_0P125_VOL_STEP_DB;
	aw_pa->ops.aw_reg_val_to_db = aw_6_0P125_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_6_0P125_db_val_to_reg;

	aw_pa->uls_hmute_desc.reg = AW_REG_NONE;

	aw_pa->txen_desc.reg = AW_PID_2013_I2SCTRL1_REG;
	aw_pa->txen_desc.mask = AW_PID_2013_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_2013_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_2013_I2STXEN_DISABLE_VALUE;

	aw_pa->vcalb_desc.vcalb_reg = AW_PID_2013_VSNTM1_REG;
	aw_pa->vcalb_desc.vcal_factor = AW_PID_2013_VCAL_FACTOR;
	aw_pa->vcalb_desc.cabl_base_value = AW_PID_2013_CABL_BASE_VALUE;

	aw_pa->vcalb_desc.icalk_value_factor = AW_PID_2013_ICABLK_FACTOR;
	aw_pa->vcalb_desc.icalk_reg = AW_PID_2013_EFRH_REG;
	aw_pa->vcalb_desc.icalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.icalk_reg_mask = AW_PID_2013_EF_ISN_GESLP_MASK;
	aw_pa->vcalb_desc.icalk_sign_mask = AW_PID_2013_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_neg_mask = AW_PID_2013_EF_ISN_GESLP_NEG;

	aw_pa->vcalb_desc.vcalk_reg = AW_PID_2013_EFRM2_REG;
	aw_pa->vcalb_desc.vcalk_reg_mask = AW_PID_2013_EF_VSN_GESLP_MASK;
	aw_pa->vcalb_desc.vcalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.vcalk_sign_mask = AW_PID_2013_EF_ISN_GESLP2_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_neg_mask = AW_PID_2013_EF_ISN_GESLP2_NEG;
	aw_pa->vcalb_desc.vcalk_value_factor = AW_PID_2013_VCABLK_FACTOR;

	aw_pa->profctrl_desc.reg = AW_PID_2013_SYSCTRL_REG;
	aw_pa->profctrl_desc.mask = AW_PID_2013_EN_TRAN_MASK;
	aw_pa->profctrl_desc.spk_mode = AW_PID_2013_EN_TRAN_SPK_VALUE;

	aw_pa->bstctrl_desc.reg = AW_PID_2013_BSTCTRL2_REG;
	aw_pa->bstctrl_desc.mask = AW_PID_2013_BST_MODE_MASK;
	aw_pa->bstctrl_desc.frc_bst = AW_PID_2013_BST_MODE_FORCE_BOOST_VALUE;
	aw_pa->bstctrl_desc.tsp_type = AW_PID_2013_BST_MODE_TRANSPARENT_VALUE;

	aw_pa->cco_mux_desc.reg = AW_PID_2013_PLLCTRL3_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_2013_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_2013_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_2013_CCO_MUX_BYPASS_VALUE;

	aw_pa->volume_desc.mute_volume = AW_PID_2013_MUTE_VOL;
	aw_pa->volume_desc.ctl_volume = AW_VOL_DEFAULT_VALUE;

	aw_pa->bop_desc.reg = AW_REG_NONE;

	aw_pa->dither_desc.reg = AW_PID_2013_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW_PID_2013_DITHER_MASK;
	aw_pa->dither_desc.enable = AW_PID_2013_DITHER_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_2013_DITHER_DISABLE_VALUE;
	aw_pa->dither_desc.name = "btn 2013 dither";

	aw_pa->noise_gate_desc.reg = AW_REG_NONE;

	aw_pa->ef_desc.count = 1;
	aw_pa->ef_desc.sequence[0].reg = AW_PID_2013_EFRH_REG;
	aw_pa->ef_desc.sequence[0].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[0].check_val = AW_EF_LOCK_ENABLE_VALUE;

	aw_pid_2013_efver_check(aw_pa);
}

/******************************************************
 *
 * aw882xx_init pid 2032
 *
 ******************************************************/
static void aw_pid_2032_dev_init(struct aw_device *aw_pa)
{
	aw_pa->reg_num = AW_PID_2032_REG_MAX;
	/* call aw device init func */
	memcpy(aw_pa->monitor_name, AW_PID_2032_MONITOR_FILE, strlen(AW_PID_2032_MONITOR_FILE));
	aw_pa->vol_step = AW_6_0P125_VOL_STEP_DB;

	aw_pa->ops.aw_reg_val_to_db = aw_6_0P125_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_6_0P125_db_val_to_reg;

	aw_pa->mute_desc.reg = AW_PID_2032_SYSCTRL2_REG;
	aw_pa->mute_desc.mask = AW_PID_2032_HMUTE_MASK;
	aw_pa->mute_desc.enable = AW_PID_2032_HMUTE_ENABLE_VALUE;
	aw_pa->mute_desc.disable = AW_PID_2032_HMUTE_DISABLE_VALUE;
	aw_pa->mute_desc.name = "btn 2032 hmute";

	aw_pa->uls_hmute_desc.reg = AW_REG_NONE;

	aw_pa->txen_desc.reg = AW_PID_2032_I2SCFG1_REG;
	aw_pa->txen_desc.mask = AW_PID_2032_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_2032_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_2032_I2STXEN_DISABLE_VALUE;

	aw_pa->vcalb_desc.vcalb_reg = AW_PID_2032_VTMCTRL3_REG;
	aw_pa->vcalb_desc.vcal_factor = AW_PID_2032_VCAL_FACTOR;
	aw_pa->vcalb_desc.cabl_base_value = AW_PID_2032_CABL_BASE_VALUE;

	aw_pa->vcalb_desc.icalk_value_factor = AW_PID_2032_ICABLK_FACTOR;
	aw_pa->vcalb_desc.icalk_reg = AW_PID_2032_EFRM1_REG;
	aw_pa->vcalb_desc.icalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.icalk_reg_mask = AW_PID_2032_EF_ISN_GESLP_MASK;
	aw_pa->vcalb_desc.icalk_sign_mask = AW_PID_2032_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_neg_mask = AW_PID_2032_EF_ISN_GESLP_NEG;

	aw_pa->vcalb_desc.vcalk_reg = AW_PID_2032_EFRH_REG;
	aw_pa->vcalb_desc.vcalk_reg_mask = AW_PID_2032_EF_VSN_GESLP_MASK;
	aw_pa->vcalb_desc.vcalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.vcalk_sign_mask = AW_PID_2032_EF_VSN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_neg_mask = AW_PID_2032_EF_VSN_GESLP_NEG;
	aw_pa->vcalb_desc.vcalk_value_factor = AW_PID_2032_VCABLK_FACTOR;

	aw_pa->cco_mux_desc.reg = AW_PID_2032_PLLCTRL1_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_2032_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_2032_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_2032_CCO_MUX_BYPASS_VALUE;

	aw_pa->voltage_desc.reg = AW_PID_2032_VBAT_REG;
	aw_pa->temp_desc.reg = AW_PID_2032_TEMP_REG;

	aw_pa->ipeak_desc.reg = AW_PID_2032_SYSCTRL2_REG;
	aw_pa->ipeak_desc.mask = AW_PID_2032_BST_IPEAK_MASK;

	aw_pa->volume_desc.reg = AW_PID_2032_SYSCTRL2_REG;
	aw_pa->volume_desc.mask = AW_PID_2032_VOL_MASK;
	aw_pa->volume_desc.shift = AW_PID_2032_VOL_START_BIT;
	aw_pa->volume_desc.mute_volume = AW_PID_2032_MUTE_VOL;
	aw_pa->volume_desc.ctl_volume = AW_VOL_DEFAULT_VALUE;

	aw_pa->bop_desc.reg = AW_REG_NONE;

	aw_pa->dither_desc.reg = AW_PID_2032_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW_PID_2032_DITHER_MASK;
	aw_pa->dither_desc.enable = AW_PID_2032_DITHER_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_2032_DITHER_DISABLE_VALUE;
	aw_pa->dither_desc.name = "btn 2032 dither";

	aw_pa->noise_gate_desc.reg = AW_REG_NONE;

	aw_pa->ef_desc.count = 1;
	aw_pa->ef_desc.sequence[0].reg = AW_PID_2032_EFRH_REG;
	aw_pa->ef_desc.sequence[0].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[0].check_val = AW_EF_LOCK_ENABLE_VALUE;
}

/******************************************************
 *
 * aw882xx_init pid 2055
 *
 ******************************************************/
static void aw_pid_2055_dev_init(struct aw_device *aw_pa)
{

	aw_pa->reg_num = AW_PID_2055_REG_MAX;
	/*call aw device init func*/
	memcpy(aw_pa->monitor_name, AW_PID_2055_MONITOR_FILE, strlen(AW_PID_2055_MONITOR_FILE));

	aw_pa->vol_step = AW_6_0P125_VOL_STEP_DB;

	aw_pa->ops.aw_reg_val_to_db = aw_6_0P125_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_6_0P125_db_val_to_reg;

	aw_pa->uls_hmute_desc.reg = AW_REG_NONE;

	aw_pa->txen_desc.reg = AW_PID_2055_I2SCTRL1_REG;
	aw_pa->txen_desc.mask = AW_PID_2055_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_2055_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_2055_I2STXEN_DISABLE_VALUE;

	aw_pa->volume_desc.mute_volume = AW_PID_2055_MUTE_VOL;

	aw_pa->cco_mux_desc.reg = AW_PID_2055_PLLCTRL3_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_2055_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_2055_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_2055_CCO_MUX_BYPASS_VALUE;

	aw_pa->bop_desc.reg = AW_PID_2055_SADCCTRL3_REG;
	aw_pa->bop_desc.mask = AW_PID_2055_BOP_EN_MASK;
	aw_pa->bop_desc.enable = AW_PID_2055_BOP_EN_ENABLE_VALUE;
	aw_pa->bop_desc.disable = AW_PID_2055_BOP_EN_DISABLE_VALUE;
	aw_pa->bop_desc.name = "btn 2055 bop";

	aw_pa->dither_desc.reg = AW_PID_2055_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW_PID_2055_DITHER_MASK;
	aw_pa->dither_desc.enable = AW_PID_2055_DITHER_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_2055_DITHER_DISABLE_VALUE;
	aw_pa->dither_desc.name = "btn 2055 dither";

	aw_pa->noise_gate_desc.reg = AW_REG_NONE;

	aw_pa->ef_desc.count = 2;
	aw_pa->ef_desc.sequence[0].reg = AW_PID_2055_EFRH_REG;
	aw_pa->ef_desc.sequence[0].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[0].check_val = AW_EF_LOCK_ENABLE_VALUE;
	aw_pa->ef_desc.sequence[1].reg = AW_PID_2055_EFRL_REG;
	aw_pa->ef_desc.sequence[1].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[1].check_val = AW_EF_LOCK_ENABLE_VALUE;
}

static void aw_pid_2055a_dev_init(struct aw_device *aw_pa)
{

	aw_pa->chip_id = PID_2055A_ID;

	/*call aw device init func*/
	memcpy(aw_pa->monitor_name, AW_PID_2055A_MONITOR_FILE, strlen(AW_PID_2055A_MONITOR_FILE));

	aw_pa->work_mode.reg = AW_REG_NONE;
	aw_pa->voltage_desc.reg = AW_REG_NONE;
	aw_pa->temp_desc.reg = AW_REG_NONE;
}


static int aw_pid_2055_dev_check(struct aw_device *aw_pa)
{
	unsigned int reg_data = 0;

	aw_pid_2055_dev_init(aw_pa);

	aw_pa->ops.aw_i2c_write(aw_pa->i2c, AW882XX_SOFT_RESET_REG, AW882XX_SOFT_RESET_VALUE);
	usleep_range(AW_1000_US, AW_1000_US + 100);

	aw_pa->ops.aw_i2c_read(aw_pa->i2c, AW_PID_2055_VERSION_DIFF_REG, &reg_data);

	aw_pid_2055_dev_init(aw_pa);

	if (reg_data == AW_PID_2055A_VERSION_VALUE)
		aw_pid_2055a_dev_init(aw_pa);

	usleep_range(AW_2000_US, AW_2000_US + 10);
	aw_pa->ops.aw_i2c_write(aw_pa->i2c, AW_PID_2055_INIT_CHECK_REG,
					AW_PID_2055_INIT_CHECK_VALUE);
	usleep_range(AW_3000_US, AW_3000_US + 10);

	return 0;
}
/******************************************************
 *
 * aw882xx_init pid 2071
 *
 ******************************************************/
static void aw_pid_2071_dev_init(struct aw_device *aw_pa)
{

	aw_pa->reg_num = AW_PID_2071_REG_MAX;
	/*call aw device init func*/
	memcpy(aw_pa->monitor_name, AW_PID_2071_MONITOR_FILE, strlen(AW_PID_2071_MONITOR_FILE));

	aw_pa->vol_step = AW_6_0P125_VOL_STEP_DB;
	aw_pa->ops.aw_reg_val_to_db = aw_6_0P125_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_6_0P125_db_val_to_reg;

	aw_pa->mute_desc.reg = AW_PID_2071_SYSCTRL2_REG;
	aw_pa->mute_desc.mask = AW_PID_2071_HMUTE_MASK;
	aw_pa->mute_desc.enable = AW_PID_2071_HMUTE_ENABLE_VALUE;
	aw_pa->mute_desc.disable = AW_PID_2071_HMUTE_DISABLE_VALUE;
	aw_pa->mute_desc.name = "btn 2071 hmute";

	aw_pa->uls_hmute_desc.reg = AW_REG_NONE;

	aw_pa->txen_desc.reg = AW_PID_2071_I2SCFG1_REG;
	aw_pa->txen_desc.mask = AW_PID_2071_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_2071_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_2071_I2STXEN_DISABLE_VALUE;

	aw_pa->vcalb_desc.vcalb_reg = AW_PID_2071_VTMCTRL3_REG;
	aw_pa->vcalb_desc.vcal_factor = AW_PID_2071_VCAL_FACTOR;
	aw_pa->vcalb_desc.cabl_base_value = AW_PID_2071_CABL_BASE_VALUE;

	aw_pa->vcalb_desc.icalk_value_factor = AW_PID_2071_ICABLK_FACTOR;
	aw_pa->vcalb_desc.icalk_reg = AW_PID_2071_EFRH_REG;
	aw_pa->vcalb_desc.icalk_reg_mask = AW_PID_2071_EF_VSN_OFFSET_MASK;
	aw_pa->vcalb_desc.icalk_shift = AW_PID_2071_ICALK_SHIFT;
	aw_pa->vcalb_desc.icalkl_reg = AW_PID_2071_EFRM1_REG;
	aw_pa->vcalb_desc.icalkl_reg_mask = AW_PID_2071_EF_ISN_OFFSET_MASK;
	aw_pa->vcalb_desc.icalkl_shift = AW_PID_2071_ICALKL_SHIFT;
	aw_pa->vcalb_desc.icalk_sign_mask = AW_PID_2071_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_neg_mask = AW_PID_2071_EF_ISN_GESLP_NEG;

	aw_pa->vcalb_desc.vcalk_reg = AW_PID_2071_EFRH_REG;
	aw_pa->vcalb_desc.vcalk_reg_mask = AW_PID_2071_EF_VSN_GESLP_MASK;
	aw_pa->vcalb_desc.vcalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.vcalk_sign_mask = AW_PID_2071_EF_VSN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_neg_mask = AW_PID_2071_EF_VSN_GESLP_NEG;
	aw_pa->vcalb_desc.vcalk_value_factor = AW_PID_2071_VCABLK_FACTOR;

	aw_pa->cco_mux_desc.reg = AW_PID_2071_PLLCTRL1_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_2071_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_2071_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_2071_CCO_MUX_BYPASS_VALUE;

	aw_pa->voltage_desc.reg = AW_PID_2071_VBAT_REG;
	aw_pa->temp_desc.reg = AW_PID_2071_TEMP_REG;

	aw_pa->ipeak_desc.reg = AW_PID_2071_SYSCTRL2_REG;
	aw_pa->ipeak_desc.mask = AW_PID_2071_BST_IPEAK_MASK;

	aw_pa->volume_desc.reg = AW_PID_2071_SYSCTRL2_REG;
	aw_pa->volume_desc.mask = AW_PID_2071_VOL_MASK;
	aw_pa->volume_desc.shift = AW_PID_2071_VOL_START_BIT;
	aw_pa->volume_desc.mute_volume = AW_PID_2071_MUTE_VOL;
	aw_pa->volume_desc.ctl_volume = AW_VOL_DEFAULT_VALUE;

	aw_pa->efuse_check = AW_EF_OR_CHECK;

	aw_pa->bop_desc.reg = AW_REG_NONE;

	aw_pa->dither_desc.reg = AW_PID_2071_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW_PID_2071_DITHER_MASK;
	aw_pa->dither_desc.enable = AW_PID_2071_DITHER_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_2071_DITHER_DISABLE_VALUE;
	aw_pa->dither_desc.name = "btn 2071 dither";

	aw_pa->noise_gate_desc.reg = AW_REG_NONE;

	aw_pa->ef_desc.count = 1;
	aw_pa->ef_desc.sequence[0].reg = AW_PID_2071_EFRH_REG;
	aw_pa->ef_desc.sequence[0].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[0].check_val = AW_EF_LOCK_ENABLE_VALUE;
}

/******************************************************
 *
 * aw882xx_init pid 2113
 *
 ******************************************************/
static int aw_pid_2113_frcset_check(struct aw_device *aw_dev)
{
	unsigned int reg_val = 0;
	uint16_t temh = 0;
	uint16_t teml = 0;
	uint16_t tem = 0;

	aw_dev->ops.aw_i2c_read(aw_dev->i2c,
			AW_PID_2113_EFRH3_REG, &reg_val);
	temh = ((uint16_t)reg_val & (~AW_PID_2113_TEMH_MASK));

	aw_dev->ops.aw_i2c_read(aw_dev->i2c,
			AW_PID_2113_EFRL3_REG, &reg_val);
	teml = ((uint16_t)reg_val & (~AW_PID_2113_TEML_MASK));

	if (aw_dev->efuse_check == AW_EF_OR_CHECK)
		tem = (temh | teml);
	else
		tem = (temh & teml);

	if (tem == AW_PID_2113_DEFAULT_CFG)
		aw_dev->frcset_en = AW_FRCSET_ENABLE;
	else
		aw_dev->frcset_en = AW_FRCSET_DISABLE;

	aw_dev_info(aw_dev->dev, "tem is 0x%04x, frcset_en is %d",
						tem, aw_dev->frcset_en);
	return 0;
}

static void aw_pid_2113_reg_force_set(struct aw_device *aw_dev)
{
	aw_dev_dbg(aw_dev->dev, "enter");

	if (aw_dev->frcset_en == AW_FRCSET_ENABLE) {
		/*set FORCE_PWM*/
		aw_dev->ops.aw_i2c_write_bits(aw_dev->i2c, AW_PID_2113_BSTCTRL3_REG,
				AW_PID_2113_FORCE_PWM_MASK, AW_PID_2113_FORCE_PWM_FORCEMINUS_PWM_VALUE);
		/*set BOOST_OS_WIDTH*/
		aw_dev->ops.aw_i2c_write_bits(aw_dev->i2c, AW_PID_2113_BSTCTRL5_REG,
				AW_PID_2113_BST_OS_WIDTH_MASK, AW_PID_2113_BST_OS_WIDTH_50NS_VALUE);
		/*set BURST_LOOPR*/
		aw_dev->ops.aw_i2c_write_bits(aw_dev->i2c, AW_PID_2113_BSTCTRL6_REG,
				AW_PID_2113_BST_LOOPR_MASK, AW_PID_2113_BST_LOOPR_340K_VALUE);
		/*set RSQN_DLY*/
		aw_dev->ops.aw_i2c_write_bits(aw_dev->i2c, AW_PID_2113_BSTCTRL7_REG,
				AW_PID_2113_RSQN_DLY_MASK, AW_PID_2113_RSQN_DLY_35NS_VALUE);
		/*set BURST_SSMODE*/
		aw_dev->ops.aw_i2c_write_bits(aw_dev->i2c, AW_PID_2113_BSTCTRL8_REG,
				AW_PID_2113_BURST_SSMODE_MASK, AW_PID_2113_BURST_SSMODE_FAST_VALUE);
		/*set BST_BURST*/
		aw_dev->ops.aw_i2c_write_bits(aw_dev->i2c, AW_PID_2113_BSTCTRL9_REG,
				AW_PID_2113_BST_BURST_MASK, AW_PID_2113_BST_BURST_30MA_VALUE);
		aw_dev_dbg(aw_dev->dev, "force set reg done!");
	} else {
		aw_dev_info(aw_dev->dev, "needn't set reg value");
	}
}

static void aw_pid_2113_dev_init(struct aw_device *aw_pa)
{

	aw_pa->reg_num = AW_PID_2113_REG_MAX;
	/*call aw device init func*/
	memcpy(aw_pa->monitor_name, AW_PID_2113_MONITOR_FILE, strlen(AW_PID_2113_MONITOR_FILE));
	aw_pa->vol_step = AW_6_0P125_VOL_STEP_DB;

	aw_pa->ops.aw_reg_val_to_db = aw_6_0P125_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_6_0P125_db_val_to_reg;
	aw_pa->ops.aw_reg_force_set = aw_pid_2113_reg_force_set;
	aw_pa->ops.aw_frcset_check = aw_pid_2113_frcset_check;

	aw_pa->uls_hmute_desc.reg = AW_PID_2113_SYSCTRL_REG;
	aw_pa->uls_hmute_desc.mask = AW_PID_2113_ULS_HMUTE_MASK;
	aw_pa->uls_hmute_desc.enable = AW_PID_2113_ULS_HMUTE_ENABLE_VALUE;
	aw_pa->uls_hmute_desc.disable = AW_PID_2113_ULS_HMUTE_DISABLE_VALUE;
	aw_pa->uls_hmute_desc.name = "btn 2113 uls hmute";

	aw_pa->vcalb_desc.vcalb_reg = AW_PID_2113_VSNTM1_REG;
	aw_pa->vcalb_desc.vcal_factor = AW_PID_2113_VCAL_FACTOR;
	aw_pa->vcalb_desc.cabl_base_value = AW_PID_2113_CABL_BASE_VALUE;

	aw_pa->txen_desc.reg = AW_PID_2113_I2SCTRL3_REG;
	aw_pa->txen_desc.mask = AW_PID_2113_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_2113_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_2113_I2STXEN_DISABLE_VALUE;

	aw_pa->vcalb_desc.icalk_reg = AW_PID_2113_EFRH4_REG;
	aw_pa->vcalb_desc.icalk_reg_mask = AW_PID_2113_EF_ISN_GESLP_H_MASK;
	aw_pa->vcalb_desc.icalk_shift = AW_PID_2113_ICALK_SHIFT;
	aw_pa->vcalb_desc.icalkl_reg = AW_PID_2113_EFRL4_REG;
	aw_pa->vcalb_desc.icalkl_reg_mask = AW_PID_2113_EF_ISN_GESLP_L_MASK;
	aw_pa->vcalb_desc.icalkl_shift = AW_PID_2113_ICALKL_SHIFT;
	aw_pa->vcalb_desc.icalk_sign_mask = AW_PID_2113_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_neg_mask = AW_PID_2113_EF_ISN_GESLP_NEG;
	aw_pa->vcalb_desc.icalk_value_factor = AW_PID_2113_ICABLK_FACTOR;

	aw_pa->vcalb_desc.vcalk_reg = AW_PID_2113_EFRH3_REG;
	aw_pa->vcalb_desc.vcalk_reg_mask = AW_PID_2113_EF_VSN_GESLP_H_MASK;
	aw_pa->vcalb_desc.vcalk_shift = AW_PID_2113_VCALK_SHIFT;
	aw_pa->vcalb_desc.vcalkl_reg = AW_PID_2113_EFRL3_REG;
	aw_pa->vcalb_desc.vcalkl_reg_mask = AW_PID_2113_EF_VSN_GESLP_L_MASK;
	aw_pa->vcalb_desc.vcalkl_shift = AW_PID_2113_VCALKL_SHIFT;
	aw_pa->vcalb_desc.vcalk_sign_mask = AW_PID_2113_EF_VSN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_neg_mask = AW_PID_2113_EF_VSN_GESLP_NEG;
	aw_pa->vcalb_desc.vcalk_value_factor = AW_PID_2113_VCABLK_FACTOR;


	aw_pa->cco_mux_desc.reg = AW_PID_2113_PLLCTRL1_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_2113_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_2113_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_2113_CCO_MUX_BYPASS_VALUE;

	aw_pa->volume_desc.mute_volume = AW_PID_2113_MUTE_VOL;

	aw_pa->bop_desc.reg = AW_PID_2113_SADCCTRL3_REG;
	aw_pa->bop_desc.mask = AW_PID_2113_BOP_EN_MASK;
	aw_pa->bop_desc.enable = AW_PID_2113_BOP_EN_ENABLE_VALUE;
	aw_pa->bop_desc.disable = AW_PID_2113_BOP_EN_DISABLE_VALUE;
	aw_pa->bop_desc.name = "btn 2113 bop";

	aw_pa->ops.aw_i2c_write(aw_pa->i2c, AW_PID_2113_INIT_CHECK_REG,
					AW_PID_2113_INIT_CHECK_VALUE);
	usleep_range(AW_3000_US, AW_3000_US + 10);

	aw_pa->efcheck_desc.reg = AW_PID_2113_DBGCTRL_REG;
	aw_pa->efcheck_desc.mask = AW_PID_2113_EF_DBMD_MASK;
	aw_pa->efcheck_desc.and_val = AW_PID_2113_AND_VALUE;
	aw_pa->efcheck_desc.or_val = AW_PID_2113_OR_VALUE;

	aw_pa->dither_desc.reg = AW_PID_2113_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW_PID_2113_DITHER_MASK;
	aw_pa->dither_desc.enable = AW_PID_2113_DITHER_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_2113_DITHER_DISABLE_VALUE;
	aw_pa->dither_desc.name = "btn 2113 dither";

	aw_pa->noise_gate_desc.reg = AW_REG_NONE;
}

/******************************************************
 *
 * aw882xx_init pid 2116
 *
 ******************************************************/
static void aw_pid_2116_dev_init(struct aw_device *aw_pa)
{

	/*call aw device init func*/
	aw_pa->reg_num = AW_PID_2116_REG_MAX;

	aw_pa->ops.aw_get_irq_type = aw_get_irq_nobst_type;

	aw_pa->int_desc.mask_default = AW_PID_2116_SYSINTM_DEFAULT;
	aw_pa->int_desc.int_mask = AW_PID_2116_SYSINTM_DEFAULT;

	aw_pa->work_mode.reg = AW_REG_NONE;

	aw_pa->sysst_desc.mask = AW_PID_2116_SYSST_CHECK_MASK;
	aw_pa->sysst_desc.st_check = AW_PID_2116_NO_SWS_SYSST_CHECK;
	aw_pa->sysst_desc.st_sws_check = AW_PID_2116_SWS_SYSST_CHECK;

	aw_pa->amppd_desc.reg = AW_PID_2116_SYSCTRL_REG;
	aw_pa->amppd_desc.mask = AW_PID_2116_EN_PA_MASK;
	aw_pa->amppd_desc.enable = AW_PID_2116_EN_PA_POWER_DOWN_VALUE;
	aw_pa->amppd_desc.disable = AW_PID_2116_EN_PA_WORKING_VALUE;
	aw_pa->amppd_desc.name = "btn 2116 amppd";

	aw_pa->uls_hmute_desc.reg = AW_PID_2116_SYSCTRL_REG;
	aw_pa->uls_hmute_desc.mask = AW_PID_2116_ULS_HMUTE_MASK;
	aw_pa->uls_hmute_desc.enable = AW_PID_2116_ULS_HMUTE_ENABLE_VALUE;
	aw_pa->uls_hmute_desc.disable = AW_PID_2116_ULS_HMUTE_DISABLE_VALUE;
	aw_pa->uls_hmute_desc.name = "btn 2116 uls hmute";

	aw_pa->txen_desc.reg = AW_PID_2116_I2SCTRL3_REG;
	aw_pa->txen_desc.mask = AW_PID_2116_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_PID_2116_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_PID_2116_I2STXEN_DISABLE_VALUE;

	aw_pa->cco_mux_desc.reg = AW_PID_2116_PLLCTRL1_REG;
	aw_pa->cco_mux_desc.mask = AW_PID_2116_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_PID_2116_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_PID_2116_CCO_MUX_BYPASS_VALUE;

	aw_pa->voltage_desc.reg = AW_REG_NONE;
	aw_pa->temp_desc.reg = AW_REG_NONE;
	aw_pa->ipeak_desc.reg = AW_REG_NONE;

	aw_pa->bop_desc.reg = AW_PID_2116_SYSCTRL2_REG;
	aw_pa->bop_desc.mask = AW_PID_2116_BOP_EN_MASK;
	aw_pa->bop_desc.enable = AW_PID_2116_BOP_EN_ENABLE_VALUE;
	aw_pa->bop_desc.disable = AW_PID_2116_BOP_EN_DISABLE_VALUE;
	aw_pa->bop_desc.name = "btn 2116 bop";

	aw_pa->noise_gate_desc.reg = AW_PID_2116_PWMCTRL4_REG;
	aw_pa->noise_gate_desc.mask = AW_PID_2116_NOISE_GATE_EN_MASK;
	aw_pa->noise_gate_desc.enable = AW_PID_2116_NOISE_GATE_EN_ENABLE_VALUE;
	aw_pa->noise_gate_desc.disable = AW_PID_2116_NOISE_GATE_EN_DISABLE_VALUE;
	aw_pa->noise_gate_desc.name = "btn 2116 noise gate";

	aw_pa->ef_desc.count = 0;
}

/******************************************************
 *
 * aw882xx_init pid 2308
 *
 ******************************************************/
static int aw_pid_2308_get_voltage_offset(struct aw_device *aw_dev, int32_t *offset)
{
	int ret = 0;
	signed char val = -1;
	int32_t temp = -1;
	unsigned int reg_value = 0;

	ret = aw_dev->ops.aw_i2c_read(aw_dev->i2c, AW_PID_2308_EFRH2_REG, &reg_value);
	aw_dev_dbg(aw_dev->dev, "get addr %x val 0x%x",
						AW_PID_2308_EFRH2_REG, reg_value);

	val = (signed char)(reg_value & 0x00ff);
	temp = val;
	*offset = temp << 1;
	aw_dev_info(aw_dev->dev, "get vol offset val %d", *offset);
	return ret;
}

static void aw_pid_2308_dev_init(struct aw_device *aw_pa)
{

	aw_pa->reg_num = AW_PID_2308_REG_MAX;
	/*call aw device init func*/
	memcpy(aw_pa->monitor_name, AW_PID_2308_MONITOR_FILE, strlen(AW_PID_2308_MONITOR_FILE));

	aw_pa->ops.aw_get_voltage_offset = aw_pid_2308_get_voltage_offset;
	aw_pa->ops.aw_reg_val_to_db = aw_direct_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_direct_db_val_to_reg;

	aw_pa->auth_desc.reg_in = AW_PID_2308_TESTIN_REG;
	aw_pa->auth_desc.reg_out = AW_PID_2308_TESTOUT_REG;

	aw_pa->psm_desc.reg = AW_PID_2308_SYSCTRL2_REG;
	aw_pa->psm_desc.mask = AW_PID_2308_PSM_EN_MASK;
	aw_pa->psm_desc.enable = AW_PID_2308_PSM_EN_ENABLE_VALUE;
	aw_pa->psm_desc.disable = AW_PID_2308_PSM_EN_DISABLE_VALUE;
	aw_pa->psm_desc.name = "btn 2308 psm";

	aw_pa->mpd_desc.reg = AW_PID_2308_SYSCTRL2_REG;
	aw_pa->mpd_desc.mask = AW_PID_2308_EN_MPD_MASK;
	aw_pa->mpd_desc.enable = AW_PID_2308_EN_MPD_ENABLE_VALUE;
	aw_pa->mpd_desc.disable = AW_PID_2308_EN_MPD_DISABLE_VALUE;
	aw_pa->mpd_desc.name = "btn 2308 mpd";

	aw_pa->dsmzth_desc.reg = AW_PID_2308_NGCTRL3_REG;
	aw_pa->dsmzth_desc.mask = AW_PID_2308_DSMZTH_MASK;
	aw_pa->dsmzth_desc.enable = AW_PID_2308_DSMZTH_21P33MS_VALUE;
	aw_pa->dsmzth_desc.disable = AW_PID_2308_DSMZTH_NO_RESET_VALUE;
	aw_pa->dsmzth_desc.name = "btn 2308 dsmzth";
}

/******************************************************
 *
 * aw882xx_init common
 *
 ******************************************************/
void aw_dev_init_cmm(struct aw_device *aw_pa)
{
	/*call aw device init func*/
	memset(aw_pa->monitor_name, 0, AW_NAME_MAX);

	aw_pa->vol_step = FADE_DEFAULT_VOL_STEP;
	aw_pa->prof_info.prof_desc = NULL;

	aw_pa->ops.aw_reg_val_to_db = aw_direct_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_direct_db_val_to_reg;
	aw_pa->ops.aw_get_irq_type = aw_get_irq_type;

	aw_pa->int_desc.mask_reg = AW_SYSINTM_REG;
	aw_pa->int_desc.mask_default = AW_SYSINTM_DEFAULT;
	aw_pa->int_desc.int_mask = AW_SYSINTM_DEFAULT;
	aw_pa->int_desc.st_reg = AW_SYSINT_REG;

	aw_pa->work_mode.reg = AW_SYSCTRL_REG;
	aw_pa->work_mode.mask = AW_RCV_MODE_MASK;
	aw_pa->work_mode.spk_val = AW_RCV_MODE_SPEAKER_VALUE;
	aw_pa->work_mode.rcv_val = AW_RCV_MODE_RECEIVER_VALUE;

	aw_pa->pwd_desc.reg = AW_SYSCTRL_REG;
	aw_pa->pwd_desc.mask = AW_PWDN_MASK;
	aw_pa->pwd_desc.enable = AW_PWDN_ENABLE_VALUE;
	aw_pa->pwd_desc.disable = AW_PWDN_DISABLE_VALUE;
	aw_pa->pwd_desc.name = "btn pwd";

	aw_pa->amppd_desc.reg = AW_SYSCTRL_REG;
	aw_pa->amppd_desc.mask = AW_AMPPD_MASK;
	aw_pa->amppd_desc.enable = AW_AMPPD_POWERDONE_VALUE;
	aw_pa->amppd_desc.disable = AW_AMPPD_WORK_VALUE;
	aw_pa->amppd_desc.name = "btn amppd";

	aw_pa->mute_desc.reg = AW_SYSCTRL_REG;
	aw_pa->mute_desc.mask = AW_HMUTE_MASK;
	aw_pa->mute_desc.enable = AW_HMUTE_ENABLE_VALUE;
	aw_pa->mute_desc.disable = AW_HMUTE_DISABLE_VALUE;
	aw_pa->mute_desc.name = "btn hmute";

	aw_pa->uls_hmute_desc.reg = AW_SYSCTRL_REG;
	aw_pa->uls_hmute_desc.mask = AW_ULS_HMUTE_MASK;
	aw_pa->uls_hmute_desc.enable = AW_ULS_HMUTE_ENABLE_VALUE;
	aw_pa->uls_hmute_desc.disable = AW_ULS_HMUTE_DISABLE_VALUE;
	aw_pa->uls_hmute_desc.name = "btn uls humte";

	aw_pa->txen_desc.reg = AW_SYSCTRL_REG;
	aw_pa->txen_desc.mask = AW_I2STXEN_MASK;
	aw_pa->txen_desc.enable = AW_I2STXEN_ENABLE_VALUE;
	aw_pa->txen_desc.disable = AW_I2STXEN_DISABLE_VALUE;

	aw_pa->sysst_desc.reg = AW_SYSST_REG;
	aw_pa->sysst_desc.mask = AW_SYSST_CHECK_MASK;
	aw_pa->sysst_desc.st_check = AW_NO_SWS_SYSST_CHECK;
	aw_pa->sysst_desc.st_sws_check = AW_SWS_SYSST_CHECK;
	aw_pa->sysst_desc.pll_check = AW_IIS_CHECK;

	aw_pa->spin_desc.rx_desc.reg = AW_I2SCTRL1_REG;
	aw_pa->spin_desc.rx_desc.mask = AW_CHSEL_MASK;
	aw_pa->spin_desc.rx_desc.left_val = AW_CHSEL_LEFT_VALUE;
	aw_pa->spin_desc.rx_desc.right_val = AW_CHSEL_RIGHT_VALUE;

	aw_pa->cco_mux_desc.reg = AW_DBGCTRL_REG;
	aw_pa->cco_mux_desc.mask = AW_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divided_val = AW_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass_val = AW_CCO_MUX_BYPASS_VALUE;

	aw_pa->voltage_desc.reg = AW_VBAT_REG;
	aw_pa->voltage_desc.int_bit = AW_MONITOR_INT_10BIT;
	aw_pa->voltage_desc.vbat_range = AW_MONITOR_VBAT_RANGE;

	aw_pa->temp_desc.reg = AW_TEMP_REG;
	aw_pa->temp_desc.neg_mask = AW_MONITOR_TEMP_NEG_MASK;
	aw_pa->temp_desc.sign_mask = AW_MONITOR_TEMP_SIGN_MASK;

	aw_pa->ipeak_desc.reg = AW_BSTCTRL2_REG;
	aw_pa->ipeak_desc.mask = AW_BST_IPEAK_MASK;

	aw_pa->volume_desc.reg = AW_SYSCTRL2_REG;
	aw_pa->volume_desc.mask = AW_VOL_MASK;
	aw_pa->volume_desc.shift = AW_VOL_START_BIT;
	aw_pa->volume_desc.mute_volume = AW_MUTE_VOL;
	aw_pa->volume_desc.ctl_volume = AW_VOL_DEFAULT_VALUE;
	aw_pa->vol_step = AW_VOL_STEP;

	aw_pa->bop_desc.reg = AW_SYSCTRL_REG;
	aw_pa->bop_desc.mask = AW_BOP_EN_MASK;
	aw_pa->bop_desc.enable = AW_BOP_EN_ENABLE_VALUE;
	aw_pa->bop_desc.disable = AW_BOP_EN_DISABLE_VALUE;
	aw_pa->bop_desc.name = "btn bop";

	aw_pa->soft_rst.reg = AW_SOFT_RST_REG;
	aw_pa->soft_rst.reg_value = AW_SOFT_RST_VAL;

	aw_pa->noise_gate_desc.reg = AW_NGCTRL3_REG;
	aw_pa->noise_gate_desc.mask = AW_NOISE_GATE_EN_MASK;
	aw_pa->noise_gate_desc.enable = AW_NOISE_GATE_EN_ENABLE_VALUE;
	aw_pa->noise_gate_desc.disable = AW_NOISE_GATE_EN_DISABLE_VALUE;
	aw_pa->noise_gate_desc.name = "btn noise gate";

	aw_pa->psm_desc.reg = AW_REG_NONE;
	aw_pa->psm_desc.name = "btn none psm";
	aw_pa->mpd_desc.reg = AW_REG_NONE;
	aw_pa->mpd_desc.name = "btn none mpd";
	aw_pa->dsmzth_desc.reg = AW_REG_NONE;
	aw_pa->dsmzth_desc.name = "btn none dsmzth";
	aw_pa->efcheck_desc.reg = AW_REG_NONE;
	aw_pa->dither_desc.reg = AW_REG_NONE;
	aw_pa->dither_desc.name = "btn none dither";
	aw_pa->vcalb_desc.vcalb_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.icalk_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.vcalk_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.icalkl_reg = AW_REG_NONE;
	aw_pa->vcalb_desc.vcalkl_reg = AW_REG_NONE;
	aw_pa->profctrl_desc.reg = AW_REG_NONE;
	aw_pa->bstctrl_desc.reg = AW_REG_NONE;

	aw_pa->ef_desc.count = 2;
	aw_pa->ef_desc.sequence[0].reg = AW_LOCKH_REG;
	aw_pa->ef_desc.sequence[0].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[0].check_val = AW_EF_LOCK_ENABLE_VALUE;
	aw_pa->ef_desc.sequence[1].reg = AW_LOCKL_REG;
	aw_pa->ef_desc.sequence[1].mask = AW_EF_LOCK_MASK;
	aw_pa->ef_desc.sequence[1].check_val = AW_EF_LOCK_ENABLE_VALUE;
}


int aw882xx_init(struct aw_device *aw_pa)
{
	int ret = 0;

	aw_dev_init_cmm(aw_pa);

	switch (aw_pa->chip_id) {
	case PID_1852_ID: {
		aw_pid_1852_dev_init(aw_pa);
	} break;
	case PID_2013_ID: {
		aw_pid_2013_dev_init(aw_pa);
	} break;
	case PID_2032_ID: {
		aw_pid_2032_dev_init(aw_pa);
	} break;
	case PID_2055_ID: {
		aw_pid_2055_dev_check(aw_pa);
	} break;
	case PID_2071_ID: {
		aw_pid_2071_dev_init(aw_pa);
	} break;
	case PID_2113_ID: {
		aw_pid_2113_dev_init(aw_pa);
	} break;
	case PID_2116_ID: {
		aw_pid_2116_dev_init(aw_pa);
	} break;
	case PID_2308_ID: {
		aw_pid_2308_dev_init(aw_pa);
	} break;
	default:
		aw_dev_err(aw_pa->dev, "unsupported chip id 0x%04x", aw_pa->chip_id);
		return -EINVAL;
	}

	ret = aw882xx_dev_check_ef_lock(aw_pa);
	if (ret < 0) {
		aw_dev_err(aw_pa->dev, "ef has not been locked");
		return ret;
	}

	return aw882xx_dev_probe(aw_pa);
}

