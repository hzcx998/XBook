/*
 * file:		kernel/kgc/window/tty.c
 * auther:		Jason Hu
 * time:		2020/2/19
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/task.h>
#include <video/video.h>
#include <char/chr-dev.h>
#include <kgc/input/mouse.h>
#include <kgc/window/message.h>
#include <kgc/window/window.h>
#include <kgc/window/draw.h>
#include <lib/string.h>
#include <lib/vsprintf.h>
#include <lib/ioctl.h>
#include <input/keycode.h>

#define DRV_NAME "gtty"

#define WIN_WIDTH   480
#define WIN_HEIGHT  360

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define FRAME_PAGES 10	//有多少个显存帧

#define CHAR_TABLE_LEN 4	//table表示多少个空格

#define SCROLL_UP 1	//向上滚屏
#define SCROLL_DOWN 2	//向下滚屏

typedef struct {
    int x, y;   /* 光标的位置 */
    int width, height;  /* 光标的宽高 */
    uint32_t color;     /* 光标的颜色 */
    int line;           /* 光标所在的行 */
} Cursor_t;

/* 图形tty终端 */
typedef struct {
    struct CharDevice *chrdev;  /* 字符设备 */
    int deviceID;               /* 设备ID，0 ~ MAX_TTY_NR-1 */
    KGC_Window_t *window;       /* 对应的tty窗口 */
    
    pid_t holdPid;              /* tty持有者进程id */

    uint8_t charWidth, charHeight;  /* 字符的宽度和高度 */
    int rows, columns ;   /* 行数和列数 */
    uint32_t backColor; /* 背景颜色 */
    uint32_t fontColor; /* 字体颜色 */
    
    size_t charBufferSize;  /* 字符缓冲大小 */
    int framePages;         /* 帧页数 */
    char *charBuffer;       /* 字符缓冲区 */
    char *charBufferCurrent;/* 字符缓冲区当前位置 */

    uint8_t tableLength;    /* table代表多少个空格：4，8常用 */
    
    Cursor_t cursor;    /* 光标 */
} Gtty_t;

/* gtty 表 */
Gtty_t gttyTabel[GTTY_MINORS];

PRIVATE void CursorDraw(Gtty_t *gtty);
PRIVATE int CanScrollUp(Gtty_t *gtty);
PRIVATE int CanScrollDown(Gtty_t *gtty);

PRIVATE int GttyPutc(struct Device *device, unsigned int ch);


PRIVATE INLINE void CharGet(Gtty_t *gtty, char *ch, int x, int y)
{

	*ch = gtty->charBufferCurrent[y * gtty->columns + x];
}

PRIVATE INLINE void CharSet(Gtty_t *gtty, char ch, int x, int y)
{
	//保存字符
	gtty->charBufferCurrent[y * gtty->columns + x] = ch;
}

/*
把光标从屏幕上去除
*/
PRIVATE void CursorClear(Gtty_t *gtty)
{
	//在光标位置绘制背景
	int x = gtty->cursor.x * gtty->charWidth;
	int y = gtty->cursor.y * gtty->charHeight;

	//绘制背景
	KGC_WindowDrawRectangle(gtty->window, x, y,
        gtty->charWidth, gtty->charHeight, gtty->backColor);
    KGC_WindowRefresh(gtty->window, x, y, x + gtty->charWidth, y + gtty->charHeight);
}

PRIVATE void CursorDraw(Gtty_t *gtty)
{
	//先在光标位置绘制背景，再绘制光标
	int x = gtty->cursor.x * gtty->charWidth;
	int y = gtty->cursor.y * gtty->charHeight;
	//绘制背景
	KGC_WindowDrawRectangle(gtty->window, x, y,
        gtty->charWidth, gtty->charHeight, gtty->backColor);
    
    //绘制光标
	KGC_WindowDrawRectangle(gtty->window, x, y,
        gtty->cursor.width, gtty->cursor.height, gtty->cursor.color);
    
	KGC_WindowRefresh(gtty->window, x, y, x + gtty->charWidth, y + gtty->charHeight);
}


void GttyLoadChars(Gtty_t *gtty)
{
	int bx, by, x, y;
	char ch;

	for (by = 0; by < gtty->rows; by++) {
		for (bx = 0; bx < gtty->columns; bx++) {
			CharGet(gtty, &ch, bx, by);
			
			x = bx * gtty->charWidth;
			y = by * gtty->charHeight;
			
			if (ch == '\n') {
				
			} else {
				KGC_WindowDrawChar(gtty->window, x, y, ch, gtty->fontColor);
			}
		}
    }
}


