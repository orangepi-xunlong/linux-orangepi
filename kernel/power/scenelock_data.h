
#ifndef _LINUX_SCENELOCK_DATA_H
#define _LINUX_SCENELOCK_DATA_H

#include <linux/power/axp_depend.h>

#if defined CONFIG_ARCH_SUN8IW1P1
scene_extended_standby_t extended_standby[8] = {
	{
		{
			.id           	= TALKING_STANDBY_FLAG,
			.pwr_dm_en      = 0xf3,      //mean gpu, cpu is powered off.
			.osc_en         = 0xf,
			.init_pll_dis   = (~(0x12)), //mean pll2 is on. pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x21,      //mean enable pll1 and pll6
			.pll_change     = 0x0,
			.bus_change     = 0x0,
		},
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
	},
	{
		{
			.id  		= USB_STANDBY_FLAG,
			.pwr_dm_en      = 0xf3,      //mean gpu, cpu is powered off.
			.osc_en         = 0xf ,      //mean all osc is powered on.
			.init_pll_dis   = (~(0x30)), //mean pll6 is on.pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x1,       //mean enable pll1 and pll6
			.pll_change     = 0x20,
			.pll_factor[5]  = {0x18,0x1,0x1,0},
			.bus_change     = 0x4,
			.bus_factor[2]  = {0x8,0,0x3,0,0},
		},
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
	},
	{
		{
			.id  		= MP3_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
	},
	{
		{
			.id		= BOOT_FAST_STANDBY_FLAG,
		},
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
	},
	{
		{
		.id             = SUPER_STANDBY_FLAG,
		.pwr_dm_en      = 0xa1,      //mean avcc, dram, cpus is on.
		.osc_en         = 0x4,	// mean losc is on.
		.init_pll_dis   = (~(0x10)), //mean pll5 is shundowned by dram driver.
		.exit_pll_en    = 0x0,
		.pll_change     = 0x0,
		.bus_change     = 0x0,
		},
		.scene_type	= SCENE_SUPER_STANDBY,
		.name		= "super_standby",
	},
	{
		{
			.id             = NORMAL_STANDBY_FLAG,
			.pwr_dm_en      = 0xfff,     	 //mean all power domain is on.
			.osc_en         = 0xf,		// mean Hosc&Losc&ldo&ldo1 is on.
			.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
			.exit_pll_en    = (~(0x10)),
			.pll_change     = 0x1,
			.pll_factor[0]  = {0x10,0,0,0},
			.bus_change     = 0x5,	             //ahb1&apb2 is changed.
			.bus_factor[0]  = {0x2,0,0,0,0},     //apb2 src is Hosc.
			.bus_factor[2]  = {0x2,0,0,0,0},     //ahb1 src is Hosc.
		},
		.scene_type	= SCENE_NORMAL_STANDBY,
		.name		= "normal_standby",
	},
	{
		{
			.id             = GPIO_STANDBY_FLAG,
			.pwr_dm_en      = 0xb3,     	 //mean avcc, dram, sys, io, cpus is on.
			.osc_en         = 0x4,		// mean losc is on.
			.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x0,
			.pll_change     = 0x0,
			.bus_change     = 0x4,	//ahb1 is changed.
			.bus_factor[2]  = {0x0,0,0,0,0},     //src is losc.
		},
		.scene_type	= SCENE_GPIO_STANDBY,
		.name		= "gpio_standby",
	},
	{
		{
			.id		= MISC_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
	},
};
#elif defined CONFIG_ARCH_SUN8IW3P1
scene_extended_standby_t extended_standby[8] = {
	 {
	 	{
			.id           	= TALKING_STANDBY_FLAG,
			.pwr_dm_en      = 0xf7,      //mean cpu is powered off.
			.osc_en         = 0xf,
			.init_pll_dis   = (~(0x12)), //mean pll2 is on. pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x21,      //mean enable pll1 and pll6
			.pll_change     = 0x0,
			.bus_change     = 0x0,
	 	},
	 	.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
	},
	{
		{
			.id  		= USB_STANDBY_FLAG,
			.pwr_dm_en      = 0xf7,      //mean cpu is powered off.
			.osc_en         = 0xf ,      //mean all osc is powered on.
			.init_pll_dis   = (~(0x30)), //mean pll6 is on.pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x1,       //mean enable pll1 and pll6
			.pll_change     = 0x20,
			.pll_factor[5]  = {0x1,0x2,0,0},        //pll6 is 24M
			.bus_change     = 0x14,
			.bus_factor[2]  = {0,0,0,0,0},      //switch AHB1 src to IOSC
			.bus_factor[4]  = {0,0,0,0,0},      //switch cpu/axi src to IOSC
	 	},
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
	},
	{
		{
			.id  		= MP3_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
	},
	{
		{
			.id  		= BOOT_FAST_STANDBY_FLAG,
		},
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
	},
	{
		{
			.id             = SUPER_STANDBY_FLAG,
			.pwr_dm_en      = 0xa1,      //mean avcc, dram, cpus is on.
			.osc_en         = 0x4,	     // mean losc is on.
			.init_pll_dis   = (~(0x10)), //mean pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x0,
			.pll_change     = 0x0,
			.bus_change     = 0x0,
		},
		.scene_type	= SCENE_SUPER_STANDBY,
		.name		= "super_standby",
	},
	{
		{
			.id             = NORMAL_STANDBY_FLAG,
			.pwr_dm_en      = 0xfff,     	 //mean all power domain is on.
			.osc_en         = 0xf,		// mean Hosc&Losc&ldo&ldo1 is on.
			.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
			.exit_pll_en    = (~(0x10)),
			.pll_change     = 0x1,
			.pll_factor[0]  = {0x10,0,0,0},
			.bus_change     = 0x5,	             //ahb1&apb2 is changed.
			.bus_factor[0]  = {0x2,0,0,0,0},     //apb2 src is Hosc.
			.bus_factor[2]  = {0x2,0,0,0,0},     //ahb1 src is Hosc.
		},
		.scene_type	= SCENE_NORMAL_STANDBY,
		.name		= "normal_standby",
	},
	{
		{
			.id             = GPIO_STANDBY_FLAG,
			.pwr_dm_en      = 0xb3,     	 //mean avcc, dram, sys, io, cpus is on.
			.osc_en         = 0x4,		// mean losc is on.
			.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x0,
			.pll_change     = 0x0,
			.bus_change     = 0x4,	//ahb1 is changed.
			.bus_factor[2]  = {0x0,0,0,0,0},     //src is losc.
		},
		.scene_type	= SCENE_GPIO_STANDBY,
		.name		= "gpio_standby",
	},
	{
		{
			.id		= MISC_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
	},
};
#elif defined CONFIG_ARCH_SUN8IW5P1	// FIXME: need double check.
scene_extended_standby_t extended_standby[8] = {
	 {
	 	{
			.id           	= TALKING_STANDBY_FLAG,
			.pwr_dm_en      = 0xf7,      //mean cpu is powered off.
			.osc_en         = 0xf,
			.init_pll_dis   = (~(0x12)), //mean pll2 is on. pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x21,      //mean enable pll1 and pll6
			.pll_change     = 0x0,
			.bus_change     = 0x0,
		},
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
	},
	{
		{
			.id  		= USB_STANDBY_FLAG,
			.pwr_dm_en      = 0xf7,      //mean cpu is powered off.
			.osc_en         = 0xf ,      //mean all osc is powered on.
			.init_pll_dis   = (~(0x30)), //mean pll6 is on.pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x1,       //mean enable pll1 and pll6
			.pll_change     = 0x20,
			.pll_factor[5]  = {0x1,0x2,0,0},        //pll6 is 24M
			.bus_change     = 0x14,
			//.bus_factor[2]  = {0x2,0x2,0x3,0,0},      //ahb1 src is 1M.
			//.bus_factor[4]  = {0x2,0x0,0x0,0,0},      //axi src is 24M.
			.bus_factor[2]  = {0,0,0,0,0},      //switch AHB1 src to IOSC
			.bus_factor[4]  = {0,0,0,0,0},      //switch cpu/axi src to IOSC
		},
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
	},
	{
		{
			.id  		= MP3_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
	},
	{
		{
			.id  		= BOOT_FAST_STANDBY_FLAG,
		},
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
	},
	{
		{
			.id             = SUPER_STANDBY_FLAG,
			.pwr_dm_en      = 0xa1,      //mean avcc, dram, cpus is on.
			.osc_en         = 0x4,	// mean losc is on.
			.init_pll_dis   = (~(0x10)), //mean pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x0,
			.pll_change     = 0x0,
			.bus_change     = 0x0,
		},
		.scene_type	= SCENE_SUPER_STANDBY,
		.name		= "super_standby",
	},
	{
		{
			.id             = NORMAL_STANDBY_FLAG,
			.pwr_dm_en      = 0xfff,     	 //mean all power domain is on.
			.osc_en         = 0xf,		// mean Hosc&Losc&ldo&ldo1 is on.
			.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
			.exit_pll_en    = (~(0x10)),
			.pll_change     = 0x1,
			.pll_factor[0]  = {0x10,0,0,0},
			.bus_change     = 0x5,	             //ahb1&apb2 is changed.
			.bus_factor[0]  = {0x2,0,0,0,0},     //apb2 src is Hosc.
			.bus_factor[2]  = {0x2,0,0,0,0},     //ahb1 src is Hosc.
		},
		.scene_type	= SCENE_NORMAL_STANDBY,
		.name		= "normal_standby",
	},
	{
		{
			.id             = GPIO_STANDBY_FLAG,
			.pwr_dm_en      = 0xb3,     	 //mean avcc, dram, sys, io, cpus is on.
			.osc_en         = 0x4,		// mean losc is on.
			.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
			.exit_pll_en    = 0x0,
			.pll_change     = 0x0,
			.bus_change     = 0x4,
			.bus_factor[2]  = {0x0,0,0,0,0},     //src is losc.
		},
		.scene_type	= SCENE_GPIO_STANDBY,
		.name		= "gpio_standby",
	},
	{
		{
			.id		= MISC_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
	},
};
#elif defined CONFIG_ARCH_SUN8IW6P1	// FIXME: need double check.
scene_extended_standby_t extended_standby[] = {
	{
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
		.soc_pwr_dep.id             = TALKING_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT), 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
#if 0		
		//for pb port
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c20824, 0x0000ffff, 0x00007777},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c20828, 0x00000ff0, 0x00000770},
#endif

	},
	
	{
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
		.soc_pwr_dep.id  		= USB_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_ldoin, vdd_sys is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT) |\
							     BITMAP(VDD_SYS_BIT),
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = BITMAP(OSC_LOSC_BIT) | BITMAP(OSC_HOSC_BIT) | BITMAP(OSC_LDO1_BIT) | BITMAP(OSC_LDO0_BIT),	// mean all osc is off. +losc, +hosc
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM) | BITMAP(PM_PLL_PERIPH), //mean pll5 is shutdowned & open by dram driver.
													    //hsic pll can be disabled, cpus can change cci400 clk from hsic_pll.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = BITMAP(PM_PLL_PERIPH),
		.soc_pwr_dep.cpux_clk_state.pll_factor[PM_PLL_PERIPH] = { ////PLL_PERIPH freq = 24*12/2/2= 72M
		    .factor1 = 12, //N=12
		    .factor2 = 1, //Div1 = 1 + 1 = 2
		    .factor3 = 1, //Div2 = 1 + 1 = 2
		},
		.soc_pwr_dep.cpux_clk_state.bus_change     = BITMAP(BUS_AHB1) | BITMAP(BUS_AHB2),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1]     = {
		    .src = CLK_SRC_LOSC,				//need make sure losc is on.
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB2]     = {
		    .src = CLK_SRC_AHB1,				//need make sure AHB1 is on.
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},
	{
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
		.soc_pwr_dep.id  		= MP3_STANDBY_FLAG,
	},
	{
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
		.soc_pwr_dep.id  		= BOOT_FAST_STANDBY_FLAG,
	},
	{
		.scene_type		    = SCENE_SUPER_STANDBY,
		.name			    = "super_standby",
		.soc_pwr_dep.id             = SUPER_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},

	},
	{
		.scene_type		    = SCENE_GPIO_HOLD_STANDBY,
		.name			    = "gpio_hold_standby",
		.soc_pwr_dep.id             = GPIO_HOLD_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT) | BITMAP(VCC_IO_BIT) | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},

	},
