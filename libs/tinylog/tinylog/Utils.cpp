#include "Utils.h"

namespace tinylog {

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
  time_t now = time(nullptr);
  tv->tv_sec = (long)now;
  tv->tv_usec = 0;
  *tm = localtime(&now);
}

} // namespace tinylog
