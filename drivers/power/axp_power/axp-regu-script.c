/*
 * Regulators driver for allwinnertech AXP
 *
 * Copyright (C) 2014 allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/sys_config.h>
#include <linux/power/axp_depend.h>
#include "axp-regu.h"

static struct axp_consumer_supply *consumer_supply_count = NULL;

int axp_regu_fetch_sysconfig_para(char * pmu_type, struct axp_reg_init *axp_init_data)
{
	int ldo_count = 0, ldo_index = 0;
	char name[32] = {0};
	script_item_u val;
	script_item_value_type_e type;
	int num = 0, supply_num = 0, i = 0, j =0, var = 0;
	struct axp_consumer_supply consumer_supply[20];
	struct regulator_consumer_supply *regu_consumer_supply = NULL;

	type = script_get_item(pmu_type, "regulator_count", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("fetch axp regu tree from sysconfig failed\n");
		return -1;
	} else
		ldo_count = val.val;

	for (ldo_index = 1; ldo_index <= ldo_count; ldo_index++) {
		sprintf(name, "regulator%d", ldo_index);
		type = script_get_item(pmu_type, name, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
			printk("get %s from sysconfig failed\n", name);
			continue;
		}
		num = sscanf(val.str, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
			consumer_supply[0].supply, consumer_supply[1].supply, consumer_supply[2].supply,
			consumer_supply[3].supply, consumer_supply[4].supply, consumer_supply[5].supply,
			consumer_supply[6].supply, consumer_supply[7].supply, consumer_supply[8].supply,
			consumer_supply[9].supply, consumer_supply[10].supply, consumer_supply[11].supply,
			consumer_supply[12].supply, consumer_supply[13].supply, consumer_supply[14].supply,
			consumer_supply[15].supply, consumer_supply[16].supply, consumer_supply[17].supply,
			consumer_supply[18].supply, consumer_supply[19].supply);
		if (num <= -1) {
			printk(KERN_ERR "parse ldo%d from sysconfig failed\n", ldo_index);
			return -1;
		} else {

			if (strcmp(consumer_supply[1].supply, "none")) {
				var = simple_strtoul(consumer_supply[1].supply, NULL, 10);
				if (var > (ldo_index-1))
					printk(KERN_ERR "supply rely set err\n");
				else
					(*(axp_init_data+(ldo_index-1))).axp_reg_init_data.supply_regulator = ((*(axp_init_data+(var-1))).axp_reg_init_data.consumer_supplies)->supply;
			}

			supply_num = num-1;
			(*(axp_init_data+(ldo_index-1))).axp_reg_init_data.num_consumer_supplies = supply_num;

			consumer_supply_count = (struct axp_consumer_supply *)kzalloc(sizeof(struct axp_consumer_supply)*supply_num, GFP_KERNEL);
			if (!consumer_supply_count) {
				printk(KERN_ERR "%s: request consumer_supply_count failed\n", __func__);
				return -1;
			}
			regu_consumer_supply = (struct regulator_consumer_supply *)kzalloc(sizeof(struct regulator_consumer_supply)*supply_num, GFP_KERNEL);
			if (!regu_consumer_supply) {
				printk(KERN_ERR "%s: request regu_consumer_supply failed\n", __func__);
				kfree(consumer_supply_count);
				return -1;
			}

			for (i = 0; i < supply_num; i++) {
				if (0 != i)
					j = i + 1;
				else
					j = i;
				strcpy((char*)(consumer_supply_count+i),
						consumer_supply[j].supply);
				(regu_consumer_supply+i)->supply = (const char*)((struct axp_consumer_supply *)(consumer_supply_count+i)->supply);

				{
					int ret = 0, sys_id_conut = 0;

					sys_id_conut = axp_check_sys_id((consumer_supply_count+i)->supply);
					if (0 <= sys_id_conut) {
						ret = get_ldo_dependence((const char *)&(consumer_supply[0].supply), sys_id_conut);
						if (ret < 0)
							printk("sys_id %s set dependence failed. \n", (consumer_supply_count+i)->supply);
					}
				}

			}
			(*(axp_init_data+(ldo_index-1))).axp_reg_init_data.consumer_supplies = regu_consumer_supply;
		}
	}
	return 0;
}

