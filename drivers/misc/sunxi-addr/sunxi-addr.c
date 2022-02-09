/*
 * The driver of SUNXI NET MAC ADDR Manager.
 *
 * Copyright (C) 2013 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#define DEBUG

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define ADDR_MGT_DBG(fmt, arg...) printk(KERN_DEBUG "[ADDR_MGT] %s: " fmt "\n",\
				__func__, ## arg)
#define ADDR_MGT_ERR(fmt, arg...) printk(KERN_ERR "[ADDR_MGT] %s: " fmt "\n",\
				__func__, ## arg)

#define MODULE_CUR_VERSION  "v1.0.9"

#define MATCH_STR_LEN       20
#define ADDR_VAL_LEN        6
#define ADDR_STR_LEN        18
#define ID_LEN              16
#define HASH_LEN            32

#define TYPE_ANY            0
#define TYPE_BURN           1
#define TYPE_IDGEN          2
#define TYPE_USER           3
#define TYPE_RAND           4

#define ADDR_FMT_STR        0
#define ADDR_FMT_VAL        1

#define IS_TYPE_INVALID(x)  ((x < TYPE_ANY) || (x > TYPE_RAND))

#define ADDR_CLASS_ATTR_ADD(name) \
static ssize_t addr_##name##_show(struct class *class, \
		struct class_attribute *attr, char *buffer) \
{ \
	char addr[ADDR_STR_LEN]; \
	if (IS_TYPE_INVALID(get_addr_by_name(ADDR_FMT_STR, addr, #name))) \
		return 0; \
	return sprintf(buffer, "%.17s\n", addr); \
} \
static ssize_t addr_##name##_store(struct class *class, \
		struct class_attribute *attr, \
		const char *buffer, size_t count) \
{ \
	if (count != ADDR_STR_LEN) { \
		ADDR_MGT_ERR("Length wrong."); \
		return -EINVAL; \
	} \
	set_addr_by_name(TYPE_USER, ADDR_FMT_STR, buffer, #name); \
	return count; \
} \
static CLASS_ATTR_RW(addr_##name);

struct addr_mgt_info {
	unsigned int type_def;
	unsigned int type_cur;
	unsigned int flag;
	char *addr;
	char *name;
};

static struct addr_mgt_info info[] = {
	{TYPE_ANY, TYPE_ANY, 1, NULL, "wifi"},
	{TYPE_ANY, TYPE_ANY, 0, NULL, "bt"  },
	{TYPE_ANY, TYPE_ANY, 1, NULL, "eth" },
};

extern int hmac_sha256(const uint8_t *plaintext, ssize_t psize, uint8_t *output);
extern int sunxi_get_soc_chipid(unsigned char *chipid);

static int addr_parse(int fmt, const char *addr, int check)
{
	char val_buf[ADDR_VAL_LEN];
	char cmp_buf[ADDR_VAL_LEN];
	int  ret = ADDR_VAL_LEN;

	if (fmt == ADDR_FMT_STR)
		ret = sscanf(addr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
					&val_buf[0], &val_buf[1], &val_buf[2],
					&val_buf[3], &val_buf[4], &val_buf[5]);
	else
		memcpy(val_buf, addr, ADDR_VAL_LEN);

	if (ret != ADDR_VAL_LEN)
		return -1;

	if (check && (val_buf[0] & 0x3))
		return -1;

	memset(cmp_buf, 0x00, ADDR_VAL_LEN);
	if (memcmp(val_buf, cmp_buf, ADDR_VAL_LEN) == 0)
		return -1;

	memset(cmp_buf, 0xFF, ADDR_VAL_LEN);
	if (memcmp(val_buf, cmp_buf, ADDR_VAL_LEN) == 0)
		return -1;

	return 0;
}

static struct addr_mgt_info *addr_find_by_name(char *name)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(info); i++) {
		if (strcmp(info[i].name, name) == 0)
			return &info[i];
	}
	return NULL;
}

static int get_addr_by_name(int fmt, char *addr, char *name)
{
	struct addr_mgt_info *t;

	t = addr_find_by_name(name);
	if (t == NULL) {
		ADDR_MGT_ERR("can't find addr named: %s", name);
		return -1;
	}

	if (IS_TYPE_INVALID(t->type_cur)) {
		ADDR_MGT_ERR("addr type invalid");
		return -1;
	}

	if (addr_parse(ADDR_FMT_VAL, t->addr, t->flag)) {
		ADDR_MGT_ERR("addr parse fail(%s)", t->addr);
		return -1;
	}

	if (fmt == ADDR_FMT_STR)
		sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X",
				t->addr[0], t->addr[1], t->addr[2],
				t->addr[3], t->addr[4], t->addr[5]);
	else
		memcpy(addr, t->addr, ADDR_VAL_LEN);

	return t->type_cur;
}

static int set_addr_by_name(int type, int fmt, const char *addr, char *name)
{
	struct addr_mgt_info *t;

	t = addr_find_by_name(name);
	if (t == NULL) {
		ADDR_MGT_ERR("can't find addr named: %s", name);
		return -1;
	}

	if (addr_parse(fmt, addr, t->flag)) {
		ADDR_MGT_ERR("addr parse fail(%s)", addr);
		return -1;
	}

	t->type_cur = type;
	if (fmt == ADDR_FMT_STR)
		sscanf(addr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
				&t->addr[0], &t->addr[1], &t->addr[2],
				&t->addr[3], &t->addr[4], &t->addr[5]);
	else
		memcpy(t->addr, addr, ADDR_VAL_LEN);

	return 0;
}

int get_custom_mac_address(int fmt, char *name, char *addr)
{
	return get_addr_by_name(fmt, addr, name);
}
EXPORT_SYMBOL_GPL(get_custom_mac_address);

static int addr_factory(struct device_node *np,
			int idx, int type, char *mac, char *name)
{
	int  ret, i;
	char match[MATCH_STR_LEN];
	const char *p;
	char id[ID_LEN], hash[HASH_LEN], cmp_buf[ID_LEN];
	struct timespec64 curtime;

	switch (type) {
	case TYPE_BURN:
		sprintf(match, "addr_%s", name);
		ret = of_property_read_string_index(np, match, 0, &p);
		if (ret)
			return -1;

		ret = sscanf(p, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
			&mac[0], &mac[1], &mac[2],
			&mac[3], &mac[4], &mac[5]);

		if (ret != ADDR_VAL_LEN)
			return -1;
		break;
	case TYPE_IDGEN:
		if (idx > HASH_LEN / ADDR_VAL_LEN - 1)
			return -1;
		if (sunxi_get_soc_chipid(id))
			return -1;
		memset(cmp_buf, 0x00, ID_LEN);
		if (memcmp(id, cmp_buf, ID_LEN) == 0)
			return -1;
		if (hmac_sha256(id, ID_LEN, hash))
			return -1;
		memcpy(mac, &hash[idx * ADDR_VAL_LEN], ADDR_VAL_LEN);
		break;
	case TYPE_RAND:
		for (i = 0; i < ADDR_VAL_LEN; i++) {
			ktime_get_real_ts64(&curtime);
			mac[i] = (char)curtime.tv_nsec;
		}
		break;
	default:
		ADDR_MGT_ERR("unsupport type: %d", type);
		return -1;
	}
	return 0;
}

static int addr_init(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int  type, i, j;
	char match[MATCH_STR_LEN];
	char addr[ADDR_VAL_LEN];
	int  type_tab[] = {TYPE_BURN, TYPE_IDGEN, TYPE_RAND};

	/* init addr type and value */
	for (i = 0; i < ARRAY_SIZE(info); i++) {
		sprintf(match, "type_addr_%s", info[i].name);
		if (of_property_read_u32(np, match, &type)) {
			ADDR_MGT_DBG("Failed to get type_def_%s, use default: %d",
						info[i].name, info[i].type_def);
		} else {
			info[i].type_def = type;
			info[i].type_cur = type;
		}

		if (IS_TYPE_INVALID(info[i].type_def))
			return -1;
		if (info[i].type_def != TYPE_ANY) {
			if (addr_factory(np, i, info[i].type_def, addr, info[i].name))
				return -1;
		} else {
			for (j = 0; j < ARRAY_SIZE(type_tab); j++) {
				if (!addr_factory(np, i, type_tab[j], addr, info[i].name)) {
					info[i].type_cur = type_tab[j];
					break;
				}
			}
		}

		if (info[i].flag)
			addr[0] &= 0xFC;

		if (addr_parse(ADDR_FMT_VAL, addr, info[i].flag))
			return -1;
		else {
			info[i].addr = devm_kzalloc(&pdev->dev, ADDR_VAL_LEN, GFP_KERNEL);
			memcpy(info[i].addr, addr, ADDR_VAL_LEN);
		}
	}
	return 0;
}

