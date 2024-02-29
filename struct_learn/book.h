// https://stackoverflow.com/questions/3482948/what-is-the-fundamental-difference-between-source-and-header-files-in-c
// .h 定义接口
// .c 负责实现
// 一个.h文件可能被多个.c文件引用
// 这就有一个问题，如何.h是接口，.c是实现，一个接口可以有多个实现. 那真正调用的时候如何实现多态?
// 可参考的信息: C 本身没有多态这一说法
// 可参考的链接： https://stackoverflow.com/questions/26026302/trying-to-have-multiple-source-files-linked-with-one-header-in-c
//               https://blog.csdn.net/fhb1922702569/article/details/114276707
// TODO 关于这方面需要再调查

/* 
使用typedef的好处是创建Book时可以直接写Book book, 如果没有typedef的话，则需要使用struct Book
https://www.cnblogs.com/zjuhaohaoxuexi/p/16252576.html 

还有的好处见: C程序设计语言 > 第6章 结构 > 6.7 类型定义(typedef)
*/
typedef struct 
{
    char name[10];
    int book_id;
} Book;
/* 
 C程序设计语言 > 第6章 结构 > 6.2 结构与函数
 1. 结构的合法操作只有几种：作为一个整体复制和赋值，通过&运算符取地址
 2. 3种可能的方法传递结构：一是分别传递各个结构成员，二是传递整个结构，三是传递指向结构的指针。三种方法各有利弊
 */
void print_book(Book *book);

void creat_book_and_print();