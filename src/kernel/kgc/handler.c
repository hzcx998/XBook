/*
 * file:		kernel/kgc/handler.c
 * auther:		Jason Hu
 * time:		2020/2/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/* KGC-kernel graph core 内核图形核心 */

/* 图形核心事件处理 */

/* 系统内核 */
#include <book/config.h>
#ifdef CONFIG_DISPLAY_GRAPH

#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>

#include <lib/string.h>

/* 图形系统 */
#include <kgc/draw.h>
#include <kgc/even.h>
#include <kgc/button.h>
#include <kgc/video.h>

EXTERN KGC_VideoInfo_t videoInfo;     /* 视频信息 */

/* 鼠标测试数据 */
PRIVATE int mx = 400, my = 300;

/**
 * KGC_EvenHandler - 图形事件处理
 * @even: 鼠标按钮事件
 */
PUBLIC void KGC_EvenHandler(KGC_Even_t *even)
{   
    /* 通用处理 */
    switch (even->type)
    {
    case KGC_KEY_DOWN:
        if (even->key.keycode.code == KGCK_SPACE) {
            printk("key space down\n");
        } else if (even->key.keycode.code == KGCK_ENTER) {
            printk("key enter down\n");
        } 
        break;
    case KGC_KEY_UP:
        if (even->key.keycode.code == KGCK_SPACE) {
            printk("key space up\n");
        } else if (even->key.keycode.code == KGCK_ENTER) {
            printk("key enter up\n");
        } 
        break;
    case KGC_MOUSE_BUTTON_DOWN:
        if (even->button.button == KGC_MOUSE_LEFT) {
            printk("mouse left down\n");
        } else if (even->button.button == KGC_MOUSE_RIGHT) {
            printk("mouse right down\n");
        } else if (even->button.button == KGC_MOUSE_MIDDLE) {
            printk("mouse middle down\n");
        }
        break;
    case KGC_MOUSE_BUTTON_UP:
        if (even->button.button == KGC_MOUSE_LEFT) {
            printk("mouse left up\n");
        } else if (even->button.button == KGC_MOUSE_RIGHT) {
            printk("mouse right up\n");
        } else if (even->button.button == KGC_MOUSE_MIDDLE) {
            printk("mouse middle up\n");
        }
        break;
    case KGC_MOUSE_MOTION:
        KGC_DrawRectangle(mx, my, 20, 20, KGCC_BLACK);
        mx += even->motion.x;
        my += even->motion.y;
        
        if (mx < 0) {
            mx = 0;
        }
        if (mx > videoInfo.xResolution - 1) {
            mx = videoInfo.xResolution - 1;
        }
        if (my < 0) {
            my = 0;
        }
        if (my > videoInfo.yResolution - 1) {
            my = videoInfo.yResolution - 1;
        }

        //printk("mx:%d my:%d\n", mx, my);

        // 显示鼠标
        KGC_DrawRectangle(mx, my, 20, 20, KGCC_WHITE);

        break;
    default:
        break;
    }

}

#endif /* CONFIG_DISPLAY_GRAPH */

