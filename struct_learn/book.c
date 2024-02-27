#include <stdio.h>
#include "book.h"

void print_book(Book book) {
    printf("book name is: %s, size is: %d\n", book.name, sizeof(book.name));
    printf("book id is: %d, size is: %d\n", book.book_id, sizeof(book.book_id));
}

void creat_book_and_print() {
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
    // 如何知道当前机器对齐字节是多少?
    // https://stackoverflow.com/questions/51622751/alignment-in-64bit-machine-is-not-8-bytes
    printf("book object size is: %d\n", sizeof(book));
}