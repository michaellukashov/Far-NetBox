#pragma once

#include <nbglobals.h>

namespace tinylog {

class Utils
{
public:
  using LogLevel = uint32_t;

  static const LogLevel
    LEVEL_TRACE   = 0,
    LEVEL_DEBUG   = 1,
    LEVEL_INFO    = 2,
    LEVEL_WARNING = 3,
    LEVEL_ERROR   = 4,
    LEVEL_FATAL   = 5;

  static std::string CurrentTime();
  static void CurrentTime(std::string &ref_time);
  static void CurrentTime(struct timeval *tv, struct tm **tm);
};

static constexpr const char * past_last_slash(const char * str, const char * last_slash)
{
  return *str == '\0' ? last_slash
    : ((*str == '/') || (*str == '\\')) ? past_last_slash(str + 1, str + 1) : past_last_slash(str + 1, last_slash);
}

static constexpr const char * past_last_slash(const char * str)
{
  return past_last_slash(str, str);
}

} // namespace tinylog
