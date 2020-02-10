/*
 * file:		include/net/ipv4/icmp.h
 * auther:		Jason Hu
 * time:		2020/1/17
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _NET_IPV4_ICMP_H
#define _NET_IPV4_ICMP_H

#include <lib/stdint.h>
#include <lib/types.h>

#include <net/netbuf.h>

enum IcmpEchoType {
    ICMP_ECHO_REPLY = 0,
    ICMP_ECHO_REQUEST = 8,
};


/* ICMP echo 的数据报头 */
typedef struct IcmpEchoHeader {
    uint8_t  type;
    uint8_t  code;
    uint16_t checkSum;
    uint16_t id;
    uint16_t seqNO;
} IcmpEchoHeader_t;

#define SIZEOF_ICMP_ECHO_HEADER sizeof(struct IcmpEchoHeader)

void IcmpEchoHeaderInit(
    IcmpEchoHeader_t *header,
    uint8_t type,
    uint8_t code,
    uint16_t checkSum,
    uint16_t id,
    uint16_t seq);

PUBLIC bool IcmpEechoRequest(uint32 ip, uint16 id, uint16 seq, uint8* data, uint32 len);
PUBLIC bool IcmpEchoReply(uint32 ip, uint16 id, uint16 seq, uint8 *data, uint32 len);
PUBLIC void IcmpReceive(NetBuffer_t *buf, uint32 ip);

#endif   /* _NET_IPV4_ICMP_H */
