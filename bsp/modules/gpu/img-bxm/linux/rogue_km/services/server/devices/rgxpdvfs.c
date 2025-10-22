/*************************************************************************/ /*!
@File           rgxpdvfs.c
@Title          RGX Proactive DVFS Functionality
@Codingstyle    IMG
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Kernel mode Proactive DVFS Functionality.
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

#include "rgxpdvfs.h"
#include "rgxfwutils.h"
#include "rgxpower.h"
#include "rgx_options.h"
#include "rgxtimecorr.h"

#define USEC_TO_MSEC 1000

PVRSRV_ERROR PDVFSLimitMaxFrequency(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32MaxOPPPoint)
{
	RGXFWIF_KCCB_CMD		sGPCCBCmd;
	PVRSRV_ERROR			eError;
	IMG_UINT32				ui32CmdKCCBSlot;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

	if (!_PDVFSEnabled())
	{
		/* No error message to avoid excessive messages */
		return PVRSRV_OK;
	}

	/* send feedback */
	sGPCCBCmd.eCmdType = RGXFWIF_KCCB_CMD_PDVFS_LIMIT_MAX_FREQ;
	sGPCCBCmd.uCmdData.sPDVFSMaxFreqData.ui32MaxOPPPoint = ui32MaxOPPPoint;

	/* Submit command to the firmware.  */
	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
		                                      &sGPCCBCmd,
		                                      PDUMP_FLAGS_CONTINUOUS,
		                                      &ui32CmdKCCBSlot);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

PVRSRV_ERROR PDVFSLimitMinFrequency(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32MinOPPPoint)
{
	RGXFWIF_KCCB_CMD		sGPCCBCmd;
	PVRSRV_ERROR			eError;
	IMG_UINT32				ui32CmdKCCBSlot;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

	if (!_PDVFSEnabled())
	{
		/* No error message to avoid excessive messages */
		return PVRSRV_OK;
	}

	/* send feedback */
	sGPCCBCmd.eCmdType = RGXFWIF_KCCB_CMD_PDVFS_LIMIT_MIN_FREQ;
	sGPCCBCmd.uCmdData.sPDVFSMinFreqData.ui32MinOPPPoint = ui32MinOPPPoint;

	/* Submit command to the firmware.  */
	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
		                                      &sGPCCBCmd,
		                                      PDUMP_FLAGS_CONTINUOUS,
		                                      &ui32CmdKCCBSlot);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

#if defined(RGXFW_META_SUPPORT_2ND_THREAD)
/*************************************************************************/ /*!
@Function       RGXPDVFSCheckCoreClkRateChange
@Description    Checks if core clock rate has changed since the last snap-shot.
@Input          psDevInfo    A pointer to PVRSRV_RGXDEV_INFO.
@Return         None.
*/ /**************************************************************************/
void RGXPDVFSCheckCoreClkRateChange(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_UINT32 ui32CoreClkRate = *psDevInfo->pui32RGXFWIFCoreClkRate;

	if (!_PDVFSEnabled())
	{
		/* No error message to avoid excessive messages */
		return;
	}

	if (ui32CoreClkRate != 0 && psDevInfo->ui32CoreClkRateSnapshot != ui32CoreClkRate)
	{
		psDevInfo->ui32CoreClkRateSnapshot = ui32CoreClkRate;
		RGX_PROCESS_CORE_CLK_RATE_CHANGE(psDevInfo, ui32CoreClkRate);
	}
}
#endif
