#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <const.h>
#include <unistd.h>
#include <conio.h>
#include <signal.h>
#include <ctype.h>
#include <ioctl.h>
#include <time.h>
#include <mman.h>
#include <errno.h>

#include "cmd.h"
#include "bosh.h"

/* 清屏命令后不需要换行 */
extern int shell_next_line;
extern char cwd_cache[];

extern int bosh_stdin; /* 标准输入文件时tty，退出时，需要抛弃持有者 */ 
extern int bosh_stdout;
extern int bosh_stderr;

extern char tty_path[];

extern void signal_handler(int signo);

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

void cmd_free(uint32_t argc, char** argv)
{
    if (argc > 1) {
        printf("free: no arguments support!\n");
        return;
    }
    meminfo_t mi;
    getmem(&mi);
    printf("         TOTAL           USED           FREE\n");
    printf("%14dB%14dB%14dB\n", mi.mi_total, mi.mi_used, mi.mi_free);
    printf("%14dM%14dM%14dM\n", mi.mi_total / MB, mi.mi_used / MB, mi.mi_free / MB);
}


void cmd_exit(uint32_t argc, char** argv)
{
    /* 假装处理 */
	signal_handler(0);
}

void cmd_reboot(uint32_t argc, char** argv)
{
    printf("buildin reboot: not support now!\n");	
    
    return;
	reboot();
}

int cmd_copy(uint32_t argc, char** argv)
{
	if(argc < 3){
		printf("cp: command syntax is incorrect.\n");	
		return -1;
	}

	if(!strcmp(argv[1], ".") || !strcmp(argv[1], "..")){
		printf("cp: src pathnamne can't be . or .. \n");	
		return -1;
	}
	if(!strcmp(argv[2], ".") || !strcmp(argv[2], "..")){
		printf("cp: dst pathname can't be . or .. \n");	
		return -1;
	}

    /* 如果2者相等则不能进行操作 */
    if (!strcmp(argv[1], argv[2])) {
        printf("cp: source file and dest file must be differern!\n");	
		return -1;
    }
    /* 复制逻辑：
        1. 打开两个文件，不存在则创建，已经存在则截断
        2. 复制数据
        3.关闭文件
     */
    int fdrd = open(argv[1], O_RDONLY);
    if (fdrd == -1) {
        printf("cp: open file %s failed!\n", argv[1]);
        return -1;
    }
    /* 如果文件已经存在则截断 */
    int fdwr = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC);
    if (fdwr == -1) {
        printf("cp: open file %s failed!\n", argv[2]);
        close(fdrd);
        return -1;
    }

    struct stat fstat;

    stat(argv[1], &fstat);

    /* 每次操作512字节 */
    char *buf = malloc(fstat.st_size);
    if (buf == NULL) {
        printf("cp: malloc for size %d failed!\n", fstat.st_size);
        goto err;
    }

    char *p = buf;
    int size = fstat.st_size;
    int readBytes;

    /* 每次读取64kb */
    int chunk = (size & 0xffff) + 1;
    
    /* 如果chunk为0，就设置块大小 */
    if (chunk == 0) {
        chunk = 0xffff;
        size -= 0xffff;
    }
        
    while (size > 0) {  
        readBytes = read(fdrd, p, chunk);
        //printf("read:%d\n", readBytes);
        if (readBytes == -1) {  /* 应该检查是错误还是结束 */
            goto failed; 
        }
        if (write(fdwr, p, readBytes) == -1) {
            goto failed;  
        }
        p += chunk;
        size -= 0xffff;
        chunk = 0xffff;
    }

    /* 设置模式和原来文件一样 */
    setmode(argv[2], fstat.st_mode);

    free(buf);
    /* 复制结束 */
    close(fdrd);
    close(fdwr);
    return 0;
failed:
    printf("cp: transmit data error!\n");
    free(buf);
err:
    /* 复制结束 */
    close(fdrd);
    close(fdwr);
    return -1;
}

