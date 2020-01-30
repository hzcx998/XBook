/*
 * file:		drivers/serial.c
 * auther:		Jason Hu
 * time:		2019/1/30
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/bitops.h>
#include <book/interrupt.h>

#include <share/vsprintf.h>

#include <book/char.h>
#include <book/chr-dev.h>

#include <drivers/serial.h>

#define DRV_NAME "com"

//#define SERIAL_TEST
//#define DEBUG_SERIAL

/* 串口的地址是IO地址 */
#define COM1_BASE   0X3F8
#define COM2_BASE   0X2F8
#define COM3_BASE   0X3E8
#define COM4_BASE   0X2E8

/* 最大波特率值 */
#define MAX_BAUD_VALUE  11520

/* 波特率值表：
Baud Rate   | Divisor 
50          | 2304  
110         | 1047  
220         | 524  
300         | 384  
600         | 192  
1200        | 96  
2400        | 48  
4800        | 24  
9600        | 12  
19200       | 6  
38400       | 3  
57600       | 2  
115200      | 1  

Divisor计算方法：
Divisor = 115200 / BaudRate 
*/

/* 默认波特率值 */
#define DEFAULT_BAUD_VALUE  11520
#define DEFAULT_DIVISOR_VALUE (MAX_BAUD_VALUE / DEFAULT_BAUD_VALUE)

#define SERIAL_IRQ_4    IRQ4
#define SERIAL_IRQ_3    IRQ3

/* 串口调试端口的索引 */
#define SERIAL_DEGUB_IDX    0

enum FifoControlRegisterBits {
    FIFO_ENABLE = 1,                             /* Enable FIFOs */
    FIFO_CLEAR_RECEIVE   = (1 << 1),             /* Clear Receive FIFO */
    FIFO_CLEAR_TRANSMIT  = (1 << 2),             /* Clear Transmit FIFO */
    FIFO_DMA_MODE_SELECT = (1 << 3),             /* DMA Mode Select */
    FIFO_RESERVED        = (1 << 4),             /* Reserved */
    FIFO_ENABLE_64       = (1 << 5),             /* Enable 64 Byte FIFO(16750) */
    /* Interrupt Trigger Level/Trigger Level  */
    FIFO_TRIGGER_1       = (0 << 6),             /* 1 Byte */
    FIFO_TRIGGER_4       = (1 << 6),             /* 4 Byte */
    FIFO_TRIGGER_8       = (1 << 7),             /* 8 Byte */
    FIFO_TRIGGER_14      = (1 << 6) | (1 << 7),  /* 14 Byte */
};

enum LineControlRegisterBits {
    /* Word Length */
    LINE_WORD_LENGTH_5   = 0,                    /* 5 Bits */
    LINE_WORD_LENGTH_6   = 1,                    /* 6 Bits */
    LINE_WORD_LENGTH_7   = (1 << 1),             /* 7 Bits */
    LINE_WORD_LENGTH_8   = ((1 << 1) | 1),       /* 8 Bits */
    LINE_STOP_BIT_1      = (0 << 2),             /* One Stop Bit */
    LINE_STOP_BIT_2      = (1 << 2),             /* 1.5 Stop Bits or 2 Stop Bits */
        /* Parity Select */
    LINE_PARITY_NO       = (0 << 3),             /* No Parity */
    LINE_PARITY_ODD      = (1 << 3),             /* Odd Parity */
    LINE_PARITY_EVEN     = (1 << 3) | (1 << 4),  /* Even Parity */
    LINE_PARITY_MARK     = (1 << 3) | (1 << 5),  /* Mark */
    LINE_PARITY_SPACE    = (1 << 3) | (1 << 4) | (1 << 5), /* Space */
    LINE_BREAK_ENABLE    = (1 << 6),             /* Set Break Enable */
    LINE_DLAB            = (1 << 7),             /* Divisor Latch Access Bit */
};
enum InterruptEnableRegisterBits {
    INTR_RECV_DATA_AVALIABLE = 1,        /* Enable Received Data Available Interrupt */
    INTR_TRANSMIT_HOLDING    = (1 << 1), /* Enable Transmitter Holding Register Empty Interrupt */
    INTR_RECV_LINE_STATUS    = (1 << 2), /* Enable Receiver Line Status Interrupt */
    INTR_MODEM_STATUS        = (1 << 3), /* Enable Modem Status Interrupt */
    INTR_SLEEP_MODE          = (1 << 4), /* Enable Sleep Mode(16750) */
    INTR_LOW_POWER_MODE      = (1 << 5), /* Enable Low Power Mode(16750) */
    INTR_RESERVED1           = (1 << 6), /* Reserved */
    INTR_RESERVED2           = (1 << 7), /* Reserved */
};

