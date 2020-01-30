/*
 * file:		include/arch/dma.h
 * auther:		Jason Hu
 * time:		2020/1/19
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_DMA_H
#define _ARCH_DMA_H

#include <share/stdint.h>
#include <share/types.h>

/* 最多有8个DMA通道 */
#define MAX_DMA_CHANNELS    8

/* DMA地址寄存器 */
#define DMA_ADDRESS_0    0x00
#define DMA_ADDRESS_1    0x02
#define DMA_ADDRESS_2    0x04
#define DMA_ADDRESS_3    0x06

#define DMA_ADDRESS_4    0xC0
#define DMA_ADDRESS_5    0xC4
#define DMA_ADDRESS_6    0xC8
#define DMA_ADDRESS_7    0xCC


/* DMA计数寄存器 */
#define DMA_COUNT_0     0x01
#define DMA_COUNT_1     0x03
#define DMA_COUNT_2     0x05
#define DMA_COUNT_3     0x07

#define DMA_COUNT_4     0xC2
#define DMA_COUNT_5     0xC6
#define DMA_COUNT_6     0xCA
#define DMA_COUNT_7     0xCE

/* DMA页寄存器 */
#define DMA_PAGE_0     0x87
#define DMA_PAGE_1     0x83
#define DMA_PAGE_2     0x81
#define DMA_PAGE_3     0x82
#define DMA_PAGE_5     0x8B
#define DMA_PAGE_6     0x89
#define DMA_PAGE_7     0x8A

/* DMA控制器寄存器 */
#define DMA1_CMD_REG        0x08   /* Command register(w) */
#define DMA1_STAT_REG       0x08   /* Status register(r) */
#define DMA1_REQ_REG        0x09   /* Request register(w) */
#define DMA1_MASK_REG       0x0A   /* single-channel mask(w) */
#define DMA1_MODE_REG       0x0B   /* Mode register(w) */
#define DMA1_CLEAR_FF_REG   0x0C   /* clear point flip-flop(w) */
#define DMA1_TEMP_REG       0x0D   /* Temporary register(r) */
#define DMA1_RESET_REG      0x0D   /* Master clear(w) */
#define DMA1_CLEAR_MASK_REG 0x0E   /* Clear Mask */
#define DMA1_MASK_ALL_REG   0x0F   /* All channel mask(w) */

#define DMA2_CMD_REG        0xD0   /* Command register(w) */
#define DMA2_STAT_REG       0xD0   /* Status register(r) */
#define DMA2_REQ_REG        0xD2   /* Request register(w) */
#define DMA2_MASK_REG       0xD4   /* single-channel mask(w) */
#define DMA2_MODE_REG       0xD6   /* Mode register(w) */
#define DMA2_CLEAR_FF_REG   0xD8   /* clear point flip-flop(w) */
#define DMA2_TEMP_REG       0xDA   /* Temporary register(r) */
#define DMA2_RESET_REG      0xDA   /* Master clear(w) */
#define DMA2_CLEAR_MASK_REG 0xDC   /* Clear Mask */
#define DMA2_MASK_ALL_REG   0xDE   /* All channel mask(w) */

enum Dma1MaskBits {
    DMA1_MASK_OFF = (0<<2),              /* 屏蔽开启 */
    DMA1_MASK_ON = (1<<2),               /* 屏蔽开启 */
    DMA1_MASK_CHANNEL_0 = (0<<0),        /* 通道0 */
    DMA1_MASK_CHANNEL_1 = (1<<0),        /* 通道1 */
    DMA1_MASK_CHANNEL_2 = (1<<1),        /* 通道2 */
    DMA1_MASK_CHANNEL_3 = (1<<0)|(1<<1), /* 通道3 */
};

enum Dma1ModeBits {
    DMA1_MODE_CHANNEL_0 = (0<<0),           /* 通道0 */
    DMA1_MODE_CHANNEL_1 = (1<<0),           /* 通道1 */
    DMA1_MODE_CHANNEL_2 = (1<<1),           /* 通道2 */
    DMA1_MODE_CHANNEL_3 = (1<<0)|(1<<1),    /* 通道3 */
    
    DMA1_MODE_TYPE_CHECK = (0<<2),          /* 校验传送 */
    DMA1_MODE_TYPE_WRITE = (1<<2),          /* 写传送 */
    DMA1_MODE_TYPE_READ = (1<<3),           /* 读传送 */
    DMA1_MODE_TYPE_UNDEF = (1<<2)|(1<<3),   /* 未定义类型 */
    DMA1_MODE_AUTO_ON = (1<<4),             /* 启动自动预置 */
    DMA1_MODE_AUTO_OFF = (0<<4),            /* 关闭自动预置 */
    DMA1_MODE_DIR_INC = (0<<5),             /* 地址方向递增 */
    DMA1_MODE_DIR_DEC = (1<<5),             /* 地址方向递减 */
    
    DMA1_MODE_METHOD_REQ = (0<<6),          /* 请求传送方式 */
    DMA1_MODE_METHOD_CHAR = (1<<6),         /* 单字节传送方式 */
    DMA1_MODE_METHOD_BLOCK = (1<<7),        /* 数据块传送方式 */
    DMA1_MODE_METHOD_CASC = (1<<6)|(1<<7),  /* 级联传送方式 */
};


enum Dma2MaskBits {
    DMA2_MASK_OFF = (0<<2),              /* 屏蔽开启 */
    DMA2_MASK_ON = (1<<2),               /* 屏蔽开启 */
    DMA2_MASK_CHANNEL_4 = (0<<0),        /* 通道4 */
    DMA2_MASK_CHANNEL_5 = (1<<0),        /* 通道5 */
    DMA2_MASK_CHANNEL_6 = (1<<1),        /* 通道6 */
    DMA2_MASK_CHANNEL_7 = (1<<0)|(1<<1), /* 通道7 */
};

enum Dma2ModeBits {
    DMA2_MODE_CHANNEL_4 = (0<<0),           /* 通道4 */
    DMA2_MODE_CHANNEL_5 = (1<<0),           /* 通道5 */
    DMA2_MODE_CHANNEL_6 = (1<<1),           /* 通道6 */
    DMA2_MODE_CHANNEL_7 = (1<<0)|(1<<1),    /* 通道7 */
    
    DMA2_MODE_TYPE_CHECK = (0<<2),          /* 校验传送 */
    DMA2_MODE_TYPE_WRITE = (1<<2),          /* 写传送 */
    DMA2_MODE_TYPE_READ = (1<<3),           /* 读传送 */
    DMA2_MODE_TYPE_UNDEF = (1<<2)|(1<<3),   /* 未定义类型 */
    DMA2_MODE_AUTO_ON = (1<<4),             /* 启动自动预置 */
    DMA2_MODE_AUTO_OFF = (0<<4),            /* 关闭自动预置 */
    DMA2_MODE_DIR_INC = (0<<5),             /* 地址方向递增 */
    DMA2_MODE_DIR_DEC = (1<<5),             /* 地址方向递减 */
    
    DMA2_MODE_METHOD_REQ = (0<<6),          /* 请求传送方式 */
    DMA2_MODE_METHOD_CHAR = (1<<6),         /* 单字节传送方式 */
    DMA2_MODE_METHOD_BLOCK = (1<<7),        /* 数据块传送方式 */
    DMA2_MODE_METHOD_CASC = (1<<6)|(1<<7),  /* 级联传送方式 */
};

#endif   /* _ARCH_DMA_H */
