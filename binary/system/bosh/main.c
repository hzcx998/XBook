#include <stdio.h>
#include <stdlib.h>
#include <share/string.h>
#include <share/const.h>
#include <unistd.h>
#include <conio.h>
#include <signal.h>

#include "bosh.h"
#include "cmd.h"



char cmd_line[CMD_LINE_LEN] = {0};
char cwd_cache[MAX_PATH_LEN] = {0};
char *cmd_argv[MAX_ARG_NR];

char tty_path[MAX_PATH_LEN] = {0};

int shell_next_line;

int stdin; /* 标准输入文件时tty，退出时，需要抛弃持有者 */ 

void signal_handler(int signo) 
{
    printf("get a sig no %d in user mode.\n", signo);
}

int main(int argc, char *argv0[])
{
    /* 根据传入的参数来初始化shell */
    if (argc <= 0) {
        //printf("argc must >= 1, now exit!\n");  
        return -1;    
    }

    /* 保存对应的tty路径 */
    memset(tty_path, 0, MAX_PATH_LEN);
    strcpy(tty_path, argv0[0]);
    
    stdin = open(argv0[0], O_RDONLY);
    if (stdin < 0)
        return -1;

    int stdout = open(argv0[0], O_WRONLY);
    if (stdout < 0)
        return -1;

    int stderr = open(argv0[0], O_WRONLY);
    if (stderr < 0)
        return -1;
    
    // 把当前进程设置为tty持有者
    ioctl(stdin, TTY_CMD_HOLD, getpid());

    //printf("hello, shell!\n");

	memset(cwd_cache, 0, MAX_PATH_LEN);

	getcwd(cwd_cache, 32);
	//printf("cwd:%s\n", cwd_cache);
	
	int arg_idx = 0;
    int status = 0;
	
	int retva = 0;

    int daemon = 0; /* 为1表示是后台进程 */

    /* 默认是会在执行完一个命令后进入下一行 */

    shell_next_line = 1;

    //signal(1, signal_handler);
    struct sigaction act, oldact;

    /* get old */
    /*sigaction(SIGINT, NULL, &oldact);

    printf("old handler %x\n", oldact.sa_handler);

    
    sigaction(SIGINT, &act, NULL);*/
    act.sa_handler = signal_handler;
    
    sigaction(SIGABRT, &act, &oldact);

    /* 忽略信号 */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    
    //printf("old handler %x\n", oldact.sa_handler);

    //abort();


	while(1){ 
        /* 显示提示符 */
		print_prompt();

		memset(cmd_line, 0, CMD_LINE_LEN);
		
		/* 读取命令行 */
		readline(cmd_line, CMD_LINE_LEN);
		
        /* 如果什么也没有输入，就回到开始处 */
		if(cmd_line[0] == 0){
			continue;
		}
		
		argc = -1;
        /* 解析成参数 */
		argc = cmd_parse(cmd_line, cmd_argv, ' ');
		
		if(argc == -1){
			printf("shell: num of arguments exceed %d\n",MAX_ARG_NR);
			continue;
		}

        daemon = 0;

		arg_idx = 0;
		while(arg_idx < argc){
			//printf("%s ",buf);
			//cmd_argv[arg_idx] = NULL;
            if (!strcmp(cmd_argv[arg_idx], "&")) {
                daemon = 1;     /* 是后台进程 */
                break;
            }
			arg_idx++;
		}

        /* 先执行内建命令，再选择磁盘中的命令 */
		if (buildin_cmd(argc, cmd_argv)) {
            
            /* 在末尾添加上结束参数 */
			cmd_argv[argc] = NULL;
			
			int pid;
            
            /* 检测文件是否存在，以及是否可执行，不然就返回命令错误 */
            if (access(cmd_argv[0], F_OK)) {
                printf("unknown cmd!\n");
            } else {
                /* 创建一个进程 */
                pid = fork();

                if (pid > 0) {  /* 父进程 */
                    if (!daemon) {
                        // 把子进程设置为前台
                        ioctl(stdin, TTY_CMD_HOLD, pid);
                        
                        /* shell程序等待子进程退出 */
                        pid = wait(&status);

                        // 恢复自己为前台
                        ioctl(stdin, TTY_CMD_HOLD, getpid());

                        printf("parent %d wait %d exit %d\n",
                            getpid(), pid, status);
                        
                    }
                    
                } else {    /* 子进程 */

                    /* 子进程执行程序 */
                    pid = execv((const char *)cmd_argv[0], (const char **)cmd_argv);
                    
                    /* 如果执行出错就退出 */
                    if(pid == -1){
                        printf("execv file %s failed!\n", cmd_argv[0]);
                        return -1;  /* 退出 */
                    }
                }
            }
		}
		shell_newline();
	}
}

