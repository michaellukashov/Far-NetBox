#include <tinylog/platform_win32.h>

#include <tinylog/TinyLog.h>
#include <tinylog/LogStream.h>
#include <tinylog/Config.h>

namespace tinylog {

/*std::string Format(const std::string& fmt, fmt::ArgList args)
{
  fmt::MemoryWriter w;
  w.write(fmt.data(), args);
  return std::string(w.data(), w.size());
}
FMT_VARIADIC_W(const std::string, Format, const std::string&)*/

class TinyLogImpl
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TinyLogImpl() = delete;
  explicit TinyLogImpl(FILE * file) noexcept;
  ~TinyLogImpl() noexcept;

  void SetFile(FILE *file) noexcept;
  void SetLogLevel(Utils::LogLevel log_level);
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
  Utils::LogLevel log_level_{Utils::LEVEL_INFO};

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

void TinyLogImpl::SetFile(FILE * file) noexcept
{
  logstream_->SetFile(file);
}

void TinyLogImpl::SetLogLevel(Utils::LogLevel log_level)
{
  log_level_ = log_level;
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

TinyLog::TinyLog() noexcept :
  impl_(std::make_unique<TinyLogImpl>(nullptr))
{
}

/*TinyLog::TinyLog(FILE *file) noexcept :
  impl_(std::make_unique<TinyLogImpl>(file))
{
  assert(file != nullptr);
}*/

TinyLog::~TinyLog() noexcept
{
}

void TinyLog::level(Utils::LogLevel log_level)
{
  impl_->SetLogLevel(log_level);
}

void TinyLog::file(FILE * file) noexcept
{
  impl_->SetFile(file);
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
  fileName_(tinylog::past_last_slash(fileName)),
  funcName_(funcName),
  lineNumber_(lineNumber)
{
  TINYLOG_TRACE(g_tinylog) << repr("%s [%10s:%3d] Entering %s()", indent_, fileName_, lineNumber_, funcName_);
//  OutputDebugStringA(repr("%s [%s:%d] Entering %s()", indent_, fileName_, lineNumber_, funcName_).c_str());
  indent_.append("  ");
}

TraceLogger::~TraceLogger()
{
  indent_.resize(indent_.length() - 2);
  TINYLOG_TRACE(g_tinylog) << repr("%s [%10s    ] Leaving %s()", indent_, fileName_, funcName_);
  //  OutputDebugStringA(repr("%s [%s] Leaving %s()", indent_, fileName_, funcName_).c_str());
}

StackWalker::StackWalker(int options) : sw::StackWalker(options)
{
  m_MaxRecursionCount = 10;
}

void StackWalker::OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
{
}

std::string StackWalker::OnFormatEntry(CallstackEntry & entry)
{
  return repr("[%s:%d] %s", past_last_slash(entry.lineFileName), entry.lineNumber, entry.name);
}

void StackWalker::OnOutput(LPCSTR szText)
{
  TINYLOG_TRACE(g_tinylog) << szText;
  OutputDebugStringA(szText);
}

void StackWalker::OnEmptyOutput(LPCSTR szText)
{
}

void StackWalker::OnInfoOutput(LPCSTR szText)
{
}

void StackWalker::OnErrorOutput(LPCSTR szText)
{
}

} // namespace tinylog
