/*************************************************************************/ /*!
@File           htbserver.h
@Title          Host Trace Buffer server implementation.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved

@Description    Host Trace Buffer provides a mechanism to log Host events to a
                buffer in a similar way to the Firmware Trace mechanism.
                Host Trace Buffer logs data using a Transport Layer buffer.
                The Transport Layer and pvrtld tool provides the mechanism to
                retrieve the trace data.

                A Host Trace can be merged with a corresponding Firmware Trace.
                This is achieved by inserting synchronisation data into both
                traces and post processing to merge them.

                The FW Trace will contain a "Sync Partition Marker". This is
                updated every time the RGX is brought out of reset (RGX clock
                timestamps reset at this point) and is repeated when the FW
                Trace buffer wraps to ensure there is always at least 1
                partition marker in the Firmware Trace buffer whenever it is
                read.

                The Host Trace will contain corresponding "Sync Partition
                Markers" - #HTBSyncPartitionMarker(). Each partition is then
                subdivided into "Sync Scale" sections - #HTBSyncScale(). The
                "Sync Scale" data allows the timestamps from the two traces to
                be correlated. The "Sync Scale" data is updated as part of the
                standard RGX time correlation code (rgxtimecorr.c) and is
                updated periodically including on power and clock changes.

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

#ifndef HTBSERVER_H
#define HTBSERVER_H

#include "img_types.h"
#include "pvrsrv_error.h"
#include "htbuffer_types.h"

#define HTBLOGK(SF, args...) do { if (HTB_GROUP_ENABLED(SF)) { (void)HTBLogSimple(SF, ## args); } } while (0)

/* macros to cast 64 or 32-bit pointers into 32-bit integer components for Host Trace */
#define HTBLOG_PTR_BITS_HIGH(p) ((IMG_UINT32)((((IMG_UINT64)((uintptr_t)p))>>32)&0xffffffff))
#define HTBLOG_PTR_BITS_LOW(p)  ((IMG_UINT32)(((IMG_UINT64)((uintptr_t)p))&0xffffffff))

/* macros to cast 64-bit integers into 32-bit integer components for Host Trace */
#define HTBLOG_U64_BITS_HIGH(u) ((IMG_UINT32)((u>>32)&0xffffffff))
#define HTBLOG_U64_BITS_LOW(u)  ((IMG_UINT32)(u&0xffffffff))

/* Host Trace Buffer name */
#define HTB_STREAM_NAME	"PVRHTBuffer"

/************************************************************************/ /*!
 @Function      HTBInit
 @Description   Initialise the Host Trace Buffer and allocate all resources

 @Return        eError          Internal services call returned eError error
                                number
*/ /**************************************************************************/
PVRSRV_ERROR
HTBInit_Impl(void);

/************************************************************************/ /*!
 @Function      HTBDeInit
 @Description   Close the Host Trace Buffer and free all resources

 @Return        eError          Internal services call returned eError error
                                number
*/ /**************************************************************************/
PVRSRV_ERROR
HTBDeInit_Impl(void);

/*************************************************************************/ /*!
 @Function      HTBControlKM
 @Description   Update the configuration of the Host Trace Buffer

 @Input         ui32NumFlagGroups Number of group enable flags words

 @Input         aui32GroupEnable  Flags words controlling groups to be logged

 @Input         ui32LogLevel    Log level to record

 @Input         ui32EnablePID   PID to enable logging for a specific process

 @Input         eLogMode        Enable logging for all or specific processes,

 @Input         eOpMode         Control the behaviour of the data buffer

 @Return        eError          Internal services call returned eError error
                                number
*/ /**************************************************************************/
PVRSRV_ERROR
HTBControlKM_Impl(const IMG_UINT32 ui32NumFlagGroups,
			 const IMG_UINT32 *aui32GroupEnable,
			 const IMG_UINT32 ui32LogLevel,
			 const IMG_UINT32 ui32EnablePID,
			 const HTB_LOGMODE_CTRL eLogMode,
			 const HTB_OPMODE_CTRL eOpMode);


/*************************************************************************/ /*!
 @Function      HTBSyncPartitionMarker
 @Description   Write an HTB sync partition marker to the HTB log

 @Input         ui32Marker      Marker value

*/ /**************************************************************************/
void
HTBSyncPartitionMarker_Impl(const IMG_UINT32 ui32Marker);

