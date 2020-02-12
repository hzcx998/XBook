
/*
 * file:		pci/core/pci.c
 * auther:		Jason Hu
 * time:		2019/10/16
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <pci/pci.h>

//#define _DEBUG_PCI

struct PciDevice pciDeviceTable[PCI_MAX_DEVICE_NR];	/*device table*/
	
PRIVATE void PciBarInit(struct PciDeviceBar *bar, uint32 addrRegValue, uint32 lenRegValue)
{
	/*if addr is 0xffffffff, we set it to 0*/
    if (addrRegValue == 0xffffffff) {
        addrRegValue = 0;
    }
	/*we judge type by addr register bit 0, if 1, type is io, if 0, type is memory*/
    if (addrRegValue & 1) {
        bar->type = PCI_BAR_TYPE_IO;
		bar->baseAddr = addrRegValue  & PCI_BASE_ADDR_IO_MASK;
        bar->length    = ~(lenRegValue & PCI_BASE_ADDR_IO_MASK) + 1;
    }
    else {
        bar->type = PCI_BAR_TYPE_MEM;
        bar->baseAddr = addrRegValue  & PCI_BASE_ADDR_MEM_MASK;
        bar->length    = ~(lenRegValue & PCI_BASE_ADDR_MEM_MASK) + 1;
    }
}

PRIVATE void PciDeviceInit(struct PciDevice *device, uint8 bus, uint8 dev, uint8 function,\
					uint16 vendorID, uint16 deviceID, uint32 classCode, uint8 revisionID,\
					uint8 multiFunction)
{
	/*set value to device*/
    device->bus = bus;
    device->dev = dev;
    device->function = function;

    device->vendorID = vendorID;
    device->deviceID = deviceID;
    device->multiFunction = multiFunction;
    device->classCode = classCode;
    device->revisionID = revisionID;
	int i;
    for (i = 0; i < PCI_MAX_BAR; i++) {
         device->bar[i].type = PCI_BAR_TYPE_INVALID;
    }
    device->irqLine = -1;
}

PRIVATE uint32 PciRead(uint32 bus, uint32 device, uint32 function, uint32 addr)
{
	uint32 reg = 0x80000000;
	/*make config add register*/
	reg |= (bus & 0xFF) << 16;
    reg |= (device & 0x1F) << 11;
    reg |= (function & 0x7) << 8;
    reg |= (addr & 0xFF) & 0xFC;	/*bit 0 and 1 always 0*/
	/*PciWrite to config addr*/
    Out32(PCI_CONFIG_ADDR, reg);
    return In32(PCI_CONFIG_DATA);	/*return confige addr's data*/
}

PRIVATE void PciWrite(uint32 bus, uint32 device, uint32 function, uint32 addr, uint32 val)
{
	uint32 reg = 0x80000000;
	/*make config add register*/
	reg |= (bus & 0xFF) << 16;
    reg |= (device & 0x1F) << 11;
    reg |= (function & 0x7) << 8;
    reg |= (addr & 0xFF) & 0xFC;	/*bit 0 and 1 always 0*/
	
	/*PciWrite to config addr*/
    Out32(PCI_CONFIG_ADDR, reg);
	/*PciWrite data to confige addr*/
    Out32(PCI_CONFIG_DATA, val);
}

PRIVATE void PciSacnDevice(uint8 bus, uint8 device, uint8 function)
{
	/*PciRead vendor id and device id*/
    uint32 val = PciRead(bus, device, function, PCI_DEVICE_VENDER);
    uint16 vendorID = val & 0xffff;
    uint16 deviceID = val >> 16;
	/*if vendor id is 0xffff, it means that this bus , device not exist!*/
    if (vendorID == 0xffff) {
        return;
    }
	/*PciRead header type*/
    val = PciRead(bus, device, function, PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE);
    uint8 headerType = ((val >> 16));
	/*PciRead command*/
    val = PciRead(bus, device, function, PCI_STATUS_COMMAND);
   // uint16 command = val & 0xffff;
	/*PciRead class code and revision id*/
    val = PciRead(bus, device, function, PCI_CLASS_CODE_REVISIONID);
    uint32 classcode = val >> 8;
    uint8 revisionID = val & 0xff;
	
	/*alloc a pci device to store info*/
	struct PciDevice *pciDev = AllocPciDevice();
	if(pciDev == NULL){
		return;
	}
	/*init pci device*/
    PciDeviceInit(pciDev, bus, device, function, vendorID, deviceID, classcode, revisionID, (headerType & 0x80));
	
	/*init pci device bar*/
	int bar, reg;
    for (bar = 0; bar < PCI_MAX_BAR; bar++) {
        reg = PCI_BASS_ADDRESS0 + (bar*4);
		/*PciRead bass address[0~5] to get address value*/
        val = PciRead(bus, device, function, reg);
		/*set 0xffffffff to bass address[0~5], so that if we PciRead again, it's addr len*/
        PciWrite(bus, device, function, reg, 0xffffffff);
       
	   /*PciRead bass address[0~5] to get addr len*/
		uint32 len = PciRead(bus, device, function, reg);
        /*PciWrite the io/mem address back to confige space*/
		PciWrite(bus, device, function, reg, val);
		/*init pci device bar*/
        if (len != 0 && len != 0xffffffff) {
            PciBarInit(&pciDev->bar[bar], val, len);
        }
    }

	/*get irq line and pin*/
    val = PciRead(bus, device, function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE);
    if ((val & 0xff) > 0 && (val & 0xff) < 32) {
        uint32 irq = val & 0xff;
        pciDev->irqLine = irq;
        pciDev->irqPin = (val >> 8)& 0xff;
    }
	
    #ifdef _DEBUG_PCI
        //printk("pci device at bus: %d, device: %d\n", bus, device);
        DumpPciDevice(pciDev);
	#endif
}


