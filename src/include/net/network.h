/*
 * file:		include/net/network.h
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/*
网络层之间传输数据的缓冲区
*/

#ifndef _NET_NETWORK_H
#define _NET_NETWORK_H

#include <lib/stdint.h>
#include <lib/types.h>

/***** 配置区域 *****/

/* 回环测试 */
//#define _LOOPBACL_DEBUG

/* 网卡配置 */
//#define _NIC_RTL8139
//#define _NIC_AMD79C973

/* 虚拟机配置 */
//#define _VM_VMWARE
//#define _VM_VBOX
#define _VM_QEMU

/* 网络协议 */
#define PROTO_IP            0x0800
#define PROTO_ARP           0x0806


/*备注以太网的情况*/
#define ETH_ALEN 6 /*以太网地址，即MAC地址，6字节*/
#define ETH_HLEN 14 /*以太网头部的总长度*/
#define ETH_ZLEN 60 /*不含CRC校验的数据最小长度*/
#define ETH_DATA_LEN 1500 /*帧内数据的最大长度*/
#define ETH_FRAME_LEN 1514 /*不含CRC校验和的最大以太网数据长度*/




PUBLIC unsigned int NetworkMakeIpAddress(
    unsigned char ip0,
    unsigned char ip1,
    unsigned char ip2, 
    unsigned char ip3);

PUBLIC int InitNetworkDevice();
PUBLIC unsigned int NetworkMakeIpAddress(
    unsigned char ip0,
    unsigned char ip1,
    unsigned char ip2, 
    unsigned char ip3);

PUBLIC void NetworkSetIpAddress(unsigned int ip);
PUBLIC unsigned int NetworkGetIpAddress();
PUBLIC unsigned int NetworkGetSubnetMask();
PUBLIC unsigned int NetworkGetGateway();

PUBLIC void DumpIpAddress(unsigned int ip);

PUBLIC uint16 NetworkCheckSum(uint8_t *data, uint32_t len);

PUBLIC int NetworkAddBuf(void *data, size_t len);


STATIC INLINE int IsValidMulticastAddr(const uint8_t *addr)
{
    int i;
    for (i = 0; i < 6; i++) {
        /* 如果不是0xff，就说嘛不是有效的地址 */
        if (addr[i] != 0xff)
            return 0;
    }
    return 1;
}

STATIC INLINE int IsZeroEtherAddr(const uint8_t *addr)
{
    int i;
    for (i = 0; i < 6; i++) {
        /* 如果不是0，就说嘛不是有效的地址 */
        if (addr[i] != 0)
            return 0;
    }
    return 1;
}

STATIC INLINE int IsValidEtherAddr(const uint8_t *addr)
{
    /* 不是多播和0地址，才是有效的以太网地址 */
    return !IsValidMulticastAddr(addr) && !IsZeroEtherAddr(addr);
}

/* 驱动注册入口，以及传输函数 */
PUBLIC int InitRtl8139Driver();
PUBLIC int Rtl8139Transmit(char *buf, uint32 len);
PUBLIC unsigned char *Rtl8139GetMACAddress();


#endif   /* _NET_NETWORK_H */
