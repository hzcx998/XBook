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

#include <drivers/rtl8139.h>
#include <drivers/amd79c973.h>
#include <drivers/xxx.h>

/* ip地址，以大端字序保存，也就是网络字序 */
PRIVATE unsigned int networkIpAddress;

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
 * NetworkSetAddress - 设置IP地址
 * @ip: IP地址
 */
PUBLIC void NetworkSetAddress(unsigned int ip)
{
    networkIpAddress = ip;
}

/**
 * NetworkGetAddress - 获取IP地址
 * 
 * 返回IP地址
 */
PUBLIC unsigned int NetworkGetAddress()
{
    return networkIpAddress;
}

PUBLIC void DumpIpAddress(unsigned int ip)
{
    printk(PART_TIP "----Ip Address----\n");
    printk(PART_TIP "%d.%d.%d.%d\n", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
            (ip >> 8) & 0xff, ip & 0xff);
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

    unsigned int ipAddress;

    /* 设置本机IP地址 */
    #ifdef _NIC_RTL8139
    ipAddress = NetworkMakeIpAddress(169,254,44,140);
    //ipAddress = NetworkMakeIpAddress(10,253,117,249);
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

    NetworkSetAddress(ipAddress);

    DumpIpAddress(NetworkGetAddress());

    return 0;
}

PRIVATE void NetwrokTest()
{
    
    #ifdef _NIC_RTL8139
        unsigned char test[6] = {
            0x00,0xFF,0x44,0x98,0x96,0x37,
        };
        char *str = "hello,asdqwe111111111123123";
        while(1){
            
            //EthernetSend(test, PROTO_ARP, str, strlen(str));
            ArpRequest(NetworkMakeIpAddress(169,254,44,2));
            //ArpRequest(NetworkMakeIpAddress(169,254,221,124));
            //printk("sleep");
            SysMSleep(1000);

            //SysSleep(1);
            //printk("wakeup");
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
    while(1);

    Spin("Network Test!\n");
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

    /* 进行网络配置 */
    NetworkConfig();

    NetwrokTest();

    PART_END();
    Spin("spin in network.");
    return 0;
}