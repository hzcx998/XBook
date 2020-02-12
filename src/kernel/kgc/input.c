/*
 * file:		kernel/kgc/input.c
 * auther:		Jason Hu
 * time:		2020/2/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/* KGC-kernel graph core 内核图形核心 */

/* 系统内核 */
#include <book/config.h>

#include <book/arch.h>
#include <book/debug.h>
#include <book/bitops.h>
#include <book/kgc.h>

#include <lib/string.h>

/* 图形系统 */
#include <kgc/draw.h>
#include <kgc/input.h>
