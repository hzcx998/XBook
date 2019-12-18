#include <stdio.h>
#include <stdlib.h>
#include <share/string.h>
#include <share/const.h>
#include <unistd.h>
#include <conio.h>

#include "cmd.h"
#include "bosh.h"

/* 清屏命令后不需要换行 */
extern int shell_next_line;
extern char cwd_cache[];


int cmd_kill(uint32_t argc, char** argv)
{
	if(argc < 2){
		printf("kill: too few arguments.\n");	
		return -1;
	}

	//默认 argv[1]是进程的pid
	
	//把传递过来的字符串转换成数字
    /*
	int pid = atoi(argv[1]);
	printf("kill: arg %s pid %d.\n", argv[1], pid);	
    */
    printf("buildin kill: not support now!\n");	
    /*
	if (thread_kill(pid) == -1) {
		printf("kill: pid %d failed.\n", pid);	
	}*/
	return 0;
}


void cmd_mm(uint32_t argc, char** argv)
{
    printf("buildin mm: not support now!\n");	
    
	/*int mem_size, mem_free;
	get_memory(&mem_size, &mem_free);
	printf("physical memory size: %d bytes ( %d mb)\nphysical memory free: %d bytes ( %d mb)\n",\
	mem_size, mem_size/(1024*1024), mem_free, mem_free/(1024*1024));
	*/
}

void cmd_exit(uint32_t argc, char** argv)
{
	//关闭进程
	exit(0);
}

void cmd_reboot(uint32_t argc, char** argv)
{
    printf("buildin reboot: not support now!\n");	
    
	//reboot(BOOT_KBD);
}

int cmd_copy(uint32_t argc, char** argv)
{
	if(argc < 3){
		printf("copy: command syntax is incorrect.\n");	
		return -1;
	}

    

	if(!strcmp(argv[1], ".") || !strcmp(argv[1], "..")){
		printf("copy: src pathnamne can't be . or .. \n");	
		return -1;
	}
	if(!strcmp(argv[2], ".") || !strcmp(argv[2], "..")){
		printf("copy: dst pathname can't be . or .. \n");	
		return -1;
	}

    printf("buildin copy: not support now!\n");	
    
    /*
	if(!copy(final_path, final_path2)){
		printf("copy: %s to", final_path);	
		printf(" %s sucess!\n", final_path2);
		return 0;
	}else{
		printf("copy: %s to", final_path);	
		printf(" %s faild!\n", final_path2);
		return -1;
	}*/
    return 0;
}

int cmd_move(uint32_t argc, char** argv)
{
	//printf("echo: arguments %d\n", argc);
	if(argc < 3){
		printf("move: command syntax is incorrect.\n");	
		return -1;
	}

	if(!strcmp(argv[1], ".") || !strcmp(argv[1], "..")){
		printf("move: src pathnamne can't be . or .. \n");	
		return -1;
	}
	if(!strcmp(argv[2], ".") || !strcmp(argv[2], "..")){
		printf("move: dst pathname can't be . or .. \n");	
		return -1;
	}

    printf("buildin move: not support now!\n");	
    
	/*if(!move(final_path, final_path2)){
		printf("move: %s to", final_path);	
		printf(" %s sucess!\n", final_path2);
		return 0;
	}else{
		printf("move: %s to", final_path);	
		printf(" %s faild!\n", final_path2);
		return -1;
	}*/
    return 0;
}

int cmd_rename(uint32_t argc, char** argv)
{
	//printf("echo: arguments %d\n", argc);
	if(argc < 3){
		printf("rename: command syntax is incorrect.\n");	
		return -1;
	}

	if(!strcmp(argv[1], ".") || !strcmp(argv[1], "..")){
		printf("rename: pathnamne can't be . or .. \n");	
		return -1;
	}
	if(!strcmp(argv[2], ".") || !strcmp(argv[2], "..")){
		printf("rename: new name can't be . or .. \n");	
		return -1;
	}

	if(!rename(argv[1], argv[2])){
		printf("rename: %s to %s sucess!\n", argv[1], argv[2]);	
		
		return 0;
	}else{
        printf("rename: %s to %s faild!\n", argv[1], argv[2]);	
		return -1;
	}

    return 0;
}


