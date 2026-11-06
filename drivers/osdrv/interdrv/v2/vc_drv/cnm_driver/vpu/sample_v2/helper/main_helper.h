//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#ifndef _MAIN_HELPER_H_
#define _MAIN_HELPER_H_

#include "config.h"
#include "vpuapifunc.h"
#include "vputypes.h"

#include "./misc/header_struct.h"
#ifdef PLATFORM_QNX
    #include <sys/stat.h>
#endif
#ifdef _MSC_VER
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#define ABS(x) ((x < 0)? (-x) : (x))

#define MATCH_OR_MISMATCH(_expected, _value, _ret)        ((_ret=(_expected == _value)) ? "MATCH" : "MISMATCH")



//When it parse the file contents, this macro confirm if the routine run to check invalid parameter in wave6 cfg parameters
//#define CHECK_2_PARSE_INVALID_PARAM

#if defined(WIN32) || defined(WIN64)
/*
 ( _MSC_VER => 1200 )     6.0     vs6
 ( _MSC_VER => 1310 )     7.1     vs2003
 ( _MSC_VER => 1400 )     8.0     vs2005
 ( _MSC_VER => 1500 )     9.0     vs2008
 ( _MSC_VER => 1600 )    10.0     vs2010
 */
#if (_MSC_VER == 1200)
#define strcasecmp          stricmp
#define strncasecmp         strnicmp
#else
#define strcasecmp          _stricmp
#define strncasecmp         _strnicmp
#endif
#define inline              _inline
#if (_MSC_VER == 1600)
#define strdup              _strdup
#endif
#endif

#define MAX_GETOPT_OPTIONS 150
//extension of option struct in getopt
struct OptionExt
{
    const char *name;
    int has_arg;
    int *flag;
    int val;
    char *help;
};

#define MAX_FILE_PATH                           (256)
#define MAX_PIC_SKIP_NUM                        (5)
#define MAX_GOP_FRAME_NUM                       (16)
#define MAX_CHANGE_PARAM_NUM                    (16)


#define MAX_FRAME_NUM                   MAX_REG_FRAME
#define EXTRA_SRC_BUFFER_NUM            0
#define VPU_WAIT_TIME_OUT               10  //should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function
#define VPU_WAIT_TIME_OUT_CQ            1
#define VPU_WAIT_TIME_OUT_LONG          5000
#define MAX_NOT_DEC_COUNT               2000
#define COMPARE_RESOLUTION(_src, _dst)  (_src->width == _dst->width && _src->height == _dst->height)

typedef union {
    struct {
        Uint32  ctu_force_mode  :  2; //[ 1: 0]
        Uint32  ctu_coeff_drop  :  1; //[    2]
        Uint32  reserved        :  5; //[ 7: 3]
        Uint32  sub_ctu_qp_0    :  6; //[13: 8]
        Uint32  sub_ctu_qp_1    :  6; //[19:14]
        Uint32  sub_ctu_qp_2    :  6; //[25:20]
        Uint32  sub_ctu_qp_3    :  6; //[31:26]

        Uint32  lambda_sad_0    :  8; //[39:32]
        Uint32  lambda_sad_1    :  8; //[47:40]
        Uint32  lambda_sad_2    :  8; //[55:48]
        Uint32  lambda_sad_3    :  8; //[63:56]
    } field;
} EncCustomMap; // for wave5xx custom map (1 CTU = 64bits)

typedef union {
    struct {
        Uint8  mb_force_mode  :  2; //lint !e46 [ 1: 0]
        Uint8  mb_qp          :  6; //lint !e46 [ 7: 2]
    } field;
} AvcEncCustomMap; // for AVC custom map on wave  (1 MB = 8bits)

typedef enum {
    MODE_YUV_LOAD = 0,
    MODE_COMP_JYUV,
    MODE_SAVE_JYUV,

    MODE_COMP_CONV_YUV,
    MODE_SAVE_CONV_YUV,

    MODE_SAVE_LOAD_YUV,

    MODE_COMP_RECON,
    MODE_SAVE_RECON,

    MODE_COMP_ENCODED,
    MODE_SAVE_ENCODED
} CompSaveMode;

