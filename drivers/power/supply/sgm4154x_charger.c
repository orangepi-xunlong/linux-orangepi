// SPDX-License-Identifier: GPL-2.0
// SGM4154x driver version 2021-09-09-003
// Copyright (C) 2021 Texas Instruments Incorporated - http://www.sg-micro.com

#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/usb/phy.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>

#include <linux/acpi.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "sgm4154x_charger.h"

static struct power_supply_desc sgm4154x_power_supply_desc;
static struct power_supply_desc sec_sgm4154x_power_supply_desc;
static struct reg_default sgm4154x_reg_defs[] = {
	{SGM4154x_CHRG_CTRL_0, 0x0a},
	{SGM4154x_CHRG_CTRL_1, 0x1a},
	{SGM4154x_CHRG_CTRL_2, 0x88},
	{SGM4154x_CHRG_CTRL_3, 0x22},
	{SGM4154x_CHRG_CTRL_4, 0x58},
	{SGM4154x_CHRG_CTRL_5, 0x9f},
	{SGM4154x_CHRG_CTRL_6, 0xe6},
	{SGM4154x_CHRG_CTRL_7, 0x4c},
	{SGM4154x_CHRG_STAT,   0x00},
	{SGM4154x_CHRG_FAULT,  0x00},
	{SGM4154x_CHRG_CTRL_a, 0x00},//
	{SGM4154x_CHRG_CTRL_b, 0x64},
	{SGM4154x_CHRG_CTRL_c, 0x75},
	{SGM4154x_CHRG_CTRL_d, 0x00},
	{SGM4154x_INPUT_DET,   0x00},
	{SGM4154x_CHRG_CTRL_f, 0x00},
};

/* SGM4154x REG06 BOOST_LIM[5:4], uV */
static const unsigned int BOOST_VOLT_LIMIT[] = {
	4850000, 5000000, 5150000, 5300000
};
 /* SGM4154x REG02 BOOST_LIM[7:7], uA */
#if defined(__SGM41542_CHIP_ID__)|| defined(__SGM41541_CHIP_ID__) || defined(__SGM41543D_CHIP_ID__)|| defined(__SGM41543_CHIP_ID__)
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	1200000, 2000000
};
#else
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	500000, 1200000
};
#endif

#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))

static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};
#endif

enum SGM4154x_VREG_FT {
	VREG_FT_DISABLE,
	VREG_FT_UP_8mV,
	VREG_FT_DN_8mV,
	VREG_FT_DN_16mV,
};

enum SGM4154x_VINDPM_OS {
	VINDPM_OS_3900mV,
	VINDPM_OS_5900mV,
	VINDPM_OS_7500mV,
	VINDPM_OS_10500mV,
};

enum SGM4154x_QC_VOLT {
	QC_20_5000mV,
	QC_20_9000mV,
	QC_20_12000mV,
};
#if 0
static enum power_supply_usb_type sgm4154x_usb_type[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
};
#endif
static int sgm4154x_usb_notifier(struct notifier_block *nb, unsigned long val,
				void *priv)
{
	struct sgm4154x_device *sgm =
			container_of(nb, struct sgm4154x_device, usb_nb);

	sgm->usb_event = val;

	queue_work(system_power_efficient_wq, &sgm->usb_work);

	return NOTIFY_OK;
}

static void sgm4154x_usb_work(struct work_struct *data)
{
	struct sgm4154x_device *sgm =
			container_of(data, struct sgm4154x_device, usb_work);

	switch (sgm->usb_event) {
	case USB_EVENT_ID:
		break;

	case USB_EVENT_NONE:
		power_supply_changed(sgm->charger);
		break;
	}

	return;

	dev_err(sgm->dev, "Error switching to charger mode.\n");
}
#if 0
static int sgm4154x_get_term_curr(struct sgm4154x_device *sgm)
{
	int ret;
	int reg_val;
	int offset = SGM4154x_TERMCHRG_I_MIN_uA;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val &= SGM4154x_TERMCHRG_CUR_MASK;
	reg_val = reg_val * SGM4154x_TERMCHRG_CURRENT_STEP_uA + offset;
	return reg_val;
}

static int sgm4154x_get_prechrg_curr(struct sgm4154x_device *sgm)
{
	int ret;
	int reg_val;
	int offset = SGM4154x_PRECHRG_I_MIN_uA;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val = (reg_val&SGM4154x_PRECHRG_CUR_MASK)>>4;
	reg_val = reg_val * SGM4154x_PRECHRG_CURRENT_STEP_uA + offset;
	return reg_val;
}

static int sgm4154x_get_ichg_curr(struct sgm4154x_device *sgm)
{
	int ret;
	int ichg;
	unsigned int curr;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_2, &ichg);
	if (ret)
		return ret;

	ichg &= SGM4154x_ICHRG_CUR_MASK;
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	if (ichg <= 0x8)
		curr = ichg * 5000;
	else if (ichg <= 0xF)
		curr = 40000 + (ichg - 0x8) * 10000;
	else if (ichg <= 0x17)
		curr = 110000 + (ichg - 0xF) * 20000;
	else if (ichg <= 0x20)
		curr = 270000 + (ichg - 0x17) * 30000;
	else if (ichg <= 0x30)
		curr = 540000 + (ichg - 0x20) * 60000;
	else if (ichg <= 0x3C)
		curr = 1500000 + (ichg - 0x30) * 120000;
	else
		curr = 3000000;
#else
	curr = ichg * SGM4154x_ICHRG_I_STEP_uA;
#endif
	return curr;
}
#endif
static int sgm4154x_set_term_curr(struct sgm4154x_device *sgm, int uA)
{
	int reg_val;

#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))

	for(reg_val = 1; reg_val < 16 && uA >= ITERM_CURRENT_STABLE[reg_val]; reg_val++)
		;
	reg_val--;
#else
	if (uA < SGM4154x_TERMCHRG_I_MIN_uA)
		uA = SGM4154x_TERMCHRG_I_MIN_uA;
	else if (uA > SGM4154x_TERMCHRG_I_MAX_uA)
		uA = SGM4154x_TERMCHRG_I_MAX_uA;

	reg_val = (uA - SGM4154x_TERMCHRG_I_MIN_uA) / SGM4154x_TERMCHRG_CURRENT_STEP_uA;
#endif
	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_3,
				  SGM4154x_TERMCHRG_CUR_MASK, reg_val);
}

static int sgm4154x_set_prechrg_curr(struct sgm4154x_device *sgm, int uA)
{
	int reg_val;

#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	for(reg_val = 1; reg_val < 16 && uA >= IPRECHG_CURRENT_STABLE[reg_val]; reg_val++)
		;
	reg_val--;
#else
	if (uA < SGM4154x_PRECHRG_I_MIN_uA)
		uA = SGM4154x_PRECHRG_I_MIN_uA;
	else if (uA > SGM4154x_PRECHRG_I_MAX_uA)
		uA = SGM4154x_PRECHRG_I_MAX_uA;

	reg_val = (uA - SGM4154x_PRECHRG_I_MIN_uA) / SGM4154x_PRECHRG_CURRENT_STEP_uA;
#endif
	reg_val = reg_val << 4;
	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_3,
				  SGM4154x_PRECHRG_CUR_MASK, reg_val);
}

static int sgm4154x_set_ichrg_curr(struct sgm4154x_device *sgm, int uA)
{
	int ret;
	int reg_val;

#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	if (uA <= 40000)
		reg_val = uA / 5000;
	else if (uA <= 110000)
		reg_val = 0x08 + (uA -40000) / 10000;
	else if (uA <= 270000)
		reg_val = 0x0F + (uA -110000) / 20000;
	else if (uA <= 540000)
		reg_val = 0x17 + (uA -270000) / 30000;
	else if (uA <= 1500000)
		reg_val = 0x20 + (uA -540000) / 60000;
	else if (uA <= 2940000)
		reg_val = 0x30 + (uA -1500000) / 120000;
	else
		reg_val = 0x3d;
#else
	if (uA < SGM4154x_ICHRG_I_MIN_uA)
		uA = SGM4154x_ICHRG_I_MIN_uA;
	else if ( uA > sgm->init_data.max_ichg)
		uA = sgm->init_data.max_ichg;

	reg_val = uA / SGM4154x_ICHRG_I_STEP_uA;
#endif

	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_2,
				  SGM4154x_ICHRG_CUR_MASK, reg_val);

	return ret;
}

