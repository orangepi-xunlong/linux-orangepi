/*************************************************************************/ /*!
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
*/ /**************************************************************************/

#if !defined(DC_EXAMPLE_H)
#define DC_EXAMPLE_H

#include "img_types.h"
#include "pvrsrv_error.h"

#if defined(LMA)
	#define DRVNAME	"dc_example_LMA"
#else
	#define DRVNAME	"dc_example_UMA"
#endif

#define MODNAME "dc_example"

/******************************************************************************
 * dc_example OS functions
 *****************************************************************************/

/* Module parameters */
typedef struct _DC_EXAMPLE_MODULE_PARAMETERS_
{
	IMG_UINT32 ui32Width;
	IMG_UINT32 ui32Height;
	IMG_UINT32 ui32Depth;
	IMG_UINT32 ui32Format;
	IMG_UINT32 ui32MemLayout;
	IMG_UINT32 ui32FBCFormat;
	IMG_UINT32 ui32RefreshRate;
	IMG_UINT32 ui32XDpi;
	IMG_UINT32 ui32YDpi;
	IMG_UINT32 ui32NumDevices;
} DC_EXAMPLE_MODULE_PARAMETERS;

const DC_EXAMPLE_MODULE_PARAMETERS *DCExampleGetModuleParameters(void);

void        *DCExampleVirtualAllocUncached(size_t uiSize);
IMG_BOOL     DCExampleVirtualFree(void *pvAllocHandle);
PVRSRV_ERROR DCExampleGetLinAddr(void *pvAllocHandle, IMG_CPU_VIRTADDR *ppvLinAddr);
PVRSRV_ERROR DCExampleGetDevPAddrs(void *pvAllocHandle, IMG_DEV_PHYADDR *pasDevPAddr,
				   uint32_t uiPageNo, size_t uiSize);
#if defined(INTEGRITY_OS)
PVRSRV_ERROR DCExampleOSAcquireKernelMappingData(void *pvAllocHandle, IMG_HANDLE *phMapping, void **ppPhysAddr);
#endif

/******************************************************************************
 * dc_example functions
 *****************************************************************************/

PVRSRV_ERROR DCExampleInit(IMG_UINT32 *puiNumDevices);
void DCExampleDeinit(void);

#endif /* !defined(DC_EXAMPLE_H) */
