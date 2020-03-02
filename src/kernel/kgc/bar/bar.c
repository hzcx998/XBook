/*
 * file:		kernel/kgc/bar/bar.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <kgc/bar/bar.h>
#include <kgc/video.h>
#include <kgc/container/container.h>
#include <kgc/container/draw.h>
#include <kgc/controls/widget.h>
#include <kgc/window/window.h>
#include <kgc/window/switch.h>

KGC_Bar_t kgcbar;

/*
系统快捷键：
[可屏蔽] alt + tab, alt + shift + tab:
    切换任务
[不屏蔽] ctl + alt + delete:
    打开系统管理界面
*/


PUBLIC void KGC_BarDoTimer(int ticks)
{
    /*  */
    //printk("bar timer!\n");
    /* 更新时间 */
    KGC_MenuBarUpdateTime();   
}

PUBLIC void KGC_BarMouseDown(int button, int mx, int my)
{
    //printk("bar mouse down\n");
    /* 控件检测 */
    KGC_WidgetMouseButtonDown(&kgcbar.container->widgetListHead, button, mx, my);
    
}

PUBLIC void KGC_BarMouseUp(int button, int mx, int my)
{
    //printk("bar mouse up\n");
    /* 控件检测 */
    KGC_WidgetMouseButtonUp(&kgcbar.container->widgetListHead, button, mx, my);

}

PUBLIC void KGC_BarMouseMotion(int mx, int my)
{
    //printk("bar mouse motion\n");
    KGC_WidgetMouseMotion(&kgcbar.container->widgetListHead, mx, my);

}

PUBLIC int KGC_BarKeyDown(KGC_KeyboardEven_t *even)
{
    //printk("bar key down\n");
    /* 1.如果是不可屏蔽快捷键，处理后就立即返回不再继续处理按键的条件。 */
    if (even->keycode.code == KGCK_DELETE) {
        /* CTRL + ALT + DELETE: 打开系统管理界面 */
        if ((even->keycode.modify & KGC_KMOD_ALT) && 
            (even->keycode.modify & KGC_KMOD_CTRL)) {
            /* 系统管理界面 */
            
            return -1;  
        }
    }
    
    /* 2.如果是可屏蔽快捷键，查看屏蔽标志，根据屏蔽标志来判断是否继续处理按键 */
    
    if (even->keycode.code == KGCK_TAB) {
        /* CTRL + SHIFT + TAB: 向前切换任务 */
        if ((even->keycode.modify & KGC_KMOD_ALT) && 
            (even->keycode.modify & KGC_KMOD_SHIFT)) {
            if (GET_CURRENT_WINDOW()) {
                /* 查看当前窗口的可否获取系统快捷键 */
                if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS) 
                    return 0;
                
            } else {
                /* 没有屏蔽才能进行操作 */
                KGC_SwitchPrevWindowAuto();
                return -1;
            }
                
        /* CTRL + TAB: 向后切换任务 */
        } else if (even->keycode.modify & KGC_KMOD_ALT) {
            if (GET_CURRENT_WINDOW()) {
                /* 查看当前窗口的可否获取系统快捷键 */
                if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS) 
                    return 0;
                
            } else {
                /* 没有屏蔽才能进行操作 */
                KGC_SwitchNextWindowAuto();
                return -1;
            }
        }
    } else if (even->keycode.code == KGCK_ESCAPE) {
        if (GET_CURRENT_WINDOW()) {
            
            /* 查看当前窗口的可否获取系统快捷键 */
            if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS)
                return 0;
            
            /* 没有屏蔽才能进行操作 */
            KGC_SwitchNextWindowAuto();
        } else {
            /* 没有屏蔽才能进行操作 */
            KGC_SwitchNextWindowAuto();
            return -1;
        }
    }
    /* 3.非系统快捷键，返回继续处理按键 */
    return 0;
}

PUBLIC int KGC_BarKeyUp(KGC_KeyboardEven_t *even)
{
    //printk("bar key up\n");
    /* 1.如果是不可屏蔽快捷键，处理后就立即返回不再继续处理按键的条件。 */
    if (even->keycode.code == KGCK_DELETE) {
        /* CTRL + ALT + DELETE: 打开系统管理界面 */
        if ((even->keycode.modify & KGC_KMOD_ALT) && 
            (even->keycode.modify & KGC_KMOD_CTRL)) {
            /* 系统管理界面 */
            
            return -1;  
        }
    }

    /* 2.如果是可屏蔽快捷键，查看屏蔽标志，根据屏蔽标志来判断是否继续处理按键 */
    if (even->keycode.code == KGCK_TAB) {
        /* CTRL + SHIFT + TAB: 向前切换任务 */
        if ((even->keycode.modify & KGC_KMOD_ALT) && 
            (even->keycode.modify & KGC_KMOD_SHIFT)) {
            if (GET_CURRENT_WINDOW()) {   
        
                /* 查看当前窗口的可否获取系统快捷键 */
                if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS)
                    return 0;
            } else {
                /* 没有屏蔽才能进行操作 */
                return -1;
            }
                
        /* CTRL + TAB: 向后切换任务 */
        } else if (even->keycode.modify & KGC_KMOD_ALT) {
            if (GET_CURRENT_WINDOW()) {   
        
                /* 查看当前窗口的可否获取系统快捷键 */
                if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS)
                    return 0;
            } else {
                /* 没有屏蔽才能进行操作 */
                return -1;
            }
        }
    } else if (even->keycode.code == KGCK_SPACE) {
        if (GET_CURRENT_WINDOW()) {   
        
            /* 查看当前窗口的可否获取系统快捷键 */
            if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS)
                return 0;
        } else {
            /* 没有屏蔽才能进行操作 */
            return -1;
        }
    } else if (even->keycode.code == KGCK_ENTER) {
        if (GET_CURRENT_WINDOW()) {   
            /* 查看当前窗口的可否获取系统快捷键 */
            if (GET_CURRENT_WINDOW()->flags & KGC_WINDOW_FLAG_KEY_SHORTCUTS)
                return 0;
        } else {
            /* 没有屏蔽才能进行操作 */
            return -1;
        }
    }
    /* 3.非系统快捷键，返回继续处理按键 */
    return 0;   
}

void KGC_DrawBar(KGC_Container_t *container)
{
    KGC_ContainerDrawRectangle(container, 0, 0, 
        container->width, KGC_MENU_BAR_HEIGHT, KGC_MENU_BAR_COLOR);

    KGC_ContainerDrawRectangle(container, 0, KGC_MENU_BAR_HEIGHT, 
        container->width, KGC_TASK_BAR_HEIGHT, KGC_TASK_BAR_COLOR);

}

PUBLIC int KGC_BarPollEven()
{
    
    return 0;
}

PUBLIC int KGC_InitBarContainer()
{
    /* 菜单栏 */
    kgcbar.container = KGC_ContainerAlloc();
    if (kgcbar.container == NULL)
        return -1;
    KGC_ContainerInit(kgcbar.container, "bar", 0, 0, videoInfo.xResolution, KGC_BAR_HEIGHT);

    KGC_DrawBar(kgcbar.container);

    KGC_ContainerAtTopZ(kgcbar.container);

    KGC_ContainerDrawString(kgcbar.container, 0, 0, 
        "book", KGCC_WHITE);
    KGC_ContainerRefresh(kgcbar.container, 0, 0, 
        4*8, 16);
    
    KGC_InitMenuBar();
    KGC_InitTaskBar();
    return 0;
}
    