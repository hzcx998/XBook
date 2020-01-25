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
#include <book/byteorder.h>

#include <share/string.h>
#include <share/math.h>

#include <net/network.h>
#include <net/ethernet.h>
#include <net/nllt.h>

#include <drivers/rtl8139.h>

#define DEVNAME "rtl8139"

/* define to 1, 2 or 3 to enable copious debugging info */
#define RTL8139_DEBUG 0

/* define to 1 to disable lightweight runtime debugging checks */
#undef RTL8139_NDEBUG



#define RTL8139_MAC_ADDR_LEN    6
#define RTL8139_NUM_TX_BUFFER   4
#define RTL8139_RX_BUF_SIZE  	32*1024	


/*PCI rtl8139 config space register*/
#define RTL8139_VENDOR_ID   0x10ec
#define RTL8139_DEVICE_ID   0x8139

/*rtl8139 register*/
#define RTL8139_MAC         0x00
#define RTL8139_MAR0        0x08

#define RTL8139_TX_STATUS0  0x10
#define RTL8139_TX_ADDR0    0x20
#define RTL8139_RX_BUF      0x30
#define RTL8139_COMMAND     0x37
#define RTL8139_RX_BUF_PTR  0x38
#define RTL8139_RX_BUF_ADDR 0x3A

#define RTL8139_INTR_MASK   0x3C
#define RTL8139_INTR_STATUS 0x3E
#define RTL8139_TCR         0x40
#define RTL8139_RCR         0x44
#define RTL8139_RX_MISSED   0x4C    /* 24 bits valid, write clears. */

#define RTL8139_9346CR		0x50

#define RTL8139_CONFIG_0    0x51
#define RTL8139_CONFIG_1    0x52
#define RTL8139_CONFIG_3    0x59
#define RTL8139_CONFIG_4    0x5A

#define RTL8139_HLT_CLK     0x5B
#define RTL8139_MULTI_INTR  0x5C

#define RTL8139_CSCR        0x74    /* Chip Status and Configuration Register. */


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
#define RTL8139_CMD_TX  	0x04	/*[R/W] Transmitter Enable*/
#define RTL8139_CMD_RX  	0x08	/*[R/W] Receiver Enable*/
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
#define RTL8139_RX_TX_MXDMA  	    0x06		// 110b(1024KB)
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

#define RTL8139_TSD_BOTH  (RTL8139_TSD_OWN | RTL8139_TSD_TOK)

/*RTL8139 9346CMD register*/
#define RTL8139_9346CMD_EE  0XC0	

/*RTL8139 Tranmit configuration register*/
#define RTL8139_TCR_MXDMA_UNLIMITED  	0X07	
#define RTL8139_TCR_IFG_NORMAL  		0X07	

#define RTL8139_ETH_ZLEN  		60	

#define RX_MIN_PACKET_LENGTH  		8	

#define RX_MAX_PACKET_LENGTH  		8*1024	

#define RX_BUFFER_SIZE  		(8*1024+16)	

/* 传输缓冲区数量 */
#define TX_BUFFER_NR 4

/* ----定义接收和传输缓冲区的大小---- */

#define RX_BUF_IDX	2	/* 32K ring */

#define RX_BUF_LEN	(8192 << RX_BUF_IDX)
#define RX_BUF_PAD	16      /* 填充16字节 */
#define RX_BUF_WRAP_PAD 2048 /* spare padding to handle lack of packet wrap */

/* 接收缓冲区的总长度 */
#define RX_BUF_TOT_LEN	(RX_BUF_LEN + RX_BUF_PAD + RX_BUF_WRAP_PAD)


/* Number of Tx descriptor registers. */
#define NUM_TX_DESC	4

/* max supported ethernet frame size -- must be at least (dev->mtu+14+4).*/
#define MAX_ETH_FRAME_SIZE	1536

/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE	MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_LEN	(TX_BUF_SIZE * NUM_TX_DESC)

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH  256	/* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024, 7==end of packet. */
#define RX_FIFO_THRESH	7	/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	7	/* Maximum PCI burst, '7' is unlimited */
#define TX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define TX_RETRY	8	/* 0-15.  retries = 16 + (TX_RETRY * 16) */


enum {
	HAS_MII_XCVR = 0x010000,
	HAS_CHIP_XCVR = 0x020000,
	HAS_LNK_CHNG = 0x040000,
};

#define RTL8129_CAPS	HAS_MII_XCVR
#define RTL8139_CAPS	(HAS_CHIP_XCVR|HAS_LNK_CHNG)

typedef enum {
	RTL8139 = 0,
	RTL8129,
} board_t;


/* indexed by board_t, above */
PRIVATE CONST struct {
	const char *name;
	u32 hw_flags;
} boardInfo[] = {
	{ "RealTek RTL8139", RTL8139_CAPS },
	{ "RealTek RTL8129", RTL8129_CAPS },
};

enum ClearBitMasks {
	MULTI_INTR_CLEAR	= 0xF000,
	CHIP_CMD_CLEAR	= 0xE2,
	CONFIG1_CLEAR	= (1<<7)|(1<<6)|(1<<3)|(1<<2)|(1<<1),
};

enum ChipCmdBits {
	CMD_RESET	    = 0x10,
	CMD_RX_ENABLE	= 0x08,
	CMD_TX_ENABLE	= 0x04,
	RX_BUFFER_EMPTY	= 0x01,
};

/* Interrupt register bits, using my own meaningful names.
中断状态位
 */
enum IntrStatusBits {
	PCI_ERR		= 0x8000,
	PCS_TIMEOUT	= 0x4000,
	RX_FIFO_OVER= 0x40,
	RX_UNDERRUN	= 0x20,
	RX_OVERFLOW	= 0x10,
	TX_ERR		= 0x08,
	TX_OK		= 0x04,
	RX_ERR		= 0x02,
	RX_OK		= 0x01,
	RX_ACK_BITS	= RX_FIFO_OVER | RX_OVERFLOW | RX_OK,
};

/* 传输状态位 */
enum TxStatusBits {
	TX_HOST_OWNS	= 0x2000,
	TX_UNDERRUN	    = 0x4000,
	TX_STAT_OK	    = 0x8000,
	TX_OUT_OF_WINDOW= 0x20000000,
	TX_ABORTED	    = 0x40000000,
	TX_CARRIER_LOST	= 0x80000000,
};

/* 接收状态位 */
enum RxStatusBits {
	RX_MULTICAST	= 0x8000,
	RX_PHYSICAL	    = 0x4000,
	RX_BROADCAST    = 0x2000,
	RX_BAD_SYMBOL	= 0x0020,
	RX_RUNT		    = 0x0010,
	RX_TOO_LONG	    = 0x0008,
	RX_CRC_ERR	    = 0x0004,
	RX_BAD_Align	= 0x0002,
	RX_STATUS_OK    = 0x0001,
};

/* Bits in RxConfig.
接收模式位
 */
enum RxModeBits {
	ACCEPT_ERR	= 0x20,
	ACCEPT_RUNT	= 0x10,
	ACCEPT_BROADCAST	= 0x08,
	ACCEPT_MULTICAST	= 0x04,
	ACCEPT_MY_PHYS	= 0x02,
	ACCEPT_ALL_PHYS	= 0x01,
};

/* Bits in TxConfig. */
enum TxConfigBits {
    /* Interframe Gap Time. Only TxIFG96 doesn't violate IEEE 802.3 */
    TX_IFG_SHIFT	= 24,
    TX_IFG84		= (0 << TX_IFG_SHIFT), /* 8.4us / 840ns (10 / 100Mbps) */
    TX_IFG88		= (1 << TX_IFG_SHIFT), /* 8.8us / 880ns (10 / 100Mbps) */
    TX_IFG92		= (2 << TX_IFG_SHIFT), /* 9.2us / 920ns (10 / 100Mbps) */
    TX_IFG96		= (3 << TX_IFG_SHIFT), /* 9.6us / 960ns (10 / 100Mbps) */

