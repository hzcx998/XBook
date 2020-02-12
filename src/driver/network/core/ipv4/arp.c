/*
 * file:		network/core/ipv4/arp.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <book/spinlock.h>
#include <book/memcache.h>
#include <lib/string.h>
#include <lib/inet.h>
#include <net/ipv4/arp.h>
#include <net/ipv4/ethernet.h>
#include <net/network.h>
#include <clock/clock.h>

//#define _ARP_DEBUG

#define ARP_CACHE_LIVE_TIME    (5*60*HZ)    // 5分钟

#define ARP_TIMEOUT     1*HZ    /* 1秒 */
#define ARP_RETRY       3       /* 重试3次 */  


/* arp表，用于存放缓存的ip与mac的键值对 */
ArpCache_t *arpCacheTable;

/* 用于对缓存的存入和删除进行保护 */
Spinlock_t arpCacheLock;

/* 用于对对队列进行保护 */
Spinlock_t arpQueueLock;

/* arp队列链表头 */
struct List arpQueueList;

/**
 * ArpHeaderInit - ARP头部的初始化
 * @arp: arp头部
 * @hwType: 硬件类型
 * @protoType: 协议类型
 * @hwLen: 硬件地址长度
 * @protoLen: 协议地址长度
 * @opcode: 操作
 * @srcEthAddr: 源以太网（MAC）地址
 * @srcProtoAddr: 源协议（IP）地址
 * @dstEthAddr: 目的以太网（MAC）地址
 * @dstProtoAddr: 目的协议（IP）地址
 * 
 * 对头部成员赋值
 */
void ArpHeaderInit(ArpHeader_t *arp, uint16_t hwType, uint16_t protoType, 
        uint8_t hwLen, uint8_t protoLen,
        uint16_t opcode,
        uint8_t *srcEthAddr, uint32_t srcProtoAddr,
        uint8_t *dstEthAddr, uint32_t dstProtoAddr)
{
    arp->hardwareType = hwType;
    arp->protocolType = protoType;
    arp->hardwareAddresslen = hwLen;
    arp->protocolAddressLen = protoLen;
    arp->opcode = opcode;
    /* 复制物理地址 */
    memcpy(arp->sourceEthernetAddress, srcEthAddr, ETH_ADDR_LEN);
    arp->sourceProtocolAddress = srcProtoAddr;
    
    memcpy(arp->destEthernetAddress, dstEthAddr, ETH_ADDR_LEN);
    arp->destProtocolAddress = dstProtoAddr;
}

/**
 * ArpRequest - 请求IP地址对应的MAC地址
 * @ip: ip地址
 * 
 */
PUBLIC void ArpRequest(unsigned int ip)
{
    /* 内容全是0 */
    static unsigned char emptyMacAddr[ETH_ADDR_LEN] = {0x0};
    
    /* 内容全是0xff */
    static unsigned char broadcastMacAddr[ETH_ADDR_LEN] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    ArpHeader_t header;
    ArpHeaderInit(&header,
            ntohs(0x0001),                                  /* ethernet以太网 */
            ntohs(PROTO_IP),                                /* IPv4 */
            0x06,                                           /* 以太网地址长度 */
            0x04,                                           /* IPv4 地址长度 */
            ntohs(ARP_OP_REQUEST),                          /* 操作，发出请求 */
            EthernetGetAddress(),                           /* 自己的以太网地址 */
            ntohl(NetworkGetIpAddress()),                   /* 自己的IP地址 */
            emptyMacAddr,                                   /* 0，想要查询的以太网地址 */
            ntohl(ip)                                       /* 目的IP地址 */
    );
    
    size_t len = SIZEOF_ARP_HEADER;
    /* 分配网络缓冲区 */
    NetBuffer_t *buf = AllocNetBuffer(len);
    if (buf != NULL) {
        
        /* 填写网络缓冲区 */
        buf->dataLen = len;
        //printk("arp data len %d\n", buf->dataLen);
        //buf->data = (unsigned char *)buf + ASSUME_SIZEOF_NET_BUFFER;

        /* 复制ARP头部 */
        memcpy(buf->data, &header, len);
        
        /* 以太网发送数据，广播，ARP协议。 */
        EthernetSend(broadcastMacAddr, PROTO_ARP, 
                buf->data, buf->dataLen);
        /* 释放网络缓冲区 */
        FreeNetBuffer(buf);
    }
}

