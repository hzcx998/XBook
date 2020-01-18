/*
 * file:		include/net/arp.h
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _NET_ARP_H
#define _NET_ARP_H

#include <share/stdint.h>
#include <share/types.h>

#include <net/netbuf.h>
#include <net/ethernet.h>

#include <book/timer.h>

/* ARP操作 */
#define ARP_OP_REQUEST  1   // 请求
#define ARP_OP_REPLY    2   // 回复

typedef struct ArpHeader {
    unsigned short hardwareType;        // 硬件类型
    unsigned short protocolType;        // 协议类型
    unsigned char hardwareAddresslen;   // 硬件地址长度，MAC地址占用6字节
    unsigned char protocolAddressLen;   // 协议地址长度，IP地址占用4字节
    unsigned short opcode;              // 操作码

    /* 发送端以太网地址 */
    unsigned char sourceEthernetAddress[ETH_ADDR_LEN];
    unsigned int sourceProtocolAddress; // 发送端IP地址
    
    /* 目的以太网地址 */
    unsigned char destEthernetAddress[ETH_ADDR_LEN];
    unsigned int destProtocolAddress;   // 目的IP地址

} PACKED ArpHeader_t;

/* ARP头部长度 */
#define SIZEOF_ARP_HEADER  sizeof(struct ArpHeader)


void ArpHeaderInit(ArpHeader_t *arp, uint16_t hwType, uint16_t protoType, 
        uint8_t hwLen, uint8_t protoLen,
        uint16_t opcode,
        uint8_t *srcEthAddr, uint32_t srcProtoAddr,
        uint8_t *dstEthAddr, uint32_t dstProtoAddr);

PUBLIC void ArpRequest(unsigned int ip);
PUBLIC void ArpReceive(unsigned char *ethAddr, NetBuffer_t *buf);

PUBLIC void DumpArpHeader(ArpHeader_t *header);

/* ARP缓存 */

#define MAX_ARP_CACHE_NR    128

typedef struct ArpCache {
    unsigned int ipAddress;                 /* IP地址 */
    unsigned char macAddress[ETH_ADDR_LEN]; /* MAC地址 */
    unsigned int liveTime;                  /* 生存时间 */
} ArpCache_t;

/* ARP头部长度 */
#define SIZEOF_ARP_CACHE  sizeof(struct ArpCache)

void ArpCacheInit(
    ArpCache_t *cache,
    uint32_t ip,
    uint8_t *mac
);

PUBLIC int InitARP();

PUBLIC int ArpAddCache(unsigned int ip, unsigned char *mac);
PUBLIC int ArpLookupCache(unsigned int ip, unsigned char *mac);


/* ARP队列 */
typedef struct ArpQueue {
    struct List list;       /* 队列链表，多个队列链接成一个链表 */
    uint32_t ipAddress;     /* ip地址 */
    uint8_t retryTimes;     /* 重试次数 */
    Timer_t *timer;         /* 定时器，用于请求超时控制 */
    struct List bufferList; /* 缓冲区链表，缓冲区挂载在此链表上 */
} ArpQueue_t;

#define SIZEOF_ARP_QUEUE sizeof(struct ArpQueue)

void ArpQueueInit(
    ArpQueue_t *queue,
    uint32_t ip,
    uint32_t retryTimes,
    Timer_t *timer
);

PUBLIC void ArpAddToWaitQueue(uint32 ip, NetBuffer_t *buf);
PUBLIC void ArpProcessWaitQueue(uint32 ip, uint8 *ethAddr);
PUBLIC void ArpRequestTimeout(uint32 ip);

#endif   /* _NET_ARP_H */
