/*
 * file:		drivers/network/amd79c973.c
 * auther:		Jason Hu
 * time:		2020/1/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/* 今天是2020年的第一天，开始研究网卡驱动，希望2020可以圆梦！ */

/*
主要流程:
1.从PCI获取网卡信息
2.初始化网卡配置空间以及中断
3.读取网卡信息，并初始化
4.重启网卡
5.网卡发送
6.网卡接收
*/

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>

#include <share/string.h>


#include <book/interrupt.h>

#include <drivers/amd79c973.h>

#include <net/ethernet.h>
#include <net/arp.h>
#include <net/nllt.h>

#define DEVNAME "amd79c973"


/*PCI am79c973 config space register*/
#define AM79C973_VENDOR_ID   0x1022
#define AM79C973_DEVICE_ID   0x2000

typedef struct InitializationBlock
{
    uint16 mode;
    unsigned int reserved1 : 4;
    unsigned int numSendBuffers : 4;
    unsigned int reserved2 : 4;
    unsigned int numRecvBuffers : 4;
    uint8   physicalAddress[6];
    uint16 reserved3;
    uint32  logicalAddress[2];

    uint32  recvBufferDescrAddress;
    uint32  sendBufferDescrAddress;
} PACKED InitializationBlock_t;

typedef struct BufferDescriptor
{
    uint32  address;
    uint32  flags;
    uint32  flags2;
    uint32  avail;
} PACKED BufferDescriptor_t;

struct Amd79c973Private
{
	struct PciDevice *pciDevice;
	uint32 ioAddress;
    uint32 irq;
    uint8  macAddress[ETH_ADDR_LEN];

    uint16_t MACAddress0Port;
    uint16_t MACAddress2Port;
    uint16_t MACAddress4Port;
    uint16_t registerDataPort;
    uint16_t registerAddressPort;
    uint16_t resetPort;
    uint16_t busControlRegisterDataPort;

    struct InitializationBlock initBlock;

    struct BufferDescriptor *sendBufferDescr;

    uint8_t *sendBuffersStart;
    uint8_t *sendBuffers[8];
    uint8_t currentSendBuffer;

    struct BufferDescriptor *recvBufferDescr;
    uint8_t *recvBuffersStart;
    uint8_t *recvBuffers[8];
    uint8_t currentRecvBuffer;

} amd79c973Private;

PRIVATE int Amd79c973GetInfoFromPCI(struct Amd79c973Private *private)
{

    /* get pci device */
    struct PciDevice* device = GetPciDevice(AM79C973_VENDOR_ID, AM79C973_DEVICE_ID);
    if (device == NULL) {
        printk("AMD79C973 init failed: not find pci device.\n");
        return -1;
    }
	private->pciDevice = device;
	
    printk("find AMD79C973 device, vendor id: 0x%x, device id: 0x%x\n",\
            device->vendorID, device->deviceID);

    /* enable bus mastering */
	EnablePciBusMastering(device);

    /* get io address */
    private->ioAddress = GetPciDeviceIoAddr(device);
    if (private->ioAddress == 0) {
        printk("AMD79C973 init failed: INVALID pci device io address.\n");
        return -1;
    }
    printk("AMD79C973 io address: 0x%x\n", private->ioAddress);

    /* get irq */
    private->irq = GetPciDeviceIrqLine(device);
    if (private->irq == 0xff) {
        printk("AMD79C973 init failed: INVALID irq.\n");
        return -1;
    }
	
    printk("AMD79C973 irq: %d\n", private->irq);

    return 0;
}