static int sgm4154x_set_chrg_volt(struct sgm4154x_device *sgm, int chrg_volt)
{
	int ret;
	int reg_val;

	if (chrg_volt < SGM4154x_VREG_V_MIN_uV)
		chrg_volt = SGM4154x_VREG_V_MIN_uV;
	else if (chrg_volt > sgm->init_data.max_vreg)
		chrg_volt = sgm->init_data.max_vreg;


	reg_val = (chrg_volt-SGM4154x_VREG_V_MIN_uV) / SGM4154x_VREG_V_STEP_uV;
	reg_val = reg_val<<3;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_4,
				  SGM4154x_VREG_V_MASK, reg_val);

	return ret;
}

static int sgm4154x_set_jeita_config(struct sgm4154x_device *sgm)
{
	int ret, i;
	int reg_val;

	for(i = 0;i < JEITA_TEMP_T2_LEVEL;i++)
		if (sgm->data.temp_t2_thres == jeita_cool_thres[i]) {
			reg_val = i << 2;
			ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_c,
					JEITA_TEMP_T2_SET_MASK, reg_val);
			if (ret < 0)
				return ret;
			break;
		}
	if (i == JEITA_TEMP_T2_LEVEL) {
		pr_err("%s: jeita T2 temp wrong value.Please use the specified value\n",__func__);
		return -EINVAL;
	}

	for(i = 0;i < JEITA_TEMP_T3_LEVEL;i++)
		if (sgm->data.temp_t3_thres == jeita_warm_thres[i]) {
			reg_val = i;
			ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_c,
					JEITA_TEMP_T3_SET_MASK, reg_val);
			if(ret < 0)
				return ret;
			break;
		}
	if (i == JEITA_TEMP_T3_LEVEL) {
		pr_err("%s: jeita T3 temp wrong value.Please use the specified value\n",__func__);
		return -EINVAL;
	}

	for(i = 0;i < JEITA_CUR_T2_LEVEL;i++)
		if ((sgm->data.jeita_temp_t1_to_t2_cc_current * 100 / sgm->data.jeita_temp_t2_to_t3_cc_current)
				== jeita_cool_cur_perc[i]) {
			reg_val = i;
			regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_c,
					JEITA_CUR_T2_EN_MASK, 1 << 6);
			ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_5,
					JEITA_CUR_T2_SET_MASK, reg_val);
			if (ret < 0)
				return ret;
			break;
		}
	if (i == JEITA_CUR_T2_LEVEL) {
		pr_err("%s: jeita T1-T2 charge current wrong value.Please use the specified value\n",__func__);
		return -EINVAL;
	}

	for(i = 0;i < JEITA_CUR_T3_LEVEL;i++)
		if ((sgm->data.jeita_temp_t3_to_t4_cc_current * 100 / sgm->data.jeita_temp_t2_to_t3_cc_current)
				== jeita_warm_cur_perc[i]) {
			reg_val = i << 4;
			ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_c,
					JEITA_CUR_T3_SET_MASK, reg_val);
			if (ret < 0)
				return ret;
			break;
		}
	if (i == JEITA_CUR_T3_LEVEL) {
		pr_err("%s: jeita T3-T4 charge current wrong value.Please use the specified value\n",__func__);
		return -EINVAL;
	}

	if (sgm->data.jeita_temp_t3_to_t4_cv == sgm->data.jeita_temp_t2_to_t3_cv)
		reg_val = JEITA_TEMP_NORMAL_CV_H;
	else if (sgm->data.jeita_temp_t3_to_t4_cv < sgm->data.jeita_temp_t2_to_t3_cv)
		reg_val = JEITA_TEMP_LOW_CV_H;
	else {
		pr_err("%s: jeita T3 charge voltage wrong value.Please use the specified value\n",__func__);
		return -EINVAL;
	}
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_7,
			JEITA_TEMP_CV_H_MASK, reg_val << 4);
	if (ret < 0)
		return ret;

	if (sgm->data.jeita_temp_t1_to_t2_cv == sgm->data.jeita_temp_t2_to_t3_cv)
		reg_val = JEITA_TEMP_NORMAL_CV_L;
	else if (sgm->data.jeita_temp_t1_to_t2_cv < sgm->data.jeita_temp_t2_to_t3_cv)
		reg_val = JEITA_TEMP_LOW_CV_L;
	else {
		pr_err("%s: jeita T1 charge voltage wrong value.Please use the specified value\n",__func__);
		return -EINVAL;
	}
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_c,
			JEITA_TEMP_CV_L_MASK, reg_val << 7);
	if (ret < 0)
		return ret;

	return ret;
}
#if 0
static int sgm4154x_get_chrg_volt(struct sgm4154x_device *sgm)
{
	int ret;
	int vreg_val, chrg_volt;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_4, &vreg_val);
	if (ret)
		return ret;

	vreg_val = (vreg_val & SGM4154x_VREG_V_MASK)>>3;

	if (15 == vreg_val)
		chrg_volt = 4352000; //default
	else if (vreg_val < 25)
		chrg_volt = vreg_val*SGM4154x_VREG_V_STEP_uV + SGM4154x_VREG_V_MIN_uV;

	return chrg_volt;
}
#endif
#if 0//(defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__) || defined(__SGM41543D_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
static int sgm4154x_enable_qc20_hvdcp_9v(struct sgm4154x_device *sgm)
{
	int ret;
	int dp_val, dm_val;

	/*dp and dm connected,dp 0.6V dm Hiz*/
    dp_val = 0x2<<3;
    ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 0.6V
    if (ret)
        return ret;

	dm_val = 0;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm Hiz
    if (ret)
        return ret;
    //mdelay(1000);
	msleep(1400);

	/* dp 3.3v and dm 0.6v out 9V */
	dp_val = SGM4154x_DP_VSEL_MASK;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 3.3v
	if (ret)
		return ret;

	dm_val = 0x2<<1;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm 0.6v

	return ret;
}

static int sgm4154x_enable_qc20_hvdcp_12v(struct sgm4154x_device *sgm)
{
	int ret;
	int dp_val, dm_val;

	/*dp and dm connected,dp 0.6V dm Hiz*/
    dp_val = 0x2<<3;
    ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 0.6V
    if (ret)
        return ret;

	dm_val = 0<<1;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm Hiz
    if (ret)
        return ret;
    //mdelay(1000);
	msleep(1400);

	/* dp 0.6v and dm 0.6v out 12V */
	dp_val = 0x2<<3;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 0.6v
	if (ret)
		return ret;
	//mdelay(1250);

	dm_val = 0x2<<1;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm 0.6v

	return ret;
}

/* step 1. entry QC3.0 mode
   step 2. up or down 200mv
   step 3. retry step 2 */
static int sgm4154x_enable_qc30_hvdcp(struct sgm4154x_device *sgm)
{
	int ret;
	int dp_val, dm_val;

	dp_val = 0x2<<3;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 0.6v
	if (ret)
		return ret;

	dm_val = 0;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm Hiz
    if (ret)
        return ret;
    //mdelay(1000);
	msleep(1400);
	dm_val = 0x2;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm 0V
	mdelay(10);

	dm_val = SGM4154x_DM_VSEL_MASK;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm 3.3v

	return ret;
}

// Must enter 3.0 mode to call ,otherwise cannot step correctly.
static int sgm4154x_qc30_step_up_vbus(struct sgm4154x_device *sgm)
{
	int ret;
	int dp_val;

	/*  dm 3.3v to dm 0.6v  step up 200mV when IC is QC3.0 mode*/
	dp_val = SGM4154x_DP_VSEL_MASK;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 3.3v
	if (ret)
		return ret;

	dp_val = 0x2<<3;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DP_VSEL_MASK, dp_val); //dp 0.6v
	if (ret)
		return ret;

	udelay(100);
	return ret;
}
// Must enter 3.0 mode to call ,otherwise cannot step correctly.
static int sgm4154x_qc30_step_down_vbus(struct sgm4154x_device *sgm)
{
	int ret;
	int dm_val;

	/* dp 0.6v and dm 0.6v step down 200mV when IC is QC3.0 mode*/
	dm_val = 0x2<<1;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm 0.6V
    if (ret)
        return ret;

	dm_val = SGM4154x_DM_VSEL_MASK;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_d,
				  SGM4154x_DM_VSEL_MASK, dm_val); //dm 3.3v
	udelay(100);

	return ret;
}