enum LineStatusRegisterBits {
    LINE_STATUS_DATA_READY                  = 1,        /* Data Ready */
    LINE_STATUS_OVERRUN_ERROR               = (1 << 1), /* Overrun Error */
    LINE_STATUS_PARITY_ERROR                = (1 << 2), /* Parity Error */
    LINE_STATUS_FRAMING_ERROR               = (1 << 3), /* Framing Error */
    LINE_STATUS_BREAK_INTERRUPT             = (1 << 4), /* Break Interrupt */
    LINE_STATUS_EMPTY_TRANSMITTER_HOLDING   = (1 << 5), /* Empty Transmitter Holding Register */
    LINE_STATUS_EMPTY_DATA_HOLDING          = (1 << 6), /* Empty Data Holding Registers */
    LINE_STATUS_ERROR_RECEIVE_FIFO          = (1 << 7), /* Error in Received FIFO */
};

enum InterruptIdentificationRegisterBits {
    INTR_STATUS_PENDING_FLAG        = 1,        /* Interrupt Pending Flag */
    /* 产生的什么中断 */                
    INTR_STATUS_MODEM               = (0 << 1), /* Transmitter Holding Register Empty Interrupt	 */
    INTR_STATUS_TRANSMITTER_HOLDING = (1 << 1), /* Received Data Available Interrupt */
    INTR_STATUS_RECEIVE_DATA        = (1 << 2), /* Received Data Available Interrupt */
    INTR_STATUS_RECEIVE_LINE        = (1 << 1) | (1 << 2),  /* Receiver Line Status Interrupt */
    INTR_STATUS_TIME_OUT_PENDING    = (1 << 2) | (1 << 3),  /* Time-out Interrupt Pending (16550 & later) */
    INTR_STATUS_64BYTE_FIFO         = (1 << 5), /* 64 Byte FIFO Enabled (16750 only) */
    INTR_STATUS_NO_FIFO             = (0 << 6), /* No FIFO on chip */
    INTR_STATUS_RESERVED_CONDITION  = (1 << 6), /* Reserved condition */
    INTR_STATUS_FIFO_NOT_FUNC       = (1 << 7), /* FIFO enabled, but not functioning */
    INTR_STATUS_FIFO                = (1 << 6) | (1 << 7),  /* FIFO enabled */
};

/* 最多有4个串口 */
#define MAX_COM_NR  4 

/* 默认初始化的串口数 */
#define DEFAULT_COM_NR  2 

/* 每一个com端口都有一个串行结构来描述 */
struct SerialPrivate {
    char irq;           /* irq号 */
    /* ----串口的寄存器---- */
    uint16_t ioBase;                    /* IO基地址 */
    uint16_t dataRegister;              /* 数据寄存器，读数据/写数据 */
    uint16_t divisorLowRegister;        /* 除数寄存器低8位 */
    uint16_t interruptEnableRegister;   /* 中断使能寄存器 */
    uint16_t divisorHighRegister;       /* 除数寄存器高8位 */
    uint16_t interruptIdentificationRegister;       /* 中断标识寄存器 */
    uint16_t fifoRegister;       /* fifo寄存器 */
    uint16_t lineControlRegister;       /* 行控制寄存器 */
    uint16_t modemControlRegister;       /* 调制解调器控制寄存器 */
    uint16_t lineStatusRegister;       /* 行状态寄存器 */
    uint16_t modemStatusRegister;       /* 调制解调器状态寄存器 */
    uint16_t scratchRegister;       /* 刮伤寄存器 */

    uint8_t serialID;           /* 串口id，对应着哪个串口 */

    struct CharDevice *chrdev;  /* 字符设备 */

    uint8_t receivedData;       /* 接收到的数据 */
    uint8_t receiveFlags;       /* 接收标志，判断是否有数据接收到 */
};

