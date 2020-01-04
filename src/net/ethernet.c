/*
 * file:		net/ethernet.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/inet.h>

#include <net/network.h>
#include <net/ethernet.h>
#include <net/netbuf.h>
#include <net/nllt.h>
#include <net/arp.h>

/* 以太网地址，即MAC地址 */
unsigned char ethernetAddress[ETH_ADDR_LEN];


/**
 * EthernetHeaderInit - 以太网头部初始化
 * @eth: 以太网头部
 * @dest: 目标地址
 * @source: 源地址
 * @protocal: 协议
 * 
 */
PUBLIC void EthernetHeaderInit(
    EthernetHeader_t *eth,
    unsigned char *dest,
    unsigned char *source,
    unsigned short protocol)
{
    memcpy(eth->dest, dest, ETH_ADDR_LEN);
    memcpy(eth->source, source, ETH_ADDR_LEN);
    eth->protocol = protocol;
}

/**
 * EthernetSend - 以太网发送数据
 * @destAddr: 目标地址
 * @data: 数据
 * @len: 数据长度
 * 
 */
PUBLIC void EthernetSend(
    unsigned char *destAddr,
    unsigned short protocol, 
    unsigned char *data,
    size_t len)
{
    
    size_t length = len;
    
    /* 对长度进行填充，数据长度至少是46字节 */
    /*if (length < 46) {
        length = 46;
    }*/
    /* 如果是发送给自己，那么就返回，暂不处理 */
    /*if (!memcmp(destAddr, ethernetAddress, ETH_ADDR_LEN)) {
        printk("send to myself!\n");
        return;
    }*/
    
    size_t total = length + SIZEOF_ETHERNET_HEADER;
    

    printk("ethernet data len %d\n", total);    

    NetBuffer_t *buf = AllocNetBuffer(total);
    
    if (buf != NULL) {
        /* 创建并初始化以太网头部 */
        EthernetHeader_t ethHeader;
        EthernetHeaderInit(&ethHeader, destAddr, ethernetAddress, ntohs(protocol));

        /* 填写缓冲区 */
        buf->dataLen = total;
        buf->data = (unsigned char *)buf + sizeof(NetBuffer_t);
        
        unsigned char *p = buf->data;
        /* 把以太网帧复制到缓冲区 */
        memcpy(p, &ethHeader, SIZEOF_ETHERNET_HEADER);

        p += SIZEOF_ETHERNET_HEADER;
        
        /* 只复制参数长度的大小 */
        memcpy(p, data, len);
        
        /* 长度小于46字节的部分都用0填充 */
        if (length > len) {
            memset(p + len, 0, length - len);
        }

        /* 用网卡把缓冲区传输出去 */
        NlltSend(buf);

        /* 释放缓冲区 */
        FreeNetBuffer(buf);
    }
}

/**
 * EthernetReceive - 以太网接受数据
 * @data: 数据
 * @len: 数据长度
 * 
 */
PUBLIC void EthernetReceive(unsigned char *data, size_t len)
{
    NetBuffer_t *buf = AllocNetBuffer(len);
    if (buf != NULL) {
        /* 填写缓冲头 */
        buf->dataLen = len;
        buf->data = (unsigned char *) buf + SIZEOF_ETHERNET_HEADER;
        /* 复制数据 */
        memcpy(buf->data, data, len);

        EthernetHeader_t *header = (EthernetHeader_t *)buf->data;

        /* 如果是arp协议，就传输给arp处理 */
        if (htons(header->protocol) == PROTO_ARP) {
            buf->data += SIZEOF_ETHERNET_HEADER;
            buf->dataLen -= SIZEOF_ETHERNET_HEADER;

            ArpReceive(header->source, buf);
        } else {
            printk("net receive from [%2x:%2x:%2x:%2x:%2x:%2x] to [%2x:%2x:%2x:%2x:%2x:%2x]\n",
                header->source[0], header->source[1], header->source[2],
                header->source[3], header->source[4], header->source[5],
                header->dest[0], header->dest[1], header->dest[2], 
                header->dest[3], header->dest[4], header->dest[5]);

            /* 如果接收者是自己，那么就打印数据 */
            if (!memcmp(ethernetAddress, header->dest, ETH_ADDR_LEN)) {
                char *p = (char *) buf->data;
                printk("data: %s\n", p);
            }
        }

        /* 释放缓冲区 */
        FreeNetBuffer(buf);
    }
}

/**
 * EthernetCrc - 计算以太网的校验和
 * @data: 数据地址
 * @length: 长度
 * 
 * 返回计算后的数值
 */
PUBLIC unsigned int EthernetCrc(unsigned char *data, int length)
{
    static const unsigned int crcTable[] = {
        0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
        0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
        0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
        0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000
    };
 
    unsigned int crc = 0;
    int n;
    for (n = 0; n < length; n++) {
        crc = (crc >> 4) ^ crcTable[(crc ^ (data[n] >> 0)) & 0x0F];  /* lower nibble */
        crc = (crc >> 4) ^ crcTable[(crc ^ (data[n] >> 4)) & 0x0F];  /* upper nibble */
    }
 
    return crc;
}

/**
 * EthernetSetAddress - 设置以太网地址（MAC）
 * @ethAddr: 以太网地址
 * 
 */
PUBLIC void EthernetSetAddress(unsigned char *ethAddr)
{
    memcpy(ethernetAddress, ethAddr, ETH_ADDR_LEN);
}

/**
 * EthernetGetAddress - 获取以太网地址（MAC）
 * 
 */
PUBLIC unsigned char *EthernetGetAddress()
{
    return ethernetAddress;
}

PUBLIC void DumpEthernetAddress(unsigned char *ethAddr)
{
    printk(PART_TIP "Ethernet Address:");
    printk(PART_TIP "%x:%x:%x:%x:%x:%x\n", ethAddr[0],
            ethAddr[1], ethAddr[2], ethAddr[3],
            ethAddr[4], ethAddr[5]);
    
}

PUBLIC void DumpEthernetHeader(EthernetHeader_t *header)
{
    printk(PART_TIP "Ethernet Header:");
    DumpEthernetAddress(header->source);
    DumpEthernetAddress(header->dest);
    
    printk(PART_TIP "protocol:%x\n", header->protocol);
    
}

