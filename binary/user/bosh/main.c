#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <const.h>
#include <unistd.h>
#include <conio.h>
#include <signal.h>
#include <ioctl.h>
#include <ctype.h>

#include "bosh.h"
#include "cmd.h"



char cmd_line[CMD_LINE_LEN] = {0};
char cwd_cache[MAX_PATH_LEN] = {0};
char *cmd_argv[MAX_ARG_NR];

char tty_path[MAX_PATH_LEN] = {0};

int bosh_stdin; /* 标准输入文件时tty，退出时，需要抛弃持有者 */ 
int bosh_stdout;
int bosh_stderr;

/* 标准输入输出，错误的备份 */
int bosh_stdin_backup;
int bosh_stdout_backup;
int bosh_stderr_backup;

static int execute_cmd(int argc, char **argv, uint32_t redirect_mask);

void signal_handler(int signo) 
{
    /*printf("get a sig no %d in user mode.\n", signo);
    
    printf("%d-%d-%d\n", bosh_stdin, bosh_stdout, bosh_stderr);
    */
    /* 关闭tty并退出 */
    close(bosh_stdin);
    close(bosh_stdout);
    close(bosh_stderr);
    exit(0);
}

int main(int argc, char *argv0[])
{
    /* 根据传入的参数来初始化shell */
    if (argc < 2) {
        printf("argc must >= 2, now exit!\n");  
        return -1;
    }
    /* 需要关闭旧gtty */
    if (argc == 3) {
        close(0);
        close(1);
        close(2);
    }
    /* 保存对应的tty路径 */
    memset(tty_path, 0, MAX_PATH_LEN);
    strcpy(tty_path, argv0[1]);
    
    /* 打开标准输入输出设备，tty */
    bosh_stdin = open(argv0[1], O_RDONLY);
    if (bosh_stdin < 0)
        return -1;

    bosh_stdout = open(argv0[1], O_WRONLY);
    if (bosh_stdout < 0)
        return -1;

    bosh_stderr = open(argv0[1], O_WRONLY);
    if (bosh_stderr < 0)
        return -1;
    
    // 把当前进程设置为tty持有者
    ioctl(bosh_stdin, GTTY_IOCTL_HOLD, getpid());
    
	memset(cwd_cache, 0, MAX_PATH_LEN);
    
	getcwd(cwd_cache, 32);

    /* 注册结束信号，当要关闭时，需要关闭tty，并退出运行 */
    struct sigaction act, oldact;
    act.sa_handler = signal_handler;
    sigaction(SIGTERM, &act, &oldact);

    signal(SIGTTIN, signal_handler);
    signal(SIGTTOU, signal_handler);
    
    /* 忽略信号 */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    
    /* 复制标准输入和输出作为备份 */
    bosh_stdin_backup = dup(STDIN_FILENO);
    //printf("stdin: %d\n", bosh_stdin_backup);
    
    /* 备份标准输出 */
    bosh_stdout_backup = dup(STDOUT_FILENO);
    //printf("stdout: %d\n", bosh_stdout_backup);
    
    /* 备份标准错误 */
    bosh_stderr_backup = dup(STDERR_FILENO);
    //printf("stderr: %d\n", bosh_stderr_backup);
    
    /* 备份标准输入 */
	while(1){ 
        /* 显示提示符 */
		print_prompt();
        
		memset(cmd_line, 0, CMD_LINE_LEN);
		/* 读取命令行 */
		readline(cmd_line, CMD_LINE_LEN);
		
        /* 如果什么也没有输入，就回到开始处 */
		if(cmd_line[0] == 0){
			continue;
		}

        /* 查找管道符号 */
        char *pipe_symbol = strchr(cmd_line, '|');
        if (pipe_symbol) {
            /* 支持多重管道操作，如cmd1 | cmd2 | cmd3| cmdn */
            int pipe_flag = 0;

            /* 1.准备管道 */
            int pipe_fd[2] = {-1};  
            if (pipe(pipe_fd)) {
                printf("create pipe failed!\n");
                continue;
            }

            redirect(STDOUT_FILENO, pipe_fd[1]);

            /* 2.获取第一个命令 */
            char *each_cmd = cmd_line;
            pipe_symbol = strchr(each_cmd, '|');
            
            /* 把'|'替换成字符串终止符'\0' */
            *pipe_symbol = '\0';

            /* 解析成参数 */
            argc = -1;
            argc = cmd_parse(each_cmd, cmd_argv, ' ');
            if (argc == -1) {
                /* 执行失败后，就不往后面运行了 */
                /* 恢复标准输出重定向 */
                redirect(STDOUT_FILENO, bosh_stdout_backup);
                /* 关闭管道 */
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                printf("bosh: num of arguments exceed %d\n",MAX_ARG_NR);
                
                continue;
            }
            /* 执行第一个命令：允许输入重定向 */
            if (execute_cmd(argc, cmd_argv, RD_FLAG_STDIN)) {
                /* 执行失败后，就不往后面运行了 */
                /* 恢复标准输出重定向 */
                redirect(STDOUT_FILENO, bosh_stdout_backup);
                /* 关闭管道 */
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                printf("bosh: execute cmd %s falied!\n", cmd_argv[0]);
                    
                continue;
            }

            /* 跨过'|',处理下一个命令 */
            each_cmd = pipe_symbol + 1;

            redirect(STDIN_FILENO, pipe_fd[0]);

            /* 3.处理中间的命令，如果后面还有|就来处理 */
            while ((pipe_symbol = strchr(each_cmd, '|'))) {
                /* 把'|'替换成字符串终止符'\0' */
                *pipe_symbol = '\0';
                argc = -1;
                argc = cmd_parse(each_cmd, cmd_argv, ' ');
                if (argc == -1) {
                    /* 执行失败后，就不往后面运行了 */
                    /* 恢复标准输出重定向 */
                    redirect(STDOUT_FILENO, bosh_stdout_backup);
                    /* 恢复标准输出重定向 */
                    redirect(STDIN_FILENO, bosh_stdin_backup);
                    /* 关闭管道 */
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                    pipe_flag = 1;
                    printf("bosh: num of arguments exceed %d\n",MAX_ARG_NR);
                
                    break;
                }
                /* 执行中间命令：不允许输入输出重定向 */
                if (execute_cmd(argc, cmd_argv, 0)) {
                    /* 执行失败后，就不往后面运行了 */
                    /* 恢复标准输出重定向 */
                    redirect(STDOUT_FILENO, bosh_stdout_backup);
                    /* 恢复标准输出重定向 */
                    redirect(STDIN_FILENO, bosh_stdin_backup);
                    /* 关闭管道 */
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                    pipe_flag = 1;
                    printf("bosh: execute cmd %s falied!\n", cmd_argv[0]);
                    break;
                    
                }

                each_cmd = pipe_symbol + 1;
            }
            /* 如果执行失败，就回到最开头 */
            if (pipe_flag) {
                continue;
            }       

            /* 4.处理最后一个命令 */
            /* 恢复标准输出重定向 */
            redirect(STDOUT_FILENO, bosh_stdout_backup);

            argc = -1;
            argc = cmd_parse(each_cmd, cmd_argv, ' ');
            if (argc == -1) {
                /* 执行失败后，就不往后面运行了 */
                /* 恢复标准输出重定向 */
                redirect(STDIN_FILENO, bosh_stdin_backup);
                /* 关闭管道 */
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                printf("bosh: num of arguments exceed %d\n",MAX_ARG_NR);
                
                continue;
            }

            /* 执行最后一个命令：允许输出重定向 */
            if (execute_cmd(argc, cmd_argv, RD_FLAG_STDOUT | RD_FLAG_STDERR)) {
                printf("bosh: execute cmd %s falied!\n", cmd_argv[0]);
            }

            /* 5.恢复标准输入重定向 */
            redirect(STDIN_FILENO, bosh_stdin_backup);

            /* 6.关闭管道 */
            close(pipe_fd[0]);
            close(pipe_fd[1]);

        } else {
            /* 解析成参数 */
            argc = -1;
            argc = cmd_parse(cmd_line, cmd_argv, ' ');
            if(argc == -1){
                printf("bosh: num of arguments exceed %d\n",MAX_ARG_NR);
                continue;
            }
            /* 管道执行 */
            if (execute_cmd(argc, cmd_argv, RD_FLAG_STDIN | RD_FLAG_STDOUT | RD_FLAG_STDERR)) {
                printf("bosh: execute cmd %s falied!\n", cmd_argv[0]);
            }
        }
    }

    while (1);
}

