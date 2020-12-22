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
#include <mach/gpio.h>
#include <linux/ctype.h>
#include <linux/module.h>

typedef enum{
	DEBUG_WAKEUP_SRC            =   (0x01<<0),
	DEBUG_WAKEUP_GPIO_MAP       =   (0x01<<1),
	DEBUG_WAKEUP_GPIO_GROUP_MAP =   (0x01<<2),
	DEBUG_PWR_DM_MAP            =   (0x01<<3)
}parse_bitmap_en_flag;

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define uk_printf(s, size, fmt, args...) do { \
	if (NULL != s) {			\
		s += scnprintf(s, size, fmt, ## args);	\
	} else {				\
		printk(fmt, ## args);		\
	}					\
    } while(0)

#define SUNXI_EXSTANDBY_DBG   1
#undef EXSTANDBY_DBG
#if(SUNXI_EXSTANDBY_DBG)
#define EXSTANDBY_DBG(format,args...)   printk("[exstandby]"format,##args)
#else
#define EXSTANDBY_DBG(format,args...)   do{}while(0)
#endif

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

static bool calculate_pll(int index, scene_extended_standby_t *standby_data)
{
	__u32 standby_rate;
	__u32 temp_standby_rata;
	__u32 dividend;
	__u32 divisor;

	switch (index) {
	case 1: /* PLL2 */
		dividend = standby_data->extended_standby_data.pll_factor[index].n;
		divisor = standby_data->extended_standby_data.pll_factor[index].m * standby_data->extended_standby_data.pll_factor[index].p;
		standby_rate =  do_div(dividend, divisor);

		dividend = temp_standby_data.pll_factor[index].n;
		divisor = temp_standby_data.pll_factor[index].m * temp_standby_data.pll_factor[index].p;
		temp_standby_rata = do_div(dividend, divisor);
		if (standby_rate > temp_standby_rata)
			return true;
		else
			return false;
	case 0: /* PLL1 */
	case 4: /* PLL5 */
	case 8: /* MIPI */
		dividend = standby_data->extended_standby_data.pll_factor[index].n * standby_data->extended_standby_data.pll_factor[index].k;
		divisor = standby_data->extended_standby_data.pll_factor[index].m;
		standby_rate = do_div(dividend, divisor);

		dividend = temp_standby_data.pll_factor[index].n * temp_standby_data.pll_factor[index].k;
		divisor = temp_standby_data.pll_factor[index].m;
		temp_standby_rata = do_div(dividend, divisor);
		if (standby_rate > temp_standby_rata)
			return true;
		else
			return false;
	case 5: /* PLL6 */
		dividend = standby_data->extended_standby_data.pll_factor[index].n * standby_data->extended_standby_data.pll_factor[index].k;
		divisor = 2;
		standby_rate = do_div(dividend, divisor);

		dividend = temp_standby_data.pll_factor[index].n * temp_standby_data.pll_factor[index].k;
		divisor = 2;
		temp_standby_rata = do_div(dividend, divisor);
		if (standby_rate > temp_standby_rata)
			return true;
		else
			return false;
	case 2: /* PLL3 */
	case 3: /* PLL4 */
	case 7: /* PLL8 */
	case 9: /* PLL9 */
	case 10: /* PLL10 */
		dividend = standby_data->extended_standby_data.pll_factor[index].n;
		divisor = standby_data->extended_standby_data.pll_factor[index].m;
		standby_rate = do_div(dividend, divisor);

		dividend = temp_standby_data.pll_factor[index].n;
		divisor = temp_standby_data.pll_factor[index].m;
		temp_standby_rata = do_div(dividend, divisor);
		if (standby_rate > temp_standby_rata)
			return true;
		else
			return false;
	default:
		return true;
	}
}

static bool calculate_bus(int index, scene_extended_standby_t *standby_data)
{
	switch (index) {
	case 0: /* AXI */
		if(standby_data->extended_standby_data.bus_factor[index].src > temp_standby_data.bus_factor[index].src)
			return true;
		else
			return false;
	case 1: /* AHB1/APB1 */
	case 2: /* APB2 */
		if(standby_data->extended_standby_data.bus_factor[index].src > temp_standby_data.bus_factor[index].src)
			return true;
		else
			return false;
		break;
	default:
		return true;
	}
}

static int copy_extended_standby_data(scene_extended_standby_t *standby_data)
{
	int i = 0;

	if (!standby_data) {
		temp_standby_data.id = 0;
		temp_standby_data.pwr_dm_en = 0;
		temp_standby_data.osc_en = 0;
		temp_standby_data.init_pll_dis = 0;
		temp_standby_data.exit_pll_en = 0;
		temp_standby_data.pll_change = 0;
		temp_standby_data.bus_change = 0;
		memset(&temp_standby_data.pll_factor, 0, sizeof(temp_standby_data.pll_factor));
		memset(&temp_standby_data.bus_factor, 0, sizeof(temp_standby_data.bus_factor));
	} else {
		if ((0 != temp_standby_data.id) && (!((standby_data->extended_standby_data.id) & (temp_standby_data.id)))) {
			temp_standby_data.id |= standby_data->extended_standby_data.id;
			temp_standby_data.pwr_dm_en |= standby_data->extended_standby_data.pwr_dm_en;
			temp_standby_data.osc_en |= standby_data->extended_standby_data.osc_en;
			temp_standby_data.init_pll_dis &= standby_data->extended_standby_data.init_pll_dis;
			temp_standby_data.exit_pll_en |= standby_data->extended_standby_data.exit_pll_en;
			if (0 != standby_data->extended_standby_data.pll_change) {
				for (i=0; i<PLL_NUM; i++) {
					if (standby_data->extended_standby_data.pll_change & (0x1<<i)) {
						if (!(temp_standby_data.pll_change & (0x1<<i)))
							temp_standby_data.pll_factor[i] = standby_data->extended_standby_data.pll_factor[i];
						else if(calculate_pll(i, standby_data))
							temp_standby_data.pll_factor[i] = standby_data->extended_standby_data.pll_factor[i];
					}
				}
				temp_standby_data.pll_change |= standby_data->extended_standby_data.pll_change;
			}
			if (0 != standby_data->extended_standby_data.bus_change) {
				for (i=0; i<BUS_NUM; i++) {
					if (standby_data->extended_standby_data.bus_change & (0x1<<i)) {
						if (!(temp_standby_data.bus_change & (0x1<<i)))
							temp_standby_data.bus_factor[i] = standby_data->extended_standby_data.bus_factor[i];
						else if(calculate_bus(i, standby_data))
							temp_standby_data.bus_factor[i] = standby_data->extended_standby_data.bus_factor[i];
					}
				}
				temp_standby_data.bus_change |= standby_data->extended_standby_data.bus_change;
			}
		} else if ((0 == temp_standby_data.id)) {
			temp_standby_data.id = standby_data->extended_standby_data.id;
			temp_standby_data.pwr_dm_en = standby_data->extended_standby_data.pwr_dm_en;
			temp_standby_data.osc_en = standby_data->extended_standby_data.osc_en;
			temp_standby_data.init_pll_dis = standby_data->extended_standby_data.init_pll_dis;
			temp_standby_data.exit_pll_en = standby_data->extended_standby_data.exit_pll_en;
			temp_standby_data.pll_change = standby_data->extended_standby_data.pll_change;
			if (0 != standby_data->extended_standby_data.pll_change) {
				for (i=0; i<PLL_NUM; i++) {
					temp_standby_data.pll_factor[i] = standby_data->extended_standby_data.pll_factor[i];
				}
			} else
				memset(&temp_standby_data.pll_factor, 0, sizeof(temp_standby_data.pll_factor));

			temp_standby_data.bus_change = standby_data->extended_standby_data.bus_change;
			if (0 != standby_data->extended_standby_data.bus_change) {
				for (i=0; i<BUS_NUM; i++) {
					temp_standby_data.bus_factor[i] = standby_data->extended_standby_data.bus_factor[i];
				}
			} else
				memset(&temp_standby_data.bus_factor, 0, sizeof(temp_standby_data.bus_factor));
		}
	}

	return 0;
}

/**
 *	get_extended_standby_manager - get the extended_standby_manager pointer
 *
 *	Return	: if the extended_standby_manager is effective, return the extended_standby_manager pointer;
 *		  else return NULL;
 *	Notes	: you can check the configuration from the pointer.
 */
const extended_standby_manager_t *get_extended_standby_manager(void)
{
	unsigned long irqflags;
	extended_standby_manager_t *manager_data = NULL;

	spin_lock_irqsave(&data_lock, irqflags);
	manager_data = &extended_standby_manager;
	spin_unlock_irqrestore(&data_lock, irqflags);
	if ((NULL != manager_data) && (NULL != manager_data->pextended_standby))
		EXSTANDBY_DBG("leave %s : id 0x%lx\n", __func__, manager_data->pextended_standby->id);

	return manager_data;
}

/**
 *	set_extended_standby_manager - set the extended_standby_manager;
 *	manager@: the manager config.
 *
 *      return value: if the setting is correct, return true.
 *		      else return false;
 *      notes: the function will check the struct member: pextended_standby and event.
 *		if the setting is not proper, return false.
 */
bool set_extended_standby_manager(scene_extended_standby_t *local_standby)
{
	unsigned long irqflags;

	EXSTANDBY_DBG("enter %s\n", __func__);

	if (local_standby && 0 == local_standby->extended_standby_data.pwr_dm_en) {
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
		EXSTANDBY_DBG("leave %s : id 0x%lx\n", __func__, extended_standby_manager.pextended_standby->id);

	return true;
}

/**
 *	extended_standby_enable_wakeup_src   - 	enable the wakeup src.
 *
 *	function:		the device driver care about the wakeup src.
 *				if the device driver do want the system be wakenup while in standby state.
 *				the device driver should use this function to enable corresponding intterupt.
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src,
 *				else ignored.
 *	notice:			1. for gpio intterupt, only access the enable bit, mean u need care about other config,
 *				such as: int mode, pull up or pull down resistance, etc.
 *				2. At a31, only gpio pa, pb, pe, pg, pl, pm int wakeup src is supported.
*/
int extended_standby_enable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;

	spin_lock_irqsave(&data_lock, irqflags);
	extended_standby_manager.event |= src;
	if (CPUS_GPIO_SRC & src) {
		if ( para >= AXP_PIN_BASE) {
			extended_standby_manager.wakeup_gpio_map |= (WAKEUP_GPIO_AXP((para - AXP_PIN_BASE)));
		} else if ( para >= SUNXI_PM_BASE) {
			extended_standby_manager.wakeup_gpio_map |= (WAKEUP_GPIO_PM((para - SUNXI_PM_BASE)));
		} else if ( para >= SUNXI_PL_BASE) {
			extended_standby_manager.wakeup_gpio_map |= (WAKEUP_GPIO_PL((para - SUNXI_PL_BASE)));
		} else if ( para >= SUNXI_PH_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('H'));
		} else if ( para >= SUNXI_PG_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('G'));
		} else if ( para >= SUNXI_PF_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('F'));
		} else if ( para >= SUNXI_PE_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('E'));
		} else if ( para >= SUNXI_PD_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('D'));
		} else if ( para >= SUNXI_PC_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('C'));
		} else if ( para >= SUNXI_PB_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('B'));
		} else if ( para >= SUNXI_PA_BASE) {
			extended_standby_manager.wakeup_gpio_group |= (WAKEUP_GPIO_GROUP('A'));
		} else {
			pr_info("cpux need care gpio %d. but, notice, currently, \
				cpux not support it.\n", para);
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);
	EXSTANDBY_DBG("leave %s : event 0x%lx\n", __func__, extended_standby_manager.event);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_map 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_map);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_group 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_group);

	return 0;
}

