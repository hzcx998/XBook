/*
 * file:		video/vbe/vesa.c
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/*
VESA驱动程序数据传输协议

*/
#include <book/arch.h>
#include <book/debug.h>
#include <book/vmarea.h>
#include <book/bitops.h>
#include <char/chr-dev.h>
#include <lib/stddef.h>
#include <lib/math.h>
#include <kgc/video.h>

#define DRV_NAME "video"

//#define VESA_DEBUG
//#define VESA_TEST

/* vesa信息的内存地址 */
#define VESA_INFO_ADDR  0x80001100
#define VESA_MODE_ADDR  0x80001300

enum VesaFlagsBits {
    VESA_FLAGS_INIT_DONE = 1,        /* 初始化完成标志 */
};

/* 单个像素由多少位组成 */
enum BitsPerPixel {
    BITS_PER_PIXEL_8    = 8,    /* 1像素8位 */
    BITS_PER_PIXEL_16   = 16,   /* 1像素16位 */
    BITS_PER_PIXEL_24   = 24,   /* 1像素24位 */
    BITS_PER_PIXEL_32   = 32,   /* 1像素32位 */
};
/* 生成5-6-5格式的像素 */
#define ARGB_TO_16(r, g, b) ((((r >> 3) & 0x1f) << 11)   | \
    (((g >> 2)& 0x3f) << 5)   | ((b >> 3) & 0x1f ))

/* VBE信息块结构体 */
struct VbeInfoBlock {
    uint8_t vbeSignature[4];       /* VEB Signature: 'VESA' */
    uint16_t vbeVeision;            /* VEB Version:0300h */
    uint32_t oemStringPtr;         /* VbeFarPtr to OEM string */
    uint8_t capabilities[4];       /* Capabilities of graphics controller */
    uint32_t videoModePtr;         /* VbeFarPtr to VideoModeList */
    uint16_t totalMemory;           /* Number of 64kb memory blocks added for VEB2.0+ */
    uint16_t oemSoftwareRev;        /* VEB implementation Software revision */
    uint32_t oemVendorNamePtr;     /* VbeFarPtr to Vendor Name String */
    uint32_t oemProductNamePtr;    /* VbeFarPtr to Product Name String */
    uint32_t oemProductRevPtr;     /* VbeFarPtr to Product Revision String */
    uint8_t reserved[222];         /* Reserved for VBE implementation scratch area */
    uint8_t oemData[256];          /* Data Area for OEM String */
} PACKED;

struct VbeModeInfoBlock {
    /* Mandatory information for all VBE revisions */
    uint16_t modeAttributes;        /* mode attributes */
    uint8_t winAAttributes;        /* window A attributes */
    uint8_t winBAttributes;        /* window B attributes */
    uint16_t winGranulaity;         /* window granulaity */
    uint16_t winSize;               /* window size */
    uint16_t winASegment;           /* window A start segment */
    uint16_t winBSegment;           /* window B start segment */
    uint32_t winFuncPtr;           /* real mode pointer to window function */
    uint16_t bytesPerScanLine;      /* bytes per scan line */
    /* Mandatory information for VBE1.2 and above */
    uint16_t xResolution;           /* horizontal resolution in pixels or characters */
    uint16_t yResolution;           /* vertical resolution in pixels or characters */
    uint8_t xCharSize;             /* character cell width in pixels */
    uint8_t yCharSize;             /* character cell height in pixels */
    uint8_t numberOfPlanes;        /* number of banks */
    uint8_t bitsPerPixel;          /* bits per pixel */
    uint8_t numberOfBanks;         /* number of banks */
    uint8_t memoryModel;           /* memory model type */
    uint8_t bankSize;              /* bank size in KB */
    uint8_t numberOfImagePages;    /* number of images */
    uint8_t reserved0;             /* reserved for page function: 1 */
    uint8_t redMaskSize;           /* size of direct color red mask in bits */
    uint8_t redFieldPosition;      /* bit position of lsb of red mask */
    uint8_t greenMaskSize;         /* size of direct color green mask in bits */
    uint8_t greenFieldPosition;    /* bit position of lsb of green mask */
    uint8_t blueMaskSize;          /* size of direct color blue mask in bits */
    uint8_t blueFieldPosition;     /* bit position of lsb of blue mask */
    uint8_t rsvdMaskSize;          /* size of direct color reserved mask in bits */
    uint8_t rsvdFieldPosition;     /* bit position of lsb of reserved mask */
    uint8_t directColorModeInfo;   /* direct color mode attributes */
    
