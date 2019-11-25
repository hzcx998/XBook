/*
 * file:		   include/share/stdint.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_STDINT_H
#define _SHARE_STDINT_H

typedef unsigned long   uint64_t;
typedef signed long     int64_t;
typedef unsigned int    uint32_t;
typedef signed int      int32_t;
typedef unsigned short  uint16_t;
typedef signed short    int16_t;
typedef unsigned char   uint8_t;
typedef signed char     int8_t;

typedef unsigned long   uint64;
typedef signed long     int64;
typedef unsigned int    uint32;
typedef signed int      int32;
typedef unsigned short  uint16;
typedef signed short    int16;
typedef unsigned char   uint8;
typedef signed char     int8;

typedef unsigned long   u64;
typedef signed long     s64;
typedef unsigned int    u32;
typedef signed int      s32;
typedef unsigned short  u16;
typedef signed short    s16;
typedef unsigned char   u8;
typedef signed char     s8;

typedef unsigned long   QWORD;
typedef unsigned int    DWORD;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;

typedef unsigned int    UINT;
typedef unsigned short  WCHAR;

typedef DWORD           FSIZE_t;
typedef DWORD           LBA_t;

#define UINT32_MAX 0xffffffff
#define UINT16_MAX 0xffff
#define UINT8_MAX 0xff

#endif  /*_SHARE_STDINT_H*/
