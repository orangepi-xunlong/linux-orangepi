/*************************************************************************/ /*!
@File
@Title          Environment PMR functions and data
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Environment specific PMR functions and data
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

#if !defined(PMR_ENV_H)
#define PMR_ENV_H

#include "pvrsrv_memallocflags.h"
#include "powervr/mem_types.h"
#include "pmr_impl.h"

/* Structures, functions and definitions shared with the generic PMR code */
struct _PMR_ENV_
{
	/*
	 * The smp_store_release and smp_load_acquire functions are used
	 * to access this field, in order to avoid taking locks, where
	 * possible.
	 */
	void *pvDmaBufExportData;
};

typedef struct _PMR_ENV_ PMR_ENV;

#define	DECLARE_PMR_ENV_DATA(name) PMR_ENV name;

/* The generic PMR code calls these functions, via the macros that follow */
void PMREnvInitialize(PMR_ENV *psPMREnv);
void PMREnvFinalize(PMR_ENV *psPMREnv);

#define	PMR_ENV_INITIALIZE(psPMR, sEnvData) PMREnvInitialize(&psPMR->sEnvData)
#define	PMR_ENV_FINALIZE(psPMR, sEnvData) PMREnvFinalize(&psPMR->sEnvData)

/* The generic PMR code provides this function, using the macro that follows */
PMR_ENV *PMREnvGetData(PMR *psPMR);

#define	DEFINE_PMR_ENV_GET_DATA(psPMR, sEnvData) \
	PMR_ENV *PMREnvGetData(PMR *psPMR) { return &psPMR->sEnvData; }

/*
 * Structures, functions and definitions used by the environment specific
 * PMR code.
 */

void
PMREnvDmaBufSetExportData(PMR *psPMR, void *pvData);

void *
PMREnvDmaBufGetExportData(PMR *psPMR);

#endif /* !defined(PMR_ENV_H) */
