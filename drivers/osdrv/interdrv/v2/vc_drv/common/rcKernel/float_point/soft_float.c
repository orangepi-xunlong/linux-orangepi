#include "soft_float.h"

typedef unsigned int flag;

typedef unsigned short bits16;
typedef unsigned int bits32;
typedef unsigned long long int bits64;

typedef int sbits32;
typedef signed long long int sbits64;

unsigned int float_exception_flags;
int8_t float_detect_tininess = float_tininess_after_rounding;

#define is_Negative_NaN 0xFFFFFFFF
#define isNaN 0x7fffffff
#define isINF 0x7f800000
#define is_Negative_INF 0xff800000
#define FLOAT32_ONE 0x3f800000
#define FLOAT32_Negative_ONE 0xbf800000
/* LOG */
#define LOGF_TABLE_BITS 4
#define LOGF_POLY_ORDER 4
#define LOGF_N (1 << LOGF_TABLE_BITS)
#define OFF 0x3f330000

const sw_float logf_incv_table[] = {
    0x3fb30f64,
    0x3fab8f6a,
    0x3fa4a9d0,
    0x3f9e4cae,
    0x3f9868c8,
    0x3f92f114,
    0x3f8dda52,
    0x3f891ac8,
    0x3f84a9fa,
    0x3f800000,
    0x3f730468,
    0x3f652599,
    0x3f5901b3,
    0x3f4e168b,
    0x3f443730,
    0x3f3b3ee8,
};

const sw_float logf_logc_table[] = {
    0xbeabdfbc,
    0xbe95f785,
    0xbe80f574,
    0xbe598ec5,
    0xbe32ba78,
    0xbe0d515e,
    0xbdd273b6,
    0xbd8cb9e3,
    0xbd1297a2,
    0x0,
    0x3d552d53,
    0x3de2f29d,
    0x3e29372c,
    0x3e5e1430,
    0x3e882c5e,
    0x3ea02183,
};

const sw_float logf_poly_table[] = {
    0xbe80751a,
    0x3eaabad8,
    0xbefffff8,
};

const sw_float logf_ln2 = 0x3f317218;
/* LOG */

/* POW */
#define POWF_LOG2_TABLE_BITS 4
#define POWF_LOG2_POLY_ORDER 5
#define POWF_SCALE_BITS 0
#define POWF_SCALE (1 << POWF_SCALE_BITS)
#define POW_N (1 << POWF_LOG2_TABLE_BITS)

uint32_t powf_log2_data_invc[] = {
    0x3fb30f64,
    0x3fab8f6a,
    0x3fa4a9d0,
    0x3f9e4cae,
    0x3f9868c8,
    0x3f92f114,
    0x3f8dda52,
    0x3f891ac8,
    0x3f84a9fa,
    0x3f800000,
    0x3f730468,
    0x3f652599,
    0x3f5901b3,
    0x3f4e168b,
    0x3f443730,
    0x3f3b3ee8,
};
uint32_t powf_log2_data_logc[] = {
    0xbef7f633,
    0xbed85b42,
    0xbeba0c58,
    0xbe9cef49,
    0xbe80ece0,
    0xbe4be0e9,
    0xbe17cf1d,
    0xbdcb065e,
    0xbd537cee,
    0x0,
    0x3d99c655,
    0x3e23b54b,
    0x3e74205a,
    0x3ea03230,
    0x3ec474e1,
    0x3ee70522,
};

uint32_t powf_log2_data_poly[] = {
    0x3e93b0b6,
    0xbeb8cb4d,
    0x3ef63853,
    0xbf38aa3a,
    0x3fb8aa3b,
};
/* POW */

/* EXP */
#define EXP2F_TABLE_BITS 5
#define EXPF_N (1 << EXP2F_TABLE_BITS)
#define SIGN_BIAS (1 << (EXP2F_TABLE_BITS + 11))
#define InvLn2N_f32 (0x4238aa3b)
sw_float expf_tab[] = {
    0x3f800000,
    0x3f7ecd87,
    0x3f7daac3,
    0x3f7c980f,
    0x3f7b95c2,
    0x3f7aa43a,
    0x3f79c3d3,
    0x3f78f4f0,
    0x3f7837f0,
    0x3f778d3a,
    0x3f76f532,
    0x3f767043,
    0x3f75fed7,
    0x3f75a15b,
    0x3f75583f,
    0x3f7523f6,
    0x3f7504f3,
    0x3f74fbaf,
    0x3f7508a4,
    0x3f752c4d,
    0x3f75672a,
    0x3f75b9be,
    0x3f76248c,
    0x3f76a81e,
    0x3f7744fd,
    0x3f77fbb8,
    0x3f78ccdf,
    0x3f79b907,
    0x3f7ac0c7,
    0x3f7be4ba,
    0x3f7d257d,
    0x3f7e83b3,
};

sw_float expf_poly_scaled[] = {
    0x35e357c2,
    0x3975fe73,
    0x3cb17218,
};

sw_float expf_poly[] = {
    0x3d6357c2,
    0x3e75fe73,
    0x3f317218,
};
/* EXP */

void float_raise(int8_t flags)
{
    float_exception_flags |= flags;
}

#if 0
    static inline float
sw_floatTofloat(sw_float f)
{
    union {
        sw_float f;
        float i;
    } u = {f};
    return u.i;
}

    static inline sw_float
floatTosw_float(float f)
{
    union {
        float f;
        sw_float i;
    } u = {f};
    return u.i;
}
#endif

/*
 * Returns 0 if not int, 1 if odd int, 2 if even int.  The argument is
 * the bit representation of a non-zero finite floating-point value.
 */
static inline int checkint(uint32_t iy)
{
    int e = iy >> 23 & 0xff;
    if (e < 0x7f)
        return 0;
    if (e > 0x7f + 23)
        return 2;
    if (iy & ((1 << (0x7f + 23 - e)) - 1))
        return 0;
    if (iy & (1 << (0x7f + 23 - e)))
        return 1;
    return 2;
}

static inline int zeroinfnan(uint32_t ix)
{
    return 2 * ix - 1 >= 2u * 0x7f800000 - 1;
}

static inline flag extractFloat32Sign(sw_float a)
{
    return a >> 31;
}

/*
 *  -------------------------------------------------------------------------------
 *  Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
 *  single-precision floating-point value, returning the result.  After being
 *  shifted into the proper positions, the three fields are simply added
 *  together to form the result.  This means that any integer portion of `zSig'
 *  will be added into the exponent.  Since a properly normalized significand
 *  will have an integer portion equal to 1, the `zExp' input should be 1 less
 *  than the desired result exponent whenever `zSig' is a complete, normalized
 *  significand.
 *  -------------------------------------------------------------------------------
 */
