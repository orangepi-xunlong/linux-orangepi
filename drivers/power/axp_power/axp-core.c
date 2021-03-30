#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/arisc/arisc.h>
#include "axp-core.h"

int axp_suspend_flag = AXP_NOT_SUSPEND;
struct axp_platform_ops ap_ops[AXP_ONLINE_SUM];
const char *axp_name[AXP_ONLINE_SUM];
static LIST_HEAD(axp_dev_list);
static DEFINE_SPINLOCK(axp_list_lock);
static int axp_dev_register_count;
struct work_struct axp_irq_work;

void axp_platform_ops_set(int pmu_num, struct axp_platform_ops *ops)
{
	ap_ops[pmu_num].usb_det = ops->usb_det;
	ap_ops[pmu_num].usb_vbus_output = ops->usb_vbus_output;
	ap_ops[pmu_num].cfg_pmux_para = ops->cfg_pmux_para;
	ap_ops[pmu_num].get_pmu_name = ops->get_pmu_name;
	ap_ops[pmu_num].get_pmu_dev  = ops->get_pmu_dev;
	ap_ops[pmu_num].pmu_regulator_save = ops->pmu_regulator_save;
	ap_ops[pmu_num].pmu_regulator_restore = ops->pmu_regulator_restore;
}

s32 axp_usb_det(void)
{
	return ap_ops[0].usb_det();
}
EXPORT_SYMBOL_GPL(axp_usb_det);

s32 axp_usb_vbus_output(int high)
{
	return ap_ops[0].usb_vbus_output(high);
}
EXPORT_SYMBOL_GPL(axp_usb_vbus_output);

int config_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
{
	if (num >= AXP_ONLINE_SUM)
		return -EINVAL;

	if (ap_ops[num].cfg_pmux_para)
		return ap_ops[num].cfg_pmux_para(num, api, pmu_id);
	else
		return -EINVAL;
}
EXPORT_SYMBOL_GPL(config_pmux_para);

const char *get_pmu_cur_name(int pmu_num)
{
	if (ap_ops[pmu_num].get_pmu_name)
		return ap_ops[pmu_num].get_pmu_name();
	else
		return NULL;
}
EXPORT_SYMBOL_GPL(get_pmu_cur_name);

struct axp_dev *get_pmu_cur_dev(int pmu_num)
{
	if (ap_ops[pmu_num].get_pmu_dev)
		return ap_ops[pmu_num].get_pmu_dev();
	else
		return NULL;
}
EXPORT_SYMBOL_GPL(get_pmu_cur_dev);

int axp_mem_save(void)
{
	if (ap_ops[0].pmu_regulator_save)
		return ap_ops[0].pmu_regulator_save();
	return 0;
}
EXPORT_SYMBOL_GPL(axp_mem_save);

void axp_mem_restore(void)
{
	if (ap_ops[0].pmu_regulator_restore)
		return ap_ops[0].pmu_regulator_restore();
}
EXPORT_SYMBOL_GPL(axp_mem_restore);

int axp_get_pmu_num(const struct of_device_id *ids, int size)
{
	struct device_node *np;
	int i, j, pmu_num = -EINVAL;
	char node_name[8];
	const char *prop_name = NULL;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		sprintf(node_name, "pmu%d", i);

		np = of_find_node_by_type(NULL, node_name);
		if (NULL == np) {
			BUG_ON(i == 0);
			break;
		}

		if (of_property_read_string(np, "compatible",
					&prop_name)) {
			pr_err("%s get failed\n", prop_name);
			break;
		}

		for (j = 0; j < size; j++) {
			if (!strcmp(prop_name, ids[j].compatible)) {
				pmu_num = i;
				break;
			}
		}
	}

	return pmu_num;
}

int axp_mfd_cell_name_init(struct axp_platform_ops *ops, int count, int pmu_num,
				int size, struct mfd_cell *cells)
{
	int i, j, find = 0;

	for (j = 0; j < count; j++) {
		if ((ops->powerkey_name[j] != NULL)
				&& (strstr(ops->powerkey_name[j],
				axp_name[pmu_num]) != NULL)) {
			find = 1;
			break;
		}

		if ((ops->regulator_name[j] != NULL)
				&& (strstr(ops->regulator_name[j],
				axp_name[pmu_num]) != NULL)) {
			find = 1;
			break;
		}

		if ((ops->charger_name[j] != NULL)
				&& (strstr(ops->charger_name[j],
				axp_name[pmu_num]) != NULL)) {
			find = 1;
			break;
		}

		if ((ops->gpio_name[j] != NULL)
				&& (strstr(ops->gpio_name[j],
				axp_name[pmu_num]) != NULL)) {
			find = 1;
			break;
		}
	}

	if (find == 0) {
		pr_err("%s no axp mfd cell find\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		if (strstr(cells[i].name, "powerkey") != NULL)
			cells[i].of_compatible = ops->powerkey_name[j];
		else if (strstr(cells[i].name, "regulator") != NULL)
			cells[i].of_compatible = ops->regulator_name[j];
		else if (strstr(cells[i].name, "charger") != NULL)
			cells[i].of_compatible = ops->charger_name[j];
		else if (strstr(cells[i].name, "gpio") != NULL)
			cells[i].of_compatible = ops->gpio_name[j];
	}

	return 0;
}

#ifdef CONFIG_AXP_TWI_USED
static s32 __axp_read_i2c(struct i2c_client *client, u32 reg, u8 *val)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (u8)ret;

	return 0;
}