    /* Mandatory information for VBE2.0 and above */
    uint32_t phyBasePtr;           /* physical address for flat memory frame buffer */
    uint32_t reserved1;            /* reserved-always set to 0 */
    uint16_t reserved2;             /* reserved-always set to 0 */
    /* Mandatory information for VBE3.0 and above */
    uint16_t linebytesPerScanLine;  /* bytes per scan line for linear modes */
    uint8_t bnkNumberOfImagePages; /* number of images for banked modes */
    uint8_t linNumberOfImagePages; /* number of images for linear modes */
    uint8_t linRedMaskSize;        /* size of direct color red mask(linear modes) */
    uint8_t linRedFieldPosition;   /* bit position of lsb of red mask(linear modes) */
    uint8_t linGreenMaskSize;      /* size of direct color green mask(linear modes) */
    uint8_t linGreenFieldPosition; /* bit position of lsb of green mask(linear modes) */
    uint8_t linBlueMaskSize;       /* size of direct color blue mask(linear modes) */
    uint8_t linBlueFieldPosition;  /* bit position of lsb of blue mask(linear modes) */
    uint8_t linRsvdMaskSize;       /* size of direct color reserved mask(linear modes) */
    uint8_t linRsvdFieldPosition;  /* bit position of lsb of reserved mask(linear modes) */
    uint32_t maxPixelClock;        /* maximum pixel clock (in HZ) for graphics mode */
    uint8_t reserved3[189];        /* remainder of ModeInfoBlock */
} PACKED;

/*
图形的私有数据
*/
struct VesaPrivate {
	uint8_t bitsPerPixel;               /* 每个像素的位数 */
    uint32_t bytesPerScanLine;          /* 单行的字节数 */
    uint32_t xResolution, yResolution;  /* 分辨率x，y */
    uint32_t phyBasePtr;                /* 物理显存的指针 */
    
    uint32_t bytesScreenSize;           /* 整个屏幕占用的字节大小 */
    uint32_t screenSize;                /* 整个屏幕占用的大小（x*y） */
    
    uint8_t *vram;                      /* 显存映射到内存中的地址 */
    struct CharDevice *chrdev;	        /* 字符设备 */
    uint8_t flags;                      /* 驱动标志 */

    /* 函数指针 */
    void (*vesaWriteOnePixel)(struct VesaPrivate *, int , int , uint32_t );
};

#ifdef VESA_DEBUG
PRIVATE void DumpVbeInfoBlock(struct VbeInfoBlock *info)
{
    if (info == NULL)
        return;
    printk("VBE info block:\n");
    printk("vbe signature:%c%c%c%c\n", info->vbeSignature[0],
        info->vbeSignature[1], info->vbeSignature[2], info->vbeSignature[3]);
    printk("vbe version:%x\n", info->vbeVeision);
    printk("OEM string ptr:%x\n", info->oemStringPtr);
    /* 低8位有效，其他位reserved */
    printk("capabilities:%x\n", info->capabilities[0]);
    printk("video mode ptr:%x\n", info->videoModePtr);
    printk("total memory:%d\n", info->totalMemory);
    printk("oem software reversion:%x\n", info->oemSoftwareRev);
    printk("oem vendor name ptr:%x\n", info->oemVendorNamePtr);
    printk("oem product name ptr:%x\n", info->oemProductNamePtr);
    printk("oem product reversion ptr:%x\n", info->oemProductRevPtr);

}

PRIVATE void DumpVbeModeInfoBlock(struct VbeModeInfoBlock *info)
{
    if (info == NULL)
        return;

    printk("VBE mode info block:\n");
    printk("mode attributes:%x\n", info->modeAttributes);
    printk("horizontal resolution in pixels:%d\n", info->xResolution);
    printk("vertical resolution in pixels:%d\n", info->yResolution);
    printk("bits per pixel:%d\n", info->bitsPerPixel);
    printk("physical address for flat memory frame buffer:%x\n", info->phyBasePtr);
    printk("bytesPerScanLine:%x\n", info->bytesPerScanLine);
    
}
#endif /* VESA_DEBUG */