static inline sw_float packFloat32(flag zSign, int16_t zExp, bits32 zSig)
{
    return (((bits32)zSign) << 31) + (((bits32)zExp) << 23) + zSig;
}


/*
 *  -------------------------------------------------------------------------------
 *  Shifts `a' right by the number of bits given in `count'.  If any nonzero
 *  bits are shifted off, they are ``jammed'' into the least significant bit of
 *  the result by setting the least significant bit to 1.  The value of `count'
 *  can be arbitrarily large; in particular, if `count' is greater than 32, the
 *  result will be either 0 or 1, depending on whether `a' is zero or nonzero.
 *  The result is stored in the location pointed to by `zPtr'.
 *  -------------------------------------------------------------------------------
 */
static inline void shift32RightJamming(bits32 a, int16_t count, bits32 *zPtr)
{
    bits32 z;
    if (count == 0) {
        z = a;
    } else if (count < 32) {
        z = (a >> count) | ((a << ((-count) & 31)) != 0);
    } else {
        z = (a != 0);
    }
    *zPtr = z;
}

/*
 *  -------------------------------------------------------------------------------
 *  Returns the number of leading 0 bits before the most-significant 1 bit
 *  of `a'.  If `a' is zero, 32 is returned.
 *  -------------------------------------------------------------------------------
 */
