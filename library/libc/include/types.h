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

/* 共有，可以被整个内核使用 */
#ifndef PUBLIC
#define PUBLIC 
#endif

/* 私有，只能被本文件使用 */
#ifndef PRIVATE 
#define PRIVATE static
#endif

/* 扩展，如果是PUBLIC的，扩展后，引用该扩展的文件可以使用 */
#ifndef EXTERN
#define EXTERN extern
#endif

/* 保护，同类可以扩展，其它的不可以扩展。即不在头文件中EXTERN,而是在C文件中EXTERN */
#ifndef PROTECT
#define PROTECT 
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
#define PACKED __attribute__((packed))
#endif

#ifndef CONST
#define CONST const
#endif

#endif  /*_LIB_TYPES_H*/
