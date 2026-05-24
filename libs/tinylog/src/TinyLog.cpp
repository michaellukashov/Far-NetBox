#include <Global.h>
#include <cassert>
#include <tinylog/platform_win32.h>

#include <mutex>
#include <vector>
#include <algorithm>

#include <tinylog/TinyLog.h>
#include <tinylog/LogStream.h>
#include <tinylog/Config.h>
namespace tinylog {

class TinyLogImpl
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TinyLogImpl() = delete;
  explicit TinyLogImpl(FILE * file) noexcept;
  ~TinyLogImpl() noexcept;

  void SetFile(FILE * file) noexcept;
  void SetLogLevel(Utils::LogLevel log_level);
  Utils::LogLevel GetLogLevel() const;
  LogStream & GetLogStream(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level);

  size_t Write(const char * data, size_t ToWrite);
  void Close();
  bool EmergencyFlush(uint32_t TimeoutMs);

private:
  TinyLogImpl(TinyLogImpl const &) = delete;
  void operator =(TinyLogImpl const &) = delete;

private:
  void SetPrefix(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level);
  static DWORD WINAPI ThreadFunc(void *pt_arg);
  int32_t MainLoop();

  LogStream * logstream_{nullptr};
  Utils::LogLevel log_level_{Utils::LEVEL_INFO};

  pthread_t thrd_{INVALID_HANDLE_VALUE};
  DWORD ThreadId_{0};
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  std::atomic<bool> run_{false};
  std::atomic<bool> drain_buffer_{false};
};

TinyLogImpl::TinyLogImpl(FILE * file) noexcept :
  log_level_(Utils::LEVEL_INFO),
  thrd_(INVALID_HANDLE_VALUE),
  ThreadId_(0),
  run_(true),
  drain_buffer_(false)
{
  pthread_mutex_init(&mutex_, nullptr);
  pthread_cond_init(&cond_, nullptr);
  void * ls_mem = nb::chcalloc(sizeof(LogStream));
  if (!ls_mem)
  {
    OutputDebugStringA("tinylog: FATAL: nb::chcalloc failed for LogStream\n");
    std::abort();
  }
  logstream_ = new(ls_mem) LogStream(file, mutex_, cond_, drain_buffer_);
  void * Parameter = this;

  thrd_ = ::CreateThread(nullptr,
    0,
    static_cast<LPTHREAD_START_ROUTINE>(&TinyLogImpl::ThreadFunc),
    Parameter,
    0, &ThreadId_);
}

TinyLogImpl::~TinyLogImpl() noexcept
{
  if (logstream_)
    logstream_->WriteBuffer();
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

size_t TinyLogImpl::Write(const char * data, size_t ToWrite)
{
  return logstream_->Write(data, ToWrite);
}

void TinyLogImpl::Close()
{
  if (run_.load(std::memory_order_relaxed))
  {
    run_.store(false, std::memory_order_release);
    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
    pthread_join(thrd_, nullptr);
    if (logstream_)
    {
      logstream_->~LogStream();
      nb_free(logstream_);
      logstream_ = nullptr;
    }
  }
}

bool TinyLogImpl::EmergencyFlush(uint32_t TimeoutMs)
{
  bool flushed = false;
  if (logstream_)
  {
    flushed = logstream_->EmergencyFlush();
  }

  if (flushed)
  {
    run_.store(false, std::memory_order_release);
    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);

    DWORD wait_result = WaitForSingleObject(thrd_, TimeoutMs);
    if (wait_result == WAIT_TIMEOUT)
    {
      OutputDebugStringA("tinylog: EmergencyFlush worker thread timeout\n");
    }
  }

  char msg[256];
  _snprintf_s(msg, sizeof(msg), _TRUNCATE,
    "tinylog: TinyLogImpl::EmergencyFlush result=%d\n", flushed ? 1 : 0);
  OutputDebugStringA(msg);

  return flushed;
}

DWORD WINAPI TinyLogImpl::ThreadFunc(void * pt_arg)
{
  TinyLogImpl * pt_tinylog = static_cast<TinyLogImpl *>(pt_arg);
  pt_tinylog->MainLoop();

  return 0;
}

int32_t TinyLogImpl::MainLoop()
{
  int result;
  const DWORD timeout_millisecs = 1000 * TIME_OUT_SECOND;

  pthread_mutex_lock(&mutex_);
  while (run_.load(std::memory_order_acquire))
  {
    while (run_.load(std::memory_order_acquire) && !drain_buffer_.load(std::memory_order_acquire))
    {
      result = pthread_cond_timedwait(&cond_, &mutex_, timeout_millisecs);
      if (result == WAIT_TIMEOUT)
      {
        break;
      }
    }

    logstream_->WriteBuffer();

    if (drain_buffer_.load(std::memory_order_acquire))
    {
      logstream_->SwapBuffer();
      logstream_->UpdateBaseTime();
      drain_buffer_.store(false, std::memory_order_release);
    }
  }
  pthread_mutex_unlock(&mutex_);

  return 0;
}

