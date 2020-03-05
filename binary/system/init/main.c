#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <file.h>
#include <string.h>

//#define CONFIG_MORE_TTY


/* 最多32个配置参数 */
char *config_arg[32] = {0};

void match_config_arg(char *buf);

/* init */
int main(int argc, char *argv[])
{
    /* 读取配置文件，根据配置文件打开终端 */
    int fd = open("root:/etc/init.cfg", O_RDONLY);
    if (fd < 0) {
        return -1;  /* 如果打开文件失败，就退出 */
    }
    int fsize = lseek(fd, 0, SEEK_END);
    
    /* 分配缓冲区 */
    char *cfgbuf = malloc(fsize + 1);
    lseek(fd, 0, SEEK_SET);
    
    /* 如果读取失败，就退出 */
    if (read(fd, cfgbuf, fsize) != fsize) {
        close(fd);
        free(cfgbuf);
        return -1;
    }
    close(fd);

    /* 匹配出对应的参数 */
    match_config_arg(cfgbuf);

    /* 必须使用的参数必须有才行 */
    if (!config_arg[0] && !config_arg[1] && !config_arg[2]) {
        free(cfgbuf);
        return -1;
    }

    /* 提前分支，这样父进程打开输入输出就不会影响到子进程 */
	int pid = fork();

    if (pid == -1) 
        return -1;
    
	if (pid > 0) {
#ifdef CONFIG_MORE_TTY
        /* 第一个子进程用来执行第一个shell */
        pid = fork();
        if (pid == -1) 
            return -1;
        if (pid > 0) {  /* 本身 */

            pid = fork();
            if (pid == -1) 
                return -1;
            if (pid > 0) {  /* 本身 */
            
#endif  /* CONFIG_MORE_TTY */
                
                /* 打开标准输入，输出，错误 */
                int stdinno = open("sys:/dev/tty0", O_RDONLY);
                if (stdinno < 0)
                    return -1;

                int stdoutno = open("sys:/dev/tty0", O_WRONLY);
                if (stdoutno < 0)
                    return -1;
                
                int stderrno = open("sys:/dev/tty0", O_WRONLY);
                if (stderrno < 0)
                    return -1;
                    
                /* 关闭文件描述符 */
                /*close(stdout);
                close(stdin);*/

                //printf("I am parent, my pid is %d my child is %d.\n", getpid(), pid);
                /* init进程就不断等待子进程，然后把他们回收 */
                while (1) {
                    int status;
                    pid = _wait(&status);
                    if (pid != -1)
                        printf("task pid %d exit with status %d.\n", pid, status);	
                }
#ifdef CONFIG_MORE_TTY
            } else {    /* 子进程 */
                /* 第三个子进程用来执行第三个shell */
                const char *args[2];
                /* shell需要打开的tty的名字 */
                args[0] = "sys:/dev/tty2";
                args[1] = NULL;

                /* 开启第一个shell */
                if (execv("/shell", args)) {
                    //printf("execute failed!\n");
                    return -1;
                }
            }
            
        } else {    /* 子进程 */
            
            const char *args[2];
            /* shell需要打开的tty的名字 */
            args[0] = "sys:/dev/tty1";
            args[1] = NULL;

            /* 开启第一个shell */
            if (execv("/shell", args)) {
                //printf("execute failed!\n");
                return -1;
            }
        }
#endif  /* CONFIG_MORE_TTY */        
	} else {
        const char *args[4];
        /* shell需要打开的tty的名字 */
        args[0] = config_arg[0];
        args[1] = config_arg[1];
        args[2] = config_arg[2];
        args[3] = NULL;
        /* 开启第一个shell */
        if (execv(args[0], args)) {
            //printf("execute failed!\n");
            return -1;
        }
	}
	return 0;
}


void match_config_arg(char *buf)
{
    char *start, *end;  /* 单个配置 */
    char *name, *value; /* 配置的名字和值 */
    start = buf;
    while (*start) {
        /* 如果找到了一个完整的配置，才进行进一步处理 */
        if (!(end = strchr(start, '\n'))) {
            break;
        }
        *end = 0;
        /* 查找':'分隔符 */
        name = start;
        if (!(value = strchr(name, '='))) {
            break;
        }
        *value = 0;
        /* 找到参数和值 */
        if (!strcmp(name, "shell")) {
            value++;
            config_arg[0] = value;
        } else if (!strcmp(name, "sharg")) {
            value++;
            config_arg[1] = value;
        } else if (!strcmp(name, "tty")) {
            value++;
            config_arg[2] = value;
        }
        ++end;
        start = end;
    }
}

