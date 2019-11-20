/*
 * file:		include/book/synclock.h
 * auther:		Jason Hu
 * time:		2019/7/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*
同步锁：
    用于同步，让进程可以有序地执行
*/
#ifndef _BOOK_SYNCLOCK_H
#define _BOOK_SYNCLOCK_H

#include <book/config.h>
#include <share/types.h>
#include <share/const.h>
#include <book/list.h>
#include <book/semaphore.h>
#include <book/semaphore2.h>
#include <book/task.h>

typedef struct Synclock {
	struct Task *holder;			// 锁的持有者
	#ifdef CONFIG_SEMAPHORE_M
	struct Semaphore semaphore;		// 用多元信号量来实现锁
	#endif
	#ifdef CONFIG_SEMAPHORE_B
	struct Semaphore2 semaphore;		// 用二元信号量来实现锁	
	#endif
	unsigned int holderReaptCount;	// 锁的持有者重复申请锁的次数
} Synclock_t;

PUBLIC void SynclockInit(struct Synclock *lock);
PUBLIC void SyncLock(struct Synclock *lock);
PUBLIC void SyncUnlock(struct Synclock *lock);

#endif   /*_BOOK_SYNCLOCK_H */
