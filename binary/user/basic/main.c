#include "tiny_basic_interpreter.h"
#include <stdio.h>

#include <string.h>

/*
static  char program[] =
"10 GOSUB 100\n\
20 for i = 1 to 10\n\
30 print i\n\
40 next i\n\
50 print \"goto end ok\"\n\
60 end\n\
100 print \"gosub ok\"\n\
110 return\n";

int main(int argc, char *argv[])
{
    interpreter_init(program);
    do {
        do_interpretation();
    } while(!interpreter_finished());
    return 0; 
}
*/
#define MAXLENGTH 1000
int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        char dest[MAXLENGTH];
        memset(dest, 0, MAXLENGTH);
        FILE *file;
        int pos, temp, i;
        file = fopen(argv[1], "r");

        if (NULL == file)
        {
            fprintf(stderr, "open %s error\n", argv[1]);
            return -1;
        }

        pos = 0;
        
        for (i = 0; i < MAXLENGTH - 1; i++)
        {
            temp = fgetc(file);
            if (EOF == temp)
                break;
            if (temp == '\r')
                temp = ' ';
            dest[pos++] = temp;
        }
        //关闭文件
        fclose(file);
        //在数组末尾加0
        dest[pos] = 0;

        interpreter_init(dest);

        do
        {
            do_interpretation();
        } while (!interpreter_finished());
        
    }
}
