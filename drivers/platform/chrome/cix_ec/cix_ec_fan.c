// SPDX-License-Identifier: GPL-2.0
/*
 * fan driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd..
 */

#include <linux/module.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#define MODE_NAME_LEN			16
#define MODE_OFFSET			2

enum {
	MUTE_MODE,
	NORMAL_MODE,
	PERF_MODE,
	MAX_MODE,
};

struct cix_ec_fan_data {
	struct cros_ec_device *ec;
	struct kobject *cix_kobj;
	int mode;
};

struct cix_ec_fan_data *cix_ec_fan_data;

static char *fan_available_mode[] = {
	"mute",
	"normal",
	"performance"
};

static int cix_set_fan_mode(struct cros_ec_device *ec, int mode)
{
	int rc;
	struct {
		struct cros_ec_command msg;
		struct ec_fan_info params;
	} __packed buf;
	struct ec_fan_info *params = &buf.params;
	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_THERMAL_AUTO_FAN_CTRL;
	msg->insize = 0;
	msg->outsize = sizeof(*params);
	params->mode = mode + MODE_OFFSET;
	rc = cros_ec_cmd_xfer_status(ec, msg);
	if (rc < 0)
		return rc;

	return 0;
}

static ssize_t mode_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct cix_ec_fan_data *fd = cix_ec_fan_data;

	if (fd->mode < 0 || fd->mode >= MAX_MODE)
		return -EINVAL;

	return sprintf(buf, "%s\n", fan_available_mode[fd->mode]);
}

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct cix_ec_fan_data *fd = cix_ec_fan_data;
	char mode[MODE_NAME_LEN + 1];
	int ret;
	int i;

	ret = sscanf(buf, "%s", mode);
	if (ret != 1)
		return -EINVAL;

	for (i = 0; i < MAX_MODE; i++) {
		if (!strcmp(mode, fan_available_mode[i]))
			break;
	}

	if (i < MAX_MODE) {
		fd->mode = i;
		ret = cix_set_fan_mode(fd->ec, fd->mode);
		if (ret < 0)
			pr_err("Failed: set fan mode!\n");
	}

	return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t available_mode_show(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t count = 0;
	int i;

	for (i = 0; i < MAX_MODE; i++) {
		count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			           "%s ", fan_available_mode[i]);
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

static int cix_ec_fan_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec;

	cix_ec_fan_data =
		devm_kzalloc(&pdev->dev, sizeof(*cix_ec_fan_data), GFP_KERNEL);
	if (!cix_ec_fan_data)
		return -ENOMEM;
	ec = dev_get_drvdata(pdev->dev.parent);
	cix_ec_fan_data->ec = ec;
	cix_ec_fan_data->mode = NORMAL_MODE;
	platform_set_drvdata(pdev, cix_ec_fan_data);

	cix_ec_fan_data->cix_kobj = kobject_create_and_add("cix_fan", NULL);
	if (!cix_ec_fan_data->cix_kobj)
		return -ENOMEM;
	CREATE_SYSFS_FILE(cix_ec_fan_data->cix_kobj, mode);
	CREATE_SYSFS_FILE(cix_ec_fan_data->cix_kobj, available_mode);
	return 0;
}

static int cix_ec_fan_remove(struct platform_device *pdev)
{
	struct cix_ec_fan_data *fd = cix_ec_fan_data;

	sysfs_remove_file(fd->cix_kobj, &dev_attr_mode.attr);
	sysfs_remove_file(fd->cix_kobj, &dev_attr_available_mode.attr);
	kobject_put(fd->cix_kobj);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cix_ec_fan_of_match[] = {
	{ .compatible = "cix,cix-ec-fan" },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_fan_of_match);
#endif

static struct platform_driver cix_ec_fan_driver = {
	.probe = cix_ec_fan_probe,
	.remove = cix_ec_fan_remove,
	.driver = {
		.name = "cix-ec-fan",
		.of_match_table = of_match_ptr(cix_ec_fan_of_match),
	},
};
module_platform_driver(cix_ec_fan_driver);

MODULE_ALIAS("platform:cix-fan");
MODULE_DESCRIPTION("CIX EC fan");
MODULE_LICENSE("GPL v2");
