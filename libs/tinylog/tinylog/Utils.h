//
// Created by Administrator on 2016/12/15.
//

#ifndef TINYLOG_UTILS_H
#define TINYLOG_UTILS_H

#include <iostream>
#include <sys/time.h>
#include <string>

class Utils {
public:
    enum LogLevel {
        TRACE   = 0,
        DEBUG   = 1,
        INFO    = 2,
        WARNING = 3,
        ERROR   = 4,
        FATAL   = 5,
    };

    static std::string GetCurrentTime();

    static void GetCurrentTime(std::string &ref_time);

    static void GetCurrentTime(struct timeval *tv, struct tm **tm);
};


#endif //TINYLOG_UTILS_H
