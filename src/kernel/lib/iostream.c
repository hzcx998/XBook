/*
 * file:		kernel/lib/iostream.c
 * auther:		Jason Hu
 * time:		2019/9/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <book/arch.h>
#include <book/iostream.h>
#include <lib/string.h>

PUBLIC struct IoStream *CreateIoStream(char *name, size_t size)
{
	struct IoStream *stream = kmalloc(sizeof(struct IoStream), GFP_KERNEL);
	if (stream == NULL) 
		return stream;
	
	stream->start = kmalloc(size, GFP_KERNEL);
	if (stream->start == NULL) {
		kfree(stream);
		return NULL;
	}

	memset(stream->name, 0, IOSTREAM_NAMELEN);
	strcpy(stream->name, name);
	stream->size = size;
	stream->pos = 0;
	return stream;
}

PUBLIC int IoStreamSeek(struct IoStream *stream, unsigned offset, char flags)
{
	if (flags == IOSTREAM_SEEL_SET) {
		
		stream->pos = offset;
	}

	return 0;
}

PUBLIC int IoStreamRead(struct IoStream *stream, void *buffer, size_t size)
{
	unsigned char *buf = stream->start + stream->pos;

	memcpy(buffer, buf, size);
	
	return size;
}

PUBLIC int IoStreamWrite(struct IoStream *stream, void *buffer, unsigned int size)
{
	unsigned char *buf = stream->start + stream->pos;

	memcpy(buf, buffer, size);

	return size;
}

PUBLIC void DestoryIoStream(struct IoStream *stream)
{
	if (stream != NULL) {	
		kfree(stream->start);
		kfree(stream);
	}
}
