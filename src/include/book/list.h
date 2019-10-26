/*
 * file:		   include/book/list.h
 * auther:		Jason Hu
 * time:		   2019/6/25
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_LIST_H
#define _BOOK_LIST_H

#include <share/stdint.h>
#include <share/types.h>
#include <share/stddef.h>

/* 
 * 链表数据结构，在看了Linux的链表结构之后，觉得他那个比较通用，
 * 而且也比较清晰，所以我打算移植一个过来。本文件里面的都是内联函数，
 * 使用的时候会编译在调用的地方。并且还有很多宏定义。
 */

/*
 * 链表结构体
 */
struct List {
   struct List *prev;
   struct List *next;
};

/* 为链表结构赋值 */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/* 创建并赋值 */
#define LIST_HEAD(name) \
      struct List name = LIST_HEAD_INIT(name)

/* 让链表内容指针指向自己本身 */
PRIVATE INLINE void INIT_LIST_HEAD(struct List *list)
{
   list->next = list;
   list->prev = list;  
}

/* 把一个新的节点new插入到prev后，next前 */
PRIVATE INLINE void __ListAdd(struct List *new, 
                              struct List *prev, 
                              struct List *next)
{
   //new和next绑定关系
   next->prev = new; 
   new->next = next; 
   //new和next绑定关系
   new->prev = prev; 
   prev->next = new; 
}

/*
 * ListAdd - 添加一个新的节点到链表头
 * @new： 要新添加的节点
 * @head：要添加到哪个链表头
 * 
 * 把一个节点添加到链表头后面，相当于添加到整个链表的最前面。
 */
PRIVATE INLINE void ListAdd(struct List *new, struct List *head)
{
   // :) 插入到链表头和链表头的下一个节点之间
   __ListAdd(new, head, head->next);
}

/*
 * ListAddBefore - 把节点添加到一个节点前面
 * @new： 要新添加的节点
 * @head：比较的节点
 * 
 * 把一个新节点添加到一个节点前面。旧节点的前驱需要指向新节点，
 * 旧节点的前驱指向新节点，新节点的前驱指向旧节点的前驱，后驱指向旧节点。
 * 
 */
PRIVATE INLINE void ListAddBefore(struct List *new, struct List *node)
{
   node->prev->next = new;

   new->prev = node->prev;
   new->next = node;

   node->prev = new;
}

/*
 * ListAddAfter - 把节点添加到一个节点后面
 * @new： 要新添加的节点
 * @head：比较的节点
 * 
 * 把一个新节点添加到一个节点后面。旧节点的后驱需要指向新节点，
 * 旧节点的后驱指向新节点，新节点的前驱指向旧节点，后驱指向旧节点的后驱。
 * 
 */
PRIVATE INLINE void ListAddAfter(struct List *new, struct List *node)
{
   node->next->prev = new;

   new->prev = node;
   new->next = node->next;

   node->next = new;
}

/*
 * ListAddTail - 添加一个新的节点到链表尾
 * @new： 要新添加的节点
 * @head：要添加到哪个链表头
 * 
 * 把一个节点添加到链表头前面，相当于添加到整个链表的最后面。
 */
PRIVATE INLINE void ListAddTail(struct List *new, struct List *head)
{
   // :) 插入到链表头前一个和链表头之间
   __ListAdd(new, head->prev, head);
}

/* 把一个节点从链表中删除 */
PRIVATE INLINE void __ListDel(struct List *prev, struct List *next)
{
   // ^_^ 把前一个和下一个进行关联，那中间一个就被删去了
   next->prev = prev;
   prev->next = next;
}

/* 把一个节点从链表中删除 */
PRIVATE INLINE void __ListDelNode(struct List *node)
{
   // 传入节点的前一个和后一个，执行后只是脱离链表，而自身还保留了节点信息
   __ListDel(node->prev, node->next);
}

/*
 * ListDel - 把节点从链表中删除
 * @node：要删除的节点
 * 
 * 把一个已经存在于链表中的节点删除
 */
PRIVATE INLINE void ListDel(struct List *node)
{
   __ListDelNode(node);
   // @.@ 把前驱指针和后驱指针都指向空，完全脱离
   node->prev = NULL;
   node->next = NULL;
}

/*
 * ListDelInit - 把节点从链表中删除
 * @node：要删除的节点
 * 
 * 把一个已经存在于链表中的节点删除
 */
PRIVATE INLINE void ListDelInit(struct List *node)
{
   __ListDelNode(node);
   //初始化节点，使得可以成为链表头，我猜的。:-)
   INIT_LIST_HEAD(node);
}

/*
 * ListReplace - 用新的节点替代旧的节点
 * @old：旧的节点
 * @new：要插入的新节点
 * 
 * 用一个节点替代已经存在于链表中的节点
 */
