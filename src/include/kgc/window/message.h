/*
 * file:		include/kgc/window/message.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_WINDOW_MESSAGE_H
#define _KGC_WINDOW_MESSAGE_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <lib/sys/kgc.h>
#include <kgc/color.h>
#include <kgc/window/window.h>

/* 消息节点，多个消息节点组成窗口消息队列 */
typedef struct KGC_MessageNode {
    struct List list;           /* 链表节点 */
    KGC_Message_t message;      /* 消息 */
} KGC_MessageNode_t;

/* 管理 */
PUBLIC int KGC_SendMessage(KGC_Message_t *message);
PUBLIC int KGC_RecvMessage(KGC_Message_t *message);
PUBLIC int SysKGC_Message(int operate, KGC_Message_t *message);
PUBLIC KGC_MessageNode_t *KGC_CreateMessageNode();
PUBLIC void KGC_AddMessageNode(KGC_MessageNode_t *node, KGC_Window_t *window);
PUBLIC void KGC_FreeMessageList(KGC_Window_t *window);

/* 执行 */
PUBLIC int KGC_MessageDoWindow(KGC_MessageWindow_t *message);
PUBLIC int KGC_MessageDoDraw(KGC_MessageDraw_t *message);

#endif   /* _KGC_WINDOW_MESSAGE_H */
