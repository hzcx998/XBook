/*
 * file:		fs/interface.c
 * auther:		Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/vmarea.h>

#include <book/blk-dev.h>
#include <fs/bofs/bofs.h>
#include <fs/bofs/drive.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/file.h>
#include <share/file.h>

#include <fs/fs.h>
/* 同步磁盘上的数据到文件系统 */
#define SYNC_DISK_DATA

/* 数据是在软盘上的 */
#define DATA_ON_FLOPPY
#define FLOPPY_DATA_ADDR	0x80050000

/* 数据是在软盘上的 */
#define DATA_ON_IDE

/* 要写入文件系统的文件 */
#define FILE_ID 2

#if FILE_ID == 1
	#define FILE_NAME "root:/init"
	#define FILE_SECTORS 30
#elif FILE_ID == 2
	#define FILE_NAME "root:/shell"
	#define FILE_SECTORS 30
#elif FILE_ID == 3
	#define FILE_NAME "root:/test"
	#define FILE_SECTORS 50
#endif

#define FILE_SIZE (FILE_SECTORS * SECTOR_SIZE)

PRIVATE void WriteDataToFS()
{
	#ifdef SYNC_DISK_DATA
	char *buf = kmalloc(FILE_SECTORS * SECTOR_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		Panic("kmalloc for buf failed!\n");
	}
	
	/* 把文件加载到文件系统中来 */
	int fd = SysOpen(FILE_NAME, O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
		return;
    }
	
	memset(buf, 0, FILE_SECTORS * SECTOR_SIZE);
	
	/* 根据数据存在的不同形式选择不同的加载方式 */
	#ifdef DATA_ON_FLOPPY
    char *data = (char *)FLOPPY_DATA_ADDR;

    memcpy(buf, data, FILE_SIZE);

    #endif

	
	#ifdef DATA_ON_IDE

	#endif
	
	if (SysWrite(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("write failed!\n");
	}
    
	SysLseek(fd, 0, SEEK_SET);
	if (SysRead(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("read failed!\n");
	}

	kfree(buf);
	SysClose(fd);
	printk("load file sucess!\n");
    /* 主动同步到磁盘 */
    BlockSync();

    //Panic("test");
	#endif
}

PRIVATE void Test();

/**
 * InitFileSystem - 初始化文件系统
 * 
 */
PUBLIC void InitFileSystem()
{
	PART_START("File System");

    InitBoFS();
    
    Test();

    WriteDataToFS();

	PART_END();
}

PRIVATE void Test()
{
    /*SysMakeDir("root:/test");
    SysMakeDir("root:/test/mnt");
    */
    /*
    SysRemoveDir("root:/test");
    SysRemoveDir("root:/");
    */
    /*SysMakeDir("test");

    DIR *dir = SysOpenDir("test");
    if (dir == NULL) {
        Panic("open dir failed!");
    }
    printk("open\n");

    SysRewindDir(dir);

    DIRENT de;
    do {
        if (SysReadDir(dir, &de))
            break;

        printk("name:%s inode:%d type:%d\n", de.name, de.inode, de.type);
    } while (1);

    SysCloseDir(dir);

    SysChangeName("test", "test111");*/
    
    /*
    int fd = SysOpen("root:/bin3", O_CREAT | O_RDWR);
    if (fd == -1) {
        printk("file open failed!\n");
    }

    struct stat buf;
    if(!SysStat("root:/bin", &buf))
        printk("size:%d mode:%x devno:%x inode:%d\n", 
        buf.st_size, buf.st_mode, buf.st_dev, buf.st_ino);
    if(!SysStat("root:/", &buf))
        printk("size:%d mode:%x devno:%x inode:%d\n", 
        buf.st_size, buf.st_mode, buf.st_dev, buf.st_ino);

    SysClose(fd);

    //SysUnlink("root:/bin");
    if(!SysStat("root:/bin", &buf))
        printk("size:%d mode:%x devno:%x inode:%d\n", 
        buf.st_size, buf.st_mode, buf.st_dev, buf.st_ino);

    if(SysAccess("root:/bin", F_OK)) {
        printk("file not exist!\n");
    }

    if(SysAccess("root:/bin", R_OK)) {
        printk("file not readable!\n");
    }

    if(SysAccess("root:/bin", W_OK)) {
        printk("file not writeable!\n");
    }

    printk("access exe!\n");
    if(SysAccess("root:/bin", X_OK)) {
        printk("file not executable!\n");
    }

    mode_t mode = SysGetMode("root:/bin3");
    printk("mode:%x\n", mode);

    mode |= S_IEXEC;

    SysChangeMode("root:/bin3", mode);

    mode = SysGetMode("root:/bin3");
    printk("mode:%x\n", mode);
    */

    /*char name[MAX_PATH_LEN];
    memset(name, 0, MAX_PATH_LEN);
    MakeClearAbsPath("/test/", name);
    printk(name);

    MakeClearAbsPath("test/a/./b", name);
    printk(name);

    MakeClearAbsPath("../test/a/../b", name);
    printk(name);*/

    /*if (SysMakeDir("root:/qq")) 
        printk("mkdir failde!\n");
    if (SysMakeDir("tt"))
        printk("mkdir failde!\n");
    
    if (SysRemoveDir("root:/qq"))
        printk("rmdir failde!\n");
    
    if (SysRemoveDir("tt"))
        printk("rmdir failde!\n");*/
    
/*
    char path[MAX_PATH_LEN] = {0};
    SysGetCWD(path, MAX_PATH_LEN);
    printk("cwd:%s\n", path);

    SysChangeCWD("usr");
    SysChangeCWD("/a/../abc");
    SysChangeCWD("123.txt");
    
    //SysChangeCWD("../");

    SysChangeCWD("/");

    SysChangeCWD("root:/usr");
    */
    /*
    磁盘符:路径

    绝对路径：磁盘符：路径
    相对路径：路径 -> /abc， abc， ./abc

    绝对路径：root:/abc/def/a
    相对路径：a，工作目录：root:/abc/def

    临时路径：root:/abc/def/../a/./c
    
    分离：root，/abc/def/../a/./c

    清洗路径：/abc/def/../a/./c

    文件系统识别的路径：/abc/a/c
    */
}

PUBLIC int SysGetCWD(char* buf, unsigned int size)
{
    return BOFS_GetCWD(buf, size);
}

PUBLIC int SysChangeCWD(const char *pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    //printk("begin:%s process:%s finall:%s\n", pathname,  absPath, finalPath);

    /* 调用文件系统的改变工作目录 */
    return BOFS_ChangeCWD(finalPath, drive);
}

PUBLIC int SysOpen(const char *pathname, unsigned int flags)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    /* 根据文件系统接口的标志来转换成本地接口的标志 */
    
    unsigned int newFlags = 0;

    if (flags & O_RDONLY)
        newFlags |= BOFS_O_RDONLY;
    
    if (flags & O_WRONLY)
        newFlags |= BOFS_O_WRONLY;
    
    if (flags & O_RDWR)
        newFlags |= BOFS_O_RDWR;
    
    if (flags & O_CREAT)
        newFlags |= BOFS_O_CREAT;
    
    if (flags & O_TRUNC)
        newFlags |= BOFS_O_TRUNC;
    
    if (flags & O_APPEDN)
        newFlags |= BOFS_O_APPEDN;
    
    if (flags & O_EXEC)
        newFlags |= BOFS_O_EXEC;

    return BOFS_Open(finalPath, newFlags, drive->sb);
}

PUBLIC int SysLseek(int fd, unsigned int offset, char flags)
{
    /* 根据文件系统接口的标志来转换成本地接口的标志 */
    
    unsigned int newFlags = 0;
    
    if (flags == SEEK_SET)
        newFlags = BOFS_SEEK_SET;
    
    if (flags == SEEK_CUR)
        newFlags = BOFS_SEEK_CUR;
    
    if (flags == SEEK_END)
        newFlags = BOFS_SEEK_END;
    
    return BOFS_Lseek(fd, offset, newFlags);
}

PUBLIC int SysRead(int fd, void *buffer, unsigned int size)
{
    return BOFS_Read(fd, buffer, size);
}

PUBLIC int SysWrite(int fd, void *buffer, unsigned int size)
{
    return BOFS_Write(fd, buffer, size);
}

PUBLIC int SysClose(int fd)
{
	return BOFS_Close(fd);
}

PUBLIC int SysStat(const char *pathname, struct stat *buf)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    struct BOFS_Stat bstat;

    /* 获取数据 */
    if (BOFS_Stat(finalPath, &bstat, drive->sb)) {
        return -1;
    }

    mode_t newMode = 0;

    if (bstat.mode & BOFS_IMODE_R) {
        newMode |= S_IRUSR;
    }
    if (bstat.mode & BOFS_IMODE_W) {
        newMode |= S_IWUSR;
    }
    if (bstat.mode & BOFS_IMODE_X) {
        newMode |= S_IXUSR;
    }
    if (bstat.mode & BOFS_IMODE_F) {
        newMode |= S_IFREG;
    }
    if (bstat.mode & BOFS_IMODE_D) {
        newMode |= S_IFDIR;
    }

    /* 转换数据 */
    buf->st_size = bstat.size;
    buf->st_mode = newMode;
    buf->st_ino = bstat.inode;
    buf->st_dev = bstat.devno;
    buf->st_rdev = bstat.devno2;
    buf->st_ctime = bstat.crttime;
    buf->st_mtime = bstat.mdftime;
    buf->st_atime = bstat.acstime;
    buf->st_blksize = bstat.blksize;
    buf->st_blocks = bstat.blocks;
    return 0;
}

