/*************************************************************************/ /*!
@File
@Title          RGX firmware RISC-V utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware RISC-V utility routines
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

#ifndef RGXFWRISCV_H
#define RGXFWRISCV_H

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
#define RGXRISCVFW_GET_REMAP_SECURE(r)          (RGX_CR_FWCORE_ADDR_REMAP_CONFIG0__HOST_SECURITY_GEQ4 + ((r) * 8U))
#define RGXRISCVFW_BOOTLDR_CODE_REMAP_SECURE    (RGXRISCVFW_GET_REMAP_SECURE(RGXRISCVFW_BOOTLDR_CODE_REGION))
#define RGXRISCVFW_BOOTLDR_DATA_REMAP_SECURE    (RGXRISCVFW_GET_REMAP_SECURE(RGXRISCVFW_BOOTLDR_DATA_REGION))
#define RGX_GET_RISCV_REGS_BASE(psDevInfo) \
	((RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) >= 4) ? \
	 (psDevInfo)->pvSecureRegsBaseKM : (psDevInfo)->pvRegsBaseKM)
#else
#define RGX_GET_RISCV_REGS_BASE(psDevInfo) ((psDevInfo)->pvRegsBaseKM)
#endif

/*!
*******************************************************************************
@Function       RGXRiscvHalt

@Description    Halt the RISC-V FW core (required for certain operations
                done through Debug Module)

@Input          psDevInfo       Pointer to device info

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvHalt(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************
@Function       RGXRiscvIsHalted

@Description    Check if the RISC-V FW is halted

@Input          psDevInfo       Pointer to device info

@Return         IMG_BOOL
******************************************************************************/
IMG_BOOL RGXRiscvIsHalted(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************
@Function       RGXRiscvResume

@Description    Resume the RISC-V FW core

@Input          psDevInfo       Pointer to device info

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvResume(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************
@Function       RGXRiscvReadReg

@Description    Read a value from the given RISC-V register (GPR or CSR)

@Input          psDevInfo       Pointer to device info
@Input          ui32RegAddr     RISC-V register address

@Output         pui32Value      Read value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvReadReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32RegAddr,
                             IMG_UINT32 *pui32Value);

/*!
*******************************************************************************
@Function       RGXRiscvPollReg

@Description    Poll for a value from the given RISC-V register (GPR or CSR)

@Input          psDevInfo       Pointer to device info
@Input          ui32RegAddr     RISC-V register address
@Input          ui32Value       Expected value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvPollReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32RegAddr,
                             IMG_UINT32 ui32Value);

/*!
*******************************************************************************
@Function       RGXRiscvWriteReg

@Description    Write a value to the given RISC-V register (GPR or CSR)

@Input          psDevInfo       Pointer to device info
@Input          ui32RegAddr     RISC-V register address
@Input          ui32Value       Write value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvWriteReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                              IMG_UINT32 ui32RegAddr,
                              IMG_UINT32 ui32Value);

/*!
*******************************************************************************
@Function       RGXRiscvPollMem

@Description    Poll for a value at the given address in RISC-V memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Expected value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvPollMem(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32Addr,
                             IMG_UINT32 ui32Value);

#if !defined(EMULATOR)
/*!
*******************************************************************************
@Function       RGXRiscvReadMem

@Description    Read a value at the given address in RISC-V memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space

@Output         pui32Value      Read value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvReadMem(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32Addr,
                             IMG_UINT32 *pui32Value);

/*!
*******************************************************************************
@Function       RGXRiscvWriteMem

@Description    Write a value to the given address in RISC-V memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Write value

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvWriteMem(PVRSRV_RGXDEV_INFO *psDevInfo,
                              IMG_UINT32 ui32Addr,
                              IMG_UINT32 ui32Value);
#endif /* !defined(EMULATOR) */

/*!
*******************************************************************************
@Function       RGXRiscvDmiOp

@Description    Acquire the powerlock and perform an operation on the RISC-V
                Debug Module Interface, but only if the GPU is powered on.

@Input          psDevInfo       Pointer to device info
@InOut          pui64DMI        Encoding of a request for the RISC-V Debug
                                Module with same format as the 'dmi' register
                                from the RISC-V debug specification (v0.13+).
                                On return, this is updated with the result of
                                the request, encoded the same way.

@Return         PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXRiscvDmiOp(PVRSRV_RGXDEV_INFO *psDevInfo,
                           IMG_UINT64 *pui64DMI);

#endif /* RGXFWRISCV_H */
/******************************************************************************
 End of file (rgxfwriscv.h)
******************************************************************************/