/**
 *	extended_standby_disable_wakeup_src  - 	disable the wakeup src.
 *
 *	function:		if the device driver do not want the system be wakenup while in standby state again.
 *				the device driver should use this function to disable the corresponding intterupt.
 *
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src,
 *				else ignored.
 *	notice:			for gpio intterupt, only access the enable bit, mean u need care about other config,
 *				such as: int mode, pull up or pull down resistance, etc.
 */
int extended_standby_disable_wakeup_src(cpu_wakeup_src_e src, int para)
{
	unsigned long irqflags;

	spin_lock_irqsave(&data_lock, irqflags);
	extended_standby_manager.event &= (~src);
	if (CPUS_GPIO_SRC & src) {
		if ( para >= AXP_PIN_BASE) {
			extended_standby_manager.wakeup_gpio_map &= (~(WAKEUP_GPIO_AXP((para - AXP_PIN_BASE))));
		}else if ( para >= SUNXI_PM_BASE) {
			extended_standby_manager.wakeup_gpio_map &= (~(WAKEUP_GPIO_PM((para - SUNXI_PM_BASE))));
		}else if ( para >= SUNXI_PL_BASE) {
			extended_standby_manager.wakeup_gpio_map &= (~(WAKEUP_GPIO_PL((para - SUNXI_PL_BASE))));
		}else if ( para >= SUNXI_PH_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('H')));
		}else if ( para >= SUNXI_PG_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('G')));
		}else if ( para >= SUNXI_PF_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('F')));
		}else if ( para >= SUNXI_PE_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('E')));
		}else if ( para >= SUNXI_PD_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('D')));
		}else if ( para >= SUNXI_PC_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('C')));
		}else if ( para >= SUNXI_PB_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('B')));
		}else if ( para >= SUNXI_PA_BASE) {
			extended_standby_manager.wakeup_gpio_group &= (~(WAKEUP_GPIO_GROUP('A')));
		}else {
			pr_info("cpux need care gpio %d. but, notice, currently, \
				cpux not support it.\n", para);
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);
	EXSTANDBY_DBG("leave %s : event 0x%lx\n", __func__, extended_standby_manager.event);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_map 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_map);
	EXSTANDBY_DBG("leave %s : wakeup_gpio_group 0x%lx\n", __func__, extended_standby_manager.wakeup_gpio_group);

	return 0;
}