PRIVATE INLINE void ListReplace(struct List *old, struct List *new)
{
   /*
   @.@ 把old的前后指针都指向new，那么new就替代了old，真可恶！
   不过，旧的节点中还保留了链表的信息
   */
   new->next = old->next;
   new->next->prev = new;
   new->prev = old->prev;
   new->prev->next = new;
}

PRIVATE INLINE void ListReplaceInit(struct List *old, struct List *new)
{
   /*
   先把old取代，然后把old节点初始化，使它完全脱离链表。
   */
   ListReplace(old, new);
   INIT_LIST_HEAD(old);
}

/*
 * ListMove - 从一个链表删除，然后移动到另外一个链表头后面
 * @node：要操作的节点
 * @head：新的链表头
 */
PRIVATE INLINE void ListMove(struct List *node, struct List *head)
{
   // ^.^ 先把自己脱离关系，然后添加到新的链表
   __ListDelNode(node);
   ListAdd(node, head);   
}

/*
 * ListMoveTail - 从一个链表删除，然后移动到另外一个链表头前面
 * @node：要操作的节点
 * @head：新的链表头
 */
PRIVATE INLINE void ListMoveTail(struct List *node, struct List *head)
{
   // ^.^ 先把自己脱离关系，然后添加到新的链表
   __ListDelNode(node);
   ListAddTail(node, head);   
}

/*
 * ListIsFirst - 检测节点是否是链表中的第一个节点
 * @node：要检测的节点
 * @head：链表头
 */
PRIVATE INLINE int ListIsFirst(const struct List *node, 
                                 const struct List *head)
{
   return (node->prev == head); //节点的前一个是否为链表头
}

/*
 * ListIsLast - 检测节点是否是链表中的最后一个节点
 * @node：要检测的节点
 * @head：链表头
 */
PRIVATE INLINE int ListIsLast(const struct List *node, 
                                 const struct List *head)
{
   return (node->next == head); //节点的后一个是否为链表头
}

/*
 * ListEmpty - 测试链表是否为空链表
 * @head：链表头
 * 
 * 把链表头传进去，通过链表头来判断
 */
PRIVATE INLINE int ListEmpty(const struct List *head)
{
   return (head->next == head); //链表头的下一个是否为自己
}

/* ！！！！前方！！！！高能！！！！ */

/*
 * ListOwner - 获取节点的宿主
 * @ptr： 节点的指针
 * @type： 宿主结构体的类型
 * @member: 节点在宿主结构体中的名字 
 */
#define ListOwner(ptr, type, member) container_of(ptr, type, member)

/* 嘻嘻，就这样就把container_of用上了 */

/*
 * ListFirstOwner - 获取链表中的第一个宿主
 * @head： 链表头
 * @type： 宿主结构体的类型
 * @member: 节点在宿主结构体中的名字 
 * 
 * 注：链表不能为空
 */
#define ListFirstOwner(head, type, member) \
      ListOwner((head)->next, type, member)


/*
 * ListLastOwner - 获取链表中的最后一个宿主
 * @head:  链表头
 * @type： 宿主结构体的类型
 * @member: 节点在宿主结构体中的名字 
 * 
 * 注：链表不能为空
 */
#define ListLastOwner(head, type, member) \
      ListOwner((head)->prev, type, member)

/*
 * ListFirstOwnerOrNull - 获取链表中的第一个宿主
 * @head： 链表头
 * @type： 宿主结构体的类型
 * @member: 节点在宿主结构体中的名字 
 * 
 * 注：如果链表是空就返回NULL
 */
#define ListFirstOwnerOrNull(head, type, member) ({ \
      struct List *__head = (head); \
      struct List *__pos = (__head->next); \
      __pos != __head ? ListOwner(__pos, type, member) : NULL; \
})

/*
 * ListNextOwner - 获取链表中的下一个宿主
 * @pos： 临时宿主的指针
 * @member: 节点在宿主结构体中的名字 
 */
#define ListNextOwner(pos, member) \
      ListOwner((pos)->member.next, typeof(*(pos)), member)

/*
 * ListPrevOwner - 获取链表中的前一个宿主
 * @pos： 临时宿主的指针
 * @member: 节点在宿主结构体中的名字 
 */
#define ListPrevOwner(pos, member) \
      ListOwner((pos)->member.prev, typeof(*(pos)), member)

/* 把代码自己打一遍，好累啊！但是感觉这些东西也更加明白了 */

/* 记住啦，这是遍历链表节点，不是宿主 -->>*/

/*
 * ListForEach - 从前往后遍历每一个链表节点
 * @pos： 节点指针
 * @head: 链表头 
 */
#define ListForEach(pos, head) \
      for (pos = (head)->next; pos != (head); pos = pos->next)

