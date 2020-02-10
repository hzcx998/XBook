/*
 * file:		include/book/atomic.h
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_ATOMIC_H
#define _BOOK_ATOMIC_H

#include <book/config.h>
#include <lib/types.h>
#include <book/arch.h>

EXTERN void __AtomicAdd(int *a, int b);
EXTERN void __AtomicSub(int *a, int b);
EXTERN void __AtomicInc(int *a);
EXTERN void __AtomicDec(int *a);
EXTERN void __AtomicOr(int *a, int b);
EXTERN void __AtomicAnd(int *a, int b);

/* 原子变量结构体 */
typedef struct {
   int value;   // 变量的值
}Atomic_t;


#define AtomicGet(atomic)	((atomic)->value)
#define AtomicSet(atomic,val)	(((atomic)->value) = (val))

#define ATOMIC_INIT(val)	{val}


/**
 * AtomicAdd - 原子加运算
 * @atomic: 原子结构
 * @value: 数值
 */
PRIVATE INLINE void AtomicAdd(Atomic_t *atomic, int value)
{
   __AtomicAdd(&atomic->value, value);
}

/**
 * AtomicSub - 原子减运算
 * @atomic: 原子结构
 * @value: 数值
 */
PRIVATE INLINE void AtomicSub(Atomic_t *atomic, int value)
{
   __AtomicSub(&atomic->value, value);
}


/**
 * AtomicInc - 原子增运算
 * @atomic: 原子结构
 */
PRIVATE INLINE void AtomicInc(Atomic_t *atomic)
{
   __AtomicInc(&atomic->value);
}

/**
 * AtomicDec - 原子减运算
 * @atomic: 原子结构
 */
PRIVATE INLINE void AtomicDec(Atomic_t *atomic)
{
   __AtomicDec(&atomic->value);
}

/**
 * AtomicSetMask - 设置位
 * @atomic: 原子结构
 * @mask: 位值
 */
PRIVATE INLINE void AtomicSetMask(Atomic_t *atomic, int mask)
{
   __AtomicOr(&atomic->value, mask);
}

/**
 * AtomicClearMask - 清除位
 * @atomic: 原子结构
 * @mask: 位值
 */
PRIVATE INLINE void AtomicClearMask(Atomic_t *atomic, int mask)
{
   __AtomicAnd(&atomic->value, ~mask);
}

#define ATOMIC_XCHG(v, new) (XCHG(&((v)->value), new))

#endif   /* _BOOK_ATOMIC_H */
