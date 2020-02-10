;----
;file:		arch/x86/kernel/core/interrupt.asm
;auther:	Jason Hu
;time:		2019/6/23
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----
%include "kernel/const.inc"

;interruptHandlerTable是C中注册的中断处理程序数组
extern interruptHandlerTable		 
extern DoIRQ		 
extern DoSoftirq		 
extern DoSignal

[bits 32]
[section .text]

EXCEPTION_ENTRY 0x00,NO_ERROR_CODE
EXCEPTION_ENTRY 0x01,NO_ERROR_CODE
EXCEPTION_ENTRY 0x02,NO_ERROR_CODE
EXCEPTION_ENTRY 0x03,NO_ERROR_CODE 
EXCEPTION_ENTRY 0x04,NO_ERROR_CODE
EXCEPTION_ENTRY 0x05,NO_ERROR_CODE
EXCEPTION_ENTRY 0x06,NO_ERROR_CODE
EXCEPTION_ENTRY 0x07,NO_ERROR_CODE 
EXCEPTION_ENTRY 0x08,ERROR_CODE
EXCEPTION_ENTRY 0x09,NO_ERROR_CODE
EXCEPTION_ENTRY 0x0a,ERROR_CODE
EXCEPTION_ENTRY 0x0b,ERROR_CODE 
EXCEPTION_ENTRY 0x0c,NO_ERROR_CODE
EXCEPTION_ENTRY 0x0d,ERROR_CODE
EXCEPTION_ENTRY 0x0e,ERROR_CODE
EXCEPTION_ENTRY 0x0f,NO_ERROR_CODE 
EXCEPTION_ENTRY 0x10,NO_ERROR_CODE
EXCEPTION_ENTRY 0x11,ERROR_CODE
EXCEPTION_ENTRY 0x12,NO_ERROR_CODE
EXCEPTION_ENTRY 0x13,NO_ERROR_CODE 
EXCEPTION_ENTRY 0x14,NO_ERROR_CODE
EXCEPTION_ENTRY 0x15,NO_ERROR_CODE
EXCEPTION_ENTRY 0x16,NO_ERROR_CODE
EXCEPTION_ENTRY 0x17,NO_ERROR_CODE 
EXCEPTION_ENTRY 0x18,ERROR_CODE
EXCEPTION_ENTRY 0x19,NO_ERROR_CODE
EXCEPTION_ENTRY 0x1a,ERROR_CODE
EXCEPTION_ENTRY 0x1b,ERROR_CODE 
EXCEPTION_ENTRY 0x1c,NO_ERROR_CODE
EXCEPTION_ENTRY 0x1d,ERROR_CODE
EXCEPTION_ENTRY 0x1e,ERROR_CODE
EXCEPTION_ENTRY 0x1f,NO_ERROR_CODE 
INTERRUPT_ENTRY 0x20,NO_ERROR_CODE	;时钟中断对应的入口
INTERRUPT_ENTRY 0x21,NO_ERROR_CODE	;键盘中断对应的入口
INTERRUPT_ENTRY 0x22,NO_ERROR_CODE	;级联用的
INTERRUPT_ENTRY 0x23,NO_ERROR_CODE	;串口2对应的入口
INTERRUPT_ENTRY 0x24,NO_ERROR_CODE	;串口1对应的入口
INTERRUPT_ENTRY 0x25,NO_ERROR_CODE	;并口2对应的入口
INTERRUPT_ENTRY 0x26,NO_ERROR_CODE	;软盘对应的入口
INTERRUPT_ENTRY 0x27,NO_ERROR_CODE	;并口1对应的入口
INTERRUPT_ENTRY 0x28,NO_ERROR_CODE	;实时时钟对应的入口
INTERRUPT_ENTRY 0x29,NO_ERROR_CODE	;重定向
INTERRUPT_ENTRY 0x2a,NO_ERROR_CODE	;保留
INTERRUPT_ENTRY 0x2b,NO_ERROR_CODE	;保留
INTERRUPT_ENTRY 0x2c,NO_ERROR_CODE	;ps/2鼠标
INTERRUPT_ENTRY 0x2d,NO_ERROR_CODE	;fpu浮点单元异常
INTERRUPT_ENTRY 0x2e,NO_ERROR_CODE	;硬盘
INTERRUPT_ENTRY 0x2f,NO_ERROR_CODE	;保留


;系统调用中断
[bits 32]
[section .text]
;导入系统调用表
extern syscallTable	
extern SyscallCheck

