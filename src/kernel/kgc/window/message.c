/*
 * file:		kernel/kgc/window/message.c
 * auther:		Jason Hu
 * time:		2020/2/19
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/task.h>
#include <kgc/video.h>
#include <kgc/input/mouse.h>
#include <kgc/window/message.h>
#include <kgc/window/window.h>

PUBLIC KGC_MessageNode_t *KGC_CreateMessageNode()
{
    KGC_MessageNode_t *node = kmalloc(sizeof(KGC_MessageNode_t), GFP_KERNEL);
    return node;
}

PUBLIC void KGC_FreeMessageList(KGC_Window_t *window)
{
    KGC_MessageNode_t *node, *next;
    /* 释放消息队列 */
    ListForEachOwnerSafe (node, next, &window->messageListHead, list) {
        /* 从链表中删除 */
        ListDel(&node->list);
        /* 释放消息节点 */
        kfree(node);
    }
}

PUBLIC void KGC_AddMessageNode(KGC_MessageNode_t *node, KGC_Window_t *window)
{
    SpinLock(&window->messageLock);
    /* 如果消息数量超过最大数量，那么就删除掉队列中的第一个消息 */
    ListAddTail(&node->list, &window->messageListHead);
    SpinUnlock(&window->messageLock);
}

/**
 * KGC_RecvMessage - 获取一个消息
 * @message: 消息
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int KGC_RecvMessage(KGC_Message_t *message)
{
    /* 从窗口的消息队列种获取一个消息 */
    
    /* 获取指针并检测 */
    Task_t *cur = CurrentTask();
    if (!cur->window) 
        return -1;
    KGC_Window_t *window = cur->window;
    if (!window)
        return -1;

    SpinLock(&window->messageLock);
    
    /* 没有消息 */
    if (ListEmpty(&window->messageListHead)) {
        SpinUnlock(&window->messageLock);
        return -1;
    }
    //printk("[receive]");
    
    /* 获取一个消息 */
    KGC_MessageNode_t *node = ListOwner(window->messageListHead.next, KGC_MessageNode_t, list);
    /* 从链表删除 */
    ListDel(&node->list);
    
    SpinUnlock(&window->messageLock);
        
    *message = node->message;

    kfree(node);
    //printk("[receive]");
    return 0;
}

/**
 * KGC_SetMessage - 放置一个消息
 * @message: 消息
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int KGC_SendMessage(KGC_Message_t *message)
{
    int retval = -1;
    switch (message->type)
    {
    /* 窗口管理 */
    case KGC_MSG_WINDOW_CREATE:
    case KGC_MSG_WINDOW_CLOSE:
        retval = KGC_MessageDoWindow(&message->window);
        break;
    /* 绘图 */        
    case KGC_MSG_DRAW_PIXEL:
    case KGC_MSG_DRAW_BITMAP:
    case KGC_MSG_DRAW_RECTANGLE:
    case KGC_MSG_DRAW_LINE:
    case KGC_MSG_DRAW_CHAR:
    case KGC_MSG_DRAW_STRING:
    case KGC_MSG_DRAW_BITMAP_PLUS:
    case KGC_MSG_DRAW_RECTANGLE_PLUS:
    case KGC_MSG_DRAW_LINE_PLUS:
    case KGC_MSG_DRAW_CHAR_PLUS:
    case KGC_MSG_DRAW_STRING_PLUS:
    case KGC_MSG_DRAW_UPDATE:
        retval = KGC_MessageDoDraw(&message->draw);
        break;
    default:
        break;
    }
    
    return retval;
}

PUBLIC int SysKGC_Message(int operate, KGC_Message_t *message)
{
    
    if (operate == KGC_MSG_SEND) {
        return KGC_SendMessage(message);
    } else if (operate == KGC_MSG_RECV) {
        return KGC_RecvMessage(message);
    }
    return -1;
}
