;----
;file:      arch/x86/include/boot/config.inc
;auther:    Jason Hu
;time:      2019/8/9
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

; 对引导项进行手动配置

; 配置vbe驱动，注意也要修改内核中的图形配置
%define CONFIG_VESA

; 配置图形模式
%define CONFIG_GRAPH

%ifdef CONFIG_VESA
;图形模式列表：
; 16位
VMODE_640_480_16     EQU 0x111
VMODE_800_600_16     EQU 0x114
VMODE_1024_768_16    EQU 0x117
VMODE_1280_1024_16   EQU 0x11A

; 24位
; 24bit 不能再vmware中使用，不知道为何，可能是不支持
VMODE_640_480_24            EQU 0x112
VMODE_800_600_24            EQU 0x115
VMODE_1024_768_24           EQU 0x118
VMODE_1280_1024_24          EQU 0x11B
VMODE_VMWARE_800_600_24     EQU	0x140

; 使用的模式
VESA_MODE	EQU	VMODE_800_600_16
%endif  ; CONFIG_VESA
