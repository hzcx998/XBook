/*
 * file:		include/lib/inet.h
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_INET_H
#define _LIB_INET_H

#include "stdint.h"

uint32_t htonl(uint32_t hostlong);  // htonl()–“Host to Network Long”
uint16_t htons(uint16_t hostshort); // ntohl()–“Network to Host Long”
uint32_t ntohl(uint32_t netlong);   // htons()–“Host to Network Short”
uint16_t ntohs(uint16_t netshort);  // ntohs()–“Network to Host Short”

#endif  /* _LIB_INET_H */