static int8_t countLeadingZeros32(bits32 a)
{
    static const int8_t countLeadingZerosHigh[] = {
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int8_t shiftCount;

    shiftCount = 0;
    if (a < 0x10000) {
        shiftCount += 16;
        a <<= 16;
    }

    if (a < 0x1000000) {
        shiftCount += 8;
        a <<= 8;
    }
    shiftCount += countLeadingZerosHigh[a >> 24];
    return shiftCount;
}

/*
 *  -------------------------------------------------------------------------------
 *  Normalizes the subnormal single-precision floating-point value represented
 *  by the denormalized significand `aSig'.  The normalized exponent and
 *  significand are stored at the locations pointed to by `zExpPtr' and
 *  `zSigPtr', respectively.
 *  -------------------------------------------------------------------------------
 */
static void normalizeFloat32Subnormal(bits32 aSig, int16_t *zExpPtr, bits32 *zSigPtr)
{
    int8_t shiftCount;

    shiftCount = countLeadingZeros32(aSig) - 8;
    *zSigPtr = aSig << shiftCount;
    *zExpPtr = 1 - shiftCount;
}

/*
 *  -------------------------------------------------------------------------------
 *  Takes an abstract floating-point value having sign `zSign', exponent `zExp',
 *  and significand `zSig', and returns the proper single-precision floating-
 *  point value corresponding to the abstract input.  Ordinarily, the abstract
 *  value is simply rounded and packed into the single-precision format, with
 *  the inexact exception raised if the abstract input cannot be represented
 *  exactly.  If the abstract value is too large, however, the overflow and
 *  inexact exceptions are raised and an infinity or maximal finite value is
 *  returned.  If the abstract value is too small, the input value is rounded to
 *  a subnormal number, and the underflow and inexact exceptions are raised if
 *  the abstract input cannot be represented exactly as a subnormal single-
 *  precision floating-point number.
 *  The input significand `zSig' has its binary point between bits 30
 *  and 29, which is 7 bits to the left of the usual location.  This shifted
 *  significand must be normalized or smaller.  If `zSig' is not normalized,
 *  `zExp' must be 0; in that case, the result returned is a subnormal number,
 *  and it must not require rounding.  In the usual case that `zSig' is
 *  normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
 *  The handling of underflow and overflow follows the IEC/IEEE Standard for
 *  Binary Floating-point Arithmetic.
 *  -------------------------------------------------------------------------------
 */
static sw_float roundAndPackFloat32(struct roundingData *roundData, flag zSign, int16_t zExp, bits32 zSig)
{
    int8_t roundingMode;
    flag roundNearestEven;
    int8_t roundIncrement, roundBits;
    flag isTiny;

    roundingMode = roundData->mode;
    roundNearestEven = (roundingMode == float_round_nearest_even);
    roundIncrement = 0x40;
    if (!roundNearestEven) {
        if (roundingMode == float_round_to_zero) {
            roundIncrement = 0;
        } else {
            roundIncrement = 0x7F;
            if (zSign) {
                if (roundingMode == float_round_up)
                    roundIncrement = 0;
            } else {
                if (roundingMode == float_round_down)
                    roundIncrement = 0;
            }
        }
    }
    roundBits = zSig & 0x7F;
    if ((bits16)zExp >= 0xFD) {
        if ((zExp > 0xFD) || ((zExp == 0xFD) && ((sbits32)(zSig + roundIncrement) < 0))) {
            float_raise(float_flag_overflow | float_flag_inexact);
            roundData->exception |= float_flag_overflow | float_flag_inexact;
            return packFloat32(zSign, 0xFF, 0) - (roundIncrement == 0);
        }

        if (zExp < 0) {
            isTiny =
                (float_detect_tininess == float_tininess_before_rounding) ||
                (zExp < -1) || (zSig + roundIncrement < 0x80000000);
            shift32RightJamming(zSig, -zExp, &zSig);
            zExp = 0;
            roundBits = zSig & 0x7F;

            if (isTiny && roundBits) {
                roundData->exception |= float_flag_underflow;
                float_raise(float_flag_underflow);
            }
        }
    }

    if (roundBits) {
        float_raise(float_flag_inexact);
    }
    zSig = (zSig + roundIncrement) >> 7;
    zSig &= ~(((roundBits ^ 0x40) == 0) & roundNearestEven);
    if (zSig == 0)
        zExp = 0;
    return packFloat32(zSign, zExp, zSig);
}

/*
 *  -------------------------------------------------------------------------------
 *  Takes a 64-bit fixed-point value `absZ' with binary point between bits 6
 *  and 7, and returns the properly rounded 32-bit integer corresponding to the
 *  input.  If `zSign' is nonzero, the input is negated before being converted
 *  to an integer.  Bit 63 of `absZ' must be zero.  Ordinarily, the fixed-point
 *  input is simply rounded to an integer, with the inexact exception raised if
 *  the input cannot be represented exactly as an integer.  If the fixed-point
 *  input is too large, however, the invalid exception is raised and the largest
 *  positive or negative integer is returned.
 *  -------------------------------------------------------------------------------
 */
static int32_t roundAndPackInt32(struct roundingData *roundData, flag zSign, bits64 absZ)
{
    int8_t roundingMode;
    flag roundNearestEven;
    int8_t roundIncrement, roundBits;
    int32_t z;

    roundingMode = roundData->mode;
    roundNearestEven = (roundingMode == float_round_nearest_even);
    roundIncrement = 0x40;
    if (!roundNearestEven) {
        if (roundingMode == float_round_to_zero) {
            roundIncrement = 0;
        } else {
            roundIncrement = 0x7F;
            if (zSign) {
                if (roundingMode == float_round_up)
                    roundIncrement = 0;
            } else {
                if (roundingMode == float_round_down)
                    roundIncrement = 0;
            }
        }
    }
    roundBits = absZ & 0x7F;
    absZ = (absZ + roundIncrement) >> 7;
    absZ &= ~(((roundBits ^ 0x40) == 0) & roundNearestEven);
    z = absZ;
    if (zSign)
        z = -z;
    if ((absZ >> 32) || (z && ((z < 0) ^ zSign))) {
        roundData->exception |= float_flag_invalid;
        float_raise(float_flag_invalid);
        return zSign ? 0x80000000 : 0x7FFFFFFF;
    }
    if (roundBits) {
        float_raise(float_flag_inexact);
        roundData->exception |= float_flag_inexact;
    }
    return z;
}

/*
 *  -------------------------------------------------------------------------------
 *  Returns the fraction bits of the single-precision floating-point value `a'.
 *  -------------------------------------------------------------------------------
 */
static inline bits32 extractFloat32Frac(sw_float a)
{
    return a & 0x007FFFFF;
}

/*
 *  -------------------------------------------------------------------------------
 *  Returns the exponent bits of the single-precision floating-point value `a'.
 *  -------------------------------------------------------------------------------
 */
static inline int16_t extractFloat32Exp(sw_float a)
{
    return (a >> 23) & 0xFF;
}

flag float32_is_signaling_nan(sw_float a)
{
    return (((a >> 22) & 0x1FF) == 0x1FE) && (a & 0x003FFFFF);
}

/*
 *  -------------------------------------------------------------------------------
 *  Returns 1 if the single-precision floating-point value `a' is a NaN;
 *  otherwise returns 0.
 *  -------------------------------------------------------------------------------
 */

flag float32_is_nan(sw_float a)
{
    return (0xFF000000 < (bits32)(a << 1));
}

void shift64RightJamming(bits64 a, int16_t count, bits64 *zPtr)
{
    bits64 z;

    if (count == 0) {
        z = a;
    } else if (count < 64) {
        z = (a >> count) | ((a << ((-count) & 63)) != 0);
    } else {
        z = (a != 0);
    }
    *zPtr = z;
}

/*
 * -------------------------------------------------------------------------------
 * Takes an abstract floating-point value having sign `zSign', exponent `zExp',
 * and significand `zSig', and returns the proper single-precision floating-
 * point value corresponding to the abstract input.  This routine is just like
 * `roundAndPackFloat32' except that `zSig' does not have to be normalized in
 * any way.  In all cases, `zExp' must be 1 less than the ``true'' floating-
 * point exponent.
 * -------------------------------------------------------------------------------
 */
static sw_float normalizeRoundAndPackFloat32(struct roundingData *roundData, flag zSign, int16_t zExp, bits32 zSig)
{
    int8_t shiftCount;

    shiftCount = countLeadingZeros32(zSig) - 1;
    return roundAndPackFloat32(roundData, zSign, zExp - shiftCount, zSig << shiftCount);
}

/*
 * -------------------------------------------------------------------------------
 * Takes two single-precision floating-point values `a' and `b', one of which
 * is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
 * signaling NaN, the invalid exception is raised.
 * -------------------------------------------------------------------------------
 */
static sw_float propagateFloat32NaN(sw_float a, sw_float b)
{
    flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

    aIsNaN = float32_is_nan(a);
    aIsSignalingNaN = float32_is_signaling_nan(a);
    bIsNaN = float32_is_nan(b);
    bIsSignalingNaN = float32_is_signaling_nan(b);
    a |= 0x00400000;
    b |= 0x00400000;
    if (aIsSignalingNaN | bIsSignalingNaN)
        float_raise(float_flag_invalid);
    if (aIsNaN) {
        return (aIsSignalingNaN & bIsNaN) ? b : a;
    } else {
        return b;
    }
}

static sw_float addFloat32Sigs(struct roundingData *roundData, sw_float a, sw_float b, flag zSign)
{
    int16_t aExp, bExp, zExp;
    bits32 aSig, bSig, zSig;
    int16_t expDiff;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    bSig = extractFloat32Frac(b);
    bExp = extractFloat32Exp(b);
    expDiff = aExp - bExp;
    aSig <<= 6;
    bSig <<= 6;
    if (expDiff > 0) {
        if (aExp == 0xFF) {
            if (aSig)
                return propagateFloat32NaN(a, b);
            return a;
        }
        if (bExp == 0) {
            --expDiff;
        } else {
            bSig |= 0x20000000;
        }
        shift32RightJamming(bSig, expDiff, &bSig);
        zExp = aExp;
    } else if (expDiff < 0) {
        if (bExp == 0xFF) {
            if (bSig)
                return propagateFloat32NaN(a, b);
            return packFloat32(zSign, 0xFF, 0);
        }
        if (aExp == 0) {
            ++expDiff;
        } else {
            aSig |= 0x20000000;
        }
        shift32RightJamming(aSig, -expDiff, &aSig);
        zExp = bExp;
    } else {
        if (aExp == 0xFF) {
            if (aSig | bSig)
                return propagateFloat32NaN(a, b);
            return a;
        }
        if (aExp == 0)
            return packFloat32(zSign, 0, (aSig + bSig) >> 6);
        zSig = 0x40000000 + aSig + bSig;
        zExp = aExp;
        goto roundAndPack;
    }
    aSig |= 0x20000000;
    zSig = (aSig + bSig) << 1;
    --zExp;
    if ((sbits32)zSig < 0) {
        zSig = aSig + bSig;
        ++zExp;
    }
roundAndPack:
    return roundAndPackFloat32(roundData, zSign, zExp, zSig);
}

/*
 * -------------------------------------------------------------------------------
 * Returns the result of subtracting the absolute values of the single-
 * precision floating-point values `a' and `b'.  If `zSign' is true, the
 * difference is negated before being returned.  `zSign' is ignored if the
 * result is a NaN.  The subtraction is performed according to the IEC/IEEE
 * Standard for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
static sw_float subFloat32Sigs(struct roundingData *roundData, sw_float a, sw_float b, flag zSign)
{
    int16_t aExp, bExp, zExp;
    bits32 aSig, bSig, zSig;
    int16_t expDiff;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    bSig = extractFloat32Frac(b);
    bExp = extractFloat32Exp(b);
    expDiff = aExp - bExp;
    aSig <<= 7;
    bSig <<= 7;
    if (expDiff > 0)
        goto aExpBigger;
    if (expDiff < 0)
        goto bExpBigger;
    if (aExp == 0xFF) {
        if (aSig | bSig)
            return propagateFloat32NaN(a, b);
        float_raise(float_flag_invalid);
        roundData->exception |= float_flag_invalid;
        return is_Negative_NaN;
    }
    if (aExp == 0) {
        aExp = 1;
        bExp = 1;
    }
    if (bSig < aSig)
        goto aBigger;
    if (aSig < bSig)
        goto bBigger;
    return packFloat32(roundData->mode == float_round_down, 0, 0);
bExpBigger:
    if (bExp == 0xFF) {
        if (bSig)
            return propagateFloat32NaN(a, b);
        return packFloat32(zSign ^ 1, 0xFF, 0);
    }
    if (aExp == 0) {
        ++expDiff;
    } else {
        aSig |= 0x40000000;
    }
    shift32RightJamming(aSig, -expDiff, &aSig);
    bSig |= 0x40000000;
bBigger:
    zSig = bSig - aSig;
    zExp = bExp;
    zSign ^= 1;
    goto normalizeRoundAndPack;
aExpBigger:
    if (aExp == 0xFF) {
        if (aSig)
            return propagateFloat32NaN(a, b);
        return a;
    }
    if (bExp == 0) {
        --expDiff;
    } else {
        bSig |= 0x40000000;
    }
    shift32RightJamming(bSig, expDiff, &bSig);
    aSig |= 0x40000000;
aBigger:
    zSig = aSig - bSig;
    zExp = aExp;
normalizeRoundAndPack:
    --zExp;
    return normalizeRoundAndPackFloat32(roundData, zSign, zExp, zSig);
}

sw_float float32_add(struct roundingData *roundData, sw_float a, sw_float b)
{
    flag aSign, bSign;

    aSign = extractFloat32Sign(a);
    bSign = extractFloat32Sign(b);
    if (aSign == bSign) {
        return addFloat32Sigs(roundData, a, b, aSign);
    } else {
        return subFloat32Sigs(roundData, a, b, aSign);
    }
}

/*
 * -------------------------------------------------------------------------------
 * Returns the result of subtracting the single-precision floating-point values
 * `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
 * for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
sw_float float32_sub(struct roundingData *roundData, sw_float a, sw_float b)
{
    flag aSign, bSign;

    aSign = extractFloat32Sign(a);
    bSign = extractFloat32Sign(b);
    if (aSign == bSign) {
        return subFloat32Sigs(roundData, a, b, aSign);
    } else {
        return addFloat32Sigs(roundData, a, b, aSign);
    }
}

/*
 * -------------------------------------------------------------------------------
 * Returns the result of multiplying the single-precision floating-point values
 * `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
 * for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
sw_float float32_mul(struct roundingData *roundData, sw_float a, sw_float b)
{
    flag aSign, bSign, zSign;
    int16_t aExp, bExp, zExp;
    bits32 aSig, bSig;
    bits64 zSig64;
    bits32 zSig;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    aSign = extractFloat32Sign(a);
    bSig = extractFloat32Frac(b);
    bExp = extractFloat32Exp(b);
    bSign = extractFloat32Sign(b);
    zSign = aSign ^ bSign;
    if (aExp == 0xFF) {
        if (aSig || ((bExp == 0xFF) && bSig)) {
            return propagateFloat32NaN(a, b);
        }
        if ((bExp | bSig) == 0) {
            roundData->exception |= float_flag_invalid;
            float_raise(float_flag_invalid);
            return is_Negative_NaN;
        }
        return packFloat32(zSign, 0xFF, 0);
    }
    if (bExp == 0xFF) {
        if (bSig)
            return propagateFloat32NaN(a, b);
        if ((aExp | aSig) == 0) {
            roundData->exception |= float_flag_invalid;
            float_raise(float_flag_invalid);
            return is_Negative_NaN;
        }
        return packFloat32(zSign, 0xFF, 0);
    }
    if (aExp == 0) {
        if (aSig == 0)
            return packFloat32(zSign, 0, 0);
        normalizeFloat32Subnormal(aSig, &aExp, &aSig);
    }
    if (bExp == 0) {
        if (bSig == 0)
            return packFloat32(zSign, 0, 0);
        normalizeFloat32Subnormal(bSig, &bExp, &bSig);
    }
    zExp = aExp + bExp - 0x7F;
    aSig = (aSig | 0x00800000) << 7;
    bSig = (bSig | 0x00800000) << 8;
    shift64RightJamming(((bits64)aSig) * bSig, 32, &zSig64);
    zSig = zSig64;
    if (0 <= (sbits32)(zSig << 1)) {
        zSig <<= 1;
        --zExp;
    }
    return roundAndPackFloat32(roundData, zSign, zExp, zSig);
}

static uint32_t do_div(bits64 *n, bits32 base)
{
    uint64_t rem = *n;
    uint64_t b = base;
    uint64_t res, d = 1;
    uint32_t high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;
    if (high >= base) {
        high /= base;
        res = (uint64_t)high << 32;
        rem -= (uint64_t)(high * base) << 32;
    }

    while ((int64_t)b > 0 && b < rem) {
        b = b + b;
        d = d + d;
    }

    do {
        if (rem >= b) {
            rem -= b;
            res += d;
        }
        b >>= 1;
        d >>= 1;
    } while (d);

    *n = res;
    return rem;
}

/*
 * -------------------------------------------------------------------------------
 * Returns the result of dividing the single-precision floating-point value `a'
 * by the corresponding value `b'.  The operation is performed according to the
 * IEC/IEEE Standard for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
sw_float float32_div(struct roundingData *roundData, sw_float a, sw_float b)
{
    flag aSign, bSign, zSign;
    int16_t aExp, bExp, zExp;
    bits32 aSig, bSig, zSig;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    aSign = extractFloat32Sign(a);
    bSig = extractFloat32Frac(b);
    bExp = extractFloat32Exp(b);
    bSign = extractFloat32Sign(b);
    zSign = aSign ^ bSign;
    if (aExp == 0xFF) {
        if (aSig)
            return propagateFloat32NaN(a, b);
        if (bExp == 0xFF) {
            if (bSig)
                return propagateFloat32NaN(a, b);
            roundData->exception |= float_flag_invalid;
            float_raise(float_flag_invalid);
            return is_Negative_NaN;
        }
        return packFloat32(zSign, 0xFF, 0);
    }
    if (bExp == 0xFF) {
        if (bSig)
            return propagateFloat32NaN(a, b);
        return packFloat32(zSign, 0, 0);
    }
    if (bExp == 0) {
        if (bSig == 0) {
            if ((aExp | aSig) == 0) {
                roundData->exception |= float_flag_invalid;
                float_raise(float_flag_invalid);
                return is_Negative_NaN;
            }
            roundData->exception |= float_flag_divbyzero;
            float_raise(float_flag_divbyzero);
            return packFloat32(zSign, 0xFF, 0);
        }
        normalizeFloat32Subnormal(bSig, &bExp, &bSig);
    }
    if (aExp == 0) {
        if (aSig == 0)
            return packFloat32(zSign, 0, 0);
        normalizeFloat32Subnormal(aSig, &aExp, &aSig);
    }
    zExp = aExp - bExp + 0x7D;
    aSig = (aSig | 0x00800000) << 7;
    bSig = (bSig | 0x00800000) << 8;
    if (bSig <= (aSig + aSig)) {
        aSig >>= 1;
        ++zExp;
    }
    {
        bits64 tmp = ((bits64)aSig) << 32;
        do_div(&tmp, bSig);
        zSig = tmp;
    }
    if ((zSig & 0x3F) == 0) {
        zSig |= (((bits64)bSig) * zSig != ((bits64)aSig) << 32);
    }
    return roundAndPackFloat32(roundData, zSign, zExp, zSig);
}

sw_float float32_abs(sw_float x)
{
    sw_float x_abs = 0x7fffffff & (x);
    return x_abs;
}

/*
 * -------------------------------------------------------------------------------
 * Returns 1 if the single-precision floating-point value `a' is equal to the
 * corresponding value `b', and 0 otherwise.  The comparison is performed
 * according to the IEC/IEEE Standard for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
uint32_t float32_eq(sw_float a, sw_float b)
{

    if (((extractFloat32Exp(a) == 0xFF) && extractFloat32Frac(a)) ||
        ((extractFloat32Exp(b) == 0xFF) && extractFloat32Frac(b))) {
        if (float32_is_signaling_nan(a) || float32_is_signaling_nan(b)) {
            float_raise(float_flag_invalid);
        }
        return 0;
    }
    return (a == b) || ((bits32)((a | b) << 1) == 0);
}

/*
 * -------------------------------------------------------------------------------
 * Returns 1 if the single-precision floating-point value `a' is less than or
 * equal to the corresponding value `b', and 0 otherwise.  The comparison is
 * performed according to the IEC/IEEE Standard for Binary Floating-point
 * Arithmetic.
 * -------------------------------------------------------------------------------
 */
uint32_t float32_le(sw_float a, sw_float b)
{
    flag aSign, bSign;

    if (((extractFloat32Exp(a) == 0xFF) && extractFloat32Frac(a)) ||
        ((extractFloat32Exp(b) == 0xFF) && extractFloat32Frac(b))) {
        float_raise(float_flag_invalid);
        return 0;
    }
    aSign = extractFloat32Sign(a);
    bSign = extractFloat32Sign(b);
    if (aSign != bSign)
        return aSign || ((bits32)((a | b) << 1) == 0);
    return (a == b) || (aSign ^ (a < b));
}

/*
 * -------------------------------------------------------------------------------
 * Returns 1 if the single-precision floating-point value `a' is less than
 * the corresponding value `b', and 0 otherwise.  The comparison is performed
 * according to the IEC/IEEE Standard for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
uint32_t float32_lt(sw_float a, sw_float b)
{
    flag aSign, bSign;

    if (((extractFloat32Exp(a) == 0xFF) && extractFloat32Frac(a)) ||
        ((extractFloat32Exp(b) == 0xFF) && extractFloat32Frac(b))) {
        float_raise(float_flag_invalid);
        return 0;
    }
    aSign = extractFloat32Sign(a);
    bSign = extractFloat32Sign(b);
    if (aSign != bSign)
        return aSign && ((bits32)((a | b) << 1) != 0);
    return (a != b) && (aSign ^ (a < b));
}

/*
 * -------------------------------------------------------------------------------
 * Returns the result of converting the single-precision floating-point value
 * `a' to the 32-bit two's complement integer format.  The conversion is
 * performed according to the IEC/IEEE Standard for Binary Floating-point
 * Arithmetic---which means in particular that the conversion is rounded
 * according to the current rounding mode.  If `a' is a NaN, the largest
 * positive integer is returned.  Otherwise, if the conversion overflows, the
 * largest integer with the same sign as `a' is returned.
 * -------------------------------------------------------------------------------
 */
int32_t float32_to_int32(struct roundingData *roundData, sw_float a)
{
    flag aSign;
    int16_t aExp, shiftCount;
    bits32 aSig;
    bits64 zSig;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    aSign = extractFloat32Sign(a);
    if ((aExp == 0x7FF) && aSig)
        aSign = 0;
    if (aExp)
        aSig |= 0x00800000;
    shiftCount = 0xAF - aExp;
    zSig = aSig;
    zSig <<= 32;
    if (shiftCount > 0)
        shift64RightJamming(zSig, shiftCount, &zSig);
    return roundAndPackInt32(roundData, aSign, zSig);
}

int32_t float32_to_frac_int(struct roundingData *roundData, sw_float a, int frac_bit)
{
    flag aSign;
    int16_t aExp, shiftCount;
    bits32 aSig;
    bits64 zSig;
    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a) + frac_bit;
    aSign = extractFloat32Sign(a);
    if ((aExp == 0x7FF) && aSig)
        aSign = 0;
    if (aExp)
        aSig |= 0x00800000;
    shiftCount = 0xAF - aExp;
    zSig = aSig;
    zSig <<= 32;
    if (shiftCount > 0)
        shift64RightJamming(zSig, shiftCount, &zSig);
    return roundAndPackInt32(roundData, aSign, zSig);
}
/*
 * -------------------------------------------------------------------------------
 * Returns the result of converting the single-precision floating-point value
 * `a' to the 32-bit two's complement integer format.  The conversion is
 * performed according to the IEC/IEEE Standard for Binary Floating-point
 * Arithmetic, except that the conversion is always rounded toward zero.  If
 * `a' is a NaN, the largest positive integer is returned.  Otherwise, if the
 * conversion overflows, the largest integer with the same sign as `a' is
 * returned.
 * -------------------------------------------------------------------------------
 */