typedef enum {
    CFGP_IDX_InputFile = 0,
    CFGP_IDX_SourceWidth,
    CFGP_IDX_SourceHeight,
    CFGP_IDX_InputBitDepth,
    CFGP_IDX_FrameRate,
    CFGP_IDX_FramesToBeEncoded,
    CFGP_IDX_BitstreamFile,
    CFGP_IDX_ReconFile,
    CFGP_IDX_InternalBitDepth,
    CFGP_IDX_DecodingRefreshType,
    CFGP_IDX_IdrPeriod,
    CFGP_IDX_IntraPeriod,
    CFGP_IDX_GopPreset,
    CFGP_IDX_GOPSize,
    CFGP_IDX_FrameN,
    CFGP_IDX_UseAsLongTermRefPeriod,
    CFGP_IDX_RefLongTermPeriod,
    CFGP_IDX_RateControl,
    CFGP_IDX_EncBitrate,
    CFGP_IDX_VbvBufferSize,
    CFGP_IDX_CULevelRateControl,
    CFGP_IDX_EnHvsQp,
    CFGP_IDX_HvsQpScaleDiv2,
    CFGP_IDX_HvsMaxDeltaQp,
    CFGP_IDX_RcInitialQp,
    CFGP_IDX_RcUpdateSpeed,
    CFGP_IDX_PicRcMaxDqp,
    CFGP_IDX_MinQp_AV1,
    CFGP_IDX_MinQp_AVC_HEVC,
    CFGP_IDX_MaxQp_AV1,
    CFGP_IDX_MaxQp_AVC_HEVC,
    CFGP_IDX_MaxBitrate,
    CFGP_IDX_RcInitLevel,
    CFGP_IDX_EnBgDetect,
    CFGP_IDX_BgThDiff,
    CFGP_IDX_BgThMeanDiff,
    CFGP_IDX_BgDeltaQp,
    CFGP_IDX_StrongIntraSmoothing,
    CFGP_IDX_ConstrainedIntraPred,
    CFGP_IDX_IntraNxN_HEVC,
    CFGP_IDX_IntraNxN_AV1,
    CFGP_IDX_IntraTransSkip,
    CFGP_IDX_IntraRefreshMode,
    CFGP_IDX_IntraRefreshArg,
    CFGP_IDX_EnTemporalMVP,
    CFGP_IDX_MeCenter,
    CFGP_IDX_CABAC,
    CFGP_IDX_Transform8x8,
    CFGP_IDX_LFCrossSliceBoundaryFlag,
    CFGP_IDX_EnDBK,
    CFGP_IDX_BetaOffsetDiv2,
    CFGP_IDX_TcOffsetDiv2,
    CFGP_IDX_LfSharpness,
    CFGP_IDX_EnSAO,
    CFGP_IDX_EnCdef,
    CFGP_IDX_EnWiener,
    CFGP_IDX_QP,
    CFGP_IDX_CbQpOffset,
    CFGP_IDX_CrQpOffset,
    CFGP_IDX_YDcQpDelta,
    CFGP_IDX_UDcQpDelta,
    CFGP_IDX_VDcQpDelta,
    CFGP_IDX_UAcQpDelta,
    CFGP_IDX_VAcQpDelta,
    CFGP_IDX_ScalingList,
    CFGP_IDX_AdaptiveRound,
    CFGP_IDX_QRoundIntra,
    CFGP_IDX_QRoundInter,
    CFGP_IDX_DisableCoefClear,
    CFGP_IDX_LambdaDqpIntra,
    CFGP_IDX_LambdaDqpInter,
    CFGP_IDX_SliceMode,
    CFGP_IDX_SliceArg,
    CFGP_IDX_NumColTiles,
    CFGP_IDX_NumRowTiles,
    CFGP_IDX_EnQpMap,
    CFGP_IDX_EnModeMap,
    CFGP_IDX_CustomMapFile,
    CFGP_IDX_EnCustomLambda,
    CFGP_IDX_CustomLambdaFile,
    CFGP_IDX_ForcePicSkipStart,
    CFGP_IDX_ForcePicSkipEnd,
    CFGP_IDX_ForceIdrPicIdx,
    //CFGP_IDX_RdoBiasBlk4,
    //CFGP_IDX_RdoBiasBlk8,
    //CFGP_IDX_RdoBiasBlk16,
    //CFGP_IDX_RdoBiasBlk32,
    //CFGP_IDX_RdoBiasIntra,
    //CFGP_IDX_RdoBiasInter,
    //CFGP_IDX_RdoBiasMerge,
    CFGP_IDX_NumUnitsInTick,
    CFGP_IDX_TimeScale,
    CFGP_IDX_ConfWindSizeTop,
    CFGP_IDX_ConfWindSizeBot,
    CFGP_IDX_ConfWindSizeRight,
    CFGP_IDX_ConfWindSizeLeft,
    CFGP_IDX_NoLastFrameCheck,
    CFGP_IDX_EnPrefixSeiData,
    CFGP_IDX_PrefixSeiDataFile,
    CFGP_IDX_PrefixSeiSize,
    CFGP_IDX_EnSuffixSeiData,
    CFGP_IDX_SuffixSeiDataFile,
    CFGP_IDX_SuffixSeiSize,
    CFGP_IDX_EncodeRbspHrd,
    CFGP_IDX_RbspHrdFile,
    CFGP_IDX_RbspHrdSize,
    CFGP_IDX_EncodeRbspVui,
    CFGP_IDX_RbspVuiFile,
    CFGP_IDX_RbspVuiSize,
    CFGP_IDX_EncAUD,
    CFGP_IDX_EncEOS,
    CFGP_IDX_EncEOB,
    CFGP_IDX_NumTicksPocDiffOne,
    CFGP_IDX_MinQpI_AV1,
    CFGP_IDX_MinQpI_AVC_HEVC,
    CFGP_IDX_MaxQpI_AV1,
    CFGP_IDX_MaxQpI_AVC_HEVC,
    CFGP_IDX_MinQpP_AV1,
    CFGP_IDX_MinQpP_AVC_HEVC,
    CFGP_IDX_MaxQpP_AV1,
    CFGP_IDX_MaxQpP_AVC_HEVC,
    CFGP_IDX_MinQpB_AV1,
    CFGP_IDX_MinQpB_AVC_HEVC,
    CFGP_IDX_MaxQpB_AV1,
    CFGP_IDX_MaxQpB_AVC_HEVC,
    CFGP_IDX_Tid0_QpI_AV1,
    CFGP_IDX_Tid0_QpI_AVC_HEVC,
    CFGP_IDX_Tid0_QpP_AV1,
    CFGP_IDX_Tid0_QpP_AVC_HEVC,
    CFGP_IDX_Tid0_QpB_AV1,
    CFGP_IDX_Tid0_QpB_AVC_HEVC,
    CFGP_IDX_Tid1_QpI_AV1,
    CFGP_IDX_Tid1_QpI_AVC_HEVC,
    CFGP_IDX_Tid1_QpP_AV1,
    CFGP_IDX_Tid1_QpP_AVC_HEVC,
    CFGP_IDX_Tid1_QpB_AV1,
    CFGP_IDX_Tid1_QpB_AVC_HEVC,
    CFGP_IDX_Tid2_QpI_AV1,
    CFGP_IDX_Tid2_QpI_AVC_HEVC,
    CFGP_IDX_Tid2_QpP_AV1,
    CFGP_IDX_Tid2_QpP_AVC_HEVC,
    CFGP_IDX_Tid2_QpB_AV1,
    CFGP_IDX_Tid2_QpB_AVC_HEVC,
    CFGP_IDX_Tid3_QpI_AV1,
    CFGP_IDX_Tid3_QpI_AVC_HEVC,
    CFGP_IDX_Tid3_QpP_AV1,
    CFGP_IDX_Tid3_QpP_AVC_HEVC,
    CFGP_IDX_Tid3_QpB_AV1,
    CFGP_IDX_Tid3_QpB_AVC_HEVC,
    CFGP_IDX_ForcePicQpPicIdx,
    CFGP_IDX_ForcePicQpValue_AV1,
    CFGP_IDX_ForcePicQpValue_AVC_HEVC,
    CFGP_IDX_ForcePicTypePicIdx,
    CFGP_IDX_ForcePicTypeValue,
    CFGP_IDX_TemporalLayerCount,
    CFGP_IDX_ApiScenario,
    CFGP_IDX_SPChN,
    CFGP_IDX_PPChN,
    CFGP_IDX_MAX,
}CFGP_IDX;