/**
 *	extended_standby_check_wakeup_state   -   to get the corresponding wakeup src intterupt state, enable or disable.
 *
 *	@src:			wakeup src.
 *	@para:			if wakeup src need para, be the para of wakeup src,
 *				else ignored.
 *
 *	return value:		enable, 	return 1,
 *				disable,	return 2,
 *				error: 		return -1.
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
 *	extended_standby_show_state  - 	show current standby state, for debug purpose.
 *
 *	function:		standby state including locked_scene, power_supply dependancy, the wakeup src.
 *
 *	return value:		succeed, return 0, else return -1.
 */
int extended_standby_show_state(void)
{
	unsigned long irqflags;
	int i;

	standby_show_state();

	spin_lock_irqsave(&data_lock, irqflags);
	printk("wakeup_src 0x%lx\n", extended_standby_manager.event);
	printk("wakeup_gpio_map 0x%lx\n", extended_standby_manager.wakeup_gpio_map);
	printk("wakeup_gpio_group 0x%lx\n", extended_standby_manager.wakeup_gpio_group);
	if (NULL != extended_standby_manager.pextended_standby) {
		printk("extended_standby id = 0x%lx\n", extended_standby_manager.pextended_standby->id);
		if (0 != extended_standby_manager.pextended_standby->pll_change) {
			for (i=0; i<PLL_NUM; i++) {
				EXSTANDBY_DBG("pll%i: n=%d k=%d m=%d p=%d\n", i, \
				              extended_standby_manager.pextended_standby->pll_factor[i].n, \
				              extended_standby_manager.pextended_standby->pll_factor[i].k, \
				              extended_standby_manager.pextended_standby->pll_factor[i].m, \
				              extended_standby_manager.pextended_standby->pll_factor[i].p);
			}
		}
		if (0 != extended_standby_manager.pextended_standby->bus_change) {
			for (i=0; i<BUS_NUM; i++) {
				EXSTANDBY_DBG("bus%i: src=%d pre_div=%d div_ratio=%d n=%d m=%d\n", i, \
				              extended_standby_manager.pextended_standby->bus_factor[i].src, \
				              extended_standby_manager.pextended_standby->bus_factor[i].pre_div, \
				              extended_standby_manager.pextended_standby->bus_factor[i].div_ratio, \
				              extended_standby_manager.pextended_standby->bus_factor[i].n, \
				              extended_standby_manager.pextended_standby->bus_factor[i].m);
			}
		}
	}
	spin_unlock_irqrestore(&data_lock, irqflags);

	return 0;
}


