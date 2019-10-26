/*
 * file:		include/book/blk-elevator.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_BLOCK_ELEVATOR_H
#define _BOOK_BLOCK_ELEVATOR_H

#include <share/types.h>
#include <book/blk-request.h>

PUBLIC void ElevatorIoSchedule(struct RequestQueue *queue, struct Request *request);

#endif   /* _BOOK_BLOCK_ELEVATOR_H */
