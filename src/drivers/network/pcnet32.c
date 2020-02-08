/*
 * file:		drivers/network/pcnet32.c
 * auther:		Jason Hu
 * time:		2020/2/4
 * copyright:	(C) 2018-2019 by Book OS selfelopers. All rights reserved.
 */

/* 今天是2020年的第一天，开始研究网卡驱动，希望2020可以圆梦！ */

/*
主要流程:
1.从PCI获取网卡信息
2.初始化网卡配置空间以及中断
3.读取网卡信息，并初始化
4.重置网卡
5.网卡发送
6.网卡接收
*/

#include <book/config.h>

#ifdef CONFIG_DRV_PCNET32

#include <book/debug.h>
#include <book/arch.h>
#include <book/interrupt.h>
#include <book/byteorder.h>

#include <share/string.h>
#include <share/math.h>

#include <net/network.h>
#include <net/ethernet.h>
#include <net/nllt.h>

#define DRV_NAME	"pcnet32"
#define DRV_VERSION	"0.1"

#define PCNET32_DRIVER_NAME   self_NAME DRV_VERSION

/* PCI 配置空间寄存器 */
#define PCNET32_VENDOR_ID   0x1022
#define PCNET32_DEVICE_ID   0x2000
 
/* ----定义接收和传输缓冲区的大小---- */
/* 网络框架结构 */

/* feature */
#define NET_FEATURE_RXALL     (1 << 0)        // 接收所有包
#define NET_FEATURE_RXFCS     (1 << 1)        // 接收所有包


#define PCNET32_PORT_AUI      0x00
#define PCNET32_PORT_10BT     0x01
#define PCNET32_PORT_GPSI     0x02
#define PCNET32_PORT_MII      0x03

#define PCNET32_PORT_PORTSEL  0x03
#define PCNET32_PORT_ASEL     0x04
#define PCNET32_PORT_100      0x40
#define PCNET32_PORT_FD       0x80

#define PCNET32_DMA_MASK 0xffffffff

/*
 * table to translate option values from tulip
 * to internal options
 */
static const unsigned char options_mapping[] = {
    PCNET32_PORT_ASEL,          /*  0 Auto-select      */
    PCNET32_PORT_AUI,           /*  1 BNC/AUI          */
    PCNET32_PORT_AUI,           /*  2 AUI/BNC          */
    PCNET32_PORT_ASEL,          /*  3 not supported    */
    PCNET32_PORT_10BT | PCNET32_PORT_FD,    /*  4 10baseT-FD       */
    PCNET32_PORT_ASEL,          /*  5 not supported    */
    PCNET32_PORT_ASEL,          /*  6 not supported    */
    PCNET32_PORT_ASEL,          /*  7 not supported    */
    PCNET32_PORT_ASEL,          /*  8 not supported    */
    PCNET32_PORT_MII,           /*  9 MII 10baseT      */
    PCNET32_PORT_MII | PCNET32_PORT_FD, /* 10 MII 10baseT-FD   */
    PCNET32_PORT_MII,           /* 11 MII (autosel)    */
    PCNET32_PORT_10BT,          /* 12 10BaseT          */
    PCNET32_PORT_MII | PCNET32_PORT_100,    /* 13 MII 100BaseTx    */
                        /* 14 MII 100BaseTx-FD */
    PCNET32_PORT_MII | PCNET32_PORT_100 | PCNET32_PORT_FD,
    PCNET32_PORT_ASEL           /* 15 not supported    */
};

#define MAX_UNITS 8     /* More are supported, limit only on options */
static int options[MAX_UNITS];
static int full_duplex[MAX_UNITS];
static int homepna[MAX_UNITS];

/*
 *              Theory of Operation
 *
 * This driver uses the same software structure as the normal lance
 * driver. So look for a verbose description in lance.c. The differences
 * to the normal lance driver is the use of the 32bit mode of PCnet32
 * and PCnetPCI chips. Because these chips are 32bit chips, there is no
 * 16MB limitation and we don't need bounce buffers.
 */

/*
 * Set the number of Tx and Rx buffers, using Log_2(# buffers).
 * Reasonable default values are 4 Tx buffers, and 16 Rx buffers.
 * That translates to 2 (4 == 2^^2) and 4 (16 == 2^^4).
 */
#ifndef PCNET32_LOG_TX_BUFFERS
#define PCNET32_LOG_TX_BUFFERS      4
#define PCNET32_LOG_RX_BUFFERS      5
#define PCNET32_LOG_MAX_TX_BUFFERS  9   /* 2^9 == 512 */
#define PCNET32_LOG_MAX_RX_BUFFERS  9
#endif

#define TX_RING_SIZE        (1 << (PCNET32_LOG_TX_BUFFERS))
#define TX_MAX_RING_SIZE    (1 << (PCNET32_LOG_MAX_TX_BUFFERS))

#define RX_RING_SIZE        (1 << (PCNET32_LOG_RX_BUFFERS))
#define RX_MAX_RING_SIZE    (1 << (PCNET32_LOG_MAX_RX_BUFFERS))

/* actual buffer length after being aligned */
#define PKT_BUF_SIZE        1548
/* chip wants twos complement of the (aligned) buffer length */
#define NEG_BUF_SIZE        (-PKT_BUF_SIZE)

/* Offsets from base I/O address. */
#define PCNET32_WIO_RDP     0x10
#define PCNET32_WIO_RAP     0x12
#define PCNET32_WIO_RESET   0x14
#define PCNET32_WIO_BDP     0x16

#define PCNET32_DWIO_RDP    0x10
#define PCNET32_DWIO_RAP    0x14
#define PCNET32_DWIO_RESET  0x18
#define PCNET32_DWIO_BDP    0x1C

#define PCNET32_TOTAL_SIZE  0x20

#define CSR0        0
#define CSR0_INIT   0x1
#define CSR0_START  0x2
#define CSR0_STOP   0x4
#define CSR0_TXPOLL 0x8
#define CSR0_INTEN  0x40
#define CSR0_IDON   0x0100
#define CSR0_NORMAL (CSR0_START | CSR0_INTEN)
#define PCNET32_INIT_LOW    1
#define PCNET32_INIT_HIGH   2
#define CSR3        3
#define CSR4        4
#define CSR5        5
#define CSR5_SUSPEND    0x0001
#define CSR15       15
#define PCNET32_MC_FILTER   8   /* 广播过滤 */
#define CSR58       58

