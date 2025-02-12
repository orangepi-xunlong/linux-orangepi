/*************************************************************************/ /*!
@File
@Title          RGX heap (server) definitions
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
#if !defined RGX_HEAPS_SERVER_H
#define RGX_HEAPS_SERVER_H

#include "img_types.h"
#include "rgx_heaps.h"

/*
 *  Supported log2 page size values for RGX_GENERAL_NON_4K_HEAP_ID
 */
#define RGX_HEAP_PAGE_SHIFTS_DEF \
	X(4KB, 12U) \
	X(16KB, 14U) \
	X(64KB, 16U) \
	X(256KB, 18U) \
	X(1MB, 20U) \
	X(2MB, 21U)

typedef enum RGX_HEAP_PAGE_SHIFTS_TAG
{
#define X(_name, _shift) RGX_HEAP_ ## _name ## _PAGE_SHIFT = _shift,
	RGX_HEAP_PAGE_SHIFTS_DEF
#undef X
} RGX_HEAP_PAGE_SHIFTS;

/* Base and size alignment 2MB */
#define RGX_HEAP_BASE_SIZE_ALIGN 0x200000UL
#define RGX_GENERAL_SVM_BASE_SIZE_ALIGNMENT 0x8000UL

/*************************************************************************/ /*!
@Function       RGXGetValidHeapPageSizeMask
@Description    Returns a bitmask indicating all supported virtual heap page sizes.

@Return         IMG_UINT32  A 32-bit mask with enabled bits indicating valid
                            page sizes.
*/ /**************************************************************************/
static inline IMG_UINT32 RGXGetValidHeapPageSizeMask(void)
{
	/* Generates a bit mask with the values of RGX_HEAP_PAGE_SHIFTS_DEF.
	 * 0 is required for the first shift to properly bitwise OR. */
#define X(_name, _shift) | (1 << _shift)
	return 0 RGX_HEAP_PAGE_SHIFTS_DEF;
#undef X
}

#endif /* RGX_HEAPS_SERVER_H */
