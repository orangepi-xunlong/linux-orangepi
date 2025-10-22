// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 liujuan1@allwinnertech.com
 */

#define pr_fmt(fmt)  "sunxi-soft-uart: " fmt

#include <linux/of.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/version.h>
#include <linux/of_address.h>
#include <linux/tty_driver.h>
#include <linux/serial_core.h>
#include <linux/of_platform.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>

#include "sunxi-soft-uart-core.h"

#define SOFT_UART_MAJOR            0
#define N_PORTS                    1
#define NONE                       0
#define TX_BUFFER_FLUSH_TIMEOUT 4000  // milliseconds

// Driver instance.
// @TODO: add struct soft_uart chip, and transfer the following global variable
static struct tty_driver *soft_uart_driver;
static struct tty_port port;

/**
 * Opens a given TTY device.
 * @param tty given TTY device
 * @param file
 * @return error code.
 */
static int soft_uart_open(struct tty_struct *tty, struct file *file)
{
	int error = NONE;

	if (sunxi_soft_uart_open(tty)) {
		pr_info("Device opened.\n");
	} else {
		pr_err("Device busy.\n");
		error = -ENODEV;
	}

	return error;
}

/**
 * Closes a given TTY device.
 * @param tty
 * @param file
 */
static void soft_uart_close(struct tty_struct *tty, struct file *file)
{
	// Waits for the TX buffer to be empty before closing the UART.
	int wait_time = 0;
	while ((sunxi_soft_uart_get_tx_queue_size() > 0)
			&& (wait_time < TX_BUFFER_FLUSH_TIMEOUT)) {
		msleep(100);
		wait_time += 100;
	}

	if (sunxi_soft_uart_close())
		pr_info("Device closed.\n");
	else
		pr_err("Could not close the device.\n");
}

/**
 * Writes the contents of a given buffer into a given TTY device.
 * @param tty given TTY device
 * @param buffer given buffer
 * @param buffer_size number of bytes contained in the given buffer
 * @return number of bytes successfuly written into the TTY device
 */
static int soft_uart_write(struct tty_struct *tty, const unsigned char *buffer, int buffer_size)
{
	return sunxi_soft_uart_send_string(buffer, buffer_size);
}

/**
 * Tells the kernel the number of bytes that can be written to a given TTY.
 * @param tty given TTY
 * @return number of bytes
 */
static unsigned int soft_uart_write_room(struct tty_struct *tty)
{
	return sunxi_soft_uart_get_tx_queue_room();
}

/**
 * Does nothing.
 * @param tty
 */
static void soft_uart_flush_buffer(struct tty_struct *tty)
{
}

/**
 * Tells the kernel the number of bytes contained in the buffer of a given TTY.
 * @param tty given TTY
 * @return number of bytes
 */
static unsigned int soft_uart_chars_in_buffer(struct tty_struct *tty)
{
	return sunxi_soft_uart_get_tx_queue_size();
}

/**
 * Sets the UART parameters for a given TTY (only the baudrate is taken into account).
 * @param tty given TTY
 * @param termios parameters
 */
static void soft_uart_set_termios(struct tty_struct *tty, struct ktermios *termios)
{
	int cflag = 0;
	speed_t baudrate = tty_get_baud_rate(tty);
	pr_info("soft_uart_set_termios: baudrate = %d.\n", baudrate);

	// Gets the cflag.
	cflag = tty->termios.c_cflag;

	// Verifies the number of data bits (it must be 8).
	if ((cflag & CSIZE) != CS8)
		pr_info("Invalid number of data bits.\n");

	// Verifies the number of stop bits (it must be 1).
	if (cflag & CSTOPB)
		pr_info("Invalid number of stop bits.\n");

	// Verifies the parity (it must be none).
	if (cflag & PARENB)
		pr_info("Invalid parity.\n");

	// Configure the baudrate.
	if (!sunxi_soft_uart_set_baudrate(baudrate))
		pr_info("Invalid baudrate.\n");
}

static void soft_uart_stop(struct tty_struct *tty)
{
	pr_info("soft_uart_stop.\n");
}

static void soft_uart_start(struct tty_struct *tty)
{
	pr_info("soft_uart_start.\n");
}

static void soft_uart_hangup(struct tty_struct *tty)
{
	pr_info("soft_uart_hangup.\n");
}

static int soft_uart_tiocmget(struct tty_struct *tty)
{
	return 0;
}

static int soft_uart_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear)
{
	return 0;
}

static int soft_uart_ioctl(struct tty_struct *tty, unsigned int command, unsigned int long parameter)
{
	int error = NONE;

	switch (command) {
	case TIOCMSET:
		error = NONE;
		break;

	case TIOCMGET:
		error = NONE;
		break;

	default:
		error = -ENOIOCTLCMD;
		break;
	}

	return error;
}

static void soft_uart_throttle(struct tty_struct *tty)
{
	pr_info("soft_uart_throttle.\n");
}

static void soft_uart_unthrottle(struct tty_struct *tty)
{
	pr_info("soft_uart_unthrottle.\n");
}

