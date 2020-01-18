/*
 * file:		net/net.c
 * auther:		Jason Hu
 * time:		2019/12/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/* 明天就是元旦节了，多么幸福的日子啊！ */

#include <book/config.h>
#include <book/debug.h>
#include <book/task.h>
#include <share/string.h>

#include <net/network.h>
#include <net/netbuf.h>
#include <net/ethernet.h>
#include <net/arp.h>
#include <net/ip.h>

#include <drivers/rtl8139.h>
#include <drivers/amd79c973.h>
#include <drivers/xxx.h>
#include <drivers/clock.h>

/* ip地址，以大端字序保存，也就是网络字序 */
PRIVATE unsigned int networkIpAddress;

/* 子网掩码 */
PRIVATE uint32 networkSubnetMask;

/* 网关地址 */
PRIVATE uint32 networkGateway;

/* DNS服务器地址 */
PRIVATE uint32 networkDnsAddress;

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

    
    #ifdef _NIC_RTL8139
    EthernetSetAddress(Rtl8139GetMACAddress());
    #endif

    #ifdef _NIC_AMD79C973
        
        EthernetSetAddress(Amd79c973GetMACAddress());
        
    #endif
    
#endif

    DumpEthernetAddress(EthernetGetAddress());

    unsigned int ipAddress, gateway, subnetMask;

    /* 设置本机IP地址 */
    #ifdef _NIC_RTL8139

    // 进行桥接
    ipAddress = NetworkMakeIpAddress(192,168,43,101);

    // 网关和物理机一致
    gateway = NetworkMakeIpAddress(192,168,43,1);

    subnetMask = NetworkMakeIpAddress(255,255,255,0);
    #endif

    #ifdef _NIC_AMD79C973
        #ifdef _VM_VMWARE
        ipAddress = NetworkMakeIpAddress(192,168,70,135);
        #endif
        #ifdef _VM_VBOX
        ipAddress = NetworkMakeIpAddress(10,0,2,15);
        #endif
        #ifdef _VM_QEMU
        ipAddress = NetworkMakeIpAddress(192,168,70,135);
        #endif
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
    
    #ifdef _NIC_RTL8139
        unsigned char gateway[6] = {
            0x14,0x30,0x04,0x41,0x8d,0x1b
        };
        /* 添加网关缓存 */
        ArpAddCache(NetworkMakeIpAddress(10,253,0,1), gateway);
        
        char *str = "hello,asdqwe111111111123123";
        uint32_t ipAddr;
        
        //ipAddr = NetworkMakeIpAddress(10,0,251,18);   // 和外网沟通
        ipAddr = NetworkMakeIpAddress(192,168,43,196);  // 和物理机沟通
        //ipAddr = NetworkMakeIpAddress(10,253,0,1);    // 和网关沟通

        while(1){
            
            //EthernetSend(test, PROTO_ARP, str, strlen(str));
            //ArpRequest(ipAddr);

            IpTransmit(ipAddr, str, strlen(str), 255);

            //ArpRequest(NetworkMakeIpAddress(169,254,221,124));
            //printk("sleep");
            SysMSleep(1000);

            //SysSleep(1);
            printk("wakeup");
        }
    #endif

    #ifdef _NIC_AMD79C973
        #ifdef _VM_VMWARE
        while(1){
            ArpRequest(NetworkMakeIpAddress(192,168,70,2));
        }
        #endif
        #ifdef _VM_VBOX
        /*while(1){
        ArpRequest(NetworkMakeIpAddress(10,0,2,2));
        }*/
        #endif
        #ifdef _VM_QEMU
        /*while(1){
        ArpRequest(NetworkMakeIpAddress(192,168,70,2));
        }*/
        #endif
    #endif
    
/*
    printk("ethernet size:%d arp size:%d\n", SIZEOF_ETHERNET_HEADER, SIZEOF_ARP_HEADER);

    Spin("test");*/
    /*unsigned char broadCoast[6] = {
        0xff,0xff,0xff,0xff,0xff,0xff
    };
    
    EthernetSend(broadCoast, ntohs(PROTO_ARP), "FOO", 3);
    */
    /*
    unsigned int dstIP = NetworkMakeIpAddress(10,0,2,2);
    ArpRequest(dstIP);*/

/*
    while (1) {
        //ArpRequest(NetworkMakeIpAddress(192,168,70,2));
        ArpRequest(NetworkMakeIpAddress(192,168,1,105));
        SysSleep(1);
    }*/

    /*
    unsigned char ethAddr[ETH_ADDR_LEN] = {
        0x12, 0x34, 0x56, 0x78,0x9a,0xbe
    };*/
    //while(1) {
        
    //EthernetSend(ethAddr, PROTO_ARP, "HELLO", 6);
    
    //}
    /*unsigned char ethAddr[ETH_ADDR_LEN] = {
        0xff, 0x11, 0x33, 0x44, 0x55, 0x66
    };*/
    //ArpLookupCache(NetworkMakeIpAddress(192,168,1,1), ethAddr);
    /*while(1);

    Spin("Network Test!\n");*/
}

/**
 * InitNetworkDrivers - 初始化网卡驱动
 * 
 */
PRIVATE void InitNetworkDrivers()
{
#ifdef _NIC_RTL8139
    if (InitRtl8139Driver()) {
        printk("InitRtl8139Driver failed!\n");
    }
#endif
#ifdef _NIC_AMD79C973
    if (InitAmd79c973Driver()) {
        printk("InitAmd79c973Driver failed!\n");
    }
#endif
    /*
    if (InitXXXDriver()) {
        printk("InitXXXDriver failed!\n");
    }*/
    
    //Spin("InitNetworkDrivers Test!\n");
}

/**
 * InitNetwork - 初始化网络模块
 * 
 */
PUBLIC int InitNetwork()
{
    PART_START("Network");
    /* 初始化网卡驱动 */
    InitNetworkDrivers();
    
    /* 初始化网络缓冲区 */
    InitNetBuffer();

    /* 初始化ARP */
    InitARP();

    /* 初始化IP */
    InitNetworkIp();

    /* 进行网络配置 */
    NetworkConfig();

    NetwrokTest();

    PART_END();
    Spin("spin in network.");
    return 0;
}