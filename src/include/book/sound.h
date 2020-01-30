/*
 * file:		include/book/sound.h
 * auther:		Jason Hu
 * time:		2019/1/17
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SOUND_H
#define _BOOK_SOUND_H

#include <share/types.h>

/* 音频的io控制命令 */
enum SoundIoctlCmds {
    SND_CMD_PLAY = 0,       /* 播放声音 */
    SND_CMD_STOP,           /* 停止声音 */
    SND_CMD_TURN_ON,        /* 打开播放器 */
    SND_CMD_TURN_OFF,       /* 关闭播放器 */
    SND_CMD_FREQUENCE,      /* 设置播放频率 */
};


PUBLIC int InitSoundSystem();

#endif   /* _BOOK_SOUND_H */
