#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <string.h>

char test[4096];

char test2[4096];

int sum(int n)
{
	if (n)
		return n + sum(n-1);
	return 0;
}

int main(int argc, char *argv[])
{
	printf("Welcome to test. my pid %d\n", getpid());
	
	int i = 0;
	while(i < argc){
		printf("%s ", argv[i]);
		i++;
	}
	printf("\nI will do some test and exit\n");	
	
	return 0;
}
