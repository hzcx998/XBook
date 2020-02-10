#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <share/types.h>
#include <stdio.h>

//#define CONFIG_MORE_TTY

/* init */
int main(int argc, char *argv[])
{
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
                int stdin = open("sys:/dev/tty0", O_RDONLY);
                if (stdin < 0)
                    return -1;

                int stdout = open("sys:/dev/tty0", O_WRONLY);
                if (stdout < 0)
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
        const char *args[2];
        /* shell需要打开的tty的名字 */
        args[0] = "sys:/dev/tty0";
        args[1] = NULL;

        /* 开启第一个shell */
        if (execv("/shell", args)) {
            //printf("execute failed!\n");
            return -1;
        }
	}
	return 0;
}
