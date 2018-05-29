#pragma once

#include <memory>
#include "Buffer.h"
#include "Utils.h"

namespace tinylog {

class LogStream
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:

  explicit LogStream(FILE *file, pthread_mutex_t &mutex, pthread_cond_t &cond, bool &already_swap);
  ~LogStream();

  intptr_t Write(const void *data, intptr_t ToWrite);

  void SwapBuffer();
  void WriteBuffer();
#if 0
  void SetPrefix(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);
#endif // #if 0
  LogStream &operator<<(const char *pt_log);
  template<typename StringType>
  LogStream &operator<<(const StringType &ref_log)
  {
    return this->operator<<(ref_log.c_str());
  }

  void UpdateBaseTime();

private:
  LogStream(const LogStream &);
  LogStream &operator=(const LogStream &);

  intptr_t InternalWrite(const void *data, intptr_t ToWrite);

  std::unique_ptr<Buffer> pt_front_buff_;
  std::unique_ptr<Buffer> pt_back_buff_;
  FILE *file_; // TODO: gsl::not_null
  int i_line_;
  const char *pt_func_;
#if 0
  std::string str_log_level_;
#endif // #if 0
  struct timeval tv_base_;
  struct tm *pt_tm_base_;
  pthread_mutex_t &mutex_;
  pthread_cond_t &cond_;
  bool &already_swap_;
};

#if 0
inline
void LogStream::SetPrefix(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level)
{
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
#endif // #if 0

} // namespace tinylog