	TX_LOOP_BACK	= (1 << 18) | (1 << 17), /* enable loopback test mode */
	TX_CRC		    = (1 << 16),	/* DISABLE Tx pkt CRC append */
	TX_CLEAR_ABT	= (1 << 0),	/* Clear abort (WO) */
	TX_DMA_SHIFT	= 8, /* DMA burst value (0-7) is shifted X many bits */
	TX_RETRY_SHIFT	= 4, /* TXRR value (0-15) is shifted X many bits */

	TX_VERSION_MASK	= 0x7C800000, /* mask out version bits 30-26, 23 */
};

enum RxConfigBits {
	/* rx fifo threshold */
	RX_CFG_FIFO_SHIFT	= 13,
	RX_CFG_FIFO_NONE	= (7 << RX_CFG_FIFO_SHIFT),

	/* Max DMA burst */
	RX_CFG_DMA_SHIFT	= 8,
	RX_CFG_DMA_UNLIMITED = (7 << RX_CFG_DMA_SHIFT),

	/* rx ring buffer length */
	RX_CFG_RCV_8K	= 0,
	RX_CFG_RCV_16K	= (1 << 11),
	RX_CFG_RCV_32K	= (1 << 12),
	RX_CFG_RCV_64K	= (1 << 11) | (1 << 12),

	/* Disable packet wrap at end of Rx buffer. (not possible with 64k) */
	RX_NO_WRAP	= (1 << 7),
};

/* Bits in Config1 */
enum Config1Bits {
	CFG1_PM_ENABLE	= 0x01,
	CFG1_VPD_ENABLE	= 0x02,
	CFG1_PIO	= 0x04,
	CFG1_MMIO	= 0x08,
	LWAKE		= 0x10,		/* not on 8139, 8139A */
	CFG1_DRIVER_LOAD = 0x20,
	CFG1_LED0	= 0x40,
	CFG1_LED1	= 0x80,
	SLEEP		= (1 << 1),	/* only on 8139, 8139A */
	PWRDN		= (1 << 0),	/* only on 8139, 8139A */
};

/* Bits in Config3 */
enum Config3Bits {
	CFG3_FAST_ENABLE   	    = (1 << 0), /* 1	= Fast Back to Back */
	CFG3_FUNCTION_ENABLE	= (1 << 1), /* 1	= enable CardBus Function registers */
	CFG3_CLKRUN_ENABLE	    = (1 << 2), /* 1	= enable CLKRUN */
	CFG3_CARD_BUS_ENABLE 	= (1 << 3), /* 1	= enable CardBus registers */
	CFG3_LINK_UP   	        = (1 << 4), /* 1	= wake up on link up */
	CFG3_MAGIC    	        = (1 << 5), /* 1	= wake up on Magic Packet (tm) */
	CFG3_PARM_ENABLE  	    = (1 << 6), /* 0	= software can set twister parameters */
	CFG3_GNT   	            = (1 << 7), /* 1	= delay 1 clock from PCI GNT signal */
};

/* Twister tuning parameters from RealTek.
   Completely undocumented, but required to tune bad links on some boards. */
enum CSCRBits {
	CSCR_LINK_OK		= 0x0400,
	CSCR_LINK_CHANGE	= 0x0800,
	CSCR_LINK_STATUS	= 0x0f000,
	CSCR_LINK_DOWN_OFF_CMD	= 0x003c0,
	CSCR_LINK_DOWN_CMD	= 0x0f3c0,
};

enum Config9346Bits {
	CFG9346_LOCK	= 0x00,
	CFG9346_UNLOCK	= 0xC0,
};


typedef enum {
	CH_8139	= 0,
	CH_8139_K,
	CH_8139A,
	CH_8139A_G,
	CH_8139B,
	CH_8130,
	CH_8139C,
	CH_8100,
	CH_8100B_8139D,
	CH_8101,
} Chip_t;

enum ChipFlags {
	HAS_HLT_CLK	= (1 << 0),
	HAS_LWAKE	= (1 << 1),
};

#define HW_REVID(b30, b29, b28, b27, b26, b23, b22) \
	(b30<<30 | b29<<29 | b28<<28 | b27<<27 | b26<<26 | b23<<23 | b22<<22)
#define HW_REVID_MASK	HW_REVID(1, 1, 1, 1, 1, 1, 1)

#define CHIP_INFO_NR    10

/* directly indexed by Chip_t, above */
PRIVATE const struct {
	const char *name;
	u32 version; /* from RTL8139C/RTL8139D docs */
	u32 flags;
} RtlChipInfo[CHIP_INFO_NR] = {
	{ "RTL-8139",
	  HW_REVID(1, 0, 0, 0, 0, 0, 0),
	  HAS_HLT_CLK,
	},

	{ "RTL-8139 rev K",
	  HW_REVID(1, 1, 0, 0, 0, 0, 0),
	  HAS_HLT_CLK,
	},

	{ "RTL-8139A",
	  HW_REVID(1, 1, 1, 0, 0, 0, 0),
	  HAS_HLT_CLK, /* XXX undocumented? */
	},

	{ "RTL-8139A rev G",
	  HW_REVID(1, 1, 1, 0, 0, 1, 0),
	  HAS_HLT_CLK, /* XXX undocumented? */
	},

	{ "RTL-8139B",
	  HW_REVID(1, 1, 1, 1, 0, 0, 0),
	  HAS_LWAKE,
	},

	{ "RTL-8130",
	  HW_REVID(1, 1, 1, 1, 1, 0, 0),
	  HAS_LWAKE,
	},

	{ "RTL-8139C",
	  HW_REVID(1, 1, 1, 0, 1, 0, 0),
	  HAS_LWAKE,
	},

	{ "RTL-8100",
	  HW_REVID(1, 1, 1, 1, 0, 1, 0),
 	  HAS_LWAKE,
 	},

	{ "RTL-8100B/8139D",
	  HW_REVID(1, 1, 1, 0, 1, 0, 1),
	  HAS_HLT_CLK /* XXX undocumented? */
	| HAS_LWAKE,
	},

	{ "RTL-8101",
	  HW_REVID(1, 1, 1, 0, 1, 1, 1),
	  HAS_LWAKE,
	},
};


struct RtlExtraStats {
	unsigned long earlyRX;
	unsigned long txBufMapped;
	unsigned long txTimeouts;
	unsigned long rxLostInRing;
};


struct Rtl8139Stats {
	u64	packets;
	u64	bytes;
};


struct NetDeviceStatus {
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
    /* 正确记录 */
    unsigned long txPackets;        /* 传输包数量 */
    unsigned long txBytes;          /* 传输字节数量 */
    
    unsigned long collisions;       /* 碰撞次数 */
};

struct Rtl8139Private
{
	unsigned int ioAddress;
    int drvFlags;           /* 驱动标志 */
    unsigned int irq;
    unsigned char macAddress[6];
	flags_t flags;
	struct PciDevice *pciDevice;
	struct NetDeviceStatus stats;  /* 设备状态 */

    struct RtlExtraStats xstats;    /* 扩展状态 */            

    unsigned char *rxBuffer;
	unsigned char *rxRing;
    unsigned char  currentRX;   /* CAPR, Current Address of Packet Read */
    unsigned char  rxWriteAddr; /* CBR, Current Buffer Address of Packet write */
    unsigned int  rxBufferLen;
    flags_t rxFlags;
    dma_addr_t rxRingDma;       /* dma物理地址 */
    struct Rtl8139Stats	rxStats;

    unsigned char *txBuffers;
    unsigned char *txBuffer[NUM_TX_DESC];
    unsigned long   currentTX;
    unsigned long   dirtyTX;
    Atomic_t   txFreeNR;   /* 传输空闲数量 */
    unsigned char   txSetup, txFinish;
    flags_t txFlags;
    dma_addr_t txBuffersDma;       /* dma物理地址 */
    struct Rtl8139Stats	txStats;

    Chip_t chipset;     /* 芯片集 */

    Spinlock_t lock;        /* 普通锁 */
	Spinlock_t rxLock;      /* 接受锁 */

    uint32_t rxConfig;      /* 接收配置 */

} rtl8139Private;

