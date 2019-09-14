/*
 * file:		fs/absurd/main.c
 * auther:		Jason Hu
 * time:		2019/9/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <fs/absurd.h>
#include <share/string.h>

#include <fs/absurd/direntry.h>
#include <fs/absurd/meta.h>
#include <fs/absurd/superblock.h>
