#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <string.h>

/* shell */
int main(int argc, char *argv[])
{
	printf("Welcome to shell. my pid %d\n", getpid());
	printf("/ $> \n");
	//while(1);
	int pid = fork();
	if (pid > 0) {
		printf("I am father. my pid %d, my child %d\n", getpid(), pid);	
		
		int status;
		int pid;
		pid = _wait(&status);
		printf("my child pid %d status %d\n", pid, status);
		
	} else if (pid == 0) {
		printf("I am child. my pid %d\n", getpid());
		
		const char *arg[2];
		
		arg[0] = "test";
		arg[1] = 0;

		execv("root:/test", arg);	
	}
	return 0;
}
