/* extended_standby.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *
 *  By      : liming
 *  Version : v1.0
 *  Date    : 2013-4-17 09:08
 */
#include <linux/module.h>
#include <linux/power/aw_pm.h>
#include <linux/power/scenelock.h>
#include "pm.h"
#include "../../../../kernel/power/power.h"

static int aw_ex_standby_debug_mask;
#undef EXSTANDBY_DBG
#define EXSTANDBY_DBG(format, args...)  do { \
	if (aw_ex_standby_debug_mask) {	    \
		pr_info("[exstandby]"format, ##args); \
	} else {				    \
		do {} while (0)			    \
		;			\
	}					    \
} while (0)

static DEFINE_SPINLOCK(data_lock);

static extended_standby_t temp_standby_data = {
	.id = 0,
};

static extended_standby_manager_t extended_standby_manager = {
	.pextended_standby = NULL,
	.event = 0,
	.wakeup_gpio_map = 0,
	.wakeup_gpio_group = 0,
};

static struct kobject *aw_ex_standby_kobj;

static bool calculate_pll(int index, scene_extended_standby_t *standby_data)
{
	__u32 standby_rate;
	__u32 tmp_standby_rate;
	__u32 dividend;
	__u32 divisor;
	cpux_clk_para_t *pclk_para;
	cpux_clk_para_t *ptmp_clk_para;
	pclk_para = &(standby_data->soc_pwr_dep.cpux_clk_state);
	ptmp_clk_para = &(temp_standby_data.cpux_clk_state);

	switch (index) {
	case PM_PLL_C0:
	case PM_PLL_C1:
	case PM_PLL_AUDIO:
	case PM_PLL_VIDEO0:
	case PM_PLL_VE:
	case PM_PLL_DRAM:
		pr_err("%s err: not ready.\n", __func__);
		break;

	case PM_PLL_PERIPH:
		dividend = pclk_para->pll_factor[index].factor1 *
		    (pclk_para->pll_factor[index].factor2 + 1);
		divisor = pclk_para->pll_factor[index].factor3 + 1;
		standby_rate = do_div(dividend, divisor);

		dividend = ptmp_clk_para->pll_factor[index].factor1 *
		    (ptmp_clk_para->pll_factor[index].factor2 + 1);
		divisor = ptmp_clk_para->pll_factor[index].factor3 + 1;
		tmp_standby_rate = do_div(dividend, divisor);

		if (standby_rate > tmp_standby_rate)
			return true;
		else
			return false;

	case PM_PLL_GPU:
	case PM_PLL_HSIC:
	case PM_PLL_DE:
	case PM_PLL_VIDEO1:
		pr_err("%s err: not ready.\n", __func__);
		break;

	default:
		pr_err("%s err: input para.\n", __func__);
		break;
	}

	return false;
}

static bool calculate_bus(int index, scene_extended_standby_t *standby_data)
{
	cpux_clk_para_t *pclk_para =
	    &(standby_data->soc_pwr_dep.cpux_clk_state);
	cpux_clk_para_t *ptmp_clk_para = &(temp_standby_data.cpux_clk_state);
	if (BUS_NUM <= index) {
		if (pclk_para->bus_factor[index].src >
		    ptmp_clk_para->bus_factor[index].src)
			return true;
		else
			return false;

	} else {
		pr_info("%s: input para err.\n", __func__);
	}

	return false;
}

/*
 * function: make dependency check,
 * for make sure the pwr dependency is reasonable.
 * return 0: if reasonable.
 *        -1: if not reasonable.
 */
