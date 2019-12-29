/*
	File:blibc/src/syscall.s
	Auther:	Suote127
	Time:		2019/11/29
	Copyright(C) 2019 by Book OS developers.All rights reserved
*/

.section .text

.global _log,_mmap,_ummap
.global _fork,_getpid,_execv
.global _sleep,_msleep,_exit
.global _wait

//	void log(char *buf);
_log:
	mov $0x00,%eax
	mov 4(%esp),%ebp
	
	int $0x80
	
	ret

//	int mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags);
_mmap:
	mov $0x01,%eax
	mov 4(%esp),%ebx
	mov 8(%esp),%ecx
	mov 12(%esp),%esi
	mov 16(%esp),%edi
	
	int $0x80
	
	ret

// int munmap(uint32_t addr, uint32_t len);
_munmap:
	mov $0x02,%eax
	mov 4(%esp),%ebx
	mov 8(%esp),%ecx
	
	int $0x80
	
	ret

// int fork();
_fork:
	mov $0x03,%eax
	
	int $0x80
	
	ret

// uint32_t getpid();
_getpid:
	mov $4,%eax
	
	int $0x80
	
	ret

// int execv(const char *path, const char *argv[]);
_execv:
	mov $5,%eax
	
	mov 4(%esp),%ebx
	mov	8(%esp),%ecx
	
	int $0x80
	
	ret

// void sleep(int second);
_sleep:
	mov $6,%eax
	
	mov 4(%esp),%ebx
	
	int $0x80
	
	ret
// void msleep(int msecond);
_msleep:
	mov $7,%eax
	
	mov 4(%esp),%ebx
	
	int $0x80
	
	ret
	
// void exit(int stauts);
_exit:
	mov $8,%eax
	
	mov 4(%esp),%ebx
	
	int $0x80
	
	ret

// void wait(int *stauts)
_wait:
	mov $9,%eax
	
	mov 4(%esp),%ebx
	
	int $0x80
	
	ret
// int brk()
