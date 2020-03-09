/*
 * file:		ffile.c
 * auther:		Jason Hu
 * time:		2020/2/130
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <file.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <vsprintf.h>

#define STREAM_FILE_BUF_SIZE  1024    /* 默认为单个文件打开多大的缓冲区 */

#define STREAM_FLAGS_UNUSED     0x00
#define STREAM_FLAGS_USING      0x01
#define STREAM_FLAGS_BINARY     0x02

/* 本文件是有缓冲区的c文件操作，fopen,fclose...以stream为操作对象 */

/* 保存在用户空间的一个数组 */
FILE stream_file_table[MAX_STREAM_FILE_NR] = {{0},};

/* 标准输入，输出，流文件 */
FILE *stdin, *stdout, *stderr; 

static int __stream_error;

/**
 * __alloc_stream_file - 分配一个流式文件
 * 
 * @return: 成功返回一个流文件描述，失败返回NULL
 */
FILE *__alloc_stream_file()
{
    FILE *file = &stream_file_table[0];
    int i;
    for (i = 0; i < MAX_STREAM_FILE_NR; i++) {
        if (file->flags == STREAM_FLAGS_UNUSED) {
            file->flags = STREAM_FLAGS_USING;
            return file;
        }
        file++;
    }
    return NULL;
}

/**
 * __free_stream_file - 释放一个流式文件
 * @stream: 文件流
 */
void __free_stream_file(FILE *stream)
{
    stream->flags = STREAM_FLAGS_UNUSED;
}

/**
 * __stream_file_mode - 字符模式转换成数字模式
 * @mode: 字符模式
 * 
 * @return: 返回字符模式对应的数字模式
 */
unsigned int __stream_file_mode(const char *mode)
{
    unsigned int mod = 0;
    char *p = (char *)mode;

    /* 分为3个类别：'r', 'w', 'a'
    每次都要判断*p是否有数据
     */
    switch (*p)
    {
    case 'r': /* 'r' 可以追加 '+' 和 'b' */
        p++;
        if (*p && *p == '+') { /* '+' 可以追加 'b' */
            p++;
            if (*p && *p == 'b') {
                /* 有追加 'b', mode='r+b' */
                mod = (O_RDWR | O_BINARY);
            } else { 
                /* 没有追加 'b', mode='r+' */
                mod = O_RDWR;
            }
        } else if (*p == 'b') { /* 'b' 可以追加 '+' */
            p++;
            if (*p && *p == '+') {
                /* 有追加 '+', mode='rb+' */
                mod = (O_RDWR | O_BINARY);
            } else { 
                /* 没有追加 '+', mode='rb' */
                mod = O_RDONLY | O_BINARY;
            }
        } else {
            mod = O_RDONLY; /* mode='r' */
        }
        break;
    case 'w': /* 'w'，可以追加 '+' 和 'b' */
        /* w会把文件截断，所以加上O_TRUNC标志 */
        p++;
        if (*p && *p == '+') { /* '+' 可以追加 'b' */
            p++;
            if (*p && *p == 'b') {
                /* 有追加 'b', mode='w+b' */
                mod = (O_RDWR | O_BINARY | O_TRUNC | O_CREAT);
            } else { 
                /* 没有追加 'b', mode='w+' */
                mod = (O_RDWR | O_TRUNC | O_CREAT);
            }
        } else if (*p == 'b') { /* 'b' 可以追加 '+' */
            p++;
            if (*p && *p == '+') {
                /* 有追加 '+', mode='wb+' */
                mod = (O_RDWR | O_TRUNC | O_BINARY | O_CREAT);
            } else { 
                /* 没有追加 '+', mode='wb' */
                mod = (O_WRONLY | O_TRUNC | O_BINARY | O_CREAT);
            }
        } else {
            mod = (O_WRONLY | O_TRUNC | O_CREAT); /* mode='w' */
        }
        break;
    case 'a': /* 'a'，可以追加 '+' 和 'b' */
        p++;
        if (*p && *p == '+') { /* '+' 可以追加 'b' */
            p++;
            if (*p && *p == 'b') {
                /* 有追加 'b', mode='a+b' */
                mod = (O_RDWR | O_APPEDN | O_BINARY | O_CREAT);
            } else { 
                /* 没有追加 'b', mode='a+' */
                mod = (O_RDWR | O_APPEDN | O_CREAT);
            }
        } else if (*p == 'b') { /* 'b' 可以追加 '+' */
            p++;
            if (*p && *p == '+') {
                /* 有追加 '+', mode='ab+' */
                mod = (O_RDWR | O_APPEDN | O_BINARY | O_CREAT);
            } else { 
                /* 没有追加 '+', mode='ab' */
                mod = (O_WRONLY | O_APPEDN | O_BINARY | O_CREAT);
            }
        } else {
            mod = (O_WRONLY | O_APPEDN | O_CREAT); /* mode='a' */
        }
        break;
    default:
        break;
    }
    return mod;
}

