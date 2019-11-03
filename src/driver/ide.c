/*
 * file:		driver/ide.c
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/config.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <driver/ide.h>
#include <book/ioqueue.h>
#include <book/device.h>
#include <share/string.h>
#include <book/task.h>
#include <share/math.h>
#include <share/const.h>
#include <share/vsprintf.h>
#include <driver/clock.h>

#include <book/blk-disk.h>
#include <book/blk-dev.h>
#include <book/blk-request.h>
#include <book/blk-buffer.h>
#include <book/block.h>

/* 配置开始 */
//#define _DEBUG_IDE_INFO
//#define _DEBUG_IDE
//#define _DEBUG_TEST

/* 配置结束 */

/* IDE设备最多的磁盘数量 */
#define MAX_IDE_DISK_NR			4

/* IDE设备1个块的大小 */
#define IDE_BLOCK_SIZE			(SECTOR_SIZE * 2)

/* IDE磁盘数在BIOS阶段可以储存到这个地址，直接从里面或取就可以了 */
#define IDE_DISK_NR_ADDR		0X80000475

/*8 bit main status registers*/
#define	ATA_STATUS_BUSY		0x80	//Disk busy
#define	ATA_STATUS_READY	0x40	//Disk ready for 
#define	ATA_STATUS_DF		0x20	//Disk write default
#define	ATA_STATUS_SEEK		0x10	//Seek end
#define	ATA_STATUS_DRQ		0x08	//Request data
#define	ATA_STATUS_CORR		0x04	//Correct data
#define	ATA_STATUS_INDEX	0x02	//Receive index
#define	ATA_STATUS_ERR		0x01	//error

/* The Features/Error Port, which returns the most recent error upon read,
 has these possible bit masks */
#define ATA_ERROR_BBK      0x80    // Bad block
#define ATA_ERROR_UNC      0x40    // Uncorrectable data
#define ATA_ERROR_MC       0x20    // Media changed
#define ATA_ERROR_IDNF     0x10    // ID mark not found
#define ATA_ERROR_MCR      0x08    // Media change request
#define ATA_ERROR_ABRT     0x04    // Command aborted
#define ATA_ERROR_TK0NF    0x02    // Track 0 not found
#define ATA_ERROR_AMNF     0x01    // No address mark

/*AT disk controller command*/
#define	ATA_CMD_RESTORE				0x10	//driver retsore
#define ATA_CMD_READ_PIO          	0x20
#define ATA_CMD_READ_PIO_EXT      	0x24
#define ATA_CMD_READ_DMA          	0xC8
#define ATA_CMD_READ_DMA_EXT      	0x25
#define ATA_CMD_WRITE_PIO         	0x30
#define ATA_CMD_WRITE_PIO_EXT     	0x34
#define ATA_CMD_WRITE_DMA         	0xCA
#define ATA_CMD_WRITE_DMA_EXT     	0x35
#define ATA_CMD_CACHE_FLUSH       	0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   	0xEA
#define ATA_CMD_PACKET            	0xA0
#define ATA_CMD_IDENTIFY_PACKET   	0xA1
#define ATA_CMD_IDENTIFY          	0xEC

/* ATAPI的命令 */
#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

/* 设备类型 */
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define IDE_READ       1
#define IDE_WRITE      2

/* 主设备，从设备 */
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

/* 主通道，从通道 */
#define ATA_PRIMARY   	0x00
#define ATA_SECONDARY   0x01

/* 端口 */
#define	ATA_PRIMARY_PORT	0x1F0
#define	ATA_SECONDARY_PORT	0x170

#define ATA_REG_DATA(channel) 			(channel->base + 0)
#define ATA_REG_FEATURE(channel) 		(channel->base + 1)
#define ATA_REG_ERROR(channel) 			ATA_REG_FEATURE(channel)
#define ATA_REG_SECTOR_CNT(channel) 	(channel->base + 2)
#define ATA_REG_SECTOR_LOW(channel) 	(channel->base + 3)
#define ATA_REG_SECTOR_MID(channel) 	(channel->base + 4)
#define ATA_REG_SECTOR_HIGH(channel) 	(channel->base + 5)
#define ATA_REG_DEVICE(channel) 		(channel->base + 6)
#define ATA_REG_STATUS(channel) 		(channel->base + 7)
#define ATA_REG_CMD(channel) 			ATA_REG_STATUS(channel)

#define ATA_REG_ALT_STATUS(channel) 	(channel->base + 0x206)
#define ATA_REG_CTL(channel) 			ATA_REG_ALT_STATUS(channel)

/* 设备寄存器的位 */
#define BIT_DEV_MBS		0xA0	//bit 7 and 5 are 1

/* 生存ATA设备寄存器的值 */
#define ATA_MKDEV_REG(lbaMode, slave, head) (BIT_DEV_MBS | \
		0x40 | \
		(slave << 4) | \
		head)

/* IDE通道结构体 */
struct IdeChannel {
   	unsigned short base;  // I/O Base.
	char irqno;		 	// 本通道所用的中断号
   	struct SyncLock lock;
   	struct IdeDevice *devices;	// 通道上面的设备
	char who;		/* 通道上主磁盘在活动还是从磁盘在活动 */
	char what;		/* 执行的是什么操作 */
} channels[2];

/* IDE块设备结构体 */
PRIVATE struct IdeDevice {
	struct Disk *disk;		/* 磁盘设备 */
	struct RequestQueue *requestQueue;	/* 请求队列 */
	struct IdeChannel *channel;		/* 所在的通道 */
	struct IdeIdentifyInfo *info;	/* 磁盘信息 */
	unsigned char reserved;	// disk exist
	unsigned char drive;	// 0 (Master Drive) or 1 (Slave Drive).
	unsigned char type;		// 0: ATA, 1:ATAPI.
	unsigned char signature;// Drive Signature
	unsigned int capabilities;// Features.
	unsigned int commandSets; // Command Sets Supported.
	unsigned int size;		// Size in Sectors.
	/* 状态信息 */
	unsigned int rdSectors;	// 读取了多少扇区
	unsigned int wrSectors;	// 写入了多少扇区

}devices[MAX_IDE_DISK_NR];		/* 最多有4块磁盘 */


/* 磁盘信息结构体 */
struct IdeIdentifyInfo {
	//	0	General configuration bit-significant information
	unsigned short General_Config;

	//	1	Obsolete
	unsigned short Obsolete0;

	//	2	Specific configuration
	unsigned short Specific_Coinfig;

	//	3	Obsolete
	unsigned short Obsolete1;

	//	4-5	Retired
	unsigned short Retired0[2];

	//	6	Obsolete
	unsigned short Obsolete2;

	//	7-8	Reserved for the CompactFlash Association
	unsigned short CompactFlash[2];

	//	9	Retired
	unsigned short Retired1;

	//	10-19	Serial number (20 ASCII characters)
	unsigned short Serial_Number[10];

	//	20-21	Retired
	unsigned short Retired2[2];

