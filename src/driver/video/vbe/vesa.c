/*
 * file:		video/vbe/vesa.c
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/vmarea.h>
#include <book/bitops.h>
#include <video/video.h>
#include <lib/stddef.h>
#include <lib/math.h>

#define DRV_NAME "video-VESA"

//#define VESA_DEBUG
//#define VESA_TEST

/* vesa信息的内存地址 */
#define VESA_INFO_ADDR  0x80001100
#define VESA_MODE_ADDR  0x80001300

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
	VideoDevice_t vdodev;               /* 视频设备 */
};

/* 图形的私有数据 */
PRIVATE struct VesaPrivate vesaPrivate;

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

/**
 * InitVesaDriver - 初始化图形驱动
 */
PUBLIC int InitVesaDriver()
{
    /* 在loader中从BIOS读取VBE数据到内存，现在可以获取之 */
    struct VbeModeInfoBlock *vbeModeInfo = (struct VbeModeInfoBlock *)VESA_MODE_ADDR;

    VideoInfo_t *info = &vesaPrivate.vdodev.info;
    /* 保存参数 */
    info->bitsPerPixel = vbeModeInfo->bitsPerPixel;
    info->bytesPerScanLine = vbeModeInfo->bytesPerScanLine;
    info->phyBasePtr = vbeModeInfo->phyBasePtr;
    info->xResolution = vbeModeInfo->xResolution;
    info->yResolution = vbeModeInfo->yResolution;
    
#ifdef VESA_DEBUG
    struct VbeInfoBlock *vbeInfo = (struct VbeInfoBlock *)VESA_INFO_ADDR;
    
    printk("sizeof vbe info block %d mode block %d\n", sizeof(struct VbeInfoBlock),
        sizeof(struct VbeModeInfoBlock));
    DumpVbeInfoBlock(vbeInfo);
    DumpVbeModeInfoBlock(vbeModeInfo);
    printk("screen size :%x int bytes:%x\n", self->screenSize, self->bytesScreenSize);
#endif  /* VESA_DEBUG */

    if (RegisterVideoDevice(&vesaPrivate.vdodev, DEV_VIDEO, DRV_NAME, 1,
        &vesaPrivate) == -1)
        return -1;

    printk("video: %d*%d*%d, phy:%x vir:%x\n", 
        info->xResolution, info->yResolution, info->bitsPerPixel, 
        info->phyBasePtr, info->virBasePtr);

    return 0;
}

/**
 * ExitVesaDriver - 退出图形驱动
 */
PUBLIC void ExitVesaDriver()
{
    
    UnregisterVideoDevice(&vesaPrivate.vdodev);
}
