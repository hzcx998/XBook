/*
 * file:		include/fs/node.h
 * auther:		Jason Hu
 * time:		2019/11/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FLAT_NODE_H
#define _FLAT_NODE_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/device.h>

void NodeFileInit(struct NodeFile *node,
	dev_t devno,
    unsigned int id);
int CleanNodeFile(struct NodeFile *node, struct SuperBlock *sb);
PUBLIC int SyncNodeFile(struct NodeFile *node, struct SuperBlock *sb);
void DumpNodeFile(struct NodeFile *node);


#endif	/* _FLAT_NODE_H */

