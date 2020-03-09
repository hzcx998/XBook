/*
 * file:		include/lib/limits.h
 * auther:		Jason Hu
 * time:		2020/2/13
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_LIMITS_H
#define _LIB_LIMITS_H
#define _YVALS
#ifndef _YVALS
#include "yvals.h"
#endif

#define _C2 1

#define _CSIGN 1
#define _ILONG 1
/**char properties*/
#define CHAR_BIT
#if _CSIGN
    #define CHAR_MAX    127
    #define CHAR_MIN    (-127 - _C2) /**由my_yvals.h可知_C2为1*/
#else
    #define CHAR_MAX    255
    #define CHAR_MIN    0
#endif // _CSIGN
#if _ILONG
    #define INT_MAX     2147483647
    #define INT_MIN     (-2147483647 - _C2)
    #define UINT_MAX    4294967295
#else
    #define INT_MAX     32767
    #define INT_MIN     (-32767 - _C2)
    #define UINT_MAX    65535
#endif // _ILONG
#define LONG_MAX        2147483647
#define LONG_MIN        (-2147483647 - _C2)
#define MB_LEN_MAX      _MBMAX
#define SCHAR_MAX       127
#define SCHAR_MIN       (-127 - _C2)
#define SHRT_MAX        32767
#define SHRT_MIN        (-32767 - _C2)
#define UCHAR_MAX       255
#define ULONG_MAX       4294967295
#define USHRT_MAX       65535

#endif /* _LIB_LIMITS_H */
