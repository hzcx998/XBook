/*
 * file:		include/fs/bofs/bofs.h
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _FS_BOFS_H
#define _FS_BOFS_H

/*
Book File System (BOFS) -V0.2
这是对v0.1的升级。
0.2主要用于X内核
*/

#include <lib/types.h>
#include <lib/stdint.h>
#include <lib/const.h>

#include <fs/bofs/super_block.h>

/*
-----
super block
-----
sector bitmap
-----
inode bitmap
-----
inode table
-----
data
-----
*/

PUBLIC int InitBoFS();

#endif

