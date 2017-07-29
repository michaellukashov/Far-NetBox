#include "TinyLog.h"
#include "Config.h"

namespace tinylog {

//struct thread_cond_t
//{
//  HANDLE semaphore;
//  CRITICAL_SECTION csection;
//  unsigned long num_waiting;
//  unsigned long num_wake;
//  unsigned long generation;
//};

pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
bool g_already_swap = false;

static DWORD WINAPI ThreadFunc(void *pt_arg)
{
  TinyLog *pt_tinylog = static_cast<TinyLog *>(pt_arg);
  pt_tinylog->MainLoop();

  return 0;
}

TinyLog::TinyLog()
{
  pthread_mutex_init(&g_mutex, nullptr);
  pthread_cond_init(&g_cond, nullptr);
  pt_logstream_ = new LogStream();
  e_log_level_ = Utils::LEVEL_INFO;
  b_run_ = true;
  void *Parameter = this;
  DWORD ThreadId;

  thrd_ = ::CreateThread(nullptr,
    0,
    static_cast<LPTHREAD_START_ROUTINE>(&ThreadFunc),
    Parameter,
    CREATE_SUSPENDED, &ThreadId);

}

TinyLog::~TinyLog()
{
  b_run_ = false;
  pthread_join(thrd_, nullptr);
  delete pt_logstream_;
  pthread_cond_destroy(&g_cond);
  pthread_mutex_destroy(&g_mutex);
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

int32_t TinyLog::MainLoop()
{
  struct timespec st_time_out;

  while (b_run_)
  {
    st_time_out.tv_sec = pt_logstream_->tv_base_.tv_sec + TIME_OUT_SECOND;
    st_time_out.tv_nsec = pt_logstream_->tv_base_.tv_usec * 1000;

    pthread_mutex_lock(&g_mutex);

    while (!g_already_swap)
    {
      if (pthread_cond_timedwait(&g_cond, &g_mutex, &st_time_out) == ETIMEDOUT)
      {
        pt_logstream_->SwapBuffer();
        pt_logstream_->UpdateBaseTime();
        break;
      }
    }

    if (g_already_swap)
    {
      g_already_swap = false;
    }

    pt_logstream_->WriteBuffer();

    pthread_mutex_unlock(&g_mutex);
  }

  return 0;
}

} // namespace tinylog
