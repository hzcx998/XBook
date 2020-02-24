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
#include <kgc/window/window.h>
#include <kgc/bar/bar.h>
#include <kgc/container/container.h>

EXTERN struct List windowListHead;

/**
 * KGC_WindowSwitch - 切换窗口
 * @window: 窗口
 * 
 * 切换到指定窗口
 */
PUBLIC void KGC_SwitchWindow(KGC_Window_t *window)
{
    if (!window)
        return;
    
    /* 当前窗口置为隐藏 */
    if (GET_CURRENT_WINDOW())
        KGC_ContainerZ(GET_CURRENT_WINDOW()->container, -1);
    
    /* 切换后地窗口放到顶部下面 */
    KGC_ContainerBelowTopZ(window->container);
    /* 设置当前窗口 */
    SET_CURRENT_WINDOW(window);

    /* 激活控制窗口 */
    KGC_WindowControllerActive(KGC_WindowControllerGet(window));
}

/**
 * KGC_SwitchFirstWindow - 切换到第一个窗口
 * 
 */
PUBLIC void KGC_SwitchFirstWindow()
{
    
    /* 获取首窗口 */
    KGC_Window_t *window = ListFirstOwnerOrNull(&windowListHead, struct KGC_Window, list);
    /* 切换窗口，如果是NULL也没问题 */
    KGC_SwitchWindow(window);
}

/**
 * KGC_SwitchLastWindow - 切换到最后一个窗口
 * 
 */
PUBLIC void KGC_SwitchLastWindow()
{
    
    /* 获取首窗口 */
    KGC_Window_t *window = ListLastOwnerOrNull(&windowListHead, struct KGC_Window, list);
    /* 如果相同就不切换 */
    if (currentWindow == window)
        return;
    /* 切换窗口，如果是NULL也没问题 */
    KGC_SwitchWindow(window);
}

/**
 * KGC_SwitchNextWindow - 切换到下一个窗口
 * @window: 当前窗口
 * 
 */
PUBLIC void KGC_SwitchNextWindow(KGC_Window_t *window)
{
    if (!window)
        return;
    
    /* 获取下个窗口 */
    KGC_Window_t *win;
    struct List *list, *tmp = window->list.next;
    do {
        list = tmp;
        tmp = tmp->next;
    } while (list == &windowListHead);
    
    win = ListOwner(list, struct KGC_Window, list);

    /* 如果相同就不切换 */
    if (win == window)
        return;
    /* 切换窗口，如果是NULL也没问题 */
    KGC_SwitchWindow(win);
}
/**
 * KGC_SwitchNextWindow - 切换到上一个窗口
 * @window: 当前窗口
 * 
 */
PUBLIC void KGC_SwitchPrevWindow(KGC_Window_t *window)
{
    if (!window)
        return;
    
    /* 获取下个窗口 */
    KGC_Window_t *win;
    struct List *list, *tmp = window->list.prev;
    do {
        list = tmp;
        tmp = tmp->prev;
    } while (list == &windowListHead);
    
    win = ListOwner(list, struct KGC_Window, list);

    /* 如果相同就不切换 */
    if (win == window)
        return;
    /* 切换窗口，如果是NULL也没问题 */
    KGC_SwitchWindow(win);
}

/**
 * KGC_SwitchNextWindowAuto - 切换到下一个自动选择的窗口
 * 
 */
PUBLIC void KGC_SwitchNextWindowAuto()
{
    /* 如果当前窗口是空，说明有可能是才执行完关闭窗口，被置空 */
    if (currentWindow == NULL) {
        printk("current window is null\n");
        
        /* 如果还有窗口，那么就可以进行切换 */
        if (ListLength(&windowListHead) > 0)
            KGC_SwitchFirstWindow();

        return;
    } 
        
    /* 获取下个窗口 */
    KGC_Window_t *win;
    struct List *list, *tmp = currentWindow->list.next;
    do {
        list = tmp;
        tmp = tmp->next;
    } while (list == &windowListHead);
    
    win = ListOwner(list, struct KGC_Window, list);
    /*
    printk("switch from %x to %x\n", currentWindow, win);
    printk("window %d:%d, %d:%d\n", win->x, win->y, win->width, win->height);
    KGC_View_t *v = win->view;
    printk("view %x %d:%d, %d:%d\n", v, v->x, v->y, v->width, v->height);
    */
    /* 如果相同就不切换 */
    if (win == currentWindow)
        return;
    /* 切换窗口，如果是NULL也没问题 */
    KGC_SwitchWindow(win);
}

/**
 * KGC_SwitchNextWindowAuto - 切换到上一个自动选择的窗口
 * 
 */
PUBLIC void KGC_SwitchPrevWindowAuto()
{
    /* 如果当前窗口是空，说明有可能是才执行完关闭窗口，被置空 */
    if (currentWindow == NULL) {
        printk("current window is null\n");
        /* 如果还有窗口，那么就可以进行切换 */
        if (ListLength(&windowListHead) > 0)
            KGC_SwitchLastWindow();
        return;
    } 

    /* 获取上个窗口 */
    KGC_Window_t *win;
    struct List *list, *tmp = currentWindow->list.prev;
    do {
        list = tmp;
        tmp = tmp->prev;
    } while (list == &windowListHead);
    
    win = ListOwner(list, struct KGC_Window, list);
    /*
    printk("switch from %x to %x\n", currentWindow, win);
    printk("window %d:%d, %d:%d\n", win->x, win->y, win->width, win->height);
    */
    /* 如果相同就不切换 */
    if (win == currentWindow)
        return;
    /* 切换窗口，如果是NULL也没问题 */
    KGC_SwitchWindow(win);
}
