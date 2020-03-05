/*
 * file:		kernel/kgc/window/window.c
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/task.h>
#include <video/video.h>
#include <lib/string.h>
#include <kgc/draw.h>
#include <kgc/input/mouse.h>
#include <kgc/window/message.h>
#include <kgc/window/window.h>
#include <kgc/window/switch.h>
#include <kgc/window/draw.h>
#include <kgc/bar/bar.h>
#include <kgc/font/font.h>

/* 窗口链表头，所有窗口都在此链表上 */
PROTECT LIST_HEAD(windowListHead);

/* 当前激活的窗口 */
PUBLIC KGC_Window_t *currentWindow;

/**
 * KGC_WindowCreate - 创建一个窗口
 * @width: 窗口宽度
 * @height: 窗口高度
 * 
 * @return: 返回一个创建的窗口，失败返回NULL
 */
PUBLIC KGC_Window_t *KGC_WindowCreate(
    char *title,
    int width,
    int height)
{
    /* 参数检测 */
    if ((strlen(title) >= KGC_WINDOW_TITLE_NAME_LEN - 1) ||
        !width || !height)
        return NULL;
    
    /* 创建窗口对象 */
    KGC_Window_t *window = (KGC_Window_t *) kmalloc(sizeof(KGC_Window_t), GFP_KERNEL);
    if (window == NULL)
        return NULL;
    
    memset(window->title, 0, KGC_WINDOW_TITLE_NAME_LEN);
    strcpy(window->title, title);

    /* 窗口的宽高 */
    window->width = width;
    window->height = height;
    /* 把窗口添加到窗口链表 */
    ListAddTail(&window->list, &windowListHead);
    /* 窗口的位置是在视图中的偏移位置 */
    window->x = 0;  
    window->y = 0;  
    
    /* 设置窗口显示模式为固定 */
    window->flags = KGC_WINDOW_FLAG_FIX;
    
    window->container = NULL;

    /* 绑定任务，创建者 */
    window->task = CurrentTask();
    CurrentTask()->window = window;
    
    /* 消息队列 */
    INIT_LIST_HEAD(&window->messageListHead);

    /* 消息锁 */
    SpinLockInit(&window->messageLock);

    return window;
}

/**
 * KGC_WindowAdd - 把窗口添加到一个视图上
 * 
 * 
 */
PUBLIC int KGC_WindowAdd(KGC_Window_t *window)
{
    if (!window)
        return -1;

    /* 绑定视图 */
    window->container = KGC_ContainerAlloc();
    if (window->container == NULL) {
        return -1;
    }

    int left = videoInfo.xResolution / 2 - window->width / 2;  
    int top = videoInfo.yResolution / 2 - window->height / 2;  
    
    if (left < 0) 
        left = 0;
    if (top < 0) 
        top = 0;

    /* 把视图的位置加上窗口区域的位置 */
    if (top < KGC_BAR_HEIGHT)
        top = KGC_BAR_HEIGHT;

    KGC_ContainerInit(window->container, "window", left, top, window->width, window->height);
    /* 视图的位置根据窗口大小调整，居中设置 */
    KGC_WindowDrawRectangle(window, 0, 0, window->width, window->height, KGCC_BLACK);

    /* 当前窗口置为隐藏 */
    if (GET_CURRENT_WINDOW())
        KGC_ContainerZ(GET_CURRENT_WINDOW()->container, -1);
    
    /* 新窗口放在最前面 */
    KGC_ContainerAtTopZ(window->container); 

    /* 设置为当前窗口 */
    SET_CURRENT_WINDOW(window);

    /* 添加到任务栏 */
    KGC_TaskBarAddWindow(window);

    return 0;
}

/**
 * KGC_WindowDel - 删除一个窗口及其对应的视图
 * 
 * 
 */
PUBLIC int KGC_WindowDel(KGC_Window_t *window)
{
    
    /* 释放窗口控制器 */
    KGC_TaskBarDelWindow(window);
    
    /* 销毁窗口容器 */
    KGC_ContainerFree(window->container);
    
    return 0;
}

