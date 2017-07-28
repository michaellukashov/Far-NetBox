#pragma once

#include <iostream>
#include <stdint.h>
//#include <sys/time.h>
#include <apr_portable.h>
#include <apr_time.h>
#include <string>

namespace tinylog {

class Utils
{
public:
  typedef uint32_t LogLevel;

  static const LogLevel
    LL_TRACE   = 0,
    LL_DEBUG   = 1,
    LL_INFO    = 2,
    LL_WARNING = 3,
    LL_ERROR   = 4,
    LL_FATAL   = 5;

  static std::string CurrentTime();

  static void CurrentTime(std::string &ref_time);

//  static void CurrentTime(timeval *tv, tm **tm);
  static void CurrentTime(apr_time_exp_t *aprtime, apr_os_exp_time_t ** ostime);
};

} // namespace tinylog
