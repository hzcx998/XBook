/*
 * file:		share/math.c
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/math.h>

/*
 * max - 求最大值
 * @a: 第一个值
 * @b: 第二个值
 * 
 * 返回二者的大值
 */
int max(int a, int b)
{
	return a>b?a:b;
}

/*
 * min - 求最大值
 * @a: 第一个值
 * @b: 第二个值
 * 
 * 返回二者的小值
 */
int min(int a, int b)
{
	return a<b?a:b;
}
/*
 * abs - 求绝对值
 * @n: 要求的值
 */
int abs(int n)
{
	return n<0?-n:n;
}
/*
 * pow - 求指数
 * @x: 底数
 * @y: 指数
 * 
 * I make the python code to c code from net.
 * So I don't known how does it work, but it runs very well!
 * 2019.4.13 ^.^ return x^y
 */
int pow(int x, int y)
{
	int r = 1;
	while(y > 1){
		if((y&1) == 1){
			r *= x;
		}
		x *= x;
		y >>= 1;
	}
	return r * x;
}