/**
 * KGC_WindowDestroy - 销毁一个窗口
 * @window: 窗口对象
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int KGC_WindowDestroy(KGC_Window_t *window)
{
    if (!window)
        return -1;

    /* 从窗口队列中删除 */
    ListDel(&window->list);

    /* 释放消息队列 */
    KGC_FreeMessageList(window);
    
    if (window->task) {
        /* 任务窗口为空 */
        Task_t *task = (Task_t *)window->task;
        task->window = NULL;    
    }
    
    /* 释放窗口对象 */
    kfree(window);
    //printk("free window\n");
    return 0;
}

/**
 * KGC_WindowClose - 关闭窗口
 * @window: 窗口
 * 
 * 关闭指定窗口
 */
PUBLIC int KGC_WindowClose(KGC_Window_t *window)
{
    if (!window)
        return -1;
    
    /* 如果是窗口的创建者，才能够关闭窗口 */
    if (window->task != CurrentTask())
        return -1;

    /* 当前窗口置为隐藏 */
    if (GET_CURRENT_WINDOW())
        KGC_ContainerZ(GET_CURRENT_WINDOW()->container, -1);
    
    /* 窗口关闭后，不指向窗口。 */
    currentWindow = NULL;
    
    /* 删除窗口 */
    if (!KGC_WindowDel(window)) {
        /* 销毁窗口 */
        if (!KGC_WindowDestroy(window)) {
            return 0;
        }
    }
    return -1;
}

#if 1
PRIVATE void ThreadWindow(void *arg)
{
    /*KGC_Window_t *win = KGC_WindowCreate("test", 400, 300);
    if (win == NULL)
        Panic("create window failed!\n");
    if (KGC_WindowAdd(win))
        Panic("add window failed!\n");
    
    KGC_SwitchWindow(win);

    KGC_WindowDrawRectangle(win, 0,0,100,100, KGCC_WHITE);
    KGC_WindowRefresh(win, 0,0,100,100);
    */
    KGC_Message_t msg;
    msg.type = KGC_MSG_WINDOW_CREATE;
    msg.window.title = "hello";
    msg.window.width = 400;
    msg.window.height = 300;
    
    if (KGC_SendMessage(&msg)) {
        printk("create window failed!\n");
    }
    
    msg.type = KGC_MSG_DRAW_RECTANGLE;
    msg.draw.left = 100;
    msg.draw.top = 50;
    msg.draw.width = 50;
    msg.draw.height = 100;
    msg.draw.color = KGCC_YELLOW;
    KGC_SendMessage(&msg);
    
    
    msg.type = KGC_MSG_DRAW_STRING;
    msg.draw.left = 200;
    msg.draw.top = 50;
    msg.draw.string = "HELLO , KGC";
    msg.draw.color = KGCC_YELLOW;
    KGC_SendMessage(&msg);
    
    msg.type = KGC_MSG_DRAW_LINE;
    msg.draw.left = 200;
    msg.draw.top = 50;
    msg.draw.right = 250;
    msg.draw.buttom = 100;
    msg.draw.color = KGCC_BLUE;
    KGC_SendMessage(&msg);
    
    msg.type = KGC_MSG_DRAW_UPDATE;
    msg.draw.left = 100;
    msg.draw.top = 50;
    msg.draw.right = 300 + 50;
    msg.draw.buttom = 50 + 100;
    KGC_SendMessage(&msg);
    
    uint32_t color = 10;
    int quit = 0;
    while (!quit) {
        
        msg.type = KGC_MSG_DRAW_RECTANGLE;
        msg.draw.left = 100;
        msg.draw.top = 50;
        msg.draw.width = 50;
        msg.draw.height = 100;
        msg.draw.color = 0XFF000000 + color;
        KGC_SendMessage(&msg);
        
        color += 10;
        /*
        msg.type = KGC_MSG_DRAW_UPDATE;
        msg.draw.left = 100;
        msg.draw.top = 50;
        msg.draw.right = 100 + 50;
        msg.draw.buttom = 50 + 100;
        KGC_SendMessage(&msg);*/
        memset(&msg, 0, sizeof(KGC_Message_t));
        if (!KGC_RecvMessage(&msg)) {
            switch (msg.type)
            {
            case KGC_MSG_KEY_DOWN:
                //printk("msg [key down]->code:%x, modify:%x\n", msg.key.code, msg.key.modify);
                
                if (msg.key.code == KGCK_ESCAPE) {
                    //printk("!quit!\n");
                    msg.type = KGC_MSG_WINDOW_CLOSE;
                    KGC_SendMessage(&msg);
                }
                break;
            case KGC_MSG_KEY_UP:
                //printk("msg [key up]->code:%x, modify:%x\n", msg.key.code, msg.key.modify);
                break;
            case KGC_MSG_MOUSE_MOTION:
                //printk("msg [mouse motion]->mx:%d, my:%d\n", msg.mouse.x, mouse.y);
                break;
            case KGC_MSG_MOUSE_BUTTON_DOWN:
                //printk("msg [mouse key down]->button:%x, mx:%d, my:%d\n", msg.mouse.button, msg.mouse.x, mouse.y);
                break;
            case KGC_MSG_MOUSE_BUTTON_UP:
                //printk("msg [mouse key up]->button:%x, mx:%d, my:%d\n", msg.mouse.button, msg.mouse.x, mouse.y);
                break;
            case KGC_MSG_QUIT:
                //printk("msg [quit]\n");
                
                quit = 1;
                break;            
            default:
                break;
            }
        }
        //printk("t");
    }
    //ThreadStart("window", 2, ThreadWindow, NULL);
    msg.type = KGC_MSG_WINDOW_CLOSE;
    KGC_SendMessage(&msg);
    
    ThreadExit(CurrentTask());
}
#endif

