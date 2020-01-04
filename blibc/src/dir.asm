;----
;file:		user/lib/log.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

global mkdir
; int mkdir(const char *pathname);
mkdir:
	

	mov eax, SYS_MKDIR
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global rmdir
; int rmdir(const char *pathname);
rmdir:
	

	mov eax, SYS_RMDIR
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global getcwd
; int getcwd(char* buf, unsigned int size);
getcwd:
	

	mov eax, SYS_GETCWD
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global chdir
; int chdir(const char *pathname);
chdir:
	

	mov eax, SYS_CHDIR
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global rename
; int rename(const char *pathname, char *name);
rename:
	

	mov eax, SYS_RENAME
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global opendir
; int opendir(const char *pathname);
opendir:
	

	mov eax, SYS_OPENDIR
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global closedir
; int closedir(DIR *dir);
closedir:
	

	mov eax, SYS_CLOSEDIR
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret

global readdir
; int readdir(DIR *dir, DIRENT *buf);
readdir:
	

	mov eax, SYS_READDIR
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	
	
	ret
global rewinddir
; int rewinddir(DIR *dir);
rewinddir:
	

	mov eax, SYS_REWINDDIR
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret
