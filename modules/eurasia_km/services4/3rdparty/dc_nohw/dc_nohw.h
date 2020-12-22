/*************************************************************************/ /*!
@Title          Dummy 3rd party display driver structures and prototypes
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

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the
 IMG PowerVR Services driver with 3rd Party display hardware.
 It is NOT a specification for a display controller driver, rather a
 specification to extend the API for a pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG PowerVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.

 Functions of the API include

  - query primary surface attributes (width, height, stride, pixel format,
      CPU physical and virtual address)
  - swap/flip chain creation and subsequent query of surface attributes
  - asynchronous display surface flipping, taking account of asynchronous
      read (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map
 the display memory to any IMG PowerVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver
 may be extended to support the 3rd Party Display interface to
 PowerVR Services - IMG is not providing a display driver implementation
 **************************************************************************/

#ifndef __DC_NOHW_H__
#define __DC_NOHW_H__


#if defined(__cplusplus)
extern "C" {
#endif

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
#if defined (ENABLE_DISPLAY_MODE_TRACKING)
#error Cannot have both USE_BASE_VIDEO_FRAMEBUFFER and ENABLE_DISPLAY_MODE_TRACKING defined
#endif
#endif

#if !defined(DC_NOHW_BUFFER_WIDTH) && !defined(DC_NOHW_BUFFER_HEIGHT)
/* Default buffer size */
#define DC_NOHW_BUFFER_WIDTH		320
#define DC_NOHW_BUFFER_HEIGHT		240
#endif

#define DC_NOHW_BUFFER_BIT_DEPTH	32
#define	DC_NOHW_BUFFER_PIXEL_FORMAT	PVRSRV_PIXEL_FORMAT_ARGB8888

#define	DC_NOHW_DEPTH_BITS_PER_BYTE	8

#define	dc_nohw_byte_depth_from_bit_depth(bit_depth) (((IMG_UINT32)(bit_depth) + DC_NOHW_DEPTH_BITS_PER_BYTE - 1)/DC_NOHW_DEPTH_BITS_PER_BYTE)
#define	dc_nohw_bit_depth_from_byte_depth(byte_depth) ((IMG_UINT32)(byte_depth) * DC_NOHW_DEPTH_BITS_PER_BYTE)
#define dc_nohw_roundup_bit_depth(bd) dc_nohw_bit_depth_from_byte_depth(dc_nohw_byte_depth_from_bit_depth(bd))

#define	dc_nohw_byte_stride(width, bit_depth) ((IMG_UINT32)(width) * dc_nohw_byte_depth_from_bit_depth(bit_depth))

#if defined(DC_NOHW_GET_BUFFER_DIMENSIONS)
IMG_BOOL GetBufferDimensions(IMG_UINT32 *pui32Width, IMG_UINT32 *pui32Height, PVRSRV_PIXEL_FORMAT *pePixelFormat, IMG_UINT32 *pui32Stride);
#else
#define DC_NOHW_BUFFER_BYTE_STRIDE	dc_nohw_byte_stride(DC_NOHW_BUFFER_WIDTH, DC_NOHW_BUFFER_BIT_DEPTH)
#endif

extern IMG_BOOL IMG_IMPORT PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable);

#define DC_NOHW_MAXFORMATS	(1)
#define DC_NOHW_MAXDIMS		(1)
#define DC_NOHW_MAX_BACKBUFFERS (3)


typedef void *       DC_HANDLE;

typedef struct DC_NOHW_BUFFER_TAG
{
	DC_HANDLE					hSwapChain;
	DC_HANDLE					hMemChunk;

	/* member using IMG structures to minimise API function code */
	/* replace with own structures where necessary */
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
	IMG_SYS_PHYADDR				*psSysAddr;
#else
	IMG_SYS_PHYADDR				sSysAddr;
#endif
	IMG_DEV_VIRTADDR			sDevVAddr;
	IMG_CPU_VIRTADDR			sCPUVAddr;
	PVRSRV_SYNC_DATA*			psSyncData;

	struct DC_NOHW_BUFFER_TAG	*psNext;
} DC_NOHW_BUFFER;


/* DC_NOHW buffer structure */
typedef struct DC_NOHW_SWAPCHAIN_TAG
{
	unsigned long ulBufferCount;
	DC_NOHW_BUFFER *psBuffer;
} DC_NOHW_SWAPCHAIN;


