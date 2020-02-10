/*
 * file:		include/net/nllt.h
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/*
Netwrok Low Level Transport(NLLT)低等级数据传输
网卡把数据发送到这里，然后再往上传送。
以太网把数据发送到这里，然后再传输到网卡。
*/

#ifndef _NET_NLLT_H
#define _NET_NLLT_H

#include <lib/stdint.h>
#include <lib/types.h>
#include <net/netbuf.h>

int NlltSend(NetBuffer_t *buf);
int NlltReceive(unsigned char *data, unsigned int length);

#endif   /* _NET_NLLT_H */
