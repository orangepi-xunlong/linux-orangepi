/*
 * u_serial.h - interface to USB gadget "serial port"/TTY utilities
 *
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#ifndef __U_SERIAL_H
#define __U_SERIAL_H

#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>
#include <linux/ioctl.h>

#define MAX_U_SERIAL_PORTS	4

struct bm_serial_opts {
	struct usb_function_instance func_inst;
	u8 port_num;
};

/*
 * One non-multiplexed "serial" I/O port ... there can be several of these
 * on any given USB peripheral device, if it provides enough endpoints.
 *
 * The "u_serial" utility component exists to do one thing:  manage TTY
 * style I/O using the USB peripheral endpoints listed here, including
 * hookups to sysfs and /dev for each logical "tty" device.
 *
 * REVISIT at least ACM could support tiocmget() if needed.
 *
 * REVISIT someday, allow multiplexing several TTYs over these endpoints.
 */
struct bmserial {
	struct usb_function		func;

	/* port is managed by gserial_{connect,disconnect} */
	struct gs_port			*ioport;

	struct usb_ep			*in;
	struct usb_ep			*out;

	/* REVISIT avoid this CDC-ACM support harder ... */
	struct usb_cdc_line_coding port_line_coding;	/* 9600-8-N-1 etc */

	/* notification callbacks */
	void (*connect)(struct bmserial *p);
	void (*disconnect)(struct bmserial *p);
	int (*send_break)(struct bmserial *p, int duration);
};

/*ioctl declare*/
struct ioctl_arg {
	unsigned int reg;
	unsigned int val;
};

#define IOC_MAGIC ('\x66')

#define IOCTL_VALSET      _IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_VALGET      _IOR(IOC_MAGIC, 1, struct ioctl_arg)
#define IOCTL_VALGET_NUM  _IOR(IOC_MAGIC, 2, int)
#define IOCTL_VALSET_NUM  _IOW(IOC_MAGIC, 3, int)

#define IOCTL_VAL_MAXNR 3

/* utilities to allocate/free request and buffer */
struct usb_request *bm_alloc_req(struct usb_ep *ep, unsigned int len, gfp_t flags);
void bm_free_req(struct usb_ep *ep, struct usb_request *req);

/* management of individual TTY ports */
int bmserial_alloc_line(unsigned char *port_line);
void bmserial_free_line(unsigned char port_line);

/* connect/disconnect is handled by individual functions */
int bmserial_connect(struct bmserial *gser, u8 port_num);
void bmserial_disconnect(struct bmserial *gser);

/* functions are bound to configurations by a config or gadget driver */
int bmser_bind_config(struct usb_configuration *c, u8 port_num);

#endif /* __U_SERIAL_H */
