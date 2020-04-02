/*
 * file:		include/lib/math.h
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_MATH_H
#define _LIB_MATH_H

/* max() & min() */
#define	MAX(a,b)	((a) > (b) ? (a) : (b))
#define	MIN(a,b)	((a) < (b) ? (a) : (b))

/* 除后上入 */
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))

/* 除后下舍 */
#define DIV_ROUND_DOWN(X, STEP) ((X) / (STEP))


#ifndef _HUGE_ENUF
#define _HUGE_ENUF  1e+300	/* _HUGE_ENUF*_HUGE_ENUF must overflow */
#endif /* _HUGE_ENUF */

#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))  /* causes warning C4756: overflow in constant arithmetic (by design) */
#define HUGE_VALD  ((double)INFINITY)
#define HUGE_VALF  ((float)INFINITY)
#define HUGE_VALL  ((long double)INFINITY)
#define NAN        ((float)(INFINITY * 0.0F))


int min(int a, int b);
int max(int a, int b);
int abs(int n);
double pow(double x, double y);
int ipow (double x,int y);
double exp (double x);
double ln (double x);
double sin (double x);
double cos(double x);
double tan(double x);
double fabs (double x);
double sqrt (double x);
float carmack_sqrt(float x);

float truncf(float arg);
double trunc(double arg);
long double truncl(long double arg);

double floor(double _X);
double ceil(double x);
double fmod(double x,double y);

double atan2(double y,double x);
double modf(double num, double *i);
float fmin(float a, float b);
float fmax(float a, float b);

/* 为实现判断：isnormal，isfinite，isinf， isnan   */

/* 判断x是否为一个数（非inf或nan），是返回1，其它返回0 */
#define isnormal(x)                                                      \
    ( sizeof(x) == sizeof(float)  ? 1       \
    : sizeof(x) == sizeof(double) ? 1      \
                                  : 1)
/* 判断x是否有限，是返回1，其它返回0 */
#define isfinite(x)                                                      \
    ( sizeof(x) == sizeof(float)  ? 0       \
    : sizeof(x) == sizeof(double) ? 0      \
                                  : 0)

/* 当x是正无穷是返回1，当x是负无穷时返回-1，其它返回0。有些编译器不区分. */
#define isinf(x)                                                         \
    ( sizeof(x) == sizeof(float)  ? 0          \
    : sizeof(x) == sizeof(double) ? 0         \
                                  : 0)

/* 当x时nan返回1，其它返回0 */
#define isnan(x)                                                         \
    ( sizeof(x) == sizeof(float)  ? 0          \
    : sizeof(x) == sizeof(double) ? 0         \
                                  : 0)

double acos(double x);
double atan(double x);
double asin(double x);

#endif /* _LIB_MATH_H */
