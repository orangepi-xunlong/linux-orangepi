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

#ifndef _FISHEYE_ALGO_H_
#define _FISHEYE_ALGO_H_

#define _LIMIT_CHECK(x, y, xmin, xmax, ymin, ymax)                             \
	{                                                                      \
		if ((x) < (xmin + 2))                                          \
			(x) = (xmin) + 2;                                      \
		if ((x) > (xmax - 2))                                          \
			(x) = (xmax)-2;                                        \
		if ((y) < (ymin + 2))                                          \
			(y) = (ymin) + 2;                                      \
		if ((y) > (ymax - 2))                                          \
			(y) = (ymax)-2;                                        \
	}

#define POINT_TRANSFORM(xa, ya, x, y)                                          \
	{                                                                      \
		float x1, y1, z1;                                              \
		x1 = projectH[0] * x + projectH[1] * y + projectH[2];          \
		y1 = projectH[3] * x + projectH[4] * y + projectH[5];          \
		z1 = projectH[6] * x + projectH[7] * y + projectH[8];          \
		xa = x1 / z1;                                                  \
		ya = y1 / z1;                                                  \
	}

void FISHEYE_GET_SOURCE_POS(float *src_h, float *src_v, float des_h,
			    float des_v, pDistoration_t distoration,
			    int32 count);
void FISHEYE_GET_DISTORATION(float *xSrc, float *ySrc, float centerSrcX,
			     float centerSrcY, float xPos, float yPos,
			     float scaleH, float scaleV, float pixelH,
			     float pixelV, pDistoration_t distoration,
			     int32 count);

#endif
