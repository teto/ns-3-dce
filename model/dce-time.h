#ifndef SIMU_TIME_H
#define SIMU_TIME_H

#include "sys/dce-time.h"
#include <time.h>
#include <utime.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(time_t, time, time_t *t);
DCE(struct tm *, gmtime, const time_t *timep);
DCE(struct tm * , localtime, const time_t *timep);
DCE(char * , ctime, const time_t *timep);
DCE(char * , asctime, const struct tm *tm);
DCE(int , clock_gettime, clockid_t which_clock, struct timespec *tp);
DCE(int , sysinfo, struct sysinfo *info);
DCE(void , tzset);
DCE(int , clock_getres, clockid_t c, struct timespec *r);
DCE(int , utime, const char *filename, const struct utimbuf *times);
DCE(int , timer_create,clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
DCE(int , timer_settime, timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
DCE(int , timer_gettime,timer_t timerid, struct itimerspec *cur_value);

NATIVE (asctime_r, const struct tm *tm, char *bug)
NATIVE (ctime_r, const time_t *timep, char *buf)
NATIVE(gmtime_r, const time_t *timep, struct tm *result)

DCE_ALIAS (gmtime, localtime)
DCE_ALIAS (gmtime_r, localtime_r)

NATIVE (mktime, struct tm *tm)
NATIVE (strftime, char *s, size_t max, const char *format, const struct tm *tm)
NATIVE (strptime, const char *s, const char *format, struct tm *tm)
NATIVE (timegm, struct tm *tm)
NATIVE (timelocal, struct tm *tm)

#ifdef __cplusplus
}
#endif

#endif /* SIMU_TIME_H */
