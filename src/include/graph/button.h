/*
 * file:		include/graph/button.h
 * auther:		Jason Hu
 * time:		2020/2/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 鼠标按钮 */
#ifndef _KGC_BUTTON_H
#define _KGC_BUTTON_H

#include <share/types.h>
#include <share/stdint.h>
/* KGC-kernel graph core 内核图形核心 */

/* 鼠标按钮 */
enum KGC_MouseButton {
    KGC_MOUSE_LEFT        = 0x01, /* 鼠标左键 */
    KGC_MOUSE_RIGHT       = 0x02, /* 鼠标右键 */
    KGC_MOUSE_MIDDLE      = 0x04, /* 鼠标中键 */
};

#endif   /* _KGC_BUTTON_H */
