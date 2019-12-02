/*
 * file:		mm/exec.c
 * auther:	    Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/schedule.h>
#include <book/arch.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/task.h>
#include <book/elf32.h>
#include <share/math.h>
#include <fs/fs.h>
#include <book/iostream.h>

/**
 * ReadFileFrom - 从文件读取数据
 * @fd: 文件描述符
 * @buffer: 读取到的buffer
 * @offset: 偏移
 * @size: 要读取的数据数量
 */
PRIVATE int ReadFileFrom(int fd, void *buffer, uint32_t offset, uint32_t size)
{
    SysLseek(fd, offset, SEEK_SET);
    //printk("seek!\n");
    if (SysRead(fd, buffer, size) != size) {
        printk(PART_ERROR "ReadFileFrom: read failed!\n");
        return -1;
    }
    return 0;
}

/**
 * SegmentLoad - 加载段
 * @fd: 文件描述符
 * @offset: 
 * @fileSize: 段大小
 * @vaddr: 虚拟地址
 * 
 * 加载一个段到内存
 */
PRIVATE int 
SegmentLoad(int  fd, uint32_t offset, uint32_t fileSize, uint32_t vaddr)
{
    /*printk(PART_TIP "SegmentLoad:fd %d off %x size %x vaddr %x\n",
        fd, offset, fileSize, vaddr);
     */
    /* 获取虚拟地址的页对齐地址 */
    uint32_t vaddrFirstPage = vaddr & PAGE_ADDR_MASK;

    /* 获取在第一个页中的大小 */
    uint32_t sizeInFirstPage = PAGE_SIZE - (vaddr & PAGE_INSIDE);

    /*  段要占用多少个页 */
    uint32_t occupyPages = 0;

    /* 计算最终占用多少个页 */
    if (fileSize > sizeInFirstPage) {
        /* 计算文件大小比第一个页中的大小多多少，也就是还剩下多少字节 */
        uint32_t leftSize = fileSize - sizeInFirstPage;

        /* 整除后向上取商，然后加上1（1是first page） */
        occupyPages = DIV_ROUND_UP(leftSize, PAGE_SIZE) + 1;
    } else {
        occupyPages = 1;
    }

    /* 虚拟地址和物理地址进行映射 */
    if (MapPagesMaybeMapped(vaddrFirstPage, occupyPages, PAGE_US_U | PAGE_RW_W)) {
        printk(PART_ERROR "SegmentLoad: MapPagesMaybeMapped failed!\n");
        return -1;
    }

    /* 映射内存空间 */
    /*if (MapPages(vaddrFirstPage, occupyPages * PAGE_SIZE, PAGE_US_U | PAGE_RW_W)) {
        return -1;
    }*/

    //printk(PART_TIP "addr %x link done!\n", vaddrFirstPage);
    
    /* 映射虚拟空间 */  
    int32 ret = (int32)SysMmap(vaddrFirstPage, occupyPages*PAGE_SIZE, 
            PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED);
    if (ret < 0) {
        printk(PART_ERROR "SegmentLoad: SysMmap failed!\n");
        return -1;
    }
    //printk(PART_TIP "VMSpace: addr %x page %d\n", vaddrFirstPage, occupyPages);
    
    //memset(vaddr, 0, fileSize);

    /* 读取数据到内存中 */
    if (ReadFileFrom(fd, (void *)vaddr, offset, fileSize)) {
        return -1;
    }

    //memcpy(vaddr, iobuf, fileSize);

    return 0;
}

/**
 * LoadElfBinary - 加载文件镜像
 * @pathname: 文件的位置
 */
