/*
 * file:		drivers/sound/pcspeaker.c
 * auther:		Jason Hu
 * time:		2020/1/17
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>
#include <book/char.h>
#include <book/chr-dev.h>

#include <book/sound.h>

#include <arch/intel8253.h>
#include <arch/intel8255.h>

#include <share/string.h>

#include <drivers/pcspeaker.h>
#include <drivers/clock.h>

#define DRV_NAME "pcspeaker" 

//#define DEBUG_PCSPEAKER

struct SpeakerPrivate {
    struct CharDevice *chrdev;  /* 字符设备 */
    
    uint32_t frequence; /* 发声频率 */

};

PRIVATE struct SpeakerPrivate speakerPrivate; 

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

PRIVATE void SpeakerSetFrequence(uint32_t frequence)
{
    uint32_t div;
 	
    // 求要传入的频率
 	div = TIMER_FREQ / (frequence * CLOCK_QUICKEN);
 	
    /* 即将设置蜂鸣器的频率 */
    Out8(PIT_CTRL, PIT_MODE_COUNTER_2 | PIT_MODE_MSB_LSB | 
            PIT_MODE_3 | PIT_MODE_BINARY);

    /* 设置低位和高位数据 */
 	Out8(PIT_COUNTER2, (uint8_t) (div));
 	Out8(PIT_COUNTER2, (uint8_t) (div >> 8));
}

/**
 * PcspeakerBeep - 发出声音
 * @frequence: 发声频率
 * 
 * 发声频率决定音高
 * 人能识别的HZ是20KHZ~20HZ，最低frequence取值20~20000
 */
PUBLIC void PcspeakerBeep(uint32_t frequence)
{
    /* 设置工作频率 */
 	SpeakerSetFrequence(frequence);
    /* 播放 */
    SpeakerOn();
}

/**
 * SpeakerIoctl - speaker的IO控制
 * @device: 设备
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int SpeakerIoctl(struct Device *device, int cmd, int arg)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct SpeakerPrivate *self = (struct SpeakerPrivate *)chrdev->private;
    
	int retval = 0;
	switch (cmd)
	{
    case SND_CMD_PLAY:    /* 开始播放声音 */
        SpeakerOn();
        break;
    case SND_CMD_STOP:    /* 结束播放声音 */
        SpeakerOff();
        break;
    case SND_CMD_FREQUENCE:    /* 设置声音频率 */
        SpeakerSetFrequence(arg);
        break;
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

PRIVATE struct DeviceOperations speakerOpSets = {
	.ioctl = SpeakerIoctl,
};

PRIVATE void SpeakerTest()
{
#ifdef DEBUG_PCSPEAKER
    int i;

    for (i = 20; i < 20000; i += 100) {
        PcspeakerBeep(i);
        SysMSleep(10);
    }

    SpeakerOff();
#endif  /* DEBUG_PCSPEAKER */

}

PRIVATE int PcspeakerInitOne()
{
    struct SpeakerPrivate *self = &speakerPrivate;
    
    /* 设置一个字符设备号 */
    self->chrdev = AllocCharDevice(DEV_PCSPEAKER);
	if (self->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for pcspeaker failed!\n");
		return -1;
	}
    
	/* 初始化字符设备信息 */
	CharDeviceInit(self->chrdev, 1, self);
	CharDeviceSetup(self->chrdev, &speakerOpSets);

	CharDeviceSetName(self->chrdev, DRV_NAME);
	
	/* 把字符设备添加到系统 */
	AddCharDevice(self->chrdev);

    return 0;
}

PUBLIC int InitPcspeakerDriver()
{
    if (PcspeakerInitOne()) {
        return -1;
    }

    SpeakerTest();

    return 0;
}