int32_t float32_to_int32_round_to_zero(sw_float a)
{
    flag aSign;
    int16_t aExp, shiftCount;
    bits32 aSig;
    int32_t z;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    aSign = extractFloat32Sign(a);
    shiftCount = aExp - 0x9E;
    if (shiftCount >= 0) {
        if (a == 0xCF000000)
            return 0x80000000;
        float_raise(float_flag_invalid);
        if (!aSign || ((aExp == 0xFF) && aSig))
            return 0x7FFFFFFF;
        return 0x80000000;
    } else if (aExp <= 0x7E) {
        if (aExp | aSig)
            float_raise(float_flag_inexact);
        return 0;
    }
    aSig = (aSig | 0x00800000) << 8;
    z = aSig >> (-shiftCount);
    if ((bits32)(aSig << (shiftCount & 31))) {
        float_raise(float_flag_inexact);
    }
    return aSign ? -z : z;
}

/*
 * -------------------------------------------------------------------------------
 * Rounds the single-precision floating-point value `a' to an integer, and
 * returns the result as a single-precision floating-point value.  The
 * operation is performed according to the IEC/IEEE Standard for Binary
 * Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
sw_float float32_round_to_int(struct roundingData *roundData, sw_float a)
{
    flag aSign;
    int16_t aExp;
    bits32 lastBitMask, roundBitsMask;
    int8_t roundingMode;
    sw_float z;

    aExp = extractFloat32Exp(a);
    if (aExp >= 0x96) {
        if ((aExp == 0xFF) && extractFloat32Frac(a)) {
            return propagateFloat32NaN(a, a);
        }
        return a;
    }
    roundingMode = roundData->mode;
    if (aExp <= 0x7E) {
        if ((bits32)(a << 1) == 0)
            return a;
        roundData->exception |= float_flag_inexact;
        float_raise(float_flag_inexact);
        aSign = extractFloat32Sign(a);
        switch (roundingMode) {
        case float_round_nearest_even:
            if ((aExp == 0x7E) && extractFloat32Frac(a)) {
                return packFloat32(aSign, 0x7F, 0);
            }
            break;
        case float_round_down:
            return aSign ? 0xBF800000 : 0;
        case float_round_up:
            return aSign ? 0x80000000 : 0x3F800000;
        }
        return packFloat32(aSign, 0, 0);
    }
    lastBitMask = 1;
    lastBitMask <<= 0x96 - aExp;
    roundBitsMask = lastBitMask - 1;
    z = a;
    if (roundingMode == float_round_nearest_even) {
        z += lastBitMask >> 1;
        if ((z & roundBitsMask) == 0)
            z &= ~lastBitMask;
    } else if (roundingMode != float_round_to_zero) {
        if (extractFloat32Sign(z) ^ (roundingMode == float_round_up)) {
            z += roundBitsMask;
        }
    }
    z &= ~roundBitsMask;
    if (z != a) {
        roundData->exception |= float_flag_inexact;
        float_raise(float_flag_inexact);
    }
    return z;
}

void float32_extract_int_and_float(sw_float a, sw_float *int_a, sw_float *float_a)
{
    struct roundingData roundData;
    roundData.mode = float_round_to_zero;
    *int_a = float32_round_to_int(&roundData, a);
    *float_a = float32_sub(&roundData, a, *int_a);
}

/*
 * -------------------------------------------------------------------------------
 * Returns the result of converting the 32-bit two's complement integer `a' to
 * the single-precision floating-point format.  The conversion is performed
 * according to the IEC/IEEE Standard for Binary Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
sw_float int32_to_float32(struct roundingData *roundData, int32_t a)
{
    flag zSign;

    if (a == 0)
        return 0;
    if (a == (int32_t) 0x80000000)
        return packFloat32(1, 0x9E, 0);
    zSign = (a < 0);
    return normalizeRoundAndPackFloat32(roundData, zSign, 0x9C, zSign ? -a : a);
}

sw_float frac_int_to_float32(struct roundingData *roundData, int32_t a, int32_t frac_bit)
{
    flag zSign;
    if (a == 0)
        return 0;
    if (a == (int32_t) 0x80000000)
        return packFloat32(1, 0x9E, 0);
    zSign = (a < 0);
    return normalizeRoundAndPackFloat32(roundData, zSign, 0x9C - frac_bit, zSign ? -a : a);
}
/*
 * -------------------------------------------------------------------------------
 * Returns an approximation to the square root of the 32-bit significand given
 * by `a'.  Considered as an integer, `a' must be at least 2^31.  If bit 0 of
 * `aExp' (the least significant bit) is 1, the integer returned approximates
 * 2^31*sqrt(`a'/2^31), where `a' is considered an integer.  If bit 0 of `aExp'
 * is 0, the integer returned approximates 2^31*sqrt(`a'/2^30).  In either
 * case, the approximation returned lies strictly within +/-2 of the exact
 * value.
 * -------------------------------------------------------------------------------
 */
