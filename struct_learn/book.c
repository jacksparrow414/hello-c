#include <stdio.h>
#include "book.h"

void print_book(Book *book) {
    // 结构指针的另一种写法: C程序设计语言 > 第6章 结构 > 6.2 结构与函数
    printf("book name is: %s, size is: %d\n", book->name, sizeof(book->name));
    // .的优先级比*高
    printf("book id is: %d, size is: %d\n", (*book).book_id, sizeof((*book).book_id));
}

void creat_book_and_print() {
    // create a structure variable
    Book book;
    Book *bp;
    printf("please enter book name\n");
    // 通过指针进行赋值
    
    // 指针
    bp=&book;
    scanf("%s", bp->name);
    printf("please enter book id\n");
    // 指针
    scanf("%d", &book.book_id);
    printf("display book info\n");
    print_book(bp);
    print_book(&book);
    // 本应该是14个字节，内存对齐最终是16字节
    // 如何知道当前机器对齐字节是多少?
    // https://stackoverflow.com/questions/51622751/alignment-in-64bit-machine-is-not-8-bytes
    printf("book object size is: %d\n", sizeof(book));
}