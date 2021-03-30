/*
 * File    : pm-sun8iw10.c
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm_o.h"
standby_space_cfg_t standby_space;

__u32 debug_mask = PM_STANDBY_TEST;	/* | PM_STANDBY_PRINT_STANDBY | PM_STANDBY_PRINT_RESUME| PM_STANDBY_ENABLE_JTAG; */
int suspend_freq = SUSPEND_FREQ;
int suspend_delay_ms = SUSPEND_DELAY_MS;
unsigned long time_to_wakeup;

#ifdef GET_CYCLE_CNT
static int start;
static int resume0_period;
static int resume1_period;

static int pm_start;
static int invalidate_data_time;
static int invalidate_instruct_time;
static int before_restore_processor;
static int after_restore_process;
/*static int restore_runtime_peroid;*/

/*late_resume timing*/
static int late_resume_start;
static int backup_area_start;
static int backup_area1_start;
static int backup_area2_start;
static int clk_restore_start;
static int gpio_restore_start;
static int twi_restore_start;
static int int_restore_start;
static int tmr_restore_start;
static int sram_restore_start;
static int late_resume_end;
#endif

struct aw_mem_para mem_para_info;
struct super_standby_para super_standby_para_info;
const extended_standby_manager_t *extended_standby_manager_id;

standby_type_e standby_type = NON_STANDBY;
EXPORT_SYMBOL(standby_type);
standby_level_e standby_level = STANDBY_INITIAL;
EXPORT_SYMBOL(standby_level);

/*static volatile int enter_flag;*/
int standby_mode;

__mem_tmr_reg_t saved_tmr_state;

struct aw_pm_info standby_info = {
	.standby_para = {
			 .event = CPU0_MEM_WAKEUP,
			 .axp_event = CPUS_MEM_WAKEUP,
			 .timeout = 0,
			 },
	.pmu_arg = {
		    .twi_port = 0,
		    .dev_addr = 10,
		    },
};

void aw_pm_dump_regs(const char *str, u32 *bottom, u32 *top)
{
	u32 *first;
	int i;

	pr_info("%s reg state: (0x%p to 0x%p)", str, bottom, top);

	for (first = bottom; first < top; first += 4) {
		u32 *p;
		char str[sizeof(" 0x12345678") * 4 + 1];

		memset(str, ' ', sizeof(str));
		str[sizeof(str) - 1] = '\0';

		for (p = first, i = 0; i < 4 && p < top; i++, p++) {
			if (p >= bottom && p < top) {
				sprintf(str + i * 11, " 0x%08x",
						*(volatile u32 *)p);
			}
		}
		pr_info("0x%p:%s\n", first, str);
	}
}

void aw_pm_dev_status(char *name, int mask)
{
	u32 *base = 0;
	u32 len = 0;

	if (unlikely(debug_mask & mask)) {
		pm_get_dev_info(name, 0, &base, &len);
		len /= sizeof(u32);
		aw_pm_dump_regs(name, base, base + len);

		if (!strncmp(name, "pio", 3))
			sunxi_pinctrl_state_show();
	}

	return;
}

void aw_pm_show_dev_status(void)
{
	aw_pm_dev_status("pio", PM_STANDBY_PRINT_IO_STATUS);
	aw_pm_dev_status("r_pio", PM_STANDBY_PRINT_CPUS_IO_STATUS);
	aw_pm_dev_status("clocks", PM_STANDBY_PRINT_CCU_STATUS);

	return;
}

