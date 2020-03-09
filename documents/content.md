# Book OS


## 2020.3.10
* optimize the process scheduling algorithm
* optimize file descriptor management
* add graphical interface with multiple Windows
* improve user - mode graphical interface
* separate user and system libraries
* fixed some bugs in the system
* simple init configuration file
* add video and input driver framework

## 2020.3.2
* add gtty driver
* bosh run base on gtty
* update time management
* add a calender application with terminal
* add ls, ps, mv, cp, mm, echo buildin command
* make pipe and redirect in bosh
* add null device driver

## 2020.2.24
* make a simple GUI based on KGC  

## 2020.2.10
* refactor source directory  

## 2020.2.8
* mouse driver  
* update keyboard driver  
* vesa video driver  
* kernel graph core-KGC  

## 2020.1.29
* libc (api lib for user application)
* low memory(for old device)
* rtl8139 driver（Only in QEMU）
* pcspeaker
* serial support 
 
## 2019.12.29
* signal system 

## 2019.12.11
* bosh
* tty

## 2019.12.1 
* new file system（BOFS v0.3）

## 2019.11.17
* synclock
* mutexlock
* spinlock

## 2019.10.21
* flat file system of device file
* new keyboard driver
* char device
* ide driver
* ramdisk driver
* block device
* new HAL define
* memory cache
* node phy memory management

## 2019.8.22
* keyboard driver
* keyboard hal
* work queue
* task assist
* softirq

## 2019.8.11
* realloc and calloc
* malloc and free
* brk and sbrk

## 2019.8.10
* fork and execv
* sync lock
* semaphore and semaphore2
* atomic operate
* load ELF binary
* exit and wait
* sleep and wakeup
* block and unblock
* user process
* kernel thread
* VMSpace memory

## 2019.7.26 
* Slab memory
* Vmarea memory

## 2019.7.4 
* buddy memory  

## 2019.7.3 
* ram hal  

## 2019.6.27 
* cpu hal  

## 2019.6.26
* load kernel with elf format  
* 32 bits protect mode  
* multi-platform architecture
* get memory size by ards  
* page mode  
* gate and segment descriptor  
* HAL (Hardware Abstraction Layer)  
* list struct  
* bitmap struct  
* debug system  
* display hal  
* clock hal  
* console driver  
* clock driver  
* string lib
