/*
 * Allwinner A1X SoCs pinctrl driver.
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 * sunny         <sunny@allwinnertech.com>
 * 
 * 2013-05-02  add sunxi board config(sys config) support.
 * 2013-05-13  add pin interrupt support.
 * 2013-05-14  redefine sunxi pinconf interfaces.
 * 
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/bitmap.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include <linux/sysfs.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include "core.h"
#include "pinctrl-sunxi.h"
#define SUNXI_MAX_NAME_LEN 20
static char	sunxi_dbg_pinname[SUNXI_MAX_NAME_LEN];
static  unsigned int		sunxi_dbg_function;
static  unsigned int		sunxi_dbg_data;
static  unsigned int		sunxi_dbg_level;
static  unsigned int		sunxi_dbg_pull;
static  unsigned int		sunxi_dbg_trigger;

/* add for huangshr-20140701*/
static void pin_reset_bias(unsigned int *pin_bias, unsigned int pin)
{
#if defined(CONFIG_ARCH_SUN8IW9)
	if(pin < SUNXI_PL_BASE){
		*pin_bias = pin;
	}else if((pin >= SUNXI_PL_BASE) && (pin < SUNXI_PO_BASE)){
		*pin_bias = pin - SUNXI_PL_BASE;
	}else if(pin >= SUNXI_PO_BASE){
		*pin_bias = pin - 3*32;
	}
#else
	if(pin< SUNXI_PL_BASE){
		*pin_bias = pin;
	}else{
		*pin_bias = pin - SUNXI_PL_BASE;
	}
#endif
	
}
/*end for huangshr-20140701*/

/* this func main to fix sun8iw7 interrupt error
 *
 */
#ifdef CONFIG_ARCH_SUN8IW7P1
unsigned int sunxi_pinctrl_read_data_for_sun8iw7(struct sunxi_pin_bank *bank, unsigned int pin_bias)	
{

	unsigned int reg_offset, shift;
	unsigned int value, data = 0, loop = 0, i=0;
	unsigned int mask, trigger,pending = 0;
	unsigned int clk_src = 0,  scal = 0;

	/* readl pin mode */
	reg_offset = sunxi_mux_reg(pin_bias);
	shift = sunxi_mux_offset(pin_bias);
	value = pinctrl_readl_reg(bank->membase + reg_offset);
	value = (value >> shift) & MUX_PINS_MASK;
	if(value == SUNXI_PIN_EINT_FUNC){
		/* check if trigger mode is edge */
		reg_offset = sunxi_eint_cfg_reg(pin_bias);
		shift = sunxi_eint_cfg_offset(pin_bias);
		value = pinctrl_readl_reg(bank->membase + reg_offset);
		trigger = (value >> shift) & EINT_CFG_MASK;
		if(trigger == 0 || trigger == 1 || trigger == 4){
			value = pinctrl_readl_reg(bank->membase + sunxi_eint_ctrl_reg(pin_bias));
			mask = EINT_CTRL_MASK << sunxi_eint_ctrl_offset(pin_bias);
			pinctrl_write_reg((value & ~mask), bank->membase + sunxi_eint_ctrl_reg(pin_bias));
		}
		/* switch to input */
		value = pinctrl_readl_reg(bank->membase + sunxi_mux_reg(pin_bias));
		mask = MUX_PINS_MASK << sunxi_mux_offset(pin_bias);
		value=(value & ~mask) | (0 << sunxi_mux_offset(pin_bias));
		pinctrl_write_reg(value,(bank->membase + sunxi_mux_reg(pin_bias)));
		
		/* read data */
		value = pinctrl_readl_reg(bank->membase + sunxi_data_reg(pin_bias));
		data = (value >> sunxi_data_offset(pin_bias)) & DATA_PINS_MASK;

		/* switch to eint */
		value = pinctrl_readl_reg(bank->membase + sunxi_mux_reg(pin_bias));
		mask = MUX_PINS_MASK << sunxi_mux_offset(pin_bias);
		value=(value & ~mask) | (6 << sunxi_mux_offset(pin_bias));
		pinctrl_write_reg(value,(bank->membase + sunxi_mux_reg(pin_bias)));

		/*read debounce check clk src and scat*/
		value = pinctrl_readl_reg(bank->membase + sunxi_eint_debounce_reg(pin_bias));
		clk_src = value & 0x1;
		scal = (value >> 4) & 0x7;
		if(clk_src == 1){
			/*as for 24M clk, loggest sample time is 13us.*/
			loop = 2;
		}else{
			/*as for 32K clk, sample time nearly equal (2*1000*n*1.2)/32 us */
			loop = 8*(2<<scal);
		}
		/* wait for pending and clear*/
		while(i<=loop){
			/* readl pending */
			value = readl(bank->membase + sunxi_eint_status_reg(pin_bias));
			pending = (value >> (pin_bias % SUNXI_BANK_SIZE)) & 1;
			if(pending){
				break;
			}
			i++;
			udelay(10);
		}
		if(pending){
			pinctrl_write_reg(1 << (pin_bias % SUNXI_BANK_SIZE), bank->membase + sunxi_eint_status_reg(pin_bias));
		}
		if(trigger == 0 || trigger == 1 || trigger == 4){
			mask = pinctrl_readl_reg(bank->membase + sunxi_eint_ctrl_reg(pin_bias));
			mask |= EINT_CTRL_MASK << sunxi_eint_ctrl_offset(pin_bias);
			pinctrl_write_reg(mask, bank->membase + sunxi_eint_ctrl_reg(pin_bias));
		}
	}else{
		value = pinctrl_readl_reg(bank->membase + sunxi_data_reg(pin_bias));
		data = (value >> sunxi_data_offset(pin_bias)) & DATA_PINS_MASK;
	}
	return data;
}
#endif/*end */

static struct sunxi_pinctrl *sunxi_gc_to_pinctrl(struct gpio_chip *gc)
{
	return container_of(gc, struct sunxi_pinctrl, chip);
}

static struct sunxi_pin_bank *sunxi_pin_to_bank(struct sunxi_pinctrl *pctl, 
                                                unsigned pin)
{
	unsigned pin_offset = pin % SUNXI_BANK_SIZE;
	unsigned pin_base = pin - pin_offset;
	int      i;
	
	/* search target bank within pinctrl->desc banks */
	for (i = 0; i < pctl->desc->nbanks; i++) {
		if (pin_base == pctl->desc->banks[i].pin_base) {
			return &(pctl->desc->banks[i]);
		}
	}
	/* invalid pin number to seach bank */
	pr_debug("seach pin [%d] target bank failed\n", pin);
	return NULL;
}

static bool sunxi_pin_valid(struct sunxi_pinctrl *pctl, unsigned pin)
{
	unsigned pin_offset = pin % SUNXI_BANK_SIZE;
	unsigned pin_base = pin - pin_offset;
	int      i;
	
	if (IS_AXP_PIN(pin)) {
		/* valid axp pin number */
		return false;
	} 
	
	/* search target bank within pinctrl->desc banks */
	for (i = 0; i < pctl->desc->nbanks; i++) {
		if ((pin_base == pctl->desc->banks[i].pin_base) &&
		     pin_offset < pctl->desc->banks[i].nr_pins) {
			/* valid pin number */
			return true;
		}
	}
	/* invalid pin number */
	return false;
}


#if defined(CONFIG_OF)
static struct sunxi_pinctrl_group *
sunxi_pinctrl_find_group_by_name(struct sunxi_pinctrl *pctl, const char *group)
{
	int i;

	for (i = 0; i < pctl->ngroups; i++) {
		struct sunxi_pinctrl_group *grp = pctl->groups + i;

		if (!strcmp(grp->name, group))
			return grp;
	}

	return NULL;
}
#endif

static struct sunxi_pinctrl_function *
sunxi_pinctrl_find_function_by_name(struct sunxi_pinctrl *pctl,
				    const char *name)
{
	struct sunxi_pinctrl_function *func = pctl->functions;
	int i;

	for (i = 0; i < pctl->nfunctions; i++) {
		if (!func[i].name)
			break;

		if (!strcmp(func[i].name, name))
			return func + i;
	}

	return NULL;
}

static struct sunxi_desc_function *
sunxi_pinctrl_desc_find_function_by_name(struct sunxi_pinctrl *pctl,
					 const char *pin_name,
					 const char *func_name)
{
	int i;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		if (!strcmp(pin->pin.name, pin_name)) {
			struct sunxi_desc_function *func = pin->functions;

			while (func->name) {
				if (!strcmp(func->name, func_name))
					return func;

				func++;
			}
		}
	}

	return NULL;
}

static int sunxi_pctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->ngroups;
}

static const char *sunxi_pctrl_get_group_name(struct pinctrl_dev *pctldev,
					      unsigned group)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->groups[group].name;
}

static int sunxi_pctrl_get_group_pins(struct pinctrl_dev *pctldev,
				      unsigned group,
				      const unsigned **pins,
				      unsigned *num_pins)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*pins = (unsigned *)&pctl->groups[group].pin;
	*num_pins = 1;

	return 0;
}

