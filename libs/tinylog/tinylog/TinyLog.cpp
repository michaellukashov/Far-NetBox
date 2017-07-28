//
// Created by ouyangliduo on 2016/12/13.
//

//#include <time.h>
#include <assert.h>
#include <apr_portable.h>
#include <apr_arch_thread_cond.h>
#include <apr_arch_thread_mutex.h>
//#include <apr_thread_cond.h>

#include "TinyLog.h"
#include "Config.h"

namespace tinylog {

apr_thread_mutex_t g_mutex;
apr_thread_cond_t g_cond;
bool g_already_swap = false;

static void * APR_THREAD_FUNC ThreadFunc(apr_thread_t *, void *pt_arg)
{
  TinyLog *pt_tinylog = (TinyLog *)pt_arg;
  pt_tinylog->MainLoop();

  return NULL;
}

TinyLog::TinyLog()
{
  pt_logstream_ = new LogStream();
  e_log_level_ = Utils::LL_INFO;
  b_run_ = true;
//  pthread_create(&tid_, NULL, ThreadFunc, this);
//  apr_thread_t **new_thread = NULL;
  apr_threadattr_t *attr = NULL;
//  apr_thread_start_t func;
  void *data = NULL;
  apr_pool_t *cont = NULL;
  if (apr_thread_create(&thrd_, attr, ThreadFunc, data, cont) != APR_SUCCESS)
  {
    assert(false);
  }
}

TinyLog::~TinyLog()
{
  b_run_ = false;
  apr_status_t retval;
  apr_thread_join(&retval, NULL);
  delete pt_logstream_;
}

void TinyLog::SetLogLevel(Utils::LogLevel e_log_level)
{
  e_log_level_ = e_log_level;
}

Utils::LogLevel TinyLog::GetLogLevel() const
{
  return e_log_level_;
}

LogStream& TinyLog::GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level)
{
  pt_logstream_->SetPrefix(pt_file, i_line, pt_func, e_log_level);
  return *pt_logstream_;
}

int32_t TinyLog::MainLoop()
{
  apr_interval_time_t st_time_out;

  while (b_run_)
  {
    st_time_out = apr_time_from_sec(pt_logstream_->tv_base_.tm_sec + TIME_OUT_SECOND);

    apr_thread_mutex_lock(&g_mutex);

    while (!g_already_swap)
    {
      if (apr_thread_cond_timedwait(&g_cond, &g_mutex, st_time_out) == ETIMEDOUT)
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

    apr_thread_mutex_unlock(&g_mutex);
  }

  return 0;
}

} // namespace tinylog
