/*
 * file:		include/book/sync_Lock.h
 * auther:		Jason Hu
 * time:		2019/7/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SYNC_LOCK_H
#define _BOOK_SYNC_LOCK_H


#include <book/config.h>
#include <share/types.h>
#include <share/const.h>
#include <book/list.h>
#include <book/semaphore.h>
#include <book/semaphore2.h>
#include <book/task.h>




struct SyncLock {
	struct Task *holder;			// 锁的持有者
	#ifdef CONFIG_SEMAPHORE_M
	struct Semaphore semaphore;		// 用多元信号量来实现锁
	#endif
	#ifdef CONFIG_SEMAPHORE_B
	struct Semaphore2 semaphore;		// 用二元信号量来实现锁	
	#endif
	unsigned int holderReaptCount;	// 锁的持有者重复申请锁的次数
};

PUBLIC void SyncLockInit(struct SyncLock *lock);
PUBLIC void SyncLockAcquire(struct SyncLock *lock);
PUBLIC void SyncLockRelease(struct SyncLock *lock);

#endif   /*_BOOK_SYNC_LOCK_H */
