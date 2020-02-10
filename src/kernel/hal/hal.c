/*
 * file:		   kernel/hal.c
 * auther:	   Jason Hu
 * time:		   2019/6/22
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/hal.h>
#include <book/debug.h>
#include <book/list.h>
#include <book/interrupt.h>
#include <lib/vsprintf.h>
#include <lib/string.h>
#include <char/uart/serial.h>

//创建全局变量 halListHead 来管理所有的hal
LIST_HEAD(halListHead);

/* ---------------------
 * 需要把hal的宿主导入此文件
 * ---------------------
 */
HAL_EXTERN(halOfCpu);
HAL_EXTERN(halOfRam);
//HAL_EXTERN(halOfKeyboard);

/* 上一次搜索的hal，用来当做缓存 */ 
struct Hal *lastSearchedHal;

/*
 * InitHal - 初始化hal环境
 */
PUBLIC void InitHalEnvironment()
{
    // 初始化hal链表头
    INIT_LIST_HEAD(&halListHead);
   
    lastSearchedHal = NULL;

    // 初始化控制台
	InitConsoleDebugDriver();
#ifdef CONFIG_SERIAL_DEBUG
    // 初始化串口驱动
    InitSerialDebugDriver();
#endif /* CONFIG_SERIAL_DEBUG */
    /* 初始化调试输出，可以使用printk */
    InitDebugPrint();

    /* ----从此开始调试显示信息之旅---- */

    if (HalCreate(&halOfCpu)) {
        Panic("Register %s name samed!\n", halOfCpu.halName);
    }
    if (HalCreate(&halOfRam)) {
        Panic("Register %s name samed!\n", halOfRam.halName);
    }
}

/*
 * HalNameToHal - 通过名字找到hal指针
 * @name: 要获取的hal的名字
 * 
 * 通过hal名字可以获取到hal地址，需要对hal进行操作。
 */
PUBLIC struct Hal *HalNameToHal(char *name)
{
   struct Hal *p;
   //循环查找，是否有同名的hal
   /*
   这是一种野蛮方法，从前往后，每次都要进行这种方式查找。
   如果数量太多，可能导致效率下降，所以找到一种可以提升的办法，
   用一个缓存，来表示上次查找到的hal，如果这次的缓存和要朝招的一样，
   那么就直接返回，就不会从头开始查找。这种方法在使用连续同一个hal的
   时候就显得十分高效率。
   */

   /* 如果存在缓存，那么就先比较缓存 */
   if (lastSearchedHal)
   {
      /* 如果名字一样就直接返回缓存中的hal */
      if (!strcmp(lastSearchedHal->halName, name))
         return lastSearchedHal;
   }
   
   /* 缓存中没有才进行查找 */
   ListForEachOwner(p, &halListHead, halList) {
      //如果相同就返回
      if (!strcmp(p->halName, name)) {
         /* 写入到缓存中 */
         lastSearchedHal = p;
         return p;
      }
   }

   /* 没找到就清空缓存 */
   lastSearchedHal = NULL;
   //没有找到
   return NULL;
}

/*
 * HalInit - 初始化一个hal
 * @hal: 要初始化的hal
 * 
 * 调用初始化hal的函数
 */
PRIVATE void HalInit(struct Hal *hal)
{
   hal->isOpened = 0;
   hal->halOperate->Init();
}

/*
 * HalCreate - 创建hal设备
 * @hal: 已经准备好的hal设备的hal
 * 
 * 创建一个hal设备，然后后面就可以通过hal设备的名字进行访问
 */
