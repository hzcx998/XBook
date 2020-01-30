/*
 * file:		drivers/sound/sb16.c
 * auther:		Jason Hu
 * time:		2020/1/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>
#include <book/interrupt.h>
#include <book/lowmem.h>

#include <arch/intel8253.h>
#include <arch/intel8255.h>
#include <arch/dma.h>

#include <share/string.h>
#include <share/math.h>
#include <share/stddef.h>

#include <drivers/sb16.h>

#include <fs/fs.h>

#include <user/wav.h>

#ifdef CONFIG_SB16

#define DEVNAME "sound blaster 16"

/* 配置信息设置 */
#define SB16_TIME_CONST

/* 输入输出模式 */
enum SB16IoMode {
    SB16_IO_INPUT = 0,
    SB16_IO_OUTPUT,
};

/* 输入输出模式 */
enum SB16WorkMode {
    SB16_WORK_8 = 8,    /* 8位工作模式 */
    SB16_WORK_16 = 16,  /* 16位工作模式 */
};

#define SB16_IO_MODE SB16_IO_OUTPUT
#define SB16_WORK_MODE SB16_WORK_8

/* -------- */


//IO端口基地址
#define SB16_BASE 0x0220

#define SB16_MIXER_REG      (SB16_BASE + 4)      /* 混合寄存器端口 */
#define SB16_MIXER_DATA_REG (SB16_MIXER_REG + 1)  /* 混合寄存器数据端口 */

#define DSP_RESET_REG      (SB16_BASE + 6)      /* 重置DSP（W） */
#define DSP_READ_REG       (SB16_BASE + 0xa)    /* 读取数据(R) */
#define DSP_WRITE_BS_REG   (SB16_BASE + 0xc)    /* 写缓冲区状态(R/W) */
#define DSP_WRITE_CMD_REG  (SB16_BASE + 0xc)    /* 发送命令(R/W) */
#define DSP_WRITE_DATA_REG (SB16_BASE + 0xc)    /* 写入数据(R/W) */
#define DSP_READ_BS_REG    (SB16_BASE + 0xe)    /* 读缓冲区状态(R) */
#define DSP_ACK_REG        (SB16_BASE + 0xe)    /* 应答(R) */
#define DSP_ACK_16_REG     (SB16_BASE + 0xf)    /* 应答16位(R) */

/* DSP命令 */
enum DspCmds {
    DSP_CMD_VERSION = 0xE1,                 /* 获取版本命令 */             
    DSP_CMD_TURN_ON_SPEAKER = 0xD1,         /* 打开喇叭 */
    DSP_CMD_TURN_OFF_SPEAKER = 0xD3,        /* 关闭喇叭 */
    DSP_CMD_GET_SPEAKER_STATUS = 0xD8,      /* 获取喇叭状态 */
                 
    

};

enum SB16IntrStatus {
    SB16_INTR_STATUS_8DMA = (1<<0),      /* 8位DMA模式的中断 */
    SB16_INTR_STATUS_MIDI = (1<<0),      /* MIDI模式的中断 */
    SB16_INTR_STATUS_16DMA = (1<<1),      /* 16位DMA模式的中断 */
    SB16_INTR_STATUS_MPU = (1<<2),      /* MPU-401模式的中断 */ 
};


/* dma通道号，16位模式是5号通道，8位模式是1号通道 */
#define SB16_DMA1_CHANNEL  1
#define SB16_DMA2_CHANNEL  5

/* 文件分割成chunk大小一份 */
#define SB16_LOAD_CHUNK_SIZE 8192

/* 每次声卡操作的数据块的大小 */
#define SB16_DATA_BLOCK_LENGTH    32*1024

//#define SB16_DATA_BLOCK_LENGTH    256

#define lo(value) (unsigned char)((value) & 0x00FF)
#define hi(value) (unsigned char)((value) >> 8)

/* 私有数据结构体 */
struct SB16Private {
    uint16_t ioBase;        /* io基地址 */