/*
向上或者向下滚动屏幕
dir 是方向
lines 是几行
accord 是不是自动
*/
PRIVATE void GttyScroll(Gtty_t *gtty, int dir, int lines, int accord)
{
	if (dir == SCROLL_UP) {
		//判断是否能滚屏
		if (!CanScrollUp(gtty)) {
            //如果不能向下滚屏就返回
			return;
		}

        //清空背景
		KGC_WindowDrawRectangle(gtty->window, 0, 0,
            gtty->window->width, gtty->window->height, gtty->backColor);
        
        //修改显存起始位置
		gtty->charBufferCurrent -= gtty->columns * lines;
            
		//把字符全部加载到窗口
		GttyLoadChars(gtty);

        /* 刷新全部 */
        KGC_WindowRefresh(gtty->window, 0, 0, gtty->window->width, gtty->window->height);

        gtty->cursor.x = 0;
		gtty->cursor.y += lines;
		if (gtty->cursor.y > gtty->rows - 1) {
			gtty->cursor.y = gtty->rows - 1;
		}
		//修改光标位置
		CursorDraw(gtty);
	} else if (dir == SCROLL_DOWN) {
		
		//判断是否能滚屏
		if (!CanScrollDown(gtty)) {
			//如果不能向下滚屏就返回
			return;
		}
		
		//清空背景
		KGC_WindowDrawRectangle(gtty->window, 0, 0,
            gtty->window->width, gtty->window->height, gtty->backColor);
        
		//修改显存起始位置
		gtty->charBufferCurrent += gtty->columns * lines;

		//把字符全部加载到窗口
		GttyLoadChars(gtty);

		if (!accord) {
            gtty->cursor.x = 0;
			gtty->cursor.y -= lines;
			if (gtty->cursor.y < 0) {
				gtty->cursor.y = 0;
			}
		}
        /* 刷新全部 */
        KGC_WindowRefresh(gtty->window, 0, 0, gtty->window->width, gtty->window->height);
        
		//修改光标位置
		CursorDraw(gtty);
	}
}

PRIVATE void CursorPositionCheck(Gtty_t *gtty)
{
	//如果光标向左移动超出，就切换到上一行最后
	if (gtty->cursor.x < 0) {
		
		if (gtty->cursor.y > 0) {
			//向左移动，如果发现y > 0，那么就可以移动行尾
			gtty->cursor.x = gtty->columns - 1;
		} else {
			//如果向左移动，发现y <= 0，那么就只能在行首
			gtty->cursor.x = 0;
		}
		//移动到上一行
		gtty->cursor.y--;
	}

	//如果光标向右移动超出，就切换到下一行
	if (gtty->cursor.x > gtty->columns - 1) {
        /* 如果行超出最大范围，就回到开头 */
        if (gtty->cursor.y < gtty->rows) {
            //如果y 没有到达最后一行，就移动到行首
			gtty->cursor.x = 0;
		} else {
			//如果y到达最后一行，就移动到行尾
			gtty->cursor.x = gtty->columns - 1;
		}
		//移动到下一行
		gtty->cursor.y++;
	}

	//如果光标向上移动超出，就修复
	if (gtty->cursor.y < 0) {
		//做修复处理
		gtty->cursor.y = 0;

	}

	//如果光标向下移动超出，就向下滚动屏幕
	if (gtty->cursor.y > gtty->rows -1) {
		//暂时做修复处理
		gtty->cursor.y = gtty->rows -1;

		GttyScroll(gtty, SCROLL_DOWN, 1, 1);
	}
}

/*
显示一个可见字符
*/
PRIVATE void GttyOutVisual(Gtty_t *gtty, char ch, int x, int y)
{
	if (0x20 <= ch && ch <= 0x7e) {
        KGC_WindowDrawChar(gtty->window, x, y,
        ch, gtty->fontColor);
        
    	KGC_WindowRefresh(gtty->window, x, y, 
        x + gtty->charWidth, y + gtty->charHeight);
	}
}

