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
#include <lib/time.h>

#define CONFIG_TIMEZONE_AUTO 1 	/* 自动转换时区 */

/* ----驱动程序初始化文件导入---- */
EXTERN void InitPitClockDriver();
/* ----驱动程序初始化文件导入完毕---- */

PUBLIC volatile clock_t systicks;

PUBLIC struct SystemDate systemDate;

/* 每月对应的天数，2月的会在闰年是加1 */
CONST char monthDay[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

/**
 * IsLeap - 判断是否是闰年 
 * @year: 传入的年份 
 * 
 * 四年一闰，百年不闰，四百年再闰
 */
int IsLeap(int year)
{
	if (year % 4) return 0;
    if (year % 400) return 1;
    if (year % 100) return 0;
    return 1;
}



/**
 * getWeek - 返回某天是周几 
 * @year: 年
 * @month: 月
 * @day: 日
 * 
 * 返回周几（0-6） 
 */
PRIVATE int getWeekDay(int year, int month, int day)
{
    int c, y, week;
    if (month == 1 || month == 2) { //判断month是否为1或2　
        year--;
        month+=12;
    }
    c = year / 100;
    y = year - c * 100;
    week = (c / 4) - 2 * c + (y + y / 4) + (13 * (month + 1) / 5) + day - 1;
    while(week < 0) 
        week += 7;
    week %= 7;
    return week;
}

PRIVATE int GetMonthDays()
{
    /* 如果是2月，就需要判断是否是闰年 */
    if (systemDate.month == 2) {
        /* 如果是闰年就要加1 */
        return monthDay[systemDate.month] + IsLeap(systemDate.year);
    } else {
        return monthDay[systemDate.month];
    }
}

PRIVATE int GetYearDays()
{
    int i;
    int sum = 0;
    /* 计算前几个月的日数 */
    for (i = 1; i < systemDate.month; i++) {
        if (i == 2)   /* 对于2月份，都要判断闰年 */
            sum += monthDay[i] + IsLeap(systemDate.year);
        else
            sum += monthDay[i];
        
    }
    /* 计算当前月的日数 */
    sum += systemDate.day;

    /* 如果是2月以上的，才进行闰年检测 */
    if (systemDate.month >= 2) {
        /* 2月只有day是28的时候，才进行检测 */
        if (systemDate.month == 2) {
            if (systemDate.day == 28)
                sum += IsLeap(systemDate.year);    
        } else {
            sum += IsLeap(systemDate.year);
        }
    }

    return sum;
}

/**
 * ClockChangeSystemDate - 改变系统的时间
*/
PUBLIC void ClockChangeSystemDate()
{
	systemDate.second++;
	if(systemDate.second > 59){
		systemDate.minute++;
		systemDate.second = 0;
		if(systemDate.minute > 59){
			systemDate.hour++;
			systemDate.minute = 0;
			if(systemDate.hour > 23){
				systemDate.day++;
				systemDate.hour = 0;
                
                systemDate.weekDay++;
                /* 如果大于1-6是周一到周六，0是周日 */
                if (systemDate.weekDay > 6)
                    systemDate.weekDay = 0;

                /* 获取是今年的多少个日子 */
                systemDate.yearDay = GetYearDays();

				//现在开始就要判断平年和闰年，每个月的月数之类的
				if(systemDate.day > GetMonthDays()){
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
 * SysTime - 获取时间
 * @tm: 时间
 * 
 * 返回系统ticks
 */
PUBLIC unsigned long SysTime(struct tm *tm)
{
    if (tm) {
        /* 复制数据 */
        tm->tm_sec = systemDate.second;
        tm->tm_min = systemDate.minute;
        tm->tm_hour = systemDate.hour;
        tm->tm_year = systemDate.year - 1900;
        tm->tm_mon = systemDate.month - 1;
        tm->tm_mday = systemDate.day;
        tm->tm_wday = systemDate.weekDay;
        tm->tm_yday = systemDate.yearDay - 1;
        tm->tm_isdst = systemDate.isDst;      /* 默认不是夏令时 */
    }

    return systicks;
}

/**
 * PrintSystemDate - 打印系统时间
 */
PUBLIC void PrintSystemDate()
{
	printk("Time:%d:%d:%d Date:%d/%d/%d\n", \
		systemDate.hour, systemDate.minute, systemDate.second,\
		systemDate.year, systemDate.month, systemDate.day);
	
	char *weekday[7];
	weekday[0] = "Sunday";
    weekday[1] = "Monday";
	weekday[2] = "Tuesday";
	weekday[3] = "Wednesday";
	weekday[4] = "Thursday";
	weekday[5] = "Friday";
	weekday[6] = "Saturday";
	printk("weekday:%d-%s yearday:%d\n", systemDate.weekDay, weekday[systemDate.weekDay], systemDate.yearDay);
}

/**
 * InitClockSystem - 初始化时钟系统
 * 多任务的运行依赖于此
 */
PUBLIC void InitClockSystem()
{
    /* 初始化系统时间 */
    
    //用一个循环让秒相等
	do{
		systemDate.year = CMOS_GetYear();
		systemDate.month = CMOS_GetMonHex();
		systemDate.day = CMOS_GetDayOfMonth();
		
		systemDate.hour = CMOS_GetHourHex();
		systemDate.minute =  CMOS_GetMinHex8();
		systemDate.second = CMOS_GetSecHex();
		
		/* 转换成本地时间 */
		/* 自动转换时区 */
#if CONFIG_TIMEZONE_AUTO == 1
        /*if(systemDate.hour >= 16){
			systemDate.hour -= 16;
		}else{
			systemDate.hour += 8;
		}*/
#endif /* CONFIG_TIMEZONE_AUTO */
	}while(systemDate.second != CMOS_GetSecHex());

    systemDate.weekDay = getWeekDay(systemDate.year, systemDate.month, systemDate.day);
    systemDate.yearDay = GetYearDays();    
    systemDate.isDst = 0;
    
    PrintSystemDate();

    InitPitClockDriver();
}
