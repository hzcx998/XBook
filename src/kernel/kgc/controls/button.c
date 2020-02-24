/*
 * file:		kernel/kgc/controls/button.c
 * auther:		Jason Hu
 * time:		2020/2/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/memcache.h>
#include <kgc/video.h>
#include <kgc/color.h>
#include <kgc/controls/button.h>
#include <lib/string.h>

/**
 * KGC_ButtonMouseDown - 鼠标按下事件
 * @widget: 控件
 * @button: 按钮
 * @mx: 鼠标横坐标
 * @my: 鼠标纵坐标
 */
PRIVATE void KGC_ButtonMouseDown(KGC_Widget_t *widget, int btn, int mx, int my)
{
    KGC_Button_t *button = (KGC_Button_t *) widget;
    KGC_Label_t *label = &button->label;
    
    if (label->x <= mx && mx < label->x + label->width &&
        label->y <= my && my < label->y + label->height) {
        /* 必须是聚焦了才能点击，点击后变成选择状态 */
        if (button->state == KGC_BUTTON_FOCUS) {
            /* 聚焦 */
            button->state = KGC_BUTTON_SELECTED;
            label->backColor = button->selectedColor;
            /* 重绘 */
            label->widget.drawCounter = 0;    
        }
    }
}

/**
 * KGC_ButtonMouseDown - 鼠标弹起事件
 * @widget: 控件
 * @button: 按钮
 * @mx: 鼠标横坐标
 * @my: 鼠标纵坐标
 */
PRIVATE void KGC_ButtonMouseUp(KGC_Widget_t *widget, int btn, int mx, int my)
{
    KGC_Button_t *button = (KGC_Button_t *) widget;
    KGC_Label_t *label = &button->label;

    if (label->x <= mx && mx < label->x + label->width &&
        label->y <= my && my < label->y + label->height) {
        /* 如果已经是选择状态，那么弹起后，就会去处理调用函数。由于还在按钮内，所以状态设置为聚焦 */
        if (button->state == KGC_BUTTON_SELECTED) {
            /* 聚焦 */
            button->state = KGC_BUTTON_FOCUS;
            label->backColor = button->focusColor;
            /* 重绘 */
            label->widget.drawCounter = 0;      
            /* 调用处理函数 */
            //printk(">>>call handler!\n");
            if (button->handler)
                button->handler(button);
        }
    } else {
        /* 如果弹起的时候没在按钮内，只有选择状态才会设置成默认状态。 */
        if (button->state == KGC_BUTTON_SELECTED) {
            /* 弹起的时候没有在按钮内，就设置成默认 */
            /* 默认 */
            button->state = KGC_BUTTON_DEFAULT;
            label->backColor = button->defaultColor;
            /* 重绘 */
            label->widget.drawCounter = 0;
        }
    }
}

/**
 * KGC_ButtonMouseDown - 鼠标弹起事件
 * @widget: 控件
 * @button: 按钮
 * @mx: 鼠标横坐标
 * @my: 鼠标纵坐标
 */
PRIVATE void KGC_ButtonMouseMotion(KGC_Widget_t *widget, int mx, int my)
{
    KGC_Button_t *button = (KGC_Button_t *) widget;
    KGC_Label_t *label = &button->label;

    if (label->x <= mx && mx < label->x + label->width &&
        label->y <= my && my < label->y + label->height) {
        /* 在移动的时候，如果是默认状态，并且碰撞到，那么就设置成聚焦状态 */
        if (button->state == KGC_BUTTON_DEFAULT) {
            /* 聚焦 */
            button->state = KGC_BUTTON_FOCUS;
            label->backColor = button->focusColor;
            /* 重绘 */
            label->widget.drawCounter = 0;    
        }
    } else {
        /* 没有碰撞，当是聚焦的时候，移动出来了才设置成默认，如果是选择状态，
        就先保持，说不定后面会移动回来，那样就可以继续处理弹起。 */
        if (button->state == KGC_BUTTON_FOCUS) {
            /* 默认 */
            button->state = KGC_BUTTON_DEFAULT;
            label->backColor = button->defaultColor;
            /* 重绘 */
            label->widget.drawCounter = 0;
        } 
    }
}

PUBLIC int KGC_ButtonInit(KGC_Button_t *button)
{
    
    if (KGC_LabelInit(&button->label)) {
        return -1;
    }
    
    /* 重载一些信息 */
    KGC_LabelSetName(&button->label, "button");
    KGC_LabelSetText(&button->label, "button");
    KGC_WidgetInit(&button->label.widget, KGC_WIDGET_BUTTON);
    KGC_WidgetSetDraw(&button->label.widget, &KGC_ButtonShow);
    /* 初始化按钮自己的信息 */
    button->state = KGC_BUTTON_DEFAULT;
    button->defaultColor = KGC_BUTTON_DEFAULT_COLOR;
    button->focusColor = KGC_BUTTON_FOCUS_COLOR;
    button->selectedColor = KGC_BUTTON_SELECTED_COLOR;
    button->handler = NULL;
    
    KGC_WidgetSetMouseEven(&button->label.widget, 
        &KGC_ButtonMouseDown, &KGC_ButtonMouseUp, &KGC_ButtonMouseMotion);
    return 0;
}