/* 4个串口 */
struct SerialPrivate serialTable[DEFAULT_COM_NR];

/**
 * SerialReceive - 从串口接收数据
 * @self: 私有结构体指针
 * 
 * @return: 返回收到的数据
 */
PRIVATE uint8_t SerialReceive(struct SerialPrivate *self)
{
    /* 如果接收的时候数据没有准备好，就不能接收 */
    while (!(In8(self->lineStatusRegister) & 
            LINE_STATUS_DATA_READY));

    /* 从数据端口读取数据 */
    return In8(self->dataRegister);
}

/**
 * SerialGetc - 从串口读取字符
 * @device: 设备指针
 * 
 */
PRIVATE int SerialGetc(struct Device *device)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct SerialPrivate *self = (struct SerialPrivate *)chrdev->private;

    return SerialReceive(self);
}

/**
 * SerialRead - 从串口读取数据到缓冲区
 * @device: 设备指针
 * @off: 偏移（未用）
 * @buffer: 缓冲区
 * @len: 长度
 * 
 * 读取数据到缓冲区中。
 */
PRIVATE int SerialRead(struct Device *device, uint32_t off, void *buffer, size_t len)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct SerialPrivate *self = (struct SerialPrivate *)chrdev->private;
    char *p = (char *)buffer;
    while (len--) {
        *p++ = SerialReceive(self);
    }
    return 0;
}

/**
 * SerialSend - 串口发送数据
 * @self: 私有结构体指针
 * @data: 传输的数据
 */
PRIVATE int SerialSend(struct SerialPrivate *self, char data)
{
    /* 如果发送的时候不持有传输状态，就不能发送 */
    while (!(In8(self->lineStatusRegister) & 
        LINE_STATUS_EMPTY_TRANSMITTER_HOLDING));

    /* 往数据端口写入数据 */
    Out8(self->dataRegister, data);
    return 0;
}

/**
 * SerialDebugPutChar - 串口调试输出字符
 * @ch: 要输出的字符
 */
PUBLIC void SerialDebugPutChar(char ch)
{
    /* 如果是回车，就需要发送一个'\r'，以兼容unix/linux操作系统的输出 */
    if(ch == '\n')
        SerialSend(&serialTable[SERIAL_DEGUB_IDX], '\r');
    
	SerialSend(&serialTable[SERIAL_DEGUB_IDX], ch);
}

/**
 * SerialPutc - 往串口输出字符
 * @device: 设备指针
 * @ch: 字符
 * 
 */
PRIVATE int SerialPutc(struct Device *device, unsigned int ch)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct SerialPrivate *self = (struct SerialPrivate *)chrdev->private;
    
    SerialSend(self, ch);
    return 0;
}

/**
 * SerialWrite - 把缓冲区中的数据写到串口
 * @device: 设备指针
 * @off: 偏移（未用）
 * @buffer: 缓冲区
 * @len: 长度
 * 
 * 把缓冲区中的数据写到串口中
 */
