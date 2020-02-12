/*
 * file:		kernel/kgc/even.c
 * auther:		Jason Hu
 * time:		2020/2/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/* KGC-kernel graph core 内核图形核心 */

/* 系统内核 */
#include <book/config.h>

#include <book/arch.h>
#include <book/debug.h>
#include <lib/string.h>

/* 图形系统 */
#include <kgc/even.h>
#include <kgc/button.h>

/* 记录鼠标上次状态 */
struct KGC_MouseRecord {
    u8 left;
    u8 right;
    u8 middle;
};

PRIVATE struct KGC_MouseRecord mouseRecord = {0,0,0};

/**
 * KGC_EvenKeyboardKey - 对键盘键值的处理
 * 
 */
PUBLIC void KGC_EvenKeyboardKey(int key, KGC_KeyboardEven_t *even)
{
    /* 默认是没有 */
    even->type = KGC_NOEVENT;
    even->state = 0;

    /* 按键类型 */
    if (key & KBD_FLAG_BREAK) {
        even->type = KGC_KEY_UP;
        even->state = KGC_PRESSED;
    } else {
        even->type = KGC_KEY_DOWN;
        even->state = KGC_RELEASED;
    }
    /* 检测修饰符 */
    if (key & KBD_FLAG_SHIFT_L) {
        even->keycode.modify |= KGC_KMOD_LSHIFT;
    }
    if (key & KBD_FLAG_SHIFT_R) {
        even->keycode.modify |= KGC_KMOD_RSHIFT;
    }
    if (key & KBD_FLAG_CTRL_L) {
        even->keycode.modify |= KGC_KMOD_LCTRL;
    }
    if (key & KBD_FLAG_CTRL_R) {
        even->keycode.modify |= KGC_KMOD_RCTRL;
    }
    if (key & KBD_FLAG_ALT_L) {
        even->keycode.modify |= KGC_KMOD_LALT;
    }
    if (key & KBD_FLAG_ALT_R) {
        even->keycode.modify |= KGC_KMOD_RALT;
    }
    if (key & KBD_FLAG_PAD) {
        even->keycode.modify |= KGC_KMOD_PAD;
    }
    
    if (key & KBD_FLAG_NUM) {
        even->keycode.modify |= KGC_KMOD_NUM;
    }
    if (key & KBD_FLAG_CAPS) {
        even->keycode.modify |= KGC_KMOD_CAPS;
    }

    /* 除去修饰符后原来的键值 */
    even->keycode.scanCode = key & KBD_KEY_MASK;
    
    /* 获取按键码 */
    key &= KBD_KEY_MASK;
    even->keycode.code =  key & (~KBD_FLAG_BREAK);

#ifdef DEBUG_KEYBOARD
    printk("keyboard even: type-%d, scancode-%x keycode-%x|%c modify-%x\n",
        even->type,even->keycode.scanCode,even->keycode.code,even->keycode.code, even->keycode.modify);
#endif  /* DEBUG_KEYBOARD */
    
}   

/**
 * KGC_EvenMouseMotion - 对鼠标移动的处理
 * 
 */
PUBLIC void KGC_EvenMouseMotion(int32_t x, int32_t y, int32_t z, KGC_MouseMotionEven_t *even)
{
    even->type = KGC_NOEVENT;
    even->x = 0;
    even->y = 0;
        
    /* 如果有一个是变化的，那么就说明鼠标移动了 */
    if (x || y) {
        even->type = KGC_MOUSE_MOTION;
        even->x = x;
        even->y = y;
    }
}

/**
 * KGC_EvenMouseButton - 对鼠标按键的处理
 * 
 */
PUBLIC void KGC_EvenMouseButton(uint8_t button, KGC_MouseButtonEven_t *even)
{   
    /* 默认是没有 */
    even->type = KGC_NOEVENT;
    even->state = KGC_PRESSED;

    /*
    如果左键按下，那么就是down，left
    如果此时右键也按下，那么就是down，left，right，
    如果此时左键弹起，那么就是up，left，down，right
    ！鼠标按钮的变化，需要根据上一个按钮状态来判断！
    */

    /* 对是否按压做判断 */
    if ((button & 0x01) || (button & 0x02) || (button & 0x04)) {
        /* 只要有一个按扭，就算是按压 */
        even->state = KGC_PRESSED;
    } else {
        even->state = KGC_RELEASED;
    }

    /* 左键按下 */
    if (button & 0x01) {
        /* 如果没有按下，才可以按下 */
        if (!mouseRecord.left) {
            even->type = KGC_MOUSE_BUTTON_DOWN;
            /* 按下的时候，可以是多个按键，所以用或 */
            even->button |= KGC_MOUSE_LEFT;   /* 左键 */
            mouseRecord.left = 1;
        }
    } else {
        /* 如果上次是按下，这次又没有按下，就是弹起 */
        if (mouseRecord.left) {
            even->type = KGC_MOUSE_BUTTON_UP;
            /* 弹起的时候只检测1个按键 */
            even->button = KGC_MOUSE_LEFT;   /* 左键 */

            mouseRecord.left = 0; 
            
            /* 打印事件状态 */
            //printk("type:%x state:%x button:%x\n", even->type, even->state, even->button);  
            return; /* 如果有按键弹起就直接返回，不再往后面检测 */         
        }
    } 
    /* 右键按下 */
    if ((button & 0x02)) {
        /* 如果没有按下，才可以按下 */
        if (!mouseRecord.right) {
            even->type = KGC_MOUSE_BUTTON_DOWN;
            /* 按下的时候，可以是多个按键，所以用或 */
            
            even->button |= KGC_MOUSE_RIGHT;   /* 右键 */
            mouseRecord.right = 1;    
        }
    } else {
        /* 如果上次是按下，这次又没有按下，就是弹起 */
        if (mouseRecord.right) {
            even->type = KGC_MOUSE_BUTTON_UP;
            /* 弹起的时候只检测1个按键 */
            even->button = KGC_MOUSE_RIGHT;   /* 右键 */
        
            mouseRecord.right = 0;

            /* 打印事件状态 */
            //printk("type:%x state:%x button:%x\n", even->type, even->state, even->button);      
            return;   /* 如果有按键弹起就直接返回，不再往后面检测 */         
           
        }
    } 
    /* 中键按下 */
    if ((button & 0x04)) {
        /* 如果没有按下，才可以按下 */
        if (!mouseRecord.middle) {
            even->type = KGC_MOUSE_BUTTON_DOWN;
            /* 按下的时候，可以是多个按键，所以用或 */
            
            even->button |= KGC_MOUSE_MIDDLE;   /* 中键 */
            mouseRecord.middle = 1;    
        }
    } else {
        /* 如果上次是按下，这次又没有按下，就是弹起 */
        if (mouseRecord.middle) {
            even->type = KGC_MOUSE_BUTTON_UP;
            /* 弹起的时候只检测1个按键 */
            even->button = KGC_MOUSE_MIDDLE;   /* 中键 */
        
            mouseRecord.middle = 0;

            /* 打印事件状态 */
            //printk("type:%x state:%x button:%x\n", even->type, even->state, even->button);      
            return;   /* 如果有按键弹起就直接返回，不再往后面检测 */         
           
        }
    } 
    /* 打印事件状态 */
    //printk("type:%x state:%x button:%x\n", even->type, even->state, even->button);
}   