	//	22	Obsolete
	unsigned short Obsolete3;

	//	23-26	Firmware revision(8 ASCII characters)
	unsigned short Firmware_Version[4];

	//	27-46	Model number (40 ASCII characters)
	unsigned short Model_Number[20];

	//	47	15:8 	80h 
	//		7:0  	00h=Reserved 
	//			01h-FFh = Maximumnumber of logical sectors that shall be transferred per DRQ data block on READ/WRITE MULTIPLE commands
	unsigned short Max_logical_transferred_per_DRQ;

	//	48	Trusted Computing feature set options
	unsigned short Trusted_Computing_feature_set_options;

	//	49	Capabilities
	unsigned short Capabilities0;

	//	50	Capabilities
	unsigned short Capabilities1;

	//	51-52	Obsolete
	unsigned short Obsolete4[2];

	//	53	15:8	Free-fall Control Sensitivity
	//		7:3 	Reserved
	//		2 	the fields reported in word 88 are valid
	//		1 	the fields reported in words (70:64) are valid
	unsigned short Report_88_70to64_valid;

	//	54-58	Obsolete
	unsigned short Obsolete5[5];

	//	59	15:9	Reserved
	//		8	Multiple sector setting is valid	
	//		7:0	xxh current setting for number of logical sectors that shall be transferred per DRQ data block on READ/WRITE Multiple commands
	unsigned short Mul_Sec_Setting_Valid;

	//	60-61	Total number of user addresssable logical sectors for 28bit CMD
	unsigned short lba28Sectors[2];

	//	62	Obsolete
	unsigned short Obsolete6;

	//	63	15:11	Reserved
	//		10:8=1 	Multiword DMA mode 210 is selected
	//		7:3 	Reserved
	//		2:0=1 	Multiword DMA mode 210 and below are supported
	unsigned short MultWord_DMA_Select;

	//	64	15:8	Reserved
	//		7:0	PIO mdoes supported
	unsigned short PIO_mode_supported;

	//	65	Minimum Multiword DMA transfer cycle time per word
	unsigned short Min_MulWord_DMA_cycle_time_per_word;

	//	66	Manufacturer`s recommended Multiword DMA transfer cycle time
	unsigned short Manufacture_Recommend_MulWord_DMA_cycle_time;

	//	67	Minimum PIO transfer cycle time without flow control
	unsigned short Min_PIO_cycle_time_Flow_Control;

	//	68	Minimum PIO transfer cycle time with IORDY flow control
	unsigned short Min_PIO_cycle_time_IOREDY_Flow_Control;

	//	69-70	Reserved
	unsigned short Reserved1[2];

	//	71-74	Reserved for the IDENTIFY PACKET DEVICE command
	unsigned short Reserved2[4];

	//	75	Queue depth
	unsigned short Queue_depth;

	//	76	Serial ATA Capabilities 
	unsigned short SATA_Capabilities;

	//	77	Reserved for Serial ATA 
	unsigned short Reserved3;

	//	78	Serial ATA features Supported 
	unsigned short SATA_features_Supported;

	//	79	Serial ATA features enabled
	unsigned short SATA_features_enabled;

	//	80	Major Version number
	unsigned short Major_Version;

	//	81	Minor version number
	unsigned short Minor_Version;

	//	82	Commands and feature sets supported
	unsigned short cmdSet0;

	//	83	Commands and feature sets supported	
	unsigned short cmdSet1;

	//	84	Commands and feature sets supported
	unsigned short Cmd_feature_sets_supported2;

	//	85	Commands and feature sets supported or enabled
	unsigned short Cmd_feature_sets_supported3;

	//	86	Commands and feature sets supported or enabled
	unsigned short Cmd_feature_sets_supported4;

	//	87	Commands and feature sets supported or enabled
	unsigned short Cmd_feature_sets_supported5;

	//	88	15 	Reserved 
	//		14:8=1 	Ultra DMA mode 6543210 is selected 
	//		7 	Reserved 
	//		6:0=1 	Ultra DMA mode 6543210 and below are suported
	unsigned short Ultra_DMA_modes;

	//	89	Time required for Normal Erase mode SECURITY ERASE UNIT command
	unsigned short Time_required_Erase_CMD;

	//	90	Time required for an Enhanced Erase mode SECURITY ERASE UNIT command
	unsigned short Time_required_Enhanced_CMD;

	//	91	Current APM level value
	unsigned short Current_APM_level_Value;

	//	92	Master Password Identifier
	unsigned short Master_Password_Identifier;

	//	93	Hardware resset result.The contents of bits (12:0) of this word shall change only during the execution of a hardware reset.
	unsigned short HardWare_Reset_Result;

	//	94	Current AAM value 
	//		15:8 	Vendor’s recommended AAM value 
	//		7:0 	Current AAM value
	unsigned short Current_AAM_value;

	//	95	Stream Minimum Request Size
	unsigned short Stream_Min_Request_Size;

	//	96	Streaming Transger Time-DMA 
	unsigned short Streaming_Transger_time_DMA;

	//	97	Streaming Access Latency-DMA and PIO
	unsigned short Streaming_Access_Latency_DMA_PIO;

	//	98-99	Streaming Performance Granularity (DWord)
	unsigned short Streaming_Performance_Granularity[2];

	//	100-103	Total Number of User Addressable Logical Sectors for 48-bit commands (QWord)
	unsigned short lba48Sectors[4];

	//	104	Streaming Transger Time-PIO
	unsigned short Streaming_Transfer_Time_PIO;

	//	105	Reserved
	unsigned short Reserved4;

	//	106	Physical Sector size/Logical Sector Size
	unsigned short Physical_Logical_Sector_Size;

	//	107	Inter-seek delay for ISO-7779 acoustic testing in microseconds
	unsigned short Inter_seek_delay;

	//	108-111	World wide name	
	unsigned short World_wide_name[4];

	//	112-115	Reserved
	unsigned short Reserved5[4];

	//	116	Reserved for TLC
	unsigned short Reserved6;

	//	117-118	Logical sector size (DWord)
	unsigned short Words_per_Logical_Sector[2];

	//	119	Commands and feature sets supported (Continued from words 84:82)
	unsigned short CMD_feature_Supported;

	//	120	Commands and feature sets supported or enabled (Continued from words 87:85)
	unsigned short CMD_feature_Supported_enabled;

	//	121-126	Reserved for expanded supported and enabled settings
	unsigned short Reserved7[6];

	//	127	Obsolete
	unsigned short Obsolete7;

	//	128	Security status
	unsigned short Security_Status;

	//	129-159	Vendor specific
	unsigned short Vendor_Specific[31];

	//	160	CFA power mode
	unsigned short CFA_Power_mode;

	//	161-167	Reserved for the CompactFlash Association
	unsigned short Reserved8[7];

	//	168	Device Nominal Form Factor
	unsigned short Dev_from_Factor;

	//	169-175	Reserved
	unsigned short Reserved9[7];