typedef struct {
    int picX;
    int picY;
    int internalBitDepth;
    int losslessEnable;
    int constIntraPredFlag;
    int gopSize;
    int decodingRefreshType;
    int intraQP;
    int intraPeriod;
    int frameRate;

    int confWinTop;
    int confWinBot;
    int confWinLeft;
    int confWinRight;

    int independSliceMode;
    int independSliceModeArg;
    int dependSliceMode;
    int dependSliceModeArg;
    int intraRefreshMode;
    int intraRefreshArg;

    int useRecommendEncParam;
    int scalingListEnable;
    int tmvpEnable;
    int wppenable;
    int maxNumMerge;

    int disableDeblk;
    int lfCrossSliceBoundaryEnable;
    int betaOffsetDiv2;
    int tcOffsetDiv2;
    int skipIntraTrans;
    int saoEnable;
    int intraNxNEnable;
    int rcEnable;

    int bitRate;
    int bitAllocMode;
    int fixedBitRatio[MAX_GOP_NUM];
    int cuLevelRCEnable;
    int hvsQPEnable;

    int hvsQpScale;
    int minQp;
    int maxQp;
    int maxDeltaQp;

    int gopPresetIdx;
    // CUSTOM_GOP
    CustomGopParam gopParam;

    // ROI / CTU mode
    int roiEnable;                      /**< It enables ROI map. NOTE: It is valid when rcEnable is on. */
    char roiFileName[MAX_FILE_PATH];
    char roiQpMapFile[MAX_FILE_PATH];

    // VUI
    Uint32 numUnitsInTick;
    Uint32 timeScale;
    Uint32 numTicksPocDiffOne;

    int encAUD;
    int encEOS;
    int encEOB;

    int chromaCbQpOffset;
    int chromaCrQpOffset;

    Uint32 initialRcQp;

    Uint32  nrYEnable;
    Uint32  nrCbEnable;
    Uint32  nrCrEnable;
    Uint32  nrNoiseEstEnable;
    Uint32  nrNoiseSigmaY;
    Uint32  nrNoiseSigmaCb;
    Uint32  nrNoiseSigmaCr;

    Uint32  nrIntraWeightY;
    Uint32  nrIntraWeightCb;
    Uint32  nrIntraWeightCr;

    Uint32  nrInterWeightY;
    Uint32  nrInterWeightCb;
    Uint32  nrInterWeightCr;

    Uint32 useAsLongtermPeriod;
    Uint32 refLongtermPeriod;

    // newly added for encoder
    Uint32 monochromeEnable;
    Uint32 strongIntraSmoothEnable;
    Uint32 roiAvgQp;
    Uint32 weightPredEnable;
    Uint32 bgDetectEnable;
    Uint32 bgThrDiff;
    Uint32 bgThrMeanDiff;
    Uint32 bgLambdaQp;
    int    bgDeltaQp;
    Uint32 lambdaMapEnable;
    Uint32 customLambdaEnable;
    Uint32 customMDEnable;
    int    pu04DeltaRate;
    int    pu08DeltaRate;
    int    pu16DeltaRate;
    int    pu32DeltaRate;
    int    pu04IntraPlanarDeltaRate;
    int    pu04IntraDcDeltaRate;
    int    pu04IntraAngleDeltaRate;
    int    pu08IntraPlanarDeltaRate;
    int    pu08IntraDcDeltaRate;
    int    pu08IntraAngleDeltaRate;
    int    pu16IntraPlanarDeltaRate;
    int    pu16IntraDcDeltaRate;
    int    pu16IntraAngleDeltaRate;
    int    pu32IntraPlanarDeltaRate;
    int    pu32IntraDcDeltaRate;
    int    pu32IntraAngleDeltaRate;
    int    cu08IntraDeltaRate;
    int    cu08InterDeltaRate;
    int    cu08MergeDeltaRate;
    int    cu16IntraDeltaRate;
    int    cu16InterDeltaRate;
    int    cu16MergeDeltaRate;
    int    cu32IntraDeltaRate;
    int    cu32InterDeltaRate;
    int    cu32MergeDeltaRate;
    int    coefClearDisable;
    int    forcePicSkipStart;
    int    forcePicSkipEnd;
    int    forceCoefDropStart;
    int    forceCoefDropEnd;
    char   scalingListFileName[MAX_FILE_PATH];
    char   customLambdaFileName[MAX_FILE_PATH];

    Uint32 enStillPicture;

    // custom map
    int    customLambdaMapEnable;
    char   customLambdaMapFileName[MAX_FILE_PATH];
    int    customModeMapFlag;
    char   customModeMapFileName[MAX_FILE_PATH];

    char   WpParamFileName[MAX_FILE_PATH];

    // for H.264 on WAVE
    int idrPeriod;
    int rdoSkip;
    int lambdaScalingEnable;
    int transform8x8;
    int avcSliceMode;
    int avcSliceArg;
    int intraMbRefreshMode;
    int intraMbRefreshArg;
    int mbLevelRc;
    int entropyCodingMode;

    int s2fmeDisable;
    int forceIdrPicIdx;
    int forcedIdrHeaderEnable;
    Uint32 vuiDataEnable;
    Uint32 vuiDataSize;
    char   vuiDataFileName[MAX_FILE_PATH];
    Uint32 hrdInVPS;
    Uint32 hrdDataSize;
    char   hrdDataFileName[MAX_FILE_PATH];
    Uint32 prefixSeiEnable;
    Uint32 prefixSeiDataSize;
    char   prefixSeiDataFileName[MAX_FILE_PATH];
    Uint32 suffixSeiEnable;
    Uint32 suffixSeiDataSize;
    char   suffixSeiDataFileName[MAX_FILE_PATH];
    int s2SearchRangeXDiv4;
    int s2SearchRangeYDiv4;
    int s2SearchCenterEnable;
    int s2SearchCenterXDiv4;
    int s2SearchCenterYDiv4;
    Uint32 rcWeightParam;
    Uint32 rcWeightBuf;
} WAVE_ENC_CFG;

typedef enum {
    GOP_PRESET_CUSTOM = 0,
    GOP_PRESET_I_1 = 1,
    GOP_PRESET_P_1 = 2,
    GOP_PRESET_B_1 = 3,
    GOP_PRESET_BP_2 = 4,
    GOP_PRESET_BBBP_3 = 5,
    GOP_PRESET_LP_4 = 6,
    GOP_PRESET_LD_4 = 7,
    GOP_PRESET_RA_8 = 8,
    // single_ref
    GOP_PRESET_SP_1 = 9,
    GOP_PRESET_BSP_2 = 10,
    GOP_PRESET_BBBSP_3 = 11,
    GOP_PRESET_LSP_4 = 12,
    // newly added
    AVC_ENC_NUM_GOP_PRESET_NUM = 13,
    GOP_PRESET_BBP_3 = 13,
    GOP_PRESET_BBSP_3 = 14,
    GOP_PRESET_BBBBBBBP_8 = 15,
    GOP_PRESET_BBBBBBBSP_8 = 16,
    NUM_GOP_PRESET_NUM = 17,
}W6_GOPPRESET;

typedef enum{
    CGOP_IDX_SLICE_TYPE=0,
    CGOP_IDX_CURRNET_POC,
    CGOP_IDX_DELTA_QP,
    CGOP_IDX_USE_MULTI_REF_P,
    CGOP_IDX_TEMPORAL_ID,
    CGOP_IDX_REF_PIC_POC_0,
    CGOP_IDX_REF_PIC_POC_1,
    CGOP_IDX_MAX,
}CGOP_IDX;

typedef enum {
    CHP_IDX_CHNAGE = 0,
    CHP_IDX_ENABLE,
    CHP_IDX_CFGDIR,
    CHP_IDX_MAX
}CHANGE_PARAM_IDX;

typedef struct {
    // ChangePara
    int setParaChgFrmNum;
    int enableOption;
    char cfgName[MAX_FILE_PATH];
} WaveChangeParam;

typedef struct {
    int chgParamNum;
    WaveChangeParam chgParam[MAX_CHANGE_PARAM_NUM];
}ChangeParamInfo;

typedef struct {
    // ChangePara
    int setParaChgFrmNum;
    int enableOption;
    char cfgName[MAX_FILE_PATH];
} WaveChangePicParam;

typedef struct {
    int chgPicParamNum;
    WaveChangePicParam chgPicParam[MAX_CHANGE_PARAM_NUM];
}ChangePicParamInfo;

