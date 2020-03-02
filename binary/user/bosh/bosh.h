#ifndef _BOSH_H
#define _BOSH_H

/*
BOSH 核心
*/

#define BOSH_NAME "bosh"

#define CMD_LINE_LEN 128
#define MAX_ARG_NR 16

#define TTY_CMD_CLEAN 0
#define TTY_CMD_HOLD 1


/* 重定向标志 */
#define RD_FLAG_TRANCE  0x01
#define RD_FLAG_APPEND  0x02
#define RD_FLAG_READ    0x04
#define RD_FLAG_MATCH   0x08
#define RD_FLAG_NEDD    0x10
#define RD_FLAG_OK      0x20
#define RD_FLAG_STDIN   0x40    /* 输入重定向 */
#define RD_FLAG_STDOUT  0x80    /* 输出重定向 */
#define RD_FLAG_STDERR  0x100   /* 错误重定向 */
#define RD_FLAG_DONE    0x200   /* 重定向完成 */

struct redirect_info {
    uint32_t flags;     /* 标志 */
    char *filename;     /* 重定向到的文件名 */
    uint8_t index;      /* 命令行索引 */
    int fd;        /* 重定向到的文件 */

};

//func
void print_prompt();
int cmd_parse(char * cmd_str, char **argv, char token);
int read_key(char *start, char *buf, int count);
void readline( char *buf, uint32_t count);
void shell_newline();
int wait(int *status);

#endif  //_BOSH_H
