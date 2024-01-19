// https://stackoverflow.com/questions/3482948/what-is-the-fundamental-difference-between-source-and-header-files-in-c
// .h 定义接口
// .c 负责实现
// 一个.h文件可能被多个.c文件引用
// 这就有一个问题，如何.h是接口，.c是实现，一个接口可以有多个实现. 那真正调用的时候如何实现多态?
// 可参考的信息: C 本身没有多态这一说法
// 可参考的链接： https://stackoverflow.com/questions/26026302/trying-to-have-multiple-source-files-linked-with-one-header-in-c
//               https://blog.csdn.net/fhb1922702569/article/details/114276707
// TODO 关于这方面需要再调查
typedef struct 
{
    char name[10];
    int book_id;
} Book;

void print_book(Book);