int cmd_move(uint32_t argc, char** argv)
{
	if(argc < 3){
		printf("mv: command syntax is incorrect.\n");	
		return -1;
	}

	if(!strcmp(argv[1], ".") || !strcmp(argv[1], "..")){
		printf("mv: src pathnamne can't be . or .. \n");	
		return -1;
	}
	if(!strcmp(argv[2], ".") || !strcmp(argv[2], "..")){
		printf("mv: dst pathname can't be . or .. \n");	
		return -1;
	}

    /* 如果2者相等则不能进行操作 */
    if (!strcmp(argv[1], argv[2])) {
        printf("mv: source file and dest file must be differern!\n");	
		return -1;
    }
    /* 复制逻辑：
        1. 打开两个文件，不存在则创建，已经存在则截断
        2. 复制数据
        3.关闭文件
     */
    int fdrd = open(argv[1], O_RDONLY);
    if (fdrd == -1) {
        printf("mv: open file %s failed!\n", argv[1]);
        return -1;
    }
    /* 如果文件已经存在则截断 */
    int fdwr = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC);
    if (fdwr == -1) {
        printf("mv: open file %s failed!\n", argv[2]);
        close(fdrd);
        return -1;
    }

    struct stat fstat;

    stat(argv[1], &fstat);

    /* 每次操作512字节 */
    char *buf = malloc(fstat.st_size);
    if (buf == NULL) {
        printf("mv: malloc for size %d failed!\n", fstat.st_size);
        goto err;
    }

    char *p = buf;
    int size = fstat.st_size;
    int readBytes;

    /* 每次读取64kb */
    int chunk = (size & 0xffff) + 1;

    if (chunk == 0) {
        chunk = 0xffff;
        size -= 0xffff;
    }    
    while (size > 0) {  
        readBytes = read(fdrd, p, chunk);
        //printf("read:%d\n", readBytes);
        if (readBytes == -1) {  /* 应该检查是错误还是结束 */
            goto failed; 
        }
        if (write(fdwr, p, readBytes) == -1) {
            goto failed;  
        }
        p += chunk;
        size -= 0xffff;
        chunk = 0xffff;
    }

    /* 设置模式和原来文件一样 */
    setmode(argv[2], fstat.st_mode);
    
    free(buf);
    /* 复制结束 */
    close(fdrd);
    close(fdwr);

    /* 移动后删除源文件 */
    if(remove(argv[1]) == 0){
        printf("mv: delete source file %s success.\n", argv[1]);
    }else{
        printf("mv: delete source file %s faild!\n", argv[1]);
        /* 删除复制后的文件 */
        remove(argv[2]);
    }

    return 0;
failed:
    printf("mv: transmit data error!\n");
    free(buf);
err:
    /* 复制结束 */
    close(fdrd);
    close(fdwr);
    return -1;
}