static bits32 estimateSqrt32(int16_t aExp, bits32 a)
{
    static const bits16 sqrtOddAdjustments[] = {
        0x0004, 0x0022, 0x005D, 0x00B1, 0x011D, 0x019F, 0x0236, 0x02E0,
        0x039C, 0x0468, 0x0545, 0x0631, 0x072B, 0x0832, 0x0946, 0x0A67};
    static const bits16 sqrtEvenAdjustments[] = {
        0x0A2D, 0x08AF, 0x075A, 0x0629, 0x051A, 0x0429, 0x0356, 0x029E,
        0x0200, 0x0179, 0x0109, 0x00AF, 0x0068, 0x0034, 0x0012, 0x0002};
    int8_t index;
    bits32 z;
    bits64 A;

    index = (a >> 27) & 15;
    if (aExp & 1) {
        z = 0x4000 + (a >> 17) - sqrtOddAdjustments[index];
        z = ((a / z) << 14) + (z << 15);
        a >>= 1;
    } else {
        z = 0x8000 + (a >> 17) - sqrtEvenAdjustments[index];
        z = a / z + z;
        z = (z >= 0x20000) ? 0xFFFF8000 : (z << 15);
        if (z <= a)
            return (bits32)(((sbits32)a) >> 1);
    }
    A = ((bits64)a) << 31;
    do_div(&A, z);
    return ((bits32)A) + (z >> 1);
}