static void redirect_init(struct redirect_info *rd)
{
    rd->filename = NULL;
    rd->flags = 0;
    rd->index = 0;
}

/* 返回临时文件描述符 */
static int redirect_here_document(char *delimter)
{
    int ch;
    char line[80] = {0};
    int idx = 0;

    /* 打开一个临时文件 */
    int fd = open(tmpnam(NULL), O_CREAT | O_RDWR | O_TRUNC);
    if (fd < 0) {
        printf("open tmp file failed!\n");
        return -1;
    }
    //printf("delimter:%s\n", delimter);
    /* 输入提示符 */
    printf("> ");
    while (1) {
        if (read(STDIN_FILENO, &ch, 4) != -1) {
            if (ch == '\b') {   /* 删除 */
                if (idx > 0) {
                    --idx;
                    line[idx] = '\0';   /* 设置为0 */    
                    /* 终端也要向前移动一个位置 */
                    printf("\b");
                }
            } else if (ch == '\n') {   /* 换行 */
                /* 查看内容是否和delimter匹配，如果是就退出 */
                if (!strcmp(line, delimter)) {
                    break;
                }

                /* 先在文本末尾添加0 */
                if (idx < 80)
                    line[idx++] = ch;
                
                /* 写入文件保存 */
                write(fd, line, idx);

                memset(line, 0, 80);
                idx = 0;
                /* 输入提示符 */
                printf("\n> ");
            } else {
                if (idx < 80) {
                    line[idx++] = ch;
                    printf("%c", ch);
                } else {    /* 回车 */
                    /* 查看内容是否和delimter匹配，如果是就退出 */
                    if (!strcmp(line, delimter)) {
                        break;
                    }
                    
                    /* 先在文本末尾添加0 */
                    if (idx < 80)
                        line[idx++] = ch;
                    
                    /* 写入文件保存 */
                    write(fd, line, idx);
                    
                    memset(line, 0, 80);
                    idx = 0;
                    
                    /* 输入提示符 */
                    printf("\n> ");
                }
            }            
        }
    }
    /* 现在数据已经写入到临时文件，需要调整pos到最开始，后面才能正常读取 */
    /*
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    char buf[128] = {0};
    read(fd, buf, size);
    printf("%s\n", buf);
    */
    lseek(fd, 0, SEEK_SET);

    return fd;
}

