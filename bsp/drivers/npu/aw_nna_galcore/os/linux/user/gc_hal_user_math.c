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

#include "gc_hal_types.h"
#include "gc_hal_user_math.h"
#include "gc_hal_user_linux.h"

gctUINT32
gcoMATH_Log2in5dot5(
    IN gctINT X
    )
{
    gctUINT32 res = 0;

    if ( X <= 1 )
    {
        return 0;
    }

    if (!(X & 0xFF))
    {
        X >>= 8;
        res += 8 * 32;
    }
    if (!(X & 0xF))
    {
        X >>= 4;
        res += 4 * 32;
    }
    if (!(X & 0x3))
    {
        X >>= 2;
        res += 2 * 32;
    }
    if (!(X & 0x1))
    {
        X >>= 1;
        res += 32;
    }

    switch(X)
    {
    case 1:
        break;
    case 3:
        /* Return res + log_2(3)*32.f = res + (gctUIN32)50.7188f */
        res += 50;
        break;
    case 5:
        /* Return res + log_2(5)*32.f = res + (gctUIN32)74.301699f */
        res += 74;
        break;
    default:
        /* Return res + log_2(x)*32.f = res + log_e(x) / log_e(2) * 32.f
                                      = res + (gctUIN32)46.166241f
        */
        res += (gctUINT32)(gcoMATH_Log(X) * 46.166241f);
    }

    return res;
}


gctUINT32
gcoMATH_FloatAsUInt(
    IN gctFLOAT X
    )
{
    union __anon_1
    {
        gctFLOAT f;
        gctUINT32 u;
    }
    var;

    var.f = X;
    return var.u;
}

gctFLOAT
gcoMATH_UIntAsFloat(
    IN gctUINT32 X
    )
{
    union __anon_1
    {
        gctFLOAT f;
        gctUINT32 u;
    }
    var;

    var.u = X;
    return var.f;
}

gctBOOL
gcoMATH_CompareEqualF(
    IN gctFLOAT X,
    IN gctFLOAT Y
    )
{
    union __anon_1
    {
        gctFLOAT f;
        gctUINT32 u;
    }
    var1, var2;

    var1.f = X;
    var2.f = Y;

    return (var1.u == var2.u);
}

/* Return result in 16bit UINT format.
 * Don't round up the UINT input.
 */
gctUINT16
gcoMATH_UInt8AsFloat16(
    IN gctUINT8 X
    )
{
    gctUINT16 exp, x16, mask;

    if (X == 0x0)
        return (gctUINT16)0x0;

    x16  = X;
    exp  = 15;
    mask = 0x1 << 8;

    while((x16 & mask) == 0)
    {
        exp--;
        x16 <<= 1;
    }

    x16 = (x16 & 0xFF) << 2;

    return (exp << 10) | x16;
}


/* Defines and constants for 32-bit floating point. */
#define gcdSINGLEFLOATEXP              8
#define gcdSINGLEFLOATEXPMASK          0xFF
#define gcdSINGLEFLOATEXPBIAS          (gcdSINGLEFLOATEXPMASK >> 1)
#define gcdGETSINGLEFLOATEXP(x)        ((x >> (32 - 1 - 8)) & gcdSINGLEFLOATEXPMASK)
#define gcdSINGLEFLOATMAN              23
#define gcdFORMSINGLEFLOAT(s,e,m)       (((s)<<(32 - 1))|((e)<<(32 - 9))|(m))
#define gcdSINGLEFLOATEXPINFINITY       0xFF

/* Defines and constants for 16-bit floating point. */
#define gcdFLOAT16_EXPONENTSIZE            5
#define gcdFLOAT16_EXPONENTMASK            ((1 << gcdFLOAT16_EXPONENTSIZE) - 1)
#define gcdFLOAT16_MANTISSASIZE            10
#define gcdFLOAT16_MANTISSAMASK            ((1 << gcdFLOAT16_MANTISSASIZE) - 1)
#define gcdFLOAT16_BIAS                    (gcdFLOAT16_EXPONENTMASK >> 1)
#define gcdFLOAT16_INFINITY                gcdFLOAT16_EXPONENTMASK

