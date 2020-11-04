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
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <linux/sunxi-sid.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define ADDR_MGT_DBG(fmt, arg...) pr_debug("[ADDR_MGT] %s: " fmt "\n",\
				__func__, ## arg)
#define ADDR_MGT_ERR(fmt, arg...) pr_err("[ADDR_MGT] %s: " fmt "\n",\
				__func__, ## arg)

#define BUFF_MAX            20
#define ID_LEN              16
#define HASH_LEN            16

#define TYPE_ANY            0
#define TYPE_BURN           1
#define TYPE_IDGEN          2
#define TYPE_USER           3

#define IS_TYPE_INVALID(x) ((x != TYPE_BURN) && \
							(x != TYPE_IDGEN) && \
							(x != TYPE_ANY) && \
							(x != TYPE_USER))

#define DEF_TYPE_ADDR_WIFI  TYPE_ANY
#define DEF_TYPE_ADDR_BT    TYPE_ANY
#define DEF_TYPE_ADDR_ETH   TYPE_ANY

struct addr_mgt_info {
	unsigned int type_def_wifi;
	unsigned int type_cur_wifi;
	unsigned int type_def_bt;
	unsigned int type_cur_bt;
	unsigned int type_def_eth;
	unsigned int type_cur_eth;
	char addr_wifi[BUFF_MAX];
	char addr_bt[BUFF_MAX];
	char addr_eth[BUFF_MAX];
};

static struct addr_mgt_info info;

static int addr_parse(const char *addr_str, int check)
{
	char addr[6];
	char cmp_buf[6];
	int ret = sscanf(addr_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
					&addr[0], &addr[1], &addr[2],
					&addr[3], &addr[4], &addr[5]);
	if (ret != 6)
		return -1;

	if (check && (addr[0] & 0x3))
		return -1;

	memset(cmp_buf, 0x00, 6);
	if (memcmp(addr, cmp_buf, 6) == 0)
		return -1;

	memset(cmp_buf, 0xFF, 6);
	if (memcmp(addr, cmp_buf, 6) == 0)
		return -1;

	return 0;
}

int get_wifi_custom_mac_address(char *addr_str)
{
	if (IS_TYPE_INVALID(info.type_cur_wifi) ||
		addr_parse(info.addr_wifi, 1))
		return -1;

	strcpy(addr_str, info.addr_wifi);
	return info.type_cur_wifi;
}
EXPORT_SYMBOL_GPL(get_wifi_custom_mac_address);

int get_bt_custom_mac_address(char *addr_str)
{
	if (IS_TYPE_INVALID(info.type_cur_bt) ||
		addr_parse(info.addr_bt, 0))
		return -1;

	strcpy(addr_str, info.addr_bt);
	return info.type_cur_bt;
}
EXPORT_SYMBOL_GPL(get_bt_custom_mac_address);

int get_eth_custom_mac_address(char *addr_str)
{
	if (IS_TYPE_INVALID(info.type_cur_eth) ||
		addr_parse(info.addr_eth, 1))
		return -1;

	strcpy(addr_str, info.addr_eth);
	return info.type_cur_eth;
}
EXPORT_SYMBOL_GPL(get_eth_custom_mac_address);

static ssize_t addr_type_show(struct class *class,
				struct class_attribute *attr, char *buffer)
{
	return sprintf(buffer,
			"TYPE_DEF_WIFI: %d, TYPE_DEF_BT: %d, TYPE_DEF_ETH: %d\n"
			"TYPE_CUR_WIFI: %d, TYPE_CUR_BT: %d, TYPE_CUR_ETH: %d\n",
			info.type_def_wifi, info.type_def_bt, info.type_def_eth,
			info.type_cur_wifi, info.type_cur_bt, info.type_cur_eth);
}

static ssize_t addr_wifi_show(struct class *class,
				struct class_attribute *attr, char *buffer)
{
	if (IS_TYPE_INVALID(info.type_cur_wifi) ||
		addr_parse(info.addr_wifi, 1)) {
		ADDR_MGT_ERR("Wrong addr type or addr value.");
		return 0;
	}

	return sprintf(buffer, "%.17s\n", info.addr_wifi);
}

