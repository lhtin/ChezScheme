/* time.c -- ChezSchemeOS freestanding libc: time functions using RV64 rdtime CSR
 *
 * QEMU virt timer runs at 10 MHz.
 */

#include <stddef.h>
#include <stdint.h>

extern void *memset(void *, int, size_t);

/* Types */
typedef long time_t;
typedef long clock_t;
typedef int clockid_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct timeval {
    time_t tv_sec;
    long tv_usec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
    /* remaining fields not needed */
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
};

#define CLOCK_REALTIME          0
#define CLOCK_MONOTONIC         1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3

#define CLOCKS_PER_SEC 1000000

#define TIMER_FREQ 10000000UL  /* 10 MHz for QEMU virt */

static inline uint64_t rdtime_val(void) {
    uint64_t val;
    __asm__ volatile("rdtime %0" : "=r"(val));
    return val;
}

int clock_gettime(clockid_t clk, struct timespec *tp) {
    (void)clk;
    if (!tp) return -1;
    uint64_t t = rdtime_val();
    tp->tv_sec = (time_t)(t / TIMER_FREQ);
    tp->tv_nsec = (long)((t % TIMER_FREQ) * (1000000000UL / TIMER_FREQ));
    return 0;
}

int gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;
    if (!tv) return -1;
    uint64_t t = rdtime_val();
    tv->tv_sec = (time_t)(t / TIMER_FREQ);
    tv->tv_usec = (long)((t % TIMER_FREQ) * (1000000UL / TIMER_FREQ));
    return 0;
}

time_t time(time_t *tloc) {
    uint64_t t = rdtime_val();
    time_t seconds = (time_t)(t / TIMER_FREQ);
    if (tloc) *tloc = seconds;
    return seconds;
}

clock_t clock(void) {
    uint64_t t = rdtime_val();
    return (clock_t)((t * CLOCKS_PER_SEC) / TIMER_FREQ);
}

double difftime(time_t t1, time_t t0) {
    return (double)(t1 - t0);
}

/* Days in each month (non-leap year) */
static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_year(int year) {
    return is_leap_year(year) ? 366 : 365;
}

struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    if (!timep || !result) return NULL;

    time_t t = *timep;
    int days = (int)(t / 86400);
    int rem = (int)(t % 86400);
    if (rem < 0) { rem += 86400; days--; }

    result->tm_hour = rem / 3600;
    rem %= 3600;
    result->tm_min = rem / 60;
    result->tm_sec = rem % 60;

    /* Day of week: Jan 1 1970 was Thursday (4) */
    result->tm_wday = (days + 4) % 7;
    if (result->tm_wday < 0) result->tm_wday += 7;

    int year = 1970;
    while (days >= days_in_year(year)) {
        days -= days_in_year(year);
        year++;
    }
    while (days < 0) {
        year--;
        days += days_in_year(year);
    }

    result->tm_year = year - 1900;
    result->tm_yday = days;

    int mon;
    for (mon = 0; mon < 12; mon++) {
        int dim = days_in_month[mon];
        if (mon == 1 && is_leap_year(year)) dim++;
        if (days < dim) break;
        days -= dim;
    }
    result->tm_mon = mon;
    result->tm_mday = days + 1;
    result->tm_isdst = 0;

    return result;
}

static struct tm _gmtime_buf;

struct tm *gmtime(const time_t *timep) {
    return gmtime_r(timep, &_gmtime_buf);
}

struct tm *localtime_r(const time_t *timep, struct tm *result) {
    return gmtime_r(timep, result); /* No timezone on bare-metal */
}

static struct tm _localtime_buf;

struct tm *localtime(const time_t *timep) {
    return localtime_r(timep, &_localtime_buf);
}

time_t mktime(struct tm *tm) {
    int year = tm->tm_year + 1900;
    time_t days = 0;

    /* Days from 1970 to year */
    if (year >= 1970) {
        for (int y = 1970; y < year; y++) days += days_in_year(y);
    } else {
        for (int y = year; y < 1970; y++) days -= days_in_year(y);
    }

    /* Days from month */
    for (int m = 0; m < tm->tm_mon; m++) {
        days += days_in_month[m];
        if (m == 1 && is_leap_year(year)) days++;
    }

    days += tm->tm_mday - 1;

    return days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}

