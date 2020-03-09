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

/* 创建窗口皮肤 */
PROTECT KGC_WindowSkin_t windowSkin;

/* 窗口移动 */
PROTECT KGC_WindowMovement_t windowMovement;

/**
 * KGC_WindowCreate - 创建一个窗口
 * @width: 窗口宽度
 * @height: 窗口高度
 * 
 * @return: 返回一个创建的窗口，失败返回NULL
 */
PUBLIC KGC_Window_t *KGC_WindowCreate(
    char *name,
    char *title,
    uint32_t style,
    int x,
    int y,
    int width,
    int height,
    void *param)
{
    /* 参数检测 */
    if ((strlen(title) >= KGC_WINDOW_TITLE_LEN - 1) ||
        (strlen(name) >= KGC_WINDOW_NAME_LEN - 1) ||
        !width || !height)
        return NULL;
    
    /* 创建窗口对象 */
    KGC_Window_t *window = (KGC_Window_t *) kmalloc(sizeof(KGC_Window_t), GFP_KERNEL);
    if (window == NULL)
        return NULL;
    
    memset(window->name, 0, KGC_WINDOW_NAME_LEN);
    strcpy(window->name, name);
    
    memset(window->title, 0, KGC_WINDOW_TITLE_LEN);
    strcpy(window->title, title);

    /* 窗口的宽高 */
    window->width = width;
    window->height = height;
    
    /* 窗口的位置 */
    window->x = x;  
    window->y = y;  
    
    /* 设置窗口显示模式为固定 */
    window->flags = KGC_WINDOW_FLAG_FIX;
    
    window->style = style;
    
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

    /* 绑定容器 */
    window->container = KGC_ContainerAlloc();
    if (window->container == NULL) {
        return -1;
    }

    /* 传入的窗口位置是绝对值，现在使用后，变成偏移值 */
    int left = window->x;  
    int top = window->y;  
    window->x = 0;
    window->y = 0;

    if (left < 0) 
        left = 0;
    if (top < 0) 
        top = 0;

    /* 把视图的位置加上窗口区域的位置 */
    if (top < KGC_BAR_HEIGHT)
        top = KGC_BAR_HEIGHT;

    /* 由于窗口大小只是窗口显示的大小，除此之外，还应该包括一些边框或者标题栏。 */
    if (!window->style) {   /* 使用默认配置 */
        /* code */
        window->style = DEFAULT_KGC_WINDOW_STYLE;
    }
    
    /* 根据风格计算出窗口容器的大小 */
    int width = window->width;
    int height = window->height;
    
    /* 对风格进行判断 */
    if (window->style & KGC_WINSTYPLE_BORDER) {
        /* 有边框 */
        width += 2;
        height += 2;
        window->x = 1;
        window->y = 1;   
    }
    if (window->style & KGC_WINSTYPLE_CAPTION) {
        /* 有标题栏 */
        height += 24;
        window->y += 24;
    }
    
    KGC_ContainerInit(window->container, "window", left, top, width, height, window);
    /* 视图的位置根据窗口大小调整，居中设置 */
    //KGC_WindowDrawRectangle(window, 0, 0, window->width, window->height, KGCC_BLACK);
    KGC_WindowPaint(window);

    /* 当前窗口置为非激活 */
    if (GET_CURRENT_WINDOW())
        KGC_WindowPaintActive(GET_CURRENT_WINDOW(), 0);
    
    /* 激活新建窗口 */
    KGC_WindowPaintActive(window, 1);
    
    /* 新窗口放在最前面 */
    KGC_ContainerAtTopZ(window->container); 

    /* 把窗口添加到窗口链表 */
    ListAddTail(&window->list, &windowListHead);
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
    /* 从窗口队列中删除 */
    ListDel(&window->list);
    
    /* 释放窗口控制器 */
    KGC_TaskBarDelWindow(window);
    
    /* 销毁窗口容器 */
    KGC_ContainerFree(window->container);
    
    /* 解除绑定 */
    if (window->task) {
        /* 任务窗口为空 */
        Task_t *task = (Task_t *)window->task;
        /* 解除任务与窗口的绑定 */
        task->window = NULL;    
        /* 解除窗口与任务的绑定 */
        window->task = NULL;
    }
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

    /* 释放消息队列 */
    KGC_FreeMessageList(window);
    
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

    /* 窗口关闭的时候，需要将窗口隐藏 */
    KGC_ContainerZ(window->container, -1);
    /* 如果要关闭的窗口是当前窗口 */
    if (GET_CURRENT_WINDOW() == window)
        currentWindow = NULL;   /* 窗口关闭后，不指向窗口。 */
        
    /* 删除窗口 */
    if (!KGC_WindowDel(window)) {
        /* 销毁窗口 */
        if (!KGC_WindowDestroy(window)) {
            return 0;
        }
    }
    return -1;
}

#if 0
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

    /* 初始化窗口皮肤 */
    windowSkin.boderColor = KGCC_ARGB(255, 80, 80, 80);
    windowSkin.captionColor = KGCC_ARGB(255, 50, 50, 50);
    windowSkin.titleColor = KGCC_ARGB(255, 128, 128, 128);
    windowSkin.blankColor = KGCC_WHITE;
    windowSkin.boderActiveColor = KGCC_ARGB(255, 100, 100, 100);
    windowSkin.titleActiveColor = KGCC_WHITE;

#if KGC_WINDOW_MOVEMENT_SHRINK == 1
    /* 创建一个窗口移动时显示的容器 */
    windowMovement.walker = KGC_ContainerAlloc();
    /* 全屏大小 */
    KGC_ContainerInit(windowMovement.walker, "windowMovement", 0, 0, videoInfo.xResolution, 12, NULL);

    /* 全透明 */
    KGC_ContainerDrawRectangle(windowMovement.walker, 0, 0, videoInfo.xResolution, 12, KGCC_NONE);
    
    /* 隐藏 */    
    KGC_ContainerZ(windowMovement.walker, -1);
#endif /* KGC_WINDOW_MOVEMENT_SHRINK */

    /* 打开一个测试窗口任务 */
    //ThreadStart("window", 2, ThreadWindow, NULL);
    return 0;
}