static s32 __axp_reads_i2c(struct i2c_client *client,
				int reg, int len, u8 *val)
{
	s32 ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static s32 __axp_write_i2c(struct i2c_client *client, int reg, u8 val)
{
	s32 ret;

	/* axp_reg_debug(reg, 1, &val); */
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}

	return 0;
}

static s32 __axp_writes_i2c(struct i2c_client *client,
				int reg, int len, u8 *val)
{
	s32 ret;

	/* axp_reg_debug(reg, len, val); */
	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static inline s32 __axp_read_arisc_rsb(char devaddr, int reg,
			u8 *val, bool syncflag)
{
	return 0;
}
static inline s32 __axp_reads_arisc_rsb(char devaddr, int reg,
			int len, u8 *val, bool syncflag)
{
	return 0;
}
static inline s32 __axp_write_arisc_rsb(char devaddr, int reg,
			u8 val, bool syncflag)
{
	return 0;
}
static inline s32 __axp_writes_arisc_rsb(char devaddr, int reg,
			int len, u8 *val, bool syncflag)
{
	return 0;
}

static inline s32 __axp_read_arisc_twi(int reg, u8 *val, bool syncflag)
{
	return 0;
}
static inline s32 __axp_reads_arisc_twi(int reg, int len,
				u8 *val, bool syncflag)
{
	return 0;
}
static inline s32 __axp_write_arisc_twi(int reg, u8 val, bool syncflag)
{
	return 0;
}
static inline s32 __axp_writes_arisc_twi(int reg, int len,
				u8 *val, bool syncflag)
{
	return 0;
}
#else
static inline s32 __axp_read_i2c(struct i2c_client *client,
				u32 reg, u8 *val)
{
	return 0;
}
static inline s32 __axp_reads_i2c(struct i2c_client *client,
				int reg, int len, u8 *val)
{
	return 0;
}
static inline s32 __axp_write_i2c(struct i2c_client *client,
				int reg, u8 val)
{
	return 0;
}
static inline s32 __axp_writes_i2c(struct i2c_client *client,
				int reg, int len, u8 *val)
{
	return 0;
}

static s32 __axp_read_arisc_rsb(char devaddr, int reg, u8 *val, bool syncflag)
{
	s32 ret;
	u8 addr = (u8)reg;
	u8 data = 0;
	arisc_rsb_block_cfg_t rsb_data;
	u32 data_temp;

	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_BYTE;

	if (syncflag)
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
	else
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

	rsb_data.devaddr = devaddr;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data_temp;

	/* write axp registers */
	ret = arisc_rsb_read_block_data(&rsb_data);
	if (ret != 0) {
		pr_err("failed read to 0x%02x\n", reg);
		return ret;
	}

	data = (u8)data_temp;
	*val = data;

	return 0;
}

static s32 __axp_reads_arisc_rsb(char devaddr, int reg,
				int len, u8 *val, bool syncflag)
{
	s32 ret, i, rd_len;
	u8 addr[AXP_TRANS_BYTE_MAX];
	u8 data[AXP_TRANS_BYTE_MAX];
	u8 *cur_data = val;
	arisc_rsb_block_cfg_t rsb_data;
	u32 data_temp[AXP_TRANS_BYTE_MAX];

	/* fetch first register address */
	while (len > 0) {
		rd_len = min(len, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < rd_len; i++)
			addr[i] = reg++;

		rsb_data.len = rd_len;
		rsb_data.datatype = RSB_DATA_TYPE_BYTE;

		if (syncflag)
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
		else
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

		rsb_data.devaddr = devaddr;
		rsb_data.regaddr = addr;
		rsb_data.data = data_temp;

		/* read axp registers */
		ret = arisc_rsb_read_block_data(&rsb_data);
		if (ret != 0) {
			pr_err("failed reads to 0x%02x\n", reg);
			return ret;
		}

		for (i = 0; i < rd_len; i++)
			data[i] = (u8)data_temp[i];

		/* copy data to user buffer */
		memcpy(cur_data, data, rd_len);
		cur_data = cur_data + rd_len;

		/* process next time read */
		len -= rd_len;
	}

	return 0;
}

static s32 __axp_write_arisc_rsb(char devaddr, int reg, u8 val, bool syncflag)
{
	s32 ret;
	u8 addr = (u8)reg;
	arisc_rsb_block_cfg_t rsb_data;
	u32 data;

	/* axp_reg_debug(reg, 1, &val); */

	data = (unsigned int)val;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_BYTE;

	if (syncflag)
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
	else
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

	rsb_data.devaddr = devaddr;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* write axp registers */
	ret = arisc_rsb_write_block_data(&rsb_data);
	if (ret != 0) {
		pr_err("failed writing to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static s32 __axp_writes_arisc_rsb(char devaddr, int reg,
				int len, u8 *val, bool syncflag)
{
	s32 ret = 0, i, first_flag, wr_len;
	u8 addr[AXP_TRANS_BYTE_MAX];
	u8 data[AXP_TRANS_BYTE_MAX];
	arisc_rsb_block_cfg_t rsb_data;
	u32 data_temp[AXP_TRANS_BYTE_MAX];

	/* axp_reg_debug(reg, len, val); */

	/* fetch first register address */
	first_flag = 1;
	addr[0] = (u8)reg;
	len = len + 1;  /* + first reg addr */
	len = len >> 1; /* len = len / 2 */

	while (len > 0) {
		wr_len = min(len, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < wr_len; i++) {
			if (first_flag) {
				/* skip the first reg addr */
				data[i] = *val++;
				first_flag = 0;
			} else {
				addr[i] = *val++;
				data[i] = *val++;
			}
		}

		for (i = 0; i < wr_len; i++)
			data_temp[i] = (unsigned int)data[i];

		rsb_data.len = wr_len;
		rsb_data.datatype = RSB_DATA_TYPE_BYTE;

		if (syncflag)
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
		else
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

		rsb_data.devaddr = devaddr;
		rsb_data.regaddr = addr;
		rsb_data.data = data_temp;

		/* write axp registers */
		ret = arisc_rsb_write_block_data(&rsb_data);
		if (ret != 0) {
			pr_err("failed writings to 0x%02x\n", reg);
			return ret;
		}

		/* process next time write */
		len -= wr_len;
	}

	return 0;
}

static s32 __axp_read_arisc_twi(int reg, u8 *val, bool syncflag)
{
	s32 ret;
	u8 addr = (u8)reg;
	arisc_twi_block_cfg_t twi_data;
	u8 data = 0;

	if (syncflag)
		twi_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
	else
		twi_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

	twi_data.len = 1;
	twi_data.addr = &addr;
	twi_data.data = &data;

	/* write axp registers */
	ret = arisc_twi_read_block_data(&twi_data);
	if (ret != 0) {
		pr_err("failed read to 0x%02x\n", reg);
		return ret;
	}

	*val = data;

	return 0;
}

static s32 __axp_reads_arisc_twi(int reg, int len, u8 *val, bool syncflag)
{
	arisc_twi_block_cfg_t twi_data;
	u8 addr[TWI_TRANS_BYTE_MAX] = {0};
	u8 data[TWI_TRANS_BYTE_MAX] = {0};
	u8 *cur_data = val;
	s32 ret, i, rd_len;

	/* fetch first register address */
	while (len > 0) {
		rd_len = min(len, TWI_TRANS_BYTE_MAX);

		for (i = 0; i < rd_len; i++)
			addr[i] = reg++;

		if (syncflag)
			twi_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
		else
			twi_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

		twi_data.len = rd_len;
		twi_data.addr = addr;
		twi_data.data = data;

		/* write axp registers */
		ret = arisc_twi_read_block_data(&twi_data);
		if (ret != 0) {
			pr_err("failed read to 0x%02x\n", reg);
			return ret;
		}

		/* copy data to user buffer */
		memcpy(cur_data, data, rd_len);
		cur_data = cur_data + rd_len;

		/* process next time read */
		len -= rd_len;
	}

	return 0;
}

static s32 __axp_write_arisc_twi(int reg, u8 val, bool syncflag)
{
	s32 ret;
	u8 addr = (u8)reg;
	arisc_twi_block_cfg_t twi_data;
	u8 data = val;

	if (syncflag)
		twi_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
	else
		twi_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

	twi_data.len = 1;
	twi_data.addr = &addr;
	twi_data.data = &data;

	/* write axp registers */
	ret = arisc_twi_write_block_data(&twi_data);
	if (ret != 0) {
		pr_err("failed writing to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static s32 __axp_writes_arisc_twi(int reg, int len, u8 *val, bool syncflag)
{
	arisc_twi_block_cfg_t twi_data;
	int len_to_write = (len + 1) >> 1;
	u8 addr[TWI_TRANS_BYTE_MAX] = {0};
	u8 data[TWI_TRANS_BYTE_MAX] = {0};
	s32 ret, i, wr_len, first_flag = 1;

	addr[0] = (u8)reg;
	while (len_to_write > 0) {
		wr_len = min(len_to_write, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < wr_len; i++) {
			if (first_flag) {
				/* skip the first reg addr */
				data[i] = *val++;
				first_flag = 0;
			} else {
				addr[i] = *val++;
				data[i] = *val++;
			}
		}

		if (syncflag)
			twi_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
		else
			twi_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

		twi_data.len = wr_len;
		twi_data.addr = addr;
		twi_data.data = data;

		/* write axp registers */
		ret = arisc_twi_write_block_data(&twi_data);
		if (ret != 0) {
			pr_err("failed writing to 0x%02x\n", reg);
			return ret;
		}

		/* process next time write */
		len_to_write -= wr_len;
	}

	return 0;
}
#endif

static s32 _axp_write(struct axp_regmap *map, s32 reg, u8 val, bool sync)
{
	s32 ret = 0;

	if (map->type == AXP_REGMAP_I2C)
		ret = __axp_write_i2c(map->client, reg, val);
	else if (map->type == AXP_REGMAP_ARISC_RSB)
		ret = __axp_write_arisc_rsb(map->rsbaddr, reg, val, sync);
	else if (map->type == AXP_REGMAP_ARISC_TWI)
		ret = __axp_write_arisc_twi(reg, val, sync);

	return ret;
}

static s32 _axp_writes(struct axp_regmap *map, s32 reg,
				s32 len, u8 *val, bool sync)
{
	s32 ret = 0, i;
	s32 wr_len, rw_reg;
	u8 wr_val[32];

	while (len) {
		wr_len = min(len, 15);
		rw_reg = reg++;
		wr_val[0] = *val++;

		for (i = 1; i < wr_len; i++) {
			wr_val[i*2-1] = reg++;
			wr_val[i*2] = *val++;
		}

		if (map->type == AXP_REGMAP_I2C)
			ret = __axp_writes_i2c(map->client,
					rw_reg, 2*wr_len-1, wr_val);
		else if (map->type == AXP_REGMAP_ARISC_RSB)
			ret = __axp_writes_arisc_rsb(map->rsbaddr,
					rw_reg, 2*wr_len-1, wr_val, sync);
		else if (map->type == AXP_REGMAP_ARISC_TWI)
			ret = __axp_writes_arisc_twi(rw_reg,
					2*wr_len-1, wr_val, sync);

		if (ret)
			return ret;

		len -= wr_len;
	}

	return 0;
}

static s32 _axp_read(struct axp_regmap *map, s32 reg, u8 *val, bool sync)
{
	s32 ret = 0;

	if (map->type == AXP_REGMAP_I2C)
		ret = __axp_read_i2c(map->client, reg, val);
	else if (map->type == AXP_REGMAP_ARISC_RSB)
		ret = __axp_read_arisc_rsb(map->rsbaddr, reg, val, sync);
	else if (map->type == AXP_REGMAP_ARISC_TWI)
		ret = __axp_read_arisc_twi(reg, val, sync);

	return ret;
}

static s32 _axp_reads(struct axp_regmap *map, s32 reg,
				s32 len, u8 *val, bool sync)
{
	s32 ret = 0;

	if (map->type == AXP_REGMAP_I2C)
		ret = __axp_reads_i2c(map->client, reg, len, val);
	else if (map->type == AXP_REGMAP_ARISC_RSB)
		ret = __axp_reads_arisc_rsb(map->rsbaddr, reg, len, val, sync);
	else if (map->type == AXP_REGMAP_ARISC_TWI)
		ret = __axp_reads_arisc_twi(reg, len, val, sync);

	return ret;
}

s32 axp_regmap_write(struct axp_regmap *map, s32 reg, u8 val)
{
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_write(map, reg, val, false);
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_write);

s32 axp_regmap_writes(struct axp_regmap *map, s32 reg, s32 len, u8 *val)
{
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_writes(map, reg, len, val, false);
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_writes);

s32 axp_regmap_read(struct axp_regmap *map, s32 reg, u8 *val)
{
	return _axp_read(map, reg, val, false);
}
EXPORT_SYMBOL_GPL(axp_regmap_read);

s32 axp_regmap_reads(struct axp_regmap *map, s32 reg, s32 len, u8 *val)
{
	return _axp_reads(map, reg, len, val, false);
}
EXPORT_SYMBOL_GPL(axp_regmap_reads);

s32 axp_regmap_set_bits(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_read(map, reg, &reg_val, false);

	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = _axp_write(map, reg, reg_val, false);
	}

out:
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_set_bits);

s32 axp_regmap_clr_bits(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_read(map, reg, &reg_val, false);

	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = _axp_write(map, reg, reg_val, false);
	}

out:
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_clr_bits);

s32 axp_regmap_update(struct axp_regmap *map, s32 reg, u8 val, u8 mask)
{
	u8 reg_val;
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_read(map, reg, &reg_val, false);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = _axp_write(map, reg, reg_val, false);
	}

out:
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_update);


s32 axp_regmap_set_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;
#ifndef CONFIG_AXP_TWI_USED
	unsigned long irqflags;

	spin_lock_irqsave(&map->spinlock, irqflags);
#else
	mutex_lock(&map->lock);
#endif

	ret = _axp_read(map, reg, &reg_val, true);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = _axp_write(map, reg, reg_val, true);
	}

out:
#ifndef CONFIG_AXP_TWI_USED
	spin_unlock_irqrestore(&map->spinlock, irqflags);
#else
	mutex_unlock(&map->lock);
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_set_bits_sync);

s32 axp_regmap_clr_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;
#ifndef CONFIG_AXP_TWI_USED
	unsigned long irqflags;

	spin_lock_irqsave(&map->spinlock, irqflags);
#else
	mutex_lock(&map->lock);
#endif

	ret = _axp_read(map, reg, &reg_val, true);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = _axp_write(map, reg, reg_val, true);
	}

