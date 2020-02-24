/*
 * file:		network/core/network.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/spinlock.h>
#include <lib/string.h>
#include <net/network.h>
#include <net/netbuf.h>
#include <net/ipv4/ethernet.h>
#include <net/ipv4/arp.h>
#include <net/ipv4/ip.h>
#include <net/ipv4/icmp.h>
#include <clock/clock.h>

/* ----驱动程序导入---- */
EXTERN int InitRtl8139Driver();
EXTERN unsigned char *Rtl8139GetMACAddress();
/* ----驱动程序导入完毕---- */
/* 配置信息 */
//#define NETWORK_TEST

/* ip地址，以大端字序保存，也就是网络字序 */
PRIVATE unsigned int networkIpAddress;

/* 子网掩码 */
PRIVATE uint32 networkSubnetMask;

/* 网关地址 */
PRIVATE uint32 networkGateway;

/* DNS服务器地址 */
PRIVATE uint32 networkDnsAddress;

/* 数据包链表 */
LIST_HEAD(netwrokReceiveList);

/* 保护数据包的链表 */
Spinlock_t recvLock;

/* 网卡驱动初始化导入 */
EXTERN int InitRtl8139Driver();

/**
 * NetworkMakeIpAddress - 生成ip地址
 * @ip0: ip地址24~32位
 * @ip1: ip地址16~23位
 * @ip2: ip地址8~15位
 * @ip3: ip地址0~7位
 * 
 * 生成网络字序的ip地址
 * 
 * 返回生成后的ip地址
 */
PUBLIC unsigned int NetworkMakeIpAddress(
    unsigned char ip0,
    unsigned char ip1,
    unsigned char ip2, 
    unsigned char ip3)
{
    return (unsigned int) ((unsigned int) ip0 << 24) | \
            ((unsigned int) ip1 << 16) | ((unsigned int) ip2 << 8) | ip3;
}

/**
 * NetworkSetIpAddress - 设置IP地址
 * @ip: IP地址
 */
PUBLIC void NetworkSetIpAddress(unsigned int ip)
{
    networkIpAddress = ip;
}

/**
 * NetworkSetSubnetMask - 设置子网掩码
 * @mask: 掩码
 */
PUBLIC void NetworkSetSubnetMask(uint32_t mask)
{
    networkSubnetMask = mask;
}

/**
 * NetworkSetGateway - 设置网关
 * @gateway: 网关
 */
PUBLIC void NetworkSetGateway(uint32_t gateway)
{
    networkGateway = gateway;
}

/**
 * NetworkGetIpAddress - 获取IP地址
 * 
 * 返回IP地址
 */
PUBLIC unsigned int NetworkGetIpAddress()
{
    return networkIpAddress;
}

/**
 * NetworkGetSubnetMask - 获取子网掩码
 * 
 * 返回子网掩码
 */
PUBLIC unsigned int NetworkGetSubnetMask()
{
    return networkSubnetMask;
}

/**
 * NetworkGetGateway - 获取网关
 * 
 * 返回网关
 */
PUBLIC unsigned int NetworkGetGateway()
{
    return networkGateway;
}

PUBLIC void DumpIpAddress(unsigned int ip)
{
    //printk(PART_TIP "----Ip Address----\n");
    printk("%d.%d.%d.%d\n", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
            (ip >> 8) & 0xff, ip & 0xff);
}

/**
 * NetworkCheckSum - 计算校验和
 * 
 */
PUBLIC uint16 NetworkCheckSum(uint8_t *data, uint32_t len)
{
    uint32_t acc = 0;
    uint16_t *p = (uint16_t *) data;
    
    /* 先把所有数据都加起来 */
    int i = len;
    for (; i > 1; i -= 2, p++) {
        acc += *p;
    }

    /* 如果有漏单的，再加一次 */
    if (i == 1) {
        acc += *((uint8_t *) p);
    }

    while (acc >> 16) {
        acc = (acc & 0xffff) + (acc >> 16);
    }
    
    /* 取反 */
    return (uint16_t) ~acc;
}

/**
 * NetworkConfig - 网络配置
 * 
 * 设定网络工作必须的内容
 */
PRIVATE int NetworkConfig()
{
    /* 设置本机MAC地址 */
#ifdef _LOOPBACL_DEBUG
    unsigned char ethAddr[ETH_ADDR_LEN] = {
        0x12, 0x34, 0x56, 0xfe, 0xdc, 0xba
    };
    EthernetSetAddress(ethAddr);
#else 
    
#ifdef CONFIG_DRV_RTL8139
    EthernetSetAddress(Rtl8139GetMACAddress());
#endif /* CONFIG_DRV_RTL8139 */
#ifdef CONFIG_DRV_PCNET32
        //EthernetSetAddress(Pcnet32GetMACAddress());
#endif /* CONFIG_DRV_PCNET32 */
#endif

    DumpEthernetAddress(EthernetGetAddress());

    unsigned int ipAddress, gateway, subnetMask;

    /* 设置本机IP地址 */
#ifdef CONFIG_DRV_RTL8139

    // 设置和tapN同一网段
    ipAddress = NetworkMakeIpAddress(192,168,137,105);

    // 网关和设置为tapN的ip地址
    gateway = NetworkMakeIpAddress(192,168,137,1);
    
    subnetMask = NetworkMakeIpAddress(255,255,255,0);
#endif
    NetworkSetIpAddress(ipAddress);
    NetworkSetSubnetMask(subnetMask);
    NetworkSetGateway(gateway);

    DumpIpAddress(NetworkGetIpAddress());
    DumpIpAddress(NetworkGetGateway());
    DumpIpAddress(NetworkGetSubnetMask());

    return 0;
}

