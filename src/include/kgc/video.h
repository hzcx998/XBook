/*
 * file:		include/kgc/video.h
 * auther:		Jason Hu
 * time:		2020/2/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形视频 */
#ifndef _KGC_VIDEO_H
#define _KGC_VIDEO_H
/* KGC-kernel graph core 内核图形核心 */

#include <lib/types.h>
#include <lib/stdint.h>

/* 图形设备IO的命令 */
enum KGC_IoCommands {
    GRAPH_IO_BASE_PACKT = 1,    /* 获取图形基础信息包 */
};

/* 图形基础信息包 */
typedef struct KGC_VideoInfo {
    uint32_t xResolution;       /* 屏幕宽度 */
    uint32_t yResolution;       /* 屏幕高度 */
    uint32_t bytesPerScanLine;  /* 屏幕高度 */
    uint8_t bitsPerPixel;       /* 像素位数 */
} KGC_VideoInfo_t;

EXTERN KGC_VideoInfo_t videoInfo;

#endif   /* _KGC_VIDEO_H */
