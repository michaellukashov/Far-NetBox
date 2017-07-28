//
// Created by Administrator on 2016/12/15.
//

#include "Utils.h"

std::string Utils::GetCurrentTime()
{
    struct timeval tv;
    struct tm *time_ptr;

    gettimeofday(&tv, NULL);

    time_ptr = localtime(&tv.tv_sec);
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

void Utils::GetCurrentTime(std::string &ref_time)
{
    struct timeval tv;
    struct tm *pt_tm;

    gettimeofday(&tv, NULL);
    pt_tm = localtime(&tv.tv_sec);

    char buff[128];
    sprintf(buff, "%04d/%02d/%02d %2d:%2d:%2d.%3ld", 1900 + pt_tm->tm_year, pt_tm->tm_mon, pt_tm->tm_mday,
            pt_tm->tm_hour, pt_tm->tm_min, pt_tm->tm_sec, tv.tv_usec);

    ref_time = buff;
}

void Utils::GetCurrentTime(struct timeval *tv, struct tm **tm)
{
    gettimeofday(tv, NULL);
    *tm = localtime(&tv->tv_sec);
}