/*
光标移动一个位置
x是x方向上的移动
y是y方向上的移动
*/
void CursorMove(Gtty_t *gtty, int x, int y)
{
	//先把光标消除
	CursorClear(gtty);

	//把原来位置上的字符显示出来
	char ch;
	CharGet(gtty, &ch, gtty->cursor.x, gtty->cursor.y);
	
	//文字颜色
    GttyOutVisual(gtty, ch, gtty->cursor.x * gtty->charWidth,
        gtty->cursor.y * gtty->charHeight);

	//移动光标
	gtty->cursor.x = x;
	gtty->cursor.y = y;
	//修复位置
	CursorPositionCheck(gtty);

	//显示光标
	CursorDraw(gtty);
	//把光标所在的字符显示出来
	CharGet(gtty, &ch, gtty->cursor.x, gtty->cursor.y);
	
	//背景的颜色
	GttyOutVisual(gtty, ch, gtty->cursor.x * gtty->charWidth,
        gtty->cursor.y * gtty->charHeight);
}

PRIVATE int CanScrollUp(Gtty_t *gtty)
{
	if (gtty->charBufferCurrent > gtty->charBuffer) {
		return 1;
	}
	return 0;
}

PRIVATE int CanScrollDown(Gtty_t *gtty)
{
	if (gtty->charBufferCurrent < gtty->charBuffer + gtty->charBufferSize - \
        gtty->rows * gtty->columns) {
		return 1;
	}
	return 0;
}


PRIVATE void GttyOutChar(Gtty_t *gtty, char ch)
{
	//先把光标去除
	CursorClear(gtty);
    int counts;
	//对字符进行设定，如果是可显示字符就显示
	switch (ch) {
		case '\n':
			//光标的位置设定一个字符
			CharSet(gtty, ch, gtty->cursor.x, gtty->cursor.y);
            
			//能否回车
			if (CanScrollDown(gtty))
				CursorMove(gtty, 0, gtty->cursor.y + 1);
			
			break;
		case '\b':
			//改变位置
			gtty->cursor.x--;

			//改变位置后需要做检测，因为要写入字符
			CursorPositionCheck(gtty);

			CharSet(gtty, 0, gtty->cursor.x, gtty->cursor.y);

			CursorDraw(gtty);
			break;
		case '\t':
            /* 离当前位置有多少个字符 */
            counts = ((gtty->cursor.x + 4) & (~(4 - 1))) - gtty->cursor.x;
            while (counts--) {
                CharSet(gtty, ' ', gtty->cursor.x, gtty->cursor.y);
                CursorMove(gtty, gtty->cursor.x + 1, gtty->cursor.y);
            }
			break;
		default :
			CharSet(gtty, ch, gtty->cursor.x, gtty->cursor.y);

            CursorMove(gtty, gtty->cursor.x + 1, gtty->cursor.y);
            break;
	}
}


/*
清除屏幕上的所有东西，
字符缓冲区里面的文字
*/
PRIVATE void GttyClear(Gtty_t *gtty)
{
	//清空背景
    KGC_WindowDrawRectangle(gtty->window, 0, 0,
        gtty->window->width, gtty->window->height, gtty->backColor);
    KGC_WindowRefresh(gtty->window, 0, 0, 
        gtty->window->width, gtty->window->height);

	//清空字符缓冲区
	memset(gtty->charBuffer, 0, gtty->charBufferSize);

	//修改字符缓冲区指针
	gtty->charBufferCurrent = gtty->charBuffer;

	//重置光标
	gtty->cursor.x = 0;
	gtty->cursor.y = 0;

	//绘制光标
	CursorDraw(gtty);
}