struct RxPacketHeader {
    /* 头信息 */
    uint16_t status;    /* 状态 */
    /*
    uint8_t pad2: 1;
    uint8_t pad3: 1;
    uint8_t pad4: 1;
    uint8_t pad5: 1;
    uint8_t pad6: 1;
    uint8_t BAR: 1;
    uint8_t PAM: 1;
    uint8_t MAR: 1;

    uint8_t ROK: 1;
    uint8_t FAE: 1;
    uint8_t CRC: 1;
    uint8_t LONG: 1;
    uint8_t RUNT: 1;
    uint8_t ISE: 1;
    uint8_t pad0: 1;
    uint8_t pad1: 1;*/
    /* 数据长度 */
    uint16_t length;    /* 数据长度 */
} PACKED;

/* 接收配置 */
PRIVATE CONST unsigned int rtl8139RxConfig =
	RX_CFG_RCV_32K | RX_NO_WRAP |
	(RX_FIFO_THRESH << RX_CFG_FIFO_SHIFT) |
	(RX_DMA_BURST << RX_CFG_DMA_SHIFT);

/* 传输配置 */
PRIVATE CONST unsigned int rtl8139TxConfig =
	TX_IFG96 | (TX_DMA_BURST << TX_DMA_SHIFT) | (TX_RETRY << TX_RETRY_SHIFT);

/* 中断屏蔽配置 */
PRIVATE CONST u16 rtl8139IntrMask =
	PCI_ERR | PCS_TIMEOUT | RX_UNDERRUN | RX_OVERFLOW | RX_FIFO_OVER |
	TX_ERR | TX_OK | RX_ERR | RX_OK;

/* 没有接收的中断屏蔽 */
PRIVATE CONST u16 rtl8139NorxIntrMask =
	PCI_ERR | PCS_TIMEOUT | RX_UNDERRUN |
	TX_ERR | TX_OK | RX_ERR ;

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

unsigned char NextDesc(unsigned char CurrentDescriptor)
{
    // (CurrentDescriptor == TX_BUFFER_NR-1) ? 0 : (1 + CurrentDescriptor);
    if(CurrentDescriptor == RTL8139_NUM_TX_BUFFER-1) {
        return 0;
    } else {
        return ( 1 + CurrentDescriptor);
    }
}

uint32_t CheckTSDStatus(unsigned char Desc)
{
    struct Rtl8139Private *private = &rtl8139Private;

    uint32_t Offset = Desc << 2;
    uint32_t tmpTSD;

    tmpTSD = In32(private->ioAddress + RTL8139_TX_STATUS0 + Offset);
    switch ( tmpTSD & (RTL8139_TSD_OWN | RTL8139_TSD_TOK) )
    {
    case (RTL8139_TSD_BOTH): return (RTL8139_TSD_BOTH);
    case (RTL8139_TSD_TOK) : return RTL8139_TSD_TOK;
    case (RTL8139_TSD_OWN) : return RTL8139_TSD_OWN;
    case 0 : return 0;
    }
    return 0;
}

void IssueCMD(unsigned char descriptor, uint32_t phyAddr, uint32_t len)
{
    struct Rtl8139Private *private = &rtl8139Private;

    unsigned long offset = descriptor << 2;
    Out32(private->ioAddress + RTL8139_TX_ADDR0 + offset, phyAddr);
    Out32(private->ioAddress + RTL8139_TX_STATUS0 + offset , len);
}

/**
 * Rtl8139NextDesc - 获取下一个传输项
 * @currentDesc: 当前传输项
 * 
 * 让传输项的取值处于0~3
 */
PRIVATE int Rtl8139NextDesc(int currentDesc)
{
    return (currentDesc == NUM_TX_DESC - 1) ? 0 : (currentDesc + 1);
}

PUBLIC int Rtl8139Transmit(char *buf, uint32 len)
{
    struct Rtl8139Private *private = &rtl8139Private;
    uint32_t entry;
    uint32_t length = len;

    /* 获取当前传输项 */
    entry = private->currentTX;
    /* 改变传输项状态，开始数据传输 */
    DisableInterrupt();

    /* 如果还有剩余的传输项 */
    if (AtomicGet(&private->txFreeNR) > 0) {
#if RTL8139_DEBUG == 1        
        printk("Start TX, free %d\n", AtomicGet(&private->txFreeNR));
#endif
        /* 如果长度是在传输范围内 */
        if (likely(length < TX_BUF_SIZE)) {
            /* 比最小数据帧还少 */
            if (length < ETH_ZLEN)
                memset(private->txBuffer[entry], 0, ETH_ZLEN);  /* 前面的部分置0 */

            /* 复制数据 */
            memcpy(private->txBuffer[entry], buf, length);

        } else {    /* 长度过长 */
            /* 丢掉数据包 */
            private->stats.txDropped++; 
            printk("dropped a packed!\n");
            return 0;
        }
        
        /*
        * Writing to TxStatus triggers a DMA transfer of the data
        * copied to private->txBuffer[entry] above. Use a memory barrier
        * to make sure that the device sees the updated data.
        * 上面复制了数据，为了让设备知道更新了数据，这里用一个写内存屏障
        */
        WriteMemoryBarrier();

        /* 传输至少60字节的数据 */
        Out32(private->ioAddress + RTL8139_TX_STATUS0 + (entry * 4),
                private->txFlags | max(length, (uint32_t )ETH_ZLEN));
        In32(private->ioAddress + RTL8139_TX_STATUS0 + (entry * 4)); // flush

        /* 指向下一个传输描述符 */
        private->currentTX = Rtl8139NextDesc(private->currentTX);

        /* 减少传输项数量 */
        AtomicDec(&private->txFreeNR);
#if RTL8139_DEBUG == 1
        printk("Start OK, free %d\n", AtomicGet(&private->txFreeNR));
#endif        
    } else {
        // 停止传输 */
        printk("Stop Tx packet!\n");
        //netif_stop_queue (dev);

        EnableInterrupt();
        return -1;
    }
    EnableInterrupt();
#if RTL8139_DEBUG == 1 
    printk("Queued Tx packet size %d to slot %d\n",
		    length, entry);
#endif
    return 0;
}

PRIVATE int Rtl8139TxInterrupt(struct Rtl8139Private *private)
{
    printk("TX\n");
    /* 空闲数量小于传输描述符数量才能进行处理 */
    while (AtomicGet(&private->txFreeNR) < NUM_TX_DESC) {
#if RTL8139_DEBUG == 1        
        printk("TX, free %d\n", AtomicGet(&private->txFreeNR));
#endif
        /* 获取第一个脏传输 */
        int entry = private->dirtyTX;
        int txStatus;

        /* 读取传输状态 */
        txStatus = In32(private->ioAddress + RTL8139_TX_STATUS0 + (entry * 4));
        
        /* 如果传输状态不是下面的状态之一，就跳出，说明还被处理 */
        if (!(txStatus & (TX_STAT_OK | TX_UNDERRUN | TX_ABORTED))) {
            printk("tx status not we want!\n");
            break;
        }

        /* Note: TxCarrierLost is always asserted at 100mbps.
        如果是超出窗口，出错
         */
        if (txStatus & (TX_OUT_OF_WINDOW | TX_ABORTED)) {
            /* 有错误，记录下来 */
            printk("Transmit error, Tx status %x\n",
				    txStatus);    
            private->stats.txErrors++;
            if (txStatus & TX_ABORTED) {
                private->stats.txAbortedErrors++;
                /* 清除传输控制的abort位 */
                Out32(private->ioAddress + RTL8139_TCR, TX_CLEAR_ABT);

                /* 中断状态设置成传输错误 */
                Out16(private->ioAddress + RTL8139_INTR_STATUS, TX_ERR);
                /* 写内存屏障 */
                WriteMemoryBarrier();
            }
            /* 携带丢失 */
            if (txStatus & TX_CARRIER_LOST)
                private->stats.txCarrierErrors++;
            
            /* 超过窗口大小 */
            if (txStatus & TX_OUT_OF_WINDOW)
                private->stats.txWindowErrors++;
            
        } else {
            /* 传输正在运行中 */
            if (txStatus & TX_UNDERRUN) {
                /* Add 64 to the Tx FIFO threshold.
                传输阈值增加64字节
                 */
                if (private->txFlags < 0x00300000) {
                    private->txFlags += 0x00020000;
                }
                private->stats.txFifoErrors++;

            }    
            /* 记录碰撞次数 */        
            private->stats.collisions += (txStatus >> 24) & 15;

            /* 增加包数量 */
            private->txStats.packets++;
            /* 增加传输的字节数 */
            private->txStats.bytes += txStatus & 0x7ff;
        }

        /* 指向下一个传输描述符 */
        private->dirtyTX = Rtl8139NextDesc(private->dirtyTX);

        /* 如果空闲数量是0，表明需要唤醒传输队列 */
        if (AtomicGet(&private->txFreeNR) == 0) {
            /* 执行唤醒传输队列操作 */
            MemoryBarrier();
            // 唤醒队列 
		    //netif_wake_queue (dev);
            printk("Wake up queue!\n");
        }

        /* 空闲数量又增加 */
        AtomicInc(&private->txFreeNR);
#if RTL8139_DEBUG == 1
        printk("OK, free %d\n", AtomicGet(&private->txFreeNR));
#endif
    }
#if RTL8139_DEBUG == 1    
    printk("TX, end free %d\n", AtomicGet(&private->txFreeNR));
#endif
    return 0;
}



