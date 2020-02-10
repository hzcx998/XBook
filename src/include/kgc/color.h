/*
 * file:		include/kgc/color.h
 * auther:		Jason Hu
 * time:		2020/2/5
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形绘制 */
#ifndef _KGC_COLOR_H
#define _KGC_COLOR_H

#include <lib/types.h>
#include <lib/stdint.h>
/* KGC-kernel graph core 内核图形核心 */

/* KGCC-kernel graph core color 内核图形核心颜色 */

/* 生成RGB颜色 */
#define KGCC_RGB(r, g, b)      (((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)) 
/* 生成ARGB颜色 */
#define KGCC_ARGB(a, r, g, b)  ((a & 0xff) << 24) | KGCC_RGB(r, g, b)

/* 获取颜色 */
#define KGCC_GET_ALPHA(c)  ((c >> 24) & 0xff)
#define KGCC_GET_RED(c)    ((c >> 16) & 0xff)
#define KGCC_GET_GREEN(c)  ((c >> 8) & 0xff)
#define KGCC_GET_BLUE(c)   (c & 0xff)

/* 常用颜色 */
#define KGCC_RED       KGCC_RGB(255, 0, 0)
#define KGCC_GREEN     KGCC_RGB(0, 255, 0)
#define KGCC_BLUE      KGCC_RGB(0, 0, 255)
#define KGCC_WHITE     KGCC_RGB(255, 255, 255)
#define KGCC_BLACK     KGCC_RGB(0, 0, 0)
#define KGCC_GRAY      KGCC_RGB(195, 195, 195)
#define KGCC_LEAD      KGCC_RGB(127, 127, 127)
#define KGCC_YELLOW    KGCC_RGB(255, 255, 127)

#endif   /* _KGC_COLOR_H */
