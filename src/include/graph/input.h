/*
 * file:		include/graph/input.h
 * auther:		Jason Hu
 * time:		2020/2/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形输入 */
#ifndef _KGC_INPUT_H
#define _KGC_INPUT_H
/* KGC-kernel graph core 内核图形核心 */

#include <share/types.h>
#include <share/stdint.h>

/* 最多一次可以储存多少个鼠标数据包 */
#define MAX_MOUSE_PACKET_NR  12

/* 鼠标缓冲区 */
typedef struct KGC_InputMousePacket {
    uint8_t button;
    int16_t xIncrease;  /* 有符号 */
    int16_t yIncrease;
    int16_t zIncrease;
} PACKED KGC_InputMousePacket_t;

#endif   /* _KGC_INPUT_H */
