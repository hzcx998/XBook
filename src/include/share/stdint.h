/*
 * file:		   include/share/stdint.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_STDINT_H
#define _SHARE_STDINT_H

typedef unsigned long  uint64_t;
typedef          long  int64_t;
typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

#define UINT32_MAX 0xffffffff
#define UINT16_MAX 0xffff
#define UINT8_MAX 0xff

#endif  /*_SHARE_STDINT_H*/
