/*
 * file:		include/lib/graph.h
 * auther:		Jason Hu
 * time:		2020/2/24
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_GRAPH_H
#define _LIB_GRAPH_H

#include "sys/kgc.h"

/* 输入键值定义0~512,0x200 */
enum GUI_KeyCode {
    GUI_KCOD_UNKNOWN      = 0,                /* unknown keycode */
    GUI_KCOD_FIRST,                           /* first key */
    GUI_KCOD_CLEAR,     /* clear */
    GUI_KCOD_PAUSE,   /* pause */
    GUI_KCOD_UP,           /* up arrow */
    GUI_KCOD_DOWN,         /* down arrow */
    GUI_KCOD_RIGHT,        /* right arrow */
    GUI_KCOD_LEFT,         /* left arrow */
    GUI_KCOD_BACKSPACE,    /* backspace */
    GUI_KCOD_TAB,          /* 9: tab */
    GUI_KCOD_INSERT,       /* insert */
    GUI_KCOD_HOME,         /* home */
    GUI_KCOD_END,          /* end */
    GUI_KCOD_ENTER,        /* 13: enter */
    GUI_KCOD_PAGEUP,       /* page up */
    GUI_KCOD_PAGEDOWN,     /* page down */
    GUI_KCOD_F1,           /* F1 */
    GUI_KCOD_F2,           /* F2 */
    GUI_KCOD_F3,           /* F3 */
    GUI_KCOD_F4,           /* F4 */
    GUI_KCOD_F5,           /* F5 */
    GUI_KCOD_F6,           /* F6 */
    GUI_KCOD_F7,           /* F7 */
    GUI_KCOD_F8,           /* F8 */
    GUI_KCOD_F9,           /* F9 */
    GUI_KCOD_F10,          /* F10 */
    GUI_KCOD_F11,          /* F11 */
    GUI_KCOD_ESCAPE,          /* 27: escape */
    GUI_KCOD_F12,          /* F12 */
    GUI_KCOD_F13,      /* F13 */
    GUI_KCOD_F14,      /* F14 */
    GUI_KCOD_F15,      /* F15 */
    /* 可显示字符按照ascill码排列 */
    GUI_KCOD_SPACE,              /*  space */
    GUI_KCOD_EXCLAIM,              /* ! exclamation mark */
    GUI_KCOD_QUOTEDBL,              /*" double quote */
    GUI_KCOD_HASH,              /* # hash */
    GUI_KCOD_DOLLAR,              /* $ dollar */
    GUI_KCOD_PERSENT,              /* % persent */
    GUI_KCOD_AMPERSAND,              /* & ampersand */
    GUI_KCOD_QUOTE,             /* ' single quote */
    GUI_KCOD_LEFTPAREN,              /* ( left parenthesis */
    GUI_KCOD_RIGHTPAREN,              /* ) right parenthesis */
    GUI_KCOD_ASTERISK,              /* * asterisk */
    GUI_KCOD_PLUS,              /* + plus sign */
    GUI_KCOD_COMMA,              /* , comma */
    GUI_KCOD_MINUS,              /* - minus sign */
    GUI_KCOD_PERIOD,              /* . period/full stop */
    GUI_KCOD_SLASH,              /* / forward slash */
    GUI_KCOD_0,              /* 0 */
    GUI_KCOD_1,              /* 1 */
    GUI_KCOD_2,              /* 2 */
    GUI_KCOD_3,              /* 3 */
    GUI_KCOD_4,              /* 4 */
    GUI_KCOD_5,              /* 5 */
    GUI_KCOD_6,              /* 6 */
    GUI_KCOD_7,              /* 7 */
    GUI_KCOD_8,              /* 8 */
    GUI_KCOD_9,              /* 9 */
    GUI_KCOD_COLON,              /* : colon */
    GUI_KCOD_SEMICOLON,              /* ;semicolon */
    GUI_KCOD_LESS,              /* < less-than sign */
    GUI_KCOD_EQUALS,              /* = equals sign */
    GUI_KCOD_GREATER,              /* > greater-then sign */
    GUI_KCOD_QUESTION,      /* ? question mark */
    GUI_KCOD_AT,              /* @ at */
    GUI_KCOD_A,              /* A */
    GUI_KCOD_B,              /* B */
    GUI_KCOD_C,              /* C */
    GUI_KCOD_D,              /* D */
    GUI_KCOD_E,              /* E */
    GUI_KCOD_F,              /* F */
    GUI_KCOD_G,              /* G */
    GUI_KCOD_H,              /* H */
    GUI_KCOD_I,              /* I */
    GUI_KCOD_J,              /* J */
    GUI_KCOD_K,              /* K */
    GUI_KCOD_L,              /* L */
    GUI_KCOD_M,              /* M */
    GUI_KCOD_N,              /* N */
    GUI_KCOD_O,              /* O */
    GUI_KCOD_P,              /* P */
    GUI_KCOD_Q,              /* Q */
    GUI_KCOD_R,              /* R */
    GUI_KCOD_S,              /* S */
    GUI_KCOD_T,              /* T */
    GUI_KCOD_U,              /* U */
    GUI_KCOD_V,              /* V */
    GUI_KCOD_W,              /* W */
    GUI_KCOD_X,              /* X */
    GUI_KCOD_Y,              /* Y */
    GUI_KCOD_Z,              /* Z */
    GUI_KCOD_LEFTSQUAREBRACKET,     /* [ left square bracket */
    GUI_KCOD_BACKSLASH,             /* \ backslash */
    GUI_KCOD_RIGHTSQUAREBRACKET,    /* ]right square bracket */
    GUI_KCOD_CARET,              /* ^ caret */
    GUI_KCOD_UNDERSCRE,              /* _ underscore */
    GUI_KCOD_BACKQUOTE,              /* ` grave */
    GUI_KCOD_a,              /* a */
    GUI_KCOD_b,              /* b */
    GUI_KCOD_c,              /* c */
    GUI_KCOD_d,              /* d */
    GUI_KCOD_e,              /* e */
    GUI_KCOD_f,              /* f */
    GUI_KCOD_g,              /* g */
    GUI_KCOD_h,              /* h */
    GUI_KCOD_i,              /* i */
    GUI_KCOD_j,              /* j */
    GUI_KCOD_k,              /* k */
    GUI_KCOD_l,              /* l */
    GUI_KCOD_m,              /* m */
    GUI_KCOD_n,              /* n */
    GUI_KCOD_o,              /* o */
    GUI_KCOD_p,              /* p */
    GUI_KCOD_q,              /* q */
    GUI_KCOD_r,              /* r */
    GUI_KCOD_s,              /* s */
    GUI_KCOD_t,              /* t */
    GUI_KCOD_u,              /* u */
    GUI_KCOD_v,              /* v */
    GUI_KCOD_w,              /* w */
    GUI_KCOD_x,              /* x */
    GUI_KCOD_y,              /* y */
    GUI_KCOD_z,              /* z */
    GUI_KCOD_LEFTBRACKET,              /* { left bracket */
    GUI_KCOD_VERTICAL,              /* | vertical virgul */
    GUI_KCOD_RIGHTBRACKET,              /* } left bracket */
    GUI_KCOD_TILDE,              /* ~ tilde */
    GUI_KCOD_DELETE,       /* 127 delete */
    GUI_KCOD_KP0,        /* keypad 0 */
    GUI_KCOD_KP1,        /* keypad 1 */
    GUI_KCOD_KP2,        /* keypad 2 */
    GUI_KCOD_KP3,        /* keypad 3 */
    GUI_KCOD_KP4,        /* keypad 4 */
    GUI_KCOD_KP5,        /* keypad 5 */
    GUI_KCOD_KP6,        /* keypad 6 */
    GUI_KCOD_KP7,        /* keypad 7 */
    GUI_KCOD_KP8,        /* keypad 8 */
    GUI_KCOD_KP9,        /* keypad 9 */
    GUI_KCOD_KP_PERIOD,      /* keypad period    '.' */
    GUI_KCOD_KP_DIVIDE,    /* keypad divide    '/' */
    GUI_KCOD_KP_MULTIPLY,     /* keypad multiply  '*' */
    GUI_KCOD_KP_MINUS,    /* keypad minus     '-' */
    GUI_KCOD_KP_PLUS,     /* keypad plus      '+' */
    GUI_KCOD_KP_ENTER,    /* keypad enter     '\r'*/
    GUI_KCOD_KP_EQUALS,    /* !keypad equals   '=' */
    GUI_KCOD_NUMLOCK,     /* numlock */
    GUI_KCOD_CAPSLOCK,    /* capslock */
    GUI_KCOD_SCROLLOCK,  /* scrollock */
    GUI_KCOD_RSHIFT,      /* right shift */
    GUI_KCOD_LSHIFT,      /* left shift */
    GUI_KCOD_RCTRL,       /* right ctrl */
    GUI_KCOD_LCTRL,       /* left ctrl */
    GUI_KCOD_RALT,        /* right alt / alt gr */
    GUI_KCOD_LALT,        /* left alt */
    GUI_KCOD_RMETA,   /* right meta */
    GUI_KCOD_LMETA,   /* left meta */
    GUI_KCOD_RSUPER,   /* right windows key */
    GUI_KCOD_LSUPER,   /* left windows key */
    GUI_KCOD_MODE,   /* mode shift */
    GUI_KCOD_COMPOSE,   /* compose */
    GUI_KCOD_HELP,   /* help */
    GUI_KCOD_PRINT,  /* print-screen */
    GUI_KCOD_SYSREQ,   /* sys rq */
    GUI_KCOD_BREAK,   /* break */
    GUI_KCOD_MENU,   /* menu */
    GUI_KCOD_POWER,   /* power */
    GUI_KCOD_EURO,   /* euro */
    GUI_KCOD_UNDO,   /* undo */
    GUI_KCOD_LAST       /* last one */        
};