static int redirect_do_stdin(struct redirect_info *rdin, uint32_t redirect_mask, int *argc)
{
    uint32_t open_flags = 0;
    char *p;
    
    if ((rdin->flags & RD_FLAG_OK) && (redirect_mask & RD_FLAG_STDIN)) {
        if (rdin->flags & RD_FLAG_READ) {
            //printf("\nstdin redirect to %s with read.\n", rdin->filename);
            open_flags = O_RDONLY;  /* 只读形式打开 */
    
            /* 打开文件 */
            p = rdin->filename;
            /* 如果第一个是'&'，那么后面就可能跟着文件描述符 */
            if (*p == '&' && isdigitstr(p + 1)) {
                rdin->fd = atoi(p + 1);
            } else {
                rdin->fd = open(rdin->filename, open_flags);
            }
            if (rdin->fd < 0) {
                printf("open file redirect failed!\n");    
                return -1;
            }
        } else if (rdin->flags & RD_FLAG_MATCH) {
            //printf("\nstdin redirect to match with %s.\n", rdin->filename);
            /* 需要从键盘读取数据，直到和filename一致，才退出，然后传递给程序 */

            /* 从键盘读取输入后，写入临时文件。返回临时文件描述符。执行完后关闭临时文件。 */
            rdin->fd = redirect_here_document(rdin->filename);
            if (rdin->fd < 0) {
                printf("open file redirect failed!\n");    
                return -1;
            }
        }
        
        //printf("out index:%d, argc:%d\n", rdin->index, *argc);
        
        /* 修改参数数量 */
        if (rdin->index < *argc) {
            *argc = rdin->index;
        }

        //printf("redirect to fd %d\n", rdin->fd);

        /* 进行重定向 */
        redirect(STDIN_FILENO, rdin->fd);
    }
    return 0;
}