static char *parse_debug_mask(unsigned int bitmap, char *s, char *end)
{
	int i = 0;
	int counted = 0;
	int count = 0;
	unsigned int bit_event = 0;

	for (i = 0; i < 32; i++) {
		bit_event = (1 << i & bitmap);
		switch (bit_event) {
		case 0:
			break;
		case PM_STANDBY_PRINT_STANDBY:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_STANDBY         ",
				       PM_STANDBY_PRINT_STANDBY);
			count++;
			break;
		case PM_STANDBY_PRINT_RESUME:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_RESUME          ",
				       PM_STANDBY_PRINT_RESUME);
			count++;
			break;
		case PM_STANDBY_ENABLE_JTAG:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_ENABLE_JTAG           ",
				       PM_STANDBY_ENABLE_JTAG);
			count++;
			break;
		case PM_STANDBY_PRINT_PORT:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_PORT            ",
				       PM_STANDBY_PRINT_PORT);
			count++;
			break;
		case PM_STANDBY_PRINT_IO_STATUS:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_IO_STATUS       ",
				       PM_STANDBY_PRINT_IO_STATUS);
			count++;
			break;
		case PM_STANDBY_PRINT_CACHE_TLB_MISS:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_CACHE_TLB_MISS  ",
				       PM_STANDBY_PRINT_CACHE_TLB_MISS);
			count++;
			break;
		case PM_STANDBY_PRINT_CCU_STATUS:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_CCU_STATUS      ",
				       PM_STANDBY_PRINT_CCU_STATUS);
			count++;
			break;
		case PM_STANDBY_PRINT_PWR_STATUS:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_PWR_STATUS      ",
				       PM_STANDBY_PRINT_PWR_STATUS);
			count++;
			break;
		case PM_STANDBY_PRINT_CPUS_IO_STATUS:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_CPUS_IO_STATUS  ",
				       PM_STANDBY_PRINT_CPUS_IO_STATUS);
			count++;
			break;
		case PM_STANDBY_PRINT_CCI400_REG:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_CCI400_REG      ",
				       PM_STANDBY_PRINT_CCI400_REG);
			count++;
			break;
		case PM_STANDBY_PRINT_GTBUS_REG:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_GTBUS_REG       ",
				       PM_STANDBY_PRINT_GTBUS_REG);
			count++;
			break;
		case PM_STANDBY_TEST:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_TEST                  ",
				       PM_STANDBY_TEST);
			count++;
			break;
		case PM_STANDBY_PRINT_RESUME_IO_STATUS:
			s += scnprintf(s, end - s, "%-34s bit 0x%x\t",
				       "PM_STANDBY_PRINT_RESUME_IO_STATUS",
				       PM_STANDBY_PRINT_RESUME_IO_STATUS);
			count++;
			break;
		default:
			break;

		}
		if (counted != count && 0 == count % 2) {
			counted = count;
			s += scnprintf(s, end - s, "\n");
		}
	}

	s += scnprintf(s, end - s, "\n");

	return s;
}

ssize_t debug_mask_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;

	s += sprintf(buf, "0x%x\n", debug_mask);
	s = parse_debug_mask(debug_mask, s, end);

	s += sprintf(s, "%s\n", "debug_mask usage help info:");
	s += sprintf(s, "%s\n",
		     "target: for enable checking the io, ccu... suspended status.");
	s += sprintf(s, "%s\n",
		     "bitmap: each bit corresponding one module, as follow:");
	s = parse_debug_mask(0xffff, s, end);

	return s - buf;
}

#define TMPBUFLEN 22
static int get_long(const char **buf, size_t *size, unsigned long *val,
		    bool *neg)
{
	size_t len;
	char *p, tmp[TMPBUFLEN];

	if (!*size)
		return -EINVAL;

	len = *size;
	if (len > TMPBUFLEN - 1)
		len = TMPBUFLEN - 1;

	memcpy(tmp, *buf, len);

	tmp[len] = 0;
	p = tmp;
	if (*p == '-' && *size > 1) {
		*neg = true;
		p++;
	} else
		*neg = false;
	if (!isdigit(*p))
		return -EINVAL;

	*val = simple_strtoul(p, &p, 0);

	len = p - tmp;

	/* We don't know if the next char is whitespace thus we may accept
	 * invalid integers (e.g. 1234...a) or two integers instead of one
	 * (e.g. 123...1). So lets not allow such large numbers. */
	if (len == TMPBUFLEN - 1)
		return -EINVAL;

	return 0;
}