int GttyOpen(struct Device *device, unsigned int flags)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Gtty_t *gtty = (Gtty_t *)chrdev->private;

    /* 打开一个窗口 */
    gtty->window = KGC_WindowCreate(device->name, device->name, 0, 0, 0, 
        WIN_WIDTH, WIN_HEIGHT, NULL);
    if (gtty->window == NULL) {
        printk("open gtty device failed!\n");
        return -1;
    }
    
    /* 设置字符宽高 */
    gtty->charWidth = CHAR_WIDTH;
    gtty->charHeight = CHAR_HEIGHT;
    
    /* 设置行列数量 */
    gtty->rows = gtty->window->height / gtty->charHeight;
    gtty->columns = gtty->window->width / gtty->charWidth;
    
    /* 背景颜色 */
    gtty->backColor = KGCC_BLACK;
    /* 字体颜色 */
    gtty->fontColor = KGCC_WHITE;
    
    /* 页帧数 */
    gtty->framePages = FRAME_PAGES;
    
    gtty->tableLength = CHAR_TABLE_LEN;
    
    gtty->holdPid = CurrentTask()->pid;

    /* 配置缓冲区 */
    gtty->charBufferSize = gtty->rows * gtty->columns * gtty->framePages;
    
    
    gtty->charBuffer = kmalloc(gtty->charBufferSize, GFP_KERNEL);
    if (gtty->charBuffer == NULL) {
        KGC_WindowDestroy(gtty->window);
        printk("alloc char buffer failed!\n");
        return -1;
    }
    memset(gtty->charBuffer, 0, gtty->charBufferSize);
    gtty->charBufferCurrent = gtty->charBuffer;
    
    /* 设置光标 */
    Cursor_t *cursor = &gtty->cursor;
    cursor->x = 0;
    cursor->y = 0;
    cursor->line = 0;
    cursor->width = gtty->charWidth;
    cursor->height = gtty->charHeight;
    
    cursor->color = KGCC_WHITE;
    
    /* 添加窗口 */
    KGC_WindowAdd(gtty->window);

    /* 绘制背景以及光标 */
    KGC_WindowDrawRectangle(gtty->window, 0, 0,
        gtty->window->width, gtty->window->height, gtty->backColor);

    CursorDraw(gtty);

    KGC_WindowRefresh(gtty->window, 0, 0, gtty->window->width, gtty->window->height);
    
    return 0;
}

int GttyClose(struct Device *device)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Gtty_t *gtty = (Gtty_t *)chrdev->private;

    //printk("close gtty\n");
    
    int retval = -1;
    if (gtty->charBuffer) {

        kfree(gtty->charBuffer);
        //printk("close char buffer\n");   
    }
    if (gtty->window) {
        retval = KGC_WindowClose(gtty->window);
        gtty->window = NULL;
        //printk("close windows\n");
    }
    gtty->holdPid = 0;
    //printk("close done\n");
    
    return retval;
}

/**
 * GttyGetc - 终端读取接口
 * @device: tty设备
 * 
 * 从tty读取键盘输入,根据len读取不同的长度
 */
PRIVATE int GttyGetc(struct Device *device)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Gtty_t *gtty = (Gtty_t *)chrdev->private;
    
    int retval = 0;
    char flags = 0;
    /* 从窗口的消息队列中读取按键 */
    if (gtty->holdPid == CurrentTask()->pid) {
        KGC_Message_t msg;
        if (!KGC_RecvMessage(&msg)) {
            /* 有消息产生 */
            switch (msg.type) {
            /* 处理按键按下事件 */
            case KGC_MSG_KEY_DOWN:
                /* 如果是特殊按键，会进行预处理，然后不传递给进程。 */
                /* 如果有ctrl键按下，就检测 */
                if (msg.key.modify & KGC_KMOD_CTRL) {                            
                    switch (msg.key.code) {
                    /* ctl + c 结束一个前台进程 */
                    case 'c':
                    case 'C':
                        if (gtty->holdPid > 0) {
                            /* 发送终止提示符 */
                            GttyPutc(device, '^');
                            GttyPutc(device, 'C');
                            //GttyPutc(device, '\n');
                            
                            SysKill(gtty->holdPid, SIGINT);
                            flags = 1;
                        }
                        break;
                    /* ctl + \ 结束一个前台进程 */
                    case '\\':
                        if (gtty->holdPid > 0) {
                            GttyPutc(device, '^');
                            GttyPutc(device, '\\');
                            //GttyPutc(device, '\n');
                            SysKill(gtty->holdPid, SIGQUIT);
                            flags = 1;
                        }
                        break;
                    /* ctl + z 让前台进程暂停运行 */
                    case 'z':
                    case 'Z':

                        if (gtty->holdPid > 0) {
                            GttyPutc(device, '^');
                            GttyPutc(device, 'Z');
                            //GttyPutc(device, '\n');
                            SysKill(gtty->holdPid, SIGTSTP);
                            flags = 1;
                        }
                        break;
                    /* ctl + x 恢复前台进程运行 */
                    case 'x':
                    case 'X':
                        if (gtty->holdPid > 0) {
                            GttyPutc(device, '^');
                            GttyPutc(device, 'X');
                            //GttyPutc(device, '\n');
                            SysKill(gtty->holdPid, SIGCONT);
                            flags = 1;
                        }
                        break;
                    default:
                        break;
                    }
                }
                /* 特殊按键转换成ascii码 */
                if (!flags) {
                    switch (msg.key.code)
                    {
                    case IKEY_ENTER:
                        retval = '\n';
                        break;
                    case IKEY_BACKSPACE:
                        retval = '\b';
                        break;
                    /*case IKEY_TAB:    tab不是用于显示输出，而是用于控制
                        retval = '\t';*/
                        break;
                    /* 需要忽略的按键 */
                    case IKEY_LSHIFT:
                    case IKEY_RSHIFT:
                    case IKEY_LCTRL:
                    case IKEY_RCTRL:
                    case IKEY_LALT:
                    case IKEY_RALT:
                        retval = 0;
                        break;
                        
                    default:
                        retval = msg.key.code;
                        break;
                    }
                }
                //retval = msg.key.code;
                    
                break;
            case KGC_MSG_QUIT:  /* 退出消息 */
                /* 退出执行 */
                SysKill(CurrentTask()->pid, SIGTERM);
                break;
            default: 
                break;
            }
        }
    } else {
        printk("getc: task %d not gtty %d holder %d, kill it!\n", CurrentTask()->pid, gtty->deviceID, gtty->holdPid);
        /* 不是前台任务进行读取，就会产生SIGTTIN */
        SysKill(CurrentTask()->pid, SIGTTIN);
        
    }
    return retval;
}

