/*
 * file:		network/core/netbuf.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <book/spinlock.h>
#include <book/interrupt.h>

#include <lib/string.h>

#include <net/netbuf.h>

/* 网络缓冲是基于二次分配的，这样可以提高分配缓冲的速度 */
NetBuffer_t *netBufferTable;

/* 保护缓冲区的分配与释放 */
Spinlock_t netBufferLock;

/**
 * AllocNetBuffer - 分配网络缓冲区
 * @len: 要分配的长度
 * 
 * 成功返回缓冲区，失败返回NULL
 */
PUBLIC NetBuffer_t *AllocNetBuffer(size_t len)
{
    if (len > NET_BUF_DATA_SIZE) {
        return NULL;
    }

    enum InterruptStatus oldStatus =  SpinLockSaveIntrrupt(&netBufferLock);

    NetBuffer_t *buf = NULL;
    int i;
    /* 遍历查找 */
    for (i = 0; i < MAX_NET_BUF_NR; i++) {
        if (netBufferTable[i].status == NET_BUF_UNUSED) {
            netBufferTable[i].status = NET_BUF_USING;
            buf = &netBufferTable[i];
            buf->data = (unsigned char *)buf + ASSUME_SIZEOF_NET_BUFFER;
            memset(buf->data, 0, NET_BUF_DATA_SIZE);
            //printk("[-]Net Buffer Alloc At %x\n", buf);
            break;
        }
    }
    SpinUnlockRestoreInterrupt(&netBufferLock, oldStatus);

    return buf;
}

/**
 * FreeNetBuffer - 释放网络缓冲区
 * @buf: 要释放的缓冲区
 */
PUBLIC void FreeNetBuffer(NetBuffer_t *buf)
{
    enum InterruptStatus oldStatus =  SpinLockSaveIntrrupt(&netBufferLock);

    /* 修改状态为未使用 */
    buf->status = NET_BUF_UNUSED;

    //printk("[-]Net Buffer Free At %x\n", buf);

    SpinUnlockRestoreInterrupt(&netBufferLock, oldStatus);
    
}

/**
 * InitNetBuffer - 初始化网络缓冲区
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int InitNetBuffer()
{
    /* 初始化自旋锁 */
    SpinLockInit(&netBufferLock);

    /* 分配缓冲区 */
    netBufferTable = (NetBuffer_t *)kmalloc(NET_BUF_SIZE * MAX_NET_BUF_NR, GFP_KERNEL);
    if (netBufferTable == NULL) {
        return -1;
    }

    int i;
    /* 初始化每一个网络缓冲 */
    for (i = 0; i < MAX_NET_BUF_NR; i++) {
        netBufferTable[i].status = NET_BUF_UNUSED;  /* 处于未使用状态 */
        netBufferTable[i].data = NULL;
        netBufferTable[i].dataLen = 0;
        INIT_LIST_HEAD(&netBufferTable[i].list);
    }

    return 0;
}