#ifdef CONFIG_OF
static int sunxi_pctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
				      struct device_node *node,
				      struct pinctrl_map **map,
				      unsigned *num_maps)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned long *pinconfig;
	struct property *prop;
	const char *function;
	const char *group;
	int ret, nmaps, i = 0;
	u32 val;

	*map = NULL;
	*num_maps = 0;

	ret = of_property_read_string(node, "allwinner,function", &function);
	if (ret) {
		dev_err(pctl->dev,
			"missing allwinner,function property in node %s\n",
			node->name);
		return -EINVAL;
	}

	nmaps = of_property_count_strings(node, "allwinner,pins") * 2;
	if (nmaps < 0) {
		dev_err(pctl->dev,
			"missing allwinner,pins property in node %s\n",
			node->name);
		return -EINVAL;
	}

	*map = kmalloc(nmaps * sizeof(struct pinctrl_map), GFP_KERNEL);
	if (!map)
		return -ENOMEM;

	of_property_for_each_string(node, "allwinner,pins", prop, group) {
		struct sunxi_pinctrl_group *grp =
			sunxi_pinctrl_find_group_by_name(pctl, group);
		int j = 0, configlen = 0;

		if (!grp) {
			dev_err(pctl->dev, "unknown pin %s", group);
			continue;
		}

		if (!sunxi_pinctrl_desc_find_function_by_name(pctl,
							      grp->name,
							      function)) {
			dev_err(pctl->dev, "unsupported function %s on pin %s",
				function, group);
			continue;
		}

		(*map)[i].type = PIN_MAP_TYPE_MUX_GROUP;
		(*map)[i].data.mux.group = group;
		(*map)[i].data.mux.function = function;

		i++;

		(*map)[i].type = PIN_MAP_TYPE_CONFIGS_GROUP;
		(*map)[i].data.configs.group_or_pin = group;

		if (of_find_property(node, "allwinner,drive", NULL))
			configlen++;
		if (of_find_property(node, "allwinner,pull", NULL))
			configlen++;

		pinconfig = kzalloc(configlen * sizeof(*pinconfig), GFP_KERNEL);

		if (!of_property_read_u32(node, "allwinner,drive", &val)) {
			u16 strength = (val + 1) * 10;
			pinconfig[j++] =
				pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH,
							 strength);
		}

		if (!of_property_read_u32(node, "allwinner,pull", &val)) {
			enum pin_config_param pull = PIN_CONFIG_END;
			if (val == 1)
				pull = PIN_CONFIG_BIAS_PULL_UP;
			else if (val == 2)
				pull = PIN_CONFIG_BIAS_PULL_DOWN;
			pinconfig[j++] = pinconf_to_config_packed(pull, 0);
		}

		(*map)[i].data.configs.configs = pinconfig;
		(*map)[i].data.configs.num_configs = configlen;

		i++;
	}

	*num_maps = nmaps;

	return 0;
}

static void sunxi_pctrl_dt_free_map(struct pinctrl_dev *pctldev,
				    struct pinctrl_map *map,
				    unsigned num_maps)
{
	int i;

	for (i = 0; i < num_maps; i++) {
		if (map[i].type == PIN_MAP_TYPE_CONFIGS_GROUP)
			kfree(map[i].data.configs.configs);
	}

	kfree(map);
}

#endif /* CONFIG_OF */

static struct pinctrl_ops sunxi_pctrl_ops = {
#ifdef CONFIG_OF
	.dt_node_to_map		= sunxi_pctrl_dt_node_to_map,
	.dt_free_map		= sunxi_pctrl_dt_free_map,
#endif /* CONFIG_OF */
	.get_groups_count	= sunxi_pctrl_get_groups_count,
	.get_group_name		= sunxi_pctrl_get_group_name,
	.get_group_pins		= sunxi_pctrl_get_group_pins,
};

static int sunxi_pinconf_get(struct pinctrl_dev *pctldev,
		             unsigned pin,
			     unsigned long *config)
{
	struct sunxi_pinctrl  *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pin_bank *bank = sunxi_pin_to_bank(pctl, pin);
	u32                  val;
	u16                  dlevel;
	u16                  data;
	u16                  func;
	u16                  pull;
	unsigned int  		 pin_bias;

	pin_reset_bias(&pin_bias, pin);
	if (IS_ERR_OR_NULL(bank)) {
		pr_debug("invalid pin number [%d] to get pinconf\n", pin);
		return -EINVAL;
	}
	switch (SUNXI_PINCFG_UNPACK_TYPE(*config)) {
	case SUNXI_PINCFG_TYPE_DRV:
		val = pinctrl_readl_reg(bank->membase + sunxi_dlevel_reg(pin_bias));
		dlevel = (val >> sunxi_dlevel_offset(pin_bias)) & DLEVEL_PINS_MASK;
		*config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, dlevel);
	        pr_debug("sunxi pconf get pin [%s] drive strength [LEVEL%d]\n", 
		         pin_get_name(pctl->pctl_dev, pin), dlevel);
		break;
	case SUNXI_PINCFG_TYPE_PUD:
		val = pinctrl_readl_reg(bank->membase + sunxi_pull_reg(pin_bias));
		pull = (val >> sunxi_pull_offset(pin_bias)) & PULL_PINS_MASK;
		*config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pull);
	        pr_debug("sunxi pconf get pin [%s] pull [%d]\n", 
		         pin_get_name(pctl->pctl_dev, pin), pull);
		break;
	case SUNXI_PINCFG_TYPE_DAT:
#ifndef CONFIG_ARCH_SUN8IW7
		val = pinctrl_readl_reg(bank->membase + sunxi_data_reg(pin_bias));
		data = (val >> sunxi_data_offset(pin_bias)) & DATA_PINS_MASK;
#else
		data = sunxi_pinctrl_read_data_for_sun8iw7(bank, pin_bias);
#endif
		*config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, data);
	        pr_debug("sunxi pconf get pin [%s] data [%d]\n", 
		         pin_get_name(pctl->pctl_dev, pin), data);
		break;
	case SUNXI_PINCFG_TYPE_FUNC:
		val = pinctrl_readl_reg(bank->membase + sunxi_mux_reg(pin_bias));
		func = (val >> sunxi_mux_offset(pin_bias)) & MUX_PINS_MASK;
		*config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, func);
	        pr_debug("sunxi pconf get pin [%s] func [%d]\n", 
		         pin_get_name(pctl->pctl_dev, pin), func);
		break;
	default:
		pr_debug("invalid sunxi pconf type for get\n");
		return -EINVAL;
	}
	return 0;
}

static int sunxi_pinconf_set(struct pinctrl_dev *pctldev,
			     unsigned pin,
			     unsigned long config)
{
	struct sunxi_pinctrl  *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pin_bank *bank = sunxi_pin_to_bank(pctl, pin);
	unsigned int  pin_bias;
	void __iomem		*reg;
	u32                  val;
	u32                  mask;
	u16                  dlevel;
	u16                  data;
	u16                  func;
	u16                  pull;

	pin_reset_bias(&pin_bias, pin);
	if (IS_ERR_OR_NULL(bank)) {
		pr_debug("invalid pin number [%d] to set pinconf\n", pin);
		return -EINVAL;
	}
	switch (SUNXI_PINCFG_UNPACK_TYPE(config)) {
	case SUNXI_PINCFG_TYPE_DRV:
		dlevel = SUNXI_PINCFG_UNPACK_VALUE(config);
		val = pinctrl_readl_reg(bank->membase + sunxi_dlevel_reg(pin_bias));
	    mask = DLEVEL_PINS_MASK << sunxi_dlevel_offset(pin_bias);
		val=(val & ~mask) | (dlevel << sunxi_dlevel_offset(pin_bias));
		reg=bank->membase + sunxi_dlevel_reg(pin_bias);
		pinctrl_write_reg(val,reg);
		pr_debug("sunxi pconf set pin [%s] drive strength to [LEVEL%d]\n", 
		         pin_get_name(pctl->pctl_dev, pin), dlevel);
		break;
	case SUNXI_PINCFG_TYPE_PUD:
		pull = SUNXI_PINCFG_UNPACK_VALUE(config);
		val = pinctrl_readl_reg(bank->membase + sunxi_pull_reg(pin_bias));
		mask = PULL_PINS_MASK << sunxi_pull_offset(pin_bias);
		val=(val & ~mask) | (pull << sunxi_pull_offset(pin_bias));
		reg=bank->membase + sunxi_pull_reg(pin_bias);
		pinctrl_write_reg(val,reg);
		pr_debug("sunxi pconf set pin [%s] pull to [%d]\n", 
		        pin_get_name(pctl->pctl_dev, pin), pull);
		break;
	case SUNXI_PINCFG_TYPE_DAT:
		data = SUNXI_PINCFG_UNPACK_VALUE(config);
		val = pinctrl_readl_reg(bank->membase + sunxi_data_reg(pin_bias));
		mask = DATA_PINS_MASK << sunxi_data_offset(pin_bias);
		val=(val & ~mask) | (data << sunxi_data_offset(pin_bias));
		reg=bank->membase + sunxi_data_reg(pin_bias);
		pinctrl_write_reg(val,reg);
		pr_debug("sunxi pconf set pin [%s] data to [%d]\n", 
		        pin_get_name(pctl->pctl_dev, pin), data);
		break;
	case SUNXI_PINCFG_TYPE_FUNC:
		func = SUNXI_PINCFG_UNPACK_VALUE(config);
		val = pinctrl_readl_reg(bank->membase + sunxi_mux_reg(pin_bias));
	    mask = MUX_PINS_MASK << sunxi_mux_offset(pin_bias);
		val=(val & ~mask) | (func << sunxi_mux_offset(pin_bias));
		reg=bank->membase + sunxi_mux_reg(pin_bias);
		pinctrl_write_reg(val,reg);
		pr_debug("sunxi pconf set pin [%s] func to [%d]\n", 
		         pin_get_name(pctl->pctl_dev, pin), func);
		break;
	default:
		pr_debug("invalid sunxi pconf type for set\n");
		return -EINVAL;
	}
	return 0;
}