static int check_cfg(void)
{
	int ret = 0;
	int i = 0;
	cpux_clk_para_t *ptmp_clk_para = &(temp_standby_data.cpux_clk_state);
	/* make sure bus parent is exist. */
	if (0 != ptmp_clk_para->bus_change) {
		for (i = 0; i < BUS_NUM; i++) {
			if ((CLK_SRC_LOSC == ptmp_clk_para->bus_factor[i].src)
			    && !(ptmp_clk_para->osc_en & BITMAP(OSC_LOSC_BIT))) {
				ret = -1;
			}
			if ((CLK_SRC_HOSC == ptmp_clk_para->bus_factor[i].src)
			    && !(ptmp_clk_para->osc_en & BITMAP(OSC_HOSC_BIT))) {
				ret = -2;
			}
			if ((CLK_SRC_PLL6 == ptmp_clk_para->bus_factor[i].src)
			    && !(ptmp_clk_para->init_pll_dis &
				 BITMAP(PM_PLL_PERIPH))) {
				ret = -3;
			}
		}
	}

	/* check hold_flag is reasonable. */
	if (1 == temp_standby_data.soc_io_state.hold_flag &&
	    (temp_standby_data.soc_pwr_dm_state.state &
	     temp_standby_data.soc_pwr_dm_state.sys_mask &
	     BITMAP(VDD_SYS_BIT))) {
		/* when vdd_sys is ON, no need to set hold_flag; */
		ret = -11;
	}

	/* make sure selfresh flag is reasonable */
	if (0 == temp_standby_data.soc_dram_state.selfresh_flag) {
		/* when selfresh is disable, then VDD_SYS_BIT is needed */
		if (!(temp_standby_data.soc_pwr_dm_state.state &
		      temp_standby_data.soc_pwr_dm_state.sys_mask &
		      BITMAP(VDD_SYS_BIT))) {
			ret = -21;
		}
	}

	if (-1 == ret) {
		pr_info("func: %s, ret = %d.\n", __func__, ret);
		dump_stack();
	}

	return ret;
}

/*
 * return value:
 *        0: merge io done for ptmp_io_para->io_state[i]
 *        -1: exist new io config, need to remerge.
 */
static int merge_io(soc_pwr_dep_t *ppwr_dep, int j,
		    soc_io_para_t *ptmp_io_para, int i)
{
	int new_config_flag = 0;
	soc_io_para_t *io_state = &(ppwr_dep->soc_io_state);
	/* orig configed */
	/* when io has not been initialized. */
	if (0 == ptmp_io_para->io_state[i].paddr) {
		ptmp_io_para->io_state[i].paddr = io_state->io_state[j].paddr;
		ptmp_io_para->io_state[i].value_mask =
		    io_state->io_state[j].value_mask;
		ptmp_io_para->io_state[i].value = io_state->io_state[j].value;
		return 0;
	} else {
		if (ptmp_io_para->io_state[i].paddr ==
		    io_state->io_state[j].paddr &&
		    ptmp_io_para->io_state[i].value_mask ==
		    io_state->io_state[j].value_mask) {
			if (ptmp_io_para->io_state[i].value !=
			    io_state->io_state[j].value) {
				pr_info("NOTICE: io config conflict.\n");
				dump_stack();
			} else {
				pr_info("NOTICE: io config is the same.\n");
				new_config_flag = 0;
				return 0;
			}
		} else {
			/* new config? */
			new_config_flag = 1;
		}
	}
	if (1 == new_config_flag)
		pr_err("NOTICE: exist new io config.\n");

	return -1;
}

/* notice: how to merge io config
 * not supprt add io config,
 * this code just for checking io config.*/
static void merge_io_config(soc_pwr_dep_t *ppwr_dep,
			    soc_io_para_t *ptmp_io_para)
{
	int i = 0;
	int j = 0;

	/* unhold_flag has higher level priority. */
	ptmp_io_para->hold_flag &= ppwr_dep->soc_io_state.hold_flag;

	for (j = 0; j < IO_NUM; j++) {
		/* new added */
		if (0 == ppwr_dep->soc_io_state.io_state[j].paddr) {
			pr_info("io config is not in effect.\n");
			continue;
		}
		for (i = 0; i < IO_NUM; i++) {
			if (0 == merge_io(ppwr_dep, j, ptmp_io_para, i))
				break;
			else
				continue;
		}
	}
	return;
}

/* only update voltage when new config has pwr on info;
 * for stable reason, remain the higher voltage;*/
static void merge_volt_config(soc_pwr_dep_t *ppwr_dep,
			      pwr_dm_state_t *ptmp_pwr_dep)
{
	int i = 0;

	if (0 != (ptmp_pwr_dep->state & ptmp_pwr_dep->sys_mask)) {
		for (i = 0; i < VCC_MAX_INDEX; i++) {
			if (ppwr_dep->soc_pwr_dm_state.volt[i] >
			    ptmp_pwr_dep->volt[i]) {
				ptmp_pwr_dep->volt[i] =
				    ppwr_dep->soc_pwr_dm_state.volt[i];
			}
		}
	}
}

