#include <stdbool.h>
/*
 二叉树结构：包括数据域和指针域
*/
typedef struct binary_tree
{
    int data;
    /* 
     C程序设计语言 > 第6章 结构 > 6.5 自引用结构 
     一个包含其自身实例的结构是非法的，但是下列声明是合法的
     struct tnode *left;
     它将left声明为指向tnode的指针，而不是tnode实例本身
    */
    struct binary_tree *left_sub_tree;
    struct binary_tree *right_sub_tree;
} Binarytree;

Binarytree *create_new_binary_tree();

Binarytree *build_simple_binary_tree(Binarytree *binary_tree, int data);

bool check_node_exist(Binarytree *binary_tree);

void free_binary_tree(Binarytree *binary_tree);

/*
打印二叉树
mode: 0 前序遍历
      1 中序遍历
      2 后序遍历
*/
void print_binary_tree(Binarytree *binary_tree, int mode);

void create_binary_tree_from_user_input();

int get_binary_tree_height(Binarytree *binary_tree);