/**
 * ArpReceive - 受到一个ARP数据报
 * @ethAddr: 发送者的以太网地址
 * @buf: 数据缓冲区
 * 
 */
PUBLIC void ArpReceive(unsigned char *ethAddr, NetBuffer_t *buf)
{
    /* 如果数据太小，就直接返回 */
    if (buf->dataLen < SIZEOF_ARP_HEADER) {
        return;
    }
    /* 获取arp头部 */
    ArpHeader_t *header = (ArpHeader_t *)buf->data;
    
    /* 获取源IP地址，也就是发送者 */
    unsigned int sourceIP = htonl(header->sourceProtocolAddress);
    unsigned char *sourceIpInByte = (unsigned char *)(&sourceIP);

    /* 获取目标IP地址，也就是接受者，自己 */
    unsigned int destIP = htonl(header->destProtocolAddress);
    unsigned char *destIpInByte = (unsigned char *) (&destIP);
    
    /* 源以太网地址（MAC） */
    unsigned char sourceEthernet[ETH_ADDR_LEN];
    memcpy(sourceEthernet, header->sourceEthernetAddress, ETH_ADDR_LEN);
        
    /* 根据操作码，执行不同的操作 */
    switch (htons(header->opcode)) {
    case ARP_OP_REQUEST:
        printk(".REQ");
        
        /*printk("%d.%d.%d.%d want to know who have IP: %d.%d.%d.%d\n",
            sourceIpInByte[3], sourceIpInByte[2], sourceIpInByte[1], sourceIpInByte[0],
            destIpInByte[3], destIpInByte[2], destIpInByte[1], destIpInByte[0]);
        */
        /* 如果目标IP和自己的IP一样，也就是要请求本机的IP，那么发送一个“回复”给发送者（其它电脑）。 */
        if (destIP == NetworkGetIpAddress()) {
            printk(".ME");
        
            printk("oh, I have it\n");
            printk("%d.%d.%d.%d want to know who have IP: %d.%d.%d.%d\n",
            sourceIpInByte[3], sourceIpInByte[2], sourceIpInByte[1], sourceIpInByte[0],
            destIpInByte[3], destIpInByte[2], destIpInByte[1], destIpInByte[0]);
            
            /* 现在，源就是本机，目标就是原来的源地址（发送者） */
            ArpHeaderInit(header,
                    ntohs(0x0001),                                  /* ethernet */
                    ntohs(PROTO_IP),                                /* IPv4 */
                    0x06,                                           /* ethernet 地址长度 */
                    0x04,                                           /* IPv4 地址长度 */
                    ntohs(ARP_OP_REPLY),                            /* 回复操作 */
                    EthernetGetAddress(),                           /* 自己的MAC地址 */
                    ntohl(NetworkGetIpAddress()),                       /* 自己的IP地址 */
                    ethAddr,                                        /* 目标以太网（MAC）地址 */
                    ntohl(sourceIP)                                 /* 目标IP地址 */
            );
            
            printk("will send back!\n");
            //DumpArpHeader((ArpHeader_t *)header);

            /* 发送给之前的源以太网地址，ARP协议 */
            EthernetSend(ethAddr, PROTO_ARP, buf->data, buf->dataLen);
        } else {
            printk(".OTHER");
        
            //printk("host:%x not the dest that %x want to know!\n", NetworkGetIpAddress(), destIP);
        }
        break;
    case ARP_OP_REPLY:
        printk(".REP");
        
        printk("arp reply [%d.%d.%d.%d] -> [%2x:%2x:%2x:%2x:%2x:%2x]\n",
            sourceIpInByte[3], sourceIpInByte[2], sourceIpInByte[1], sourceIpInByte[0], 
            sourceEthernet[0], sourceEthernet[1], sourceEthernet[2], sourceEthernet[3], sourceEthernet[4], sourceEthernet[5]);
        
        /* 添加到缓存 */
        ArpAddCache(sourceIP, sourceEthernet);
        /* 处理等待队列中的ip地址对应的缓冲区 */
        ArpProcessWaitQueue(sourceIP, sourceEthernet);
        break;
    default:
        break;
    }
}