unsigned int parse_bitmap_en = 0x0;

unsigned int show_gpio_config(char *s, unsigned int size)
{
	char *start = s;
	char *end = NULL;

	if (NULL == s || 0 == size) {
		/* buffer is empty */
		s = NULL;
	} else {
		end = s + size;
	}

	uk_printf(s, end - s, "\t SUNXI_BANK_SIZE bit 0x%x \n", SUNXI_BANK_SIZE);
	uk_printf(s, end - s, "\t SUNXI_PA_BASE   bit 0x%x \n", SUNXI_PA_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PB_BASE   bit 0x%x \n", SUNXI_PB_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PC_BASE   bit 0x%x \n", SUNXI_PC_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PD_BASE   bit 0x%x \n", SUNXI_PD_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PE_BASE   bit 0x%x \n", SUNXI_PE_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PF_BASE   bit 0x%x \n", SUNXI_PF_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PG_BASE   bit 0x%x \n", SUNXI_PG_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PH_BASE   bit 0x%x \n", SUNXI_PH_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PI_BASE   bit 0x%x \n", SUNXI_PI_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PJ_BASE   bit 0x%x \n", SUNXI_PJ_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PK_BASE   bit 0x%x \n", SUNXI_PK_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PL_BASE   bit 0x%x \n", SUNXI_PL_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PM_BASE   bit 0x%x \n", SUNXI_PM_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PN_BASE   bit 0x%x \n", SUNXI_PN_BASE  );
	uk_printf(s, end - s, "\t SUNXI_PO_BASE   bit 0x%x \n", SUNXI_PO_BASE  );
	uk_printf(s, end - s, "\t AXP_PIN_BASE    bit 0x%x \n", AXP_PIN_BASE   );

	return (s - start);

}

static unsigned int parse_bitmap(char *s, unsigned int size, unsigned int bitmap)
{
	char *start = s;
	char *end = NULL;

	if (NULL == s || 0 == size){
		/* buffer is empty */
		if(!(parse_bitmap_en & DEBUG_WAKEUP_GPIO_MAP))
			return 0;
		s = NULL;
	} else {
		end = s + size;
	}

	switch(bitmap){
	case 1<<0  : uk_printf(s, end - s, "\t\tport 0. \n"); break;
	case 1<<1  : uk_printf(s, end - s, "\t\tport 1. \n"); break;
	case 1<<2  : uk_printf(s, end - s, "\t\tport 2. \n"); break;
	case 1<<3  : uk_printf(s, end - s, "\t\tport 3. \n"); break;
	case 1<<4  : uk_printf(s, end - s, "\t\tport 4. \n"); break;
	case 1<<5  : uk_printf(s, end - s, "\t\tport 5. \n"); break;
	case 1<<6  : uk_printf(s, end - s, "\t\tport 6. \n"); break;
	case 1<<7  : uk_printf(s, end - s, "\t\tport 7. \n"); break;
	case 1<<8  : uk_printf(s, end - s, "\t\tport 8. \n"); break;
	case 1<<9  : uk_printf(s, end - s, "\t\tport 9. \n"); break;
	case 1<<10 : uk_printf(s, end - s, "\t\tport 10. \n"); break;
	case 1<<11 : uk_printf(s, end - s, "\t\tport 11. \n"); break;
	default:						    break;
    }

	return (s - start);
}

static unsigned int parse_group_bitmap(char *s, unsigned int size, unsigned int group_bitmap)
{
	char *start = s;
	char *end = NULL;

	if(NULL == s || 0 == size){
		/* buffer is empty */
		if (!(parse_bitmap_en & DEBUG_WAKEUP_GPIO_GROUP_MAP))
			return 0;
		s = NULL;
	} else {
		end = s + size;
    }

	switch(group_bitmap){
	case 1<<0  : uk_printf(s, end - s, "\t\tgroup 'A'. \n"); break;
	case 1<<1  : uk_printf(s, end - s, "\t\tgroup 'B'. \n"); break;
	case 1<<2  : uk_printf(s, end - s, "\t\tgroup 'C'. \n"); break;
	case 1<<3  : uk_printf(s, end - s, "\t\tgroup 'D'. \n"); break;
	case 1<<4  : uk_printf(s, end - s, "\t\tgroup 'E'. \n"); break;
	case 1<<5  : uk_printf(s, end - s, "\t\tgroup 'F'. \n"); break;
	case 1<<6  : uk_printf(s, end - s, "\t\tgroup 'G'. \n"); break;
	case 1<<7  : uk_printf(s, end - s, "\t\tgroup 'H'. \n"); break;
	case 1<<8  : uk_printf(s, end - s, "\t\tgroup 'I'. \n"); break;
	case 1<<9  : uk_printf(s, end - s, "\t\tgroup 'J'. \n"); break;
	case 1<<10 : uk_printf(s, end - s, "\t\tgroup 'K'. \n"); break;
	case 1<<11 : uk_printf(s, end - s, "\t\tgroup 'L'. \n"); break;
	default:						    break;
	}

	return (s - start);
}

unsigned int parse_wakeup_event(char *s, unsigned int size, unsigned int event, event_cpu_id_e cpu_id)
{
	int i = 0;
	int count = 0;
	int counted = 0;
	unsigned int bit_event = 0;
	char *start = s;
	char *end = NULL;

	if (NULL == s || 0 == size) {
		/* buffer is empty */
		if (!(parse_bitmap_en & DEBUG_WAKEUP_SRC)) {
			return 0;
		}
		s = NULL;
	} else {
		end = s + size;
	}

	uk_printf(s, end - s, "WAKEUP_SRC is as follow: \n");

	for(i=0; i<32; i++){
		bit_event = (1<<i & event);
		switch(bit_event){
		case 0                      : break;
		case CPU0_WAKEUP_MSGBOX     : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPU0_WAKEUP_MSGBOX   ", CPU0_WAKEUP_MSGBOX   ); count++;    break;
		case CPU0_WAKEUP_KEY        : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPU0_WAKEUP_KEY      ", CPU0_WAKEUP_KEY      ); count++;    break;
		case CPUS_WAKEUP_LOWBATT    : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_LOWBATT  ", CPUS_WAKEUP_LOWBATT  ); count++;    break;
		case CPUS_WAKEUP_USB        : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_USB      ", CPUS_WAKEUP_USB      ); count++;    break;
		case CPUS_WAKEUP_AC         : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_AC       ", CPUS_WAKEUP_AC       ); count++;    break;
		case CPUS_WAKEUP_ASCEND     : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_ASCEND   ", CPUS_WAKEUP_ASCEND   ); count++;    break;
		case CPUS_WAKEUP_DESCEND    : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_DESCEND  ", CPUS_WAKEUP_DESCEND  ); count++;    break;
		case CPUS_WAKEUP_SHORT_KEY  : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_SHORT_KEY", CPUS_WAKEUP_SHORT_KEY); count++;    break;
		case CPUS_WAKEUP_LONG_KEY   : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_LONG_KEY ", CPUS_WAKEUP_LONG_KEY ); count++;    break;
		case CPUS_WAKEUP_IR         : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_IR       ", CPUS_WAKEUP_IR       ); count++;    break;
		case CPUS_WAKEUP_ALM0       : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_ALM0     ", CPUS_WAKEUP_ALM0     ); count++;    break;
		case CPUS_WAKEUP_ALM1       : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_ALM1     ", CPUS_WAKEUP_ALM1     ); count++;    break;
		case CPUS_WAKEUP_TIMEOUT    : uk_printf(s, end - s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_TIMEOUT  ", CPUS_WAKEUP_TIMEOUT  ); count++;    break;
		case CPUS_WAKEUP_GPIO       : uk_printf(s, end - s, "\n%-36s bit 0x%x \t ", "CPUS_WAKEUP_GPIO	", CPUS_WAKEUP_GPIO     );
		                              uk_printf(s, end - s, "\n\twant to know gpio config & suspended status detail? \n\t\tcat /sys/power/aw_pm/debug_mask for help.\n");
		                              count++;
		                              break;
		case CPUS_WAKEUP_USBMOUSE   : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_USBMOUSE ", CPUS_WAKEUP_USBMOUSE ); count++;    break;
		case CPUS_WAKEUP_LRADC      : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_LRADC    ", CPUS_WAKEUP_LRADC    ); count++;    break;
		case CPUS_WAKEUP_CODEC      : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_CODEC    ", CPUS_WAKEUP_CODEC    ); count++;    break;
		case CPUS_WAKEUP_BAT_TEMP   : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_BAT_TEMP ", CPUS_WAKEUP_BAT_TEMP ); count++;    break;
		case CPUS_WAKEUP_FULLBATT   : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_FULLBATT ", CPUS_WAKEUP_FULLBATT ); count++;    break;
		case CPUS_WAKEUP_HMIC       : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_HMIC     ", CPUS_WAKEUP_HMIC     ); count++;    break;
		case CPUS_WAKEUP_POWER_EXP  : uk_printf(s, end -s, "%-36s bit 0x%x \t ", "CPUS_WAKEUP_POWER_EXP", CPUS_WAKEUP_POWER_EXP); count++;    break;
		default:                      break;
		}
		if(counted != count && 0 == count%2){
			counted = count;
			uk_printf(s, end-s, "\n");
		}
	}

	uk_printf(s, end-s, "\n");

	return (s - start);
}

unsigned int parse_wakeup_gpio_map(char *s, unsigned int size, unsigned int gpio_map)
{
	int i = 0;
	unsigned int bit_event = 0;
	char *start = s;
	char *end = NULL;

	if(NULL == s || 0 == size){
		/* buffer is empty */
		if(!(parse_bitmap_en & DEBUG_WAKEUP_GPIO_MAP))
			return 0;
		s = NULL;
	} else {
		end = s + size;
	}

	uk_printf(s, end - s, "%s", "WAKEUP_GPIO,for cpus:pl,pm, and axp, is as follow: \n");

	for (i = 0; i < 32; i++) {
		bit_event = (1<<i & gpio_map);
		if (0 != bit_event) {
			if (bit_event <= WAKEUP_GPIO_PL(GPIO_PL_MAX_NUM)) {
				uk_printf(s, end - s, "\tWAKEUP_GPIO_PL	");
				s += parse_bitmap(s, end - s, bit_event);
			} else if (bit_event <= WAKEUP_GPIO_PM(GPIO_PM_MAX_NUM)) {
				uk_printf(s, end - s, "\tWAKEUP_GPIO_PM	");
				s += parse_bitmap(s, end - s, bit_event>>(GPIO_PL_MAX_NUM + 1));
			} else if (bit_event <= WAKEUP_GPIO_AXP(GPIO_AXP_MAX_NUM)) {
				uk_printf(s, end - s, "\tWAKEUP_GPIO_AXP	");
				s += parse_bitmap(s, end - s, bit_event>>(GPIO_PL_MAX_NUM + 1 + GPIO_PM_MAX_NUM + 1));
			} else {
				uk_printf(s, end - s, "parse err.\n");
			}
		}
	}

	return (s - start);
}

unsigned int parse_wakeup_gpio_group_map(char *s, unsigned int size, unsigned int group_map)
{
	int i = 0;
	unsigned int bit_event = 0;
	char *start = s;
	char *end = NULL;

	if (NULL == s || 0 == size) {
		/* buffer is empty */
		if(!(parse_bitmap_en & DEBUG_WAKEUP_GPIO_GROUP_MAP))
			return 0;
		s = NULL;
	} else {
		end = s + size;
	}
	uk_printf(s, end - s, "WAKEUP_GPIO,for cpux:pa,pb,pc,pd,.., is as follow: \n");
	for (i = 0; i < 32; i++) {
		bit_event = (1<<i & group_map);
		if (0 != bit_event) {
			uk_printf(s, end - s, "\tWAKEUP_GPIO_GROUP: ");
			s += parse_group_bitmap(s, end - s, bit_event);
		}
	}

	return (s - start);
}

module_param_named(parse_bitmap_en, parse_bitmap_en, uint, S_IRUGO | S_IWUSR);