// fine tuning termination voltage,to Improve accuracy
static int sgm4154x_vreg_fine_tuning(struct sgm4154x_device *sgm,enum SGM4154x_VREG_FT ft)
{
	int ret;
	int reg_val;

	switch(ft) {
		case VREG_FT_DISABLE:
			reg_val = 0;
			break;

		case VREG_FT_UP_8mV:
			reg_val = SGM4154x_VREG_FT_UP_8mV;
			break;

		case VREG_FT_DN_8mV:
			reg_val = SGM4154x_VREG_FT_DN_8mV;
			break;

		case VREG_FT_DN_16mV:
			reg_val = SGM4154x_VREG_FT_DN_16mV;
			break;

		default:
			reg_val = 0;
			break;
	}
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_f,
				  SGM4154x_VREG_FT_MASK, reg_val);
	pr_debug("%s reg_val:%d\n",__func__,reg_val);

	return ret;
}

#endif
static int sgm4154x_get_vindpm_offset_os(struct sgm4154x_device *sgm)
{
	int ret;
	int reg_val;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_f, &reg_val);
	if (ret)
		return ret;

	reg_val = reg_val & SGM4154x_VINDPM_OS_MASK;

	return reg_val;
}


static int sgm4154x_get_input_volt_lim(struct sgm4154x_device *sgm)
{
	int ret;
	int offset;
	int vlim;
	int temp;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_6, &vlim);
	if (ret)
		return ret;

	temp = sgm4154x_get_vindpm_offset_os(sgm);
	if (VINDPM_OS_3900mV == temp)
		offset = 3900000; //uv
	else if (VINDPM_OS_5900mV == temp)
		offset = 5900000;
	else if (VINDPM_OS_7500mV == temp)
		offset = 7500000;
	else if (VINDPM_OS_10500mV == temp)
		offset = 10500000;

	vlim = offset + (vlim & 0x0F) * SGM4154x_VINDPM_STEP_uV;
	return vlim;
}


static int sgm4154x_set_vindpm_offset_os(struct sgm4154x_device *sgm,enum SGM4154x_VINDPM_OS offset_os)
{
	int ret;


	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_f,
				  SGM4154x_VINDPM_OS_MASK, offset_os);

	if (ret){
		pr_err("%s fail\n",__func__);
		return ret;
	}

	return ret;
}

static int sgm4154x_set_input_volt_lim(struct sgm4154x_device *sgm, unsigned int vindpm)
{
	int ret;
	unsigned int offset;
	u8 reg_val;
	enum SGM4154x_VINDPM_OS os_val;

	if (vindpm < SGM4154x_VINDPM_V_MIN_uV ||
	    vindpm > SGM4154x_VINDPM_V_MAX_uV)
 		return -EINVAL;

	if (vindpm < 5900000){
		os_val = VINDPM_OS_3900mV;
		offset = 3900000;
	}
	else if (vindpm >= 5900000 && vindpm < 7500000){
		os_val = VINDPM_OS_5900mV;
		offset = 5900000; //uv
	}
	else if (vindpm >= 7500000 && vindpm < 10500000){
		os_val = VINDPM_OS_7500mV;
		offset = 7500000; //uv
	}
	else{
		os_val = VINDPM_OS_10500mV;
		offset = 10500000; //uv
	}

	sgm4154x_set_vindpm_offset_os(sgm,os_val);
	reg_val = (vindpm - offset) / SGM4154x_VINDPM_STEP_uV;

	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_VINDPM_V_MASK, reg_val);

	return ret;
}

bool sgm4154x_is_hvdcp(struct sgm4154x_device *sgm,enum SGM4154x_QC_VOLT val)
{
    int i = 20;
	int vlim;
	int temp;

	vlim = sgm4154x_get_input_volt_lim(sgm);

	if (QC_20_9000mV == val)
	{
		sgm4154x_set_input_volt_lim(sgm,8000000); //8v
	}
	else if (QC_20_12000mV == val)
	{
		sgm4154x_set_input_volt_lim(sgm,11000000); //11v
	}
	else
	{
		sgm4154x_set_input_volt_lim(sgm,4500000); //4.5v
		return 0;
	}
	mdelay(1);
	while(i--){
		regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_a, &temp);

		if(0 == (temp&0x40)){
			return 1;
		}
		else if(1 == !!(temp&0x40) && i == 1){
			sgm4154x_set_input_volt_lim(sgm,vlim);
			return 0;
		}
		mdelay(10);
	}
	return 0;
}

static int sgm4154x_set_input_curr_lim(struct sgm4154x_device *sgm, int iindpm)
{

	int reg_val;

	if (iindpm < SGM4154x_IINDPM_I_MIN_uA ||
			iindpm > SGM4154x_IINDPM_I_MAX_uA)
		return -EINVAL;
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	reg_val = (iindpm-SGM4154x_IINDPM_I_MIN_uA) / SGM4154x_IINDPM_STEP_uA;
#else
	if (iindpm >= SGM4154x_IINDPM_I_MIN_uA && iindpm <= 3100000)//default
		reg_val = (iindpm-SGM4154x_IINDPM_I_MIN_uA) / SGM4154x_IINDPM_STEP_uA;
	else if (iindpm > 3100000 && iindpm < SGM4154x_IINDPM_I_MAX_uA)
		reg_val = 0x1E;
	else
		reg_val = 0x1F;
#endif
	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_0,
				  SGM4154x_IINDPM_I_MASK, reg_val);
}

static int sgm4154x_get_input_curr_lim(struct sgm4154x_device *sgm)
{
	int ret;
	int ilim;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_0, &ilim);
	if (ret)
		return ret;
	if (SGM4154x_IINDPM_I_MASK == (ilim & SGM4154x_IINDPM_I_MASK))
		return SGM4154x_IINDPM_I_MAX_uA;

	ilim = (ilim & SGM4154x_IINDPM_I_MASK)*SGM4154x_IINDPM_STEP_uA + SGM4154x_IINDPM_I_MIN_uA;

	return ilim;
}

static int sgm4154x_set_watchdog_timer(struct sgm4154x_device *sgm, int time)
{
	int ret;
	u8 reg_val;

	if (time == 0)
		reg_val = SGM4154x_WDT_TIMER_DISABLE;
	else if (time == 40)
		reg_val = SGM4154x_WDT_TIMER_40S;
	else if (time == 80)
		reg_val = SGM4154x_WDT_TIMER_80S;
	else
		reg_val = SGM4154x_WDT_TIMER_160S;

	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_5,
				SGM4154x_WDT_TIMER_MASK, reg_val);

	return ret;
}
#if 0
static int sgm4154x_set_wdt_rst(struct sgm4154x_device *sgm, bool is_rst)
{
	int val = 0;

	if (is_rst)
		val = SGM4154x_WDT_RST_MASK;
	else
		val = 0;
	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_1,
				  SGM4154x_WDT_RST_MASK, val);
}
#endif
static int sgm4154x_get_state(struct sgm4154x_device *sgm,
			     struct sgm4154x_state *state)
{
	int chrg_stat;
	int fault;
	int chrg_param_0,chrg_param_1,chrg_param_2;
	int ret;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_STAT, &chrg_stat);
	if (ret){
		ret = regmap_read(sgm->regmap, SGM4154x_CHRG_STAT, &chrg_stat);
		if (ret){
			pr_err("%s read SGM4154x_CHRG_STAT fail\n",__func__);
			return ret;
		}
	}

	state->chrg_type = chrg_stat & SGM4154x_VBUS_STAT_MASK;
	state->chrg_stat = chrg_stat & SGM4154x_CHG_STAT_MASK;
	state->online = !!(chrg_stat & SGM4154x_PG_STAT);
	state->therm_stat = !!(chrg_stat & SGM4154x_THERM_STAT);
	state->vsys_stat = !!(chrg_stat & SGM4154x_VSYS_STAT);

	pr_debug("%s chrg_stat =%d,vbus_status =%d online = %d\n",__func__,chrg_stat,state->chrg_type,state->online);


	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_FAULT, &fault);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_FAULT fail\n",__func__);
		return ret;
	}
	state->chrg_fault = fault;
	state->ntc_fault = fault & SGM4154x_TEMP_MASK;
	state->health = state->ntc_fault;
	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_0, &chrg_param_0);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_0 fail\n",__func__);
		return ret;
	}
	state->hiz_en = !!(chrg_param_0 & SGM4154x_HIZ_EN);

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_5, &chrg_param_1);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_5 fail\n",__func__);
		return ret;
	}
	state->term_en = !!(chrg_param_1 & SGM4154x_TERM_EN);

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_a, &chrg_param_2);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_a fail\n",__func__);
		return ret;
	}
	state->vbus_gd = !!(chrg_param_2 & SGM4154x_VBUS_GOOD);

	return 0;
}
#if 0
static int sgm4154x_set_hiz_en(struct sgm4154x_device *sgm, bool hiz_en)
{
	int reg_val;

	dev_notice(sgm->dev, "%s:%d", __func__, hiz_en);
	reg_val = hiz_en ? SGM4154x_HIZ_EN : 0;

	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_0,
				  SGM4154x_HIZ_EN, reg_val);
}
#endif
int sgm4154x_enable_charger(struct sgm4154x_device *sgm)
{
    int ret;
    printk("sgm4154x_enable_charger\n");
    ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_1, SGM4154x_CHRG_EN,
                     SGM4154x_CHRG_EN);

    return ret;
}

