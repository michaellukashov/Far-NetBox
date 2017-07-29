#include "TinyLog.h"
#include "Config.h"

namespace tinylog {

DWORD WINAPI TinyLog::ThreadFunc(void *pt_arg)
{
  TinyLog *pt_tinylog = static_cast<TinyLog *>(pt_arg);
  pt_tinylog->MainLoop();

  return 0;
}

//TinyLog::TinyLog() :
//  pt_logstream_(nullptr),
//  thrd_(INVALID_HANDLE_VALUE),
//  ThreadId_(0),
//  e_log_level_(Utils::LEVEL_INFO),
//  b_run_(true),
//  already_swap_(false)
//{
//}

TinyLog::TinyLog(FILE *file) :
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
      static_cast<LPTHREAD_START_ROUTINE>(&TinyLog::ThreadFunc),
      Parameter,
      CREATE_SUSPENDED, &ThreadId_);
}

TinyLog::~TinyLog()
{
  Close();
  pthread_cond_destroy(&cond_);
  pthread_mutex_destroy(&mutex_);
}

void TinyLog::SetLogLevel(Utils::LogLevel e_log_level)
{
  e_log_level_ = e_log_level;
}

Utils::LogLevel TinyLog::GetLogLevel() const
{
  return e_log_level_;
}

LogStream &TinyLog::GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level)
{
  pt_logstream_->SetPrefix(pt_file, i_line, pt_func, e_log_level);
  return *pt_logstream_;
}

intptr_t TinyLog::Write(const void *data, intptr_t ToWrite)
{
  return pt_logstream_->Write(data, ToWrite);
}

void TinyLog::Close()
{
  if (b_run_)
  {
    b_run_ = false;
    pthread_join(thrd_, nullptr);
    delete pt_logstream_;
    pt_logstream_ = nullptr;
  }
}

int32_t TinyLog::MainLoop()
{
  struct timespec st_time_out;

  while (b_run_)
  {
    st_time_out.tv_sec = pt_logstream_->tv_base_.tv_sec + TIME_OUT_SECOND;
    st_time_out.tv_nsec = pt_logstream_->tv_base_.tv_usec * 1000;

    pthread_mutex_lock(&mutex_);

    while (!already_swap_)
    {
      if (pthread_cond_timedwait(&cond_, &mutex_, &st_time_out) == WAIT_TIMEOUT)
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

} // namespace tinylog