    int irq;      /* 中断号 */
    uint16_t dmaMaskPort;   /* dma屏蔽端口 */
    uint16_t dmaClearPort;   /* dma清除端口 */
    uint16_t dmaModePort;   /* dma模式端口 */
    uint16_t dmaBaseAddrPort;   /* dma基地址端口 */
    uint16_t dmaCountPort;   /* dma字节数端口 */
    uint16_t dmaPagePort;   /* dma页面端口 */
    uint16_t dmaStopMask;   /* dma停止掩码 */
    uint16_t dmaStartMask;   /* dma开始掩码 */
    uint8_t dmaChannelNR;   /* 通道号 */
    uint8_t dmaMode;    /* DMA模式 */
    dma_addr_t dmaBufferAddr; /* DMA缓冲区物理地址 */
    dma_addr_t dmaBufferOffset; /* DMA缓冲区偏移 */
    dma_addr_t dmaBufferPage; /* DMA缓冲区页面地址 */
    uint32_t dmaBufferLength; /* DMA缓冲区长度 */
    uint32_t dmaBlockLength; /* DMA缓冲区长度 */
    
    uint32_t samplingrate;  /* 采样率 */
    uint32_t numChannels;  /* 通道数 */
    
    enum SB16IoMode ioMode; /* 输入输出模式 */
    enum SB16WorkMode workMode; /* 工作模式 */
    bool done;      /* 完成一次工作 */
    int32_t samplesRemaining;  /* 剩余的要处理的数据 */
    uint8_t currentBlock;   /* 2个块轮换使用 */
    uint8_t versionMajor, versionMinor; /* 版本号 */
    uint8_t speakerStatus;  /* 喇叭的状态，0xff为打开，0x00为关闭 */
};

/* 创建一个私有数据对象 */
struct SB16Private sb16Private;

PRIVATE uint8_t ReadDsp()
{
    /* 当第7位不为1时就一直循环等待，如果为1就可以从DSP_READ_REG读取数据 */
    while (!(In8(DSP_READ_BS_REG) & 0x80));
    /* 从读端口读取数据 */
    return In8(DSP_READ_REG);
}

PRIVATE void WriteDsp(uint8_t data)
{
    /* 当第7位为1时就一直循环等待，如果为0就可以往里面写入数据了 */
    while (In8(DSP_WRITE_BS_REG) & 0x80);
    
    /* 写入数据到写入端口 */
    Out8(DSP_WRITE_DATA_REG, data);   
}

/**
 * ResetDsp - 重置DSP
 * 
 * @return: 成功返回非0，失败返回0
 */
