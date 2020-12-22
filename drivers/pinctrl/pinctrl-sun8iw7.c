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

static struct sunxi_desc_pin sun8i_w7_pins[] = {
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*TX*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*MS0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*RX*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*CK0*/
		SUNXI_FUNCTION(0x5, "Vdevice"),		/*virtual device for pinctrl testing*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*RTS*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*DO0*/
		SUNXI_FUNCTION(0x5, "Vdevice"),		/*virtual device for pinctrl testing*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/*CTS*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*DI0*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/*TX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),		/*RX*/
		SUNXI_FUNCTION(0x3, "pwm0"),
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sim0"),		/*PWREN*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sim0"),		/*CLK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sim0"),		/*DATA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sim0"),		/*RST*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT9*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sim0"),		/*DET*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x3, "di0"),			/*TX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT11*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x3, "di0"),			/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT12*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi1"),		/*CS*/
		SUNXI_FUNCTION(0x3, "uart3"), 		/*TX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT13*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA14,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi1"),		/*CLK*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT14*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA15,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi1"),		/*MOSI*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*RTS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT15*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA16,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi1"),		/*MISO*/
		SUNXI_FUNCTION(0x3, "uart3"),		/*CTS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT16*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA17,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spdif0"),		/*OUT*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT17*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA18,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm0"),		/*SYNC*/
		SUNXI_FUNCTION(0x3, "twi1"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT18*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA19,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm0"),		/*CLK*/
		SUNXI_FUNCTION(0x3, "twi1"),		/*SDA*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT19*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA20,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm0"),		/*DOUT*/
		SUNXI_FUNCTION(0x3, "sim0"),		/*VPPEN*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT20*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PA21,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm0"),		/*DIN*/
		SUNXI_FUNCTION(0x3, "sim0"),		/*VPPPP*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT21*/
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
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/*RXD3*/
		SUNXI_FUNCTION(0x3, "di0"),		/*TX */
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*RXD2*/
		SUNXI_FUNCTION(0x3, "di0"),		/*RX */
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*RXD1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*RXD0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*RXCK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*RXCTL*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*NULL*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*TXD3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*TXD2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*TXD1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*TXD0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*NULL*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*TXCK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*TXCTL*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD14,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*NULL*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD15,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"), 		/*CLKIN*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD16,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PD17,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),
		SUNXI_FUNCTION(0x7, "io_disable")),
		
	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*PCLK*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*CLK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*MCLK*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*ERR*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE2,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*HSYNC*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*SYNC*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE3,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*VSYNC*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*DVLD*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE4,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D0*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE5,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D1*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D1*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE6,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D2*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D2*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D3*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D3*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D4*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D4*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D5*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D5*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D6*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*D7*/
		SUNXI_FUNCTION(0x3, "ts0"),		/*D7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*SCK*/
		SUNXI_FUNCTION(0x3, "twi2"),		/*SCK*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/*SDA*/
		SUNXI_FUNCTION(0x3, "twi2"),		/*SDA*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE14,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PE15,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x7, "io_disable")),

	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PF0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/*D1*/
		SUNXI_FUNCTION(0x3, "jtag0"),		/*MSI*/
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
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT6*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG7,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT7*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG8,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*RTS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/*CTS*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT9*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG10,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm1"),		/*SYNC*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT10*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG11,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm1"),		/*CLK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT11*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG12,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm1"),		/*DOUT*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT12*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PG13,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pcm1"),		/*DIN*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT13*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	
	/*HOLE*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL0,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_twi0"),		/*SCK*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT0*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL1,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_twi0"),		/*SDA*/
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
		SUNXI_FUNCTION(0x2, "s_jtag0"),		/*DO*/
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
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT8*/
		SUNXI_FUNCTION(0x7, "io_disable")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN_PL9,
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
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
		SUNXI_FUNCTION(0x2, "s_cir0"),		/*RX*/
		SUNXI_FUNCTION(0x6, "eint"),		/*EINT11*/	
		SUNXI_FUNCTION(0x7, "io_disable")),
};
static struct sunxi_pin_bank sun8i_w7_banks[] = {
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PA_BASE, 22, SUNXI_EINT_TYPE_GPIO, "PA", SUNXI_IRQ_EINTA),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PC_BASE, 19, SUNXI_EINT_TYPE_NONE, "PC", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PD_BASE, 18, SUNXI_EINT_TYPE_NONE, "PD", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PE_BASE, 16, SUNXI_EINT_TYPE_NONE, "PE", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PF_BASE,  7, SUNXI_EINT_TYPE_NONE, "PF", SUNXI_IRQ_MAX  ),
	SUNXI_PIN_BANK(SUNXI_PIO_VBASE,   SUNXI_PG_BASE, 14, SUNXI_EINT_TYPE_GPIO, "PG", SUNXI_IRQ_EINTG),
	SUNXI_PIN_BANK(SUNXI_R_PIO_VBASE, SUNXI_PL_BASE, 12, SUNXI_EINT_TYPE_GPIO, "PL", SUNXI_IRQ_EINTL),
};

struct sunxi_pinctrl_desc sunxi_pinctrl_data = {
	.pins   = sun8i_w7_pins,
	.npins  = ARRAY_SIZE(sun8i_w7_pins),
	.banks  = sun8i_w7_banks,
	.nbanks = ARRAY_SIZE(sun8i_w7_banks),
};
