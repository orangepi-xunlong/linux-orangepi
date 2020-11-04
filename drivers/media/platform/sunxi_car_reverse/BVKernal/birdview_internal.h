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

#ifndef _BIRDVIEW_INTERNAL_H_
#define _BIRDVIEW_INTERNAL_H_

#define DEFAULT_MARGIN_LENGTH                                                  \
	(50 / 100) /* percentage of border used for stitching comparison */
#define DEFAULT_MARGIN_WIDTH 10 /* in pixel */
#define DEFAULT_PEAK_ZONE                                                      \
	(50 / 100) /* percentage of border used for stitching comparison */
#define DEFAULT_EDGE_SIZE 5
#define POPULATION_RATIO 5

/* Final Histo Filtering */
#define DISCONT_FILTER_LENGTH 20
#define AREA_SIZE 8
#define ALL_CHECKED ~0
#define SECTOR_DISCONTINUITY 30
#define EDGE_DISCONTINUITY 80

#if USE_BLEND
/* For blend */
#define STITCH_COMMON_HEIGHT 64
#define SHIFT_BIT 5
#define STITCH_BLEND_HEIGHT 32 /* 2^SHIT_BIT */
#define STITCH_SCALE (STITCH_COMMON_HEIGHT / STITCH_BLEND_HEIGHT)
#else
#define STITCH_COMMON_HEIGHT 0
#define SHIFT_BIT 0
#define STITCH_BLEND_HEIGHT 0 /* 2^SHIT_BIT */
#define STITCH_SCALE 0
#endif
#define HISTO_AVERAGE 5

/*Bridview configurate parameters*/
typedef struct _BV_Instance_t {
	pBV_Config_t pCfg; /*birdview configuration*/
	int16 mLastY;      /*last total birdview Y*/
	int16 mLastU;      /*last total birdview U*/
	int16 mLastV;      /*last total birdview V*/

	/* LUT computation Parameters */
	int32 *mMagicLUTd;
	void *mMagicLUTdHandle;
	uint32 mMagicLUTSize;
	/*LUT stop position for each camera*/
	uint32 mEndIdxCam1, mEndIdxCam2, mEndIdxCam3, mEndIdxCam4;
	uint32 mEndIdxUVCam1, mEndIdxUVCam2, mEndIdxUVCam3, mEndIdxUVCam4;
	uint32 mBlendIdxCam1, mBlendUVIdxCam1, mBlendIdxCam3, mBlendUVIdxCam3;

	/*histogram param*/
	int8 mShiftRIdx[4], mShiftGIdx[4], mShiftBIdx[4];
	int8 mShiftRCount[4], mShiftGCount[4], mShiftBCount[4];
	/* Time Filtering Variables */
	int8 mShiftAvgR[4][DISCONT_FILTER_LENGTH],
	mShiftAvgG[4][DISCONT_FILTER_LENGTH],
	mShiftAvgB[4][DISCONT_FILTER_LENGTH];

	/*Hostogram LUT*/
	uint16 *mHistoMagicLUT;
	void *mHistoMagicLUTHandle;
	/*LUT stop position for each camera left side*/
	uint32 mPeakEndIdxCam1, mPeakEndIdxCam2, mPeakEndIdxCam3,
	mPeakEndIdxCam4, mPeakEndIdxCam5, mPeakEndIdxCam6, mPeakEndIdxCam7,
	mPeakEndIdxCam8;
	uint32 mPeakEndIdxUVCam1, mPeakEndIdxUVCam2, mPeakEndIdxUVCam3,
	mPeakEndIdxUVCam4, mPeakEndIdxUVCam5, mPeakEndIdxUVCam6,
	mPeakEndIdxUVCam7, mPeakEndIdxUVCam8;

	/*LUT stop position for each camera right side*/
	uint32 mHistoEndIdxCam1, mHistoEndIdxCam2, mHistoEndIdxCam3,
	mHistoEndIdxCam4, mHistoEndIdxCam5, mHistoEndIdxCam6,
	mHistoEndIdxCam7, mHistoEndIdxCam8;
	uint32 mHistoEndIdxUVCam1, mHistoEndIdxUVCam2, mHistoEndIdxUVCam3,
	mHistoEndIdxUVCam4, mHistoEndIdxUVCam5, mHistoEndIdxUVCam6,
	mHistoEndIdxUVCam7, mHistoEndIdxUVCam8;

	/* YUV Shift Parameters */
	int8 ColorShift_U[4];
	int8 ColorShift_V[4];
	int8 ColorShift_Y[4];

	/* frame counter, do histogram every HISTO_AVERAGE frames */
	uint16 histoAverage;
} BV_Instance_t, *pBV_Instance_t;

int32 BirdviewCreate(void **instHandle, int32 *inst);
int32 BirdviewConfig(int32 inst, pBV_Config_t pcfg);
int32 BirdviewDistory(int32 instHandle, int32 inst);
int32 ComputeCombinedLUT(int32 inst, pBV_Config_t pCfg, int32 viewId,
	int32 inpW, int32 inpH, int32 outW, int32 outH,
	int32 inpPitch, int32 outPitch, int32 ChromaMode,
	int32 fourSrcData);

int32 RenderBirdview(int32 inst, void *srcY1, void *srcUV11, void *srcUV21,
	void *srcY2, void *srcUV12, void *srcUV22, void *srcY3,
	void *srcUV13, void *srcUV23, void *srcY4, void *srcUV14,
	void *srcUV24, void *dstY, void *dstUV, void *dstUV2,
	int32 inpW, int32 inpH, int32 inpPitch, int32 outW,
	int32 outH, int32 outPitch);

#endif