bool TinyLog::destroyed_ = false;

static std::mutex g_registry_mutex;
static std::vector<TinyLog *> g_registry;

void TinyLog::Register(TinyLog * logger)
{
  std::lock_guard<std::mutex> lock(g_registry_mutex);
  g_registry.push_back(logger);
  char msg[256];
  _snprintf_s(msg, sizeof(msg), _TRUNCATE, "tinylog: Registered instance (total: %zu)\n", g_registry.size());
  OutputDebugStringA(msg);
}

void TinyLog::Unregister(TinyLog * logger)
{
  std::lock_guard<std::mutex> lock(g_registry_mutex);
  auto it = std::find(g_registry.begin(), g_registry.end(), logger);
  if (it != g_registry.end())
  {
    g_registry.erase(it);
  }
  char msg[256];
  _snprintf_s(msg, sizeof(msg), _TRUNCATE, "tinylog: Unregistered instance (total: %zu)\n", g_registry.size());
  OutputDebugStringA(msg);
}

bool TinyLog::EmergencyFlushAll(uint32_t TimeoutMs)
{
  std::lock_guard<std::mutex> lock(g_registry_mutex);
  uint32_t remaining = TimeoutMs;
  bool all_ok = true;
  char msg[256];
  _snprintf_s(msg, sizeof(msg), _TRUNCATE,
    "tinylog: EmergencyFlushAll starting (%zu instances, timeout: %u ms)\n",
    g_registry.size(), TimeoutMs);
  OutputDebugStringA(msg);
  for (TinyLog * logger : g_registry)
  {
    if (remaining == 0)
    {
      all_ok = false;
      break;
    }
    uint32_t per_instance = 50;
    if (per_instance > remaining)
      per_instance = remaining;
    try
    {
      if (!logger->EmergencyFlush(per_instance))
        all_ok = false;
    }
    catch (...)
    {
      OutputDebugStringA("tinylog: EmergencyFlushAll: instance flush failed\n");
      all_ok = false;
    }
    remaining -= per_instance;
  }
  _snprintf_s(msg, sizeof(msg), _TRUNCATE,
    "tinylog: EmergencyFlushAll complete (ok=%d)\n", all_ok ? 1 : 0);
  OutputDebugStringA(msg);
  return all_ok;
}

TinyLog::TinyLog() noexcept
{
  void * impl_mem = nb::chcalloc(sizeof(TinyLogImpl));
  if (!impl_mem)
  {
    OutputDebugStringA("tinylog: FATAL: nb::chcalloc failed for TinyLogImpl\n");
    std::abort();
  }
  impl_ = new(impl_mem) TinyLogImpl(nullptr);
  Register(this);
}

TinyLog::~TinyLog() noexcept
{
  Unregister(this);
  if (impl_)
  {
    impl_->~TinyLogImpl();
    nb_free(impl_);
    impl_ = nullptr;
  }
  destroyed_ = true;
}

auto TinyLog::instance() -> TinyLog * &
{
  static TinyLog * s_instance = nullptr;
  static std::once_flag s_once;
  static TinyLog * s_null = nullptr;

  if (destroyed_)
    return s_null;

  std::call_once(s_once, [&]() {
    void * mem = nb::chcalloc(sizeof(TinyLog));
    if (!mem)
    {
      OutputDebugStringA("tinylog: FATAL: nb::chcalloc failed for TinyLog singleton\n");
      std::abort();
    }
    s_instance = new(mem) TinyLog();
  });

  return s_instance;
}

void TinyLog::DestroyInstance() noexcept
{
  TinyLog * & inst = TinyLog::instance();
  if (inst)
  {
    inst->~TinyLog();
    nb_free(inst);
    inst = nullptr;
    LogStream::CleanupTls();
  }
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

LogStream & TinyLog::GetLogStream(const char * file_name, int32_t line_num, const char * func_name, Utils::LogLevel log_level)
{
  //impl_->SetPrefix(file_name, line_num, func_name, log_level);
  return impl_->GetLogStream(file_name, line_num, func_name, log_level);
}

size_t TinyLog::Write(const char * data, size_t ToWrite)
{
  return impl_->Write(data, ToWrite);
}

void TinyLog::Close()
{
  impl_->Close();
}

bool TinyLog::EmergencyFlush(uint32_t TimeoutMs)
{
  return impl_->EmergencyFlush(TimeoutMs);
}

#if defined(_DEBUG)

std::string TraceLogger::indent_;

TraceLogger::TraceLogger(const char * fileName, const char * funcName, int32_t lineNumber) :
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

#endif //#if defined(_DEBUG)
/*
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
  // OutputDebugStringA(szText);
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
*/
} // namespace tinylog
