/**************************************************************************/ /*!
@File           dkf_server.h
@Title          Functions for supporting the DRM Key Framework
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /***************************************************************************/

#if !defined(DKF_SERVER_H)
#define DKF_SERVER_H

#include "img_types.h"
#include "pvrsrv_error.h"

#if defined(SUPPORT_LINUX_FDINFO)

#include <drm/drm_print.h>
typedef void (DKF_VPRINTF_FUNC)(struct drm_printer *p, const char *fmt, va_list *va) __printf(2, 0);

#else /* !defined(SUPPORT_LINUX_FDINFO) */
typedef void (DKF_VPRINTF_FUNC)(void *p, const char *fmt, ...) __printf(2, 3);

#endif	/* defined(SUPPORT_LINUX_FDINFO) */
struct _PVRSRV_DEVICE_NODE_;

typedef IMG_UINT32 DKF_CONNECTION_FLAGS;

#define DKF_CONNECTION_FLAG_SYNC        BIT(0)
#define DKF_CONNECTION_FLAG_SERVICES    BIT(1)

#define DKF_CONNECTION_FLAG_INVALID     IMG_UINT32_C(0)

/*! @Function PVRDKFTraverse
 *
 * @Description
 * Outputs the DKP data associated with the given device node's
 * framework entries.
 *
 * @Input   pfnPrint            The print function callback to be used to output.
 * @Input   pvArg               Print function first argument.
 * @Input   psDevNode           Device node associated with fdinfo owner.
 * @Input   pid                 Process ID of the process owning the FD the fdinfo
 *                              file relates to.
 * @Input   ui32ConnectionType  A value indicating the PVR connection type
 *                              (sync or services).
 */
void PVRDKFTraverse(DKF_VPRINTF_FUNC *pfnPrint,
                    void *pvArg,
                    struct _PVRSRV_DEVICE_NODE_ *psDevNode,
                    IMG_PID pid,
                    DKF_CONNECTION_FLAGS ui32ConnectionType);

/* @Function PVRDKFInit
 *
 * @Description
 * Initialises the DKF infrastructure for subsequent usage by the PVR system.
 *
 * @Returns  PVRSRV_ERROR.
 */
PVRSRV_ERROR PVRDKFInit(void);

/* @Function PVRDKFDeInit
 *
 * @Description
 * Removes and frees all associated system-specific DKF meta-data.
 */
void PVRDKFDeInit(void);

#endif
