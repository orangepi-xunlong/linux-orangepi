
#ifndef __SOFTFLOAT__
#define __SOFTFLOAT__

#include <linux/types.h>

// exactly the same with IEEE 754 single-floating point binary representation
typedef unsigned int sw_float;

/*
 * -------------------------------------------------------------------------------
 * Software IEC/IEEE floating-point rounding mode.
 * -------------------------------------------------------------------------------
 */
//extern int8 float_rounding_mode;
enum {
    float_round_nearest_even = 0,
    float_round_to_zero      = 1,
    float_round_down         = 2,
    float_round_up           = 3
};

enum {
    float_flag_invalid   =  1,
    float_flag_divbyzero =  2,
    float_flag_overflow  =  4,
    float_flag_underflow =  8,
    float_flag_inexact   = 16
};

enum {
    float_tininess_after_rounding  = 0,
    float_tininess_before_rounding = 1
};

struct roundingData {
    char mode;
    char precision;
    signed char exception;
};

sw_float float32_add(struct roundingData *roundData, sw_float a, sw_float b);
sw_float float32_sub(struct roundingData *roundData, sw_float a, sw_float b);
sw_float float32_mul(struct roundingData *roundData, sw_float a, sw_float b);
sw_float float32_div(struct roundingData *roundData, sw_float a, sw_float b);

uint32_t float32_eq(sw_float a, sw_float b);
uint32_t float32_le(sw_float a, sw_float b);
uint32_t float32_lt(sw_float a, sw_float b);

int32_t float32_to_int32(struct roundingData *roundData, sw_float a);
int32_t float32_to_frac_int(struct roundingData *roundData, sw_float a, int frac_bit);
int32_t float32_to_int32_round_to_zero(sw_float a);
sw_float float32_round_to_int(struct roundingData *roundData, sw_float a);
sw_float int32_to_float32(struct roundingData *roundData, int32_t a);
sw_float frac_int_to_float32(struct roundingData *roundData, int32_t a, int32_t frac_bit);
sw_float float32_abs(sw_float x);
sw_float float32_sqrt(struct roundingData *roundData, sw_float a);
sw_float float32_exp(sw_float x);
sw_float float32_log(sw_float x_32);
sw_float float32_pow(sw_float x_32, sw_float y_32);
#endif
