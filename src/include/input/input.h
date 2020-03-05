/*
 * file:		include/input/input.h
 * auther:		Jason Hu
 * time:		2020/2/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_INPUT_H
#define _DRIVER_INPUT_H

#include <lib/stdint.h>
#include <lib/types.h>
#include <book/ioqueue.h>


/* 输入设备类型 */
typedef enum InputDeviceType {
    INPUT_DEVICE_IOQUEUE = 1,
    INPUT_DEVICE_BUFFER,
} InputDeviceType_t;


/* 输入缓冲区类型 */
typedef enum InputBufferType {
    INPUT_BUFFER_MOUSE = 1,
} InputBufferType_t;


/* 鼠标缓冲区最大数 */
#define MAX_MOUSE_BUFFER_NR     16

/* 输入缓冲区之鼠标 */
typedef struct InputBufferMouse {
    InputBufferType_t type;
    uint8_t button;     /*  按钮 */
    int16_t xIncrease; /* 水平增长 */
    int16_t yIncrease; /* 垂直增长 */
    int16_t zIncrease; /* 纵深增长 */
} InputBufferMouse_t;

/* 缓冲区类型输入设备鼠标数据模型 */
typedef union InputBuffer {
    InputBufferType_t type;
    InputBufferMouse_t mouse;
} InputBuffer_t;

/* 输入设备 */
typedef struct InputDevice {
    struct CharDevice *chrdev;	/* 字符设备 */
    
    int irq;        /* 中断号 */

    InputDeviceType_t type;     /* 输入设备类型 */
    /* io队列类型输入设备 */
    struct IoQueue ioqueue; 
    uint8_t *iobuffer;
    
	/* 缓冲区类型输入设备 */
    InputBuffer_t *buffer;      /* 缓冲区地址 */
    uint32_t buffers;           /* 缓冲区数量 */
    
    uint32_t bufferLength;      /* 目前持有的缓冲区长度 */

    /* 缓冲区的长度 */
    uint32_t bufferSize;
} InputDevice_t;

PUBLIC int RegisterInputDevice(InputDevice_t *iptdev,
    dev_t devno,
    char *name,
    int count,
    void *private,
    struct DeviceOperations *ops,
    InputDeviceType_t type,
    int size,
    int number);

PUBLIC int UnregisterInputDevice(InputDevice_t *iptdev);

STATIC INLINE void InputDevicePutIoData(InputDevice_t *iptdev, uint32_t data)
{
    IoQueuePut(&iptdev->ioqueue, data);
}

STATIC INLINE uint32_t InputDeviceGetIoData(InputDevice_t *iptdev)
{
    return IoQueueGet(&iptdev->ioqueue);
}

PUBLIC void InputDevicePutBuffer(InputDevice_t *iptdev, InputBuffer_t *buffer);
PUBLIC int InputDeviceGetBuffer(InputDevice_t *iptdev, InputBuffer_t *buffer, uint32_t *buffers);

PUBLIC int InitInputSystem();

#endif  /* _DRIVER_INPUT_H */
