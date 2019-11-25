/*
 * file:		fs/bofs/device.c
 * auther:		Jason Hu
 * time:		2019/9/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <book/device.h>
#include <share/vsprintf.h>

#include <fs/bofs/device.h>
#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/bitmap.h>

#include <book/blk-buffer.h>