int sgm4154x_disable_charger(struct sgm4154x_device *sgm)
{
    int ret;
    printk("sgm4154x_disable_charger\n");
    ret =
        regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_1, SGM4154x_CHRG_EN,
                     0);
    return ret;
}
#if 0
float sgm4154x_get_charger_output_power(struct sgm4154x_device *sgm)
{
    int ret;
	int i = 0x1F;
	int j = 0;
	int vlim;
	int ilim;
	int temp;
	int offset;
	int output_volt;
	int output_curr;
	float o_i,o_v;

    ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_6, &vlim); //read default setting to save
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_6 fail\n",__func__);
		return ret;
	}

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_0, &ilim); //read default setting to save
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_0 fail\n",__func__);
		return ret;
	}


	regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_VINDPM_V_MASK, 0);
	while(i--){

		regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_0,
				  SGM4154x_IINDPM_I_MASK, i);
		mdelay(50);
		ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_a, &temp);
		if (ret){
			pr_err("%s read SGM4154x_CHRG_CTRL_a fail\n",__func__);
			return ret;
		}
		if (1 == !!(temp&0x20)){
			output_curr = 100 + i*100; //mA
			if (0x1F == i)
				output_curr = 3800;	      //mA
		}
	}

	regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_0,
				  SGM4154x_IINDPM_I_MASK, SGM4154x_IINDPM_I_MASK);
	for(j = 0;j <= 0xF;j ++)
	{
		regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_VINDPM_V_MASK, j);

		mdelay(10);
		ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_a, &temp);
		if (ret){
			pr_err("%s read SGM4154x_CHRG_CTRL_a fail\n",__func__);
			return ret;
		}

		if (1 == !!(temp&0x40)){

			temp = sgm4154x_get_vindpm_offset_os(sgm);
			if (0 == temp)
				offset = 3900;  //mv
			else if (1 == temp)
				offset = 5900;  //mv
			else if (2 == temp)
				offset = 7500;  //mv
			else if (3 == temp)
				offset = 10500; //mv
			output_volt = offset + j*100; //mv
		}

	}
	o_i = (float)output_curr/1000;
	o_v = (float)output_volt/1000;
    return o_i * o_v;
}
#endif

static int sgm4154x_set_vac_ovp(struct sgm4154x_device *sgm)
{
	int reg_val;

	dev_notice(sgm->dev, "%s", __func__);
	reg_val = 0xFF & SGM4154x_VAC_OVP_MASK;

	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_VAC_OVP_MASK, reg_val);
}

static int sgm4154x_set_recharge_volt(struct sgm4154x_device *sgm, int recharge_volt)
{
	int reg_val;
	dev_notice(sgm->dev, "%s:%d", __func__, recharge_volt);
	reg_val = (recharge_volt - SGM4154x_VRECHRG_OFFSET_mV) / SGM4154x_VRECHRG_STEP_mV;

	return regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_4,
				  SGM4154x_VRECHARGE, reg_val);
}

#if 0//(defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__) || defined(__SGM41543D_CHIP_ID__)|| defined(__SGM41513D_CHIP_ID__))
static int get_charger_type(struct sgm4154x_device * sgm)
{
	enum power_supply_usb_type usb_type;
	switch(sgm->state.chrg_type) {
		case SGM4154x_USB_SDP:
			sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB;
			usb_type = POWER_SUPPLY_USB_TYPE_SDP;
			break;

		case SGM4154x_USB_CDP:
			sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
			usb_type = POWER_SUPPLY_USB_TYPE_CDP;
			break;

		case SGM4154x_USB_DCP:
			sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			usb_type = POWER_SUPPLY_USB_TYPE_DCP;
			break;

		case SGM4154x_NON_STANDARD:
			sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;

		default:
			sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
	}
	pr_err("%s psy.type:%d,usb_type:%d\n",__func__,sgm4154x_power_supply_desc.type,usb_type);
	return usb_type;
}
#endif
static int sgm4154x_property_is_writeable(struct power_supply *psy,
					 enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
	//case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		return true;
	default:
		return false;
	}
}
static int sgm4154x_charger_set_property(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *val)
{
	struct sgm4154x_device *sgm = power_supply_get_drvdata(psy);
	int ret = -EINVAL;

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = sgm4154x_set_input_curr_lim(sgm, val->intval);
		break;
#if 0
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		if (val->intval)
			ret = sgm4154x_enable_charger(sgm);
		else
			ret = sgm4154x_disable_charger(sgm);
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_set_input_volt_lim(sgm, val->intval);
		break;

#endif
	default:
		return -EINVAL;
	}

	return ret;
}

static int sgm4154x_charger_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sgm4154x_device *sgm = power_supply_get_drvdata(psy);
	struct sgm4154x_state state;
	int ret = 0;

	mutex_lock(&sgm->lock);
	//ret = sgm4154x_get_state(sgm, &state);
	state = sgm->state;
	mutex_unlock(&sgm->lock);
	if (ret)
		return ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.chrg_type || (state.chrg_type == SGM4154x_OTG_MODE))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (!state.chrg_stat)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.chrg_stat == SGM4154x_TERM_CHRG)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch (state.chrg_stat) {
		case SGM4154x_PRECHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SGM4154x_FAST_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case SGM4154x_TERM_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SGM4154x_NOT_CHRGING:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		default:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		}
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = SGM4154x_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = SGM4154x_NAME;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.online;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = state.vbus_gd;
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = sgm4154x_power_supply_desc.type;
		break;
/*	case POWER_SUPPLY_PROP_USB_TYPE:
#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__) || defined(__SGM41543D_CHIP_ID__)|| defined(__SGM41513D_CHIP_ID__))
		val->intval = get_charger_type(sgm);
#endif
		break;
*/
	case POWER_SUPPLY_PROP_HEALTH:
		if (state.chrg_fault & 0xF8)
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;

		switch (state.health) {
		case SGM4154x_TEMP_HOT:
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			break;
		case SGM4154x_TEMP_WARM:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SGM4154x_TEMP_COOL:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SGM4154x_TEMP_COLD:
			val->intval = POWER_SUPPLY_HEALTH_COLD;
			break;
		}
		break;

	//case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		//val->intval = state.vbus_adc;
		//break;

	//case POWER_SUPPLY_PROP_CURRENT_NOW:
		//val->intval = state.ibus_adc;
		//break;

/*	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_get_input_volt_lim(sgm);
		if (ret < 0)
			return ret;

		val->intval = ret;
		break;*/

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = sgm4154x_get_input_curr_lim(sgm);
		if (ret < 0)
			return ret;

		val->intval = ret;
		break;
/*
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = !state.hiz_en;
		break;
*/
	default:
		return -EINVAL;
	}

	return ret;
}


