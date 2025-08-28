#ifndef UIDQ_RWLOCK_INCLUDE_H
#define UIDQ_RWLOCK_INCLUDE_H

#include <stdatomic.h>

void uidq_rw_wlock();
void uidq_rw_rlock();
void uidq_rw_unlock();

#endif /* UIDQ_RWLOCK_INCLUDE_H */
