/*
 * file:		clock/pit/clock.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/schedule.h>
#include <book/task.h>
#include <book/timer.h>
#include <book/interrupt.h>
#include <book/alarm.h>
#include <lib/math.h>
#include <lib/stddef.h>
#include <clock/clock.h>

/* 时钟 */

/* 时钟产生频率增快，默认是1，最大时10，表示增大10倍，现在的linux内核都是10
我们约定，当有图形界面的时候把它设置为5~10，没有的时候设置为1~5
 */

#define COUNTER0_VALUE  (TIMER_FREQ / HZ)	    //pit count0 数值

/* 1 ticks 对应的毫秒数 */
#define MILLISECOND_PER_TICKS (1000 / HZ)

PUBLIC struct SystemDate systemDate;

PRIVATE int ticks;

/**
 * ClockHandler - 时钟中断处理函数
 */
PRIVATE void ClockHandler(unsigned int irq, unsigned int data)
{
    /* 改变ticks计数 */
	ticks++;
	
	/* 激活定时器软中断 */
	ActiveSoftirq(TIMER_SOFTIRQ);

	/* 激活调度器软中断 */
	ActiveSoftirq(SCHED_SOFTIRQ);
	
}

/**
 * ClockChangeSystemDate - 改变系统的时间
*/
PRIVATE void ClockChangeSystemDate()
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

/* 定时器软中断处理 */
PRIVATE void TimerSoftirqHandler(struct SoftirqAction *action)
{
	/* 改变系统时间 */
    if (ticks % HZ == 0) {  /* 1s更新一次 */
        ClockChangeSystemDate();
        //printk("t");
    }
	
	/* 更新闹钟 */
    UpdateAlarmSystem();

	/* 更新定时器 */
	UpdateTimerSystem();
    //printk("s");
}

/* 调度程序软中断处理 */
PRIVATE void SchedSoftirqHandler(struct SoftirqAction *action)
{
	struct Task *current = CurrentTask();

	/* 检测内核栈是否溢出 */
	ASSERT(current->stackMagic == TASK_STACK_MAGIC);
	/* 更新任务调度 */
	current->elapsedTicks++;
	
	if (current->ticks <= 0) {
		Schedule();
	} else {
		
		current->ticks--;
	}
}

/**
 * SleepByTicks - 休眠ticks时间
 * @sleepTicks: 要休眠多少个ticks
 */
PRIVATE void SleepByTicks(uint32_t sleepTicks)
{
	/* 获取起始ticks，后面进行差值运算 */
	uint32_t startTicks = ticks;

	/* 如果最新ticks和开始ticks差值小于要休眠的ticks，就继续休眠 */
	while (ticks - startTicks < sleepTicks) {
		/* 让出cpu占用，相当于休眠 */
		TaskYield();
        //CpuNop();
	}
}

/**
 
 * SysMSleep - 以毫秒为单位进行休眠
 */
PUBLIC void SysMSleep(uint32_t msecond)
{
	/* 把毫秒转换成ticks */
	uint32_t sleepTicks = DIV_ROUND_UP(msecond, MILLISECOND_PER_TICKS);
	ASSERT(sleepTicks > 0);
	/* 进行休眠 */
	SleepByTicks(sleepTicks);
}

/**
 * InitPitClockDriver - 初始化时钟管理
 */
PUBLIC void InitPitClockDriver()
{
	//初始化时钟
	Out8(PIT_CTRL, PIT_MODE_2 | PIT_MODE_MSB_LSB | 
            PIT_MODE_COUNTER_0 | PIT_MODE_BINARY);
	Out8(PIT_COUNTER0, (uint8_t) COUNTER0_VALUE);
	Out8(PIT_COUNTER0, (uint8_t) (COUNTER0_VALUE >> 8));
    
    /*Out8(PIT_COUNTER0, 0x9c);
	Out8(PIT_COUNTER0, 0x2e);
    printk("timer value:%x , %x\n", COUNTER0_VALUE, (0x2e << 8) | 0x9c);
    */
	ticks = 0;

	//用一个循环让秒相等
	do{
		systemDate.year = CMOS_GetYear();
		systemDate.month = CMOS_GetMonHex();
		systemDate.day = CMOS_GetDayOfMonth();
		
		systemDate.hour = CMOS_GetHourHex();
		systemDate.minute =  CMOS_GetMinHex8();
		systemDate.second = CMOS_GetSecHex();
		systemDate.weekDay = CMOS_GetDayOfWeek();
		
		/* 转换成本地时间 */
		/*if(systemDate.hour >= 16){
			systemDate.hour -= 16;
		}else{
			systemDate.hour += 8;
		}*/

	}while(systemDate.second != CMOS_GetSecHex());

	/* 初始化定时器 */
	InitTimer();

	/* 注册定时器软中断处理 */
	BuildSoftirq(TIMER_SOFTIRQ, TimerSoftirqHandler);

	/* 注册定时器软中断处理 */
	BuildSoftirq(SCHED_SOFTIRQ, SchedSoftirqHandler);

	/* 注册时钟中断并打开中断 */	
	RegisterIRQ(IRQ0_CLOCK, &ClockHandler, IRQF_DISABLED, "clockirq", "clock", 0);

}
