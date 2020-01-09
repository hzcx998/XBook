/*
 * file:		drivers/network/rtl8139.c
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
#include <book/interrupt.h>

#include <share/string.h>

#include <net/ethernet.h>
#include <net/nllt.h>

#include <drivers/rtl8139.h>

#define DEVNAME "rtl8139"


#define RTL8139_MAC_ADDR_LEN    6
#define RTL8139_NUM_TX_BUFFER   4
#define RTL8139_RX_BUF_SIZE  	32*1024	


/*PCI rtl8139 config space register*/
#define RTL8139_VENDOR_ID   0x10ec
#define RTL8139_DEVICE_ID   0x8139

/*rtl8139 register*/
#define RTL8139_MAC         0x00
#define RTL8139_TX_STATUS0  0x10
#define RTL8139_TX_ADDR0    0x20
#define RTL8139_RX_BUF_ADDR 0x30
#define RTL8139_COMMAND     0x37
#define RTL8139_CAPR        0x38
#define RTL8139_CBR         0x3A

#define RTL8139_INTR_MASK   0x3C
#define RTL8139_INTR_STATUS 0x3E
#define RTL8139_TCR         0x40
#define RTL8139_RCR         0x44
#define RTL8139_9346CR		0x50

#define RTL8139_CONFIG_0    0x51
#define RTL8139_CONFIG_1    0x52

/*rtl8139 intrrupt status register*/
#define RTL8139_RX_OK   	  	0x0001
#define RTL8139_INTR_STATUS_RER  		0x0002
#define RTL8139_TX_OK     	0x0004
#define RTL8139_INTR_STATUS_TER			0x0008
#define RTL8139_INTR_STATUS_RXOVW		0x0010
#define RTL8139_INTR_STATUS_PUN 		0x0020 
#define RTL8139_INTR_STATUS_FOVW 		0x0040 
/*reserved*/
#define RTL8139_INTR_STATUS_LENCHG		0x2000 
#define RTL8139_INTR_STATUS_TIMEOUT 	0x4000
#define RTL8139_INTR_STATUS_SERR   		0x8000 

/*rtl8139 command register*/
#define RTL8139_CMD_BUFE   	0x01	/*[R] Buffer Empty*/
#define RTL8139_CMD_TE  	0x04	/*[R/W] Transmitter Enable*/
#define RTL8139_CMD_RE  	0x08	/*[R/W] Receiver Enable*/
#define RTL8139_CMD_RST  	0x10	/*[R/W] Reset*/

/*rtl8139 receive config register*/
#define RTL8139_RCR_AAP   	0x01	/*[R/W] Accept All Packets*/
#define RTL8139_RCR_APM   	0x02	/*[R/W] Accept Physical Match packets*/
#define RTL8139_RCR_AM   	0x04	/*[R/W] Accept Multicast packets*/
#define RTL8139_RCR_AB   	0x08	/*[R/W] Accept Broadcast packets*/
#define RTL8139_RCR_AR   	0x10	/*[R/W] Accept Broadcast packets*/
#define RTL8139_RCR_AER   	0x20	/*[R/W] Accept Broadcast packets*/


#define RTL8139_RCR_RBLEN_32K   	2	/*[R/W] 2 means 32k + 16 byte*/
#define RTL8139_RCR_RBLEN_SHIFT   	11
	
#define RTL8139_RCR_MXDMA_1024K   	0x06		// 110b
#define RTL8139_RCR_MXDMA_SHIFT 	8

#define RTL8139_RCR_RXFTH_NOMAL   	0x07		// 111b
#define RTL8139_RCR_RXFTH_SHIFT 	13

/*rtl8139 transmit config register*/
#define RTL8139_TCR_MXDMA_1024K  	0x06		// 110b
#define RTL8139_TCR_MXDMA_SHIFT  	8

#define RTL8139_TCR_RXFTH_NORMAL  	0x03	// 11b
#define RTL8139_TCR_RXFTH_SHIFT 	24

/*RTL8139 TSD register*/
#define RTL8139_TSD_OWN   0x2000		/*[R/W] OWN*/
#define RTL8139_TSD_TUN   0x4000		/*[R] Transmit FIFO Underrun*/
#define RTL8139_TSD_TOK   0x8000		/*[R] Transmit OK*/
#define RTL8139_TSD_NCC   0x1000000		/*[R/W] NCC*/
#define RTL8139_TSD_CDH   0x10000000	/*[R] CD Heart Beat*/
#define RTL8139_TSD_OWC   0x20000000	/*[R] Out of Window Collision*/
#define RTL8139_TSD_TABT  0x40000000	/*[R] Transmit Abort*/
#define RTL8139_TSD_CRS   0x80000000	/*[R] Carrier Sense Lost*/

