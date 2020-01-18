/*
 * file:		include/net/netbuf.h
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*
网络层之间传输数据的缓冲区
*/

#ifndef _NET_NETBUF_H
#define _NET_NETBUF_H

#include <book/list.h>

#include <share/stdint.h>
#include <share/types.h>

/* 状态 */
#define NET_BUF_UNUSED          0   /* 未使用 */
#define NET_BUF_USING           1   /* 使用中 */


#define NET_BUF_SIZE        2048

/* 约定网络缓冲结构大小 */
#define ASSUME_SIZEOF_NET_BUFFER    24   


/* NetBuffer最多占用24字节，所以，这里数据区域就是2048-24 */
#define NET_BUF_DATA_SIZE   (NET_BUF_SIZE - ASSUME_SIZEOF_NET_BUFFER)
/* 最大的NetBuffer数量，2048*64=128kb */
#define MAX_NET_BUF_NR       64
 
/* 约定24字节 */
typedef struct NetBuffer {
    struct List list;               /* 缓冲区链表，占8字节 */
    unsigned int status;            /* 缓冲区的状态 */          
    unsigned int dataLen;           /* 实际拥有的数据长度 */
    unsigned char *data;            /* 实际数据的指针 */
    unsigned char pad0[ASSUME_SIZEOF_NET_BUFFER-20];
    unsigned char pad[NET_BUF_SIZE-ASSUME_SIZEOF_NET_BUFFER];  /* 要用总大小-结构大小 */
} PACKED NetBuffer_t;


PUBLIC int InitNetBuffer();
PUBLIC void FreeNetBuffer(NetBuffer_t *buf);
PUBLIC NetBuffer_t *AllocNetBuffer(size_t len);

#endif   /* _NET_NETBUF_H */
