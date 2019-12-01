#include <conio.h>
#include <mman.h>
#include <stdlib.h>

/* init */
int main(int argc, char *argv[])
{
	int pid = fork();

	if (pid > 0) {
		printf("I am parent, my pid is %d my child is %d.\n", getpid(), pid);
		
		//while(1);
		/* init进程就不断等待子进程，然后把他们回收 */
		while (1) {
			int status;
			int pid;
			pid = _wait(&status);
			if (pid != -1)
				printf("task pid %d exit with status %d.\n", pid, status);	
		}
	} else {
		printf("I am child, my pid is %d.\n", getpid());	
		
		printf("I will exit now.\n");	
		return 0;
		while(1);
		/* init的第一个子进程就执行shell */
		if (execv("/shell", 0)) {
			printf("execute failed!\n");
		}
	}
	return 0;
}
