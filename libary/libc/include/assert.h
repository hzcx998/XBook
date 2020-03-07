/*
 * file:		include/lib/_G_config.h
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_ASSERT_H
#define _LIB_ASSERT_H

//断言
#define ASSERT_DBG
#ifdef ASSERT_DBG
void assert_failure(char *exp, char *file, char *baseFile, int line);
#define assert(exp)  if (exp) ; \
        else assert_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif


#endif  /* _LIB_ASSERT_H */
