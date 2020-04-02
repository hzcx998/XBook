/*
author : Adancurusul
chen.yuheng@nexuslink.cn
version：0.0.1
介绍：
这是一个可以嵌入任何只要支持printf函数的系统或单片机中的basic解释器；
代码量仅千行，但已经支持basic中let,print,if,then,else,for,to,next,goto,gosub,return,call,end
语句。
*******************************************************************************************
使用只用传入程序的字符串数组到interpreter_init
然后do_interpretation即可
interpreter_finished作为结束标志

*******************************************************************************************
目前变量名还只支持26个英文字母，也只能简单的运算。
本解释器目标是作为一种能进行科学运算的语言。这一个版本只是跑通基本语法
敬请期待后续

*/


#include <stdio.h>//printf
#include <string.h>//printf

#include "tiny_basic_interpreter.h"
#define MAX_GOSUB_STACK_DEPTH 20
#define MAX_STRINGLEN 50
#define MAX_FOR_STACK_DEPTH 6
#define MAX_VARNUM 26
#define MAX_NUMLEN 6

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define CHANGE_LOWER 'A' - 'a'

static char string[MAX_STRINGLEN];
static int gosub_stack[MAX_GOSUB_STACK_DEPTH];
static int gosub_stack_ptr;
static char const *program_ptr, *ptr, *nextptr;
typedef struct for_state
{
    int line_after_for;
    int for_variable;
    int to;
} FOR_STATE;

static FOR_STATE for_stack[MAX_FOR_STACK_DEPTH];
static int for_stack_ptr;
static char variables[MAX_VARNUM];
static int ended;

static int expr(void);
static void line_handler(void);
static void handler(void);
int search_finished(void);

int get_variable(int varnum);
void set_variable(int varnum, int value);

enum
{
    ERROR,
    ENDINPUT,
    NUMBER,
    STRING,
    VARIABLE,

    K_LET,
    K_PRINT,
    K_IF,
    K_THEN,
    K_ELSE,
    K_FOR,
    K_TO,
    K_NEXT,
    K_GOTO,
    K_GOSUB,
    K_RETURN,
    K_CALL,
    K_END,

    COMMA,
    SEMICOLON,
    PLUS,
    MINUS,
    AND,
    OR,
    ASTRISK,
    SLASH,
    PERCENT,
    LEFTBRACKET,
    RIGHTBRACKET,
    LIGHTER,
    GREATER,
    EQUAL,
    CR,
};

///////////////////////////////////////////////
//////////////////////////////////////////////
//下面是string的替代，如目标平台支持下面几个函数可去掉
#if 0
int atoi(const char *src);
void *memcpy(void *dest, const void *src, int count);
char * strchr(char * str, const char c);
unsigned int strlen(const char * str);
char *strncpy(char *dest, const char *str, int count);
int strncmp(const char * str1, const char * str2, int count);
char *strncpy(char *dest, const char *str, int count)
{
    //assert(dest != NULL&&str != NULL);
    char *ret = dest;
    while (count-- && (*dest++ = *str++))
    {
        ;
    }
    if (count > 0) //当上述判断条件不为真时并且count未到零，在dest后继续加/0；
    {
        while (count--)
        {
            *dest++ = '/0';
        }
    }
    return ret;
}
int strncmp(const char * str1, const char * str2, int count)
{
   
    if(!count)
        return 0;
    while(--count && *str1 && *str1 == *str2)
    {
        str1++;
        str2++;
    }
    return (*str1 - *str2);
}

void *memcpy(void *dest, const void *src, int count)
{
    if (dest == NULL || src == NULL)
    {
        return NULL;
    }
    char *pdest = (char *)dest;
    char *psrc = (char *)src;
    while (count--)
    {
        *pdest++ = *psrc++;
    }
    return dest;
}

char * strchr(char * str, const char c)
{
    
    while(*str != '\0' && *str != c)  
    {
        str++;
    }
    
    return (*str == c ? str : NULL);
}


unsigned int strlen(const char * str)
{
   
    unsigned length = 0;  
    while(*str != '\0')  
    {
        length++;  
        str++;  
    }
    return length;
}
////////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

