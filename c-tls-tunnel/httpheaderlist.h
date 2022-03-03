#ifndef HTTP_Header_List_H_
#define HTTP_Header_List_H_

#include <stdbool.h>
#include "common.h"

/* 特定程序的声明 */

struct http_header
{
    char header[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
};

/* 一般类型定义 */

typedef struct http_header Item;
typedef struct node
{
    Item item;
    struct node *next;
} Node;

// 可以理解为，List 类型是指针，指针指向的是 Node 类型的结构体
// 如果要赋值给 List 类型的变量，则需要使用 & 地址运算符
// 如 List myNode = &node;
typedef Node *List;

/* 函数原型 */

/* 操作：  初始化一个链表 */
/* 前提条件：    plist 指向一个链表 */
/* 后置条件：    链表初始化为空 */
void http_header_initializeList(List *plist);

/* 操作：  确定链表是否为空定义，plist 指向一个已初始化的链表 */
/* 后置条件：    如果链表为空，该函数返回 true；范泽返回 false */
bool http_header_listIsEmpty(const List *plist);

/* 操作：  确定链表是否已满，plist 指向一个已初始化的链表 */
/* 后置条件：    如果链表已满，该函数返回 true；范泽返回 false */
bool http_header_listIsFull(const List *plist);

/* 操作：  确定链表中的项数，plist 指向一个已初始化的链表 */
/* 后置条件：    该函数返回链表中的项数 */
unsigned int http_header_listItemCount(const List *plist);

/* 操作：  在链表末尾添加项 */
/* 前提条件：    item 是一个待添加至链表的项，plist 指向一个已初始化的链表 */
/* 后置条件：    如果可以，该函数在链表末尾添加一个项，且返回 true；否则返回 false */
bool http_header_addItem(Item item, List *plise);

/* 操作：  把函数作用于链表中的每一项 */
/*          plist指向一个已初始化的链表 */
/*          pfun 指向一个函数，该函数接受一个 Item 类型的参数，且无返回值 */
/* 后置条件：pfun 指向的函数作用于链表中的每一项一次 */
void http_header_traverse(const List *plist, void (*pfun)(Item item));

/* 操作：  释放已分配的内存（如果有的话） */
/*          plist 指向一个已初始化的链表*/
/* 后置条件：    释放了为链表分配的所有内存，链表设置为空 */
void http_header_emptyTheList(List *plist);
#endif // HTTP_Header_List_H_