// Module operations.
static const struct tty_operations soft_uart_operations = {
	.open            = soft_uart_open,
	.close           = soft_uart_close,
	.write           = soft_uart_write,
	.write_room      = soft_uart_write_room,
	.flush_buffer    = soft_uart_flush_buffer,
	.chars_in_buffer = soft_uart_chars_in_buffer,
	.ioctl           = soft_uart_ioctl,
	.set_termios     = soft_uart_set_termios,
	.stop            = soft_uart_stop,
	.start           = soft_uart_start,
	.hangup          = soft_uart_hangup,
	.tiocmget        = soft_uart_tiocmget,
	.tiocmset        = soft_uart_tiocmset,
	.throttle        = soft_uart_throttle,
	.unthrottle      = soft_uart_unthrottle
};

/**
 * Module initialization.
 */
static int soft_uart_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	int gpio_tx;
	int gpio_rx;
	struct device_node *node = pdev->dev.of_node;

	pr_info("Initializing module...\n");

	gpio_tx = of_get_named_gpio(node, "gpio-tx", 0);
	if (!gpio_is_valid(gpio_tx)) {
		pr_err("%s: gpio_tx is invalid.\n", __func__);
		return -EINVAL;
	}
	gpio_rx = of_get_named_gpio(node, "gpio-rx", 0);
	if (!gpio_is_valid(gpio_rx)) {
		pr_err("%s: gpio_rx is invalid.\n", __func__);
		return -EINVAL;
	}
	irq = of_irq_get(node, 0);

	if (!sunxi_soft_uart_init(gpio_tx, gpio_rx, irq)) {
		pr_err("Failed initialize GPIO.\n");
		return -ENOMEM;
	}

	// Initializes the port.
	tty_port_init(&port);
	port.flags &= ~UPF_LOW_LATENCY;

	soft_uart_driver = tty_alloc_driver(N_PORTS, TTY_DRIVER_REAL_RAW);
	if (IS_ERR(soft_uart_driver)) {
		pr_err("Failed to allocate the driver.\n");
		ret = -ENOMEM;
		goto alloc_fail;
	}

	// Initializes the driver.
	soft_uart_driver->owner                 = THIS_MODULE;
	soft_uart_driver->driver_name           = "sunxi-soft-uart";
	soft_uart_driver->name                  = "ttySOFT";
	soft_uart_driver->major                 = SOFT_UART_MAJOR;
	soft_uart_driver->minor_start           = 0;
	soft_uart_driver->flags                 = TTY_DRIVER_REAL_RAW;
	soft_uart_driver->type                  = TTY_DRIVER_TYPE_SERIAL;
	soft_uart_driver->subtype               = SERIAL_TYPE_NORMAL;
	soft_uart_driver->init_termios          = tty_std_termios;
	soft_uart_driver->init_termios.c_ispeed = 4800;
	soft_uart_driver->init_termios.c_ospeed = 4800;
	soft_uart_driver->init_termios.c_cflag  = B4800 | CREAD | CS8 | CLOCAL;

	// Sets the callbacks for the driver.
	tty_set_operations(soft_uart_driver, &soft_uart_operations);

	// Link the port with the driver.
	tty_port_link_device(&port, soft_uart_driver, 0);

	// Registers the TTY driver.
	if (tty_register_driver(soft_uart_driver)) {
		pr_err("Failed to register the driver.\n");
		ret = -1;
		goto register_fail;
	}

	pr_info("Module initialized.\n");
	return 0;

register_fail:
	tty_driver_kref_put(soft_uart_driver);
alloc_fail:
	tty_port_destroy(&port);
	return ret;
}

/**
 * Cleanup function that gets called when the module is unloaded.
 */
static int soft_uart_remove(struct platform_device *pdev)
{
	pr_info("Finalizing the module...\n");

	// Finalizes the soft UART.
	if (!sunxi_soft_uart_finalize()) {
		pr_err("Something went wrong whilst finalizing the soft UART.\n");
	}


	// Unregisters the driver.
	tty_unregister_driver(soft_uart_driver);
	tty_driver_kref_put(soft_uart_driver);
	tty_port_destroy(&port);

	pr_info("Module finalized.\n");
	return 0;
}

static const struct of_device_id soft_uart_of_match[] = {
	{.compatible = "sunxi-soft-uart",},
};

static struct platform_driver soft_uart = {
	.driver = {
		.name = "sunxi-soft-uart",
		.owner = THIS_MODULE,
		.of_match_table = soft_uart_of_match,
	},
	.probe = soft_uart_probe,
	.remove = soft_uart_remove,
};

static int __init soft_uart_init(void)
{
	return platform_driver_register(&soft_uart);
}

static void __exit soft_uart_exit(void)
{
	platform_driver_unregister(&soft_uart);
}

// Module entry points.
module_init(soft_uart_init);
module_exit(soft_uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adriano Marto Reis");
MODULE_AUTHOR("emma <liujuan1@allwinnertech.com>");
MODULE_DESCRIPTION("Soft-UART Driver");
MODULE_VERSION("1.0.0");
