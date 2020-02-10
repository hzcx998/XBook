/*
 * file:		include/kgc/keycode.h
 * auther:		Jason Hu
 * time:		2020/2/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 键值 */
#ifndef _KGC_KEYCODE_H
#define _KGC_KEYCODE_H
/* KGC-kernel graph core 内核图形核心 */

#include <lib/types.h>
#include <lib/stdint.h>

#include <input/keyboard/ps2.h>

/* KGCK-kernel graph core key 内核图形核心键值 */

/* 图形按键定义 */
enum KGC_KeyCode {
    KGCK_UNKNOWN      = 0,                /* unknown keycode */
    KGCK_FIRST        = 0,                /* first key */
    KGCK_BACKSPACE    = KBD_BACKSPACE,    /* backspace */
    KGCK_TAB          = KBD_TAB,          /* tab */
    KGCK_CLEAR        = KGCK_UNKNOWN,   /* clear */
    KGCK_RETURN       = KBD_ENTER,        /* return */
    KGCK_ENTER        = KBD_ENTER,        /* enter */
    KGCK_PAUSE        = KBD_PAUSEBREAK,   /* pause */
    KGCK_ESCAPE       = KBD_ESC,          /* escape */
    KGCK_SPACE        = ' ',              /* space */
    KGCK_EXCLAIM      = '!',              /* exclamation mark */
    KGCK_QUOTEDBL     = '"',              /* double quote */
    KGCK_HASH         = '#',              /* hash */
    KGCK_DOLLAR       = '$',              /* dollar */
    KGCK_AMPERSAND    = '&',              /* ampersand */
    KGCK_QUOTE        = '\'',             /* single quote */
    KGCK_LEFTPAREN    = '(',              /* left parenthesis */
    KGCK_RIGHTPAREN   = ')',              /* right parenthesis */
    KGCK_ASTERISK     = '*',              /* asterisk */
    KGCK_PLUS         = '+',              /* plus sign */
    KGCK_COMMA        = ',',              /* comma */
    KGCK_MINUS        = '-',              /* minus sign */
    KGCK_PERIOD       = '.',              /* period/full stop */
    KGCK_SLASH        = '/',              /* forward slash */
    KGCK_0            = '0',              /* 0 */
    KGCK_1            = '1',              /* 1 */
    KGCK_2            = '2',              /* 2 */
    KGCK_3            = '3',              /* 3 */
    KGCK_4            = '4',              /* 4 */
    KGCK_5            = '5',              /* 5 */
    KGCK_6            = '6',              /* 6 */
    KGCK_7            = '7',              /* 7 */
    KGCK_8            = '8',              /* 8 */
    KGCK_9            = '9',              /* 9 */
    KGCK_COLON        = ':',              /* colon */
    KGCK_SEMICOLON    = ';',              /* semicolon */
    KGCK_LESS         = '<',              /* less-than sign */
    KGCK_EQUALS       = '=',              /* equals sign */
    KGCK_GREATER      = '>',              /* greater-then sign */
    KGCK_QUESTION     = '?',              /* question mark */
    KGCK_AT           = '@',              /* at */
    KGCK_LEFTBRACKET  = '[',              /* left bracket */
    KGCK_BACKSLASH    = '\\',             /* backslash */
    KGCK_RIGHTSLASH   = ']',              /* right bracket */
    KGCK_CARET        = '^',              /* caret */
    KGCK_UNDERSCRE    = '_',              /* underscore */
    KGCK_BACKQUOTE    = '`',              /* grave */
    KGCK_a            = 'a',              /* a */
    KGCK_b            = 'b',              /* b */
    KGCK_c            = 'c',              /* c */
    KGCK_d            = 'd',              /* d */
    KGCK_e            = 'e',              /* e */
    KGCK_f            = 'f',              /* f */
    KGCK_g            = 'g',              /* g */
    KGCK_h            = 'h',              /* h */
    KGCK_i            = 'i',              /* i */
    KGCK_j            = 'j',              /* j */
    KGCK_k            = 'k',              /* k */
    KGCK_l            = 'l',              /* l */
    KGCK_m            = 'm',              /* m */
    KGCK_n            = 'n',              /* n */
    KGCK_o            = 'o',              /* o */
    KGCK_p            = 'p',              /* p */
    KGCK_q            = 'q',              /* q */
    KGCK_r            = 'r',              /* r */
    KGCK_s            = 's',              /* s */
    KGCK_t            = 't',              /* t */
    KGCK_u            = 'u',              /* u */
    KGCK_v            = 'v',              /* v */
    KGCK_w            = 'w',              /* w */
    KGCK_x            = 'x',              /* x */
    KGCK_y            = 'y',              /* y */
    KGCK_z            = 'z',              /* z */
    KGCK_A            = 'A',              /* A */
    KGCK_B            = 'B',              /* B */
    KGCK_C            = 'C',              /* C */
    KGCK_D            = 'D',              /* D */
    KGCK_E            = 'E',              /* E */
    KGCK_F            = 'F',              /* F */
    KGCK_G            = 'G',              /* G */
    KGCK_H            = 'H',              /* H */
    KGCK_I            = 'I',              /* I */
    KGCK_J            = 'J',              /* J */
    KGCK_K            = 'K',              /* K */
    KGCK_L            = 'L',              /* L */
    KGCK_M            = 'M',              /* M */
    KGCK_N            = 'N',              /* N */
    KGCK_O            = 'O',              /* O */
    KGCK_P            = 'P',              /* P */
    KGCK_Q            = 'Q',              /* Q */
    KGCK_R            = 'R',              /* R */
    KGCK_S            = 'S',              /* S */
    KGCK_T            = 'T',              /* T */
    KGCK_U            = 'U',              /* U */
    KGCK_V            = 'V',              /* V */
    KGCK_W            = 'W',              /* W */
    KGCK_X            = 'X',              /* X */
    KGCK_Y            = 'Y',              /* Y */
    KGCK_Z            = 'Z',              /* Z */
    KGCK_DELETE       = KBD_DELETE,       /* delete */
    KGCK_KP0          = KBD_PAD_0,        /* keypad 0 */
    KGCK_KP1          = KBD_PAD_1,        /* keypad 1 */
    KGCK_KP2          = KBD_PAD_2,        /* keypad 2 */
    KGCK_KP3          = KBD_PAD_3,        /* keypad 3 */
    KGCK_KP4          = KBD_PAD_4,        /* keypad 4 */
    KGCK_KP5          = KBD_PAD_5,        /* keypad 5 */
    KGCK_KP6          = KBD_PAD_6,        /* keypad 6 */
    KGCK_KP7          = KBD_PAD_7,        /* keypad 7 */
    KGCK_KP8          = KBD_PAD_8,        /* keypad 8 */
    KGCK_KP9          = KBD_PAD_9,        /* keypad 9 */
    KGCK_KP_PERIOD    = KBD_PAD_DOT,      /* keypad period    '.' */
    KGCK_KP_DIVIDE    = KBD_PAD_SLASH,    /* keypad divide    '/' */
    KGCK_KP_MULTIPLY  = KBD_PAD_STAR,     /* keypad multiply  '*' */
    KGCK_KP_MINUS     = KBD_PAD_MINUS,    /* keypad minus     '-' */
    KGCK_KP_PLUS      = KBD_PAD_PLUS,     /* keypad plus      '+' */
    KGCK_KP_ENTER     = KBD_PAD_ENTER,    /* keypad enter     '\r'*/
    KGCK_KP_EQUALS    = KBD_PAD_ENTER,    /* !keypad equals   '=' */
    KGCK_UP           = KBD_UP,           /* up arrow */
    KGCK_DOWN         = KBD_DOWN,         /* down arrow */
    KGCK_RIGHT        = KBD_RIGHT,        /* right arrow */
    KGCK_LEFT         = KBD_LEFT,         /* left arrow */
    KGCK_INSERT       = KBD_INSERT,       /* insert */
    KGCK_HOME         = KBD_HOME,         /* home */
    KGCK_END          = KBD_END,          /* end */
    KGCK_PAGEUP       = KBD_PAGEUP,       /* page up */
    KGCK_PAGEDOWN     = KBD_PAGEDOWN,     /* page down */
    KGCK_F1           = KBD_F1,           /* F1 */
    KGCK_F2           = KBD_F2,           /* F2 */
    KGCK_F3           = KBD_F3,           /* F3 */
    KGCK_F4           = KBD_F4,           /* F4 */
    KGCK_F5           = KBD_F5,           /* F5 */
    KGCK_F6           = KBD_F6,           /* F6 */
    KGCK_F7           = KBD_F7,           /* F7 */
    KGCK_F8           = KBD_F8,           /* F8 */
    KGCK_F9           = KBD_F9,           /* F9 */
    KGCK_F10          = KBD_F10,          /* F10 */
    KGCK_F11          = KBD_F11,          /* F11 */
    KGCK_F12          = KBD_F12,          /* F12 */
    KGCK_F13          = KGCK_UNKNOWN,      /* F13 */
    KGCK_F14          = KGCK_UNKNOWN,      /* F14 */
    KGCK_F15          = KGCK_UNKNOWN,      /* F15 */
    KGCK_NUMLOCK      = KBD_NUM_LOCK,     /* numlock */
    KGCK_CAPSLOCK     = KBD_CAPS_LOCK,    /* capslock */
    KGCK_SCROLLOCK    = KBD_SCROLL_LOCK,  /* scrollock */
    KGCK_RSHIFT       = KBD_SHIFT_R,      /* right shift */
    KGCK_LSHIFT       = KBD_SHIFT_L,      /* left shift */
    KGCK_RCTRL        = KBD_CTRL_R,       /* right ctrl */
    KGCK_LCTRL        = KBD_CTRL_L,       /* left ctrl */
    KGCK_RALT         = KBD_ALT_R,        /* right alt / alt gr */
    KGCK_LALT         = KBD_ALT_L,        /* left alt */
    KGCK_RMETA        = KGCK_UNKNOWN,   /* right meta */
    KGCK_LMETA        = KGCK_UNKNOWN,   /* left meta */
    KGCK_RSUPER       = KGCK_UNKNOWN,   /* right windows key */
    KGCK_LSUPER       = KGCK_UNKNOWN,   /* left windows key */
    KGCK_MODE         = KGCK_UNKNOWN,   /* mode shift */
    KGCK_COMPOSE      = KGCK_UNKNOWN,   /* compose */
    KGCK_HELP         = KGCK_UNKNOWN,   /* help */
    KGCK_PRINT        = KBD_PRINTSCREEN,  /* print-screen */
    KGCK_SYSREQ       = KGCK_UNKNOWN,   /* sys rq */
    KGCK_BREAK        = KBD_PAUSEBREAK,   /* break */
    KGCK_MENU         = KGCK_UNKNOWN,   /* menu */
    KGCK_POWER        = KGCK_UNKNOWN,   /* power */
    KGCK_EURO         = KGCK_UNKNOWN,   /* euro */
    KGCK_UNDO         = KGCK_UNKNOWN,   /* undo */
    KGCK_LAST                             /* last one */        
};