/**
 * Amd79c973Handler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void Amd79c973Handler(unsigned int irq, unsigned int data)
{
	printk("Amd79c973Handler occur!\n");
    
    struct Amd79c973Private *private = &amd79c973Private;

    Out16(private->registerAddressPort, 0);
    uint32_t temp = In16(private->registerDataPort);
    
    if((temp & 0x8000) == 0x8000) printk("AMD am79c973 ERROR\n");
    if((temp & 0x2000) == 0x2000) printk("AMD am79c973 COLLISION ERROR\n");
    if((temp & 0x1000) == 0x1000) printk("AMD am79c973 MISSED FRAME\n");
    if((temp & 0x0800) == 0x0800) printk("AMD am79c973 MEMORY ERROR\n");
    if((temp & 0x0400) == 0x0400) printk(" RECEIVED"); //Receive();
    if((temp & 0x0200) == 0x0200) printk(" SENT");

    // acknoledge
    Out16(private->registerAddressPort, 0);
    Out16(private->registerDataPort, temp);
    
    if((temp & 0x0100) == 0x0100) printk("AMD am79c973 INIT DONE\n");
    
}


PRIVATE int Amd79c973Config(struct Amd79c973Private *private)
{
    int i;
    
    private->MACAddress0Port = private->ioAddress;
    private->MACAddress2Port = private->ioAddress + 0x02;
    private->MACAddress4Port = private->ioAddress + 0x04;
    private->registerDataPort = private->ioAddress + 0x10;
    private->registerAddressPort = private->ioAddress + 0x12;
    private->resetPort = private->ioAddress + 0x14;
    private->busControlRegisterDataPort = private->ioAddress + 0x16;
    
    private->currentSendBuffer = 0;
    private->currentRecvBuffer = 0;

    /* 从配置空间中读取MAC地址 */
    private->macAddress[0] = (uint8_t )(In16(private->MACAddress0Port) % 256);
    private->macAddress[1] = (uint8_t )(In16(private->MACAddress0Port) / 256);
    private->macAddress[2] = (uint8_t )(In16(private->MACAddress2Port) % 256);
    private->macAddress[3] = (uint8_t )(In16(private->MACAddress2Port) / 256);
    private->macAddress[4] = (uint8_t )(In16(private->MACAddress4Port) % 256);
    private->macAddress[5] = (uint8_t )(In16(private->MACAddress4Port) / 256);
    
    printk("mac addr: %x:%x:%x:%x:%x:%x\n", private->macAddress[0], private->macAddress[1],
            private->macAddress[2], private->macAddress[3], 
            private->macAddress[4], private->macAddress[5]);

    //EthernetSetAddress(private->macAddress);

    // 32 bit mode
    Out16(private->registerAddressPort, 20);
    Out16(private->busControlRegisterDataPort, 0x102);

    // STOP reset
    Out16(private->registerAddressPort, 0);
    Out16(private->registerDataPort, 0x04);
    
    // initBlock
    struct InitializationBlock *initBlock = &private->initBlock;
    initBlock->mode = 0x0000; // promiscuous mode = false
    initBlock->reserved1 = 0;
    initBlock->numSendBuffers = 3;
    initBlock->reserved2 = 0;
    initBlock->numRecvBuffers = 3;

    for (i = 0; i < 6; i++) {
        initBlock->physicalAddress[i] = private->macAddress[i];
    }
    
    initBlock->reserved3 = 0;
    initBlock->logicalAddress[0] = 0;
    initBlock->logicalAddress[1] = 0;
    
    /* 初始化缓冲描述符 */
    unsigned char *sendBuffer = kmalloc(sizeof(BufferDescriptor_t) * 8, GFP_KERNEL);
    if (sendBuffer == NULL) {
        printk("kmalloc for send buffer descriptor!\n");
        return -1;
    }

    /* 16字节对齐处理 */
    private->sendBufferDescr = (BufferDescriptor_t *)((((uint32_t)sendBuffer) + 15) & ~((uint32_t)0xF));

    /* 写入物理地址 */
    initBlock->sendBufferDescrAddress = (uint32_t )Vir2Phy(private->sendBufferDescr);

    printk("send buffer descriptor: %x -> %x -> %x\n", 
            sendBuffer, private->sendBufferDescr, initBlock->sendBufferDescrAddress);

    unsigned char *receiveBuffer = kmalloc(sizeof(BufferDescriptor_t) * 8, GFP_KERNEL);
    if (receiveBuffer == NULL) {
        printk("kmalloc for receive buffer descriptor!\n");
        return -1;
    }

    /* 16字节对齐处理 */
    private->recvBufferDescr = (BufferDescriptor_t *)((((uint32_t)receiveBuffer) + 15) & ~((uint32_t)0xF));

    /* 写入物理地址 */
    initBlock->recvBufferDescrAddress = (uint32_t )Vir2Phy(private->recvBufferDescr);

    printk("receive buffer descriptor: %x -> %x -> %x\n", 
            receiveBuffer, private->recvBufferDescr, initBlock->recvBufferDescrAddress);

    private->sendBuffersStart = kmalloc(PAGE_SIZE * 8, GFP_KERNEL);
    if (private->sendBuffersStart == NULL) {
        printk("kmalloc for send sendBuffers!\n");
        return -1;
    }
    
    private->recvBuffersStart = kmalloc(PAGE_SIZE * 8, GFP_KERNEL);
    if (private->recvBuffersStart == NULL) {
        printk("kmalloc for send sendBuffers!\n");
        return -1;
    }

    uint32_t virAddr, phyAddr;
    /* 缓冲区 */
    for(i = 0; i < 8; i++)
    {
        virAddr = (((uint32_t)(private->sendBuffersStart + i * PAGE_SIZE)) + 15 ) & ~(uint32_t)0xF;
        private->sendBuffers[i] = (uint8_t *)virAddr;

        private->sendBufferDescr[i].address = Vir2Phy((void *)virAddr);
        private->sendBufferDescr[i].flags = 0x7FF
                                 | 0xF000;
        private->sendBufferDescr[i].flags2 = 0;
        private->sendBufferDescr[i].avail = 0;
        
        virAddr = (((uint32_t)(private->recvBuffersStart + i * PAGE_SIZE)) + 15 ) & ~(uint32_t)0xF;
        private->recvBuffers[i] = (uint8_t *)virAddr;
        private->recvBufferDescr[i].address = Vir2Phy((void *)virAddr);
        private->recvBufferDescr[i].flags = 0xF7FF
                                 | 0x80000000;
        private->recvBufferDescr[i].flags2 = 0;
        private->recvBufferDescr[i].avail = 0;
    }

    /* 记录初始化块地址 */
    
    virAddr = (uint32_t)(initBlock);
    phyAddr = Vir2Phy((void *)virAddr);

    printk("init block: %x -> %x\n", virAddr, phyAddr);

    virAddr = (uint32_t)(initBlock) & 0xFFFF;
    phyAddr = Vir2Phy((void *)virAddr);

    printk("init block(0~15): %x -> %x\n", virAddr, phyAddr);
    Out16(private->registerAddressPort, 1);
    Out16(private->registerDataPort, phyAddr);
    
    virAddr = ((uint32_t)(initBlock) >> 16) & 0xFFFF;
    phyAddr = Vir2Phy((void *)virAddr);

    printk("init block(16~31): %x -> %x\n", virAddr, virAddr);
    Out16(private->registerAddressPort, 2);
    Out16(private->registerDataPort, phyAddr);

    return 0;
}