void shell_newline()
{
    /* 如果有换行标志才会换行 */
    if(shell_next_line){
        printf("\n");
    }
    shell_next_line = 1;
}


/**
 * cmd_parse - 从输入的命令行解析参数
 * @cmd_str: 命令行缓冲
 * @argv: 参数数组
 * @token: 分割符
 */
int cmd_parse(char * cmd_str, char **argv, char token)
{
	if(cmd_str == NULL){
		return -1;
	}
	int arg_idx = 0;
	while(arg_idx < MAX_ARG_NR){
		argv[arg_idx] = NULL;
		arg_idx++;
	}
	char *next = cmd_str;
	int argc = 0;
	while(*next){
		//跳过token字符
		while(*next == token){
			next++;
		}
		//如果最后一个参数后有空格 例如"cd / "
		if(*next ==0){
			break;
		}
		//存入一个字符串地址，保存一个参数项
		argv[argc] = next;
		
		//每一个参数确定后，next跳到这个参数的最后面
		while(*next && *next != token){
			next++;
		}
		//如果此时还没有解析完，就把这个空格变成'\0'，当做字符串结尾
		if(*next){
			*next++ = 0;
		}
		//参数越界，解析失败
		if(argc > MAX_ARG_NR){
			return -1;
		}
		//指向下一个参数
		argc++;
		//让下一个字符串指向0
		argv[argc] = 0;
	}
	return argc;
}

/**
 * readline - 读取一行输入
 * @buf: 缓冲区
 * @count: 数据量
 * 
 * 输入回车结束输入
 */
void readline( char *buf, uint32_t count)
{
	char *pos = buf;
	while(read_key(buf, pos, count) && (pos - buf) < count){
		switch(*pos){
			case '\n':
				//当到达底部了就不在继续了，目前还没有设定
				*pos = 0;
				printf("\n");
				return;
			case '\b':
				if(buf[0] != '\b'){
					--pos;
					printf("\b");
				}
				break;
			default:
				printf("%c", *pos);
				pos++;
		}
	}
	printf("\nreadline: error!\n");
}

/**
 * read_key - 从键盘读取一个按键
 * @start: 
 * @buf: 缓冲区
 * @count: 字符数
 * 
 * 
 */
int read_key(char *start, char *buf, int count)
{
	//从键盘获取按键
	int key, modi;
	int status;
	//循环获取按键

	do {
		status = read(0, &key, 4);
		
		if (status != -1) {
			//获取按键
			//读取控制键状态
			
			if (((0 < key) || key&0x8000) && !(key & 0x80)) {
				//是一般字符就传输出去
				*buf = (char)key;
				break;
			}
			
			key = -1;
		}
	}while (status == -1);
	
	return 1;
}

/*
int read_key(char *buf)
{
	//从键盘获取按键
	char key;
	do{
		key = getchar();
	}while(key == -1);
	*buf = key;
	return 1;
}*/

/**
 * print_prompt - 打印提示符
 *  
 */
void print_prompt()
{
	printf("%s>", cwd_cache);
}

/**
 * wait - 等待一个子进程退出
 * @status: 子进程的退出状态码
 * 
 * 返回子进程pid
 */
int wait(int *status)
{
	int pid;
	do{
		pid = _wait(status);
	}while(pid == -1);
	return pid;
}