static void merge_clk_config(cpux_clk_para_t *ptmp_clk_para,
			     cpux_clk_para_t *pclk_para,
			     scene_extended_standby_t *standby_data)
{
	int i = 0;

	ptmp_clk_para->osc_en |= pclk_para->osc_en;

	/* 0 is disable, enable have higher priority. */
	ptmp_clk_para->init_pll_dis |= pclk_para->init_pll_dis;
	ptmp_clk_para->exit_pll_en |= pclk_para->exit_pll_en;

	if (0 != pclk_para->pll_change) {
		for (i = 0; i < PLL_NUM; i++) {
			if (pclk_para->pll_change & (0x1 << i)) {
				if (!(ptmp_clk_para->pll_change & (0x1 << i))
				    || calculate_pll(i, standby_data))
					ptmp_clk_para->pll_factor[i]
					    = pclk_para->pll_factor[i];
			}
		}
		ptmp_clk_para->pll_change |= pclk_para->pll_change;
	}
	if (0 != pclk_para->bus_change) {
		for (i = 0; i < BUS_NUM; i++) {
			if (pclk_para->bus_change & (0x1 << i)) {
				if (!(ptmp_clk_para->bus_change & (0x1 << i))
				    || calculate_bus(i, standby_data))
					ptmp_clk_para->bus_factor[i]
					    = pclk_para->bus_factor[i];
			}
		}
		ptmp_clk_para->bus_change |= pclk_para->bus_change;
	}

	return;
}

static int copy_extended_standby_data(scene_extended_standby_t *standby_data)
{
	int i = 0;
	cpux_clk_para_t *pclk_para =
	    &(standby_data->soc_pwr_dep.cpux_clk_state);
	cpux_clk_para_t *ptmp_clk_para = &(temp_standby_data.cpux_clk_state);
	soc_pwr_dep_t *ppwr_dep = &(standby_data->soc_pwr_dep);
	pwr_dm_state_t *ptmp_pwr_dep = &(temp_standby_data.soc_pwr_dm_state);
	soc_io_para_t *ptmp_io_para = &(temp_standby_data.soc_io_state);

	if (!standby_data) {
		temp_standby_data.id = 0;
		ptmp_pwr_dep->state = 0;
		ptmp_pwr_dep->sys_mask = 0;
		memset(&ptmp_pwr_dep->volt, 0, sizeof(ptmp_pwr_dep->volt));

		ptmp_clk_para->osc_en = 0;
		ptmp_clk_para->init_pll_dis = 0;
		ptmp_clk_para->exit_pll_en = 0;
		ptmp_clk_para->pll_change = 0;
		ptmp_clk_para->bus_change = 0;
		memset(&ptmp_clk_para->pll_factor, 0,
		       sizeof(ptmp_clk_para->pll_factor));
		memset(&ptmp_clk_para->bus_factor, 0,
		       sizeof(ptmp_clk_para->bus_factor));

		ptmp_io_para->hold_flag = 0;
		memset(&ptmp_io_para->io_state, 0,
		       sizeof(ptmp_io_para->io_state));

		temp_standby_data.soc_dram_state.selfresh_flag = 0;
	} else {
		if ((0 != temp_standby_data.id)
		    && (!((ppwr_dep->id) & (temp_standby_data.id)))) {
			temp_standby_data.id |= ppwr_dep->id;
			ptmp_pwr_dep->state |= ppwr_dep->soc_pwr_dm_state.state;

			merge_volt_config(ppwr_dep, ptmp_pwr_dep);
			merge_clk_config(ptmp_clk_para,
					 pclk_para, standby_data);
			merge_io_config(ppwr_dep, ptmp_io_para);

			/* un_selfresh_flag has higher level priority. */
			temp_standby_data.soc_dram_state.selfresh_flag
			    &= ppwr_dep->soc_dram_state.selfresh_flag;
		} else if ((0 == temp_standby_data.id)) {
			/* update sys_mask:
			 * when scene_unlock happend or scene_lock cnt > 0 */
#if defined(CONFIG_AW_AXP)
			ptmp_pwr_dep->sys_mask = get_sys_pwr_dm_mask();
#endif
			temp_standby_data.id = ppwr_dep->id;
			ptmp_pwr_dep->state = ppwr_dep->soc_pwr_dm_state.state;
			if (0 != (ptmp_pwr_dep->state & ptmp_pwr_dep->sys_mask)) {
				for (i = 0; i < VCC_MAX_INDEX; i++) {
					ptmp_pwr_dep->volt[i] =
					    ppwr_dep->soc_pwr_dm_state.volt[i];
				}
			} else
				memset(&ptmp_pwr_dep->volt, 0,
				       sizeof(ptmp_pwr_dep->volt));

			ptmp_clk_para->osc_en = pclk_para->osc_en;
			ptmp_clk_para->init_pll_dis = pclk_para->init_pll_dis;
			ptmp_clk_para->exit_pll_en = pclk_para->exit_pll_en;
			ptmp_clk_para->pll_change = pclk_para->pll_change;
			if (0 != pclk_para->pll_change) {
				for (i = 0; i < PLL_NUM; i++) {
					ptmp_clk_para->pll_factor[i] =
					    pclk_para->pll_factor[i];
				}
			} else
				memset(&ptmp_clk_para->pll_factor, 0,
				       sizeof(ptmp_clk_para->pll_factor));

			ptmp_clk_para->bus_change = pclk_para->bus_change;
			if (0 != pclk_para->bus_change) {
				for (i = 0; i < BUS_NUM; i++) {
					ptmp_clk_para->bus_factor[i] =
					    pclk_para->bus_factor[i];
				}
			} else
				memset(&ptmp_clk_para->bus_factor, 0,
				       sizeof(ptmp_clk_para->bus_factor));

			ptmp_io_para->hold_flag =
			    ppwr_dep->soc_io_state.hold_flag;
			for (i = 0; i < IO_NUM; i++) {
				ptmp_io_para->io_state[i] =
				    ppwr_dep->soc_io_state.io_state[i];
			}

			temp_standby_data.soc_dram_state.selfresh_flag =
			    ppwr_dep->soc_dram_state.selfresh_flag;
		}
	}

	return check_cfg();
}

