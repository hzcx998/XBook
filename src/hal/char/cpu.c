/*
 * file:		hal/char/cpu.c
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/stddef.h>
#include <hal/char/cpu.h>
#include <book/arch.h>
#include <share/string.h>
#include <share/math.h>
/*
 * cpuid 分为2组信息，一组为基本信息和扩展信息，往eax输入0到3是基本信息，
 * 输入0x80000000到0x80000004返回扩展信息
 */

PRIVATE struct CpuPrivate {
	char vendor[16];
	char brand[50];
	unsigned int family;	//系列
	unsigned int model;		//型号
	unsigned int type;		//类型
	unsigned int stepping;	//频率
	unsigned int maxCpuid;	
	unsigned int  maxCpuidExt;
	char *familyString;
	char *modelString;

	char steerType;	//操作的类型：string 和 number
	char *steerString;	//操作指向，用于获取cpu字符串信息
	unsigned int steerNumber;	//操作指向，用于获取cpu数字信息
}cpuPrivate;

/**CPU family*/
char CpuFamily06H[] = "Pentium 4";
char Cpufamily0FH[] = "P6";
char CpufamilyAMD[] = "Unknown AMD Processer";
char CpufamilyAMDK5[] = "AMD K5";
char CpufamilyVIA[] = "VIA Processer";
char CpufamilySiS[] = "SiS Processer";
char CpufamilyVMware[] = "Vmware Virtual Processor";
char Cpufamilyvpc[] = "Hyper-v Virtual Processor";
char CpufamilyUnknown[] = "Unknown";

/**CPU model*/
char CpuModel06_2AH[] = "Intel Core i7 i5 i3 2xxx";
char CpuModel06_0FH[] = "Intel Dual-core processor";
char CpuModelUnknown[] = "Unknown";

/* Cpu end of loop */
char CpuUnknownInfo[] = "Unknown";

#define STEERING_TYPE_STRING 0
#define STEERING_TYPE_NUMBER 1


/*
PRIVATE void CpiHalPrintInfo()
{
	printk("\n |- vendor:%s \n", cpuPrivate.vendor);
	printk(" |- brand:%s \n", cpuPrivate.brand);
	printk(" |- max Cpuid:%x externed:%x\n", cpuPrivate.maxCpuid, 
			cpuPrivate.maxCpuidExt);

	printk(" |- family:%s[%x]\n", cpuPrivate.familyString, cpuPrivate.family);
	printk(" |- model:%s[%x]\n", cpuPrivate.modelString,cpuPrivate.model);
	
	printk(" |- type:%d \n", cpuPrivate.type);
	printk(" |- stepping:%d \n", cpuPrivate.stepping);
}*/

PRIVATE void CpuHalIoctl(unsigned int cmd, unsigned int param)
{
	//根据类型设置不同的值
	switch (cmd)
	{
	case CPU_HAL_IO_INVLPG:
		X86Invlpg(param);

		break;
	case CPU_HAL_IO_STEER:
		if (param == CPU_HAL_VENDOR) {
			//指向vendor
			cpuPrivate.steerString = cpuPrivate.vendor;

			cpuPrivate.steerType = STEERING_TYPE_STRING;
		} else if (param == CPU_HAL_BRAND) {
			//指向brand
			cpuPrivate.steerString = cpuPrivate.brand;

			cpuPrivate.steerType = STEERING_TYPE_STRING;
		} else if (param == CPU_HAL_FAMILY) {
			//指向family
			cpuPrivate.steerString = cpuPrivate.familyString;

			cpuPrivate.steerType = STEERING_TYPE_STRING;
		} else if (param == CPU_HAL_MODEL) {
			//指向model
			cpuPrivate.steerString = cpuPrivate.modelString;

			cpuPrivate.steerType = STEERING_TYPE_STRING;
		} else if (param == CPU_HAL_STEEPING) {
			//指向model
			cpuPrivate.steerNumber = cpuPrivate.stepping;

			cpuPrivate.steerType = STEERING_TYPE_NUMBER;
		} else if (param == CPU_HAL_MAX_CPUID) {
			//指向model
			cpuPrivate.steerNumber = cpuPrivate.maxCpuid;

			cpuPrivate.steerType = STEERING_TYPE_NUMBER;
		} else if (param == CPU_HAL_MAX_CPUID_EXT) {
			//指向model
			cpuPrivate.steerNumber = cpuPrivate.maxCpuidExt;

			cpuPrivate.steerType = STEERING_TYPE_NUMBER;
		} else if (param == CPU_HAL_TYPE) {
			//指向model
			cpuPrivate.steerNumber = cpuPrivate.type;

			cpuPrivate.steerType = STEERING_TYPE_NUMBER;
		}
		break;
	

	default:
		break;
	}
}