/**
 * __stream_file_flush - 把缓冲区中的数据刷新到文件系统中
 * @fp: 文件
 * 
 * pos就相当于要写入文件的大小
 * 只有当写入数据超过缓冲区大小，关闭文件，主动刷新的时候才会调用本函数
 * @return: 成功返回0，失败返回EOF
 */
int __stream_file_flush(FILE *fp)
{
    /* 位置是0，不刷新 */
    if (!fp->pos) 
        return EOF;
    /* 位置已经超过最大了，先把缓冲区中的数据写入到文件后，在进行写入 */
    if (write(fp->fd, fp->buf, fp->pos) != fp->pos)
        return EOF; /* 写入失败 */
    
    /* 把缓冲区置0 */
    memset(fp->buf, 0, fp->pos);
    /* 恢复位置 */
    fp->pos = 0;
    return 0;
}

/**
 * __stream_out_char - 输出一个字符到缓冲区
 * @c: 字符
 * @fp: 文件流
 * 
 * @return: 成功返回0，失败返回EOF
 */
int __stream_out_char(int c, FILE *fp)
{
    if (fp->pos >= STREAM_FILE_BUF_SIZE) {
        //printf("flush buffer");
        if (__stream_file_flush(fp) == EOF)
            return EOF;
    }
    /* 把数据保存到缓冲区中 */
    fp->buf[fp->pos] = c;
    fp->pos++;
    return 0;
}

/**
 * __stream_in_char - 读入一个字符到缓冲区
 * @fp: 文件流
 * 
 * @return: 成功返回0，失败返回EOF
 */
int __stream_in_char(FILE *fp)
{
    int ch = 0;
    if (read(fp->fd, &ch, 1) == -1) {
        return EOF;
    }
    return ch;
}

/**
 * feof - 文件是否结束
 * 
 * @return: 如果到达结尾，返回1，没有则返回0
 */
int feof(FILE *fp)
{
    return isfoot(fp->fd);
}

/**
 * fopen - 打卡一个流式文件
 * @path: 文件路径
 * @mode: 操作模式
 * 
 * @return: 成功返回流式指针，失败返回NULL
 */
FILE *fopen(const char *path, const char *mode)
{
    __stream_error = 0;
    /* 对参数进行检测 */
    if (path == NULL || mode == NULL || !strlen(mode)) {
        __stream_error = 1;
        return NULL;
    }

    /* 分配一个文件流 */
    FILE *file = __alloc_stream_file();
    if (file == NULL) {
        __stream_error = 1;
        return NULL;
    }
        
    /* 设置模式 */
    unsigned int mod;

    mod = __stream_file_mode(mode);
    /* 如果是二进制方式打开 */
    if (mod & O_BINARY) {
        file->flags |= STREAM_FLAGS_BINARY;
    }
    
    /* 反解析 */
    #if 0
    printf("\nfile mode: ");
    
    if (mod & O_RDONLY || mod & O_RDWR) {
        printf("read ");
    }
    if (mod & O_WRONLY || mod & O_RDWR) {
        printf("write ");
    }
    if (mod & O_EXEC) {
        printf("exec ");
    }
    if (mod & O_APPEDN) {
        printf("append ");
    }
    if (mod & O_BINARY) {
        printf("binary ");
    }
    if (mod & O_CREAT) {
        printf("create ");
    }
    printf("\n");
    #endif 
    void *buf = malloc(STREAM_FILE_BUF_SIZE);
    if (buf == NULL) {
        /* 打开失败，释放资源并返回 */
        __free_stream_file(file);
        __stream_error = 1;
        return NULL;
    }
    memset(buf, 0, STREAM_FILE_BUF_SIZE);
    
    file->buf = (char *)buf;
    
    file->pos = 0;    /* 指向头部 */
    /* 通过open打开文件 */
    int fd = open(path, mod);
    if (fd == -1) {
        /* 打开失败，释放资源并返回 */
        __free_stream_file(file);
        __stream_error = 1;
        return NULL;
    }
    /* 记录文件描述符 */
    file->fd = fd;

    __stream_error = 0;
    /* 返回文件流 */
    return file;
}

/**
 * fclose - 关闭文件
 * @stream: 文件流
 * 
 * 关闭一个流式文件
 */
int fclose(FILE *stream)
{
    if (!stream) {
        __stream_error = 1;
        return EOF;
    }
        
    /* 刷新缓冲区到文件系统 */
    __stream_file_flush(stream);

    /* 释放缓冲区 */
    free(stream->buf);
    /* 关闭文件描述符 */
    close(stream->fd);
    /* 释放文件流资源 */
    __free_stream_file(stream);
    __stream_error = 0;
    return 0;
}