/**
 *	get_extended_standby_manager -
 *			get the extended_standby_manager pointer
 *
 *	Return	: if the extended_standby_manager is effective,
 *				return the extended_standby_manager pointer;
 *				else return NULL;
 *	Notes	: you can check the configuration from the pointer.
 */
const extended_standby_manager_t *get_extended_standby_manager(void)
{
	unsigned long irqflags;
	extended_standby_manager_t *manager_data = NULL;
	spin_lock_irqsave(&data_lock, irqflags);
	manager_data = &extended_standby_manager;
	spin_unlock_irqrestore(&data_lock, irqflags);
	if ((NULL != manager_data) && (NULL != manager_data->pextended_standby)) {
#if defined(CONFIG_AW_AXP)
		/* update sys_mask */
		manager_data->pextended_standby->soc_pwr_dm_state.sys_mask
		    = get_sys_pwr_dm_mask();
#endif
		EXSTANDBY_DBG("leave %s : id 0x%x\n", __func__,
			      manager_data->pextended_standby->id);
	}
	return manager_data;
}

/**
 *	set_extended_standby_manager - set the extended_standby_manager;
 *	manager@: the manager config.
 *
 *      return value: if the setting is correct, return true.
 *		      else return false;
 *      notes: the function will check
 *				the struct member: pextended_standby and event.
 *		if the setting is not proper, return false.
 */
