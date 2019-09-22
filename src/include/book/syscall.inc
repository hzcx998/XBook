
; 系统调用中断向量号
INT_VECTOR_SYS_CALL equ 0x80


; 系统调用函数id

SYS_LOG         EQU 0
SYS_MMAP        EQU 1
SYS_MUNMAP      EQU 2
SYS_FORK        EQU 3
SYS_GETPID      EQU 4
SYS_EXECV       EQU 5
SYS_SLEEP       EQU 6
SYS_MSLEEP      EQU 7
SYS_EXIT        EQU 8
SYS_WAIT        EQU 9
SYS_BRK         EQU 10
SYS_OPEN        EQU 11
SYS_CLOSE       EQU 12
SYS_READ        EQU 13
SYS_WRITE       EQU 14
SYS_LSEEK       EQU 15
SYS_STAT        EQU 16

SYS_UNLINK      EQU 17
SYS_IOCTL       EQU 18
SYS_GETMODE     EQU 19
SYS_SETMODE     EQU 20
SYS_MKDIR       EQU 21
SYS_RMDIR       EQU 22
SYS_MOUNT       EQU 23
SYS_UNMOUNT     EQU 24
SYS_GETCWD      EQU 25
SYS_CHDIR       EQU 26
SYS_RENAME      EQU 27
SYS_OPENDIR     EQU 28
SYS_CLOSEDIR    EQU 29
SYS_READDIR     EQU 30
SYS_REWINDDIR   EQU 31


; SYS_CALL_SAVE 宏用于保存通用寄存器
%macro SYS_CALL_SAVE 0
    push ebp ; 把ebp保存到栈中
	mov	ebp, esp ; 把esp中的值给ebp
    add ebp,4 ; 让ebp指向没有保存ebp之前的栈位置
    pushad ; 保存所有的通用寄存器
%endmacro

; SYS_CALL_RECOVER 宏用于恢复通用寄存器
%macro SYS_CALL_RECOVER 0
    popad ;恢复通用寄存器的值
	pop ebp	; 恢复ebp的值
%endmacro
