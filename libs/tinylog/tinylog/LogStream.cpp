#include <io.h>
#include <fcntl.h>

#include <apr_arch_threadproc.h>
#include <apr_arch_thread_cond.h>

#include "LogStream.h"
#include "Config.h"

namespace tinylog {

extern apr_thread_mutex_t g_mutex;
extern apr_thread_cond_t g_cond;
extern bool g_already_swap;

LogStream::LogStream()
{
  log_file_fd_ = open(LOG_PATH, _O_WRONLY | _O_CREAT);

  pt_front_buff_ = new Buffer(BUFFER_SIZE);
  pt_back_buff_  = new Buffer(BUFFER_SIZE);

  Utils::CurrentTime(&tv_base_, &pt_tm_base_);
}

LogStream::~LogStream()
{
  delete(pt_front_buff_);
  delete(pt_back_buff_);

  close(log_file_fd_);
}

/*
 * Swap front buffer and back buffer.
 * This function should be locked.
 */
void LogStream::SwapBuffer()
{
  Buffer *pt_tmp = pt_front_buff_;
  pt_front_buff_ = pt_back_buff_;
  pt_back_buff_ = pt_tmp;
}

/*
 * Write buffer data to log file.
 * This function should be locked.
 */
void LogStream::WriteBuffer()
{
  pt_back_buff_->Flush(log_file_fd_);
  pt_back_buff_->Clear();
}

LogStream& LogStream::operator<<(const char *pt_log)
{
  UpdateBaseTime();

  apr_thread_mutex_lock(&g_mutex);

  if (pt_front_buff_->TryAppend(pt_tm_base_, (long)tv_base_.tm_usec, pt_file_, i_line_, pt_func_, str_log_level_, pt_log) < 0)
  {
    SwapBuffer();
    g_already_swap = true;
    pt_front_buff_->TryAppend(pt_tm_base_, (long)tv_base_.tm_usec, pt_file_, i_line_, pt_func_, str_log_level_, pt_log);
  }

  apr_thread_cond_signal(&g_cond);
  apr_thread_mutex_unlock(&g_mutex);

  return *this;
}

LogStream& LogStream::operator<<(const std::string &ref_log)
{
  return this->operator<<(ref_log.c_str());
}

void LogStream::UpdateBaseTime()
{
  apr_time_exp_t aprexptime;
  apr_time_t now = apr_time_now();
  apr_time_exp_lt(&aprexptime, now);

  apr_os_exp_time_get(&pt_tm_base_, &aprexptime);

  if (aprexptime.tm_sec != tv_base_.tm_sec)
  {
    int new_sec = pt_tm_base_->wSecond + int(aprexptime.tm_sec - tv_base_.tm_sec);
    if (new_sec >= 60)
    {
      pt_tm_base_->wSecond = new_sec % 60;
      int new_min = pt_tm_base_->wMinute + new_sec / 60;
      if (new_min >= 60)
      {
        pt_tm_base_->wMinute = new_min % 60;
        int new_hour = pt_tm_base_->wHour + new_min / 60;
        if (new_hour >= 24)
        {
          Utils::CurrentTime(&aprexptime, &pt_tm_base_);
        }
        else
        {
          pt_tm_base_->wHour = new_hour;
        }
      }
      else
      {
        pt_tm_base_->wMinute = new_min;
      }
    }
    else
    {
      pt_tm_base_->wSecond = new_sec;
    }

    tv_base_ = aprexptime;
  }
  else
  {
    tv_base_.tm_usec = aprexptime.tm_usec;
  }
}

} // namespace tinylog
