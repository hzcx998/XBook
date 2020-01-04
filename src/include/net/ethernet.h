/*
 * file:		include/net/ethernet.h
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*
以太网（ethernet）和因特网（internet）的区别：
1、概念不一样：以太网是一种计算机局域网技术。因特网则是一组全球信息资源的总汇。

2、内容不一样：以太网包括物理层的连线、电子信号和介质访问层协议的内容。
    因特网最高层域名分为机构性域名和地理性域名两大类，目前主要有14 种机构性域名。

3、特点不一样：以太网通讯具有自相关性的特点，这对于电信通讯工程十分重要。
    因特网的特点在于他是一个全球计算机互联网络，是一个巨大的信息资料，
    最重要的是Internet是一个大家庭，有几千万人参与，共同享用着人类自己创造的财富( 即资源)。
*/


#ifndef _NET_ETHERNET_H
#define _NET_ETHERNET_H

#include <share/stdint.h>
#include <share/types.h>

/* 以太网头部 */

/* 头部地址长度，6，MAC地址 */
#define ETH_ADDR_LEN        6

/* 以太网头部结构体 */
typedef struct EthernetHeader {
    /* 地址都是MAC地址 */
    unsigned char dest[ETH_ADDR_LEN];   /* 目标地址 */
    unsigned char source[ETH_ADDR_LEN]; /* 源地址 */
    unsigned short protocol;            /* 协议 */
} PACKED EthernetHeader_t;

/* 以太网帧头部长度 */
#define SIZEOF_ETHERNET_HEADER  sizeof(struct EthernetHeader)

PUBLIC void EthernetHeaderInit(
    EthernetHeader_t *eth,
    unsigned char *dest,
    unsigned char *source,
    unsigned short protocol
);

PUBLIC void EthernetSend(
    unsigned char *destAddr,
    unsigned short protocol, 
    unsigned char *data,
    size_t len
);

PUBLIC void EthernetReceive(unsigned char *data, size_t len);

PUBLIC uint32_t EthernetCrc(unsigned char *data, int length);

PUBLIC void EthernetSetAddress(unsigned char *ethAddr);
PUBLIC unsigned char *EthernetGetAddress();

PUBLIC void DumpEthernetAddress(unsigned char *ethAddr);
PUBLIC void DumpEthernetHeader(EthernetHeader_t *header);
/*  */

#endif   /* _NET_ETHERNET_H */