PUBLIC int SysRemove(const char *pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    return BOFS_Remove(finalPath, drive->sb);
}

PUBLIC int SysIoctl(int fd, int cmd, int arg)
{

	return 0;
}

PUBLIC int SysFcntl(int fd, int cmd, int arg)
{
    
	return 0;
}

PUBLIC int SysFsync(int fd)
{
    return BOFS_Fsync(fd);
}


PUBLIC int SysAccess(const char *pathname, mode_t mode)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    mode_t newMode = 0;
    
    switch (mode) {
    case F_OK:
        newMode = BOFS_F_OK;
        break;
    case X_OK:
        newMode = BOFS_X_OK;
        break;
    case W_OK:
        newMode = BOFS_W_OK;
        break;    
    case R_OK:
        newMode = BOFS_R_OK;
        break;    
    }
    
    return BOFS_Access(finalPath, newMode, drive->sb);
}

PUBLIC int SysGetMode(const char* pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
	mode_t mode = BOFS_GetMode(finalPath, drive->sb);
    //printk("get the mode:%x\n", mode);

    mode_t newMode = 0;

    if (mode & BOFS_IMODE_R) {
        newMode |= S_IRUSR;
    }
    if (mode & BOFS_IMODE_W) {
        newMode |= S_IWUSR;
    }
    if (mode & BOFS_IMODE_X) {
        newMode |= S_IXUSR;
    }
    if (mode & BOFS_IMODE_F) {
        newMode |= S_IFREG;
    }
    if (mode & BOFS_IMODE_D) {
        newMode |= S_IFDIR;
    }
    //printk("get the new mode:%x\n", newMode);
    
    return newMode;
}

