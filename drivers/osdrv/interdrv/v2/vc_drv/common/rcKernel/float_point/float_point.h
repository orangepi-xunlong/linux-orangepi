
#ifndef __FLOAT_POINT__
#define __FLOAT_POINT__

#include <linux/types.h>

#define SOFT_FLOAT 1

#if SOFT_FLOAT
#include "soft_float.h"
typedef sw_float float32;
#else
#include <math.h>
typedef float float32;
#endif

float32 FRAC_INT_TO_FLOAT(int32_t a, int frac_bit);
int32_t FLOAT_TO_FRAC_INT(float32 a, int frac_bit);

float32 INT_TO_FLOAT(int32_t a);
int32_t FLOAT_TO_INT(float32 a);

float32 FLOAT_ADD(float32 a, float32 b);
float32 FLOAT_SUB(float32 a, float32 b);
float32 FLOAT_MUL(float32 a, float32 b);
float32 FLOAT_EXP(float32 a);
float32 FLOAT_LOG(float32 a);
float32 FLOAT_POW(float32 a, float32 b);
float32 FLOAT_DIV(float32 a, float32 b);

uint32_t FLOAT_EQ(float32 a, float32 b);
uint32_t FLOAT_LT(float32 a, float32 b);
uint32_t FLOAT_LE(float32 a, float32 b);
uint32_t FLOAT_GT(float32 a, float32 b);
uint32_t FLOAT_GE(float32 a, float32 b);

float32 FLOAT_MIN(float32 a, float32 b);
float32 FLOAT_MAX(float32 a, float32 b);
float32 FLOAT_CLIP(float32 low, float32 high, float32 a);
#endif