/*
 * ListFind - 从前往后遍历查找链表节点
 * @list: 要查找的节点指针
 * @head: 链表头
 * 
 * 找到返回1，否则返回0 
 */
PRIVATE INLINE int ListFind(struct List *list, struct List *head)
{
   struct List *__list;
   ListForEach(__list, head) {
      // 找到一样的
      if (__list == list) {
         return 1;
      }
   }
   return 0;
}

/*
 * ListLength - 获取链表长度
 * @head: 链表头
 */
PRIVATE INLINE int ListLength(struct List *head)
{
   struct List *list;
   int n = 0;
   ListForEach(list, head) {
      // 找到一样的
      if (list == head)
         break;
      n++;
   }
   return n;
}

/*
 * ListForEachPrev - 从后往前遍历每一个链表节点
 * @pos： 节点指针
 * @head: 链表头 
 */
#define ListForEachPrev(pos, head) \
      for (pos = (head)->prev; pos != (head); pos = pos->prev)

/*
 * ListForEachSafe - 从前往后遍历每一个链表节点
 * @pos: 节点指针
 * @_next: 临时节点指针（为了避免和pos->next混淆，在前面加_）
 * @head: 链表头 
 * 
 * 用next来保存下一个节点指针，如果在遍历过程中pos出的节点被删除了，
 * 还是可以继续往后面遍历其它节点。
 */
#define ListForEachSafe(pos, _next, head) \
      for (pos = (head)->next, _next = pos->next; pos != (head); \
         pos = _next, _next = pos->next)

/*
 * ListForEachPrevSafe - 从后往前遍历每一个链表节点
 * @pos: 节点指针
 * @_prev: 临时节点指针（为了避免和pos->prev混淆，在前面加_）
 * @head: 链表头 
 * 
 * 用prev来保存前一个节点指针，如果在遍历过程中pos出的节点被删除了，
 * 还是可以继续往前面遍历其它节点。
 */
#define ListForEachPrevSafe(pos, _prev, head) \
      for (pos = (head)->prev, _prev = pos->prev; pos != (head); \
         pos = _prev, _prev = pos->prev)

/*  <<-- 遍历链表节点结束了，接下来开始的是遍历宿主 -->> */

/*
 * ListForEachOwner - 从前往后遍历每一个链表节点宿主
 * @pos: 宿主类型结构体指针
 * @head: 链表头 
 * @member: 节点在宿主中的名字
 */
#define ListForEachOwner(pos, head, member)                    \
      for (pos = ListFirstOwner(head, typeof(*pos), member);   \
         &pos->member != (head);                               \
         pos = ListNextOwner(pos, member))

/*
 * ListForEachOwner - 从后往前遍历每一个链表节点宿主
 * @pos: 宿主类型结构体指针
 * @head: 链表头 
 * @member: 节点在宿主中的名字
 */
#define ListForEachOwnerReverse(pos, head, member)            \
      for (pos = ListLastOwner(head, typeof(*pos), member);   \
         &pos->member != (head);                              \
         pos = ListPrevOwner(pos, member))


/*
 * ListForEachOwnerSafe - 从前往后遍历每一个链表节点宿主
 * @pos: 宿主类型结构体指针
 * @next: 临时指向下一个节点的指针
 * @head: 链表头 
 * @member: 节点在宿主中的名字
 * 
 * 可以保证在遍历过程中如果
 */
#define ListForEachOwnerSafe(pos, next, head, member)          \
      for (pos = ListFirstOwner(head, typeof(*pos), member),   \
         next = ListNextOwner(pos, member);                    \
         &pos->member != (head);                               \
         pos = next, next = ListNextOwner(next, member))

/*
 * ListForEachOwnerReverseSafe - 从后往前遍历每一个链表节点宿主
 * @pos: 宿主类型结构体指针
 * @_prev: 临时指向前一个节点的指针
 * @head: 链表头 
 * @member: 节点在宿主中的名字
 * 
 * 可以保证在遍历过程中如果
 */
#define ListForEachOwnerReverseSafe(pos, prev, head, member)   \
      for (pos = ListLastOwner(head, typeof(*pos), member),    \
         prev = ListPrevOwner(pos, member);                    \
         &pos->member != (head);                               \
         pos = prev, prev = ListPrevOwner(prev, member))

/*  <<-- 遍历链表宿主也结束了，very nice 啊！ */

/*
 * 还有很多操作没有移植过来，如果以后有需要，可以直接到linux源码处去学习。
 *  (*^_^*)
 * 我自己修改了很多表达，比如讲list_head修改成List，
 * 自己添加了node（节点：单个list）和
 * owner（宿主：就是List结构所在的结构体）的表达方式
 */

#endif   /*_BOOK_LIST_H*/
