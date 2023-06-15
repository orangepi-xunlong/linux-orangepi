/*
 * cyttsp5_devtree.c
 * Parade TrueTouch(TM) Standard Product V5 Device Tree Support Module.
 * For use with Parade touchscreen controllers.
 * Supported parts include:
 * CYTMA5XX
 * CYTMA448
 * CYTMA445A
 * CYTT21XXX
 * CYTT31XXX
 *
 * Copyright (C) 2015 Parade Technologies
 * Copyright (C) 2013-2015 Cypress Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Parade Technologies at www.paradetech.com <ttdrivers@paradetech.com>
 *
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/slab.h>

/* cyttsp */
#include "cyttsp5_regs.h"
#include "cyttsp5_platform.h"

//#define ENABLE_VIRTUAL_KEYS

#define MAX_NAME_LENGTH		64

enum cyttsp5_device_type {
	DEVICE_MT,
	DEVICE_BTN,
	DEVICE_PROXIMITY,
	DEVICE_TYPE_MAX,
};

struct cyttsp5_device_pdata_func {
	void *(*create_and_get_pdata)(struct device_node *);
	void (*free_pdata)(void *);
};

struct cyttsp5_pdata_ptr {
	void **pdata;
};

#ifdef ENABLE_VIRTUAL_KEYS
static struct kobject *board_properties_kobj;

struct cyttsp5_virtual_keys {
	struct kobj_attribute kobj_attr;
	u16 *data;
	int size;
};
#endif

struct cyttsp5_extended_mt_platform_data {
	struct cyttsp5_mt_platform_data pdata;
#ifdef ENABLE_VIRTUAL_KEYS
	struct cyttsp5_virtual_keys vkeys;
#endif
};

static inline int get_btn_inp_dev_name(struct device_node *dev_node,
				       const char **inp_dev_name)
{
	return of_property_read_string(dev_node, "cy_btn_inp_dev_name",
				       inp_dev_name);
}

static inline int get_prox_inp_dev_name(struct device_node *dev_node,
					const char **inp_dev_name)
{
	return of_property_read_string(dev_node, "cy_prox_inp_dev_name",
				       inp_dev_name);
}

static inline int get_mt_inp_dev_name(struct device_node *dev_node,
				      const char **inp_dev_name)
{
	return of_property_read_string(dev_node, "cy_mt_inp_dev_name",
				       inp_dev_name);
}

static s16 *create_and_get_u16_array(struct device_node *dev_node,
				     const char *name, int *size)
{
	s16 *val_array;
	int len;
	int sz;
	int rc;
	int i;
	char pname[64];

	sprintf(pname, "%s_number", name);
	rc = of_property_read_u32(dev_node, pname, &len);
	if (rc)
		return NULL;

	sz = len; // sizeof(u32);
	pr_debug("%s: %s size:%d\n", __func__, name, sz);

	val_array = kcalloc(sz, sizeof(s16), GFP_KERNEL);
	if (!val_array) {
		rc = -ENOMEM;
		goto fail;
	}

	for (i = 0; i < sz; i++) {
		u32 v;
		sprintf(pname, "%s_%d", name, i + 1);
		rc = of_property_read_u32(dev_node, pname, &v);
		val_array[i] = v;
	}

	*size = sz;
	for (i = 0; i < sz; ++i) {
		pr_debug("%s: %d\n", __func__, val_array[i]);
	}

	return val_array;

fail:
	return ERR_PTR(rc);
}

static struct touch_framework *create_and_get_mt_touch_framework(
	struct device_node *dev_node)
{
	struct touch_framework *frmwrk;
	s16 *abs;
	int size;
	int rc;
#if 1
	abs = create_and_get_u16_array(dev_node, "cy_mt_abs", &size);
	if (IS_ERR_OR_NULL(abs))
		return (void *)abs;
#else
	abs = kcalloc(size, sizeof(s16), GFP_KERNEL);
	size = 45;
#endif

	/* Check for valid abs size */
	if (size % CY_NUM_ABS_SET) {
		rc = -EINVAL;
		goto fail_free_abs;
	}

	frmwrk = kzalloc(sizeof(*frmwrk), GFP_KERNEL);
	if (!frmwrk) {
		rc = -ENOMEM;
		goto fail_free_abs;
	}