/*
 * CpuHalRead - 从cpu读取数据到buffer中
 * buffer: 缓冲区buf
 * count: 数据数量
 * 
 * 读取完数据后会自动指向下一个数据区域，可以通过ioctl进行设置
 */
PRIVATE int CpuHalRead(unsigned char *buffer, unsigned int count)
{
	if (cpuPrivate.steerType == STEERING_TYPE_STRING) {
		//我们以短的字符为标准
		unsigned int size = min(strlen(cpuPrivate.steerString), count);

		//把数据复制过去
		memcpy(buffer, cpuPrivate.steerString, size);
		
		//如果大于传递进来的count大小，那么我们需要把buffer最后一个字符修改'\0'
		if (size >= count) {
			buffer[size-1] = '\0';

		} else {
			//如果是字符串的大小，我们需要在buffer最后面添加'\0'
			buffer[size] = '\0';

		}
		
	} else if (cpuPrivate.steerType == STEERING_TYPE_NUMBER) {
		//把数据复制过去
		memcpy(buffer, &cpuPrivate.steerNumber, sizeof(cpuPrivate.steerNumber));
	}
	return 0;
}

PUBLIC void CpuHalInit()
{
	PART_START("Cpu hal");

	register_t eax, ebx, ecx, edx;
	/* 
	 * 获取vendor ID
	 * 输入: eax = 0：
	 * 返回：eax = Maximum input Value for Basic CPUID information
	 */
	X86Cpuid(0x00000000, &eax, &ebx, &ecx, &edx);
	cpuPrivate.maxCpuid = eax;
	
	memcpy(cpuPrivate.vendor    , &ebx, 4);
	memcpy(cpuPrivate.vendor + 4, &edx, 4);
	memcpy(cpuPrivate.vendor + 8, &ecx, 4);
	cpuPrivate.vendor[12] = '\0';

	/*
    * eax == 0x800000000
    * 如果CPU支持扩展信息，则在EAX中返 >= 0x80000001的值。
    */
	X86Cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
	cpuPrivate.maxCpuidExt = eax;

	//先判断是哪种厂商
	if (!strncmp(cpuPrivate.vendor, "GenuineIntel", 12)) {
		
		//get version information
		X86Cpuid(0x00000001, &eax, &ebx, &ecx, &edx);

		cpuPrivate.family   = (((eax >> 20) & 0xFF) << 4)
				+ ((eax >> 8) & 0xF);
	
		cpuPrivate.model    = (((eax >> 16) & 0xF) << 4)
				+ ((eax >> 4) & 0xF);
		
		//获取stepping和type
		cpuPrivate.stepping = (eax >> 0) & 0xF;
		cpuPrivate.type = (eax >> 12) & 0xF;	//只取2位
		
		/*
		 * CPU family & model information
		 * get family and model string.
		 */
		if (cpuPrivate.family == 0x06) {
			cpuPrivate.familyString = CpuFamily06H;

			if (cpuPrivate.model == 0x2a) {
				cpuPrivate.modelString = CpuModel06_2AH;
			} else if (cpuPrivate.model == 0x0f) {
				cpuPrivate.modelString = CpuModel06_0FH;
			} else {
				cpuPrivate.modelString = CpuModelUnknown;
			}
		} else if (cpuPrivate.family == 0x0f) {
			cpuPrivate.familyString = Cpufamily0FH;
			cpuPrivate.modelString = CpuModelUnknown;
		} else {
			cpuPrivate.familyString = CpufamilyUnknown;
			cpuPrivate.modelString = CpuModelUnknown;
		}
	} else if (!strncmp(cpuPrivate.vendor, "AuthenticAMD", 12)) {

		cpuPrivate.familyString = CpufamilyAMD;
		cpuPrivate.modelString = CpuModelUnknown;
	} else if (!strncmp(cpuPrivate.vendor, "AMDisbetter!", 12)) {

		cpuPrivate.familyString = CpufamilyAMDK5;
		cpuPrivate.modelString = CpuModelUnknown;
	} else if (!strncmp(cpuPrivate.vendor, "SiS SiS SiS ", 12)) {

		cpuPrivate.familyString = CpufamilySiS;
		cpuPrivate.modelString = CpuModelUnknown;
	} else if (!strncmp(cpuPrivate.vendor, "VIA VIA VIA ", 12)) {

		cpuPrivate.familyString = CpufamilyVIA;
		cpuPrivate.modelString = CpuModelUnknown;
	} else if (!strncmp(cpuPrivate.vendor, "Microsoft Hv", 12)) {

		cpuPrivate.familyString = Cpufamilyvpc;
		cpuPrivate.modelString = CpuModelUnknown;
	} else if (!strncmp(cpuPrivate.vendor, "VMwareVMware", 12)) {

		cpuPrivate.familyString = CpufamilyVMware;
		cpuPrivate.modelString = CpuModelUnknown;
	} else {

		cpuPrivate.familyString = CpufamilyUnknown;
		cpuPrivate.modelString = CpuModelUnknown;
	}

	/*
    * 如果CPU支持Brand String，则在max cpu externed >= 0x80000004的值。
	* 以下获取扩展商标
    */
	if(cpuPrivate.maxCpuidExt >= 0x80000004){
		X86Cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
		memcpy(cpuPrivate.brand      , &eax, 4);
		memcpy(cpuPrivate.brand  +  4, &ebx, 4);
		memcpy(cpuPrivate.brand  +  8, &ecx, 4);
		memcpy(cpuPrivate.brand  + 12, &edx, 4);
		X86Cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
		memcpy(cpuPrivate.brand  + 16, &eax, 4);
		memcpy(cpuPrivate.brand  + 20, &ebx, 4);
		memcpy(cpuPrivate.brand  + 24, &ecx, 4);
		memcpy(cpuPrivate.brand  + 28, &edx, 4);
		X86Cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
		memcpy(cpuPrivate.brand  + 32, &eax, 4);
		memcpy(cpuPrivate.brand  + 36, &ebx, 4);
		memcpy(cpuPrivate.brand  + 40, &ecx, 4);
		memcpy(cpuPrivate.brand  + 44, &edx, 4);
		cpuPrivate.brand[49] = '\0';
		int i;
		for (i = 0; i < 49; i++){
			if (cpuPrivate.brand[i] > 0x20) {
				break;
			}
		}
	}

	//默认指向vendor
	cpuPrivate.steerString = cpuPrivate.vendor;
	//默认是string
	cpuPrivate.steerType = STEERING_TYPE_STRING;

	PART_END();
}

/* hal操作函数 */
PRIVATE struct HalOperate halOperate = {
	.Init = &CpuHalInit,
	.Read = &CpuHalRead,
	.Ioctl = &CpuHalIoctl,
};

/* hal对象，需要把它导入到hal系统中 */
PUBLIC HAL(halOfCpu, halOperate, "cpu");
