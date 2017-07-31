#pragma once

#ifdef TINYLOG_EXPORT
#define TINYLOG_API __declspec(dllexport)
#elif defined(TINYLOG_IMPORT)
#define TINYLOG_API __declspec(dllimport)
#endif
#ifndef TINYLOG_API
#define TINYLOG_API
#endif

#include "Utils.h"

namespace tinylog {

class TinyLogImpl;

class TINYLOG_API TinyLog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TinyLog(FILE * file);
  ~TinyLog();

  void SetLogLevel(Utils::LogLevel e_log_level);
  Utils::LogLevel GetLogLevel() const;
#if 0
  LogStream &GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level);
#endif // #if 0

  intptr_t Write(const void *data, intptr_t ToWrite);
  void Close();

private:
  TinyLogImpl * impl_;

private:
  TinyLog(TinyLog const &);
  void operator=(TinyLog const &);
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