	frmwrk->abs = abs;
	frmwrk->size = size;

	return frmwrk;

fail_free_abs:
	kfree(abs);

	return ERR_PTR(rc);
}

static struct touch_framework *create_and_get_prox_touch_framework(
	struct device_node *dev_node)
{
	struct touch_framework *frmwrk;
	s16 *abs;
	int size;
	int rc;

	abs = create_and_get_u16_array(dev_node, "cy_prox_abs", &size);
	if (IS_ERR_OR_NULL(abs))
		return (void *)abs;

	/* Check for valid abs size */
	if (size % CY_NUM_ABS_SET) {
		rc = -EINVAL;
		goto fail_free_abs;
	}

	frmwrk = kzalloc(sizeof(*frmwrk), GFP_KERNEL);
	if (!frmwrk) {
		rc = -ENOMEM;
		goto fail_free_abs;
	}

	frmwrk->abs = abs;
	frmwrk->size = size;

	return frmwrk;

fail_free_abs:
	kfree(abs);

	return ERR_PTR(rc);
}

static void free_touch_framework(struct touch_framework *frmwrk)
{
	kfree(frmwrk->abs);
	kfree(frmwrk);
}

#ifdef ENABLE_VIRTUAL_KEYS
#define VIRTUAL_KEY_ELEMENT_SIZE	5
static ssize_t virtual_keys_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	struct cyttsp5_virtual_keys *vkeys = container_of(attr,
					     struct cyttsp5_virtual_keys, kobj_attr);
	u16 *data = vkeys->data;
	int size = vkeys->size;
	int index;
	int i;

	index = 0;
	for (i = 0; i < size; i += VIRTUAL_KEY_ELEMENT_SIZE)
		index += scnprintf(buf + index, CY_MAX_PRBUF_SIZE - index,
				   "0x01:%d:%d:%d:%d:%d\n",
				   data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4]);

	return index;
}

static int setup_virtual_keys(struct device_node *dev_node,
			      const char *inp_dev_name, struct cyttsp5_virtual_keys *vkeys)
{
	char *name;
	u16 *data;
	int size;
	int rc;

	data = create_and_get_u16_array(dev_node, "cy_virtual_keys", &size);
	if (data == NULL)
		return 0;
	else if (IS_ERR(data)) {
		rc = PTR_ERR(data);
		goto fail;
	}

	/* Check for valid virtual keys size */
	if (size % VIRTUAL_KEY_ELEMENT_SIZE) {
		rc = -EINVAL;
		goto fail_free_data;
	}

	name = kzalloc(MAX_NAME_LENGTH, GFP_KERNEL);
	if (!name) {
		rc = -ENOMEM;
		goto fail_free_data;
	}

	snprintf(name, MAX_NAME_LENGTH, "virtualkeys.%s", inp_dev_name);

	vkeys->data = data;
	vkeys->size = size;

	/* TODO: Instantiate in board file and export it */
	if (board_properties_kobj == NULL)
		board_properties_kobj =
			kobject_create_and_add("board_properties", NULL);
	if (board_properties_kobj == NULL) {
		pr_err("%s: Cannot get board_properties kobject!\n", __func__);
		rc = -EINVAL;
		goto fail_free_name;
	}

	/* Initialize dynamic SysFs attribute */
	sysfs_attr_init(&vkeys->kobj_attr.attr);
	vkeys->kobj_attr.attr.name = name;
	vkeys->kobj_attr.attr.mode = S_IRUGO;
	vkeys->kobj_attr.show = virtual_keys_show;

	rc = sysfs_create_file(board_properties_kobj, &vkeys->kobj_attr.attr);
	if (rc)
		goto fail_del_kobj;

	return 0;

fail_del_kobj:
	kobject_del(board_properties_kobj);
fail_free_name:
	kfree(name);
	vkeys->kobj_attr.attr.name = NULL;
fail_free_data:
	kfree(data);
	vkeys->data = NULL;
fail:
	return rc;
}

static void free_virtual_keys(struct cyttsp5_virtual_keys *vkeys)
{
	if (board_properties_kobj)
		sysfs_remove_file(board_properties_kobj,
				  &vkeys->kobj_attr.attr);


	kobject_del(board_properties_kobj);
	board_properties_kobj = NULL;

	kfree(vkeys->data);
	kfree(vkeys->kobj_attr.attr.name);
}
#endif

