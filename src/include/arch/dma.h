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

#endif   /* _ARCH_DMA_H */