EXTERN int InitGttyDriver();


PUBLIC int KGC_InitWindowContainer()
{
    /* 设置当前窗口为空 */
    currentWindow = NULL;
    /* 创建第一个窗口 */
    /*KGC_Window_t *win0 = KGC_WindowCreate("test1", 400, 300);
    if (win0 == NULL)
        return -1;
    if (KGC_WindowAdd(win0))
        return -1;

    KGC_WindowDrawRectangle(win0, 0, 0, 400, 300, KGCC_ARGB(255, 0xaa, 0xaa, 0xaa));
    KGC_WindowRefresh(win0, 0, 0, 400, 300);
    
    KGC_WindowDrawPixel(win0, 15, 20, KGCC_ARGB(255, 0xff, 0xff, 0xff));
    KGC_WindowRefresh(win0, 10, 10, 15, 20);
    
    KGC_WindowDrawRectangle(win0, 50, 50, 100, 100, KGCC_ARGB(255, 0xff, 0, 0));
    KGC_WindowRefresh(win0, 50, 50, 150, 150);*/
    /*KGC_Window_t *win1 = KGC_WindowCreate("test2", 400, 300);
    if (win1 == NULL)
        return -1;
    
    if (KGC_WindowAdd(win1))
        return -1;
    
    KGC_WindowDrawRectangle(win1, 0, 0, 400, 300, KGCC_ARGB(255, 0xaa, 0, 0xaa));
    KGC_WindowRefresh(win1, 0, 0, 400, 300);
    

    KGC_Window_t *win2 = KGC_WindowCreate("test3", 400, 300);
    if (win2 == NULL)
        return -1;
    
    if (KGC_WindowAdd(win2))
        return -1;
    
    KGC_WindowDrawRectangle(win2, 0, 0, 400, 300, KGCC_ARGB(255, 0, 0, 0xaa));
    KGC_WindowRefresh(win2, 0, 0, 400, 300);
    */
    /* 打开一个测试窗口任务 */
    //ThreadStart("window", 2, ThreadWindow, NULL);
    return 0;
}