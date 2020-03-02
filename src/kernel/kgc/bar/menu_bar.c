/*
 * file:		kernel/kgc/bar/menu_bar.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>

#include <kgc/video.h>
#include <kgc/window/window.h>
#include <kgc/window/message.h>
#include <kgc/bar/menubar.h>
#include <kgc/bar/bar.h>
#include <kgc/input/mouse.h>
#include <kgc/container/draw.h>

#include <clock/clock.h>
#include <lib/vsprintf.h>
#include <lib/string.h>

EXTERN KGC_Bar_t kgcbar;

PUBLIC void KGC_MenuBarHandler(KGC_Button_t *button)
{
    if (button == kgcbar.close) {
        //printk("click close button!\n");
        /* 给当前窗口发送关闭窗口消息 */
        if (GET_CURRENT_WINDOW()) {       
            KGC_MessageNode_t *node = KGC_CreateMessageNode();
            if (node) {
                /* 发出退出事件 */
                node->message.type = KGC_MSG_QUIT;
                KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
                //printk("send close to %s\n", GET_CURRENT_WINDOW()->title);
            }
        }
    }
}

/** 
 * KGC_MenuBarUpdateTime - 更新时间显示
 * 
 */
PUBLIC void KGC_MenuBarUpdateTime()
{
    /* 先将原来的位置清空 */
    KGC_ContainerDrawRectangle(kgcbar.container, kgcbar.time->label.x,
        kgcbar.time->label.y, kgcbar.time->label.width, 
        kgcbar.time->label.height, KGC_MENU_BAR_COLOR);

    /* 解析时间 */
    char buf[6];
    memset(buf, 0, 6);
    sprintf(buf, "%d:%d", systemDate.hour, systemDate.minute);
    /* 设置文本，然后显示 */
    KGC_ButtonSetText(kgcbar.time, buf);
    KGC_ButtonShow((KGC_Widget_t *)kgcbar.time);
}

/* 图标数据 */
PRIVATE uint8_t iconRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,
    0,0,0,1,0,1,2,2,2,2,1,0,0,0,0,0,
    0,0,0,1,1,2,2,2,2,2,2,1,0,0,0,0,
    0,0,0,1,2,2,2,2,2,2,2,2,1,0,0,0,
    0,0,1,2,2,2,2,2,2,2,2,2,2,1,0,0,
    0,1,1,1,2,2,1,1,1,1,2,2,1,1,1,0,
    0,0,0,1,2,1,2,2,2,2,1,2,1,0,0,0,
    0,0,0,1,2,1,2,2,2,2,1,2,1,0,0,0,
    0,0,0,1,2,2,2,2,2,2,1,2,1,0,0,0,
    0,0,0,1,2,2,2,2,2,2,1,2,1,0,0,0,
    0,0,0,1,2,2,2,2,2,2,1,2,1,0,0,0,
    0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


/* 最小化数据 */
PRIVATE uint8_t minimRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,
    0,0,1,2,2,2,2,2,2,2,1,0,0,0,0,0,
    0,0,1,2,2,2,2,2,2,2,1,0,0,0,0,0,
    0,0,1,2,2,2,2,2,2,2,1,0,0,0,0,0,
    0,0,1,2,2,2,2,2,2,2,1,0,0,0,0,0,
    0,0,1,2,2,2,2,2,2,2,1,0,0,0,0,0,
    0,0,1,2,2,2,2,2,2,2,1,0,0,0,0,0,
    0,0,1,2,2,2,2,2,1,1,1,1,1,0,0,0,
    0,0,1,2,2,2,2,2,1,2,1,2,1,0,0,0,
    0,0,0,1,1,1,1,1,1,1,1,2,1,0,0,0,
    0,0,0,0,0,0,0,0,1,2,2,2,1,0,0,0,
    0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


/* 最大化数据 */
PRIVATE uint8_t maximRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,
    0,0,0,0,1,2,2,2,2,2,2,2,1,0,0,0,
    0,0,0,0,1,2,2,2,2,2,2,2,1,0,0,0,
    0,0,0,0,1,2,2,2,2,2,2,2,1,0,0,0,
    0,0,0,0,1,2,2,2,2,2,2,2,1,0,0,0,
    0,0,0,0,1,2,2,2,2,2,2,2,1,0,0,0,
    0,0,0,0,1,2,2,2,2,2,2,2,1,0,0,0,
    0,0,1,1,1,1,1,2,2,2,2,2,1,0,0,0,
    0,0,1,2,1,2,1,2,2,2,2,2,1,0,0,0,
    0,0,1,2,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,1,2,2,2,1,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* 最大化数据 */
PRIVATE uint8_t closeRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,2,2,2,2,2,0,0,0,0,0,0,
    0,0,0,2,2,2,2,2,2,2,2,2,0,0,0,0,
    0,0,2,2,2,2,2,2,2,2,2,2,2,0,0,0,
    0,2,2,2,1,2,2,2,2,2,1,2,2,2,0,0,
    0,2,2,1,1,1,2,2,2,1,1,1,2,2,0,0,
    2,2,2,2,1,1,1,2,1,1,1,2,2,2,2,0,
    2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,0,
    2,2,2,2,2,2,1,1,1,2,2,2,2,2,2,0,
    2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,0,
    2,2,2,2,1,1,1,2,1,1,1,2,2,2,2,0,
    0,2,2,1,1,1,2,2,2,1,1,1,2,2,0,0,
    0,2,2,2,1,2,2,2,2,2,1,2,2,2,0,0,
    0,0,2,2,2,2,2,2,2,2,2,2,2,0,0,0,
    0,0,0,2,2,2,2,2,2,2,2,2,0,0,0,0,
    0,0,0,0,0,2,2,2,2,2,0,0,0,0,0,0,
};


