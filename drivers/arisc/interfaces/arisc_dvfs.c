/*
 *  drivers/arisc/interfaces/arisc_dvfs.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"
#include <mach/sys_config.h>
#include <mach/sunxi-chip.h>

typedef struct arisc_freq_voltage
{
	u32 freq;       //cpu frequency
	u32 voltage;    //voltage for the frequency
	u32 axi_div;    //the divide ratio of axi bus
} arisc_freq_voltage_t;

static int arisc_dvfs_get_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("arisc dvfs config:%s type:%d err!\n", sub, type);
		return -EINVAL;
	}
	*val = script_val.val;
	ARISC_INF("arisc dvfs config [%s] [%s] : %d\n", main, sub, *val);
	return 0;
}

//cpu voltage-freq table
#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || \
    (defined CONFIG_ARCH_SUN8IW7P1)
static struct arisc_freq_voltage arisc_vf_table[ARISC_DVFS_VF_TABLE_MAX] =
{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
};
#elif defined (CONFIG_ARCH_SUN9IW1P1) || (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN8IW9P1)
static struct arisc_freq_voltage arisc_vf_table[2][ARISC_DVFS_VF_TABLE_MAX] =
{
	{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	},

	{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	},
};
#endif

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || \
    (defined CONFIG_ARCH_SUN8IW7P1)
int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    result = 0;
	int    vf_table_size = 0;
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	struct arisc_message *pmessage;
	u32    ver;
	u32    is_def_table = 0;
	spinlock_t    dvfs_lock;    /* spinlock for dvfs */
	unsigned long dvfs_flag;
#if (defined CONFIG_ARCH_SUN8IW7P1)
	script_item_u script_val;
	script_item_value_type_e type;
	script_item_u *pin_list;
	int pin_count = 0;
	int pin_index = 0;
	struct gpio_config *pin_cfg;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	unsigned long config;
#endif

	/* initialize message manager spinlock */
	spin_lock_init(&(dvfs_lock));
	dvfs_flag = 0;

	spin_lock_irqsave(&(dvfs_lock), dvfs_flag);
	ver = readl(IO_ADDRESS(SUNXI_SRAMCTRL_PBASE + 0x24));
	ver |= 0x1 << 15;
	writel(ver, IO_ADDRESS(SUNXI_SRAMCTRL_PBASE + 0x24));
	ver = readl(IO_ADDRESS(SUNXI_SRAMCTRL_PBASE + 0x24)) >> 16;
	spin_unlock_irqrestore(&(dvfs_lock), dvfs_flag);

	if (ver == 0x1661) {
		if (arisc_dvfs_get_cfg("dvfs_table_bak", "LV_count", &vf_table_size)) {
			strcpy(vf_table_main_key, "dvfs_table");
			is_def_table = 1;
		} else {
			strcpy(vf_table_main_key, "dvfs_table_bak");
			is_def_table = 0;
		}
	} else {
		strcpy(vf_table_main_key, "dvfs_table");
		is_def_table = 1;
	}

	/* parse system config v-f table information */
	if (arisc_dvfs_get_cfg(vf_table_main_key, "LV_count", &vf_table_size)) {
		ARISC_WRN("parse system config dvfs_table size fail\n");
	}
	for (index = 0; index < vf_table_size; index++) {
		sprintf(vf_table_sub_key, "LV%d_freq", index + 1);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			arisc_vf_table[index].freq = value;
		}
		sprintf(vf_table_sub_key, "LV%d_volt", index + 1);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			if ((value < 1100) && (ver == 0x1661) && (is_def_table)) {
				value = 1100;
			}
			arisc_vf_table[index].voltage = value;
		}
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
		/* initialize message
		 *
		 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
		 * | index  |  freq  |voltage |axi_div |  pll  |
		 */
		pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
		pmessage->paras[0]   = index;
		pmessage->paras[1]   = arisc_vf_table[index].freq;
		pmessage->paras[2]   = arisc_vf_table[index].voltage;
		pmessage->paras[3]   = arisc_vf_table[index].axi_div;
		pmessage->paras[4]   = 0;
		pmessage->state      = ARISC_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;

		ARISC_INF("v-f table: index %d freq %d vol %d axi_div %d\n",
		pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);

		/* send request message */
		arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

		//check config fail or not
		if (pmessage->result) {
			ARISC_WRN("config dvfs v-f table [%d] fail\n", index);
			result = -EINVAL;
			break;
		}
	}
