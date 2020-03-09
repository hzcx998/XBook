/*
 * file:		include/lib/bits/predefs.h
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_BITS_PREDEFS_H 
#define _LIB_BITS_PREDEFS_H 

/* Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _FEATURES_H
# error "Never use <bits/predefs.h> directly; include <features.h> instead."
#endif

#ifndef _PREDEFS_H
#define _PREDEFS_H

/* We do support the IEC 559 math functionality, real and complex.  */
#define __STDC_IEC_559__		1
#define __STDC_IEC_559_COMPLEX__	1

#endif /* predefs.h */

#endif  /* _LIB_BITS_PREDEFS_H  */