PUBLIC int KGC_InitMenuBar()
{
    /* 图标 */
    kgcbar.icon = KGC_CreateLabel();
    if (kgcbar.icon == NULL) {
        printk("create label failed!\n");
    }
    KGC_LabelSetLocation(kgcbar.icon, 0, 0);
    KGC_LabelSetSize(kgcbar.icon, 24, 24);
    KGC_LabelSetImage(kgcbar.icon, 16, 16, iconRawData, KGCC_WHITE, KGCC_BLACK);
    KGC_LabelSetImageAlign(kgcbar.icon, KGC_WIDGET_ALIGN_CENTER);
    KGC_LabelSetName(kgcbar.icon, "icon");
    KGC_LabelAdd(kgcbar.icon, kgcbar.container);
    KGC_LabelShow(&kgcbar.icon->widget);
    
    /* 控制按钮 */
    kgcbar.minim = KGC_CreateButton();
    if (kgcbar.minim == NULL) {
        printk("create button failed!\n");
    }
    kgcbar.minim->label.textAlign = KGC_WIDGET_ALIGN_CENTER; 
    KGC_ButtonSetLocation(kgcbar.minim, 24, 0);
    KGC_ButtonSetSize(kgcbar.minim, 24, 24);
    KGC_ButtonSetName(kgcbar.minim, "min");
    KGC_ButtonSetHandler(kgcbar.minim, &KGC_MenuBarHandler); 
    KGC_ButtonSetImage(kgcbar.minim, 16, 16, minimRawData, KGCC_WHITE, KGCC_BLACK);
    KGC_ButtonSetImageAlign(kgcbar.minim, KGC_WIDGET_ALIGN_CENTER);
    KGC_ButtonAdd(kgcbar.minim, kgcbar.container);
    KGC_ButtonShow(&kgcbar.minim->label.widget);
    
    kgcbar.maxim = KGC_CreateButton();
    if (kgcbar.maxim == NULL) {
        printk("create button failed!\n");
    }
    kgcbar.maxim->label.textAlign = KGC_WIDGET_ALIGN_CENTER; 
    KGC_ButtonSetLocation(kgcbar.maxim, 24+24, 0);
    KGC_ButtonSetSize(kgcbar.maxim, 24, 24);
    //KGC_ButtonSetText(kgcbar.maxim, "#");
    KGC_ButtonSetImage(kgcbar.maxim, 16, 16, maximRawData, KGCC_WHITE, KGCC_BLACK);
    KGC_ButtonSetImageAlign(kgcbar.maxim, KGC_WIDGET_ALIGN_CENTER);
    KGC_ButtonSetName(kgcbar.maxim, "max");
    KGC_ButtonAdd(kgcbar.maxim, kgcbar.container);
    KGC_ButtonSetHandler(kgcbar.maxim, &KGC_MenuBarHandler);
    KGC_ButtonShow(&kgcbar.maxim->label.widget);
    
    kgcbar.close = KGC_CreateButton();
    if (kgcbar.close == NULL) {
        printk("create button failed!\n");
    }
    kgcbar.close->label.textAlign = KGC_WIDGET_ALIGN_CENTER; 
    KGC_ButtonSetLocation(kgcbar.close, 24+24*2, 0);
    KGC_ButtonSetSize(kgcbar.close, 24, 24);
    //KGC_ButtonSetText(kgcbar.close, "x");
    KGC_ButtonSetImage(kgcbar.close, 16, 16, closeRawData, KGCC_BLACK, KGCC_WHITE);
    KGC_ButtonSetImageAlign(kgcbar.close, KGC_WIDGET_ALIGN_CENTER);
    KGC_ButtonSetName(kgcbar.close, "close");
    KGC_ButtonAdd(kgcbar.close, kgcbar.container);
    KGC_ButtonSetHandler(kgcbar.close, &KGC_MenuBarHandler);
    KGC_ButtonShow(&kgcbar.close->label.widget);
    
    kgcbar.time = KGC_CreateButton();
    if (kgcbar.time == NULL) {
        printk("create button failed!\n");
    } 
    KGC_ButtonSetLocation(kgcbar.time, kgcbar.container->width - 6*8, 0);
    KGC_ButtonSetSize(kgcbar.time, 6*8, 24);
    char buf[6];
    memset(buf, 0, 6);
    sprintf(buf, "%d:%d", systemDate.hour, systemDate.minute);
    KGC_ButtonSetText(kgcbar.time, buf);
    KGC_ButtonSetName(kgcbar.time, "time");
    KGC_ButtonAdd(kgcbar.time, kgcbar.container);
    KGC_ButtonSetHandler(kgcbar.time, &KGC_MenuBarHandler);
    KGC_ButtonShow(&kgcbar.time->label.widget);

    /* 标题 */
    kgcbar.title = KGC_CreateLabel();
    if (kgcbar.title == NULL) {
        printk("create label failed!\n");
    }
    kgcbar.title->textAlign = KGC_WIDGET_ALIGN_CENTER; 
    KGC_LabelSetLocation(kgcbar.title, 24*4, 0);
    KGC_LabelSetSize(kgcbar.title, kgcbar.container->width - 24*4 - 6*8, 24);
    KGC_LabelSetText(kgcbar.title, "this is title");
    KGC_LabelSetName(kgcbar.title, "title");
    KGC_LabelAdd(kgcbar.title, kgcbar.container);
    KGC_LabelShow(&kgcbar.title->widget);
    
    return 0;
}
    