#if 0
	{
		.scene_type	= SCENE_NORMAL_STANDBY,
		.name		= "normal_standby",
		.id             = NORMAL_STANDBY_FLAG,
		.pwr_dm_en      = 0xfff,     	 //mean all power domain is on.
		.osc_en         = 0xf,		// mean Hosc&Losc&ldo&ldo1 is on.
		.init_pll_dis   = (~(0x10)), 	//mean pll5 is shundowned by dram driver.
		.exit_pll_en    = (~(0x10)),
		.pll_change     = 0x1,
		.pll_factor[0]  = {0x10,0,0,0},
		.bus_change     = 0x5,	             //ahb1&apb2 is changed.
		.bus_factor[0]  = {0x2,0,0,0,0},     //apb2 src is Hosc.
		.bus_factor[2]  = {0x2,0,0,0,0},     //ahb1 src is Hosc.
	},
#endif
	{
		.scene_type		    = SCENE_GPIO_STANDBY,
		.name			    = "gpio_standby",
		.soc_pwr_dep.id             = GPIO_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on. +vdd_sys
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_SYS_BIT) | BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT) | BITMAP(VCC_IO_BIT) | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = BITMAP(OSC_LOSC_BIT),	// mean all osc is off. +losc
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = BITMAP(BUS_AHB1),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1]     = {
		    .src = CLK_SRC_LOSC,				//need make sure losc is on.
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
#if 0		
		//for pb port
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c20824, 0x0000ffff, 0x00007777},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c20828, 0x00000ff0, 0x00000770},
#endif

	},

	{
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
		.soc_pwr_dep.id = MISC_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT) | BITMAP(VCC_IO_BIT) ,
		    
		    0x0644, //-vcc_io; -dram, ldoin/ adc/ cpvdd   
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (~(0x20)), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},
	{
		.scene_type	= SCENE_MISC1_STANDBY,
		.name		= "misc1_standby",
		.soc_pwr_dep.id = MISC1_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (~(0x20)), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,		//hold gpio
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},
	{
		//for parse sysconfig config. //
		//default config according dram enter selfresh.
		//when not enable enter selfresh, need open vdd_sys.
		.scene_type		    = SCENE_DYNAMIC_STANDBY,
		.name			    = "dynamic_standby",
		.soc_pwr_dep.id             = DYNAMIC_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (BITMAP(PM_PLL_DRAM)), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,	    //enter selfresh, for compatible reason.
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},

};
#elif defined CONFIG_ARCH_SUN8IW8P1	// FIXME: need double check.
scene_extended_standby_t extended_standby[] = {
	{
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
		.soc_pwr_dep.id             = TALKING_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT), 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
#if 0		
		//for pb port
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c20824, 0x0000ffff, 0x00007777},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c20828, 0x00000ff0, 0x00000770},
#endif

	},
	
	{
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
		.soc_pwr_dep.id  		= USB_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_ldoin, vdd_sys is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT) |\
							     BITMAP(VDD_SYS_BIT), 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = BITMAP(OSC_LOSC_BIT) | BITMAP(OSC_HOSC_BIT),	// mean all osc is off. +losc, +hosc
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = BITMAP(PM_PLL_PERIPH),
		.soc_pwr_dep.cpux_clk_state.pll_factor[PM_PLL_PERIPH] = { //PLL_PERIPH freq = 24*12/2/2= 72M 
		    .factor1 = 12, //N=12
		    .factor2 = 1, //Div1 = 1 + 1 = 2
		    .factor3 = 1, //Div2 = 1 + 1 = 2
		},
		.soc_pwr_dep.cpux_clk_state.bus_change     = BITMAP(BUS_AHB1) | BITMAP(BUS_AHB2),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1]     = {
		    .src = CLK_SRC_LOSC,				//need make sure losc is on.
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB2]     = {
		    .src = CLK_SRC_AHB1,				//need make sure AHB1 is on.
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},
	{
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
		.soc_pwr_dep.id  		= MP3_STANDBY_FLAG,
	},
	{
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
		.soc_pwr_dep.id  		= BOOT_FAST_STANDBY_FLAG,
	},
	{
		.scene_type		    = SCENE_SUPER_STANDBY,
		.name			    = "super_standby",
		.soc_pwr_dep.id             = SUPER_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},

	},
	{
		.scene_type		    = SCENE_GPIO_HOLD_STANDBY,
		.name			    = "gpio_hold_standby",
		.soc_pwr_dep.id             = GPIO_HOLD_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT) | BITMAP(VCC_IO_BIT) | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},

	},
	{
		.scene_type		    = SCENE_NORMAL_STANDBY,
		.name			    = "normal_standby",
		.soc_pwr_dep.id             = NORMAL_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},

	},
	{
		.scene_type		    = SCENE_GPIO_STANDBY,
		.name			    = "gpio_standby",
		.soc_pwr_dep.id             = GPIO_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on. +vdd_sys
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_SYS_BIT) | BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT) | BITMAP(VCC_IO_BIT) | BITMAP(VCC_LDOIN_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = BITMAP(OSC_LOSC_BIT),	// mean all osc is off. +losc
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = BITMAP(PM_PLL_DRAM), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = BITMAP(BUS_AHB1),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1]     = {
		    .src = CLK_SRC_LOSC,				//need make sure losc is on.
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
#if 0		
		//for pb port
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c20824, 0x0000ffff, 0x00007777},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c20828, 0x00000ff0, 0x00000770},
#endif

	},

	{
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
		.soc_pwr_dep.id = MISC_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT) | BITMAP(VCC_IO_BIT) ,
		    
		    0x0644, //-vcc_io; -dram, ldoin/ adc/ cpvdd   
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (~(0x20)), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},
	{
		.scene_type	= SCENE_MISC1_STANDBY,
		.name		= "misc1_standby",
		.soc_pwr_dep.id = MISC1_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT), 
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (~(0x20)), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,		//hold gpio
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},
	{
		//for parse sysconfig config. //
		//default config according dram enter selfresh.
		//when not enable enter selfresh, need open vdd_sys.
		.scene_type		    = SCENE_DYNAMIC_STANDBY,
		.name			    = "dynamic_standby",
		.soc_pwr_dep.id             = DYNAMIC_STANDBY_FLAG,
		//mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		//note: vcc_pm is marked on, just for cross-platform reason.
		//at a83: with the sys_mask's help, we know we do not need care about vcc_pm state.
		.soc_pwr_dep.soc_pwr_dm_state.state	   = BITMAP(VCC_DRAM_BIT) | BITMAP(VDD_CPUS_BIT) |\
							     BITMAP(VDD_DRAMPLL_BIT) | BITMAP(VCC_PL_BIT) | \
							     BITMAP(VCC_PM_BIT)  | BITMAP(VCC_LDOIN_BIT) | \
							     BITMAP(VDD_SYS_BIT)| BITMAP(VDD_CPUA_BIT),
		//mean care about cpua, dram, sys, cpus, dram_pll, vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll 
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,	//mean: donot need care about the voltage.
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,	// mean all osc is off.
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (BITMAP(PM_PLL_DRAM)), //mean pll5 is shutdowned & open by dram driver.
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,	    //enter selfresh, for compatible reason.
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		//for pf port: set the io to disable state.;
		.soc_pwr_dep.soc_io_state.io_state[0]     = {(unsigned int *)0x01c208b4, 0x00f0f0ff, 0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]     = {(unsigned int *)0x01c208b4, 0x000f0f00, 0x00070700},
	},

};
#elif defined CONFIG_ARCH_SUN8IW7P1
/* pwr_dm_en: bit0:pwr_sys, bit1:pwr_io, bit2:pwr_cpu
 * bus_change: bit0:axi, bit1:ahb1/apb1, bit2:ahb2
 * osc_en: bit0:HOSC
 */