void cmd_time(uint32_t argc, char** argv)
{
    printf("buildin time: not support now!\n");	
    
	/*struct time tm;
	gettime(&tm);
	
	printf("current time: ");
	printf("%d:",tm.hour);
	printf("%d:",tm.minute);
	printf("%d\n",tm.second);
	*/
}

void cmd_date(uint32_t argc, char** argv)
{
    printf("buildin date: not support now!\n");	
    
    /*
	struct time tm;
	gettime(&tm);
	
	printf("current date: ");
	printf("%d/",tm.year);
	printf("%d/",tm.month);
	printf("%d\n",tm.day);
	*/
}

void cmd_ver(uint32_t argc, char** argv)
{
	
	printf("\n%s ",OS_NAME);
	printf("[Version %s]\n",OS_VERSION);
	
}

void cmd_dir(uint32_t argc, char** argv)
{
	/* 只有一个参数 */
    if (argc == 1) {
        /* 列出当前工作目录所在的文件 */
        cmd_ls_sub(".", 1);
    } else {
        /*  */
        int arg_path_nr = 0;
        int arg_idx = 1;	//跳过argv[0]
        char *path = NULL;

        while(arg_idx < argc){
        
            if(arg_path_nr == 0){
                /* 获取路径 */
                path = argv[arg_idx];
                
                arg_path_nr = 1;
            }else{
                printf("ls: only support one path!\n");
                return;
            }
            arg_idx++;
        }
        if (path == NULL) { /* 没有路径就列出当前目录 */
            cmd_ls_sub(".", 1);
        } else {    /* 有路径就列出路径 */
            cmd_ls_sub(path, 1);
        }
    }
}
/*
cat: print a file
*/
int cmd_cat(int argc, char *argv[])
{
	//printf("argc: %d\n", argc);
	if(argc == 1){	//只有一个参数，自己的名字，退出
		printf("cat: please input filename!\n");
		return 0;
	}
	if(argc > 2){
		printf("cat: only support 2 argument!\n");
		return -1;
	}
	
    const char *path = (const char *)argv[1];

	int fd = open(path, O_RDONLY);
	if(fd == -1){
		printf("cat: fd %d error\n", fd);
		return 0;
	}
	
	struct stat fstat;
	stat(path, &fstat);
	
	char *buf = (char *)malloc(fstat.st_size);
	
	int bytes = read(fd, buf, fstat.st_size);
	//printf("read:%d\n", bytes);
	close(fd);
	
	int i = 0;
	while(i < bytes){
		printf("%c", buf[i]);
		i++;
	}
	free(buf);
	printf("\n");
	return 0;
}

/*
touch: create a file
*/
static int cmd_touch(int argc, char *argv[])
{
	//printf("argc: %d\n", argc);
	if(argc == 1){	//只有一个参数，自己的名字，退出
		printf("touch: please input filename!\n");
		return 0;
	}
	if(argc > 2){
		printf("touch: only support 2 argument!\n");
		return -1;
	}
	
    const char *path = (const char *)argv[1];

	int fd = open(path, O_CREAT | O_RDWR);
	if(fd == -1){
		printf("touch: fd %d error\n", fd);
		return 0;
	}

	close(fd);
	return 0;
}

/*
type filename
*/
int cmd_type(int argc, char *argv[])
{
	//printf("argc: %d\n", argc);
	if(argc == 1){	//只有一个参数，自己的名字，退出
		printf("type: please input filename!\n");
		return 0;
	}
	if(argc > 2){
		printf("type: only support 2 argument!\n");
		return -1;
	}
	const char *path = (const char *)argv[1];
    
	int fd = open(path, O_RDONLY);
	if(fd == -1){
		printf("type: fd %d error\n", fd);
		return 0;
	}
	
	struct stat fstat;
	stat(path, &fstat);
	
	char *buf = (char *)malloc(fstat.st_size);
	
	int bytes = read(fd, buf, fstat.st_size);
	//printf("read:%d\n", bytes);
	close(fd);
	
	int i = 0;
	while(i < bytes){
		printf("%c", buf[i]);
		i++;
	}
	free(buf);
	
	printf("\n");
	return 0;
}

/*
echo string			//show a string
echo string>file 	//output a string to file
echo string>>file	//apend a string to file
*/
int cmd_echo(int argc, char *argv[])
{
    printf("buildin echo: not support now!\n");	
    
	return 0;
}