PRIVATE int ResetDsp()
{
    /* 往重置寄存器写入1，开始重置 */
    Out8(DSP_RESET_REG, 1);
    /* 等待3us,每个in指令花费1us */
    In8(0x80);
    In8(0x80);
    In8(0x80);
    /* 清除重置位 */
    Out8(DSP_RESET_REG, 0);
    
    int i = 0xffff;
    /* 是否有数据准备读取 */
    while (!(In8(DSP_READ_BS_REG) & 0x80) && i--);

    /* 没有数据准备好，重置失败 */
    if (!i) 
        return 0;
    i = 1000;
    /* 如果是0XAA就表示重置成功 */
    while ((In8(DSP_READ_REG) != 0xAA) && i--);
    return i;
}
PRIVATE int done;
/**
 * SoundBlasterHandler - 声霸卡中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void SoundBlasterHandler(unsigned int irq, unsigned int data)
{
	printk("sound int occur!\n");
    struct SB16Private *self = &sb16Private;
    /* 1.对中断状态进行判断 */

    /* 选择混合寄存器的0x82索引的寄存器 */
    Out8(SB16_MIXER_REG, 0x82);
    
    /* 读取中断状态 */
    uint8_t status = In8(SB16_MIXER_DATA_REG);
    /* 如果是共享中断，那么可以根据这个返回 */
    if (status == 0) {
        printk("!not sound blaster 16 intr!\n");
        return;
    }

    if (status & SB16_INTR_STATUS_8DMA) {
        printk("8 bit DMA intr occur!\n");
    } 
    if (status & SB16_INTR_STATUS_MIDI) {
        printk("MIDI intr occur!\n");
    } 
    if (status & SB16_INTR_STATUS_16DMA) {
        printk("16 bit DMA intr occur!\n");
    }
    if (status & SB16_INTR_STATUS_MPU) {
        printk("MPU intr occur!\n");
    }

    /* 退出自动初始化模式 */
    //WriteDsp(0xda);

    /* 关闭扬声器 */
    //WriteDsp(0xd3);

    //printk("play sound ok!\n");
    
    /* 2.数据处理 */
    printk("remain:%d", self->samplesRemaining);

    self->samplesRemaining -= self->dmaBlockLength;

    self->currentBlock = !self->currentBlock;         /* Toggle current block */

    printk("exit auto!\n");
    
    /* 退出自动化 */
    WriteDsp(0xDa);

    if (done) {
        /* 关闭 */
        WriteDsp(0xD3);
    }

    done = TRUE;
    
    In16(DSP_ACK_REG);
    
    return;

    if (self->samplesRemaining < 0 && self->done == FALSE) {
        printk("play done!\n");
        self->done = TRUE;
        WriteDsp(0xD9); /* 退出16位自动初始化模式 */
    } else if (self->done) { /* 如果已经完成传输 */    
        printk("exit sound!\n");
        WriteDsp(0xD5); /* 暂停16位音频输出 */
        
        /* 关闭扬声器 */
        WriteDsp(0xd3);

        ResetDsp();
    }

    /* 3.对声卡中断应答 */
    In16(DSP_ACK_16_REG);
}

PRIVATE int SoundBlasterStartIO(uint64_t length)
{
    struct SB16Private *self = &sb16Private;

    char *p = Phy2Vir(self->dmaBufferAddr);
    int i;
    for (i = 0; i < 15; i++) {
        printk("%x ", p[i]);
    }
    
    self->done = FALSE;
    self->samplesRemaining = length;
    self->currentBlock = 0; // 最开始是第一个块
    self->samplesRemaining -= self->dmaBlockLength;
    /* ----开始设置DMA----*/
    /*
    使用主片上的通道1。
    设定DMA主片屏蔽寄存器，以免受到DMA请求信号的打扰
    */
    Out8(self->dmaMaskPort, self->dmaStopMask);
    
    /* 初始化写入顺序 */
    Out8(self->dmaClearPort, 0);
    
    /* 设置基准地址寄存器 */
    
    Out8(self->dmaBaseAddrPort, lo(self->dmaBufferAddr));        // 0~7位
    Out8(self->dmaBaseAddrPort, hi(self->dmaBufferAddr)); // 8~15位
    
    /* 最高4位写入到页面寄存器 */
    Out8(self->dmaPagePort, self->dmaBufferPage);   // 16~19位
    
    /* 设置基准字数寄存器 */
    Out8(self->dmaCountPort, lo(self->dmaBufferLength - 1));        // 0~7位
    Out8(self->dmaCountPort, hi(self->dmaBufferLength - 1)); // 8~15位
    
    /* 设置传输模式 */
    Out8(self->dmaModePort, self->dmaMode);
    
    /* 取消对通道的屏蔽 */
    Out8(self->dmaMaskPort, self->dmaStartMask);

    /* ----声卡编程---- */

    /* 设置采样率 */
#ifdef SB16_TIME_CONST
    
    /* 计算时间常量，是16位值 */
    uint16_t time = 65536-(256000000/(self->numChannels*self->samplingrate));
    printk("const time:%d\n", time);

    /* 准备写入回放时间常量 */
    WriteDsp(0x40);
    
    printk("ready write time!\n");
    /* SB-16只使用其高8位 */
    
    WriteDsp((time >> 8) & 0xff);
    printk("write time done!\n");
#else
    switch (self->ioMode)
    {
    case SB16_IO_INPUT:
        WriteDsp(0x42); /* Set input sampling rate  */
        printk("set input sampling ");

        break;
    case SB16_IO_OUTPUT:
        WriteDsp(0x41); /* Set output sampling rate */
        printk("set output sampling ");

        break;
    default:
        break;
    }
    /* 注意，这里是高位先写入，低位后写入 */
    WriteDsp(hi(self->samplingrate)); /* High byte of sampling rate */
    WriteDsp(lo(self->samplingrate)); /* Low byte of sampling rate */
    printk("done!\n");
#endif
    /* 打开扬声器 */
    WriteDsp(0xd1);
    //EnableIRQ(self->irq);

    printk("work mode:%d\n", self->workMode);
    /* 设置DSP数据大小 */
    switch (self->ioMode)
    {
    case SB16_IO_INPUT:
        WriteDsp(0xBE); /* 16-bit A->D, A/I, FIFO   */
        
        break;
    case SB16_IO_OUTPUT:
        WriteDsp(0xB0); /* 16-bit D->A, A/I, FIFO   */
        
        break;
    default:
        break;
    }
    /* 根据文件内容设置传输模式，
    0x00：8-bit mono unsigned PCM
    0x20：8-bit stereo unsigned PCM
    0x10：16-bit mono signed PCM
    0x30：16-bit stereo signed PCM
    */
    WriteDsp(0x10);                         /* DMA Mode:  16-bit signed mono */
    
    WriteDsp(lo(self->dmaBlockLength -1));  /* Low byte of block length      */
    WriteDsp(hi(self->dmaBlockLength -1));  /* High byte of block length     */     
    
    /* 启动音频回放 */
    printk("start paly!\n");
    
    return 0;
}

