/*
 * file:		include/net/ip.h
 * auther:		Jason Hu
 * time:		2020/1/10
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _NET_IP_H
#define _NET_IP_H

#include <share/stdint.h>
#include <share/types.h>
#include <net/netbuf.h>

enum IpProtocol {
    IP_PROTO_ICMP = 1,
    IP_PROTO_TCP = 6,
    IP_PROTO_UDP = 17,
    IP_PROTO_RAW = 0xff
};


#define IP_VERSION4  0x04  /* ipv4 */
#define IP_VERSION6  0x06  /* ipv6 */



typedef struct IpHeader {
    uint8_t   headerLen:4;
    uint8_t   version:4;
    uint8_t   tos;          /* type of service */
    uint16_t  totalLen;
    uint16_t  id;
    uint16_t  offset;       /* fragment offset field */
    uint8_t   ttl;          /* time to live */
    uint8_t   protocol;
    uint16_t  checkSum;
    uint32_t  sourceIP;
    uint32_t  destIP;
} PACKED IpHeader_t;

#define SIZEOF_IP_HEADER sizeof(IpHeader_t)

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
    uint32_t destIP);

PUBLIC int InitNetworkIp();

PUBLIC bool IpIsBroadcast(uint32_t ip);
PUBLIC bool IpCheckIp(uint32_t ip);
PUBLIC bool IpIsSameSubnet(uint32_t ip1, uint32_t ip2, uint32_t mask);

PUBLIC int IpTransmit(uint32 ip, uint8 *data, uint32 len, uint8 protocol);
PUBLIC int IpReceive(NetBuffer_t *buf);

PUBLIC void DumpIpHeader(IpHeader_t *header);

#endif   /* _NET_IP_H */