static ssize_t addr_bt_show(struct class *class,
				struct class_attribute *attr, char *buffer)
{
	if (IS_TYPE_INVALID(info.type_cur_bt) ||
		addr_parse(info.addr_bt, 0)) {
		ADDR_MGT_ERR("Wrong addr type or addr value.");
		return 0;
	}

	return sprintf(buffer, "%.17s\n", info.addr_bt);
}

static ssize_t addr_eth_show(struct class *class,
				struct class_attribute *attr, char *buffer)
{
	if (IS_TYPE_INVALID(info.type_cur_eth) ||
		addr_parse(info.addr_eth, 1)) {
		ADDR_MGT_ERR("Wrong addr type or addr value.");
		return 0;
	}

	return sprintf(buffer, "%.17s\n", info.addr_eth);
}

static ssize_t addr_wifi_store(struct class *class,
				struct class_attribute *attr,
				const char *buffer, size_t count)
{
	if ((count > BUFF_MAX) || addr_parse(buffer, 1)) {
		ADDR_MGT_ERR("Format wrong.");
		return -EINVAL;
	}

	info.type_cur_wifi = TYPE_USER;
	return sprintf(info.addr_wifi, "%s", buffer);
}

static ssize_t addr_bt_store(struct class *class,
				struct class_attribute *attr,
				const char *buffer, size_t count)
{
	if ((count > BUFF_MAX) || addr_parse(buffer, 0)) {
		ADDR_MGT_ERR("Format wrong.");
		return -EINVAL;
	}

	info.type_cur_bt = TYPE_USER;
	return sprintf(info.addr_bt, "%s", buffer);
}

static ssize_t addr_eth_store(struct class *class,
				struct class_attribute *attr,
				const char *buffer, size_t count)
{
	if ((count > BUFF_MAX) || addr_parse(buffer, 1)) {
		ADDR_MGT_ERR("Format wrong.");
		return -EINVAL;
	}

	info.type_cur_eth = TYPE_USER;
	return sprintf(info.addr_eth, "%s", buffer);
}

static int __init setup_addr_wifi(char *str)
{
	if (str != NULL)
		strlcpy(info.addr_wifi, str, 18);
	return 0;
}
__setup("wifi_mac=", setup_addr_wifi);

static int __init setup_addr_bt(char *str)
{
	if (str != NULL)
		strlcpy(info.addr_bt, str, 18);
	return 0;
}
__setup("bt_mac=", setup_addr_bt);

static int __init setup_addr_eth(char *str)
{
	if (str != NULL)
		strlcpy(info.addr_eth, str, 18);
	return 0;
}
__setup("mac_addr=", setup_addr_eth);

static int hash_md5(char *buffer, ssize_t size, char *result)
{
	struct crypto_ahash *tfm;
	struct ahash_request *req;
	struct scatterlist sg;

	memset(result, 0, ID_LEN);

	tfm = crypto_alloc_ahash("md5", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		ADDR_MGT_ERR("Failed to alloc md5\n");
		return -1;
	}

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto out;

	ahash_request_set_callback(req, 0, NULL, NULL);

	if (crypto_ahash_init(req)) {
		ADDR_MGT_ERR("crypto_ahash_init() failed\n");
		goto out_req;
	}

	sg_init_one(&sg, buffer, size);
	ahash_request_set_crypt(req, &sg, result, size);

	if (crypto_ahash_update(req)) {
		ADDR_MGT_ERR("crypto_ahash_update() failed for id\n");
		goto out_req;
	}

	if (crypto_ahash_final(req)) {
		ADDR_MGT_ERR("crypto_ahash_final() failed for result\n");
		goto out_req;
	}

	ahash_request_free(req);
	crypto_free_ahash(tfm);
	return 0;

out_req:
	ahash_request_free(req);
out:
	crypto_free_ahash(tfm);
	return -1;
}

