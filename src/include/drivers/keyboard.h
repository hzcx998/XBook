/*
 * file:		include/driver/keyboard.h
 * auther:		Jason Hu
 * time:		2019/8/21
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_CHAR_KEYBOARD_H
#define _DRIVER_CHAR_KEYBOARD_H

#include <share/stdint.h>
#include <share/types.h>

/* raw key value = code passed to tty & MASK_RAW
the value can be found either in the keymap column 0
or in the list below */
#define KBD_MASK_RAW	0x01FF		

/* 键盘值屏蔽，通过key和mask与，就可以得出数值 */
#define KBD_KEY_MASK	0x01FF		

#define KBD_FLAG_EXT	0x0100		/* Normal function keys		*/

/* 按键的一些标志 */
#define KBD_FLAG_BREAK	0x0080		/* Break Code			*/
#define KBD_FLAG_SHIFT_L	0x0200		/* Shift key			*/
#define KBD_FLAG_SHIFT_R	0x0400		/* Shift key			*/
#define KBD_FLAG_CTRL_L	0x0800		/* Control key			*/
#define KBD_FLAG_CTRL_R	0x1000		/* Control key			*/
#define KBD_FLAG_ALT_L	0x2000		/* Alternate key		*/
#define KBD_FLAG_ALT_R	0x4000		/* Alternate key		*/
#define KBD_FLAG_PAD	0x8000		/* keys in num pad		*/
#define KBD_FLAG_NUM	0x10000	/* 数字锁		*/
#define KBD_FLAG_CAPS	0x20000	/* 数字锁		*/

/* Special keys */
#define KBD_ESC		(0x01 + KBD_FLAG_EXT)	/* Esc		*/
#define KBD_TAB		(0x02 + KBD_FLAG_EXT)	/* Tab		*/
#define KBD_ENTER		(0x03 + KBD_FLAG_EXT)	/* Enter	*/
#define KBD_BACKSPACE	(0x04 + KBD_FLAG_EXT)	/* BackSpace	*/

#define KBD_GUI_L		(0x05 + KBD_FLAG_EXT)	/* L GUI	*/
#define KBD_GUI_R		(0x06 + KBD_FLAG_EXT)	/* R GUI	*/
#define KBD_APPS		(0x07 + KBD_FLAG_EXT)	/* APPS	*/

/* Shift, Ctrl, Alt */
#define KBD_SHIFT_L		(0x08 + KBD_FLAG_EXT)	/* L Shift	*/
#define KBD_SHIFT_R		(0x09 + KBD_FLAG_EXT)	/* R Shift	*/
#define KBD_CTRL_L		(0x0A + KBD_FLAG_EXT)	/* L Ctrl	*/
#define KBD_CTRL_R		(0x0B + KBD_FLAG_EXT)	/* R Ctrl	*/
#define KBD_ALT_L		(0x0C + KBD_FLAG_EXT)	/* L Alt	*/
#define KBD_ALT_R		(0x0D + KBD_FLAG_EXT)	/* R Alt	*/

/* Lock keys */
#define KBD_CAPS_LOCK	(0x0E + KBD_FLAG_EXT)	/* Caps Lock	*/
#define	KBD_NUM_LOCK	(0x0F + KBD_FLAG_EXT)	/* Number Lock	*/
#define KBD_SCROLL_LOCK	(0x10 + KBD_FLAG_EXT)	/* Scroll Lock	*/

/* Function keys */
#define KBD_F1		(0x11 + KBD_FLAG_EXT)	/* F1		*/
#define KBD_F2		(0x12 + KBD_FLAG_EXT)	/* F2		*/
#define KBD_F3		(0x13 + KBD_FLAG_EXT)	/* F3		*/
#define KBD_F4		(0x14 + KBD_FLAG_EXT)	/* F4		*/
#define KBD_F5		(0x15 + KBD_FLAG_EXT)	/* F5		*/
#define KBD_F6		(0x16 + KBD_FLAG_EXT)	/* F6		*/
#define KBD_F7		(0x17 + KBD_FLAG_EXT)	/* F7		*/
#define KBD_F8		(0x18 + KBD_FLAG_EXT)	/* F8		*/
#define KBD_F9		(0x19 + KBD_FLAG_EXT)	/* F9		*/
#define KBD_F10		(0x1A + KBD_FLAG_EXT)	/* F10		*/
#define KBD_F11		(0x1B + KBD_FLAG_EXT)	/* F11		*/
#define KBD_F12		(0x1C + KBD_FLAG_EXT)	/* F12		*/