static ssize_t summary_show(struct class *class,
				struct class_attribute *attr, char *buffer)
{
	int i = 0, ret = 0;

	ret += sprintf(&buffer[ret], "name cfg cur address\n");
	for (i = 0; i < ARRAY_SIZE(info); i++) {
		ret += sprintf(&buffer[ret],
			"%4s  %d   %d  %02X:%02X:%02X:%02X:%02X:%02X\n",
			info[i].name,   info[i].type_def, info[i].type_cur,
			info[i].addr[0], info[i].addr[1], info[i].addr[2],
			info[i].addr[3], info[i].addr[4], info[i].addr[5]);
	}
	return ret;
}
static CLASS_ATTR_RO(summary);

ADDR_CLASS_ATTR_ADD(wifi);
ADDR_CLASS_ATTR_ADD(bt);
ADDR_CLASS_ATTR_ADD(eth);

static struct attribute *addr_class_attrs[] = {
	&class_attr_summary.attr,
	&class_attr_addr_wifi.attr,
	&class_attr_addr_bt.attr,
	&class_attr_addr_eth.attr,
	NULL
};
ATTRIBUTE_GROUPS(addr_class);

static struct class addr_class = {
	.name = "addr_mgt",
	.owner = THIS_MODULE,
	.class_groups = addr_class_groups,
};

static const struct of_device_id addr_mgt_ids[] = {
	{ .compatible = "allwinner,sunxi-addr_mgt" },
	{ /* Sentinel */ }
};

static int addr_mgt_probe(struct platform_device *pdev)
{
	int status;

	ADDR_MGT_DBG("module version: %s", MODULE_CUR_VERSION);
	status = class_register(&addr_class);
	if (status < 0) {
		ADDR_MGT_ERR("class register error, status: %d.", status);
		return -1;
	}

	if (addr_init(pdev)) {
		ADDR_MGT_ERR("failed to init addr.");
		class_unregister(&addr_class);
		return -1;
	}
	ADDR_MGT_DBG("success.");
	return 0;
}

static int addr_mgt_remove(struct platform_device *pdev)
{
	class_unregister(&addr_class);
	return 0;
}

static struct platform_driver addr_mgt_driver = {
	.probe  = addr_mgt_probe,
	.remove = addr_mgt_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name  = "sunxi-addr-mgt",
		.of_match_table = addr_mgt_ids,
	},
};

module_platform_driver_probe(addr_mgt_driver, addr_mgt_probe);

MODULE_AUTHOR("Allwinnertech");
MODULE_DESCRIPTION("Network MAC Addess Manager");
MODULE_LICENSE("GPL");
