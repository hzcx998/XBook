/*
 * file:		   kernel/hal.c
 * auther:	   Jason Hu
 * time:		   2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/hal.h>
#include <book/debug.h>
#include <book/list.h>
#include <share/vsprintf.h>
#include <share/string.h>

//创建全局变量 halListHead 来管理所有的hal
LIST_HEAD(halListHead);

/* ---------------------
 * 需要把hal的宿主导入此文件
 * ---------------------
 */
HAL_EXTERN(halOfDisplay);
HAL_EXTERN(halOfClock);
HAL_EXTERN(halOfCpu);
HAL_EXTERN(halOfRam);



/*
 * InitHal - 初始化hal环境
 */
PUBLIC void InitHalEnvironment()
{
   // 初始化hal链表头
   INIT_LIST_HEAD(&halListHead);
   
}

/*
 * InitHalEarly - 初始化早期的hal，主要是平台相关的hal
 */
PUBLIC void InitHalEarly()
{
   if (HalCreate(&halOfDisplay)) {
      while(1);
   }
   // 初始化控制台，基于硬件抽象层
	ConsoleInit();

   /* ----从此开始调试显示信息之旅---- */

   if (HalCreate(&halOfCpu)) {
      printk("Register %s name samed!\n", halOfCpu.halName);
      while(1);
   }
   if (HalCreate(&halOfRam)) {
      printk("Register %s name samed!\n", halOfRam.halName);
      while(1);
   }

}

/*
 * InitHalKernel - 初始化内核中的hal，主要是内核相关的hal
 */
PUBLIC void InitHalKernel()
{
   //printk("-> Hal ");
   PART_START("Hal kernel");

   if (HalCreate(&halOfClock)) {
      printk("Register %s name samed!\n", halOfClock.halName);
      while(1);
   }

   // 查看已经创建的hal
   struct Hal *hal;
   ListForEachOwner(hal, &halListHead, halList) {
      printk(" |- Registed hal-%s\n", hal->halName);
   }
   //printk("<-\n");
   PART_END();
}

/*
 * HalInit - 初始化一个hal
 * @hal: 要初始化的hal
 * 
 * 调用初始化hal的函数
 */
PRIVATE void HalInit(struct Hal *hal)
{
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
   ListAdd(&hal->halList, &halListHead);

   /* 3.初始化hal */
   HalInit(hal);

   return 0;
}

/*
 * HalNameToHal - 通过名字找到hal指针
 * @name: 要获取的hal的名字
 * 
 * 通过hal名字可以获取到hal地址，需要对hal进行操作。
 */
PRIVATE struct Hal *HalNameToHal(char *name)
{
   struct Hal *p;
   //循环查找，是否有同名的hal
   ListForEachOwner(p, &halListHead, halList) {
      //如果相同就返回
      if (!strcmp(p->halName, name)) {
         return p;
      }
   }
   //没有找到
   return NULL;
}

/*
 * HalOpen - 打开hal设备
 * @name: 要操作的hal
 */
PUBLIC void HalOpen(char *name)
{
   struct Hal *hal = HalNameToHal(name);
   //如果没有找到就返回
   if (hal == NULL) {
      return;
   }
   hal->isOpened++;
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
 * HalRead - 从hal硬件读取数据
 * @name: 要操作的hal
 * @buffer: 数据存放的缓冲区
 * @count: 读取的字节数
 * 
 * 从hal设备读取数据，可以是一个一个地读，也可以存放到换个缓冲区中
 * 返回值是是否操作成功，如果是0则成功，-1则失败
 */
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
   if (hal->halOperate->Read == NULL) {
      return -1;
   }
   return hal->halOperate->Read(buffer, count);
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
