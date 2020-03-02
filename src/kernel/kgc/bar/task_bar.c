/*
 * file:		kernel/kgc/bar/task_bar.c
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>

#include <kgc/video.h>
#include <kgc/window/window.h>
#include <kgc/bar/taskbar.h>
#include <kgc/bar/bar.h>
#include <kgc/input/mouse.h>
#include <kgc/container/draw.h>
#include <kgc/desktop/desktop.h>

LIST_HEAD(windowControllerListHead);

/* 最大化数据 */
PRIVATE uint8_t wcRawData[26 * 32] = {

    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
    1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
    1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,2,1,1,1,2,2,2,2,1,1,2,2,2,2,1,1,1,0,
    1,1,1,1,1,1,1,2,1,2,1,1,2,1,1,1,2,1,2,1,1,1,2,1,1,0,
    1,1,1,1,1,1,1,2,1,2,1,1,2,1,1,1,2,1,2,1,1,1,2,1,1,0,
    1,1,1,1,1,1,2,1,1,1,2,1,2,1,1,1,2,1,2,1,1,1,2,1,1,0,
    1,1,1,1,1,1,2,2,2,2,2,1,2,2,2,2,1,1,2,2,2,2,1,1,1,0,
    1,1,1,1,1,1,2,1,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,0,
    1,1,1,1,1,1,2,1,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

PUBLIC void KGC_TaskBarHandler(KGC_Button_t *button)
{
    //printk("switch window\n");
    KGC_WindowController_t *controller = (KGC_WindowController_t *)button;
    /* 如果是窗口，就切换窗口 */
    if (controller->type == KGC_WINCTRL_WINDOW) {
        if (controller->window)
            KGC_SwitchWindow(controller->window);
    } else if (controller->type == KGC_WINCTRL_CONTAINER) {
        /* 如果是容器，就激活对应的容器，并把窗口置空 */
        KGC_WindowControllerActive(kgcbar.idle);
        /* 当前窗口置空 */
        currentWindow = NULL;
    }
}

PUBLIC KGC_WindowController_t *KGC_CreateWindowController()
{
    KGC_WindowController_t *controller = kmalloc(sizeof(KGC_WindowController_t), GFP_KERNEL);
    if (controller == NULL) {
        return NULL;
    }

    if (KGC_ButtonInit(&controller->button)) {
        kfree(controller);
        return NULL;
    }

    KGC_ButtonSetSize(&controller->button, KGC_TASK_BAR_SIZE, KGC_TASK_BAR_SIZE);
    KGC_ButtonSetName(&controller->button, "window controller");
    
    /* 设置按钮默认状态颜色 */
    controller->button.defaultColor = KGC_TASK_BAR_COLOR;

    KGC_ButtonSetImage(&controller->button, 26, 32, wcRawData, KGCC_BLACK, KGCC_WHITE);
    KGC_ButtonSetImageAlign(&controller->button, KGC_WIDGET_ALIGN_CENTER);

    INIT_LIST_HEAD(&controller->list);
    
    /* 默认控制的都是窗口 */
    controller->type = KGC_WINCTRL_WINDOW;
    return controller;
}

PUBLIC void KGC_WindowControllerAdd(KGC_WindowController_t *controller)
{
    KGC_ButtonSetLocation(&controller->button, 
        ListLength(&windowControllerListHead) * (KGC_TASK_BAR_SIZE + 2), KGC_MENU_BAR_HEIGHT);
    
    KGC_ButtonSetHandler(&controller->button, &KGC_TaskBarHandler);
    
    /* fifo */
    ListAddTail(&controller->list, &windowControllerListHead);
    
    KGC_ButtonAdd(&controller->button, kgcbar.container);
    
    KGC_ButtonShow(&controller->button.label.widget);
}

PUBLIC void KGC_WindowControllerBindWindow(KGC_WindowController_t *controller, KGC_Window_t *window)
{
    controller->window = window;
    controller->type = KGC_WINCTRL_WINDOW;
    
}

PUBLIC void KGC_WindowControllerBindContainer(KGC_WindowController_t *controller, KGC_Container_t *container)
{
    controller->container = container;   
    controller->type = KGC_WINCTRL_CONTAINER;
}

/**
 * KGC_TaskBarUpdateWindowMenuShow - 更新窗口在菜单栏中的显示
 * 
 */
PUBLIC void KGC_TaskBarUpdateWindowMenuShow(KGC_Window_t *window)
{
    /* 显示窗口信息 */
    KGC_LabelSetText(kgcbar.title, window->title);
    KGC_LabelShow(&kgcbar.title->widget);

    /* 更新图标，控制按钮等 */
}

/**
 * KGC_TaskBarUpdateDesktopMenuShow - 更新桌面菜单栏中的显示
 * 
 */
PUBLIC void KGC_TaskBarUpdateDesktopMenuShow(KGC_Container_t *container)
{
    /* 显示窗口信息 */
    KGC_LabelSetText(kgcbar.title, container->name);
    KGC_LabelShow(&kgcbar.title->widget);

    /* 更新图标，控制按钮等 */

}


/**
 * KGC_WindowControllerActive - 激活窗口控制器
 * @window: 要激活的窗口
 * 
 */
PUBLIC void KGC_WindowControllerActive(KGC_WindowController_t *controller)
{
    if (!controller)
        return;
    
    /* 激活控制器的时候，每个控制器都要检测 */
    KGC_WindowController_t *tmp;
    ListForEachOwner (tmp, &windowControllerListHead, list) {
        if (tmp == controller) {
            tmp->button.defaultColor = KGCC_ARGB(255, 128, 128, 128);

            tmp->button.label.backColor = tmp->button.defaultColor;
            tmp->button.label.widget.drawCounter = 0;
            KGC_ButtonShow(&tmp->button.label.widget);
        } else {
            tmp->button.defaultColor = KGC_TASK_BAR_COLOR;
            tmp->button.label.backColor = tmp->button.defaultColor;
            tmp->button.label.widget.drawCounter = 0;
            KGC_ButtonShow(&tmp->button.label.widget);
        }
    }
    /* 是窗口才显示窗口信息 */
    if (controller->type == KGC_WINCTRL_WINDOW) {
        KGC_TaskBarUpdateWindowMenuShow(controller->window);
    } else if (controller->type == KGC_WINCTRL_CONTAINER) {
        KGC_TaskBarUpdateDesktopMenuShow(controller->container);

        /* 当前窗口置为隐藏，选择桌面后，要把窗口隐藏才行。 */
        if (GET_CURRENT_WINDOW())
            KGC_ContainerZ(GET_CURRENT_WINDOW()->container, -1);
        
    }
}

/**
 * KGC_TaskBarSortController - 对控制器排序
 * 
 */
PUBLIC void KGC_TaskBarSortController()
{
    /* 先清空显示。链表长度+1，才是原来的数量，才能保证全部都刷新到 */
    KGC_ContainerDrawRectanglePlus(kgcbar.container, 0, KGC_MENU_BAR_HEIGHT,
        (ListLength(&windowControllerListHead) + 1) * (KGC_TASK_BAR_SIZE + 2),
        KGC_TASK_BAR_HEIGHT, KGC_TASK_BAR_COLOR);

    /* 从前往后，进行排序 */
    KGC_WindowController_t *controller;
    int n = 0;
    ListForEachOwner (controller, &windowControllerListHead, list) {
        /* 先设置位置 */
        KGC_ButtonSetLocation(&controller->button, 
            n * (KGC_TASK_BAR_SIZE + 2), KGC_MENU_BAR_HEIGHT);
        
        /* 再进行显示 */
        KGC_ButtonShow(&controller->button.label.widget);
        n++;
    }
}

/**
 * KGC_TaskBarAddWindow - 任务栏添加窗口
 * 
 * 当创建一个窗口的时候，需要在任务栏添加一个窗口控制器
 */
PUBLIC void KGC_TaskBarAddWindow(KGC_Window_t *window)
{
    /* 创建一个任务控制，和窗口绑定 */
    KGC_WindowController_t *controller = KGC_CreateWindowController();
    if (controller == NULL) {
        printk("create win con failed!\n");
        return;
    }
    KGC_WindowControllerAdd(controller);

    KGC_WindowControllerBindWindow(controller, window);

    /* 在调用前必须进行窗口绑定 */
    KGC_WindowControllerActive(controller);
}

/**
 * KGC_TaskBarDelWindow - 任务栏删除窗口
 * 
 * 当窗口关闭的时候，需要把任务栏中的窗口控制器关闭。
 */
PUBLIC int KGC_TaskBarDelWindow(KGC_Window_t *window)
{
    KGC_WindowController_t *controller = KGC_WindowControllerGet(window);
    if (controller == NULL)
        return -1;

    /* 删除一个控制器的时候，就切换到idel控制器 */
    KGC_WindowControllerActive(kgcbar.idle);

    KGC_WindowControllerDel(controller);
    
    KGC_WindowControllerDestroy(controller);

    /* 对所有控制器进行重新排序 */
    KGC_TaskBarSortController();

    return 0;
}

/**
 * WindowControllerGet - 通过窗口获取一个窗口控制器
 * @window: 窗口
 * 
 * 成功返回窗口控制器，失败返回NULL
 */
PUBLIC KGC_WindowController_t *KGC_WindowControllerGet(KGC_Window_t *window)
{
    KGC_WindowController_t *controller = NULL, *tmp;
    ListForEachOwner (tmp, &windowControllerListHead, list) {
        if (tmp->window == window) {
            controller = tmp;
            break;
        }
    }
    return controller;
}

PUBLIC void KGC_WindowControllerDel(KGC_WindowController_t *controller)
{
    KGC_Label_t *label = &controller->button.label;
    /* 清除控制器显示 */
    KGC_ContainerDrawRectanglePlus(kgcbar.container, label->x, label->y,
        label->width, label->height, KGC_TASK_BAR_COLOR);
    /* 删除按钮 */
    KGC_ButtonDel(&controller->button);
    /* 脱离控制器链表 */
    ListDel(&controller->list);
}

PUBLIC void KGC_WindowControllerDestroy(KGC_WindowController_t *controller)
{
    /* 销毁按钮 */
    KGC_ButtonDestroySub(&controller->button);

    /* 销毁控制器 */
    kfree(controller);
}

PUBLIC int KGC_InitTaskBar()
{
    /* 创建哨兵窗口控制器 */
    kgcbar.idle = KGC_CreateWindowController();
    if (kgcbar.idle == NULL) {
        printk("create win con failed!\n");
    }
    /* 绑定桌面 */
    KGC_WindowControllerBindContainer(kgcbar.idle, kgcDesktop.container);
    KGC_WindowControllerAdd(kgcbar.idle);
    /* 激活控制器 */
    KGC_WindowControllerActive(kgcbar.idle);

    return 0;
}