int atoi(const char *src)
{
    //assert(NULL != src);
    int _num = 0;
    int _sign = 0;
    while ('0' == *src || ' ' == *src || '\n' == *src || '-' == *src || '+' == *src) //如果有空,空格或者换行跳过去
    {
        if (*src == '-')
            _sign = 1;

        src++;
    }

    while (*src >= '0' && *src <= '9')
    {
        _num = _num * 10 + *src - '0';
        src++;
    }

    if (_sign == 1)
        return -_num;
    else
        return _num;
}
#endif

typedef struct keyword_token
{
    char *keyword;
    int token;
} KEYS;

static int token_now = ERROR;

static const KEYS keywords[] = {
    {"let", K_LET},
    {"print", K_PRINT},
    {"if", K_IF},
    {"then", K_THEN},
    {"else", K_ELSE},
    {"for", K_FOR},
    {"to", K_TO},
    {"next", K_NEXT},
    {"goto", K_GOTO},
    {"gosub", K_GOSUB},
    {"return", K_RETURN},
    {"call", K_CALL},
    {"end", K_END},
    {NULL, ERROR}};

static int if_one_char(void)
{
    switch (*ptr)
    {
    case '\n':
        return CR;
        break;
    case ',':
        return COMMA;
        break;
    case ';':
        return SEMICOLON;
        break;
    case '+':
        return PLUS;
        break;
    case '-':
        return MINUS;
        break;
    case '&':
        return AND;
        break;
    case '|':
        return OR;
        break;
    case '/':
        return SLASH;
        break;
    case '*':
        return ASTRISK;
        break;
    case '(':
        return LEFTBRACKET;
        break;
    case ')':
        return RIGHTBRACKET;
        break;
    case '<':
        return LIGHTER;
        break;
    case '>':
        return GREATER;
        break;

    case '=':
        return EQUAL;
        break;
    default:
        return 0;
        break;
    }
}

static int get_next_token(void)
{
    KEYS const *kt;
    int i;

    if (*ptr == 0)
    {
        return ENDINPUT;
    }

    if (isdigit(*ptr))
    {
        for (i = 0; i < MAX_NUMLEN; ++i)
        {
            if (!isdigit(ptr[i]))
            {
                if (i > 0)
                {
                    nextptr = ptr + i;
                    return NUMBER;
                }
                else
                {
                    printf("get_next_token: error due to too short number\n");
                    return ERROR;
                }
            }
            if (!isdigit(ptr[i]))
            {
                printf("get_next_token: error due to malformed number\n");
                return ERROR;
            }
        }
        printf("get_next_token: error due to too long number\n");
        return ERROR;
    }
    else if (if_one_char())
    {
        nextptr = ptr + 1;
        return if_one_char();
    }
    else if (*ptr == '"')
    {
        nextptr = ptr;
        do
        {
            ++nextptr;
        } while (*nextptr != '"');
        ++nextptr;
        return STRING;
    }
    else
    {
        for (kt = keywords; kt->keyword != NULL; ++kt)
        {
            if (strncmp(ptr, kt->keyword, strlen(kt->keyword)) == 0)
            {
                nextptr = ptr + strlen(kt->keyword);
                return kt->token;
            }
        }
    }

    if (*ptr >= 'a' && *ptr <= 'z')
    {
        nextptr = ptr + 1;
        return VARIABLE;
    }

    return ERROR;
}

void search_init(const char *program)
{
    ptr = program;
    token_now = get_next_token();
}

int search_token(void)
{
    return token_now;
}

void search_next(void)
{

    if (search_finished())
    {
        return;
    }

    ptr = nextptr;
    while (*ptr == ' ')
    {
        ++ptr;
    }
    token_now = get_next_token();
    return;
}

int search_num(void)
{
    return atoi(ptr);
}

void sear_string(char *dest, int len)
{
    char *string_end;
    int string_len;

    if (search_token() != STRING)
    {
        return;
    }
    string_end = strchr(ptr + 1, '"');
    if (string_end == NULL)
    {
        return;
    }
    string_len = string_end - ptr - 1;
    if (len < string_len)
    {
        string_len = len;
    }
    memcpy(dest, ptr + 1, string_len);
    dest[string_len] = 0;
}

void search_error_print(void)
{
    printf("search_error_print: '%s'\n", ptr);
}

int search_finished(void)
{
    return *ptr == 0 || token_now == ENDINPUT;
}

