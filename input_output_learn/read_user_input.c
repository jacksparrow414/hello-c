#include <stdio.h>
#include <stdlib.h>
#include "read_user_input.h"

// https://stackoverflow.com/questions/16870485/how-can-i-read-an-input-string-of-unknown-length
/* 在函数定义中，形参 char s[] 和 char *s等价
上述文字来自于 C程序设计语言 > 指针与数组 5.3章节 */
void read_a_string_with_unknowd_length() {
    // 读取并丢弃输入缓冲区中的换行符
    getchar();
    printf("please enter string:\n");
    // 用一个char数组接收用户输入
    char *str;
    int init_alloc_memory_size = 2<<4;
    // 申请分配内存, 初始内存大小
    str = malloc(sizeof(*str)*init_alloc_memory_size);
    // 判断分配内存是否成功
    if (str == NULL)
    {
        printf("unable to alloc memory\n");
        return;
    }
    int ch;
    int str_len = 0;
    // TODO 读取用户输入
    while ((ch=fgetc(stdin)) != EOF && ch != '\n')
    {
        str[str_len++] = ch;
        // 如果char数组空间不够了, 则需要再申请分配内存
        if (str_len == init_alloc_memory_size)
        {
            // 扩容为原来的1倍
            str = realloc(str, sizeof(*str)*(init_alloc_memory_size*=2));
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
    // str[str_len++]='\0';
    printf("The string you entered is: %s\n", str);
    // fputs(str, stdout);
    // 释放分配给str的内存空间
    free(str);
}