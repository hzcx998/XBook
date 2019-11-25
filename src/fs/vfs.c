/*
 * file:		fs/vfs.c
 * auther:		Jason Hu
 * time:		2019/11/24
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/vmarea.h>
#include <fs/interface.h>
#include <book/blk-dev.h>
#include <fs/vfs.h>

#include <fs/bofs/super_block.h>


/* 文件系统链表头 */
LIST_HEAD(fileSystemListHead);


/**
 * VFS_FindByName - 通过文件系统名字查找文件系统类型
 * 
 */
struct FileSystemType *VFS_FindTypeByName(char *name)
{
    struct FileSystemType *fstype;
    
    ListForEachOwner(fstype, &fileSystemListHead, list) {
        if (!strcmp(fstype->name, name)) {
            return fstype;
        }
    }
    return NULL;
}

/**
 * VFS_AddFileSystem - 添加一个文件系统到VFS
 * 
 */
PUBLIC int VFS_AddFileSystem(struct FileSystemType *fstype,
    char *name, 
    struct FileSystemOperations *fsOpSets)
{
    memset(fstype->name, 0, FILE_SYSTEM_NAME_LEN);
    
    if (strlen(name) >= FILE_SYSTEM_NAME_LEN) {
        printk(PART_ERROR "file system name is too long!\n");
        return -1;
    }
    strcpy(fstype->name, name);

    fstype->fsOpSets = fsOpSets;
    
    /* 不在队列中才添加 */
    if (!ListFind(&fstype->list, &fileSystemListHead))
        ListAddTail(&fstype->list, &fileSystemListHead);
    else {
        printk(PART_ERROR "file system had existed!\n");
        return -1;
    }
    return 0;
}

PUBLIC int VFS_BindDirOpSets(char *name,
    struct DirOperations *dirOpSets)
{
    
    struct FileSystemType *fstype = VFS_FindTypeByName(name);
    
    if (fstype == NULL) {
        printk(PART_ERROR "vfs %s not found!\n", name);
        return -1;
    }
    fstype->dirOpSets = dirOpSets;
    return 0;
}


PUBLIC int VFS_BindFileOpSets(char *name,
    struct DirOperations *dirOpSets)
{
    
    struct FileSystemType *fstype = VFS_FindTypeByName(name);
    
    if (fstype == NULL) {
        printk(PART_ERROR "vfs %s not found!\n", name);
        return -1;
    }
    fstype->dirOpSets = dirOpSets;
    return 0;
}

/* 根目录 */
DEFINE_VIR_DIR(rootDir);

PUBLIC void VirDirInit(struct VirDir *dir, char *name)
{
    /* 初始化链表头，自己是父目录 */
    INIT_LIST_HEAD(&dir->listHead);

    /* 初始化链表，自己是子目录 */
    INIT_LIST_HEAD(&dir->list);

    memset(dir->name, 0, VFS_DIR_NAME_LEN);
    strcpy(dir->name, name);

    dir->private = NULL;
}

/**
 * CreateVirDir - 创建一个虚拟目录
 * @name: 目录名
 * 
 * 成功返回创建的目录，失败返回NULL
 */
PUBLIC struct VirDir *CreateVirDir(char *name)
{
    struct VirDir *dir = kmalloc(sizeof(struct VirDir), GFP_KERNEL);
    if (dir == NULL) {
        printk(PART_ERROR "kmalloc for vir dir failed!\n");
        return NULL;
    }

    VirDirInit(dir, name);

    return dir;
}


/**
 * VirDirBind - 目录绑定私有数据
 * @dir: 虚拟目录
 * @private: 私有数据
 */
PUBLIC void VirDirBind(struct VirDir *dir, void *private, struct FileSystemType *fstype)
{
    dir->private = private;
    dir->fstype = fstype;
}

/**
 * VFS_MountRootDir - 挂载根目录
 * @fsname: 要挂载的文件系统类型
 * @part: 根目录挂载的磁盘位置
 * 
 */
PUBLIC int VFS_MountRootDir(char *fsname, struct DiskPartition *part)
{
    struct FileSystemType *fstype = VFS_FindTypeByName(fsname);
    if (fstype == NULL) {
        printk(PART_ERROR "file system type %s not support!\n", fsname);
        return -1;
    }

    struct FileSystemOperations *fsOpSets = fstype->fsOpSets; 
    
    void *private = fsOpSets->probeFileSystem(part);

    /* 不存在就要创建一个文件系统 */
    if (private == NULL) {
        printk(PART_TIP "file system type %s not exist on partition.\n", fsname);
        private = fsOpSets->makeFileSystem(part);
        if (private == NULL) {
            printk(PART_ERROR "make file system %s on disk failed!\n", fsname);
            return -1;
        }
    }
    
    /* 挂载文件系统 */
    if (fsOpSets->mountFileSystem(private)) {
        printk(PART_ERROR "mount file system %s!\n", fsname);
        return -1;
    }

    /* 设置根目录的信息 */
    VirDirInit(&rootDir, "root");
    VirDirBind(&rootDir, private, fstype);
    
    return 0;
}

#define VFS_ROOT_DISK_NAME "hda"

/* 特殊的目录名 */
/*PRIVATE char specialDirName[] = {
    "dev",
    "sys",
    "mnt"
};*/

/**
 * VFS_IsValidPath - 判断是否为有效的路径
 * @path: 路径
 * 
 * 是则返回1，不是则返回0
 */