#if 0
static bool sgm4154x_state_changed(struct sgm4154x_device *sgm,
				  struct sgm4154x_state *new_state)
{
	struct sgm4154x_state old_state;

	mutex_lock(&sgm->lock);
	old_state = sgm->state;
	mutex_unlock(&sgm->lock);

	return (old_state.chrg_type != new_state->chrg_type ||
		old_state.chrg_stat != new_state->chrg_stat	||
		old_state.online != new_state->online		||
		old_state.therm_stat != new_state->therm_stat	||
		old_state.vsys_stat != new_state->vsys_stat	||
		old_state.chrg_fault != new_state->chrg_fault
		);
}
#endif

static void sgm4154x_dump_register(struct sgm4154x_device * sgm)
{
	int i = 0;
	int reg = 0;

	for(i=0; i<=SGM4154x_CHRG_CTRL_f; i++) {
		regmap_read(sgm->regmap, i, &reg);
		pr_debug("%s REG%x    %X\n", __func__, i, reg);
	}
}

#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__)|| defined(__SGM41513D_CHIP_ID__)|| defined(__SGM41513A_CHIP_ID__))
static bool sgm4154x_dpdm_detect_is_done(struct sgm4154x_device * sgm)
{
	int chrg_stat;
	int ret;

	ret = regmap_read(sgm->regmap, SGM4154x_INPUT_DET, &chrg_stat);
	if(ret) {
		dev_err(sgm->dev, "Check DPDM detecte error\n");
	}

	return (chrg_stat&SGM4154x_DPDM_ONGOING)?true:false;
}
#endif

static void charger_monitor_work_func(struct work_struct *work)
{
	int ret = 0;
	struct sgm4154x_device * sgm = NULL;
	struct delayed_work *charge_monitor_work = NULL;
	//static u8 last_chg_method = 0;
	struct sgm4154x_state state;

	charge_monitor_work = container_of(work, struct delayed_work, work);
	if(charge_monitor_work == NULL) {
		pr_err("Cann't get charge_monitor_work\n");
		return ;
	}
	sgm = container_of(charge_monitor_work, struct sgm4154x_device, charge_monitor_work);
	if(sgm == NULL) {
		pr_err("Cann't get sgm \n");
		return ;
	}

	ret = sgm4154x_get_state(sgm, &state);
	mutex_lock(&sgm->lock);
	sgm->state = state;
	mutex_unlock(&sgm->lock);

	sgm4154x_dump_register(sgm);
	pr_debug("%s\n",__func__);
	schedule_delayed_work(&sgm->charge_monitor_work, 10*HZ);
}

static void charger_detect_work_func(struct work_struct *work)
{
	struct delayed_work *charge_detect_delayed_work = NULL;
	struct sgm4154x_device * sgm = NULL;
	//static int charge_type_old = 0;
	int curr_in_limit = 0;
	struct sgm4154x_state state;
	int ret;

	charge_detect_delayed_work = container_of(work, struct delayed_work, work);
	if(charge_detect_delayed_work == NULL) {
		pr_err("Cann't get charge_detect_delayed_work\n");
		return;
	}
	sgm = container_of(charge_detect_delayed_work, struct sgm4154x_device, charge_detect_delayed_work);
	if(sgm == NULL) {
		pr_err("Cann't get sgm4154x_device\n");
		return;
	}

	if (!sgm->charger_wakelock->active)
		__pm_stay_awake(sgm->charger_wakelock);

	ret = sgm4154x_get_state(sgm, &state);
	mutex_lock(&sgm->lock);
	sgm->state = state;
	mutex_unlock(&sgm->lock);

	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "Vbus not present, disable charge\n");
		sgm4154x_disable_charger(sgm);
		goto err;
	}
	if(!state.online)
	{
		dev_err(sgm->dev, "Vbus not online\n");
		goto err;
	}

#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__)|| defined(__SGM41513D_CHIP_ID__)|| defined(__SGM41513A_CHIP_ID__))
	if(!sgm4154x_dpdm_detect_is_done(sgm)) {
		dev_err(sgm->dev, "DPDM detecte not done, disable charge\n");
		sgm4154x_disable_charger(sgm);
		goto err;
	}
	switch(sgm->state.chrg_type) {
		case SGM4154x_USB_SDP:
			pr_err("SGM4154x charger type: SDP\n");
			curr_in_limit = 500000;
			break;

		case SGM4154x_USB_CDP:
			pr_err("SGM4154x charger type: CDP\n");
			curr_in_limit = 1500000;
			break;

		case SGM4154x_USB_DCP:
			pr_err("SGM4154x charger type: DCP can select up volt\n");
			curr_in_limit = 2000000;
			//sgm4154x_enable_qc20_hvdcp_9v(sgm);
			//sgm4154x_is_hvdcp(sgm,0);
			break;

		case SGM4154x_UNKNOWN:
			pr_err("SGM4154x charger type: UNKNOWN\n");
			curr_in_limit = 500000;
			break;
		case SGM4154x_NON_STANDARD:
			pr_err("SGM4154x charger type: NON_STANDARD\n");
			curr_in_limit = 1000000;
			break;

		case SGM4154x_OTG_MODE:
			pr_err("SGM4154x OTG mode do nothing\n");
			goto err;

		default:
			pr_err("SGM4154x charger type: default\n");
			//curr_in_limit = 500000;
			//break;
			return;
	}

	//set charge parameters
	dev_err(sgm->dev, "Update: curr_in_limit = %d\n", curr_in_limit);
	sgm4154x_set_input_curr_lim(sgm, curr_in_limit);
#endif
	//enable charge
#if defined(__SGM41513_CHIP_ID__)
	curr_in_limit = 1000000;
	sgm4154x_set_input_curr_lim(sgm, curr_in_limit);
#endif
	sgm4154x_enable_charger(sgm);
	sgm4154x_dump_register(sgm);
err:
	//release wakelock
	power_supply_changed(sgm->charger);
	dev_dbg(sgm->dev, "Relax wakelock\n");
	__pm_relax(sgm->charger_wakelock);
	return;
}

static irqreturn_t sgm4154x_irq_handler_thread(int irq, void *private)
{
	struct sgm4154x_device *sgm = private;

	//lock wakelock
	pr_debug("%s entry\n",__func__);
	schedule_delayed_work(&sgm->charge_detect_delayed_work, 100);
	//power_supply_changed(sgm->charger);

	return IRQ_HANDLED;
}

static enum power_supply_property sgm4154x_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	//POWER_SUPPLY_PROP_VOLTAGE_NOW,
	//POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	//POWER_SUPPLY_PROP_USB_TYPE,
	//POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_PRESENT
};

static char *sgm4154x_charger_supplied_to[] = {
	"battery",
};

static struct power_supply_desc sgm4154x_power_supply_desc = {
	.name = "sgm4154x-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	//.usb_types = sgm4154x_usb_type,
	//.num_usb_types = ARRAY_SIZE(sgm4154x_usb_type),
	.properties = sgm4154x_power_supply_props,
	.num_properties = ARRAY_SIZE(sgm4154x_power_supply_props),
	.get_property = sgm4154x_charger_get_property,
	.set_property = sgm4154x_charger_set_property,
	.property_is_writeable = sgm4154x_property_is_writeable,
};

static struct power_supply_desc sec_sgm4154x_power_supply_desc = {
	.name = "secondary-sgm4154x-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	//.usb_types = sgm4154x_usb_type,
	//.num_usb_types = ARRAY_SIZE(sgm4154x_usb_type),
	.properties = sgm4154x_power_supply_props,
	.num_properties = ARRAY_SIZE(sgm4154x_power_supply_props),
	.get_property = sgm4154x_charger_get_property,
	.set_property = sgm4154x_charger_set_property,
	.property_is_writeable = sgm4154x_property_is_writeable,
};


static bool sgm4154x_is_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case SGM4154x_CHRG_CTRL_0...SGM4154x_CHRG_CTRL_f:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config sgm4154x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = SGM4154x_CHRG_CTRL_f,
	.reg_defaults	= sgm4154x_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(sgm4154x_reg_defs),
	.cache_type = REGCACHE_RBTREE,

	.volatile_reg = sgm4154x_is_volatile_reg,
};