typedef struct{
    char InputFile[MAX_FILE_PATH];
    int SourceWidth;
    int SourceHeight;
    int InputBitDepth;
    int FrameRate;
    int FramesToBeEncoded;
    char BitstreamFile[MAX_FILE_PATH];
    char ReconFile[MAX_FILE_PATH];
    int InternalBitDepth;
    int DecodingRefreshType;
    int IdrPeriod;
    int IntraPeriod;
    int GopPreset;
    int GOPSize;
    int UseAsLongTermRefPeriod;
    int RefLongTermPeriod;
    int RateControl;
    int EncBitrate;
    int VbvBufferSize;
    int CULevelRateControl;
    int EnHvsQp;
    int HvsQpScaleDiv2;
    int HvsMaxDeltaQp;
    int RcInitialQp;
    int RcUpdateSpeed;
    int PicRcMaxDqp;
    int MinQp;
    int MaxQp;
    int MaxBitrate;
    int RcInitLevel;
    int EnBgDetect;
    int BgThDiff;
    int BgThMeanDiff;
    int BgDeltaQp;
    int StrongIntraSmoothing;
    int ConstrainedIntraPred;
    int IntraNxN;
    int IntraTransSkip;
    int IntraRefreshMode;
    int IntraRefreshArg;
    int EnTemporalMVP;
    int MeCenter;
    int CABAC;
    int Transform8x8;
    int LFCrossSliceBoundaryFlag;
    int EnDBK;
    int BetaOffsetDiv2;
    int TcOffsetDiv2;
    int LfSharpness;
    int EnSAO;
    int EnCdef;
    int EnWiener;
    int QP;
    int ForceIdrPicIdx;
    int CbQpOffset;
    int CrQpOffset;
    int YDcQpDelta;
    int UDcQpDelta;
    int VDcQpDelta;
    int UAcQpDelta;
    int VAcQpDelta;
    int ScalingList;
    int AdaptiveRound;

    int EnCustomQRound;
    int QRoundIntra;
    int QRoundInter;

    int DisableCoefClear;
    int LambdaDqpIntra;
    int LambdaDqpInter;
    int SliceMode;
    int SliceArg;
    int EnQpMap;
    int EnModeMap;
    char CustomMapFile[MAX_FILE_PATH];
    int EnCustomLambda;
    char CustomLambdaFile[MAX_FILE_PATH];
    int ForcePicSkipStart;
    int ForcePicSkipEnd;

    int TimeScale;
    int NumUnitsInTick;
    int NumColTiles;
    int NumRowTiles;
    int ConfWindSizeTop;
    int ConfWindSizeBot;
    int ConfWindSizeRight;
    int ConfWindSizeLeft;
    BOOL NoLastFrameCheck;
//SEI
    int EnPrefixSeiData;
    char PrefixSeiDataFile[MAX_FILE_PATH];
    int PrefixSeiSize;
    int EnSuffixSeiData;
    char SuffixSeiDataFile[MAX_FILE_PATH];
    int SuffixSeiSize;
//HRD_RBSP
    int EncodeRbspHrd;
    char RbspHrdFile[MAX_FILE_PATH];
    int RbspHrdSize;
//VUI_RBSP
    int EncodeRbspVui;
    char RbspVuiFile[MAX_FILE_PATH];
    int RbspVuiSize;
//AUD_EOS_EOB
    int EncAUD;
    int EncEOS;
    int EncEOB;
    int NumTicksPocDiffOne;
    int minQpI;
    int maxQpI;
    int minQpP;
    int maxQpP;
    int minQpB;
    int maxQpB;
    int temporal_layer_count;
    TemporalLayerParam temporal_layer_qp[MAX_NUM_CHANGEABLE_TEMPORAL_LAYER];
    int ForcePicQpPicIdx;
    int ForcePicQpValue;
    int ForcePicTypePicIdx;
    int ForcePicTypeValue;
    int ApiScenario;
    CustomGopParam gopParam;
    ChangeParamInfo chgParamInfo;
    ChangePicParamInfo chgPicParamInfo;
}WAVE6_ENC_CFG;

typedef struct {
    char SrcFileName[MAX_FILE_PATH];
    char BitStreamFileName[MAX_FILE_PATH];
    BOOL srcCbCrInterleave;
    int NumFrame;
    int PicX;
    int PicY;
    int FrameRate;

    // MPEG4 ONLY
    int VerId;
    int DataPartEn;
    int RevVlcEn;
    int ShortVideoHeader;
    int AnnexI;
    int AnnexJ;
    int AnnexK;
    int AnnexT;
    int IntraDcVlcThr;
    int VopQuant;

    // H.264 ONLY
    int ConstIntraPredFlag;
    int DisableDeblk;
    int DeblkOffsetA;
    int DeblkOffsetB;
    int ChromaQpOffset;
    int PicQpY;
    int audEnable;
    int level;
    // COMMON
    int GopPicNum;
    int SliceMode;
    int SliceSizeMode;
    int SliceSizeNum;
    // COMMON - RC
    int RcEnable;
    int RcBitRate;
    int RcInitDelay;
    int VbvBufferSize;
    int RcBufSize;
    int IntraRefreshNum;
    int ConscIntraRefreshEnable;
    int frameSkipDisable;
    int ConstantIntraQPEnable;
    int MaxQpSetEnable;
    int MaxQp;

    int frameCroppingFlag;
    int frameCropLeft;
    int frameCropRight;
    int frameCropTop;
    int frameCropBottom;

    //H.264 only
    int MaxDeltaQpSetEnable;
    int MaxDeltaQp;
    int MinQpSetEnable;
    int MinQp;
    int MinDeltaQpSetEnable;
    int MinDeltaQp;
    int intraCostWeight;

    //MP4 Only
    int RCIntraQP;
    int HecEnable;

    int GammaSetEnable;
    int Gamma;

    // NEW RC Scheme
    int rcIntervalMode;
    int RcMBInterval;
    int skipPicNums[MAX_PIC_SKIP_NUM];
    int SearchRange;    // for coda960

    int MeUseZeroPmv;    // will be removed. must be 264 = 0, mpeg4 = 1 263 = 0
    int MeBlkModeEnable; // only api option
    int IDRInterval;
    int SrcBitDepth;
    int Profile;

    WAVE_ENC_CFG waveCfg;

    int numChangeParam;
    WaveChangeParam changeParam[MAX_CHANGE_PARAM_NUM];
    ParamChange coda9ParamChange;
    int rcWeightFactor;
} ENC_CFG;

#define SFS_UPDATE_NEW_FRAME 1
#define SFS_UPDATE_END_OF_ROW 1
typedef struct {
    Uint32 subFrameSyncOn;
    Uint32 subFrameSyncSrcWriteMode;                /**< It indicates the number of pixel rows to run VPU_EncStartOneFrame() with. (default/min: 64, max: picture height) This is used for test purpose.  */
    EncSubFrameSyncState subFrameSyncState;         /**< representing the status of Subframe Sync(SFS) when SFS works in register-based mode.*/
    Uint32 subFrameSyncMode;
    Uint32 subFrameSyncFbCount;
} ENC_subFrameSyncCfg; /* SFS = SubFrameSync */


extern void replace_character(char* str,
    char  c,
    char  r);

extern Uint32 randomSeed;

/* yuv & md5 */
#define NO_COMPARE              0
#define YUV_COMPARE             1
#define MD5_COMPARE             2
#define STREAM_COMPARE          3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Performance report */
typedef void*   PFCtx;

