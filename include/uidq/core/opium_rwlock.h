#ifndef OPIUM_RWLOCK_INCLUDE_H
#define OPIUM_RWLOCK_INCLUDE_H

#include <stdatomic.h>

void opium_rw_wlock();
void opium_rw_rlock();
void opium_rw_unlock();

#endif /* OPIUM_RWLOCK_INCLUDE_H */
