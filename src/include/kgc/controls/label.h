/*
 * file:		include/kgc/controls/label.h
 * auther:		Jason Hu
 * time:		2020/2/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 窗口控件之标签 */
#ifndef _KGC_CONTROLS_LABEL_H
#define _KGC_CONTROLS_LABEL_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <kgc/color.h>
#include <book/list.h>
#include <kgc/controls/widget.h>
#include <kgc/font/font.h>
#include <kgc/container/container.h>

/* 标签名长度 */
#define KGC_LABEL_NAME_LEN     24

/* 默认文本长度 */
#define KGC_MAX_LABEL_TEXT_LEN     255
#define KGC_DEFAULT_LABEL_TEXT_LEN     32

/* 标签默认颜色 */
#define KGC_LABEL_BACK_COLOR    KGCC_ARGB(255, 50, 50, 50)
#define KGC_LABEL_FONT_COLOR    KGCC_WHITE
#define KGC_LABEL_DISABEL_COLOR KGCC_ARGB(255, 192, 192, 192)

/* 默认宽高 */
#define KGC_DEFAULT_LABEL_WIDTH         40
#define KGC_DEFAULT_LABEL_HEIGHT        20

/* kgc 图像 */
typedef struct KGC_Image {
    int width, height;  /* 图像大小 */
    uint32_t *data;     /* 图像数据 */
    uint8_t *raw;      /* 原生数据 */
    kgcc_t borderColor; /* 边框颜色 */
    kgcc_t fillColor;   /* 填充颜色 */
} KGC_Image_t;

typedef enum KGC_LabelTypes {
    KGC_LABEL_TEXT  = 0,    /* 文本标签 */
    KGC_LABEL_IMAGE,        /* 图像标签 */
    KGC_LABEL_PICTURE,      /* 图片标签 */
    KGC_LABEL_GRAPH         /* 图形标签 */
} KGC_LabelTypes_t;

typedef struct KGC_Label {
    KGC_Widget_t widget;            /* 继承控件：第一个成员 */
    int x, y;                       /* 位置属性 */
    int width, height;              /* 大小属性 */
    char visable;                   /* 可见 */
    char disabel;                   /* 禁止 */
    
    char name[KGC_LABEL_NAME_LEN];  /* 标签名 */
    uint8_t nameLength;             /* 名字长度 */
    /* 颜色 */
    kgcc_t backColor;               /* 背景色 */
    kgcc_t fontColor;               /* 字体色 */
    kgcc_t disableColor;            /* 禁止字体色 */
    
    /* 字体 */
    KGC_Font_t *font;

    /* 标签的内容 */
    KGC_LabelTypes_t type;          /* 标签类型 */
    char *text;                     /* 文本 */
    uint8_t textLength;             /* 文本长度 */
    uint8_t textMaxLength;          /* 文本最大长度 */
    KGC_WidgetAlign_t textAlign;    /* 文本对齐 */

    KGC_Image_t image;              /* 图像 */
    KGC_WidgetAlign_t imageAlign;   /* 图像对齐 */

    /* 图形 */
} KGC_Label_t;

PUBLIC KGC_Label_t *KGC_CreateLabel();
PUBLIC int KGC_LabelInit(KGC_Label_t *label);
PUBLIC void KGC_LabelSetLocation(KGC_Label_t *label, int x, int y);
PUBLIC void KGC_LabelSetSize(KGC_Label_t *label, int width, int height);
PUBLIC void KGC_LabelSetColor(KGC_Label_t *label, kgcc_t back, kgcc_t font);
PUBLIC void KGC_LabelSetName(KGC_Label_t *label, char *name);
PUBLIC int KGC_LabelSetTextMaxLength(KGC_Label_t *label, int length);
PUBLIC void KGC_LabelSetText(KGC_Label_t *label, char *text);
PUBLIC int KGC_LabelSetFont(KGC_Label_t *label, char *fontName);
PUBLIC void KGC_LabelAdd(KGC_Label_t *label, KGC_Container_t *container);
PUBLIC void KGC_LabelShow(KGC_Widget_t *widget);
PUBLIC void KGC_LabelSetImage(KGC_Label_t *label, int width,
    int height, uint8_t *raw, kgcc_t border, kgcc_t fill);
PUBLIC void KGC_LabelDestroy(KGC_Label_t *label);
PUBLIC void KGC_LabelDestroySub(KGC_Label_t *label);
PUBLIC void KGC_LabelSetImageAlign(KGC_Label_t *label, KGC_WidgetAlign_t align);




#endif   /* _KGC_CONTROLS_LABEL_H */