	//	176-205	Current media serial number (ATA string)
	unsigned short Current_Media_Serial_Number[30];

	//	206	SCT Command Transport
	unsigned short SCT_Cmd_Transport;

	//	207-208	Reserved for CE-ATA
	unsigned short Reserved10[2];

	//	209	Alignment of logical blocks within a physical block
	unsigned short Alignment_Logical_blocks_within_a_physical_block;

	//	210-211	Write-Read-Verify Sector Count Mode 3 (DWord)
	unsigned short Write_Read_Verify_Sector_Count_Mode_3[2];

	//	212-213	Write-Read-Verify Sector Count Mode 2 (DWord)
	unsigned short Write_Read_Verify_Sector_Count_Mode_2[2];

	//	214	NV Cache Capabilities
	unsigned short NV_Cache_Capabilities;

	//	215-216	NV Cache Size in Logical Blocks (DWord)
	unsigned short NV_Cache_Size[2];

	//	217	Nominal media rotation rate
	unsigned short Nominal_media_rotation_rate;

	//	218	Reserved
	unsigned short Reserved11;

	//	219	NV Cache Options
	unsigned short NV_Cache_Options;

	//	220	Write-Read-Verify feature set current mode
	unsigned short Write_Read_Verify_feature_set_current_mode;

	//	221	Reserved
	unsigned short Reserved12;

	//	222	Transport major version number. 
	//		0000h or ffffh = device does not report version
	unsigned short Transport_Major_Version_Number;

	//	223	Transport Minor version number
	unsigned short Transport_Minor_Version_Number;

	//	224-233	Reserved for CE-ATA
	unsigned short Reserved13[10];

	//	234	Minimum number of 512-byte data blocks per DOWNLOAD MICROCODE command for mode 03h
	unsigned short Mini_blocks_per_CMD;

	//	235	Maximum number of 512-byte data blocks per DOWNLOAD MICROCODE command for mode 03h
	unsigned short Max_blocks_per_CMD;

	//	236-254	Reserved
	unsigned short Reserved14[19];

	//	255	Integrity word
	//		15:8	Checksum
	//		7:0	Checksum Validity Indicator
	unsigned short Integrity_word;
}__attribute__((packed));

/* 扫描到的磁盘数 */
unsigned char ideDiskFound;

/**
 * IdePrintError - 打印错误
 * @dev: 设备
 * @err: 错误码
 * 
 * 根据错误码打印对应的错误
 */
PRIVATE unsigned char IdePrintError(struct IdeDevice *dev, unsigned char err)
{
   	if (err == 0)
      	return err;
	
   	printk("IDE:");
   	if (err == 1) {printk("- Device Fault\n     ");}
   	else if (err == 2) {	/* 其它错误 */
		unsigned char state = In8(ATA_REG_ERROR(dev->channel));
		if (state & ATA_ERROR_AMNF)	{printk("- No Address Mark Found\n     ");}
		if (state & ATA_ERROR_TK0NF){printk("- No Media or Media Error\n     ");}
		if (state & ATA_ERROR_ABRT)	{printk("- Command Aborted\n     ");      	}
		if (state & ATA_ERROR_MCR) 	{printk("- No Media or Media Error\n     "); }
		if (state & ATA_ERROR_IDNF) {printk("- ID mark not Found\n     ");      }
		if (state & ATA_ERROR_MC) 	{printk("- No Media or Media Error\n     "); }
		if (state & ATA_ERROR_UNC)  {printk("- Uncorrectable Data Error\n     ");}
		if (state & ATA_ERROR_BBK)  {printk("- Bad Sectors\n     "); }
	} else  if (err == 3) {	/* 读取错误 */
		printk("- Reads Nothing\n     ");
	} else if (err == 4)  {	/* 写错误 */
		printk("- Write Protected\n     ");
	} else if (err == 5)  {	/* 超时 */
		printk("- Time Out\n     ");
	}
	printk("- [%s %s]\n",
		(const char *[]){"Primary", "Secondary"}[dev->channel - channels], // Use the channel as an index into the array
		(const char *[]){"Master", "Slave"}[dev->drive] // Same as above, using the drive
		);

   return err;
}

PRIVATE void DumpIdeChannel(struct IdeChannel *channel)
{
	printk(PART_TIP "----Ide Channel----\n");

	printk(PART_TIP "self:%x base:%x irq:%d\n", channel, channel->base, channel->irqno);
}

PRIVATE void DumpIdeDevice(struct IdeDevice *dev)
{
	printk(PART_TIP "----Ide Device----\n");

	printk(PART_TIP "self:%x channel:%x drive:%d type:%s \n",
	 	dev, dev->channel, dev->drive,
		dev->type == IDE_ATA ? "ATA" : "ATAPI");

	printk(PART_TIP "capabilities:%x commandSets:%x signature:%x\n",
		dev->capabilities, dev->commandSets, dev->signature);
	
	if (dev->info->cmdSet1 & 0x0400) {
		printk(PART_TIP "Total Sector(LBA 48):");
	} else {
		printk(PART_TIP "Total Sector(LBA 28):");
	}
	printk("%d\n", dev->size);

	#ifdef _DEBUG_IDE_INFO
	
	printk(PART_TIP "Serial Number:");
	
	int i;
	for (i = 0; i < 10; i++) {
		printk("%c%c", (dev->info->Serial_Number[i] >> 8) & 0xff,
			dev->info->Serial_Number[i] & 0xff);
	}

	printk("\n" PART_TIP "Fireware Version:");
	for (i = 0; i < 4; i++) {
		printk("%c%c", (dev->info->Firmware_Version[i] >> 8) & 0xff,
			dev->info->Firmware_Version[i] & 0xff);
	}

	printk("\n" PART_TIP "Model Number:");
	for (i = 0; i < 20; i++) {
		printk("%c%c", (dev->info->Model_Number[i] >> 8) & 0xff,
			dev->info->Model_Number[i] & 0xff);
	}
	
	printk("\n" PART_TIP "LBA supported:%s ",(dev->info->Capabilities0 & 0x0200) ? "Yes" : "No");
	printk("LBA48 supported:%s\n",((dev->info->cmdSet1 & 0x0400) ? "Yes" : "No"));
	
	#endif
}

/**
 * Write2Sector - 硬盘读入count个扇区的数据到buf
 * @dev: 设备
 * @buf: 缓冲区
 * @count: 扇区数 
 */
PRIVATE void ReadFromSector(struct IdeDevice* dev, void* buf, unsigned int count) {
	unsigned int sizeInByte;
	if (count == 0) {
	/* 因为sec_cnt是8位变量,由主调函数将其赋值时,若为256则会将最高位的1丢掉变为0 */
		sizeInByte = 256 * SECTOR_SIZE;
	} else { 
		sizeInByte = count * SECTOR_SIZE; 
	}
	PortRead(ATA_REG_DATA(dev->channel), buf, sizeInByte);
}

/**
 * Write2Sector - 将buf中count扇区的数据写入硬盘
 * @dev: 设备
 * @buf: 缓冲区
 * @count: 扇区数 
 */