void cmd_help(uint32_t argc, char** argv)
{
	if(argc != 1){
		printf("help: no argument support!\n");
		return;
	}
	printf("  cat         print a file.\n");
	printf("  cls         clean screen.\n");
	printf("  cd          change current work dirctory.\n");
	printf("  copy        copy a file.\n");
	printf("  date        get current date.\n");
	printf("  dir         list files in current dirctory.\n");
	printf("  exit        exit shell.\n");
	printf("  kill        close a thread.\n");
	printf("  ls          list files in current dirctory.\n");
	printf("  lsdisk      list disk drives.\n");
	printf("  mkdir       create a dir.\n");
	printf("  mm          print memory info.\n");
	printf("  mv          move a file.\n");
	printf("  ps          print tasks.\n");
	printf("  pwd         print work directory.\n");
	printf("  rmdir       remove a dir.\n");
	printf("  rename      reset file/dirctory name.\n");
	printf("  reboot      reboot system.\n");
	printf("  rm          delete a file.\n");
	printf("  time        get current time.\n");
    printf("  touch       create a empty file.\n");
    printf("  tty         show my tty device file.\n");
	printf("  ver         show os version.\n");
}

void cmd_ps(uint32_t argc, char** argv)
{
	if(argc != 1){
		printf("ps: no argument support!\n");
		return;
	}
    
    log("");

	//printf("=====Task Info=====\n");

    //cmd_ls_sub("sys:/tsk", 1);
    /*
	struct thread *buf = thread_connect();

	do {
		if (buf != NULL) {
			if (buf->status != THREAD_UNUSED) {
				printf("name:%s  pid:%d  status:%d\n", buf->name, buf->pid, buf->status);
			}
		}
		buf = thread_getinfo(buf);
	} while (buf != NULL);*/

	//ps();
}

extern char tty_path[];
void cmd_tty(uint32_t argc, char** argv)
{
	if(argc != 1){
		printf("ps: no argument support!\n");
		return;
	}
    /* 打印tty路径 */
    printf("%s\n", tty_path);
}

void cmd_ls(uint32_t argc, char** argv)
{
    /*int i;
    for (i = 0; i < argc; i++) {
        printf("%s ",argv[i]);
    }*/

    /* 只有一个参数 */
    if (argc == 1) {
        /* 列出当前工作目录所在的文件 */
        cmd_ls_sub(".", 0);
    } else {
        /*  */
        int arg_path_nr = 0;
        int arg_idx = 1;	//跳过argv[0]
        char *path = NULL;

        int detail = 0;
        while(arg_idx < argc){
            if(argv[arg_idx][0] == '-'){//可选参数
                char *option = &argv[arg_idx][1];
                /* 有可选参数 */
                if (*option) {
                    /* 列出详细信息 */
                    if (*option == 'l') {
                        detail = 1;
                    }
                }
            } else {
                if(arg_path_nr == 0){
                    /* 获取路径 */
                    path = argv[arg_idx];
                    
                    arg_path_nr = 1;
                }else{
                    printf("ls: only support one path!\n");
                    return;
                }
            }
            arg_idx++;
        }
        if (path == NULL) { /* 没有路径就列出当前目录 */
            cmd_ls_sub(".", detail);
        } else {    /* 有路径就列出路径 */
            cmd_ls_sub(path, detail);
        }
    }

    return;
}

int cmd_cd(uint32_t argc, char** argv)
{
	//printf("pwd: argc %d\n", argc);
	if(argc > 2){
		printf("cd: only support 1 argument!\n");
		return -1;
	}
    /*int i;
    for (i = 0; i < argc; i++) {
        printf("%s",argv[i]);
    }*/

    /* 只有1个参数，是cd，那么不处理 */
    if (argc == 1) {
        return -1; 
    }
    
    char *path = argv[1];

	if(chdir(path) == -1){
		printf("cd: no such directory %s\n",argv[1]);
		return -1;
	}
	return 0;
}

void cmd_pwd(uint32_t argc, char** argv)
{
	//printf("pwd: argc %d\n", argc);
	if(argc != 1){
		printf("pwd: no argument support!\n");
		return;
	}else{
        printf("%s\n", cwd_cache);
        /*char cwdpath[MAX_PATH_LEN];
		if(!getcwd(cwdpath, MAX_PATH_LEN)){
			printf("%s\n", cwdpath);
		}else{
			printf("pwd: get current work directory failed!\n");
		}*/
	}
}

