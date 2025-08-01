#ifndef UIDQ_H
#define UIDQ_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "uidq_utils.h"

#define UIDQ_VERSION 0000001 
#define UIDQ_VER "0.0.1"

#define UIDQ_CONF_NAME "config.ini"

#define UIDQ_VER_BUILD "build/"UIDQ_VERSION
#define UIDQ_VER_CONF UIDQ_VER_BUILD"/"UIDQ_CONF_NAME

int uidq_daemon();

#endif