/**
 * fputc - 把字符写入到fp所指向输出流中
 * @c: 字符
 * @fp: 文件指针
 * 
 * @return: 成功返回0，错误返回EOF
 */
int fputc(int c, FILE *fp)
{
    if (!fp) {
        __stream_error = 1;
        return EOF;
    }
    if (__stream_out_char(c, fp) == EOF) {
        __stream_error = 1;
        return EOF;
    }
    __stream_error = 0;
    return 0;
}

/**
 * fgetc - 把字符读取到fp所指向输入流中
 * @fp: 文件指针
 * 
 * @return: 成功返回读取的字符，错误返回EOF
 */
int fgetc(FILE *fp)
{
    if (!fp) {
        __stream_error = 1;
        return EOF;
    }
    int c;
    if ((c = __stream_in_char(fp)) == EOF) {
        __stream_error = 1;
        return EOF;
    }
    __stream_error = 0;
    return c;
}

/**
 * fputs - 把字符串写入到fp所指向输出流中
 * @s: 字符串
 * @fp: 文件指针
 * 
 * @return: 成功返回0，错误返回EOF
 */
int fputs(const char *s, FILE *fp)
{
    if (!s || !fp) {
        __stream_error = 1;
        return EOF;
    }
    
    /* 循环输出字符串 */
    while (*s) {
        if (__stream_out_char(*s++, fp) == EOF) {
            __stream_error = 1;    
            return EOF;
        }
            
    }
    __stream_error = 0;
    return 0;
}

/**
 * fputs - 把字符串写入到fp所指向输出流中
 * @str: 字符串缓冲区
 * @num: 表示从文件中读出的字符串不超过 n-1个字符。
 *       在读入的最后一个字符后加上串结束标志'\0'
 * @fp: 文件指针
 * 
 * 1. 在读出n-1个字符之前，如遇到了换行符或EOF，则读出结束。
 * 2. fgets函数也有返回值，其返回值是字符数组的首地址。
 * 
 * @return: 成功返回0，错误返回EOF
 */
char *fgets(char *str, int num, FILE *fp)
{
    if (!str || !fp || !num) {
        __stream_error = 1;
        return NULL;
    }
        
    int real = num - 1;
    /* 读取长度只有1，但是结尾要留给'\0'，所以错误 */
    if (!real) {
        __stream_error = 1;
        return NULL;
    }
    char *s = str;
    int i = 0;
    /* 循环输出字符串 */
    while (i < real) {
        /* 读取一个字符 */
        *s = __stream_in_char(fp);
        /* 遇到'\n'或者EOF，提前结束 */
        if (*s == '\n') {
            s++;    /* 指向下一个位置 */
            break;  /* 跳出 */
        } else if (*s == EOF && feof(fp)) {
            s++;    /* 指向下一个位置 */
            break;  /* 跳出 */
        } else if (*s == EOF) {
           __stream_error = 1;
            /* 单纯的错误 */
            return NULL;
        }
        s++;
        i++;
    }
    /* 正常的结尾留给null */
    *s = '\0';
    __stream_error = 0; 
    return str;
}
/** 
 * fprintf - 格式化打印输出到文件
 * @fp: 文件流
 * @fmt: 格式以及字符串
 * @...: 可变参数
 * 
 * @return: 成功返回0，失败返回EOF
 */
int fprintf(FILE *fp, const char *fmt, ...)
{
    if (!fp || !fmt) {
        __stream_error = 1;
        return EOF;
    }
        
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	vsprintf(buf, fmt, arg);
	
    if (fputs((const char *)buf, fp) == EOF) {
        __stream_error = 1;
        return EOF;
    } else {
        __stream_error = 0;
        return strlen(buf);
    }
}

/**
 * fflush - 刷新输出缓冲区到文件
 * @fp: 文件流
 * 
 * @return: 成功返回0，失败返回EOF
 */
int fflush(FILE *fp)
{
    if (fp == NULL) {
        /* 刷新全部 */
        int i;
        FILE *stream;
        for (i = 0; i < MAX_STREAM_FILE_NR; i++) {
            stream = &stream_file_table[i];
            if (stream->flags) {
                if (__stream_file_flush(stream) == EOF) {
                    __stream_error = 1;
                    return EOF;
                }
            }
        }
    } else {
        /* 刷新缓冲区到文件 */
        if (__stream_file_flush(fp) == EOF) {
            __stream_error = 1;
            return EOF;
        } else {
            __stream_error = 0;
            return 0;
        }
    }
    __stream_error = 0;
    return 0;
}