void cmd_cls(uint32_t argc, char** argv)
{
	//printf("cls: argc %d\n", argc);
	if(argc != 1){
		printf("cls: no argument support!\n");
		return;
	}

    /* 对标准输出发出清屏操作 */
    ioctl(1, TTY_CMD_CLEAN, 0);

}

int cmd_mkdir(uint32_t argc, char** argv)
{
	int ret = -1;
	if(argc != 2){
		printf("mkdir: no argument support!\n");
	}else{
    
        if(mkdir(argv[1]) == 0){
            printf("mkdir: create a dir %s success.\n", argv[1]);
            ret = 0;
        }else{
            printf("mkdir: create directory %s faild!\n", argv[1]);
        }
	}
	return ret;
}

int cmd_rmdir(uint32_t argc, char** argv)
{
	int ret = -1;
	if(argc != 2){
		printf("mkdir: no argument support!\n");
	}else{
		
        if(rmdir(argv[1]) == 0){
            printf("rmdir: remove %s success.\n", argv[1]);
            ret = 0;
        }else{
            printf("rmdir: remove %s faild!\n", argv[1]);
        }
		
	}
	return ret;
}

int cmd_rm(uint32_t argc, char** argv)
{
	int ret = -1;
	if(argc != 2){
		printf("rm: no argument support!\n");
	}else{
		
        if(remove(argv[1]) == 0){
            printf("rm: delete %s success.\n", argv[1]);
            ret = 0;
        }else{
            printf("rm: delete %s faild!\n", argv[1]);
        }
		
	}
	return ret;
}

void cmd_ls_sub(char *pathname, int detail)
{
	DIR *dir = opendir(pathname);
	if(dir == NULL){
		printf("opendir failed!\n");
        return;
	}
	rewinddir(dir);
	
	dirent de;

    char subpath[MAX_PATH_LEN];

    struct stat fstat;  
    char type;
    char attrR, attrW, attrX;   /* 读写，执行属性 */ 

    do {
        if (readdir(dir, &de)) {
            break;
        }
        
        if (de.type != DIR_INVALID) {
            if (detail) {
                /* 列出详细信息 */
                switch (de.type) {
                case DIR_NORMAL:
                    type = '-';
                    break;
                case DIR_DIRECTORY:
                    type = 'd';
                    break;
                case DIR_BLOCK:
                    type = 'b';
                    break;
                case DIR_CHAR:
                    type = 'c';
                    break;
                case DIR_NET:
                    type = 'n';
                    break;
                case DIR_TASK:
                    type = 't';
                    break;
                case DIR_FIFO:
                    type = 'f';
                    break;
                default:
                    type = 'x';
                    break;
                }
                
                memset(subpath, 0, MAX_PATH_LEN);
                /* 合并路径 */
                strcat(subpath, pathname);
                strcat(subpath, "/");
                strcat(subpath, de.name);
                    
                memset(&fstat, 0, sizeof(struct stat));
                /* 如果获取失败就获取下一个 */
                if (stat(subpath, &fstat)) {
                    continue;
                }
                
                if (fstat.st_mode & S_IREAD) {
                    attrR = 'r';
                } else {
                    attrR = '-';
                }

                if (fstat.st_mode & S_IWRITE) {
                    attrW = 'w';
                } else {
                    attrW = '-';
                }

                if (fstat.st_mode & S_IEXEC) {
                    attrX = 'x';
                } else {
                    attrX = '-';
                }
                
                /* 类型,属性，文件日期，大小，名字 */
                printf("%c%c%c%c %d/%d/%d %d:%d %d %s\n",
                    type, attrR, attrW, attrX,
                    DATA16_TO_DATE_YEA(fstat.st_mtime>>16),
                    DATA16_TO_DATE_MON(fstat.st_mtime>>16),
                    DATA16_TO_DATE_DAY(fstat.st_mtime>>16),
                    DATA16_TO_TIME_HOU(fstat.st_mtime&0xffff),
				    DATA16_TO_TIME_MIN(fstat.st_mtime&0xffff),
                    fstat.st_size, de.name);
                //printf("type:%x inode:%d name:%s\n", de.type, de.inode, de.name);
            } else {
                printf("%s ", de.name);
            }
        }
    } while (1);
	
	closedir(dir);
    if (!detail) {
        printf("\n");
    }
}