int cmd_rename(uint32_t argc, char** argv)
{
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

void cmd_date(uint32_t argc, char** argv)
{
    struct tm tm;
    time(&tm);
    printf("Date: %s\n", asctime(&tm));
}

void cmd_ver(uint32_t argc, char** argv)
{
	char buf[48];
    getver(buf, 48);
    printf("%s\n",buf);
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
	/* 如果没有参数，就接收输入，输入类型：管道，文件，设备 */
    if (argc == 1) {
        /*  */
        /*char buf;
        while (read(STDIN_FILENO, &buf, 1) > 0) {
            printf("%c", buf);
        }*/

        /* 只接受4096字节输入 */
        char *buf = malloc(4096 + 1);
        memset(buf, 0, 4096 + 1);
        int readBytes = read(STDIN_FILENO, buf, 4096);
        //printf("read fd0:%d\n", readBytes);
        if (readBytes > 0) {            
            char *p = buf;
            while (readBytes--) {
                printf("%c", *p);
                p++;
            }
        }
        free(buf);
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
		return -1;
	}
	
	struct stat fstat;
	stat(path, &fstat);
	
	char *buf = (char *)malloc(fstat.st_size);
	
	int bytes = read(fd, buf, fstat.st_size);
	//printf("read %s fd%d:%d\n", path,  fd, bytes);
	close(fd);
	
	int i = 0;
	while(i < bytes){
		printf("%c", buf[i]);
		i++;
	}
	free(buf);
	//printf("\n");
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

#define ECHO_VERSION "0.1"

/*
echo string			//show a string
echo string>file 	//output a string to file
echo string>>file	//apend a string to file
*/
int cmd_echo(int argc, char *argv[])
{
    /*printf("buildin echo: not support now!\n");	
    */

    /* 选项解析 */
    if (argc == 1) {
        return 0;
    }
    /* 没有换行 */
    char no_newline = 0;
    /* 转义 */
    char trope = 0;
    
    /* 第一个参数位置 */
    int first_arg = 1;

    char *p = argv[first_arg];
    
    /* 有选项 */
    if (*p == '-') {
        p++;
        switch (*p)
        {
        case 'n':   /* 输出后不换行 */
            no_newline = 1;
            break;
        case 'e':   /* 转义字符 */
            trope = 1;
            break;
        case 'h':   /* 帮助 */
            printf("Usage: echo [option] [str1] [str2] [str3] ...\n");
            printf("Option:\n");
            printf("  -n    No new line after output. Example: echo -n hello!\n");
            printf("  -e    Escape a string. Example: echo -e this\\nis a string.\\n \n");
            printf("  -v    Print version. Example: echo -v \n");
            printf("  -h    Print usage help. Example: echo -h \n");
            printf("No option:\n");
            printf("  Print what you input. Example: echo a b 123 \n");
            
            return 0;
        case 'v':   /* 版本 */
            printf("version: %s\n", ECHO_VERSION);
            return 0;
        default:
            printf("echo: unknown option!\n");
            return -1;
        }
        /* 指向下一个参数 */
        first_arg++;
    }
    int i;
    /* 输出参数 */
    for (i = first_arg; i < argc; i++) {
        if (trope) {
            /* 有转义就需要把转义字符输出成转义符号 */
            p = argv[i];
            while (*p) {    
                /* 如果是'\'字符，后面是转义符号 */
                if (*p == '\\') {
                    switch (*(p + 1)) {
                    case 'b':   /* '\b' */
                        p += 2;
                        putch('\b');
                        break;
                    case 'n':   /* '\n' */
                        p += 2;
                        putch('\n');                    
                        break;
                    case 'c':   /* '\c' */
                        p += 2;
                        /* 不换行 */
                        no_newline = 1;
                        break;
                    case 't':   /* '\t' */
                        p += 2;
                        putch('\t');                    
                        break;
                    case '\\':   /* '\\' */
                        p += 2;
                        putch('\\');
                        break;                
                    default:
                        /* 不是可识别的转义字符，直接输出 */
                        putch(*p++);
                        break;
                    }
                } else {
                    /* 不是转义字符开头，就直接显示 */
                    putch(*p++);
                }   
            }
        } else {    /* 没有转义就直接输出 */
            printf("%s", argv[i]);
        }
        /* 不是最后才输出一个空格 */
        if (i < argc - 1)
            putch(' ');
            
    }

    /* 换行 */
    if (!no_newline) {
        printf("\n");
    }
    
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
	printf("  cp          copy a file.\n");
	printf("  date        get current date.\n");
	printf("  dir         list files in current dirctory.\n");
	printf("  exit        exit shell.\n");
	printf("  kill        close a thread.\n");
	printf("  ls          list files in current dirctory.\n");
	printf("  lsdisk      list disk drives.\n");
	printf("  mkdir       create a dir.\n");
	printf("  free        print memory info.\n");
	printf("  mv          move a file.\n");
	printf("  ps          print tasks.\n");
	printf("  pwd         print work directory.\n");
	printf("  rmdir       remove a dir.\n");
	printf("  rename      reset file/dirctory name.\n");
	printf("  reboot      reboot system.\n");
	printf("  rm          delete a file.\n");
	printf("  touch       create a empty file.\n");
    printf("  tty         show my tty device file.\n");
	printf("  ver         show os version.\n");
}

static const char *task_status[] = {
    "READY",
    "RUNNING",
    "BLOCKED",
    "WAITING",
    "STOPED",
    "ZOMBIE",
    "DIED"
};

int cmd_ps(uint32_t argc, char** argv)
{
    
    taskscan_status_t ts;
    int num = 0;
    
    int all = 0;

    if (argc > 1) {
        char *p = (char *)argv[1];
        if (*p == '-') {
            p++;
            switch (*p)
            {
            case 'a':   /* 显示所有信息 */
                all = 1;
                break;
            case 'h':   /* 显示帮助信息 */
                printf("Usage: ps [option]\n");
                printf("Option:\n");
                printf("  -a    Print all tasks. Example: ps -a \n");
                printf("  -h    Get help of ps. Example: ps -h \n");
                printf("Note: If no arguments, only print user process.\n");
                return 0;
            default:
                printf("ps: unknown option!\n");
                return -1;
            }
        } else {
            printf("ps: unknown argument!\n");
            return -1;
        }
    }

    printf("   PID   PPID     STAT    PRO        TICKS NAME\n");
    while (!taskscan(&ts, &num)) {
        /* 如果没有全部标志，就只显示用户进程。也就是ppid不为-1的进程 */
        if (!all) {
            if (ts.ts_ppid == -1)
                continue;
        }
        printf("%6d %6d %8s %6d %12d %s\n", 
            ts.ts_pid, ts.ts_ppid, task_status[ts.ts_state], ts.ts_priority,
            ts.ts_runticks, ts.ts_name);
    }
    return 0;
	/*if(argc != 1){
		printf("ps: no argument support!\n");
		return;
	}*/
    
    //log("");
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
                    } else if (*option == 'h') {
                        printf("Usage: ls [option]\n");
                        printf("Option:\n");
                        printf("  -l    Print all infomation about file or directory. Example: ls -l \n");
                        printf("  -h    Get help of ls. Example: ls -h \n");
                        printf("  [dir] Print [dir] file or directory. Example: ls / \n");
                        printf("Note: If no arguments, only print name in cwd.\n");
                        
                        return;
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
    ioctl(bosh_stdout, GTTY_IOCTL_CLEAR, 0);

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
        //printf("de.type:%d", de.type);
            
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
                    type = '?';
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
                printf("%c%c%c%c %04d/%02d/%02d %02d:%02d:%02d %12d %s\n",
                    type, attrR, attrW, attrX,
                    FILE_TIME_YEA(fstat.st_mtime>>16),
                    FILE_TIME_MON(fstat.st_mtime>>16),
                    FILE_TIME_DAY(fstat.st_mtime>>16),
                    FILE_TIME_HOU(fstat.st_mtime&0xffff),
				    FILE_TIME_MIN(fstat.st_mtime&0xffff),
                    FILE_TIME_SEC(fstat.st_mtime&0xffff),
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
	cmd_ls_sub("sys:/drv", 0);
}


/**
 * do_buildin_cmd - 执行内建命令
 */
int buildin_cmd(int cmd_argc, char **cmd_argv)
{
    int retval = 0;

    if(!strcmp("cls", cmd_argv[0])){
        cmd_cls(cmd_argc, cmd_argv);

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
    }else if(!strcmp("date", cmd_argv[0])){
        cmd_date(cmd_argc, cmd_argv);
    }else if(!strcmp("rename", cmd_argv[0])){
        cmd_rename(cmd_argc, cmd_argv);
    }else if(!strcmp("mv", cmd_argv[0])){
        cmd_move(cmd_argc, cmd_argv);
    }else if(!strcmp("cp", cmd_argv[0])){
        cmd_copy(cmd_argc, cmd_argv);
    }else if(!strcmp("reboot", cmd_argv[0])){
        cmd_reboot(cmd_argc, cmd_argv);
    }else if(!strcmp("exit", cmd_argv[0])){
        cmd_exit(cmd_argc, cmd_argv);
    }else if(!strcmp("free", cmd_argv[0])){
        cmd_free(cmd_argc, cmd_argv);
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