bool set_extended_standby_manager(scene_extended_standby_t *local_standby)
{
	unsigned long irqflags;

	EXSTANDBY_DBG("enter %s\n", __func__);

	if (local_standby &&
	    (0 == local_standby->soc_pwr_dep.soc_pwr_dm_state.state)) {
		return true;
	}

	if (!local_standby) {
		spin_lock_irqsave(&data_lock, irqflags);
		copy_extended_standby_data(NULL);
		extended_standby_manager.pextended_standby = NULL;
		spin_unlock_irqrestore(&data_lock, irqflags);
		return true;
	} else {
		spin_lock_irqsave(&data_lock, irqflags);
		copy_extended_standby_data(local_standby);
		extended_standby_manager.pextended_standby = &temp_standby_data;
		spin_unlock_irqrestore(&data_lock, irqflags);
	}

	if (NULL != extended_standby_manager.pextended_standby)
		EXSTANDBY_DBG("leave %s : id 0x%x\n", __func__,
			      extended_standby_manager.pextended_standby->id);
	return true;
}

/**
 *	extended_standby_set_pmu_id   -     set pmu_id for suspend modules.
 *
 *	@num:	pmu serial number;
 *	@pmu_id: corresponding pmu_id;
 */
int extended_standby_set_pmu_id(unsigned int num, unsigned int pmu_id)
{
	unsigned int tmp;

	if (num > 4 || num < 0) {
		pr_err("num: %ux not valid.\n", num);
		return -1;
	}

	tmp = temp_standby_data.pmu_id;
	tmp &= ~(0xff << ((num) * 8));
	tmp |= (pmu_id << ((num) * 8));
	temp_standby_data.pmu_id = tmp;

	return 0;
}

/**
 *	extended_standby_get_pmu_id   -
 *  get specific pmu_id for suspend modules.
 *
 *	@num:	pmu serial number;
 */
int extended_standby_get_pmu_id(unsigned int num)
{
	unsigned int tmp;

	if (num > 4 || num < 0) {
		pr_err("num: %ux not valid.\n", num);
		return -1;
	}

	tmp = temp_standby_data.pmu_id;
	tmp >>= ((num) * 8);
	tmp &= (0xff);

	return tmp;
}

/**
 *	extended_standby_store_dram_crc_paras
 *			-     store dram_crc_paras for suspend modules.
 *
 *	@num:	pmu serial number;
 *	@dram_crc_paras: corresponding dram_crc_paras;
 */
static ssize_t extended_standby_dram_crc_paras_store(struct device *dev, struct
						     device_attribute
						     *attr,
						     const char *buf,
						     size_t size)
{
	unsigned int dram_crc_en;
	unsigned int dram_crc_start;
	unsigned int dram_crc_len;

	sscanf(buf, "%x %x %x\n", &dram_crc_en, &dram_crc_start, &dram_crc_len);
	if ((dram_crc_en != 0) && (dram_crc_en != 1)) {
		pr_err("invalid paras for dram_crc: [%x] [%x] [%x]\n",
		       dram_crc_en, dram_crc_start, dram_crc_len);
		return size;
	}

	temp_standby_data.soc_dram_state.crc_en = dram_crc_en;
	temp_standby_data.soc_dram_state.crc_start = dram_crc_start;
	temp_standby_data.soc_dram_state.crc_len = dram_crc_len;

	return size;
}

/**
 *	extended_standby_show_dram_crc_paras
 *		-     show specific dram_crc_paras for suspend modules.
 *
 *	@num:	pmu serial number;
 */
ssize_t extended_standby_dram_crc_paras_show(struct device *dev,
					     struct device_attribute *
					     attr, char *buf)
{
	char *s = buf;

	s += sprintf(buf, "dram_crc_paras: enable, start, len == \
			[%x] [%x] [%x]\n", temp_standby_data.soc_dram_state.crc_en, temp_standby_data.soc_dram_state.crc_start, temp_standby_data.soc_dram_state.crc_len);

	return s - buf;
}

/**
 *	extended_standby_enable_wakeup_src   -	enable the wakeup src.
 *
 *	function:	the device driver care about the wakeup src.
 *				if the device driver do want the system
 *				be wakenup while in standby state.
 *				the device driver should use this function
 *              to enable corresponding intterupt.
 *	@src:		wakeup src.
 *	@para:		if wakeup src need para, be the para of wakeup src,
 *				else ignored.
 *	notice:		1. for gpio intterupt, only access the enable bit,
 *					mean u need care about other config,
 *					such as: int mode, pull up
 *						or pull down resistance, etc.
 *				2. At a31, only gpio��pa, pb, pe, pg,
 *                 pl, pm��int wakeup src is supported.
 */
