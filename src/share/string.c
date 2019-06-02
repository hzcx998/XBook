/*
 * file:		share/string.c
 * auther:	    Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/string.h>

/*
 * 功能: n个字符比对
 * 参数: s1     字符串1
 *      s2      字符串2
 *      s3      要比较的字符数
 * 返回: 0 表示字符串一样
 *      小于0 表示s1小于s2
 *      大于0 表示s1大于s2
 * 说明: 引导和加载完成后，就会跳到这里
 */
int strncmp (const char * s1, const char * s2, int n)
{ 
	if(!n)return(0);

	while (--n && *s1 && *s1 == *s2)
	{ 
		s1++;
		s2++;
	}
	return( *s1 - *s2 );
}

char* itoa(char ** ps, int val, int base)
{
	int m = val % base;
	int q = val / base;
	if (q) {
		itoa(ps, q, base);
	}
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps;
}

int atoi(const char *src)
{
    int s = 0;
    char is_minus = 0;
  
    while (*src == ' ') {
			src++; 
		}
  
	if (*src == '+' || *src == '-') {
        if (*src == '-') {
           is_minus = 1;
        }
        src++;
    } else if (*src < '0' || *src > '9') {
		 s = 2147483647;
        return s;
    }
  
    while (*src != '\0' && *src >= '0' && *src <= '9') {
        s = s * 10 + *src - '0';
        src++;
    }
    return s * (is_minus ? -1 : 1);
}

void *memset(void* src, uint8_t value, uint32_t size) 
{
	uint8_t* s = (uint8_t*)src;
	while (size-- > 0){
		*s++ = value;
	}
	return src;
}

void *memset16(void* src, uint16_t value, uint32_t size) 
{
	uint16_t* s = (uint16_t*)src;
	while (size-- > 0){
		*s++ = value;
	}
	return src;
}

void *memset32(void* src, uint32_t value, uint32_t size) 
{
	uint32_t* s = (uint32_t*)src;
	while (size-- > 0){
		*s++ = value;
	}
	return src;
}

void memcpy(void* dst_, const void* src_, uint32_t size) {
 
   uint8_t* dst = dst_;
   const uint8_t* src = src_;
   while (size-- > 0)
      *dst++ = *src++;
}

char* strcpy(char* dst_, const char* src_) {
  
   char* r = dst_;		       
   while((*dst_++ = *src_++));
   return r;
}

char* strncpy(char* dst_, char* src_, int n) 
{
  
   char* r = dst_;		      
   while((*dst_++ = *src_++) && n > 0) n--;
   return r;
}

uint32_t strlen(const char* str) {
  
   const char* p = str;
   while(*p++);
   return (p - str - 1);
}

int8_t strcmp (const char* a, const char* b) {
  
   while (*a != 0 && *a == *b) {
      a++;
      b++;
   }
   return *a < *b ? -1 : *a > *b;
}

int memcmp(const void * s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = (const char *)s1;
	const char * p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++,p1++,p2++) {
		if (*p1 != *p2) {
			return (*p1 - *p2);
		}
	}
	return 0;
}
char* strrchr(char* str, int c)
{
   
    char* ret = NULL;
    while (*str)
    {
        if (*str == (char)c)
            ret = (char *)str;
        str++;
    }
    if ((char)c == *str)
        ret = (char *)str;

    return ret;
}

char* strcat(char* strDest , const char* strSrc)
{
    char* address = strDest;
    while(*strDest)
    {
        strDest++;
    }
    while((*strDest++=*strSrc++));
    return (char* )address;
}

int strpos(char *str, char ch)
{
	int i = 0;
	int flags = 0;
	while(*str){
		if(*str == ch){
			flags = 1;	//find ch
			break;
		}
		i++;
		str++;
	}
	if(flags){
		return i;
	}else{
		return -1;	//str over but not found
	}
}

char *strncat(char *dst, const char *src, int n)
{
	char *ret = dst;
	while(*dst != '\0'){
		dst++;
	}
	while(n && (*dst++ = *src++) != '\0'){
		n--;
	}
	dst = '\0';
	return ret;
}

char *strchr(const char *s, int c)
{
    if(s == NULL)
    {
        return NULL;
    }

    while(*s != '\0')
    {
        if(*s == (char)c )
        {
            return (char *)s;
        }
        s++;
    }
    return NULL;
}

void* memmove(void* dst,const void* src,uint32_t count)
{
    char* tmpdst = (char*)dst;
    char* tmpsrc = (char*)src;

    if (tmpdst <= tmpsrc || tmpdst >= tmpsrc + count)
    {
        while(count--)
        {
            *tmpdst++ = *tmpsrc++; 
        }
    }
    else
    {
        tmpdst = tmpdst + count - 1;
        tmpsrc = tmpsrc + count - 1;
        while(count--)
        {
            *tmpdst-- = *tmpsrc--;
        }
    }

    return dst; 
}

char *itoa16_align(char * str, int num)
{
	char *	p = str;
	char	ch;
	int	i;
	//为0
	if(num == 0){
		*p++ = '0';
	}
	else{	//4位4位的分解出来
		for(i=28;i>=0;i-=4){		//从最高得4位开始
			ch = (num >> i) & 0xF;	//取得4位
			ch += '0';			//大于0就+'0'变成ASICA的数字
			if(ch > '9'){		//大于9就加上7变成ASICA的字母
				ch += 7;		
			}
			*p++ = ch;			//指针地址上记录下来。
		}
	}
	*p = 0;							//最后在指针地址后加个0用于字符串结束
	return str;
}

/*
 *本文件大部分都是从网上搜索到的代码，如有侵权，请联系我。
 */