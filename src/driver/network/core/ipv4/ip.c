/*
 * file:		network/core/ipv4/ip.c
 * auther:		Jason Hu
 * time:		2020/1/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/spinlock.h>
#include <book/interrupt.h>
#include <lib/string.h>
#include <lib/inet.h>

#include <net/ipv4/ip.h>
#include <net/network.h>
#include <net/ipv4/ethernet.h>
#include <net/ipv4/arp.h>
#include <net/ipv4/icmp.h>

//#define _IP_DEBUG   

/* 数据包的生存时间（time to live），默认是32或64 */
#define IP_TTL  32


/* 每发送一个ip报文，就会给报文分配一个不一样的id值 */
PRIVATE uint32_t ipNextID;

/**
 * IpHeaderInit - 初始化IP头
 * 
 */
PUBLIC void IpHeaderInit(
    IpHeader_t *header,
    uint8_t headerLen,
    uint8_t version,
    uint8_t tos,
    uint16_t totalLen,
    uint16_t id,
    uint16_t offset,
    uint8_t ttl,
    uint8_t protocol,
    uint16_t checkSum, 
    uint32_t sourceIP,
    uint32_t destIP)
{
    header->headerLen = headerLen;
    header->version = version;
    header->tos = tos;
    header->totalLen = totalLen;
    header->id = id;
    header->offset = offset;
    header->ttl = ttl;
    header->protocol = protocol;
    header->checkSum = checkSum;
    header->sourceIP = sourceIP;
    header->destIP = destIP;
}

/**
 * IpIsBroadcast - 判断ip地址是否是广播地址
 * @ip: 要检测的ip地址
 * 
 * 是广播地址返回true，不是则返回false
 */
PUBLIC bool IpIsBroadcast(uint32_t ip)
{
    /* 如果是0xffffffff或者0x00000000，说明是广播 */
    if (ip == 0xffffffff || ip == 0x00000000) {
        return true;
    }
    /* 获取子网掩码 */
    uint32_t mask = NetworkGetSubnetMask();
    
    /* 如果ip的子网部分每个位都是1，说明也是广播 */
    if ((ip & ~mask) == (0xffffffff & ~mask)) {
        return true;
    }

    return false;
}

/**
 * IpCheckIp - 检测ip是否支持
 * @ip: ip地址
 * 
 * 支持则返回true，不支持则返回false
 */
PUBLIC bool IpCheckIp(uint32_t ip)
{
    if (IpIsBroadcast(ip)) {
        printk("DO NOT support broadcast now!\n");
        return false;
    }
    
    if (ip == NetworkGetIpAddress()) {
        printk("DO NOT support loopback now!\n");
        return false;
    }
    
    return true;
}

/**
 * IpIsSameSubnet - 检测IP地址是否有相同的子网
 * @ip1: ip地址1
 * @ip2: ip地址2
 * @mask: 子网掩码
 * 
 * 是则返回true，不是则返回false
 */
PUBLIC bool IpIsSameSubnet(uint32_t ip1, uint32_t ip2, uint32_t mask)
{
    return (ip1 & mask) == (ip2 & mask);
}