/*************************************************************************/ /*!
 @Function      HTBSyncPartitionMarkerRpt
 @Description   Write a HTB sync partition marker to the HTB log, given
                the previous values to repeat.

 @Input         ui32Marker      Marker value
 @Input         ui64SyncOSTS    previous OSTS
 @Input         ui64SyncCRTS    previous CRTS
 @Input         ui32ClkSpeed    previous Clockspeed

*/ /**************************************************************************/
void
HTBSyncPartitionMarkerRepeat_Impl(const IMG_UINT32 ui32Marker,
							 const IMG_UINT64 ui64SyncOSTS,
							 const IMG_UINT64 ui64SyncCRTS,
							 const IMG_UINT32 ui32ClkSpeed);

/*************************************************************************/ /*!
 @Function      HTBSyncScale
 @Description   Write FW-Host synchronisation data to the HTB log when clocks
                change or are re-calibrated

 @Input         bLogValues      IMG_TRUE if value should be immediately written
                                out to the log

 @Input         ui64OSTS        OS Timestamp

 @Input         ui64CRTS        Rogue timestamp

 @Input         ui32CalcClkSpd  Calculated clock speed

*/ /**************************************************************************/
void
HTBSyncScale_Impl(const IMG_BOOL bLogValues, const IMG_UINT64 ui64OSTS,
			 const IMG_UINT64 ui64CRTS, const IMG_UINT32 ui32CalcClkSpd);

/*************************************************************************/ /*!
 @Function      HTBLogSimple
 @Description   Record a Host Trace Buffer log event with implicit PID and Timestamp

 @Input         SF              The log event ID

 @Input         ...             Log parameters

 @Return        PVRSRV_OK       Success.

*/ /**************************************************************************/
IMG_INTERNAL PVRSRV_ERROR
HTBLogSimple_Impl(IMG_UINT32 SF, ...);

/*  DEBUG log group enable */
#if !defined(HTB_DEBUG_LOG_GROUP)
#undef HTB_LOG_TYPE_DBG    /* No trace statements in this log group should be checked in */
#define HTB_LOG_TYPE_DBG    __BUILDERROR__
#endif

#if defined(PVRSRV_ENABLE_HTB)
/*************************************************************************/ /*!
 @Function      HTBIsConfigured
 @Description   Determine if HTB stream has been configured

 @Input         none

 @Return        IMG_FALSE       Stream has not been configured
                IMG_TRUE        Stream has been configured

*/ /**************************************************************************/
IMG_BOOL
HTBIsConfigured_Impl(void);

#define HTBIsConfigured HTBIsConfigured_Impl
#define HTBLogSimple HTBLogSimple_Impl
#define HTBSyncScale(bLogValues, ui64OSTS, ui64CRTS, ui32CalcClkSpd) \
  HTBSyncScale_Impl((bLogValues), (ui64OSTS), (ui64CRTS), (ui32CalcClkSpd))
#define HTBSyncPartitionMarkerRepeat(ui32Marker, ui64SyncOSTS, ui64SyncCRTS, ui32ClkSpeed) \
  HTBSyncPartitionMarkerRepeat_Impl((ui32Marker), (ui64SyncOSTS), (ui64SyncCRTS), (ui32ClkSpeed))
#define HTBSyncPartitionMarker(a) HTBSyncPartitionMarker_Impl((a))
#define HTBControlKM(ui32NumFlagGroups, aui32GroupEnable, ui32LogLevel, ui32EnablePID, eLogMode, eOpMode) \
  HTBControlKM_Impl((ui32NumFlagGroups), (aui32GroupEnable), (ui32LogLevel), (ui32EnablePID), (eLogMode), (eOpMode))
#define HTBInit() HTBInit_Impl()
#define HTBDeInit() HTBDeInit_Impl()
#else	/* !PVRSRV_ENABLE_HTB) */
#define HTBIsConfigured()  IMG_FALSE
#define HTBLogSimple(SF, args...)  PVRSRV_OK
#define HTBSyncScale(a, b, c, d)
#define HTBSyncPartitionMarkerRepeat(a, b, c, d)
#define HTBSyncPartitionMarker(a)
#define HTBControlKM(a, b, c, d, e, f) PVRSRV_OK
#define HTBDeInit() PVRSRV_OK
#define HTBInit() PVRSRV_OK
#endif	/* PVRSRV_ENABLE_HTB */
#endif /* HTBSERVER_H */

/* EOF */
