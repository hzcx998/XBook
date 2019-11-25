/*
 * file:		include/fs/vfs.h
 * auther:		Jason Hu
 * time:		2019/11/24
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_VFS_H
#define _FS_VFS_H

#include <share/const.h>
#include <book/list.h>

#ifndef VFS
#define VFS
#endif

#define FILE_SYSTEM_NAME_LEN    12

/* 文件系统的操作 */
struct FileSystemOperations {
    void *(*probeFileSystem)(struct DiskPartition *part);
    void *(*makeFileSystem)(struct DiskPartition *part);
    int (*mountFileSystem)(void *);

};

/* 目录的操作 */
struct DirOperations {
    int (*makeDir)(const char *, void *);       /* @path   @private  */
    int (*removeDir)(const char *, void *);     /* @path   @private  */
    void *(*openDir)(const char *, void *);     /* @path   @private  */
    void (*closeDir)(void *);                   /* @dir */
    void *(*readDir)(void *);                   /* @dir */
    void (*rewindDir)(void *);                  /* @dir */
};

/* 目录的操作 */
struct FileOperations {
    int (*lseek)(int , off_t , unsigned char);          /* @fd   @offset @whence */
    int (*open)(const char *, unsigned int , void *);  /* @path   @mode    @private  */
    int (*close)(int );                                 /* @path   @private  */
    int (*unlink)(const char *, void *);                /* @path   @private */
    int (*write)(int , void* , size_t );                /* @fd   @buf   @count */
    int (*read)(int , void* , size_t );                 /* @fd   @buf   @count */
    int (*access)(const char * , mode_t , void *);      /* @path   @mode   @private */
    int (*getMode)(const char * , void *);              /* @path   @private */
    int (*setMode)(const char * , mode_t, void *);      /* @path   @private */
    int (*stat)(const char * , void *, void *);         /* @path   @buf  @private */
};

/*
文件系统类型描述
*/
struct FileSystemType {
    struct List list;                       /* 文件系统链表 */
    char name[FILE_SYSTEM_NAME_LEN];        /* 文件系统的名字 */
    struct FileSystemOperations *fsOpSets;    /* 文件系统操作集 */
    struct DirOperations *dirOpSets;          /* 目录操作集 */
    struct FileOperations *fileOpSets;      /* 文件操作集 */
};

#define DEFINE_FILE_SYSTEM(fsname) \
        struct FileSystemType fsname

PUBLIC int VFS_AddFileSystem(struct FileSystemType *fstype,
    char *name, 
    struct FileSystemOperations *fsOpSets);

PUBLIC int VFS_BindDirOpSets(char *name,
    struct DirOperations *dirOpSets);
PUBLIC int VFS_BindFileOpSets(char *name,
    struct DirOperations *dirOpSets);

PUBLIC void InitVFS();

#define VFS_DIR_NAME_LEN    128

/* 虚拟目录 */
struct VirDir {
    struct List list;       /* 自己所在的队列，自己是子目录 */
    struct List listHead;   /* 自己的队列，自己是父目录 */
    char name[VFS_DIR_NAME_LEN];
    void *private;          /* 目录的私有数据，指向挂在的文件系统 */
    struct FileSystemType *fstype;  /* 如果是挂载目录，就需要指定文件系统类型 */
};

#define DEFINE_VIR_DIR(dirname) \
        struct VirDir dirname

#define VFS_FILE_NAME_LEN    128

/* 虚拟文件 */
struct VirFile {
    struct List list;       /* 文件所在的队列 */
    char type;              /* 虚拟文件的类型，设备文件，管道文件，进程文件之类的 */
    char name[VFS_FILE_NAME_LEN];   
    void *private;          /* 文件的私有数据 */
};


#endif  /* _FS_VFS_H */