out:
#ifndef CONFIG_AXP_TWI_USED
	spin_unlock_irqrestore(&map->spinlock, irqflags);
#else
	mutex_unlock(&map->lock);
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_clr_bits_sync);

s32 axp_regmap_update_sync(struct axp_regmap *map, s32 reg, u8 val, u8 mask)
{
	u8 reg_val;
	s32 ret = 0;
#ifndef CONFIG_AXP_TWI_USED
	unsigned long irqflags;

	spin_lock_irqsave(&map->spinlock, irqflags);
#else
	mutex_lock(&map->lock);
#endif

	ret = _axp_read(map, reg, &reg_val, true);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = _axp_write(map, reg, reg_val, true);
	}

out:
#ifndef CONFIG_AXP_TWI_USED
	spin_unlock_irqrestore(&map->spinlock, irqflags);
#else
	mutex_unlock(&map->lock);
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_update_sync);

struct axp_regmap *axp_regmap_init_i2c(struct device *dev)
{
	struct axp_regmap *map = NULL;

	map = devm_kzalloc(dev, sizeof(*map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(map)) {
		pr_err("%s: not enough memory!\n", __func__);
		return NULL;
	}

	map->type = AXP_REGMAP_I2C;
	map->client = to_i2c_client(dev);
	mutex_init(&map->lock);

	return map;
}
EXPORT_SYMBOL_GPL(axp_regmap_init_i2c);

struct axp_regmap *axp_regmap_init_arisc_rsb(struct device *dev, u8 addr)
{
	struct axp_regmap *map = NULL;

