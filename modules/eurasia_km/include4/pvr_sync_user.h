/*************************************************************************/ /*!
@File           pvr_sync_user.h
@Title          Userspace definitions to use the kernel sync driver
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Version numbers and strings for PVR Consumer services
				components.
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
*/ /**************************************************************************/

#ifndef _PVR_SYNC_USER_H_
#define _PVR_SYNC_USER_H_

#include <linux/ioctl.h>

#ifdef __KERNEL__
#include "sgxapi_km.h"
#else
#include "sgxapi.h"
#endif

#include "servicesext.h" // PVRSRV_SYNC_DATA
#include "img_types.h"

/* This matches the sw_sync create ioctl data */
struct PVR_SYNC_CREATE_IOCTL_DATA
{
	/* Input: Name of this sync pt. Passed to base sync driver. */
	char	name[32];

	/* Input: An fd from a previous call to ALLOC ioctl. Cannot be <0. */
	__s32	allocdSyncInfo;

	/* Output: An fd returned from the CREATE ioctl. */
	__s32	fence;
};

struct PVR_SYNC_ALLOC_IOCTL_DATA
{
	/* Output: An fd returned from the ALLOC ioctl */
	__s32 fence;

	/* Output: IMG_TRUE if the timeline looked idle at alloc time */
	__u32 bTimelineIdle;
};

#define PVR_SYNC_DEBUG_MAX_POINTS 3

typedef struct
{
	/* Output: A globally unique stamp/ID for the sync */
	IMG_UINT64 ui64Stamp;

	/* Output: The WOP snapshot for the sync */
	IMG_UINT32 ui32WriteOpsPendingSnapshot;
}
PVR_SYNC_DEBUG;

struct PVR_SYNC_DEBUG_IOCTL_DATA
{
	/* Input: Fence to acquire debug for */
	int						iFenceFD;

	/* Output: Number of points merged into this fence */
	IMG_UINT32				ui32NumPoints;

	struct
	{
		/* Output: Metadata for sync point */
		PVR_SYNC_DEBUG		sMetaData;

		/* Output: 'Live' sync information. */
		PVRSRV_SYNC_DATA	sSyncData;
	}
	sSync[PVR_SYNC_DEBUG_MAX_POINTS];
};

#define PVR_SYNC_IOC_MAGIC	'W'

#define PVR_SYNC_IOC_CREATE_FENCE \
	_IOWR(PVR_SYNC_IOC_MAGIC, 0, struct PVR_SYNC_CREATE_IOCTL_DATA)

#define PVR_SYNC_IOC_DEBUG_FENCE \
	_IOWR(PVR_SYNC_IOC_MAGIC, 1, struct PVR_SYNC_DEBUG_IOCTL_DATA)

#define PVR_SYNC_IOC_ALLOC_FENCE \
	_IOWR(PVR_SYNC_IOC_MAGIC, 2, struct PVR_SYNC_ALLOC_IOCTL_DATA)

#define PVRSYNC_MODNAME "pvr_sync"

#endif /* _PVR_SYNC_USER_H_ */
