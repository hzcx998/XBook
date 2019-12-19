#include <conio.h>
#include <mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>

char test[4096];

char test2[4096];

int sum(int n)
{
	if (n)
		return n + sum(n-1);
	return 0;
}

#define FIFO_PATH "sys:/pip/fifot"

int pipe_read()
{
    int fifofd = open(FIFO_PATH, O_RDONLY);
    if (fifofd < 0) {
        printf("open fifo failed!\n");
        return -1;
    }

    printf("read open fifo ok!\n");
  
    char buf[32];
    memset(buf, 0, 32);
    printf("read %d bytes.\n", read(fifofd, buf, 32));

    printf("read data:%s\n", buf);

    close(fifofd);
    printf("read %d close fifo sucess!\n", getpid());

    return 0;
}

int pipe_write()
{
    int fifofd = open(FIFO_PATH, O_WRONLY);
    if (fifofd < 0) {
        printf("open fifo failed!\n");
        return -1;
    }

    printf("write open fifo ok!\n");

    char buf[32];
    memset(buf, 0, 32);
    strcpy(buf, "hello, fifo!\n");

    printf("write %d bytes.\n", write(fifofd, buf, strlen(buf)));
    printf("write data:%s\n", buf);

    close(fifofd);
    printf("write %d close fifo sucess!\n", getpid());

    //printf("write %d bytes.\n", write(fifofd, buf, strlen(buf)));
    
    return 0;
}

int pipe_test()
{
    printf("----pipe test----\n");
    
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        printf("make pipe failed!\n");
        return -1;
    }
    printf("pipe 0:%d 1:%d\n", pipefd[0], pipefd[1]);
    int fret = fork();
    if (fret == 0) {
        printf("I am child, my pid is %d.\n", getpid());
        /* 关闭读端 */
        //close(pipefd[0]);
        char buf[32];
        memset(buf, 0, 32);
        strcpy(buf, "Hello, pipe!\n");
        /* 给父进程发送数据 */
        printf("write %d bytes\n", write(pipefd[1], buf, strlen(buf) + 1));
        sleep(1);

        int rcount = read(pipefd[0], buf, 32);
        printf("read %d bytes, data:%s\n", rcount, buf);
        
        //close(pipefd[1]);
    } else {
        printf("I am parent, my pid is %d.\n", getpid());
        
        /* 关闭写端 */
        //close(pipefd[1]);
        char buf[32];
        memset(buf, 0, 32);
        /* 读取子进程发送来的数据 */
        int rcount = read(pipefd[0], buf, 32);
        printf("read %d bytes, data:%s\n", rcount, buf);
        sleep(1);
        
        memset(buf, 0, 32);
        strcpy(buf, "Parent reply!\n");
        printf("write %d bytes\n", write(pipefd[1], buf, strlen(buf) + 1));
        
        /* 等待子进程 */
        _wait(NULL);
        //close(pipefd[0]);
    }
}

int main(int argc, char *argv[])
{
	printf("Welcome to test. my pid %d\n", getpid());

	int i = 0;
	while(i < argc){
		printf("args %s ", argv[i]);
		i++;
	}

    printf("I will do some test and exit\n");	

    printf("----fifo test----\n");
    
    if (access(FIFO_PATH, F_OK)) {
        if (mkfifo(FIFO_PATH, M_IREAD | M_IWRITE)) {
            printf("mkfifo failed!\n");
            return -1;
        }
        remove(FIFO_PATH);
    }
    return 0;
    return pipe_write();
    

    //pipe_test();
    
    return 0;
	printf("----file test----\n");
    
    //return 0;
	int fd = open("test", O_RDONLY);
	if (fd == -1) {
		printf("fd error\n");
		exit(-1);
	}
    printf("open a fd %d\n", fd);

	char fb[10];
	memset(fb, 0, 10);
	if (read(fd, fb, 10) != 10) {
		printf("read error\n");
	}
	printf("%x %x\n", fb[0], fb[9]);

	lseek(fd, 10, SEEK_SET);
	
	memset(fb, 0, 10);
	if (read(fd, fb, 10) != 10) {
		printf("read error\n");
	}
	printf("%x %x\n", fb[0], fb[9]);

	close(fd);

	//while(1);
	if (brk(0)) {
		printf("brk failed!\n");
	}

	printf("----start brk/sbrk test----\n");

	char *p = sbrk(0);

	char *new = sbrk(4);
	*new = 0;
	printf("brk %x alloc addr %x\n", p, new);

	p = sbrk(0);
	printf("brk %x\n", p);

	printf("----start malloc test----\n");

	char *a = malloc(16);
	//memset(a, 0, 16);

	char *b = malloc(128);
	//memset(b, 0, 128);

	char *c = malloc(4096);
	//memset(c, 0, 4096);

	printf("a:%x b:%x c:%x\n", a, b, c);

	free(b);
	free(c);
	free(a);
	
	a = malloc(16);
	memset(a, 0, 16);

	b = malloc(256);
	memset(b, 0, 16);
	free(a);
	
	c = malloc(4096);
	memset(c, 0, 4096);
	free(c);
	free(b);

	char *array = calloc(10, 10);
	if (!array)
		printf("calloc failed!\n");

	for (i = 0; i < 10*10; i++) {	
		array[i] = i;
		//printf("%d", i);
	}
	printf("calloc %x", array);

	char *table[10];
	for (i = 0; i < 10; i++) {
		table[i] = malloc(i*32);
		//printf("%x->", table[i]);
	}

	for (i = 0; i < 10; i++) {
		table[i] = realloc(table[i], i*64);
		//printf("%x->", table[i]);
	}

	for (i = 0; i < 10; i++) {
		free(table[i]);
	}

	char *str = malloc(12);
	strcpy(str, "hello,world");
	printf(str);

	str = realloc(str, 32);
	printf(str);
	strcat(str, "I am Book OS\n");
	printf(str);

	char *mm = realloc(NULL, 32);
	printf("realloc addr %x size %d\n", mm, malloc_usable_size(mm));

	realloc(mm, 0);

	mm = malloc(128);
	printf("realloc addr %x size %d\n", mm, malloc_usable_size(mm));

	mm = realloc(mm, 64);
	printf("realloc addr %x size %d\n", mm, malloc_usable_size(mm));

	//memory_state();
	
	char *maped = mmap(0, 4096, 0, 0);

	if(maped == (void *)-1) {
		printf("mmap failed!\n");
		exit(-1);
	}
	
	printf("mmap sucess!\n");
	
	*maped = 0xfa;
	printf("mmap %x %x.", maped, *maped);

    
    printf("I will exit now!\n");

	return 0;
}