static void *create_and_get_mt_pdata(struct device_node *dev_node)
{
	struct cyttsp5_extended_mt_platform_data *ext_pdata;
	struct cyttsp5_mt_platform_data *pdata;
	u32 value;
	int rc;

	ext_pdata = kzalloc(sizeof(*ext_pdata), GFP_KERNEL);
	if (!ext_pdata) {
		rc = -ENOMEM;
		goto fail;
	}

	pdata = &ext_pdata->pdata;

	pr_debug("%s : 0\n", __func__);
	rc = get_mt_inp_dev_name(dev_node, &pdata->inp_dev_name);
	if (rc)
		goto fail_free_pdata;
	pr_debug("%s : name: %s\n", __func__, pdata->inp_dev_name);

	/* Optional fields */
	rc = of_property_read_u32(dev_node, "cy_mt_flags", &value);
	if (!rc)
		pdata->flags = value;

	rc = of_property_read_u32(dev_node, "cy_mt_vkeys_x", &value);
	if (!rc)
		pdata->vkeys_x = value;

	rc = of_property_read_u32(dev_node, "cy_mt_vkeys_y", &value);
	if (!rc)
		pdata->vkeys_y = value;

	/* Required fields */
	pdata->frmwrk = create_and_get_mt_touch_framework(dev_node);
	if (pdata->frmwrk == NULL) {
		rc = -EINVAL;
		goto fail_free_pdata;
	} else if (IS_ERR(pdata->frmwrk)) {
		rc = PTR_ERR(pdata->frmwrk);
		goto fail_free_pdata;
	}
	pr_debug("%s : frmwrk \n", __func__);
#ifdef ENABLE_VIRTUAL_KEYS
	rc = setup_virtual_keys(dev_node, pdata->inp_dev_name,
				&ext_pdata->vkeys);
	if (rc) {
		pr_err("%s: Cannot setup virtual keys!\n", __func__);
		goto fail_free_pdata;
	}
#endif
	return pdata;

fail_free_pdata:
	kfree(ext_pdata);
fail:
	return ERR_PTR(rc);
}

static void free_mt_pdata(void *pdata)
{
	struct cyttsp5_mt_platform_data *mt_pdata =
		(struct cyttsp5_mt_platform_data *)pdata;
	struct cyttsp5_extended_mt_platform_data *ext_mt_pdata =
		container_of(mt_pdata,
			     struct cyttsp5_extended_mt_platform_data, pdata);

	free_touch_framework(mt_pdata->frmwrk);
#ifdef ENABLE_VIRTUAL_KEYS
	free_virtual_keys(&ext_mt_pdata->vkeys);
#endif
	kfree(ext_mt_pdata);
}

static void *create_and_get_btn_pdata(struct device_node *dev_node)
{
	struct cyttsp5_btn_platform_data *pdata;
	int rc;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		rc = -ENOMEM;
		goto fail;
	}

	rc = get_btn_inp_dev_name(dev_node, &pdata->inp_dev_name);
	if (rc)
		goto fail_free_pdata;

	return pdata;

fail_free_pdata:
	kfree(pdata);
fail:
	return ERR_PTR(rc);
}

static void free_btn_pdata(void *pdata)
{
	struct cyttsp5_btn_platform_data *btn_pdata =
		(struct cyttsp5_btn_platform_data *)pdata;

	kfree(btn_pdata);
}

static void *create_and_get_proximity_pdata(struct device_node *dev_node)
{
	struct cyttsp5_proximity_platform_data *pdata;
	int rc;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		rc = -ENOMEM;
		goto fail;
	}

	rc = get_prox_inp_dev_name(dev_node, &pdata->inp_dev_name);
	if (rc)
		goto fail_free_pdata;

	pdata->frmwrk = create_and_get_prox_touch_framework(dev_node);
	if (pdata->frmwrk == NULL) {
		rc = -EINVAL;
		goto fail_free_pdata;
	} else if (IS_ERR(pdata->frmwrk)) {
		rc = PTR_ERR(pdata->frmwrk);
		goto fail_free_pdata;
	}

	return pdata;

fail_free_pdata:
	kfree(pdata);
fail:
	return ERR_PTR(rc);
}

