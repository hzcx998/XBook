/*
 * file:		include/book/byteorder.h
 * auther:		Jason Hu
 * time:		2019/8/19
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*
字序转换，大端字序和小端字序
*/

#ifndef _BOOK_BYTEORDER_H
#define _BOOK_BYTEORDER_H

#include <share/types.h>
#include <share/stdint.h>

/**
 * BytesSwap16 - 16位的字节交换
 * @data: 需要交换的数据
 * 
 * 返回交换后的数据
 */
STATIC INLINE uint16_t BytesSwap16(uint16_t data)
{
  return (uint16_t)((data & 0xFF) << 8) | ((data & 0xFF00) >> 8);
}

/**
 * BytesSwap32 - 32位的字节交换
 * @data: 需要交换的数据
 * 
 * 返回交换后的数据
 */
STATIC INLINE uint32_t BytesSwap32(uint32_t data)
{
  return (uint32_t)((data & 0xFF) << 24) | ((data & 0xFF00) << 8) | \
        ((data & 0xFF0000) >> 8) | ((data & 0xFF000000) >> 24);
}

/**
 * GetEndianness - 获取CPU字序
 * 
 * 是0就是小端，是1就是大端
 */
STATIC INLINE int GetCpuEndianness()  
{  
    short s = 0x0110;  
    char *p = (char *) &s;  
    if (p[0] == 0x10)
        return 0; // 小端格式  
    else  
        return 1; // 大端格式  
}


/**
 * CpuToLittleEndian16 - cpu字序转换成小端字序
 * data: 需要转换的数据
 */
STATIC INLINE uint16_t CpuToLittleEndian16(uint16_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return BytesSwap16(data);   /* 返回交换后的数据 */
    } else {    /* 是小端 */
        return data;    /* 已经是小端了，就不用转换，直接返回 */
    }
}

/**
 * CpuToLittleEndian32 - cpu字序转换成小端字序
 * data: 需要转换的数据
 */
STATIC INLINE uint32_t CpuToLittleEndian32(uint32_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return BytesSwap32(data);   /* 返回交换后的数据 */
    } else {    /* 是小端 */
        return data;    /* 已经是小端了，就不用转换，直接返回 */
    }
}

/**
 * CpuToBigEndian16 - cpu字序转换成大端字序
 * data: 需要转换的数据
 */
STATIC INLINE uint16_t CpuToBigEndian16(uint16_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return data;    /* 已经是大端了，就不用转换，直接返回 */
    } else {    /* 是小端 */
        return BytesSwap16(data);   /* 返回交换后的数据 */
    }
}

/**
 * CpuToBigEndian32 - cpu字序转换成大端字序
 * data: 需要转换的数据
 */
STATIC INLINE uint32_t CpuToBigEndian32(uint32_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return data;    /* 已经是大端了，就不用转换，直接返回 */
    } else {    /* 是小端 */
        return BytesSwap32(data);   /* 返回交换后的数据 */
    }
}

/**
 * BigEndian16ToCpu - 大端字序转换成cpu字序
 * data: 需要转换的数据
 */
STATIC INLINE uint16_t BigEndian16ToCpu(uint16_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return data;    /* 已经是大端了，就不用转换，直接返回 */
    } else {    /* 是小端 */
        return BytesSwap16(data);   /* 返回交换后的数据 */
    }
}

/**
 * BigEndian32ToCpu - 大端字序转换成cpu字序
 * data: 需要转换的数据
 */
STATIC INLINE uint32_t BigEndian32ToCpu(uint32_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return data;    /* 已经是大端了，就不用转换，直接返回 */
    } else {    /* 是小端 */
        return BytesSwap32(data);   /* 返回交换后的数据 */
    }
}


/**
 * LittleEndian16ToCpu - 小端字序转换成cpu字序
 * data: 需要转换的数据
 */
STATIC INLINE uint16_t LittleEndian16ToCpu(uint16_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return BytesSwap16(data);   /* 返回交换后的数据 */
    } else {    /* 是小端 */
        return data;    /* 已经是小端了，就不用转换，直接返回 */
    }
}

/**
 * LittleEndian32ToCpu - 小端字序转换成cpu字序
 * data: 需要转换的数据
 */
STATIC INLINE uint32_t LittleEndian32ToCpu(uint32_t data)
{
    /* 如果是大端 */
    if (GetCpuEndianness()) {
        return BytesSwap32(data);   /* 返回交换后的数据 */
    } else {    /* 是小端 */
        return data;    /* 已经是小端了，就不用转换，直接返回 */
    }
}



#endif   /* _BOOK_BYTEORDER_H */
