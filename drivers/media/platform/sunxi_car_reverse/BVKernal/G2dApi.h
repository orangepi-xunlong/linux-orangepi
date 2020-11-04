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

#ifndef __G2DAPI_H__
#define __G2DAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RECT {
	int left;
	int top;
	int width;  /* preview width */
	int height; /* preview height */
} stRECT;

typedef struct V4L2BUF {
	unsigned long addrPhyY; /* physical Y address of this frame */
	unsigned long addrPhyC; /* physical Y address of this frame */
	unsigned long addrVirY; /* virtual Y address of this frame */
	unsigned long addrVirC; /* virtual Y address of this frame */
	unsigned int width;
	unsigned int height;
	int index;	   /* DQUE id number */
	long long timeStamp; /* time stamp of this frame */
	stRECT crop_rect;
	int format;
	void *overlay_info;

	/* thumb */
	unsigned char isThumbAvailable;
	unsigned char thumbUsedForPreview;
	unsigned char thumbUsedForPhoto;
	unsigned char thumbUsedForVideo;
	unsigned long thumbAddrPhyY; /* physical Y address of thumb buffer */
	unsigned long thumbAddrVirY; /* virtual Y address of thumb buffer */
	unsigned int thumbWidth;
	unsigned int thumbHeight;
	stRECT thumb_crop_rect;
	int thumbFormat;

	int refCnt;		/* used for releasing this frame */
	unsigned int bytesused; /* used by compressed source */
	int nDmaBufFd;		/* dma fd callback to codec */
	int nShareBufFd;	/* share fd callback to codec */
	void *buf_node;
} stV4L2BUF;

typedef enum {
	G2D_ROTATE90,
	G2D_ROTATE180,
	G2D_ROTATE270,
	G2D_FLIP_HORIZONTAL,
	G2D_FLIP_VERTICAL,
	G2D_MIRROR45,
	G2D_MIRROR135,
} g2dRotateAngle;

typedef struct G2dOpsS_ {
	int (*fpG2dInit)(void);
	int (*fpG2dUnit)(int g2dHandle);
	int (*fpG2dAllocBuff)(stV4L2BUF *bufHandle, int width, int hight);
	int (*fpG2dFreeBuff)(stV4L2BUF *bufHandle);
	void *(*fpG2dMalloc)(void **handle, int size);
	void (*fpG2dFree)(void *bufHandle);
	int (*fpG2dScale)(int g2dHandle, unsigned char *src, int src_width,
			  int src_height, unsigned char *dst, int dst_width,
			  int dst_height);
	int (*fpG2dClip)(int g2dHandle, void *psrc, int src_w, int src_h,
			 int src_x, int src_y, int width, int height,
			 void *pdst, int dst_w, int dst_h, int dst_x,
			 int dst_y);
	int (*fpG2dRotate)(int g2dHandle, g2dRotateAngle angle,
			   unsigned char *src, int src_width, int src_height,
			   unsigned char *dst, int dst_width, int dst_height);
} G2dOpsS;

#ifdef __cplusplus
}
#endif

#endif
