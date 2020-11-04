/*
 * Fast car reverse head file
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Golden.Ye <golden@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _BIRDVIEW_COMMON_H_
#define _BIRDVIEW_COMMON_H_

#include "G2dApi.h"
#include <linux/kernel.h>
#include <linux/string.h>

/* True and False definition */
#define FALSE 0
#define TRUE 1

/*
 * BirdView implementation
 */
/* If True, Timing measures are performed */
#define WITH_TIME FALSE

/* Rendering Flags */
/* If True, print black borders at view boundaries */
#define LINE_BORDER FALSE
/* If True, some extra pixels at the bottom are considered when rendering the
 * views */
#define WITH_CAR FALSE
/* If True, In/Out image planes are YUV Semiplanar 422 */
#define WITH_YUV_PROCESSING TRUE

#define INTERP_FACTOR 8 /* 2^INTERP_SHIFT */
#define INTERP_SHIFT 3  /* log2(INTERP_FACTOR) */

#define USE_BLEND 1
#define USE_HISTO 1

/* Look up table Flags */
/* If True, LUT are used for final rendering of BV and 4FE. */

#define WITH_MAGIC_LUT TRUE

/* Histogram Flags */
/* If True, Histogram compensation is used */

/* Note: WITH_HISTO need to be enabled for histogram balancing */
#define WITH_HISTO TRUE

/*#define WITH_HISTO FALSE */
/* If True, Histogram processing is based also on a precomputed LUT */
/* Note: WITH_MAGIC_HISTO_LUT need to be enabled for histogram balancing */
#define WITH_MAGIC_HISTO_LUT TRUE /* FALSE //TRUE */
/* If True, additional color overlays are added on the boundary lines */
#define DEBUG_HISTO FALSE

/* BV Computation Flags */
/* If True, the 4FE LUT is computed with double precision */
#define WITH_4FE_DOUBLE_PRECISION_LUT TRUE
/* If True, the BV projections are computed using double precision Computer
 * Vision matrices */
#define WITH_CV_CALIB FALSE
/* If True, the 4 inputs can be of different characteristics */
#define WITH_4FE_MULTIFOV TRUE

#define NUM_VIEWS 4
#define MATRIX_ROWS 3
#define NUM_PROJECT_ITEMS 9

#define NUM_REF_COL 6
#define NUM_REF_ROW 4
#define NUM_REF_POINTS (NUM_REF_COL * NUM_REF_ROW)

#define KERN_ERR KERN_SOH "3" /* error conditions */
#define KERN_SOH "\001"       /* ASCII Start Of Header */

/*product type*/
typedef enum _ProductType_t {
	Product_AHD,
	Product_NTSC,
	Product_PAL
} ProductType_t;

/*data srouce type*/
typedef enum _DataType_t { Data_Four, Data_One } DataType_t;

typedef struct _Rect_t {
	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;
} Rect_t, *pRect_t;

/* Define Checks ! */
#if ((WITH_MAGIC_HISTO_LUT == TRUE) &&                                         \
     ((WITH_MAGIC_LUT == FALSE) || (WITH_HISTO == FALSE)))
#error WITH_MAGIC_HISTO_LUT requires WITH_MAGIC_LUT to be TRUE.
#undef WITH_MAGIC_HISTO_LUT
#define WITH_MAGIC_HISTO_LUT FALSE
#endif /* (WITH_MAGIC_LUT == FALSE || WITH_HISTO == FALSE) */

/*
 * generic
 * same as VDpluggins.h
 */
#ifndef VD_STANDARD_TYPES_DECLARED
#define VD_STANDARD_TYPES_DECLARED
typedef unsigned long uint32;
typedef signed long int32;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned char uint8;
typedef signed char int8;
#endif

/*
 * color types and stuff
 */
typedef enum {
	XRGB8888,
	RGB888,
	YUV422,
	YUV420,
	NUM_FORMAT_TYPE
} FormatTypes_t;

#define MAX_SHIFT 120
#define _MAX(a, b) ((a) > (b) ? (a) : (b))
#define _MIN(a, b) ((a) < (b) ? (a) : (b))
#define _CLIP(v, t) ((v) = ((v) > 0 ? _MIN(v, t) : _MAX(v, -t))) /* signed */

/*
 * some malloc/free stuff
 */
#define MY_FREE(g2DOpsS, handle, _lut)                                         \
	do {                                                                   \
		if ((g2DOpsS != NULL) && (handle != NULL)) {                   \
			g2DOpsS->fpG2dFree(handle);                            \
			handle = NULL;                                         \
			_lut = NULL;                                           \
		}                                                              \
	} while (0);
#define MY_MALLOC(g2DOpsS, handle, _lut, _size, _type)                         \
	do {                                                                   \
		if (g2DOpsS != NULL) {                                         \
			MY_FREE(g2DOpsS, *handle, _lut);                       \
			_lut = (_type *)g2DOpsS->fpG2dMalloc(                  \
			    handle, (_size) * sizeof(_type));                  \
		}                                                              \
	} while (0);

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) < (b)) ? (b) : (a))
#endif

#endif /* _NXP_FAST_COMMON_H_ */