/* 图形按键修饰 */
enum KGC_KeyMode {
    KGC_KMOD_NONE = 0,        /* 无按键修饰 */

    KGC_KMOD_NUM = 0x01,      /* 数字键 */
    KGC_KMOD_CAPS = 0x02,     /* 大写键 */

    KGC_KMOD_LCTRL = 0x04,    /* 左ctrl */
    KGC_KMOD_RCTRL = 0x08,    /* 右ctrl */
    KGC_KMOD_CTRL = (KGC_KMOD_LCTRL | KGC_KMOD_RCTRL),     /* 任意ctrl */
    
    KGC_KMOD_LSHIFT = 0x20,   /* 左shift */
    KGC_KMOD_RSHIFT = 0x40,   /* 右shift */
    KGC_KMOD_SHIFT = (KGC_KMOD_LSHIFT | KGC_KMOD_RSHIFT),    /* 任意shift */
    
    KGC_KMOD_LALT = 0x100,    /* 左alt */
    KGC_KMOD_RALT = 0x200,    /* 右alt */
    KGC_KMOD_ALT = (KGC_KMOD_LALT | KGC_KMOD_RALT),     /* 任意alt */

    KGC_KMOD_PAD = 0x400,    /* 小键盘按键 */    
};

/* 内核图形按键信息 */
typedef struct KGC_KeyInfo {
    int scanCode;       /* 扫描码 */
    int code;           /* 键值 */
    int modify;         /* 修饰按键 */
} KGC_KeyInfo_t;

#endif   /* _KGC_KEYCODE_H */
