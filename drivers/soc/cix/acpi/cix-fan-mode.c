// SPDX-License-Identifier: GPL-2.0
/*
 * fan driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd..
 */

#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define MODE_NAME_LEN 16
#define MODE_ARGS_MAX 3

enum {
	MUTE_MODE,
	NORMAL_MODE,
	PERF_MODE,
	MANUAL_MODE,
	MAX_MODE,
};

struct cix_fan_mode {
	char *name;
	char *set_method;
	char *get_method;
	int set_argc;
	int get_argc;
};

struct cix_fan_mode_data {
	struct kobject *cix_kobj;
	struct cix_fan_mode fmode[MAX_MODE];
	int mode;
	int duty;
	int type;
	int index;
};

struct cix_fan_mode_data cix_fan_mdata = {
	.mode = NORMAL_MODE,
	.fmode[MUTE_MODE] = {
		.name = "mute",
		.set_method = "SFMT",
		.set_argc = 0,
	},
	.fmode[NORMAL_MODE] = {
		.name = "normal",
		.set_method = "SFAT",
		.set_argc = 0,
	},
	.fmode[PERF_MODE] = {
		.name = "performance",
		.set_method = "SFPF",
		.set_argc = 0,
	},
	.fmode[MANUAL_MODE] = {
		.name = "manual",
	}
};

static int cix_has_fan_control_device(void)
{
	acpi_status status;
	acpi_handle handle;

	status = acpi_get_handle(NULL, "\\_SB.HWMN", &handle);
	if (ACPI_FAILURE(status))
		return 0;

	return 1;
}

static int cix_set_fan_mode(struct device *dev, int mode)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;
	struct cix_fan_mode *fmode = &fmdata->fmode[mode];
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	acpi_status status;
	acpi_handle handle;

	if (!fmode->set_method)
		return -ENOENT;

	status = acpi_get_handle(NULL, "\\_SB.HWMN", &handle);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	status = acpi_evaluate_object(handle, fmode->set_method, NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		pr_err("set fan mode failed: %s\n",
					acpi_format_exception(status));
		return -EINVAL;
	}
	kfree(buffer.pointer);

	fmdata->mode = mode;

	return 0;
}

static ssize_t mode_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;
	int mode = fmdata->mode;

	if (mode < 0 || mode >= MAX_MODE)
		return -EINVAL;

	return sprintf(buf, "%s\n", fmdata->fmode[mode].name);
}

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;
	char mode[MODE_NAME_LEN + 1];
	int ret, i, argc;

	argc = sscanf(buf, "%s", mode);
	if (argc != 1)
		return -EINVAL;

	for (i = 0; i < MAX_MODE; i++)
		if (!strcmp(mode, fmdata->fmode[i].name))
			break;
	if (i >= MAX_MODE)
		return -EINVAL;

	ret = cix_set_fan_mode(dev, i);
	if (ret < 0)
		pr_err("Failed: set fan mode!\n");

	pr_info("set fan mode %s\n", mode);

	return count;
}
static DEVICE_ATTR_RW(mode);

static int cix_set_fan_pwm(struct device *dev, int duty, int type, int index)
{
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_object_list obj_list;
	union acpi_object objs[3];
	acpi_status status;
	acpi_handle handle;

	objs[0].type = ACPI_TYPE_INTEGER;
	objs[0].integer.value = duty;
	objs[1].type = ACPI_TYPE_INTEGER;
	objs[1].integer.value = type;
	objs[2].type = ACPI_TYPE_INTEGER;
	objs[2].integer.value = index;
	obj_list.pointer = objs;
	obj_list.count = 3;

	status = acpi_get_handle(NULL, "\\_SB.HWMN", &handle);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	status = acpi_evaluate_object(handle, "SFPW", &obj_list, &buffer);
	if (ACPI_FAILURE(status)) {
		pr_err("set fan pwm failed: %s\n",
					acpi_format_exception(status));
		return -EINVAL;
	}
	kfree(buffer.pointer);

	return 0;
}