static int addr_init(void)
{
	char id_buf[ID_LEN];
	char hash_buf[HASH_LEN];

	if (sunxi_get_soc_chipid(id_buf))
		return -1;

	if (hash_md5(id_buf, ID_LEN, hash_buf))
		return -1;

	if ((info.type_cur_wifi == TYPE_IDGEN) ||
		((info.type_cur_wifi == TYPE_ANY) &&
		addr_parse(info.addr_wifi, 1))) {
		info.type_cur_wifi = TYPE_IDGEN;
		hash_buf[0] &= 0xFC;
		memset(info.addr_wifi, 0x0, BUFF_MAX);
		sprintf(info.addr_wifi, "%02X:%02X:%02X:%02X:%02X:%02X",
			hash_buf[0], hash_buf[1], hash_buf[2],
			hash_buf[3], hash_buf[4], hash_buf[5]);
	}

	memcpy(&id_buf[0], &hash_buf[0], HASH_LEN);
	hash_md5(id_buf, ID_LEN, hash_buf);
	if ((info.type_cur_bt == TYPE_IDGEN) ||
		((info.type_cur_bt == TYPE_ANY) &&
		addr_parse(info.addr_bt, 0))) {
		info.type_cur_bt = TYPE_IDGEN;
		memset(info.addr_bt, 0x0, BUFF_MAX);
		sprintf(info.addr_bt, "%02X:%02X:%02X:%02X:%02X:%02X",
			hash_buf[0], hash_buf[1], hash_buf[2],
			hash_buf[3], hash_buf[4], hash_buf[5]);
	}

	memcpy(&id_buf[0], &hash_buf[0], HASH_LEN);
	hash_md5(id_buf, ID_LEN, hash_buf);
	if ((info.type_cur_eth == TYPE_IDGEN) ||
		((info.type_cur_eth == TYPE_ANY) &&
		addr_parse(info.addr_eth, 1))) {
		info.type_cur_eth = TYPE_IDGEN;
		hash_buf[0] &= 0xFC;
		memset(info.addr_eth, 0x0, BUFF_MAX);
		sprintf(info.addr_eth, "%02X:%02X:%02X:%02X:%02X:%02X",
			hash_buf[0], hash_buf[1], hash_buf[2],
			hash_buf[3], hash_buf[4], hash_buf[5]);
	}

	return 0;
}

static struct class_attribute addr_class_attrs[] = {
	__ATTR(addr_type, 0444, addr_type_show, NULL),
	__ATTR(addr_wifi, 0644, addr_wifi_show, addr_wifi_store),
	__ATTR(addr_bt,   0644, addr_bt_show,   addr_bt_store),
	__ATTR(addr_eth,  0644, addr_eth_show,  addr_eth_store),
	__ATTR_NULL,
};

static struct class addr_class = {
	.name = "addr_mgt",
	.owner = THIS_MODULE,
	.class_attrs = addr_class_attrs,
};

static const struct of_device_id addr_mgt_ids[] = {
	{ .compatible = "allwinner,sunxi-addr_mgt" },
	{ /* Sentinel */ }
};

static int addr_mgt_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int status;

	info.type_def_wifi = DEF_TYPE_ADDR_WIFI;
	info.type_def_bt   = DEF_TYPE_ADDR_BT;
	info.type_def_eth  = DEF_TYPE_ADDR_ETH;

	if (of_property_read_u32(np, "type_addr_wifi", &info.type_def_wifi))
		ADDR_MGT_DBG("Failed to get type_def_wifi, use default: %d",
						DEF_TYPE_ADDR_WIFI);

	if (of_property_read_u32(np, "type_addr_bt", &info.type_def_bt))
		ADDR_MGT_DBG("Failed to get type_def_bt, use default: %d",
						DEF_TYPE_ADDR_BT);

	if (of_property_read_u32(np, "type_addr_eth", &info.type_def_eth))
		ADDR_MGT_DBG("Failed to get type_def_eth, use default: %d",
						DEF_TYPE_ADDR_ETH);

	info.type_cur_wifi = info.type_def_wifi;
	info.type_cur_bt   = info.type_def_bt;
	info.type_cur_eth  = info.type_def_eth;
	status = class_register(&addr_class);
	if (status < 0) {
		ADDR_MGT_ERR("class register error, status: %d.", status);
		return -1;
	}
	ADDR_MGT_DBG("success.");
	addr_init();
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

module_platform_driver(addr_mgt_driver);

MODULE_AUTHOR("Allwinnertech");
MODULE_DESCRIPTION("Network MAC Addess Manager");
MODULE_LICENSE("GPL");
