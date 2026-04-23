#pragma once

#include <atomic>
#include <chrono>
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
  explicit LogStream(FILE * file, pthread_mutex_t & mutex, pthread_cond_t & cond, std::atomic<bool> & already_swap);
  ~LogStream();

  size_t Write(const char * data, size_t ToWrite);

  void SwapBuffer();
  void WriteBuffer();
  void SetFile(FILE * file);
  void SetPrefix(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level);
  LogStream & operator <<(const std::string & log_data);
  LogStream & operator <<(const char * log_data);
  template<typename StringType>
  LogStream & operator <<(const StringType & log_data)
  {
    return this->operator <<(log_data.c_str());
  }

  void UpdateBaseTime();

private:
  LogStream(const LogStream &) = delete;
  LogStream & operator =(const LogStream &) = delete;

  size_t InternalWrite(const char * log_data, size_t ToWrite);
  size_t FormattedWrite(const char * log_data, size_t ToWrite);

  std::unique_ptr<Buffer> front_buff_;
  std::unique_ptr<Buffer> back_buff_;
//  std::unique_ptr<LockFreeQueue> queue_;
  FILE * file_{nullptr}; // TODO: use gsl::not_null
  const char * file_name_{nullptr}; // TODO: use gsl::not_null
  int32_t line_{0};
  const char * func_name_{nullptr};
  const char * str_log_level_;
  std::atomic<uint64_t> timestamp_us_{0};
  std::atomic<size_t> dropped_count_{0};  // Track dropped log entries due to overflow
  pthread_mutex_t & mutex_;
  pthread_cond_t & cond_;
  std::atomic<bool> & drain_buffer_;
  // Thread-local staging buffer for reduced mutex contention
  static thread_local std::array<char, 4096> tls_buffer_;
  static thread_local size_t tls_buffer_used_;
};

} // namespace tinylog