ssize_t debug_mask_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long data = 0;
	bool neg = false;

	if (!get_long(&buf, &count, &data, &neg)) {
		if (true != neg) {
			debug_mask = (unsigned int)data;
		} else {
			printk("%s\n", "minus is Illegal. ");
			return -EINVAL;
		}
	} else {
		printk("%s\n", "non-digital is Illegal. ");
		return -EINVAL;
	}

	return count;
}

ssize_t parse_status_code_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	unsigned int status_code = 0;
	unsigned int index = 0;

	sscanf(buf, "%x %x\n", &status_code, &index);
	parse_status_code(status_code, index);
	return size;
}

ssize_t parse_status_code_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	char *s = buf;

	show_mem_status();
	return s - buf;
}

#if (defined(CONFIG_ARCH_SUN8IW8P1) || \
	defined(CONFIG_ARCH_SUN8IW6P1)  || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1))
void init_wakeup_src(unsigned int event, unsigned int gpio_enable_bitmap, unsigned int cpux_gpiog_bitmap)
{
	/*config int src. */
	mem_int_save();
	mem_tmr_init();
	mem_tmr_save(&(saved_tmr_state));
	if (event & CPU0_WAKEUP_TIMEOUT) {
		printk("enable CPUS_WAKEUP_TIMEOUT. \n");
		/* set timer for power off */
		if (super_standby_para_info.timeout) {
			mem_tmr_set(super_standby_para_info.timeout);
			mem_enable_int(INT_SOURCE_TIMER1);
		}
	}
	return;
}

void exit_wakeup_src(unsigned int event, unsigned int gpio_enable_bitmap, unsigned int cpux_gpiog_bitmap)
{
	mem_tmr_restore(&(saved_tmr_state));
	mem_tmr_exit();
	mem_int_restore();
	return;
}
#endif

#if (defined(CONFIG_ARCH_SUN8IW8P1) || \
	defined(CONFIG_ARCH_SUN8IW6P1) || \
	defined(CONFIG_ARCH_SUN8IW10P1) || \
	defined(CONFIG_ARCH_SUN8IW11P1) || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)) && \
	defined(CONFIG_AW_AXP)
static unsigned int pwr_dm_mask_saved;
static int save_sys_pwr_state(const char *id)
{
	int bitmap = 0;

	bitmap = is_sys_pwr_dm_id(id);
	if ((-1) != bitmap) {
		pwr_dm_mask_saved |= (0x1 << bitmap);
	} else {
		pm_printk(PM_STANDBY_PRINT_PWR_STATUS,
			  "%s: is not sys\n", id);
	}
	return 0;

}

int resume_sys_pwr_state(void)
{
	int i = 0, ret = -1;
	char *sys_name = NULL;

	for (i = 0; i < 32; i++) {
		if (pwr_dm_mask_saved & (0x1 << i)) {
			sys_name = get_sys_pwr_dm_id(i);
			if (NULL != sys_name) {
				ret = add_sys_pwr_dm(sys_name);
				if (ret < 0) {
					pm_printk(PM_STANDBY_PRINT_PWR_STATUS,
					     "%s: resume failed\n", sys_name);
				}
			}
		}
	}
	pwr_dm_mask_saved = 0;
	return 0;
}

static int check_sys_pwr_dm_status(char *pwr_dm)
{
	char ldo_name[20] = "\0";
	char enable_id[20] = "\0";
	int ret = 0;
	int i = 0;

	ret = get_ldo_name(pwr_dm, ldo_name);
	if (ret < 0) {
		pm_printk(PM_STANDBY_PRINT_PWR_STATUS,
				"	    %s: get %s failed. ret = %d\n",
				__func__, pwr_dm, ret);
		return -1;
	}
	ret = get_enable_id_count(ldo_name);
	if (0 == ret) {
		pm_printk(PM_STANDBY_PRINT_PWR_STATUS,
			  "%s: no child, use by %s, property: sys.\n",
			  ldo_name, pwr_dm);
	} else {
		for (i = 0; i < ret; i++) {
			get_enable_id(ldo_name, i, (char *)enable_id);
			printk(KERN_INFO "%s: active child id %d is: %s. ",
			       ldo_name, i, enable_id);
			/*need to check all enabled id is belong to sys_pwr_dm. */
			if ((-1) != is_sys_pwr_dm_id(enable_id)) {
				pm_printk(PM_STANDBY_PRINT_PWR_STATUS,
					  "property: sys\n");
			} else {
				pm_printk(PM_STANDBY_PRINT_PWR_STATUS,
					  "property: module\n");
				del_sys_pwr_dm(pwr_dm);
				save_sys_pwr_state(pwr_dm);
				break;
			}
		}
	}
	return 0;

}

