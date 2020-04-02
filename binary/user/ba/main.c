#include <conio.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define FRAME_WIDTH 53
#define FRAME_HEIGHT 20

/* 每行需要加上2个字符,\n\r */
#define LINE_WIDTH (FRAME_WIDTH + 2)

/* 缓冲区长度 */
#define BUF_WIDTH (LINE_WIDTH + 1)

/* 每次读取10帧 */
#define MAX_FRAMES_NR 3285

const char *file_path = "root:/frame.txt";

int frame = 0;

void play_delay(int time);

int main(int argc, char *argv[])
{
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("open file failed!\b");
        return -1;
    }

    int fsize = lseek(fd, 0, SEEK_END);
    printf("file size :%d\n", fsize);
    char *fbuf = malloc(fsize);

    if (fbuf == NULL) {
        close(fd);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    /* 加载文件到内存 */
    char *pos = fbuf;
    int i;
    for (i = 0; i < fsize / 100 * 1024 + 1; i++) {
        read(fd, pos, 100 * 1024);
        pos += 100 * 1024;
    }
    pos = fbuf;
    printf("ready print\n");
    
    char tmp[BUF_WIDTH];
    memset(tmp, 0, BUF_WIDTH);

    while (1) {
        /* 显示前清屏 */
        ioctl(0, 1, 0);
        
        /* 读取完一帧，指向下一帧 */
        int y;
        for (y = 0; y < FRAME_HEIGHT; y++) {
            /* 最多一次读取单行长度 */
            for (i = 0; i < LINE_WIDTH; i++) {
                tmp[i] = *pos;  /* 从缓冲区中读取 */
                if (*pos == '\n') { /* 遇到回车，提前退出 */
                    tmp[i + 1] = '\0';
                    pos++;
                    break;
                }
                pos++;
            }
            /* 每次读取一行 */
            /* 输出单行内容 */
            printf(tmp);  
                
        }
        frame++;
        if (frame >= MAX_FRAMES_NR)
            goto end;
        
        /* 延时 */
        play_delay(4);
    }
    
// 播放结束
end: 
    printf("play over!\n");
    free(fbuf);
    close(fd);
    return 0; 
}

void play_delay(int time)
{
    int i, j;
    for (i = 0; i < 95 * time; i++) {
        for (j = 0; j < 90000; j++) {
            
        }
    }

}
