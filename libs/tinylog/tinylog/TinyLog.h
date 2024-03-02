#pragma once

#ifdef TINYLOG_EXPORT
#define TINYLOG_API __declspec(dllexport)
#elif defined(TINYLOG_IMPORT)
#define TINYLOG_API __declspec(dllimport)
#endif
#ifndef TINYLOG_API
#define TINYLOG_API
#endif

#include <fmt/format.h>
#include <fmt/printf.h>
// #include <StackWalker/StackWalker.h>
#include <icecream-cpp/icecream.hpp>

#include "Utils.h"
#include "LogStream.h"

namespace tinylog {

class TinyLogImpl;

class TinyLog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TinyLog() noexcept;
  // explicit TinyLog(FILE * file) noexcept;
  ~TinyLog() noexcept;

  static auto instance() -> TinyLog &
  {
    static TinyLog instance_;
    return instance_;
  }

  void level(Utils::LogLevel log_level);
  void file(FILE * file) noexcept;
  tinylog::Utils::LogLevel GetLogLevel() const;
  LogStream & GetLogStream(const char * file_name, int32_t line_num, const char * func_name, tinylog::Utils::LogLevel log_level);

  size_t Write(const char * data, size_t ToWrite);
  void Close();

  // TODO: group / groupEnd

private:
  std::unique_ptr<TinyLogImpl> impl_;

private:
  TinyLog(TinyLog const &) = delete;
  void operator =(TinyLog const &) = delete;
};

#ifndef NDEBUG

class TraceLogger
{
public:
  explicit TraceLogger(const char * fileName, const char * funcName, int32_t lineNumber);
  ~TraceLogger();

private:
  const char * fileName_{nullptr};
  const char * funcName_{nullptr};
  const int32_t lineNumber_{0};
  static std::string indent_;
};

#endif //ifndef NDEBUG

#define SHORT_FILE_NAME \
    [](){ \
        constexpr const char * sfn{tinylog::past_last_slash(__FILE__)}; \
        return sfn; \
    }()
#define SHORT_FILE_NAME_AND_LINE \
    [](){ \
        constexpr const char * sfn{tinylog::past_last_slash(__FILE__)}; \
        return tinylog::repr("%s:%d", sfn, __LINE__); \
    }()
#define FUNC_NAME __func__ // __FUNCTION__ // __PRETTY_FUNCTION__
#define LOCATION_STR tinylog::repr("[%s] %s", SHORT_FILE_NAME_AND_LINE, FUNC_NAME)
#define LOCATION_STR_SHORT tinylog::repr("%s", SHORT_FILE_NAME_AND_LINE)

#ifndef NDEBUG
#define TINYLOG_TRACE_ENTER() tinylog::TraceLogger traceLogger(__FILE__, __func__, __LINE__); (void)traceLogger;
#else
#define TINYLOG_TRACE_ENTER()
#endif //ifndef NDEBUG
/*
class StackWalker : public sw::StackWalker
{
public:
  explicit StackWalker(int32_t options = StackWalker::RetrieveSymbol|StackWalker::RetrieveLine);
protected:
  virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr) override;
  virtual std::string OnFormatEntry(CallstackEntry& entry) override;
  virtual void OnOutput(LPCSTR szText) override;
  virtual void OnEmptyOutput(LPCSTR szText) override;
  virtual void OnInfoOutput(LPCSTR szText) override;
  virtual void OnErrorOutput(LPCSTR szText) override;
};
*/
} // namespace tinylog

#define g_tinylog (tinylog::TinyLog::instance())

#define TINYLOG_TRACE(logger) if (logger.GetLogLevel() <= tinylog::Utils::LEVEL_TRACE) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, tinylog::Utils::LEVEL_TRACE)

#define TINYLOG_DEBUG(logger) if (logger.GetLogLevel() <= tinylog::Utils::LEVEL_DEBUG) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, tinylog::Utils::LEVEL_DEBUG)

#define TINYLOG_INFO(logger) if (logger.GetLogLevel() <= tinylog::Utils::LEVEL_INFO) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, tinylog::Utils::LEVEL_INFO)

#define TINYLOG_WARNING(logger) if (logger.GetLogLevel() <= tinylog::Utils::LEVEL_WARNING) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, tinylog::Utils::LEVEL_WARNING)

#define TINYLOG_ERROR(logger) if (logger.GetLogLevel() <= tinylog::Utils::LEVEL_ERROR) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, tinylog::Utils::LEVEL_ERROR)

#define TINYLOG_FATAL(logger) if (logger.GetLogLevel() <= tinylog::Utils::LEVEL_FATAL) \
  logger.GetLogStream(__FILE__, __LINE__, __func__, tinylog::Utils::LEVEL_FATAL)

template<typename... Args>
static inline std::string repr(const char * fmt, Args &&... args)
{
  return fmt::sprintf(fmt, std::forward<Args>(args)...);
}

template<class T>
static inline std::string to_str(const T & t)
{
    std::ostringstream out;
    out << t;
    return out.str();
}