PRIVATE int LoadElfBinary(struct MemoryManager *mm, struct Elf32_Ehdr *elfHeader, int fd)
{
    struct Elf32_Phdr progHeader;
    /* 获取程序头起始偏移 */
    Elf32_Off progHeaderOffset = elfHeader->e_phoff;
    Elf32_Half progHeaderSize = elfHeader->e_phentsize;
    
    //printk(PART_TIP "prog offset %x size %d\n", progHeaderOffset, progHeaderSize);
    
    /* 遍历所有程序头 */
    uint32_t progIdx = 0;
    while (progIdx < elfHeader->e_phnum) {
        memset(&progHeader, 0, progHeaderSize);

        /* 读取程序头 */
        if (ReadFileFrom(fd, (void *)&progHeader, progHeaderOffset, progHeaderSize)) {
            
            return -1;
        }
        /*printk(PART_TIP "read prog header off %x vaddr %x memsz %x\n", 
            progHeader.p_offset, progHeader.p_vaddr, progHeader.p_memsz);
         */
        /* 如果是可加载的段就加载到内存中 */
        if (progHeader.p_type == PT_LOAD) {
            //printk(PART_TIP "prog at %x type LOAD \n", &progHeader);
            
            /* 由于bss段不占用文件大小，但是要占用内存，
            所以这个地方我们加载的时候就加载成memsz，
            运行的时候访问未初始化的全局变量时才可以正确 */
            if (SegmentLoad(fd, progHeader.p_offset, 
                    max(progHeader.p_memsz, progHeader.p_filesz), progHeader.p_vaddr)) {
                return -1;
            }
            //printk(PART_TIP "seg load success\n");
            
            /* 如果内存占用大于文件占用，就要把内存中多余的部分置0 */
            if (progHeader.p_memsz > progHeader.p_filesz) {
                memset((void *)(progHeader.p_vaddr + progHeader.p_filesz), 0,
                    progHeader.p_memsz - progHeader.p_filesz);
                /* printk(PART_TIP "memsz(%x) > filesz(%x)\n", 
                    progHeader.p_memsz, progHeader.p_filesz);
                */
            }
            
            /* 设置段的起始和结束 */
            if (progHeader.p_flags == ELF32_PHDR_CODE) {
                mm->codeStart = progHeader.p_vaddr;
                mm->codeEnd = progHeader.p_vaddr + progHeader.p_memsz;
            } else if (progHeader.p_flags == ELF32_PHDR_DATA) {
                mm->dataStart = progHeader.p_vaddr;
                mm->dataEnd = progHeader.p_vaddr + progHeader.p_memsz;
            }
        }

        /* 指向下一个程序头 */
        progHeaderOffset += progHeaderSize;
        progIdx++;
    }
    return 0;
}

PRIVATE int InitUserHeap(struct Task *current)
{
    /* brk默认在数据的后面 */
    current->mm->brkStart = current->mm->dataEnd;
    
    /* 页对齐 */
    current->mm->brkStart = PAGE_ALIGN(current->mm->brkStart);
    current->mm->brk = current->mm->brkStart;

    return 0;
}

