/*
 *  drivers/soc/sunxi/arisc/include/arisc_hwmsgbox.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
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
 */

#ifndef __ARISC_HWMSGBOX_H__
#define __ARISC_HWMSGBOX_H__

#include <linux/arisc/arisc.h>

int arisc_hwmsgbox_init(void);
irqreturn_t arisc_hwmsgbox_int_handler(int irq, void *dev);
int arisc_hwmsgbox_send_message(struct arisc_message *pmessage, unsigned int timeout);
int arisc_hwmsgbox_standby_resume(void);
int arisc_hwmsgbox_standby_suspend(void);
int arisc_hwmsgbox_message_feedback(struct arisc_message *pmessage);
int arisc_hwmsgbox_wait_message_feedback(struct arisc_message *pmessage);
struct arisc_message *arisc_hwmsgbox_query_message(void);
int arisc_hwmsgbox_clear_receiver_pending(int queue, int user);
int arisc_hwmsgbox_query_receiver_pending(int queue, int user);
int arisc_hwmsgbox_disable_receiver_int(int queue, int user);
int arisc_hwmsgbox_enable_receiver_int(int queue, int user);
int arisc_hwmsgbox_feedback_message(struct arisc_message *pmessage, unsigned int timeout);
int arisc_hwmsgbox_exit(void);

#endif
