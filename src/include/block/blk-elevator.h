/*
 * file:		include/block/blk-elevator.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BLOCK_ELEVATOR_H
#define _BLOCK_ELEVATOR_H

#include <lib/types.h>
#include <block/blk-request.h>

PUBLIC void ElevatorIoSchedule(struct RequestQueue *queue, struct Request *request);

#endif   /* _BLOCK_ELEVATOR_H */
