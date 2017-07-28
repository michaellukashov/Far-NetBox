//
// Created by ouyangliduo on 2016/12/13.
//

#ifndef TINYLOG_LOGSTREAM_H
#define TINYLOG_LOGSTREAM_H

#include <sys/time.h>

#include "Buffer.h"
#include "Utils.h"

class LogStream {
public:
    friend class TinyLog;

    LogStream();

    ~LogStream();

    void SwapBuffer();

    void WriteBuffer();

    void SetPrefix(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);

    LogStream& operator<<(const char *pt_log);

    LogStream& operator<<(const std::string &ref_log);

    void UpdateBaseTime();
private:

    Buffer *pt_front_buff_;

    Buffer *pt_back_buff_;

    int log_file_fd_;

    const char *pt_file_;

    int i_line_;

    const char *pt_func_;

    std::string str_log_level_;

    struct timeval tv_base_;

    struct tm *pt_tm_base_;
};

inline
void LogStream::SetPrefix(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level)
{
    pt_file_ = pt_file;
    i_line_  = i_line;
    pt_func_ = pt_func;

    switch (e_log_level)
    {
        case Utils::DEBUG  : str_log_level_ = "[DEBUG  ]"; break;
        case Utils::INFO   : str_log_level_ = "[INFO   ]"; break;
        case Utils::WARNING: str_log_level_ = "[WARNING]"; break;
        case Utils::ERROR  : str_log_level_ = "[ERROR  ]"; break;
        case Utils::FATAL  : str_log_level_ = "[FATAL  ]"; break;
        default: str_log_level_ = "[INFO   ]"; break;
    }
}

#endif //TINYLOG_LOGSTREAM_H
