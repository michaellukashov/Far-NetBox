#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <WinSock2.h>
#include "platform_win32.h"
#include "Buffer.h"
#include "Utils.h"

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
  bool EmergencyFlush();
  void SetFile(FILE * file);
  void SetPrefix(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level);
  size_t GetDroppedCount() const;
  LogStream & operator <<(const char * log_data);
  LogStream & operator <<(const std::string & log_data)
  {
    FormattedWrite(log_data.c_str(), log_data.size());
    return *this;
  }

  void UpdateBaseTime();
  static void CleanupTls();
  static bool WasCleanupTlsCalled();

private:
  LogStream(const LogStream &) = delete;
  LogStream & operator =(const LogStream &) = delete;

  size_t InternalWrite(const char * log_data, size_t ToWrite);
  size_t FormattedWrite(const char * log_data, size_t ToWrite);

  Buffer * front_buff_{nullptr};
  Buffer * back_buff_{nullptr};
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
  // Thread-local staging buffer for reduced mutex contention.
  // Dynamically resolves FlsAlloc (Vista+) via GetProcAddress;
  // falls back to TlsAlloc on XP.  Avoids C++11 thread_local which
  // is unreliable in DLLs loaded via LoadLibrary.
  struct TlsLogBuffer
  {
    std::array<char, 4096> buffer{};
    size_t used{0};
  };
  friend VOID WINAPI TlsBufferCleanup(PVOID data);
  static DWORD TlsIndex_;
  static TlsLogBuffer & EnsureTls();
};

} // namespace tinylog
