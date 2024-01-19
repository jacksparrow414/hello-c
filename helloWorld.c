// 引用系统.h使用<>
#include <stdio.h>
// 引用自定义的.h使用""
#include "struct_learn/book.h"

int main()
{
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
    printf("book object size is: %d", sizeof(book));
    return 0;
}