PRIVATE void Amd79c973Active(struct Amd79c973Private *private)
{
    Out16(private->registerAddressPort, 0);
    Out16(private->registerDataPort, 0x41);

    Out16(private->registerAddressPort, 4);
    uint32_t temp = In16(private->registerDataPort);
    Out16(private->registerAddressPort, 4);
    Out16(private->registerDataPort, temp | 0xC00);
    
    Out16(private->registerAddressPort, 0);
    Out16(private->registerDataPort, 0x42);

}

int Amd79c973Reset(struct Amd79c973Private *private)
{

    In16(private->resetPort);
    Out16(private->resetPort, 0);
    
    return 0;
}

PUBLIC void Amd79c973Send(uint8_t* buffer, int size)
{
    //printk("In Amd79c973Send\n");
    struct Amd79c973Private *private = &amd79c973Private;

    int sendDescriptor = private->currentSendBuffer;
    private->currentSendBuffer = (private->currentSendBuffer + 1) % 8;
    
    if(size > 1518)
        size = 1518;
    
    uint8_t *src = buffer;
    uint8_t *dst = (uint8_t*)(private->sendBuffers[sendDescriptor]);

    //printk("copy buf\n");
 
    memcpy(dst, src, size);

    printk("\nSEND: ");
    int i;
    for(i = 0; i < (size>64?64:size); i++)
    {
        printk("%x ", buffer[i]);
    }
    src = buffer;
    EthernetHeader_t *ethHeader = (EthernetHeader_t *)src;

    //DumpEthernetHeader(ethHeader);

    src += SIZEOF_ETHERNET_HEADER;

    ArpHeader_t *arpHeader = (ArpHeader_t *)src;
    //DumpArpHeader((ArpHeader_t *)arpHeader);
    
    for(i = 0; i < (size>64?64:size); i++)
    {
        //printk("%x ", buffer[i]);
    }

    private->sendBufferDescr[sendDescriptor].avail = 0;
    private->sendBufferDescr[sendDescriptor].flags2 = 0;
    private->sendBufferDescr[sendDescriptor].flags = 0x8300F000
                                          | ((uint16_t)((-size) & 0xFFF));
    
    Out16(private->registerAddressPort, 0);
    Out16(private->registerDataPort, 0x48);
}

PUBLIC void Amd79c973SetIPAddress(uint32_t ip)
{
    struct Amd79c973Private *private = &amd79c973Private;

    private->initBlock.logicalAddress[0] = ip;
}

PUBLIC uint32_t Amd79c973GetIPAddress()
{
    struct Amd79c973Private *private = &amd79c973Private;

    return private->initBlock.logicalAddress[0];
}

PUBLIC unsigned char *Amd79c973GetMACAddress()
{
    struct Amd79c973Private *private = &amd79c973Private;

    return private->macAddress;
}


PUBLIC int InitAmd79c973Driver()
{
    PART_START("AMD79C973 Driver");

    struct Amd79c973Private *private = &amd79c973Private;

    if (Amd79c973GetInfoFromPCI(private)) {
        return -1;
    }

    if (Amd79c973Config(private)) {
        return -1;
    }

    /* 配置中断 */

    /* 注册并打开中断 */
	RegisterIRQ(private->irq, Amd79c973Handler, IRQF_DISABLED, "IRQ-Network", DEVNAME, 0);
    EnableIRQ(private->irq);
    
    // 激活设备
    Amd79c973Active(private);

    PART_END();
    return 0;
}
