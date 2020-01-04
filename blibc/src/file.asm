;----
;file:		user/lib/log.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

global open
; int open(const char *path, unsigned int flags);
open:
	
	mov eax, SYS_OPEN
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	
	
	ret
global close
; int close(int fd);
close:
	

	mov eax, SYS_CLOSE
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global read
; int read(int fd, void *buffer, unsigned int size);
read:
	

	mov eax, SYS_READ
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov esi, [esp + 12]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global write
; int write(int fd, void *buffer, unsigned int size);
write:
	mov eax, SYS_WRITE
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov esi, [esp + 12]
	int INT_VECTOR_SYS_CALL
	ret

global lseek
; int lseek(int fd, unsigned int offset, char flags);
lseek:
	mov eax, SYS_LSEEK
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov esi, [esp + 12]
	int INT_VECTOR_SYS_CALL
	ret

global stat
; int stat(const char *path, struct stat *buf);
stat:
	mov eax, SYS_STAT
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	ret

global remove
; int remove(const char *pathname);
remove:
	mov eax, SYS_REMOVE
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

global ioctl
; int ioctl(int fd, int cmd, int arg);
ioctl:
	mov eax, SYS_IOCTL
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov esi, [esp + 12]
	int INT_VECTOR_SYS_CALL
	ret

global getmode
; int getmode(const char* pathname);
getmode:
	mov eax, SYS_GETMODE
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

global setmode
; int setmode(const char* pathname, int mode);
setmode:
    push ebx 
    push ecx 

	mov eax, SYS_SETMODE
	mov ebx, [esp + 8 + 4]
	mov ecx, [esp + 8 + 8]
	int INT_VECTOR_SYS_CALL

    pop ecx 
    pop ebx 
	ret

global access
; int access(const char* filepath, int mode);
access:
    push ebx 
    push ecx 

	mov eax, SYS_ACCESS
	mov ebx, [esp + 8 + 4]
	mov ecx, [esp + 8 + 8]
	int INT_VECTOR_SYS_CALL

    pop ecx 
    pop ebx 
	ret

global fcntl
; int fcntl(int fd, int cmd, long arg);
fcntl:
    push ebx 
    push ecx 
    push esi

	mov eax, SYS_FCNTL
	mov ebx, [esp + 12 + 4]
	mov ecx, [esp + 12 + 8]
    mov esi, [esp + 12 + 12]
	int INT_VECTOR_SYS_CALL
	
    pop esi
    pop ecx 
    pop ebx 
    
    ret


global fsync
; int fsync(int fd);
fsync:
    push ebx 

	mov eax, SYS_FSYNC
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
    pop ebx
    ret
