#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include "core.h"
#include "pinctrl-sunxi.h"
static struct sunxi_desc_pin sun8i_w6_pins[] = {

#ifdef CONFIG_FPGA_V4_PLATFORM
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*MS0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/*SDA*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*CK0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
#else
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*TX*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*MS0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*RX*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*CK0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
#endif
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*RTS*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*DO0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*CTS*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*DI0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s0"),		/*LRCK*/
		SUNXI_FUNCTION(0x3, "tdm0"),		/*LRCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s0"),		/*BCLK*/
		SUNXI_FUNCTION(0x3, "tdm0"),		/*BCLK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s0"),		/*DOUT*/
		SUNXI_FUNCTION(0x3, "tdm0"),		/*DOUT*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s0"),		/*DIN*/
		SUNXI_FUNCTION(0x3, "tdm0"),		/*DIN*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s0"),		/*MCLK*/
		SUNXI_FUNCTION(0x3, "tdm0"),		/*MCLK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/*TX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT9*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PB10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*WE*/
		SUNXI_FUNCTION(0x3, "spi0"),		/*MOSI*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*ALE*/
		SUNXI_FUNCTION(0x3, "spi0"),		/*MISO*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*CLE*/
		SUNXI_FUNCTION(0x3, "spi0"),		/*CLK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*CE1*/
		SUNXI_FUNCTION(0x3, "spi0"),		/*CS*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*CE0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*RE*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*CLK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*RB0*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*CMD*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*RB1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ0*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ1*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ2*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ3*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ4*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ5*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC14,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ6*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC15,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQ7*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*D7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC16,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*DQS*/
		SUNXI_FUNCTION(0x3, "sdc2"),		/*RST*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC17,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*CE2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PC18,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/*CE3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D2*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/**/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D3*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*grxd2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D4*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*grxd1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D5*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*grxd0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D6*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*grxck*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D7*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GRXCTL/ERXDV*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D10*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GNULL/ERXERR*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D11*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GTXD3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D12*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GTXD2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D13*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GTXD1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD14,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D14*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GTXD0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD15,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D15*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GNULL/ECRS*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD18,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D18*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VP0*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GTXCK/ETXCK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD19,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D19*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VN0*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GTXCTL/ETXEN*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD20,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D20*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VP1*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GNULL/ETXERR*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD21,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D21*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VN1*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GCLKIN/ECOL*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD22,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D22*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VP2*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GMDC*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD23,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*D23*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VN2*/
		SUNXI_FUNCTION(0x4, "gmac0"),		/*GMDIO*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD24,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*CLK*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VPC*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD25,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*DE*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VNC*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD26,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*HSYNC*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VP3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD27,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/*VSYNC*/
		SUNXI_FUNCTION(0x3, "lvds0"),		/*VN3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD28,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm0"),
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD29,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),

	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*PCLK*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*CLK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*MCLK*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*DE*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*HSYNC*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*HSYNC*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*VSYNC*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*VSYNC*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D2*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D3*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D4*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D5*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D6*/
		SUNXI_FUNCTION(0x3, "uart4"),		/*TX*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D7*/
		SUNXI_FUNCTION(0x3, "uart4"),		/*RX*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D8*/
		SUNXI_FUNCTION(0x3, "uart4"),		/*RTS*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D9*/
		SUNXI_FUNCTION(0x3, "uart4"),		/*CTS*/
		SUNXI_FUNCTION(0x4, "ccir0"),		/*D7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE14,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*SCK*/
		SUNXI_FUNCTION(0x3, "twi2"),		/*SCK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE15,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*SDA*/
		SUNXI_FUNCTION(0x3, "twi2"),		/*SDA*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE16,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE17,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE18,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "spdif0"),		/*DOUT*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE19,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),

	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*D1*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*MS1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*D0*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*DI1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*CLK*/
		SUNXI_FUNCTION(0x3, "uart0"),		/*TX*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*CMD*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*DO1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*D3*/
		SUNXI_FUNCTION(0x3, "uart0"),		/*RX*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*D2*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*CK1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),

	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/*CLK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/*CMD*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/*D0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/*D1*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/*D2*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/*D3*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*TX*/
		SUNXI_FUNCTION(0x3, "spi1"),		/*CS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*RX*/
		SUNXI_FUNCTION(0x3, "spi1"),		/*CLK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*RTS*/
		SUNXI_FUNCTION(0x3, "spi1"),		/*DOUT*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*CTS*/
		SUNXI_FUNCTION(0x3, "spi1"),		/*DIN*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT9*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/*BCLK*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*TX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/*LRCK*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT11*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/*DOUT*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*RTS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT12*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/*DIN*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*CTS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT13*/
		SUNXI_FUNCTION(0x7, "io_disable")),

	/*HOLE*/
#ifdef CONFIG_FPGA_V4_PLATFORM
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
#else
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/*SDA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
#endif
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/*SDA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi2"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi2"),		/*SDA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "hscl"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "hsda"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "hcec"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT9*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x5, "Vdevice"),		/*virtual device for pinctrl testing*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PH11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x5, "Vdevice"),		/*virtual device for pinctrl testing*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT11*/
		SUNXI_FUNCTION(0x7, "io_disable")),

	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_rsb0"),		/*SCK*/
		SUNXI_FUNCTION(0x3, "s_twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_rsb0"),		/*SDA*/
		SUNXI_FUNCTION(0x3, "s_twi0"),		/*SDA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_uart0"),		/*TX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_uart0"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/*MS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/*CK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/*D0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/*DI*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_twi0"),		/*SDA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT9*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_pwm0"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_cir0"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
};
static struct sunxi_pin_bank sun8i_w6_banks[] = {
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PB_BASE, 11, SUNXI_EINT_TYPE_GPIO, "PB", SUNXI_IRQ_EINTB),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PC_BASE, 19, SUNXI_EINT_TYPE_NONE, "PC", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PD_BASE, 24, SUNXI_EINT_TYPE_NONE, "PD", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PE_BASE, 20, SUNXI_EINT_TYPE_NONE, "PE", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PF_BASE,  7, SUNXI_EINT_TYPE_NONE, "PF", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PG_BASE, 14, SUNXI_EINT_TYPE_GPIO, "PG", SUNXI_IRQ_EINTG),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PH_BASE, 12, SUNXI_EINT_TYPE_GPIO, "PH", SUNXI_IRQ_EINTH),
	SUNXI_PIN_BANK(SUNXI_R_PIO_VBASE, SUNXI_PL_BASE, 13, SUNXI_EINT_TYPE_GPIO, "PL", SUNXI_IRQ_EINTL),
};
struct sunxi_pinctrl_desc sunxi_pinctrl_data = {
	.pins   = sun8i_w6_pins,
	.npins  = ARRAY_SIZE(sun8i_w6_pins),
	.banks  = sun8i_w6_banks,
	.nbanks = ARRAY_SIZE(sun8i_w6_banks),
};


