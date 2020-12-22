#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/err.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include "module_pm.h"

struct rf_mod_info mod_info;
static char *module_para = "module_para";
static struct clk *ap_32k = NULL;
struct regulator* wifi_ldo[5] = {NULL};

#define rf_pm_msg(...)    do {printk("[rf_pm]: "__VA_ARGS__);} while(0)

int sunxi_gpio_req(struct gpio_config *gpio)
{
	int            ret = 0;
	char           pin_name[8] = {0};
	unsigned long  config;

	sunxi_gpio_to_name(gpio->gpio, pin_name);
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
	ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
	if (ret) {
		rf_pm_msg("set gpio %s mulsel failed.\n",pin_name);
		return -1;
	}

	if (gpio->pull != GPIO_PULL_DEFAULT){
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			rf_pm_msg("set gpio %s pull mode failed.\n",pin_name);
			return -1;
		}
	}

	if (gpio->drv_level != GPIO_DRVLVL_DEFAULT){
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, gpio->drv_level);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			rf_pm_msg("set gpio %s driver level failed.\n",pin_name);
			return -1;
		}
	}

	if (gpio->data != GPIO_DATA_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, gpio->data);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
 			rf_pm_msg("set gpio %s initial val failed.\n",pin_name);
			return -1;
		}
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_gpio_req);

int rf_module_power(int onoff)
{
	int i = 0, ret = 0;

	if (onoff) {
		rf_pm_msg("regulator on.\n");
		for (i = 0; mod_info.power[i].used > 0; i++){
			if(mod_info.power[i].used == 1) {
				wifi_ldo[i] = regulator_get(NULL, mod_info.power[i].power_axp);
				if (IS_ERR(wifi_ldo[i])) {
					rf_pm_msg("get power regulator %s failed.\n", mod_info.power[i].power_axp);
					break;
				}

				if(mod_info.power[i].vol > 0) {
					ret = regulator_set_voltage(wifi_ldo[i], mod_info.power[i].vol, mod_info.power[i].vol);
					if (ret < 0) {
						rf_pm_msg("set_voltage %s %d fail, return %d.\n", mod_info.power[i].power_axp, mod_info.power[i].vol, ret);
						goto exit;
					}
				}

				ret = regulator_enable(wifi_ldo[i]);
				if (ret < 0) {
					rf_pm_msg("regulator_enable %s fail, return %d.\n", mod_info.power[i].power_axp, ret);
					goto exit;
				}
			} else if(mod_info.power[i].used == 2) {
				if (mod_info.power[i].power_gpio != -1){
					gpio_set_value(mod_info.power[i].power_gpio, mod_info.power[i].vol? 1:0);
				} else {
					rf_pm_msg("get mod_info.power[%d].power_gpio failed\n", i);
					return -1;
				}
			}
		}
		return ret;
	} else {
		rf_pm_msg("regulator off.\n");
		for(i = 0; mod_info.power[i].used > 0; i++){
			if(mod_info.power[i].used == 1) {
				ret = regulator_disable(wifi_ldo[i]);
				if (ret < 0) {
					rf_pm_msg("regulator_disable %s fail, return %d.\n", mod_info.power[i].power_axp, ret);
					goto exit;
				}
			} else if(mod_info.power[i].used == 2) {
				if (mod_info.power[i].power_gpio != -1){
					gpio_set_value(mod_info.power[i].power_gpio, mod_info.power[i].vol? 0:1);
				} else {
					rf_pm_msg("get mod_info.power[%d].power_gpio failed\n", i);
					return -1;
				}
			}
		}
	}

exit:
	for(i = 0; i < ARRAY_SIZE(wifi_ldo); i++){
		if(wifi_ldo[i]) {
			regulator_put(wifi_ldo[i]);
			wifi_ldo[i] = NULL;
		}
	}

	return ret;
}
EXPORT_SYMBOL(rf_module_power);

