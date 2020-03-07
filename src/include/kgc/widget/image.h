/*
 * file:		include/kgc/widget/widget.h
 * auther:		Jason Hu
 * time:		2020/2/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 窗口控件之图像 */
#ifndef _KGC_WIDGET_IMAGE_H
#define _KGC_WIDGET_IMAGE_H

#include <lib/types.h>
#include <lib/stdint.h>

/* kgc 图像 */
typedef struct KGC_Image {
    int width, height;  /* 图像大小 */
    uint32_t *data;     /* 图像数据 */
    uint8_t *raw;      /* 原生数据 */
    kgcc_t borderColor; /* 边框颜色 */
    kgcc_t fillColor;   /* 填充颜色 */
} KGC_Image_t;


#endif   /* _KGC_WIDGET_IMAGE_H */
