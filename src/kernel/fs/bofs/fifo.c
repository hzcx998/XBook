/*
 * file:		kernel/fs/bofs/fifo.c
 * auther:		Jason Hu
 * time:		2019/12/15
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <book/device.h>
#include <book/ioqueue.h>
#include <book/signal.h>

#include <lib/string.h>
#include <lib/math.h>

#include <fs/bofs/inode.h>
#include <fs/bofs/file.h>
#include <fs/bofs/fifo.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/pipe.h>

/**
 * BOFS_CreateNewFifo - 创建一个新的命名管道
 * @sb: 超级块儿
 * @parentDir: 父目录
 * @childDir: 子目录
 * @name: 命名管道的名字
 * 
 * 在父目录中创建一个命名管道文件
 * @return: 成功返回-1，失败返回0
 */
PRIVATE int BOFS_CreateNewFifo(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir,
    char *name,
    mode_t mode)
{
	/**
	 * 1.分配节点ID
	 * 2.创建节点
	 * 3.创建目录项
     * 4.修改节点数据
	 */
	struct BOFS_Inode inode;
	
	int inodeID = BOFS_AllocBitmap(sb, BOFS_BMT_INODE, 1); 
	if (inodeID == -1) {
		printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
		return -1;
	}
	///printk("alloc inode id %d\n", inodeID);
	
	/* 把节点id同步到磁盘 */
	BOFS_SyncBitmap(sb, BOFS_BMT_INODE, inodeID);

    //printk("BOFS_SyncBitmap\n");
	BOFS_CreateInode(&inode, inodeID, mode,
		0, sb->devno);    
    //printk("BOFS_CreateInode\n");
	
	BOFS_CreateDirEntry(childDir, inodeID, BOFS_FILE_TYPE_FIFO, name);

    struct BOFS_Pipe *pipe = kmalloc(sizeof(struct BOFS_Pipe), GFP_KERNEL);
    if (pipe == NULL) 
        return -1;
    
    if (BOFS_PipeInit(pipe)) {
        kfree(pipe);
        return -1;
    }

    /* 第一个数据区保存缓冲区首地址 */
    inode.blocks[0] = (unsigned int)pipe;

	BOFS_SyncInode(&inode, sb);

    //BOFS_DumpDirEntry(childDir);
	int result = BOFS_SyncDirEntry(parentDir, childDir, sb);
	
    //BOFS_DumpDirEntry(parentDir);

    //printk("sync result:%d\n", result);
    //printk("BOFS_SyncDirEntry\n");
	if(result > 0){	//successed
		if(result == 1){
			/* 当创建的目录不是使用的无效的目录项时才修改父目录的大小 */
			BOFS_LoadInodeByID(&inode, parentDir->inode, sb);
			
			inode.size += sizeof(struct BOFS_DirEntry);
			BOFS_SyncInode(&inode, sb);
            //printk("change parent size!");

           // BOFS_DumpInode(&inode);
		}
		return 0;
	}
    printk("sync child dir failed!\n");
	return -1;
}