static int sunxi_pinconf_group_set(struct pinctrl_dev *pctldev,
			           unsigned group, unsigned long config)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	return sunxi_pinconf_set(pctldev, pctl->groups[group].pin, config);
}			          

static int sunxi_pinconf_group_get(struct pinctrl_dev *pctldev,
				   unsigned group,
				   unsigned long *config)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	return sunxi_pinconf_get(pctldev, pctl->groups[group].pin, config);
}

static struct pinconf_ops sunxi_pconf_ops = {
	.pin_config_get		= sunxi_pinconf_get,
	.pin_config_set		= sunxi_pinconf_set,
	.pin_config_group_get	= sunxi_pinconf_group_get,
	.pin_config_group_set	= sunxi_pinconf_group_set,
};

static int sunxi_pmx_get_funcs_cnt(struct pinctrl_dev *pctldev)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->nfunctions;
}

static const char *sunxi_pmx_get_func_name(struct pinctrl_dev *pctldev,
					   unsigned function)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->functions[function].name;
}

static int sunxi_pmx_get_func_groups(struct pinctrl_dev *pctldev,
				     unsigned function,
				     const char * const **groups,
				     unsigned * const num_groups)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*groups = pctl->functions[function].groups;
	*num_groups = pctl->functions[function].ngroups;

	return 0;
}

static void sunxi_pmx_set(struct pinctrl_dev *pctldev,unsigned pin,
                          u8 config,bool enable)
{
	struct sunxi_pinctrl  *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pin_bank *bank = sunxi_pin_to_bank(pctl, pin);
	void __iomem            *reg;
	unsigned long          value;
	unsigned int           offset;
	unsigned int           shift;
	unsigned int  		 pin_bias;

	pin_reset_bias(&pin_bias, pin);
	if (IS_ERR_OR_NULL(bank)) {
		pr_debug("invalid pin number [%d] to pmx set\n", pin);
		return;
	}
	offset = sunxi_mux_reg(pin_bias);
	shift  = sunxi_mux_offset(pin_bias);
	value  = pinctrl_readl_reg(bank->membase + offset);
	value &= ~(MUX_PINS_MASK << shift);
	if(enable){
		value |= (config << shift);
	}else{
		value |= (MUX_PINS_MASK << shift);
	}
	reg=bank->membase + offset;
	pinctrl_write_reg(value,reg);
	pr_debug("sunxi pmx set pin [%s] to %d\n", 
	         pin_get_name(pctldev, pin), config);
}

static int sunxi_pmx_enable(struct pinctrl_dev *pctldev,
			    unsigned function,
			    unsigned group)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pinctrl_group *g = pctl->groups + group;
	struct sunxi_pinctrl_function *func = pctl->functions + function;
	struct sunxi_desc_function *desc =
		sunxi_pinctrl_desc_find_function_by_name(pctl,
							 g->name,
							 func->name);
	if (!desc)
		return -EINVAL;

	sunxi_pmx_set(pctldev, g->pin, desc->muxval,true);

	return 0;
}
static void sunxi_pmx_disable(struct pinctrl_dev *pctldev,
			    unsigned function,
			    unsigned group)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pinctrl_group *g = pctl->groups + group;
	struct sunxi_pinctrl_function *func = pctl->functions + function;
	struct sunxi_desc_function *desc =
		sunxi_pinctrl_desc_find_function_by_name(pctl,
							 g->name,
							 func->name);
	sunxi_pmx_set(pctldev, g->pin, desc->muxval,false);

}


static int
sunxi_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range,
			unsigned offset,
			bool input)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_desc_function *desc;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	const char *func;
	u8 bank, pin;
	int ret;

	bank = (offset) / PINS_PER_BANK;
	pin = (offset) % PINS_PER_BANK;

	ret = sprintf(pin_name, "P%c%d", 'A' + bank, pin);
	if (!ret)
		goto error;

	if (input)
		func = "gpio_in";
	else
		func = "gpio_out";

	desc = sunxi_pinctrl_desc_find_function_by_name(pctl,
							pin_name,
							func);
	if (!desc) {
		ret = -EINVAL;
		goto error;
	}

	sunxi_pmx_set(pctldev, offset, desc->muxval,true);

	ret = 0;

error:
	return ret;
}

static struct pinmux_ops sunxi_pmx_ops = {
	.get_functions_count	= sunxi_pmx_get_funcs_cnt,
	.get_function_name	= sunxi_pmx_get_func_name,
	.get_function_groups	= sunxi_pmx_get_func_groups,
	.enable			= sunxi_pmx_enable,
	.disable		= sunxi_pmx_disable,
	.gpio_set_direction	= sunxi_pmx_gpio_set_direction,
};

static struct pinctrl_desc sunxi_pctrl_desc = {
	.confops	= &sunxi_pconf_ops,
	.pctlops	= &sunxi_pctrl_ops,
	.pmxops		= &sunxi_pmx_ops,
};

static int sunxi_pinctrl_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_request_gpio((unsigned)(chip->base) + offset);
}

static void sunxi_pinctrl_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	pinctrl_free_gpio(chip->base + offset);
}

static int sunxi_pinctrl_gpio_direction_input(struct gpio_chip *chip,
					unsigned offset)
{
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int sunxi_pinctrl_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct sunxi_pinctrl  *pctrl = dev_get_drvdata(chip->dev);
	struct sunxi_pin_bank *bank  = sunxi_pin_to_bank(pctrl, offset);
	unsigned long          reg;
	unsigned int           shift;
	int                    val;
	unsigned int  		 pin_bias;
	
	pin_reset_bias(&pin_bias, offset);
	
	if (IS_ERR_OR_NULL(bank)) {
		pr_debug("invalid pin number [%d] to get gpio data\n", offset);
		return -EINVAL;
	}
	reg = sunxi_data_reg(pin_bias);
	shift = sunxi_data_offset(pin_bias);
#ifndef CONFIG_ARCH_SUN8IW7P1
	val = (pinctrl_readl_reg(bank->membase + reg) >> shift) & DATA_PINS_MASK;
#else
	val = sunxi_pinctrl_read_data_for_sun8iw7(bank, pin_bias);
#endif
	return val;
}
static void sunxi_pinctrl_gpio_set(struct gpio_chip *chip,
				   unsigned offset, 
				   int value)
{
	struct sunxi_pinctrl	*pctrl = dev_get_drvdata(chip->dev);
	struct sunxi_pin_bank	*bank  = sunxi_pin_to_bank(pctrl, offset);
	unsigned long		reg;
	unsigned int		shift;
	int			reg_val;
	unsigned int		pin_bias;
	
	pin_reset_bias(&pin_bias, offset);
	if (IS_ERR_OR_NULL(bank)) {
		pr_debug("invalid pin number [%d] to set gpio data\n", offset);
		return;
	}
	reg = sunxi_data_reg(pin_bias);
	shift = sunxi_data_offset(pin_bias);
	reg_val = pinctrl_readl_reg(bank->membase + reg);
	reg_val &= ~(DATA_PINS_MASK << shift);
	reg_val |= ((value & DATA_PINS_MASK) << shift);
	pinctrl_write_reg(reg_val, bank->membase + reg);
}

static int sunxi_pinctrl_gpio_direction_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	sunxi_pinctrl_gpio_set(chip,offset,value);
	return pinctrl_gpio_direction_output(chip->base + offset);
}



#if defined(CONFIG_OF)
static int sunxi_pinctrl_gpio_of_xlate(struct gpio_chip *gc,
				const struct of_phandle_args *gpiospec,
				u32 *flags)
{
	int pin, base;

	base = PINS_PER_BANK * gpiospec->args[0];
	pin = base + gpiospec->args[1];

	if (pin > (gc->base + gc->ngpio))
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[2];

	return pin;
}
#endif

static int sunxi_pinctrl_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct sunxi_pinctrl  *pctrl = sunxi_gc_to_pinctrl(gc);
	struct sunxi_pin_bank *bank = NULL;
	unsigned int pin_idx = offset % SUNXI_BANK_SIZE;
	unsigned int pin_base = offset - pin_idx;
	unsigned int virq;
	unsigned int i;
	
	/* check offset is valid eint pin or not */
	for (i = 0; i < pctrl->desc->nbanks; i++) {
		if ((pctrl->desc->banks[i].eint_type == SUNXI_EINT_TYPE_GPIO) &&
		    (pin_base == pctrl->desc->banks[i].pin_base)) {
			/* target bank find */
			bank = &(pctrl->desc->banks[i]);
			break;
		}
	}
	if (!bank) {
		pr_debug("invalid gpio [%d] to irq\n", offset);
		return -ENXIO;
	}
	if (!bank->irq_domain) {
		return -ENXIO;
	}
	virq = irq_create_mapping(bank->irq_domain, pin_idx);

	return (virq) ? : -ENXIO;
}
static int sunxi_pinctrl_gpio_set_debounce(struct gpio_chip *chip,
					unsigned offset, unsigned value)
{
	struct sunxi_pinctrl  *pctrl = dev_get_drvdata(chip->dev);
	struct sunxi_pin_bank *bank  = sunxi_pin_to_bank(pctrl, offset);
	int			ret = 0;
	unsigned long		reg;
	int			reg_val;
	unsigned int		pin_bias;
	unsigned int 		val_clk_per_scale;
	unsigned int		val_clk_select;
	
	pin_reset_bias(&pin_bias, offset);
	if (SUNXI_EINT_TYPE_NONE == bank->eint_type){
		pr_debug("pin bank havn't eint pin type\n");
		return -EINVAL;
	}
	reg = sunxi_eint_debounce_reg(pin_bias);
	reg_val = pinctrl_readl_reg(bank->membase + reg);
	val_clk_select = value & 1;
	val_clk_per_scale= (value >> 4) & 0b111;

	/*set debounce pio interrupt clock select */
	reg_val &= ~(1 << 0);
	reg_val |= val_clk_select ;

	/* set debounce clock pre scale */
	reg_val &= ~(7 << 4);
	reg_val |= val_clk_per_scale << 4;
	pinctrl_write_reg(reg_val, bank->membase + reg);
	//pr_debug("reg:   0x%8x     value: 0x%8x\n",bank->membase + reg,reg_val);
	return ret;
}


