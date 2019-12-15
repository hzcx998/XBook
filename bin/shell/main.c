#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* shell */
int main(int argc, char *argv[])
{
	/*printf("Welcome to shell. my pid %d\n", getpid());
	printf("/ $> \n");*/
	
    if (argc <= 0) {
        //printf("argc must >= 1, now exit!\n");  
        return -1;    
    }
    
    /*int i;
    for (i = 0; i < argc; i++) {
        //printf("arg[%d]=%s\n", i, argv[i]);
    }*/

    /* 参数0存放着需要打开的tty设备名 */
    int stdin = open(argv[0], O_RDONLY);
    if (stdin < 0)
        return -1;

    int stdout = open(argv[0], O_WRONLY);
    if (stdout < 0)
        return -1;

    int stderr = open(argv[0], O_WRONLY);
    if (stderr < 0)
        return -1;
    
    printf("hello, shell!\n");

	int pid = fork();
	if (pid > 0) {
		printf("I am father. my pid %d, my child %d\n", getpid(), pid);	
		
        int key = 0;
        while (1) {
            key = 0;
            if (!read(0, &key, 1)) {
                printf("%c", key);
            }
        }
        /*
		int status;
		int pid;
		pid = _wait(&status);
		printf("my child pid %d status %d\n", pid, status);
		*/
	} else if (pid == 0) {
		printf("I am child. my pid %d\n", getpid());
		
		const char *arg[2];
		
		arg[0] = "test";
		arg[1] = 0;

		execv("root:/test", arg);	
	}
	return 0;
}