/* 图形的私有数据 */
PRIVATE struct VesaPrivate vesaPrivate;

/**
 * VesaGetc - 从图形获取一个按键
 * @device: 设备
 */
PRIVATE int VesaGetc(struct Device *device)
{
    return 0;    /* 读取图形 */
}

/**
 * VesaWriteOnePixel8 - 写入单个像素8位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VesaWriteOnePixel8(struct VesaPrivate *self, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = self->vram + (y * self->xResolution + x);

    /* 写入像素数据 */
    *p = LOW8(value);
}

/**
 * VesaWriteOnePixel16 - 写入单个像素16位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VesaWriteOnePixel16(struct VesaPrivate *self, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = self->vram + ((y * self->xResolution + x) * 2); // 左位移1相当于乘以2

    /* 写入像素数据 */
    *(uint16_t *)p = ARGB_TO_16((value >> 16) , (value >> 8), (value));
}

/**
 * VesaWriteOnePixel24 - 写入单个像素24位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VesaWriteOnePixel24(struct VesaPrivate *self, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = self->vram + (y * self->xResolution + x)*3;
    
    /* 写入像素数据 */
    *(uint16_t *)p = LOW16(value);
    p += 2;
    *p = LOW8(HIGH16(value));

}

/**
 * VesaWriteOnePixel32 - 写入单个像素32位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VesaWriteOnePixel32(struct VesaPrivate *self, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    
    /* 获取像素位置 */
    p = self->vram + ((y * self->xResolution + x) << 2); // 左移2位相当于乘以4
    
    /* 写入像素数据 */
    *(uint32_t *)p = value;
}

/**
 * VesaWrite - 绘制图形
 * 
 * 
 */
PRIVATE int VesaWrite(struct Device *device, unsigned int offset, void *buffer, unsigned int size)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct VesaPrivate *self = (struct VesaPrivate *)chrdev->private;
	
    if (self->vesaWriteOnePixel) {   
        uint32_t *p = (uint32_t *)buffer;

        int x0 = HIGH16(offset), y0 = LOW16(offset);
        int x1 = x0 + HIGH16(size);
        int y1 = y0 + LOW16(size);
        for (y0 = LOW16(offset); y0 < y1; y0++) {    
            for (x0 = HIGH16(offset); x0 < x1; x0++) {
                /* 在屏幕范围内才绘制 */
                if (x0 >= 0 && y0 >= 0 && x0 < self->xResolution && y0 < self->yResolution) {
                    /* 绘制像素 */
                    self->vesaWriteOnePixel(self, x0, y0, *p++);
                }
            }
        }
    }

    return 0;
}

/**
 * VesaWriteDirect - 直接使用绘制功能，不通过驱动层。
 */
PUBLIC int VesaWriteDirect(int x, int y, unsigned int color)
{
    struct VesaPrivate *self = &vesaPrivate;
    if (self->vesaWriteOnePixel) {
        /* 在屏幕范围内才绘制 */
        if (x >= 0 && y >= 0 && x < self->xResolution && y < self->yResolution) {
            /* 绘制像素 */
            self->vesaWriteOnePixel(self, x, y, color);
        }
    }
    
    return 0;
}

