#ifndef OPIUM_CORE_INCLUDE_H
#define OPIUM_CORE_INCLUDE_H

#define OPIUM_RET_OK   0
#define OPIUM_RET_ERR  -1
#define OPIUM_RET_FULL -2

#define OPIUM_MEMCPY_LIMIT 2048
#define OPIUM_MEMSET_LIMIT 2048

#define opium_min(a,b) ((a) < (b) ? (a) : (b))
#define opium_max(a,b) ((a) > (b) ? (a) : (b))

void opium_cpuinfo(void);

#endif /* OPIUM_CORE_INCLUDE_H */
