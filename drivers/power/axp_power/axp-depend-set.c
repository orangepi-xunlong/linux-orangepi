/*
 * Regulators dependence driver for allwinnertech AXP
 *
 * Copyright (C) 2014 allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/regulator/consumer.h>
#include <linux/power/axp_depend.h>

/*
 * function: add the pwr_dm specified by input id to sys_pwr_dm;
 *
 * */
int add_sys_pwr_dm(const char *id)
{
	int ret = 0, sys_id_conut = 0;
	char ldo_name[20] = {0};
	unsigned int sys_pwr_mask = 0;

	sys_id_conut = axp_check_sys_id(id);
	if (0 > sys_id_conut) {
		printk(KERN_ERR "%s: %s not sys id.\n", __func__, id);
		return -1;
	} else {
		sys_pwr_mask = get_sys_pwr_dm_mask();
		if (sys_pwr_mask & (0x1 << sys_id_conut)) {
			printk(KERN_INFO "%s: sys_pwr_mask = 0x%x, sys_mask already set.\n", __func__, sys_pwr_mask);
			return 1;
		}
	}

	ret = get_ldo_name(id, (char *)&ldo_name);
	if (ret < 0) {
		printk(KERN_ERR "%s: get ldo name failed\n", __func__);
		return -1;
	}

	ret = check_ldo_alwayson((const char *)&ldo_name);
	if (ret == 0) {
		if (set_ldo_alwayson((const char *)&ldo_name, 1)) {
			printk(KERN_ERR "%s: %s set_ldo_alwayson failed.\n", __func__, ldo_name);
			return -1;
		}
	} else if (ret == 1) {
		printk(KERN_ERR "%s: %s ldo already alwayson.\n", __func__, ldo_name);
	} else {
		printk(KERN_ERR "%s: %s set err.\n", __func__, ldo_name);
		return -1;
	}

	set_sys_pwr_dm_mask(sys_id_conut, 1);
	return 0;
}

int del_sys_pwr_dm(const char *id)
{
	int ret = 0, sys_id_conut = 0, i = 0;
	char ldo_name[20] = {0};
	char sys_ldo_name[20] = {0};
	unsigned int sys_pwr_mask = 0;
	char * sys_id;

	sys_id_conut = axp_check_sys_id(id);
	if (0 > sys_id_conut) {
		printk(KERN_ERR "%s: %s not sys id.\n", __func__, id);
		return -1;
	} else {

		sys_pwr_mask = get_sys_pwr_dm_mask();
		if (!(sys_pwr_mask & (0x1 << sys_id_conut)))
			return 1;
	}

	ret = get_ldo_name(id, (char *)&ldo_name);
	if (ret < 0) {
		printk(KERN_ERR "%s: get ldo name  for id: %s failed\n", __func__, id);
		return -1;
	}


	ret = check_ldo_alwayson((const char *)&ldo_name);
	if (ret == 0) {
		printk(KERN_ERR "%s: %s ldo is already not alwayson.\n", __func__, ldo_name);
	} else if (ret == 1) {
		for (i=0; i <VCC_MAX_INDEX; i++) {
			if (sys_id_conut == i)
				continue;
			if(is_sys_pwr_dm_active(i)) {
				sys_id = get_sys_pwr_dm_id(i);
				ret = get_ldo_name(sys_id, (char *)&sys_ldo_name);
				if (ret < 0) {
					printk(KERN_ERR "%s: get sys_ldo_name failed\n", __func__);
					return -1;
				}
				if (strcmp(sys_ldo_name, ldo_name) == 0) {
					set_sys_pwr_dm_mask(sys_id_conut, 0);
					return 0;
				}
			}
		}
		if (set_ldo_alwayson((const char *)&ldo_name, 0)) {
			printk(KERN_ERR "%s: %s set_ldo_alwayson failed.\n", __func__, ldo_name);
			return -1;
		}
	} else {
		printk(KERN_ERR "%s: %s set err.\n", __func__, ldo_name);
		return -1;
	}

	set_sys_pwr_dm_mask(sys_id_conut, 0);

	return 0;
}

/*
 *  function: judge whether pwr_dm is part of sys_pwr_domain.
 *  input: pwr_dm name, such as: "vdd_sys".
 *  return:
 *	nonnegative number: the sys_pwr_domain bitmap.
 *	-1: the input pwr_dm is not belong to sys_pwr_domain.
 *
 * */
int is_sys_pwr_dm_id(const char *id)
{
	int sys_id_conut = 0;

	sys_id_conut = axp_check_sys_id(id);
	if (0 <= sys_id_conut) {
		return sys_id_conut;
	} else {
		return -1;
	}
}

/*
 *  function: judge whether sys_pwr_domain is active.
 *  input: sys_pwr_domain bitmap.
 *  return:
 *	1: the input sys_pwr_domain is active.
 *	0: the input sys_pwr_domain is not active.
 *
 * */
int is_sys_pwr_dm_active(unsigned int bitmap)
{
	unsigned int sys_pwr_mask = 0;

	sys_pwr_mask = get_sys_pwr_dm_mask();
	if (sys_pwr_mask & (0x1 << bitmap))
			return 1;
	else
			return 0;
}
/*
 *  function:  get sys_pwr_dm_id name.
 *  input: sys_pwr_domain bitmap.
 *  return:
 *       failed:  NULL.
 *       success: sys_pwr_dm_id.
 * */
char *get_sys_pwr_dm_id(unsigned int bitmap)
{
	return axp_get_sys_id(bitmap);
}