/*RTL8139 9346CMD register*/
#define RTL8139_9346CMD_EE  0XC0	

/*RTL8139 Tranmit configuration register*/
#define RTL8139_TCR_MXDMA_UNLIMITED  	0X07	
#define RTL8139_TCR_IFG_NORMAL  		0X07	

#define RTL8139_ETH_ZLEN  		60	

struct Rtl8139Private
{
	unsigned int ioAddress;
    unsigned int irq;
    unsigned char macAddress[6];
	
	struct PciDevice *pciDevice;
	
	unsigned char *rxBuffer;
    unsigned char  currentRX;   /* CAPR, Current Address of Packet Read */
    unsigned int  rxBufferLen;

    unsigned char*  txBuffers[RTL8139_NUM_TX_BUFFER];
    unsigned char   currentTX;
} rtl8139Private;

PRIVATE int Rtl8139GetInfoFromPCI(struct Rtl8139Private *private)
{

    /* get pci device */
    struct PciDevice* device = GetPciDevice(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (device == NULL) {
        printk("RTL8139 init failed: not find pci device.\n");
        return -1;
    }
	private->pciDevice = device;
	
    printk("find rtl8139 device, vendor id: 0x%x, device id: 0x%x\n",\
            device->vendorID, device->deviceID);

    /* enable bus mastering */
	EnablePciBusMastering(device);

    /* get io address */
    private->ioAddress = GetPciDeviceIoAddr(device);
    if (private->ioAddress == 0) {
        printk("RTL8139 init failed: INVALID pci device io address.\n");
        return -1;
    }
    printk("rlt8139 io address: 0x%x\n", private->ioAddress);

    /* get irq */
    private->irq = GetPciDeviceIrqLine(device);
    if (private->irq == 0xff) {
        printk("RTL8139 init failed: INVALID irq.\n");
        return -1;
    }
	
    printk("rlt8139 irq: %d\n", private->irq);

    return 0;
}

PUBLIC int Rtl8139Transmit(char *buf, uint32 len)
{
    
    struct Rtl8139Private *private = &rtl8139Private;
    /* 复制数据到当前的传输项 */
    memcpy(private->txBuffers[private->currentTX], buf, len);
    
    uint32 status = In16(private->ioAddress + RTL8139_INTR_STATUS);
    /*printk("transmit TX_STATUS0: 0x%x, INTR_STATUS: %x\n", 
            In32(private->ioAddress + RTL8139_TX_STATUS0), status);
    */
    enum InterruptStatus oldStatus = InterruptDisable();
    /* 把当前数据项对应的缓冲区的物理地址写入寄存器 */
    Out32(private->ioAddress + RTL8139_TX_ADDR0 + private->currentTX * 4, Vir2Phy(private->txBuffers[private->currentTX]));
    
    /* 往传输状态寄存器写入传输的状态 */
    Out32(private->ioAddress + RTL8139_TX_STATUS0 + private->currentTX * 4, (256 << 16) | 0x0 | len);
    
    //printk("after transmit TX_STATUS0: 0x%x\n", In32(private->ioAddress + RTL8139_TX_STATUS0 + private->currentTX * 4));
    
    /* 指向下一个传输项 */
    private->currentTX = (private->currentTX + 1) % 4;

    InterruptSetStatus(oldStatus);

    return 0;
}

void Rtl8139Receive()
{
    struct Rtl8139Private *private = &rtl8139Private;
    
    printk("RXOK, ");
    uint8* rx = private->rxBuffer;
    uint32 cur = private->currentRX;

    /* 如果属于主机 */
    while ((In8(private->ioAddress + RTL8139_COMMAND) & 0x01) == 0) {
        uint32 offset = cur % private->rxBufferLen;
        uint8* buf = rx + offset;
        uint32 status = *(uint32 *) buf;
        uint32 size   = status >> 16;
        
        if(size == 0)
            break;
        
        printk("[%x, %x] | ", offset, size);

        int i;
        for (i = 0; i < 16; i++) {
            printk("%x ", buf[4 + i + offset]);
        }
        printk("\n");

        //NlltReceive(buf + 4, size - 4);

        memset(buf, 0, size);
        /* 更新成下一个项 */
        cur = (cur + size + 7) & ~3;

        /* 写入寄存器 */
        Out16(private->ioAddress + RTL8139_CAPR, cur);
        
        private->currentRX = cur;
    }

    //private->currentRX = cur;
}

PUBLIC unsigned char *Rtl8139GetMACAddress()
{
    struct Rtl8139Private *private = &rtl8139Private;

    return private->macAddress;
}

/**
 * KeyboardHandler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void Rtl8139Handler(unsigned int irq, unsigned int data)
{
	//printk("Rtl8139Handler occur!\n");
    
    struct Rtl8139Private *private = &rtl8139Private;

    uint32 status = In16(private->ioAddress + RTL8139_INTR_STATUS);
    /* 写回状态 */
    Out16(private->ioAddress + RTL8139_INTR_STATUS, status);

    if (status & RTL8139_RX_OK) {
        Rtl8139Receive();
    } else if (status & RTL8139_TX_OK) {
        uint32 tx_status = In32(private->ioAddress + RTL8139_TX_STATUS0);
        printk("TXOK, TX_STATUS0: 0x%x\n", tx_status);
    } else {
        //printk("0x%x\n", status);
    }
}

