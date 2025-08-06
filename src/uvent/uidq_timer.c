#include <bits/types/struct_timeval.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "core/uidq_string.h"
#include "core/uidq_utils.h"
#include "uvent/uidq_timer.h"

int uidq_timer_yes() { 
   time_t t;
   t = time(NULL);

   struct tm *gmp;
   struct tm gm;

   DEBUG_FUNC("tm: %d\n", gm.tm_mday);

   gmp = gmtime(&t);
   DEBUG_FUNC("tm. year:%d, month: %d week:%d day: %d\n",
         gmp->tm_year, gmp->tm_mon, gmp->tm_wday, gmp->tm_mday);

   int year = gmp->tm_year + 1900;
   int month = gmp->tm_mon + 1;
   int wday = gmp->tm_wday;
   int mday = gmp->tm_mday;

   DEBUG_FUNC("tm. year:%d, month: %d week:%d day: %d\n",
         year, month, wday, mday);

   struct timeval tv = {0, 0};
   DEBUG_FUNC("tv before: sec: %ld, usec: %ld\n", tv.tv_sec, tv.tv_usec);
   gettimeofday(&tv, NULL);
   DEBUG_FUNC("tv after: sec: %ld, usec: %ld\n", tv.tv_sec, tv.tv_usec);

   DEBUG_FUNC("tm before local time. year:%d, month: %d week:%d day: %d\n",
         gmp->tm_year, gmp->tm_mon, gmp->tm_wday, gmp->tm_mday);
   gmp = localtime(&t);
   DEBUG_FUNC("tm after local time. year:%d, month: %d week:%d day: %d\n",
         gmp->tm_year, gmp->tm_mon, gmp->tm_wday, gmp->tm_mday);

   gm = *gmp;

   DEBUG_FUNC("asctime() format the gmtime() as: %s", asctime(&gm));
   DEBUG_FUNC("ctime() format the gmtime() as: %s", ctime(&t));
   DEBUG_FUNC("mktime() format the gmtime() as: %ld\n", (long)mktime(&gm));

   size_t buff_size = 1000;
   char buff[buff_size];
   memset(buff, 0, buff_size); 

   time_t timer;
   struct tm *tm;
   timer = time(NULL);
   tm = localtime(&timer);

   size_t size = strftime(buff, buff_size, "%A", tm);
   DEBUG_FUNC("buffer: %s, buffer size: %zu\n", buff, size);

   size_t len = uidq_strlen(buff, buff_size);

   DEBUG_FUNC("buffer len: %zu\n", len);

   return -1;  
}
