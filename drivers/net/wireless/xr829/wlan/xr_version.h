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

#define XRADIO_MAJOR_VER 2
#define XRADIO_MINOR_VER 18
#define XRADIO_PATCH_VER 0

#define XRADIO_SPEC_MAJOR_VER 0
#define XRADIO_SPEC_MINOR_VER 0
#define XRADIO_COMMIT_HASH "01cf28e"
#define XRADIO_COMMIT_BRANCH "(compat5.4-linux5.4-xr829)"

#define __XR_VERSTR(x)   #x
#define _XR_VERSTR(x)   __XR_VERSTR(x)

#define XRADIO_VERSION  "XR_v"_XR_VERSTR(XRADIO_MAJOR_VER) "." \
			_XR_VERSTR(XRADIO_MINOR_VER) "." \
			_XR_VERSTR(XRADIO_PATCH_VER) "_" \
			_XR_VERSTR(XRADIO_SPEC_MAJOR_VER) "." \
			_XR_VERSTR(XRADIO_SPEC_MINOR_VER) "_" \
			XRADIO_COMMIT_HASH \
			XRADIO_COMMIT_BRANCH

#endif
