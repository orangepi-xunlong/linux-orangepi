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

#ifndef _BIRDVIEW_API_H_
#define _BIRDVIEW_API_H_
#include "BVCommon.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BV_ERR_NONE 0
#define BV_ERR_NO_ENOUGH_MEM -1
#define BV_ERR_NULL_OBJ -2
#define BV_ERR_NO_CONFIG -3
#define BV_ERR_TIME_OUT -4

/*! \brief Define birdview configuration
	*
	***************************************************************************/
typedef struct _BV_Config_t {
	uint32 size; /*!< configuration size */
	uint32 crc;  /*!< CRC code of this struct */

	int32 mDistance;	      /*!< The view position from ground */
	Camera_t mCameras[NUM_VIEWS]; /*!< Four cameras configuration */
	int16 refExisted;	     /*!< Corner points are existed or not? */
	int16 matrixExisted;	  /*!< Porject matrix is existed or not? */
	float projectH[NUM_VIEWS]
		      [NUM_PROJECT_ITEMS]; /*!< Four cameras project matrix */
	float refPoints[NUM_VIEWS]
		       [NUM_REF_POINTS * 2]; /*!< Four cameras pattern points */
	int32 patternPos[NUM_VIEWS]
			[NUM_REF_POINTS *
			 2]; /*!< Four cameras real points, unit is mm */

	int32 carW;       /*!< Car width, unit is mm */
	int32 carH;       /*!< Car length, unit is mm */
	int32 sideOffset; /*!< Left and right camera offset from front of car,
	unit is mm */
	int32 carOffsetX; /*!< If car is not in the middle of birdview, give x
	offset of car, unit is mm */
	int32 carOffsetY; /*!< Y offset of car, unit is mm */

	int32
	    mFrontOffset;  /*!< Front strips position from top, unit is pixel */
	int32 mRearOffset; /*!< Reverse strips position from bottom, unit is
	pixel */

	int32 inpW;    /*!< Input image width*/
	int32 inpH;    /*!< Input image height */
	int32 inPitch; /*!< Input image stride */

	int32 outPitch; /*!< Output image stride */

	uint32 combinedViewWidth;  /*!< The total width combined birdview and
	sideview */
	uint32 combinedViewHeight; /*!< The total height combined birdview and
	sideview */

	uint32 BirdViewWidth;  /*!< Birdview width */
	uint32 BirdViewHeight; /*!< Birdview height */

	uint32 SideViewWidth;	/*!< Sideview width */
	uint32 SideViewHeight;       /*!< Sideview height */
	uint32 SideViewDisplayedFOV; /*!< Sideview FOV(Field of View) */

	uint32 SideViewUL_X; /*!< Left position of Sideview*/
	uint32 SideViewUL_Y; /*!< Top position of Sideview*/

	uint32 BirdViewUL_X; /*!< Left position of Birdview*/
	uint32 BirdViewUL_Y; /*!< Top position of Birdview*/

	uint32 externalSideview; /*!< Sideview is draw external or not?*/
	FormatTypes_t format;    /*!< Input image color space*/

	uint32 patternType; /*!< Pattern type for calibration*/
	float projectSideH[NUM_VIEWS]
			  [NUM_PROJECT_ITEMS]; /*!< Sideview project matrix*/
} BV_Config_t, *pBV_Config_t;

/*! \brief Define instance type of birdview
	*
	***************************************************************************/
typedef void *BV_Inst_t;

/*Sideview display defintion*/
typedef struct _SV_Config_t {
	/*scale sideview image*/
	float scale;
	int16 offsetX;
	int16 offsetY;
} SV_Config_t, *pSV_Config_t;

/*track definition*/
typedef struct _SV_TrackPt_t {
	uint32 colorIdx:4;
	uint32 alpha:8;
	uint32 xPos:10;
	uint32 yPos:10;
} SV_TrackPt_t, *pSV_TrackPt_t;

/*angle track point*/
typedef struct _SV_AngleTrack_t {
	uint16 ptCount;  /*total track point count*/
	uint32 ptOffset; /*Track point position offset with track data start*/
} SV_AngleTrack_t, *pSV_AngleTrack_t;

typedef struct _SV_Track_t {
	uint32 compressSize; /*track data compress size, if compress size is 0,
	trackDataSize is memory size of trackAngle, and
	track must be calculated again*/
	uint32 showRule;
	pSV_AngleTrack_t trackAux; /*Aux line data */
	uint32 auxDataSize;	/*Total aux data size, with byte*/
} SV_Track_t, *pSV_Track_t;

/*Extention struct for birdview instance*/
typedef struct _BV_ConfigExt_t {
	uint32 size;
	uint32 crc;
	uint32 showNormal;
	SV_Config_t svConfig[NUM_VIEWS]; /*Four side view config*/
	SV_Track_t svTrack;		 /*Track data*/
	uint32 reserved[64];
} BV_ConfigExt_t, *pBV_ConfigExt_t;