int rf_pm_gpio_ctrl(char *name, int level)
{
	int i = 0;
	int gpio = 0;
	char * gpio_name[1] = {"chip_en"};

	for (i = 0; i < 1; i++) {
		if (strcmp(name, gpio_name[i]) == 0) {
			switch (i)
			{
				case 0: /*chip_en*/
					gpio = mod_info.chip_en;
					break;
				default:
					rf_pm_msg("no matched gpio.\n");
			}
			break;
		}
	}

	if (gpio != -1){
		gpio_set_value(gpio, level);
		printk("gpio %s set val %d, act val %d\n", name, level, gpio_get_value(gpio));
	}

	return 0;
}
EXPORT_SYMBOL(rf_pm_gpio_ctrl);

static void enable_ap_32k(int enable)
{
	int ret = 0;

	if (enable){
		ret = clk_prepare_enable(ap_32k);
		if (ret) {
			rf_pm_msg("enable ap 32k failed!\n");
		}
	} else {
		clk_disable_unprepare(ap_32k);
	}
}

//get module resource
static int get_module_res(void)
{
	struct power_info {
		char *power_name;
		char *value_name;
	};
	struct power_info power_table[4] = {
		{
			.power_name = "module_power0",
			.value_name = "module_power0_vol",
		},
		{
			.power_name = "module_power1",
			.value_name = "module_power1_vol",
		},
		{
			.power_name = "module_power2",
			.value_name = "module_power2_vol",
		},
		{
			.power_name = "module_power3",
			.value_name = "module_power3_vol",
		},
	};
	script_item_u val;
	script_item_value_type_e type;
	struct gpio_config  *gpio_p = NULL;
	struct rf_mod_info *mod_info_p = &mod_info;
	int i = 0;

	memset(mod_info_p, 0, sizeof(struct rf_mod_info));

	for(i = 0; i < 4; i++) {
		mod_info_p->power[i].used = 0; //unused
		type = script_get_item(module_para, power_table[i].power_name, &val);
		if(SCIRPT_ITEM_VALUE_TYPE_STR == type) {
			if(!strcmp(val.str, "")) {
				continue;
			} else {
				mod_info_p->power[i].power_axp = val.str;
				rf_pm_msg("module power%d name %s\n", i, mod_info_p->power[i].power_axp);
				mod_info_p->power[i].used = 1;    //used power_axp
			}
		} else if(SCIRPT_ITEM_VALUE_TYPE_PIO == type){
			gpio_p = &val.gpio;
			mod_info_p->power[i].power_gpio = gpio_p->gpio;
			sunxi_gpio_req(gpio_p);
			mod_info_p->power[i].used = 2;    //used power_gpio
		} else {
			rf_pm_msg("Did not config %s in sys_config\n",power_table[i].power_name);
			continue;
		}
		if(mod_info_p->power[i].used) {
			type = script_get_item(module_para, power_table[i].value_name, &val);
			if(SCIRPT_ITEM_VALUE_TYPE_INT != type) {
				rf_pm_msg("failed to fetch %s\n",power_table[i].value_name);
				mod_info_p->power[i].used = 0;
				break;
			} else {
				mod_info_p->power[i].vol = val.val;
			}
		}
	}

	mod_info_p->chip_en = -1;
	type = script_get_item(module_para, "chip_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		rf_pm_msg("mod has no chip_en gpio\n");
	else {
		gpio_p = &val.gpio;
		mod_info_p->chip_en = gpio_p->gpio;
		sunxi_gpio_req(gpio_p);
	}

	type = script_get_item(module_para, "lpo_use_apclk", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type)
		rf_pm_msg("failed to fetch lpo_use_apclk\n");
	else {
		mod_info_p->lpo_use_apclk = val.str;
	}

	return 0;
}

static int __init rf_pm_init(void)
{
	get_module_res();
	rf_module_power(1);

	//opt ap 32k
	if(mod_info.lpo_use_apclk != NULL){
		ap_32k = clk_get(NULL, mod_info.lpo_use_apclk);
		if (!ap_32k || IS_ERR(ap_32k)){
			rf_pm_msg("Get ap 32k clk out failed!\n");
			return -1;
		}
		rf_pm_msg("set %s 32k out", mod_info.lpo_use_apclk);
		enable_ap_32k(1);
	}

	return 0;
}

static void __exit rf_pm_exit(void)
{
	if (ap_32k){
		enable_ap_32k(0);
		ap_32k = NULL;
	}
	rf_module_power(0);
}

late_initcall_sync(rf_pm_init);
module_exit(rf_pm_exit);
