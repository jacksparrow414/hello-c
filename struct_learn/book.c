#include <stdio.h>
#include "book.h"

void print_book(Book book) {
    printf("book name is: %s, size is: %d\n", book.name, sizeof(book.name));
    printf("book id is: %d, size is: %d\n", book.book_id, sizeof(book.book_id));
}