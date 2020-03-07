/*
 * file:		include/lib/bits/wordsize.h
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_BITS_WORDSIZE_H 
#define _LIB_BITS_WORDSIZE_H

/* Determine the wordsize from the preprocessor defines.  */

#if defined __x86_64__
# define __WORDSIZE	64
# define __WORDSIZE_COMPAT32	1
#else
# define __WORDSIZE	32
#endif

#endif  /* _LIB_BITS_WORDSIZE_H  */