int VARIABLE_num(void)
{
    return *ptr - 'a';
}
/*
char lower(char pro[]){
    
    int length = strlen(pro);
    printf("%d\n",length);
    
    for (int i=0;i<length;i++){
        if (pro[i]>='A'&&pro[i]<='Z'){
            pro[i] += CHANGE_LOWER;
        }
    }
    printf("%s",pro);
    char *p = pro;
    return pro;

}
*/
void interpreter_init(char pro[])
{
    int length = strlen(pro);
    //printf("%d\n",length);

    for (int i = 0; i < length; i++)
    {
        if (pro[i] >= 'A' && pro[i] <= 'Z')
        {
            pro[i] -= CHANGE_LOWER;
            //printf("%c",pro[i]);
            //while (1){

            //}
        }
    }
    //printf("%s",pro);
    char *program = pro;

    //char *program = pro;
    //printf("the%s",program);
    program_ptr = program;
    for_stack_ptr = gosub_stack_ptr = 0;
    search_init(program);
    ended = 0;
}

static void accept(int token)
{
    if (token != search_token())
    {

        search_error_print();
    }
    search_next();
}

static int varfactor(void)
{
    int r;
    r = get_variable(VARIABLE_num());
    accept(VARIABLE);
    return r;
}

static int factor(void)
{
    int r;

    switch (search_token())
    {
    case NUMBER:
        r = search_num();
        accept(NUMBER);
        break;
    case LEFTBRACKET:
        accept(LEFTBRACKET);
        r = expr();
        accept(RIGHTBRACKET);
        break;
    default:
        r = varfactor();
        break;
    }
    return r;
}

static int
term(void)
{
    int f1, f2;
    int op;

    f1 = factor();
    op = search_token();
    while (op == ASTRISK ||
           op == SLASH ||
           op == PERCENT)
    {
        search_next();
        f2 = factor();
        switch (op)
        {
        case ASTRISK:
            f1 = f1 * f2;
            break;
        case SLASH:
            f1 = f1 / f2;
            break;
        case PERCENT:
            f1 = f1 % f2;
            break;
        }
        op = search_token();
    }
    return f1;
}

static int
expr(void)
{
    int t1, t2;
    int op;

    t1 = term();
    op = search_token();
    while (op == PLUS ||
           op == MINUS ||
           op == AND ||
           op == OR)
    {
        search_next();
        t2 = term();
        switch (op)
        {
        case PLUS:
            t1 = t1 + t2;
            break;
        case MINUS:
            t1 = t1 - t2;
            break;
        case AND:
            t1 = t1 & t2;
            break;
        case OR:
            t1 = t1 | t2;
            break;
        }
        op = search_token();
    }
    return t1;
}

static int relation(void)
{
    int r1, r2;
    int op;

    r1 = expr();
    op = search_token();
    while (op == LIGHTER ||
           op == GREATER ||
           op == EQUAL)
    {
        search_next();
        r2 = expr();
        switch (op)
        {
        case LIGHTER:
            r1 = r1 < r2;
            break;
        case GREATER:
            r1 = r1 > r2;
            break;
        case EQUAL:
            r1 = r1 == r2;
            break;
        }
        op = search_token();
    }
    return r1;
}

static void
jump_linenum(int linenum)
{
    search_init(program_ptr);
    while (search_num() != linenum)
    {
        do
        {
            do
            {
                search_next();
            } while (search_token() != CR &&
                     search_token() != ENDINPUT);
            if (search_token() == CR)
            {
                search_next();
            }
        } while (search_token() != NUMBER);
    }
}

static void
goto_handler(void)
{
    accept(K_GOTO);
    jump_linenum(search_num());
}

static void
print_handler(void)
{
    accept(K_PRINT);
    do
    {
        if (search_token() == STRING)
        {
            sear_string(string, sizeof(string));
            printf("%s", string);
            search_next();
        }
        else if (search_token() == COMMA)
        {
            printf(" ");
            search_next();
        }
        else if (search_token() == SEMICOLON)
        {
            search_next();
        }
        else if (search_token() == VARIABLE ||
                 search_token() == NUMBER)
        {
            printf("%d", expr());
        }
        else
        {
            break;
        }
    } while (search_token() != CR &&
             search_token() != ENDINPUT);
    printf("\n");
    search_next();
}

