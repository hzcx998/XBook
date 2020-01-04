/*
 * file:		include/drivers/rtl8139.h
 * auther:		Jason Hu
 * time:		2020/1/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_NETWORK_RTL8139_H
#define _DRIVER_NETWORK_RTL8139_H

#include <share/stdint.h>
#include <share/types.h>

PUBLIC int InitRtl8139Driver();
PUBLIC int Rtl8139Transmit(char *buf, uint32 len);

PUBLIC unsigned char *Rtl8139GetMACAddress();

#endif  /* _DRIVER_NETWORK_RTL8139_H */
