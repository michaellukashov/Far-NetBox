#include "Utils.h"

namespace tinylog {

std::string Utils::CurrentTime()
{
  apr_time_t now = apr_time_now();
  apr_time_exp_t result;
  apr_time_exp_t * time_ptr = &result;

  if (apr_time_exp_lt(time_ptr, now) == APR_SUCCESS)
  {
    char buff[128];

    sprintf(buff, "%d-%02d-%02d %02d:%02d:%02d.%.03ld",
    time_ptr->tm_year + 1900,
    time_ptr->tm_mon + 1,
    time_ptr->tm_mday,
    time_ptr->tm_hour,
    time_ptr->tm_min,
    time_ptr->tm_sec,
    (long)time_ptr->tm_usec / 1000);

    return std::string(buff);
  }
  return std::string();
}

void Utils::CurrentTime(std::string &ref_time)
{
//  struct timeval tv;
//  struct tm *pt_tm;

//  gettimeofday(&tv, NULL);
//  pt_tm = localtime(&tv.tv_sec);
  apr_time_t now = apr_time_now();
  apr_time_exp_t result;
  apr_time_exp_t * time_ptr = &result;

  if (apr_time_exp_lt(time_ptr, now) == APR_SUCCESS)
  {
    char buff[128];
    sprintf(buff, "%04d/%02d/%02d %2d:%2d:%2d.%3ld",
      1900 + time_ptr->tm_year, time_ptr->tm_mon, time_ptr->tm_mday,
      time_ptr->tm_hour, time_ptr->tm_min, time_ptr->tm_sec, time_ptr->tm_usec);

    ref_time = buff;
  }
}

void Utils::CurrentTime(apr_time_exp_t *aprtime, apr_os_exp_time_t ** ostime)
{
//  gettimeofday(tv, NULL);
//  *tm = localtime(&tv->tv_sec);

  apr_time_t now = apr_time_now();
//  apr_time_exp_t result;
//  apr_time_exp_t * time_ptr = &result;

  if (apr_time_exp_lt(aprtime, now) == APR_SUCCESS)
  {
    // *tm = localtime(time_ptr->tm_sec);
    apr_os_exp_time_get(ostime, aprtime);
  }
}

} // namespace tinylog
