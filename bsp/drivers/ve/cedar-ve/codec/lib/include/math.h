/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef __MATH_H__
#define __MATH_H__
//normal use interface
double fabs(double x);
float fabsf(float x);
long double fabsl(long double x);
float sqrtf(float x);
double sqrt(double x);
double exp(double x);
float expf(float x);
double pow(double x, double y);
float powf(float x, float y);
double log(double x);
double log10(double x);
double log2(double x);
float logf(float x);

double sin(double x);
double cos(double x);
float sinf(float x);
float cosf(float x);
double atan(double x);
float atanf(float x);
double atan2(double y, double x);
float atan2f(float y, float x);
void sincos(double x, double *sn, double *cs);

double tan(double x);
double acos(double x);
double asin(double x);
double ceil(double x);
float ceilf(float x);

#endif