static int redirect_do_stdout(struct redirect_info *rdout, uint32_t redirect_mask, int *argc)
{
    uint32_t open_flags = 0;
    char *p;
    
    if ((rdout->flags & RD_FLAG_OK) && (redirect_mask & RD_FLAG_STDOUT)) {
        open_flags = O_CREAT | O_WRONLY;

        if (rdout->flags & RD_FLAG_APPEND) {
            /* 如果有输出重定向标志，才能进行重定向 */
            //printf("\nstdout redirect to %s with append.\n", rdout->filename);
            open_flags |= O_APPEDN;

        } else if (rdout->flags & RD_FLAG_TRANCE) {
            /* 如果有输出重定向标志，才能进行重定向 */
            //printf("\nstdout redirect to %s with trance.\n", rdout->filename);            
            open_flags |= O_TRUNC;
        }
        //printf("out index:%d, argc:%d\n", rdout->index, *argc);
        
        /* 修改参数数量 */
        if (rdout->index < *argc) {
            *argc = rdout->index;
        }

        /* 打开文件 */
        p = rdout->filename;
        /* 如果第一个是'&'，那么后面就可能跟着文件描述符 */
        if (*p == '&' && isdigitstr(p + 1)) {
            rdout->fd = atoi(p + 1);
        } else {
            rdout->fd = open(rdout->filename, open_flags);
        }
        if (rdout->fd < 0) {
            //printf("open file redirect failed!\n");    
            return -1;
        }
        //printf("redirect to fd %d\n", rdout->fd);

        /* 进行重定向 */
        redirect(STDOUT_FILENO, rdout->fd);
    }
    return 0;
}

static int redirect_do_stderr(struct redirect_info *rderr, uint32_t redirect_mask, int *argc)
{
    uint32_t open_flags = 0;
    char *p;
    
    if ((rderr->flags & RD_FLAG_OK) && (redirect_mask & RD_FLAG_STDERR)) {
        open_flags = O_CREAT | O_WRONLY;

        if (rderr->flags & RD_FLAG_APPEND) {
            //printf("\nstderr redirect to %s with append.\n", rderr->filename);
            open_flags |= O_APPEDN;

        } else if (rderr->flags & RD_FLAG_TRANCE) {
            //printf("\nstderr redirect to %s with trance.\n", rderr->filename);
            open_flags |= O_TRUNC;
        
        }
        //printf("out index:%d, argc:%d\n", rderr->index, *argc);
        
        /* 修改参数数量 */
        if (rderr->index < *argc) {
            *argc = rderr->index;
        }

        /* 打开文件 */
        p = rderr->filename;
        /* 如果第一个是'&'，那么后面就可能跟着文件描述符 */
        if (*p == '&' && isdigitstr(p + 1)) {
            rderr->fd = atoi(p + 1);
        } else {
            rderr->fd = open(rderr->filename, open_flags);
        }
        if (rderr->fd < 0) {
            printf("open file redirect failed!\n");    
            return -1;
        }
        //printf("redirect to fd %d\n", rderr->fd);

        /* 进行重定向 */
        redirect(STDERR_FILENO, rderr->fd);
    }
    return 0;
}

static int redirect_do(struct redirect_info *rdi, uint32_t redirect_mask, int *argc)
{
    int order = 0;
    int i, j;
    for (i = 0; i < MAX_ARG_NR; i++, order++) {
        for (j = 0; j < 3; j++) {
            /* 如果重定向所在的位置比顺序小，那么他就是前面的重定向 */
            if ((rdi[j].index <= order) && !(rdi[j].flags & RD_FLAG_DONE)) {
                if (j == 0) {
                    redirect_do_stdin(&rdi[j], redirect_mask, argc);
                    rdi[j].flags |= RD_FLAG_DONE;
                } else if (j == 1) {
                    redirect_do_stdout(&rdi[j], redirect_mask, argc);
                    rdi[j].flags |= RD_FLAG_DONE;
                } else {    /* j == 2 */
                    redirect_do_stderr(&rdi[j], redirect_mask, argc);
                    rdi[j].flags |= RD_FLAG_DONE;
                }
            }
        }    
    }
    return 0;
}

static void redirect_restore(struct redirect_info *rdi, uint32_t redirect_mask)
{
    struct redirect_info *rdin = &rdi[0];
    struct redirect_info *rdout = &rdi[1];
    struct redirect_info *rderr = &rdi[2];
    if ((rdin->flags & RD_FLAG_OK) && (redirect_mask & RD_FLAG_STDIN)) {
        /* 进行重定向，重定向到备份，就恢复了输出 */
        redirect(STDIN_FILENO, bosh_stdin_backup);

        /* 关闭重定向的文件，如果不是shell使用的文件，才关闭 */
        if (rdin->fd > bosh_stdin_backup) {
            close(rdin->fd);
        }
        //printf("restore to shell.\n");
    }
    if ((rdout->flags & RD_FLAG_OK) && (redirect_mask & RD_FLAG_STDOUT)) {
        /* 进行重定向，重定向到备份，就恢复了输出 */
        redirect(STDOUT_FILENO, bosh_stdout_backup);

        /* 关闭重定向的文件，如果不是shell使用的文件，才关闭 */
        if (rdout->fd > bosh_stderr_backup) {
            close(rdout->fd);
        }
        //printf("restore to shell.\n");
    }
    if ((rderr->flags & RD_FLAG_OK) && (redirect_mask & RD_FLAG_STDERR)) {
        /* 进行重定向，重定向到备份，就恢复了输出 */
        redirect(STDERR_FILENO, bosh_stderr_backup);

        /* 关闭重定向的文件，如果不是shell使用的文件，才关闭 */
        if (rderr->fd > bosh_stderr_backup) {
            close(rderr->fd);
        }
        //printf("restore to shell.\n");
    }
    
}

