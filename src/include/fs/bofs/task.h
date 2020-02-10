/*
 * file:		include/fs/bofs/task.h
 * auther:		Jason Hu
 * time:		2019/12/9
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_TASK_H
#define _BOFS_TASK_H

#define BOFS_TASK_STATUS_LEN 24

/* 任务信息结构 */
typedef struct TaskInfo {
    pid_t pid;                          /* 任务的进程id */
    pid_t parentPid;                    /* 任务的父进程id */
    char status[BOFS_TASK_STATUS_LEN];  /* 状态字符串 */
    uint32_t elapsedTicks;              /* 已经运行了时间，ticks为单位 */
} TaskInfo_t;

#include <fs/bofs/super_block.h>

PUBLIC int BOFS_MakeTask(const char *pathname, pid_t pid, struct BOFS_SuperBlock *sb);

#endif  /* _BOFS_TASK_H */