/*
** Convert float16 to single float
*/
gctUINT32
gcoMATH_Float16ToFloat(
    IN gctUINT16 In
    )
{
    gctUINT16 signIn = In >> (gcdFLOAT16_MANTISSASIZE + gcdFLOAT16_EXPONENTSIZE);
    gctUINT16 expIn  = (In >> gcdFLOAT16_MANTISSASIZE) & gcdFLOAT16_EXPONENTMASK;
    gctUINT16 expOut;
    gctUINT32 manIn, manOut;

    if (expIn == 0)
    {
        return gcdFORMSINGLEFLOAT(signIn, 0, 0);
    }

    if (expIn == gcdFLOAT16_INFINITY)
    {
        return gcdFORMSINGLEFLOAT(signIn, gcdSINGLEFLOATEXPINFINITY, 0);
    }

    manIn = In & gcdFLOAT16_MANTISSAMASK;

    expOut = expIn - gcdFLOAT16_BIAS + gcdSINGLEFLOATEXPBIAS;
    manOut = manIn << (gcdSINGLEFLOATMAN - gcdFLOAT16_MANTISSASIZE);

    return gcdFORMSINGLEFLOAT(signIn, expOut, manOut);
}


#define  gcdFLOAT16_SIGNBIT            15
#define  gcdFLOAT16_EXPONENTBIAS       ((1U << (gcdFLOAT16_EXPONENTSIZE - 1)) - 1)                /* Exponent bias */
#define  gcdFLOAT16_EXPONENTMAX        gcdFLOAT16_EXPONENTBIAS                                   /* Max exponent  */
#define  gcdFLOAT16_EXPONENTMIN        (-gcdFLOAT16_EXPONENTBIAS + 1)                            /* MIn exponent  */
#define  gcdFLOAT16_MAXNORMAL          (((gcdFLOAT16_EXPONENTMAX + 127) << 23) | 0x7FE000)
#define  gcdFLOAT16_MINNORMAL          ((gcdFLOAT16_EXPONENTMIN + 127) << 23)
#define  gcdFLOAT16_BIASDIFF           ((gcdFLOAT16_EXPONENTBIAS - 127) << 23)
#define  gcdFLOAT16_MANTISSASIZEDIFF   (23 - gcdFLOAT16_MANTISSASIZE)

/* pass value is not as good as pass pointer, for SNaN may be changed to QNaN,
   but I thInk it's ok for current usage. */
gctUINT16
gcoMATH_FloatToFloat16(
    IN gctUINT32 In
    )
{
    gctUINT16 ret;
    gctUINT32 u = In;
    gctUINT32 sign = (u & 0x80000000) >> 16;
    gctUINT32 mag = u & 0x7FFFFFFF;

    if((u & (0xff<<23)) == (0xff<<23))
    {
        /* INF or NaN */
        ret = (gctUINT16)(sign | (((1 << gcdFLOAT16_EXPONENTSIZE) - 1))<<gcdFLOAT16_MANTISSASIZE);
        if( (u & (~0xff800000)) != 0 )
        {
            /*
            ** NaN - smash together the fraction bits to
            **       keep the random 1's (In a way that keeps float16->float->float16
            **       conversion Invertible down to the bit level, even with NaN).
            */
            ret = (gctUINT16)(ret| (((u>>13)|(u>>3)|(u))&0x000003ff));
        }
    }
    else if (mag > gcdFLOAT16_MAXNORMAL)
    {
        /* Not representable by 16 bit float -> use flt16_max (due to round to zero) */
        ret = (gctUINT16)(sign | ((((1 << gcdFLOAT16_EXPONENTSIZE) - 2))<<gcdFLOAT16_MANTISSASIZE) | gcdFLOAT16_MANTISSAMASK);
    }
    else if (mag < gcdFLOAT16_MINNORMAL)
    {
        /* Denormalized value */

        /* Make implicit 1 explicit */
        gctUINT32 Frac = (mag & ((1<<23)-1)) | (1<<23);
        gctINT nshift = (gcdFLOAT16_EXPONENTMIN + 127 - (mag >> 23));

        if (nshift < 24)
        {
            mag = Frac >> nshift;
        }
        else
        {
            mag = 0;
        }

        /* Round to zero */
        ret = (gctUINT16)(sign | (mag>>gcdFLOAT16_MANTISSASIZEDIFF));
    }
    else
    {
        /* Normalize value with Round to zero */
        ret = (gctUINT16)(sign | ((mag + gcdFLOAT16_BIASDIFF)>>gcdFLOAT16_MANTISSASIZEDIFF));
    }

    return ret;
}