extern PFCtx PFMonitorSetup(
    Uint32  coreIndex,
    Uint32  instanceIndex,
    Uint32  referenceClkInMHz,
    Uint32  fps,
    char*   strLogDir,
    BOOL    isEnc
    );

extern void PFMonitorRelease(
    PFCtx   context
    );

extern void PFMonitorUpdate(
    Uint32  coreIndex,
    PFCtx   context,
    Uint32  cycles,
    ...
    );

extern void PrepareDecoderTest(
    DecHandle decHandle
    );

extern void byte_swap(
    unsigned char* data,
    int len
    );

extern void word_swap(
    unsigned char* data,
    int len
    );

extern void dword_swap(
    unsigned char* data,
    int len
    );

extern void lword_swap(
    unsigned char* data,
    int len
    );

extern Int32 LoadFirmware(
    Int32       productId,
    Uint8**   retFirmware,
    Uint32*   retSizeInWord,
    const char* path
    );

extern void PrintDecSeqWarningMessages(
    Uint32          productId,
    DecInitialInfo* seqInfo
    );

extern void DisplayDecodedInformation(
    DecHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    DecOutputInfo* decodedInfo,
    ...
    );

extern void DisplayEncodedInformation(
    EncHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    EncOutputInfo* encodedInfo,
    ...
    );

extern void PrintEncSppStatus(
    Uint32 coreIdx,
    Uint32 productId
    );
/*
 * VPU Helper functions
 */
/************************************************************************/
/* Video                                                                */
/************************************************************************/

#define PUT_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b;

#define PUT_BUFFER(_p, _buf, _len) \
    osal_memcpy(_p, _buf, _len); \
    (_p) += (_len);

#define PUT_LE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>16); \
    *_p++ = (unsigned char)((_var)>>24);

#define PUT_BE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>24);  \
    *_p++ = (unsigned char)((_var)>>16);  \
    *_p++ = (unsigned char)((_var)>>8); \
    *_p++ = (unsigned char)((_var)>>0);

#define PUT_LE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);

#define PUT_BE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>0);

extern Int32 ConvFOURCCToMp4Class(
    Int32   fourcc
    );

extern Int32 ConvFOURCCToCodStd(
    Uint32 fourcc
    );

extern Int32 ConvCodecIdToMp4Class(
    Uint32 codecId
    );

extern Int32 ConvCodecIdToCodStd(
    Int32   codecId
    );

extern Int32 ConvCodecIdToFourcc(
    Int32   codecId
    );

/*!
 * \brief       wrapper function of StoreYuvImageBurstFormat()
 */
extern Uint8* GetYUVFromFrameBuffer(
    DecHandle       decHandle,
    FrameBuffer*    fb,
    VpuRect         rcFrame,
    Uint32*         retWidth,
    Uint32*         retHeight,
    Uint32*         retBpp,
    size_t*         retSize,
    BOOL            cropped_flag
    );

/************************************************************************/
/* linked list                                                          */
/************************************************************************/
typedef struct list_node {
    void *data;
    struct list_node *next;
} list_node;

/* linked list */
list_node* list_create(void *data);
void list_destroy(list_node **list);
list_node* list_insert_after(list_node *node, void *data);
BOOL list_remove(list_node **list, list_node *node);
list_node* list_find_node(list_node *list, list_node *node);

/************************************************************************/
/* ETC                                                                  */
/************************************************************************/
extern Uint32 GetRandom(
    Uint32 start,
    Uint32 end
    );

/************************************************************************/
/* MD5                                                                  */
/************************************************************************/

typedef struct MD5state_st {
    Uint32 A,B,C,D;
    Uint32 Nl,Nh;
    Uint32 data[16];
    Uint32 num;
} MD5_CTX;

extern Int32 MD5_Init(
    MD5_CTX *c
    );

extern Int32 MD5_Update(
    MD5_CTX*    c,
    const void* data,
    size_t      len);

extern Int32 MD5_Final(
    Uint8*      md,
    MD5_CTX*    c
    );

extern Uint8* MD5(
    const Uint8*  d,
    size_t        n,
    Uint8*        md
    );

extern void plane_md5(MD5_CTX *md5_ctx,
    Uint8  *src,
    int    src_x,
    int    src_y,
    int    out_x,
    int    out_y,
    int    stride,
    int    bpp,
    Uint16 zero
    );

extern void SaveMD5ToFile(
    Uint32* md5,
    Uint32 md5Size,
    const char* outPath
    );


/************************************************************************/
/* Bitstream Feeder                                                     */
/************************************************************************/
typedef enum {
    FEEDING_METHOD_FIXED_SIZE,
    FEEDING_METHOD_FRAME_SIZE,
    FEEDING_METHOD_SIZE_PLUS_ES,
    FEEDING_METHOD_HOST_FRAME_SIZE,
    FEEDING_METHOD_MAX
} FeedingMethod;

typedef struct {
    void*       data;
    Uint32    size;
    BOOL        eos;        //!<< End of stream
    int seqHeaderSize;
    BOOL isHeader;
} BSChunk;

typedef void* BSFeeder;

typedef void (*BSFeederHook)(BSFeeder feeder, void* data, Uint32 size, void* arg);

/**
 * \brief           BitstreamFeeder consumes bitstream and updates information of bitstream buffer of VPU.
 * \param handle    handle of decoder
 * \param path      bitstream path
 * \param method    feeding method. see FeedingMethod.
 * \param loopCount If @loopCount is greater than 1 then BistreamFeeder reads the start of bitstream again
 *                  when it encounters the end of stream @loopCount times.
 * \param ...       FEEDING_METHOD_FIXED_SIZE:
 *                      This value of parameter is size of chunk at a time.
 *                      If the size of chunk is equal to zero than the BitstreamFeeder reads bistream in random size.(1Byte ~ 4MB)
 * \return          It returns the pointer of handle containing the context of the BitstreamFeeder.
 */
extern void* BitstreamFeeder_Create(
    Uint32          coreIdx,
    const char*     path,
    CodStd          codecId,
    Uint32          format,
    FeedingMethod   method,
    EndianMode      endian
    );

/**
 * \brief           This is helper function set to simplify the flow that update bit-stream
 *                  to the VPU.
 */
extern Uint32 BitstreamFeeder_Act(
    BSFeeder        feeder,
    vpu_buffer_t*   bsBuffer,
    PhysicalAddress wrPtr,
    Uint32          room,
    PhysicalAddress* newWrPtr
    );

extern BOOL BitstreamFeeder_SetFeedingSize(
    BSFeeder    feeder,
    Uint32      size
    );
/**
 * \brief           Set filling bitstream as ringbuffer mode or linebuffer mode.
 * \param   mode    0 : auto
 *                  1 : ringbuffer
 *                  2 : linebuffer.
 */
#define BSF_FILLING_AUTO                    0
#define BSF_FILLING_RINGBUFFER              1
#define BSF_FILLING_LINEBUFFER              2
/* BSF_FILLING_RINBGUFFER_WITH_ENDFLAG:
 * Scenario:
 * - Application writes 1 ~ 10 frames into bitstream buffer.
 * - Set stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, 0).
 * - Application clears stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, -1).
 *   when indexFrameDisplay is equal to -1.
 * NOTE:
 * - Last frame cannot be a complete frame.
 */