PRIVATE int InitUserStack(struct Task *task, struct TrapFrame *frame, 
        char **argv, int argc)
{
    /* 设置栈空间 */
    struct VMSpace *space = (struct VMSpace *)\
            kmalloc(sizeof(struct VMSpace), GFP_KERNEL);
    if (!space) {
        printk(PART_ERROR "SysExecv: kmalloc for stack space failed!\n");
        return -1; 
    }
    space->end = USER_STACK_TOP;
    space->start = USER_STACK_TOP - PAGE_SIZE;
    space->pageProt = PROT_READ | PROT_WRITE;
    space->flags = VMS_STACK;
    space->next = NULL;
    
    /* 重新映射栈 */

    /* 先分配一个物理页 */
    uint32_t paddr = AllocPage();
    if (!paddr) {
        printk(PART_ERROR "SysExecv: GetFreePage for stack space failed!\n");
        
        // 需要释放虚拟空间
        goto ToFreeSpace; 
    }
    //printk(PART_TIP "stack alloc paddr %x\n", paddr);
    
    // 进行地址链接
    if(PageTableAdd(space->start, paddr, 
             PAGE_US_U | PAGE_RW_W) == -1) {
        // 链接失败
        // 需要释放物理页
        goto ToFreePage;
    }

    //paddr = VirtualToPhysic(space->start);
    //printk(PART_TIP "stack to paddr %x\n", paddr);

    /* 插入到空间 */
    if (InsertVMSpace(task->mm, space)) {
        // 需要取消链接
        printk(PART_ERROR "SysExecv: InsertVMSpace for stack space failed!\n");
        goto ToUnlinkAddr;
    }

    task->mm->stackStart = space->start;

    /* 新进程的栈为栈顶 */
    frame->esp = (uint32_t)USER_STACK_TOP - 4;

    task->mm->argEnd = frame->esp;

    int i;
    /* 构建参数，用栈的方式传递参数，不用寄存器保存参数，再传递给main */
    if (argc != 0) {
        // 预留字符串的空间
        for (i = 0; i < argc; i++) {
            frame->esp -= (strlen(argv[i]) + 1);
        }
        // 4字节对齐
        frame->esp -= frame->esp % 4;

        // 预留argv[]
        frame->esp -= argc * sizeof(char*);
        // 预留argv
        frame->esp -= argc * sizeof(char**);
        // 预留argc
        frame->esp -= sizeof(uint32);

        task->mm->argStart = frame->esp;
    
        /* 在下面把参数写入到栈中，可以直接在main函数中使用 */
        
        uint32_t top = frame->esp;
        
        // argc
        *(uint32_t *)top = argc;
        top += sizeof(uint32_t);

        // argv
        *(uint32_t *)top = top + sizeof(char **);
        top += sizeof(uint32_t);

        // 指向指针数组

        // argv[]
        char** _argv = (char **) top;

        // 指向参数值
        char* p = (char *) top + sizeof(char *) * argc;

        /* 把每一个参数值的首地址给指针数组 */
        for (i = 0; i < argc; i++) {
            /* 首地址 */
            _argv[i] = p;
            /* 复制参数值字符串 */
            strcpy(p, argv[i]);
            /* 指向下一个字符串 */
            p += (strlen(p) + 1);
        }
    } else {
        /* 把参数设置为空 */

        // 预留argv
        frame->esp -= argc * sizeof(char**);
        // 预留argc
        frame->esp -= sizeof(uint32);

        task->mm->argStart = frame->esp;
        
        /* 在下面把参数写入到栈中，可以直接在main函数中使用 */
        
        uint32_t top = frame->esp;
        
        // argc
        *(uint32_t *)top = 0;
        top += sizeof(uint32_t);

        // argv
        *(uint32_t *)top = 0;
        top += sizeof(uint32_t);
    }
    
    return 0;
ToUnlinkAddr:
    // 取消页链接
    RemoveFromPageTable(space->start);
ToFreePage:
    // 释放物理页
    FreePages(paddr);
ToFreeSpace:
    // 释放虚拟空间
    kfree(space);
    
    return -1;
}

PRIVATE void ResetRegisters(struct TrapFrame *frame)
{
    /* 数据段 */
    frame->ds = frame->es = \
    frame->fs = frame->gs = USER_DATA_SEL; 

    /* 代码段 */
    frame->cs = USER_CODE_SEL;

    /* 栈段 */
    frame->ss = USER_STACK_SEL;

    /* 设置通用寄存器的值 */
    frame->edi = frame->esi = \
    frame->ebp = frame->espDummy = 0;

    frame->eax = frame->ebx = \
    frame->ecx = frame->edx = 0;

    /* 为了能让程序首次运行，这里需要设置eflags的值 */
    frame->eflags = (EFLAGS_MBS | EFLAGS_IF_1 | EFLAGS_IOPL_0);
}

/**
 * MakeArguments - 把参数重新制作到一个缓冲区
 * @buf: 制作到的缓冲区
 * @argv: 参数
 * 
 * 由于用户空间将被释放，所以在这里我们先把参数制作到内核缓冲
 */