PRIVATE void NetwrokTest()
{
    
    #ifdef CONFIG_DRV_RTL8139
        unsigned char gateway[6] = {
            0x14,0x30,0x04,0x41,0x8d,0x1b
        };
        /* 添加网关缓存 */
        //ArpAddCache(NetworkMakeIpAddress(10,253,0,1), gateway);
        
        char *str = "hello,asdqwe111111111123123";
        uint32_t ipAddr;
        
        //ipAddr = NetworkMakeIpAddress(10,0,251,18);   // 和外网沟通
        //ipAddr = NetworkMakeIpAddress(192,168,0,104);  // 和物理机沟通
        ipAddr = NetworkMakeIpAddress(192,168,43,216);  // 和物理机沟通
        
        //ipAddr = NetworkMakeIpAddress(10,253,0,1);    // 和网关沟通
        int seq = 0;
        //IcmpEechoRequest(ipAddr, 0, seq, "", 0);
        
        while(1){
            seq++;
            //EthernetSend(test, PROTO_ARP, str, strlen(str));
            //ArpRequest(ipAddr);
            
            
            //IpTransmit(ipAddr, str, strlen(str), 255);
            //IcmpEechoRequest(ipAddr, 0, seq, "", 0);
            //ArpRequest(NetworkMakeIpAddress(169,254,221,124));
            //SysSleep(1);

            //SysSleep(1);
            printk("<network>\n");
        }
    #endif
}

/**
 * InitNetworkDrivers - 初始化网卡驱动
 * 
 */
PRIVATE void InitNetworkDrivers()
{
#ifdef CONFIG_DRV_RTL8139
    if (InitRtl8139Driver()) {
        printk("InitRtl8139Driver failed!\n");
    }
#endif
#ifdef CONFIG_DRV_PCNET32
    if (InitPcnet32Driver()) {
        printk("init pcnet32 driver failed!\n");
    }
#endif
}

PUBLIC int NetworkAddBuf(void *data, size_t len)
{
    ASSERT(data);
            
    NetBuffer_t *buffer;
    /* 分配一个缓冲区 */
    buffer = AllocNetBuffer(len);
    if (buffer == NULL)
        return -1;

    /* 复制数据 */
    buffer->dataLen = len;

    memcpy(buffer->data, data, len);
    //printk("add to");
    uint32_t eflags;
    eflags = SpinLockIrqSave(&recvLock);
    ListAddTail(&buffer->list, &netwrokReceiveList);
    SpinUnlockIrqSave(&recvLock, eflags);
    
    return 0;
}

/**
 * TaskNetworkIn - 网络接收线程
 * @arg: 参数
 */
PRIVATE void TaskNetworkIn(void *arg)
{
    NetBuffer_t *buffer;
    unsigned int eflags;
	while (1) {
        /* 接收列表不为空才进行处理 */
        if (!ListEmpty(&netwrokReceiveList)) {
            eflags = SpinLockIrqSave(&recvLock);

            buffer = ListFirstOwner(&netwrokReceiveList, NetBuffer_t, list);
            ASSERT(buffer);

            ListDel(&buffer->list);

            SpinUnlockIrqSave(&recvLock, eflags);
            
            /* 以太网接受数据 */
            EthernetReceive(buffer->data, buffer->dataLen);

            /* 释放缓冲区 */
            FreeNetBuffer(buffer);
        }
    }
}
/**
 * InitNetwork - 初始化网络模块
 * 
 */
PUBLIC int InitNetworkDevice()
{
#ifdef CONFIG_NET_DEVICE
    /*
    1.初始化协议栈
    2.初始化网卡
    3.初始化配置
     */
    /* 初始化网络缓冲区 */
    InitNetBuffer();

    /* 初始化ARP */
    InitARP();

    SpinLockInit(&recvLock);

    ThreadStart("netin", 3, TaskNetworkIn, NULL);
    /* 初始化网卡驱动 */
    InitNetworkDrivers();
    
    /* 初始化IP */
    InitNetworkIp();

    /* 进行网络配置 */
    NetworkConfig();
#ifdef NETWORK_TEST
    NetwrokTest();
#endif  /* NETWORK_TEST */
#endif  /* CONFIG_NET_DEVICE */    
    //Spin("spin in network.");
    return 0;
}