#define PCNET32_79C970A 0x2621

typedef unsigned int __le32;
typedef unsigned short __le16;



/* The PCNET32 Rx and Tx ring descriptors. */
struct pcnet32_rx_head {
    __le32  base;///存储该描述符对应的缓冲区的首地址
    __le16  buf_length; /* two`s complement of length */ ///二进制补码形式，缓冲区大小
    __le16  status;////每一位都有自己的含义，硬件手册有规定
    __le32  msg_length;
    __le32  reserved;
}PACKED;

struct pcnet32_tx_head {
    __le32  base;
    __le16  length;     /* two`s complement of length */
    __le16  status;
    __le32  misc; ///用于错误标示和计数，详见硬件手册
    __le32  reserved;
}PACKED;

//* The PCNET32 32-Bit initialization block, described in databook. */
/*
 *The Mode Register (CSR15) allows alteration of the chip’s operating 
 *parameters. The Mode field of the Initialization Block is copied directly 
 *into CSR15. Normal operation is the result of configuring the Mode field 
 *with all bits zero.
 */
struct pcnet32_init_block {
    __le16  mode;
    __le16  tlen_rlen;
    u8  phys_addr[6];
    __le16  reserved;
    __le32  filter[2];
    /* Receive and transmit ring base, along with extra bits. */
    __le32  rx_ring;
    __le32  tx_ring;
} PACKED;

/* PCnet32 a functions */
struct pcnet32_a {
    u16 (*read_csr) (unsigned long, int);
    void    (*write_csr) (unsigned long, int, u16);
    u16 (*read_bcr) (unsigned long, int);
    void    (*write_bcr) (unsigned long, int, u16);
    u16 (*read_rap) (unsigned long);
    void    (*write_rap) (unsigned long, u16);
    void    (*reset) (unsigned long);
};

struct NetdeviceStatus {
    /* 错误记录 */
    unsigned long txErrors;         /* 传输错误记录 */
    unsigned long txAbortedErrors;  /* 传输故障记录 */
    unsigned long txCarrierErrors;  /* 传输携带错误记录 */
    unsigned long txWindowErrors;   /* 传输窗口错误记录 */
    unsigned long txFifoErrors;     /* 传输fifo错误记录 */
    unsigned long txDropped;        /* 传输丢弃记录 */

    unsigned long rxErrors;         /* 接收错误 */    
    unsigned long rxLengthErrors;   /* 接收长度错误 */
    unsigned long rxMissedErrors;   /* 接收丢失错误 */
    unsigned long rxFifoErrors;     /* 接收Fifo错误 */
    unsigned long rxCrcErrors;      /* 接收CRC错误 */
    unsigned long rxFrameErrors;    /* 接收帧错误 */
    unsigned long rxDropped;        /* 接收丢弃记录 */
    unsigned long rxOverErrors;      /* 接收溢出记录 */
    
    /* 正确记录 */
    unsigned long txPackets;        /* 传输包数量 */
    unsigned long txBytes;          /* 传输字节数量 */
    
    unsigned long rxPackets;        /* 接收包数量 */
    unsigned long rxBytes;          /* 接收字节数量 */
    
    unsigned long collisions;       /* 碰撞次数 */
};

struct Pcnet32Private
{
	unsigned int ioAddress;
    int drvFlags;           /* 驱动标志 */
    unsigned int irq;
    unsigned char macAddress[6];
    unsigned char promAddress[6];
    
    char *name;

	flags_t flags;
	struct PciDevice *pciDevice;

    unsigned int selfFeatures;       /* 设备结构特征 */

	struct NetdeviceStatus stats;  /* 设备状态 */

    struct pcnet32_init_block *init_block;
    /* The Tx and Rx ring entries must be aligned on 16-byte boundaries in 32bit mode. */
    struct pcnet32_rx_head  *rx_ring;
    struct pcnet32_tx_head  *tx_ring;
    dma_addr_t      init_dma_addr;/* DMA address of beginning of the init block,
                   returned by pci_alloc_consistent */

    /* 每个传输和接收缓冲区的DMA地址 */
    dma_addr_t      tx_dma_addr;
    dma_addr_t      rx_dma_addr;
    /* 每一个传输和接收缓冲区的首地址 */
    uint8_t *tx_buffer_addr;
    uint8_t *rx_buffer_addr;

    struct pcnet32_a   a;
    Spinlock_t      lock;       /* Guard lock */
    unsigned int        cur_rx, cur_tx; /* The next free ring entry */
    unsigned int        rx_ring_size;   /* current rx ring size */
    unsigned int        tx_ring_size;   /* current tx ring size */
    unsigned int        rx_mod_mask;    /* rx ring modular mask */
    unsigned int        tx_mod_mask;    /* tx ring modular mask */
    unsigned short      rx_len_bits;
    unsigned short      tx_len_bits;
    dma_addr_t      rx_ring_dma_addr;
    dma_addr_t      tx_ring_dma_addr;
    unsigned int        dirty_rx,   /* ring entries to be freed. */
                dirty_tx;

    int options;

    unsigned short      chip_version;   /* which variant this is */
};

struct Pcnet32Private pcnet32Private;

PRIVATE int cards_found = 0;

/*
工作在16-bit IO mode下，要读取第index个CSR的值，首先往RAP中写入index,然后再从RDP
中读取数据(此时读到的数据就是第index个CSR低16bit的数据），
*/
static u16 pcnet32_wio_read_csr(unsigned long addr, int index)
{
    Out16(addr + PCNET32_WIO_RAP, index);
    return In16(addr + PCNET32_WIO_RDP);
}

/*工作在16-bit IO mode下，要往第index个CSR中写入数据，首先往RAP中写入index，然后
再往RDP中写入数据(数据会自动被写入CSR index)，写入数据超过16bit的话，高位会被忽略掉*/
static void pcnet32_wio_write_csr(unsigned long addr, int index, u16 val)
{
    Out16(addr + PCNET32_WIO_RAP, index);
    Out16(addr + PCNET32_WIO_RDP, val);
}