int check_pwr_status(void)
{
	check_sys_pwr_dm_status("vdd-cpua");
	check_sys_pwr_dm_status("vdd-cpub");
	check_sys_pwr_dm_status("vcc-dram");
	check_sys_pwr_dm_status("vdd-gpu");
	check_sys_pwr_dm_status("vdd-sys");
	check_sys_pwr_dm_status("vdd-vpu");
	check_sys_pwr_dm_status("vdd-cpus");
	check_sys_pwr_dm_status("vdd-drampll");
	check_sys_pwr_dm_status("vcc-lpddr");
	check_sys_pwr_dm_status("vcc-adc");
	check_sys_pwr_dm_status("vcc-pl");
	check_sys_pwr_dm_status("vcc-pm");
	check_sys_pwr_dm_status("vcc-io");
	check_sys_pwr_dm_status("vcc-cpvdd");
	check_sys_pwr_dm_status("vcc-ldoin");
	check_sys_pwr_dm_status("vcc-pll");
	check_sys_pwr_dm_status("vcc-pc");

	return 0;
}

int init_sys_pwr_dm(void)
{
	unsigned int sys_mask = 0;

	add_sys_pwr_dm("vdd-cpua");
	/*add_sys_pwr_dm("vdd-cpub"); */
	add_sys_pwr_dm("vcc-dram");
	/*add_sys_pwr_dm("vdd-gpu"); */
	add_sys_pwr_dm("vdd-sys");
	/*add_sys_pwr_dm("vdd-vpu"); */
	add_sys_pwr_dm("vdd-cpus");
	/*add_sys_pwr_dm("vdd-drampll"); */
	add_sys_pwr_dm("vcc-lpddr");
	/*add_sys_pwr_dm("vcc-adc"); */
	add_sys_pwr_dm("vcc-pl");
	/*add_sys_pwr_dm("vcc-pm"); */
	add_sys_pwr_dm("vcc-io");
	/*add_sys_pwr_dm("vcc-cpvdd"); */
	/*add_sys_pwr_dm("vcc-ldoin"); */
	add_sys_pwr_dm("vcc-pll");
	add_sys_pwr_dm("vcc-pc");

	sys_mask = get_sys_pwr_dm_mask();
	printk(KERN_INFO "after inited: sys_mask config = 0x%x. \n", sys_mask);

	return 0;
}
#endif

#if (defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)) && \
	defined(CONFIG_AW_AXP)
