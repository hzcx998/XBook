/*
 * file:		include/lib/time.h
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_TIME_H
#define _LIB_TIME_H

# include "stddef.h"

typedef unsigned long clock_t;
typedef unsigned long time_t;

struct tm
{
  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int tm_min;			/* Minutes.	[0-59] */
  int tm_hour;			/* Hours.	[0-23] */
  int tm_mday;			/* Day.		[1-31] */
  int tm_mon;			/* Month.	[0-11] */
  int tm_year;			/* Year	- 1900.  */
  int tm_wday;			/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/
};

unsigned int time(struct tm *tm);
char *asctime_r(const struct tm *tp, char *buf);
char *asctime(const struct tm *tp);

#endif  /*_LIB_TIME_H*/