	map = devm_kzalloc(dev, sizeof(*map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(map)) {
		pr_err("%s: not enough memory!\n", __func__);
		return NULL;
	}

	map->type = AXP_REGMAP_ARISC_RSB;
	map->rsbaddr = addr;
#ifndef CONFIG_AXP_TWI_USED
	spin_lock_init(&map->spinlock);
#endif
	mutex_init(&map->lock);

	return map;
}
EXPORT_SYMBOL_GPL(axp_regmap_init_arisc_rsb);

struct axp_regmap *axp_regmap_init_arisc_twi(struct device *dev)
{
	struct axp_regmap *map = NULL;

	map = devm_kzalloc(dev, sizeof(*map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(map)) {
		pr_err("%s: not enough memory!\n", __func__);
		return NULL;
	}

	map->type = AXP_REGMAP_ARISC_TWI;
#ifndef CONFIG_AXP_TWI_USED
	spin_lock_init(&map->spinlock);
#endif
	mutex_init(&map->lock);

	return map;
}
EXPORT_SYMBOL_GPL(axp_regmap_init_arisc_twi);

static void __do_irq(int pmu_num, struct axp_irq_chip_data *irq_data)
{
	u64 irqs = 0;
	u8 reg_val[8];
	u32 i, j;
	void *idata;

	if (irq_data == NULL)
		return;

	axp_regmap_reads(irq_data->map, irq_data->chip->status_base,
			irq_data->chip->num_regs, reg_val);

	for (i = 0; i < irq_data->chip->num_regs; i++)
		irqs |= (u64)reg_val[i] << (i * AXP_REG_WIDTH);

	irqs &= irq_data->irqs_enabled;
	if (irqs == 0)
		return;

	AXP_DEBUG(AXP_INT, pmu_num, "irqs enabled = 0x%llx\n",
				irq_data->irqs_enabled);
	AXP_DEBUG(AXP_INT, pmu_num, "irqs = 0x%llx\n", irqs);

	for_each_set_bit(j, (unsigned long *)&irqs, irq_data->num_irqs) {
		if (irq_data->irqs[j].handler) {
			idata = irq_data->irqs[j].data;
			irq_data->irqs[j].handler(j, idata);
		}
	}

	for (i = 0; i < irq_data->chip->num_regs; i++) {
		if (reg_val[i] != 0) {
			axp_regmap_write(irq_data->map,
				irq_data->chip->status_base + i, reg_val[i]);
			udelay(30);
		}
	}
}

static void axp_irq_work_func(struct work_struct *work)
{
	struct axp_dev *adev;

	list_for_each_entry(adev, &axp_dev_list, list) {
		__do_irq(adev->pmu_num, adev->irq_data);
	}


#ifdef CONFIG_AXP_NMI_USED
	clear_nmi_status();
	enable_nmi();
#endif
}

static irqreturn_t axp_irq(int irq, void *data)
{
	struct axp_dev *adev;

#ifdef CONFIG_AXP_NMI_USED
	disable_nmi();
#endif

	if (axp_suspend_flag == AXP_NOT_SUSPEND) {
		schedule_work(&axp_irq_work);
	} else if (axp_suspend_flag == AXP_WAS_SUSPEND) {
		list_for_each_entry(adev, &axp_dev_list, list) {
			if (adev->irq_data->wakeup_event) {
				adev->irq_data->wakeup_event();
				axp_suspend_flag = AXP_SUSPEND_WITH_IRQ;
			}
		}
	}

	return IRQ_HANDLED;
}

struct axp_irq_chip_data *axp_irq_chip_register(struct axp_regmap *map,
			int irq_no, int irq_flags,
			struct axp_regmap_irq_chip *irq_chip,
			void (*wakeup_event)(void))
{
	struct axp_irq_chip_data *irq_data = NULL;
	struct axp_regmap_irq *irqs = NULL;
	int i, err = 0;

	irq_data = kzalloc(sizeof(*irq_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(irq_data)) {
		pr_err("axp irq data: not enough memory for irq data\n");
		return NULL;
	}

	irq_data->map = map;
	irq_data->chip = irq_chip;
	irq_data->num_irqs = AXP_REG_WIDTH * irq_chip->num_regs;

	irqs = kzalloc(irq_chip->num_regs * AXP_REG_WIDTH * sizeof(*irqs),
				GFP_KERNEL);
	if (IS_ERR_OR_NULL(irqs)) {
		pr_err("axp irq data: not enough memory for irq disc\n");
		goto free_irq_data;
	}

	mutex_init(&irq_data->lock);
	irq_data->irqs = irqs;
	irq_data->irqs_enabled = 0;
	irq_data->wakeup_event = wakeup_event;

	/* disable all irq and clear all irq pending */
	for (i = 0; i < irq_chip->num_regs; i++) {
		axp_regmap_clr_bits(map, irq_chip->enable_base + i, 0xff);
		axp_regmap_set_bits(map, irq_chip->status_base + i, 0xff);
	}

#ifdef CONFIG_DUAL_AXP_USED
	if (axp_dev_register_count == 1) {
		err = request_irq(irq_no, axp_irq, irq_flags, "axp", irq_data);
		goto irq_out;
	} else if (axp_dev_register_count == 2) {
		return irq_data;
	}
#else
	err = request_irq(irq_no, axp_irq, irq_flags, irq_chip->name, irq_data);
#endif

#ifdef CONFIG_DUAL_AXP_USED
irq_out:
#endif
	if (err)
		goto free_irqs;

	INIT_WORK(&axp_irq_work, axp_irq_work_func);

#ifdef CONFIG_AXP_NMI_USED
	set_nmi_trigger(IRQF_TRIGGER_LOW);
	clear_nmi_status();
	enable_nmi();
#endif

	return irq_data;

free_irqs:
	kfree(irqs);
free_irq_data:
	kfree(irq_data);

	return NULL;
}
EXPORT_SYMBOL_GPL(axp_irq_chip_register);

void axp_irq_chip_unregister(int irq, struct axp_irq_chip_data *irq_data)
{
	int i;
	struct axp_regmap *map = irq_data->map;

	free_irq(irq, irq_data);

	/* disable all irq and clear all irq pending */
	for (i = 0; i < irq_data->chip->num_regs; i++) {
		axp_regmap_clr_bits(map,
				irq_data->chip->enable_base + i, 0xff);
		axp_regmap_write(map,
				irq_data->chip->status_base + i, 0xff);
	}

	kfree(irq_data->irqs);
	kfree(irq_data);

#ifdef CONFIG_AXP_NMI_USED
	disable_nmi();
#endif
}
EXPORT_SYMBOL_GPL(axp_irq_chip_unregister);

int axp_request_irq(struct axp_dev *adev, int irq_no,
				irq_handler_t handler, void *data)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	struct axp_regmap_irq *irqs = irq_data->irqs;
	int reg, ret;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs || !handler)
		return -1;