/*Bridview display state*/
typedef enum _BV_Display_e {
	BV_Display_None = 0,       /*!< No display*/
	BV_Display_Birdview_Front, /*!< Display Front + Birdview*/
	BV_Display_Birdview_Right, /*!< Display Right + Birdview*/
	BV_Display_Birdview_Rear,  /*!< Display Rear + Birdview*/
	BV_Display_Birdview_Left,  /*!< Display Left + Birdview*/
	BV_Display_Birdview_Only,  /*!< Display Birdview only, in the middle of
	view*/
	BV_Display_Debug_Front,    /*!< Display Front*/
	BV_Display_Debug_Right,    /*!< Display Right*/
	BV_Display_Debug_Rear,     /*!< Display Rear*/
	BV_Display_Debug_Left      /*!< Display Left*/
} BV_Display_e,
    *pBV_Display_e;

/*! \brief Define Camera index
	*
	***************************************************************************/
typedef enum _Camera_Index_e {
	Camera_Index_Front = 0x1, /*!< Front camera*/
	Camera_Index_Right = 0x2, /*!< Right camera*/
	Camera_Index_Rear = 0x4,  /*!< Reverse camera*/
	Camera_Index_Left = 0x8,  /*!< Left camera*/
	Camera_Index_All = 0xf    /*!< Four cameras*/
} Camera_Index_e;

typedef struct _SV_ConfigSet_t {
	float minScale;
	float maxScale;
	int16 minOffsetX;
	int16 maxOffsetX;
	int16 minOffsetY;
	int16 maxOffsetY;
} SV_ConfigSet_t, *pSV_ConfigSet_t;

typedef struct _BIRDVIEW_CONFIG_ {
	int isNtsc;
	int displayMode;
	int cameraIdx;
	int svbvShow;
	BV_Config_t config;
	BV_ConfigExt_t configExt;
} BIRDVIEW_CONFIG;

typedef struct _AW360_CONFIG_ {
	uint32 size;
	uint32 crc;
	int workMode;
	int brightness;
	int contrast;
	int saturation;
	int hue;
	int recorderMode;
	int activationStatus;
	char dataPath[256];
	BIRDVIEW_CONFIG birdviewConfig;
} AW360_CONFIG;

/*! \brief Create function, used to create birdview instance.
	*
	* \note None.
	*
	* \param pInst created birdview instance pointer
	*
	* \retval BV_ERR_NONE, create successfully.
	* \retval BV_ERR_NO_ENOUGH_MEM, no enough memory.
	***************************************************************************/
int32 CreateBVInst(BV_Inst_t *pInst, ProductType_t productType,
		   DataType_t dataType, G2dOpsS *g2DOpsS);

/*! \brief Destroy function, used to release birdview instance.
	*
	* \note None.
	*
	* \param pInst birdview instance
	*
	* \retval BV_ERR_NONE, destroy successfully.
	* \retval BV_ERR_NULL_OBJ, object is null.
	***************************************************************************/
int32 DestroyBVInst(BV_Inst_t inst);

/*! \brief Change birdview display state.
	*
	* \note The state will be changed in next frame.
	*
	* \param inst birdview instance
	* \param state birdview display state
	*
	* \retval BV_ERR_NONE, destroy successfully.
	* \retval BV_ERR_NULL_OBJ, object is null.
	* \retval BV_ERR_NO_CONFIG, no configuration information
	***************************************************************************/
int32 ChangeBVDisplay(BV_Inst_t inst, BV_Display_e state);

int32 ConfigBVInstEx(BV_Inst_t inst, pBV_Config_t pCfg,
		     pBV_ConfigExt_t pCfgExt);

/*! \brief Render birdview and sideview image based on input camera image.
	*
	* \note None.
	*
	* \param inst birdview instance
	* \param srcY Y buffer of camera image
	* \param srcUV1 UV buffer for semi packet or U buffer for planner
	* \param srcUV2 V buffer for planner, if not planner, it is not used
	* \param dstY Y buffer of output image
	* \param dstUV1 UV buffer for semi packet or U buffer for planner
	* \param dstUV2 V buffer for planner, if not planner, it is not used
	* \param inpW input camera image width
	* \param inpH input camera image height
	* \param inpPitch input camera image stride
	* \param outW output image width
	* \param outH output image height
	* \param outPitch output image stride
	*
	* \retval BV_ERR_NONE, destroy successfully.
	* \retval BV_ERR_NULL_OBJ, inst is null.
	***************************************************************************/
int32 RenderBVImage(BV_Inst_t inst, void *srcY, void *srcUV, void *srcUV2,
		    void *dstY, void *dstUV, void *dstUV2, int32 inpW,
		    int32 inpH, int32 inpPitch);

int32 RenderBVImageMulti(BV_Inst_t inst, void *srcY1, void *srcUV1,
			 void *srcUV21, void *srcY2, void *srcUV2,
			 void *srcUV22, void *srcY3, void *srcUV3,
			 void *srcUV23, void *srcY4, void *srcUV4,
			 void *srcUV24, void *dstY, void *dstUV, void *dstUV2,
			 int32 inpW, int32 inpH, int32 inpPitch);

void SetBirdVidwConfigDefaults(pBV_Config_t pCfg, pBV_ConfigExt_t pCfgExt,
			       ProductType_t productType);
int shareDataStruct(BIRDVIEW_CONFIG *configData);
int bvLoadDefault(ProductType_t productType);
pRect_t getOverlayPos(int command, int value, const char *params);
pRect_t getBirdViewOverlayPos(int command, int value, const char *params);

#ifdef __cplusplus
}
#endif

#endif