/**
 * WavLoadData - WAV文件加载数据
 * @fname: 文件名字
 * @wavHeader: 头信息
 * 
 * @return: 成功返回一个装载数据后的缓冲区，失败返回NULL
 */
PRIVATE uint8_t *WavLoadData(const char *fname, Wav_t *wavHeader)
{
    int fd = SysOpen(fname, O_RDONLY);
    if (fd < 0) {
        printk("wav file open failed!\n");
        return NULL;
    }
    /* 获取wav文件头 */
    Wav_t wav;
    memset(&wav, 0, sizeof(Wav_t));
    if (SysRead(fd, &wav, sizeof(Wav_t)) != sizeof(Wav_t)) {
        printk("wav file read header failed!\n");
        return NULL;
    }
    memcpy(wavHeader, &wav, sizeof(Wav_t));

    WavRIFF_t *riff = &wav.riff;
    WavFMT_t *fmt = &wav.fmt;
    WavData_t *data = &wav.data;
    /* 打印文件头信息 */
    printk("Wave file header:\n");
    printk("chunkID:%c%c%c%c\n", riff->chunkID[0],riff->chunkID[1],
            riff->chunkID[2],riff->chunkID[3]);
    printk("chunkSize:%d\n", riff->chunkSize);
    printk("format:%c%c%c%c\n\n", riff->format[0],riff->format[1],
            riff->format[2],riff->format[3]);
    
    printk("subchunk1ID:%c%c%c%c\n", fmt->subchunk1ID[0],fmt->subchunk1ID[1],
            fmt->subchunk1ID[2],fmt->subchunk1ID[3]);
    printk("subchunk1Size:%d\n", fmt->subchunk1Size);
    printk("audioFormat:%d\n", fmt->audioFormat);
    printk("numChannels:%d\n", fmt->numChannels);
    printk("sampleRate:%d\n", fmt->sampleRate);
    printk("byteRate:%d\n", fmt->byteRate);
    printk("blockAlign:%d\n", fmt->blockAlign);
    printk("bitsPerSample:%d\n\n", fmt->bitsPerSample);

    printk("subchunk2ID:%c%c%c%c\n", data->subchunk2ID[0],data->subchunk2ID[1],
            data->subchunk2ID[2],data->subchunk2ID[3]);
    printk("subchunk2Size:%d\n", data->subchunk2Size);
    
    /* 读取数据到内存中 */
    uint8_t *buffer = kmalloc(data->subchunk2Size, GFP_KERNEL);
    if (buffer == NULL) {    
        printk("kmalloc for wave data failed!\n");
        SysClose(fd);
        return NULL;
    }

    if (SysRead(fd, buffer, data->subchunk2Size) != data->subchunk2Size) {
        printk("read wave data failed!\n");
        kfree(buffer);
        SysClose(fd);
        return NULL;
    }
    
    int i;
    for (i = 0; i < 16; i++) {
        printk("%x ", buffer[i]);
    }
    
    printk("\n");
    
    printk("%x %x %x %x\n", buffer[data->subchunk2Size-4],buffer[data->subchunk2Size-3],
            buffer[data->subchunk2Size-2],buffer[data->subchunk2Size-1]);
    
    SysClose(fd);
    return buffer;
}