/* Defines and constants for 11-bit floatIng poInt. */
#define gcdFLOAT11_EXPONENTSIZE            5
#define gcdFLOAT11_EXPONENTMASK            ((1 << gcdFLOAT11_EXPONENTSIZE) - 1)
#define gcdFLOAT11_MANTISSASIZE            6
#define gcdFLOAT11_MANTISSAMASK            ((1 << gcdFLOAT11_MANTISSASIZE) - 1)
#define gcdFLOAT11_BIAS                    (gcdFLOAT11_EXPONENTMASK >> 1)
#define gcdFLOAT11_INFINITY                gcdFLOAT11_EXPONENTMASK

/*
** Convert float11 to single float
*/
gctUINT32
gcoMATH_Float11ToFloat(
    IN gctUINT32 In
    )
{
    gctUINT16 expIn  = (In >> gcdFLOAT11_MANTISSASIZE) & gcdFLOAT11_EXPONENTMASK;
    gctUINT16 expOut;
    gctUINT32 manIn, manOut;

    if (expIn == 0)
    {
        return gcdFORMSINGLEFLOAT(0, 0, 0);
    }

    if (expIn == gcdFLOAT11_INFINITY)
    {
        return gcdFORMSINGLEFLOAT(0, gcdSINGLEFLOATEXPINFINITY, 0);
    }

    manIn = In & gcdFLOAT11_MANTISSAMASK;

    expOut = expIn - gcdFLOAT11_BIAS + gcdSINGLEFLOATEXPBIAS;
    manOut = manIn << (gcdSINGLEFLOATMAN - gcdFLOAT11_MANTISSASIZE);

    return gcdFORMSINGLEFLOAT(0, expOut, manOut);
}


#define  gcdFLOAT11_EXPONENTBIAS        ((1U << (gcdFLOAT11_EXPONENTSIZE - 1)) - 1)                /* Exponent bias */
#define  gcdFLOAT11_EXPONENTMAX         gcdFLOAT11_EXPONENTBIAS                                   /* Max exponent */
#define  gcdFLOAT11_EXPONENTMIN         (-gcdFLOAT11_EXPONENTBIAS + 1)                            /* MIn exponent */
#define  gcdFLOAT11_MAXNORMAL           (((gcdFLOAT11_EXPONENTMAX + 127) << 23) | 0x7FE000)
#define  gcdFLOAT11_MINNORMAL           ((gcdFLOAT11_EXPONENTMIN + 127) << 23)
#define  gcdFLOAT11_BIASDIFF            ((gcdFLOAT11_EXPONENTBIAS - 127) << 23)
#define  gcdFLOAT11_MANTISSASIZEDIFF    (23 - gcdFLOAT11_MANTISSASIZE)

/* pass value is not as good as pass pointer, for SNaN may be changed to QNaN,
   but I thInk it's ok for current usage. */
