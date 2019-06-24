/*
 * file:		include/share/stdarg.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_STDDEF_H
#define _SHARE_STDDEF_H


/*
 *这里是typedef类型的
 */
typedef unsigned int flags_t;
typedef unsigned int size_t;

/*
 *这里是define类型的
 */
#define WRITE_ONCE(var, val) \
    (*((volatile typeof(val) *)(&(var))) = (val))


#endif  /*_SHARE_STDDEF_H*/