/**
 * 重定向：
 * a.一个选项必须时完整的。例如：>a, > b, >> c.
 * 错误示例：> , > >>.
 * b.同类选项只保留第一个。例如：>a, >>b, >> d.
 * 看似进行了3次重定向，不过，我们只会保留第一个正确的重定向。
 * 
 */
static void redirect_cmd(int argc, char **argv, struct redirect_info *rdi)
{
    /* 
    标志：截断，追加，读取，匹配。
    类型：输出，错误，输入
     */

    /* 标准输入，输出，错误重定向 */
    struct redirect_info *rdin = &rdi[0];
    struct redirect_info *rdout = &rdi[1];
    struct redirect_info *rderr = &rdi[2];
    
    redirect_init(rdin);
    redirect_init(rdout);
    redirect_init(rderr);

    /* 判断输入输出重定向 */
    int arg_idx = 0;
    char *p;
    while(arg_idx < argc){
        p = argv[arg_idx];
        /* 期待值0,1,2,>,<,& */
        switch (*p) {
        case '0':
            /* < , 向后看一个字符，如果不是的话，就不能往后面走，
            避免是名字的情况下字符串被破坏。例如：123，应该是名字，
            如果在这里提前p++，那么，后面读取到的名字其实是23.*/
            if ((*(p + 1) == '<') && !(rdin->flags & RD_FLAG_OK)) {
                p++;    /* 需要移动2个位置，上一层没有移动 */
                p++;
                /* >> */
                if (*p == '<') {
                    /* 匹配 */
                    p++;
                    rdin->flags |= RD_FLAG_MATCH;
                    //printf("MATCH ");
                    
                    /* 有字符串 */
                    if (*p) {
                        /* 后续字符串 */
                        rdin->filename = p;
                        rdin->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
                        //printf("FILE:%s ", p); 
                    } else {
                        /* 需要字符串 */
                        rdin->flags |= RD_FLAG_NEDD;
                        //printf("NEED "); 
                    }
                    rdin->index = arg_idx; /* 记录当前索引值 */
                } else {
                    /* 读取 */
                    rdin->flags |= RD_FLAG_READ;
                    //printf("READ ");
                    
                    /* 有字符串 */
                    if (*p) {
                        /* 后续字符串 */
                        rdin->filename = p;
                        rdin->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
                        //printf("FILE:%s ", p); 
                    } else {
                        /* 需要字符串 */
                        rdin->flags |= RD_FLAG_NEDD;
                        //printf("NEED "); 
                    }
                    rdin->index = arg_idx; /* 记录当前索引值 */
                }
                
            } else {
                /* 不是重定向 */
                //printf("NOT \n");
            }
            break;
        case '1':
            /* > , 向后看一个字符，如果不是的话，就不能往后面走，
            避免是名字的情况下字符串被破坏。例如：123，应该是名字，
            如果在这里提前p++，那么，后面读取到的名字其实是23.*/
            if ((*(p + 1) == '>') && !(rdout->flags & RD_FLAG_OK)) {
                p++;    /* 需要移动2个位置，上一层没有移动 */
                p++;
                /* >> */
                if (*p == '>') {
                    /* 追加 */
                    p++;
                    rdout->flags |= RD_FLAG_APPEND;
                    //printf("APPEND ");
                } else {
                    rdout->flags |= RD_FLAG_TRANCE;
                    //printf("TRANCE "); 
                }
                /* 有字符串 */
                if (*p) {
                    /* 后续文件名 */
                    rdout->filename = p;
                    rdout->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
                    //printf("FILE:%s ", p); 
                } else {
                    /* 需要名字 */
                    rdout->flags |= RD_FLAG_NEDD;
                    //printf("NEED "); 
                }
                rdout->index = arg_idx; /* 记录当前索引值 */
                    
            } else {
                /* 不是重定向 */
                //printf("NOT \n");
            }
            break;
        case '2':
            /* > , 向后看一个字符，如果不是的话，就不能往后面走，
            避免是名字的情况下字符串被破坏。例如：123，应该是名字，
            如果在这里提前p++，那么，后面读取到的名字其实是23.*/
            if ((*(p + 1) == '>') && !(rderr->flags & RD_FLAG_OK)) {
                p++;    /* 需要移动2个位置，上一层没有移动 */
                p++;
                /* >> */
                if (*p == '>') {
                    /* 追加 */
                    p++;
                    rderr->flags |= RD_FLAG_APPEND;
                    //printf("APPEND ");
                } else {
                    rderr->flags |= RD_FLAG_TRANCE;
                    //printf("TRANCE "); 
                }
                /* 有字符串 */
                if (*p) {
                    /* 后续文件名 */
                    rderr->filename = p;
                    rderr->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
                    //printf("FILE:%s ", p); 
                } else {
                    /* 需要名字 */
                    rderr->flags |= RD_FLAG_NEDD;
                    //printf("NEED "); 
                }
                rderr->index = arg_idx; /* 记录当前索引值 */
            } else {
                /* 不是重定向 */
                //printf("NOT \n");
            }
            break;
        case '>':
            /* >> */
            if (!(rdout->flags & RD_FLAG_OK)) {
                if (*(p + 1) == '>') {
                    /* 追加 */
                    p++;
                    p++;
                        
                    rdout->flags |= RD_FLAG_APPEND;
                    //printf("APPEND ");
                    /* 有字符串 */
                    if (*p) {
                        /* 后续字符串 */
                        rdout->filename = p;
                        rdout->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */

                        //printf("FILE:%s ", p); 
                    } else {
                        /* 需要字符串 */
                        rdout->flags |= RD_FLAG_NEDD;
                        //printf("NEED "); 
                    }
                    rdout->index = arg_idx; /* 记录当前索引值 */
                } else {
                    rdout->flags |= RD_FLAG_TRANCE;
                    //printf("TRANCE "); 

                    /* 有字符串 */
                    if (*(p + 1)) {
                        p++;

                        /* 后续字符串 */
                        rdout->filename = p;
                        rdout->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
                        
                        //printf("FILE:%s ", p); 
                    } else {
                        /* 需要字符串 */
                        rdout->flags |= RD_FLAG_NEDD;
                        //printf("NEED "); 
                    }
                    rdout->index = arg_idx; /* 记录当前索引值 */
                }
            }        
            break;
        case '<':
            if (!(rdin->flags & RD_FLAG_OK)) {
                /* << */
                if (*(p + 1) == '<') {
                    /* 匹配 */
                    p++;
                    p++;
                        
                    rdin->flags |= RD_FLAG_MATCH;
                    //printf("MATCH ");
                    
                    /* 有字符串 */
                    if (*p) {
                        /* 后续字符串 */
                        rdin->filename = p;
                        rdin->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */

                        //printf("FILE:%s ", p); 
                    } else {
                        /* 需要字符串 */
                        rdin->flags |= RD_FLAG_NEDD;
                        //printf("NEED "); 
                    }
                    rdin->index = arg_idx; /* 记录当前索引值 */
                } else {    /* < */
                    /* 读取 */
                    rdin->flags |= RD_FLAG_READ;
                    //printf("READ ");
                    
                    /* 有字符串 */
                    if (*(p + 1)) {
                        p++;

                        /* 后续字符串 */
                        rdin->filename = p;
                        rdin->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
                        
                        //printf("FILE:%s ", p); 
                    } else {
                        /* 需要字符串 */
                        rdin->flags |= RD_FLAG_NEDD;
                        //printf("NEED "); 
                    }
                    rdin->index = arg_idx; /* 记录当前索引值 */
                }
            }
            break;
        case '&':   /* 标准输出和错误都要重定向 */
            /* &> , &>> */
            if ((*(p + 1) == '>') && (!(rdout->flags & RD_FLAG_OK)) &&
                (!(rdin->flags & RD_FLAG_OK))) {
                p++;    /* 需要移动2个位置，上一层没有移动 */
                p++;
                /* >> */
                if (*p == '>') {
                    /* 追加 */
                    p++;
                    rdout->flags |= RD_FLAG_APPEND;
                    rderr->flags |= RD_FLAG_APPEND;
                
                    //printf("APPEND ");
                } else {
                    rdout->flags |= RD_FLAG_TRANCE;
                    rderr->flags |= RD_FLAG_TRANCE;
                    //printf("TRANCE "); 
                }
                /* 有字符串 */
                if (*p) {
                    /* 后续文件名 */
                    rdout->filename = p;
                    rdout->flags |= RD_FLAG_OK;   /* 已经完整 */
                    
                    rderr->filename = p;
                    rderr->flags |= RD_FLAG_OK;   /* 已经完整 */
                    
                    //printf("FILE:%s ", p); 
                } else {
                    /* 需要名字 */
                    rdout->flags |= RD_FLAG_NEDD;
                    
                    rderr->flags |= RD_FLAG_NEDD;
                    
                    //printf("NEED "); 
                }
                rderr->index = arg_idx; /* 记录当前索引值 */
                rdout->index = arg_idx; /* 记录当前索引值 */

            } else {
                /* 不是重定向 */
                //printf("NOT \n");
            }
            break;
        default:
            break;
        }
        
        /* 检测文件名必须是后一个参数，并且，后一个参数不能是err,out */
        if ((arg_idx == rdin->index + 1) &&
            (arg_idx != rderr->index) &&
            (arg_idx != rdout->index)) {
            /* 需要时才会解析 */
            if ((rdin->flags & RD_FLAG_NEDD) && !(rdin->flags & RD_FLAG_OK)) {
                /* 正是需要的名字 */
                rdin->filename = p;
                //printf("FILE2:%s ", p);    
                rdin->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
            }
        }
        
        /* 检测文件名必须是后一个参数，并且，后一个参数不能是in,err */
        if ((arg_idx == rdout->index + 1) &&
            (arg_idx != rdin->index) &&
            (arg_idx != rderr->index)) {
            /* 需要时才会解析 */
            if ((rdout->flags & RD_FLAG_NEDD) && !(rdout->flags & RD_FLAG_OK)) {
                /* 正是需要的名字 */
                rdout->filename = p;
                //printf("FILE2:%s ", p);    
                rdout->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */   
            }
        }
        
        /* 检测文件名必须是后一个参数，并且，后一个参数不能是in,out */
        if ((arg_idx == rderr->index + 1) &&
            (arg_idx != rdin->index) &&
            (arg_idx != rdout->index)) {
            /* 需要时才会解析 */
            if ((rderr->flags & RD_FLAG_NEDD) && !(rderr->flags & RD_FLAG_OK)) {
                /* 正是需要的名字 */
                rderr->filename = p;
                //printf("FILE2:%s ", p);    
                rderr->flags |= RD_FLAG_OK;   /* 标准输出已经完整 */
            }
        }
        
        arg_idx++;
    }

}