PUBLIC int Rtl8139Transmit3(char *buf, uint32 len)
{
    printk("tx: len %d\n", len);
    struct Rtl8139Private *private = &rtl8139Private;
    //enum InterruptStatus oldStatus = InterruptDisable();

    DisableInterrupt();
    if (AtomicGet(&private->txFreeNR) > 0) {
        uint32 tsd = In32(private->ioAddress + RTL8139_TX_STATUS0 + (private->txSetup << 2));
        
        /* 如果该传输项还未被使用，就不传输 */
        if (!(tsd & (RTL8139_TSD_TOK | RTL8139_TSD_TUN | RTL8139_TSD_TABT)))
			return -1;

        /* 如果有错误，就不操作该项 */

        /* 复制数据到当前的传输项 */
        memcpy(private->txBuffer[private->txSetup], buf, len);

        uint32 status = In16(private->ioAddress + RTL8139_INTR_STATUS);

        printk(">>>transmit TX_STATUS%d: %x, INTR_STATUS: %x\n", private->txSetup, 
                In32(private->ioAddress + RTL8139_TX_STATUS0 + (private->txSetup << 2)), status);
        
        IssueCMD(private->txSetup, Vir2Phy(private->txBuffer[private->txSetup]), len);
        
        printk("<<<after transmit TX_STATUS%d: TSD %x\n", private->txSetup, 
                In32(private->ioAddress + RTL8139_TX_STATUS0 + (private->txSetup << 2)));
        
        private->txSetup = NextDesc(private->txSetup);

        AtomicDec(&private->txFreeNR);
        //private->txFreeNR--;
        EnableInterrupt();
        //InterruptSetStatus(oldStatus);
        return -1;
    } else {
        printk("!!!no free desc!!!\n");
    }
    EnableInterrupt();
    //InterruptSetStatus(oldStatus);
    return 0;
}

PUBLIC int Rtl8139Transmit2(char *buf, uint32 len)
{
    
    struct Rtl8139Private *private = &rtl8139Private;
    /* 复制数据到当前的传输项 */
    memcpy(private->txBuffer[private->currentTX], buf, len);
    
    //uint32 status = In16(private->ioAddress + RTL8139_INTR_STATUS);
    /*printk("transmit TX_STATUS0: 0x%x, INTR_STATUS: %x\n", 
            In32(private->ioAddress + RTL8139_TX_STATUS0), status);
    */
    enum InterruptStatus oldStatus = InterruptDisable();
    /* 把当前数据项对应的缓冲区的物理地址写入寄存器 */
    //Out32(private->ioAddress + RTL8139_TX_ADDR0 + private->currentTX * 4, Vir2Phy(private->txBuffers[private->currentTX]));
    
    /* 往传输状态寄存器写入传输的状态
    数据长度是0~12位。
     */
    Out32(private->ioAddress + RTL8139_TX_STATUS0 + private->currentTX * 4, (256 << 16) | 0x0 | (len & 0X1FFF));
    
    //printk("after transmit TX_STATUS0: 0x%x\n", In32(private->ioAddress + RTL8139_TX_STATUS0 + private->currentTX * 4));
    
    /* 指向下一个传输项 */
    private->currentTX = (private->currentTX + 1) % 4;

    InterruptSetStatus(oldStatus);

    return 0;
}

bool PacketOK(uint16_t status, uint16_t length)
{
    uint16_t badPacket = (status & (1<<4)) || /* RUNT */
        (status & (1<<5)) || /* ISE */
        (status & (1<<3)) || /* LONG */
        (status & (1<<2)) ||    /* CRC */
        (status & (1<<1));      /* FAE */
    
    //uint16_t badPacket = 0;
    
    if( ( !badPacket ) && ( status & 0x01) ) {
        if ( (length > RX_MAX_PACKET_LENGTH ) ||
                (length < RX_MIN_PACKET_LENGTH ) ) {
            return false; 
        }
        //PacketReceivedGood++;
        //ByteReceived += pktHeader->PacketLength;
        //printk("@@@");
        return true;
    } else {
        return false;
    }
}

PRIVATE int TxInterruptHandler()
{
    struct Rtl8139Private *private = &rtl8139Private;

    /*while ((CheckTSDStatus(private->txFinish) == RTL8139_TSD_BOTH) &&
        (private->txFreeNR < TX_BUFFER_NR)) 
    {*/
        uint32 tx_status = In32(private->ioAddress + RTL8139_TX_STATUS0 + private->txFinish * 4);
        /*printk("TXOK, TX_STATUS%d: %x ->", private->txFinish, tx_status);

        if (tx_status & (1<<13)) printk("OWN ");
        if (tx_status & (1<<14)) printk("TUN ");
        if (tx_status & (1<<15)) printk("TOK ");
        if (tx_status & (1<<29)) printk("OWC ");
        if (tx_status & (1<<30)) printk("TABT ");
        if (tx_status & (1<<31)) printk("CRS ");*/
        
        tx_status &= ~0x1fffUL;
        Out32(private->ioAddress + RTL8139_TX_STATUS0 + private->txFinish * 4 , tx_status);

        private->txFinish = NextDesc(private->txFinish);
        //private->txFreeNR++;
        AtomicInc(&private->txFreeNR);
    //}
    return 0;
}

