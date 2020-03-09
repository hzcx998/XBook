/*
 * file:		inet.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <inet.h>

uint16_t htons(uint16_t hostshort)
{
  return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
}

uint16_t ntohs(uint16_t netshort)
{
  return ((netshort & 0xFF) << 8) | ((netshort & 0xFF00) >> 8);
}

uint32_t htonl(uint32_t hostlong)
{
  return ((hostlong & 0xFF) << 24) | ((hostlong & 0xFF00) << 8) | \
        ((hostlong & 0xFF0000) >> 8) | ((hostlong & 0xFF000000) >> 24);
}

uint32_t ntohl(uint32_t netlong)
{
  return ((netlong & 0xFF) << 24) | ((netlong & 0xFF00) << 8) | \
        ((netlong & 0xFF0000) >> 8) | ((netlong & 0xFF000000) >> 24);
}