scene_extended_standby_t extended_standby[8] = {
	{
		{
			.id           	= TALKING_STANDBY_FLAG,
			.pwr_dm_en      = 0xfb,       /* pwr_cpu is closed                              */
			.osc_en         = 0xf,        /* mean Hosc is on                                */
			.init_pll_dis   = 0x1fff,     /* disable all plls                               */
			.exit_pll_en    = 0x1fff,
			.pll_change     = 0,          /* all pll are closed                             */
			.bus_change     = 0x3,
			.bus_factor[0]	= {0, 0, 0, 0, 0}, /* switch cpu/axi src to IOSC                */
			.bus_factor[1]	= {0, 0, 1, 0, 0}, /* switch ahb1/apb1 src to IOSC              */
		},
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
	},
	{
		{
			.id  		= USB_STANDBY_FLAG,
			.pwr_dm_en      = 0xfb,       /* pwr_cpu is closed                              */
			.osc_en         = 0xf,        /* mean Hosc is on                                */
			.init_pll_dis   = 0x1fff,     /* disable all plls                               */
			.exit_pll_en    = 0x1fff,
			.pll_change     = 0,          /* all pll are closed                             */
			.bus_change     = 0x3,
			.bus_factor[0]	= {0, 0, 0, 0, 0}, /* switch cpu/axi src to IOSC                */
			.bus_factor[1]	= {0, 0, 1, 0, 0}, /* switch ahb1/apb1 src to IOSC              */
		},
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
	},
	{
		{
			.id  		= MP3_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
	},
	{
		{
			.id  		= BOOT_FAST_STANDBY_FLAG,
		},
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
	},
	{
		{
			.id             = SUPER_STANDBY_FLAG,
			.pwr_dm_en      = 0xfb,       /* pwr_cpu is closed                              */
			.osc_en         = 0x0,        /* mean Hosc is off                               */
			.init_pll_dis   = 0x1fff,     /* disable all plls                               */
			.exit_pll_en    = 0x1fff,
			.pll_change     = 0,          /* all pll are closed                             */
			.bus_change     = 0x3,
			.bus_factor[0]	= {0, 0, 0, 0, 0}, /* switch cpu/axi src to IOSC                */
			.bus_factor[1]	= {0, 0, 1, 0, 0}, /* switch ahb1/apb1 src to IOSC              */
		},
		.scene_type	= SCENE_SUPER_STANDBY,
		.name		= "super_standby",
	},
	{
		{
			.id             = NORMAL_STANDBY_FLAG,
			.pwr_dm_en      = 0xfb,       /* pwr_cpu is closed                              */
			.osc_en         = 0xf,        /* mean Hosc is on                                */
			.init_pll_dis   = 0x1fff,     /* disable all plls                               */
			.exit_pll_en    = 0x1fff,
			.pll_change     = 0,          /* all pll are closed                             */
			.bus_change     = 0x3,
			.bus_factor[0]	= {0, 0, 0, 0, 0}, /* switch cpu/axi src to IOSC                */
			.bus_factor[1]	= {0, 0, 1, 0, 0}, /* switch ahb1/apb1 src to IOSC              */
		},
		.scene_type	= SCENE_NORMAL_STANDBY,
		.name		= "normal_standby",
	},
	{
		{
			.id             = GPIO_STANDBY_FLAG,
			.pwr_dm_en      = 0xfb,       /* pwr_cpu is closed                              */
			.osc_en         = 0xf,        /* mean Hosc is on                                */
			.init_pll_dis   = 0x1fff,     /* disable all plls                               */
			.exit_pll_en    = 0x1fff,
			.pll_change     = 0,          /* all pll are closed                             */
			.bus_change     = 0x3,
			.bus_factor[0]	= {0, 0, 0, 0, 0}, /* switch cpu/axi src to IOSC                */
			.bus_factor[1]	= {0, 0, 1, 0, 0}, /* switch ahb1/apb1 src to IOSC              */
		},
		.scene_type	= SCENE_GPIO_STANDBY,
		.name		= "gpio_standby",
	},
	{
		{
			.id		= MISC_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
	},
};
#elif defined (CONFIG_ARCH_SUN9IW1)	//FIXME. the config need double check.
scene_extended_standby_t extended_standby[8] = {
	{
		{
			.id           	= TALKING_STANDBY_FLAG,
			.pwr_dm_en      = 0x0ff7,     /* only cpul is power off                           */
			.osc_en         = 0xf,        /* 24M & pll_ldo are reversed open                  */
			.init_pll_dis   = 0x1f0e,     /* pll_c0/ve/video0/1 can not be closed             */
			.exit_pll_en    = 0x0,        /* all plls are reversed                            */
			.pll_change     = 0x0,
			.bus_change     = 0x0,
		},
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
	},
	{
		{
			.id  		= USB_STANDBY_FLAG,
			.pwr_dm_en      = 0x0a7,      /* only dc5ldo & dcdc1/4/5 & s-bldo2 power on       */
			.osc_en         = 0x9,        /* 24M & pll_ldo are reversed open                  */
			.init_pll_dis   = 0x1fdf,     /* all plls ard closed                              */
			.exit_pll_en    = 0x0d1,      /* open pll_c0/VE/vedio0/1 for system resume stable */
			.bus_change     = 0x07fc0,    /* adjust all BUSES to lowest freq for power save   */
			.bus_factor[6]  = {1<<0, 0, 4, 0, 0},   /* GTBUS set to 24M/4=6M for AHB0 use for USB HOST  */
			.bus_factor[7]  = {1<<7, 0, 8, 0, 0},   /* AHB0 need adjust to GTBUS/8=750K for USB HOST    */
			.bus_factor[8]  = {1<<5, 0, 8, 0, 0},   /* AHB1 adjust to pll_per0(0MHz) for power save     */
			.bus_factor[9]  = {1<<5, 0, 8, 0, 0},   /* AHB2 adjust to pll_per0(0MHz) for power save     */
			.bus_factor[10] = {1<<0, 0, 8, 0, 0},   /* APB0 adjust to 24M/8=3M for power save           */
			.bus_factor[11] = {1<<5, 0, 8, 0,32},   /* APB1 adjust to pll_per0(0MHz) for power save     */
			.bus_factor[12] = {1<<5, 0, 4, 0, 0},   /* CCI adjust to pll_per0(0MHz) for power save      */
			.bus_factor[13] = {1<<5, 0, 0, 0, 8},   /* ATS adjust to pll_per0(0MHz) for power save      */
			.bus_factor[14] = {1<<5, 0, 0, 0, 8},   /* TRACE adjust to pll_per0(0MHz) for power save    */
		},
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
	},
	{
		{
			.id  		= MP3_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
	},
	{
		{
			.id		= BOOT_FAST_STANDBY_FLAG,
		},
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
	},
	{
		{
			.id             = SUPER_STANDBY_FLAG,
			.pwr_dm_en      = 0x0084,     /* reserve dcdc5 & dc5ldo power                     */
			.osc_en         = 0,          /* 24M & pll_ldo close, so all plls will can't used */
			.init_pll_dis   = 0,          /* all plls should be closed, but no need disabled  */
			.exit_pll_en    = 0x0,        /* all plls is default, for BROM need default envir */
			.pll_change     = 0x0,
			.bus_change     = 0x0,
		},
		.scene_type	= SCENE_SUPER_STANDBY,
		.name		= "super_standby",
	},
	{
		{
			.id             = NORMAL_STANDBY_FLAG,
			.pwr_dm_en      = 0x0fff,     /* mean all power domains are reversed power on     */
			.osc_en         = 0xf,        /* 24M & pll_ldo are reversed open                  */
			.init_pll_dis   = 0x1f0e,     /* pll_c0/ve/video0/1 can not be closed             */
			.exit_pll_en    = 0,
			.pll_change     = 0,
			.pll_factor[0]  = {0x10,0,0,0},
			.bus_change     = 0,
			.bus_factor[0]  = {0x2,0,0,0,0},
			.bus_factor[2]  = {0x2,0,0,0,0},
		},
		.scene_type	= SCENE_NORMAL_STANDBY,
		.name		= "normal_standby",
	},
	{
		{
			.id             = GPIO_STANDBY_FLAG,
			.pwr_dm_en      = 0x0fab,     /* mean avcc, dram, sys, io, cpus is on             */
			.osc_en         = 0xf,        /* 24M & pll_ldo are reversed open                  */
			.init_pll_dis   = 0x1f0e,     /* pll_c0/ve/video0/1 can not be closed             */
			.exit_pll_en    = 0x0,
			.pll_change     = 0x0,
			.bus_change     = 0x0,
			.bus_factor[2]  = {0x0,0,0,0,0},
		},
		.scene_type	= SCENE_GPIO_STANDBY,
		.name		= "gpio_standby",
	},
	{
		{
			.id		= MISC_STANDBY_FLAG,
		},
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
	},
};
#endif

int extended_standby_cnt = sizeof(extended_standby)/sizeof(extended_standby[0]);

#endif

