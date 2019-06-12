#include "platform_win32.h"
#include "Utils.h"

namespace tinylog {

static uint64_t getCurrentTimeMs()
{
  FILETIME filetime;
  GetSystemTimeAsFileTime(&filetime);

  uint64_t nowWindows = (uint64_t)filetime.dwLowDateTime
    + ((uint64_t)(filetime.dwHighDateTime) << 32ULL);

  uint64_t nowUnix = nowWindows - 116444736000000000ULL;
  return nowUnix / 10000ULL;
}

#if 0
std::string Utils::CurrentTime()
{
  apr_time_t now = apr_time_now();
  apr_time_exp_t aprexptime;
  apr_time_exp_lt(&aprexptime, now);

  char buff[128];
  sprintf(buff, "%d-%02d-%02d %02d:%02d:%02d.%.03ld",
    aprexptime.tm_year + 1900,
    aprexptime.tm_mon + 1,
    aprexptime.tm_mday,
    aprexptime.tm_hour,
    aprexptime.tm_min,
    aprexptime.tm_sec,
    (long)aprexptime.tm_usec / 1000);

  return std::string(buff);
}

void Utils::CurrentTime(std::string &ref_time)
{
  apr_time_t now = apr_time_now();
  apr_time_exp_t aprexptime;
  apr_time_exp_lt(&aprexptime, now);

  char buff[128];
  sprintf(buff, "%04d/%02d/%02d %2d:%2d:%2d.%3ld",
    1900 + aprexptime.tm_year, aprexptime.tm_mon, aprexptime.tm_mday,
    aprexptime.tm_hour, aprexptime.tm_min, aprexptime.tm_sec, aprexptime.tm_usec);

  ref_time = buff;
}
#endif // #if 0

void Utils::CurrentTime(struct timeval *tv, struct tm **tm)
{
//  gettimeofday(tv, NULL);
//  *tm = localtime(&tv->tv_sec);
//  time_t rawtime;
//  time(&rawtime);
  time_t time = static_cast<time_t>(tv->tv_sec);
  *tm = localtime(&time);
}

} // namespace tinylog
