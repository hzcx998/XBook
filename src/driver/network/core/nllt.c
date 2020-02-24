/*
 * file:		network/core/llt.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <lib/string.h>

#include <net/ipv4/ethernet.h>
#include <net/ipv4/arp.h>
#include <net/network.h>
#include <net/nllt.h>

/* 导入网卡传输函数 */
EXTERN int Rtl8139Transmit(char *buf, uint32 len);


/**
 * NlltSend - 发送数据
 * @buf: 要发送的数据
 * 
 */
int NlltSend(NetBuffer_t *buf)
{
    //printk("NLLT: [send] -data:%x -length:%d\n", buf->data, buf->dataLen);

    //EthernetHeader_t *ethHeader = (EthernetHeader_t *)buf->data;
    
    //DumpEthernetHeader(ethHeader);

    //char *p = (char *)(buf->data + SIZEOF_ETHERNET_HEADER);

    //Rtl8139Transmit(buf->data, buf->dataLen);

    //ArpHeader_t *arpHeader = (ArpHeader_t *)p;
    
    //DumpArpHeader((ArpHeader_t *)arpHeader);
        
    //DumpArpHeader(arpHeader);

    /* 环回测试，自己发送给自己作为测试 */
#ifdef _NET_LOOPBACK

    /* 打印数据结构 */
    /*uint8_t *src = buf->data;

    EthernetHeader_t *ethHeader = (EthernetHeader_t *)src;
    DumpEthernetHeader(ethHeader);
    */
    printk("<loop back>\n");
    NlltReceive(buf->data, buf->dataLen);
#else 

#ifdef CONFIG_DRV_RTL8139
    Rtl8139Transmit((char *)buf->data, buf->dataLen);
#endif /* CONFIG_DRV_RTL8139 */

#endif /* _NET_LOOPBACK */

    return 0;
}

/**
 * NlltReceive - 接收数据
 * @data: 要接收的数据
 * @length: 数据长度
 */
int NlltReceive(unsigned char *data, unsigned int length)
{
    //printk("NLLT: [receive] -data:%x -length:%d\n", data, length);

    /* 复制数据到队列中，并返回 */
    NetworkAddBuf(data, length);

    return 0;
}
