/*
 * file:		conio.c
 * auther:		Jason Hu
 * time:		2020/3/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <conio.h>
#include <unistd.h>
#include <file.h>

int getch(void)
{
    /* 从控制台读取一个字符 */
    int buf;
    while (read(STDIN_FILENO, &buf, 1) == -1);
    return buf;
}

int putch(int ch)
{
    /* 往控制台写入一个字符 */
    return write(STDOUT_FILENO, &ch, 1);
}

