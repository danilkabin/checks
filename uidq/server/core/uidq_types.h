#ifndef UIDQ_TYPES_INCLUDE_H
#define UIDQ_TYPES_INCLUDE_H

typedef enum {
   UIDQ_RET_OK  = 0,
   UIDQ_RET_ERR  = -1,
   UIDQ_RET_FULL = -2,
} uidq_ret_t;

#define UIDQ_MIN(a,b) ((a) < (b) ? (a) : (b))

#endif /* UIDQ_TYPES_INCLUDE_H */