PRIVATE void PciScanBuses()
{
	uint16 bus;
	uint8 device, function;
	/*扫描每一条总线上的设备*/
    for (bus = 0; bus < PCI_MAX_BUS; bus++) {
        for (device = 0; device < PCI_MAX_DEV; device++) {
           for (function = 0; function < PCI_MAX_FUN; function++) {
				PciSacnDevice(bus, device, function);
			}
        }
    }
}

PUBLIC struct PciDevice* GetPciDevice(uint16 vendorID, uint16 deviceID)
{
	int i;
	struct PciDevice* device;
	
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		device = &pciDeviceTable[i];
		if (device->vendorID == vendorID && device->deviceID == deviceID) {
			return device;
		}
	}
    return NULL;
}

PUBLIC void EnablePciBusMastering(struct PciDevice *device)
{
    uint32 val = PciRead(device->bus, device->dev, device->function, PCI_STATUS_COMMAND);
    #ifdef _DEBUG_PCI
        printk("before enable bus mastering, command: %x\n", val);
    
    #endif
	val |= 4;
    PciWrite(device->bus, device->dev, device->function, PCI_STATUS_COMMAND, val);

    val = PciRead(device->bus, device->dev, device->function, PCI_STATUS_COMMAND);
    #ifdef _DEBUG_PCI
        printk("after enable bus mastering, command: %x\n", val);

    #endif
}

PUBLIC uint32 GetPciDeviceConnected()
{
	int i;
	struct PciDevice *device;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		device = &pciDeviceTable[i];
        if (device->status != PCI_DEVICE_STATUS_USING) {
            break;
        }
    }
	return i;
}

PUBLIC struct PciDevice *AllocPciDevice()
{
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		if (pciDeviceTable[i].status == PCI_DEVICE_STATUS_INVALID) {
			pciDeviceTable[i].status = PCI_DEVICE_STATUS_USING;
			return &pciDeviceTable[i];
		}
	}
	return NULL;
}

PUBLIC int FreePciDevice(struct PciDevice *device)
{
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		if (&pciDeviceTable[i] == device) {
			device->status = PCI_DEVICE_STATUS_INVALID;
			return 0;
		}
	}
	return -1;
}


/*read value from pci device config space register*/
PUBLIC uint32 PciDeviceRead(struct PciDevice *device, uint32 reg)
{
    return PciRead(device->bus, device->dev, device->function, reg);
}

/*write value to pci device config space register*/
PUBLIC void PciDeviceWrite(struct PciDevice *device, uint32 reg, uint32 value)
{
    PciWrite(device->bus, device->dev, device->function, reg, value);
}


PUBLIC uint32 GetPciDeviceIoAddr(struct PciDevice *device)
{
	int i;
    for (i = 0; i < PCI_MAX_BAR; i++) {
        if (device->bar[i].type == PCI_BAR_TYPE_IO) {
            return device->bar[i].baseAddr;
        }
    }

    return 0;
}

PUBLIC uint32 GetPciDeviceMemAddr(struct PciDevice *device)
{
	int i;
    for (i = 0; i < PCI_MAX_BAR; i++) {
        if (device->bar[i].type == PCI_BAR_TYPE_MEM) {
            return device->bar[i].baseAddr;
        }
    }

    return 0;
}

PUBLIC uint32 GetPciDeviceIrqLine(struct PciDevice *device)
{
    return device->irqLine;
}

PUBLIC void PciBarDump(struct PciDeviceBar *bar)
{
	
    printk("type: %s, ", bar->type == PCI_BAR_TYPE_IO ? "io base address" : "mem base address");
    printk("base address: %x, ", bar->baseAddr);
    printk("len: %x\n", bar->length);
}

PUBLIC void DumpPciDevice(struct PciDevice *device)
{
	//printk("status:      %d\n", device->status);
    printk("vendor id:      %x  ", device->vendorID);
    printk("device id:      %x  ", device->deviceID);
	printk("class code:     %x\n", device->classCode);
    /*printk("revision id:       %x  ", device->revisionID);
    printk("multi function: %d\n", device->multiFunction);*/
    //printk("irq line: %d  ", device->irqLine);
    //printk("irq pin:  %d\n", device->irqPin);
    int i;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		/*if not a invalid bar*/
        if (device->bar[i].type != PCI_BAR_TYPE_INVALID) {
            /*printk("bar %d: ", i);
			PciBarDump(&device->bar[i]);*/
        }
    }
}

PUBLIC void InitPci()
{
    /*init pci device table*/
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		pciDeviceTable[i].status = PCI_DEVICE_STATUS_INVALID;
	}

	/*scan all pci buses*/
	PciScanBuses();

	#ifdef _DEBUG_PCI
	    printk("PCI: device connected number is %d.\n", GetPciDeviceConnected());
	#endif
}