static struct gpio_chip sunxi_pinctrl_gpio_chip = {
	.owner			= THIS_MODULE,
	.request		= sunxi_pinctrl_gpio_request,
	.free			= sunxi_pinctrl_gpio_free,
	.direction_input	= sunxi_pinctrl_gpio_direction_input,
	.direction_output	= sunxi_pinctrl_gpio_direction_output,
	.get			= sunxi_pinctrl_gpio_get,
	.set			= sunxi_pinctrl_gpio_set,
	.to_irq                 = sunxi_pinctrl_gpio_to_irq,
	.set_debounce		= sunxi_pinctrl_gpio_set_debounce,
#if defined(CONFIG_OF_GPIO)
	.of_xlate		= sunxi_pinctrl_gpio_of_xlate,
	.of_gpio_n_cells	= 3,
#endif
	.can_sleep		= 0,
};

#if defined(CONFIG_OF)
static struct of_device_id sunxi_pinctrl_match[] = {
	{ .compatible = "allwinner,sun4i-a10-pinctrl", .data = (void *)&sun4i_a10_pinctrl_data },
	{ .compatible = "allwinner,sun5i-a13-pinctrl", .data = (void *)&sun5i_a13_pinctrl_data },
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_pinctrl_match);
#endif

static int sunxi_pinctrl_add_function(struct sunxi_pinctrl *pctl,
					const char *name)
{
	struct sunxi_pinctrl_function *func = pctl->functions;
	while (func->name) {
		/* function already there */
		if (strcmp(func->name, name) == 0) {
			func->ngroups++;
			return -EEXIST;
		}
		func++;
	}
	func->name = name;
	func->ngroups = 1;

	pctl->nfunctions++;

	return 0;
}

static int sunxi_pinctrl_build_state(struct platform_device *pdev)
{
	struct sunxi_pinctrl *pctl = platform_get_drvdata(pdev);
	int i;

	pctl->ngroups = pctl->desc->npins;

	/* Allocate groups */
	pctl->groups = devm_kzalloc(&pdev->dev,
				    pctl->ngroups * sizeof(*pctl->groups),
				    GFP_KERNEL);
	if (!pctl->groups)
		return -ENOMEM;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_pinctrl_group *group = pctl->groups + i;

		group->name = pin->pin.name;
		group->pin = pin->pin.number;
	}

	/*
	 * We suppose that we won't have any more functions than pins,
	 * we'll reallocate that later anyway
	 */
	pctl->functions = devm_kzalloc(&pdev->dev,
				pctl->desc->npins * sizeof(*pctl->functions),
				GFP_KERNEL);
	if (!pctl->functions)
		return -ENOMEM;

	/* Count functions and their associated groups */
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func = pin->functions;

		while (func->name) {
			sunxi_pinctrl_add_function(pctl, func->name);
			func++;
		}
	}

	pctl->functions = krealloc(pctl->functions,
				pctl->nfunctions * sizeof(*pctl->functions),
				GFP_KERNEL);

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func = pin->functions;

		while (func->name) {
			struct sunxi_pinctrl_function *func_item;
			const char **func_grp;

			func_item = sunxi_pinctrl_find_function_by_name(pctl,
									func->name);
			if (!func_item)
				return -EINVAL;

			if (!func_item->groups) {
				func_item->groups =
					devm_kzalloc(&pdev->dev,
						     func_item->ngroups * sizeof(*func_item->groups),
						     GFP_KERNEL);
				if (!func_item->groups)
					return -ENOMEM;
			}

			func_grp = func_item->groups;
			while (*func_grp)
				func_grp++;

			*func_grp = pin->pin.name;
			func++;
		}
	}

	return 0;
}

#ifndef CONFIG_OF

static char *sunxi_pinctrl_mainkey_to_device_name(char *mainkey_name)
{
	char 	*device_name;
	script_item_value_type_e type;
	script_item_u   item;
	/* try to get device name */
	type = script_get_item(mainkey_name, "device_name", &item);
	if ((type == SCIRPT_ITEM_VALUE_TYPE_STR) && (item.str)) {
		/* the mainkey have valid device-name,
		 * use the config device_name
		 */
		device_name = item.str;
	} else {
		/* have no device_name config,
		 * default use mainkey name as device name
		 */
		device_name = mainkey_name;
	}
	return device_name;
}

static int sunxi_pin_cfg_to_pin_map_default(struct platform_device *pdev,
                                    struct gpio_config *cfg, 
                                    struct pinctrl_map *map,
                                    char  *device_name, char *state_name)
{
	int          num_configs;
	unsigned long *configs;
	unsigned int pin_number;
	const char   *pin_name;
	const char   *ctrl_name = dev_name(&pdev->dev);
	const char   *function_name;
	struct sunxi_pinctrl *pctl = platform_get_drvdata(pdev);
	struct sunxi_desc_function *func;
	int          eint_mux = -1;
	
	/* check config pin is valid sunxi pinctrl pins */
	if (!sunxi_pin_valid(pctl, cfg->gpio)) {
		pr_debug("invalid pin number under sunxi platform.[%d]\n", cfg->gpio);
		return 0;
	}
	
	/* find pin name by number */
	pin_number = cfg->gpio;
	pin_name = pin_get_name(pctl->pctl_dev, pin_number);
	if (!pin_name) {
		pr_debug("find pin [%d] name failed\n", pin_number);
		return 0;
	}
	/* try to fins eint pin mux value,
	 * sunxi eint pin have no fix mux value,
	 * so we should dynamicly seach by pin name.
	 */
	func = sunxi_pinctrl_desc_find_function_by_name(pctl, pin_name, "eint");
	if (!IS_ERR_OR_NULL(func)) {
		/* the pin have "eint" function */
		eint_mux = func->muxval;
	}
	/* mux pinctrl map */
	if (cfg->mul_sel == SUNXI_PIN_INPUT_FUNC) {
		/* pin mux : input */
		function_name = "gpio_in";
	} else if (cfg->mul_sel == SUNXI_PIN_OUTPUT_FUNC) {
		/* pin mux : output */
		function_name = "gpio_out";
	} else if (cfg->mul_sel == eint_mux) {
		/* pin mux : eint */
		function_name = "eint";
	} else if (cfg->mul_sel == SUNXI_PIN_IO_DISABLE){
		function_name = "io_disable";
	} else{
		/* default use mainkey(device name) as function name */
		function_name = device_name;
	}
	map[0].dev_name      = device_name;
	map[0].name          = state_name;
	map[0].type          = PIN_MAP_TYPE_MUX_GROUP;
	map[0].ctrl_dev_name = ctrl_name;
	map[0].data.mux.function = function_name;
	map[0].data.mux.group    = pin_name;
    	
    	/* configuration pinctrl map,
	 * suppose max pin config is 3.
	 * sunxi have 3 type configurations: pull/driver-level/output-data/.
	 * yes I know the configs memory will binding to pinctrl always,
	 * if we binding it to pinctrl, we don't free it anywhere!
	 * the configs memory will always allocate for pinctrl.
	 * by sunny at 2013-4-28 15:52:16.
	 */
	num_configs = 0;
	configs = kzalloc(sizeof(unsigned int) * 3, GFP_KERNEL);
	if (!configs) {
		pr_err("allocate memory for pin config failed\n");
		return -ENOMEM;
	}
	if (cfg->pull != GPIO_PULL_DEFAULT) {
		configs[num_configs] = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, cfg->pull);
		num_configs++;
	}
	if (cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
		configs[num_configs] = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, cfg->drv_level);
		num_configs++;
	}
	if (cfg->data != GPIO_DATA_DEFAULT) {
		configs[num_configs] = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, cfg->data);
		num_configs++;
	}
	if (num_configs == 0) {
		/* this pin have no config,
		 * use hardware default value.
		 * we have only mux mapping.
		 */
		 kfree(configs);
		 return 1;
	}
	map[1].dev_name      = device_name;
	map[1].name          = state_name;
	map[1].type          = PIN_MAP_TYPE_CONFIGS_GROUP;
	map[1].ctrl_dev_name = ctrl_name;
	map[1].data.configs.group_or_pin = pin_name;
	map[1].data.configs.configs      = configs;
	map[1].data.configs.num_configs  = num_configs;
	
	/* we have two maps: mux + configs */
    	return 2;
}
static int sunxi_pinctrl_creat_mappings(struct platform_device *pdev, int pin_count,
					script_item_u  *pin_list, char *device_name,
					char *state_name)
{
	struct pinctrl_map *maps;
	int 		pin_index;
	int 		map_index;

	/* allocate pinctrl_map table,
	 * max map table size = pin count * 2 :
	 * mux map and config map.
	 */
	maps = kzalloc(sizeof(*maps) * (pin_count * 2), GFP_KERNEL);
	if (!maps) {
		pr_err("allocate memory for sunxi pinctrl map table failed\n");
		return -ENOMEM;
	}
	map_index = 0;
	for (pin_index = 0; pin_index < pin_count; pin_index++) {
		/* convert struct sunxi_pin_cfg to struct pinctrl_map */
		map_index += sunxi_pin_cfg_to_pin_map_default(pdev,
				&(pin_list[pin_index].gpio),
				&(maps[map_index]),
				device_name, state_name);
	}
	if (map_index) {
		/* register maps to pinctrl */
		pinctrl_register_mappings(maps, map_index);
	}
	/* free pinctrl_map table directly,
	 * pinctrl subsytem will dup this map table
	 */
	kfree(maps);

	return 0;
}