gctUINT16
gcoMATH_FloatToFloat11(
    IN gctUINT32 In
    )
{
    gctUINT16 ret;
    gctUINT32 u = In;
    gctUINT32 sign = (u & 0x80000000);
    gctUINT32 mag = u & 0x7FFFFFFF;

    if((u & (0xff<<23)) == (0xff<<23))
    {
        /* INF or NaN */
        if( (u & (~0xff800000)) != 0 )
        {
            /*
            ** NaN - smash together the fraction bits to
            **       keep the random 1's (In a way that keeps float16->float->float16
            **       conversion Invertible down to the bit level, even with NaN).
            */
            ret = (gctUINT16)(gcdFLOAT11_EXPONENTMASK << gcdFLOAT11_MANTISSASIZE) |1;
        }
        else
        {
            if (sign) /*-INF */
            {
                ret = (gctUINT16)0;
            }
            else /*+INF */
            {
                ret = (gctUINT16)(gcdFLOAT11_EXPONENTMASK << gcdFLOAT11_MANTISSASIZE);
            }
        }
    }
    else if (mag > gcdFLOAT11_MAXNORMAL)
    {
        if (sign)
        {
            ret = 0;
        }
        else
        {
            /* Not representable by 16 bit float -> use flt16_max (due to round to zero) */
            ret = (gctUINT16)(((((1 << gcdFLOAT11_EXPONENTSIZE) - 2))<<gcdFLOAT11_MANTISSASIZE) | gcdFLOAT11_MANTISSAMASK);
        }
    }
    else if (mag < gcdFLOAT11_MINNORMAL)
    {
        /* Denormalized value */

        /* Make implicit 1 explicit */
        gctUINT32 Frac = (mag & ((1<<23)-1)) | (1<<23);
        gctINT nshift = (gcdFLOAT11_EXPONENTMIN + 127 - (mag >> 23));

        if (nshift < 24)
        {
            mag = Frac >> nshift;
        }
        else
        {
            mag = 0;
        }

        if (sign)
        {
            ret = 0;
        }
        else
        {
            /* Round to zero */
            ret = (gctUINT16)(mag>>gcdFLOAT11_MANTISSASIZEDIFF);
        }
    }
    else
    {
        if (sign)
        {
            ret = 0;
        }
        else
        {
            /* Normalize value with Round to zero */
            ret = (gctUINT16)((mag + gcdFLOAT11_BIASDIFF)>>gcdFLOAT11_MANTISSASIZEDIFF);
        }
    }

    return ret;
}


/* Defines and constants for 10-bit floatIng poInt. */
#define gcdFLOAT10_EXPONENTSIZE            5
#define gcdFLOAT10_EXPONENTMASK            ((1 << gcdFLOAT10_EXPONENTSIZE) - 1)
#define gcdFLOAT10_MANTISSASIZE            5
#define gcdFLOAT10_MANTISSAMASK            ((1 << gcdFLOAT10_MANTISSASIZE) - 1)
#define gcdFLOAT10_BIAS                    (gcdFLOAT10_EXPONENTMASK >> 1)
#define gcdFLOAT10_INFINITY                gcdFLOAT10_EXPONENTMASK

/*
** Convert float10 to single float
*/
gctUINT32
gcoMATH_Float10ToFloat(
    IN gctUINT32 In
    )
{
    gctUINT16 expIn  = (In >> gcdFLOAT10_MANTISSASIZE) & gcdFLOAT10_EXPONENTMASK;
    gctUINT16 expOut;
    gctUINT32 manIn, manOut;

    if (expIn == 0)
    {
        return gcdFORMSINGLEFLOAT(0, 0, 0);
    }

    if (expIn == gcdFLOAT10_INFINITY)
    {
        return gcdFORMSINGLEFLOAT(0, gcdSINGLEFLOATEXPINFINITY, 0);
    }

    manIn = In & gcdFLOAT10_MANTISSAMASK;

    expOut = expIn - gcdFLOAT10_BIAS + gcdSINGLEFLOATEXPBIAS;
    manOut = manIn << (gcdSINGLEFLOATMAN - gcdFLOAT10_MANTISSASIZE);

    return gcdFORMSINGLEFLOAT(0, expOut, manOut);
}


#define  gcdFLOAT10_EXPONENTBIAS          ((1U << (gcdFLOAT10_EXPONENTSIZE - 1)) - 1)                /* Exponent bias */
#define  gcdFLOAT10_EXPONENTMAX           gcdFLOAT10_EXPONENTBIAS                                   /* Max exponent */
#define  gcdFLOAT10_EXPONENTMIN           (-gcdFLOAT10_EXPONENTBIAS + 1)                            /* MIn exponent */
#define  gcdFLOAT10_MAXNORMAL             (((gcdFLOAT10_EXPONENTMAX + 127) << 23) | 0x7FE000)
#define  gcdFLOAT10_MINNORMAL             ((gcdFLOAT10_EXPONENTMIN + 127) << 23)
#define  gcdFLOAT10_BIASDIFF              ((gcdFLOAT10_EXPONENTBIAS - 127) << 23)
#define  gcdFLOAT10_MANTISSASIZEDIFF      (23 - gcdFLOAT10_MANTISSASIZE)

/* Pass value is not as good as pass pointer, for SNaN may be changed to QNaN,
   but I thInk it's ok for current usage. */