/**
 * IpTransmit - IP数据报传输
 * @ip: ip地址
 * @data: 数据
 * @len: 数据长
 * @protocol: 协议
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int IpTransmit(uint32 ip, uint8 *data, uint32 len, uint8 protocol)
{
    /* 检测IP是否可以传输 */
    if (!IpCheckIp(ip)) {
        return -1;
    }
    
    uint32 destIP = ip;
    
    /* 
    如果目的ip和自己是在同一个子网，就直接发送到目的ip
    如果目的ip和自己不是在同一个子网，就发送到默认网关
     */
    if (!IpIsSameSubnet(ip, NetworkGetIpAddress(), NetworkGetSubnetMask())) {
        destIP = NetworkGetGateway();
        printk("send to gate way!\n");
    }
    
    uint32 total = len + SIZEOF_IP_HEADER;

    NetBuffer_t *buffer = AllocNetBuffer(total);
    if (buffer == NULL) {
        printk("allocate net buffer failed!\n");
        return - 1;
    }

    IpHeader_t header;
    IpHeaderInit(&header,
            SIZEOF_IP_HEADER / 4,                   /* header len */
            IP_VERSION4,                            /* version, IPv4是4，IPV6是6 */
            0,                                      /* tos */
            htons(total),                           /* total len */
            htons(ipNextID++),                      /* id */
            htons(0),                               /* offset, don't fragment */
            IP_TTL,                                 /* ttl */
            protocol,                               /* protocol */
            0,                                      /* check sum */
            htonl(NetworkGetIpAddress()),           /* source ip */
            htonl(ip));                             /* dest ip */

    /* 计算校验和 */
    header.checkSum = NetworkCheckSum((uint8 *) (&header), SIZEOF_IP_HEADER);
    
    buffer->dataLen = total;

    /* 复制IP头部 */
    uint8 *p = buffer->data;
    memcpy(p, &header, SIZEOF_IP_HEADER);
    
    /* 复制IP数据报数据 */
    p += SIZEOF_IP_HEADER;
    memcpy(p, data, len);
    
    uint8 ethAddr[ETH_ADDR_LEN] = {0};
    if (ArpLookupCache(destIP, ethAddr)) {
        //printk("find mac addr of ip in arp cache\n");
        EthernetSend(ethAddr, PROTO_IP, buffer->data, buffer->dataLen);
        FreeNetBuffer(buffer);
    } else {
        printk("not find mac addr of ip in arp cache\n");
        /* 添加到arp等待队列 */
        ArpAddToWaitQueue(ip, buffer);
    }
    
    return 0;
}

PUBLIC int IpReceive(NetBuffer_t *buf)
{
    IpHeader_t *header = (IpHeader_t *) buf->data;
    buf->data += SIZEOF_IP_HEADER;
    
    if (NetworkCheckSum((uint8 *)header, SIZEOF_IP_HEADER) != 0) {
        printk("get a ip package, from: ");
        DumpIpAddress(header->sourceIP);
        printk(" but it's checksum is wrong, drop it\n");
        return -1;
    }
    
    /* 检测协议类型 */ 
    switch (header->protocol) {
    case IP_PROTO_RAW:
        printk("#get a raw ip package, data: %s\n", buf->data);
        FreeNetBuffer(buf);
        break;
    case IP_PROTO_ICMP:
        IcmpReceive(buf, ntohl(header->sourceIP));
        FreeNetBuffer(buf);
        break;
    case IP_PROTO_UDP:
        //printk("#get a udp ip package, data: %s\n", buf->data);
        /*if (!UdpReceive(buf, ntohl(header->sourceIP))) {
            FreeNetBuffer(buf);
        }*/
        break;
    case IP_PROTO_TCP:
        //printk("#get a tcp ip package, data: %s\n", buf->data);
        /*if (TcpReceive(buf, ntohl(header->sourceIP))) {
            FreeNetBuffer(buf);
        }*/
        break;
    default:
        //printk("#get an ip package with protocol: %x, not support.\n", header->protocol);
        FreeNetBuffer(buf);
        break;
    }

    /*if (header->protocol == 0xff) {
        printk("get a raw ip package, data: %s\n", buf->data);
    } else {
        //printk("get a non-raw ip package protocol: %x\n", header->protocol);
    }*/
#ifdef _IP_DEBUG
    DumpIpHeader(header);

    uint8_t *p = buf->data;
    
    int i;
    for (i = 0; i < 16; i++) {
        printk("%x ", p[i]);
    }
    printk("\n");
#endif
    return 0;
}

PUBLIC void DumpIpHeader(IpHeader_t *header)
{
    printk("Ip Header:\n");
    printk("header len:%d version:%d tos:%x total len:%d id:%d offset:%d\n\
ttl:%d protocol:%x check sum:%x\n", 
            header->headerLen,
            header->version,
            header->tos,
            header->totalLen,
            header->id,
            header->offset,
            header->ttl,
            header->protocol,
            header->checkSum);
    DumpIpAddress(header->sourceIP);
    DumpIpAddress(header->destIP);
}


/**
 * InitNetworkIp - 初始网络的IP部分
 * 
 */
PUBLIC int InitNetworkIp()
{
    ipNextID = 0;
    return 0;
}