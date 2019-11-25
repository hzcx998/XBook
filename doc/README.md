# 代码风格以及命名
欢迎来到BookOSX
开始吧！  
我们C代码和汇编代码，我们分开说明！  
## 汇编代码:  
    1.标签都是大写字母开头，单词间没有间隔  
    举例: LableStart, Entry, LoopForFoo123    
    2.宏定义都是大写字母，单词间用下划线隔开  
    举例: LOADER_START_SECTOR, FOO_MAX_NR, TIMES_2019_FAST  
    3.其它的所有命名都是小写  
    举例:  mov byte [0], al  dw 0xaa55  
    汇编的规则就这些，其它的可以自我发挥！
## C代码:  
    1.函数命名: 每个单词开头大写  
    举例：Init, InitCpu, CreateWindow, AllocKernelMemoryPage，WindowSetTitle  
    2.结构体命名：每个单词开头大写  
    举例：  
    struct FooProgram  
    {  
        int foo;  
    };   
    3.宏定义都是大写字母，单词间用下划线隔开  
    举例: MAX_DIR_NR, FOO_MAX_NR  
    4.变量的命名，首字母小写，后面都大写。areYouLikeThis，isTrue，above80Right  
    5.注意多使用空格，其它的可以自我发挥。  
如果还有其它的需要添加，后面慢慢完善！  
:)  
## 注意
- 编码格式统一 utf-8  

## 注释的重要性  
注释可以用英文也可以用中文，看个人爱好  
1.在文件开头必须添加注释，表明文件是什么，内容主要是：  
文件：文件的全路径  
作者：作者名字  
日期：文件创建日期  
版权：版权声明  
其它内容可以自行添加  
##  
    汇编代码  
    ;----  
    ;file:      arch/x86/include/const.inc  
    ;auther:    Jason Hu  
    ;time:      2018/1/23  
    ;copyright:	(C) 2018-2019 by BookOS developers. All rights reserved.  
    ;----  
    C语言代码  
    /*  
    * file:		init/main.c  
    * auther:	Jason Hu  
    * time:		2019/6/2  
    * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.  
    */  
    makefile代码
    #----  
    #file:		arch/x86/boot/makefile  
    #auther: 	Jason Hu  
    #time: 		2019/6/2  
    #copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.  
    #----  

2.在编写函数的时候要加上对函数的注释，主要内容是：  
功能：函数的主要功能  
参数：有哪些参数  
返回值：返回哪些值，不同的值有啥意思  
说明：需要提供的其它更多信息  
##  
    /**
    * SysMmap - 内存映射
    * @addr: 地址
    * @len: 长度
    * @prot: 页保护
    * @flags: 空间的标志
    *
    * 执行内存映射，使得该范围内的虚拟地址可以使用
    * 成功返回映射后的地址，失败返回NULL
    */
    
