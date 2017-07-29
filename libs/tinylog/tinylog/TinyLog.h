#pragma once

#include "LogStream.h"
#include "Utils.h"

namespace tinylog {

class TinyLog
{
public:
  static TinyLog &GetInstance()
  {
    static TinyLog instance;
    return instance;
  }

  TinyLog();
  ~TinyLog();

  int32_t MainLoop();
  void SetLogLevel(Utils::LogLevel e_log_level);
  Utils::LogLevel GetLogLevel() const;
  LogStream &GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);

private:
  TinyLog(TinyLog const &);
  void operator=(TinyLog const &);

  LogStream *pt_logstream_;
  pthread_t thrd_;
  DWORD ThreadId_;
  Utils::LogLevel e_log_level_;
  bool b_run_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  bool already_swap_;
};

} // namespace tinylog

#define g_tinylog (tinylog::TinyLog::GetInstance())

#define LOG_TRACE if (g_tinylog.GetLogLevel() <= Utils::LEVEL_TRACE)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_TRACE)

#define LOG_DEBUG if (g_tinylog.GetLogLevel() <= Utils::LEVEL_DEBUG)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_DEBUG)

#define LOG_INFO if (g_tinylog.GetLogLevel() <= Utils::LEVEL_INFO)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_INFO)

#define LOG_WARNING if (g_tinylog.GetLogLevel() <= Utils::LEVEL_WARNING) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_WARNING)

#define LOG_ERROR if (g_tinylog.GetLogLevel() <= Utils::LEVEL_ERROR) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_ERROR)

#define LOG_FATAL if (g_tinylog.GetLogLevel() <= Utils::LEVEL_FATAL) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_FATAL)
