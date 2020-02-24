/*
 * file:		lib/errno.c
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <lib/errno.h>

int errno = 0;

static char *errno_info[] = {
    "ENULL", 
    "EPERM", 
    "ENOFILE or ENOENT", 
    "ESRCH", 
    "EINTR", 
    "EIO", 
    "ENXIO", 
    "E2BIG", 
    "ENOEXEC", 
    "EBADF", 
    "ECHILD", 
    "EAGAIN", 
    "ENOMEM", 
    "EACCES", 
    "EFAULT", 
    "Unknown", 
    "EBUSY", 
    "EEXIST", 
    "EXDEV", 
    "ENODEV", 
    "ENOTDIR", 
    "EISDIR", 
    "EINVAL", 
    "ENFILE", 
    "EMFILE", 
    "ENOTTY", 
    "Unknown", 
    "EFBIG", 
    "ENOSPC", 
    "ESPIPE", 
    "EROFS", 
    "EMLINK", 
    "EPIPE", 
    "EDOM", 
    "ERANGE", 
    "Unknown", 
    "EDEADLOCK", 
    "EDEADLK", 
    "Unknown", 
    "ENAMETOOLONG", 
    "ENOLCK", 
    "ENOSYS", 
    "ENOTEMPTY", 
    "EILSEQ", 
};

char *strerror(int errnum)
{
    if (errnum > 0 && errnum < EMAXNR) {
        return errno_info[errnum];
    }
    return errno_info[0];
}

void perror(char *str)
{
    char *error = strerror(errno);
    printf("%s: %s\n", str, error);
}
