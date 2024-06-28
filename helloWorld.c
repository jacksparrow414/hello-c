// 引用系统.h使用<>
#include <stdio.h>
// 引用自定义的.h使用""
#include "struct_learn/book.h"
#include "struct_learn/binary_tree.h"
#include "input_output_learn/read_user_input.h"
#include "file_socket_learn/file_example.h"
#include "file_socket_learn/socket_client_example.h"

// 将后续使用的函数声明放在前面，否则编译器会报异常
// https://stackoverflow.com/questions/16182115/note-previous-implicit-declaration-of-point-forward-was-here


int main()
{
    int mode;
    printf("please enter mode, mode should be a number, all alvaliable modes are dispalyed as following:\n");
    printf("0: exit\n");
    printf("1: strut learn demo\n");
    printf("2: read user input demo\n");
    printf("3: file and socket demo\n");
    printf("4: read a binary file then copy\n");
    printf("5: create a simple binary tree\n");
    scanf("%d", &mode);
    /* 当scanf读取数字时, 需要跟一个getchar(), 读取并丢弃输入缓冲区中的换行符
    这是因为在使用 scanf 读取整数时，输入缓冲区中的换行符可能会被留下，而后续使用 fgetc 读取字符时会受到影响。
    因此，为了确保从输入缓冲区中获取正确的字符，我们需要先读取并丢弃换行符
    https://stackoverflow.com/questions/69689147/how-does-scanf-and-getchar-work-together */
    getchar();
    switch (mode) {
        case 0:
            printf("exit");
            break;
        case 1:
            creat_book_and_print();
            break;
        case 2:
            read_a_string_with_unknowd_length();
            break;
        case 3:
            write_user_input_to_file(10);
            break;
        case 4:
            read_binary_file_write_it_back();
            break;
        case 5:
            create_binary_tree_from_user_input();
            break;
        default:
            printf("invalid mode %d", mode);
            break;
    }
    return 0;
}