PRIVATE int VFS_IsValidPath(const char *path)
{
    char *p = (char *)path;
    
    int len = strlen(p);
    /* 先判断长度 */
    if (len == 0) {
        printk(PART_WARRING "path name len is 0!\n");
        return 0;
    }

    /* 不是根目录 */
    if (*p != '/') {
        printk(PART_WARRING "first char must be '/'!\n");
        return 0;
    }

    /* 最后一个字符是'/' */
    if (p[len-1] == '/' && len > 1) {
        printk(PART_WARRING "last char can not be '/'!\n");
        return 0;
    }

    /* 检测字符是否有效 */
    

    /* 是有效路径 */
    return 1;
}

PUBLIC int VFS_GetPathDepth(const char *path)
{
    char *p = (char *)path;
    char *q = p;
    
	char depth = 0;
    
	while(*p){
		if(*p == '/'){
            /* 如果是2个连续的//那么就只算作一个深度 */
            if ((p - q) != 1) {
			    depth++;
            }
            q = p;
		}
		p++;
	}
    return depth;
}

/**
 * VFS_PathToLastName - 解析出最终的名字
 * @pathname: 路径名
 * @buf: 储存名字的地方
 * 
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int VFS_PathToLastName(const char *path, char *buf)
{
	char *p = (char *)path;
	char depth = 0;
	char name[VFS_DIR_NAME_LEN];
	int i;
	
    /* 获取目录深度 */
	depth = VFS_GetPathDepth(path);

	//printk("depth:%d \n",depth);
	p = (char *)path;
	for(i = 0; i < depth; i++){
		memset(name, 0, VFS_DIR_NAME_LEN);
        /* 跳过第一个'/' */
		p++;

		if(strmet(p, name, '/') == 0){
			printk(PART_ERROR "name is empty!\n");
			return -1;
		}
		
        /* 找到我们想要的深度的名字 */
		if(i == depth - 1){
            strcpy(buf, name);
            buf[strlen(name)] = '\0';
			return 0;	//success
		}
	}
	return -1;
}

/**
 * VFS_PathToFirstName - 解析出第一个的名字
 * @pathname: 路径名
 * @buf: 储存名字的地方
 * 
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int VFS_PathToFirstName(const char *path, char *buf)
{
	char *p = (char *)path;
    if (strmet(p + 1, buf, '/') == 0)
        return -1;
	return 0;
}

/**
 * VFS_MakeDir - 创建一个目录
 * @path: 目录路径
 * @mode: 目录的模式
 * @vir: 虚拟的目录（1是虚拟目录，0不是虚拟目录）
 * 
 */
PUBLIC int VFS_MakeVirDir(const char *path, mode_t mode)
{
    /* 路径是否规范，是否是一个合格的路径 */
    if (!VFS_IsValidPath(path)) {
        return -1;
    }

    /* 获取一级路径名，检测是哪种类型的路径
    如果时挂载目录，就直接到挂载目录下面去创建目录
    如果不是挂载目录，就判断是否为特殊目录
    如果不是挂载目录，那么就载根目录下面创建一个目录
     */
    char firstName[VFS_DIR_NAME_LEN];
    memset(firstName, 0, VFS_DIR_NAME_LEN);
    if (VFS_PathToFirstName(path, firstName)) {
        printk(PART_ERROR "get first name failed!\n");
        return -1;
    }
    printk("path %s first %s\n", path, firstName);

    /* 检测是否存在
    虚拟目录特权级高于磁盘目录，先检测
     */
    char exist = 0;
    struct VirDir *vdir;
    ListForEachOwner(vdir, &rootDir.listHead, list) {
        if (!strcmp(vdir->name, firstName)) {
            exist = 1;
            break;
        }
    }
    if (exist) {
        printk("dir %s had exist in path %s !\n", firstName, path);
        return -1;
    }

    struct FileSystemOperations *fsOpSets = rootDir.fstype->fsOpSets;
    /* 检测目录是否存在 */



    return 0;
}

PUBLIC void InitVFS()
{
    PART_START("VFS");
    struct Device *device;
    struct DiskPartition *part;

    struct Disk *disk;
	ListForEachOwner (device, &allDeviceListHead, list) {
        printk("block device:%s\n", device->name);
	}

    disk = GetDiskByName(VFS_ROOT_DISK_NAME);
    if (disk == NULL) {
        Panic(PART_ERROR "disk %s not exist!\n", VFS_ROOT_DISK_NAME);
    }
    part = disk->part;

    if (VFS_MountRootDir("bofs", part)) {
        printk(PART_ERROR "VFS mount root dir failed!\n");
    }

    /* 创建默认目录 */
    BOFS_DumpSuperBlock(rootDir.private);
    
    char buf[VFS_DIR_NAME_LEN];

    VFS_IsValidPath("//");
    VFS_IsValidPath("/a");
    VFS_IsValidPath("//ab/asd/123as/a");
    VFS_IsValidPath("//ab/asd/");
    
    printk("%d ",VFS_GetPathDepth("//"));
    printk("%d ",VFS_GetPathDepth("/a"));
    printk("%d ",VFS_GetPathDepth("//ab/asd/123as/a"));
    printk("%d ",VFS_GetPathDepth("//ab/asd/"));
    
    VFS_MakeVirDir("/bin", 0);
    VFS_MakeVirDir("/dev", 0);
    VFS_MakeVirDir("/mnt", 0);
    


    PART_END();
}