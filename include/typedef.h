#ifndef TYPEDEF_H
#define TYPEDEF_H
 
#include "stdio.h"

#ifdef _WIN32
#include <windows.h>
#define TIXO(ms) Sleep(ms)
#else
#include <unistd.h>
#define TIXO(s) sleep(s)
#endif

#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"

#define LOG_INFO(msg)    printf(ANSI_GREEN "[INFO]:  %s\n" ANSI_RESET, msg)
#define LOG_WARN(msg)    printf(ANSI_YELLOW "[WARN]:  %s\n" ANSI_RESET, msg)
#define LOG_ERR(msg)    printf(ANSI_RED "[ERROR]:  %s\n" ANSI_RESET, msg)

#define PLANE_PATH_SIZE 256


   typedef unsigned char u_int_8;
   typedef unsigned short u_int_16;
   typedef unsigned int u_int_32;
   typedef unsigned long u_int_64;

   typedef signed char s_int_8;
   typedef signed short s_int_16;
   typedef signed int s_int_32;
   typedef signed long s_int_64;

#endif