#if (defined CONFIG_ARCH_SUN8IW7P1)
/* [pmuic_type]:0~2, 0:none, 1:gpio, 2:i2c
 * [gpio0_cfg ]:bit0~15:gpio num, bit16:used
 * [gpio1_cfg ]:bit0~15:gpio num, bit16:used
 * [pmu_level0]:bit0~15:voltage(mV), bit16~19:gpio0 state, bit20~23:gpio1 state
 * [pmu_level1]:bit0~15:voltage(mV), bit16~19:gpio0 state, bit20~23:gpio1 state
 * [pmu_level2]:bit0~15:voltage(mV), bit16~19:gpio0 state, bit20~23:gpio1 state
 * [pmu_level3]:bit0~15:voltage(mV), bit16~19:gpio0 state, bit20~23:gpio1 state
 */
#define GPIO_USED_CFG(x) ((x) | (1<<16))
#define VOL_LEVEL_CFG(x) ((x)%10000) | ((((x)/10000)%10)<<16) | ((((x)/100000)%10)<<20)

	/* get pmuic type */
	type = script_get_item("dvfs_table", "pmuic_type", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("arisc dvfs config err! type:%d\n", type);
		return -EINVAL;
	}
	pmessage->paras[0] = script_val.val;

	if (pmessage->paras[0] != 1) /* not gpio pmu */
		goto out;

	/* get gpio config */
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pin_count = script_get_pio_list ("dvfs_table", &pin_list);
	for (pin_index = 0; pin_index < pin_count; pin_index++) {
		pin_cfg = &(pin_list[pin_index].gpio);
		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg->pull);
			pin_config_set (SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg->drv_level);
			pin_config_set (SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
			pin_config_set (SUNXI_PINCTRL, pin_name, config);
		}
		/* NOTE: only used PL, the gpio group info. is discard */
		pmessage->paras[1 + pin_index] = GPIO_USED_CFG((pin_cfg->gpio) % SUNXI_BANK_SIZE);
		ARISC_INF("%s,%d,id:%d,num:%d,reg:%x\n", __func__, __LINE__, pin_index, \
		          pin_cfg->gpio, pmessage->paras[1 + pin_index]);
	}

	if (pin_count < 2) /* only gpio0 used */
		vf_table_size = 2;
	else
		vf_table_size = 4;

	for (index = 0; index < vf_table_size; index++) {
		sprintf(vf_table_sub_key, "pmu_level%d", index);
		type = script_get_item(vf_table_main_key, vf_table_sub_key, &script_val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			ARISC_ERR("arisc dvfs config err! type:%d\n", type);
			return -EINVAL;
		}
		pmessage->paras[3 + index] = VOL_LEVEL_CFG(script_val.val);
	}

out:
	pmessage->type       = ARISC_CPUX_DVFS_CFG_REQ;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	if (pmessage->result) {
		ARISC_WRN("config dvfs cfg fail\n");
		result = -EINVAL;
	}
#endif
	/* free allocated message */
	arisc_message_free(pmessage);

	return result;

}

