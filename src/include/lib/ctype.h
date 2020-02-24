/*
 * file:		include/lib/ctype.h
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_CTYPE_H
#define _LIB_CTYPE_H

int isspace(char c);
int isalnum(int ch);
int isxdigit (int c);
int isdigit( int ch );
unsigned long strtoul(const char *cp,char **endp,unsigned int base);
long strtol(const char *cp,char **endp,unsigned int base);
int isalpha(int ch);
double strtod(const char* s, char** endptr);
double atof(char *str);
int tolower(int c);
int toupper(int c);


#endif  /* _LIB_CTYPE_H */
