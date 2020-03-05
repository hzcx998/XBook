/*
 * file:		input/core/input.c
 * auther:		Jason Hu
 * time:		2020/2/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/kgc.h>
#include <input/input.h>
#include <lib/string.h>

/* ----驱动程序初始化文件导入---- */
EXTERN int InitPs2KeyboardDriver();
EXTERN int InitPs2MouseDriver();
EXTERN int InitTTYDriver();
/* ----驱动程序初始化文件导入完毕---- */

InputBuffer_t mouseBuffers[MAX_MOUSE_BUFFER_NR];   /* 存放的完整数据包 */

Task_t *taskMouseEven;

PRIVATE void MouseEvenWakeup()
{
    if (taskMouseEven->status == TASK_BLOCKED) {
        TaskUnblock(taskMouseEven);
    }
}

PRIVATE void KeyboardEven(void *arg)
{
    uint32_t key;
    while (1) {
        key = DeviceGetc(DEV_KEYBOARD);
        if (key) {
            //printk("io: %x\n", key);    
            /* 把数据交给输入系统 */
            KGC_KeyboardInput(key);
            
        }
            
    }
}

PRIVATE void MouseEven(void *arg)
{
    uint32_t retval, buffers = 0;
    int i;
    InputBuffer_t *buffer;
    while (1) {
        retval = DeviceRead(DEV_MOUSE, 0, &mouseBuffers[0], (unsigned int )&buffers);
        if (!retval) {
            for (i = 0; i < buffers; i++) {
                buffer = &mouseBuffers[i];
                //printk("%x, %d, %d, %d\n", buffer->mouse.button, buffer->mouse.xIncrease, buffer->mouse.yIncrease, buffer->mouse.zIncrease);   
                
                /* 把数据交给输入系统 */
                KGC_MouseInput((InputBufferMouse_t *)buffer);
            }
        } else {
            TaskBlock(TASK_BLOCKED);
        }
    }
}

PUBLIC int RegisterInputDevice(InputDevice_t *iptdev,
    dev_t devno,
    char *name,
    int count,
    void *private,
    struct DeviceOperations *ops,
    InputDeviceType_t type,
    int size,
    int number)
{
    if (!iptdev)
        return -1;
    iptdev->chrdev = AllocCharDevice(devno);
	if (iptdev->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for input failed!\n");
		return -1;
	}
	/* 初始化字符设备信息 */
	CharDeviceInit(iptdev->chrdev, count, private);
	CharDeviceSetup(iptdev->chrdev, ops);

	CharDeviceSetName(iptdev->chrdev, name);
	
    iptdev->type = type; 
    iptdev->bufferSize = number * size;
    
    iptdev->iobuffer = NULL;
    iptdev->buffer = NULL;
    
    if (type == INPUT_DEVICE_IOQUEUE) {
        iptdev->iobuffer = kmalloc(iptdev->bufferSize, GFP_KERNEL);
        if (iptdev->iobuffer == NULL) {
            FreeCharDevice(iptdev->chrdev);
            return -1;
        }
        /* 初始化io队列 */
        IoQueueInit(&iptdev->ioqueue, iptdev->iobuffer, iptdev->bufferSize, size);

    } else if (type == INPUT_DEVICE_BUFFER) {
        iptdev->buffer = kmalloc(iptdev->bufferSize, GFP_KERNEL);
        if (iptdev->buffer == NULL) {
            FreeCharDevice(iptdev->chrdev);
            return -1;
        }
        iptdev->buffers = 0;
        iptdev->bufferLength = number;
        /* 初始化缓冲区内存 */
        memset(iptdev->buffer, 0, iptdev->bufferSize);
    }
	/* 把字符设备添加到系统 */
	AddCharDevice(iptdev->chrdev);

    return 0;
}

PUBLIC int UnregisterInputDevice(InputDevice_t *iptdev)
{
    if (!iptdev)
        return -1;
    
    if (iptdev->type == INPUT_DEVICE_IOQUEUE) {
        kfree(iptdev->iobuffer);
    } else if (iptdev->type == INPUT_DEVICE_BUFFER) {
        
    }
	DelCharDevice(iptdev->chrdev);
    FreeCharDevice(iptdev->chrdev);
    return 0;
}

PUBLIC void InputDevicePutBuffer(InputDevice_t *iptdev, InputBuffer_t *buffer)
{
    if (iptdev->buffers < iptdev->bufferLength) {
        iptdev->buffer[iptdev->buffers] = *buffer;
        iptdev->buffers++;
        
        /* 唤醒缓冲区处理任务 */
        MouseEvenWakeup();
    }
}

PUBLIC int InputDeviceGetBuffer(InputDevice_t *iptdev, InputBuffer_t *buffer, uint32_t *buffers)
{
    int retval = -1;
    unsigned long eflags = InterruptSave();
    /* 如果有缓冲区，才进行数据复制 */
    if (iptdev->buffers > 0) {
        
        /* 处理数据 */
        InputBuffer_t *p = (InputBuffer_t *)buffer;
        
        /* 传出包的数量 */
        *(uint32_t *)buffers = iptdev->buffers;
        
        int idx = 0;
        /* 循环传递 */
        while (idx < iptdev->buffers) {
            *p = iptdev->buffer[idx];
            p++;
            idx++;
        }
        iptdev->buffers = 0;
        retval = 0;
        goto ToEnd;
    }
ToEnd:
    InterruptRestore(eflags);
    return retval;
}


PRIVATE void InitInputDrivers()
{
    
#ifdef CONFIG_DRV_KEYBOARD
	/* 初始化键盘驱动 */
	if (InitPs2KeyboardDriver()) {
        printk("init keyboard driver failed!\n");
    }
#endif  /* CONFIG_DRV_KEYBOARD */
    
#ifdef CONFIG_DRV_MOUSE
    if (InitPs2MouseDriver()) {
        printk("init tty driver failed!\n");
    }
#endif  /* CONFIG_DRV_MOUSE */
    
    
#ifdef CONFIG_DRV_TTY
    if (InitTTYDriver()) {
        printk("init tty driver failed!\n");
    }
#endif  /* CONFIG_DRV_TTY */

}

/**
 * InitInputSystem - 初始化输入系统
 */
PUBLIC int InitInputSystem()
{
    InitInputDrivers();

    DeviceOpen(DEV_KEYBOARD, 0);
    DeviceOpen(DEV_MOUSE, 0);
    
    ThreadStart("event-kbd", TASK_PRIORITY_RT, KeyboardEven, NULL);
    taskMouseEven = ThreadStart("event-mouse", TASK_PRIORITY_RT, MouseEven, NULL);
    if (taskMouseEven == NULL) {
        return -1;
    }
    //while (1);

    return 0;
}
