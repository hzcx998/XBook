extern main
extern exit
extern __init_stream_file
[bits 32]
[section .text]

global _start
_start:
    call __init_stream_file
    cmp eax, 0
    jne .exit

	call main 

.exit:
    
	push eax
	call exit
	jmp $
