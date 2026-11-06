#include "float_point.h"

float32 FRAC_INT_TO_FLOAT(int32_t a, int frac_bit)
{
#if SOFT_FLOAT
    struct roundingData roundData;
    float32 out;

    if (a <= 0)
        return 0;
    roundData.mode = float_round_nearest_even;
    out = frac_int_to_float32(&roundData, a, frac_bit);

    return out;
#else
    return ((float32)a / (float32)(1 << frac_bit));
#endif
}

int32_t FLOAT_TO_FRAC_INT(float32 a, int frac_bit)
{
#if SOFT_FLOAT
    struct roundingData roundData;

    roundData.mode = float_round_nearest_even;
    return float32_to_frac_int(&roundData, a, frac_bit);
#else
    return (int32_t)(a*((int32_t)1 << frac_bit));
#endif
}

float32 INT_TO_FLOAT(int32_t a)
{
#if SOFT_FLOAT
    struct roundingData roundData;
    roundData.mode = float_round_nearest_even;
    return int32_to_float32(&roundData, a);
#else
    return (float32)a;
#endif
}

int32_t FLOAT_TO_INT(float32 a)
{
#if SOFT_FLOAT
    struct roundingData roundData;
    roundData.mode = float_round_nearest_even;
    return float32_to_int32(&roundData, a);
#else
    return (int32_t)(a + 0.5);
#endif
}

float32 FLOAT_ADD(float32 a, float32 b)
{
    float32 c;
#if SOFT_FLOAT
    struct roundingData roundData;

    roundData.mode = float_round_nearest_even;
    c = float32_add(&roundData, a, b);
#else
    c = a + b;
#endif

    return c;
}

float32 FLOAT_SUB(float32 a, float32 b)
{
    float32 c;
#if SOFT_FLOAT
    struct roundingData roundData;

    roundData.mode = float_round_nearest_even;
    c = float32_sub(&roundData, a, b);
#else
    c = a - b;
#endif

    return c;
}

float32 FLOAT_MUL(float32 a, float32 b)
{
    float32 c;
#if SOFT_FLOAT
    struct roundingData roundData;
    roundData.mode = float_round_nearest_even;
    c = float32_mul(&roundData, a, b);
#else
    c = a * b;
#endif

    return c;
}

float32 FLOAT_EXP(float32 a)
{
    float32 c;

#if SOFT_FLOAT
    c = float32_exp(a);
#else
    c = expf(a);
#endif

    return c;
}

float32 FLOAT_LOG(float32 a)
{
    float32 c;

#if SOFT_FLOAT
    c = float32_log(a);
#else
    c = logf(a);
#endif

    return c;
}

float32 FLOAT_POW(float32 a, float32 b)
{
    float32 c;

#if SOFT_FLOAT
    c = float32_pow(a, b);
#else
    c = powf(a, b);
#endif

    return c;
}

float32 FLOAT_DIV(float32 a, float32 b)
{
    float32 c;
#if SOFT_FLOAT
    struct roundingData roundData;

    roundData.mode = float_round_nearest_even;
    c = float32_div(&roundData, a, b);
#else
    c = a / b;
#endif

    return c;
}

uint32_t FLOAT_EQ(float32 a, float32 b)
{
#if SOFT_FLOAT
    return float32_eq(a, b);
#else
    return a == b;
#endif
}

uint32_t FLOAT_LE(float32 a, float32 b)
{
#if SOFT_FLOAT
    return float32_le(a, b);
#else
    return a <= b;
#endif
}

uint32_t FLOAT_LT(float32 a, float32 b)
{
#if SOFT_FLOAT
    return float32_lt(a, b);
#else
    return a < b;
#endif
}

uint32_t FLOAT_GT(float32 a, float32 b)
{
#if SOFT_FLOAT
    return float32_le(b, a);
#else
    return a > b;
#endif
}

uint32_t FLOAT_GE(float32 a, float32 b)
{
#if SOFT_FLOAT
    return float32_lt(b, a);
#else
    return a >= b;
#endif
}

float32 FLOAT_MIN(float32 a, float32 b)
{
    return (FLOAT_LT(a, b)) ? a : b;
}
float32 FLOAT_MAX(float32 a, float32 b)
{
    return (FLOAT_GT(a, b)) ? a : b;
}
float32 FLOAT_CLIP(float32 low, float32 high, float32 a)
{
    return FLOAT_MIN((FLOAT_MAX(low, a)), high);
}
