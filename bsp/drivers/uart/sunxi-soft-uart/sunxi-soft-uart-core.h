// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 liujuan1@allwinnertech.com
 */

#ifndef SUNXI_SOFT_UART_CORE_H
#define SUNXI_SOFT_UART_CORE_H

#include <linux/tty.h>

int raspberry_soft_uart_init(const int gpio_tx, const int gpio_rx);
int raspberry_soft_uart_finalize(void);
int raspberry_soft_uart_open(struct tty_struct *tty);
int raspberry_soft_uart_close(void);
int raspberry_soft_uart_set_baudrate(const int baudrate);
int raspberry_soft_uart_send_string(const unsigned char *string, int string_size);
int raspberry_soft_uart_get_tx_queue_room(void);
int raspberry_soft_uart_get_tx_queue_size(void);
int sunxi_soft_uart_init(const int _gpio_tx, const int _gpio_rx, const int irq);
int sunxi_soft_uart_finalize(void);
int sunxi_soft_uart_open(struct tty_struct *tty);
int sunxi_soft_uart_get_tx_queue_size(void);
int sunxi_soft_uart_close(void);
int sunxi_soft_uart_send_string(const unsigned char *string, int string_size);
int sunxi_soft_uart_get_tx_queue_room(void);
int sunxi_soft_uart_set_baudrate(const int baudrate);

#endif