/* kernel device information structure */
typedef struct DC_NOHW_DEVINFO_TAG
{
	unsigned int			 uiDeviceID;

	/* system surface info */
	DC_NOHW_BUFFER           sSystemBuffer;

	/* number of supported display formats */
	unsigned long            ulNumFormats;

	/* number of supported display dims */
	unsigned long            ulNumDims;

	/* jump table into PVR services */
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;

	/* jump table into DC */
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	/*
		handle for connection to kernel services
		- OS specific - may not be required
	*/
	DC_HANDLE                hPVRServices;

	/* back buffer info */
	DC_NOHW_BUFFER           asBackBuffers[DC_NOHW_MAX_BACKBUFFERS];

	/* ref count */
	unsigned long            ulRefCount;

	DC_NOHW_SWAPCHAIN       *psSwapChain;

	/* member using IMG structures to minimise API function code */
	/* replace with own structures where necessary */
	DISPLAY_INFO             sDisplayInfo;

	/* system surface info */
	DISPLAY_FORMAT           sSysFormat;
	DISPLAY_DIMS             sSysDims;
	IMG_UINT32               ui32BufferSize;

	/* list of supported display formats */
	DISPLAY_FORMAT           asDisplayFormatList[DC_NOHW_MAXFORMATS];

	/* list of supported display formats */
	DISPLAY_DIMS             asDisplayDimList[DC_NOHW_MAXDIMS];

	/* back buffer info */
	DISPLAY_FORMAT           sBackBufferFormat[DC_NOHW_MAXFORMATS];

}  DC_NOHW_DEVINFO;


/*!
 *****************************************************************************
 * Error values
 *****************************************************************************/
typedef enum _DC_ERROR_
{
	DC_OK								=  0,
	DC_ERROR_GENERIC					=  1,
	DC_ERROR_OUT_OF_MEMORY				=  2,
	DC_ERROR_TOO_FEW_BUFFERS			=  3,
	DC_ERROR_INVALID_PARAMS				=  4,
	DC_ERROR_INIT_FAILURE				=  5,
	DC_ERROR_CANT_REGISTER_CALLBACK 	=  6,
	DC_ERROR_INVALID_DEVICE				=  7,
	DC_ERROR_DEVICE_REGISTER_FAILED 	=  8
} DC_ERROR;


#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

DC_ERROR Init(void);
DC_ERROR Deinit(void);

#if defined(USE_BASE_VIDEO_FRAMEBUFFER) || defined (ENABLE_DISPLAY_MODE_TRACKING)
DC_ERROR OpenMiniport(void);
DC_ERROR CloseMiniport(void);
#endif /* #if defined(USE_BASE_VIDEO_FRAMEBUFFER) || defined (ENABLE_DISPLAY_MODE_TRACKING) */ 

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
PVRSRV_ERROR SetupDevInfo (DC_NOHW_DEVINFO *psDevInfo);
PVRSRV_ERROR FreeBackBuffers (DC_NOHW_DEVINFO *psDevInfo);
#endif

#if defined (ENABLE_DISPLAY_MODE_TRACKING)
DC_ERROR Shadow_Desktop_Resolution(DC_NOHW_DEVINFO	*psDevInfo);
#endif /* #if defined (ENABLE_DISPLAY_MODE_TRACKING) */

#if !defined(DC_NOHW_DISCONTIG_BUFFERS) && !defined(USE_BASE_VIDEO_FRAMEBUFFER)
IMG_SYS_PHYADDR CpuPAddrToSysPAddr(IMG_CPU_PHYADDR cpu_paddr);
IMG_CPU_PHYADDR SysPAddrToCpuPAddr(IMG_SYS_PHYADDR sys_paddr);
#endif

/* OS Specific APIs */
DC_ERROR OpenPVRServices  (DC_HANDLE *phPVRServices);
DC_ERROR ClosePVRServices (DC_HANDLE hPVRServices);

#if defined(DC_NOHW_DISCONTIG_BUFFERS)
DC_ERROR AllocDiscontigMemory(unsigned long ulSize,
                              DC_HANDLE * phMemChunk,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **pPhysAddr);

void FreeDiscontigMemory(unsigned long ulSize,
                         DC_HANDLE hMemChunk,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr);
#else



DC_ERROR AllocContigMemory(unsigned long ulSize,
                           DC_HANDLE * phMemHandle,
                           IMG_CPU_VIRTADDR *pLinAddr,
                           IMG_CPU_PHYADDR *pPhysAddr);

void FreeContigMemory(unsigned long ulSize,
                      DC_HANDLE hMemChunk,
                      IMG_CPU_VIRTADDR LinAddr,
                      IMG_CPU_PHYADDR PhysAddr);


#endif

void *AllocKernelMem(unsigned long ulSize);
void FreeKernelMem  (void *pvMem);

DC_ERROR GetLibFuncAddr (DC_HANDLE hExtDrv, char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);

#if defined(__cplusplus)
}
#endif

#endif /* __DC_NOHW_H__ */

/******************************************************************************
 End of file (dc_nohw.h)
******************************************************************************/

