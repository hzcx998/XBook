# 代码风格以及命名
欢迎来到BookOS起源  
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
    1.函数命名: 第一个单词小写，第二个单词也小写，第一个单词和第二个单词用下划线隔开，后面的单词都大写，没有下划线。  
    举例：init, init_cpu, create_windowButton, alloc_kernelMemoryPage，window_setTitle  
    2.结构体命名：结构体名称小写，每个单词之间用下划线隔开，在最后加上_s，并且要写typedef，typedef后的新名字为和前面一样只是最后改成了_t  
    举例：  
    typedef struct foo_program_s  
    {  
        int foo;  
    }foo_program_t;  
    3.宏定义都是大写字母，单词间用下划线隔开  
    举例: MAX_DIR_NR, FOO_MAX_NR  
    4.注意多实用空格，其它的可以自我发挥。  
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

2.在编写函数的时候要加上对函数的注释，主要内容是：  
功能：函数的主要功能
参数：有哪些参数
返回值：返回哪些值，不同的值有啥意思
说明：需要提供的其它更多信息
##
    /*
    * function: the mian of kernel
    * parameters: none
    * return: do not return, just pretend to return
    * explain: after boot and loader, it will be there!
    */
    /*
    * 功能: 内核的主函数
    * 参数: 无
    * 返回: 不返回，只是假装返回
    * 说明: 引导和加载完成后，就会跳到这里
    */