static int sunxi_pin_cfg_to_pin_map_suspend(struct platform_device *pdev,
                                    struct gpio_config *cfg,
                                    struct pinctrl_map *map,
                                    char  *device_name, char *state_name)
{
	const char     *pin_name;
	unsigned int 	pin_number;
	struct sunxi_pinctrl *pctl = platform_get_drvdata(pdev);

	/* check config pin is valid sunxi pinctrl pins */
	if (!sunxi_pin_valid(pctl, cfg->gpio)) {
		return 0;
	}
	/* find pin name by number */
	pin_number = cfg->gpio;
	pin_name = pin_get_name(pctl->pctl_dev, pin_number);
	if (!pin_name) {
		return 0;
	}
	map[0].dev_name      = device_name;
	map[0].name          = state_name;
	map[0].type          = PIN_MAP_TYPE_MUX_GROUP;
	map[0].ctrl_dev_name = dev_name(&pdev->dev);
	map[0].data.mux.function = "io_disable";
	map[0].data.mux.group    = pin_name;

	return 1;

}
static int sunxi_pinctrl_creat_suspend_mappings(struct platform_device *pdev,
					int pin_count, script_item_u  *pin_list,
					char *device_name, char *state_name)
{
	struct pinctrl_map *maps;
	int 		pin_index;
	int 		map_index;
	maps = kzalloc(sizeof(*maps) * pin_count, GFP_KERNEL);
	if (!maps) {
		pr_err("allocate memory for sunxi pinctrl map table failed\n");
		return -ENOMEM;
	}
	map_index = 0;
	for (pin_index = 0; pin_index < pin_count; pin_index++) {
		/* convert struct sunxi_pin_cfg to struct pinctrl_map */
		map_index += sunxi_pin_cfg_to_pin_map_suspend(pdev,
				&(pin_list[pin_index].gpio),
				&(maps[map_index]),
				device_name, state_name);

	}
	if (map_index) {
		pinctrl_register_mappings(maps, map_index);
	}
	kfree(maps);

	return 0;

}

static int sunxi_pinctrl_parse_pin_cfg(struct platform_device *pdev)
{
	int             mainkey_count;
	int             mainkey_idx;
	int		ret;
	
	/* get main key count */
	mainkey_count = script_get_main_key_count();
	pr_debug("mainkey total count : %d\n", mainkey_count);

	for (mainkey_idx = 0; mainkey_idx < mainkey_count; mainkey_idx++) {
		char           *mainkey_name;
		script_item_u  *pin_list;
		int             pin_count;
		char           *device_name;
		char	       *state_name;
		char 	       *mainkey_suspend;

		/* get main key name by index */
		mainkey_name = script_get_main_key_name(mainkey_idx);
		if (!mainkey_name) {
			/* can not get mainkey name */
			pr_debug("get mainkey [%s] name failed\n", mainkey_name);
			continue;
		}
		if(strstr(mainkey_name, "_suspend")){
			/* if mainkey have a suffix _suspend */
			continue;
		}
		pin_count = script_get_pio_list(mainkey_name, &pin_list);
		if (pin_count == 0) {
			/* mainley have no pin configuration */
			continue;
		}
		/* build default state */
		device_name = sunxi_pinctrl_mainkey_to_device_name(mainkey_name);
		state_name  = PINCTRL_STATE_DEFAULT;
		ret = sunxi_pinctrl_creat_mappings(pdev, pin_count, pin_list, device_name, state_name);

		/* build suspend state */
		mainkey_suspend = kzalloc(32 * sizeof(char *), GFP_KERNEL);
		strcpy(mainkey_suspend, mainkey_name);
		mainkey_suspend = strcat(mainkey_suspend, "_suspend");

		if(script_is_main_key_exist(mainkey_suspend)){
			pin_count = script_get_pio_list(mainkey_suspend, &pin_list);
			if (pin_count == 0) {
				/* mainley have no pin configuration */
				continue;
			}
			/* build suspend state */
			state_name = PINCTRL_STATE_SUSPEND;
			ret = sunxi_pinctrl_creat_mappings(pdev, pin_count, pin_list, device_name, state_name);
		}else{
			state_name = PINCTRL_STATE_SUSPEND;
			ret = sunxi_pinctrl_creat_suspend_mappings(pdev, pin_count, pin_list, device_name, state_name);
		}
		kfree(mainkey_suspend);
	}
	return 0;
}
#endif /* CONFIG_OF */