static void free_proximity_pdata(void *pdata)
{
	struct cyttsp5_proximity_platform_data *proximity_pdata =
		(struct cyttsp5_proximity_platform_data *)pdata;

	free_touch_framework(proximity_pdata->frmwrk);

	kfree(proximity_pdata);
}

static struct cyttsp5_device_pdata_func device_pdata_funcs[DEVICE_TYPE_MAX] = {
	[DEVICE_MT] = {
		.create_and_get_pdata = create_and_get_mt_pdata,
		.free_pdata = free_mt_pdata,
	},
	[DEVICE_BTN] = {
		.create_and_get_pdata = create_and_get_btn_pdata,
		.free_pdata = free_btn_pdata,
	},
	[DEVICE_PROXIMITY] = {
		.create_and_get_pdata = create_and_get_proximity_pdata,
		.free_pdata = free_proximity_pdata,
	},
};

static struct cyttsp5_pdata_ptr pdata_ptr[DEVICE_TYPE_MAX];

#if 0
static const char *device_names[DEVICE_TYPE_MAX] = {
	[DEVICE_MT] = "cy,mt",
	[DEVICE_BTN] = "cy,btn",
	[DEVICE_PROXIMITY] = "cy,proximity",
};
#endif

static void set_pdata_ptr(struct cyttsp5_platform_data *pdata)
{
	pdata_ptr[DEVICE_MT].pdata = (void **)&pdata->mt_pdata;
	pdata_ptr[DEVICE_BTN].pdata = (void **)&pdata->btn_pdata;
	pdata_ptr[DEVICE_PROXIMITY].pdata = (void **)&pdata->prox_pdata;
}

#if 0
static int get_device_type(struct device_node *dev_node,
			   enum cyttsp5_device_type *type)
{
	const char *name;
	enum cyttsp5_device_type t;
	int rc;

	rc = of_property_read_string(dev_node, "name", &name);
	if (rc)
		return rc;

	for (t = 0; t < DEVICE_TYPE_MAX; t++)
		if (!strncmp(name, device_names[t], MAX_NAME_LENGTH)) {
			*type = t;
			return 0;
		}

	return -EINVAL;
}
#endif

static inline void *create_and_get_device_pdata(struct device_node *dev_node,
		enum cyttsp5_device_type type)
{
	return device_pdata_funcs[type].create_and_get_pdata(dev_node);
}

static inline void free_device_pdata(enum cyttsp5_device_type type)
{
	device_pdata_funcs[type].free_pdata(*pdata_ptr[type].pdata);
}

static struct touch_settings *create_and_get_touch_setting(
	struct device_node *core_node, const char *name)
{
	struct touch_settings *setting;
	char *tag_name;
	u32 tag_value;
	u16 *data;
	int size;
	int rc;

	data = create_and_get_u16_array(core_node, name, &size);
	if (IS_ERR_OR_NULL(data))
		return (void *)data;

	pr_debug("%s: Touch setting:'%s' size:%d\n", __func__, name, size);

	setting = kzalloc(sizeof(*setting), GFP_KERNEL);
	if (!setting) {
		rc = -ENOMEM;
		goto fail_free_data;
	}

	setting->data = (u8 *)data;
	setting->size = size;

	tag_name = kzalloc(MAX_NAME_LENGTH, GFP_KERNEL);
	if (!tag_name) {
		rc = -ENOMEM;
		goto fail_free_setting;
	}

	snprintf(tag_name, MAX_NAME_LENGTH, "%s_tag", name);

	rc = of_property_read_u32(core_node, tag_name, &tag_value);
	if (!rc)
		setting->tag = tag_value;

	kfree(tag_name);

	return setting;

fail_free_setting:
	kfree(setting);
fail_free_data:
	kfree(data);

	return ERR_PTR(rc);
}

static void free_touch_setting(struct touch_settings *setting)
{
	if (setting) {
		kfree(setting->data);
		kfree(setting);
	}
}

