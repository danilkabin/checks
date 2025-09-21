#ifndef OPIUM_LISTHEAD_INCLUDE_H
#define OPIUM_LISTHEAD_INCLUDE_H

#include <stddef.h>

#define OPIUM_LIST_POISON1  ((void *)0x00100100)
#define OPIUM_LIST_POISON2  ((void *)0x00200200)

#define opium_container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

struct opium_list_head {
    struct opium_list_head *next;
    struct opium_list_head *prev;
};

static inline void OPIUM_INIT_LIST_HEAD(struct opium_list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline int __opium_list_add_valid(struct opium_list_head *prev, struct opium_list_head *next) {
    return prev->next == next && next->prev == prev;
}

static inline void __opium_list_add(struct opium_list_head *new,
                                   struct opium_list_head *prev,
                                   struct opium_list_head *next) {
    if (!__opium_list_add_valid(prev, next)) {
        return;
    }
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline int opium_list_empty(struct opium_list_head *head) {
   return head->next == head;
}

static inline void __opium_list_del(struct opium_list_head *prev, struct opium_list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void opium_list_add(struct opium_list_head *new, struct opium_list_head *list) {
    __opium_list_add(new, list, list->next);
}

static inline void opium_list_add_tail(struct opium_list_head *new, struct opium_list_head *list) {
    __opium_list_add(new, list->prev, list);
}

static inline void opium_list_del(struct opium_list_head *entry) {
    __opium_list_del(entry->prev, entry->next);
    entry->next = OPIUM_LIST_POISON1;
    entry->prev = OPIUM_LIST_POISON2;
}

static inline int opium_list_is_head(struct opium_list_head *entry, struct opium_list_head *head) {
    return head == entry;
}

static inline int opium_list_is_linked(struct opium_list_head *entry) {
    return entry->next != OPIUM_LIST_POISON1 && entry->prev != OPIUM_LIST_POISON2;
}

#define OPIUM_LIST_HEAD_INIT(name) { &(name), &(name) }

#define OPIUM_LIST_HEAD(name) \
    struct opium_list_head name = OPIUM_LIST_HEAD_INIT(name)

#define opium_list_entry(ptr, type, member) \
    opium_container_of(ptr, type, member)

#define opium_list_entry_is_head(pos, head, member) \
    opium_list_is_head(&(pos)->member, (head))

#define opium_list_first_entry(ptr, type, member) \
    opium_list_entry((ptr)->next, type, member)

#define opium_list_last_entry(ptr, type, member) \
    opium_list_entry((ptr)->prev, type, member)

#define opium_list_next_entry(pos, member) \
    opium_list_entry((pos)->member.next, typeof(*(pos)), member)

#define opium_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define opium_list_for_each_entry_safe(pos, temp, head, member)                   \
    for (pos = opium_list_first_entry(head, typeof(*pos), member),                \
         temp = opium_list_next_entry(pos, member);                               \
         !opium_list_entry_is_head(pos, head, member);                            \
         pos = temp, temp = opium_list_next_entry(temp, member))

#endif /* OPIUM_LISTHEAD_INCLUDE_H */