#ifdef CONFIG_EVB_PLATFORM
static void sunxi_gpio_irq_mask(struct irq_data *irqd)
{
	struct sunxi_pin_bank *bank    = irq_data_get_irq_chip_data(irqd);
	unsigned int           pin_idx = irqd->hwirq;
	unsigned int           pin     = bank->pin_base + pin_idx;
	unsigned long          val;
	unsigned long          mask;
	unsigned int  		 pin_bias;
	if(pin < SUNXI_PL_BASE){
		pin_bias = pin;
	}else{
		pin_bias = pin - SUNXI_PL_BASE;
	}
	
	/* disable hardware enable ctrl bit */
	val = pinctrl_readl_reg(bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	mask = EINT_CTRL_MASK << sunxi_eint_ctrl_offset(pin_bias);
	pinctrl_write_reg((val & ~mask), bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	/* disable bank enable bit */
	clear_bit(pin_idx, &(bank->eint_en));
	//pr_debug("sunxi mask [%s] irq[%d]\n",
	//        pin_get_name(bank->pinctrl->pctl_dev, pin), irqd->irq);
}

static void sunxi_gpio_irq_unmask(struct irq_data *irqd)
{
	struct sunxi_pin_bank *bank    = irq_data_get_irq_chip_data(irqd);
	unsigned int           pin_idx = irqd->hwirq;
	unsigned int           pin     = bank->pin_base + pin_idx;
	unsigned long          mask;
	unsigned int  		 pin_bias;

	pin_reset_bias(&pin_bias, pin);
	/* set hardware enable ctrl bit */
	mask = pinctrl_readl_reg(bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	mask |= EINT_CTRL_MASK << sunxi_eint_ctrl_offset(pin_bias);
	pinctrl_write_reg(mask, bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	/* set bank enable bit */
	set_bit(pin_idx, &(bank->eint_en));
}

static void sunxi_gpio_irq_enable(struct irq_data *irqd)
{
	struct sunxi_pin_bank *bank    = irq_data_get_irq_chip_data(irqd);
	unsigned int           pin_idx = irqd->hwirq;
	unsigned int           pin     = bank->pin_base + pin_idx;
	unsigned long          mask;
	unsigned int  		 pin_bias;

	pin_reset_bias(&pin_bias, pin);
	/*clear pending bit */
	pinctrl_write_reg(1 << pin_idx, bank->membase + sunxi_eint_status_reg(pin_bias));
	/* set hardware enable ctrl bit */
	mask = pinctrl_readl_reg(bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	mask |= EINT_CTRL_MASK << sunxi_eint_ctrl_offset(pin_bias);
	pinctrl_write_reg(mask, bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	/* set bank enable bit */
	set_bit(pin_idx, &(bank->eint_en));
}


static void sunxi_gpio_irq_ack(struct irq_data *irqd)
{
	struct sunxi_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	unsigned int           pin_idx = irqd->hwirq;
	unsigned int           pin     = bank->pin_base + pin_idx;
	unsigned int  		 pin_bias;

	pin_reset_bias(&pin_bias, pin);
	pinctrl_write_reg(1 << pin_idx, bank->membase + sunxi_eint_status_reg(pin_bias));
}

static int sunxi_gpio_irq_set_type(struct irq_data *irqd, unsigned int type)
{
	struct sunxi_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	unsigned int          pin = bank->pin_base + irqd->hwirq;
	unsigned int          trig_type;
	unsigned int          value;
	unsigned long         reg_offset;
	unsigned long         shift;
	const char            *pin_name;
	struct sunxi_desc_function *func;
	unsigned int  		 pin_bias;

	pin_reset_bias(&pin_bias, pin);
	/* find pin name by number */
	pin_name = pin_get_name(bank->pinctrl->pctl_dev, pin);
	if (IS_ERR_OR_NULL(pin_name)) {
		pr_debug("find pin [%d] name failed\n", pin);
		return -EINVAL;
	}
	/* check this pin have "eint" function or not */
	func = sunxi_pinctrl_desc_find_function_by_name(bank->pinctrl, 
	                                                pin_name, "eint");
	if (IS_ERR_OR_NULL(func)) {
		/* this pin have no "eint" function */
		pr_debug("the pin [%d] have no eint function\n", pin);
		return -EINVAL;
	}
	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		trig_type = SUNXI_PIN_EINT_POSITIVE_EDGE;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		trig_type = SUNXI_PIN_EINT_NEGATIVE_EDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		trig_type = SUNXI_PIN_EINT_DOUBLE_EDGE;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		trig_type = SUNXI_PIN_EINT_HIGN_LEVEL;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		trig_type = SUNXI_PIN_EINT_LOW_LEVEL;
		break;
	default:
		pr_err("sunxi unsupported external interrupt type to set\n");
		return -EINVAL;
	}

	if (type & IRQ_TYPE_EDGE_BOTH) {
		__irq_set_handler_locked(irqd->irq, handle_edge_irq);
	} else {
		__irq_set_handler_locked(irqd->irq, handle_level_irq);
	}
	
	/* config eint trigger type */
	reg_offset = sunxi_eint_cfg_reg(pin_bias);
	shift = sunxi_eint_cfg_offset(pin_bias);
	value = pinctrl_readl_reg(bank->membase + reg_offset);
	value &= ~(EINT_CFG_MASK << shift);
	value |= trig_type << shift;
	pinctrl_write_reg(value, bank->membase + reg_offset);

	/* config pin as eint functions */
	reg_offset = sunxi_mux_reg(pin_bias);
	shift = sunxi_mux_offset(pin_bias);
	value = pinctrl_readl_reg(bank->membase + reg_offset);
	value &= ~(MUX_PINS_MASK << shift);
	value |= func->muxval << shift;
	pinctrl_write_reg(value, bank->membase + reg_offset);
	
	return 0;
}

/*
 * irq_chip for gpio interrupts.
 */
static struct irq_chip sunxi_gpio_irq_chip = {
	.name		= "sunxi_gpio_irq_chip",
	.irq_unmask	= sunxi_gpio_irq_unmask,
	.irq_mask	= sunxi_gpio_irq_mask,
	.irq_enable	= sunxi_gpio_irq_enable,
	.irq_ack	= sunxi_gpio_irq_ack,
	.irq_set_type	= sunxi_gpio_irq_set_type,
};
static irqreturn_t sunxi_gpio_irq_handler(int irq, void *dev_id)
{
	struct sunxi_pin_bank *bank  = dev_id;
	unsigned long         reg_offset;
	unsigned int          pin = 0;
	unsigned              offset;
	unsigned long         events;
	unsigned long         status;
	unsigned long         mask;
	unsigned int		 pin_bias;
	
	if(bank->pin_base < SUNXI_PL_BASE){
		pin_bias = bank->pin_base;
	}else{
		pin_bias = bank->pin_base - SUNXI_PL_BASE;
	}
	/* read out eint status(pending) register value */
	reg_offset = sunxi_eint_status_reg(pin_bias);
	status = pinctrl_readl_reg(bank->membase + reg_offset);
	mask = pinctrl_readl_reg(bank->membase + sunxi_eint_ctrl_reg(pin_bias));
	
	/* mask the non-enable eint */
	events = status & mask;
	
	/* process all valid pins interrupt */
	for_each_set_bit(offset, &events, 32) {
		pin = bank->pin_base + offset;
		generic_handle_irq(irq_linear_revmap(bank->irq_domain, (pin - bank->pin_base)));
	}
	return events ? IRQ_HANDLED : IRQ_NONE;
}

static int sunxi_gpio_irq_map(struct irq_domain *h, 
			      unsigned int virq,
			      irq_hw_number_t hw)
{
	struct sunxi_pin_bank *bank = h->host_data;

	irq_set_chip_data(virq, bank);
	irq_set_chip_and_handler(virq, &sunxi_gpio_irq_chip, handle_level_irq);
	set_irq_flags(virq, IRQF_VALID);
	
	return 0;
}

/*
 * irq domain callbacks for external gpio interrupt controller.
 */
static const struct irq_domain_ops sunxi_gpio_irqd_ops = {
	.map	= sunxi_gpio_irq_map,
	.xlate	= irq_domain_xlate_twocell,
};

static int sunxi_eint_gpio_init(struct platform_device *pdev)
{
	struct device         *dev   = &pdev->dev;
	struct sunxi_pinctrl  *pctrl = platform_get_drvdata(pdev);
	struct sunxi_pin_bank *bank;
	int                   err;
	int                   i;
	
	for (i = 0; i < pctrl->desc->nbanks; i++) {
		bank = &(pctrl->desc->banks[i]);
		bank->pinctrl = pctrl;
		bank->eint_en = 0;
		if (bank->eint_type == SUNXI_EINT_TYPE_NONE) {
			/* this bank not support external interrupt, skip it */
			continue;
		}
		bank->irq_domain = irq_domain_add_linear(NULL, bank->nr_pins,
			            &sunxi_gpio_irqd_ops, bank);
		if (IS_ERR_OR_NULL(bank->irq_domain)) {
			dev_err(dev, "could not create IRQ domain\n");
			return -ENOMEM;
		}
		/* request pin irq */
#if defined(CONFIG_ARCH_SUN8IW7P1)
		if(bank->pin_base == SUNXI_PL_BASE)
			err = devm_request_irq(dev, bank->irq, sunxi_gpio_irq_handler,
				       IRQF_SHARED | IRQF_NO_SUSPEND, bank->name, bank);
		else{

			err = devm_request_irq(dev, bank->irq, sunxi_gpio_irq_handler,
					IRQF_SHARED, bank->name, bank);

		}
#elif defined(CONFIG_ARCH_SUN8IW8P1)
				if(bank->pin_base == SUNXI_PB_BASE || bank->pin_base == SUNXI_PG_BASE) {
					err = devm_request_irq(dev, bank->irq, sunxi_gpio_irq_handler,
						       IRQF_SHARED | IRQF_NO_SUSPEND, bank->name, bank);
				} else {

					err = devm_request_irq(dev, bank->irq, sunxi_gpio_irq_handler,
							IRQF_SHARED, bank->name, bank);

				}

#else
		err = devm_request_irq(dev, bank->irq, sunxi_gpio_irq_handler, 
				       IRQF_SHARED, bank->name, bank);
#endif
		if (IS_ERR_VALUE(err)) {
			dev_err(dev, "unable to request eint irq %d\n", bank->irq);
			return err;
		}
	}
	return 0;
}
#endif /* config evb plotform */


#ifdef CONFIG_DEBUG_FS
static int sunxi_pin_configure_show(struct seq_file *s, void *d)
{
	int						pin;
	int						function;
	int						data;
	int						dlevel;
	int						pull;
	int						trigger;
	int						mask;
	int						pending;
	unsigned int			value;
	unsigned long			reg_offset;
	unsigned long			shift;
	char 					*dev_name="sunxi-pinctrl";
	struct pinctrl_dev 		*pctldev;
	struct sunxi_pinctrl 	*pctl;
	struct sunxi_pin_bank 	*bank;
	unsigned int  		 pin_bias;
	/* get pinctrl device */
	pctldev = get_pinctrl_dev_from_devname(dev_name);
	if (!pctldev) {
		seq_printf(s, "cannot get pinctrl device from devname\n");
		return -EINVAL;
	}
	/* change pin name to pin index */
	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0){
		seq_printf(s, "unvalid pin for debugfs\n");
		return -EINVAL;
	}
	pctl = pinctrl_dev_get_drvdata(pctldev);
	bank = sunxi_pin_to_bank(pctl, pin);
	/*modified pin bias */

	pin_reset_bias(&pin_bias, pin);
	seq_printf(s, "printf register value...\n");
	/*get pin func*/
	value = pinctrl_readl_reg(bank->membase + sunxi_mux_reg(pin_bias));
	function = (value >> sunxi_mux_offset(pin_bias)) & MUX_PINS_MASK;
	seq_printf(s, "pin[%s] function:  %x;                 register addr: 0x%p\n"
		, sunxi_dbg_pinname,function,(bank->membase + sunxi_mux_reg(pin_bias)));
	/*get pin data*/
	value = pinctrl_readl_reg(bank->membase + sunxi_data_reg(pin_bias));
	data = (value >> sunxi_data_offset(pin_bias)) & DATA_PINS_MASK;
	seq_printf(s, "pin[%s] data:  %x(default value : 0);  register addr: 0x%p\n"
		, sunxi_dbg_pinname,data,(bank->membase + sunxi_data_reg(pin_bias)));
	/*get pin dlevel*/
	value = pinctrl_readl_reg(bank->membase + sunxi_dlevel_reg(pin_bias));
	dlevel = (value >> sunxi_dlevel_offset(pin_bias)) & DLEVEL_PINS_MASK;
	seq_printf(s, "pin[%s] dleve: %x(default value : 1);  register addr: 0x%p\n"
		, sunxi_dbg_pinname,dlevel,(bank->membase + sunxi_dlevel_reg(pin_bias)));
	/*get pin pull*/
	value = pinctrl_readl_reg(bank->membase + sunxi_pull_reg(pin_bias));
	pull = (value >> sunxi_pull_offset(pin_bias)) & PULL_PINS_MASK;
	seq_printf(s, "pin[%s] pull:  %x(default value : 0);  register addr: 0x%p\n"
		, sunxi_dbg_pinname,pull,(bank->membase + sunxi_pull_reg(pin_bias)));
	/*for enit pin,print enit configure value*/
	if (bank->eint_type == SUNXI_EINT_TYPE_GPIO){
		/* config eint trigger type */
		reg_offset = sunxi_eint_cfg_reg(pin_bias);
		shift = sunxi_eint_cfg_offset(pin_bias);
		value = pinctrl_readl_reg(bank->membase + reg_offset);
		trigger =(value>>shift) &  EINT_CFG_MASK;
		seq_printf(s, "\npin[%s] trigger:  %x;                  register addr: 0x%p\n"
			, sunxi_dbg_pinname,trigger,(bank->membase + reg_offset));
		seq_printf(s, "trigger mode in sunxi platform:       \n");
		seq_printf(s, "0 : Positive edge   1: Negative edge  \n");
		seq_printf(s, "2 : High level      3: Low level      \n");
		seq_printf(s, "4 : Double level    5: other          \n");

		/*  eint mask  */
		reg_offset = sunxi_eint_ctrl_reg(pin_bias);
		shift = sunxi_eint_ctrl_offset(pin_bias);
		value = pinctrl_readl_reg(bank->membase + reg_offset);
		mask =(value>>shift) &  EINT_CTRL_MASK;
		seq_printf(s, "pin[%s] mask :     %x(0:disable 1:enable);        register addr: 0x%p\n"
			, sunxi_dbg_pinname,mask,(bank->membase + reg_offset));

		/*  eint pending  */
		reg_offset = sunxi_eint_status_reg(pin_bias);
		shift = sunxi_eint_status_offset(pin_bias);
		value = pinctrl_readl_reg(bank->membase + reg_offset);
		pending =(value>>shift) &  EINT_STATUS_MASK;
		seq_printf(s, "pin[%s] pending :  %x(0:No Pending 1:Pending);    register addr: 0x%p\n"
			, sunxi_dbg_pinname,pending,(bank->membase + reg_offset));

		return 0;
	}

	return 0;

}

static int sunxi_pin_configure_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int						err;
	unsigned int			pin;
	unsigned int			function;
	unsigned int			data;
	unsigned int			pull;
	unsigned int 			dlevel;	
	unsigned int			trigger;
	unsigned int			value;
	unsigned long			reg_offset;
	unsigned long			shift;
	unsigned long            mask;
	char 					*dev_name = "sunxi-pinctrl";
	struct pinctrl_dev 		*pctldev;
	struct sunxi_pinctrl 	*pctl;
	struct sunxi_pin_bank 	*bank;
	unsigned int  		 	pin_bias;
	
	err=sscanf(user_buf, "%s %u %u %u %u %u", sunxi_dbg_pinname,
		&function,&data,&dlevel,&pull,&trigger);
	if(err != 6)
		return -EINVAL;
	if (function <= 7)
		sunxi_dbg_function = function;
	else{
		pr_debug("Input Parameters function error!\n");
		return -EINVAL;
	}
	if (data <= 1)
		sunxi_dbg_data = data;
	else{
		pr_debug("Input Parameters data error!\n");
		return -EINVAL;
	}
	if (pull <= 3)
		sunxi_dbg_pull = pull;
	else{
		pr_debug("Input Parameters pull error!\n");
		return -EINVAL;
	}
	if (dlevel <=3 )
		sunxi_dbg_level = dlevel;
	else{
		pr_debug("Input Parameters dlevel error!\n");
		return -EINVAL;
	}
	if (trigger <= 7)
		sunxi_dbg_trigger = trigger;
	else{
		pr_debug("Input Parameters trigger error!\n");
		return -EINVAL;
	}
	pctldev = get_pinctrl_dev_from_devname(dev_name);
	if (!pctldev) {
		return -EINVAL;
	}
	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0){
		return -EINVAL;
	}
	pctl = pinctrl_dev_get_drvdata(pctldev);
	bank = sunxi_pin_to_bank(pctl, pin);

	pin_reset_bias(&pin_bias, pin);
	/* set function value*/
	value = pinctrl_readl_reg(bank->membase + sunxi_mux_reg(pin_bias));
	mask = MUX_PINS_MASK << sunxi_mux_offset(pin_bias);
	value=(value & ~mask) | (function << sunxi_mux_offset(pin_bias));
	pinctrl_write_reg(value,bank->membase + sunxi_mux_reg(pin_bias));
	pr_debug("pin[%s] set function:     %x;     register addr: 0x%p\n"
		, sunxi_dbg_pinname,function,(bank->membase + sunxi_mux_reg(pin_bias)));
	/* set data value*/
	value = pinctrl_readl_reg(bank->membase + sunxi_data_reg(pin_bias));
	mask = DATA_PINS_MASK << sunxi_data_offset(pin_bias);
	value=(value & ~mask) | (data << sunxi_data_offset(pin_bias));
	pinctrl_write_reg(value,bank->membase + sunxi_data_reg(pin_bias));
	pr_debug("pin[%s] set data:         %x;     register addr: 0x%p\n"
		, sunxi_dbg_pinname,data,(bank->membase + sunxi_data_reg(pin_bias)));
	/* set dlevel value*/
	value = pinctrl_readl_reg(bank->membase + sunxi_dlevel_reg(pin_bias));
	mask = DLEVEL_PINS_MASK << sunxi_dlevel_offset(pin_bias);
	value=(value & ~mask) | (dlevel << sunxi_dlevel_offset(pin_bias));
	pinctrl_write_reg(value,bank->membase + sunxi_dlevel_reg(pin_bias));
	pr_debug("pin[%s] set dleve:        %x;     register addr: 0x%p\n"
		, sunxi_dbg_pinname,dlevel,(bank->membase + sunxi_dlevel_reg(pin_bias)));
	/* set pull value*/
	value = pinctrl_readl_reg(bank->membase + sunxi_pull_reg(pin_bias));
	mask = PULL_PINS_MASK << sunxi_pull_offset(pin_bias);
	value=(value & ~mask) | (pull << sunxi_pull_offset(pin_bias));
	pinctrl_write_reg(value,bank->membase + sunxi_pull_reg(pin_bias));
	pr_debug("pin[%s] set pull:         %x;     register addr: 0x%p\n"
		, sunxi_dbg_pinname,pull,(bank->membase + sunxi_pull_reg(pin_bias)));
	if (bank->eint_type == SUNXI_EINT_TYPE_GPIO){
		reg_offset = sunxi_eint_cfg_reg(pin_bias);
		shift = sunxi_eint_cfg_offset(pin_bias);
		value = pinctrl_readl_reg(bank->membase + reg_offset);
		value &= ~(EINT_CFG_MASK << shift);
		value |= trigger << shift;
		pinctrl_write_reg(value, bank->membase + reg_offset);
		pr_debug("pin[%s] set trigger :     %x;     register addr: 0x%p\n"
			, sunxi_dbg_pinname,trigger,(bank->membase + reg_offset));
		return count;
	}
	return count;
	
}
static int sunxi_pin_show(struct seq_file *s, void *d)
{
	if (strlen(sunxi_dbg_pinname))
		seq_printf(s, "%s\n", sunxi_dbg_pinname);
	else
		seq_printf(s, "No pin name set\n");
	return 0;
}

static int sunxi_pin_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;

	if (count > SUNXI_MAX_NAME_LEN)
		return -EINVAL;

	err = sscanf(user_buf, "%19s", sunxi_dbg_pinname);
	if (err != 1)
		return -EINVAL;

	return count;
}

static int sunxi_pin_function_show(struct seq_file *s, void *d)
{	
	long unsigned int 		config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,0XFFFF);
	pin_config_get(SUNXI_PINCTRL,sunxi_dbg_pinname,&config);
	seq_printf(s, "pin[%s] funciton: %lx\n", sunxi_dbg_pinname,
					SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;
}

static int sunxi_pin_function_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int						err;
	unsigned int			function;
	long unsigned int 		config;
	err = sscanf(user_buf, "%s %u", sunxi_dbg_pinname,&function);
	if(err != 2 )
		return err;
	if (function <= 7)
		sunxi_dbg_function = function;
	else{
		pr_debug("Input Parameters function error!\n");
		return -EINVAL;
	}
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,function);
	pin_config_set(SUNXI_PINCTRL,sunxi_dbg_pinname,config);
	return count;
}
static int sunxi_pin_data_show(struct seq_file *s, void *d)
{
	long unsigned int 		config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT,0XFFFF);
	pin_config_get(SUNXI_PINCTRL,sunxi_dbg_pinname,&config);
	seq_printf(s, "pin[%s] data: %lx\n", sunxi_dbg_pinname,
					SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;

}

