#include "key_script.h"

__s32  reset_key_gpio_read_one_pin_value(u32 p_handler, const char *gpio_name)
{
	if(p_handler) {		
		return __gpio_get_value(p_handler);
	}	
	printk("OSAL_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");	
	return -1;
}

__s32  reset_key_gpio_write_one_pin_value(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
	if(p_handler) {	
		__gpio_set_value(p_handler, value_to_gpio);	
	} else {	
	__wrn("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");	
	}	

	return 0;
}

int reset_key_sys_script_get_item(char *main_name, char *sub_name, int value[], int count)
{
    script_item_u   val;
	script_item_value_type_e  type;
	int ret = -1;

	type = script_get_item(main_name, sub_name, &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
		ret = 0;
		*value = val.val;
		__inf("%s.%s=%d\n", main_name, sub_name, *value);
	}	else if(SCIRPT_ITEM_VALUE_TYPE_PIO == type) {
		reset_key_gpio_set_t *gpio_info = (reset_key_gpio_set_t *)value;

		ret = 0;
		gpio_info->gpio = val.gpio.gpio;
		gpio_info->mul_sel = val.gpio.mul_sel;
		gpio_info->pull = val.gpio.pull;
		gpio_info->drv_level = val.gpio.drv_level;
		gpio_info->data = val.gpio.data;
		memcpy(gpio_info->gpio_name, sub_name, strlen(sub_name)+1);
		__inf("%s.%s gpio=%d,mul_sel=%d,data:%d\n",main_name, sub_name, gpio_info->gpio, gpio_info->mul_sel, gpio_info->data);
	}	else if(SCIRPT_ITEM_VALUE_TYPE_STR == type) {
		memcpy((void*)value, (void*)val.str, strlen(val.str)+1);
		__inf("%s.%s=%s\n",main_name, sub_name, val.str);
	} else {
		ret = -1;
		__inf("fetch script data %s.%s fail\n", main_name, sub_name);
	}

	return type;

}

int reset_key_sys_gpio_request_simple(reset_key_gpio_set_t *gpio_list, u32 group_count_max)
{
	
	int ret = 0;
	struct gpio_config pin_cfg;

	if(gpio_list == NULL)
		return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if(0 != ret) {
		__wrn("%s failed, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
		return ret;
	} else {
		__inf("%s, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
	}
	ret = pin_cfg.gpio;

	return ret;
}

int reset_key_sys_gpio_request(reset_key_gpio_set_t *gpio_list, u32 group_count_max)
{
	int ret = 0;
	struct gpio_config pin_cfg;
	char   pin_name[32];
	u32 config;

	if(gpio_list == NULL)
		return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if(0 != ret) {
		__wrn("%s failed, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
		return ret;
	} else {
		__inf("%s, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
	}
	ret = pin_cfg.gpio;

	if (!IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of sunxi-pinctrl,
		* config pin attributes individually.
		*/
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg.mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg.pull != GPIO_PULL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg.pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.drv_level != GPIO_DRVLVL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg.drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	} else if (IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of axp-pinctrl,
		* config pin attributes individually.
		*/
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(AXP_PINCTRL, pin_name, config);
		}
	} else {
		pr_warn("invalid pin [%d] from sys-config\n", pin_cfg.gpio);
	}

	return ret;
}

int reset_key_sys_gpio_release(int p_handler, s32 if_release_to_default_status)
{
	if(p_handler) {
		gpio_free(p_handler);
	} else {
		__wrn("OSAL_GPIO_Release, hdl is NULL\n");
	}
	return 0;
}