PRIVATE int SB16AllocBuffer()
{
    /* 分配一个缓冲区，并把数据复制进去 */
    void *viraddr = lmalloc(1);
    if (viraddr == NULL) {
        return -1;
    }
    struct SB16Private *self = &sb16Private;
    
    self->dmaBufferAddr = Vir2Phy(viraddr);
    printk("dma addr:%x\n", self->dmaBufferAddr);

    self->dmaBufferPage = self->dmaBufferAddr >> 16;
    
    /* 每次DMA传输1个块，2个块轮换使用 */
    self->dmaBufferLength = SB16_DATA_BLOCK_LENGTH*2;
    /*if (wavHeader.fmt.bitsPerSample == 8) {
        self->dmaBlockLength = self->dmaBufferLength / 2;
    } else if (wavHeader.fmt.bitsPerSample == 16) {
        self->dmaBlockLength = self->dmaBufferLength / 4;
    }*/
    return 0;
}

PRIVATE int LoadBuffer()
{
    Wav_t wavHeader;
    uint8_t *buffer = WavLoadData("root:/wav", &wavHeader);
    if (buffer == NULL) {
        return -1;
    }
    /* 读取文件内容到DMA缓冲区，并转换成物理地址 */
    struct SB16Private *self = &sb16Private;
    
    /* 复制数据到低端内存 */
    memcpy(Phy2Vir(self->dmaBufferAddr), buffer, MIN(SB16_DATA_BLOCK_LENGTH * 2, wavHeader.data.subchunk2Size));

    self->samplingrate = wavHeader.fmt.sampleRate; 
    self->numChannels = wavHeader.fmt.numChannels;

    return 0;
}