static int sunxi_pin_data_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int						err;
	unsigned int			data;
	long unsigned int 		config;
	err = sscanf(user_buf, "%s %u", sunxi_dbg_pinname,&data);
	if(err != 2 )
		return err;
	if (data <= 1)
		sunxi_dbg_data = data;
	else{
		pr_debug("Input Parameters data error!\n");
		return -EINVAL;
	}
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT,data);
	pin_config_set(SUNXI_PINCTRL,sunxi_dbg_pinname,config);
	
	return count;

}
static int sunxi_pin_dlevel_show(struct seq_file *s, void *d)
{
	long unsigned int 		config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV,0XFFFF);
	pin_config_get(SUNXI_PINCTRL,sunxi_dbg_pinname,&config);
	seq_printf(s, "pin[%s] dlevel: %lx\n", sunxi_dbg_pinname,
					SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;

}

static int sunxi_pin_dlevel_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int						err;
	unsigned int			dlevel;
	long unsigned int 		config;
	err = sscanf(user_buf, "%s %u", sunxi_dbg_pinname,&dlevel);
	if(err != 2 )
		return err;
	if (dlevel <= 3)
		sunxi_dbg_level = dlevel;
	else{
		pr_debug("Input Parameters dlevel error!\n");
		return -EINVAL;
	}
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV,dlevel);
	pin_config_set(SUNXI_PINCTRL,sunxi_dbg_pinname,config);
	
	return count;

}
static int sunxi_pin_pull_show(struct seq_file *s, void *d)
{
	long unsigned int 		config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD,0XFFFF);
	pin_config_get(SUNXI_PINCTRL,sunxi_dbg_pinname,&config);
	seq_printf(s, "pin[%s] pull: %lx\n", sunxi_dbg_pinname,
					SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;

}