/*
与static u16 pcnet32_wio_read_csr(unsigned long addr, int index)访问方
式类似，唯一不同的是读取数据是从BDP中读取
*/
static u16 pcnet32_wio_read_bcr(unsigned long addr, int index)
{
    Out16(addr + PCNET32_WIO_RAP, index);
    return In16(addr + PCNET32_WIO_BDP);
}
/*
与static void pcnet32_wio_write_csr(unsigned long addr, int index, u16 
val)访问方式类似，唯一不同的是读取数据是从BDP中读取
*/
static void pcnet32_wio_write_bcr(unsigned long addr, int index, u16 val)
{
    Out16(addr + PCNET32_WIO_RAP, index);
    Out16(addr + PCNET32_WIO_BDP, val);
}


static u16 pcnet32_wio_read_rap(unsigned long addr)
{
    return In16(addr + PCNET32_WIO_RAP);
}

static void pcnet32_wio_write_rap(unsigned long addr, u16 val)
{
    Out16(addr + PCNET32_WIO_RAP, val);
}

///Reset causes the device to cease operation and clear its internal logic.
static void pcnet32_wio_reset(unsigned long addr)
{
    ///read a to the RESET address (i.e., offset 0x14 for 16-bit I/O, offset 0x18 for 32-bit I/O),
    In16(addr + PCNET32_WIO_RESET);  /// addr + 0x14
}

static int pcnet32_wio_check(unsigned long addr)
{
    Out16(addr + PCNET32_WIO_RAP, 88);
    return (In16(addr + PCNET32_WIO_RAP) == 88);
}

static struct pcnet32_a pcnet32_wio = {
    .read_csr = pcnet32_wio_read_csr,
    .write_csr = pcnet32_wio_write_csr,
    .read_bcr = pcnet32_wio_read_bcr,
    .write_bcr = pcnet32_wio_write_bcr,
    .read_rap = pcnet32_wio_read_rap,
    .write_rap = pcnet32_wio_write_rap,
    .reset = pcnet32_wio_reset
};

static u16 pcnet32_dwio_read_csr(unsigned long addr, int index)
{
    Out32(addr + PCNET32_DWIO_RAP, index);
    return (In32(addr + PCNET32_DWIO_RDP) & 0xffff);
}

static void pcnet32_dwio_write_csr(unsigned long addr, int index, u16 val)
{
    Out32(addr + PCNET32_DWIO_RAP, index);
    Out32(addr + PCNET32_DWIO_RDP, val);
}

static u16 pcnet32_dwio_read_bcr(unsigned long addr, int index)
{
    Out32(addr + PCNET32_DWIO_RAP, index);
    return (In32(addr + PCNET32_DWIO_BDP) & 0xffff);
}

static void pcnet32_dwio_write_bcr(unsigned long addr, int index, u16 val)
{
    Out32(addr + PCNET32_DWIO_RAP, index);
    Out32(addr + PCNET32_DWIO_BDP, val);
}

static u16 pcnet32_dwio_read_rap(unsigned long addr)
{
    return (In32(addr + PCNET32_DWIO_RAP) & 0xffff);
}

static void pcnet32_dwio_write_rap(unsigned long addr, u16 val)
{
    Out32(addr + PCNET32_DWIO_RAP, val);
}

static void pcnet32_dwio_reset(unsigned long addr)
{
    In32(addr + PCNET32_DWIO_RESET);
}

static int pcnet32_dwio_check(unsigned long addr)
{
    Out32(addr + PCNET32_DWIO_RAP, 88);
    return ((In32(addr + PCNET32_DWIO_RAP) & 0xffff) == 88);
}

static struct pcnet32_a pcnet32_dwio = {
    .read_csr = pcnet32_dwio_read_csr,
    .write_csr = pcnet32_dwio_write_csr,
    .read_bcr = pcnet32_dwio_read_bcr,
    .write_bcr = pcnet32_dwio_write_bcr,
    .read_rap = pcnet32_dwio_read_rap,
    .write_rap = pcnet32_dwio_write_rap,
    .reset = pcnet32_dwio_reset
};

PRIVATE int Pcnet32GetInfoFromPCI(struct Pcnet32Private *self)
{
    /* get pci device */
    struct PciDevice* device = GetPciDevice(PCNET32_VENDOR_ID, PCNET32_DEVICE_ID);
    if (device == NULL) {
        printk("Pcnet32 init failed: not find pci device.\n");
        return -1;
    }
	self->pciDevice = device;
	
    printk("find Pcnet32 device, vendor id: 0x%x, device id: 0x%x\n",\
            device->vendorID, device->deviceID);

    /* enable bus mastering */
	EnablePciBusMastering(device);

    /* get io address */
    self->ioAddress = GetPciDeviceIoAddr(device);
    if (self->ioAddress == 0) {
        printk("Pcnet32 init failed: INVALID pci device io address.\n");
        return -1;
    }
    printk("pcnet32 io address: 0x%x\n", self->ioAddress);

    /* get irq */
    self->irq = GetPciDeviceIrqLine(device);
    if (self->irq == 0xff) {
        printk("Pcnet32 init failed: INVALID irq.\n");
        return -1;
    }
	
    printk("pcnet32 irq: %d\n", self->irq);

    return 0;
}

PUBLIC int Pcnet32Transmit(char *buf, uint32 len)
{
    struct Pcnet32Private *self = &pcnet32Private;
    


    return 0;
}

PRIVATE int Pcnet32TxInterrupt(struct Pcnet32Private *self)
{
    return 0;
}

PUBLIC unsigned char *Pcnet32GetMACAddress()
{
    struct Pcnet32Private *self = &pcnet32Private;

    return self->macAddress;
}
/*
 * process one receive descriptor entry
 */
