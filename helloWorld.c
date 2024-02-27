// 引用系统.h使用<>
#include <stdio.h>
// 引用自定义的.h使用""
#include "struct_learn/book.h"
#include "input_output_learn/read_user_input.h"

// 将后续使用的函数声明放在前面，否则编译器会报异常
// https://stackoverflow.com/questions/16182115/note-previous-implicit-declaration-of-point-forward-was-here


void struct_learn_example() {
    // create a structure variable
    Book book;
    printf("please enter book name\n");
    // 通过指针进行赋值
    
    // 指针
    scanf("%s", &book.name);
    printf("please enter book id\n");
    // 指针
    scanf("%d", &book.book_id);
    printf("display book info\n");
    print_book(book);
    // 本应该是14个字节，内存对齐最终是16字节
    printf("book object size is: %d\n", sizeof(book));
}

void read_user_input_example() {
    read_a_string_with_unknowd_length();
}


int main()
{
    int mode;
    printf("please enter mode, mode should be a number, all alvaliable modes are dispalyed as following:\n");
    printf("0: exit\n");
    printf("1: strut learn demo\n");
    printf("2: read user input demo\n");
    scanf("%d", &mode);
    switch (mode) {
        case 0:
            printf("exit");
            break;
        case 1:
            struct_learn_example();
            break;
        case 2:
            read_user_input_example();
            break;
        default:
            printf("invalid mode %d", mode);
            break;
    }
    return 0;
}