/**
 * execute_cmd - 执行命令
 * @argc: 参数数量
 * @argv: 参数地址
 * @redirect_mask: 重定向标志
 *      当有对应的重定向标志之后，才能够解析命令中的重定向信息
 *      RD_FLAG_STDIN: 可以解析标准输入重定向
 *      RD_FLAG_STDOUT: 可以解析标准输出重定向
 *      RD_FLAG_STDERR: 可以解析标准输错误重定向
 *      
 * 命令：命令+选项+参数+重定向
 */
static int execute_cmd(int argc, char **argv, uint32_t redirect_mask)
{
    int status = 0;
    int daemon = 0;
    int arg_idx = 0;

    struct redirect_info rdi[3];
    redirect_cmd(argc, argv, rdi);

    /* 处理重定向 */
    redirect_do(rdi, redirect_mask, &argc);

    /* 重定向后，参数才是完整的。判断是否是后台进程 */
    while(arg_idx < argc){
        //printf("%s ",buf);
        /* 如果在末尾，并且是单独的'&'符号，才当做后台应用 */
        if (!strcmp(argv[arg_idx], "&") && (arg_idx == argc - 1)) {
            daemon = 1;     /* 是后台进程 */
            argc--; /* 参数-1 */
            break;
        }
        arg_idx++;
    }

    /* 先执行内建命令，再选择磁盘中的命令 */
    if (buildin_cmd(argc, argv)) {
        
        /* 在末尾添加上结束参数 */
        argv[argc] = NULL;
        int pid;
        /* 检测文件是否存在，以及是否可执行，不然就返回命令错误 */
        if (access(argv[0], F_OK)) {
            //printf("unknown cmd %s !\n", argv[0]);
            goto err;
        } else {
            /* 创建一个进程 */
            pid = fork();

            if (pid > 0) {  /* 父进程 */
                if (!daemon) {
                    // 把子进程设置为前台
                    ioctl(bosh_stdin, GTTY_IOCTL_HOLD, pid);
                    /* shell程序等待子进程退出 */
                    pid = wait(&status);

                    // 恢复自己为前台
                    ioctl(bosh_stdin, GTTY_IOCTL_HOLD, getpid());
                    /*
                    printf("parent %d wait %d exit %d\n",
                        getpid(), pid, status);
                    */
                    /* 执行失败 */
                    if (status == pid) {
                        redirect_restore(rdi, redirect_mask);
                        goto err;
                    }
                }
            } else {    /* 子进程 */
                /* 子进程执行程序 */
                pid = execv((const char *)argv[0], (const char **)argv);

                /* 如果执行出错就退出 */
                if(pid == -1){
                    printf("execv file %s failed!\n", argv[0]);
                    exit(pid);  /* 退出 */
                }
            }

        }
    }
    /* 需要在执行完后恢复重定向到shell */
    redirect_restore(rdi, redirect_mask);
    return 0;
err:
    redirect_restore(rdi, redirect_mask);
    return -1;
}

