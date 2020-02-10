# XBOOK简介
```
书是人类进步的阶梯
--高尔基
```

`X`是个很神奇的字母，所以X+BOOK就是`XBOOK`！

`xbook`是一个基于`x86`架构的`32`位操作系统，运行在PC电脑上，目前主要通过虚拟机测试开发。

一次偶然机会看到了操作系统居然可以自制，于是就感兴趣，从入门到放弃，再从放弃到入门，反反复复，折腾了几年，终于在2019年6月份定下了开发xbook。把自己感兴趣的部分，想要实现的部分都添加进来，并且希望在众多自制操作系统中有所突出，在学习他人的基础上，也添加了自己的许多想法。

目前已经支持的内容在[内容总览](https://github.com/huzichengdevelop/XBook/blob/master/documents/content.md)文件中查看。部分内容如下：
* 分页机制
* 物理内存管理
* 多进程/多线程
* 块设备驱动
* 文件系统
* 字符设备驱动
* 控制台tty
* 信号机制
* 定时器和闹钟

![os-structure](https://github.com/huzichengdevelop/XBook/raw/master/documents/picture/os-structure.png)

# 操作系统开发指南
* [源码目录结构](https://github.com/huzichengdevelop/XBook/blob/master/documents/source-tree.md)
* [开发者手册PDF版本](https://github.com/huzichengdevelop/XBook/blob/master/documents/develop-manual.pdf)
* [代码风格](https://github.com/huzichengdevelop/XBook/blob/master/documents/code-style.md)
* [开发者名单](https://github.com/huzichengdevelop/XBook/blob/master/CREDITS.md)
# 工具环境搭建
## 虚拟机-推荐qemu
* bochs
* qemu
* virtaul box
* vmware
## 代码编辑器-任选其一
* visual studio code
* notepad++
* vim
* eclipse

## 开发工具
所需工具如下gcc, nasm, ld, dd, ar, make, rm

### windows
* [工具包下载，选我我超甜](http://www.book-os.org/tools/BuildTools-v3.rar)

### linux
* 自己根据以上工具名字安装哦

### macos
* 和linux兄弟类似

注意！工具包和虚拟机都需要配置`环境变量`，这样无论源码在哪个路径都可以进行编译运行。配置方法参考开发者手册。

# 编译源码
操作指令 command in makefile  
```sh
#compile, link（编译并且链接）
make
#compile, link and run os in vm（编译链接写入磁盘并且在虚拟机中运行）
make run
#run in qemu（直接在qemu虚拟机中运行）
make qemu
#run in bochs（在bochs虚拟机中运行）
make bochs 
#run in bochsdbg（运行bochsdbg调试器）
make bochsdbg
#run in virtual box（运行在vbox虚拟机中，需要配置虚拟机名）
make vbox
#clean all .o, .bin, .a file（删除所有产生的临时文件）
make clean
# make a libary file（生成库文件，给应用程序链接）
make lib 
# remove a libary file（删除库文件）
make rmlib 
```

## window
 可以直接运行`launch_cmd.bat`打开命令行，输入`make run`即可运行。
## linux
在`xbook`目录下打开终端，输入对于指令即可运行。
## macos
和`linux`类似

# 开发交流，群贤聚集，必成大事
BookOS开发QQ官方群：`913813452`

# 资助鼓励
如果您觉得我写的系统对您来说是有价值的，并鼓励我进行更多的开源及免费开发，那您可以资助我，就算是一瓶可乐...
![donate-with-wechat](https://github.com/huzichengdevelop/XBook/raw/master/documents/picture/donate-with-wechat.jpg)
![donate-with-alipay](https://github.com/huzichengdevelop/XBook/raw/master/documents/picture/donate-with-alipay.jpg)

# 联系我
官方网址：www.book-os.org  
电子邮件：book_os@163.com  
