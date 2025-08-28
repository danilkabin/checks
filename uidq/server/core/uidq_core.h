#ifndef UIDQ_CORE_INCLUDE_H 
#define UIDQ_CORE_INCLUDE_H

#define UIDQ_RET_OK   0
#define UIDQ_RET_ERR  -1
#define UIDQ_RET_FULL -2

#define uidq_min(a,b) ((a) < (b) ? (a) : (b))
#define uidq_max(a,b) ((a) > (b) ? (a) : (b))

void uidq_cpuinfo(void);

#endif /* UIDQ_CORE_INCLUDE_H */
