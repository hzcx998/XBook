# 源码目录结构

```
xbook
├─binary
│  ├─system
│  │  ├─bosh        
│  │  └─init        
│  ├─test
│  └─user
│      └─infones
│          └─core
│              └─mapper
├─developments
│  ├─bochs
│  ├─image
│  ├─qemu
│  ├─virtualbox
│  └─vmware
├─documents
├─scripts
│  └─writefile
└─src
    ├─arch
    │  ├─x64
    │  │  ├─boot
    │  │  ├─include
    │  │  └─kernel
    │  └─x86
    │      ├─boot
    │      ├─include
    │      │  ├─boot
    │      │  ├─kernel
    │      │  └─mm
    │      └─kernel
    │          ├─core
    │          ├─hal
    │          └─mm
    ├─driver
    │  ├─block
    │  │  ├─core
    │  │  ├─ide
    │  │  ├─sata
    │  │  └─virtual
    │  ├─char
    │  │  ├─console
    │  │  ├─core
    │  │  ├─uart
    │  │  └─virtual
    │  ├─clock
    │  │  ├─core
    │  │  └─pit
    │  ├─input
    │  │  ├─core
    │  │  ├─keyboard
    │  │  └─mouse
    │  ├─network
    │  │  ├─amd
    │  │  ├─core
    │  │  │  └─ipv4
    │  │  └─realtek
    │  ├─pci
    │  │  └─core
    │  ├─sound
    │  │  ├─buzzer
    │  │  ├─core
    │  │  └─sb
    │  └─video
    │      ├─core
    │      └─vbe
    ├─external
    ├─include
    │  ├─block
    │  │  ├─ide
    │  │  └─virtual
    │  ├─book
    │  ├─char
    │  │  ├─console
    │  │  ├─uart
    │  │  └─virtual
    │  ├─clock
    │  │  └─pit
    │  ├─external
    │  ├─fs
    │  │  └─bofs
    │  ├─hal
    │  ├─input
    │  │  ├─keyboard
    │  │  └─mouse
    │  ├─kgc
    │  ├─lib
    │  ├─net
    │  │  ├─amd
    │  │  ├─ipv4
    │  │  └─realtek
    │  ├─pci
    │  ├─sound
    │  │  ├─buzzer
    │  │  └─sb
    │  └─video
    │      └─vbe
    ├─init
    ├─kernel
    │  ├─device
    │  ├─fs
    │  │  └─bofs
    │  ├─graph
    │  ├─hal
    │  ├─ipc
    │  ├─lib
    │  ├─mm
    │  ├─task
    │  └─time
    └─lib
        ├─libc
        └─libx
```

## binary

可执行文件目录，里面存放了操作系统的可执行文件，分为系统和用户2种，一个是系统`system`运行需要的程序，另一个是用户`user`使用的应用程序。如果要添加应用程序，需要在`user`目录下面添加源码。

## developments

开发时需要用到的磁盘镜像，虚拟机工具配置等，都在此目录下面。

## documents

用于存放开发文档，包括《源码目录结构》、《代码风格》等。

## scripts

一些常用的脚本文件放在此处，需要时可以直接进来运行，像`qemu-dbg`脚本就在里面。writefile脚本，把文件写入到镜像磁盘镜像，这样就可以在操作系统中把磁盘上的数据加载到文件系统中，用于文件的读写了。

## src

操作系统的源码，里面又分成了多个目录，存放了内核的各个部分。此目录下面有一个主`Makefile`文件，编译规则是`Makefile.build`文件指定。这套编译方案可以递归搜索子目录，您只需要在某个目录下面添加`makefile`文件，并且把`c`和`asm`文件对应的`.o`对象文件添加到哪个`makefile`里面即可。可以参考其它目录是怎样做的，就能很轻松地添加文件到系统中了。

### arch

体系结构主目录，每个具体的体系实现里提供了一个`boot`，`include`和`kernel`目录，`boot`用于存放引导时的代码，`kernel`是体系结构的核心，存放相关代码，`include`是头文件引用，`boot`和`kernel`的头文件都在里面。

| 名称  | 描述   |
|-----|-----|
|x86  |x86 32位指令集|

#### x86

`kernel`目录被分成3个部分，`core`，`hal`和`mm`。`core`包含了保护模式相关内容，中断管理。`mm`包含了分页机制和物理内存管理。`hal`包含了`cpu`和`ram`内存的`硬件抽象`.

### driver

驱动框架主目录，各种驱动框架，以及对于的驱动都在此目录下面。已实现的如下驱动模型：

| 目录   | 描述   |
|--------|--------------|
|block   | 块设备驱动    |
|char    | 字符设备驱动  |
|input   | 输入设备驱动  |
|netwrok | 网络设备驱动  |
|pci     | pci设备驱动   |
|sound   | 音频驱动      |
|sound   | 视频驱动      |

对于每一个驱动，`core`目录是驱动框架核心，其它目录是驱动程序。

### external
扩展程序主目录，开源的程序库目录。

| 目录   | 描述   |
|--------|-----|
| 无     | 无     |

### include 
头文件主目录，操作系统的`.h`和`.inc`头文件都在里面。而架构体系的头文件在对应的架构里面的`include`里面。kernel内容的头文件都在`book`目录下，驱动框架的偷吻都有对应的名字。对于特殊的内核内容，例如`fs`，`kgc`等，也有其对于的目录。

### init
初始化主目录，内核的初始化在此目录下面。当从`arch`里面执行完后，就会跳转到这里面来执行整个内核的初始化。

### kernel
内核主目录，提供各种核心组件实现，是操作系统的核心的核心。重中之重。

#### device
设备管理核心，高级中断管理都在里面。

#### fs
文件系统管理，目前仅支持单一的文件系统。

#### graph
图形管理，实现了一个内核图形核心KGC，用于实现内核态下面图形界面管理。有输出和输出设备，在内核下面实现对图形界面的支持。

#### hal
硬件抽象层，支持简单的硬件抽象管理，可以对`CPU`和`RAM`内存进行抽象管理。对应抽象体在`arch`里面的`hal`中实现。

#### ipc
进程间通信，目前仅支持信号机制，后续可以考虑共享内存，消息机制等。

#### lib
内核库，在内核层面上实现的一些库，像`bitmap`位图管理，`fifo`队列，`debug`调试等。

#### mm
内存管理，内存缓存池，虚拟内存区域，进程空间管理等。

#### task
多进程/线程管理，即多任务管理。调度，休眠唤醒，同步与互斥，系统调用等。

#### time
时间管理，闹钟，定时器等。

### lib
库函数，包含了`c`库和系统调用库，`c`库可以供系统使用也可以供应用程序使用，而系统调用库主要供用户使用。
