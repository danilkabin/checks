#ifndef UIDQ_CORE_H
#define UIDQ_CORE_H

#include <stddef.h>

#include "uidq_types.h"

#define UIDQ_VERSION 0000001
#define UIDQ_VER "0.0.1"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define UIDQ_CONF_NAME "config.ini"

#define UIDQ_VER_BUILD "build/" STR(UIDQ_VERSION)
#define UIDQ_VER_BIN UIDQ_VER_BUILD "/bin/"
#define UIDQ_VER_CONF UIDQ_VER_BUILD "/" UIDQ_CONF_NAME

#define UIDQ_PROCESS_WORKER_NAME "process_worker"
#define UIDQ_PROCESS_WORKER_PATH UIDQ_VER_BIN UIDQ_PROCESS_WORKER_NAME

#endif
