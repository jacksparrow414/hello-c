#include <stdio.h>
#include <stdlib.h>
#include "read_user_input.h"

// https://stackoverflow.com/questions/16870485/how-can-i-read-an-input-string-of-unknown-length
/* 在函数定义中，形参 char s[] 和 char *s等价
上述文字来自于 C程序设计语言 > 指针与数组 5.3章节 */
void read_a_string_with_unknowd_length() {
    printf("please enter string:\n");
    // 用一个char数组接收用户输入
    char *str;
    int init_alloc_memory_size = 2<<4;
    // 为指针申请分配内存, 初始内存大小
    str = malloc(sizeof(*str)*init_alloc_memory_size);
    // 判断分配内存是否成功
    if (str == NULL)
    {
        printf("unable to alloc memory\n");
        return;
    }
    int ch;
    int str_len = 0;
    /* 读取用户输入
    为什么要使用int接收char? 见C程序设计语言 > 导言 1.5章节, 
    char类型对应的字符等于此字符在机器编码表上对应的数字,并且unassigned char的范围是0-255,太小,所以使用int接收
    '\n'的ASCII是10, 1.5.3 行计数 > 最后两段话

    在程序开始执行时, 默认的三个流stdin,stdout,stderr已经处于打开状态, 见见C程序设计语言 > 附录B B.1输入于输出 章节 */
    while ((ch=fgetc(stdin)) != EOF && ch != '\n')
    {
        str[str_len++] = ch;
        // 如果char数组空间不够了, 则需要再申请分配内存
        if (str_len == init_alloc_memory_size)
        {
            /* 将指针str指向的对象的长度修改为size长度, 这里是扩容为原来的1倍

            这里写sizeof(char)和sizeof(*str)效果一样, 原因见https://stackoverflow.com/questions/58758617/why-does-sizeofpointer-gives-output-as-1 
            因为str是指针, str默认指向的是char[0], 见C程序设计语言 > 指针与数组 5.3章节 
            那么此时*str就是访问的是char[0]的内容,因为char类型占1个字节,所以这里始终返回1
            既然sizeof(char)始终为1为什么不直接省略或写成1?可能的回答https://stackoverflow.com/questions/7243872/why-write-sizeofchar-if-char-is-1-by-standard

            拓展阅读: 关于对指针sizeof和对类型sizeof的回答https://stackoverflow.com/questions/40679801/difference-between-sizeofchar-and-sizeofchar*/
            str = realloc(str, sizeof(char)*(init_alloc_memory_size*=2));
            // 再分配之后依旧需要判断分配内存是否成功
            if (str == NULL)
            {
                printf("unable to realloc memory\n");
                printf("accept string is: \n");
                // 直接退出函数并在退出前输出目前所拿到的字符串
                break;
            }
        }
    }
    // str[str_len++]='\n';
    // 给字符串末尾加'\0'表示字符串结束, 这是C语言的约定, 见C程序设计语言 > 导言 1.9章节
    str[str_len++]='\0';

    // 使用下列两种方式输出字符数组
    // printf("The string you entered is: %s", str);

    /* 使用这种方式需要flush,将缓冲区的内容输出
    https://stackoverflow.com/questions/39536212/what-are-the-rules-of-automatic-stdout-buffer-flushing-in-c
    因为terminal默认是line buffered, 我们这里的字符串又没有换行,某些情况(谁知道terminal的buffer什么时候满呢?)line buffer可能没填满导致不输出内容,所以需要手动flush. 
    多运行几次就发现有的时候没fflush也可以,有的时候就不行 */
    fputs("The string you entered is:", stdout);
    fputs(str, stdout);
    // 或者把str[str_len++]='\n'注释放开并注释掉fflush
    fflush(stdout);
    // 释放分配给str的内存空间
    free(str);
}