/**
 * GttyPutc - 终端写入接口
 * @device: 设备
 * 
 * 往tty对应的控制台写入字符数据
 * 由于Putc的效率可能比较低，所以采用write
 */
PRIVATE int GttyPutc(struct Device *device, unsigned int ch)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Gtty_t *gtty = (Gtty_t *)chrdev->private;
    
    if (gtty->holdPid == CurrentTask()->pid) {    
        //printk("%c", ch);
        GttyOutChar(gtty, ch);
    } else {
        /* 其它进程可以向此tty输出信息。 */
        //GttyOutChar(gtty, ch);
        //printk("putc: task %d not gtty %d holder %d, kill it!\n", CurrentTask()->pid, gtty->deviceID, gtty->holdPid);
        
        printk("task %d not holder %d, kill it!\n", CurrentTask()->pid, gtty->holdPid);
        /* 不是前台任务进行写入，就会产生SIGTTOU */
        SysKill(CurrentTask()->pid, SIGTTOU);
    }

    return 0;
}

/**
 * GttyIoctl - tty的IO控制
 * @device: 设备
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int GttyIoctl(struct Device *device, int cmd, int arg)
{
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Gtty_t *gtty = (Gtty_t *)chrdev->private;
    
	int retval = 0;
	switch (cmd)
	{
    case GTTY_IOCTL_CLEAR:
        GttyClear(gtty);
        break;
    case GTTY_IOCTL_HOLD:
        gtty->holdPid = arg;
        //printk("pid %d set gtty %d holder!\n", arg, gtty->deviceID);
        break;
    default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

PRIVATE struct DeviceOperations gttyOpSets = {
    .open   = GttyOpen,
    .close  = GttyClose,
    .ioctl  = GttyIoctl,
    .putc   = GttyPutc,
    .getc   = GttyGetc,
};

PRIVATE int GttyInitOne(Gtty_t *gtty)
{
    int id = gtty - gttyTabel;
    gtty->deviceID = id;
    gtty->window = NULL;
    gtty->holdPid = 0;

    /* 设置一个字符设备号 */
    gtty->chrdev = AllocCharDevice(MKDEV(GTTY_MAJOR, id));
	if (gtty->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for gtty failed!\n");
		return -1;
	}
    
	/* 初始化字符设备信息 */
	CharDeviceInit(gtty->chrdev, 1, gtty);
	CharDeviceSetup(gtty->chrdev, &gttyOpSets);

    char devname[DEVICE_NAME_LEN] = {0};
    sprintf(devname, "%s%d", DRV_NAME, id);
	CharDeviceSetName(gtty->chrdev, devname);
	//printk("gtty name %s\n", devname);
	/* 把字符设备添加到系统 */
	AddCharDevice(gtty->chrdev);
    return 0;
}

PUBLIC int InitGttyDriver()
{
    int i;
    for (i = 0; i < GTTY_MINORS; i++) {
        GttyInitOne(&gttyTabel[i]);
    }
    return 0;
}