static int cix_get_fan_pwm(struct device *dev)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_object_list obj_list;
	union acpi_object objs[2], *ret_value;
	int duty;
	acpi_status status;
	acpi_handle handle;

	objs[0].type = ACPI_TYPE_INTEGER;
	objs[0].integer.value = fmdata->type;
	objs[1].type = ACPI_TYPE_INTEGER;
	objs[1].integer.value = fmdata->index;
	obj_list.pointer = objs;
	obj_list.count = 2;

	status = acpi_get_handle(NULL, "\\_SB.HWMN", &handle);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	status = acpi_evaluate_object(handle, "GFPW", &obj_list, &buffer);
	if (ACPI_FAILURE(status)) {
		pr_err("set fan pwm failed: %s\n",
					acpi_format_exception(status));
		return -EINVAL;
	}
	ret_value = (union acpi_object *)buffer.pointer;
	if (ret_value->type != ACPI_TYPE_INTEGER)
		duty = -EINVAL;
	else
		duty = ret_value->integer.value;
	kfree(buffer.pointer);

	return duty;
}

static ssize_t pwm_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cix_get_fan_pwm(dev));
}

static ssize_t pwm_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;
	int ret, argc, duty, type, index;

	argc = sscanf(buf, "%d %d %d", &duty, &type, &index);
	if (argc < 3)
		return -EINVAL;

	ret = cix_set_fan_pwm(dev, duty, type, index);
	if (ret < 0)
		return ret;

	pr_info("set fan pwm %d %d %d\n", duty, type, index);

	fmdata->duty = duty;
	fmdata->type = type;
	fmdata->duty = duty;
	fmdata->mode = MANUAL_MODE;

	return count;
}
static DEVICE_ATTR_RW(pwm);

static ssize_t available_mode_show(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;
	ssize_t count = 0;
	int i;

	for (i = 0; i < MAX_MODE; i++) {
		count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
				"%s ", fmdata->fmode[i].name);
	}

	/* Truncate the trailing space */
	if (count)
		count--;

	count += sprintf(&buf[count], "\n");

	return count;
}
static DEVICE_ATTR_RO(available_mode);

#define CREATE_SYSFS_FILE(kobj, name)					\
{									\
	int ret;							\
	ret = sysfs_create_file(kobj, &dev_attr_##name.attr);		\
	if (ret < 0) {							\
		pr_warn("Unable to create attr(%s)\n", "##name");	\
	}								\
}									\

static int __init cix_fan_mode_init(void)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;

	if (!cix_has_fan_control_device())
		return 0;

	fmdata->mode = NORMAL_MODE;
	fmdata->type = 0;
	fmdata->index = 0;
	fmdata->cix_kobj = kobject_create_and_add("cix_fan", NULL);
	if (!fmdata->cix_kobj)
		return -ENOMEM;
	CREATE_SYSFS_FILE(fmdata->cix_kobj, mode);
	CREATE_SYSFS_FILE(fmdata->cix_kobj, available_mode);
	CREATE_SYSFS_FILE(fmdata->cix_kobj, pwm);

	return 0;
}
module_init(cix_fan_mode_init);

static void __exit cix_fan_mode_exit(void)
{
	struct cix_fan_mode_data *fmdata = &cix_fan_mdata;

	sysfs_remove_file(fmdata->cix_kobj, &dev_attr_mode.attr);
	sysfs_remove_file(fmdata->cix_kobj, &dev_attr_available_mode.attr);
	sysfs_remove_file(fmdata->cix_kobj, &dev_attr_pwm.attr);
	kobject_put(fmdata->cix_kobj);
}
module_exit(cix_fan_mode_exit);

MODULE_ALIAS("platform:cix-fan-mode");
MODULE_DESCRIPTION("CIX Fan Mode");
MODULE_LICENSE("GPL v2");