PRIVATE int MakeArguments(char *buf, char **argv)
{
	//make arguments
	uint32_t argc = 0;
	//把参数放到栈中去
	char *argStack = (char *)buf;
	if(argv != NULL){
		//计算参数
		while (argv[argc]) {  
			argc++;
		}
		//printk("argc %d\n", argc);
		//构建整个栈
		
		//临时字符串
		int strStack[MAX_STACK_ARGC/4];
		
		int stackLen = 0;
		//先预留出字符串的指针
		int i;
		for(i = 0; i < argc; i++){
			stackLen += 4;
		}
		//printk("stackLen %d\n", stackLen);
		int idx, len;
		
		for(idx = 0; idx < argc; idx++){
            /* 获取字符串长度 */
			len = strlen(argv[idx]);
            /* 复制到缓存中 */
			memcpy(argStack + stackLen, argv[idx], len);
            /* 设置数组指针 */
			strStack[idx] = (int )(argStack + stackLen);
			//printk("str ptr %x\n",strStack[idx]);
            /* 在字符串最后添加'\0' */
			argStack[stackLen + len + 1] = '\0';
            /* 长度加上一个结束符长度 */
			stackLen += len + 1;
			//printk("len %d pos %d\n",len, stackLen);
		}
		//转换成int指针   
		int *p = (int *)argStack;
		//重新填写地址
		for(idx = 0; idx < argc; idx++){
            /* 写入数组 */
			p[idx] = strStack[idx];
			//printk("ptr %x\n",p[idx]);
		}
		//p[argc] = NULL;
		//指向地址
		//char **argv2 = (char **)argStack;
		
		//argv2[argc] = NULL;
		//打印参数
		/*for (i = 0; i < argc; i++) {  
			printk("args:%s\n",argv2[i]);
		} */
		
	}
	return argc;
}


/**
 * SysExecv - 用新的镜像替换原来的镜像
 * @path: 文件的路径
 * @argv: 参数指针数组
 * 
 * 执行后，新的文件生效，程序重获新生
 * 失败返回-1，成功就执行新的镜像
 */
