#ifndef LISTHEAD_H
#define LISTHEAD_H

#include "stddef.h"
#include <urcu/pointer.h>

#define LIST_POISON1  ((void *)0x00100100)
#define LIST_POISON2  ((void *)0x00200200)

#define container_of(ptr, type, member) \
   ((type*)((char*)ptr - offsetof(type, member)))

struct list_head {
   struct list_head *next;
   struct list_head *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list) {
   list->next = list;
   list->prev = list;
}

static inline int __list_add_valid(struct list_head *prev, struct list_head *next) {
   return prev->next == next && next->prev == prev; 
}

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
   if (!__list_add_valid(prev, next)) {
      return;
   }
   next->prev = new;
   new->next = next; 
   new->prev = prev;
   prev->next = new; 
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
   next->prev = prev;
   prev->next = next;
}

static inline void __list_add_rcu(struct list_head *new, struct list_head *prev, struct list_head *next) {
   if (!__list_add_valid(prev, next)) {
      return;
   }
   rcu_assign_pointer(next->prev, new);
   rcu_assign_pointer(new->next, next);
   rcu_assign_pointer(new->prev, prev);
   rcu_assign_pointer(prev->next, new);
}

static inline void __list_del_rcu(struct list_head *prev, struct list_head *next) {
   rcu_assign_pointer(next->prev, prev);
   rcu_assign_pointer(prev->next, next);
}

static inline void list_add(struct list_head *new, struct list_head *list) {
   __list_add(new, list, list->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *list) {
   __list_add(new, list->prev, list);
}

static inline void list_del(struct list_head *entry) {
   __list_del(entry->prev, entry->next);
   entry->next = LIST_POISON1;
   entry->prev = LIST_POISON2;
}

static inline void list_add_rcu(struct list_head *new, struct list_head *list) {
   __list_add_rcu(new, list, rcu_dereference(list->next));
}

static inline void list_del_rcu(struct list_head *entry) {
   __list_del_rcu(rcu_dereference(entry->prev), rcu_dereference(entry->next));
   entry->next = LIST_POISON1;
   entry->prev = LIST_POISON2;
}

static inline int list_is_head(struct list_head *entry, struct list_head *head) {
   return head == entry;
}

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
   struct list_head name = LIST_HEAD_INIT(name)

#define list_entry(ptr, type, member) \
   container_of(ptr, type, member)

#define list_entry_is_head(pos, head, member) \
   list_is_head(&pos->member, (head))

#define list_first_entry(ptr, type, member) \
   list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
   list_entry((ptr)->prev, type, member)

#define list_next_entry(pos, member) \
   list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each(pos, head) \
   for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_rcu(pos, head) \
   for (pos = rcu_dereference((head)->next); pos != (head); pos = rcu_dereference(pos->next))

#define list_for_each_entry_rcu(pos, head, member) \
   for (pos = container_of(rcu_dereference((head)->next), typeof(*pos), member); \
        &pos->member != (head); \
        pos = container_of(rcu_dereference(pos->member.next), typeof(*pos), member))

#define list_for_each_entry_safe(pos, temp, head, member)                   \
   for (pos = list_first_entry(head, typeof(*pos), member),                \
         temp = list_next_entry(pos, member);                               \
         !list_entry_is_head(pos, head, member);                            \
         pos = temp, temp = list_next_entry(temp, member))


#endif
