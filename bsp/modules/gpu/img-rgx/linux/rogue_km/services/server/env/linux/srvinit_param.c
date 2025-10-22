/*************************************************************************/ /*!
@File
@Title          Services initialisation parameter routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific functions
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

/*
 * This is an example file which is presented as a means of providing the
 * necessary access mechanisms to allow Services Server settings to
 * be set and/or queried in non-Linux operating systems.
 *
 * The access mechanisms detailed here require the same functionality present
 * in the Services Client/UM to access AppHints. This is an example and it may
 * or may not be appropriate for the target OS. The implementation approach
 * taken needs to be considered in the context of the target OS.
 *
 * Consult the PowerVR Rogue DDK Services AppHint Guide for more details on
 * the Application Hints subsytem and the
 * Rogue DDK Services OS Porting Reference for explanations of the interface(s)
 * provided by the routines shown in this file.
 */

#include "img_defs.h"
#include "pvr_debug.h"
#include "allocmem.h"

#include "osfunc.h"
#include "os_srvinit_param.h"

void *
SrvInitParamOpen(void)
{
	void *pvHintState = NULL;

	PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);

	return pvHintState;
}

void
SrvInitParamClose(void *pvState)
{
	PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvState);
}

void
_SrvInitParamGetBOOL(void *pvState, const IMG_CHAR *pszName, const IMG_BOOL *pbDefault, IMG_BOOL *pbValue)
{
	(void) PVRSRVGetAppHint(pvState, pszName, IMG_UINT_TYPE, pbDefault, pbValue);
}

void
_SrvInitParamGetUINT32(void *pvState, const IMG_CHAR *pszName, const IMG_UINT32 *pui32Default, IMG_UINT32 *pui32Value)
{
	(void) PVRSRVGetAppHint(pvState, pszName, IMG_UINT_TYPE, pui32Default, pui32Value);
}

void
_SrvInitParamGetSTRING(void *pvState, const IMG_CHAR *pszName, const IMG_CHAR **psDefault, IMG_CHAR *pBuffer, size_t size)
{
	PVR_UNREFERENCED_PARAMETER(size);
	(void) PVRSRVGetAppHint(pvState, pszName, IMG_STRING_TYPE, psDefault, pBuffer);
}

void
_SrvInitParamGetUINT32BitField(void *pvState, const IMG_CHAR *pszBaseName, IMG_UINT32 uiDefault, const SRV_INIT_PARAM_UINT32_LOOKUP *psLookup, IMG_UINT32 uiSize, IMG_UINT32 *puiValue)
{
	IMG_UINT32 uiValue = uiDefault;
	IMG_CHAR *pszName;
	const IMG_UINT32 baseLen = strlen(pszBaseName);
	IMG_UINT32 extraLen = 0;
	IMG_UINT32 i;

	for (i = 0; i < uiSize; i++)
	{
		unsigned len = strlen(psLookup[i].pszValue);

		if (len > extraLen)
		{
			extraLen = len;
		}
	}
	extraLen++;

	pszName = OSAllocMem(baseLen + extraLen);

	if (pszName != NULL)
	{
		(void) strcpy(pszName, pszBaseName);

		for (i = 0; i < uiSize; i++)
		{
			const IMG_CHAR *pszExtra = psLookup[i].pszValue;
			IMG_UINT32 uiBDefault = 0;
			IMG_UINT32 uiBValue;
			IMG_UINT32 j;

			for (j = 0; pszExtra[j] != 0; j++)
			{
				pszName[baseLen + j] = toupper(pszExtra[j]);
			}
			pszName[baseLen + j] = 0;

			PVRSRVGetAppHint(pvState, pszName, IMG_UINT_TYPE, &uiBDefault, &uiBValue);

			if (uiBValue != 0)
			{
				uiValue |= psLookup[i].ui32Value;
			}
		}

		OSFreeMem(pszName);
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "Failed to get apphint %s, will use the default value", pszBaseName));
	}

	*puiValue = uiValue;
}

void
_SrvInitParamGetUINT32List(void *pvState, const IMG_CHAR *pszName, IMG_UINT32 uiDefault, const SRV_INIT_PARAM_UINT32_LOOKUP *psLookup, IMG_UINT32 uiSize, IMG_UINT32 *puiValue)
{
	IMG_UINT32 uiValue = uiDefault;
	IMG_CHAR acValue[APPHINT_MAX_STRING_SIZE];
	IMG_UINT32 i;

	if (PVRSRVGetAppHint(pvState, pszName, IMG_STRING_TYPE, "", &acValue))
	{
		IMG_CHAR *pszParam;

		acValue[APPHINT_MAX_STRING_SIZE - 1] = '\0';

		pszParam = &acValue[strlen(acValue)];

		/* Strip trailing blanks */
		while (pszParam >= &acValue[0])
		{
			if (*pszParam != '\0' && *pszParam != ' ' && *pszParam != '\t')
			{
				break;
			}

			*(pszParam--) = '\0';
		}

		pszParam = &acValue[0];

		/* Strip leading blanks */
		while (*pszParam == ' ' || *pszParam == '\t')
		{
			pszParam++;
		}

		for (i = 0; i < uiSize; i++)
		{
			if (strcmp(pszParam, psLookup[i].pszValue) == 0)
			{
				uiValue = psLookup[i].ui32Value;
				break;
			}
		}
		if (i ==  uiSize)
		{
			if (strlen(acValue) == 0)
			{
				PVR_DPF((PVR_DBG_WARNING, "No value set for initialisation parameter %s", pszName));
			}
			else
			{
				PVR_DPF((PVR_DBG_WARNING, "Unrecognised value (%s) for initialisation parameter %s", acValue, pszName));
			}
		}
	}

	*puiValue = uiValue;

}
