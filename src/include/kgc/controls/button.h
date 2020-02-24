/*
 * file:		include/kgc/controls/button.h
 * auther:		Jason Hu
 * time:		2020/2/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 窗口控件之按钮 */
#ifndef _KGC_CONTROLS_BUTTON_H
#define _KGC_CONTROLS_BUTTON_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <kgc/color.h>
#include <book/list.h>
#include <kgc/font/font.h>
#include <kgc/controls/label.h>

#define KGC_BUTTON_DEFAULT      0   /* 按钮默认状态 */
#define KGC_BUTTON_FOCUS        1   /* 按钮聚焦状态 */
#define KGC_BUTTON_SELECTED     2   /* 按钮选择状态 */

/* 按钮默认颜色 */
#define KGC_BUTTON_DEFAULT_COLOR    KGCC_ARGB(255, 50, 50, 50)
#define KGC_BUTTON_FOCUS_COLOR      KGCC_ARGB(255, 200, 200, 200)
#define KGC_BUTTON_SELECTED_COLOR   KGCC_ARGB(255, 100, 100, 100)

typedef struct KGC_Button {
    KGC_Label_t label;      /* 继承标签：第一个成员 */
    int state;              /* 按钮状态：默认，聚焦，点击 */
    kgcc_t defaultColor;    /* 默认颜色 */
    kgcc_t focusColor;      /* 聚焦颜色 */
    kgcc_t selectedColor;   /* 选择颜色 */
    void (*handler) (struct KGC_Button *button);

} KGC_Button_t;

typedef void (*KGC_ButtonHandler_t) (struct KGC_Button *button);

PUBLIC KGC_Button_t *KGC_CreateButton();
PUBLIC int KGC_ButtonInit(KGC_Button_t *button);
PUBLIC void KGC_ButtonSetLocation(KGC_Button_t *button, int x, int y);
PUBLIC void KGC_ButtonSetSize(KGC_Button_t *button, int width, int height);
PUBLIC void KGC_ButtonSetName(KGC_Button_t *button, char *name);
PUBLIC int  KGC_ButtonSetTextMaxLength(KGC_Button_t *button, int length);
PUBLIC void KGC_ButtonSetText(KGC_Button_t *button, char *text);
PUBLIC void KGC_ButtonAdd(KGC_Button_t *button, KGC_Container_t *container);
PUBLIC void KGC_ButtonShow(KGC_Widget_t *widget);
PUBLIC void KGC_ButtonSetHandler(KGC_Button_t *button, KGC_ButtonHandler_t handler);
PUBLIC void KGC_ButtonSetImage(KGC_Button_t *button, int width,
    int height, uint8_t *raw, kgcc_t border, kgcc_t fill);
PUBLIC void KGC_ButtonDel(KGC_Button_t *button);

PUBLIC void KGC_ButtonDestroySub(KGC_Button_t *button);
PUBLIC void KGC_ButtonDestroy(KGC_Button_t *button);
PUBLIC void KGC_ButtonSetImageAlign(KGC_Button_t *button, KGC_WidgetAlign_t align);

#endif   /* _KGC_CONTROLS_BUTTON_H */
