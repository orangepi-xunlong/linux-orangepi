/*
 * Regulators driver for allwinnertech AXP19
 *
 * Copyright (C) 2014 allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <mach/sys_config.h>
#include "../axp-regu.h"
#include "axp19-regu.h"

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{
	axp19_vcc_dcdc1,
	axp19_vcc_dcdc2,
	axp19_vcc_dcdc3,
	axp19_vcc_ldo1,
	axp19_vcc_ldo2,
	axp19_vcc_ldo3,
	axp19_vcc_ldo4,
	axp19_vcc_ldoio0,
};

struct axp_regu_data {
	struct axp_consumer_supply axp_ldo1_data[10];
	struct axp_consumer_supply axp_ldo2_data[10];
	struct axp_consumer_supply axp_ldo3_data[10];
	struct axp_consumer_supply axp_ldo4_data[10];
	struct axp_consumer_supply axp_ldo5_data[10];
	struct axp_consumer_supply axp_ldo6_data[10];
	struct axp_consumer_supply axp_ldo7_data[10];
	struct axp_consumer_supply axp_ldo8_data[10];
};

struct axp_supply_data {
	struct regulator_consumer_supply axp_ldo1_data[10];
	struct regulator_consumer_supply axp_ldo2_data[10];
	struct regulator_consumer_supply axp_ldo3_data[10];
	struct regulator_consumer_supply axp_ldo4_data[10];
	struct regulator_consumer_supply axp_ldo5_data[10];
	struct regulator_consumer_supply axp_ldo6_data[10];
	struct regulator_consumer_supply axp_ldo7_data[10];
	struct regulator_consumer_supply axp_ldo8_data[10];
};

static struct axp_regu_data axp19_regu_data;
static struct axp_supply_data axp19_supply_data = {
	{
		{
			.supply = axp19_regu_data.axp_ldo1_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo1_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo2_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo2_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo3_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo3_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo4_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo4_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo5_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo5_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo6_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo6_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo7_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo7_data[9].supply,
		},
	},
	{
		{
			.supply = axp19_regu_data.axp_ldo8_data[0].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[1].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[2].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[3].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[4].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[5].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[6].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[7].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[8].supply,
		},
		{
			.supply = axp19_regu_data.axp_ldo8_data[9].supply,
		},
	},
};

#define AXP19_LDO(_id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2,new_level, mode_addr, freq_addr)	\
	 AXP_LDO(AXP19, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr)

#define AXP19_DCDC(_id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr)	\
	 AXP_DCDC(AXP19, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr)

static struct axp_regulator_info axp19_regulator_info[] = {
	AXP19_DCDC(1,	 700,	 3500,	 25,	 DCDC1,  0,	 7,	 DCDC1EN,0, 0, 0, 0, AXP19_DCDC_MODESET, AXP19_DCDC_FREQSET),
	AXP19_DCDC(2,	 700,	 2275,	 25,	 DCDC2,  0,	 6,	 DCDC2EN,0, 0, 0, 0, AXP19_DCDC_MODESET, AXP19_DCDC_FREQSET),
	AXP19_DCDC(3,	 700,	 3500,	 25,	 DCDC3,  0,	 7,	 DCDC3EN,1, 0, 0, 0, AXP19_DCDC_MODESET, AXP19_DCDC_FREQSET),
	AXP19_LDO(1,	 3000,	 3000,	 0,	 LDO1,	 0,	 0,	 LDO1EN, 0, 0, 0, 0, 0, 0),	   //ldo1 for rtc
	AXP19_LDO(2,	 1800,	 3300,	 100,	 LDO2,	 4,	 4,	 LDO2EN, 2, 0, 0, 0, 0, 0),	   //ldo2 for analog1
	AXP19_LDO(3,	 700,	 3500,	 25,	 LDO3,	 0,	 7,	 LDO3EN, 2, 0, 0, 0, 0, 0),	   //ldo3 for digital
	AXP19_LDO(4,	 1800,	 3300,	 100,	 LDO4,	 0,	 4,	 LDO4EN, 3, 0, 0, 0, 0, 0),	   //ldo4 for analog2
	AXP19_LDO(IO0,   1800,	 3300,	 100,	 LDOIO0, 4,	 4,	 LDOIOEN,0, 0, 0, 0, 0, 0),	   //ldoio0 
};

static struct axp_reg_init axp_regl_init_data[] = {
	[axp19_vcc_dcdc1] = {
		{
			.constraints = {
				.name = "axp19_dcdc1",
				.min_uV = 700000,
				.max_uV = 3500000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo1_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_dcdc1],
	},
	[axp19_vcc_dcdc2] = {
		{
			.constraints = {
				.name = "axp19_dcdc2",
				.min_uV = 700000,
				.max_uV = 2275000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo2_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_dcdc2],
	},
	[axp19_vcc_dcdc3] = {
		{
			.constraints = {
				.name = "axp19_dcdc3",
				.min_uV = 700000,
				.max_uV = 3500000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo3_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_dcdc3],
	},
	[axp19_vcc_ldo1] = {
		{
			.constraints = {
				.name = "axp19_rtc",
				.min_uV =  3000000,
				.max_uV =  3000000,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo4_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_ldo1],
	},
	[axp19_vcc_ldo2] = {
		{
			.constraints = {
				.name = "axp19_ldo2",
				.min_uV = 1800000,
				.max_uV = 3300000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo5_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_ldo2],
	},
	[axp19_vcc_ldo3] = {
		{
			.constraints = {
				.name = "axp19_ldo3",
				.min_uV = 700000,
				.max_uV = 3500000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo6_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_ldo3],
	},
	[axp19_vcc_ldo4] = {
		{
			.constraints = {
				.name = "axp19_ldo4",
				.min_uV =  1800000,
				.max_uV =  3300000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo7_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_ldo4],
	},
	[axp19_vcc_ldoio0] = {
		{
			.constraints = {
				.name = "axp19_ldoio0",
				.min_uV = 1800000,
				.max_uV = 3300000,
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			},
			.consumer_supplies = axp19_supply_data.axp_ldo8_data,
		},
		.info = &axp19_regulator_info[axp19_vcc_ldoio0],
	},
};

static struct axp_funcdev_info axp_regldevs[8] = {
	{
		.name = "axp-regulator",
		.id = AXP19_ID_DCDC1,
		.platform_data = &axp_regl_init_data[axp19_vcc_dcdc1],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_DCDC2,
		.platform_data = &axp_regl_init_data[axp19_vcc_dcdc2],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_DCDC3,
		.platform_data = &axp_regl_init_data[axp19_vcc_dcdc3],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_LDO1,
		.platform_data = &axp_regl_init_data[axp19_vcc_ldo1],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_LDO2,
		.platform_data = &axp_regl_init_data[axp19_vcc_ldo2],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_LDO3,
		.platform_data = &axp_regl_init_data[axp19_vcc_ldo3],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_LDO4,
		.platform_data = &axp_regl_init_data[axp19_vcc_ldo4],
	}, {
		.name = "axp-regulator",
		.id = AXP19_ID_LDOIO0,
		.platform_data = &axp_regl_init_data[axp19_vcc_ldoio0],
	},
};

int axp19_regu_fetch_sysconfig_para(char * pmu_type, struct axp_regu_data *axpxx_data, struct axp_reg_init *axp_init_data)
{
	int ldo_count = 0, ldo_index = 0;
	char name[32] = {0};
	script_item_u val;
	script_item_value_type_e type;
	int num = 0, i = 0;
	struct axp_consumer_supply (*consumer_supply)[10] = NULL;
	struct axp_consumer_supply (*consumer_supply_base)[10] = NULL;
	consumer_supply_base = &(axpxx_data->axp_ldo1_data);

	type = script_get_item(pmu_type, "ldo_count", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("fetch axp regu tree from sysconfig failed\n");
		return -1;
	} else
		ldo_count = val.val;

	for (ldo_index = 1; ldo_index <= ldo_count; ldo_index++) {
		sprintf(name, "ldo%d", ldo_index);
		type = script_get_item(pmu_type, name, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
			printk("get %s from sysconfig failed\n", name);
			continue;
		}
		consumer_supply = consumer_supply_base + (ldo_index - 1);
		num = sscanf(val.str, "%s %s %s %s %s %s %s %s %s %s",
			((*consumer_supply)+0)->supply, ((*consumer_supply)+1)->supply, ((*consumer_supply)+2)->supply,
			((*consumer_supply)+3)->supply, ((*consumer_supply)+4)->supply, ((*consumer_supply)+5)->supply,
			((*consumer_supply)+6)->supply, ((*consumer_supply)+7)->supply, ((*consumer_supply)+8)->supply,
			((*consumer_supply)+9)->supply);
		if (num <= -1) {
			printk("parse ldo%d from sysconfig failed\n", ldo_index);
			return -1;
		} else {
			(*(axp_init_data+(ldo_index-1))).axp_reg_init_data.num_consumer_supplies = num;
			DBG_PSY_MSG(DEBUG_REGU, "%s: num = %d\n", __func__, num);
			for (i = 0; i < num; i++) {
				if (i == (num-1)) {
					DBG_PSY_MSG(DEBUG_REGU, "%s  \n", ((*consumer_supply)+i)->supply);
				} else {
					DBG_PSY_MSG(DEBUG_REGU, "%s  ", ((*consumer_supply)+i)->supply);
				}
			}
		}
	}
	return 0;
}

struct axp_funcdev_info (* axp19_regu_init(void))[8]
{
	int ret = 0;

	ret = axp19_regu_fetch_sysconfig_para("pmu1_regu", &axp19_regu_data, (struct axp_reg_init *)(&axp_regl_init_data));
	if (0 != ret)
		return NULL;

	return &axp_regldevs;
}

void axp19_regu_exit(void)
{
	return;
}