PUBLIC int InitRtl8139Driver()
{
    PART_START("RTL8139 Driver");

    struct Rtl8139Private *private = &rtl8139Private;

    if (Rtl8139GetInfoFromPCI(private)) {
        return -1;
    }

    /* 从配置空间中读取MAC地址 */
	int i;
    for (i = 0; i < RTL8139_MAC_ADDR_LEN; i++) {
        private->macAddress[i] = In8(private->ioAddress + RTL8139_MAC + i);
    }
    
    printk("mac addr: %x:%x:%x:%x:%x:%x\n", private->macAddress[0], private->macAddress[1],
            private->macAddress[2], private->macAddress[3], 
            private->macAddress[4], private->macAddress[5]);

    //EthernetSetAddress(private->macAddress);

    
    /* send 0x00 to CONFIG_1 register to set the LWAKE + LWPTN to active high, 
     * this should essentially power on the device */
    Out8(private->ioAddress + RTL8139_CONFIG_1, 0x00);

    /* software reset, to clear the RX and TX buffers and set everything back to defaults */
    Out8(private->ioAddress + RTL8139_COMMAND, RTL8139_CMD_RST);
    while ((In8(private->ioAddress + RTL8139_COMMAND) & 0x10) != 0) {
        CpuNop();
    }

    /* init receive buffer */
    private->rxBuffer = (unsigned char *) kmalloc(PAGE_SIZE * 8, GFP_KERNEL);
    if (private->rxBuffer == NULL) {
        printk("kmalloc for rtl8139 rx buffer failed!\n");
        return -1;
    }
    printk("rx buffer addr:%x -> %x\n", private->rxBuffer, Vir2Phy(private->rxBuffer));

    private->currentRX = 0;
    /* 4个页长度，每个接收缓冲区4096字节 */
    private->rxBufferLen = 8 * PAGE_SIZE;

    Out32(private->ioAddress + RTL8139_RX_BUF_ADDR, Vir2Phy(private->rxBuffer));
    
    Out32(private->ioAddress + RTL8139_CAPR, private->currentRX);
    
    //Out32(private->ioAddress + 0x3a, 0);
    
    /* init tx buffers */
    private->currentTX = 0;

    for (i = 0; i < 4; i++) {
        private->txBuffers[i] = (uint8 *) kmalloc(PAGE_SIZE * 2, GFP_KERNEL);
        if (private->txBuffers[i] == NULL) {
            kfree(private->rxBuffer);
            printk("kmalloc for rtl8139 tx buffer failed!\n");
            return -1;
        }
    }

    /* 
     * set IMR(Interrupt Mask Register)
     * To set the RTL8139 to accept only the Transmit OK (TOK) and Receive OK (ROK) interrupts, 
     * we would have the TOK and ROK bits of the IMR high and leave the rest low. 
     * That way when a TOK or ROK IRQ happens, it actually will go through and fire up an IRQ.
     */
    Out16(private->ioAddress + RTL8139_INTR_MASK, RTL8139_RX_OK | RTL8139_TX_OK);

    /* 
     * configuring receive buffer(RCR)
     * Before hoping to see a packet coming to you, you should tell the RTL8139 to accept packets 
     * based on various rules. The configuration register is RCR.
     *  AB - Accept Broadcast: Accept broadcast packets sent to mac ff:ff:ff:ff:ff:ff
     *  AM - Accept Multicast: Accept multicast packets.
     *  APM - Accept Physical Match: Accept packets send to NIC's MAC address.
     *  AAP - Accept All Packets. Accept all packets (run in promiscuous mode).
     *  Another bit, the WRAP bit, controls the handling of receive buffer wrap around.
     */
    Out32(private->ioAddress + RTL8139_RCR, (2 << 11) | 0xf); /* AB + AM + APM + AAP */

    /* enable receive and transmitter */
    Out8(private->ioAddress + RTL8139_COMMAND, 0x0c);

    
    /* 注册并打开中断 */
	RegisterIRQ(private->irq, Rtl8139Handler, IRQF_DISABLED, "IRQ-Network", DEVNAME, 0);
    EnableIRQ(private->irq);
    
    PART_END();
    return 0;
}