PUBLIC int SysExecv(const char *path, const char *argv[])
{
    //printk(PART_TIP "execv: path %s\n", path);
    int ret = 0;
    /* 1.读取文件 */
    /* 打开文件 */
    int fd = SysOpen(path, O_RDONLY);
    if (fd == -1) {
        printk(PART_ERROR "SysExecv: open file %s failed!\n", path);
        return -1;
    }
    // printk("open sucess!");
    struct stat fstat;
    if(SysStat(path, &fstat)) {
        printk(PART_ERROR "SysExecv: fstat %s failed!\n", path);
        return -1;
    }
 
    //printk("file info:\n size:%d inode:%d\n", fstat.st_size, fstat.st_ino);

    /* 2.读取elf头 */
    struct Elf32_Ehdr elfHeader;

    memset(&elfHeader, 0, sizeof(struct Elf32_Ehdr));
    
    /* 读取elf头部分，出错就跳到ToEnd关闭文件 */
    /*if (IoStreamRead(fd, &elfHeader, sizeof(struct Elf32_Ehdr)) !=\
            sizeof(struct Elf32_Ehdr)) {
        ret = -1;
        printk(PART_ERROR "SysExecv: read elf header failed!\n");
        goto ToEnd;
    }*/
    //ReadFileFrom(fd, &elfHeader, 0, sizeof(struct Elf32_Ehdr));
    if (ReadFileFrom(fd, &elfHeader, 0, sizeof(struct Elf32_Ehdr))) {
        ret = -1;
        printk(PART_ERROR "SysExecv: read elf header failed!\n");
        goto ToEnd;
    }

    //printk(PART_TIP "read elfHeader\n");
    /* 3.检验elf头 */
    /* 检验elf头，看它是否为一个elf格式的程序 */
    if (memcmp(elfHeader.e_ident, "\177ELF\1\1\1", 7) || \
        elfHeader.e_type != 2 || \
        elfHeader.e_machine != 3 || \
        elfHeader.e_version != 1 || \
        elfHeader.e_phnum > 1024 || \
        elfHeader.e_phentsize != sizeof(struct Elf32_Phdr)) {
        
        /* 头文件检测出错 */
        printk(PART_ERROR "SysExecv: it is not a elf format file!\n", path);
        
        ret = -1;
        goto ToEnd;
    }
    
    struct Task *current = CurrentTask();

    /* 4.解析参数
    后面可能释放内存资源，导致参数消失，
    所以要在释放资源之前解析参数
    */
        
    /* 当argv不为NULL才解析
    先存放到一个缓冲中
     */
    
    /*if (argv != NULL) {        
       
        while (argv[argc]) {
            argc++;
        }
        printk("argc %d \n", argc);
    }*/
    /* 分配空间来保存参数 */
    char *argBuf = (char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (argBuf == NULL) {
        printk(PART_ERROR "SysExecv: kmalloc for arg buf failed!\n");
        
        ret = -1;
        goto ToEnd;
    }
    memset(argBuf, 0, PAGE_SIZE);
    int argc = MakeArguments(argBuf, (char **)argv);

    //printk("argc %d \n", argc);

    /* 5.先把之前的资源释放
    当再次映射的时候才能保证数据正确
    */
    /* 由于释放资源会把进程空间传入的名字清空，所以这里做备份 */
    char name[MAX_TASK_NAMELEN];
    memset(name, 0, MAX_TASK_NAMELEN);
    strcpy(name, path);
    
    /* 只释放占用的栈和堆，而代码，数据，bss都不释放（当进程重新加载时可以提高速度） */
    MemoryManagerRelease(current->mm, VMS_STACK | VMS_HEAP);
    
    //printk("start load");
    /* 6.加载程序段 */
    if (LoadElfBinary(current->mm, &elfHeader, fd)) {
        printk(PART_ERROR "SysExecv: load elf binary failed!\n");
        
        ret = -1;
        goto ToEnd;
    }
    
    //printk("end load\n");
    /* 7.设置中断栈 */
    
    /* 构建新的中断栈 */
    struct TrapFrame *frame = (struct TrapFrame *)\
        ((unsigned int)current + PAGE_SIZE - sizeof(struct TrapFrame));
    
    if(InitUserStack(current, frame, (char **)argBuf, argc)){
        ret = -1;
        goto ToEnd;
    }

    if(InitUserHeap(current)){
        ret = -1;
        goto ToEnd;
    }
    
    /* 8.修改寄存器 */
    ResetRegisters(frame);
    
    /* 传递参数 */
    frame->ebx = (uint32_t)argv;
    frame->ecx = argc;
    frame->eip = (uint32_t)elfHeader.e_entry;

    /* 9.设置其他内容 */
    /*
    printk(PART_TIP "task %s pid %d execv path %s", 
        current->name, current->pid, path
    ); */
    
    /* 修改进程的名字 */
    memset(current->name, 0, MAX_TASK_NAMELEN);
    /* 这里复制前面备份好的名字 */
    strcpy(current->name, name);
    
    /*
    printk(PART_TIP "VMspace->\n");
    printk(PART_TIP "code start:%x end:%x data start:%x end:%x\n", 
        current->mm->codeStart, current->mm->codeEnd, current->mm->dataStart, current->mm->dataEnd
    );
    
    printk(PART_TIP "brk start and end:%x\n", current->mm->brkStart);
    printk(PART_TIP "stack start and end:%x\n", current->mm->stackStart);
    printk(PART_TIP "arg start:%x end:%x\n", current->mm->argStart, current->mm->argEnd);
          */
    //Spin("test");

    //printk(PART_TIP "exec int the end!\n");
    /* 10.命运裁决，是返回还是运行 */
ToEnd:
    SysClose(fd);
    // 释放参数缓冲
    kfree(argBuf);

    /* 返回值为-1就返回-1，其实还应该释放之前分配的数据 */
    if (ret)
        return -1;
    /* 没有错误，切换到进程空间运行 */
    SwitchToUser(frame);
    /* 这里是不需要返回的，但为了让编译器不发出提醒，就写一个返回值 */
    return 0;
}