int extended_standby_enable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;
	unsigned long gpio_map = extended_standby_manager.wakeup_gpio_map;
	unsigned long gpio_group = extended_standby_manager.wakeup_gpio_group;
	spin_lock_irqsave(&data_lock, irqflags);
	extended_standby_manager.event |= src;
	if (CPUS_GPIO_SRC & src) {
		if (para >= AXP_PIN_BASE) {
			gpio_map |= (WAKEUP_GPIO_AXP((para - AXP_PIN_BASE)));
		} else if (para >= SUNXI_PM_BASE) {
			gpio_map |= (WAKEUP_GPIO_PM((para - SUNXI_PM_BASE)));
		} else if (para >= SUNXI_PL_BASE) {
			gpio_map |= (WAKEUP_GPIO_PL((para - SUNXI_PL_BASE)));
		} else if (para >= SUNXI_PH_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('H'));
		} else if (para >= SUNXI_PG_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('G'));
		} else if (para >= SUNXI_PF_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('F'));
		} else if (para >= SUNXI_PE_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('E'));
		} else if (para >= SUNXI_PD_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('D'));
		} else if (para >= SUNXI_PC_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('C'));
		} else if (para >= SUNXI_PB_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('B'));
		} else if (para >= SUNXI_PA_BASE) {
			gpio_group |= (WAKEUP_GPIO_GROUP('A'));
		} else {
			pr_info("cpux need care gpio %d. but, notice,\
				currently, cpux not support it.\n", para);
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);
	EXSTANDBY_DBG("leave %s : event 0x%lx\n",
		      __func__, extended_standby_manager.event);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_map 0x%lx\n", __func__, gpio_map);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_group 0x%lx\n",
		      __func__, gpio_group);

	extended_standby_manager.wakeup_gpio_group = gpio_group;
	extended_standby_manager.wakeup_gpio_map = gpio_map;
	return 0;
}

/**
 *	extended_standby_disable_wakeup_src  -	disable the wakeup src.
 *
 *	function:	if the device driver do not want the system
 *               be wakenup while in standby state again.
 *			     the device driver should use this function
 *               to disable the corresponding intterupt.
 *
 *	@src:		wakeup src.
 *	@para:		if wakeup src need para, be the para of wakeup src,
 *			     else ignored.
 *	notice:		for gpio intterupt, only access the enable bit,
 *               mean u need care about other config,
 *			     such as: int mode, pull up or
 *               pull down resistance, etc.
 */
int extended_standby_disable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;
	extended_standby_manager_t *pextend_mnger = &(extended_standby_manager);
	unsigned long gpio_map = extended_standby_manager.wakeup_gpio_map;
	unsigned long gpio_group = extended_standby_manager.wakeup_gpio_group;

	spin_lock_irqsave(&data_lock, irqflags);
	pextend_mnger->event &= (~src);
	if (CPUS_GPIO_SRC & src) {
		if (para >= AXP_PIN_BASE) {
			gpio_map &= (~(WAKEUP_GPIO_AXP((para - AXP_PIN_BASE))));
		} else if (para >= SUNXI_PM_BASE) {
			gpio_map &= (~(WAKEUP_GPIO_PM((para - SUNXI_PM_BASE))));
		} else if (para >= SUNXI_PL_BASE) {
			gpio_map &= (~(WAKEUP_GPIO_PL((para - SUNXI_PL_BASE))));
		} else if (para >= SUNXI_PH_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('H')));
		} else if (para >= SUNXI_PG_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('G')));
		} else if (para >= SUNXI_PF_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('F')));
		} else if (para >= SUNXI_PE_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('E')));
		} else if (para >= SUNXI_PD_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('D')));
		} else if (para >= SUNXI_PC_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('C')));
		} else if (para >= SUNXI_PB_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('B')));
		} else if (para >= SUNXI_PA_BASE) {
			gpio_group &= (~(WAKEUP_GPIO_GROUP('A')));
		} else {
			pr_info("cpux need care gpio %d. but, notice,\
				currently, cpux not support it.\n", para);
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);
	EXSTANDBY_DBG("leave %s : event 0x%lx\n",
		      __func__, pextend_mnger->event);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_map 0x%lx\n", __func__, gpio_map);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_group 0x%lx\n",
		      __func__, gpio_group);
	extended_standby_manager.wakeup_gpio_group = gpio_group;
	extended_standby_manager.wakeup_gpio_map = gpio_map;
	return 0;
}

