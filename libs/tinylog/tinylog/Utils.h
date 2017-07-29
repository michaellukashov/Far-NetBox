#pragma once

#include <iostream>
#include <stdint.h>
#include <string>

#include "platform_win32.h"

namespace tinylog {

class Utils
{
public:
  typedef uint32_t LogLevel;

  static const LogLevel
    LEVEL_TRACE   = 0,
    LEVEL_DEBUG   = 1,
    LEVEL_INFO    = 2,
    LEVEL_WARNING = 3,
    LEVEL_ERROR   = 4,
    LEVEL_FATAL   = 5;

//  static std::string CurrentTime();
//  static void CurrentTime(std::string &ref_time);
  static void CurrentTime(struct timeval *tv, struct tm ** tm);
};

} // namespace tinylog