/**
 * KGC_CreateButton - 创建一个按钮
 * 
 * 成功返回按钮，失败返回NULL
 */
PUBLIC KGC_Button_t *KGC_CreateButton()
{
    KGC_Button_t *button = kmalloc(sizeof(KGC_Button_t), GFP_KERNEL);
    if (button == NULL) {
        return NULL;
    }
    
    if (KGC_ButtonInit(button)) {
        kfree(button);
        return NULL;
    }
        
    return button;
}

/**
 * KGC_ButtonSetLocation - 设置按钮的位置
 * @Button: 按钮
 * @x: 横坐标
 * @y: 纵坐标
 */
PUBLIC void KGC_ButtonSetLocation(KGC_Button_t *button, int x, int y)
{
    KGC_LabelSetLocation(&button->label, x, y);
}

/**
 * KGC_ButtonSetSize - 设置按钮的大小
 * @Button: 按钮
 * 
 */
PUBLIC void KGC_ButtonSetSize(KGC_Button_t *button, int width, int height)
{
    KGC_LabelSetSize(&button->label, width, height);
}

/**
 * KGC_ButtonSetName - 设置按钮的名字
 * @Button: 按钮
 * @back: 背景色
 * @font: 字体色
 */
PUBLIC void KGC_ButtonSetName(KGC_Button_t *button, char *name)
{
    KGC_LabelSetName(&button->label, name);
}

/**
 * KGC_ButtonSetTextMaxLength - 设置按钮的最大长度
 * @Button: 按钮
 * @length: 最大长度
 * 
 * 设置文本前必须调用一次
 */
PUBLIC int KGC_ButtonSetTextMaxLength(KGC_Button_t *button, int length)
{
    return KGC_LabelSetTextMaxLength(&button->label, length);
}

/**
 * KGC_ButtonSetText - 设置按钮的文本
 * @Button: 按钮
 * @text: 文本
 */
PUBLIC void KGC_ButtonSetText(KGC_Button_t *button, char *text)
{
    KGC_LabelSetText(&button->label, text);
}


PUBLIC void KGC_ButtonSetImage(KGC_Button_t *button, int width,
    int height, uint8_t *raw, kgcc_t border, kgcc_t fill)
{
    KGC_LabelSetImage(&button->label, width, height, raw, border, fill);
}

PUBLIC void KGC_ButtonSetImageAlign(KGC_Button_t *button, KGC_WidgetAlign_t align)
{
    KGC_LabelSetImageAlign(&button->label, align);
}

/**
 * KGC_AddButton - 添加按钮
 * @button: 按钮
 * @container: 容器
 * 
 * 添加标签到容器上
 */
PUBLIC void KGC_ButtonAdd(KGC_Button_t *button, KGC_Container_t *container)
{
    KGC_AddWidget(&button->label.widget, container);

}


/**
 * KGC_ButtonDestroySub - 销毁一个按钮，子操作
 * 
 */
PUBLIC void KGC_ButtonDestroySub(KGC_Button_t *button)
{
    /* 先销毁标签 */
    KGC_LabelDestroySub(&button->label);
    /* 销毁自己的内容 */

}

/**
 * KGC_ButtonDestroy - 销毁一个按钮
 * 
 */
PUBLIC void KGC_ButtonDestroy(KGC_Button_t *button)
{
    KGC_ButtonDestroySub(button);
    kfree(button);
}

/**
 * KGC_ButtonDel - 删除按钮
 * @button: 按钮
 * 
 */
PUBLIC void KGC_ButtonDel(KGC_Button_t *button)
{
    KGC_DelWidget(&button->label.widget);
}


/**
 * KGC_ButtonDestory - 销毁按钮
 * @button: 按钮
 * 
 */
PUBLIC void KGC_ButtonDestory(KGC_Button_t *button)
{
    
}


/**
 * KGC_ButtonShow - 显示按钮
 * @widget: 控件
 */
PUBLIC void KGC_ButtonShow(KGC_Widget_t *widget)
{
    KGC_LabelShow(widget);
}

/**
 * KGC_ButtonSetHandler - 设置按钮处理函数
 * @button: 按钮
 * @handler: 处理函数
 */
PUBLIC void KGC_ButtonSetHandler(KGC_Button_t *button, KGC_ButtonHandler_t handler)
{
    button->handler = handler;
}