/**
 * cmd_parse - 从输入的命令行解析参数
 * @cmd_str: 命令行缓冲
 * @argv: 参数数组
 * @token: 分割符
 */
int cmd_parse(char * cmd_str, char **argv, char token)
{
	if(cmd_str == NULL){
		return -1;
	}
	int arg_idx = 0;
	while(arg_idx < MAX_ARG_NR){
		argv[arg_idx] = NULL;
		arg_idx++;
	}
	char *next = cmd_str;
	int argc = 0;
	while(*next){
		//跳过token字符
		while(*next == token){
			next++;
		}
		//如果最后一个参数后有空格 例如"cd / "
		if(*next ==0){
			break;
		}
		//存入一个字符串地址，保存一个参数项
		argv[argc] = next;
		
		//每一个参数确定后，next跳到这个参数的最后面
		while(*next && *next != token){
			next++;
		}
		//如果此时还没有解析完，就把这个空格变成'\0'，当做字符串结尾
		if(*next){
			*next++ = 0;
		}
		//参数越界，解析失败
		if(argc > MAX_ARG_NR){
			return -1;
		}
		//指向下一个参数
		argc++;
		//让下一个字符串指向0
		argv[argc] = 0;
	}
	return argc;
}

/**
 * readline - 读取一行输入
 * @buf: 缓冲区
 * @count: 数据量
 * 
 * 输入回车结束输入
 */
