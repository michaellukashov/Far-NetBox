#pragma once

#include "LogStream.h"
#include "Utils.h"

namespace tinylog {

class TinyLog
{
public:
//  static TinyLog &GetInstance()
//  {
//    static TinyLog instance;
//    return instance;
//  }

//  TinyLog();
  explicit TinyLog(FILE * file);
  ~TinyLog();

  void SetLogLevel(Utils::LogLevel e_log_level);
  Utils::LogLevel GetLogLevel() const;
  LogStream &GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);

  intptr_t Write(const void *data, intptr_t ToWrite);
  void Close();

private:
  static DWORD WINAPI ThreadFunc(void *pt_arg);
  int32_t MainLoop();

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

//#define g_tinylog (tinylog::TinyLog::GetInstance())

#define TINYLOG_TRACE(logger) if (logger.GetLogLevel() <= Utils::LEVEL_TRACE)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_TRACE)

#define TINYLOG_DEBUG(logger) if (logger.GetLogLevel() <= Utils::LEVEL_DEBUG)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_DEBUG)

#define TINYLOG_INFO(logger) if (logger.GetLogLevel() <= Utils::LEVEL_INFO)\
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_INFO)

#define TINYLOG_WARNING(logger) if (logger.GetLogLevel() <= Utils::LEVEL_WARNING) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_WARNING)

#define TINYLOG_ERROR(logger) if (logger.GetLogLevel() <= Utils::LEVEL_ERROR) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_ERROR)

#define TINYLOG_FATAL(logger) if (logger.GetLogLevel() <= Utils::LEVEL_FATAL) \
  g_tinylog.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_FATAL)