#elif defined CONFIG_ARCH_SUN8IW5P1
int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    result = 0;
	int    vf_table_size = 0;
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	int    vf_table_count = 0;
	int    vf_table_type = 0;
	struct arisc_message *pmessage;

	if (arisc_dvfs_get_cfg("dvfs_table", "vf_table_count", &vf_table_count)) {
		ARISC_LOG("%s: support only one vf_table\n", __func__);
		sprintf(vf_table_main_key, "%s", "dvfs_table");
	} else {
		vf_table_type = sunxi_get_soc_bin();
		sprintf(vf_table_main_key, "%s%d", "vf_table", vf_table_type);
	}
	pr_info("%s: vf table type [%d=%s]\n", __func__, vf_table_type, vf_table_main_key);

	/* parse system config v-f table information */
	if (arisc_dvfs_get_cfg(vf_table_main_key, "LV_count", &vf_table_size)) {
		ARISC_WRN("parse system config dvfs_table size fail\n");
	}
	for (index = 0; index < vf_table_size; index++) {
		sprintf(vf_table_sub_key, "LV%d_freq", index + 1);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			arisc_vf_table[index].freq = value;
		}
		pr_info("%s: freq [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
		sprintf(vf_table_sub_key, "LV%d_volt", index + 1);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			arisc_vf_table[index].voltage = value;
		}
		pr_info("%s: volt [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
		/* initialize message
		 *
		 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
		 * | index  |  freq  |voltage |axi_div |  pll  |
		 */
		pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
		pmessage->paras[0]   = index;
		pmessage->paras[1]   = arisc_vf_table[index].freq;
		pmessage->paras[2]   = arisc_vf_table[index].voltage;
		pmessage->paras[3]   = arisc_vf_table[index].axi_div;
		pmessage->paras[4]   = 0;
		pmessage->state      = ARISC_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;

		ARISC_INF("v-f table: index %d freq %d vol %d axi_div %d\n",
		pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);


		/* send request message */
		arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

		//check config fail or not
		if (pmessage->result) {
			ARISC_WRN("config dvfs v-f table [%d] fail\n", index);
			result = -EINVAL;
			break;
		}
	}
	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}