static int sgm4154x_power_supply_init(struct sgm4154x_device *sgm,
							struct device *dev)
{
	struct power_supply_config psy_cfg = { .drv_data = sgm,
						.of_node = dev->of_node, };

	psy_cfg.supplied_to = sgm4154x_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(sgm4154x_charger_supplied_to);

	if(of_get_property(dev->of_node, "primary-charger",NULL))
		sgm->charger = devm_power_supply_register(sgm->dev,
						 &sgm4154x_power_supply_desc,
						 &psy_cfg);
	else if(of_get_property(dev->of_node, "secondary-charger",NULL))
		sgm->charger = devm_power_supply_register(sgm->dev,
						 &sec_sgm4154x_power_supply_desc,
						 &psy_cfg);
	if (IS_ERR(sgm->charger))
		return -EINVAL;

	return 0;
}

static int sgm4154x_hw_init(struct sgm4154x_device *sgm)
{
	int ret = 0;
	int val = 0;
	struct power_supply_battery_info bat_info = { };

	if(of_property_read_u32(sgm->dev->of_node, "sgm415xx-ichrg-uA", &val) >= 0)
		bat_info.constant_charge_current_max_ua = val;
	else
		bat_info.constant_charge_current_max_ua =
			SGM4154x_ICHRG_I_DEF_uA;

	if(of_property_read_u32(sgm->dev->of_node, "sgm415xx-vchrg-uV", &val) >= 0)
		bat_info.constant_charge_voltage_max_uv = val;
	else
		bat_info.constant_charge_voltage_max_uv =
			SGM4154x_VREG_V_DEF_uV;

	if(of_property_read_u32(sgm->dev->of_node, "sgm415xx-prechrg-uA", &val) >= 0)
		bat_info.precharge_current_ua = val;
	else
		bat_info.precharge_current_ua =
			SGM4154x_PRECHRG_I_DEF_uA;

	if(of_property_read_u32(sgm->dev->of_node, "sgm415xx-termchrg-uA", &val) >= 0)
		bat_info.charge_term_current_ua = val;
	else
		bat_info.charge_term_current_ua =
			SGM4154x_TERMCHRG_I_DEF_uA;

	sgm->init_data.max_ichg =
			SGM4154x_ICHRG_I_MAX_uA;

	sgm->init_data.max_vreg =
			SGM4154x_VREG_V_MAX_uV;

	sgm4154x_set_watchdog_timer(sgm,0);

	ret = sgm4154x_set_ichrg_curr(sgm,
				bat_info.constant_charge_current_max_ua);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_prechrg_curr(sgm, bat_info.precharge_current_ua);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_chrg_volt(sgm,
				bat_info.constant_charge_voltage_max_uv);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_term_curr(sgm, bat_info.charge_term_current_ua);
	if (ret)
		goto err_out;

	/*ret = sgm4154x_set_input_volt_lim(sgm, sgm->init_data.vlim);
	if (ret)
		goto err_out;*/

	ret = sgm4154x_set_input_curr_lim(sgm, sgm->init_data.ilim);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_vac_ovp(sgm);//14V
	if (ret)
		goto err_out;

	ret = sgm4154x_set_recharge_volt(sgm, 200);//100~200mv
	if (ret)
		goto err_out;

	if (sgm->enable_sw_jeita) {
		ret = sgm4154x_set_jeita_config(sgm);
		if (ret)
			goto err_out;
	}

	dev_dbg(sgm->dev, "ichrg_curr:%d prechrg_curr:%d chrg_vol:%d"
		" term_curr:%d input_curr_lim:%d",
		bat_info.constant_charge_current_max_ua,
		bat_info.precharge_current_ua,
		bat_info.constant_charge_voltage_max_uv,
		bat_info.charge_term_current_ua,
		sgm->init_data.ilim);

	return 0;

err_out:
	return ret;
}