PRIVATE void Write2Sector(struct IdeDevice* dev, void* buf, unsigned int count)
{
   unsigned int sizeInByte;
	if (count == 0) {
	/* 因为sec_cnt是8位变量,由主调函数将其赋值时,若为256则会将最高位的1丢掉变为0 */
		sizeInByte = 256 * 512;
	} else { 
		sizeInByte = count * 512; 
	}
   	PortWrite(ATA_REG_DATA(dev->channel), buf, sizeInByte);
}

/**
 * SendCmd - 向通道channel发命令cmd 
 * @channel: 要发送命令的通道
 * @cmd: 命令
 * 
 * 执行一个命令
 */
PRIVATE void SendCmd(struct IdeChannel* channel, unsigned char cmd)
{
   	Out8(ATA_REG_CMD(channel), cmd);
}


/**
 * BusyWait - 忙等待
 * @dev: 等待的设备
 * 
 * 返回信息：
 * 0表示等待无误
 * 1表示设备故障
 * 2表示状态出错
 * 5表示超时
 */
PRIVATE unsigned char BusyWait(struct IdeDevice* dev) {
	struct IdeChannel* channel = dev->channel;
	/* 等待1秒
	1000 * 1000 纳秒
	*/
   	unsigned int timeLimit = 10 * 1000 * 1000;
   	while (--timeLimit >= 0) {
      	if (!(In8(ATA_REG_STATUS(channel)) & ATA_STATUS_BUSY)) {
			
			unsigned char state = In8(ATA_REG_STATUS(channel));
			if (state & ATA_STATUS_ERR)
				return 2; // Error.
			
			if (state & ATA_STATUS_DF)
				return 1; // Device Fault.

			/* 数据请求，等待完成 */
			if (state & ATA_STATUS_DRQ) {
				return 0;
			}
      	} else {
			/* 读取一次耗费100ns */
			In8(ATA_REG_ALT_STATUS(channel));
      	}
   	}
   	return 5;	// time out
}

/**
 * IdePolling - 轮询操作
 * @chanel: 通道
 * @advancedCheck: 高级状态检测
 */
int IdePolling(struct IdeChannel* channel, unsigned int advancedCheck)
{
	// (I) Delay 400 nanosecond for BSY to be set:
	// -------------------------------------------------
	int i;
	for(i = 0; i < 4; i++) {
		/* Reading the Alternate Status port wastes 100ns;
		 loop four times.
		*/
		In8(ATA_REG_ALT_STATUS(channel));
 	}
	
	// (II) Wait for BSY to be cleared:
	// -------------------------------------------------
	while (In8(ATA_REG_STATUS(channel)) & ATA_STATUS_BUSY); // Wait for BSY to be zero.
	
	if (advancedCheck) {
		unsigned char state = In8(ATA_REG_STATUS(channel)); // Read Status Register.

		// (III) Check For Errors:
		// -------------------------------------------------
		if (state & ATA_STATUS_ERR) {
			return 2; // Error.
		}

		// (IV) Check If Device fault:
		// -------------------------------------------------
		if (state & ATA_STATUS_DF)
			return 1; // Device Fault.
	
		// (V) Check DRQ:
		// -------------------------------------------------
		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if ((state & ATA_STATUS_DRQ) == 0)
			return 3; // DRQ should be set
	}
	
	return 0; // No Error.
}

/**
 * SelectAddressingMode - 选择寻址模式
 * @dev: 设备
 * @lba: 逻辑扇区地址
 * @mode: 传输模式（CHS和LBA模式）
 * @head: CHS的头或者LBA28的高4位
 * @io: lba地址包，又6字节组成
 * 
 * 根据lba选择寻址模式，并生成IO地址包
 */
void SelectAddressingMode(struct IdeDevice *dev,
	unsigned int lba,
	unsigned char *mode,
	unsigned char *head,
	unsigned char *io)
{
	unsigned short cyl;
   	unsigned char sect;
	if (lba >= 0x10000000 && dev->capabilities & 0x200) { // Sure Drive should support LBA in this case, or you are
								// giving a wrong LBA.
		// LBA48:
		*mode  = 2;
		io[0] = (lba & 0x000000FF) >> 0;
		io[1] = (lba & 0x0000FF00) >> 8;
		io[2] = (lba & 0x00FF0000) >> 16;
		io[3] = (lba & 0xFF000000) >> 24;
		io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
		io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
		*head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} else if (dev->capabilities & 0x200)  { // Drive supports LBA?
		// LBA28:
		*mode  = 1;
		io[0] = (lba & 0x00000FF) >> 0;
		io[1] = (lba & 0x000FF00) >> 8;
		io[2] = (lba & 0x0FF0000) >> 16;
		io[3] = 0; // These Registers are not used here.
		io[4] = 0; // These Registers are not used here.
		io[5] = 0; // These Registers are not used here.
		*head      = (lba & 0xF000000) >> 24;
	} else {
		// CHS:
		*mode  = 0;
		sect      = (lba % 63) + 1;
		cyl       = (lba + 1  - sect) / (16 * 63);
		io[0] = sect;
		io[1] = (cyl >> 0) & 0xFF;
		io[2] = (cyl >> 8) & 0xFF;
		io[3] = 0;
		io[4] = 0;
		io[5] = 0;
		*head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
	}
}

/** 
 * SelectDevice - 选择操作的磁盘
 * @dev: 设备
 * @mode: 操作模式，0表示CHS模式，非0表示LBA模式
 */
PRIVATE void SelectDevice(struct IdeDevice* dev,
	unsigned char mode, 
	unsigned char head)
{
   	Out8(ATA_REG_DEVICE(dev->channel),
	   ATA_MKDEV_REG(mode == 0 ? 0 : 1, dev->drive, head));

	dev->channel->who = dev->drive;	/* 通道上当前的磁盘 */
}

/**
 * SelectSector - 选择扇区数，扇区位置
 * @dev: 设备
 * @mode: 传输模式（CHS和LBA模式）
 * @lbaIO: lba地址包，又6字节组成
 * @count: 扇区数
 * 
 * 向硬盘控制器写入起始扇区地址及要读写的扇区数
 */
PRIVATE void SelectSector(struct IdeDevice* dev,
	unsigned char mode,
	unsigned char *lbaIO,
	sector_t count)
{
  	struct IdeChannel* channel = dev->channel;

	/* 如果是LBA48就要写入24高端字节 */
	if (mode == 2) {
		Out8(ATA_REG_FEATURE(channel), 0); // PIO mode.

		/* 写入要读写的扇区数*/
		Out8(ATA_REG_SECTOR_CNT(channel), 0);

		/* 写入lba地址24~47位(即扇区号) */
		Out8(ATA_REG_SECTOR_LOW(channel), lbaIO[3]);
		Out8(ATA_REG_SECTOR_MID(channel), lbaIO[4]);
		Out8(ATA_REG_SECTOR_HIGH(channel), lbaIO[5]);
	}

	Out8(ATA_REG_FEATURE(channel), 0); // PIO mode.

	/* 写入要读写的扇区数*/
	Out8(ATA_REG_SECTOR_CNT(channel), count);

	/* 写入lba地址0~23位(即扇区号) */
	Out8(ATA_REG_SECTOR_LOW(channel), lbaIO[0]);
	Out8(ATA_REG_SECTOR_MID(channel), lbaIO[1]);
	Out8(ATA_REG_SECTOR_HIGH(channel), lbaIO[2]);
}