	mutex_lock(&irq_data->lock);
	irqs[irq_no].handler = handler;
	irqs[irq_no].data = data;
	irq_data->irqs_enabled |= ((u64)0x1 << irq_no);
	reg = irq_no / AXP_REG_WIDTH;
	reg += irq_data->chip->enable_base;
	mask = 1 << (irq_no % AXP_REG_WIDTH);
	ret = axp_regmap_set_bits(adev->regmap, reg, mask);
	mutex_unlock(&irq_data->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_request_irq);

int axp_enable_irq(struct axp_dev *adev, int irq_no)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	int reg, ret = 0;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs)
		return -1;

	if (irq_data->irqs[irq_no].handler) {
		mutex_lock(&irq_data->lock);
		reg = irq_no / AXP_REG_WIDTH;
		reg += irq_data->chip->enable_base;
		mask = 1 << (irq_no % AXP_REG_WIDTH);
		ret = axp_regmap_set_bits(adev->regmap, reg, mask);
		mutex_unlock(&irq_data->lock);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(axp_enable_irq);

int axp_disable_irq(struct axp_dev *adev, int irq_no)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	int reg, ret = 0;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs)
		return -1;

	mutex_lock(&irq_data->lock);
	reg = irq_no / AXP_REG_WIDTH;
	reg += irq_data->chip->enable_base;
	mask = 1 << (irq_no % AXP_REG_WIDTH);
	ret = axp_regmap_clr_bits(adev->regmap, reg, mask);
	mutex_unlock(&irq_data->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_disable_irq);

int axp_free_irq(struct axp_dev *adev, int irq_no)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	int reg;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs)
		return -1;

