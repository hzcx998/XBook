/*
 * file:		drivers/sound/pcspeaker.c
 * auther:		Jason Hu
 * time:		2020/1/17
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>

#include <arch/intel8253.h>
#include <arch/intel8255.h>

#include <share/string.h>

#include <drivers/pcspeaker.h>
#include <drivers/clock.h>

/**
 * SpeakerOn - 播放声音
 * 
 */
PRIVATE void SpeakerOn()
{
    uint8_t tmp;
    
    tmp = In8(PPI_OUTPUT);
    /* 开始播放，设定端口的低2位为1即可 */
    if (tmp != (tmp | 3)) {
 		Out8(PPI_OUTPUT, tmp | 3);
 	}
}

/**
 * SpeakerOn - 关闭声音
 * 
 */
PRIVATE void SpeakerOff()
{
    uint8_t tmp;
    
    tmp = In8(PPI_OUTPUT);
    /* 停止播放，需要设定端口的低2位为0 */
    Out8(PPI_OUTPUT, tmp & 0xfc);
}

/**
 * PcspeakerBeep - 发出声音
 * @frequence: 发声频率
 * 
 * 发声频率决定音高
 * 人能识别的HZ是20KHZ~20HZ，最低frequence取值20~20000
 */
PUBLIC void PcspeakerBeep(uint32_t frequence) {
 	uint32_t div;
 	
    // 求要传入的频率
 	div = TIMER_FREQ / (frequence * CLOCK_QUICKEN);
 	
    /* 即将设置蜂鸣器的频率 */
    Out8(PIT_CTRL, PIT_MODE_COUNTER_2 | PIT_MODE_MSB_LSB | 
            PIT_MODE_3 | PIT_MODE_BINARY);

    /* 设置低位和高位数据 */
 	Out8(PIT_COUNTER2, (uint8_t) (div));
 	Out8(PIT_COUNTER2, (uint8_t) (div >> 8));
    
    /* 播放 */
    SpeakerOn();
}

PRIVATE void SpeakerTest()
{
    int i;

    for (i = 20; i < 20000; i += 100) {
        PcspeakerBeep(i);
        SysMSleep(10);
    }

    SpeakerOff();
}

PUBLIC int InitPcspeakerDriver()
{
    SpeakerTest();

    return 0;
}