#elif (defined CONFIG_ARCH_SUN9IW1P1)
int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    cluster = 0;
	int    result = 0;
	int    vf_table_size[2] = {0, 0};
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	char   cluster_name[2] = {'L', 'B'};
	int    vf_table_count = 0;
	int    vf_table_type = 0;
	struct arisc_message *pmessage;

	if (arisc_dvfs_get_cfg("dvfs_table", "vf_table_count", &vf_table_count)) {
		ARISC_LOG("%s: support only one vf_table\n", __func__);
		sprintf(vf_table_main_key, "%s", "dvfs_table");
	} else {
		vf_table_type = sunxi_get_soc_bin();
		sprintf(vf_table_main_key, "%s%d", "vf_table", vf_table_type);
	}
	ARISC_INF("%s: vf table type [%d=%s]\n", __func__, vf_table_type, vf_table_main_key);

	/* parse system config v-f table information */
	for (cluster = 0; cluster < 2; cluster++) {
		sprintf(vf_table_sub_key, "%c_LV_count", cluster_name[cluster]);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &vf_table_size[cluster])) {
			ARISC_WRN("parse system config dvfs_table size fail\n");
		}

		for (index = 0; index < vf_table_size[cluster]; index++) {
			sprintf(vf_table_sub_key, "%c_LV%d_freq", cluster_name[cluster], index + 1);
			if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
				arisc_vf_table[cluster][index].freq = value;
			}
			ARISC_INF("%s: freq [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
			sprintf(vf_table_sub_key, "%c_LV%d_volt", cluster_name[cluster], index + 1);
			if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
				arisc_vf_table[cluster][index].voltage = value;
			}
			ARISC_INF("%s: volt [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
		}
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (cluster = 0; cluster < 2; cluster++) {
		for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
			/* initialize message
			 *
			 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
			 * | index  |  freq  |voltage |axi_div |  pll   |
			 */
			pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
			pmessage->paras[0]   = index;
			pmessage->paras[1]   = arisc_vf_table[cluster][index].freq;
			pmessage->paras[2]   = arisc_vf_table[cluster][index].voltage;
			pmessage->paras[3]   = arisc_vf_table[cluster][index].axi_div;
			pmessage->paras[4]   = cluster;
			pmessage->state      = ARISC_MESSAGE_INITIALIZED;
			pmessage->cb.handler = NULL;
			pmessage->cb.arg     = NULL;

			ARISC_INF("v-f table: cluster %d index %d freq %d vol %d axi_div %d\n",
			pmessage->paras[4], pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);

			/* send request message */
			arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

			//check config fail or not
			if (pmessage->result) {
				ARISC_WRN("config dvfs v-f table [%d][%d] fail\n", cluster, index);
				result = -EINVAL;
				break;
			}
		}
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}
#elif (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN8IW9P1)
int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    cluster = 0;
	int    result = 0;
	int    vf_table_size[2] = {0, 0};
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	char   cluster_name[2] = {'L', 'B'};
	int    vf_table_count = 0;
	int    vf_table_type = 0;
	struct arisc_message *pmessage;

	if (arisc_dvfs_get_cfg("dvfs_table", "vf_table_count", &vf_table_count)) {
		ARISC_LOG("%s: support only one vf_table\n", __func__);
		sprintf(vf_table_main_key, "%s", "dvfs_table");
	} else {
		vf_table_type = sunxi_get_soc_bin();
		sprintf(vf_table_main_key, "%s%d", "vf_table", vf_table_type);
	}
	ARISC_INF("%s: vf table type [%d=%s]\n", __func__, vf_table_type, vf_table_main_key);

	/* parse system config v-f table information */
	for (cluster = 0; cluster < 1; cluster++) {
		sprintf(vf_table_sub_key, "%c_LV_count", cluster_name[cluster]);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &vf_table_size[cluster])) {
			ARISC_WRN("parse system config dvfs_table size fail\n");
		}

		for (index = 0; index < vf_table_size[cluster]; index++) {
			sprintf(vf_table_sub_key, "%c_LV%d_freq", cluster_name[cluster], index + 1);
			if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
				arisc_vf_table[cluster][index].freq = value;
			}
			ARISC_INF("%s: freq [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
			sprintf(vf_table_sub_key, "%c_LV%d_volt", cluster_name[cluster], index + 1);
			if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
				arisc_vf_table[cluster][index].voltage = value;
			}
			ARISC_INF("%s: volt [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
		}
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (cluster = 0; cluster < 1; cluster++) {
		for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
			/* initialize message
			 *
			 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
			 * | index  |  freq  |voltage |axi_div |  pll   |
			 */
			pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
			pmessage->paras[0]   = index;
			pmessage->paras[1]   = arisc_vf_table[cluster][index].freq;
			pmessage->paras[2]   = arisc_vf_table[cluster][index].voltage;
			pmessage->paras[3]   = arisc_vf_table[cluster][index].axi_div;
			pmessage->paras[4]   = cluster;
			pmessage->state      = ARISC_MESSAGE_INITIALIZED;
			pmessage->cb.handler = NULL;
			pmessage->cb.arg     = NULL;

			ARISC_INF("v-f table: cluster %d index %d freq %d vol %d axi_div %d\n",
			pmessage->paras[4], pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);

			/* send request message */
			arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

			//check config fail or not
			if (pmessage->result) {
				ARISC_WRN("config dvfs v-f table [%d][%d] fail\n", cluster, index);
				result = -EINVAL;
				break;
			}
		}
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}
#endif

/*
 * set specific pll target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @pll:     which pll will be set
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int arisc_dvfs_set_cpufreq(unsigned int freq, unsigned int pll, unsigned long mode, arisc_cb_t cb, void *cb_arg)
{
	unsigned int          msg_attr = 0;
	struct arisc_message *pmessage;
	int                   result = 0;

	if (mode & ARISC_DVFS_SYN) {
		msg_attr |= ARISC_MESSAGE_ATTR_HARDSYN;
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(msg_attr);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message
	 *
	 * |paras[0]|paras[1]|
	 * |freq    |pll     |
	 */
	pmessage->type       = ARISC_CPUX_DVFS_REQ;
	pmessage->paras[0]   = freq;
	pmessage->paras[1]   = pll;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;

	ARISC_INF("arisc dvfs request : %d\n", freq);
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* dvfs mode : syn or not */
	if (mode & ARISC_DVFS_SYN) {
		result = pmessage->result;
		arisc_message_free(pmessage);
	}

	return result;
}
EXPORT_SYMBOL(arisc_dvfs_set_cpufreq);
