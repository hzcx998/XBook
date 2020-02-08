/*
 * file:		include/system/graph.h
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形子系统 */
#ifndef _SYSTEM_GRAPH_H
#define _SYSTEM_GRAPH_H

#include <share/types.h>
#include <share/stdint.h>

/* 内核图形核心kernel graph core */
PUBLIC int InitGraphSystem();

PUBLIC void SysGraphWrite(int offset, int size, void *buffer);

PUBLIC void KGC_CoreDraw(uint32_t position, uint32_t area, void *buffer, uint32_t color);

#endif   /* _SYSTEM_GRAPH_H */
