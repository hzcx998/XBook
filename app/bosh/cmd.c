#include <stdio.h>
#include <stdlib.h>
#include <share/string.h>
#include <share/const.h>
#include <unistd.h>
#include <conio.h>
#include <signal.h>
#include <ctype.h>

#include "cmd.h"
#include "bosh.h"

/* 清屏命令后不需要换行 */
extern int shell_next_line;
extern char cwd_cache[];

/**
 * cmd_kill - kill命令
 * 1.命令格式：
 * kill [参数] [进程号]
 * 2.命令功能：
 * 发送指定的信号到相应进程。不指定型号将发送SIGTERM（15）终止指定进程。
 * 3.命令参数：
 * -l  信号，若果不加信号的编号参数，则使用“-l”参数会列出全部的信号名称
 * -s  指定发送信号
 * 
 * 4.举例：
 * kill 10          // 默认方式杀死进程10
 * kill -9 10       // 用SIGKILL杀死进程10
 * kill -SIGKILL 10 // 用SIGKILL杀死进程10
 * kill -KILL 10    // 用SIGKILL杀死进程10
 * kill -l          // 列出所有信号
 * kill -l KILL     // 列出KILL对应的信号值
 * kill -l SIGKILL  // 列出KILL对应的信号值
 * 就实现以上命令，足矣。
 * 
 */

#define MAX_SUPPORT_SIG_NR      32

/* 信号的字符串列表，用于查找信号对应的信号值 */
char *signal_table[MAX_SUPPORT_SIG_NR][2] = {
    {"NULL", "NULL"},       /* 信号对应的值 */
    {"SIGHUP", "HUP"},     /* 信号对应的值 */
    {"SIGINT", "INT"},       /* 信号对应的值 */
    {"SIGQUIT", "QUIT"},       /* 信号对应的值 */
    {"SIGILL", "ILL"},       /* 信号对应的值 */
    {"SIGTRAP", "TRAP"},       /* 信号对应的值 */
    {"SIGABRT", "ABRT"},       /* 信号对应的值 */
    {"SIGBUS", "BUS"},       /* 信号对应的值 */
    {"SIGFPE", "FPE"},       /* 信号对应的值 */
    {"SIGKILL", "KILL"},       /* 信号对应的值 */
    {"SIGUSR1", "USR1"},       /* 信号对应的值 */
    {"SIGSEGV", "SEGV"},       /* 信号对应的值 */
    {"SIGUSR2", "USR2"},       /* 信号对应的值 */
    {"SIGPIPE", "PIPE"},       /* 信号对应的值 */
    {"SIGALRM", "ALRM"},       /* 信号对应的值 */
    {"SIGTERM", "TERM"},       /* 信号对应的值 */
    {"SIGSTKFLT", "STKFLT"},       /* 信号对应的值 */
    {"SIGCHLD", "CHLD"},       /* 信号对应的值 */
    {"SIGCONT", "CONT"},       /* 信号对应的值 */
    {"SIGSTOP", "STOP"},       /* 信号对应的值 */
    {"SIGTSTP", "TSTP"},       /* 信号对应的值 */
    {"SIGTINT", "TTIN"},       /* 信号对应的值 */
    {"SIGTTOU", "TTOU"},       /* 信号对应的值 */
    {"SIGURG", "URG"},       /* 信号对应的值 */
    {"SIGXCPU", "XCPU"},       /* 信号对应的值 */
    {"SIGXFSZ", "XFSZ"},       /* 信号对应的值 */
    {"SIGVTALRM", "VTALRM"},       /* 信号对应的值 */
    {"SIGPROF", "PROF"},       /* 信号对应的值 */
    {"SIGWINCH", "WINCH"},       /* 信号对应的值 */
    {"SIGIO", "IO"},       /* 信号对应的值 */
    {"SIGPWR", "PWR"},       /* 信号对应的值 */
    {"SIGSYS", "SYS"},       /* 信号对应的值 */
};

/* 判断字符是不是纯数字 */
int isdigitstr(const char *str)
{
    const char *p = str;
               
    while (*p) {
        /* 如果有一个字符不是数字，就返回0 */
        if (!isdigit(*p)) { 
            return 0;
        }
        p++;
    }
    return 1;
}

/**
 * find_signal_in_table - 在表中查找信号
 * @name：信号名称
 * 
 * 
 * 找到返回信号值，没找到返回0
 */
int find_signal_in_table(char *name)
{
    int idx;
    int signo = -1;
    
    for (idx = 1; idx < MAX_SUPPORT_SIG_NR; idx++) {
        /* 字符串相同就找到 */
        if (!strcmp(signal_table[idx][0], name) || !strcmp(signal_table[idx][1], name)) {
            signo = idx;
            break;
        }
    }

    /* 找到就返回信号 */
    if (signo != -1) {
        return signo;
    }
    return 0;
}


