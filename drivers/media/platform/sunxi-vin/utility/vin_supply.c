
/*
 ******************************************************************************
 *
 * vin_subdev.c
 *
 * Hawkview ISP - vin_subdev.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/device.h>
#include <linux/module.h>

#include "../vin-video/vin_core.h"
#include "vin_os.h"
#include "vin_supply.h"
#include "../platform/platform_cfg.h"
#include "../vin-csi/sunxi_csi.h"
#include "../vin.h"

/*
 * called by subdev in power on/off sequency
 */
struct vin_core *sd_to_vin_core(struct v4l2_subdev *sd)
{
	struct vin_md *vind =
	    (struct vin_md *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct vin_pipeline *pipe = NULL;
	struct vin_vid_cap *cap = NULL;
	int i, j;

	if (NULL != sd->entity.pipe) {
		pipe = to_vin_pipeline(&sd->entity);
		cap = pipe_to_vin_video(pipe);
		return cap->vinc;
	}
	for (i = 0; i < VIN_MAX_DEV; i++) {
		vinc = vind->vinc[i];
		if (NULL == vinc)
			continue;
		for (j = 0; j < VIN_IND_MAX; j++) {
			if (sd == vinc->vid_cap.pipe.sd[j])
				return vinc;
		}
	}
	for (i = 0; i < VIN_MAX_DEV; i++) {
		vinc = vind->vinc[i];
		if (NULL == vinc)
			continue;
		for (j = 0; j < MAX_DETECT_NUM; j++) {
			if ((sd == vinc->modu_cfg.modules.sensor[j].sd) ||
			    (sd == vinc->modu_cfg.modules.act[j].sd) ||
			    (sd == vinc->modu_cfg.modules.flash.sd))
				return vinc;
		}
	}
	vin_err("%s cannot find the vin core\n", sd->name);
	return NULL;
}

EXPORT_SYMBOL_GPL(sd_to_vin_core);

/*
 *enable/disable pmic channel
 */
int vin_set_pmu_channel(struct v4l2_subdev *sd, enum pmic_channel pmic_ch,
			enum on_off on_off)
{
#ifdef VIN_PMU
	struct vin_core *vinc = sd_to_vin_core(sd);
	static int def_vol[MAX_POW_NUM] = {3300000, 3300000, 1800000,
					3300000, 3300000};
	struct vin_power *power = NULL;
	int ret = 0;
	if (NULL == vinc) {
		vin_err("cannot find the vin_core is %s in\n", sd->name);
		return -1;
	}
	power = &vinc->modu_cfg.sensors.power[0];
	if (on_off == OFF) {
		if (NULL == power[pmic_ch].pmic)
			return 0;
		ret = regulator_disable(power[pmic_ch].pmic);
		if (!regulator_is_enabled(power[pmic_ch].pmic)) {
			vin_log(VIN_LOG_POWER, "regulator_is already disabled\n");
			regulator_put(power[pmic_ch].pmic);
			power[pmic_ch].pmic = NULL;
		}
	} else {
		if (power[pmic_ch].pmic
		    && regulator_is_enabled(power[pmic_ch].pmic)) {
			vin_log(VIN_LOG_POWER, "regulator_is already enabled\n");
		} else {
			if (strcmp(power[pmic_ch].power_str, "")) {
				power[pmic_ch].pmic =
				    regulator_get(NULL,
						  power[pmic_ch].power_str);
				if (IS_ERR_OR_NULL(power[pmic_ch].pmic)) {
					vin_err("get regulator %s error!\n",
						power[pmic_ch].power_str);
					power[pmic_ch].pmic = NULL;
					return -1;
				}
			} else {
				power[pmic_ch].pmic = NULL;
				return 0;
			}
		}
		ret =
		    regulator_set_voltage(power[pmic_ch].pmic,
					  power[pmic_ch].power_vol,
					  def_vol[pmic_ch]);
		vin_log(VIN_LOG_POWER, "set regulator %s = %d,return %x\n",
			power[pmic_ch].power_str, power[pmic_ch].power_vol,
			ret);
		ret = regulator_enable(power[pmic_ch].pmic);
	}
#endif
	return ret;
}

EXPORT_SYMBOL_GPL(vin_set_pmu_channel);

/*
 *enable/disable master clock
 */
int vin_set_mclk(struct v4l2_subdev *sd, enum on_off on_off)
{
#ifdef VIN_CLK
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct csi_dev *csi = NULL;
	if (NULL == vinc) {
		vin_err("cannot find the vin_core is %s in\n", sd->name);
		return -1;
	}
	if (NULL == vinc->csi_sd) {
		vin_err("cannot find the csi connected with %s\n", sd->name);
		return -1;
	}
	csi = v4l2_get_subdevdata(vinc->csi_sd);
	switch (on_off) {
	case ON:
		vin_print("mclk on\n");
		if (csi->clock[CSI_MASTER_CLK]) {
			if (clk_prepare_enable(csi->clock[CSI_MASTER_CLK])) {
				vin_err("csi%d master clock enable error\n",
					csi->id);
				return -1;
			}
		} else {
			vin_err("csi%d master clock is null\n", csi->id);
			return -1;
		}
		break;
	case OFF:
		vin_print("mclk off\n");
		if (csi->clock[CSI_MASTER_CLK]) {
			clk_disable_unprepare(csi->clock[CSI_MASTER_CLK]);
		} else {
			vin_err("csi%d master clock is null\n", csi->id);
			return -1;
		}
		usleep_range(10000, 12000);
		break;
	default:
		return -1;
	}
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(vin_set_mclk);

/*
 *set frequency of master clock
 */
int vin_set_mclk_freq(struct v4l2_subdev *sd, unsigned long freq)
{
#ifdef VIN_CLK
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct csi_dev *csi = NULL;
	struct clk *mclk_src;
	if (NULL == vinc) {
		vin_err("cannot find the vin_core is %s in\n", sd->name);
		return -1;
	}
	csi = v4l2_get_subdevdata(vinc->csi_sd);
	if (freq == 24000000 || freq == 12000000 || freq == 6000000) {
		if (csi->clock[CSI_MASTER_CLK_24M_SRC]) {
			mclk_src = csi->clock[CSI_MASTER_CLK_24M_SRC];
		} else {
			vin_err("csi master clock 24M source is null\n");
			return -1;
		}
	} else {
		if (csi->clock[CSI_MASTER_CLK_PLL_SRC]) {
			mclk_src = csi->clock[CSI_MASTER_CLK_PLL_SRC];
		} else {
			vin_err("csi master clock pll source is null\n");
			return -1;
		}
	}

	if (csi->clock[CSI_MASTER_CLK]) {
		if (clk_set_parent(csi->clock[CSI_MASTER_CLK], mclk_src)) {
			vin_err("mclk src = %s, set mclk source failed!\n",
			     mclk_src->name);
			return -1;
		}
	} else {
		vin_err("csi master clock is null\n");
		return -1;
	}

	if (csi->clock[CSI_MASTER_CLK]) {
		if (clk_set_rate(csi->clock[CSI_MASTER_CLK], freq)) {
			vin_err("set csi%d master clock error\n", csi->id);
			return -1;
		}
	} else {
		vin_err("csi master clock is null\n");
		return -1;
	}
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(vin_set_mclk_freq);

/*
 *set the gpio io status
 */
int vin_gpio_write(struct v4l2_subdev *sd, enum gpio_type gpio_type,
		   unsigned int status)
{
#ifdef VIN_GPIO
	int force_value_flag = 1;
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct gpio_config *gpio = NULL;
	if (NULL == vinc) {
		vin_err("cannot find the vin_core is %s in\n", sd->name);
		return -1;
	}
	gpio = &vinc->modu_cfg.sensors.gpio[0];
	if ((gpio_type == PWDN) || (gpio_type == RESET))
		force_value_flag = 0;
	return os_gpio_write(gpio[gpio_type].gpio, status, NULL,
			     force_value_flag);
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(vin_gpio_write);

/*
 *set the gpio io status
 */
int vin_gpio_set_status(struct v4l2_subdev *sd, enum gpio_type gpio_type,
			unsigned int status)
{
#ifdef VIN_GPIO
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct gpio_config *gpio = NULL;
	if (NULL == vinc) {
		vin_err("cannot find the vin_core is %s in\n", sd->name);
		return -1;
	}
	gpio = &vinc->modu_cfg.sensors.gpio[0];
	return os_gpio_set_status(gpio[gpio_type].gpio, status, NULL);
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(vin_gpio_set_status);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end subdev for sunxi");