#define BSF_FILLING_RINGBUFFER_WITH_ENDFLAG 3
extern void BitstreamFeeder_SetFillMode(
    BSFeeder    feeder,
    Uint32      mode
    );

extern BOOL BitstreamFeeder_IsEos(
    BSFeeder    feeder
    );


extern Uint32 BitstreamFeeder_GetSeqHeaderSize(
    BSFeeder    feeder
    );


extern BOOL BitstreamFeeder_Destroy(
    BSFeeder    feeder
    );

extern BOOL BitstreamFeeder_Rewind(
    BSFeeder feeder
    );

extern BOOL BitstreamFeeder_SetHook(
    BSFeeder        feeder,
    BSFeederHook    hookFunc,
    void*           arg
    );

/************************************************************************/
/* CNM video helper                                                    */
/************************************************************************/
/**
 *  \param  convertCbcrIntl     If this value is TRUE, it stores YUV as NV12 or NV21 to @fb
 */
extern BOOL LoadYuvImageByYCbCrLine(
    EncHandle   handle,
    Uint32      coreIdx,
    Uint8*      src,
    size_t      picWidth,
    size_t      picHeight,
    FrameBuffer* fb,
    void        *arg,
    ENC_subFrameSyncCfg *subFrameSyncConfig,
    Uint32      srcFbIndex
    );

typedef enum {
    SRC_0LINE_WRITE              = 0,
    SRC_64LINE_WRITE             = 64,
    SRC_128LINE_WRITE            = 128,
    SRC_192LINE_WRITE            = 192,
    //...
    REMAIN_SRC_DATA_WRITE     = 0x80000000
} SOURCE_LINE_WRITE;

extern BOOL LoadYuvImageBurstFormat(
    Uint32      coreIdx,
    Uint8*      src,
    size_t      picWidth,
    size_t      picHeight,
    FrameBuffer *fb,
    BOOL        convertCbcrIntl
    );

extern BOOL LoadTiledImageYuvBurst(
    VpuHandle       handle,
    Uint32          coreIdx,
    BYTE*           pYuv,
    size_t          picWidth,
    size_t          picHeight,
    FrameBuffer*    fb,
    TiledMapConfig  mapCfg
    );

extern void CalcRealFrameSize(
    FrameBufferFormat   format,
    Uint32              width,
    Uint32              height,
    BOOL                isCbcrInterleave,
    Uint32* lumaSize,
    Uint32* chromaSize,
    Uint32* frameSize
    );

extern Uint32 StoreYuvImageBurstFormat(
    Uint32          coreIndex,
    FrameBuffer*    fbSrc,
    TiledMapConfig  mapCfg,
    Uint8*          pDst,
    VpuRect         cropRect,
    BOOL            enableCrop
    );

/*******************************************************************************
 * DATATYPES AND FUNCTIONS RELATED TO REPORT
 *******************************************************************************/
typedef enum {
    AUX_BUF_TYPE_MVCOL,
    AUX_BUF_TYPE_FBC_Y_OFFSET,
    AUX_BUF_TYPE_FBC_C_OFFSET
} AUX_BUF_TYPE;

typedef struct VpuReportConfig_t {
    PhysicalAddress userDataBufAddr;
    BOOL            userDataEnable;
    Int32           userDataReportMode; // (0 : Int32errupt mode, 1 Int32errupt disable mode)
    Int32           userDataBufSize;

} VpuReportConfig_t;

extern void OpenDecReport(
    DecHandle           handle,
    VpuReportConfig_t*  cfg
    );

extern void CloseDecReport(
    DecHandle handle
    );

extern void ConfigDecReport(
    Uint32      core_idx,
    DecHandle   handle,
    CodStd      bitstreamFormat
    );

extern void SaveDecReport(
    Uint32          core_idx,
    DecHandle       handle,
    DecOutputInfo*  pDecInfo,
    CodStd          bitstreamFormat,
    Uint32          mbNumX,
    Uint32          mbNumY
    );

extern void CheckUserDataInterrupt(
    Uint32      core_idx,
    DecHandle   handle,
    Int32       decodeIdx,
    CodStd      bitstreamFormat,
    Int32       int_reason
    );

extern Int32 CalculateAuxBufferSize(
    AUX_BUF_TYPE    type,
    CodStd          codStd,
    Int32           width,
    Int32           height
    );

extern RetCode GetFBCOffsetTableSize(
    CodStd  codStd,
    int     width,
    int     height,
    int*    ysize,
    int*    csize
    );

#define MAX_ROI_LEVEL           (8)
#define LOG2_CTB_SIZE           (5)
#define CTB_SIZE                (1<<LOG2_CTB_SIZE)
#define LAMBDA_SCALE_FACTOR     (100000)
#define FLOATING_POINT_LAMBDA   (1)
#define TEMP_SCALABLE_RC        (1)
#define UI16_MAX                (0xFFFF)


typedef enum{
    CFG_AVC     = (1ULL << STD_AVC),
    CFG_HEVC    = (1ULL << STD_HEVC),
    CFG_AV1     = (1ULL << STD_AV1),
}CFG_CODEC;

typedef enum{
    CFG_DTYPE_STR=0,
    CFG_DTYPE_INT,
    CFG_DTYPE_COUNT,
}CFG_DTYPE;
typedef struct {
    char  *name;
    int    min;
    int    max;
    int    def;
} WaveCfgInfo;
typedef struct {
    CFG_CODEC      codec;
    CFG_DTYPE      dtype;
    char           *name;
    int            min;
    int            max;
    int            def;
    void           *lval;
    char           *except;
}CFGINFO;

extern void PrintVpuVersionInfo(
    Uint32 coreIdx
    );

extern void ChangePathStyle(
    char *str
    );

extern BOOL CalcYuvSize(
    Int32   format,
    Int32   picWidth,
    Int32   picHeight,
    Int32   cbcrInterleave,
    size_t  *lumaSize,
    size_t  *chromaSize,
    size_t  *frameSize,
    Int32   *bitDepth,
    Int32   *packedFormat,
    Int32   *yuv3p4b
    );


extern FrameBufferFormat GetPackedFormat (
    int srcBitDepth,
    int packedType,
    int p10bits,
    int msb
    );

extern char* GetDirname(
    const char* path
    );

extern char* GetBasename(
    const char* pathname
    );

extern char* GetFileExtension(
    const char* filename
    );

extern int parseAvcCfgFile(
    ENC_CFG*    pEncCfg,
    char*       filename
    );

extern int parseMp4CfgFile(
    ENC_CFG*    pEncCfg,
    char*       filename
    );

extern int parseWaveEncCfgFile(
    ENC_CFG*    pEncCfg,
    char*       FileName,
    int bitFormat
    );

extern int parseWaveChangeParamCfgFile(
    ENC_CFG*    pEncCfg,
    char*       FileName
    );
extern int w6_get_enc_cfg(
    char *FileName,
    CodStd cd_idx,
    WAVE6_ENC_CFG *p_w6_enc_cfg,
    BOOL init_flag
    );
extern int w6_get_gop_size(
    int gop_preset
    );