static char *touch_setting_names[CY_IC_GRPNUM_NUM] = {
	NULL,			/* CY_IC_GRPNUM_RESERVED */
	"cy_cmd_regs",		/* CY_IC_GRPNUM_CMD_REGS */
	"cy_tch_rep",		/* CY_IC_GRPNUM_TCH_REP */
	"cy_data_rec",		/* CY_IC_GRPNUM_DATA_REC */
	"cy_test_rec",		/* CY_IC_GRPNUM_TEST_REC */
	"cy_pcfg_rec",		/* CY_IC_GRPNUM_PCFG_REC */
	"cy_tch_parm_val",	/* CY_IC_GRPNUM_TCH_PARM_VAL */
	"cy_tch_parm_size",	/* CY_IC_GRPNUM_TCH_PARM_SIZE */
	NULL,			/* CY_IC_GRPNUM_RESERVED1 */
	NULL,			/* CY_IC_GRPNUM_RESERVED2 */
	"cy_opcfg_rec",		/* CY_IC_GRPNUM_OPCFG_REC */
	"cy_ddata_rec",		/* CY_IC_GRPNUM_DDATA_REC */
	"cy_mdata_rec",		/* CY_IC_GRPNUM_MDATA_REC */
	"cy_test_regs",		/* CY_IC_GRPNUM_TEST_REGS */
	"cy_btn_keys",		/* CY_IC_GRPNUM_BTN_KEYS */
	NULL,			/* CY_IC_GRPNUM_TTHE_REGS */
};

static struct cyttsp5_core_platform_data *create_and_get_core_pdata(
	struct device_node *core_node)
{
	struct cyttsp5_core_platform_data *pdata;
	u32 value;
	int rc;
	int i;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		rc = -ENOMEM;
		goto fail;
	}

	pr_debug("%s: 0\n", __func__);
	/* Required fields */
	//rc = of_property_read_u32(core_node, "cy_irq_gpio", &value);
	//data->irq_gpio.gpio = of_get_named_gpio_flags(np, "cy_irq_gpio", 0,
	//			(enum of_gpio_flags *)(&(data->irq_gpio)));
	//if (rc)
	//	goto fail_free;
	//pdata->irq_gpio = value;

	//pr_debug("%s: 1 %d\n", __func__, value);
	rc = of_property_read_u32(core_node, "cy_hid_desc_register", &value);
	if (rc)
		goto fail_free;
	pdata->hid_desc_register = value;
	//pdata->hid_desc_register = 1;

	pr_debug("%s: 2\n", __func__);
	/* Optional fields */
	/* rst_gpio is optional since a platform may use
	 * power cycling instead of using the XRES pin
	 */
	//rc = of_property_read_u32(core_node, "cy_rst_gpio", &value);
	//if (!rc)
	//	pdata->rst_gpio = value;

	rc = of_property_read_u32(core_node, "cy_level_irq_udelay", &value);
	if (!rc)
		pdata->level_irq_udelay = value;

	rc = of_property_read_u32(core_node, "cy_vendor_id", &value);
	if (!rc)
		pdata->vendor_id = value;

	rc = of_property_read_u32(core_node, "cy_product_id", &value);
	if (!rc)
		pdata->product_id = value;

	rc = of_property_read_u32(core_node, "cy_flags", &value);
	if (!rc)
		pdata->flags = value;

	rc = of_property_read_u32(core_node, "cy_easy_wakeup_gesture", &value);
	if (!rc)
		pdata->easy_wakeup_gesture = (u8)value;

	for (i = 0; (unsigned int)i < ARRAY_SIZE(touch_setting_names); i++) {
		if (touch_setting_names[i] == NULL)
			continue;

		pdata->sett[i] = create_and_get_touch_setting(core_node,
				 touch_setting_names[i]);
		if (IS_ERR(pdata->sett[i])) {
			rc = PTR_ERR(pdata->sett[i]);
			goto fail_free_sett;
		} else if (pdata->sett[i] == NULL)
			pr_debug("%s: No data for setting '%s'\n", __func__,
				 touch_setting_names[i]);
	}

	pr_debug("%s: irq_gpio:%d rst_gpio:%d\n"
		 "hid_desc_register:%d level_irq_udelay:%d vendor_id:%d product_id:%d\n"
		 "flags:%d easy_wakeup_gesture:%d\n", __func__,
		 pdata->irq_gpio, pdata->rst_gpio,
		 pdata->hid_desc_register,
		 pdata->level_irq_udelay, pdata->vendor_id, pdata->product_id,
		 pdata->flags, pdata->easy_wakeup_gesture);

	pdata->xres = cyttsp5_xres;
	pdata->init = cyttsp5_init;
	pdata->power = cyttsp5_power;
	pdata->detect = cyttsp5_detect;
	pdata->irq_stat = cyttsp5_irq_stat;

	return pdata;