PUBLIC int SysChangeMode(const char* pathname, int mode)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    mode_t newMode = 0;

    if (mode & S_IRUSR) {
        newMode |= BOFS_IMODE_R;
    }
    if (mode & S_IWUSR) {
        newMode |= BOFS_IMODE_W;
    }
    if (mode & S_IXUSR) {
        newMode |= BOFS_IMODE_X;
    }

    if (mode & S_IFREG) {
        newMode |= BOFS_IMODE_F;
    }

    if (mode & S_IFDIR) {
        newMode |= BOFS_IMODE_D;
    }

	return BOFS_ChangeMode(finalPath, newMode, drive->sb);
}

PUBLIC int SysMakeDir(const char *pathname)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);

    return BOFS_MakeDir(finalPath, drive->sb);
}

PUBLIC int SysRemoveDir(const char *pathname)
{
	char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    return BOFS_RemoveDir(finalPath, drive->sb);
}

PUBLIC int SysChangeName(const char *pathname, char *name)
{
    char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return -1;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    
    BOFS_ResetName(finalPath, name, drive->sb);
	return 0;
}

PUBLIC DIR *SysOpenDir(const char *pathname)
{
	char absPath[MAX_PATH_LEN] = {0};

    /* 生成绝对路径 */
    BOFS_MakeAbsPath(pathname, absPath);
    
    /* 获取磁盘符 */
    struct BOFS_Drive *drive = BOFS_GetDriveByPath(absPath);
    if (drive == NULL) {
        printk("get drive by path %s failed!\n", absPath);
        return NULL;
    }

    char finalPath[MAX_PATH_LEN] = {0};
    /* 移动到第一个'/'处，也就是根目录处 */
    char *path = strchr(absPath, '/');
    /*现在获得了绝对路径，需要清理路径中的.和..*/ 
    BOFS_WashPath(path, finalPath);
    //printk("finall path:%s\n", finalPath);
    struct BOFS_Dir *dir = BOFS_OpenDir(finalPath, drive->sb);
    if (dir == NULL) {
        return NULL;
    } else {
        return (DIR *)dir;
    }
}

PUBLIC void SysCloseDir(DIR *dir)
{
    BOFS_CloseDir((struct BOFS_Dir *)dir);
}

PUBLIC int SysReadDir(DIR *dir, DIRENT *buf)
{
    struct BOFS_Dir *bdir = (struct BOFS_Dir *)dir;
    off_t off = bdir->pos;
    
    struct BOFS_DirEntry *de = BOFS_ReadDir(bdir);

    /* 读取结束 */
    if (de == NULL)
        return -1;
    
    /* 填写数据 */
    memset(buf->name, 0, NAME_MAX + 1);
    strcpy(buf->name, de->name);
    buf->type = de->type;
    buf->inode = de->inode;
    buf->off = off;

	return 0;
}

PUBLIC void SysRewindDir(DIR *dir)
{
	BOFS_RewindDir((struct BOFS_Dir *)dir);
}