/**
 * SelectCmd - 选择要执行的命令
 * @rw: 传输方向（读，写）
 * @mode: 传输模式（CHS和LBA模式）
 * @dma: 直接内存传输
 * @cmd: 保存要执行的命令
 * 
 * 根据模式，dma格式还有传输方向选择一个命令
 */
PRIVATE void SelectCmd(unsigned char rw,
	unsigned char mode,
	unsigned char dma,
	unsigned char *cmd)
{
	// Routine that is followed:
	// If ( DMA & LBA48)   DO_DMA_EXT;
	// If ( DMA & LBA28)   DO_DMA_LBA;
	// If ( DMA & LBA28)   DO_DMA_CHS;
	// If (!DMA & LBA48)   DO_PIO_EXT;
	// If (!DMA & LBA28)   DO_PIO_LBA;
	// If (!DMA & !LBA#)   DO_PIO_CHS;
	if (mode == 0 && dma == 0 && rw == IDE_READ) *cmd = ATA_CMD_READ_PIO;
	if (mode == 1 && dma == 0 && rw == IDE_READ) *cmd = ATA_CMD_READ_PIO;   
	if (mode == 2 && dma == 0 && rw == IDE_READ) *cmd = ATA_CMD_READ_PIO_EXT;   
	if (mode == 0 && dma == 1 && rw == IDE_READ) *cmd = ATA_CMD_READ_DMA;
	if (mode == 1 && dma == 1 && rw == IDE_READ) *cmd = ATA_CMD_READ_DMA;
	if (mode == 2 && dma == 1 && rw == IDE_READ) *cmd = ATA_CMD_READ_DMA_EXT;
	if (mode == 0 && dma == 0 && rw == IDE_WRITE) *cmd = ATA_CMD_WRITE_PIO;
	if (mode == 1 && dma == 0 && rw == IDE_WRITE) *cmd = ATA_CMD_WRITE_PIO;
	if (mode == 2 && dma == 0 && rw == IDE_WRITE) *cmd = ATA_CMD_WRITE_PIO_EXT;
	if (mode == 0 && dma == 1 && rw == IDE_WRITE) *cmd = ATA_CMD_WRITE_DMA;
	if (mode == 1 && dma == 1 && rw == IDE_WRITE) *cmd = ATA_CMD_WRITE_DMA;
	if (mode == 2 && dma == 1 && rw == IDE_WRITE) *cmd = ATA_CMD_WRITE_DMA_EXT;

}

/**
 * ResetDriver - 重置驱动
 * @channel: 通道
 * 
 */
PRIVATE void SoftResetDriver(struct IdeChannel *channel)
{
	char ctrl = In8(ATA_REG_CTL(channel));
	//printk("ctrl = %x\n", ctrl);
	
	/* 写入控制寄存器，执行重置，会重置ata通道上的2个磁盘
	把SRST（位2）置1，就可以软重置
	 */
	Out8(ATA_REG_CTL(channel), ctrl | (1 << 2));

	/* 等待重置 */
	int i;
	/* 每次读取用100ns，读取50次，就花费5000ns，也就是5us */
	for(i = 0; i < 50; i++) {
		/* Reading the Alternate Status port wastes 100ns;
		 loop four times.
		*/
		In8(ATA_REG_ALT_STATUS(channel));
 	}
	
	/* 重置完后，需要清除该位，还原状态就行 */
	Out8(ATA_REG_CTL(channel), ctrl);
}

/**
 * ResetDriver - 重置驱动器
 * @dev: 设备
 */
PRIVATE void ResetDriver(struct IdeDevice *dev)
{

	SoftResetDriver(dev->channel);
}
/**
 * PioDataTransfer - PIO数据传输
 * @dev: 设备
 * @rw: 传输方向（读，写）
 * @mode: 传输模式（CHS和LBA模式）
 * @buf: 扇区缓冲
 * @count: 扇区数
 * 
 * 传输成功返回0，失败返回非0
 */
PRIVATE int PioDataTransfer(struct IdeDevice *dev,
	unsigned char rw,
	unsigned char mode,
	unsigned char *buf,
	unsigned short count)
{
	short i;
	unsigned char error;
	if (rw == IDE_READ) {	
		#ifdef _DEBUG_IDE
			printk("PIO read->");
		#endif
		for (i = 0; i < count; i++) {
			/* 醒来后开始执行下面代码*/
			if ((error = BusyWait(dev))) {     //  若失败
				/* 重置磁盘驱动并返回 */
				ResetDriver(dev);
				return error;
			}
			ReadFromSector(dev, buf, 1);
			buf += SECTOR_SIZE;
		}
	} else {
		#ifdef _DEBUG_IDE
			printk("PIO write->");
		#endif
		for (i = 0; i < count; i++) {
			/* 等待硬盘控制器请求数据 */
			if ((error = BusyWait(dev))) {     //  若失败
				/* 重置磁盘驱动并返回 */
				ResetDriver(dev);
				return error;
			}
			/* 把数据写入端口，完成1个扇区后会产生一次中断 */
			Write2Sector(dev, buf, 1);
			buf += SECTOR_SIZE;
		}
		/* 刷新写缓冲区 */
		Out8(ATA_REG_CMD(dev->channel), mode > 1 ?
			ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);
		IdePolling(dev->channel, 0);
	}
	return 0;
}

/**
 * AtaTypeTransfer - ATA类型数据传输
 * @dev: 设备
 * @rw: 传输方向（读，写）
 * @lba: 逻辑扇区地址
 * @count: 扇区数
 * @buf: 扇区缓冲
 * 
 * 传输成功返回0，失败返回非0
 */
