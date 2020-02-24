/*
 * file:		clock/core/clock.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <clock/clock.h>

/* ----驱动程序初始化文件导入---- */
EXTERN void InitPitClockDriver();
/* ----驱动程序初始化文件导入完毕---- */

PUBLIC volatile clock_t systicks;

PUBLIC struct SystemDate systemDate;

/**
 * ClockChangeSystemDate - 改变系统的时间
*/
PUBLIC void ClockChangeSystemDate()
{
	systemDate.second++;
	if(systemDate.second >= 60){
		systemDate.minute++;
		systemDate.second = 0;
		if(systemDate.minute >= 60){
			systemDate.hour++;
			systemDate.minute = 0;
			if(systemDate.hour >= 24){
				systemDate.day++;
				systemDate.hour = 0;
				//现在开始就要判断平年和闰年，每个月的月数之类的
				if(systemDate.day > 30){
					systemDate.month++;
					systemDate.day = 1;
					
					if(systemDate.month > 12){
						systemDate.year++;	//到年之后就不用调整了
						systemDate.month = 1;
					}
				}
			}
		}
	}
}

/**
 * GetLocalTime - 获取本地时间
 * @time: 时间结构体
 */
PUBLIC void GetLocalTime(struct SystemDate *time)
{
    *time = systemDate;
}

/**
 * PrintSystemDate - 打印系统时间
 */
PUBLIC void PrintSystemDate()
{
	printk(PART_TIP "----Time & Date----\n");

	printk("Time:%d:%d:%d Date:%d/%d/%d\n", \
		systemDate.hour, systemDate.minute, systemDate.second,\
		systemDate.year, systemDate.month, systemDate.day);
	
	/*char *weekday[7];
	weekday[0] = "Monday";
	weekday[1] = "Tuesday";
	weekday[2] = "Wednesday";
	weekday[3] = "Thursday";
	weekday[4] = "Friday";
	weekday[5] = "Saturday";
	weekday[6] = "Sunday";
	printk("weekday:%d", systemDate.weekDay);
	*/
}

/**
 * InitClockSystem - 初始化时钟系统
 * 多任务的运行依赖于此
 */
PUBLIC void InitClockSystem()
{
    InitPitClockDriver();
}
