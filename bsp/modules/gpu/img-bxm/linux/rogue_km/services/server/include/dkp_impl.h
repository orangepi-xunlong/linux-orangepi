/**************************************************************************/ /*!
@File           dkp_impl.h
@Title          Functions for supporting the DRM Key Provider
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

#ifndef DKP_IMPL_H
#define DKP_IMPL_H

typedef IMG_HANDLE PVRDKF_DKP_HANDLE;
typedef PVRDKF_DKP_HANDLE * PPVRDKF_DKP_HANDLE;

/*! @Function DKP_PFN_SHOW
 *
 * @Description
 *
 * Describes the function called by the DKF infrastructure to request the DKP
 * outputs all the mandatory and optional keys it supports. Each
 * key / value pair should be output one per line using the provided
 * PVRDPFOutput routine.
 *
 * The function should check that the passed 'psDevNode' references the
 * same device as that associated with the module-specific 'hPrivData' handle.
 * If it doesn't, no data should be displayed.
 *
 * @Input psDevNode   Pointer to device node for which data must be shown.
 * @Input pid         Process-ID for which data must be shown.
 * @Input hPrivData   Private data passed to DKF from DKP_Register. The DKP
 *                     must use this within calls to the DKP_PFN_SHOW to limit
 *                     the data key/value pairs to only those that are relevant
 *                     to this instance.
 */
typedef void (DKP_PFN_SHOW)(struct _PVRSRV_DEVICE_NODE_ *psDevNode, int pid,
                            IMG_HANDLE hPrivData);

typedef IMG_UINT32 DKP_CONNECTION_FLAGS;

#define DKP_CONNECTION_FLAG_SYNC        BIT(0)
#define DKP_CONNECTION_FLAG_SERVICES    BIT(1)

#define DKP_CONNECTION_FLAG_INVALID     IMG_UINT32_C(0)
#define DKP_CONNECTION_FLAG_ALL (DKP_CONNECTION_FLAG_SYNC | DKP_CONNECTION_FLAG_SERVICES)

/* @Function PVRSRVRegisterDKP
 *
 * @Description
 * Registers a DKP with the specified module's DKF entry.
 *
 * @Input   hPrivData               DKP Private data handle that the DKF will pass into
 *                                  the ShowPFN when called. Can be NULL if there is no
 *                                  DKP instance private data.
 * @Input   pszDKPName              Provider name associated with the caller.
 * @Input   psShowPfn               Function to be used when the fdinfo statistics
 *                                  are queried.
 * @Input   ui32Filter              The connection types this DKP should be output
 *                                  on.
 * @Output  phDkpHandle             Location to store generated handle created by this
 *                                  function.
 *
 * @Returns  PVRSRV_ERROR
 */
PVRSRV_ERROR PVRSRVRegisterDKP(IMG_HANDLE hPrivData,
                               const char *pszDKPName,
                               DKP_PFN_SHOW *psShowPfn,
                               DKP_CONNECTION_FLAGS ui32Filter,
                               PPVRDKF_DKP_HANDLE phDkpHandle);

/* @Function PVRSRVUnRegisterDKP
 *
 * @Description
 * Removes a previously registered key from the device's DKF entry.
 *
 * @Input  hPrivData      Private data handle.
 * @Input  hDkpHandle     Handle to the DKP's registered entry.
 *                         Obtained from earlier PVRSRV_DKP_Register call.
 *
 * @Returns  PVRSRV_ERROR
 */
PVRSRV_ERROR PVRSRVUnRegisterDKP(IMG_HANDLE hPrivData,
                                 PVRDKF_DKP_HANDLE hDkpHandle);


/* @Function PVRDKPOutput
 * Wrapper function which passes the printf-style arguments to the registered
 * DKF output function associated with the registered DKP handle.
 *
 * @Input   hPrivData   Handle for associated DKP instance producing the output.
 * @Input   fmt         printf-style format string
 *
 */
void PVRDKPOutput(IMG_HANDLE hPrivData, const char *fmt, ...) __printf(2, 3);


#if !defined(__linux__) || defined(INTEGRITY_OS) || defined(__QNXNTO__)

/* Stub routines follow */
static inline PVRSRV_ERROR PVRSRVRegisterDKP(IMG_HANDLE hPrivData,
                                             const char *pszDKPName,
                                             DKP_PFN_SHOW *psShowPfn,
                                             DKP_CONNECTION_FLAGS ui32Filter,
                                             PPVRDKF_DKP_HANDLE phDkpHandle)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	PVR_UNREFERENCED_PARAMETER(pszDKPName);
	PVR_UNREFERENCED_PARAMETER(psShowPfn);
	PVR_UNREFERENCED_PARAMETER(phDkpHandle);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static inline PVRSRV_ERROR PVRSRVUnRegisterDKP(IMG_HANDLE hPrivData,
                                               PVRDKF_DKP_HANDLE hDkpHandle)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	PVR_UNREFERENCED_PARAMETER(hDkpHandle);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static inline void PVRDKPOutput(IMG_HANDLE hPrivData, const char *fmt, ...) __printf(2, 3)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	PVR_UNREFERENCED_PARAMETER(fmt);
}
#endif /* !__linux__ || INTEGRITY_OS || __QNXNTO__ */

#endif /* DKP_IMPL_H */
