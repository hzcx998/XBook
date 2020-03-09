/*
 * file:		include/input/keycode.h
 * auther:		Jason Hu
 * time:		2020/3/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _INPUT_KEYCODE_H
#define _INPUT_KEYCODE_H

#include <lib/stdint.h>
#include <lib/types.h>

/* 输入键值定义0~512,0x200 */
enum InputKeyCode {
    IKEY_UNKNOWN      = 0,                /* unknown keycode */
    IKEY_FIRST,                           /* first key */
    IKEY_CLEAR,     /* clear */
    IKEY_PAUSE,   /* pause */
    IKEY_UP,           /* up arrow */
    IKEY_DOWN,         /* down arrow */
    IKEY_RIGHT,        /* right arrow */
    IKEY_LEFT,         /* left arrow */
    IKEY_BACKSPACE,    /* backspace */
    IKEY_TAB,          /* 9: tab */
    IKEY_INSERT,       /* insert */
    IKEY_HOME,         /* home */
    IKEY_END,          /* end */
    IKEY_ENTER,        /* 13: enter */
    IKEY_PAGEUP,       /* page up */
    IKEY_PAGEDOWN,     /* page down */
    IKEY_F1,           /* F1 */
    IKEY_F2,           /* F2 */
    IKEY_F3,           /* F3 */
    IKEY_F4,           /* F4 */
    IKEY_F5,           /* F5 */
    IKEY_F6,           /* F6 */
    IKEY_F7,           /* F7 */
    IKEY_F8,           /* F8 */
    IKEY_F9,           /* F9 */
    IKEY_F10,          /* F10 */
    IKEY_F11,          /* F11 */
    IKEY_ESCAPE,          /* 27: escape */
    IKEY_F12,          /* F12 */
    IKEY_F13,      /* F13 */
    IKEY_F14,      /* F14 */
    IKEY_F15,      /* F15 */
    /* 可显示字符按照ascill码排列 */
    IKEY_SPACE,              /*  space */
    IKEY_EXCLAIM,              /* ! exclamation mark */
    IKEY_QUOTEDBL,              /*" double quote */
    IKEY_HASH,              /* # hash */
    IKEY_DOLLAR,              /* $ dollar */
    IKEY_PERSENT,              /* % persent */
    IKEY_AMPERSAND,              /* & ampersand */
    IKEY_QUOTE,             /* ' single quote */
    IKEY_LEFTPAREN,              /* ( left parenthesis */
    IKEY_RIGHTPAREN,              /* ) right parenthesis */
    IKEY_ASTERISK,              /* * asterisk */
    IKEY_PLUS,              /* + plus sign */
    IKEY_COMMA,              /* , comma */
    IKEY_MINUS,              /* - minus sign */
    IKEY_PERIOD,              /* . period/full stop */
    IKEY_SLASH,              /* / forward slash */
    IKEY_0,              /* 0 */
    IKEY_1,              /* 1 */
    IKEY_2,              /* 2 */
    IKEY_3,              /* 3 */
    IKEY_4,              /* 4 */
    IKEY_5,              /* 5 */
    IKEY_6,              /* 6 */
    IKEY_7,              /* 7 */
    IKEY_8,              /* 8 */
    IKEY_9,              /* 9 */
    IKEY_COLON,              /* : colon */
    IKEY_SEMICOLON,              /* ;semicolon */
    IKEY_LESS,              /* < less-than sign */
    IKEY_EQUALS,              /* = equals sign */
    IKEY_GREATER,              /* > greater-then sign */
    IKEY_QUESTION,      /* ? question mark */
    IKEY_AT,              /* @ at */
    IKEY_A,              /* A */
    IKEY_B,              /* B */
    IKEY_C,              /* C */
    IKEY_D,              /* D */
    IKEY_E,              /* E */
    IKEY_F,              /* F */
    IKEY_G,              /* G */
    IKEY_H,              /* H */
    IKEY_I,              /* I */
    IKEY_J,              /* J */
    IKEY_K,              /* K */
    IKEY_L,              /* L */
    IKEY_M,              /* M */
    IKEY_N,              /* N */
    IKEY_O,              /* O */
    IKEY_P,              /* P */
    IKEY_Q,              /* Q */
    IKEY_R,              /* R */
    IKEY_S,              /* S */
    IKEY_T,              /* T */
    IKEY_U,              /* U */
    IKEY_V,              /* V */
    IKEY_W,              /* W */
    IKEY_X,              /* X */
    IKEY_Y,              /* Y */
    IKEY_Z,              /* Z */
    IKEY_LEFTSQUAREBRACKET,     /* [ left square bracket */
    IKEY_BACKSLASH,             /* \ backslash */
    IKEY_RIGHTSQUAREBRACKET,    /* ]right square bracket */
    IKEY_CARET,              /* ^ caret */
    IKEY_UNDERSCRE,              /* _ underscore */
    IKEY_BACKQUOTE,              /* ` grave */
    IKEY_a,              /* a */
    IKEY_b,              /* b */
    IKEY_c,              /* c */
    IKEY_d,              /* d */
    IKEY_e,              /* e */
    IKEY_f,              /* f */
    IKEY_g,              /* g */
    IKEY_h,              /* h */
    IKEY_i,              /* i */
    IKEY_j,              /* j */
    IKEY_k,              /* k */
    IKEY_l,              /* l */
    IKEY_m,              /* m */
    IKEY_n,              /* n */
    IKEY_o,              /* o */
    IKEY_p,              /* p */
    IKEY_q,              /* q */
    IKEY_r,              /* r */
    IKEY_s,              /* s */
    IKEY_t,              /* t */
    IKEY_u,              /* u */
    IKEY_v,              /* v */
    IKEY_w,              /* w */
    IKEY_x,              /* x */
    IKEY_y,              /* y */
    IKEY_z,              /* z */
    IKEY_LEFTBRACKET,              /* { left bracket */
    IKEY_VERTICAL,              /* | vertical virgul */
    IKEY_RIGHTBRACKET,              /* } left bracket */
    IKEY_TILDE,              /* ~ tilde */
    IKEY_DELETE,       /* 127 delete */
    IKEY_KP0,        /* keypad 0 */
    IKEY_KP1,        /* keypad 1 */
    IKEY_KP2,        /* keypad 2 */
    IKEY_KP3,        /* keypad 3 */
    IKEY_KP4,        /* keypad 4 */
    IKEY_KP5,        /* keypad 5 */
    IKEY_KP6,        /* keypad 6 */
    IKEY_KP7,        /* keypad 7 */
    IKEY_KP8,        /* keypad 8 */
    IKEY_KP9,        /* keypad 9 */
    IKEY_KP_PERIOD,      /* keypad period    '.' */
    IKEY_KP_DIVIDE,    /* keypad divide    '/' */
    IKEY_KP_MULTIPLY,     /* keypad multiply  '*' */
    IKEY_KP_MINUS,    /* keypad minus     '-' */
    IKEY_KP_PLUS,     /* keypad plus      '+' */
    IKEY_KP_ENTER,    /* keypad enter     '\r'*/
    IKEY_KP_EQUALS,    /* !keypad equals   '=' */
    IKEY_NUMLOCK,     /* numlock */
    IKEY_CAPSLOCK,    /* capslock */
    IKEY_SCROLLOCK,  /* scrollock */
    IKEY_RSHIFT,      /* right shift */
    IKEY_LSHIFT,      /* left shift */
    IKEY_RCTRL,       /* right ctrl */
    IKEY_LCTRL,       /* left ctrl */
    IKEY_RALT,        /* right alt / alt gr */
    IKEY_LALT,        /* left alt */
    IKEY_RMETA,   /* right meta */
    IKEY_LMETA,   /* left meta */
    IKEY_RSUPER,   /* right windows key */
    IKEY_LSUPER,   /* left windows key */
    IKEY_MODE,   /* mode shift */
    IKEY_COMPOSE,   /* compose */
    IKEY_HELP,   /* help */
    IKEY_PRINT,  /* print-screen */
    IKEY_SYSREQ,   /* sys rq */
    IKEY_BREAK,   /* break */
    IKEY_MENU,   /* menu */
    IKEY_POWER,   /* power */
    IKEY_EURO,   /* euro */
    IKEY_UNDO,   /* undo */
    IKEY_LAST       /* last one */        
};

/* 控制标志 */
enum InputKeycodeFlags {
    IKEY_FLAG_KEY_MASK  = 0x1FF,        /* 键值的mask值 */
    IKEY_FLAG_SHIFT_L   = 0x0200,		/* Shift key			*/
    IKEY_FLAG_SHIFT_R   = 0x0400,		/* Shift key			*/
    IKEY_FLAG_CTRL_L    = 0x0800,		/* Control key			*/
    IKEY_FLAG_CTRL_R    = 0x1000,		/* Control key			*/
    IKEY_FLAG_ALT_L     = 0x2000,		/* Alternate key		*/
    IKEY_FLAG_ALT_R     = 0x4000,		/* Alternate key		*/
    IKEY_FLAG_PAD	    = 0x8000,		/* keys in num pad		*/
    IKEY_FLAG_NUM	    = 0x10000,	    /* 数字锁		*/
    IKEY_FLAG_CAPS	    = 0x20000,	    /* 数字锁		*/
    IKEY_FLAG_BREAK	    = 0x40000,		/* Break Code   */
};

#endif  /* _INPUT_KEYCODE_H */