BOOLEAN RxInterruptHandler()
{
    struct Rtl8139Private *private = &rtl8139Private;

    unsigned char TmpCMD;
    unsigned int length;
    unsigned char *incomePacket, *RxReadPtr;
    //struct RxPacketHeader *packetHeader;
    uint32 cur;
    //printk("<<<");

    /*  1.获取上次读取的位置
        2.分析这次的数据
        3.移动到下一次的位置
     */

    /*if ((In8(private->ioAddress + RTL8139_COMMAND) & RTL8139_CMD_BUFE) == 0) {
        printk("\n$BUFFER DATA$\n");

    }*/

    while ((In8(private->ioAddress + RTL8139_COMMAND) & RTL8139_CMD_BUFE) == 0) {
        
        cur = private->currentRX;
        //uint32 offset = cur % private->rxBufferLen; /* 避免越界判断 */
        uint32 offset = cur;

        uint8* rx = private->rxBuffer + offset; /* 获取数据区域 */
        uint32_t status = *(uint32_t *)rx;      /* 获取状态以及长度 */
        length = status >> 16;
        uint16_t base = In16(private->ioAddress + RTL8139_RX_BUF_ADDR);
        if (status & 1) printk("ROK ");
        if (status & (1<<1)) printk("FAE ");
        if (status & (1<<2)) printk("CRC ");
        if (status & (1<<3)) printk("LONG ");
        if (status & (1<<4)) printk("RUNT ");
        if (status & (1<<5)) printk("ISE ");
        if (status & (1<<13)) printk("BAR ");
        if (status & (1<<14)) printk("PAM ");
        if (status & (1<<15)) printk("MAR ");
    
        //length = base - cur;;
        if (length == 0 && status == 0) {
            printk("EMPTY!\n");
            //break;
        }
            
        /* 打印状态以及长度 */
        printk("offset:%x status:%x length:%x\n",
            offset, status, length);

        /* 移动到下一个位置 */
        cur = (cur + length + 4 + 3) & ~3;

        printk("[write:%x ->(read:%x, next:%x) size:%x]\n", base, offset, cur, base - cur);
        
        /* 打印数据 */
        int i;
        for (i = 0; i < 16; i++) {
            printk("%x ", rx[4 + i]);
        }
        printk("\n\n");


        //-4:avoid overflow
        //Out16(private->ioAddress + RTL8139_RX_BUF_PTR, cur - 0x10); 

        /* 保存修改后的位置 */
        private->currentRX = cur;
    }
    //printk(">>>");
    return 0;





    printk("RXOK, ");
    
    while (!(In8(private->ioAddress + RTL8139_COMMAND) & RTL8139_CMD_BUFE)) {
        /*TmpCMD = In8(private->ioAddress + RTL8139_COMMAND);
        if (TmpCMD & RTL8139_CMD_BUFE) 
            break;*/
            
        //printk("b");
        //do {
            RxReadPtr = private->rxBuffer + cur;
            uint32 offset = cur % private->rxBufferLen;
            uint32_t status = *(uint32_t *)RxReadPtr;
            length = status >> 16;
            
            incomePacket = RxReadPtr + 4;
            //length = packetHeader->length; //this length include CRC
            printk("header:%x, %x\n", status, length);

            if(length == 0)
                break;

            //printk("header:%x, %x\n", status, length);
            /*   if (status & 1) printk("ROK ");
                if (status & (1<<1)) printk("FAE ");
                if (status & (1<<2)) printk("CRC ");
                if (status & (1<<3)) printk("LONG ");
                if (status & (1<<4)) printk("RUNT ");
                if (status & (1<<5)) printk("ISE ");
                if (status & (1<<13)) printk("BAR ");
                if (status & (1<<14)) printk("PAM ");
                if (status & (1<<15)) printk("MAR ");
            
            if (PacketOK(status, length)) {

                printk("\n");
                int i;
                for (i = 0; i < 32; i++) {
                    printk("%x ", incomePacket[i]);
                }
                
                printk("\n");
            }*/
            
            //update Read Pointer
            //4:for header length(length include 4 bytes CRC)
            //3:for dword alignment
            cur = (cur + length + 4 + 3) & ~3;
            //-4:avoid overflow
            Out16(private->ioAddress + RTL8139_RX_BUF_PTR, private->currentRX - 0x10); 


            if (PacketOK(status, length)) {
            /* 是正确的包才投递 */
            
                /*if ( (private->currentRX + length) > RX_BUFFER_SIZE ) {
                    //wrap around to end of RxBuffer
                    memcpy( private->rxBuffer + RX_BUFFER_SIZE ,private->rxBuffer,
                            (private->currentRX + length - RX_BUFFER_SIZE) );
                }*/
                //copy the packet out here
                //CopyPacket(incomePacket,length - 4);//don't copy 4 bytes CRC
                
                //if (PacketOK(status, length)) {
                NlltReceive(incomePacket, length - 4);
                //}

                //update Read Pointer
                //4:for header length(length include 4 bytes CRC)
                //3:for dword alignment
                //private->currentRX = (private->currentRX + length + 4 + 3) & ~3;
                
                //-4:avoid overflow
                //Out16(private->ioAddress + RTL8139_RX_BUF_PTR, private->currentRX - 0x10); 
            } else {
                break;
            }

            
            //}
            /*} else {
                // ResetRx();
                break;
            }*/
           //TmpCMD = In8(private->ioAddress + RTL8139_COMMAND);
       // } while (!(TmpCMD & RTL8139_CMD_BUFE));
    }
    private->currentRX = cur;

    printk("RXEND, ");
    return (TRUE); //Done
}

