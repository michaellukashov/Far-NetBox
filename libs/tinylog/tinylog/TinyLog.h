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
#include "LogStream.h"

namespace tinylog {

class TinyLogImpl;

class TinyLog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TinyLog();
  explicit TinyLog(FILE * file) noexcept;
  ~TinyLog() noexcept;

  static TinyLog& GetInstance()
  {
    static TinyLog instance;
    return instance;
  }

  void SetLogLevel(Utils::LogLevel e_log_level);
  Utils::LogLevel GetLogLevel() const;
  LogStream &GetLogStream(const char *file_name, int32_t line_num, const char *func_name, Utils::LogLevel log_level);

  int64_t Write(const char * data, int64_t ToWrite);
  void Close();

private:
  std::unique_ptr<TinyLogImpl> impl_;

private:
  TinyLog(TinyLog const &) = delete;
  void operator=(TinyLog const &) = delete;
};

#ifndef NDEBUG

class TraceLogger
{
public:
  explicit TraceLogger(const char* fileName, const char* funcName, int32_t lineNumber);
  ~TraceLogger();

private:
  const char* fileName_{nullptr};
  const char* funcName_{nullptr};
  const int32_t lineNumber_{0};
  static std::string indent_;
};

#endif //ifndef NDEBUG


#ifndef NDEBUG
#define TINYLOG_TRACE_ENTER TraceLogger traceLogger(__FILE__, __func__, __LINE__);
#else
#define TINYLOG_TRACE_ENTER
#endif //ifndef NDEBUG

} // namespace tinylog

#define g_tinylog (tinylog::TinyLog::GetInstance())

#define TINYLOG_TRACE(logger) if (logger.GetLogLevel() <= Utils::LEVEL_TRACE) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_TRACE)

#define TINYLOG_DEBUG(logger) if (logger.GetLogLevel() <= Utils::LEVEL_DEBUG) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_DEBUG)

#define TINYLOG_INFO(logger) if (logger.GetLogLevel() <= Utils::LEVEL_INFO) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_INFO)

#define TINYLOG_WARNING(logger) if (logger.GetLogLevel() <= Utils::LEVEL_WARNING) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_WARNING)

#define TINYLOG_ERROR(logger) if (logger.GetLogLevel() <= Utils::LEVEL_ERROR) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_ERROR)

#define TINYLOG_FATAL(logger) if (logger.GetLogLevel() <= Utils::LEVEL_FATAL) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, Utils::LEVEL_FATAL)