extern int parseRoiCtuModeParam(
    char* lineStr,
    VpuRect* roiRegion,
    int* roiLevel,
    int picX,
    int picY
    );
#ifdef __cplusplus
}
#endif /* __cplusplus */

/************************************************************************/
/* Structure                                                            */
/************************************************************************/
typedef struct {
    Int32       pvricMode;
    Int32       pvricVersion;
    Uint32      pvricIfbcTilePadY;
    Uint32      pvricIfbcTilePadUv;
} OutputPVRICInfo;

typedef struct TestDecConfig_struct {
    char                outputPath[MAX_FILE_PATH];
    char                inputPath[MAX_FILE_PATH];
    char                xmlOutputPath[MAX_FILE_PATH];
    Int32               forceOutNum;
    CodStd              bitFormat;
    Int32               disable_reorder;
    TiledMapType        mapType;
    BitStreamMode       bitstreamMode;
    BOOL                enableWTL;
    OutputPVRICInfo     outputPvric;
    FrameFlag           wtlMode;
    FrameBufferFormat   wtlFormat;
    Int32               coreIdx;
    ProductId           productId;
    BOOL                enableCrop;                 //!<< option for saving yuv
    BOOL                cbcrInterleave;             //!<< 0: None, 1: NV12, 2: NV21
    BOOL                nv21;                       //!<< FALSE: NV12, TRUE: NV21,
                                                    //!<< This variable is valid when cbcrInterleave is TRUE
    EndianMode          streamEndian;
    EndianMode          frameEndian;
    Uint32              secondaryAXI;
    Int32               compareType;
    char                md5Path[MAX_FILE_PATH];
    char                fwPath[MAX_FILE_PATH];
    char                refYuvPath[MAX_FILE_PATH];
    BOOL                thumbnailMode;
    Int32               skipMode;
    size_t              bsSize;
    BOOL                streamEndFlag;
    struct {
        BOOL        enableMvc;                      //!<< H.264 MVC
        BOOL        enableTiled2Linear;
        FrameFlag   tiled2LinearMode;
        BOOL        enableBWB;
        Uint32      rotate;                         //!<< 0, 90, 180, 270
        Uint32      mirror;
        BOOL        enableDering;                   //!<< MPEG-2/4
        BOOL        enableDeblock;                  //!<< MPEG-2/4
        Uint32      mp4class;                       //!<< MPEG_4
        Uint32      frameCacheBypass;
        Uint32      frameCacheBurst;
        Uint32      frameCacheMerge;
        Uint32      frameCacheWayShape;
        LowDelayInfo    lowDelay;                   //!<< H.264
    } coda9;
    struct {
        Uint32      numVCores;                      //!<< This numVCores is valid on PRODUCT_ID_4102 multi-core version
        BOOL        craAsBla;
        Uint32      av1Format;
    } wave;
    Uint32          pfClock;                        //!<< performance clock in Hz
    BOOL            performance;
    Uint32          bandwidth;
    Uint32          fps;
    Uint32          enableUserData;
    /* FEEDER */
    FeedingMethod       feedingMode;
    Uint32              feedingSize;
    Uint32              loopCount;
    BOOL                errorInject;
    BOOL                ignoreHangup;
    BOOL                nonRefFbcWrite;         //!<< If it is TRUE, FBC data of non-reference picture are written into framebuffer. */
    ErrorConcealMode    errConcealMode;
    ErrorConcealUnit    errConcealUnit;
} TestDecConfig;

typedef struct {
    DecOutputInfo*  pOutInfo;
    BOOL            scaleX;
    BOOL            scaleY;
    Int32           displayed_num;
} RendererOutInfo;

typedef struct TestEncConfig_struct {
    char    yuvSourceBaseDir[MAX_FILE_PATH];
    char    yuvFileName[MAX_FILE_PATH];
    char    cmdFileName[MAX_FILE_PATH];
    char    bitstreamFileName[MAX_FILE_PATH];
    char    huffFileName[MAX_FILE_PATH];
    char    cInfoFileName[MAX_FILE_PATH];
    char    qMatFileName[MAX_FILE_PATH];
    char    qpFileName[MAX_FILE_PATH];
    char    cfgFileName[MAX_FILE_PATH];
    CodStd  stdMode;
    int     picWidth;
    int     picHeight;
    int     kbps;
    int     rotAngle;
    MirrorDirection mirDir;
    int     useRot;
    int     qpReport;
    int     ringBufferEnable;
    int     rcIntraQp;
    int     outNum;
    int     skipPicNums[MAX_PIC_SKIP_NUM];
    Uint32     coreIdx;
    TiledMapType mapType;
    // 2D cache option

    int lineBufIntEn;
    int subFrameSyncEn;
    int subFrameSyncMode;
    int subFrameSyncFbCount;
    int en_container;                   //enable container
    int container_frame_rate;           //framerate for container
    int picQpY;

    int cbcrInterleave;
    int nv21;
    int i422;
    BOOL needSourceConvert;         //!<< If the format of YUV file is YUV planar mode and EncOpenParam::cbcrInterleave or EncOpenParam::nv21 is true
                                    //!<< the value of needSourceConvert should be true.
    int packedFormat;
    FrameBufferFormat srcFormat;
    int secondaryAXI;
    EndianMode stream_endian;
    int frame_endian;
    int source_endian;

    ProductId productId;
    BOOL  forced_10b;
    int   last_2bit_in_8to10;
    BOOL  csc_enable;
    int   csc_format_order;
    int compareType;
#define YUV_MODE_YUV 0
#define YUV_MODE_YUV_LOADER 2
#define YUV_MODE_CFBC       3
    int yuv_mode;
    char ref_stream_path[MAX_FILE_PATH];
    int loopCount;
    char ref_recon_md5_path[MAX_FILE_PATH];
    BOOL    nonRefFbcWrite;
    BOOL    performance;
    Uint32  bandwidth;
    Uint32  fps;
    Uint32  pfClock;
    char roi_file_name[MAX_FILE_PATH];
    osal_file_t roi_file;
    int roi_enable;

    int encAUD;
    int encEOS;
    int encEOB;
    int yuvfeeder;
    struct {
        BOOL        enableLinear2Tiled;
        FrameFlag   linear2TiledMode;
    } coda9;
    int useAsLongtermPeriod;
    int refLongtermPeriod;

    // newly added for encoder
    osal_file_t scaling_list_file;
    char   scaling_list_fileName[MAX_FILE_PATH];

    osal_file_t custom_lambda_file;
    char   custom_lambda_fileName[MAX_FILE_PATH];
    Uint32 roi_avg_qp;

    osal_file_t lambda_map_file;
    Uint32 lambda_map_enable;
    char   lambda_map_fileName[MAX_FILE_PATH];

    osal_file_t mode_map_file;
    Uint32 mode_map_flag;
    char   mode_map_fileName[MAX_FILE_PATH];

    osal_file_t wp_param_file;
    Uint32 wp_param_flag;
    char   wp_param_fileName[MAX_FILE_PATH];

    char   CustomMapFile[MAX_FILE_PATH]; //WAVE6
    osal_file_t  CustomMapFile_fp;

    Int32  force_picskip_start;
    Int32  force_picskip_end;
    Int32  force_coefdrop_start;
    Int32  force_coefdrop_end;
    Int32  numChangeParam;
    WaveChangeParam changeParam[MAX_CHANGE_PARAM_NUM];
    Int32  numChangePicParam;
    WaveChangePicParam changePicParam[MAX_CHANGE_PARAM_NUM];
    ParamChange coda9ParamChange;

    int    forceIdrPicIdx;
    int    forcePicQpPicIdx;
    int    forcePicQpValue;
    int    forcePicTypePicIdx;
    int    forcePicTypeValue;
    char optYuvPath[MAX_FILE_PATH];
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    int srcReleaseIntEnable;
#endif
    int ringBufferWrapEnable;

    EncSeiNalData seiDataEnc;
    char hrd_rbsp_file_name[MAX_FILE_PATH];
    osal_file_t hrd_rbsp_fp;
    char vui_rbsp_file_name[MAX_FILE_PATH];
    osal_file_t vui_rbsp_fp;
    char prefix_sei_nal_file_name[MAX_FILE_PATH];
    osal_file_t prefix_sei_nal_fp;
    int  prefix_sei_file_max_size;
    char suffix_sei_nal_file_name[MAX_FILE_PATH];
    osal_file_t suffix_sei_nal_fp;
    int  suffix_sei_file_max_size;
    int s2SearchCenterEnable;
    int s2SearchCenterXDiv4;
    int s2SearchCenterYDiv4;
#define NAL_SIZE_REPORT_MAX_SLICE_NUM (64+128)//reserved + register
#define NAL_SIZE_REPORT_SIZE (NAL_SIZE_REPORT_MAX_SLICE_NUM*4 + 0x20) //max size in nal-size-report
    Int32 enReportNalSize;
    char  nalSizeDataPath[MAX_FILE_PATH];
    BOOL NoLastFrameCheck;
    EndianMode      custom_map_endian;
    WAVE6_ENC_CFG   cur_w6_param;
    Uint32 isSecureInst;
    Uint32 instPriority;

    int fixed_qp;
    int fixed_qp_value;

    int api_mode;
} TestEncConfig;