void Rtl8139Receive()
{
    struct Rtl8139Private *private = &rtl8139Private;
    
    printk("RXOK, ");
    uint8* rx = private->rxBuffer;
    uint32 cur = private->currentRX;

    /* 如果属于主机 */
    while (!(In8(private->ioAddress + RTL8139_COMMAND) & 0x01)) {
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
        Out16(private->ioAddress + RTL8139_RX_BUF_PTR, cur);
        
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
 * Rtl8139WeirdInterrupt - 非传输和接收中断处理
 * 
 */
PRIVATE void Rtl8139WeirdInterrupt(struct Rtl8139Private *private,
        int status, int linkChanged)
{
#if RTL8139_DEBUG == 1
    printk("Abnormal interrupt, status %x\n", status);
#endif
    /* Update the error count.
    更新错过的包数
     */
	private->stats.rxMissedErrors += In32(private->ioAddress + RTL8139_RX_MISSED);
	/* 归零，下次计算 */
    Out32(private->ioAddress + RTL8139_RX_MISSED, 0);

	if ((status & RX_UNDERRUN) && linkChanged &&
	    (private->drvFlags & HAS_LNK_CHNG)) {
		
        /* 需要检测设备 */
        //rtl_check_media(dev, 0);
		status &= ~RX_UNDERRUN; /* 清除运行下状态位 */
	}

	if (status & (RX_UNDERRUN | RX_ERR))
		private->stats.rxErrors++;

	if (status & PCS_TIMEOUT)
		private->stats.rxLengthErrors++;
	if (status & RX_UNDERRUN)
		private->stats.rxFifoErrors++;
	if (status & PCI_ERR) {
        /* pci错误 */
		u32 pciCmdStatus;
        pciCmdStatus = PciDeviceRead(private->pciDevice, PCI_STATUS_COMMAND);
        PciDeviceWrite(private->pciDevice, PCI_STATUS_COMMAND, pciCmdStatus);
		/* 只需要高16位 */
        printk("PCI Bus error %x\n", pciCmdStatus >> 16);
	}
}

PRIVATE void Rtl8139RxError(u32 rxStatus,struct Rtl8139Private *private)
{
    u8 tmp8;
#if RTL8139_DEBUG == 1
    printk("Ethernet frame had errors, status %x\n", rxStatus);
#endif
    private->stats.rxErrors++;
    
    /* 如果没有接收成功 */
    if (!(rxStatus & RX_STATUS_OK)) {
        if (rxStatus & RX_TOO_LONG) {
            printk("Oversized Ethernet frame, status %x!\n", rxStatus);
            /* A.C.: The chip hangs here. */
        }
        /* 帧错误 */
        if (rxStatus & (RX_BAD_SYMBOL | RX_BAD_Align))
            private->stats.rxFrameErrors++;
        
        /* 长度错误 */
        if (rxStatus & (RX_RUNT | RX_TOO_LONG))
            private->stats.rxLengthErrors++;
        
        /* CRC错误 */
        if (rxStatus & RX_CRC_ERR)
            private->stats.rxCrcErrors++;
    } else {
        /* 接收成功，但会被丢掉 */
        private->xstats.rxLostInRing++;
    }

    /* ---接收重置---- */

    tmp8 = In8(private->ioAddress + RTL8139_COMMAND);
    /* 清除接收位 */
    Out8(private->ioAddress + RTL8139_COMMAND, tmp8 & ~CMD_RX_ENABLE);
    /* 再写入接收位 */
    Out8(private->ioAddress + RTL8139_COMMAND, tmp8);
    /* 写入接收配置 */
    Out32(private->ioAddress + RTL8139_RCR, private->rxConfig);
    private->currentRX = 0;

}

PRIVATE void Rtl8139IsrAck(struct Rtl8139Private *private)
{
    u16 status;

    status = In16(private->ioAddress + RTL8139_INTR_STATUS) & RX_ACK_BITS;

    /* Clear out errors and receive interrupts */
    if (likely(status != 0)) {
        /* 如果有错误，就记录错误信息 */
        if (unlikely(status & (RX_FIFO_OVER | RX_OVERFLOW))) {
            private->stats.rxErrors++;
            if (status & RX_FIFO_OVER)
                private->stats.rxFifoErrors++;   
        }
        /* 写入接收回答位 */
        Out16(private->ioAddress + RTL8139_INTR_STATUS, RX_ACK_BITS);
        In16(private->ioAddress + RTL8139_INTR_STATUS); // for flush

    }
}


PRIVATE int Rtl8139RxInterrupt(struct Rtl8139Private *private)
{
    printk("RX\n");
    int received = 0;
    unsigned char *rxRing = private->rxRing;
    unsigned int currentRX = private->currentRX;
    unsigned int rxSize = 0;
#if RTL8139_DEBUG == 1
    printk("In %s(), current %x BufAddr %x, free to %x, Cmd %x\n",
		    __func__, (u16)currentRX,
		    In16(private->ioAddress + RTL8139_RX_BUF_ADDR),
            In16(private->ioAddress + RTL8139_RX_BUF_PTR),
            In8(private->ioAddress + RTL8139_COMMAND));
#endif
    /* 当队列在运行中，并且接收缓冲区不是空 */
    while (!(In8(private->ioAddress + RTL8139_COMMAND) & RX_BUFFER_EMPTY)) {
        /* 获取数据的偏移 */
        u32 ringOffset = currentRX % RX_BUF_LEN;
        u32 rxStatus;

        unsigned int pktSize;

        /* 要读取DMA内存 */
        ReadMemoryBarrier();

        /* read size+status of next frame from DMA ring buffer
        读取接收的状态
         */
        rxStatus = LittleEndian32ToCpu(*(uint32_t *)(rxRing + ringOffset));
        /* 接收的大小在高16位 */
        rxSize = rxStatus >> 16;
        /* 如果没有不接收CRC特征，包的大小就是接收大小-4
        这里，默认没有此特征
         */
        pktSize = rxSize - 4;
#if RTL8139_DEBUG == 1
        printk("%s() status %x, size %x, cur %x\n",
			  __func__, rxStatus, rxSize, currentRX);
#endif
        /* 如果要打印收到的数据，就可以在此打印 */
#if RTL8139_DEBUG > 1
        /*printk("\nFrame contents:\n");
        int i;
        for (i = 0; i < 16; i++) {
            printk("%x ", rxRing[ringOffset + i]);
        }
        printk("\n");*/
        
#endif

        /* Packet copy from FIFO still in progress.
		 * Theoretically, this should never happen
		 * since EarlyRx is disabled.
         * 如果还在FIFO传输，就发生了此中断，就在此处理。
         * 但是如果关闭了EarlyRX，那么这个就不会发生
		 */
        if (unlikely(rxSize == 0xfff0)) {
            /* fifo 复制超时处理 */

            /* 一般处理 */
            printk("fifo copy in progress\n");
            private->xstats.earlyRX++;
            /* 跳出运行 */
            break;
        }
        /* If Rx err or invalid rx_size/rx_status received
		 * (which happens if we get lost in the ring),
		 * Rx process gets reset, so we abort any further
		 * Rx processing.
         * 接收大小过大或者过小，并且不是接收成功
		 */
        if (unlikely((rxSize > (MAX_ETH_FRAME_SIZE + 4)) ||
                (rxSize < 8) ||
                (!(rxStatus & RX_STATUS_OK)))) {
            
            /* 如果接收所有包 */
            if ((rxSize <= (MAX_ETH_FRAME_SIZE + 4)) &&
                (rxSize >= 8) &&
                (!(rxStatus & RX_STATUS_OK))) {
                private->stats.rxErrors++;
                if (rxStatus & RX_CRC_ERR) {
                    private->stats.rxCrcErrors++;
                    goto ToKeepPkt;
                }
                if (rxStatus & RX_RUNT) {
                    private->stats.rxLengthErrors++;
                    goto ToKeepPkt;
                }
            }
            /* 接收的错误处理 */
            Rtl8139RxError(rxStatus,private);
            received = -1;
            goto ToOut;
        }
        
ToKeepPkt:
        /* Malloc up new buffer, compatible with net-2e. */
		/* Omit the four octet CRC from the length. */
#if RTL8139_DEBUG == 1
        printk("\n#RX: upload packet.\n");
#endif    
        //NlltReceive(&rxRing[ringOffset + 4], pktSize);
        /* 创建接收缓冲区，并把数据复制进去 */
        //if (!NlltReceive(&rxRing[ringOffset + 4], pktSize)) {
            /* 更新状态 */
            private->rxStats.packets++;
            private->rxStats.bytes += pktSize;
            
        /*} else {
            private->stats.rxDropped++;
        }*/
        received++;

        /* 接收指针指向下一个位置 */
        currentRX = (currentRX + rxSize + 4 + 3) & ~3;
        Out16(private->ioAddress + RTL8139_RX_BUF_PTR, (u16)(currentRX - 16));

        Rtl8139IsrAck(private);

    }

    /* 如果没有接收到或者大小出错，上面的ACK就不会执行到，在这里再次调用 */
    if (unlikely(!received || rxSize == 0xfff0))
        Rtl8139IsrAck(private);
#if RTL8139_DEBUG == 1
    printk("Done %s(), current %04x BufAddr %04x, free to %04x, Cmd %02x\n",
		    __func__, currentRX,
		    In16(private->ioAddress + RTL8139_RX_BUF_ADDR),
            In16(private->ioAddress + RTL8139_RX_BUF_PTR),
            In8(private->ioAddress + RTL8139_COMMAND));
#endif
    private->currentRX = currentRX;
ToOut:
    return received;
}

/**
 * KeyboardHandler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void Rtl8139Handler(unsigned int irq, unsigned int data)
{
	//printk("Rtl8139Handler occur!\n");
    //DisableInterrupt();
    struct Rtl8139Private *private = &rtl8139Private;

    u16 status, ackstat;

    int linkChanged = 0; /* avoid bogus "uninit" warning */
	int handled = 0;

    SpinLock(&private->lock);

    /* 读取中断状态 */
    status = In16(private->ioAddress + RTL8139_INTR_STATUS);
    //Out16(private->ioAddress + RTL8139_INTR_STATUS, status);
    
    /* 如果一个状态位也没有，就退出 */
    if (unlikely((status & rtl8139IntrMask) == 0)) 
        goto ToOut;
    //printk("int status:%x\n", status);

    /* 如果网络没有运行的时候发生中断，那么就退出 */
    /*if (unlikely(!netif_running(dev))) {
        // 中断屏蔽位置0，从此不接收任何中断 
        Out16(private->ioAddress + RTL8139_INTR_MASK, 0);
        goto ToOut;
    }*/

    /* Acknowledge all of the current interrupt sources ASAP, but
	   an first get an additional status bit from CSCR.
       中断状态是接收的运行下，就查看连接是否改变
     */
	if (unlikely(status & RX_UNDERRUN))
		linkChanged = In16(private->ioAddress + RTL8139_CSCR) & CSCR_LINK_CHANGE;

    /* 读取状态中非ACK和ERR的状态 */
    ackstat = status & ~(RX_ACK_BITS | TX_ERR);
	if (ackstat)    /* 有则写入该状态 */
		Out16(private->ioAddress + RTL8139_INTR_STATUS, ackstat);

	/* 
    处理接收中断
    */
    uint32_t received;
    SpinLock(&private->rxLock);
	received = 0;
    /* 如果有接收状态，那么就处理接收包 */
    if (status & RX_ACK_BITS){
        /* 调用接收处理函数 */
        received = Rtl8139RxInterrupt(private);
#if RTL8139_DEBUG == 1
        printk("\n@RX: received %d packets\n", received);
#endif
	}
    SpinUnlock(&private->rxLock);
    
	/* Check uncommon events with one test. */
	if (unlikely(status & (PCI_ERR | PCS_TIMEOUT | RX_UNDERRUN | RX_ERR)))
        Rtl8139WeirdInterrupt(private, status, linkChanged);

    /* 处理接收 */
    if (status & (TX_OK | TX_ERR)) {
		Rtl8139TxInterrupt(private);

        /* 如果状态有错误，那么就把错误写入状态寄存器 */
		if (status & TX_ERR)
			Out16(private->ioAddress + RTL8139_INTR_STATUS, TX_ERR);
        else /* 如果没有错误，也要写一次中断状态，那么才会再次接收"接收中断" */
            Out16(private->ioAddress + RTL8139_INTR_STATUS, 0);
    }

    /* 处理完后才允许响应下一个中断 */
    
    //EnableInterrupt();

ToOut:
    SpinUnlock(&private->lock);
#if RTL8139_DEBUG == 1
    printk("exiting interrupt, intr_status=%x\n",
		   In16(private->ioAddress + RTL8139_INTR_STATUS));
#endif    
    return;
}

PRIVATE void Rtl8139ChipReset(struct Rtl8139Private *private)
{
    /* software reset, to clear the RX and TX buffers and set everything back to defaults
        重置网卡
     */
    Out8(private->ioAddress + RTL8139_COMMAND, RTL8139_CMD_RST);
    
    /* 循环检测芯片是否完成重置 */
    while (1) {
		Barrier();
        /* 如果重置完成，该位会被置0 */
		if ((In8(private->ioAddress + RTL8139_COMMAND) & RTL8139_CMD_RST) == 0)
			break;
        CpuNop();
	}

}

PRIVATE int Rtl8139InitBoard(struct Rtl8139Private *private)
{

    /* check for missing/broken hardware
    检查丢失/损坏的硬件
     */
	if (In32(private->ioAddress + RTL8139_TCR) == 0xFFFFFFFF) {
		printk("Chip not responding, ignoring board\n");
		return -1;
	}

    uint32_t version = In32(private->ioAddress + RTL8139_TCR) & HW_REVID_MASK;
    int i;
    for (i = 0; i < CHIP_INFO_NR; i++)
		if (version == RtlChipInfo[i].version) {
			private->chipset = i;
			goto ToMatch;
		}
    
    /* if unknown chip, assume array element #0, original RTL-8139 in this case */
	i = 0;
	printk("unknown chip version, assuming RTL-8139\n");
	printk("TxConfig = 0x%x\n", In32(private->ioAddress + RTL8139_TCR));
	private->chipset = 0;

ToMatch:
    printk("chipset id (%x) == index %d, '%s'\n",
            version, i, RtlChipInfo[i].name);


    /* 给网卡加电，启动它 */
	if (private->chipset >= CH_8139B) {
		printk("PCI PM wakeup, not support now!\n");
	} else {
		printk("Old chip wakeup\n");
		uint8_t tmp8 = In8(private->ioAddress + RTL8139_CONFIG_1);
		tmp8 &= ~(SLEEP | PWRDN);
        /* 启动网卡 */
        Out8(private->ioAddress + RTL8139_CONFIG_1, tmp8);
	}

    /* 重置芯片 */
    Rtl8139ChipReset(private);

    return 0;
}


/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
PRIVATE void Rtl8139InitRing(struct Rtl8139Private *private)
{
	int i;

	private->currentRX = 0;
	private->currentTX = 0;
	private->dirtyTX = 0;

    /* 空闲的传输项数量 */
    AtomicSet(&private->txFreeNR, NUM_TX_DESC);

	for (i = 0; i < NUM_TX_DESC; i++)
		private->txBuffer[i] = (unsigned char *)&private->txBuffers[i * TX_BUF_SIZE];
}

PRIVATE void __SetRxMode(struct Rtl8139Private *private)
{
	printk("rtl8139_set_rx_mode(%x) done -- Rx config %x\n",
		    private->flags,
            In32(private->ioAddress + RTL8139_RCR));

    /* 一般内容 */
    int rxMode = ACCEPT_BROADCAST | ACCEPT_MY_PHYS | ACCEPT_MULTICAST;

    /* 其它所有内容 */
    rxMode |= (ACCEPT_ERR | ACCEPT_RUNT);

    uint32_t tmp;
    tmp = rtl8139RxConfig | rxMode;
    if (private->rxConfig != tmp) {
        /* 写入寄存器 */
        Out32(private->ioAddress + RTL8139_RCR, tmp);
        /* 刷新流水线 */
        In32(private->ioAddress + RTL8139_RCR);
        
        private->rxConfig = tmp;
    }

    /* 过滤处理 */
    u32 mcFilter[2];

    mcFilter[0] = mcFilter[1] = 0;

    /* 写入过滤到寄存器 */
    Out32(private->ioAddress + RTL8139_MAR0 + 0, mcFilter[0]);
    In32(private->ioAddress + RTL8139_MAR0 + 0);
    
    Out32(private->ioAddress + RTL8139_MAR0 + 4, mcFilter[1]);
    In32(private->ioAddress + RTL8139_MAR0 + 4);
    
}

PRIVATE void Rtl8139SetRxMode(struct Rtl8139Private *private)
{
	enum InterruptStatus oldStatus = SpinLockSaveIntrrupt(&private->lock);

	__SetRxMode(private);

    SpinUnlockRestoreInterrupt(&private->lock, oldStatus);
}

PRIVATE void rtl8139HardwareStart(struct Rtl8139Private *private)
{

    /* Bring old chips out of low-power mode.
    把老芯片带出低耗模式
     */
	if (RtlChipInfo[private->chipset].flags & HAS_HLT_CLK)
		Out8(private->ioAddress + RTL8139_HLT_CLK, 'R');

    /* 重置芯片 */
    Rtl8139ChipReset(private);

    /* unlock Config[01234] and BMCR register writes
    解锁配置寄存器
     */
    Out8(private->ioAddress + RTL8139_9346CR, CFG9346_UNLOCK);
    In8(private->ioAddress + RTL8139_9346CR);   // flush

    /* Restore our idea of the MAC address.
    重载mac地址
     */
    Out32(private->ioAddress + RTL8139_MAC, 
            LittleEndian32ToCpu(*(uint32_t *)(private->macAddress + 0)));
    In32(private->ioAddress + RTL8139_MAC);
    
    Out16(private->ioAddress + RTL8139_MAC + 4, 
            LittleEndian32ToCpu(*(uint16_t *)(private->macAddress + 4)));
    In16(private->ioAddress + RTL8139_MAC + 4);
    
    private->currentRX = 0;

    /* init Rx ring buffer DMA address
    写接收缓冲区的dma地址到寄存器 */
	Out32(private->ioAddress + RTL8139_RX_BUF, private->rxRingDma);
    In32(private->ioAddress + RTL8139_RX_BUF);

    /* Must enable Tx/Rx before setting transfer thresholds!
    必须在设置传输阈值前打开TX/RX功能
     */
	Out8(private->ioAddress + RTL8139_COMMAND, RTL8139_CMD_RX | RTL8139_CMD_TX);
    
    /* 设定接收配置 */
    private->rxConfig = rtl8139RxConfig | ACCEPT_BROADCAST | ACCEPT_MY_PHYS;
	
    /* 把传输配置和接收配置都写入寄存器 */
    Out32(private->ioAddress + RTL8139_RCR, private->rxConfig);
    Out32(private->ioAddress + RTL8139_TCR, rtl8139TxConfig);

    if (private->chipset >= CH_8139B) {
		/* Disable magic packet scanning, which is enabled
		 * when PM is enabled in Config1.  It can be reenabled
		 * via ETHTOOL_SWOL if desired.
         * 清除MAGIC位
         */
		Out8(private->ioAddress + RTL8139_CONFIG_3,
            In8(private->ioAddress + RTL8139_CONFIG_3) & ~CFG3_MAGIC);
	}

    printk("init buffer addresses\n");

    /* Lock Config[01234] and BMCR register writes
    上锁，不可操作这些寄存器
     */
    Out8(private->ioAddress + RTL8139_9346CR, CFG9346_LOCK);

	/* init Tx buffer DMA addresses
    写入传输缓冲区的DMA地址到寄存器
     */
    int i;
	for (i = 0; i < NUM_TX_DESC; i++) {
        Out32(private->ioAddress + RTL8139_TX_ADDR0 + (i * 4),
            private->txBuffersDma + (private->txBuffer[i] - private->txBuffers));
        /* flush */
        In32(private->ioAddress + RTL8139_TX_ADDR0 + (i * 4));
    }

    /* 接收missed归零 */
    Out32(private->ioAddress + RTL8139_RX_MISSED, 0);

    /* 设置接收模式 */
    Rtl8139SetRxMode(private);

    /* 没有early-rx中断 */
    Out16(private->ioAddress + RTL8139_MULTI_INTR,
            In16(private->ioAddress + RTL8139_MULTI_INTR) & MULTI_INTR_CLEAR);
    
    /* 确保传输和接收都已经打开了 */
    uint8_t tmp = In8(private->ioAddress + RTL8139_COMMAND);
    if (!(tmp & RTL8139_CMD_RX) || !(tmp & RTL8139_CMD_TX))
        Out8(private->ioAddress + RTL8139_COMMAND,
                RTL8139_CMD_RX | RTL8139_CMD_TX);
    
    /* 打开已知配置好的中断 */
    Out16(private->ioAddress + RTL8139_INTR_MASK, rtl8139IntrMask);

}


PRIVATE int Rtl8139Open(struct Rtl8139Private *private)
{
    /* 注册中断 */
     /* 分配传输缓冲区 */
    private->txBuffers = (unsigned char *) kmalloc(TX_BUF_TOT_LEN, GFP_KERNEL);
    if (private->txBuffers == NULL) {
        printk("kmalloc for rtl8139 tx buffer failed!\n");
        
        return -1;
    }

    /* 分配接收缓冲区 */
    private->rxRing = (unsigned char *) kmalloc(RX_BUF_TOT_LEN, GFP_KERNEL);
    if (private->rxRing == NULL) {
        printk("kmalloc for rtl8139 rx buffer failed!\n");
        kfree(private->txBuffers);
        return -1;
    }


    /* 把虚拟地址转换成物理地址，即DMA地址 */
    private->txBuffersDma = Vir2Phy(private->txBuffers);
    private->rxRingDma = Vir2Phy(private->rxRing);

    /*for (i = 0; i < 4; i++) {
        private->txBuffers[i] = (uint8 *) kmalloc(PAGE_SIZE * 2, GFP_KERNEL);
        if (private->txBuffers[i] == NULL) {
            kfree(private->rxRing);
            printk("kmalloc for rtl8139 tx buffer failed!\n");
            return -1;
        }
    }*/


    /* 传输标志设置 */
    private->txFlags = (TX_FIFO_THRESH << 11) & 0x003f0000;

    /* 初始化传输和接收缓冲区环 */
    Rtl8139InitRing(private);
    /* 设置硬件信息 */
    rtl8139HardwareStart(private);

    /* 注册并打开中断 */
	RegisterIRQ(private->irq, Rtl8139Handler, 0, "IRQ-Network", DEVNAME, 0);
    //DisableIRQ(private->irq);
    
    printk("open done!\n");

/*
    memset(private->rxBuffer, 0, RX_BUFFER_SIZE + 1500);
    
    printk("rx buffer addr:%x -> %x\n", private->rxBuffer, Vir2Phy(private->rxBuffer));

    private->currentRX = 0;
    private->rxWriteAddr = 0;
*/
    /* 4个页长度，每个接收缓冲区4096字节 */
    //private->rxBufferLen = RX_BUFFER_SIZE;

    /* 设置接收缓冲区的起始物理地址 */
    /*Out32(private->ioAddress + RTL8139_RX_BUF_ADDR, Vir2Phy(private->rxBuffer));
    
    Out32(private->ioAddress + RTL8139_RX_BUF_PTR, private->currentRX);
    
    Out8(private->ioAddress + 0x3a, 0);
    Out8(private->ioAddress + 0x3a + 1, 0);
    */
    /* init tx buffers
    配置传送缓冲区
    */
    /*private->currentTX = 0;
    private->txSetup = 0;
    private->txFinish = 0;*/
    //private->txFreeNR = 4;
/*
    AtomicSet(&private->txFreeNR, 4);
    

    uint32_t txConfig = In32(private->ioAddress + RTL8139_TCR);
    printk("tx config :%x\n", txConfig);*/

    /* 配置传输设定 */
    //Out16(RTL8139_TCR, (0x06 << 8));    
    
    /* 设定传输的版本是rtl8139 */
    //Out8(RTL8139_TCR + 3, 0x60);
    
    /* 
     * set IMR(Interrupt Mask Register)
     * To set the RTL8139 to accept only the Transmit OK (TOK) and Receive OK (ROK) interrupts, 
     * we would have the TOK and ROK bits of the IMR high and leave the rest low. 
     * That way when a TOK or ROK IRQ happens, it actually will go through and fire up an IRQ.
     * 打开接收和传输中断
     */
   // Out16(private->ioAddress + RTL8139_INTR_MASK, RTL8139_RX_OK | RTL8139_TX_OK);

    /* 
     * configuring receive buffer(RCR)
     * Before hoping to see a packet coming to you, you should tell the RTL8139 to accept packets 
     * based on various rules. The configuration register is RCR.
     *  AB - Accept Broadcast: Accept broadcast packets sent to mac ff:ff:ff:ff:ff:ff
     *  AM - Accept Multicast: Accept multicast packets.
     *  APM - Accept Physical Match: Accept packets send to NIC's MAC address.
     *  AAP - Accept All Packets. Accept all packets (run in promiscuous mode).
     *  Another bit, the WRAP bit, controls the handling of receive buffer wrap around.
     * 配置接收的内容
     */
    /* AB + AM + APM + AAP + WRAP 
    配置接受数据的大小：
        11位-> 
        0x00 = 8kb+16byte
        0x01 = 16kb+16byte
        0x10 = 32kb+16byte
        0x11 = 64kb+16byte
    */
/*
    Out32(private->ioAddress + RTL8139_RCR, (0x00 << 13) | (0x00 << 11) | 
            (0x00 << 8) |(1 << 7) | 0x0f); 
*/

    //Spin("test");

    /* enable receive and transmitter
    开启接收和传输 */
    //Out8(private->ioAddress + RTL8139_COMMAND, 0x0c);
    return 0;
}


PUBLIC int InitRtl8139Driver()
{
    PART_START("RTL8139 Driver");

    struct Rtl8139Private *private = &rtl8139Private;

    /* 获取PCI设备 */
    if (Rtl8139GetInfoFromPCI(private)) {
        return -1;
    }

    struct PciDevice *pdev = private->pciDevice;
    
    /* 对版本进行检测 */

    /* 如果是增强的版本，输出提示信息 */
    if (pdev->vendorID  == RTL8139_VENDOR_ID &&
	    pdev->deviceID == RTL8139_DEVICE_ID && pdev->revisionID >= 0x20) {
		printk("This (id %04x:%04x rev %02x) is an enhanced 8139C+ chip, use 8139cp\n",
		        pdev->vendorID, pdev->deviceID, pdev->revisionID);
		/* 本来在这里是该返回的，return -1;
        相当于不支持，不过貌似不用增强版本代码，也可以 */
	}

    /* 初始化版本设定 */
    if (Rtl8139InitBoard(private)) {
        return -1;  /* 初始化失败 */
    }

    /* 从配置空间中读取MAC地址 */
	int i;
    for (i = 0; i < RTL8139_MAC_ADDR_LEN; i++) {
        private->macAddress[i] = In8(private->ioAddress + RTL8139_MAC + i);
    }
    
    printk("mac addr: %x:%x:%x:%x:%x:%x\n", private->macAddress[0], private->macAddress[1],
            private->macAddress[2], private->macAddress[3], 
            private->macAddress[4], private->macAddress[5]);

    private->drvFlags = 0;

    /* 初始化自旋锁 */
    SpinLockInit(&private->lock);
    SpinLockInit(&private->rxLock);

    /* Put the chip into low-power mode.
    把芯片设置成低耗模式
     */
	if (RtlChipInfo[private->chipset].flags & HAS_HLT_CLK)
		Out8(private->ioAddress + RTL8139_HLT_CLK, 'H');	/* 'R' would leave the clock running. */

    Rtl8139Open(private);

    PART_END();
    return 0;
}