static int sgm4154x_parse_dt(struct sgm4154x_device *sgm)
{
	int ret;
	u32 val = 0;
	int irq_gpio = 0, irqn = 0;
	int chg_en_gpio = 0;
	struct gpio_desc *nqon;
	#if 0
	ret = device_property_read_u32(sgm->dev, "watchdog-timer",
				       &sgm->watchdog_timer);
	if (ret)
		sgm->watchdog_timer = SGM4154x_WATCHDOG_DIS;

	if (sgm->watchdog_timer > SGM4154x_WATCHDOG_MAX ||
	    sgm->watchdog_timer < SGM4154x_WATCHDOG_DIS)
		return -EINVAL;
	#endif
	nqon = devm_gpiod_get(sgm->dev, "nqon", GPIOD_OUT_LOW);
	gpiod_set_value(nqon, 1);
	ret = device_property_read_u32(sgm->dev,
				       "input-voltage-limit-microvolt",
				       &sgm->init_data.vlim);
	if (ret)
		sgm->init_data.vlim = SGM4154x_VINDPM_DEF_uV;

	if (sgm->init_data.vlim > SGM4154x_VINDPM_V_MAX_uV ||
	    sgm->init_data.vlim < SGM4154x_VINDPM_V_MIN_uV)
		return -EINVAL;

	ret = device_property_read_u32(sgm->dev,
				       "input-current-limit-microamp",
				       &sgm->init_data.ilim);
	if (ret)
		sgm->init_data.ilim = SGM4154x_IINDPM_DEF_uA;

	if (sgm->init_data.ilim > SGM4154x_IINDPM_I_MAX_uA ||
	    sgm->init_data.ilim < SGM4154x_IINDPM_I_MIN_uA)
		return -EINVAL;

	irq_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,irq-gpio", 0);
	if (!gpio_is_valid(irq_gpio))
	{
		dev_err(sgm->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
		return -EINVAL;
	}
	ret = gpio_request(irq_gpio, "sgm4154x irq pin");
	if (ret) {
		dev_err(sgm->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
		return ret;
	}
	gpio_direction_input(irq_gpio);
	irqn = gpio_to_irq(irq_gpio);
	if (irqn < 0) {
		dev_err(sgm->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
		return irqn;
	}
	sgm->client->irq = irqn;

	chg_en_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,chg-en-gpio", 0);
	if (!gpio_is_valid(chg_en_gpio))
	{
		dev_err(sgm->dev, "%s: %d gpio get failed\n", __func__, chg_en_gpio);
		return -EINVAL;
	}
	ret = gpio_request(chg_en_gpio, "sgm chg en pin");
	if (ret) {
		dev_err(sgm->dev, "%s: %d gpio request failed\n", __func__, chg_en_gpio);
		return ret;
	}
	gpio_direction_output(chg_en_gpio,0);//default enable charge
	/* sw jeita */
	sgm->enable_sw_jeita = of_property_read_bool(sgm->dev->of_node, "enable_sw_jeita");

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_above_t4_cv", &val) >= 0)
		sgm->data.jeita_temp_above_t4_cv = val;
	else {
		dev_err(sgm->dev, "use default JEITA_TEMP_ABOVE_T4_CV:%d\n",JEITA_TEMP_ABOVE_T4_CV);
		sgm->data.jeita_temp_above_t4_cv = JEITA_TEMP_ABOVE_T4_CV;
	}

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_t3_to_t4_cv", &val) >= 0)
		sgm->data.jeita_temp_t3_to_t4_cv = val;
	else {
		dev_err(sgm->dev, "use default JEITA_TEMP_T3_TO_T4_CV:%d\n",JEITA_TEMP_T3_TO_T4_CV);
		sgm->data.jeita_temp_t3_to_t4_cv = JEITA_TEMP_T3_TO_T4_CV;
	}

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_t2_to_t3_cv", &val) >= 0)
		sgm->data.jeita_temp_t2_to_t3_cv = val;
	else {
		dev_err(sgm->dev, "use default JEITA_TEMP_T2_TO_T3_CV:%d\n",JEITA_TEMP_T2_TO_T3_CV);
		sgm->data.jeita_temp_t2_to_t3_cv = JEITA_TEMP_T2_TO_T3_CV;
	}

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_t1_to_t2_cv", &val) >= 0)
		sgm->data.jeita_temp_t1_to_t2_cv = val;
	else {
		dev_err(sgm->dev, "use default JEITA_TEMP_T1_TO_T2_CV:%d\n",JEITA_TEMP_T1_TO_T2_CV);
		sgm->data.jeita_temp_t1_to_t2_cv = JEITA_TEMP_T1_TO_T2_CV;
	}

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_below_t1_cv", &val) >= 0)
		sgm->data.jeita_temp_below_t1_cv = val;
	else {
		dev_err(sgm->dev, "use default JEITA_TEMP_BELOW_T1_CV:%d\n",JEITA_TEMP_BELOW_T1_CV);
		sgm->data.jeita_temp_below_t1_cv = JEITA_TEMP_BELOW_T1_CV;
	}
	pr_err("%s,enable_sw_jeita = %d,CV1 = %d,CV2 = %d,CV3 = %d,CV4 = %d,CV5 = %d\n",__func__,
			sgm->enable_sw_jeita,sgm->data.jeita_temp_above_t4_cv,sgm->data.jeita_temp_t3_to_t4_cv,
			sgm->data.jeita_temp_t2_to_t3_cv,sgm->data.jeita_temp_t1_to_t2_cv,
			sgm->data.jeita_temp_below_t1_cv);

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_above_t4_cc_current", &val) >= 0)
		sgm->data.jeita_temp_above_t4_cc_current = val;

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_t3_to_t4_cc_current", &val) >= 0)
		sgm->data.jeita_temp_t3_to_t4_cc_current = val;
	else
		sgm->data.jeita_temp_t3_to_t4_cc_current = JEITA_TEMP_T3_TO_T4_CC_CURRENT;

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_t2_to_t3_cc_current", &val) >= 0)
		sgm->data.jeita_temp_t2_to_t3_cc_current = val;
	else
		sgm->data.jeita_temp_t2_to_t3_cc_current = JEITA_TEMP_T2_TO_T3_CC_CURRENT;

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_t1_to_t2_cc_current", &val) >= 0)
		sgm->data.jeita_temp_t1_to_t2_cc_current = val;
	else
		sgm->data.jeita_temp_t1_to_t2_cc_current = JEITA_TEMP_T1_TO_T2_CC_CURRENT;

	if (of_property_read_u32(sgm->dev->of_node, "jeita_temp_below_t1_cc_current", &val) >= 0)
		sgm->data.jeita_temp_below_t1_cc_current = val;
	else
		sgm->data.jeita_temp_below_t1_cc_current = JEITA_TEMP_BELOW_T1_CC_CURRENT;

	pr_err("%s,CC1 = %d,CC2 = %d,CC3 = %d,CC4 = %d,CC5 = %d\n",__func__,
			sgm->data.jeita_temp_above_t4_cc_current,sgm->data.jeita_temp_t3_to_t4_cc_current,
			sgm->data.jeita_temp_t2_to_t3_cc_current,sgm->data.jeita_temp_t1_to_t2_cc_current,
			sgm->data.jeita_temp_below_t1_cc_current);

	if (of_property_read_u32(sgm->dev->of_node, "temp_t4_thres", &val) >= 0)
		sgm->data.temp_t4_thres = val;
	else {
		dev_err(sgm->dev, "use default TEMP_T4_THRES:%d\n",TEMP_T4_THRES);
		sgm->data.temp_t4_thres = TEMP_T4_THRES;
	}
	if (of_property_read_u32(sgm->dev->of_node, "temp_t4_thres_minus_x_degree", &val) >= 0)
		sgm->data.temp_t4_thres_minus_x_degree = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T4_THRES_MINUS_X_DEGREE:%d\n",TEMP_T4_THRES_MINUS_X_DEGREE);
		sgm->data.temp_t4_thres_minus_x_degree = TEMP_T4_THRES_MINUS_X_DEGREE;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_t3_thres", &val) >= 0)
		sgm->data.temp_t3_thres = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T3_THRES:%d\n",TEMP_T3_THRES);
		sgm->data.temp_t3_thres = TEMP_T3_THRES;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_t3_thres_minus_x_degree", &val) >= 0)
		sgm->data.temp_t3_thres_minus_x_degree = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T3_THRES_MINUS_X_DEGREE:%d\n",TEMP_T3_THRES_MINUS_X_DEGREE);
		sgm->data.temp_t3_thres_minus_x_degree = TEMP_T3_THRES_MINUS_X_DEGREE;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_t2_thres", &val) >= 0)
		sgm->data.temp_t2_thres = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T2_THRES:%d\n",TEMP_T2_THRES);
		sgm->data.temp_t2_thres = TEMP_T2_THRES;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_t2_thres_plus_x_degree", &val) >= 0)
		sgm->data.temp_t2_thres_plus_x_degree = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T2_THRES_PLUS_X_DEGREE:%d\n",TEMP_T2_THRES_PLUS_X_DEGREE);
		sgm->data.temp_t2_thres_plus_x_degree = TEMP_T2_THRES_PLUS_X_DEGREE;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_t1_thres", &val) >= 0)
		sgm->data.temp_t1_thres = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T1_THRES:%d\n",TEMP_T1_THRES);
		sgm->data.temp_t1_thres = TEMP_T1_THRES;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_t1_thres_plus_x_degree", &val) >= 0)
		sgm->data.temp_t1_thres_plus_x_degree = val;
	else {
		dev_err(sgm->dev,"use default TEMP_T1_THRES_PLUS_X_DEGREE:%d\n",TEMP_T1_THRES_PLUS_X_DEGREE);
		sgm->data.temp_t1_thres_plus_x_degree = TEMP_T1_THRES_PLUS_X_DEGREE;
	}

	if (of_property_read_u32(sgm->dev->of_node, "temp_neg_10_thres", &val) >= 0)
		sgm->data.temp_neg_10_thres = val;
	else {
		dev_err(sgm->dev,"use default TEMP_NEG_10_THRES:%d\n",TEMP_NEG_10_THRES);
		sgm->data.temp_neg_10_thres = TEMP_NEG_10_THRES;
	}
	pr_err("%s,thres4 = %d,thres3 = %d,thres2 = %d,thres1 = %d\n",__func__,
			sgm->data.temp_t4_thres,sgm->data.temp_t3_thres,
			sgm->data.temp_t2_thres,sgm->data.temp_t1_thres);
	return 0;
}

static int sgm4154x_set_otg_voltage(struct sgm4154x_device *sgm, int uv)
{
	int ret = 0;
	int reg_val = -1;
	int i = 0;
	while(i<4){
		if (uv == BOOST_VOLT_LIMIT[i]){
			reg_val = i;
			break;
		}
		i++;
	}
	if (reg_val < 0)
		return reg_val;
	reg_val = reg_val << 4;
	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_BOOSTV, reg_val);

	return ret;
}

static int sgm4154x_set_otg_current(struct sgm4154x_device *sgm, int ua)
{
	int ret = 0;

	if (ua == BOOST_CURRENT_LIMIT[0]){
		ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     0);
	}

	else if (ua == BOOST_CURRENT_LIMIT[1]){
		ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     BIT(7));
	}
	return ret;
}

static int sgm4154x_enable_vbus(struct regulator_dev *rdev)
{
	struct sgm4154x_device *sgm = rdev_get_drvdata(rdev);
	int ret = 0;

	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_1, SGM4154x_OTG_EN,
                     SGM4154x_OTG_EN);
	return ret;
}

static int sgm4154x_disable_vbus(struct regulator_dev *rdev)
{
	struct sgm4154x_device *sgm = rdev_get_drvdata(rdev);
	int ret = 0;

	ret = regmap_update_bits(sgm->regmap, SGM4154x_CHRG_CTRL_1, SGM4154x_OTG_EN,
                     0);

	return ret;
}

static int sgm4154x_is_enabled_vbus(struct regulator_dev *rdev)
{
	struct sgm4154x_device *sgm = rdev_get_drvdata(rdev);
	int temp = 0;
	int ret = 0;

	ret = regmap_read(sgm->regmap, SGM4154x_CHRG_CTRL_1, &temp);
	return (temp&SGM4154x_OTG_EN)? 1 : 0;
}

static struct regulator_ops sgm4154x_vbus_ops = {
	.enable = sgm4154x_enable_vbus,
	.disable = sgm4154x_disable_vbus,
	.is_enabled = sgm4154x_is_enabled_vbus,
};

static struct regulator_desc sgm4154x_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &sgm4154x_vbus_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
	.fixed_uV = 5000000,
	.n_voltages = 1,
};

