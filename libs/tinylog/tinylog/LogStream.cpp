#include <io.h>
#include <fcntl.h>

#include "LogStream.h"
#include "Config.h"

namespace tinylog {

extern pthread_mutex_t g_mutex;
extern pthread_cond_t g_cond;
extern bool g_already_swap;

LogStream::LogStream()
{
  log_file_fd_ = _open(LOG_PATH, _O_WRONLY | _O_CREAT);

  pt_front_buff_ = new Buffer(BUFFER_SIZE);
  pt_back_buff_  = new Buffer(BUFFER_SIZE);

  Utils::CurrentTime(&tv_base_, &pt_tm_base_);
}

LogStream::~LogStream()
{
  delete pt_front_buff_;
  delete pt_back_buff_;

  _close(log_file_fd_);
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

  pthread_mutex_lock(&g_mutex);

  if (pt_front_buff_->TryAppend(pt_tm_base_, (long)tv_base_.tv_usec, pt_file_, i_line_, pt_func_, str_log_level_, pt_log) < 0)
  {
    SwapBuffer();
    g_already_swap = true;
    pt_front_buff_->TryAppend(pt_tm_base_, (long)tv_base_.tv_usec, pt_file_, i_line_, pt_func_, str_log_level_, pt_log);
  }

  pthread_cond_signal(&g_cond);
  pthread_mutex_unlock(&g_mutex);

  return *this;
}

//LogStream &LogStream::operator<<(const std::string &ref_log)
//{
//  return this->operator<<(ref_log.c_str());
//}

void LogStream::UpdateBaseTime()
{
  struct timeval tv;
  time_t now = time(nullptr);
  tv.tv_sec = (long)now;
  tv.tv_usec = 0;
  struct tm * tm = localtime(&now);

  if (tv.tv_sec != tv_base_.tv_sec)
  {
    int new_sec = pt_tm_base_->tm_sec + int(tm->tm_sec - tv_base_.tv_sec);
    if (new_sec >= 60)
    {
      pt_tm_base_->tm_sec = new_sec % 60;
      int new_min = pt_tm_base_->tm_min + new_sec / 60;
      if (new_min >= 60)
      {
        pt_tm_base_->tm_min = new_min % 60;
        int new_hour = pt_tm_base_->tm_hour + new_min / 60;
        if (new_hour >= 24)
        {
          Utils::CurrentTime(&tv, &pt_tm_base_);
        }
        else
        {
          pt_tm_base_->tm_hour = new_hour;
        }
      }
      else
      {
        pt_tm_base_->tm_min = new_min;
      }
    }
    else
    {
      pt_tm_base_->tm_sec = new_sec;
    }

    tv_base_ = tv;
  }
  else
  {
    tv_base_.tv_usec = tv.tv_usec;
  }
}

} // namespace tinylog