////处理收到的一个descriptor entry，然后交给网络层
static void pcnet32_rx_entry(struct Pcnet32Private *self,
                 struct pcnet32_rx_head *rxp,
                 int entry)
{
    int status = (short)LittleEndian16ToCpu(rxp->status) >> 8;   //rxp = &lp->rx_ring[entry]
    int rx_in_place = 0;
    short pkt_len;

    //D//他的意思应该是收到的数据包都应该视为独立的frame，不存在buffer chain，如果出现了，说明出错了
    //这里0x03表示STP位被置，即Start of Packet， ENP被置，即End of Packet
    if (status != 0x03) {   /* There was an error. */ //
        /*
         * There is a tricky error noted by John Murphy,
         * <murf@perftech.com> to Russ Nelson: Even with full-sized
         * buffers it's possible for a jabber packet to use two
         * buffers, with only the last correctly noting the error.
         */
        // 下面的错误种类和位的对应关系让人有点迷茫，暂时不管
        if (status & 0x01)  /* Only count a general error at the */
            self->stats.rxErrors++; /* end of a packet. */
        if (status & 0x20)  //DM//Start of Packet
            self->stats.rxFrameErrors++;
        if (status & 0x10)
            self->stats.rxOverErrors++;
        if (status & 0x08)
            self->stats.rxCrcErrors++;
        if (status & 0x04)
            self->stats.rxFifoErrors++;
        return;
    }
    /* 获取数据长度 */
    pkt_len = (LittleEndian32ToCpu(rxp->msg_length) & 0xfff) - 4;
    //DM// 帧长大约PKT_BUF_SIZE+4或者小于64字节的包都会被丢掉
    /* Discard oversize frames. */
    if (unlikely(pkt_len > PKT_BUF_SIZE)) {
        printk("%s: Impossible packet size %d!\n",
                   self->name, pkt_len);
        self->stats.rxErrors++;
        return;
    }
    if (pkt_len < 60) {
        printk("%s: Runt packet!\n", self->name);
        self->stats.rxErrors++;
        return;
    }

    self->stats.rxBytes += pkt_len;
    /*
     *(1)将skb->self指向接收设备
     *(2)将skb->mac_header指向data(此时data就是指向mac起始地址)
     *(3)调用skb_pull(skb, ETH_HLEN)将skb->data后移14字节指向ip首部
     *(4)通过比较目的mac地址判断包的类型，并将skb->pkt_type赋值PACKET_BROADCAST
     *或PACKET_MULTICAST或者PACKET_OTHERHOST ，因为PACKET_HOST为0，所以是默认值,
     *HOST表示包的目的地址是本机
     *(5)最后判断协议类型，并返回（大部分情况下直接返回eth首部的protocol字段的值），
     *这个返回值被存在skb->protocol字段中
     */
    //skb->protocol = eth_type_trans(skb, self);
    /* 复制数据 */

    //memcpy(buf, self->rx_ring[entry].);
    NlltReceive(self->rx_buffer_addr + entry * self->rx_ring_size , pkt_len);

    self->stats.rxPackets++;
    return;
}

PRIVATE int Pcnet32RxInterrupt(struct Pcnet32Private *self)
{
    int entry = self->cur_rx & self->rx_mod_mask;
    struct pcnet32_rx_head *rxp = &self->rx_ring[entry];
    int npackets = 0;  //DM//记录已经处理的packet的数量
    
    printk("base:%x buf len:%x msg len:%x status:%x\n", rxp->base, rxp->buf_length, rxp->msg_length, rxp->status);
    
    /*由于此时网卡已经将收到的包塞入缓冲区，所以网卡要将own位清0，而own位是status的最
     *高位 ，所以，此时status的值将>0。
     *如果OWN位为1（此时status<0），那么表示网卡还没有放数据，此时根据手册，就不能再
     * 继续读下一个entry了。
     */
    /* If we own the next entry, it's a new packet. Send it up. */
    while (npackets < 2 && (short)LittleEndian16ToCpu(rxp->status) >= 0) {
        ///处理当前的descriptor entry,并将数据递交给网络层
        printk("pcnet32_rx_entry\n");
        pcnet32_rx_entry(self, rxp, entry);
        npackets += 1;
        /*
         * The docs say that the buffer length isn't touched, but Andrew
         * Boyd of QNX reports that some revs of the 79C965 clear it.
         */
        /*
         * prob/这里不太明白为何长度是个常数  1522  ，可能是因为rx descriptor的 
         * skb_buff的长度一直都是分配最大值PKT_BUF_SKB 1544，不过1544是错误的， 
         * 因为最大不能超过1518 
         */
         rxp->buf_length = CpuToLittleEndian16(NEG_BUF_SIZE);

         //写内存屏障
        WriteMemoryBarrier();  
         /* Make sure owner changes after others are visible */
        rxp->status = CpuToLittleEndian16(0x8000); //DM//把rxp的控制权还给controller
        ///处理下一个descriptor
        entry = (++self->cur_rx) & self->rx_mod_mask;
        rxp = &self->rx_ring[entry];
    }

    return npackets;
}

/**
 * KeyboardHandler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void Pcnet32Handler(unsigned int irq, unsigned int data)
{
    struct Pcnet32Private *self = &pcnet32Private;
    printk("int occur!\n");
    unsigned int ioaddr;
    u16 csr0;

    int boguscnt = 2;
    ioaddr = self->ioAddress;

    SpinLock(&self->lock);

    csr0 = self->a.read_csr(ioaddr, CSR0);

    while ((csr0 & 0x8f00) && --boguscnt >= 0) {
        if (csr0 == 0xffff) {
            break;
        }
        /* Acknowledge all of the current interrupt sources ASAP. */ //ASAP =  as soon as possible
        /*
         *CSR0的 bit0~bit5 
         * 写0是无法写入的，也不会产生影响，所以下面的代码只会置INEA（interrupt enable)为0，即禁止中断防止网卡上的中断重入
         * self->a.write_csr(ioaddr, CSR0, csr0 & ~0x004f);
         */
        printk("has data %x\n", csr0);
        //self->a.write_csr(ioaddr, CSR0, csr0 & ~0x004f);
        /* Log misc errors. */
        //CSR0第14位是reserve位。搞不懂，这个地方是一个bug，应该是0x8000才对。因为手册上15位才是ERR位。
        
        if (csr0 & 0x8000) {
            self->stats.txErrors++; /* Tx babble. */
            printk("!ERR!\n");
        }
        if (csr0 & 0x4000) {
            printk("!BABL!\n");
        }
        if (csr0 & 0x2000) {
            printk("!CERR!\n");
        }
            ///bit12 MISS位  Missed Frame is set by the PCnet-PCI II controller when it looses an incoming receive frame
            ///because a receive descriptorwas not available
        if (csr0 & 0x1000) {//CSR0 第12位为1，表示丢失了一个incoming frame
            /*
             * This happens when our receive ring is full. This
             * shouldn't be a problem as we will see normal rx
             * interrupts for the frames in the receive ring.  But
             * there are some PCI chipsets (I can reproduce this
             * on SP3G with Intel saturn chipset) which have
             * sometimes problems and will fill up the receive
             * ring with error descriptors.  In this situation we
             * don't get a rx interrupt, but a missed frame
             * interrupt sooner or later.
             */
            self->stats.rxErrors++; /* Missed a Rx frame. */
            printk("!MISS!\n");
        }
        if (csr0 & 0x0800) {//CSR0第11位置1表示Memory Error。
            printk(
                    "%s: Bus master arbitration failure, status %x.\n",
                       self->name, csr0);
            /* unlike for the lance, there is no restart needed */
        }

        //DM/下面是没有napi时的运行选项
        Pcnet32RxInterrupt(self);

        if (Pcnet32TxInterrupt(self)) {
            /* reset the chip to clear the error condition, then restart */
            self->a.reset(ioaddr);
            self->a.write_csr(ioaddr, CSR4, 0x0915); /* auto tx pad */
            //pcnet32_restart(self, CSR0_START);
            //netif_wake_queue(self);
        }

        csr0 = self->a.read_csr(ioaddr, CSR0);
    }