/**
 * BOFS_MakeTask - 创建一个任务文件
 * @pathname: 路径名
 * @pid: 进程pid
 * @sb: 超级块
 * 
 * 创建一个进程文件，根据系统中的进程信息，创建到磁盘上。
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int BOFS_MakeFifo(const char *pathname, mode_t mode, struct BOFS_SuperBlock *sb)
{
	int i, j;
	
	char *path = (char *)pathname;

	char *p = (char *)path;
	
	char name[BOFS_NAME_LEN];   
	
	struct BOFS_DirEntry parentDir, childDir;

    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("mkfifo: bad path!\n");
		return -1;
	}

	char depth = 0;
	//计算一共有多少个路径/ 
	while(*p){
		if(*p == '/'){
			depth++;
		}
		p++;
	}
	
	p = (char *)path;

    /* 根据深度进行检测 */
	for(i = 0; i < depth; i++){
		//跳过目录分割符'/'
        p++;

		/*----读取名字----*/

        /* 准备获取目录名字 */
		memset(name, 0, BOFS_NAME_LEN);
		j = 0;

        /* 如果没有遇到'/'就一直获取字符 */
		while(*p != '/' && j < BOFS_NAME_LEN){
			name[j] = *p;
			j++;
			p++;
		}
		//printk("dir name is %s\n", name);
		/*----判断是否没有名字----*/
		
        /* 如果'/'后面没有名字，就直接返回 */
		if(name[0] == 0){
			/* 创建的时候，如果没有指定名字，就只能返回。 */
			return -1;
		}
		/* 如果是在根目录下面查找，那么就需要把根目录数据复制给父目录 */
		if(i == 0){
			memcpy(&parentDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
		}
		/* 搜索子目录 */
		if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){
			/* 找到后，就把子目录转换成父目录，等待下一次进行搜索 */
			memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));
			//printk("BOFS_MakeDir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);
			/* 如果文件已经存在，并且位于末尾，说明我们想要创建一个已经存在的目录，返回失败 */
			if(i == depth - 1){
				printk("mkfifo: path %s has exist, can't create it again!\n", path);
				return -1;
			}
		}else{
			/* 没找到，并且是在路径末尾 */
			if(i == depth - 1){
				/* 创建设备 */
                //printk("new device file:%s type:%d devno:%x\n", name, type, devno);
                /* 设置模式 */
                
                /* 在父目录中创建一个设备 */	
                if (BOFS_CreateNewFifo(sb, &parentDir, &childDir, name, mode)) {
                    return -1;
                }
			}else{
				printk("mkfifo:can't create path:%s name:%s\n", pathname, name);
				return -1;
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}

/**
 * BOFS_FifoOpen - 打开管道
 * 
 * 如果是写打开，没有进程读取，就阻塞，直到有进程读打开
 * 如果是读打开，没有进程写入，就阻塞，直到有进程写打开
 * 
 */
PUBLIC int BOFS_FifoOpen(int fd, flags_t flags)
{
    int globalFd = FdLocal2Global(fd);
    struct BOFS_FileDescriptor *file = BOFS_GetFileByFD(globalFd);
    /*
    约定，blocks[1]是读引用计数
    约定，blocks[2]是写引用计数
    但是，没有原子操作，所以需要关闭中断保证原子操作
    */
    unsigned long eflags = InterruptSave();
    
    struct BOFS_Pipe *pipe = (struct BOFS_Pipe *)file->inode->blocks[0];

    struct Task *task, *nextTask;

    struct Task *cur = CurrentTask();

    /* 读打开 */
    if (flags & BOFS_O_RDONLY) {
        /* 添加到读任务表 */
        if (BOFS_PipeRecordReadTask(pipe, cur->pid)) {
            /* 没有足够空间 */
            return -1;
        }

        /* 读引用 */
        AtomicInc(&pipe->readReference); 

        /* 如果没有写，那么就会阻塞自己 */
        if (AtomicGet(&pipe->writeReference) == 0) {
            //printk("fifo open with read, no writer, block me.\n");

            /* 等待队列头部 */
            ListAdd(&cur->list, &pipe->waitList);
            InterruptRestore(eflags);
            /* 阻塞自己 */
            TaskBlock(TASK_BLOCKED);

            //printk("fifo open with read, has writer, unblock me.\n");
            /* 唤醒后获取cpu占用 */
            eflags = InterruptSave();
        }

        /* 读打开的时候，发现之前有写进程已经被阻塞，那么就唤醒被阻塞的进程 */
        if (!ListEmpty(&pipe->waitList)) {
            /* 唤醒所有阻塞的任务 */
            ListForEachOwnerSafe(task, nextTask, &pipe->waitList, list) {
                //printk("fifo open with read, has waiter, unblock %s.\n", task->name);
                /* 脱离链表 */
                ListDel(&task->list);
                
                TaskUnblock(task);
            }
        }
    } else if (flags & BOFS_O_WRONLY) {
        /* 添加到读任务表 */
        if (BOFS_PipeRecordWriteTask(pipe, cur->pid)) {
            /* 没有足够空间 */
            return -1;
        }

        /* 写引用 */
        AtomicInc(&pipe->writeReference); 

        /* 如果没有读，那么就会阻塞自己 */
        if (AtomicGet(&pipe->readReference) == 0) {
            //printk("fifo open with write, no reader, block me.\n");
            /* 等待队列头部 */
            
            ListAdd(&CurrentTask()->list, &pipe->waitList);
            InterruptRestore(eflags);
            /* 阻塞自己 */
            TaskBlock(TASK_BLOCKED);

            //printk("fifo open with write, has reader, unblock me.\n");
            
            /* 唤醒后获取cpu占用 */
            eflags = InterruptSave();
        }

        /* 写打开的时候，发现之前有读进程已经被阻塞，那么就唤醒被阻塞的进程 */
        if (!ListEmpty(&pipe->waitList)) {
            /* 唤醒所有阻塞的任务 */
            ListForEachOwnerSafe(task, nextTask, &pipe->waitList, list) {
                //printk("fifo open with write, has waiter, unblock %s.\n", task->name);
                /* 脱离链表 */
                ListDel(&task->list);
                
                TaskUnblock(task);
            }
        }
    } else {
        printk("fifo open flags error!\n");

        InterruptRestore(eflags); 
        return -1;
    }
    
    InterruptRestore(eflags); 
    return 0;
}

/**
 * BOFS_FifoRead - 从管道读取
 * 
 * 
 */
PUBLIC int BOFS_FifoRead(struct BOFS_FileDescriptor *file, void *buffer, size_t count)
{
    /*
    1.判断写端是否已经关闭
    2.判断是否有数据
    */
    struct BOFS_Pipe *pipe = (struct BOFS_Pipe *)file->inode->blocks[0];

    unsigned char *buf = (unsigned char *)buffer;
    int readBytes = 0;
    int len = 0;
    /* 判断写端是否关闭 */
    if (AtomicGet(&pipe->writeReference) > 0) {
        //printk("fifo read has writer\n");
        
        /* 尝试获取一个字符，如果成功，继续往后，不然就会阻塞 */
        *buf++ = IoQueueGet(&pipe->ioqueue);
        readBytes++;
        
        /* 现在缓冲区有数据，并且已经被读取了一个，
        获取较小的数据量.
        已经在前面获取一个字符，所以是count-1
         */
        len = MIN(count - 1, IO_QUEUE_LENGTH(&pipe->ioqueue));

        while (len > 0) {
            //printk("<%c>", *(buf-1));
            /* 从缓冲中读取数据 */
            *buf++ = IoQueueGet(&pipe->ioqueue);
            readBytes++;
            len--;
        }
        
        //printk("fifo read buf %s\n", (char *)buffer);

    } else {
        /* 写端已经关闭 */
        //printk("fifo read no writer\n");

        /* 判断数据量，返回已有的数据量，如果为0，返回0 */
        len = MIN(count, IO_QUEUE_LENGTH(&pipe->ioqueue));
        while (len > 0) {
            /* 从缓冲中读取数据 */
            *buf++ = IoQueueGet(&pipe->ioqueue);
            //printk("<%c>", *(buf-1));
            readBytes++;
            len--;
        }
    }

    //printk("fifo read %d bytes\n", readBytes);
    return readBytes;
}

PUBLIC int BOFS_FifoWrite(struct BOFS_FileDescriptor *file, void *buffer, size_t count)
{
    /*
    1.判断读端是否已经关闭
    2.判断是否数据已满
    */
    struct BOFS_Pipe *pipe = (struct BOFS_Pipe *)file->inode->blocks[0];

    unsigned char *buf = (unsigned char *)buffer;
    int writeBytes = 0;
    int len = 0;
    /* 判断读端是否关闭 */
    if (AtomicGet(&pipe->readReference) > 0) {
        //printk("fifo write has reader\n");

        /* 写入长度就是传入的长度 */
        len = count;
        while (len > 0) {
            /* 写入数据到管道 */
            IoQueuePut(&pipe->ioqueue, *buf++);
            //printk("[%c]", *(buf-1));
            writeBytes++;
            len--;
        }

        /* */
        //printk("pipe data: %s\n", pipe->ioqueue.buf);

    } else {
        /* 读端已经关闭 */
        //printk("fifo write no reader\n");

        /* 此时写入是没有意义的，所以会发出SIGPIPE信号终止进程 */
        printk(PART_ERROR "fifo write occur a SIGPIPE!");
        ForceSignal(SIGPIPE, SysGetPid());
    }

    //printk("fifo write %d bytes\n", writeBytes);

    return writeBytes;
}

PUBLIC int BOFS_FifoUpdate(struct BOFS_Pipe *pipe, struct Task *task)
{
    int retval;
    //printk("update fifo\n");
    /* 获取父进程所在的 */
    retval = BOFS_PipeInTable(pipe, task->parentPid);
    //printk("ret val %d\n", retval);
    if (retval == 0) {
        /* 在读表中 */
        AtomicInc(&pipe->readReference);

        /* 添加到读任务表 */
        if (BOFS_PipeRecordReadTask(pipe, task->pid)) {
            /* 没有足够空间 */
            return -1;
        }
        //printk("fork read ref %d\n", AtomicGet(&pipe->readReference));
    } else if (retval == 1) {
        /* 在写表中 */
        AtomicInc(&pipe->writeReference);
        //printk("write ref %d\n", AtomicGet(&pipe->writeReference));
        /* 添加到写任务表 */
        if (BOFS_PipeRecordWriteTask(pipe, task->pid)) {
            /* 没有足够空间 */
            return -1;
        }
        //printk("fork write ref %d\n", AtomicGet(&pipe->writeReference));
    } else {
        return -1;
    }
    return 0;
}

PUBLIC int BOFS_FifoClose(struct BOFS_Pipe *pipe)
{
    int retval;
    
    retval = BOFS_PipeInTable(pipe, CurrentTask()->pid);
    if (retval == -1)
        return retval;
    
    if (retval == 0) {
        /* 在读表中 */
        AtomicDec(&pipe->readReference);
        /* 从表中删除记录 */
        if (BOFS_PipeEraseReadTask(pipe, CurrentTask()->pid)) {
            /* 当前任务不再读表中 */
            return -1;
        }
        //printk("close read ref %d\n", AtomicGet(&pipe->readReference));
        
    } else {
        /* 在写表中 */
        AtomicDec(&pipe->writeReference);
        /* 从表中删除记录 */
        if (BOFS_PipeEraseWriteTask(pipe, CurrentTask()->pid)) {
            /* 当前任务不再读表中 */
            return -1;
        }
        //printk("close write ref %d\n", AtomicGet(&pipe->writeReference));
    }

    return 0;
}