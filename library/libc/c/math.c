/*
 * file:		math.c
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <math.h>
#define PI 3.14159265358979323846
#define e  2.7182818284590452354
#define ln_2 0.69314718055994530942
#define ln_10 2.30258509299404568402
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
double pow(double x, double y)
{
	return exp(y*ln(x));
}
/*
 * pow1 - 求指数
 * @x: 底数
 * @y: 指数
 * 
 * I make the python code to c code from net.
 * So I don't known how does it work, but it runs very well!
 * 2019.4.13 ^.^ return x^y
 */
int ipow (double x,int y) {
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
double _eee (double x)
{
    if(x>1e-3)
    {
        double ee = _eee (x/2);
        return ee*ee;
    }
    return 1 + x + x*x/2 + pow(x,3)/6 + pow(x,4)/24 + pow(x,5)/120;
}
/*
* exp - 求以自然常数e为底数的次幂
* @x: 指数
*/
double exp(double x)
{
    if(x<0) return 1/exp(-x);
    int n = (int)x;
    x -= n;//分割整数部分和小数部分分别计算
    double e1 = ipow(e,n);//计算整数部分
    double e2 = _eee(x);//计算小数部分
    return e1*e2;//计算结果
}

/*
	* fabs - 计算double的绝对值
	* @x 值
*/
double fabs(double x) {
	return x<0?-x:x;
}
/*
	* sin - 计算正弦值
	* @x 值
*/
double sin(double x) 
{
	double n = x,sum=0;
    int i = 1;
    do
    {
        sum += n;
        i++;
        n = -n * x*x / (2 * i - 1) / (2 * i - 2);
    } while (fabs(n)>=1e-10);
    return sum;
}
/*
	* sqrt - 计算平方根
	* @x 值
*/
double sqrt (double x)
{
    double xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f375a86- (i>>1);
    x = *(float*)&i;
    x = x*(1.5f-xhalf*x*x);
	x = 1.0 / x;
    return x;
}  
/*
	* cos - 计算余弦值
	* @x 值
*/
double cos(double x) 
{
	double tmp = sin (x);
	tmp *= tmp;
	return sqrt (1 - tmp);
}
/*
	* tan - 计算正切值
	* @x 值
*/
#define F2(x) (1/sqrt(1-x*x))
double simpson(double a, double b,int flag)
{
    double c = a + (b-a)/2;
    if(flag==1)
        return (1 / a+4*(1 / c)+(1 / b))*(b-a)/6;
    if(flag==2)
        return (F2(a)+4*F2(c)+F2(b))*(b-a)/6;
    return 0;   /* to erase control reaches end of non-void function ^_^ */
}

double asr2(double a, double b, double eps, double A,int flag)
{
    double c = a + (b-a)/2;
    double L = simpson(a, c,flag), R = simpson(c, b,flag);
    if(fabs(L+R-A) <= 15*eps) return L+R+(L+R-A)/15.0;
    return asr2(a, c, eps/2, L,flag) + asr2(c, b, eps/2, R,flag);
}

double asr(double a, double b, double eps,int flag)
{
    return asr2(a, b, eps, simpson(a, b,flag),flag);
}
/*
* ln - 计算loge (x)
* @x 真数
*/
double ln(double x)
{
    return asr(1,x,1e-8,1);
}
/*
* asin - arcsin
* @x 值
*/
double asin(double x)
{
    if(fabs(x)>1) return -1;
    double fl = 1.0;
    if(x<0) {fl*=-1;x*=-1;}
    if(fabs(x-1)<1e-7) return PI/2;//x为正负1时特判
    return (fl*asr(0,x,1e-8,2));//主体积分过程
}
/*
* acos - arccos
* @x 值
*/
double acos(double x)
{
    if(fabs(x)>1) return -1;
    return PI/2 - asin(x);//简单公式的应用
}
/*
* atan - arctan
* @x 值
*/
double atan(double x)
{
    if(x<0) return -atan(-x);
    if(x>1) return PI/2 - atan(1/x);
    if(x>1e-3) return 2*atan((sqrt(1+x*x)-1)/x);//递推地缩小自变量，使之接近0，保证泰勒公式的精度
    return x - pow(x,3)/3 + pow(x,5)/5 - pow(x,7)/7 + pow(x,9)/9;//泰勒公式
}

#undef PI
#undef e
#undef ln_2
#undef ln_10
#undef F2