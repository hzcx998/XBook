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
#include <kgc/window/draw.h>
#include <kgc/window/even.h>
#include <kgc/bar/taskbar.h>

EXTERN KGC_WindowMovement_t windowMovement;

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
    /* 遍历所有窗口 */
    KGC_Window_t *window;
    KGC_Container_t *container;
    int localX, localY;
    /* 每个窗口都检测鼠标按下 */
    int z;
    char inWindow = 0;
    for (z = -2; (container = KGC_ContainerFindByOffsetZ(z)) != NULL; z--) {
        window = container->private;
        if (window == NULL) {
            continue;
        }
        localX = mx - container->x;
        localY = my - container->y;
        /* 窗口事件监测 */
        KGC_WidgetMouseButtonDown(&container->widgetListHead, button,
            localX, localY);
        
        /* 如果在窗口里面 */
        if (mx >= container->x && my >= container->y && 
            mx < container->x + container->width &&
            my < container->y + container->height) {
            /* 如果在窗口内点击了，就设置为1 */
            inWindow = 1;      
            /* 如果是在标题栏，并且有标题栏，就尝试移动 */
            if (mx >= (container->x + 24 + 1) && my >= container->y && 
                mx < (container->x + container->width - (24*3 + 1)) &&
                my < (container->y + 25) && (window->style & KGC_WINSTYPLE_CAPTION)) {
                /* 进行按钮和菜单栏判断 */
                
                /* 如果没有点击到按钮和菜单栏，就会移动窗口 */
                if (windowMovement.holded == NULL) {
                    windowMovement.holded = window;
                    windowMovement.offsetX = localX;
                    windowMovement.offsetY = localY;
#if KGC_WINDOW_MOVEMENT_SHRINK == 1
                    
                    /* 先切换到选中的窗口 */
                    KGC_SwitchWindow(window);

                    /* 再打开walker */
                    KGC_WindowPaintMoving(windowMovement.walker, window, 1);
                    KGC_ContainerSlide(windowMovement.walker, mx - windowMovement.offsetX,
                        my - windowMovement.offsetY);
                    KGC_ContainerAtTopZ(windowMovement.walker);
#else
                    /* 切换窗口 */
                    KGC_SwitchWindow(window);
#endif /* KGC_WINDOW_MOVEMENT_SHRINK */
                    break;
                }
            } else {
                /* 只要鼠标在窗口范围内，就发送鼠标消息 */
                KGC_MessageNode_t *node = KGC_CreateMessageNode();
                if (node) {
                    node->message.type = KGC_MSG_MOUSE_BUTTON_DOWN;
                    node->message.mouse.button = button;
                    node->message.mouse.x = localX + window->x;
                    node->message.mouse.y = localX + window->y;
                    /* 上锁保护信息链表 */
                    KGC_AddMessageNode(node, window);
                }
                /* 如果不是当前窗口，才进行选择，如果已经是了，就不进行选择了 */
                if (GET_CURRENT_WINDOW() != window) {
                    /* 只是选择一个窗口 */
                    KGC_SwitchWindow(window);
                }
                break;
            }
        }
    }
    if (!inWindow) {
        /* 当不在窗口上点击，并且有当前窗口，才执行 */
        if (currentWindow) {
            /* 先把窗口设为非激活 */
            KGC_WindowPaintActive(currentWindow, 0);

            currentWindow = NULL;
            KGC_WindowControllerActive(NULL);
        }
    }

}

PUBLIC void KGC_WindowMouseUp(int button, int mx, int my)
{
    /* 遍历所有窗口 */
    KGC_Window_t *window;
    KGC_Container_t *container;
    
    int localX, localY;

    /* 每个窗口都检测鼠标按下 */
    int z;
    for (z = -2; (container = KGC_ContainerFindByOffsetZ(z)) != NULL; z--) {
        window = container->private;
        if (window == NULL) {
            continue;
        }
        localX = mx - container->x;
        localY = my - container->y;
        
        /* 窗口事件监测 */
        KGC_WidgetMouseButtonUp(&container->widgetListHead, button,
            localX, localY);
        
        /* 如果在窗口里面，没在标题栏上 */
        if (mx >= container->x && my >= container->y + 25 && 
            mx < container->x + container->width &&
            my < container->y + container->height) {
            /* 在窗口里面就发送消息给窗口 */
            KGC_MessageNode_t *node = KGC_CreateMessageNode();
            if (node) {
                node->message.type = KGC_MSG_MOUSE_BUTTON_UP;
                node->message.mouse.button = button;
                node->message.mouse.x = localX + window->x;
                node->message.mouse.y = localX + window->y;
                KGC_AddMessageNode(node, window);
            }
            break;
        }
        
    }
}

PUBLIC void KGC_WindowMouseMotion(int mx, int my)
{
    /* 遍历所有窗口 */
    KGC_Window_t *window;
    KGC_Container_t *container;
    int localX, localY;

    int z;
    for (z = -2; (container = KGC_ContainerFindByOffsetZ(z)) != NULL; z--) {
        window = container->private;
        if (window == NULL) {
            continue;
        }
        localX = mx - container->x;
        localY = my - container->y;
        
        /* 窗口事件监测 */
        KGC_WidgetMouseMotion(&container->widgetListHead,
            localX, localY);

        /* 如果在窗口里面，没在标题栏上 */
        if (mx >= container->x && my >= container->y + 25 && 
            mx < container->x + container->width &&
            my < container->y + container->height) {
            
            KGC_MessageNode_t *node = KGC_CreateMessageNode();
            if (node) {
                node->message.type = KGC_MSG_MOUSE_MOTION;
                node->message.mouse.x = localX + window->x;
                node->message.mouse.y = localX + window->y;
                KGC_AddMessageNode(node, window);
            }
            break;
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
