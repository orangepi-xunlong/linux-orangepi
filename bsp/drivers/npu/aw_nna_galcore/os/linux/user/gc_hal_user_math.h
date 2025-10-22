/****************************************************************************
*
*    Copyright (c) 2005 - 2024 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/

#ifndef __gc_hal_user_math_h_
#define __gc_hal_user_math_h_

#ifndef _ISOC99_SOURCE
#  define _ISOC99_SOURCE
#endif

#if (defined _ISOC99_SOURCE || defined _ISOC9X_SOURCE\
|| (defined __STDC_VERSION__&&__STDC_VERSION__ >= 199901L))
#define __USE_ISOC99 1
#endif

#include <math.h>

#ifdef __STRICT_ANSI__
#define gcoMATH_Sine(X)                 (gctFLOAT)(sin((X)))
#define gcoMATH_Cosine(X)               (gctFLOAT)(cos((X)))
#define gcoMATH_Floor(X)                (gctFLOAT)(floor((X)))
#define gcoMATH_Ceiling(X)              (gctFLOAT)(ceil((X)))
#define gcoMATH_Exp(X)                  (gctFLOAT)(exp((X)))
#define gcoMATH_Exp2(X)                 (gctFLOAT)(pow(2.0f, (X)))
#define gcoMATH_Absolute(X)             (gctFLOAT)(fabs((X)))
#define gcoMATH_ArcCosine(X)            (gctFLOAT)(acos((X)))
#define gcoMATH_Tangent(X)              (gctFLOAT)(tan((X)))
#define gcoMATH_TangentH(X)             (gctFLOAT)(tanh((X)))
#define gcoMATH_ArcSine(X)              (gctFLOAT)(asin((X)))
#define gcoMATH_ArcTangent(X)           (gctFLOAT)(atan((X)))
#define gcoMATH_Modulo(X, Y)            (gctFLOAT)(fmod((X),(Y)))
#define gcoMATH_Power(X, Y)             (gctFLOAT)(pow((X),(Y)))
#define gcoMATH_SquareRoot(X)           (gctFLOAT)(sqrt((X)))
#define gcoMATH_ReciprocalSquareRoot(X) (gctFLOAT)(1.0f / sqrt((X)))
#define gcoMATH_Log(X)                  (gctFLOAT)(log((X)))
#define gcoMATH_Log2(X)                 (gctFLOAT)(log((X)) / log(2.0f))
#else
#define gcoMATH_Sine(X)                 (gctFLOAT)(sinf((X)))
#define gcoMATH_Cosine(X)               (gctFLOAT)(cosf((X)))
#define gcoMATH_Floor(X)                (gctFLOAT)(floorf((X)))
#define gcoMATH_Ceiling(X)              (gctFLOAT)(ceilf((X)))
#define gcoMATH_Exp(X)                  (gctFLOAT)(expf((X)))
#define gcoMATH_Exp2(X)                 (gctFLOAT)(powf(2.0f, (X)))
#define gcoMATH_Absolute(X)             (gctFLOAT)(fabsf((X)))
#define gcoMATH_ArcCosine(X)            (gctFLOAT)(acosf((X)))
#define gcoMATH_Tangent(X)              (gctFLOAT)(tanf((X)))
#define gcoMATH_TangentH(X)             (gctFLOAT)(tanhf((X)))
#define gcoMATH_ArcSine(X)              (gctFLOAT)(asinf((X)))
#define gcoMATH_ArcTangent(X)           (gctFLOAT)(atanf((X)))
#define gcoMATH_Modulo(X, Y)            (gctFLOAT)(fmodf((X),(Y)))
#define gcoMATH_Power(X, Y)             (gctFLOAT)(powf((X),(Y)))
#define gcoMATH_SquareRoot(X)           (gctFLOAT)(sqrtf((X)))
#define gcoMATH_ReciprocalSquareRoot(X) (gctFLOAT)(1.0f / sqrtf((X)))
#define gcoMATH_Log(X)                  (gctFLOAT)(logf((X)))
#define gcoMATH_Log2(X)                 (gctFLOAT)(logf((X)) / logf(2.0f))
#endif

#define gcoMATH_Add_HighPrecision(X, Y)          (gctFLOAT)((gctDOUBLE)(X) + (gctDOUBLE)(Y))
#define gcoMATH_Multiply_HighPrecision(X, Y)     (gctFLOAT)((gctDOUBLE)(X) * (gctDOUBLE)(Y))
#define gcoMATH_Divide_HighPrecision(X, Y)       (gctFLOAT)((gctDOUBLE)(X) / (gctDOUBLE)(Y))
#define gcoMATH_Power_HighPrecision(X, Y)        (gctFLOAT)(pow((gctDOUBLE)(X),(gctDOUBLE)(Y)))
#define gcoMATH_SquareRoot_HighPrecision(X) (gctFLOAT)((gctDOUBLE)sqrt((X)))
#define gcoMATH_ReciprocalSquareRoot_HighPrecision(X) (gctFLOAT)((gctDOUBLE)(1.0 / sqrt((X))))
#define gcoMATH_Sine_HighPrecision(X)       (gctFLOAT)(sin((gctDOUBLE)(X)))
#define gcoMATH_Cosine_HighPrecision(X)     (gctFLOAT)(cos((gctDOUBLE)(X)))
#define gcoMATH_Floor_HighPrecision(X)      (gctFLOAT)(floor((gctDOUBLE)(X)))
#define gcoMATH_Ceiling_HighPrecision(X)    (gctFLOAT)(ceil((gctDOUBLE)(X)))
#define gcoMATH_Exp_HighPrecision(X)        (gctFLOAT)(exp((gctDOUBLE)(X)))
#define gcoMATH_Exp2_HighPrecision(X)        (gctFLOAT)(pow(2.0, (gctDOUBLE)(X)))
#define gcoMATH_Absolute_HighPrecision(X)   (gctFLOAT)(fabs((gctDOUBLE)(X)))
#define gcoMATH_ArcCosine_HighPrecision(X)  (gctFLOAT)(acos((gctDOUBLE)(X)))
#define gcoMATH_Tangent_HighPrecision(X)    (gctFLOAT)(tan((gctDOUBLE)(X)))
#define gcoMATH_TangentH_HighPrecision(X)   (gctFLOAT)(tanh((gctDOUBLE)(X)))
#define gcoMATH_ArcSine_HighPrecision(X)    (gctFLOAT)(asin((gctDOUBLE)(X)))
#define gcoMATH_ArcTangent_HighPrecision(X) (gctFLOAT)(atan((gctDOUBLE)(X)))
#define gcoMATH_Modulo_HighPrecision(X, Y)     (gctFLOAT)(fmod((gctDOUBLE)(X),(gctDOUBLE)(Y)))
#define gcoMATH_Log_HighPrecision(X)    (gctFLOAT)(log(((gctDOUBLE)X)))
#define gcoMATH_Log2_HighPrecision(X)   (gctFLOAT)(log(((gctDOUBLE)X)) / log(2.0))

#define gcoMATH_ModuloInt(X, Y)         (gctINT)((X) % (Y))
#define gcoMATH_ModuloUInt(X, Y)        (gctUINT)((X) % (Y))
#define gcoMATH_DivideInt(X, Y)         (gctINT)((X) / (Y))
#define gcoMATH_DivideUInt(X, Y)        (gctUINT)((X) / (Y))
#define gcoMATH_DivideUInt64(X, Y)      (gctUINT64)((X) / (Y))
#define gcoMATH_Add(X, Y)               (gctFLOAT)((X) + (Y))
#define gcoMATH_Multiply(X, Y)          (gctFLOAT)((X) * (Y))
#define gcoMATH_Divide(X, Y)            (gctFLOAT)((X) / (Y))
#define gcoMATH_DivideFromUInteger(X, Y) (gctFLOAT)(X) / (gctFLOAT)(Y)

#define gcoMATH_UInt2Float(X)           (gctFLOAT)(X)
#define gcoMATH_Int2Float(X)            (gctFLOAT)(X)
#define gcoMATH_Float2UInt(X)           (gctUINT) (X + 0.5f)
#define gcoMATH_Float2Int(X)            (gctINT)  ((X > 0.0f)? (X + 0.5f) : (X - 0.5f))
#define gcoMATH_Float2Int64(X)          (gctINT64) ((X > 0.0f)? (X + 0.5f) : (X - 0.5f))
#define gcoMATH_Fixed2Float(X)          (gctFLOAT)((X) / 65536.0f)
#define gcoMATH_Float2NormalizedUInt8(X) (gctUINT8)((X) * 255.0f + 0.5f)

#define gcoMATH_MultiplyFixed(X, Y) \
    (gctFIXED_POINT)( ((gctINT64) (X) * (Y)) >> 16 )

#define gcoMATH_DivideFixed(X, Y) \
    (gctFIXED_POINT)( (((gctINT64) (X)) << 16) / (Y) )

#define gcoMATH_MultiplyDivideFixed(X, Y, Z) \
    (gctFIXED_POINT)( (gctINT64) (X) * (Y) / (Z) )

#define gcoMATH_MIN(X, Y) (((X) < (Y))?(X):(Y))
#define gcoMATH_MAX(X, Y) (((X) > (Y))?(X):(Y))

static gcmINLINE gctUINT gcoMath_Float2UINT_STICKROUNDING(IN gctFLOAT Value)
{
    gctUINT ret = 0;
    gctFLOAT dp = 0.0f;

    if (Value < 0.0f)
    {
        return 0;
    }

    ret = (gctUINT)(Value);
    dp = Value - (gctFLOAT)ret;

    if ((dp == 0.5f && (ret & 0x1u)) || dp > 0.5f)
    {
        ret++;
    }

    return ret;
}

#endif

