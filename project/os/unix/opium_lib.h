#ifndef OPIUM_LINUX_CONF_H
#define OPIUM_LINUX_CONF_H

/* -------------------- Standard C headers -------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>   /* offsetof() */
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

/* -------------------- POSIX / system headers -------------------- */
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <glob.h>
#include <signal.h>
#include <sys/resource.h>
#include <sched.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/vfs.h>       /* statfs() */
#include <sys/utsname.h>   /* uname() */

/* -------------------- Networking headers -------------------- */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>   /* TCP_NODELAY, TCP_CORK */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
/* -------------------- Memory / misc -------------------- */
#include <malloc.h>         /* memalign() */

#include <pthread.h>

#endif /* OPIUM_LINUX_CONF_H */