static int config_pmux_para(unsigned num)
{
#define PM_NAME_LEN (25)
	char name[PM_NAME_LEN] = "\0";
	int enable = 0;
	int pmux_id = 0;
	int pmux_twi_id = 0;
	int pmux_twi_addr = 0;
	struct device_node *np;

	sprintf(name, "pmic%d", num);
	printk("pmu name: %s .\n", name);

	/*get pmu config */
	np = of_find_node_by_type(NULL, name);
	if (NULL == np) {
		printk(KERN_ERR "Warning: can not find np for %s. \n", name);
	} else {
		if (!of_device_is_available(np)) {
			enable = 0;
		} else {
			enable = 1;
		}
		printk(KERN_INFO "%s_enable = 0x%x. \n", name, enable);

		if (1 == enable) {
			if (of_property_read_u32(np, "pmu_id", &pmux_id)) {
				pr_err("Warning: %s fetch pmu_id err. \n",
				       __func__);
				pmux_id = AXP_22X_ID;
			}
			printk(KERN_INFO "pmux_id = 0x%x. \n", pmux_id);
			extended_standby_set_pmu_id(num, pmux_id);

			if (of_property_read_u32
			    (np, "pmu_twi_id", &pmux_twi_id)) {
				pr_err
				    ("Warning: %s fetch pmu_twi_id err. \n",
				     __func__);
				standby_info.pmu_arg.twi_port = 0;
			} else {
				standby_info.pmu_arg.twi_port = pmux_twi_id;
			}
			printk(KERN_INFO "pmux_twi_id = 0x%x. \n",
			       standby_info.pmu_arg.twi_port);

			if (of_property_read_u32
			    (np, "pmu_twi_addr", &pmux_twi_addr)) {
				pr_err
				    ("Warning: %s: fetch pmu_twi_addr err. \n",
				     __func__);
				standby_info.pmu_arg.dev_addr = 0x34;
			} else {
				standby_info.pmu_arg.dev_addr = pmux_twi_addr;
			}
			printk(KERN_INFO "pmux_twi_addr = 0x%x. \n",
			       standby_info.pmu_arg.dev_addr);
		}

	}

	return 0;
}

int config_pmu_para(void)
{
	config_pmux_para(0);
	config_pmux_para(1);
	return 0;
}
#endif

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)) && defined(CONFIG_AW_AXP)
extern  int config_pmux_para(int num, struct aw_pm_info *api, int *pmu_id);

int config_pmu_para(void)
{
	int pmux_id = 0;

	if (0 == config_pmux_para(0, &standby_info, &pmux_id))
		extended_standby_set_pmu_id(0, pmux_id);
	else
		pr_info("pmu0 does not exist. \n");

	if (0 == config_pmux_para(1, &standby_info, &pmux_id))
		extended_standby_set_pmu_id(1, pmux_id);
	else
		pr_info("pmu1 does not exist. \n");

	return 0;
}
#endif

#if (defined(CONFIG_ARCH_SUN8IW8P1) || \
	defined(CONFIG_ARCH_SUN8IW6P1) || \
	defined(CONFIG_ARCH_SUN8IW10P1) || \
	defined(CONFIG_ARCH_SUN8IW11P1) || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)) && \
	defined(CONFIG_AW_AXP)
