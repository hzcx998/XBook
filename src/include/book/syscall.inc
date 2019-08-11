
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
