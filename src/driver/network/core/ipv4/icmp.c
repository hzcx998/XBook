/*
 * file:		network/core/ipv4/icmp.c
 * auther:		Jason Hu
 * time:		2019/1/17
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <lib/string.h>
#include <lib/inet.h>

#include <net/ipv4/icmp.h>
#include <net/ipv4/ip.h>
#include <net/network.h>

void IcmpEchoHeaderInit(
    IcmpEchoHeader_t *header,
    uint8_t type,
    uint8_t code,
    uint16_t checkSum,
    uint16_t id,
    uint16_t seq)
{
    header->type = type;
    header->code = code;
    header->checkSum = checkSum;
    header->id = id;
    header->seqNO = seq;
}

PUBLIC bool IcmpEechoRequest(uint32 ip, uint16 id, uint16 seq, uint8* data, uint32 len)
{
    uint32 total = len + SIZEOF_ICMP_ECHO_HEADER;
    NetBuffer_t *buffer = AllocNetBuffer(total);
    if (buffer == NULL) {
        printk("ICMP echo_request, alloc net buffer failed, total: %x\n", total);
        return false;
    }

    printk("send an icmp echo request to ip: ");
    DumpIpAddress(ip);
    printk(" seq: %x\n", seq);

    IcmpEchoHeader_t header;
    IcmpEchoHeaderInit(&header,
            ICMP_ECHO_REQUEST,          /* type */
            0,                     /* code */ 
            0,                     /* check sum */
            htons(id),      /* id */
            htons(seq));    /* seq no */
    header.checkSum = NetworkCheckSum((uint8 *) &header, SIZEOF_ICMP_ECHO_HEADER);

    memcpy(buffer->data, &header, SIZEOF_ICMP_ECHO_HEADER);
    
    if (data != NULL && len != 0) {
        memcpy(buffer->data + SIZEOF_ICMP_ECHO_HEADER, data, len);
    }

    IpTransmit(ip, buffer->data, total, IP_PROTO_ICMP);
    FreeNetBuffer(buffer);
    return true;
}

PUBLIC bool IcmpEchoReply(uint32 ip, uint16 id, uint16 seq, uint8 *data, uint32 len)
{
    uint32 total = len + SIZEOF_ICMP_ECHO_HEADER;
    NetBuffer_t *buffer = AllocNetBuffer(total);
    if (buffer == NULL) {
        printk("ICMP echo_reply, alloc net buffer failed, total: %x\n", total);
        return false;
    }

    printk("send an icmp echo reply to ip: ");
    DumpIpAddress(ip);
    printk(" seq: %u\n", seq);

    IcmpEchoHeader_t header;
    IcmpEchoHeaderInit(&header,
            ICMP_ECHO_REPLY,            /* type */
            0,                     /* code */ 
            0,                     /* check sum */
            htons(id),      /* id */
            htons(seq));    /* seq no */
    header.checkSum = NetworkCheckSum((uint8 *) &header, SIZEOF_ICMP_ECHO_HEADER);

    memcpy(buffer->data, &header, SIZEOF_ICMP_ECHO_HEADER);
    
    if (data != NULL && len != 0) {
        memcpy(buffer->data + SIZEOF_ICMP_ECHO_HEADER, data, len);
    }

    IpTransmit(ip, buffer->data, total, IP_PROTO_ICMP);
    FreeNetBuffer(buffer);
    return true;
}

PRIVATE void IcmpEchoRequestReceive(NetBuffer_t *buf, uint32 ip)
{
    IcmpEchoHeader_t *header = (IcmpEchoHeader_t *) buf->data;

    uint16 checkSum = NetworkCheckSum(buf->data, buf->dataLen);
    if (checkSum != 0) {
        printk("receive an icmp echo request, but checksum is error: %x.\n", checkSum);
        return;
    }

    printk("receive an icmp echo request from ip: ");
    DumpIpAddress(ip);
    printk(" seq: %x\n", ntohs(header->seqNO));
    IcmpEchoReply(ip, ntohs(header->id), ntohs(header->seqNO), NULL, 0);
}

PRIVATE void IcmpEchoReplyReceive(NetBuffer_t *buf, uint32 ip)
{
    IcmpEchoHeader_t *header = (IcmpEchoHeader_t *) buf->data;
    uint16 checkSum = NetworkCheckSum(buf->data, buf->dataLen);
    if (checkSum != 0) {
        printk("receive an icmp echo reply, but checksum is error: %x.\n", checkSum);
        return;
    }

    printk("receive an icmp echo reply from ip: ");
    DumpIpAddress(ip);
    printk(" seq: %u\n", ntohs(header->seqNO));
}


PUBLIC void IcmpReceive(NetBuffer_t *buf, uint32 ip)
{
    uint8 type = *(uint8 *) buf->data;
    switch (type) {
    case ICMP_ECHO_REQUEST:
        IcmpEchoRequestReceive(buf, ip);
        break;
    case ICMP_ECHO_REPLY:
        IcmpEchoReplyReceive(buf, ip);
        break;
    default:
        printk("receive an icmp package, but not support the type %x now.\n", type);
        break;
    }
}