PRIVATE int AtaTypeTransfer(struct IdeDevice *dev,
	unsigned char rw,
	unsigned int lba,
	unsigned int count,
	void *buf)
{
	unsigned char mode;	/* 0: CHS, 1:LBA28, 2: LBA48 */
	unsigned char dma; /* 0: No DMA, 1: DMA */
	unsigned char cmd;	
	unsigned char *_buf = (unsigned char *)buf;

	unsigned char lbaIO[6];	/* 由于最大是48位，所以这里数组的长度为6 */

	struct IdeChannel *channel = dev->channel;

   	unsigned char head, err;

	/* 要去操作的扇区数 */
	unsigned int todo;
	/* 已经完成的扇区数 */
	unsigned int done = 0;
	
	SyncLockAcquire(&channel->lock);

	/* 保存读写操作 */
	channel->what = rw;
	
	while (done < count) {
		/* 获取要去操作的扇区数
		由于一次最大只能操作256个扇区，这里用256作为分界
		 */
		if ((done + 256) <= count) {
			todo = 256;
		} else {
			todo = count - done;
		}

		/* 选择传输模式（PIO或DMA） */
		dma = 0; // We don't support DMA

		/* 选择寻址模式 */
		// (I) Select one from LBA28, LBA48 or CHS;
		SelectAddressingMode(dev, lba + done, &mode, &head, lbaIO);

		/* 等待驱动不繁忙 */
		// (III) Wait if the drive is busy;
		while (In8(ATA_REG_STATUS(channel)) & ATA_STATUS_BUSY) CpuNop();// Wait if busy.
		/* 从控制器中选择设备 */
		SelectDevice(dev, mode, head);

		/* 填写参数，扇区和扇区数 */
		SelectSector(dev, mode, lbaIO, count);

		/* 等待磁盘控制器处于准备状态 */
		while (!(In8(ATA_REG_STATUS(channel)) & ATA_STATUS_READY)) CpuNop();

		/* 选择并发送命令 */
		SelectCmd(rw, mode, dma, &cmd);

		#ifdef _DEBUG_IDE	
			printk("lba mode %d num %d io %d %d %d %d %d %d->",
				mode, lba, lbaIO[0], lbaIO[1], lbaIO[2], lbaIO[3], lbaIO[4], lbaIO[5]);
			printk("rw %d dma %d cmd %x head %d\n",
				rw, dma, cmd, head);
		#endif
		/* 等待磁盘控制器处于准备状态 */
		while (!(In8(ATA_REG_STATUS(channel)) & ATA_STATUS_READY)) CpuNop();

		/* 发送命令 */
		SendCmd(channel, cmd);

		/* 根据不同的模式传输数据 */
		if (dma) {	/* DMA模式 */
			if (rw == IDE_READ) {
				// DMA Read.
			} else {
				// DMA Write.
			}
		} else {
			/* PIO模式数据传输 */
			if ((err = PioDataTransfer(dev, rw, mode, _buf, todo))) {
				return err;
			}
			_buf += todo * SECTOR_SIZE;
			done += todo;
		}
	}

	SyncLockRelease(&channel->lock);
	return 0;
}

/**
 * IdeReadSector - 读扇区
 * @dev: 设备
 * @lba: 逻辑扇区地址
 * @count: 扇区数
 * @buf: 扇区缓冲
 * 
 * 数据读取磁盘，成功返回0，失败返回-1
 */
PRIVATE int IdeReadSector(struct IdeDevice *dev,
	unsigned int lba,
	void *buf,
	unsigned int count)
{
	unsigned char error;
	/* 检查设备是否正确 */
	if (dev < devices || dev >= &devices[3] || dev->reserved == 0) {
		return -1;
	} else if (lba + count > dev->size && dev->type == IDE_ATA) {
		return -1;
	} else {

		/* 进行磁盘访问 */

		/*如果类型是ATA*/
			error = AtaTypeTransfer(dev, IDE_READ, lba, count, buf);
		/*如果类型是ATAPI*/
		
		/* 打印驱动错误信息 */
		if(IdePrintError(dev, error)) {
			return -1;
		}
	}

	return 0;
}

/**
 * IdeWriteSector - 写扇区
 * @dev: 设备
 * @lba: 逻辑扇区地址
 * @count: 扇区数
 * @buf: 扇区缓冲
 * 
 * 把数据写入磁盘，成功返回0，失败返回-1
 */
PRIVATE int IdeWriteSector(struct IdeDevice *dev,
	unsigned int lba,
	void *buf,
	unsigned int count)
{
	unsigned char error;
	/* 检查设备是否正确 */
	if (dev < devices || dev >= &devices[3] || dev->reserved == 0) {
		return -1;
	} else if (lba + count > dev->size && dev->type == IDE_ATA) {
		return -1;
	} else {
		/* 进行磁盘访问，如果出错，就 */
		
		/*如果类型是ATA*/
			error = AtaTypeTransfer(dev, IDE_WRITE, lba, count, buf);
		/*如果类型是ATAPI*/

		/* 打印驱动错误信息 */
		if(IdePrintError(dev, error)) {
			return -1;
		}
	}
	return 0;
}

/**
 * IdeCleanDisk - 清空磁盘的数据
 * @dev: 设备
 * @count: 清空的扇区数
 * 
 * 当count为0时，表示清空整个磁盘，不为0时就清空对于的扇区数
 */
PRIVATE int IdeCleanDisk(struct IdeDevice *dev, sector_t count)
{
	if (count == 0)
		count = dev->size;

	sector_t todo, done = 0;

	/* 每次写入10个扇区 */
	char *buffer = kmalloc(SECTOR_SIZE *10, GFP_KERNEL);
	if (!buffer) {
		printk("kmalloc for ide buf failed!\n");
		return -1;
	}

	memset(buffer, 0, SECTOR_SIZE *10);

	printk(PART_TIP "IDE clean: count%d\n", count);
	while (done < count) {
		/* 获取要去操作的扇区数这里用10作为分界 */
		if ((done + 10) <= count) {
			todo = 10;
		} else {
			todo = count - done;
		}
		//printk("ide clean: done %d todo %d\n", done, todo);
		IdeWriteSector(dev, done, buffer, todo);
		done += 10;
	}
	return 0;
}

/**
 * DoBlockRequest - 执行请求
 * @q: 请求队列
 */
PRIVATE void DoBlockRequest(struct RequestQueue *q)
{
	struct Request *rq;
	struct IdeDevice *dev = q->queuedata;

	rq = BlockFetchRequest(q);
	while (rq != NULL)
	{
		#ifdef _DEBUG_IDE
		printk("dev %s: ", rq->disk->diskName);
		printk("queue %s waiter %s cmd %s disk lba %d\n",
			q->requestList == &q->upRequestList ? "up" : "down",
			rq->waiter->name, rq->cmd == BLOCK_READ ? "read" : "write", rq->lba);
		#endif

		if (rq->cmd == BLOCK_READ) {
			IdeReadSector(dev, rq->lba, rq->buffer, rq->count);
		} else {
			IdeWriteSector(dev, rq->lba, rq->buffer, rq->count);
		}

		/* 结束当前请求 */
		BlockEndRequest(rq, 0);

		/* 获取一个新情求 */
		rq = BlockFetchRequest(q);
	}
}

/**
 * 编写设备操作接口
 */