/**
 *	extended_standby_check_wakeup_state
 *        -   to get the corresponding
 *            wakeup src intterupt state, enable or disable.
 *
 *	@src:		wakeup src.
 *	@para:		if wakeup src need para, be the para of wakeup src,
 *				else ignored.
 *
 *	return value:
 *              enable,		return 1,
 *				disable,	return 2,
 *				error:		return -1.
 */
int extended_standby_check_wakeup_state(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;
	int ret = -1;
	spin_lock_irqsave(&data_lock, irqflags);
	if (extended_standby_manager.event & src)
		ret = 1;
	else
		ret = 2;
	spin_unlock_irqrestore(&data_lock, irqflags);

	return ret;
}

/**
 *
 *	function:		standby state including locked_scene,
 *  power_supply dependancy, the wakeup src.
 *
 *	return value:
 *             succeed, return 0,
 *             else return -1.
 */
int extended_standby_show_state(void)
{
	unsigned long irqflags;
	int i = 0;
	unsigned int pwr_on_bitmap = 0;
	unsigned int pwr_off_bitmap = 0;
	extended_standby_t *pextend_standby =
	    (extended_standby_manager.pextended_standby);
	cpux_clk_para_t *pclk_para = &(pextend_standby->cpux_clk_state);

	standby_show_state();

	spin_lock_irqsave(&data_lock, irqflags);
	pr_info("dynamic config wakeup_src: 0x%16lx\n",
		extended_standby_manager.event);
	parse_wakeup_event(NULL, 0, extended_standby_manager.event);
	pr_info("wakeup_gpio_map 0x%16lx\n",
		extended_standby_manager.wakeup_gpio_map);
	parse_wakeup_gpio_map(NULL, 0,
			      extended_standby_manager.wakeup_gpio_map);
	pr_info("wakeup_gpio_group 0x%16lx\n",
		extended_standby_manager.wakeup_gpio_group);
	parse_wakeup_gpio_group_map(NULL, 0,
				    extended_standby_manager.wakeup_gpio_group);
	if (NULL != pextend_standby) {
		pr_info("extended_standby id = 0x%16x\n", pextend_standby->id);
		pr_info("extended_standby pmu_id = 0x%16x\n",
			pextend_standby->pmu_id);
		pr_info("extended_standby soc_id = 0x%16x\n",
			pextend_standby->soc_id);
		pr_info("extended_standby pwr dep as follow:\n");

		pr_info("pwr dm state as follow:\n");
		pr_info("\tpwr dm state = 0x%8x.\n",
			pextend_standby->soc_pwr_dm_state.state);
		parse_pwr_dm_map(NULL, 0,
				 pextend_standby->soc_pwr_dm_state.state);
		pr_info("\tpwr dm sys mask = 0x%8x.\n",
			pextend_standby->soc_pwr_dm_state.sys_mask);
		parse_pwr_dm_map(NULL, 0,
				 pextend_standby->soc_pwr_dm_state.sys_mask);

		pwr_on_bitmap =
		    pextend_standby->
		    soc_pwr_dm_state.sys_mask & pextend_standby->
		    soc_pwr_dm_state.state;
		pr_info("\tpwr on = 0x%x.\n", pwr_on_bitmap);
		parse_pwr_dm_map(NULL, 0, pwr_on_bitmap);

		pwr_off_bitmap =
		    (~pextend_standby->
		     soc_pwr_dm_state.sys_mask) | pextend_standby->
		    soc_pwr_dm_state.state;
		pr_info("\tpwr off = 0x%x.\n", pwr_off_bitmap);
		parse_pwr_dm_map(NULL, 0, (~pwr_off_bitmap));

		EXSTANDBY_DBG("\tpwr on volt which need adjusted:\n");
		if (0 != (pextend_standby->soc_pwr_dm_state.state &
			  pextend_standby->soc_pwr_dm_state.sys_mask)) {
			for (i = 0; i < VCC_MAX_INDEX; i++) {
				if (0 !=
				    pextend_standby->soc_pwr_dm_state.volt[i]) {
					pr_info
					    ("index = %d, volt[]= %d.\n",
					     i,
					     pextend_standby->soc_pwr_dm_state.
					     volt[i]);
				}
			}
		}

		EXSTANDBY_DBG("cpux clk state as follow:\n");
		EXSTANDBY_DBG("    cpux osc en: 0x%8x.\n", pclk_para->osc_en);
		EXSTANDBY_DBG
		    ("    cpux pll init disabled config: 0x%8x.\n",
		     pclk_para->init_pll_dis);
		EXSTANDBY_DBG("    cpux pll exit enable config: 0x%8x.\n",
			      pclk_para->exit_pll_en);

		if (0 != pclk_para->pll_change) {
			for (i = 0; i < PLL_NUM; i++) {
				EXSTANDBY_DBG("pll%i: factor1=%d factor2=%d\
			factor3=%d factor4=%d\n", i, pclk_para->pll_factor[i].factor1, pclk_para->pll_factor[i].factor2, pclk_para->pll_factor[i].factor3, pclk_para->pll_factor[i].factor4);
			}
		} else {
			EXSTANDBY_DBG("pll_change == 0: no pll need change.\n");
		}

		if (0 != pclk_para->bus_change) {
			for (i = 0; i < BUS_NUM; i++) {
				EXSTANDBY_DBG("bus%i: src=%d pre_div=%d\
			div_ratio=%d n=%d m=%d\n", i, pclk_para->bus_factor[i].src, pclk_para->bus_factor[i].pre_div, pclk_para->bus_factor[i].div_ratio, pclk_para->bus_factor[i].n, pclk_para->bus_factor[i].m);
			}
		} else {
			EXSTANDBY_DBG("bus_change == 0: no bus need change.\n");
		}

		EXSTANDBY_DBG("cpux io state as follow:\n");
		EXSTANDBY_DBG("     hold_flag = %d.\n",
			      pextend_standby->soc_io_state.hold_flag);

		for (i = 0; i < IO_NUM; i++) {
			if (0 !=
			    pextend_standby->soc_io_state.io_state[i].paddr) {
				pr_info("    count %4d io config: addr 0x%x,\
			value_mask 0x%8x, value 0x%8x.\n", i, pextend_standby->soc_io_state.io_state[i].paddr, pextend_standby->soc_io_state.io_state[i].value_mask, pextend_standby->soc_io_state.io_state[i].value);
			}
		}

		EXSTANDBY_DBG("soc dram state as follow:\n");
		EXSTANDBY_DBG("    selfresh_flag = %d.\n",
			      pextend_standby->soc_dram_state.selfresh_flag);

	}

	spin_unlock_irqrestore(&data_lock, irqflags);

	return 0;
}

static DEVICE_ATTR(dram_crc_paras, S_IRUGO | S_IWUSR | S_IWGRP,
		   extended_standby_dram_crc_paras_show,
		   extended_standby_dram_crc_paras_store);

static struct attribute *g[] = {
	&dev_attr_dram_crc_paras.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

static int __init aw_ex_standby_init(void)
{
	int error = 0;

	aw_ex_standby_kobj =
	    kobject_create_and_add("aw_ex_standby", power_kobj);
	if (!aw_ex_standby_kobj)
		return -ENOMEM;
	error = sysfs_create_group(aw_ex_standby_kobj, &attr_group);

	return error ? error : 0;
}

/*
 *************************************************************
 *                           aw_ex_standby_exit
 *
 *Description: exit ex_standby sub-system on platform;
 *
 *Arguments  : none
 *
 *Return     : none
 *
 *Notes      :
 *
 *************************************************************
 */
static void __exit aw_ex_standby_exit(void)
{
	pr_info("aw_ex_standby_exit!\n");

	return;
}

module_param_named(aw_ex_standby_debug_mask, aw_ex_standby_debug_mask,
		   int, S_IRUGO | S_IWUSR);
module_init(aw_ex_standby_init);
module_exit(aw_ex_standby_exit);
