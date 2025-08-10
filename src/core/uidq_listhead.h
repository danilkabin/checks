#ifndef UIDQ_LISTHEAD_H
#define UIDQ_LISTHEAD_H

#include <stddef.h>

#define UIDQ_LIST_POISON1  ((void *)0x00100100)
#define UIDQ_LIST_POISON2  ((void *)0x00200200)

#define uidq_container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

struct uidq_list_head {
    struct uidq_list_head *next;
    struct uidq_list_head *prev;
};

static inline void UIDQ_INIT_LIST_HEAD(struct uidq_list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline int __uidq_list_add_valid(struct uidq_list_head *prev, struct uidq_list_head *next) {
    return prev->next == next && next->prev == prev;
}

static inline void __uidq_list_add(struct uidq_list_head *new,
                                   struct uidq_list_head *prev,
                                   struct uidq_list_head *next) {
    if (!__uidq_list_add_valid(prev, next)) {
        return;
    }
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void __uidq_list_del(struct uidq_list_head *prev, struct uidq_list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void uidq_list_add(struct uidq_list_head *new, struct uidq_list_head *list) {
    __uidq_list_add(new, list, list->next);
}

static inline void uidq_list_add_tail(struct uidq_list_head *new, struct uidq_list_head *list) {
    __uidq_list_add(new, list->prev, list);
}

static inline void uidq_list_del(struct uidq_list_head *entry) {
    __uidq_list_del(entry->prev, entry->next);
    entry->next = UIDQ_LIST_POISON1;
    entry->prev = UIDQ_LIST_POISON2;
}

static inline int uidq_list_is_head(struct uidq_list_head *entry, struct uidq_list_head *head) {
    return head == entry;
}

#define UIDQ_LIST_HEAD_INIT(name) { &(name), &(name) }

#define UIDQ_LIST_HEAD(name) \
    struct uidq_list_head name = UIDQ_LIST_HEAD_INIT(name)

#define uidq_list_entry(ptr, type, member) \
    uidq_container_of(ptr, type, member)

#define uidq_list_entry_is_head(pos, head, member) \
    uidq_list_is_head(&(pos)->member, (head))

#define uidq_list_first_entry(ptr, type, member) \
    uidq_list_entry((ptr)->next, type, member)

#define uidq_list_last_entry(ptr, type, member) \
    uidq_list_entry((ptr)->prev, type, member)

#define uidq_list_next_entry(pos, member) \
    uidq_list_entry((pos)->member.next, typeof(*(pos)), member)

#define uidq_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define uidq_list_for_each_entry_safe(pos, temp, head, member)                   \
    for (pos = uidq_list_first_entry(head, typeof(*pos), member),                \
         temp = uidq_list_next_entry(pos, member);                               \
         !uidq_list_entry_is_head(pos, head, member);                            \
         pos = temp, temp = uidq_list_next_entry(temp, member))

#endif // UIDQ_LISTHEAD_H