/* Set interrupt enable. */
    ////CSR0只有 bit6写1，其他位写0都是无效的
    //self->a.write_csr(ioaddr, CSR0, CSR0_INTEN);//开中断

    printk("%s: exiting interrupt, csr0=%x.\n",
               self->name, self->a.read_csr(ioaddr, CSR0));

    SpinUnlock(&self->lock);

    return;
}

/**
 * Pcnet32AllocRing - 分配环形缓冲区
 * 
 */
PRIVATE int Pcnet32AllocRing(struct Pcnet32Private *self)
{
    /* 先分配传输ring */
    self->tx_ring = kmalloc(sizeof(struct pcnet32_tx_head) * self->tx_ring_size, GFP_DMA);
    if (self->tx_ring == NULL) {
        printk("kmalloc for tx ring failed!\n");
        return -1;
    } 
    self->tx_ring_dma_addr = Vir2Phy(self->tx_ring);

    self->rx_ring = kmalloc(sizeof(struct pcnet32_rx_head) * self->rx_ring_size, GFP_DMA);
    if (self->rx_ring == NULL) {
        printk("kmalloc for rx ring failed!\n");
        return -1;
    }

    self->rx_ring_dma_addr = Vir2Phy(self->rx_ring);

    printk("tx ring:%x dma:%x, rx ring:%x dma:%x\n", self->tx_ring, self->tx_ring_dma_addr, 
        self->rx_ring, self->rx_ring_dma_addr);

    /* 分配数据缓冲区地址 */
    self->tx_buffer_addr = kmalloc(PKT_BUF_SIZE * self->tx_ring_size, GFP_DMA);
    if (self->tx_buffer_addr == NULL) {
        return -1;
    }
    self->tx_dma_addr = Vir2Phy(self->tx_buffer_addr);
    
    self->rx_buffer_addr = kmalloc(PKT_BUF_SIZE * self->rx_ring_size, GFP_DMA);
    if (self->rx_buffer_addr == NULL) {
        return -1;
    }
    self->rx_dma_addr = Vir2Phy(self->rx_buffer_addr);
    printk("tx buffer:%x dma:%x, rx buffer:%x dma:%x\n", self->tx_buffer_addr, self->tx_dma_addr, 
        self->rx_buffer_addr, self->rx_dma_addr);

    return 0;
}

/* Initialize the Rx and Tx rings, along with various 'self' bits. */
PRIVATE int Pcnet32InitRing(struct Pcnet32Private *self)
{
    self->cur_rx = 0;
    self->cur_tx = 0;
    
    self->dirty_rx = 0;
    self->dirty_rx = 0;
    
    int i;
    for (i = 0; i < self->rx_ring_size; i++) {
        self->rx_ring[i].base = CpuToLittleEndian32(self->rx_dma_addr + i * PKT_BUF_SIZE);
        self->rx_ring[i].buf_length = CpuToLittleEndian16(NEG_BUF_SIZE);
        WriteMemoryBarrier();      /* Make sure owner changes after all others are visible */
        self->rx_ring[i].status = CpuToLittleEndian16(0x8000); //将own位置1，contorller为owner   
    }
    for (i = 0; i < self->tx_ring_size; i++) {
        self->tx_ring[i].status = CpuToLittleEndian16(0x0000); //将own位置1，contorller为owner   
        WriteMemoryBarrier();      /* Make sure owner changes after all others are visible */
    
        self->tx_ring[i].base = CpuToLittleEndian32(self->tx_dma_addr + i * PKT_BUF_SIZE);
        
    }
    ///prob\还不清楚为什么要在这里再次填充init block
    /*self->init_block->tlen_rlen =
        CpuToLittleEndian16(self->tx_len_bits | self->rx_len_bits);
    for (i = 0; i < 6; i++)
        self->init_block->phys_addr[i] = self->macAddress[i];
    self->init_block->rx_ring = CpuToLittleEndian32(self->rx_ring_dma_addr);
    self->init_block->tx_ring = CpuToLittleEndian32(self->tx_ring_dma_addr);
    WriteMemoryBarrier();*/          /* Make sure all changes are visible */
    return 0;
}

