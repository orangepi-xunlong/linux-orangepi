/*************************************************************************/ /*!
@File           pvr_dvfs_common.h
@Title          System level interface for DVFS and PDVFS
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

#ifndef PVR_DVFS_COMMON_H
#define PVR_DVFS_COMMON_H

#include "opaque_types.h"
#include "pvrsrv_error.h"
#if defined(SUPPORT_FW_OPP_TABLE) && defined(CONFIG_OF) && defined(CONFIG_PM_OPP)
#include "rgx_fwif_km.h"
#endif

struct pvr_opp_freq_table
{
	unsigned long *freq_table;
	int num_levels;
};

/*************************************************************************/ /*!
@Function       GetOPPValues

@Description    Common code to store the OPP points in the pvr_freq_table,
                for use with the IMG DVFS/devfreq implementation, or with
                the Proactive DVFS.
                Requires CONFIG_PM_OPP support in the kernel.

@Input          dev        OS Device node
@Output         min_freq   Min clock freq (Hz)
@Output         min_volt   Min voltage (V)
@Output         max_freq   Max clock freq (Hz)
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
#if defined(CONFIG_PM_OPP)
int GetOPPValues(struct device *dev,
                 unsigned long *min_freq,
                 unsigned long *min_volt,
                 unsigned long *max_freq,
                 struct pvr_opp_freq_table *pvr_freq_table);
#endif

/*************************************************************************/ /*!
@Function       DVFSCopyOPPTable

@Description    Copies the OPP points (voltage/frequency) to the target
                firmware structure RGXFWIF_OPP_INFO which must be pre-allocated.
                The source values are expected to be read into the kernel
                Power Manager from the platform's Device Tree.
                Requires CONFIG_OF and CONFIG_PM_OPP support in the kernel.

@Input          psDeviceNode       Device node
@Input          psOPPInfo          Target OPP data buffer
@Input          ui32MaxOPPLevels   Maximum number of OPP levels allowed in buffer.
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
#if defined(SUPPORT_FW_OPP_TABLE) && defined(CONFIG_OF) && defined(CONFIG_PM_OPP)
PVRSRV_ERROR DVFSCopyOPPTable(PPVRSRV_DEVICE_NODE psDeviceNode,
							  RGXFWIF_OPP_INFO   *psOPPInfo,
							  IMG_UINT32          ui32MaxOPPLevels);
#endif

#if defined(SUPPORT_PDVFS)
/*************************************************************************/ /*!
@Function       InitPDVFS

@Description    Initialise the device for Proactive DVFS support.
                Prepares the OPP table from the devicetree, if enabled.

@Input          psDeviceNode       Device node
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR InitPDVFS(PPVRSRV_DEVICE_NODE psDeviceNode);

/*************************************************************************/ /*!
@Function       InitPDVFS

@Description    Initialise the device for Proactive DVFS support.

@Input          psDeviceNode       Device node
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
void DeinitPDVFS(PPVRSRV_DEVICE_NODE psDeviceNode);
#endif

#endif /* PVR_DVFS_COMMON_H */