int config_dynamic_standby(void)
{
	aw_power_scene_e type = SCENE_DYNAMIC_STANDBY;
	scene_extended_standby_t *local_standby;
	int enable = 0;
	int dram_selfresh_flag = 1;
	unsigned int vdd_cpua_vol = 0;
	unsigned int vdd_sys_vol = 0;
	struct device_node *np;
	char *name = "dynamic_standby_para";
	int i = 0;
	int ret = 0;

	/*get customer customized dynamic_standby config */
	np = of_find_node_by_type(NULL, name);
	if (NULL == np) {
		printk(KERN_ERR "Warning: can not find np for %s. \n", name);
	} else {
		if (!of_device_is_available(np)) {
			enable = 0;
		} else {
			enable = 1;
		}
		printk(KERN_INFO "Warning: %s_enable = 0x%x. \n", name, enable);

		if (1 == enable) {
			for (i = 0; i < extended_standby_cnt; i++) {
				if (type == extended_standby[i].scene_type) {
					/*config dram_selfresh flag; */
					local_standby = &(extended_standby[i]);
					if (of_property_read_u32
					    (np, "dram_selfresh_flag",
					     &dram_selfresh_flag)) {
						printk(KERN_ERR
						       "%s: fetch dram_selfresh_flag err. \n",
						       __func__);
						dram_selfresh_flag = 1;
					}
					printk(KERN_INFO
					       "dynamic_standby dram selfresh flag = 0x%x. \n",
					       dram_selfresh_flag);
					if (0 == dram_selfresh_flag) {
						local_standby->
						    soc_pwr_dep.soc_dram_state.selfresh_flag
						    = dram_selfresh_flag;
						local_standby->
						    soc_pwr_dep.soc_pwr_dm_state.state
						    |= BITMAP(VDD_SYS_BIT);
						local_standby->soc_pwr_dep.cpux_clk_state.osc_en |= 0xf;	/* mean all osc is on. */
						/*mean pll5 is shutdowned & open by dram driver. */
						/*hsic can't closed. */
						/*periph is needed. */
						local_standby->
						    soc_pwr_dep.cpux_clk_state.init_pll_dis
						    |=
						    (BITMAP(PM_PLL_HSIC) |
						     BITMAP(PM_PLL_PERIPH)
						     | BITMAP(PM_PLL_DRAM));
					}

					/*config other flag? */
				}

				/*config other extended_standby? */
			}

			if (of_property_read_u32
			    (np, "vdd_cpua_vol", &vdd_cpua_vol)) {
			} else {
				printk(KERN_INFO "vdd_cpua_vol = 0x%x. \n",
				       vdd_cpua_vol);
				ret =
				    scene_set_volt(SCENE_DYNAMIC_STANDBY,
						   VDD_CPUA_BIT, vdd_cpua_vol);
				if (ret < 0)
					printk(KERN_ERR
					       "%s: set vdd_cpua volt failed\n",
					       __func__);

			}

			if (of_property_read_u32
			    (np, "vdd_sys_vol", &vdd_sys_vol)) {
			} else {
				printk(KERN_INFO "vdd_sys_vol = 0x%x. \n",
				       vdd_sys_vol);
				ret =
				    scene_set_volt(SCENE_DYNAMIC_STANDBY,
						   VDD_SYS_BIT, vdd_sys_vol);
				if (ret < 0)
					printk(KERN_ERR
					       "%s: set vdd_sys volt failed\n",
					       __func__);
			}

			printk(KERN_INFO
			       "enable dynamic_standby by customer.\n");
			scene_lock_store(NULL, NULL, "dynamic_standby", 0);

		}
	}

	return 0;
}

static int config_sys_pwr_dm(struct device_node *np, char *pwr_dm)
{
	int dm_enable = 0;

	if (of_property_read_u32(np, pwr_dm, &dm_enable)) {
	} else {
		printk("%s: dm_enalbe: %d. \n", pwr_dm, dm_enable);
		if (0 == dm_enable) {
			del_sys_pwr_dm(pwr_dm);
			save_sys_pwr_state(pwr_dm);
		} else {
			add_sys_pwr_dm(pwr_dm);
		}
	}

	return 0;
}

int config_sys_pwr(void)
{
	unsigned int sys_mask = 0;
	struct device_node *np;
	char *name = "sys_pwr_dm_para";

	np = of_find_node_by_type(NULL, name);
	if (NULL == np) {
		printk(KERN_INFO "info: can not find np for %s. \n", name);
	} else {
		config_sys_pwr_dm(np, "vdd-cpua");
		config_sys_pwr_dm(np, "vdd-cpub");
		config_sys_pwr_dm(np, "vcc-dram");
		config_sys_pwr_dm(np, "vdd-gpu");
		config_sys_pwr_dm(np, "vdd-sys");
		config_sys_pwr_dm(np, "vdd-vpu");
		config_sys_pwr_dm(np, "vdd-cpus");
		config_sys_pwr_dm(np, "vdd-drampll");
		config_sys_pwr_dm(np, "vcc-lpddr");
		config_sys_pwr_dm(np, "vcc-adc");
		config_sys_pwr_dm(np, "vcc-pl");
		config_sys_pwr_dm(np, "vcc-pm");
		config_sys_pwr_dm(np, "vcc-io");
		config_sys_pwr_dm(np, "vcc-cpvdd");
		config_sys_pwr_dm(np, "vcc-ldoin");
		config_sys_pwr_dm(np, "vcc-pll");
	}

	sys_mask = get_sys_pwr_dm_mask();
	printk(KERN_INFO "after customized: sys_mask config = 0x%x. \n",
	       sys_mask);

	return 0;
}
#endif