/*
 * -------------------------------------------------------------------------------
 * Returns the square root of the single-precision floating-point value `a'.
 * The operation is performed according to the IEC/IEEE Standard for Binary
 * Floating-point Arithmetic.
 * -------------------------------------------------------------------------------
 */
sw_float float32_sqrt(struct roundingData *roundData, sw_float a)
{
    flag aSign;
    int16_t aExp, zExp;
    bits32 aSig, zSig;
    bits64 rem, term;

    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    aSign = extractFloat32Sign(a);
    if (aExp == 0xFF) {
        if (aSig)
            return propagateFloat32NaN(a, 0);
        if (!aSign)
            return a;
        roundData->exception |= float_flag_invalid;
        float_raise(float_flag_invalid);
        return is_Negative_NaN;
    }
    if (aSign) {
        if ((aExp | aSig) == 0)
            return a;
        roundData->exception |= float_flag_invalid;
        float_raise(float_flag_invalid);
        return is_Negative_NaN;
    }
    if (aExp == 0) {
        if (aSig == 0)
            return 0;
        normalizeFloat32Subnormal(aSig, &aExp, &aSig);
    }
    zExp = ((aExp - 0x7F) >> 1) + 0x7E;
    aSig = (aSig | 0x00800000) << 8;
    zSig = estimateSqrt32(aExp, aSig) + 2;
    if ((zSig & 0x7F) <= 5) {
        if (zSig < 2) {
            zSig = 0xFFFFFFFF;
        } else {
            aSig >>= aExp & 1;
            term = ((bits64)zSig) * zSig;
            rem = (((bits64)aSig) << 32) - term;
            while ((sbits64)rem < 0) {
                --zSig;
                rem += (((bits64)zSig) << 1) | 1;
            }
            zSig |= (rem != 0);
        }
    }
    shift32RightJamming(zSig, 1, &zSig);
    return roundAndPackFloat32(roundData, 0, zExp, zSig);
}

