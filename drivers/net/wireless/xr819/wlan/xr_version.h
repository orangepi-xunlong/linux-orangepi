/*
 * version for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef XR_VERSION_H
#define XR_VERSION_H

#define XRADIO_SPEC_VER ""

#define _xradio_version(main, sub, rev) \
	"XR_V0" #main "." #sub "." #rev

#define XRADIO_VERSION  _xradio_version(2, 15, 73) " " XRADIO_SPEC_VER

#endif
