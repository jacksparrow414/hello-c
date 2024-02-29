#include <stdio.h>
#include "file_example.h"

void write_user_input_to_file(int buffer_size)
{
    FILE *fp;
    fp = fopen("test.txt", "a+");
    if (fp == NULL)
    {
        printf("unable to open file\n");
        return;
    }
    int character_count = 0;
    char buffer[buffer_size];
    int c = 0;
    // 这种写法必须在终端输入的字节数必须是buffer_size的整数倍,包括换行,如果不是整数倍,则会一直阻塞直到达到整数倍
    // https://sourceware.org/glibc/manual/latest/html_node/Block-Input_002fOutput.html
    while ((c = fread(&buffer, sizeof(char), buffer_size, fp)) > 0)
    {
        if (feof(fp))
        {
            printf("test");
            break;
        }
        int foundNewLine = 0;
        for (int i = 0; i < c; i++)
        {
            if (buffer[i] == '\n')
            {
                foundNewLine = 1;
                break;
            }
        }
        fwrite(&buffer, sizeof(char), c, fp);
        character_count += c;
        if (foundNewLine)
        {
            break;
        }
        // 清空数组，以便下次读取
        for (int i = 0; i < buffer_size; i++)
        {
            // 将数组中的每个元素赋值为 0
            buffer[i] = 0;
        }
    }
    fflush(fp);
    fclose(fp);
    printf("character count is: %d", character_count);
}

void read_binary_file_write_it_back()
{
    FILE *src_fp;
    /*
    如果是二进制文件要加个b
    https://sourceware.org/glibc/manual/latest/html_node/Opening-Streams.html
    https://sourceware.org/glibc/manual/latest/html_node/Binary-Streams.html
    见C程序设计语言 > 第7章 输入与输出 > 7.5 文件访问
    某些系统还区分文本文件和二进制文件，对后者的访问需要在模式字符串中增加字符"b"

    默认路径为工作目录
    Each process has associated with it a directory,
    called its current working directory or simply working directory,
    that is used in the resolution of relative file names
    https://sourceware.org/glibc/manual/latest/html_node/Working-Directory.html
    */
    src_fp = fopen("file_socket_learn/files/package_explain.jpg", "rb");
    if (src_fp == NULL)
    {
        /* 
        https://en.cppreference.com/w/c/io
        Writes every character from the null-terminated string str 
        and one additional newline character '\n' to the output stream stdout 
        */
        puts("unable to open file");
        return;
    }
    FILE *dest_fp;
    dest_fp = fopen("file_socket_learn/files/package_explain_copy.jpg", "w+b");
    if (dest_fp == NULL)
    {
        puts("unable to create file");
        return;
    }
    int buffer_size = 1024;
    char buffer[buffer_size];
    int c = 0;
    /*
    读取流时，流会自动向前移动
    https://sourceware.org/glibc/manual/latest/html_node/File-Position.html
    */
    while ((c = fread(&buffer, sizeof(char), buffer_size, src_fp)) != EOF)
    {
        if (c != buffer_size)
        {
            if (feof(src_fp))
            {
                puts("End of file is reached successfully");
                fwrite(&buffer, sizeof(char), c, dest_fp);
                break;
            }
        }
        else
        {
            puts("buffer size is reached");
            fwrite(&buffer, sizeof(char), buffer_size, dest_fp);
        }
    }
    fclose(src_fp);
    fclose(dest_fp);
}