PUBLIC void ArpCacheInit(
    ArpCache_t *cache,
    uint32_t ip,
    uint8_t *mac)
{
    cache->ipAddress = ip;
    memcpy(cache->macAddress, mac, ETH_ADDR_LEN);
    cache->liveTime = ARP_CACHE_LIVE_TIME;
}

/**
 * ArpAddCache - 添加一个信息到缓存
 * @ip: ip地址
 * @mac: mac地址
 * 
 * 成功返回1，失败返回0
 */
PUBLIC int ArpAddCache(unsigned int ip, unsigned char *mac)
{
    ArpCache_t *cache = NULL;
    int i;
    unsigned long flags = SpinLockIrqSave(&arpCacheLock);
    for (i = 0; i < MAX_ARP_CACHE_NR; i++) {
        if (arpCacheTable[i].ipAddress == 0) {
            cache = &arpCacheTable[i];
            break;
        }
    }
    
    /* 如果找到了才初始化 */
    if (cache) {
        ArpCacheInit(cache, ip, mac);
        //printk("ARP cacheed:ip(%x):mac(%x:%x:%x:%x:%x:%x)\n", ip, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    SpinUnlockIrqSave(&arpCacheLock, flags);
    
    /* 如果缓存为空就返回0，不然就返回1 */
    return cache == NULL ? 0 : 1;
}

/**
 * ArpLookupCache - 添加一个信息到缓存
 * @ip: ip地址
 * @mac: mac地址
 * 
 * 成功返回1，失败返回0
 */
PUBLIC int ArpLookupCache(unsigned int ip, unsigned char *mac)
{
    
    ArpCache_t *cache = NULL;
    int i;
    unsigned long flags = SpinLockIrqSave(&arpCacheLock);

    for (i = 0; i < MAX_ARP_CACHE_NR; i++) {
        /* ip地址一样才找到 */
        if (arpCacheTable[i].ipAddress == ip) { 
            cache = &arpCacheTable[i];
            break;
        }
    }
    
    /* 如果找到了复制 */
    if (cache) {
        memcpy(mac, cache->macAddress, ETH_ADDR_LEN);
        //printk("ARP lookup:ip(%x):mac(%x:%x:%x:%x:%x:%x)\n", ip, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    SpinUnlockIrqSave(&arpCacheLock, flags);

    /* 如果缓存为空就返回0，不然就返回1 */
    return cache == NULL ? 0 : 1;
}

PUBLIC void DumpArpHeader(ArpHeader_t *header)
{
    printk(PART_TIP "ARP Header:");
    printk(PART_TIP "hardwareType:%x protocolType:%x\nhardwareAddresslen:%d protocolAddressLen:%d\n", 
            header->hardwareType, header->protocolType, header->hardwareAddresslen, header->protocolAddressLen);
    printk(PART_TIP "opcode:%x\n", header->opcode);
    
    DumpEthernetAddress(header->sourceEthernetAddress);
    DumpIpAddress(header->sourceProtocolAddress);
    
    DumpEthernetAddress(header->destEthernetAddress);
    DumpIpAddress(header->destProtocolAddress);
}

/**
 * ArpQueueInit - 初始化arp队列
 * 
 */
void ArpQueueInit(
    ArpQueue_t *queue,
    uint32_t ip,
    uint32_t retryTimes,
    Timer_t *timer)
{
    INIT_LIST_HEAD(&queue->list);
    queue->ipAddress = ip;
    queue->retryTimes = retryTimes;
    queue->timer = timer;
    INIT_LIST_HEAD(&queue->bufferList);
}


/**
 * ArpTimeout - arp请求超时
 * @data: 存放的是ip地址
 */
PRIVATE void ArpTimeout(uint32 data)
{
    //printk("!!!!time occur!!!!\n");
    ArpRequestTimeout(data);
}


/**
 * ArpRequestTimeout - arp请求超时
 * @ip: ip地址
 * 
 * 对请求队列中的所有缓冲区进行处理
 */
PUBLIC void ArpRequestTimeout(uint32 ip)
{
    printk("arp request timeout of ip: ");
    DumpIpAddress(ip);

    /* 上锁关中断 */
    
    unsigned long flags = SpinLockIrqSave(&arpQueueLock);

    ArpQueue_t *queue = NULL, *tmp;
    /* 遍历查找ip，是否已经在请求队列中了 */
    ListForEachOwner(tmp, &arpQueueList, list) {
        /* 如果找到一样的就退出 */
        if (tmp->ipAddress == ip) {
            queue = tmp;
            break;
        }
    }

    if (queue != NULL) {
        Timer_t *timer = queue->timer;

        /* 超时的时候，就删除定时器 */
        //RemoveTimer(timer);
        if (queue->retryTimes-- > 0) {
            /* 还有剩余次数，就再次发送请求 */
            TimerInit(timer, ARP_TIMEOUT, ip, ArpTimeout);
            AddTimer(timer);

            /* 发送一个arp请求 */
            ArpRequest(ip);
#ifdef _ARP_DEBUG            
            printk("arp retry, left retry time: %d\n", queue->retryTimes);
#endif    
        } else {
            /* 不能再次发送请求，就直接取消传输队列中的所有缓冲区 */
            //printk("[DROP]");
#ifdef _ARP_DEBUG  
            printk("[DROP]arp retry over, drop queue: %x\n", queue);
#endif
            NetBuffer_t *tmpbuf, *safe;
            /* 遍历每一个缓冲区，由于需要删除，所以用安全形式 */
            ListForEachOwnerSafe(tmpbuf, safe, &queue->bufferList, list) {
#ifdef _ARP_DEBUG                   
                printk("buf:%x ", tmpbuf);
#endif
                /* 从链表中删除 */
                ListDelInit(&tmpbuf->list);
                
                /* 释放缓冲区 */
                FreeNetBuffer(tmpbuf);

            }
            /* 释放队列定时器 */
            kfree(timer);

            /* 释放队列 */
            ListDel(&queue->list);
            kfree(queue);
        }
    }

    SpinUnlockIrqSave(&arpQueueLock, flags);
}

/**
 * ArpAddToWaitQueue - 把arp请求添加到等待队列
 * @ip: ip地址
 * @buf: 要发送的缓冲区
 *  
 */
PUBLIC void ArpAddToWaitQueue(uint32 ip, NetBuffer_t *buf)
{
    /*printk("add ip: ");
    DumpIpAddress(ip);
    *///printk(" buf: 0x%x\n", buf);

    /* 上锁关中断 */
    unsigned long flags = SpinLockIrqSave(&arpQueueLock);

    ArpQueue_t *queue = NULL, *tmp;
    /* 遍历查找ip，是否已经在请求队列中了 */
    ListForEachOwner(tmp, &arpQueueList, list) {
        /* 如果找到一样的就退出 */
        if (tmp->ipAddress == ip) {
            queue = tmp;
            break;
        }
    }

    /* 找到IP地址 */
    if (queue != NULL) {
        /* 把缓冲区添加到该队列里面 */
        ListAddTail(&buf->list, &queue->bufferList);
        SpinUnlockIrqSave(&arpQueueLock, flags);

    } else {
        /* 没有找到ip，那么就需要创建一个新的队列来保存 */
        Timer_t *timer = kmalloc(SIZEOF_TIMER, GFP_KERNEL);
        if (timer == NULL) {
            return;
        }

        TimerInit(timer, ARP_TIMEOUT, ip, ArpTimeout);

        /* 分配一个队列 */
        queue = kmalloc(SIZEOF_ARP_QUEUE, GFP_KERNEL);
        if (queue == NULL) {
            kfree(timer);
            return;
        }

        ArpQueueInit(queue, ip, ARP_RETRY, timer);

        /* 把缓冲区添加到该队列里面 */
        ListAddTail(&buf->list, &queue->bufferList);

        /* 把请求队列添加到arp请求队列 */
        ListAddTail(&queue->list, &arpQueueList);
        
        /* 把定时器添加到系统 */
        AddTimer(timer);
        printk(">>>send arp request!\n");
        SpinUnlockIrqSave(&arpQueueLock, flags);

        /* 发送一个arp请求 */
        ArpRequest(ip);
    }

    /* 现在已经确保缓冲区在某个队列中 */

    /* 遍历每一个队列 */
    ListForEachOwner(tmp, &arpQueueList, list) {
#ifdef _ARP_DEBUG
        printk("[ADD]ip: ");
        DumpIpAddress(tmp->ipAddress);
#endif
        NetBuffer_t *tmpbuf;
        /* 遍历每一个缓冲区 */
        ListForEachOwner(tmpbuf, &tmp->bufferList, list) {
#ifdef _ARP_DEBUG
            printk("buf:%x ", tmpbuf);
#endif
        }
    }
    //SpinUnlockRestoreInterrupt(&arpQueueLock, oldStatus);
    //while (1);
    
}

/**
 * ArpProcessWaitQueue - 处理等待队列
 * @ip: ip地址
 * @ethAddr: 以太网地址
 */
PUBLIC void ArpProcessWaitQueue(uint32 ip, uint8 *ethAddr)
{
    //printk("in processing...\n");
    /* 上锁关中断 */
    unsigned long flags = SpinLockIrqSave(&arpQueueLock);
    //printk("end processing\n");

    ArpQueue_t *queue = NULL, *tmp;
    /* 遍历查找ip，是否已经在请求队列中了 */
    ListForEachOwner(tmp, &arpQueueList, list) {
        /* 如果找到一样的就退出 */
        if (tmp->ipAddress == ip) {
            queue = tmp;
            break;
        }
    }
    
    if (queue != NULL) {
        printk("deal arp queue %x\n", queue);
        /* 对所有的缓冲区执行处理操作 */
        NetBuffer_t *tmpbuf, *safe;
        /* 遍历每一个缓冲区 */
        ListForEachOwnerSafe(tmpbuf, safe, &tmp->bufferList, list) {
#ifdef _ARP_DEBUG
            printk("buf:%x ", tmpbuf);
#endif
            printk("send to ethnet");
            DumpEthernetAddress(ethAddr);

            EthernetSend(ethAddr, PROTO_IP, tmpbuf->data, tmpbuf->dataLen);
            
            /* 释放缓冲区 */
            ListDelInit(&tmpbuf->list);
            FreeNetBuffer(tmpbuf);
        }
        
        /* 删除定时器 */
        RemoveTimer(queue->timer);
        kfree(queue->timer);

        /* 删除队列 */
        ListDel(&queue->list);
        kfree(queue);
    }

    SpinUnlockIrqSave(&arpQueueLock, flags);
    //printk("end processing\n");
}

/**
 * InitARP - 初始化ARP
 * 
 */
PUBLIC int InitARP()
{
    /* 初始化ARP缓存 */
    arpCacheTable = kmalloc(SIZEOF_ARP_CACHE * MAX_ARP_CACHE_NR, GFP_KERNEL);
    if (arpCacheTable == NULL) {
        return -1;
    }
    int i;
    for (i = 0; i < MAX_ARP_CACHE_NR; i++) {
        arpCacheTable[i].ipAddress = 0;
        memset(arpCacheTable[i].macAddress, 0, ETH_ADDR_LEN);
    }

    /* 初始化arp队列 */
    INIT_LIST_HEAD(&arpQueueList);

    return 0;
}
