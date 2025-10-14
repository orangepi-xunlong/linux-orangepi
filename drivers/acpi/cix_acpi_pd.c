// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright 2024 Cix Technology Group Co., Ltd.
 *
 **/

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sizes.h>
#include <linux/acpi.h>
#include <linux/acpi_pd-scmi.h>

#define GET_POWER_STATE 0x00000001
#define SET_POWER_STATE 0X00000002

#define POWER_REVISION_ID		1
#define PD_MASK (0xffffffff)
#define SUCCESS 0

static const guid_t cix_pd_guid =
	GUID_INIT(0x854bcf86, 0x4dbf, 0x4f3d, 0xa1, 0x9a, 0xfb, 0xef, 0x3c, 0x7c, 0xe7, 0x31);

static int acpi_pd_config_set(struct device *dev, u32 pd_id, u32 state)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	union acpi_object *out_obj,in_obj;
	u32 *buf_val;
	int ret = 0;
	union acpi_object args[2] = {
		{ .type = ACPI_TYPE_INTEGER, },
		{ .type = ACPI_TYPE_INTEGER, },
	};

	args[0].integer.value = pd_id;
	args[1].integer.value = state;

	in_obj.type = ACPI_TYPE_PACKAGE;
	in_obj.package.count = 2;
	in_obj.package.elements = args;
	out_obj = acpi_evaluate_dsm_typed(handle, &cix_pd_guid,
			POWER_REVISION_ID, SET_POWER_STATE,
			&in_obj, ACPI_TYPE_BUFFER);
	if (!out_obj) {
		ret = -EINVAL;
		dev_err(dev, "Failed to evaluate DSM object,err:%d!\n", ret);
		return ret;
	}

	if (out_obj->buffer.type == ACPI_TYPE_BUFFER) {
		buf_val = (u32 *)out_obj->buffer.pointer;
	} else {
		ret = AE_ERROR;
		goto free_acpi_buffer;
	}

	if(buf_val[0] != SUCCESS) {
		ret = AE_NOT_FOUND;
		dev_err(dev, "ACPI return err:%d,buf_val:%d\n", ret, buf_val[0]);
		goto free_acpi_buffer;
	}

free_acpi_buffer:
	ACPI_FREE(out_obj);
	return ret;
}

static int acpi_get_pd_state(struct device *dev, u32 pd_id)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	union acpi_object *out_obj, in_obj;
	u64 ret;
	u32 *buf_val;
	union acpi_object args[1] = {
		{ .type = ACPI_TYPE_INTEGER, },
	};

	args[0].integer.value = pd_id;
	in_obj.type = ACPI_TYPE_PACKAGE;
	in_obj.package.count = 1;
	in_obj.package.elements = args;

	out_obj = acpi_evaluate_dsm_typed(handle, &cix_pd_guid,
			POWER_REVISION_ID, GET_POWER_STATE,
			&in_obj, ACPI_TYPE_BUFFER);
	if (!out_obj) {
		ret = -EINVAL;
		dev_err(dev, "Failed to evaluate DSM object,err!\n");
		return ret;
	}

	if (out_obj->buffer.type == ACPI_TYPE_BUFFER) {
		buf_val = (u32 *) out_obj->buffer.pointer;
	} else {
		ret = AE_ERROR;
		goto free_acpi_buffer;
	}

	if(buf_val[0] == SUCCESS)
		ret = buf_val[1] & PD_MASK;

free_acpi_buffer:
	ACPI_FREE(out_obj);
	return ret;
}

static int acpi_pd_power(struct device *dev, u32 pd_id, bool power_on)
{
	u32 ret_state, state;
	int ret;

	if (power_on)
		state = ACPI_PD_ON;
	else
		state = ACPI_PD_OFF;

	ret = acpi_pd_config_set(dev, pd_id, state);

	if (!ret)
		ret_state = acpi_get_pd_state(dev, pd_id);
	if (state != ret_state)
		return -EIO;
	return ret;
}

int acpi_pd_off(struct device *dev, u32 pd_id)
{
	return acpi_pd_power(dev, pd_id, false);
}
EXPORT_SYMBOL_GPL(acpi_pd_off);

int acpi_pd_on(struct device *dev, u32 pd_id)
{
	return acpi_pd_power(dev, pd_id, true);
}
EXPORT_SYMBOL_GPL(acpi_pd_on);