sw_float float32_frexp(sw_float a, int *pw2)
{
    uint32_t f = (uint32_t)a;
    int i = (((uint32_t)a) >> 23) & 0x000000ff;

    i -= 0x7e;
    *pw2 = i;
    f &= 0x807fffff; /* strip all exponent bits */
    f |= 0x3f000000; /* mantissa between 0.5 and 1 */
    return ((sw_float)f);
}

static inline uint32_t top12(sw_float x)
{
    return ((uint32_t)x) >> 20;
}

sw_float float32_exp(sw_float x_32)
{
    struct roundingData roundData;
    uint32_t abstop;
    const uint32_t top12_88 = 0x42b;
    sw_float z, shift32, kd, r, r2, s, y;
    uint32_t ki, t;

    abstop = top12(x_32) & 0x7ff;

    roundData.mode = float_round_nearest_even;
    if (abstop >= top12_88) {
        const sw_float max_input = 0x42b17217;
        const sw_float min_input = 0xc2cff1b4;

        /* |x| >= 88 or x is nan.  */
        if (x_32 == is_Negative_INF)
            return 0;
        if (abstop >= top12(isINF))
            return  float32_add(&roundData, x_32, x_32);

        //if (x > 0x1.62e42ep6f) /* x > log(0x1p128) ~= 88.72 */
        if (!float32_lt(x_32, max_input)) {
            float_raise(float_flag_overflow);
            return isINF;
        }
        //if (x < -0x1.9fe368p6f) /* x < log(0x1p-150) ~= -103.97 */
        if (float32_lt(x_32, min_input)) {
            float_raise(float_flag_underflow);
            return is_Negative_INF;
        }
    }

    /* x*N/Ln2 = k + r with r in [-1/2, 1/2] and int k.  */
    z = float32_mul(&roundData, InvLn2N_f32, x_32);

    /* Round and convert z to int, the result is in [-150*N, 128*N] and
     * ideally nearest int is used, otherwise the magnitude of r can be
     * bigger which gives larger approximation error.
     */

    shift32 = 0x4b400000; // floatTofloat32(0x1.8p+23);
    kd = float32_add(&roundData, shift32, z);
    ki = kd;
    kd = float32_sub(&roundData, kd, shift32);
    r = float32_sub(&roundData, z, kd);


    r2 = float32_mul(&roundData, r, r);
    /* exp(x) = 2^(k/N) * 2^(r/N) ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
    t = expf_tab[ki % EXPF_N];
    t += ki << (23 - EXP2F_TABLE_BITS);
    s = t;
    z = float32_mul(&roundData, expf_poly_scaled[0], r);
    z = float32_add(&roundData, z, expf_poly_scaled[1]);
    y = float32_mul(&roundData, expf_poly_scaled[2], r);
    y = float32_add(&roundData, y, FLOAT32_ONE);
    y = float32_add(&roundData, y, float32_mul(&roundData, z, r2));
    y = float32_mul(&roundData, y, s);
    return y;
}

sw_float float32_exp2(sw_float xd, uint32_t sign_bias)
{
    struct roundingData roundData;
    sw_float shift32 = 0x48c00000; //(0x1.8p+23 / N);
    sw_float kd, r, s, z, r2, y;
    uint32_t ki;
    uint32_t t, ski;

    roundData.mode = float_round_nearest_even;
    kd = float32_add(&roundData, shift32, xd);
    ki = kd;
    kd = float32_sub(&roundData, kd, shift32);
    r = float32_sub(&roundData, xd, kd);

    /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
    t = expf_tab[ki % EXPF_N];
    ski = ki + sign_bias;
    t += ski << (23 - EXP2F_TABLE_BITS);
    s = t;
    z = float32_add(&roundData, float32_mul(&roundData, expf_poly[0], r), expf_poly[1]);
    r2 = float32_mul(&roundData, r, r);
    y = float32_add(&roundData, float32_mul(&roundData, expf_poly[2], r), FLOAT32_ONE);
    y = float32_add(&roundData, float32_mul(&roundData, z, r2), y);
    y = float32_mul(&roundData, y, s);
    return y;
}

sw_float float32_log2(uint32_t ix)
{
    struct roundingData roundData;
    /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
    // double_t z, r, r2, r4, p, q, y, y0, invc, logc;
    uint32_t iz, top, tmp;
    int k, i;
    sw_float invc, logc, z, r, y0, *A, r2, y, p, r4, q;

    roundData.mode = float_round_nearest_even;

    /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
     * The range is split into N subintervals.
     * The ith subinterval contains z and c is near its center.
     */
    tmp = ix - OFF;
    i = (tmp >> (23 - POWF_LOG2_TABLE_BITS)) % POW_N;
    top = tmp & 0xff800000;
    iz = ix - top;
    k = (int32_t)top >> (23 - POWF_SCALE_BITS); /* arithmetic shift */
    invc = powf_log2_data_invc[i];
    logc = powf_log2_data_logc[i];
    z = iz;

    /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k */
    r = float32_sub(&roundData, float32_mul(&roundData, invc, z), FLOAT32_ONE);
    y0 = float32_add(&roundData, logc, int32_to_float32(&roundData, k));

    // y0 = logc + (double_t) k;

    /* Pipelined polynomial evaluation to approximate log1p(r)/ln2.  */
    A = powf_log2_data_poly;
    r2 = float32_mul(&roundData, r, r);
    y = float32_add(&roundData, float32_mul(&roundData, A[0], r), A[1]);
    p = float32_add(&roundData, float32_mul(&roundData, A[2], r), A[3]);
    r4 = float32_mul(&roundData, r2, r2);
    q = float32_add(&roundData, float32_mul(&roundData, A[4], r), y0);
    q = float32_add(&roundData, float32_mul(&roundData, p, r2), q);
    y = float32_add(&roundData, float32_mul(&roundData, y, r4), q);

    return y;
}