PUBLIC int HalCreate(struct Hal *hal)
{
   /* 1.检测名字是否已经被使用 */
   
   // 判断是否为空，为空就不检测
   if (!ListEmpty(&halListHead)) {
      struct Hal *p;
      //循环查找，是否有同名的hal
      ListForEachOwner(p, &halListHead, halList) {
         //如果相同就返回-1
         if (!strcmp(p->halName, hal->halName)) {
            return -1;
         }
      }
   }
   
   /* 2.注册到hal系统之中 */
   
   //先初始化链表
   INIT_LIST_HEAD(&hal->halList);

   //再把链表添加到hal系统中
   ListAddTail(&hal->halList, &halListHead);

   /* 3.初始化hal */
   HalInit(hal);

   return 0;
}

/*
 * HalDestory - 销毁一个已经创建的硬件抽象
 */
PUBLIC int HalDestory(char *name)
{
   struct Hal *hal = HalNameToHal(name);
   //如果没有找到就返回
   if (hal == NULL) {
      return -1;
   }
   //还在使用中是不允许关闭的，必须关闭后才能摧毁
   if (hal->isOpened > 0) {
      return -1;
   }

   //再把链表从到hal系统中删除
   ListDel(&hal->halList);
   
   if (hal->halOperate->Destruct != NULL) {
      hal->halOperate->Destruct();
   }
   
   return 0;
}
/*
 * HalOpen - 打开hal设备
 * @name: 要操作的hal
 */
PUBLIC void HalOpen(char *name)
{
   //printk("%s ", name);
   
   struct Hal *hal = HalNameToHal(name);
   //如果没有找到就返回
   if (hal == NULL) {
      return;
   }
   hal->isOpened++;
   //printk("%s %d ", hal->halName, hal->isOpened);
   
   if (hal->halOperate->Open == NULL) {
      return;
   }
   hal->halOperate->Open();
}

/*
 * HalClose - 关闭hal设备
 * @name: 要操作的hal
 */
PUBLIC void HalClose(char *name)
{
   struct Hal *hal = HalNameToHal(name);
   //如果没有找到就返回
   if (hal == NULL) {
      return;
   }
   hal->isOpened--;
	if (hal->isOpened < 0) {
		hal->isOpened = 0;
	}
   if (hal->halOperate->Close == NULL) {
      return;
   }
   hal->halOperate->Close();
}

PUBLIC int HalRead(char *name, unsigned char *buffer, unsigned int count)
{
   struct Hal *hal = HalNameToHal(name);
   
   //如果没有找到就返回
   if (hal == NULL) {
      return -1;
   }
   
   //如果没有打开就返回
   if (!hal->isOpened) {
		return -1;
	}
   //printk("3");
   
   if (hal->halOperate->Read == NULL) {
      return -1;
   }
   //printk("4");
   
   return hal->halOperate->Read(buffer, count);
}


/*
 * HalWrite - 往hal硬件写入数据
 * @name: 要操作的hal
 * @buffer: 数据存放的缓冲区
 * @count: 写入的字节数
 * 
 * 返回值是是否操作成功，如果是0则成功，-1则失败
 */
PUBLIC int HalWrite(char *name, unsigned char *buffer, unsigned int count)
{
   struct Hal *hal = HalNameToHal(name);
   //如果没有找到就返回
   if (hal == NULL) {
      return -1;
   }
   //如果没有打开就返回
   if (!hal->isOpened) {
		return -1;
	}
   if (hal->halOperate->Write == NULL) {
      return -1;
   }
   return hal->halOperate->Write(buffer, count);
}

/*
 * HalIoctl - hal输入输出控制
 * @name: 要操作的hal
 * @type: 操作的类型
 * @value: 操作需要传递的值
 * 
 * 通过这个函数可以实现一些除开读取和写入的功能
 */
PUBLIC void HalIoctl(char *name,unsigned int cmd, unsigned int param)
{
   struct Hal *hal = HalNameToHal(name);
   //如果没有找到就返回
   if (hal == NULL) {
      return;
   }
   //如果没有打开就返回
   if (!hal->isOpened) {
		return;
	}
   if (hal->halOperate->Ioctl == NULL) {
      return;
   }
   hal->halOperate->Ioctl(cmd, param);
}
