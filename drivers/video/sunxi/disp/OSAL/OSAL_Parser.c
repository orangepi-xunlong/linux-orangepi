#include "OSAL_Parser.h"


#ifndef __OSAL_PARSER_MASK__
int OSAL_Script_FetchParser_Data(char *main_name, char *sub_name, int value[], int count)
{
	script_item_u   val;
	script_item_value_type_e  type;
	int ret = -1;

	if(count == 1) {
		type = script_get_item(main_name, sub_name, &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
			ret = 0;
			*value = val.val;
			__inf("%s.%s=%d\n", main_name, sub_name, *value);
		}	else {
			ret = -1;
			__inf("fetch script data %s.%s fail\n", main_name, sub_name);
		}
	} else if(count == sizeof(disp_gpio_set_t)/sizeof(int)) {
		type = script_get_item(main_name, sub_name, &val);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO == type) {
			disp_gpio_set_t *gpio_info = (disp_gpio_set_t *)value;

			ret = 0;
			gpio_info->gpio = val.gpio.gpio;
			gpio_info->mul_sel = val.gpio.mul_sel;
			gpio_info->pull = val.gpio.pull;
			gpio_info->drv_level = val.gpio.drv_level;
			gpio_info->data = val.gpio.data;
			memcpy(gpio_info->gpio_name, sub_name, strlen(sub_name)+1);
			__inf("%s.%s gpio=%d,mul_sel=%d,data:%d\n",main_name, sub_name, gpio_info->gpio, gpio_info->mul_sel, gpio_info->data);
		}	else {
			ret = -1;
			__inf("fetch script data %s.%s fail\n", main_name, sub_name);
		}
	} else {
		type = script_get_item(main_name, sub_name, &val);
		if(SCIRPT_ITEM_VALUE_TYPE_STR == type) {
			memcpy((void*)value, (void*)val.str, strlen(val.str)+1);
			__inf("%s.%s=%s\n",main_name, sub_name, val.str);
		} else {
			ret = -1;
			__inf("fetch script data %s.%s fail\n", main_name, sub_name);
		}
	}

	return ret;
}

/* returns: 0:invalid, 1: int; 2:str, 3: gpio */
int OSAL_Script_FetchParser_Data_Ex(char *main_name, char *sub_name, int value[], int count)
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
		disp_gpio_set_t *gpio_info = (disp_gpio_set_t *)value;

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

#else

int OSAL_Script_FetchParser_Data(char *main_name, char *sub_name, int value[], int count)
{
	return 0;
}

/* returns: 0:invalid, 1: int; 2:str, 3: gpio */
int OSAL_Script_FetchParser_Data_Ex(char *main_name, char *sub_name, int value[], int count)
{
	return 0;
}

int OSAL_sw_get_ic_ver(void)
{
	return 0;
}

#endif
