/*
 * file:		include/sort.h
 * auther:		Jason Hu
 * time:		2020/3/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_SORT_H
#define _LIB_SORT_H

void swap(char * a, char * b, size_t width);
char * partition(char * lo, char * hi, int (*comp)(const void *, const void *), size_t width);
void sort(char * lo, char * hi, int (*comp)(const void *, const void *), size_t width);
void qsort(void * base, size_t num, size_t width, int (*comp)(const void *, const void *));

#endif  /*_LIB_SORT_H*/
