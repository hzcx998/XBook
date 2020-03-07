/*
 * file:		include/lib/taskscan.h
 * auther:		Jason Hu
 * time:		2020/2/28
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
#ifndef _LIB_TASKSCAN_H
#define _LIB_TASKSCAN_H

#include "stdint.h"
#include "stddef.h"
#include "types.h"

/* 任务扫描状态 */
typedef struct taskscan_status {
    pid_t ts_pid;          /* 进程id */
    pid_t ts_ppid;         /* 父进程id */
    pid_t ts_gpid;         /* 组id */
    unsigned char ts_state;         /* 状态 */
    uint32_t ts_priority;  /* 优先级 */
    uint32_t ts_ticks;     /* 剩余ticks数 */
    uint32_t ts_runticks;  /* 运行的ticks数 */
    char ts_name[32];      /* 任务名字 */
} taskscan_status_t;

int taskscan(taskscan_status_t *ts, int *idx);

#endif  /* _LIB_TASKSCAN_H */