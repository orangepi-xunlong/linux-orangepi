/*************************************************************************/ /*!
@File
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

#include <linux/io.h>
#include <linux/module.h>

#include RGX_BVNC_CORE_KM_HEADER
#include RGX_BNC_CONFIG_KM_HEADER
#include "rgxdefs_km.h"
#include "rgx_cr_defs_km.h"
#include "rgxlayer.h"

#include "tee_ddk.h"
#include "tee_rgx.h"
#include "tee_fw_premap.h"
#include "rgxfwimageutils.h"
#include "rgxstartstop.h"


static void *fwcode, *fwdata, *fwcorememcode, *fwcorememdata;
void *regbank = NULL;
PVRSRV_DEVICE_FEATURE_CONFIG *psDevFeatureCfg = NULL;

PVRSRV_ERROR TEE_LoadFirmware(IMG_HANDLE hSysData, PVRSRV_FW_PARAMS *psTDFWParams)
{
	SYS_DATA *psSysData = hSysData;
	PVRSRV_ERROR eError;
	IMG_UINT64 fwcodesize, fwdatasize, fwcorememcodesize, fwcorememdatasize;
	IMG_UINT64 fwcodepa, fwdatapa, fwcorememcodepa, fwcorememdatapa;
	RGX_FW_INFO_HEADER sFWInfoHeader;
	IMG_UINT64 ui64FwBinarySize;

	if (psSysData == NULL)
	{
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto exit;
	}

	/* retrieve the core configuration data */
	psDevFeatureCfg = &psSysData->sDevFeatureCfg;

	RGXGetFWImageAllocSize(NULL,
	                       psTDFWParams->pvFirmware,
	                       psTDFWParams->ui32FirmwareSize,
	                       &fwcodesize,
	                       &fwdatasize,
	                       &fwcorememcodesize,
	                       &fwcorememdatasize,
	                       &sFWInfoHeader);

	ui64FwBinarySize = (fwcodesize+fwdatasize+fwcorememcodesize+fwcorememdatasize);
	if (psSysData->ui64FwPrivateHeapSize < ui64FwBinarySize)
	{
		eError = PVRSRV_ERROR_INSUFFICIENT_PHYS_HEAP_MEMORY;
		RGXErrorLog(NULL, "%s: Firmware Private heap too small to fit the Fw binary"
		                  " (heap=0x%llX; required=0x%llX)", __func__,
		                   psSysData->ui64FwPrivateHeapSize, ui64FwBinarySize);
		goto exit;
	}

	fwcodepa = psSysData->ui64FwHeapCpuBase;

#if defined(RGX_FEATURE_META)
	fwdatapa = psSysData->ui64FwHeapCpuBase +
	           (psTDFWParams->uFWP.sMeta.sFWDataDevVAddr.uiAddr - FWHEAP_GPU_VA);
	fwcorememcodepa = psSysData->ui64FwHeapCpuBase +
	                  (psTDFWParams->uFWP.sMeta.sFWCorememCodeDevVAddr.uiAddr - FWHEAP_GPU_VA);
	fwcorememdatapa = psSysData->ui64FwHeapCpuBase +
	                  (psTDFWParams->uFWP.sMeta.sFWCorememDataDevVAddr.uiAddr - FWHEAP_GPU_VA);
#elif defined(RGX_FEATURE_RISCV_FW_PROCESSOR)
	fwdatapa = psSysData->ui64FwHeapCpuBase +
	           (psTDFWParams->uFWP.sRISCV.sFWDataDevVAddr.uiAddr - FWHEAP_GPU_VA);

	fwcorememcodepa = psSysData->ui64FwHeapCpuBase +
	                  (psTDFWParams->uFWP.sRISCV.sFWCorememCodeDevVAddr.uiAddr - FWHEAP_GPU_VA);

	fwcorememdatapa = psSysData->ui64FwHeapCpuBase +
	                  (psTDFWParams->uFWP.sRISCV.sFWCorememDataDevVAddr.uiAddr - FWHEAP_GPU_VA);
#else
#error "Unsupported FW CPU architecture."
#endif

	/* CPU mappings */
	fwcode = (void __iomem *)ioremap(fwcodepa, fwcodesize);
	if (fwcode == NULL)
	{
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto exit;
	}
	memset(fwcode, 0, fwcodesize);

	fwdata = (void __iomem *)ioremap(fwdatapa, fwdatasize);
	if (fwdata == NULL)
	{
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto fwdata_fail;
	}
	memset(fwdata, 0, fwdatasize);

	fwcorememcode = (void __iomem *)ioremap(fwcorememcodepa, fwcorememcodesize);
	if (fwcorememcode == NULL)
	{
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto fwcorememcode_fail;
	}
	memset(fwcorememcode, 0, fwcorememcodesize);

	fwcorememdata = (void __iomem *)ioremap(fwcorememdatapa, fwcorememdatasize);
	if (fwcorememdata == NULL)
	{
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto fwcorememdata_fail;
	}
	memset(fwcorememdata, 0, fwcorememdatasize);

	/* Load the FW code in secure memory */
	eError = RGXProcessFWImage(NULL,
	                           psTDFWParams->pvFirmware,
	                           fwcode,
	                           fwdata,
	                           fwcorememcode,
	                           fwcorememdata,
	                           &psTDFWParams->uFWP);

	iounmap(fwcorememdata);
fwcorememdata_fail:
	iounmap(fwcorememcode);
fwcorememcode_fail:
	iounmap(fwdata);
fwdata_fail:
	iounmap(fwcode);
exit:
	return eError;
}

PVRSRV_ERROR TEE_SetPowerParams(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams)
{
	SYS_DATA *psSysData = hSysData;

	return (psSysData->ui32SysDataSize == sizeof(SYS_DATA)) ?
	        PVRSRV_OK : PVRSRV_ERROR_BAD_PARAM_SIZE;
}

PVRSRV_ERROR TEE_RGXStart(IMG_HANDLE hSysData)
{
	PVRSRV_ERROR eErr;
	SYS_DATA *psSysData = hSysData;

	regbank = (void __iomem *)ioremap(psSysData->ui64GpuRegisterBase, FPGA_RGX_REG_SIZE);
	if (regbank == NULL)
	{
		RGXErrorLog(NULL, "%s: failed to map regbank memory", __func__);
		return PVRSRV_ERROR_BAD_MAPPING;
	}

	eErr = PVRSRVConfigureMMU(psSysData);
	if (eErr != PVRSRV_OK)
	{
		RGXErrorLog(NULL, "%s: PVRSRVConfigureMMU() failed (%u)", __func__, eErr);
		return eErr;
	}

	return RGXStart(hSysData);
}

PVRSRV_ERROR TEE_RGXStop(IMG_HANDLE hSysData)
{
	PVRSRV_ERROR eErr = RGXStop(hSysData);
	iounmap(regbank);

	return eErr;
}

static int __init tee_init(void)
{
	return 0;
}

static void __exit tee_exit(void)
{
}

EXPORT_SYMBOL(TEE_LoadFirmware);
EXPORT_SYMBOL(TEE_SetPowerParams);
EXPORT_SYMBOL(TEE_RGXStart);
EXPORT_SYMBOL(TEE_RGXStop);

module_init(tee_init);
module_exit(tee_exit);

MODULE_DESCRIPTION("TEE-DDK functionality test module");
MODULE_LICENSE("Dual MIT/GPL");
