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
#define TREE_MAX_HEIGHT 5

/* 
结构体有帮助的文档：https://www.yuque.com/shatang-sgdju/th9nic/mt4gga
TODO:
 1. struct初始化时如何正确内存分配malloc
    一个例子: https://github.com/antirez/smallchat/blob/main/smallchat-server.c
 2. 如何知道何时增加该struct的存储空间？ 

 这个例子可以很好的说明malloc分配的内存在堆上，而不是栈上。
 即使函数结束返回了，结构体指针依旧可用
 */
Binarytree *create_new_binary_tree() {
    Binarytree *binary_tree = malloc(sizeof(*binary_tree));
    if (binary_tree == NULL)
    {
        printf("unable to alloc memory\n");
        exit(1);
    }
    /* 
     在不进行显示初始化的情况下，外部变量和静态变量都将被初始化为0，而自动变量和寄存器变量的初值则没有定义（即初值为无用的信息）
     见 C程序设计语言 > 第4章 函数与程序结构 > 4.9 初始化

     什么是自动变量？根据查资料显示，自动变量可认为是局部变量
     或者见 C程序设计语言 > 附录A 参考手册 > A.4 标识符的含义 > A.4.1 存储类
     所以根据以上信息，C语言中对于未初始化的局部变量是没有默认值这一说法的
     */
    binary_tree->data = -1;
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

/* 
  1. 查看该方法的提交历史，一开始的错误写法 会导致 > 释放指针之后，再次访问该指针 > 这会导致 Segmentation fault 段错误
  例如：已经通过递归释放了binary_tree->left_sub_tree，在返回上一次递归时又再次在条件中使用binary_tree->left_sub_tree!=NULL判断
  访问这种失效的指针会导致出现【未定义的行为】
  2. 综上，修改问题代码，不再使用while循环，而是使用if判断，未NULL直接返回即可
 */
void free_binary_tree(Binarytree *binary_tree) {
    if (binary_tree == NULL)
    {
       return;
    }
    free_binary_tree(binary_tree->left_sub_tree);
    free_binary_tree(binary_tree->right_sub_tree);
    free(binary_tree);
    binary_tree=NULL;
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

void print_binary_tree_order(Binarytree *binary_tree) {
    while (binary_tree != NULL && binary_tree->data != -1)
    {
        printf("data is: %d", binary_tree->data);
        print_binary_tree_order(binary_tree->left_sub_tree);
        print_binary_tree_order(binary_tree->right_sub_tree);
    }
    return;
}

void create_binary_tree_from_user_input() {
    Binarytree *binary_tree = create_new_binary_tree();
    int tree_height=0;
    while (tree_height <= TREE_MAX_HEIGHT)
    {
        printf("please enter numbers: \n");
        int data;
        scanf("%d", &data);
        
        build_simple_binary_tree(binary_tree, data);
        printf("add node successful\n");
        tree_height = get_binary_tree_height(binary_tree);
        printf("tree height is %d\n", tree_height);
    }
    printf("reach tree max height, will free tree space\n");
    free_binary_tree(binary_tree);
}

int get_binary_tree_height(Binarytree *binary_tree) {
    int left_sub_tree_height = 0;
    int right_sub_tree_height = 0;
    if (binary_tree == NULL)
    {
        return 0;
    }
    left_sub_tree_height = get_binary_tree_height(binary_tree->left_sub_tree);
    right_sub_tree_height = get_binary_tree_height(binary_tree->right_sub_tree);
    return (left_sub_tree_height >= right_sub_tree_height ? left_sub_tree_height : right_sub_tree_height) + 1;
}