	mutex_lock(&irq_data->lock);
	if (irq_data->irqs[irq_no].handler) {
		reg = irq_no / AXP_REG_WIDTH;
		reg += irq_data->chip->enable_base;
		mask = 1 << (irq_no % AXP_REG_WIDTH);
		axp_regmap_clr_bits(adev->regmap, reg, mask);
		irq_data->irqs[irq_no].data = NULL;
		irq_data->irqs[irq_no].handler = NULL;
	}
	mutex_unlock(&irq_data->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(axp_free_irq);

int axp_gpio_irq_register(struct axp_dev *adev, int irq_no,
				irq_handler_t handler, void *data)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	struct axp_regmap_irq *irqs = irq_data->irqs;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs || !handler)
		return -1;

	mutex_lock(&irq_data->lock);
	irq_data->irqs_enabled |= ((u64)0x1 << irq_no);
	irqs[irq_no].handler = handler;
	irqs[irq_no].data = data;
	mutex_unlock(&irq_data->lock);

	return 0;
}

int axp_mfd_add_devices(struct axp_dev *axp_dev)
{
	int ret;
	unsigned long irqflags;

	ret = mfd_add_devices(axp_dev->dev, -1,
		axp_dev->cells, axp_dev->nr_cells, NULL, 0, NULL);
	if (ret)
		goto fail;

	dev_set_drvdata(axp_dev->dev, axp_dev);

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_add(&axp_dev->list, &axp_dev_list);
	axp_dev_register_count++;
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	return 0;

fail:
	return ret;
}
EXPORT_SYMBOL_GPL(axp_mfd_add_devices);

