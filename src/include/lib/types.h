/*
 * file:		include/lib/types.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_TYPES_H
#define _LIB_TYPES_H

#ifndef NULL
#ifdef __cplusplus
        #define NULL 0
#else
        #define NULL ((void *)0)
#endif
#endif

#ifndef __cplusplus
//#define bool _Bool      //C语言下实现Bool
#define bool char      

#define true 1
#define false 0
#endif

#ifndef BOOLEAN
#ifndef __cplusplus
    #define BOOLEAN char     
#else
    #define BOOLEAN _Bool       //C语言下实现Bool
#endif

    #ifndef TRUE
    #define TRUE    1 
    #endif

    #ifndef FALSE
    #define FALSE    0 
    #endif

#endif


#ifndef PUBLIC
#define PUBLIC 
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef INIT
#define INIT
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef PACKED
#define PACKED __attribute__ ((packed))
#endif

#ifndef CONST
#define CONST const
#endif

#endif  /*_LIB_TYPES_H*/