static int sunxi_platform_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	return count;
}

static int sunxi_platform_show(struct seq_file *s, void *d)
{

#if defined(CONFIG_ARCH_SUN8IW1)
	seq_printf(s, "SUN8IW1\n");
#elif defined(CONFIG_ARCH_SUN8IW3)
	seq_printf(s, "SUN8IW3\n");
#elif defined(CONFIG_ARCH_SUN8IW5)
	seq_printf(s, "SUN8IW5\n");
#elif defined(CONFIG_ARCH_SUN8IW6)
	seq_printf(s, "SUN8IW6\n");
#elif defined(CONFIG_ARCH_SUN8IW7)
	seq_printf(s, "SUN8IW7\n");
#elif defined(CONFIG_ARCH_SUN8IW8)
	seq_printf(s, "SUN8IW8\n");
#elif defined(CONFIG_ARCH_SUN8IW9)
	seq_printf(s, "SUN8IW9\n");
#elif defined(CONFIG_ARCH_SUN9IW1)
	seq_printf(s, "SUN9IW1\n");
#else
	seq_printf(s, "NOT MATCH\n");
#endif

	return 0;

}


static int sunxi_pin_pull_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int						err;
	unsigned int			pull;
	long unsigned int 		config;
	err=sscanf(user_buf, "%s %u", sunxi_dbg_pinname,&pull);
	if(err != 2 )
		return err;
	if (pull <= 3)
		sunxi_dbg_pull= pull;
	else{
		pr_debug("Input Parameters pull error!\n");
		return -EINVAL;
	}
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD,pull);
	pin_config_set(SUNXI_PINCTRL,sunxi_dbg_pinname,config);
	
	return count;

}

static int sunxi_pin_configure_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_configure_show, inode->i_private);
}
static int sunxi_pin_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_show, inode->i_private);
}
static int sunxi_pin_function_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_function_show, inode->i_private);
}
static int sunxi_pin_data_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_data_show, inode->i_private);
}
static int sunxi_pin_dlevel_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_dlevel_show, inode->i_private);
}
static int sunxi_pin_pull_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_pull_show, inode->i_private);
}
static int sunxi_platform_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_platform_show, inode->i_private);
}


static const struct file_operations sunxi_pin_configure_ops = {
	.open		= sunxi_pin_configure_open,
	.write		= sunxi_pin_configure_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations sunxi_pin_ops = {
	.open		= sunxi_pin_open,
	.write		= sunxi_pin_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations sunxi_pin_function_ops = {
	.open		= sunxi_pin_function_open,
	.write		= sunxi_pin_function_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations sunxi_pin_data_ops = {
	.open		= sunxi_pin_data_open,
	.write		= sunxi_pin_data_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations sunxi_pin_dlevel_ops = {
	.open		= sunxi_pin_dlevel_open,
	.write		= sunxi_pin_dlevel_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations sunxi_pin_pull_ops = {
	.open		= sunxi_pin_pull_open,
	.write		= sunxi_pin_pull_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations sunxi_platform_ops = {
	.open		= sunxi_platform_open,
	.write		= sunxi_platform_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};


static struct dentry *debugfs_root;
static void sunxi_pinctrl_debugfs(void)
{
	debugfs_root = debugfs_create_dir("sunxi_pinctrl", NULL);
	if (IS_ERR(debugfs_root) || !debugfs_root) {
		pr_debug("failed to create debugfs directory\n");
		debugfs_root = NULL;
		return;
	}
	debugfs_create_file("sunxi_pin_configure",(S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_configure_ops);
	debugfs_create_file("sunxi_pin", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_ops);
	debugfs_create_file("function", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_function_ops);
	debugfs_create_file("data", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_data_ops);
	debugfs_create_file("dlevel", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_dlevel_ops);
	debugfs_create_file("pull", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_pull_ops);
	debugfs_create_file("platform", (S_IRUGO | S_IWUSR | S_IWGRP),
				debugfs_root, NULL, &sunxi_platform_ops);

}
#endif

static int sunxi_pinctrl_probe(struct platform_device *pdev)
{
#if defined(CONFIG_OF)
	struct device_node *node = pdev->dev.of_node;
	const struct of_device_id *device;
#endif
	struct pinctrl_pin_desc *pins;
	struct sunxi_pinctrl *pctl;
	int i, ret, last_pin;
	struct clk *clk;
    	
    	pr_debug("sunxi pinctrl probe enter\n");
    	if (0) {
    		volatile u32 dbg = 0x5a;
    		unsigned long flags;
		local_irq_save(flags);
    		while (dbg == 0x5a);
    	}
    	
	pctl = devm_kzalloc(&pdev->dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;
	platform_set_drvdata(pdev, pctl);

#if defined(CONFIG_OF)
	pctl->membase = of_iomap(node, 0);
	if (!pctl->membase)
		return -ENOMEM;

	device = of_match_device(sunxi_pinctrl_match, &pdev->dev);
	if (!device)
		return -ENODEV;

	pctl->desc = (struct sunxi_pinctrl_desc *)device->data;
#else
	pctl->desc = (struct sunxi_pinctrl_desc *)(&sunxi_pinctrl_data);
#endif /* CONFIG_OF */

	ret = sunxi_pinctrl_build_state(pdev);
	if (ret) {
		dev_err(&pdev->dev, "dt probe failed: %d\n", ret);
		return ret;
	}

	pins = devm_kzalloc(&pdev->dev,
			    pctl->desc->npins * sizeof(*pins),
			    GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0; i < pctl->desc->npins; i++)
		pins[i] = pctl->desc->pins[i].pin;
	
	sunxi_pctrl_desc.name = dev_name(&pdev->dev);
	sunxi_pctrl_desc.owner = THIS_MODULE;
	sunxi_pctrl_desc.pins = pins;
	sunxi_pctrl_desc.npins = pctl->desc->npins;
	pctl->dev = &pdev->dev;
	pctl->pctl_dev = pinctrl_register(&sunxi_pctrl_desc,
					  &pdev->dev, pctl);
	if (!pctl->pctl_dev) {
		dev_err(&pdev->dev, "couldn't register pinctrl driver\n");
		return -EINVAL;
	}
	last_pin = pctl->desc->pins[pctl->desc->npins - 1].pin.number;
	pctl->chip = sunxi_pinctrl_gpio_chip;
	pctl->chip.ngpio = round_up(last_pin, PINS_PER_BANK);
	pctl->chip.label = dev_name(&pdev->dev);
	pctl->chip.dev = &pdev->dev;
	pctl->chip.base = 0;

	ret = gpiochip_add(&(pctl->chip));
	if (ret) {
		goto pinctrl_error;
	}

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		ret = gpiochip_add_pin_range(&(pctl->chip), dev_name(&pdev->dev),
					     pin->pin.number,
					     pin->pin.number, 1);
		if (ret) {
			goto gpiochip_error;
		}
	}
		
	/* setup sunxi pin configurations */
#ifndef CONFIG_OF
	sunxi_pinctrl_parse_pin_cfg(pdev);
#endif
	clk = devm_clk_get(&pdev->dev, "pio");
	if (IS_ERR(clk))
		goto gpiochip_error;
	clk_prepare_enable(clk);
#ifdef CONFIG_EVB_PLATFORM
	ret = sunxi_eint_gpio_init(pdev);
	if (IS_ERR_VALUE(ret)) {
		dev_err(&pdev->dev, "sunxi init eint failed\n");
		goto gpiochip_error;
	}
#endif
	dev_info(&pdev->dev, "initialized sunXi PIO driver\n");

#ifdef CONFIG_DEBUG_FS
	sunxi_pinctrl_debugfs();
#endif

	return 0;

gpiochip_error:
	ret = gpiochip_remove(&(pctl->chip));
pinctrl_error:
	pinctrl_unregister(pctl->pctl_dev);
	return ret;
}

static struct platform_driver sunxi_pinctrl_driver = {
	.probe = sunxi_pinctrl_probe,
	.driver = {
		.name  = SUNXI_PINCTRL,
		.owner = THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table = sunxi_pinctrl_match,
#endif
	},
};

static struct platform_device sunxi_pinctrl_device = {
	.name = SUNXI_PINCTRL,
	.id = PLATFORM_DEVID_NONE, /* this is only one device for pinctrl driver */
};

static int __init sunxi_pinctrl_init(void)
{
	int ret;
	ret = platform_device_register(&sunxi_pinctrl_device);
	if (IS_ERR_VALUE(ret)) {
		pr_debug("register sunxi platform device failed\n");
		return -EINVAL;
	}
	ret = platform_driver_register(&sunxi_pinctrl_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_debug("register sunxi platform device failed\n");
		return -EINVAL;
	}
	return 0;
}
postcore_initcall(sunxi_pinctrl_init);

MODULE_AUTHOR("sunny <sunny@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner A1X pinctrl driver");
MODULE_LICENSE("GPL");