/*
Sound Blaster 16 声卡支持双声道（立体声）、 每声道 16 位样本、 最高 44.1KHz 的采
样和回放。
*/
PRIVATE int SoundBlasterInitOne()
{
    struct SB16Private *self = &sb16Private;

    printk("init sound blaster start.\n");
    if (!ResetDsp())
        return -1;

    /* 发出读取版本命令 */
    WriteDsp(DSP_CMD_VERSION);
    self->versionMajor = ReadDsp();
    self->versionMinor = ReadDsp();
    
    printk("version major %d minor %d\n", self->versionMajor, self->versionMinor);

    /* 设置喇叭状态 */
    self->speakerStatus = 0x00;
    printk("speaker status:%x\n", self->speakerStatus);

    /* 设置默认IO模式 */
    self->ioMode = SB16_IO_MODE;

    /* 设置中断号，5号中断 */
    self->irq = IRQ5_PARALLEL2; 

    /* 设置16位工作模式 */
    self->workMode = SB16_WORK_MODE;

    /* 设置采样率 */
    self->samplingrate = 0;
    
    /* 设置DMA端口和参数 */
    self->dmaMaskPort = DMA2_MASK_REG; 
    self->dmaClearPort = DMA2_CLEAR_FF_REG; 
    self->dmaModePort = DMA2_MODE_REG; 
    self->dmaChannelNR = SB16_DMA2_CHANNEL; /* DMA通道 */
    self->dmaBaseAddrPort = DMA_ADDRESS_4 + 4 * (self->dmaChannelNR - 4); 
    self->dmaCountPort = DMA_COUNT_4 + 4 * (self->dmaChannelNR - 4); 
    
    /* 设置页号端口 */
    switch (self->dmaChannelNR) {
    case 5: 
        self->dmaPagePort = DMA_PAGE_5;
        break;
    case 6: 
        self->dmaPagePort = DMA_PAGE_6;
        break;
    case 7: 
        self->dmaPagePort = DMA_PAGE_7;
        break;
    default:
        break;    
    }

    self->dmaStopMask = (self->dmaChannelNR - 4) + DMA2_MASK_ON;
    self->dmaStartMask = (self->dmaChannelNR - 4) + DMA2_MASK_OFF;

    switch (self->ioMode) {
    case SB16_IO_INPUT:
        /* 010101xx */
        self->dmaMode = (self->dmaChannelNR - 4) + (DMA2_MODE_TYPE_WRITE |
            DMA2_MODE_AUTO_ON | DMA2_MODE_DIR_INC | DMA2_MODE_METHOD_CHAR);
        break;
    case SB16_IO_OUTPUT:
        /* 010110xx */
        
        self->dmaMode = (self->dmaChannelNR - 4) + (DMA2_MODE_TYPE_READ |
            DMA2_MODE_AUTO_ON | DMA2_MODE_DIR_INC | DMA2_MODE_METHOD_CHAR);
        break;
    
    default:
        break;
    }

    printk("Sound Blaster init done!\n");
    
    /* 注册中断处理函数 */
	RegisterIRQ(self->irq, SoundBlasterHandler, 0, "IRQ5", DEVNAME, 0);
    //DisableIRQ(self->irq);
    printk("Sound Blaster interrupt register done!\n");
    
    /* 打印数据 */
    
    printk("sb16:io mode:%d irq:%d work mode:%d\n", self->ioMode, self->irq, self->workMode);
    printk("DMA: mask port:%x clear port:%x mode port:%x chanel:%d base port:%x count port:%x\n",
            self->dmaMaskPort, self->dmaClearPort, self->dmaModePort,
            self->dmaChannelNR, self->dmaBaseAddrPort, self->dmaCountPort);
    printk("page port:%x stop mask:%x start mask:%x mode:%x\n",
        self->dmaPagePort, self->dmaStopMask, self->dmaStartMask, self->dmaMode);    
    //while (1);
    return 0;
    
    /* ----设置声卡----*/

    /* 计算时间常量，是16位值 */
    //uint16_t time = 65536-(256000000/(fmt->numChannels*fmt->sampleRate));
    //printk("const time:%d\n", time);

    /* 准备写入回放时间常量 */
    //WriteDsp(0x40);

    /* SB-16只使用其高8位 */
    /*WriteDsp((time >> 8) & 0xff);
    printk("write time done!\n");
    */
    /* 设置声卡数据长度 */
    //WriteDsp(0x48);
    /* 设置一半长度（前半部分播放结束后，发生中断通知，同时播放下半部分），并-1 */
    //uint32_t length = (data->subchunk2Size >> 1) - 1;
    /* 写入低8位 */
    //WriteDsp(length & 0xff);
    /* 写入高8位 */
    //WriteDsp((length >> 8) & 0xff);
    //printk("set data length done!\n");

    /* 打开扬声器 */
    //WriteDsp(0xd1);
    
    /* 启动音频回放 */
    //WriteDsp(0x1c);
    //printk("open sound and play!\n");

//ToLow:
    //return 0;
}