PRIVATE int SerialWrite(struct Device *device, uint32_t off, void *buffer, size_t len)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct SerialPrivate *self = (struct SerialPrivate *)chrdev->private;
    char *p = (char *)buffer;
    while (len--) {
        SerialSend(self, *p++);
    }
    return 0;
}
/**
 * SerialIoctl - 串口的IO控制
 * @device: 设备
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int SerialIoctl(struct Device *device, int cmd, int arg)
{
    /*
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct SerialPrivate *self = (struct SerialPrivate *)chrdev->private;
    */
	int retval = 0;
	switch (cmd)
	{
    case 0:    /* ... */
        
        break;
    default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

/**
 * SerialHandler - 串口的中断处理
 * @irq: 中断号
 * @data: 数据（一般是指向私有结构体数据）
 * 
 * 当收到串口数据的时候，产生串口中断，并且读取串口数据，通知系统
 */
PRIVATE void SerialHandler(unsigned int irq, unsigned int data)
{
    struct SerialPrivate *self = (struct SerialPrivate *)data;
    uint8_t status = In8(self->interruptIdentificationRegister);
    //printk("status: %x com-%d irq-%d intr status ", status, self->serialID, self->irq);

    if (status & INTR_STATUS_RECEIVE_DATA) {
        /* 读取数据寄存器做应答 */
        self->receivedData = In8(self->dataRegister);
        self->receiveFlags = 1; /* 收到了数据 */
#ifdef DEBUG_SERIAL

        printk("receive data %x.\n", self->receivedData);
#endif  /* DEBUG_SERIAL */
    } else if (status & INTR_STATUS_TRANSMITTER_HOLDING) {
#ifdef DEBUG_SERIAL
        printk("transmitter holding.\n");
#endif  /* DEBUG_SERIAL */
        /* 读取中断标识寄存器做应答 */
        //Out8(self->dataRegister, 0x00 );
        In8(self->interruptIdentificationRegister);
    } else if (status & INTR_STATUS_RECEIVE_LINE) {
#ifdef DEBUG_SERIAL

        printk("receive line.\n");
#endif  /* DEBUG_SERIAL */        
        /* 读取行状态寄存器做应答 */
        In8(self->lineStatusRegister);
        
    } else if (status & INTR_STATUS_TIME_OUT_PENDING) {
#ifdef DEBUG_SERIAL

        printk("time-out pending.\n");
#endif  /* DEBUG_SERIAL */
        /* 读取数据寄存器做应答 */
        In8(self->dataRegister);
    }
    /*if (status & INTR_STATUS_64BYTE_FIFO) {
        printk("64 byte FIFO.\n");
    }
    if (status & INTR_STATUS_NO_FIFO) {
        printk("no FIFO.\n");
    } else if (status & INTR_STATUS_RESERVED_CONDITION) {
        printk("reserved contion.\n");
    } else if (status & INTR_STATUS_FIFO_NOT_FUNC) {
        printk("FIFO enabled, but not functioning .\n");
    } else if (status & INTR_STATUS_FIFO) {
        printk("FIFO enabled, but not functioning .\n");
    }*/
    
}

/* 串口设备操作集 */
PRIVATE struct DeviceOperations serialOpSets = {
	.ioctl  = SerialIoctl,
    .putc   = SerialPutc,
    .getc   = SerialGetc,
    .read   = SerialRead,
    .write  = SerialWrite,
};

/**
 * SerialInitSub - 串口初始化子程序
 * @self: 指向私有数据的指针
 * @id: 串口的id
 * 
 * @return: 成功返回0，失败返回-1
 */
PRIVATE int SerialInitSub(struct SerialPrivate *self, uint8_t id)
{
    char irq;
    uint16_t ioBase;

    /* 根据ID设置iobase和irq */
    switch (id)
    {
    case 0:
        ioBase = COM1_BASE;
        irq = SERIAL_IRQ_4;
        break;
    case 1:
        ioBase = COM2_BASE;
        irq = SERIAL_IRQ_3;
        break;
    case 2:
        ioBase = COM3_BASE;
        irq = SERIAL_IRQ_4;
        break;
    case 3:
        ioBase = COM4_BASE;
        irq = SERIAL_IRQ_3;
        break;
    default:
        break;
    }

    /* 串口的寄存器参数设置(对齐后很好看！！！) */
    self->ioBase                            = ioBase;
    self->dataRegister                      = ioBase + 0;
    self->divisorLowRegister                = ioBase + 0;
    self->interruptEnableRegister           = ioBase + 1;
    self->divisorHighRegister               = ioBase + 1;
    self->interruptIdentificationRegister   = ioBase + 2;
    self->lineControlRegister               = ioBase + 3;
    self->modemControlRegister              = ioBase + 4;
    self->lineStatusRegister                = ioBase + 5;
    self->modemStatusRegister               = ioBase + 6;
    self->scratchRegister                   = ioBase + 7;

    /* 设置串口的id */
    self->serialID = id;

    /* irq号 */
    self->irq = irq;
    
    /* 接收的数据 */
    self->receivedData = 0; /* 接收到的数据 */
    self->receiveFlags = 0; /* 没有收到数据 */
#ifdef DEBUG_SERIAL    
    printk("com%d, base:%x irq:%d\n", id, ioBase, irq);
#endif  /* DEBUG_SERIAL */
    
    /* ----执行设备的初始化---- */

    /* 设置可以更改波特率Baud */
    Out8(self->lineControlRegister, LINE_DLAB);

    /* Set Baud rate to 115200，设置除数寄存器为新的波特率值 */
    Out8(self->divisorLowRegister, LOW8(DEFAULT_DIVISOR_VALUE));
    Out8(self->divisorHighRegister, HIGH8(DEFAULT_DIVISOR_VALUE));
    
    /* 设置 DLAB to 0, 设置字符宽度为 8, 停字为 to 1, 没有奇偶校验, 
    Break signal Disabled */
    Out8(self->lineControlRegister, LINE_WORD_LENGTH_8 | 
            LINE_STOP_BIT_1 | LINE_PARITY_NO);
    
    /* 关闭所有串口中断，所以无需注册中断号 */
    Out8(self->interruptEnableRegister, INTR_RECV_DATA_AVALIABLE | 
        INTR_RECV_LINE_STATUS | INTR_LOW_POWER_MODE); 

    /* 设置FIFO，打开FIFO, 清除接收 FIFO, 清除传输 FIFO
    打开 64Byte FIFO, 中断触发等级为 14Byte
     */
    Out8(self->fifoRegister, FIFO_ENABLE | FIFO_CLEAR_TRANSMIT |
                FIFO_CLEAR_RECEIVE | FIFO_ENABLE_64 | 
                FIFO_TRIGGER_14);

    /* 无调制解调器设置 */            
    Out8(self->modemControlRegister, 0x00);
    /* 无刮伤寄存器设置 */            
    Out8(self->scratchRegister, 0x00);

    return 0;
}

/**
 * SerialInitOne - 初始化一个串口
 * @self: 指向私有数据的指针
 * 
 * @return: 成功返回0，失败返回-1
 */
PRIVATE int SerialInitOne(struct SerialPrivate *self)
{
    uint8_t id = self - serialTable;
    if (id < 0 || id >= DEFAULT_COM_NR)
        return -1;
    
    /* 初始化串口的相关信息及设置 */
    if (SerialInitSub(self, id))
        return -1;
    
    /* 设置中断 */
    RegisterIRQ(self->irq, SerialHandler, IRQF_DISABLED, "serial", DRV_NAME, (uint32_t )self);

    /* 添加字符设备 */
    
    /* 设置一个字符设备号 */
    self->chrdev = AllocCharDevice(MKDEV(COM_MAJOR, id));
	if (self->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for serial failed!\n");
		return -1;
	}
    
	/* 初始化字符设备信息 */
	CharDeviceInit(self->chrdev, MAX_COM_NR, self);
	CharDeviceSetup(self->chrdev, &serialOpSets);
    
    char devname[DEVICE_NAME_LEN] = {0};
    /* com1,com2,com3,com4*/
    sprintf(devname, "%s%d", DRV_NAME, id + 1);
	CharDeviceSetName(self->chrdev, devname);
	
	/* 把字符设备添加到系统 */
	AddCharDevice(self->chrdev);

    return 0;
}

PRIVATE void SerialTest()
{
#ifdef SERIAL_TEST
    int i = 0;
    while (1) {
        i++;
        SerialSend(&serialTable[0], i);
    }
#endif  /* SERIAL_TEST */
}

/**
 * InitSerialDebugDriver - 初始化串口调试的驱动
 * 
 * 当打开串口调试的时候，由于进入系统初期，需要输出一些数据，
 * 所以需要提前初始化串口1，但是，一般的驱动初始化都需要用设备驱动框架，
 * 所以，本次初始化是例外的，不使用驱动框架内容。
 */
#ifdef CONFIG_SERIAL_DEBUG
PUBLIC int InitSerialDebugDriver()
{
    /* 默认把COM1作为调试的串口 */
    struct SerialPrivate *self = &serialTable[SERIAL_DEGUB_IDX];    /* 指向第一个串口 */

    /* 第一个串口的索引是0 */
    if (SerialInitSub(self, SERIAL_DEGUB_IDX))
        return -1;

    return 0;
}
#endif  /* CONFIG_SERIAL_DEBUG */

/**
 * InitSerialDriver - 初始化串口驱动
 * 
 */
PUBLIC int InitSerialDriver()
{
    int i;
    for (i = 0; i < DEFAULT_COM_NR; i++) {
        if (SerialInitOne(&serialTable[i])) {
            printk("init serial failed!\n");
        }
    }

    SerialTest();

    return 0;
}
