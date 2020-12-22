#include <linux/module.h>
#include "axp-cfg.h"

static s32 get_para_from_cmdline(const char *cmdline, const char *name, char *value)
{
	char *p = (char *)cmdline;
	char *value_p = value;

	if(!cmdline || !name || !value){
		return -1;
	}

	for(; *p != 0;){
		if(*p++ == ' '){
			if(0 == strncmp(p, name, strlen(name))){
				p += strlen(name);
				if(*p++ != '='){
					continue;
				}
				while(*p != 0 && *p != ' '){
					*value_p++ = *p++;
				}
				*value_p = '\0';
				return value_p - value;
			}
		}
	}

	return 0;
}

u32 axp_get_chip_id(void)
{
	char val[3] = {0};
	u32 value;

	//get the parameters of chip id from cmdline passed by boot
	if(0 < get_para_from_cmdline(saved_command_line, "axp_chipid",val)){
		value = simple_strtoul(val, 0, 10);
	} else {
		value = 0x0;
		printk("get chip_id failed\n");
	}

	return value;
}
EXPORT_SYMBOL(axp_get_chip_id);

static int __init axp_type_init(void)
{
	u32 value = 0;

	value = axp_get_chip_id();

	if (0x03 == value) {
		strcpy(pmu_type, "pmu2_regu");
	} else {
		strcpy(pmu_type, "pmu1_regu");
	}
	return 0;
}

core_initcall(axp_type_init);