static void
if_handler(void)
{
    int r;

    accept(K_IF);

    r = relation();
    accept(K_THEN);
    if (r)
    {
        handler();
    }
    else
    {
        do
        {
            search_next();
        } while (search_token() != K_ELSE &&
                 search_token() != CR &&
                 search_token() != ENDINPUT);
        if (search_token() == K_ELSE)
        {
            search_next();
            handler();
        }
        else if (search_token() == CR)
        {
            search_next();
        }
    }
}

static void
let_handler(void)
{
    int var;

    var = VARIABLE_num();

    accept(VARIABLE);
    accept(EQUAL);
    set_variable(var, expr());
    accept(CR);
}

static void
gosub_handler(void)
{
    int linenum;
    accept(K_GOSUB);
    linenum = search_num();
    accept(NUMBER);
    accept(CR);
    if (gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH)
    {
        gosub_stack[gosub_stack_ptr] = search_num();
        gosub_stack_ptr++;
        jump_linenum(linenum);
        //printf("jump");
    }
    else
    {
    }
}

static void
return_handler(void)
{
    accept(K_RETURN);
    if (gosub_stack_ptr > 0)
    {
        gosub_stack_ptr--;
        jump_linenum(gosub_stack[gosub_stack_ptr]);
    }
    else
    {
    }
}

static void
next_handler(void)
{
    int var;

    accept(K_NEXT);
    var = VARIABLE_num();
    accept(VARIABLE);
    if (for_stack_ptr > 0 &&
        var == for_stack[for_stack_ptr - 1].for_variable)
    {
        set_variable(var,
                     get_variable(var) + 1);
        if (get_variable(var) <= for_stack[for_stack_ptr - 1].to)
        {
            jump_linenum(for_stack[for_stack_ptr - 1].line_after_for);
        }
        else
        {
            for_stack_ptr--;
            accept(CR);
        }
    }
    else
    {
        accept(CR);
    }
}

static void
for_handler(void)
{
    int for_variable, to;

    accept(K_FOR);
    for_variable = VARIABLE_num();
    accept(VARIABLE);
    accept(EQUAL);
    set_variable(for_variable, expr());
    accept(K_TO);
    to = expr();
    accept(CR);

    if (for_stack_ptr < MAX_FOR_STACK_DEPTH)
    {
        for_stack[for_stack_ptr].line_after_for = search_num();
        for_stack[for_stack_ptr].for_variable = for_variable;
        for_stack[for_stack_ptr].to = to;

        for_stack_ptr++;
    }
    else
    {
    }
}

static void
end_handler(void)
{
    accept(K_END);
    ended = 1;
}

static void
handler(void)
{

    int token;

    token = search_token();

    switch (token)
    {
    case K_PRINT:
        //printf("print\n handler\n");
        print_handler();
        break;
    case K_IF:
        //printf("if\n handler\n");
        if_handler();
        break;
    case K_GOTO:
        //printf("goto\n handler\n");
        goto_handler();
        break;
    case K_GOSUB:
        //printf("gosub\n handler\n");
        //printf("gosub");
        gosub_handler();
        break;
    case K_RETURN:
        //printf("return\n handler\n");
        return_handler();
        break;
    case K_FOR:
        //printf("ooo\noo\nf\no\nr\nf\no\nr\nf\no\nr\n");
        for_handler();
        break;
    case K_NEXT:
        //printf("next\n handler\n");
        next_handler();
        break;
    case K_END:
        //printf("dne\n handler\n");
        end_handler();
        break;
    case K_LET:
        // printf("let\n handler\n");
        accept(K_LET);
        /* Fall through. */
    case VARIABLE:
        //printf("variable\n handler\n");
        let_handler();
        break;
    default:
    break;
        //printf("!!!!!!!!!!error!!!!!!!!!!!");
    }
}

static void line_handler(void)
{

    accept(NUMBER);
    handler();
    return;
}

void do_interpretation(void)
{
    if (search_finished())
    {

        return;
    }

    line_handler();
}

int interpreter_finished(void)
{
    return ended || search_finished();
}

void set_variable(int varnum, int value)
{
    if (varnum > 0 && varnum <= MAX_VARNUM)
    {
        variables[varnum] = value;
    }
}

int get_variable(int varnum)
{
    if (varnum > 0 && varnum <= MAX_VARNUM)
    {
        return variables[varnum];
    }
    return 0;
}