static int sgm4154x_vbus_regulator_register(struct sgm4154x_device *sgm)
{
	struct regulator_config config = {};
	int ret = 0;
	/* otg regulator */
	config.dev = sgm->dev;
	config.driver_data = sgm;
	sgm->otg_rdev = devm_regulator_register(sgm->dev,
						&sgm4154x_otg_rdesc, &config);
	sgm->otg_rdev->constraints->valid_ops_mask |= REGULATOR_CHANGE_STATUS;
	if (IS_ERR(sgm->otg_rdev)) {
		ret = PTR_ERR(sgm->otg_rdev);
		pr_debug("%s: register otg regulator failed (%d)\n", __func__, ret);
	}
	return ret;
}

static int sgm4154x_suspend_notifier(struct notifier_block *nb,
                unsigned long event,
                void *dummy)
{
    struct sgm4154x_device *sgm = container_of(nb, struct sgm4154x_device, pm_nb);

    switch (event) {

    case PM_SUSPEND_PREPARE:
        pr_err("sgm4154x PM_SUSPEND \n");

        cancel_delayed_work_sync(&sgm->charge_monitor_work);

        sgm->sgm4154x_suspend_flag = 1;

        return NOTIFY_OK;

    case PM_POST_SUSPEND:
        pr_err("sgm4154x PM_RESUME \n");

        schedule_delayed_work(&sgm->charge_monitor_work, 0);

        sgm->sgm4154x_suspend_flag = 0;

        return NOTIFY_OK;

    default:
        return NOTIFY_DONE;
    }
}

static int sgm4154x_hw_chipid_detect(struct sgm4154x_device *sgm)
{
	int ret = 0;
	int val = 0;

	ret = regmap_read(sgm->regmap,SGM4154x_CHRG_CTRL_b,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SGM4154x_CHRG_CTRL_b fail\n", __func__);
		return ret;
	}

	pr_info("[%s] Reg[0x0B]=0x%x\n", __func__,val);

	return val;
}

static int sgm4154x_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct sgm4154x_device *sgm;
	int ret;
	int otg_notify;
	char *name = NULL;

	sgm = devm_kzalloc(dev, sizeof(*sgm), GFP_KERNEL);
	if (!sgm)
		return -ENOMEM;

	sgm->client = client;
	sgm->dev = dev;

	mutex_init(&sgm->lock);

	strncpy(sgm->model_name, "sgm41515", I2C_NAME_SIZE);

	sgm->regmap = devm_regmap_init_i2c(client, &sgm4154x_regmap_config);
	if (IS_ERR(sgm->regmap)) {
		dev_err(dev, "Failed to allocate register map\n");
		return PTR_ERR(sgm->regmap);
	}

	i2c_set_clientdata(client, sgm);

	// Customer customization
	ret = sgm4154x_parse_dt(sgm);
	if (ret) {
		dev_err(dev, "Failed to read device tree properties%d\n", ret);
		return ret;
	}

	ret = sgm4154x_hw_chipid_detect(sgm);
	if ((ret & SGM4154x_PN_MASK) != SGM4154x_PN_ID){
		pr_info("[%s] device not found !!!\n", __func__);
		return ret;
	}

	name = devm_kasprintf(sgm->dev, GFP_KERNEL, "%s",
		"sgm4154x suspend wakelock");
	sgm->charger_wakelock =
		wakeup_source_register(sgm->dev, name);

	/* OTG reporting */
	sgm->usb2_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB2);
	if (!IS_ERR_OR_NULL(sgm->usb2_phy)) {
		INIT_WORK(&sgm->usb_work, sgm4154x_usb_work);
		sgm->usb_nb.notifier_call = sgm4154x_usb_notifier;
		otg_notify = usb_register_notifier(sgm->usb2_phy, &sgm->usb_nb);
	}

	sgm->usb3_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB3);
	if (!IS_ERR_OR_NULL(sgm->usb3_phy)) {
		INIT_WORK(&sgm->usb_work, sgm4154x_usb_work);
		sgm->usb_nb.notifier_call = sgm4154x_usb_notifier;
		otg_notify = usb_register_notifier(sgm->usb3_phy, &sgm->usb_nb);
	}

	if (client->irq) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						sgm4154x_irq_handler_thread,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						dev_name(&client->dev), sgm);
		if (ret)
			goto error_out;
		enable_irq_wake(client->irq);
	}

	INIT_DELAYED_WORK(&sgm->charge_detect_delayed_work, charger_detect_work_func);
	INIT_DELAYED_WORK(&sgm->charge_monitor_work, charger_monitor_work_func);
	sgm->pm_nb.notifier_call = sgm4154x_suspend_notifier;
	register_pm_notifier(&sgm->pm_nb);

	ret = sgm4154x_power_supply_init(sgm, dev);
	if (ret) {
		dev_err(dev, "Failed to register power supply\n");
		goto error_out;
	}

	ret = sgm4154x_hw_init(sgm);
	if (ret) {
		dev_err(dev, "Cannot initialize the chip.\n");
		goto error_out;
	}


	//OTG setting
	sgm4154x_set_otg_voltage(sgm, 5000000); //5V
	sgm4154x_set_otg_current(sgm, 1200000); //1.2A

	ret = sgm4154x_vbus_regulator_register(sgm);

	schedule_delayed_work(&sgm->charge_monitor_work,100);

	return ret;
error_out:
	if (!IS_ERR_OR_NULL(sgm->usb2_phy))
		usb_unregister_notifier(sgm->usb2_phy, &sgm->usb_nb);

	if (!IS_ERR_OR_NULL(sgm->usb3_phy))
		usb_unregister_notifier(sgm->usb3_phy, &sgm->usb_nb);
	return ret;
}

static void sgm4154x_charger_remove(struct i2c_client *client)
{
	struct sgm4154x_device *sgm = i2c_get_clientdata(client);

	cancel_delayed_work_sync(&sgm->charge_monitor_work);

	regulator_unregister(sgm->otg_rdev);

	power_supply_unregister(sgm->charger);

	mutex_destroy(&sgm->lock);

	return;
}

/*
static void sgm4154x_charger_shutdown(struct i2c_client *client)
{
    int ret = 0;

	struct sgm4154x_device *sgm = i2c_get_clientdata(client);
    ret = sgm4154x_disable_charger(sgm);
    if (ret) {
        pr_err("Failed to disable charger, ret = %d\n", ret);
    }
    pr_info("sgm4154x_charger_shutdown\n");
}
*/
static const struct i2c_device_id sgm4154x_i2c_ids[] = {
	{ "sgm41541", 0 },
	{ "sgm41542", 0 },
	{ "sgm41543", 0 },
	{ "sgm41543D", 0 },
	{ "sgm41513", 0 },
	{ "sgm41513A", 0 },
	{ "sgm41513D", 0 },
	{ "sgm41516", 0 },
	{ "sgm41516D", 0 },
	{ "sgm41515", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm4154x_i2c_ids);

static const struct of_device_id sgm4154x_of_match[] = {
	{ .compatible = "sgm,sgm41541", },
	{ .compatible = "sgm,sgm41542", },
	{ .compatible = "sgm,sgm41543", },
	{ .compatible = "sgm,sgm41543D", },
	{ .compatible = "sgm,sgm41513", },
	{ .compatible = "sgm,sgm41513A", },
	{ .compatible = "sgm,sgm41513D", },
	{ .compatible = "sgm,sgm41516", },
	{ .compatible = "sgm,sgm41516D", },
	{ .compatible = "sgm,sgm41515", },
	{ },
};
MODULE_DEVICE_TABLE(of, sgm4154x_of_match);

static struct i2c_driver sgm4154x_driver = {
	.driver = {
		.name = "sgm4154x-charger",
		.of_match_table = sgm4154x_of_match,
	},
	.probe = sgm4154x_probe,
	.remove = sgm4154x_charger_remove,
/*	.shutdown = sgm4154x_charger_shutdown,*/
	.id_table = sgm4154x_i2c_ids,
};
module_i2c_driver(sgm4154x_driver);

MODULE_AUTHOR(" qhq <Allen_qin@sg-micro.com>");
MODULE_DESCRIPTION("sgm4154x charger driver");
MODULE_LICENSE("GPL v2");
