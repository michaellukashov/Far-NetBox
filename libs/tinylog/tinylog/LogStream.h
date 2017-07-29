#pragma once

#include "platform_win32.h"

#include "Buffer.h"
#include "Utils.h"

namespace tinylog {

class LogStream
{
public:
  friend class TinyLog;

  LogStream();
  ~LogStream();

  void SwapBuffer();
  void WriteBuffer();
  void SetPrefix(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);
  LogStream &operator<<(const char *pt_log);
//  LogStream &operator<<(const std::string &ref_log);
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
  case Utils::LEVEL_DEBUG:
    str_log_level_ = "[DEBUG  ]";
    break;
  case Utils::LEVEL_INFO:
    str_log_level_ = "[INFO   ]";
    break;
  case Utils::LEVEL_WARNING:
    str_log_level_ = "[WARNING]";
    break;
  case Utils::LEVEL_ERROR:
    str_log_level_ = "[ERROR  ]";
    break;
  case Utils::LEVEL_FATAL:
    str_log_level_ = "[FATAL  ]";
    break;
  default:
    str_log_level_ = "[INFO   ]";
    break;
  }
}

} // namespace tinylog