global SyscallHandler
SyscallHandler:
	;1 保存上下文环境
   	push 0			    ; 压入0, 使栈中格式统一

   	push ds
   	push es
   	push fs
   	push gs
   	pushad			    ; PUSHAD指令压入32位寄存器，其入栈顺序是:
				    	; EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI 
    
    mov dx,ss
	mov ds, dx
	mov es, dx
    
   	push 0x80			; 此位置压入0x80也是为了保持统一的栈格式
    
    push eax        ; 保存eax

    ;2 系统调用号检测
    push eax
    call SyscallCheck
    add esp, 4

    cmp eax, 0      ; 如果返回值是0，说明系统调用号出错，就不执行系统调用
    je .DoSignal

    pop eax         ; 恢复eax

    ;3 为系统调用子功能传入参数
    push esp                ; 传入栈指针，可以用来获取所有陷阱栈框寄存器
    push edi                ; 系统调用中第4个参数
    push esi                ; 系统调用中第3个参数
  	push ecx			    ; 系统调用中第2个参数
   	push ebx			    ; 系统调用中第1个参数
    
	;4 调用子功能处理函数
   	call [syscallTable + eax*4]	    ; 编译器会在栈中根据C函数声明匹配正确数量的参数
   	add esp, 20			    ; 跨过上面的5个参数

	;5 将call调用后的返回值存入待当前内核栈中eax的位置
    mov [esp + 8*4], eax	
   
    ; 需要完成系统调用后再进行信号处理，因为处理过程可能会影响到寄存器的值
    ;6 signal
.DoSignal:
    push esp         ; 把中断栈指针传递进去
    call DoSignal
    add esp, 4

   jmp InterruptExit		    ; InterruptExit返回,恢复上下文

; 跳转到用户态执行的切换
global SwitchToUser

global InterruptExit

;SwitchToUser:
;    mov esp, [esp + 4]  ; process stack
;    jmp $
InterruptExit:
; 以下是恢复上下文环境
    add esp, 4			   ; 跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4			   ; 跳过error_code
    iretd
    
SwitchToUser:
    mov esp, [esp + 4]  ; process stack
; 以下是恢复上下文环境
    add esp, 4			   ; 跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4			   ; 跳过error_code
    iretd


;----
; 
; Disable an interrupt request line by setting an 8259 bit.
; Equivalent code:
;	if(IRQ < 8){
;		Out8(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) | (1 << IRQ));
;	}
;	else{
;		Out8(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) | (1 << IRQ));
;	}
;----
global DisableIRQ
DisableIRQ:	;void DisableIRQ(uint32_t IRQ);
    mov     ecx, [esp + 4]          ; IRQ
    pushf
    cli
    mov     ah, 1
    rol     ah, cl                  ; ah = (1 << (IRQ % 8))
    cmp     cl, 8
    jae     .Disable8               ; disable IRQ >= 8 at the slave 8259
.Disable0:
    in      al, INT_M_CTLMASK
    test    al, ah
	jnz     .DisAlready             ; already disabled?
	or      al, ah
	out     INT_M_CTLMASK, al       ; set bit at master 8259
	popf
	mov     eax, 1                  ; disabled by this function
	ret
.Disable8:
	in      al, INT_S_CTLMASK
	test    al, ah
	jnz     .DisAlready             ; already disabled?
	or      al, ah
	out     INT_S_CTLMASK, al       ; set bit at slave 8259
	popf
	mov     eax, 1                  ; disabled by this function
	ret
.DisAlready:
	popf
	xor     eax, eax                ; already disabled
	ret

;----
; Enable an interrupt request line by clearing an 8259 bit.
; Equivalent code:
;       if(IRQ < 8){
;               out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) & ~(1 << IRQ));
;       }
;       else{
;               out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) & ~(1 << IRQ));
;       }
;----
global EnableIRQ
EnableIRQ:	;void EnableIRQ(uint32_t IRQ);
    mov     ecx, [esp + 4]          ; IRQ
    pushf
    cli
    mov     ah, ~1
    rol     ah, cl                  ; ah = ~(1 << (IRQ % 8))
    cmp     cl, 8
    jae     .Enable8                ; enable IRQ >= 8 at the slave 8259
.Enable0:
    in      al, INT_M_CTLMASK
    and     al, ah
    out     INT_M_CTLMASK, al       ; clear bit at master 8259
    popf
    ret
.Enable8:
    in      al, INT_S_CTLMASK
    and     al, ah
    out     INT_S_CTLMASK, al       ; clear bit at slave 8259
    popf
    ret