/**
 * fwrite - 写入二进制文件 
 * @buffer: 是一个指向用于保存数据的内存位置的指针，
 *          (是一个指针，对于fwrite来说，是要获取数据的地址）
 * @size:   是每次写入的字节数
 * @count:  是写入的次数
 * @stream: 是要写入的文件的指针,是数据写入的流（输出流）
 * 
 * @return: 返回实际写入的元素（并非字节）数目
 *          如果输出过程中出现了错误，
 *          这个数字可能比请求的元素数目要小
 */
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    /* 对参数检测 */
    if (!ptr || !stream || !size || !nmemb) {
        __stream_error = 1;
        return 0;               
    }

    unsigned char *bin = (unsigned char *) ptr;
    int length = size * nmemb;
    int i = 0;
    while (i < length) {
        if (__stream_out_char(*bin++, stream) == EOF) {
            __stream_error = 0;
            return i;
        }
        i++;
    }
    __stream_error = 0;
    return i;
}

/**
 * fread - 读取二进制文件 
 * @buffer: 是读取的数据存放的内存的指针，
 *          （可以是数组，也可以是新开辟的空间）
 *          是一个指向用于保存数据的内存位置的指针（为指向缓冲区
 *          保存或读取的数据或者是用于接收数据的内存地址）
 * @size:   是每次读取的字节数
 * @count:  是读取的次数
 * @stream: 是要读取的文件的指针,是数据读取的流（输入流）
 * 
 * @return: 成功: 返回实际读取的元素（并非字节）数目
 *          失败: 返回0
 *          如果输入过程中遇到了文件尾或者读取过程中出现了错误，这个数字可能比请求的元素数目要小
 */
size_t fread(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    /* 对参数检测 */
    if (ptr == NULL || stream == NULL || !size || !nmemb) {
        __stream_error = 1;
        return 0;
    }
        
    unsigned char *bin = (unsigned char *) ptr;
    int n = 0;
    /* 循环读取次数 */
    while (nmemb--) {
        /* 每次读取size大小 */
        n += read(stream->fd, bin, size);
        bin += size;
    }
    __stream_error = 0;
    return n;
}

int fileno(FILE *fp)
{
    if (!fp) {
        __stream_error = 1;
        return -1;
    }
    return fp->fd;
}

char *getenv(const char *name)
{
    /* 默认返回空 */
    return NULL;
}

/**
 * freopen - 使用不同的文件或模式重新打开流，即重定向。
 * 
 */
FILE *freopen(const char *fileName, const char *type, FILE *stream)
{
    FILE *fileFp = fopen(fileName, type);
    int fd1 = fileno(fileFp);
    int fd2 = fileno(stream);
    if(dup2(fd1, fd2) < 0) {
        __stream_error = 1;
        return NULL;
    } else {
        __stream_error = 0;
        return stream;
    }
}

/**
 * ferror -  测试给定流 stream 的错误标识符。
 * @stream: 这是指向 FILE 对象的指针，该 FILE 对象标识了流。
 * 
 * @return: 如果设置了与流关联的错误标识符，该函数返回一个非零值，否则返回一个零值。
*/
int ferror(FILE *stream)
{
    return __stream_error;
}

/**
 * tmpfile - 创建临时文件
 * 
 */
FILE *tmpfile(void)
{
    FILE *stream = fopen("sys:/tmp", "wb+");
    __stream_error = 0;
    return stream;
}

/**
 * ungetc - 退回缓冲区
 * 
 */
int ungetc(int c, FILE *stream)
{
    __stream_error = 1;
    /* 默认失败 */
    return EOF;
}

void clearerr(FILE *stream)
{
    __stream_error = 0;
}



/**
 * fseek - 重定位流上的文件指针 
 * 
 */
int fseek(FILE *stream, long offset, int fromwhere)
{
    if (!stream) {
        __stream_error = 1;
        return -1;
    }
    /* 先刷新进文件 */
    if (__stream_file_flush(stream)) {
        __stream_error = 1;
        return -1;
    }
    lseek(stream->fd, offset, fromwhere);
    
    __stream_error = 0;
    return 0;
}

/**
 * ftell - 告诉文件指针 
 * 
 */
long ftell(FILE *stream)
{
    if (!stream) {
        __stream_error = 1;
        return -1;
    }
    __stream_error = 0;
    return tell(stream->fd);
}

/**
 * setvbuf - 设置缓冲方式
 * 
 */
int setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
    /* 现在不进行设置 */
    __stream_error = 0;
    return 0;
}


char *tmp_names_head = "sys:/tmp";
char tmp_names_buf[12] = {0};
int tmp_names_idx = 0;
/**
 * tmpnam - 生成一个临时文件名
 * 
 */
char *tmpnam(char *str)
{
    memset(tmp_names_buf, 0, 12);
    sprintf(tmp_names_buf, "%s%d", tmp_names_head, tmp_names_idx);
    if (str == NULL) {
        return tmp_names_buf;
    } else {
        memcpy(str, tmp_names_buf, strlen(tmp_names_buf));
    }
    return str;
}