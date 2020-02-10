/*
 * file:		include/book/alarm.h
 * auther:		Jason Hu
 * time:		2019/12/27
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_ALARM_H
#define _BOOK_ALARM_H

#include <lib/stdint.h>
#include <lib/types.h>

PUBLIC void UpdateAlarmSystem();
PUBLIC unsigned int SysAlarm(unsigned int seconds);

#endif   /* _BOOK_ALARM_H */
