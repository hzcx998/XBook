/*
 * file:		include/fs/absurd/direntry.h
 * auther:		Jason Hu
 * time:		2019/9/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_ABSURD_DIR_ENTRY
#define _FS_ABSURD_DIR_ENTRY

#include <share/stdint.h>
#include <share/types.h>
#include <share/string.h>
#include <fs/absurd.h>
#include <fs/absurd/meta.h>

struct DirEntry {

    
    /* 元信息，必须放在最后面 */
    struct MetaInfo meta;
};


#endif  //_FS_ABSURD_DIR_ENTRY
