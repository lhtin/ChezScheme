/* time.h -- Time functions for bare-metal RV64G */

#ifndef _BAREMETAL_TIME_H
#define _BAREMETAL_TIME_H

#include <sys/types.h>
#include <stddef.h>

#define CLOCKS_PER_SEC  1000000L

/* clockid_t values */
#define CLOCK_REALTIME              0
#define CLOCK_MONOTONIC             1
#define CLOCK_PROCESS_CPUTIME_ID    2
#define CLOCK_THREAD_CPUTIME_ID     3
#define CLOCK_MONOTONIC_RAW         4

#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC
struct timespec {
    time_t  tv_sec;
    long    tv_nsec;
};
#endif

struct tm {
    int tm_sec;     /* seconds [0,60] */
    int tm_min;     /* minutes [0,59] */
    int tm_hour;    /* hours [0,23] */
    int tm_mday;    /* day of month [1,31] */
    int tm_mon;     /* month [0,11] */
    int tm_year;    /* years since 1900 */
    int tm_wday;    /* day of week [0,6] (Sunday=0) */
    int tm_yday;    /* day of year [0,365] */
    int tm_isdst;   /* daylight saving flag */
    long tm_gmtoff; /* offset from UTC in seconds */
    const char *tm_zone; /* timezone abbreviation */
};

time_t          time(time_t *tloc);
clock_t         clock(void);
int             clock_gettime(clockid_t clk_id, struct timespec *tp);
int             clock_getres(clockid_t clk_id, struct timespec *res);
double          difftime(time_t time1, time_t time0);
struct tm      *gmtime(const time_t *timep);
struct tm      *gmtime_r(const time_t *timep, struct tm *result);
struct tm      *localtime(const time_t *timep);
struct tm      *localtime_r(const time_t *timep, struct tm *result);
time_t          mktime(struct tm *tm);
time_t          timegm(struct tm *tm);
size_t          strftime(char *s, size_t max, const char *format, const struct tm *tm);
char           *asctime(const struct tm *tm);
char           *asctime_r(const struct tm *tm, char *buf);
char           *ctime(const time_t *timep);
char           *ctime_r(const time_t *timep, char *buf);
int             nanosleep(const struct timespec *req, struct timespec *rem);
char           *strptime(const char *s, const char *format, struct tm *tm);

/* timer_t stubs */
typedef int timer_t;

#endif /* _BAREMETAL_TIME_H */