void readline( char *buf, uint32_t count)
{
	char *pos = buf;
	while(read_key(buf, pos, count) && (pos - buf) < count){
		switch(*pos){
			case '\n':
				//当到达底部了就不在继续了，目前还没有设定
				*pos = 0;
				printf("\n");
				return;
			case '\b':
				if(buf[0] != '\b'){
					--pos;
					printf("\b");
				}
				break;
			default:
				printf("%c", *pos);
				pos++;
		}
	}
	printf("\nreadline: error!\n");
    //exit(-1);
    //abort();
    while (1);
}

/**
 * read_key - 从键盘读取一个按键
 * @start: 
 * @buf: 缓冲区
 * @count: 字符数
 * 
 * 
 */
int read_key(char *start, char *buf, int count)
{
	//从键盘获取按键
	unsigned int key;
	int status;
	//循环获取按键

	do {
		status = read(0, &key, 4);
		
		if (status != -1) {
			//获取按键
			//读取控制键状态
			
			if (0 < key) {
				//是一般字符就传输出去
				*buf = (char)key;
				break;
			}
			
			key = 0;
		}
	}while (status == -1);
	
	return 1;
}

/*
int read_key(char *buf)
{
	//从键盘获取按键
	char key;
	do{
		key = getchar();
	}while(key == -1);
	*buf = key;
	return 1;
}*/

/**
 * print_prompt - 打印提示符
 *  
 */
void print_prompt()
{
	printf("%s>", cwd_cache);
}

/**
 * wait - 等待一个子进程退出
 * @status: 子进程的退出状态码
 * 
 * 返回子进程pid
 */
int wait(int *status)
{
	int pid;
	do{
		pid = _wait(status);
        /* 按键监控 */
        
	}while(pid == -1);
	return pid;
}
