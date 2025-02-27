#ifndef COSMOPOLITAN_LIBC_TIME_TIME_H_
#define COSMOPOLITAN_LIBC_TIME_TIME_H_

#define TIME_UTC 1

#if !(__ASSEMBLER__ + __LINKER__ + 0)
COSMOPOLITAN_C_START_

extern char *tzname[2];
extern long timezone;
extern int daylight;

char *ctime(const int64_t *);
char *ctime_r(const int64_t *, char[hasatleast 26]);
double difftime(int64_t, int64_t) pureconst;
int64_t posix2time(int64_t) pureconst;
int64_t time2posix(int64_t) pureconst;
void tzset(void);

#ifdef COSMO
extern const char kWeekdayNameShort[7][4];
extern const char kWeekdayName[7][10];
extern const char kMonthNameShort[12][4];
extern const char kMonthName[12][10];
extern const unsigned short kMonthYearDay[2][12];
extern long double (*nowl)(void);
long double ConvertTicksToNanos(double);
long double dsleep(long double);
long double dtime(int);
void RefreshTime(void);
#endif

COSMOPOLITAN_C_END_
#endif /* !(__ASSEMBLER__ + __LINKER__ + 0) */
#endif /* COSMOPOLITAN_LIBC_TIME_TIME_H_ */
