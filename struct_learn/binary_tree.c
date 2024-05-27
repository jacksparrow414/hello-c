/* 
由于没有原生bool类型，所以引入stdbool头文件

C程序设计语言 > 第2章 > 2.2 数据类型及长度
C语言只提供了下列几种基本类型
1. char
2. int
3. float
4. double

此外，还可以在这些基本类型的前面加一些限定符。short与long两个限定符用于限制整型：
short int sh;
long int counter;
在上述这种类型的声明中，关键字int可以省略。通常很多人也习惯这么做。

short与long两个限定符的引入可以为我们提供满足实际需要的不同长度的整型数。
int通常代表特定机器中整数的自然长度。short类型通常为16位，long类型通常为32位，int类型可以为16位或32位。
 */
#include <stdbool.h>
#include <stdio.h>
#include "binary_tree.h"

Binarytree build_binary_tree(int num) {
 Binarytree binary_tree;
 binary_tree.data =  num;
 binary_tree.left_sub_tree = NULL;
 binary_tree.right_sub_tree = NULL;
 return binary_tree;
}

bool check_node_exist(Binarytree *binary_tree) {
    return true;
}

void print_binary_tree(Binarytree *binary_tree, int mode) {
    switch (mode)
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    default:
        printf("invalid mode %d", mode);
        break;
    }
}