/**
 * KeyboardIoctl - 键盘的IO控制
 * @device: 设备项
 * @cmd: 命令
 * @arg1: 参数1
 * @arg2: 参数2
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int IdeIoctl(struct Device *device, int cmd, int arg)
{
	int retval = 0;

	/* 获取IDE设备 */
	struct BlockDevice *blkdev = (struct BlockDevice *)device;
	struct IdeDevice *dev = (struct IdeDevice *)blkdev->private;
	
	DumpIdeDevice(dev);

	switch (cmd)
	{
	case IDE_IO_CLEAN:	/* 执行清空磁盘命令 */
		if (IdeCleanDisk(dev, arg)) {
			retval = -1;
		}
		break;
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

/**
 * IdeRead - 读取数据
 * @device: 设备
 * @lba: 逻辑扇区地址
 * @buffer: 缓冲区
 * @count: 扇区数
 * 
 * @return: 成功返回0，失败返回-1
 */
PRIVATE int IdeRead(struct Device *device, unsigned int lba, void *buffer, unsigned int count)
{
	/* 获取IDE设备 */
	struct BlockDevice *blkdev = (struct BlockDevice *)device;
	struct IdeDevice *dev = (struct IdeDevice *)blkdev->private;
	
	return IdeReadSector(dev, lba, buffer, count);
}

/**
 * IdeWrite - 写入数据
 * @device: 设备
 * @lba: 逻辑扇区地址
 * @buffer: 缓冲区
 * @count: 扇区数
 * 
 * @return: 成功返回0，失败返回-1
 */
PRIVATE int IdeWrite(struct Device *device, unsigned int lba, void *buffer, unsigned int count)
{
	/* 获取IDE设备 */
	struct BlockDevice *blkdev = (struct BlockDevice *)device;
	struct IdeDevice *dev = (struct IdeDevice *)blkdev->private;
	
	return IdeWriteSector(dev, lba, buffer, count);
}

/**
 * 块设备操作的结构体
 */
PRIVATE struct DeviceOperations opSets = {
	.ioctl = IdeIoctl,
	.read = IdeRead,
	.write = IdeWrite,
};

/**
 * IdeBlockDeviceCreate - 创建块设备
 * @dev: 要创建的设备
 * @major: 主设备号
 * @diskIdx: 磁盘的索引
 */
PRIVATE int IdeBlockDeviceCreate(struct IdeDevice *dev, int major, int idx)
{
	/* 创建最大的磁盘数 */
	dev->disk = AllocDisk(IDE_PARTS);
	if (dev->disk == NULL)
		return -1;
	
	/* 初始化磁盘请求 */
	dev->requestQueue = BlockInitQueue(DoBlockRequest, dev);
	if (dev->requestQueue == NULL) {
		return -1;
	}
	
	/* 设置磁盘相关 */
	char name[DISK_NAME_LEN];
	char devname;
	/* 选择磁盘名 */
	switch (idx)
	{
	case 0:
		devname = 'a';
		break;
	case 1:
		devname = 'b';
		break;
	case 2:
		devname = 'c';
		break;
	case 3:
		devname = 'd';
		break;
	default:
		break;
	}

	sprintf(name, "hd%c", devname);
	/* 这里的firstMajor为什么+1？因为磁盘和分区是分开的，都要用块设备来描述 */
	DiskIdentity(dev->disk, name, major, IDE_MINOR(idx));
	DiskBind(dev->disk, dev->requestQueue, dev);
	SetCapacity(dev->disk, dev->size);
	
	AddDisk(dev->disk);
	#ifdef _DEBUG_IDE_INFO
	DumpDisk(dev->disk);
	#endif

	/* 添加一个Disk块设备 */
	struct BlockDevice *blkdev = AllocBlockDevice(MKDEV(major, dev->disk->firstMinor));
	if (blkdev == NULL) {
		return -1;
	}
	BlockDeviceInit(blkdev, dev->disk, -1, IDE_BLOCK_SIZE, dev);
	BlockDeviceSetup(blkdev, &opSets);
	/* 名字和磁盘名字一样 */
	BlockDeviceSetName(blkdev, name);
	
	AddBlockDevice(blkdev);

	#ifdef _DEBUG_IDE_INFO
	DumpBlockDevice(blkdev);
	#endif

	/* 添加一个分区块设备 */
	int i;
	for (i = 0; i < 1; i++) {
		// + 1 是因为在此之前为disk设置了一个block device
		blkdev = AllocBlockDevice(MKDEV(major, dev->disk->firstMinor + i + 1));
		if (blkdev == NULL) {
			return -1;
		}
		// 添加分区
		AddPartition(dev->disk, i, i * (dev->size / 2), dev->size / 2);
		
		BlockDeviceInit(blkdev, dev->disk, i, IDE_BLOCK_SIZE, dev);
		BlockDeviceSetup(blkdev, &opSets);

		memset(name, 0, DEVICE_NAME_LEN);
		sprintf(name, "hd%c%d", devname, i);
		/* 名字和磁盘名字一样，名字是磁盘名和编号，例如hda0, hdb1这种 */
		BlockDeviceSetName(blkdev, name);
	
		AddBlockDevice(blkdev);
	}
	return 0;
}

/**
 * IdeDelDevice - 删除一个块设备
 * @dev: 要删除的块设备
 */
PRIVATE void IdeDelDevice(struct IdeDevice *dev)
{
	if (dev->disk)
		DelDisk(dev->disk);
	if (dev->requestQueue)
		BlockCleanUpQueue(dev->requestQueue);
}

/* 硬盘的任务协助 */
PRIVATE struct TaskAssist ideAssist;

/**
 * IdeAssistHandler - 任务协助处理函数
 * @data: 传递的数据
 */
PRIVATE void IdeAssistHandler(unsigned int data)
{
	struct IdeChannel *channel = (struct IdeChannel *)data;

	struct IdeDevice *dev = channel->devices + channel->who;

	/* 获取状态，做出错判断 */
	if (In8(ATA_REG_STATUS(channel)) & ATA_STATUS_ERR) {
		/* 尝试重置驱动 */
		ResetDriver(dev);
	}

	/* 可以在这里面做一些统计信息 */
	if (channel->what == IDE_READ) {
		dev->rdSectors++;
	} else if (channel->what == IDE_WRITE) {
		dev->wrSectors++;
	} 
}

/**
 * IdeHandler - IDE硬盘中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void IdeHandler(unsigned int irq, unsigned int data)
{
	ideAssist.data = data;
	/* 调度协助 */
	TaskAssistSchedule(&ideAssist);
}

/**
 * IdeProbe - 探测设备
 * @diskFound: 找到的磁盘数
 * 
 * 根据磁盘数初始化对应的磁盘的信息
 */
