#pragma once

//#include <pthread.h>
#include <apr_portable.h>

#include "LogStream.h"
#include "Utils.h"

namespace tinylog {

class TinyLog
{
public:
  static TinyLog& GetInstance()
  {
    static TinyLog instance;
    return instance;
  }

  TinyLog();

  ~TinyLog();

  int32_t MainLoop();

  void SetLogLevel(Utils::LogLevel e_log_level);

  Utils::LogLevel GetLogLevel() const;

  LogStream& GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);

private:
  TinyLog(TinyLog const &);
  void operator=(TinyLog const &);

  LogStream *pt_logstream_;
//  apr_os_thread_t tid_;
  apr_thread_t * thrd_;
  Utils::LogLevel e_log_level_;
  bool b_run_;
};

} // namespace tinylog

#define g_tinylog (tinylog::TinyLog::GetInstance())

#define LOG_TRACE if (g_tinylog.GetLogLevel() <= Utils::TRACE)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::TRACE)

#define LOG_DEBUG if (g_tinylog.GetLogLevel() <= Utils::DEBUG)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::DEBUG)

#define LOG_INFO if (g_tinylog.GetLogLevel() <= Utils::INFO)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::INFO)

#define LOG_WARNING if (g_tinylog.GetLogLevel() <= Utils::WARNING) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::WARNING)

#define LOG_ERROR if (g_tinylog.GetLogLevel() <= Utils::ERROR) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::ERROR)

#define LOG_FATAL if (g_tinylog.GetLogLevel() <= Utils::FATAL) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::FATAL)