/* 图形按键修饰 */
enum GUI_KeyModify {
    GUI_KMOD_NONE = 0,        /* 无按键修饰 */

    GUI_KMOD_NUM = 0x01,      /* 数字键 */
    GUI_KMOD_CAPS = 0x02,     /* 大写键 */

    GUI_KMOD_LCTRL = 0x04,    /* 左ctrl */
    GUI_KMOD_RCTRL = 0x08,    /* 右ctrl */
    GUI_KMOD_CTRL = (GUI_KMOD_LCTRL | GUI_KMOD_RCTRL),     /* 任意ctrl */
    
    GUI_KMOD_LSHIFT = 0x20,   /* 左shift */
    GUI_KMOD_RSHIFT = 0x40,   /* 右shift */
    GUI_KMOD_SHIFT = (GUI_KMOD_LSHIFT | GUI_KMOD_RSHIFT),    /* 任意shift */
    
    GUI_KMOD_LALT = 0x100,    /* 左alt */
    GUI_KMOD_RALT = 0x200,    /* 右alt */
    GUI_KMOD_ALT = (GUI_KMOD_LALT | GUI_KMOD_RALT),     /* 任意alt */

    GUI_KMOD_PAD = 0x400,    /* 小键盘按键 */    
};

/* 鼠标按钮 */
enum GUI_MouseButton {
    GUI_MOUSE_LEFT        = 0x01, /* 鼠标左键 */
    GUI_MOUSE_RIGHT       = 0x02, /* 鼠标右键 */
    GUI_MOUSE_MIDDLE      = 0x04, /* 鼠标中键 */
};