void cmd_lsdisk(uint32_t argc, char** argv)
{
	printf("=====Disk Drive=====\n");
    cmd_ls_sub("sys:/drv", 1);
	/*printf("-Drive-   -Type-\n");

	int drive_nr = 0;
	
	struct drive_s *buf = fs_drive_connect();

	do {
		if (buf != NULL) {
			if (buf->dev_id != -1) {
				printf(" %c:        %s\n", \
					buf->name[1], buf->type_name);
				drive_nr++;	
			}
			
		}
		buf = fs_drive_get(buf);

	} while (buf != NULL);

	printf("current disk number is %d.\n", drive_nr);
    */
}


/**
 * do_buildin_cmd - 执行内建命令
 */
int buildin_cmd(int cmd_argc, char **cmd_argv)
{
    int retval = 0;

    if(!strcmp("cls", cmd_argv[0])){
        cmd_cls(cmd_argc, cmd_argv);
        shell_next_line = 0;
    }else if(!strcmp("pwd", cmd_argv[0])){
        cmd_pwd(cmd_argc, cmd_argv);
    }else if(!strcmp("cd", cmd_argv[0])){
        if(cmd_cd(cmd_argc, cmd_argv) == 0){
            /* 改变工作目录后需要刷新一次工作目录 */
            memset(cwd_cache,0, MAX_PATH_LEN);
            getcwd(cwd_cache, MAX_PATH_LEN);
        }
    }else if(!strcmp("ls", cmd_argv[0])){
        cmd_ls(cmd_argc, cmd_argv);
        
    }else if(!strcmp("ps", cmd_argv[0])){
        cmd_ps(cmd_argc, cmd_argv);
        
    }else if(!strcmp("help", cmd_argv[0])){
        cmd_help(cmd_argc, cmd_argv);
        
    }else if(!strcmp("mkdir", cmd_argv[0])){
        cmd_mkdir(cmd_argc, cmd_argv);
        
    }else if(!strcmp("rmdir", cmd_argv[0])){
        cmd_rmdir(cmd_argc, cmd_argv);
        
    }else if(!strcmp("rm", cmd_argv[0])){
        cmd_rm(cmd_argc, cmd_argv);
        
    }else if(!strcmp("echo", cmd_argv[0])){
        cmd_echo(cmd_argc, cmd_argv);
        
    }else if(!strcmp("type", cmd_argv[0])){
        cmd_type(cmd_argc, cmd_argv);
    }else if(!strcmp("cat", cmd_argv[0])){
        cmd_cat(cmd_argc, cmd_argv);	
    }else if(!strcmp("dir", cmd_argv[0])){
        cmd_dir(cmd_argc, cmd_argv);
    }else if(!strcmp("ver", cmd_argv[0])){
        cmd_ver(cmd_argc, cmd_argv);
    }else if(!strcmp("time", cmd_argv[0])){
        cmd_time(cmd_argc, cmd_argv);
    }else if(!strcmp("date", cmd_argv[0])){
        cmd_date(cmd_argc, cmd_argv);
    }else if(!strcmp("rename", cmd_argv[0])){
        cmd_rename(cmd_argc, cmd_argv);
    }else if(!strcmp("mv", cmd_argv[0])){
        cmd_move(cmd_argc, cmd_argv);
    }else if(!strcmp("copy", cmd_argv[0])){
        cmd_copy(cmd_argc, cmd_argv);
    }else if(!strcmp("reboot", cmd_argv[0])){
        cmd_reboot(cmd_argc, cmd_argv);
    }else if(!strcmp("exit", cmd_argv[0])){
        cmd_exit(cmd_argc, cmd_argv);
    }else if(!strcmp("mm", cmd_argv[0])){
        cmd_mm(cmd_argc, cmd_argv);
    }else if(!strcmp("lsdisk", cmd_argv[0])){
        cmd_lsdisk(cmd_argc, cmd_argv);
    }else if(!strcmp("kill", cmd_argv[0])){
        cmd_kill(cmd_argc, cmd_argv);    
    }else if(!strcmp("touch", cmd_argv[0])){
        cmd_touch(cmd_argc, cmd_argv);    
    }else if(!strcmp("tty", cmd_argv[0])){
        cmd_tty(cmd_argc, cmd_argv);
    } else {    /* 没有内建命令 */
        retval = -1;
    }
    return retval;
}