/**
 * VesaIoctl - 图形的IO控制
 * @device: 设备项
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int VesaIoctl(struct Device *device, int cmd, int arg)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    struct VesaPrivate *self = (struct VesaPrivate *)chrdev->private;
	int retval = 0;
    KGC_VideoInfo_t *video;    /* 数据包 */
	switch (cmd)
	{
	case GRAPH_IO_BASE_PACKT:   /* 获取水平分辨率 */
        video = (KGC_VideoInfo_t *)arg;
        /* 复制包的数据 */
        video->bitsPerPixel = self->bitsPerPixel;
        video->xResolution = self->xResolution;
        video->yResolution = self->yResolution;
        video->bytesPerScanLine = self->bytesPerScanLine;
        break;
    
    default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

PRIVATE struct DeviceOperations vesaOpSets = {
	.ioctl = VesaIoctl, 
    .getc = VesaGetc,
    .write = VesaWrite,
};

PRIVATE void VesaTest()
{
#ifdef VESA_TEST
    struct VesaPrivate *self = &vesaPrivate;

    /* 映射后就可以对该地址进行读写 */
    int y, x;
    for (y = 0; y < self->yResolution; y++) {
        for (x = 0; x < self->xResolution; x++) {
            VesaWriteDirect(x, y, 1, 1, 0xffaa55);
        }
    }
#endif /* VESA_TEST */
}

/**
 * InitVesaDriver - 初始化图形驱动
 */
PUBLIC int VesaInitOne(struct VesaPrivate *self)
{    

	/* 初始化私有数据 */
	self->flags = 0;    // 标志清0
    
    /* 在loader中从BIOS读取VBE数据到内存，现在可以获取之 */
    struct VbeModeInfoBlock *vbeModeInfo = (struct VbeModeInfoBlock *)VESA_MODE_ADDR;

    /* 保存参数 */
    self->bitsPerPixel = vbeModeInfo->bitsPerPixel;
    self->bytesPerScanLine = vbeModeInfo->bytesPerScanLine;
    self->phyBasePtr = vbeModeInfo->phyBasePtr;
    self->xResolution = vbeModeInfo->xResolution;
    self->yResolution = vbeModeInfo->yResolution;
    
    self->bytesScreenSize = self->bytesPerScanLine * self->yResolution;
    
    /* 设置函数处理指针 */
    switch (self->bitsPerPixel)
    {
    case BITS_PER_PIXEL_8:
        self->vesaWriteOnePixel = VesaWriteOnePixel8;    
        break;
    case BITS_PER_PIXEL_16:
        self->vesaWriteOnePixel = VesaWriteOnePixel16;    
        break;
    case BITS_PER_PIXEL_24:
        self->vesaWriteOnePixel = VesaWriteOnePixel24;    
        break;
    case BITS_PER_PIXEL_32:
        self->vesaWriteOnePixel = VesaWriteOnePixel32;    
        break;
    default:
        self->vesaWriteOnePixel = NULL;    
        break;
    }

    #ifdef VESA_DEBUG
    struct VbeInfoBlock *vbeInfo = (struct VbeInfoBlock *)VESA_INFO_ADDR;
    
    printk("sizeof vbe info block %d mode block %d\n", sizeof(struct VbeInfoBlock),
        sizeof(struct VbeModeInfoBlock));
    DumpVbeInfoBlock(vbeInfo);
    DumpVbeModeInfoBlock(vbeModeInfo);
    printk("screen size :%x int bytes:%x\n", self->screenSize, self->bytesScreenSize);
#endif  /* VESA_DEBUG */

    /* 对设备的显存进行IO映射到虚拟地址，就可以访问该地址来访问显存 */
    self->vram = IoRemap(self->phyBasePtr, self->bytesScreenSize);
    if (self->vram == NULL) {
        printk("IO remap for vesa driver failed!\n");
        return -1;
    } else {
        /* 设置初始化完成标志 */
        self->flags = VESA_FLAGS_INIT_DONE;
    }
    printk("video: %d*%d*%d, phy:%x vir:%x\n", 
        self->xResolution, self->yResolution, self->bitsPerPixel, 
        self->phyBasePtr, self->vram);

    /* 分配一个字符设备 */
	self->chrdev = AllocCharDevice(DEV_VIDEO);
	if (self->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for Vesa failed!\n");
		return -1;
	}

	/* 初始化字符设备信息 */
	CharDeviceInit(self->chrdev, 1, &vesaPrivate);
	CharDeviceSetup(self->chrdev, &vesaOpSets);

	CharDeviceSetName(self->chrdev, DRV_NAME);
	
	/* 把字符设备添加到系统 */
	AddCharDevice(self->chrdev);
    
	return 0;
}

/**
 * InitVesaDriver - 初始化图形驱动
 */
PUBLIC int InitVesaDriver()
{
    
	if (VesaInitOne(&vesaPrivate)) {
        printk("init vesa driver failed!\n ");
    }
    
    //VesaTest();

    //while (1);
    return 0;
}

/**
 * ExitVesaDriver - 退出图形驱动
 */
PUBLIC void ExitVesaDriver()
{
    struct VesaPrivate *self = &vesaPrivate;

	DelCharDevice(self->chrdev);
	FreeCharDevice(self->chrdev);
}
