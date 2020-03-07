/*
 * file:		include/kgc/widget/widget.h
 * auther:		Jason Hu
 * time:		2020/2/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 窗口控件之标签 */
#ifndef _KGC_WIDGET_H
#define _KGC_WIDGET_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <book/list.h>
#include <kgc/color.h>
#include <kgc/container/container.h>
#include <kgc/container/draw.h>

/* 控件的类型 */
typedef enum KGC_WidgetTypes {
    KGC_WIDGET_LABEL = 0,       /* 标签控件 */
    KGC_WIDGET_BUTTON,          /* 按钮控件 */
} KGC_WidgetType_t;

/* 控件对齐方式 */
typedef enum KGC_WidgetAlign {
    KGC_WIDGET_ALIGN_LEFT = 0,  /* 左对齐 */
    KGC_WIDGET_ALIGN_CENTER,    /* 居中对齐 */
    KGC_WIDGET_ALIGN_RIGHT,     /* 右对齐 */
} KGC_WidgetAlign_t;

/* 控件结构体 */
typedef struct KGC_Widget {
    struct List list;           /* 控件组成的链表 */
    uint8_t type;               /* 控件类型 */
    KGC_Container_t *container; /* 控件所在的容器 */
    void (*draw) (struct KGC_Widget *widget);   /* 控件绘图指针 */
    uint8_t drawCounter;                /* 计数器：为0时才绘图，不然则不会图 */
    
    /* 鼠标事件 */
    void (*mouseButtonDown) (struct KGC_Widget *widget, int button, int mx, int my);
    void (*mouseButtonUp) (struct KGC_Widget *widget,  int button, int mx, int my);
    void (*mouseMotion) (struct KGC_Widget *widget, int mx, int my);

} KGC_Widget_t;

/* 控件绘图指针 */
typedef void (*KGC_WidgetDraw_t) (KGC_Widget_t *widget);
typedef void (*KGC_WidgetMouseButton_t) (KGC_Widget_t *widget, int button, int mx, int my);
typedef void (*KGC_WidgetMouseMotion_t) (KGC_Widget_t *widget, int mx, int my);

STATIC INLINE void KGC_WidgetInit(KGC_Widget_t *widget,
    KGC_WidgetType_t type)
{
    INIT_LIST_HEAD(&widget->list);
    widget->type = type;
    widget->draw = NULL;
    widget->drawCounter = 0;

    widget->mouseButtonDown = NULL;
    widget->mouseButtonUp = NULL;
    widget->mouseMotion = NULL;

} 

STATIC INLINE void KGC_WidgetSetDraw(KGC_Widget_t *widget,
    KGC_WidgetDraw_t draw)
{
    widget->draw = draw;
} 

STATIC INLINE void KGC_WidgetSetMouseEven(KGC_Widget_t *widget,
    KGC_WidgetMouseButton_t buttonDown,
    KGC_WidgetMouseButton_t buttonUp,
    KGC_WidgetMouseMotion_t motion)
{
    widget->mouseButtonDown = buttonDown;
    widget->mouseButtonUp = buttonUp;
    widget->mouseMotion = motion;
} 

STATIC INLINE void KGC_AddWidget(KGC_Widget_t *widget, KGC_Container_t *container)
{
    /* 控件添加到容器 */
    ListAddTail(&widget->list, &container->widgetListHead);
    widget->container = container;
} 

STATIC INLINE void KGC_DelWidget(KGC_Widget_t *widget)
{
    /* 控件添加到容器 */
    ListDel(&widget->list);
    widget->container = NULL;
} 

STATIC INLINE void KGC_WidgetShow(KGC_Widget_t *widget)
{
    if (!widget->drawCounter) {
        /* 调用绘图 */
        if (widget->draw)
            widget->draw(widget);
        widget->drawCounter++;
    }
}

STATIC INLINE void KGC_WidgetMouseButtonUp(struct List *listHead, int button, int mx, int my)
{
    /* 控件检测 */
    KGC_Widget_t *widget;
    ListForEachOwner (widget, listHead, list) {
        if (widget->type == KGC_WIDGET_BUTTON) {
            if (widget->mouseButtonUp)
                widget->mouseButtonUp(widget, button, mx, my);
        }
        KGC_WidgetShow(widget);
    }
}


STATIC INLINE void KGC_WidgetMouseButtonDown(struct List *listHead, int button, int mx, int my)
{
    /* 控件检测 */
    KGC_Widget_t *widget;
    ListForEachOwner (widget, listHead, list) {
        if (widget->type == KGC_WIDGET_BUTTON) {
            if (widget->mouseButtonDown)
                widget->mouseButtonDown(widget, button, mx, my);
        }
        KGC_WidgetShow(widget);
    }
}

STATIC INLINE void KGC_WidgetMouseMotion(struct List *listHead, int mx, int my)
{
    /* 控件检测 */
    KGC_Widget_t *widget;
    ListForEachOwner (widget, listHead, list) {
        if (widget->type == KGC_WIDGET_BUTTON) {
            if (widget->mouseMotion)
                widget->mouseMotion(widget, mx, my);
        }
        
        KGC_WidgetShow(widget);
    }
}



#endif   /* _KGC_WIDGET_H */