fail_free_sett:
	for (i--; i >= 0; i--)
		free_touch_setting(pdata->sett[i]);
fail_free:
	kfree(pdata);
fail:
	return ERR_PTR(rc);
}

static void free_core_pdata(void *pdata)
{
	struct cyttsp5_core_platform_data *core_pdata = pdata;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(touch_setting_names); i++)
		free_touch_setting(core_pdata->sett[i]);
	kfree(core_pdata);
}


int cyttsp5_sysconfig_create_and_get_pdata(struct device *adap_dev)
{
	struct cyttsp5_platform_data *pdata;
	struct device_node *core_node;
	enum cyttsp5_device_type type = DEVICE_MT;
	int count = 0;
	int rc = 0;
	int i;
	/*
		if (!adap_dev->of_node)
			return 0;
	*/
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	adap_dev->platform_data = pdata;
	set_pdata_ptr(pdata);

	core_node = of_find_node_by_name(NULL, "cyttsp5");

	/* There should be only one core node */
	//printk("%s 1\n", __func__);
	if (core_node)  {
		const char *name;

		rc = of_property_read_string(core_node, "cy_name", &name);
		if (!rc)
			pr_debug("%s: name:%s\n", __func__, name);
		//printk("%s 2\n", __func__);

		pdata->core_pdata = create_and_get_core_pdata(core_node);
		if (IS_ERR(pdata->core_pdata)) {
			pr_debug("%s: core_pdata error\n", __func__);
			rc = PTR_ERR(pdata->core_pdata);
			goto out_core_node;
		}
		//printk("%s 3\n", __func__);

		/* Increment reference count */
		of_node_get(core_node);

		for (i = 0; i < DEVICE_TYPE_MAX; ++i) {
			//printk("%s: child: %d\n", __func__, i);
			*pdata_ptr[i].pdata = create_and_get_device_pdata(core_node, i);
			if (IS_ERR(*pdata_ptr[i].pdata))
				rc = PTR_ERR(*pdata_ptr[i].pdata);
			if (rc)
				break;
			//printk("%s: child: %d end\n", __func__, i);
		}
		//printk("%s 4: %d\n", __func__, rc);
		if (rc) {
			free_core_pdata(pdata->core_pdata);
			of_node_put(core_node);
			for (i = 0; i < DEVICE_PROXIMITY; ++i) {
				if ((*pdata_ptr[type].pdata) && (!IS_ERR(*pdata_ptr[i].pdata))) {
					free_device_pdata(i);
				}
			}
		}
		//printk("%s 5\n", __func__);
		pdata->loader_pdata = &_cyttsp5_loader_platform_data;
	}
out_core_node:

	pr_debug("%s: %d child node(s) found\n", __func__, count);

	return rc;
}
EXPORT_SYMBOL_GPL(cyttsp5_sysconfig_create_and_get_pdata);

int cyttsp5_sysconfig_clean_pdata(struct device *adap_dev)
{
	struct cyttsp5_platform_data *pdata;
	int rc = 0;
	int i;

	if (!adap_dev->of_node)
		return 0;
	pdata = dev_get_platdata(adap_dev);
	set_pdata_ptr(pdata);
#if 0
	struct device_node *core_node, *dev_node;
	enum cyttsp5_device_type type;
	for_each_child_of_node(adap_dev->of_node, core_node) {
		free_core_pdata(pdata->core_pdata);
		of_node_put(core_node);
		for_each_child_of_node(core_node, dev_node) {
			rc = get_device_type(dev_node, &type);
			if (rc)
				break;
			free_device_pdata(type);
			of_node_put(dev_node);
		}
	}
#else
	free_core_pdata(pdata->core_pdata);
	for (i = 0; i < DEVICE_TYPE_MAX; ++i) {
		free_device_pdata(i);
	}
#endif
	return rc;
}
EXPORT_SYMBOL_GPL(cyttsp5_sysconfig_clean_pdata);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Parade TrueTouch(R) Standard Product DeviceTree Driver");
MODULE_AUTHOR("Parade Technologies <ttdrivers@paradetech.com>");