gctUINT16
gcoMATH_FloatToFloat10(
    IN gctUINT32 In
    )
{
    gctUINT16 ret;
    gctUINT32 u = In;
    gctUINT32 sign = (u & 0x80000000);
    gctUINT32 mag = u & 0x7FFFFFFF;

    if((u & (0xff<<23)) == (0xff<<23))
    {
        /* INF or NaN */
        if( (u & (~0xff800000)) != 0 )
        {
            /*
            ** NaN - smash together the fraction bits to
            **       keep the random 1's (In a way that keeps float16->float->float16
            **       conversion Invertible down to the bit level, even with NaN).
            */
            ret = (gctUINT16)(gcdFLOAT10_EXPONENTMASK << gcdFLOAT10_MANTISSASIZE)|1;
        }
        else
        {
            if (sign) /*-INF */
            {
                ret = (gctUINT16)0;
            }
            else /*+INF */
            {
                ret = (gctUINT16)(gcdFLOAT10_EXPONENTMASK << gcdFLOAT10_MANTISSASIZE);
            }
        }
    }
    else if (mag > gcdFLOAT10_MAXNORMAL)
    {
        if (sign)
        {
            ret = 0;
        }
        else
        {
            /* Not representable by 16 bit float -> use flt16_max (due to round to zero) */
            ret = (gctUINT16)(((((1 << gcdFLOAT10_EXPONENTSIZE) - 2))<<gcdFLOAT10_MANTISSASIZE) | gcdFLOAT10_MANTISSAMASK);
        }
    }
    else if (mag < gcdFLOAT10_MINNORMAL)
    {
        /* Denormalized value */

        /* Make implicit 1 explicit */
        gctUINT32 Frac = (mag & ((1<<23)-1)) | (1<<23);
        gctINT32 nshift = (gcdFLOAT10_EXPONENTMIN + 127 - (mag >> 23));

        if (nshift < 24)
        {
            mag = Frac >> nshift;
        }
        else
        {
            mag = 0;
        }

        if (sign)
        {
            ret = 0;
        }
        else
        {
            /* Round to zero */
            ret = (gctUINT16)(mag>>gcdFLOAT10_MANTISSASIZEDIFF);
        }
    }
    else
    {
        if (sign)
        {
            ret = 0;
        }
        else
        {
            /* Normalize value with Round to zero */
            ret = (gctUINT16)((mag + gcdFLOAT10_BIASDIFF)>>gcdFLOAT10_MANTISSASIZEDIFF);
        }
    }

    return ret;
}


/* Defines and constants for 14-bit floatIng poInt. */
#define gcdFLOAT14_EXPONENTSIZE         5
#define gcdFLOAT14_EXPONENTMASK         ((1 << gcdFLOAT14_EXPONENTSIZE) - 1)
#define gcdFLOAT14_MANTISSASIZE         9
#define gcdFLOAT14_MANTISSAMASK         ((1 << gcdFLOAT14_MANTISSASIZE) - 1)
#define gcdFLOAT14_BIAS                 (gcdFLOAT14_EXPONENTMASK >> 1)
#define gcdFLOAT14_INFINITY             gcdFLOAT14_EXPONENTMASK

/*
** Convert float10 to single float
*/
gctUINT32
gcoMATH_Float14ToFloat(
    IN gctUINT16 In
    )
{
    gctUINT16 expIn  = (In >> gcdFLOAT14_MANTISSASIZE) & gcdFLOAT14_EXPONENTMASK;
    gctUINT16 expOut;
    gctUINT32 manIn, manOut;

    if (expIn == 0)
    {
        return gcdFORMSINGLEFLOAT(0, 0, 0);
    }

    if (expIn == gcdFLOAT14_INFINITY)
    {
        return gcdFORMSINGLEFLOAT(0, gcdSINGLEFLOATEXPINFINITY, 0);
    }

    manIn = In & gcdFLOAT14_MANTISSAMASK;

    expOut = expIn - gcdFLOAT14_BIAS + gcdSINGLEFLOATEXPBIAS;
    manOut = manIn << (gcdSINGLEFLOATMAN - gcdFLOAT14_MANTISSASIZE);

    return gcdFORMSINGLEFLOAT(0, expOut, manOut);
}