PRIVATE int Pcnet32Open()
{
    struct Pcnet32Private *self = &pcnet32Private;
    uint32_t ioaddr = self->ioAddress;
    u16 val;
    /* 注册并打开对应的中断 */
	RegisterIRQ(self->irq, Pcnet32Handler, IRQF_SHARED, "IRQ-Pcnet32", DRV_NAME, (unsigned int)self);
    DisableIRQ(self->irq);

    uint32_t eflags;
    eflags = LoadEflags();
    SpinLock(&self->lock);

    /* 检测是否有效 */
    if (!IsValidEtherAddr(self->macAddress)) {
        return -1;
    }

    /* set/reset autoselect bit */
    val = self->a.read_bcr(ioaddr, 2) & ~2;
    ///如果self->options是 PCNET32_PORT_ASEL，则BCR2，bit1写入1，设置auto select，在只有一个芯片的情况下，是auto-selecte
    if (self->options & PCNET32_PORT_ASEL)
        val |= 2;
    self->a.write_bcr(ioaddr, 2, val);
    printk("set ASEL done!\n");    

    /* set/reset GPSI bit in test register */
    ///\ bit4 GPSIEN:General Purpose Serial Interface Enable.，GPSI is used as an interface between Ethernet MAC and PHY blocks.
    val = self->a.read_csr(ioaddr, 124) & ~0x10;

    ///\ 单个网卡芯片，条件为否,所以GPSI为假
    if ((self->options & PCNET32_PORT_PORTSEL) == PCNET32_PORT_GPSI)
        val |= 0x10;
    self->a.write_csr(ioaddr, 124, val);

    if (self->options & PCNET32_PORT_ASEL) {
        self->a.write_bcr(ioaddr, 32,
                self->a.read_bcr(ioaddr,
                            32) | 0x0080);
        /* enable auto negotiate, setup, disable fd */
        val = self->a.read_bcr(ioaddr, 32) & ~0x98;
        val |= 0x20;
        self->a.write_bcr(ioaddr, 32, val);
    }

    ///\PCNET32_PORT_PORTSEL = 0x03, options=0x04,so mode = 0;
    self->init_block->mode =
        CpuToLittleEndian16((self->options & PCNET32_PORT_PORTSEL) << 7);

    ///init_ring初始化 self->rx_skbuff数组,并为每个sk_buff分配缓冲区，建立DMA映射
    if (Pcnet32InitRing(self)) {
        return -1;
    }
    /**DM
         * 
         * CSR4: Test and Features Control，屏蔽Jabber Error，Transmit Start，Receive Collision Counter Overflow产生的中断
         * 自动填充至60字节，加上FCS是64字节
         */
    self->a.write_csr(ioaddr, CSR4, 0x0915);  /* auto tx pad */
__le16  mode;
    __le16  tlen_rlen;
    u8  phys_addr[6];
    __le16  reserved;
    __le32  filter[2];
    /* Receive and transmit ring base, along with extra bits. */
    __le32  rx_ring;
    __le32  tx_ring;

    /* 查看初始化块的内容 */
    printk("init block:mode:%x, len:%x, phy:",
    self->init_block->mode, self->init_block->tlen_rlen);
    int i;
    for (i = 0; i < 6; i++) {
        printk("%x", self->init_block->phys_addr[i]);
    }
    printk("filter:%x,%x rx:%x tx:%x",
    self->init_block->filter[0], self->init_block->filter[1],
    self->init_block->rx_ring, self->init_block->tx_ring);

    ///INIT  启动init block从内存写入网卡
    self->a.write_csr(ioaddr, CSR0, CSR0_INIT);
    printk("init start ");    

    i = 0;
    ////循环等待，当init block写入网卡寄存器完成，contorller会自动将CSR0_IDON位置1，该位由0变为1时就说明init block传输完成了
    while (1)
        if (self->a.read_csr(ioaddr, CSR0) & CSR0_IDON)
            break;
    printk("init done!");    

    self->a.write_csr(ioaddr, 24, self->init_block->rx_ring & 0xffff);
    self->a.write_csr(ioaddr, 25, (self->init_block->rx_ring >> 16) & 0xffff);
    
    self->a.write_csr(ioaddr, 18, self->init_block->rx_ring & 0xffff);
    self->a.write_csr(ioaddr, 19, (self->init_block->rx_ring >> 16) & 0xffff);
    
    printk("CSR24:%x\n", self->a.read_csr(ioaddr, 24));
    printk("CSR25:%x\n", self->a.read_csr(ioaddr, 25));
    

    /*
     * We used to clear the InitDone bit, 0x0100, here but Mark Stockton
     * reports that doing so triggers a bug in the '974.
     */
    /// CSR0_NORMAL = (CSR0_START | CSR0_INTEN)，  下面讲 CSR0中的STRT位和EINA位置1
    ////STRT位，该位置1表示启动网卡控制器发送和接受帧，并开始执行缓冲区管理操作，为1表示不能接受发送数据
    ////IENA位，该位为1表示允许中断，该位为0表示关中断，IENA置1时还会将CSR0的STOP位清0
    self->a.write_csr(ioaddr, CSR0, CSR0_NORMAL);
    printk("start work!\n");    

    
    /* 检测寄存器的值 */

    printk("CSR1:%x\n", self->a.read_csr(ioaddr, 1));
    printk("CSR2:%x\n", self->a.read_csr(ioaddr, 2));
    printk("CSR3:%x\n", self->a.read_csr(ioaddr, 3));
    printk("CSR4:%x\n", self->a.read_csr(ioaddr, 4));
    printk("CSR6:%x\n", self->a.read_csr(ioaddr, 6));
    printk("CSR15:%x\n", self->a.read_csr(ioaddr, 15));

    printk("CSR16:%x\n", self->a.read_csr(ioaddr, 16));
    printk("CSR17:%x\n", self->a.read_csr(ioaddr, 17));
    printk("CSR24:%x\n", self->a.read_csr(ioaddr, 24));
    printk("CSR25:%x\n", self->a.read_csr(ioaddr, 25));
    
    printk("CSR18:%x\n", self->a.read_csr(ioaddr, 18));
    printk("CSR19:%x\n", self->a.read_csr(ioaddr, 19));
    printk("CSR20:%x\n", self->a.read_csr(ioaddr, 20));
    printk("CSR21:%x\n", self->a.read_csr(ioaddr, 21));
    printk("CSR22:%x\n", self->a.read_csr(ioaddr, 22));
    printk("CSR23:%x\n", self->a.read_csr(ioaddr, 23));
    
    SpinUnlock(&self->lock);
    StoreEflags(eflags);

    EnableIRQ(self->irq);
    
    return 0;
}

PRIVATE int Pcnet32Close()
{
    struct Pcnet32Private *self = &pcnet32Private;

    return 0;
}

