#include <io.h>
#include <fcntl.h>

#include <tinylog/platform_win32.h>

#include <tinylog/LogStream.h>
#include <tinylog/Config.h>
//#include <tinylog/LockFreeQueue.h>


namespace tinylog {

LogStream::LogStream(FILE *file, pthread_mutex_t &mutex, pthread_cond_t &cond, bool &already_swap) :
  front_buff_(std::make_unique<Buffer>(BUFFER_SIZE)),
  back_buff_(std::make_unique<Buffer>(BUFFER_SIZE)),
//  queue_(std::make_unique<LockFreeQueue>()),
  file_(file),
  mutex_(mutex),
  cond_(cond),
  already_swap_(already_swap)
{

  Utils::CurrentTime(&tv_base_, &tm_base_);
}

LogStream::~LogStream()
{
  if (file_ != nullptr)
  {
    fclose(file_);
    file_ = nullptr;
  }
}

int64_t LogStream::Write(const char *data, int64_t ToWrite)
{
  return InternalWrite(data, ToWrite);
}

/*
 * Swap front buffer and back buffer.
 * This function should be locked.
 */
void LogStream::SwapBuffer()
{
  /*Buffer *pt_tmp = pt_front_buff_.release();
  pt_front_buff_.reset(pt_back_buff_.release());
  pt_back_buff_.reset(pt_tmp);*/
  std::swap(front_buff_, back_buff_);
}

/*
 * Write buffer data to log file.
 * This function should be locked.
 */
void LogStream::WriteBuffer()
{
  back_buff_->Flush(file_);
  back_buff_->Clear();

  /*std::string data;
  int ret = queue_->Pop(data);

  if (ret < 0)
    return;

  size_t n_write = 0;
  while ((n_write = fwrite(data.c_str(), 1, data.length(), file_)) != 0)
  {
    if ((n_write < 0) && (errno != EINTR))
    {
      // error
      break;
    }
    else if (n_write == data.length())
    {
      // All write
      break;
    }
    else if (n_write > 0)
    {
      // Half write
    }
  }*/
}

int64_t LogStream::InternalWrite(const char *log_data, int64_t ToWrite)
{
  int64_t Result = ToWrite;
  UpdateBaseTime();
/*
  char buff[256]{};
  int n_append = sprintf_s(buff, sizeof(buff), "%d-%02d-%02d %02d:%02d:%02d.%.03ld %s %d %s %s %s\n",
      tm_base_->tm_year + 1900, tm_base_->tm_mon + 1, tm_base_->tm_mday,
      tm_base_->tm_hour, tm_base_->tm_min, tm_base_->tm_sec, (long)tv_base_.tv_usec / 1000,
      file_name_, line_, func_name_, str_log_level_.c_str(),
      log_data);

  std::string log(buff, n_append);
*/
  // queue_->Push(log);

  pthread_mutex_lock(&mutex_);

  if (front_buff_->TryAppend(tm_base_, (int64_t)tv_base_.tv_usec, file_name_, line_, func_name_, str_log_level_, log_data) < 0)
  {
    SwapBuffer();
    already_swap_ = true;
    front_buff_->TryAppend(tm_base_, (long)tv_base_.tv_usec, file_name_, line_, func_name_, str_log_level_, log_data);
  }

  pthread_cond_signal(&cond_);
  pthread_mutex_unlock(&mutex_);

  return Result;
}

LogStream &LogStream::operator<<(const char *pt_log)
{
  InternalWrite(pt_log, strlen(pt_log));

  return *this;
}

LogStream& LogStream::operator<<(const std::string& ref_log)
{
  InternalWrite(ref_log.c_str(), ref_log.size());
/*
  UpdateBaseTime();

  char buff[256]{};
  int n_append = sprintf(buff, "%d-%02d-%02d %02d:%02d:%02d.%.03ld %s %d %s %s %s\n",
      tm_base_->tm_year + 1900, tm_base_->tm_mon + 1, tm_base_->tm_mday,
      tm_base_->tm_hour, tm_base_->tm_min, tm_base_->tm_sec, (long)tv_base_.tv_usec / 1000,
      file_name_, line_, func_name_, str_log_level_.c_str(),
      ref_log.c_str());

  std::string log = buff;

  queue_->Push(log);
*/
  return *this;
}

void LogStream::UpdateBaseTime()
{
  struct timeval tv_now;
  time_t now = time(nullptr);
  tv_now.tv_sec = (long)now;
  tv_now.tv_usec = 0;
  struct tm *tm = localtime(&now);
  gettimeofday(&tv_now, NULL);

  if (tv_now.tv_sec != tv_base_.tv_sec)
  {
    int new_sec = tm_base_->tm_sec + int(tv_now.tv_sec - tv_base_.tv_sec);
    if (new_sec >= 60)
    {
      tm_base_->tm_sec = new_sec % 60;
      int new_min = tm_base_->tm_min + new_sec / 60;
      if (new_min >= 60)
      {
        tm_base_->tm_min = new_min % 60;
        int new_hour = tm_base_->tm_hour + new_min / 60;
        if (new_hour >= 24)
        {
          Utils::CurrentTime(&tv_now, &tm_base_);
        }
        else
        {
          tm_base_->tm_hour = new_hour;
        }
      }
      else
      {
        tm_base_->tm_min = new_min;
      }
    }
    else
    {
      tm_base_->tm_sec = new_sec;
    }

    tv_base_ = tv_now;
  }
  else
  {
    tv_base_.tv_usec = tv_now.tv_usec;
  }
}
} // namespace tinylog
