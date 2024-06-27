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
#include <stdlib.h>
#include "binary_tree.h"

/* 
结构体有帮助的文档：https://www.yuque.com/shatang-sgdju/th9nic/mt4gga
TODO:
 1. struct初始化时如何正确内存分配malloc
    一个例子: https://github.com/antirez/smallchat/blob/main/smallchat-server.c
 2. 如何知道何时增加该struct的存储空间？ 
 */
Binarytree *create_new_binary_tree() {
    Binarytree *binary_tree = malloc(sizeof(*binary_tree));
    if (binary_tree == NULL)
    {
        printf("unable to alloc memory\n");
        exit(1);
    }
    binary_tree ->data = -1;
    binary_tree->left_sub_tree = binary_tree->right_sub_tree = NULL;
    return binary_tree;
    
}
/* 
根据用户输入构造二叉树
只做简单的插入，不重新优化二叉树
简单的递归
 */
Binarytree *build_simple_binary_tree(Binarytree *binary_tree, int data) {
    if (binary_tree->data == -1)
    {
        binary_tree->data=data;
        return binary_tree;
    }
    if (binary_tree->data !=-1 && data < binary_tree->data)
    {
        if (binary_tree->left_sub_tree == NULL)
        {
            binary_tree->left_sub_tree = create_new_binary_tree();
        }
        return build_simple_binary_tree(binary_tree->left_sub_tree, data);
    }
    if (binary_tree->data !=-1 && data > binary_tree->data)
    {
        if (binary_tree->right_sub_tree == NULL)
        {
            binary_tree->right_sub_tree = create_new_binary_tree();
        }
        return build_simple_binary_tree(binary_tree->right_sub_tree, data);
    }
}

bool check_node_exist(Binarytree *binary_tree) {
    return true;
}

void free_binary_tree(Binarytree *binary_tree) {
    while (binary_tree->left_sub_tree != NULL)
    {
        free_binary_tree(binary_tree->left_sub_tree);
    }

    while (binary_tree->right_sub_tree != NULL)
    {
        free_binary_tree(binary_tree->right_sub_tree);
    }
    
    free(binary_tree);
    binary_tree = NULL;
}

/* 
打印二叉树
0-前序遍历
1-中序遍历
2-后序遍历
3-层序遍历 
*/
void print_binary_tree(Binarytree *binary_tree, int mode) {
    switch (mode)
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    default:
        printf("invalid mode %d", mode);
        break;
    }
}