/*
 * file:		assert.c
 * auther:		Jason Hu
 * time:		2010/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <conio.h>

void assert_failure(char *exp, char *file, char *baseFile, int line)
{
    printf("\nassert(%s) failed:\nfile: %s\nbase_file: %s\nln%d",
    	exp, file, baseFile, line);
    abort();
}