PRIVATE void SoundTest()
{
    if (!ResetDsp())
        return -1;

    /* 发出读取版本命令 */
    WriteDsp(DSP_CMD_VERSION);
    uint8_t versionMajor = ReadDsp();
    uint8_t versionMinor = ReadDsp();
    printk("version major %d minor %d\n", versionMajor, versionMinor);

    /* 设置IRQ5中断 */
    Out8(SB16_MIXER_REG, 0x80);
    Out8(SB16_MIXER_DATA_REG, 0X02);
    /* 设置DMA1通道 */
    Out8(SB16_MIXER_REG, 0x81);
    Out8(SB16_MIXER_DATA_REG, 0X02);


    /* 读取缓冲区 */
    Wav_t wavHeader;
    uint8_t *buffer = WavLoadData("root:/wav", &wavHeader);
    if (buffer == NULL) {
        return;
    }
    
    /* 分配地址 */
    void *viraddr = lmalloc(1);
    if (viraddr == NULL) {
        return;
    }
    dma_addr_t dmabuf = Vir2Phy(viraddr);

    /* 单次传输是一半，16位是以word字数计算 */
    uint32_t datalen = 32*1024;
    uint32_t blocklen = datalen / 2;

    /* 32KB数据 */
    memcpy(viraddr, buffer, datalen);

    /* DMA编程 */
    /*
    使用主片上的通道1。
    设定DMA主片屏蔽寄存器，以免受到DMA请求信号的打扰
    */
    Out8(DMA1_MASK_REG, DMA1_MASK_ON | DMA1_MODE_CHANNEL_1);
    
    /* 初始化写入顺序 */
    Out8(DMA1_CLEAR_FF_REG, 0);
    
    /* 设置基准地址寄存器 */
    
    Out8(DMA_ADDRESS_1, lo(dmabuf));        // 0~7位
    Out8(DMA_ADDRESS_1, hi(dmabuf)); // 8~15位
    
    /* 最高4位写入到页面寄存器 */
    Out8(DMA_PAGE_1, dmabuf >> 16);   // 16~19位
    
    /* 设置基准字数寄存器 */
    Out8(DMA_COUNT_1, lo(datalen - 1));        // 0~7位
    Out8(DMA_COUNT_1, hi(datalen - 1)); // 8~15位
    
    /* 设置传输模式 */
    Out8(DMA1_MODE_REG, 0X59);
    
    /* 取消对通道的屏蔽 */
    Out8(DMA1_MASK_REG, DMA1_MASK_OFF | DMA1_MODE_CHANNEL_1);
    
    /* 设置DSP采样率 */
    /* 计算时间常量，是16位值 */
    uint16_t time = 65536-(256000000/(1*8000));
    printk("const time:%d\n", time);

    /* 准备写入回放时间常量 */
    WriteDsp(0x40);
    
    /* SB-16只使用其高8位 */
    WriteDsp((time >> 8) & 0xff);
    
    /* 设置传输模式和传输长度 */
    WriteDsp(0x48);

    WriteDsp(lo(blocklen - 1));
    WriteDsp(hi(blocklen - 1));

    WriteDsp(0xd1);
    /* 启动播放 */
    WriteDsp(0x1c);
    RegisterIRQ(5, SoundBlasterHandler, 0, "IRQ5", DEVNAME, 0);
    done = FALSE;
}

PUBLIC int InitSoundBlaster16Driver()
{
    /*
    if (SoundBlasterInitOne()) {
        printk("Sound Blaster init failed!\n");
        return -1;
    }

    if (SB16AllocBuffer()) {
        printk("Sound Blaster alloc buffer failed!\n");
        return -1;
    }

    LoadBuffer();
    
    SoundBlasterStartIO(SB16_DATA_BLOCK_LENGTH * 2);
    */
    //SoundTest();

    return 0;
}
#endif /* CONFIG_SB16 */
