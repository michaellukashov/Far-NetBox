#include <tinylog/platform_win32.h>

#include <tinylog/TinyLog.h>
#include <tinylog/LogStream.h>
#include <tinylog/Config.h>
#include <fmt/format.h>
#include <fmt/printf.h>

namespace tinylog {

std::string Format(const std::string& fmt, fmt::ArgList args)
{
  fmt::MemoryWriter w;
  w.write(fmt.data(), args);
  return std::string(w.data(), w.size());
}
FMT_VARIADIC_W(const std::string, Format, const std::string&)

class TinyLogImpl
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TinyLogImpl() = delete;
  explicit TinyLogImpl(FILE * file) noexcept;
  ~TinyLogImpl() noexcept;

  void SetLogLevel(Utils::LogLevel e_log_level);
  Utils::LogLevel GetLogLevel() const;
  LogStream &GetLogStream(const char *file_name, int32_t line, const char *func_name, Utils::LogLevel log_level);

  int64_t Write(const char *data, int64_t ToWrite);
  void Close();

private:
  TinyLogImpl(TinyLogImpl const &) = delete;
  void operator=(TinyLogImpl const &) = delete;

private:
  void SetPrefix(const char *file_name, int32_t line, const char *func_name, Utils::LogLevel log_level);
  static DWORD WINAPI ThreadFunc(void *pt_arg);
  int32_t MainLoop();

  std::unique_ptr<LogStream> logstream_;
  Utils::LogLevel log_level_{};

  pthread_t thrd_{INVALID_HANDLE_VALUE};
  DWORD ThreadId_{0};
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  bool run_{false};
  bool already_swap_{false};
};

TinyLogImpl::TinyLogImpl(FILE *file) noexcept :
  log_level_(Utils::LEVEL_INFO),
  thrd_(INVALID_HANDLE_VALUE),
  ThreadId_(0),
  run_(true),
  already_swap_(false)
{
  pthread_mutex_init(&mutex_, nullptr);
  pthread_cond_init(&cond_, nullptr);
  logstream_ = std::make_unique<LogStream>(file, mutex_, cond_, already_swap_);
  void *Parameter = this;

  thrd_ = ::CreateThread(nullptr,
      0,
      static_cast<LPTHREAD_START_ROUTINE>(&TinyLogImpl::ThreadFunc),
      Parameter,
  0, &ThreadId_);
}

TinyLogImpl::~TinyLogImpl() noexcept
{
  Close();
  pthread_cond_destroy(&cond_);
  pthread_mutex_destroy(&mutex_);
}

void TinyLogImpl::SetLogLevel(Utils::LogLevel e_log_level)
{
  log_level_ = e_log_level;
}

Utils::LogLevel TinyLogImpl::GetLogLevel() const
{
  return log_level_;
}

void TinyLogImpl::SetPrefix(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level)
{
  logstream_->SetPrefix(file_name, line, func_name, log_level);
}

LogStream & TinyLogImpl::GetLogStream(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level)
{
  SetPrefix(file_name, line, func_name, log_level);
  return *logstream_;
}

int64_t TinyLogImpl::Write(const char *data, int64_t ToWrite)
{
  return logstream_->Write(data, ToWrite);
}

void TinyLogImpl::Close()
{
  if (run_)
  {
    run_ = false;
    pthread_join(thrd_, nullptr);
    logstream_.reset();
  }
}

DWORD WINAPI TinyLogImpl::ThreadFunc(void *pt_arg)
{
  TinyLogImpl *pt_tinylog = static_cast<TinyLogImpl *>(pt_arg);
  pt_tinylog->MainLoop();

  return 0;
}

int32_t TinyLogImpl::MainLoop()
{
  while (run_)
  {
    DWORD timeout_millisecs = 1000 * TIME_OUT_SECOND;

    pthread_mutex_lock(&mutex_);

    while (!already_swap_)
    {
      if (pthread_cond_timedwait(&cond_, &mutex_, timeout_millisecs) == WAIT_TIMEOUT)
      {
        logstream_->SwapBuffer();
        logstream_->UpdateBaseTime();
        break;
      }
    }

    if (already_swap_)
    {
      already_swap_ = false;
    }

    logstream_->WriteBuffer();

    pthread_mutex_unlock(&mutex_);
  }

  return 0;
}

TinyLog::TinyLog() :
  impl_(std::make_unique<TinyLogImpl>(nullptr))
{
}

TinyLog::TinyLog(FILE *file) noexcept :
  impl_(std::make_unique<TinyLogImpl>(file))
{
  assert(file != nullptr);
}

TinyLog::~TinyLog() noexcept
{
}

void TinyLog::SetLogLevel(Utils::LogLevel e_log_level)
{
  impl_->SetLogLevel(e_log_level);
}

Utils::LogLevel TinyLog::GetLogLevel() const
{
  return impl_->GetLogLevel();
}

LogStream &TinyLog::GetLogStream(const char *file_name, int32_t line_num, const char *func_name, Utils::LogLevel log_level)
{
  //impl_->SetPrefix(file_name, line_num, func_name, log_level);
  return impl_->GetLogStream(file_name, line_num, func_name, log_level);
}

int64_t TinyLog::Write(const char *data, int64_t ToWrite)
{
  return impl_->Write(data, ToWrite);
}

void TinyLog::Close()
{
  impl_->Close();
}


std::string TraceLogger::indent_;

TraceLogger::TraceLogger(const char* fileName, const char* funcName, int32_t lineNumber) :
  fileName_(fileName),
  funcName_(funcName),
  lineNumber_(lineNumber)
{
  TINYLOG_TRACE(g_tinylog) << Format("%sEntering %s() - (%s:%d)", indent_, funcName_, fileName_, lineNumber_);
  indent_.append("  ");
}

TraceLogger::~TraceLogger()
{
  indent_.resize(indent_.length() - 2);
  TINYLOG_TRACE(g_tinylog) << Format("%sLeaving %s() - (%s)", indent_, funcName_, fileName_);
}

} // namespace tinylog
