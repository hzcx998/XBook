/*
 * file:		include/clock/clock.h
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_CLOCK_H
#define _DRIVER_CLOCK_H

#include <lib/stdint.h>
#include <lib/types.h>
#include <lib/time.h>

#include <clock/pit/clock.h>

/* 时间和数据互相转换 */
#define TIME_TO_DATA16(hou, min, sec) ((unsigned short)(((hou&0x1f)<<11)|((min&0x3f)<<5)|((sec/2)&0x1f)))

#define DATA16_TO_TIME_HOU(data) ((unsigned int)((data>>11)&0x1f))
#define DATA16_TO_TIME_MIN(data) ((unsigned int)((data>>5)&0x3f))
#define DATA16_TO_TIME_SEC(data) ((unsigned int)((data&0x1f) *2))

/* 日期和数据互相转换 */
#define DATE_TO_DATA16(yea, mon, day) ((unsigned short)((((yea-1980)&0x7f)<<9)|((mon&0xf)<<5)|(day&0x1f)))

#define DATA16_TO_DATE_YEA(data) ((unsigned int)(((data>>9)&0x7f)+1980))
#define DATA16_TO_DATE_MON(data) ((unsigned int)((data>>5)&0xf))
#define DATA16_TO_DATE_DAY(data) ((unsigned int)(data&0x1f))

/**
 * 时间转换：
 * 1秒（s） = 1000毫秒（ms）
 * 1毫秒（ms） = 1000微秒（us）
 * 1微秒（us）= 1000纳秒（ns）
 * 1纳秒（ns）= 1000皮秒（ps）
 */

struct SystemDate
{
	int second;         /* [0-59] */
	int minute;         /* [0-59] */
	int hour;           /* [0-23] */
	int day;            /* [1-31] */
	int month;          /* [1-12] */
	int year;           /* year */
	int weekDay;        /* [0-6] */
	int yearDay;        /**/
	int isDst;          /* 夏令时[-1,0,1] */
};

EXTERN struct SystemDate systemDate;

EXTERN volatile clock_t systicks;


PUBLIC void InitClockSystem();
PUBLIC void ClockChangeSystemDate();

PUBLIC void PrintSystemTime();
PUBLIC void GetLocalTime(struct SystemDate *time);

PUBLIC unsigned long SysTime(struct tm *tm);

PRIVATE INLINE unsigned int SystemDateToData()
{
	unsigned short date = DATE_TO_DATA16(systemDate.year, systemDate.month, systemDate.day);
	unsigned short time =  TIME_TO_DATA16(systemDate.hour, systemDate.minute, systemDate.second);
	return (date << 16) | time;
}

#endif  /* _DRIVER_CLOCK_H */
