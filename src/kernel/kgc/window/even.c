/*
 * file:		kernel/kgc/window/even.c
 * auther:		Jason Hu
 * time:		2020/2/17
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/memcache.h>
#include <video/video.h>
#include <kgc/window/window.h>
#include <kgc/window/message.h>
#include <kgc/window/even.h>

/* 将输入转换成特殊的消息转发给当前窗口 */

PUBLIC void KGC_WindowDoTimer(int ticks)
{
    /*if (GET_CURRENT_WINDOW()) {           
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            node->message.type = KGC_MSG_TIMER;
            node->message.timer.ticks = ticks;
            KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
        }
    }*/
}

PUBLIC void KGC_WindowMouseDown(int button, int mx, int my)
{
    //printk("window mouse down\n");
    if (GET_CURRENT_WINDOW()) {           
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            node->message.type = KGC_MSG_MOUSE_BUTTON_DOWN;
            node->message.mouse.button = button;
            node->message.mouse.x = mx;
            node->message.mouse.y = my;
            /* 上锁保护信息链表 */
            KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
        }
    }
}

PUBLIC void KGC_WindowMouseUp(int button, int mx, int my)
{
    //printk("window mouse up\n");
    if (GET_CURRENT_WINDOW()) {           
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            node->message.type = KGC_MSG_MOUSE_BUTTON_UP;
            node->message.mouse.button = button;
            node->message.mouse.x = mx;
            node->message.mouse.y = my;
            KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
        }        
    }
}

PUBLIC void KGC_WindowMouseMotion(int mx, int my)
{
    //printk("window mouse motion\n");
    if (GET_CURRENT_WINDOW()) {           
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            node->message.type = KGC_MSG_MOUSE_MOTION;
            node->message.mouse.x = mx;
            node->message.mouse.y = my;
            KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
        }
    }
}

/**
 * KGC_WindowKeyDown - 窗口按键按下
 * @even: 按键事件
 * 
 */
PUBLIC int KGC_WindowKeyDown(KGC_KeyboardEven_t *even)
{
    //printk("window key down\n");
    /* 往窗口的事件队列发送一个按键信息 */
    //printk("keycode:%c mod:%x\n", even->keycode.code, even->keycode.modify);
    if (GET_CURRENT_WINDOW()) {       
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            node->message.type = KGC_MSG_KEY_DOWN;
            node->message.key.code = even->keycode.code;
            node->message.key.modify = even->keycode.modify;
            
            KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
        }
        
    }
    return 0;
}

PUBLIC int KGC_WindowKeyUp(KGC_KeyboardEven_t *even)
{
    //printk("window key up\n");
    //printk("keycode:%c mod:%x\n", even->keycode.code, even->keycode.modify);
    if (GET_CURRENT_WINDOW()) {           
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            node->message.type = KGC_MSG_KEY_UP;
            node->message.key.code = even->keycode.code;
            node->message.key.modify = even->keycode.modify;
            KGC_AddMessageNode(node, GET_CURRENT_WINDOW());
        }
    }
    return 0;
}