int axp_mfd_remove_devices(struct axp_dev *axp_dev)
{
	mfd_remove_devices(axp_dev->dev);

	return 0;
}
EXPORT_SYMBOL_GPL(axp_mfd_remove_devices);

int axp_dt_parse(struct device_node *node, int pmu_num,
			struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	if (of_property_read_u32(node, "pmu_id", &axp_config->pmu_id)) {
		pr_err("%s: get pmu_id failed\n", __func__);
		return -1;
	}

	if (of_property_read_string(node, "compatible", &axp_name[pmu_num])) {
		pr_err("%s: get pmu name failed\n", __func__);
		return -1;
	}

	if (of_property_read_u32(node, "pmu_vbusen_func",
		&axp_config->pmu_vbusen_func))
		axp_config->pmu_vbusen_func = 1;

	if (of_property_read_u32(node, "pmu_reset",
		&axp_config->pmu_reset))
		axp_config->pmu_reset = 0;

	if (of_property_read_u32(node, "pmu_irq_wakeup",
			&axp_config->pmu_irq_wakeup))
		axp_config->pmu_irq_wakeup = 0;

	if (of_property_read_u32(node, "pmu_hot_shutdown",
		&axp_config->pmu_hot_shutdown))
		axp_config->pmu_hot_shutdown = 1;

	if (of_property_read_u32(node, "pmu_inshort",
		&axp_config->pmu_inshort))
		axp_config->pmu_inshort = 0;

	if (of_property_read_u32(node, "pmu_reset_shutdown_en",
		&axp_config->pmu_reset_shutdown_en))
		axp_config->pmu_reset_shutdown_en = 0;

	if (of_property_read_u32(node, "pmu_as_slave",
		&axp_config->pmu_as_slave))
		axp_config->pmu_as_slave = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(axp_dt_parse);

int axp_num;
static ssize_t axp_num_show(struct class *class,
			struct class_attribute *attr,   char *buf)
{
	return sprintf(buf, "pmu%d\n", axp_num);
}

static ssize_t axp_num_store(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t count)
{
	int val, err;

	err = kstrtoint(buf, 16, &val);
	if (err)
		return err;

	axp_num = val;
	if ((axp_num >= AXP_ONLINE_SUM) || (axp_num < 0))
		return -EINVAL;

	return count;
}

static ssize_t axp_name_show(struct class *class,
			struct class_attribute *attr,   char *buf)
{
	return sprintf(buf, "%s\n", get_pmu_cur_name(axp_num));
}

static u8 axp_reg_addr;
static ssize_t axp_reg_show(struct class *class,
			struct class_attribute *attr,   char *buf)
{
	u8 val;
	struct axp_dev *cur_axp_dev = get_pmu_cur_dev(axp_num);

	if (cur_axp_dev == NULL)
		return sprintf(buf, "invalid parameters\n");

	if (cur_axp_dev->is_dummy)
		return sprintf(buf, "unsupported\n");

	axp_regmap_read(cur_axp_dev->regmap, axp_reg_addr, &val);
	return sprintf(buf, "%s:REG[0x%x]=0x%x\n",
				get_pmu_cur_name(axp_num), axp_reg_addr, val);
}

static ssize_t axp_reg_store(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t count)
{
	s32 tmp;
	u8 val;
	int err;
	struct axp_dev *cur_axp_dev = get_pmu_cur_dev(axp_num);

	if (cur_axp_dev == NULL) {
		pr_warn("invalid parameters\n");
		return -EINVAL;
	}

	if (cur_axp_dev->is_dummy) {
		pr_err("unsupported\n");
		return -EINVAL;
	}

	err = kstrtoint(buf, 16, &tmp);
	if (err)
		return err;

	if (tmp < 256) {
		axp_reg_addr = tmp;
	} else {
		val = tmp & 0x00FF;
		axp_reg_addr = (tmp >> 8) & 0x00FF;
		axp_regmap_write(cur_axp_dev->regmap, axp_reg_addr, val);
	}

	return count;
}

static u32 data2 = 2;
static ssize_t axp_regs_show(struct class *class,
			struct class_attribute *attr,   char *buf)
{
	u8 val;
	s32 count = 0, i = 0;
	struct axp_dev *cur_axp_dev = get_pmu_cur_dev(axp_num);

	if (cur_axp_dev == NULL)
		return sprintf(buf, "invalid parameters\n");

	if (cur_axp_dev->is_dummy)
		return sprintf(buf, "unsupported\n");

	for (i = 0; i < data2; i++) {
		axp_regmap_read(cur_axp_dev->regmap, axp_reg_addr+i, &val);
		count += sprintf(buf+count, "%s:REG[0x%x]=0x%x\n",
				get_pmu_cur_name(axp_num), axp_reg_addr+i, val);
	}

	return count;
}

static ssize_t axp_regs_store(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t count)
{
	u32 data1 = 0;
	u8 val[2];
	char *endp;
	struct axp_dev *cur_axp_dev = get_pmu_cur_dev(axp_num);

	if (cur_axp_dev == NULL) {
		pr_warn("invalid parameters\n");
		return -EINVAL;
	}

	if (cur_axp_dev->is_dummy) {
		pr_err("unsupported\n");
		return -EINVAL;
	}

	data1 = simple_strtoul(buf, &endp, 16);
	if (*endp != ' ') {
		pr_err("%s: %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	buf = endp + 1;
	data2 = simple_strtoul(buf, &endp, 10);

	if (data1 < 256) {
		axp_reg_addr = data1;
	} else {
		axp_reg_addr = (data1 >> 16) & 0xFF;
		val[0] = (data1 >> 8) & 0xFF;
		val[1] = data1 & 0xFF;
		axp_regmap_writes(cur_axp_dev->regmap, axp_reg_addr, 2, val);
	}

	return count;
}

int axp_debug;
static ssize_t debug_mask_store(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t count)
{
	int val, err;

	err = kstrtoint(buf, 16, &val);
	if (err)
		return err;

	axp_debug = val;

	return count;
}

static ssize_t debug_mask_show(struct class *class,
			struct class_attribute *attr,   char *buf)
{
	char *s = buf;
	char *end = (char *)((ptrdiff_t)buf + (ptrdiff_t)PAGE_SIZE);

	s += scnprintf(s, end - s, "%s\n", "1: SPLY 2: REGU 4: INT 8: CHG");
	s += scnprintf(s, end - s, "debug_mask=%d\n", axp_debug);

	return s - buf;
}

static struct class_attribute axp_class_attrs[] = {
	__ATTR(axp_name,   S_IRUGO,         axp_name_show,   NULL),
	__ATTR(axp_num,    S_IRUGO|S_IWUSR, axp_num_show,    axp_num_store),
	__ATTR(axp_reg,    S_IRUGO|S_IWUSR, axp_reg_show,    axp_reg_store),
	__ATTR(axp_regs,   S_IRUGO|S_IWUSR, axp_regs_show,   axp_regs_store),
	__ATTR(debug_mask, S_IRUGO|S_IWUSR, debug_mask_show, debug_mask_store),
	__ATTR_NULL
};

struct class axp_class = {
	.name = "axp",
	.class_attrs = axp_class_attrs,
};

static s32 __init axp_core_init(void)
{
	class_register(&axp_class);
	return 0;
}
arch_initcall(axp_core_init);

MODULE_DESCRIPTION("ALLWINNERTECH axp board");
MODULE_AUTHOR("Qin Yongshen");
MODULE_LICENSE("GPL");