PRIVATE void IdeProbe(char diskFound)
{
	
	/* 初始化，并获取信息 */
	int channelCnt = DIV_ROUND_UP(diskFound, 2);	   // 一个ide通道上有两个硬盘,根据硬盘数量反推有几个ide通道
	struct IdeChannel* channel;
	int channelno = 0, devno = 0; 
	char err;

	int leftDisk = diskFound;

   	/* 处理每个通道上的硬盘 */
   	while (channelno < channelCnt) {
      	channel = &channels[channelno];
		/* 为每个ide通道初始化端口基址及中断向量 */
		switch (channelno) {
		case 0:
			channel->base	 = ATA_PRIMARY_PORT;	   // ide0通道的起始端口号是0x1f0
			channel->irqno	 = IRQ14;	   // 从片8259a上倒数第二的中断引脚,温盘,也就是ide0通道的的中断向量号
			break;
		case 1:
			channel->base	 = ATA_SECONDARY_PORT;	   // ide1通道的起始端口号是0x170
			channel->irqno	 = IRQ15;	   // 从8259A上的最后一个中断引脚,我们用来响应ide1通道上的硬盘中断
			break;
		}
		channel->who = 0;	// 初始化为0
		channel->what = 0;
		SyncLockInit(&channel->lock);	

		/* 初始化任务协助 */
		TaskAssistInit(&ideAssist, IdeAssistHandler, 0);

		/* 注册中断 */
		RegisterIRQ(channel->irqno, IdeHandler, IRQF_DISABLED, "ATA", "BLOCK", (unsigned int)channel);
		#ifdef _DEBUG_IDE_INFO	
		DumpIdeChannel(channel);
		#endif

		/* 分别获取两个硬盘的参数及分区信息 */
		while (devno < 2 && leftDisk) {
			/* 选择一个设备 */
			if (channelno == ATA_SECONDARY) {
				channel->devices = &devices[2];
			} else {
				channel->devices = &devices[0];
			}
			
			/* 填写设备信息 */
			struct IdeDevice* dev = &channel->devices[devno];
			dev->channel = channel;
			dev->drive = devno;
			dev->type = IDE_ATA;

			dev->info = kmalloc(SECTOR_SIZE, GFP_KERNEL);
			if (dev->info == NULL) {
				printk("kmalloc for ide device info failed!\n");
				continue;
			}

			/* 获取磁盘的磁盘信息 */
			SelectDevice(dev, 0, 0);
			
			//等待硬盘准备好
			while (!(In8(ATA_REG_STATUS(channel)) & ATA_STATUS_READY)) CpuNop();

			SendCmd(channel, ATA_CMD_IDENTIFY);

			err = IdePolling(channel, 1);
			if (err) {
				IdePrintError(dev, err);
				continue;
			}
			
			/* 读取到缓冲区中 */
			ReadFromSector(dev, dev->info, 1);

			/* 根据信息设定一些基本数据 */
			dev->commandSets = (int)((dev->info->cmdSet1 << 16) + dev->info->cmdSet0);

			/* 根据模式设置读取扇区大小 */
			if (dev->commandSets & (1 << 26)) {
				dev->size = ((int)dev->info->lba48Sectors[1] << 16) + \
					(int)dev->info->lba48Sectors[0];
			} else {
				dev->size = ((int)dev->info->lba28Sectors[1] << 16) + 
					(int)dev->info->lba28Sectors[0];
			}

			dev->capabilities = dev->info->Capabilities0;
			dev->signature = dev->info->General_Config;
			dev->reserved = 1;	/* 设备存在 */
			#ifdef _DEBUG_IDE_INFO
			DumpIdeDevice(dev);
			#endif
			devno++; 
			leftDisk--;
		}
		devno = 0;			  	   // 将硬盘驱动器号置0,为下一个channel的两个硬盘初始化。
		channelno++;				   // 下一个channel
	}
}

/**
 * IdeTest - 测试代码
 */
PRIVATE void IdeTest()
{
	#ifdef _DEBUG_TEST

	//IdeCleanDisk(&devices[0], 0);

	printk("test start!\n");
	char *buf = kmalloc(SECTOR_SIZE*10, GFP_KERNEL);
	if (!buf)
		Panic("error!");

	int read = BlockRead(DEV_HDA, 0, buf);
	if (read == 0)
		printk("block read error!\n");

	printk("%x-%x-%x-%x\n",
	 buf[0], buf[511], buf[512],buf[512*2-1]);

	memset(buf, 0x5a, read);

	int write = BlockWrite(DEV_HDA, 0, buf, 1);
	if (write == 0)
		printk("block write error!\n");
	
	IdeReadSector(&devices[0],0,buf, 5);

	printk("%x-%x-%x-%x-%x-%x\n",
	 buf[0], buf[511], buf[512+511],buf[512*2], buf[4096],buf[10*512-1]);
	/*
	IdeReadSector(&devices[0],2,5,buf);

	printk("%x-%x-%x-%x-%x-%x\n",
	 buf[0], buf[511], buf[512+511],buf[512*2], buf[4096],buf[10*512-1]);
	*/
	memset(buf,0xfa,SECTOR_SIZE * 5);
	
	IdeWriteSector(&devices[0],10,buf,5);
	
	//Spin("test end");

	memset(buf,1,SECTOR_SIZE * 5);
	
	IdeReadSector(&devices[0],10,buf,5);

	printk("%x-%x-%x-%x-%x-%x\n",
	 buf[0], buf[511], buf[512+511],buf[512*2], buf[4096],buf[10*512-1]);

	printk("read %d write %d\n", devices[0].rdSectors, devices[0].wrSectors);
	
	#endif
}


/**
 * InitIdeDriver - 初始化IDE硬盘驱动
 */
PUBLIC int InitIdeDriver()
{
	PART_START("Ide Driver");

	/* 获取已经配置了的硬盘数量
	这种方法的设备检测需要磁盘安装顺序不能有错误，
	可以用轮训的方式来改变这种情况。 
	 */
	ideDiskFound = *((unsigned char *)IDE_DISK_NR_ADDR);
	printk(PART_TIP "Ide disks are %d\n", ideDiskFound);
	/* 有磁盘才初始化磁盘 */
	if (ideDiskFound > 0) {
		/* 驱动本身的初始化 */
		IdeProbe(ideDiskFound);

		int status;
		/* 块设备的初始化 */
		int i;
		for (i = 0; i < ideDiskFound; i++) {
			status = IdeBlockDeviceCreate(&devices[i], IDE_MAJOR, i);
			if (status < 0) {
				return status;
			}
		}
		/* 执行测试 */
		IdeTest();
		//ResetDriver(&devices[0]);
		//Spin("");
		
	}
    PART_END();
	return 0;
}

/**
 * ExitIdeDriver - 退出驱动
 */
PUBLIC void ExitIdeDriver()
{
	/* 删除块设备信息 */
	int i, j;
	for (i = 0; i < ideDiskFound; i++) {
		IdeDelDevice(&devices[i]);
	}
	
	/* 删除磁盘设备信息 */
	int channelCnt = DIV_ROUND_UP(ideDiskFound, 2);	   // 一个ide通道上有两个硬盘,根据硬盘数量反推有几个ide通道
	int diskFound = ideDiskFound;
	struct IdeChannel *channel;
	for (i = 0; i < channelCnt; i++) {
		channel = &channels[i];

		/* 注销中断 */
		UnregisterIRQ(channel->irqno, (unsigned int)channel);

		j = 0;
		/* 释放分配的资源 */
		while (j < 2 && diskFound > 0) {
			kfree(channel->devices[j].info);
			diskFound--;
		}
	}
}