/* Basic strftime */
extern int snprintf(char *, size_t, const char *, ...);
extern size_t strlen(const char *);
extern void *memcpy(void *, const void *, size_t);

size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *tm) {
    char buf[256];
    size_t pos = 0;

    while (*format && pos < maxsize - 1) {
        if (*format != '%') {
            s[pos++] = *format++;
            continue;
        }
        format++;

        int len = 0;
        switch (*format) {
        case 'Y': len = snprintf(buf, sizeof(buf), "%04d", tm->tm_year + 1900); break;
        case 'm': len = snprintf(buf, sizeof(buf), "%02d", tm->tm_mon + 1); break;
        case 'd': len = snprintf(buf, sizeof(buf), "%02d", tm->tm_mday); break;
        case 'H': len = snprintf(buf, sizeof(buf), "%02d", tm->tm_hour); break;
        case 'M': len = snprintf(buf, sizeof(buf), "%02d", tm->tm_min); break;
        case 'S': len = snprintf(buf, sizeof(buf), "%02d", tm->tm_sec); break;
        case 'Z': len = snprintf(buf, sizeof(buf), "UTC"); break;
        case 'j': len = snprintf(buf, sizeof(buf), "%03d", tm->tm_yday + 1); break;
        case '%': buf[0] = '%'; len = 1; break;
        default: buf[0] = '%'; buf[1] = *format; len = 2; break;
        }
        format++;

        for (int i = 0; i < len && pos < maxsize - 1; i++)
            s[pos++] = buf[i];
    }

    s[pos] = '\0';
    return pos;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!req) return -1;
    uint64_t target = rdtime_val();
    target += (uint64_t)req->tv_sec * TIMER_FREQ;
    target += (uint64_t)req->tv_nsec * TIMER_FREQ / 1000000000UL;
    while (rdtime_val() < target) {
        __asm__ volatile("nop");
    }
    if (rem) { rem->tv_sec = 0; rem->tv_nsec = 0; }
    return 0;
}

unsigned int sleep(unsigned int seconds) {
    struct timespec ts = { seconds, 0 };
    nanosleep(&ts, NULL);
    return 0;
}

int usleep(unsigned int usec) {
    struct timespec ts = { usec / 1000000, (usec % 1000000) * 1000L };
    return nanosleep(&ts, NULL);
}

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)

int getrusage(int who, struct rusage *usage) {
    (void)who;
    if (usage) memset(usage, 0, sizeof(*usage));
    return 0;
}

/* ctime_r: convert time_t to string */
char *ctime_r(const time_t *timep, char *buf) {
    struct tm tm;
    gmtime_r(timep, &tm);
    /* Format: "Wed Jun 30 21:49:08 1993\n" */
    static const char *wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char *mon[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                "Jul","Aug","Sep","Oct","Nov","Dec"};
    /* Simple sprintf-like formatting */
    char *p = buf;
    const char *s;
    s = wday[tm.tm_wday % 7]; while (*s) *p++ = *s++;
    *p++ = ' ';
    s = mon[tm.tm_mon % 12]; while (*s) *p++ = *s++;
    *p++ = ' ';
    *p++ = '0' + tm.tm_mday / 10;
    *p++ = '0' + tm.tm_mday % 10;
    *p++ = ' ';
    *p++ = '0' + tm.tm_hour / 10;
    *p++ = '0' + tm.tm_hour % 10;
    *p++ = ':';
    *p++ = '0' + tm.tm_min / 10;
    *p++ = '0' + tm.tm_min % 10;
    *p++ = ':';
    *p++ = '0' + tm.tm_sec / 10;
    *p++ = '0' + tm.tm_sec % 10;
    *p++ = ' ';
    int y = tm.tm_year + 1900;
    *p++ = '0' + (y / 1000) % 10;
    *p++ = '0' + (y / 100) % 10;
    *p++ = '0' + (y / 10) % 10;
    *p++ = '0' + y % 10;
    *p++ = '\n';
    *p = '\0';
    return buf;
}

char *ctime(const time_t *timep) {
    static char buf[26];
    return ctime_r(timep, buf);
}

char *asctime_r(const struct tm *tm, char *buf) {
    time_t t = mktime((struct tm *)tm);
    return ctime_r(&t, buf);
}

char *asctime(const struct tm *tm) {
    static char buf[26];
    return asctime_r(tm, buf);
}
