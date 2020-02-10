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

//func
void print_prompt();
int cmd_parse(char * cmd_str, char **argv, char token);
int read_key(char *start, char *buf, int count);
void readline( char *buf, uint32_t count);
void shell_newline();
int wait(int *status);

#endif  //_BOSH_H