extern BOOL SaveEncNalSizeReport(EncHandle handle, CodStd codecstd, char *nalSizeDataPath, EncOutputInfo encOutputInfo, int frameIdx);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
extern BOOL SetupEncoderOpenParam(
    EncOpenParam*   param,
    TestEncConfig*  config,
    ENC_CFG*        encConfig
    );

extern Int32 GetEncOpenParam(
    EncOpenParam*   pEncOP,
    TestEncConfig*  pEncConfig,
    ENC_CFG*        pEncCfg
    );

extern Int32 GetEncOpenParamDefault(
    EncOpenParam*   pEncOP,
    TestEncConfig*  pEncConfig
    );

extern void GenRegionToMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y in CTU)  */
    int *roiLevel,
    int num,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap
    );

#define VUI_HRD_RBSP_BUF_SIZE           0x4000
#define SEI_NAL_DATA_BUF_SIZE           0x4000
extern Int32 writeVuiRbsp(
    int coreIdx,
    TestEncConfig *encConfig,
    EncOpenParam *encOP,
    vpu_buffer_t *vbVuiRbsp
    );
extern Int32 writeHrdRbsp(
    int coreIdx,
    TestEncConfig *encConfig,
    EncOpenParam *encOP,
    vpu_buffer_t *vbHrdRbsp
    );

extern void setEncBgMode(
    EncParam *encParam,
    TestEncConfig encConfig
    );

extern void GenRegionToQpMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
    int *roiLevel,
    int num,
    int initQp,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap
    );

extern void CheckParamRestriction(
    Uint32 productId,
    TestEncConfig *encConfig
    );
extern int openRoiMapFile(
    TestEncConfig *encConfig
    );
extern int allocateRoiMapBuf(
    EncHandle handle,
    TestEncConfig encConfig,
    vpu_buffer_t *vbROi,
    int srcFbNum,
    int ctuNum
    );

extern void SetMapData(
    int coreIdx,
    TestEncConfig encConfig,
    EncOpenParam encOP,
    EncParam *encParam,
    int srcFrameWidth,
    int srcFrameHeight,
    unsigned long addrCustomMap
    );

extern int w6_init_change_param(
    EncHandle handle,
    Int32 changedCount,
    TestEncConfig* pTestEncConfig,
    EncOpenParam* pEncOpenParam,
    EncWave6ChangeParam* pw6_chg_param,
    vpu_buffer_t* vbHrdRbsp,
    vpu_buffer_t* vbVuiRbsp
    );
extern BOOL GetBitstreamToBuffer(
    EncHandle handle,
    Uint8* pBuffer,
    PhysicalAddress rdAddr,
    PhysicalAddress wrAddr,
    PhysicalAddress streamBufStartAddr,
    PhysicalAddress streamBufEndAddr,
    Uint32 streamSize,
    EndianMode endian,
    BOOL enabledRinbuffer
    );

extern void SetDefaultEncTestConfig(
    TestEncConfig* testConfig
    );

/************************************************************************/
/* User Parameters (ENCODER)                                            */
/************************************************************************/
// user scaling list
#define SL_NUM_MATRIX (6)

typedef struct
{
    Uint8 s4[SL_NUM_MATRIX][16]; // [INTRA_Y/U/V,INTER_Y/U/V][NUM_COEFF]
    Uint8 s8[SL_NUM_MATRIX][64];
    Uint8 s16[SL_NUM_MATRIX][64];
    Uint8 s32[SL_NUM_MATRIX][64];
    Uint8 s16dc[SL_NUM_MATRIX];
    Uint8 s32dc[2];
}UserScalingList;

enum ScalingListSize
{
    SCALING_LIST_4x4 = 0,
    SCALING_LIST_8x8,
    SCALING_LIST_16x16,
    SCALING_LIST_32x32,
    SCALING_LIST_SIZE_NUM
};

extern int parse_user_scaling_list(
    UserScalingList* sl,
    osal_file_t fp_sl,
    CodStd  stdMode
    );

// custom lambda
#define NUM_CUSTOM_LAMBDA   (2*52)
extern int parse_custom_lambda(Uint32 buf[NUM_CUSTOM_LAMBDA], osal_file_t fp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

typedef struct {
    Int32           core_idx;
    ProductId       prod_id;
    CodStd          std;
    Int32           pic_w;
    Int32           pic_h;
    BOOL            is_10bit;
    BOOL            is_enc;
    Int32           axi_mode;

    //Only decoder
    Int32           ctu_size;
}Wave2AXIParam;

extern RetCode SetChangeParam(EncHandle handle, TestEncConfig encConfig, EncOpenParam encOP, Int32 changedCount);
extern RetCode W6SetChangeParam(EncHandle handle, TestEncConfig *pEncConfig, EncOpenParam *pEncOP, Int32 changedCount, vpu_buffer_t *vbHrdRbsp, vpu_buffer_t *vbVuiRbsp);
extern RetCode W6SetChangePicParam(EncHandle handle, TestEncConfig *pEncConfig, Int32 changedCount, EncParam* encParam);
#endif	/* _MAIN_HELPER_H_ */