int cmd_kill(uint32_t argc, char** argv)
{
	if(argc < 2){
		printf("kill: too few arguments.\n");	
		return -1;
	}

	//默认 argv[1]是进程的pid
	
    int signo = SIGTERM;    /* 默认是TERM信号，如果没有指定信号的话 */
    int pid = -1;

    char *p;

    /* 不列出信号 */
    bool list_signal = false;
    /* 是否有可选参数 */
    bool has_option = false;    

    bool pid_negative = false;    /* pid为负数 */

    /* 找出pid和signo */
    int idx = 1;

    /* 扫描查看是否有可选参数 */
    while (idx < argc) {
        /* 有可选参数，并且是在第一个参数的位置 */
        if (argv[idx][0] == '-' && argv[idx][1] != '\0' && idx == 1) {
            has_option = true;
            p = (char *)(argv[idx] + 1);
            /* 查看是什么选项 */

            if (*p == 'l' && p[1] == '\0') {    /* 是 -l选项 */
                list_signal = true;     /* 需要列出信号 */
            } else {
                /* 是纯数字，即 -12 */
                if (isdigitstr((const char *)p)) {
                    /* 转换成数值 */
                    signo = atoi(p);

                    if (1 > signo || signo >= MAX_SUPPORT_SIG_NR) {
                        printf("kill: signal %s number not support!\n", signo);
                        return -1;
                    }

                } else { /* 不是是纯数字，即 -KILL，但也可能找不到 */
                    signo = find_signal_in_table(p);

                    /* 找到信号 */
                    if (signo <= 0) {
                        
                        printf("kill: signal %s not found!\n", p);
                        return -1;
                    }
                }

            }
        } else if (argv[idx][0] != '\0' && idx == 1) {  /* 还是第一个参数，是没有选项的状态 */
            p = (char *)(argv[idx]);

            /* 此时就是进程pid */

            /* 是纯数字，即 -12 */
            if (*p == '-') {    /* 可能是负数 */
                pid_negative = true;
                p++;
            }

            if (isdigitstr((const char *)p)) {
                /* 转换成数值 */
                pid = atoi(p);
                /* 如果是负的，就要进行负数处理 */
                if (pid_negative)
                    pid = -pid;

            } else {
                printf("kill: process id %s error!\n", p);
                return -1;
            }
        } else if (argv[idx][0] != '\0' && idx == 2) {  /* 第二个参数 */
            p = (char *)(argv[idx]);

            /* 
            如果是纯数字，就是进程id。
            如果是字符串，就是信号值
             */
            if (list_signal) {

                /* 是列出信号，就直接解析信号值 */
                signo = find_signal_in_table(p);
                
                /* 找到信号 */
                if (signo <= 0) {
                    printf("kill: signal %s not found!\n", p);
                    return -1;
                }
                

            } else {
                /* 是纯数字，即 -12 */
                if (*p == '-') {    /* 可能是负数 */
                    pid_negative = true;
                    p++;
                }

                if (isdigitstr((const char *)p)) {
                    /* 转换成数值 */
                    pid = atoi(p);
                    /* 如果是负的，就要进行负数处理 */
                    if (pid_negative)
                        pid = -pid;
                } else {
                    printf("kill: process id %s must be number!\n", p);
                    return -1;
                }
            }
        }
        idx++;
    }

    if (has_option) {
        /* 是列出信号 */
        if (list_signal) {
            if (argc == 2) {
                /* kill -l # 列出所有 */
                //printf("list all signum\n");
                printf(" 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL       5) SIGTRAP     ");
                printf(" 6) SIGABORT     7) SIGBUS       8) SIGFPE       9) SIGKILL     10) SIGUSR1     ");
                printf("11) SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM     15) SIGTERM     ");
                printf("16) SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP     20) SIGTSTP     ");
                printf("21) SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ     ");
                printf("26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO       30) SIGPWR      ");
                printf("31) SIGSYS      \n");
            } else {
                /* 单独列出信号值 */
                printf("%d\n", signo);
            }
            
        } else {    /* 不是列出信号，就是执行信号 */
            if (argc == 2) {
                //printf("send signal %d no pid\n", signo);
                printf("kill: please order process id!\n");
                return -1;
            } else {
                //printf("send signal %d to pid %d\n", signo, pid);
                if (kill(pid, signo) == -1) {
                    printf("kill: pid %d failed.\n", pid);
                    return -1;
                }
            }
        }
    } else {
        //printf("send signal %d tp pid %d\n", signo, pid);
        if (kill(pid, signo) == -1) {
            printf("kill: pid %d failed.\n", pid);	
            return -1;
        }
    }

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

extern int stdin;

void cmd_exit(uint32_t argc, char** argv)
{
    ioctl(stdin, TTY_CMD_HOLD, 0);
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