sw_float float32_pow(sw_float x_32, sw_float y_32)
{
    uint32_t sign_bias = 0;
    uint32_t ix, iy;
    struct roundingData roundData;
    sw_float logx, ylogx;

    roundData.mode = float_round_nearest_even;

    ix = x_32;
    iy = y_32;

    if ((ix - 0x00800000 >= 0x7f800000 - 0x00800000 || zeroinfnan(iy))) {
        /* Either (x < 0x1p-126 or inf or nan) or (y is 0 or inf or nan).  */
        if (zeroinfnan(iy)) {
            if (2 * iy == 0)    //y = 0
                return FLOAT32_ONE;
            if (ix == 0x3f800000)   //x = 1
                return FLOAT32_ONE;
            if (2 * ix > 2u * 0x7f800000 || 2 * iy > 2u * 0x7f800000)
                return float32_add(&roundData, x_32, y_32);
            if (2 * ix == 2 * 0x3f800000)  // +-1
                return FLOAT32_ONE;
            if ((2 * ix < 2 * 0x3f800000) == !(iy & 0x80000000))
                return 0; /* |x|<1 && y==inf or |x|>1 && y==-inf.  */
            return float32_mul(&roundData, y_32, y_32);
        }
        if (zeroinfnan(ix)) {
            sw_float x2 = float32_mul(&roundData, x_32, x_32);
            if (ix & 0x80000000 && checkint(iy) == 1) {
                float32_mul(&roundData, x2, FLOAT32_Negative_ONE);
                sign_bias = 1;
            }
            /* Without the barrier some versions of clang hoist the 1/x2 and
             * thus division by zero exception can be signaled spuriously.
             */
            return iy & 0x80000000 ? float32_div(&roundData, FLOAT32_ONE, x2) : x2;
        }
        /* x and y are non-zero finite.  */
        if (ix & 0x80000000) {
            /* Finite x < 0.  */
            int yint = checkint(iy);
            if (yint == 0) {
                float_raise(float_flag_invalid);
                return isNaN; // return __math_invalidf (x);
            }
            if (yint == 1)
                sign_bias = SIGN_BIAS;
            ix &= 0x7fffffff;
        }
        if (ix < 0x00800000) {
            /* Normalize subnormal x so exponent becomes negative.  */
            ix = float32_mul(&roundData, x_32, 0x4b000000 /*floatTofloat32(0x1p23f)*/);
            // ix = asuint (x * 0x1p23f);
            ix &= 0x7fffffff;
            ix -= 23 << 23;
        }
    }

    logx = float32_log2(ix);
    ylogx = float32_mul(&roundData, y_32, logx);
    if (!float32_lt(float32_abs(ylogx), 0x42fc0000 /* 126 */)) {
        if (extractFloat32Sign(ylogx))
            return is_Negative_INF;
        else
            return isINF;
#if 0
        /* |y*log(x)| >= 126.  */
        if (ylogx > 0x1.fffffffd1d571p+6 * POWF_SCALE)
            /* |x^y| > 0x1.ffffffp127.  */
            return __math_oflowf(sign_bias);
        if (WANT_ROUNDING && WANT_ERRNO && ylogx > 0x1.fffffffa3aae2p+6 * POWF_SCALE)
            /* |x^y| > 0x1.fffffep127, check if we round away from 0.  */
            if ((!sign_bias && eval_as_float(1.0f + opt_barrier_float(0x1p-25f)) != 1.0f) ||
                (sign_bias && eval_as_float(-1.0f - opt_barrier_float(0x1p-25f)) != -1.0f))
                return __math_oflowf(sign_bias);
        if (ylogx <= -150.0 * POWF_SCALE)
            return __math_uflowf(sign_bias);
#if WANT_ERRNO_UFLOW
        if (ylogx < -149.0 * POWF_SCALE)
            return __math_may_uflowf(sign_bias);
#endif
#endif
    }
    return float32_exp2(ylogx, sign_bias);
}

sw_float float32_log(sw_float x_32)
{
    struct roundingData roundData;

    /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
    uint32_t ix, iz, tmp;
    int k, i;
    sw_float z, r, r2, y0, y;

    roundData.mode = float_round_nearest_even;
    ix = x_32;
    if (ix - 0x00800000 >= 0x7f800000 - 0x00800000) {
        /* x < 0x1p-126 or inf or nan.  */
        if (ix * 2 == 0) {
            float_raise(float_flag_divbyzero);
            return isNaN;
        }
        if (ix == 0x7f800000) /* log(inf) == inf.  */
            return isINF;
        if ((ix & 0x80000000) || ix * 2 >= 0xff000000) {
            float_raise(float_flag_invalid);
            return isNaN;
        }
        /* x is subnormal, normalize it.  */
        ix = float32_mul(&roundData, x_32, 0x4b000000); //ix = asuint(x * 0x1p23f);
        ix -= 23 << 23;
    }

    /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
     * The range is split into N subintervals.
     * The ith subinterval contains z and c is near its center.
     */
    tmp = ix - OFF;
    i = (tmp >> (23 - LOGF_TABLE_BITS)) % LOGF_N;
    k = (int32_t)tmp >> 23; /* arithmetic shift */
    iz = ix - (tmp & 0x1ff << 23);
    z = iz;

    r = float32_mul(&roundData, z, logf_incv_table[i]);
    r = float32_sub(&roundData, r, FLOAT32_ONE);
    r2 = float32_mul(&roundData, r, r);
    y0 = float32_mul(&roundData, int32_to_float32(&roundData, k), logf_ln2);
    y0 = float32_add(&roundData, logf_logc_table[i], y0);

    y = float32_add(&roundData, float32_mul(&roundData, r, logf_poly_table[1]), logf_poly_table[2]);
    y = float32_add(&roundData, float32_mul(&roundData, r2, logf_poly_table[0]), y);
    y = float32_mul(&roundData, y, r2);
    y = float32_add(&roundData, y, y0);
    y = float32_add(&roundData, y, r);
    return y;
}
