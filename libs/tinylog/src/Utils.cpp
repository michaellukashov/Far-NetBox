#include <tinylog/platform_win32.h>
#include <tinylog/Utils.h>

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

std::string Utils::CurrentTime()
{
  struct timeval tv;
  struct tm* time_ptr;

  gettimeofday(&tv, NULL);

  time_t time = static_cast<time_t>(tv.tv_sec);
  time_ptr = localtime(&time);
  char buff[128];

  sprintf(buff, "%d-%02d-%02d %02d:%02d:%02d.%.03ld",
    time_ptr->tm_year + 1900,
    time_ptr->tm_mon + 1,
    time_ptr->tm_mday,
    time_ptr->tm_hour,
    time_ptr->tm_min,
    time_ptr->tm_sec,
    (long)tv.tv_usec / 1000);

  return std::string(buff);
}

void Utils::CurrentTime(std::string &ref_time)
{
  struct timeval tv;
  struct tm* pt_tm;

  gettimeofday(&tv, nullptr);
  time_t time = static_cast<time_t>(tv.tv_sec);
  pt_tm = localtime(&time);

  char buff[128];
  sprintf(buff, "%04d/%02d/%02d %2d:%2d:%2d.%3ld",
    1900 + pt_tm->tm_year, pt_tm->tm_mon, pt_tm->tm_mday,
    pt_tm->tm_hour, pt_tm->tm_min, pt_tm->tm_sec, tv.tv_usec / 1000);

  ref_time = buff;
}

void Utils::CurrentTime(struct timeval *tv, struct tm **tm)
{
  gettimeofday(tv, nullptr);
  /* *tm = localtime(&tv->tv_sec);
  time_t rawtime;
  time(&rawtime);*/
  time_t time = static_cast<time_t>(tv->tv_sec);
  *tm = localtime(&time);
}

} // namespace tinylog