PRIVATE int Pcnet32InitOne(struct Pcnet32Private *self)
{
    unsigned long ioaddr = self->ioAddress;
    struct pcnet32_a *a;    
    int fdx, mii, fset, dxsuflo;
    char *chipname;  ///芯片名称
    int media;

    /* 先执行32位重置（避免由于某种原因处于32位模式），再进行16位重置 */
    pcnet32_dwio_reset(ioaddr);
    pcnet32_wio_reset(ioaddr);

    /* 等待一微秒，重置完成,in指令花费1微妙 */
    In8(0x80);

    /* 切换到32位io工作模式 */    
    Out16(ioaddr + PCNET32_WIO_RDP, 0);
    
    /* 设置SWSTYLE=1，表示32位控制工作模式 */
    uint32_t csr58 = pcnet32_dwio_read_csr(ioaddr, CSR58);
    /* 11-31位都是保留位 */
    csr58 &= 0xffff;
    csr58 |= 0x02;  /* PCnet-PCI II */
    pcnet32_dwio_write_csr(ioaddr, CSR58, csr58);

    if (pcnet32_dwio_read_csr(ioaddr, 0) == 4
        && pcnet32_dwio_check(ioaddr)) {
        printk("default switch to 32bit mode.\n");
        a = &pcnet32_dwio;
    } else {
        printk("switch to 32bit mode fault.\n");
        return -1;
    }
    cards_found = 0;

    /* 读取mac地址 */
    uint32_t mac_low = In32(ioaddr + 0); 
    uint32_t mac_high = In32(ioaddr + 4); 
    
    uint8_t promaddr[6] = {0};


    promaddr[0] = mac_low & 0xff;
    promaddr[1] = (mac_low >> 8) & 0xff;
    promaddr[2] = (mac_low >> 16) & 0xff;
    promaddr[3] = (mac_low >> 24) & 0xff;
    
    promaddr[4] = mac_high & 0xff;
    promaddr[5] = (mac_high >> 8) & 0xff;
    printk("prom addr:");
    
    int i;
    for (i = 0; i < 6; i++) {
        printk("%x:", promaddr[i]);
    }

    for (i = 0; i < 3; i++) {
        unsigned int val;
        val = a->read_csr(ioaddr, i + 12) & 0x0ffff;
        self->macAddress[i * 2] = val & 0x0ff;
        self->macAddress[i * 2 + 1] = (val >> 8) & 0x0ff;
    }

    printk("\nmac addr:");
    for (i = 0; i < 6; i++) {
        printk("%x:", self->macAddress[i]);
    }

    /* 对2种模式进行比较，如果说在重新打开后，CSR的MAC可能改变过，需要恢复成原始MAC */
    if (memcmp(promaddr, self->macAddress, 6)
        || !IsValidEtherAddr(self->macAddress)) {
        printk("mac addr invalid!\n");
        if (IsValidEtherAddr(promaddr)) {
            printk(" warning: CSR address invalid,\n");
            printk("    using instead PROM address of");
            memcpy(self->macAddress, promaddr, 6);
        }
    }
    /// perm_addr 指的是 Permanent hardware address （即 MAC) netdevice.h中可以找到
    memcpy(self->promAddress, self->macAddress, 6);

    /* if the ethernet address is not valid, force to 00:00:00:00:00:00 */
    if (!IsValidEtherAddr(self->promAddress))
        memset(self->macAddress, 0, sizeof(self->macAddress));


    uint32_t chip_version = a->read_csr(ioaddr, 88) | (a->read_csr(ioaddr, 89) << 16);
    printk("\npcnet32 version is %x\n", chip_version);
    if ((chip_version & 0xfff) != 0x003) {
        printk("Unsupported chip version.\n");
        return -1;
    }
    fdx = mii = fset = dxsuflo = 0;
    /// get the 2 bytes befor 0x003;
    chip_version = (chip_version >> 12) & 0xffff;
    /// VMware提供的网卡是79c97cA,chip_version = 2621H
    switch (chip_version) {
    case 0x2420:
    //// because the chip supplied by vmware is 70c970, so I will concentrate on  this case
        chipname = "PCnet/PCI 79C970";  /* PCI */
        break;
    case 0x2430:
        //if (shared)
        //    chipname = "PCnet/PCI 79C970";  /* 970 gives the wrong chip id back */
        //else
        chipname = "PCnet/32 79C965";   /* 486/VL bus */
        break;
    case 0x2621:
        chipname = "PCnet/PCI II 79C970A";  /* PCI */
        fdx = 1;
        break;
    case 0x2623:
        chipname = "PCnet/FAST 79C971"; /* PCI */
        fdx = 1;
        mii = 1;
        fset = 1;
        break;
    case 0x2624:
        chipname = "PCnet/FAST+ 79C972";    /* PCI */
        fdx = 1;
        mii = 1;
        fset = 1;
        break;
    case 0x2625:
        chipname = "PCnet/FAST III 79C973"; /* PCI */
        fdx = 1;
        mii = 1;
        break;
    case 0x2626:

        /// well,the 79c978 chip looks like it's a little special, I don't have the hardware manual of it ,and  just ignore it
        chipname = "PCnet/Home 79C978"; /* PCI */
        fdx = 1;
        /*
         * This is based on specs published at www.amd.com.  This section
         * assumes that a card with a 79C978 wants to go into standard
         * ethernet mode.  The 79C978 can also go into 1Mb HomePNA mode,
         * and the module option homepna=1 can select this instead.
         */
        media = a->read_bcr(ioaddr, 49);
        media &= ~3;   /* default to 10Mb ethernet */
        if (cards_found < MAX_UNITS && homepna[cards_found])
            media |= 1; /* switch to home wiring mode */
        printk("media set to %sMbit mode.\n",
                   (media & 1) ? "1" : "10");
        a->write_bcr(ioaddr, 49, media);
        break;
    case 0x2627:
        chipname = "PCnet/FAST III 79C975"; /* PCI */
        fdx = 1;
        mii = 1;
        break;
    case 0x2628:
        chipname = "PCnet/PRO 79C976";
        fdx = 1;
        mii = 1;
        break;
    default:
        printk("PCnet version %x, no PCnet32 chip.\n",
            chip_version);
    }
    printk("name:%s version:%x fdx:%d mii:%d fset:%d media:%x\n",
    chipname, chip_version,fdx,mii, fset, media);

    //// case  79c970 ，fset == 0, false， so was ingored by me
    if (fset) {
        a->write_bcr(ioaddr, 18, (a->read_bcr(ioaddr, 18) | 0x0860));//打开BCR18的第11位，即NOUFLO位，以及第6位（BREADE）和第5位（BWRITE）
        a->write_csr(ioaddr, 80,
                 (a->read_csr(ioaddr, 80) & 0x0C00) | 0x0c00);
        dxsuflo = 1;
    }

    /* Version 0x2623 and 0x2624 */
    ///  79C70A was 0x2621  so ignored，下面没做什么实际工作，就是读取某些寄存器然后输出内容
    if (((chip_version + 1) & 0xfffe) == 0x2624) {
        i = a->read_csr(ioaddr, 80) & 0x0C00;   /* Check tx_start_pt */
        printk("\n    tx_start_pt(0x%04x):", i);
        switch (i >> 10) {
        case 0:
            printk("  20 bytes,");
            break;
        case 1:
            printk("  64 bytes,");
            break;
        case 2:
            printk(" 128 bytes,");
            break;
        case 3:
            printk("~220 bytes,");
            break;
        }
        i = a->read_bcr(ioaddr, 18);    /* Check Burst/Bus control */
        printk(" BCR18(%x):", i & 0xffff);
        if (i & (1 << 5))
            printk("BurstWrEn ");
        if (i & (1 << 6))
            printk("BurstRdEn ");
        if (i & (1 << 7))
            printk("DWordIO ");
        if (i & (1 << 11))
            printk("NoUFlow ");
        i = a->read_bcr(ioaddr, 25);//BCR25是SRAM的大小
        printk("\n    SRAMSIZE=0x%04x,", i << 8);
        i = a->read_bcr(ioaddr, 26);//BCR26: SRAM Boundary Register
        printk(" SRAM_BND=0x%04x,", i << 8);
        i = a->read_bcr(ioaddr, 27);//BCR27: SRAM Interface Control Register
        if (i & (1 << 14))
            printk("LowLatRx");
    }
    /* 设置初始化块 */
    self->init_block = kmalloc(sizeof(struct pcnet32_init_block), GFP_DMA);
    if (self->init_block == NULL) {
        printk("kmalloc for init block failed!\n");
    }
    self->init_dma_addr = Vir2Phy(self->init_block);
    printk("vir:%x dma:%x\n", self->init_block, self->init_dma_addr);

    /* 初始化自旋锁 */ 
    SpinLockInit(&self->lock);

    /* 设置芯片名 */
    self->name = chipname;

    /////self->tx_ring_size 表示发送ring的大小（即一个ring上有多少个tramsimit descriptor)，
    ///  #define TX_RING_SIZE  (1 << (PCNET32_LOG_TX_BUFFERS)) 
    /// default tx ring size *PCNET32_LOG_TX_BUFFERS是存储在init block中 TLEN域的值 
    self->tx_ring_size = TX_RING_SIZE;    
    ////和tx_ring_size类似，不同的是rx对应的域是RELN
    self->rx_ring_size = RX_RING_SIZE;    /* default rx ring size */
    self->tx_mod_mask = self->tx_ring_size - 1;  ////绕圈时通过tx_mod_mask取余来获得当前在ring中的位置坐标
    self->rx_mod_mask = self->rx_ring_size - 1;
    
    /*
     * 这里的tx_len_bits和rx_len_bits是后面用来填充init block的 tlen_rlen成员的
     *（tlen_rlen为__le16类型的 变量）。 tlen_rlen成员的bit12~big15用来存放tx_len_bits，
     * bit4~bit7用来存放rx_len_bigs，tlen_rlen = self->tx_len_bits | self->rx_len_bits,
     * 左移是为了把tx_len_bits、rx_len_bits准确地填入tlen_rlen对应位置中
     */
    self->tx_len_bits = (PCNET32_LOG_TX_BUFFERS << 12);  ////(4 << 12)
    self->rx_len_bits = (PCNET32_LOG_RX_BUFFERS << 4);   ////(5 << 4)

    self->chip_version = chip_version;

    self->options = options_mapping[options[cards_found]];;

    printk("chip version:%x options:%x\n", self->chip_version, self->options);

    self->a = *a;

    /* 初始化ring */
    if (Pcnet32AllocRing(self)) {
        printk("no memory!\n");
        return -1;
    }

    if (self->macAddress[0] == 0x00 && self->macAddress[1] == 0xe0
        && self->macAddress[2] == 0x75)
        self->options = PCNET32_PORT_FD | PCNET32_PORT_GPSI;

    /* 填写init block */

    /* Disable RX and Tx. This is set by CSR15 */
    self->init_block->mode = CpuToLittleEndian16(0x0003);
    ///对应TLEN 和 RLEN
    self->init_block->tlen_rlen =
        CpuToLittleEndian16(self->tx_len_bits | self->rx_len_bits);
    ///对应PADR 48bit
    for (i = 0; i < 6; i++)
        self->init_block->phys_addr[i] = self->macAddress[i];
    
    ///对应LADRF
    self->init_block->filter[0] = 0x00000000;
    self->init_block->filter[1] = 0x00000000;
    ///对应 RDRA和 TDRA， RDRA 是由传输任务描述符组成的数组(tx_ring)的首地址
    self->init_block->rx_ring = CpuToLittleEndian32(self->rx_ring_dma_addr);   //// rx_ring_dma_addr的值在Pcnet32AllocRing中被赋值
    self->init_block->tx_ring = CpuToLittleEndian32(self->tx_ring_dma_addr);

    /* 把init block地址传输到寄存器 */
    a->write_csr(ioaddr, 1, (self->init_dma_addr & 0xffff));//CSR1和CSR2中存放intial_block的起始地址。
    a->write_csr(ioaddr, 2, (self->init_dma_addr >> 16));

    printk("irq:%d\n", self->irq);

    /* 特殊接口 */

    /* 注册网络设备 */

    cards_found++; 

    /* ASEL */
    /*
    uint32_t bcr2 = pcnet32_dwio_read_bcr(ioaddr, 2);
    bcr2 |= 0x02;  
    pcnet32_dwio_write_bcr(ioaddr, 2, bcr2);
    */
    
    return 0;
}

PUBLIC int InitPcnet32Driver()
{
    struct Pcnet32Private *self = &pcnet32Private;

    /* 获取PCI设备 */
    if (Pcnet32GetInfoFromPCI(self)) {
        printk("Pcnet32 get info from PCI failed!\n");
        return -1;
    }

    if (Pcnet32InitOne(self)) {
        printk("Pcnet32 init one failed!\n");
        return -1;
    }

    if (Pcnet32Open()) {
        printk("Pcnet32 open failed!\n");
        return -1;
    }
    Spin("pcnet32");
    
    return 0;
}
#endif /* CONFIG_DRV_PACNET32 */