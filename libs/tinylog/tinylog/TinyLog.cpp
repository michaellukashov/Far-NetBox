#include "platform_win32.h"

#include "TinyLog.h"
#include "LogStream.h"
#include "Config.h"

namespace tinylog {

class TinyLogImpl
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TinyLogImpl(FILE * file);
  ~TinyLogImpl();

  void SetLogLevel(Utils::LogLevel e_log_level);
  Utils::LogLevel GetLogLevel() const;

  intptr_t Write(const void *data, intptr_t ToWrite);
  void Close();

private:
  TinyLogImpl(TinyLogImpl const &);
  void operator=(TinyLogImpl const &);

private:
  static DWORD WINAPI ThreadFunc(void *pt_arg);
  int32_t MainLoop();

  LogStream *pt_logstream_;
  Utils::LogLevel e_log_level_;

  pthread_t thrd_;
  DWORD ThreadId_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  bool b_run_;
  bool already_swap_;
};

TinyLogImpl::TinyLogImpl(FILE *file) :
  pt_logstream_(nullptr),
  thrd_(INVALID_HANDLE_VALUE),
  ThreadId_(0),
  e_log_level_(Utils::LEVEL_INFO),
  b_run_(true),
  already_swap_(false)
{
  pthread_mutex_init(&mutex_, nullptr);
  pthread_cond_init(&cond_, nullptr);
  pt_logstream_ = new LogStream(file, mutex_, cond_, already_swap_);
  void *Parameter = this;

  thrd_ = ::CreateThread(nullptr,
      0,
      static_cast<LPTHREAD_START_ROUTINE>(&TinyLogImpl::ThreadFunc),
      Parameter,
  0, &ThreadId_);
}

TinyLogImpl::~TinyLogImpl()
{
  Close();
  pthread_cond_destroy(&cond_);
  pthread_mutex_destroy(&mutex_);
}

void TinyLogImpl::SetLogLevel(Utils::LogLevel e_log_level)
{
  e_log_level_ = e_log_level;
}

Utils::LogLevel TinyLogImpl::GetLogLevel() const
{
  return e_log_level_;
}

intptr_t TinyLogImpl::Write(const void *data, intptr_t ToWrite)
{
  return pt_logstream_->Write(data, ToWrite);
}

void TinyLogImpl::Close()
{
  if (b_run_)
  {
    b_run_ = false;
    pthread_join(thrd_, nullptr);
    delete pt_logstream_;
    pt_logstream_ = nullptr;
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
  while (b_run_)
  {
    DWORD timeout_millisecs = 1000 * TIME_OUT_SECOND;

    pthread_mutex_lock(&mutex_);

    while (!already_swap_)
    {
      if (pthread_cond_timedwait(&cond_, &mutex_, timeout_millisecs) == WAIT_TIMEOUT)
      {
        pt_logstream_->SwapBuffer();
        pt_logstream_->UpdateBaseTime();
        break;
      }
    }

    if (already_swap_)
    {
      already_swap_ = false;
    }

    pt_logstream_->WriteBuffer();

    pthread_mutex_unlock(&mutex_);
  }

  return 0;
}


TinyLog::TinyLog(FILE *file) :
  impl_(new TinyLogImpl(file))
{
}

TinyLog::~TinyLog()
{
  delete impl_;
}

void TinyLog::SetLogLevel(Utils::LogLevel e_log_level)
{
  impl_->SetLogLevel(e_log_level);
}

Utils::LogLevel TinyLog::GetLogLevel() const
{
  return impl_->GetLogLevel();
}

#if 0
LogStream &TinyLog::GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level)
{
  pt_logstream_->SetPrefix(pt_file, i_line, pt_func, e_log_level);
  return *pt_logstream_;
}
#endif // #if 0

intptr_t TinyLog::Write(const void *data, intptr_t ToWrite)
{
  return impl_->Write(data, ToWrite);
}

void TinyLog::Close()
{
  impl_->Close();
}

} // namespace tinylog