/* Control Pad */
#define KBD_PRINTSCREEN	(0x1D + KBD_FLAG_EXT)	/* Print Screen	*/
#define KBD_PAUSEBREAK	(0x1E + KBD_FLAG_EXT)	/* Pause/Break	*/
#define KBD_INSERT		(0x1F + KBD_FLAG_EXT)	/* Insert	*/
#define KBD_DELETE		(0x20 + KBD_FLAG_EXT)	/* Delete	*/
#define KBD_HOME		(0x21 + KBD_FLAG_EXT)	/* Home		*/
#define KBD_END		(0x22 + KBD_FLAG_EXT)	/* End		*/
#define KBD_PAGEUP		(0x23 + KBD_FLAG_EXT)	/* Page Up	*/
#define KBD_PAGEDOWN	(0x24 + KBD_FLAG_EXT)	/* Page Down	*/
#define KBD_UP		(0x25 + KBD_FLAG_EXT)	/* Up		*/
#define KBD_DOWN		(0x26 + KBD_FLAG_EXT)	/* Down		*/
#define KBD_LEFT		(0x27 + KBD_FLAG_EXT)	/* Left		*/
#define KBD_RIGHT		(0x28 + KBD_FLAG_EXT)	/* Right	*/

/* ACPI keys */
#define KBD_POWER		(0x29 + KBD_FLAG_EXT)	/* Power	*/
#define KBD_SLEEP		(0x2A + KBD_FLAG_EXT)	/* Sleep	*/
#define KBD_WAKE		(0x2B + KBD_FLAG_EXT)	/* Wake Up	*/

/* Num Pad */
#define KBD_PAD_SLASH	(0x2C + KBD_FLAG_EXT)	/* /		*/
#define KBD_PAD_STAR	(0x2D + KBD_FLAG_EXT)	/* *		*/
#define KBD_PAD_MINUS	(0x2E + KBD_FLAG_EXT)	/* -		*/
#define KBD_PAD_PLUS	(0x2F + KBD_FLAG_EXT)	/* +		*/
#define KBD_PAD_ENTER	(0x30 + KBD_FLAG_EXT)	/* Enter	*/
#define KBD_PAD_DOT		(0x31 + KBD_FLAG_EXT)	/* .		*/
#define KBD_PAD_0		(0x32 + KBD_FLAG_EXT)	/* 0		*/
#define KBD_PAD_1		(0x33 + KBD_FLAG_EXT)	/* 1		*/
#define KBD_PAD_2		(0x34 + KBD_FLAG_EXT)	/* 2		*/
#define KBD_PAD_3		(0x35 + KBD_FLAG_EXT)	/* 3		*/
#define KBD_PAD_4		(0x36 + KBD_FLAG_EXT)	/* 4		*/
#define KBD_PAD_5		(0x37 + KBD_FLAG_EXT)	/* 5		*/
#define KBD_PAD_6		(0x38 + KBD_FLAG_EXT)	/* 6		*/
#define KBD_PAD_7		(0x39 + KBD_FLAG_EXT)	/* 7		*/
#define KBD_PAD_8		(0x3A + KBD_FLAG_EXT)	/* 8		*/
#define KBD_PAD_9		(0x3B + KBD_FLAG_EXT)	/* 9		*/
#define KBD_PAD_UP		KBD_PAD_8			/* Up		*/
#define KBD_PAD_DOWN	KBD_PAD_2			/* Down		*/
#define KBD_PAD_LEFT	KBD_PAD_4			/* Left		*/
#define KBD_PAD_RIGHT	KBD_PAD_6			/* Right	*/
#define KBD_PAD_HOME	KBD_PAD_7			/* Home		*/
#define KBD_PAD_END		KBD_PAD_1			/* End		*/
#define KBD_PAD_PAGEUP	KBD_PAD_9			/* Page Up	*/
#define KBD_PAD_PAGEDOWN	KBD_PAD_3			/* Page Down	*/
#define KBD_PAD_INS		KBD_PAD_0			/* Ins		*/
#define KBD_PAD_MID		KBD_PAD_5			/* Middle key	*/
#define KBD_PAD_DEL		KBD_PAD_DOT			/* Del		*/

/* 按键码 */
#define KEYCODE_NONE		0			/* 没有按键 */


PUBLIC int InitKeyboardDriver();
PUBLIC void ExitKeyboardDriver();

#endif  /* _DRIVER_CHAR_KEYBOARD_H */
