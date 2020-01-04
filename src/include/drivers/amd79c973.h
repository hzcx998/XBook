/*
 * file:		include/drivers/amd79c973.h
 * auther:		Jason Hu
 * time:		2020/1/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_NETWORK_AMD79C973_H
#define _DRIVER_NETWORK_AMD79C973_H

#include <share/stdint.h>
#include <share/types.h>


PUBLIC int InitAmd79c973Driver();
PUBLIC void Amd79c973Send(uint8_t* buffer, int size);

PUBLIC void Amd79c973SetIPAddress(uint32_t ip);
PUBLIC uint32_t Amd79c973GetIPAddress();
PUBLIC unsigned char *Amd79c973GetMACAddress();

#endif  /* _DRIVER_NETWORK_AMD79C973_H */
