/*
 * file:		include/share/math.h
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_MATH_H
#define _SHARE_MATH_H

/* max() & min() */
#define	MAX(a,b)	((a) > (b) ? (a) : (b))
#define	MIN(a,b)	((a) < (b) ? (a) : (b))

#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))

int min(int a, int b);
int max(int a, int b);
int abs(int n);
int pow(int x, int y);

#endif /* _SHARE_MATH_H */
