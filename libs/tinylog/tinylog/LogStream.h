#pragma once

#include <cstdlib>
#include <memory>
#include <WinSock2.h>
#include "platform_win32.h"
#include "Buffer.h"
#include "Utils.h"
#include "LockFreeQueue.h"

namespace tinylog {

class LogStream
{
  CUSTOM_MEM_ALLOCATION_IMPL
  friend class TinyLog;
public:
  LogStream() = delete;
  explicit LogStream(FILE *file, pthread_mutex_t &mutex, pthread_cond_t &cond, bool &already_swap);
  ~LogStream();

  int64_t Write(const char * data, int64_t ToWrite);

  void SwapBuffer();
  void WriteBuffer();
  void SetFile(FILE *file);
  void SetPrefix(const char *file_name, int32_t line, const char *func_name, Utils::LogLevel log_level);
  LogStream &operator<<(const std::string &log_data);
  LogStream &operator<<(const char *log_data);
  template<typename StringType>
  LogStream &operator<<(const StringType &log_data)
  {
    return this->operator<<(log_data.c_str());
  }

  void UpdateBaseTime();

private:
  LogStream(const LogStream &);
  LogStream &operator=(const LogStream &);

  int64_t InternalWrite(const char * log_data, int64_t ToWrite);

  std::unique_ptr<Buffer> front_buff_;
  std::unique_ptr<Buffer> back_buff_;
//  std::unique_ptr<LockFreeQueue> queue_;
  FILE *file_{nullptr}; // TODO: use gsl::not_null
  const char *file_name_{nullptr}; // TODO: use gsl::not_null
  int line_{0};
  const char *func_name_{nullptr};
  std::string str_log_level_;
  timeval tv_base_{};
  struct tm *tm_base_{nullptr};
  pthread_mutex_t &mutex_;
  pthread_cond_t &cond_;
  bool &already_swap_;
};

} // namespace tinylog
