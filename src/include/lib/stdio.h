/*
 * file:		include/lib/stdio.h
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_STDIO_H
#define _LIB_STDIO_H

#include "stdint.h"
#include "stddef.h"

#include "file.h"
#include "dir.h"

#include "libio.h"

#define EOF     -1

/* 标准输入、输出、错误流 */
extern FILE *stdin, *stdout, *stderr; 

#ifndef BUFSIZ
#define BUFSIZ _IO_BUFSIZ
#endif /* BUFSIZ */


#define _P_tmpdir   "sys:/tmp"
#define L_tmpnam   (sizeof(_P_tmpdir) + 12)

#endif  /* _LIB_STDIO_H */