/* 生成argb颜色 */
#define GUI_ARGB(a, r, g, b) ((((a) & 0xff) << 24) | \
    (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)) 
#define GUI_RGB(a, r, g, b) GUI_ARGB(255, r, g, b)

/* 图形事件 */
typedef enum {
    GUI_EVEN_NONE = 0,          /* 无事件 */
    GUI_EVEN_KEY_DOWN,          /* 按键按下事件 */
    GUI_EVEN_KEY_UP,            /* 按键弹起事件 */
    GUI_EVEN_MOUSE_MOTION,      /* 鼠标移动事件 */
    GUI_EVEN_MOUSE_BUTTONDOWN,  /* 鼠标按钮按下事件 */
    GUI_EVEN_MOUSE_BUTTONUP,    /* 鼠标按钮弹起事件 */
    GUI_EVEN_TIMER,             /* 定时器产生 */
    GUI_EVEN_QUIT,              /* 退出事件 */
} GUI_EvenType_t;

/* 按键事件 */
typedef struct {
    GUI_EvenType_t type;          /* 事件类型 */ 
    int code;           /* 按键键值 */
    int modify;         /* 修饰按键 */
} GUI_EvenKey_t ;

/* 鼠标事件 */
typedef struct {
    GUI_EvenType_t type;          /* 事件类型 */ 
    int button;         /* 鼠标按钮 */
    int x;              /* 鼠标x */
    int y;              /* 鼠标y */
} GUI_EvenMouse_t ;

/* 定时器事件 */
typedef struct {
    GUI_EvenType_t type;          /* 事件类型 */ 
    int ticks;          /* 定时器产生时的ticks */
} GUI_EvenTimer_t ;

typedef union {
    GUI_EvenType_t type;    /* 事件类型 */
    GUI_EvenKey_t key;      /* 按键 */
    GUI_EvenMouse_t mouse;  /* 鼠标 */
    GUI_EvenTimer_t timer;    /* 定时器 */
} GUI_Even_t;

int GUI_CreateWindow(char *name, char *title, unsigned int style,
    int x, int y, int width, int height, void *param);
int GUI_CloseWindow();
int GUI_DrawPixel(int x, int y, unsigned int color);
int GUI_DrawRectangle(int x, int y, int width, int height, unsigned int color);
int GUI_DrawLine(int x0, int y0, int x1, int y1, unsigned int color);
int GUI_DrawBitmap(int x, int y, int width, int height, unsigned int *bitmap);
int GUI_DrawText(int x, int y, char *text, unsigned int color);
int GUI_Update(int left, int top, int right, int buttom);
int GUI_DrawPixelPlus(int x, int y, unsigned int color);
int GUI_DrawRectanglePlus(int x, int y, int width, int height, unsigned int color);
int GUI_DrawLinePlus(int x0, int y0, int x1, int y1, unsigned int color);
int GUI_DrawBitmapPlus(int x, int y, int width, int height, unsigned int *bitmap);
int GUI_DrawTextPlus(int x, int y, char *text, unsigned int color);

int GUI_PollEven(GUI_Even_t *even);

#endif /* _LIB_GRAPH_H */