#include <io.h>
#include <fcntl.h>

#include <tinylog/platform_win32.h>

#include <tinylog/LogStream.h>
#include <tinylog/Config.h>
//#include <tinylog/LockFreeQueue.h>
// #include <Sysutils.hpp> // for DEBUG_PRINTF


namespace tinylog {

LogStream::LogStream(FILE * file, pthread_mutex_t & mutex, pthread_cond_t & cond, bool & drain_buffer) :
  front_buff_(std::make_unique<Buffer>(LOG_BUFFER_SIZE)),
  back_buff_(std::make_unique<Buffer>(LOG_BUFFER_SIZE)),
//  queue_(std::make_unique<LockFreeQueue>()),
  file_(file),
  mutex_(mutex),
  cond_(cond),
  drain_buffer_(drain_buffer)
{
  Utils::CurrentTime(&tv_base_, &tm_base_);
  // DEBUG_PRINTF("begin");
}

LogStream::~LogStream()
{
  // DEBUG_PRINTF("end");
  if (file_ != nullptr)
  {
    fclose(file_);
    file_ = nullptr;
  }
  // DEBUG_PRINTF("end");
}

int64_t LogStream::Write(const char * data, int64_t ToWrite)
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
  if (!file_)
    return;
  front_buff_->Flush(file_);
  front_buff_->Clear();

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

void LogStream::SetFile(FILE * file)
{
  assert(file_ == nullptr);
  file_ = file;
}

int64_t LogStream::InternalWrite(const char * log_data, int64_t to_write)
{
  const int64_t Result = to_write;
  UpdateBaseTime();
  // queue_->Push(log);
  char * data_to_write = (char * )log_data;

  pthread_mutex_lock(&mutex_);
  // DEBUG_PRINTF("ToWrite: %d", (int)to_write);
  while (true)
  {
    auto & buff = drain_buffer_ ? back_buff_ : front_buff_;
    // append to the current buffer
    int64_t need_capacity = static_cast<int64_t>(buff->Capacity() - buff->Size());
    if (to_write > need_capacity)
    {
      // trunc log_data
      tmp_buff_ = std::make_unique<Buffer>(need_capacity);
      tmp_buff_->TryAppend(log_data, need_capacity);
      data_to_write = tmp_buff_->Data();
      to_write = need_capacity;
    }
    if (buff->TryAppend(&tm_base_, static_cast<int64_t>(tv_base_.tv_usec), file_name_, line_, func_name_, str_log_level_, data_to_write, to_write) < 0)
    {
      if (drain_buffer_)
      {
        // we are appending to the back_buff_ and there is no more space there
        // wait until buffer is drained (very rare situation)
        pthread_mutex_unlock(&mutex_);
        while (drain_buffer_)
        {
          // yield execution to another thread
          // usually it will require only one iteration
          Sleep(1);
          // DEBUG_PRINTF("after Sleep");
        }
        pthread_mutex_lock(&mutex_);
      }
      else
      {
        drain_buffer_ = true;
      }
    }
    else
    {
      break;
    }
  }

  if (drain_buffer_)
  {
    pthread_cond_signal(&cond_);
  }
  pthread_mutex_unlock(&mutex_);

  return Result;
}

LogStream & LogStream::operator<<(const char * log_data)
{
  InternalWrite(log_data, strlen(log_data));

  return *this;
}

LogStream & LogStream::operator<<(const std::string & ref_log)
{
  InternalWrite(ref_log.c_str(), ref_log.size());
/*
  UpdateBaseTime();

  char buff[256]{};
  int n_append = sprintf(buff, "%d-%02d-%02d %02d:%02d:%02d.%.03ld %s %d %s %s %s\n",
      tm_base_.tm_year + 1900, tm_base_.tm_mon + 1, tm_base_.tm_mday,
      tm_base_.tm_hour, tm_base_.tm_min, tm_base_.tm_sec, (long)tv_base_.tv_usec / 1000,
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
  const time_t now = time(nullptr);
  tv_now.tv_sec = static_cast<long>(now);
  tv_now.tv_usec = 0;
  struct tm * tm = localtime(&now);
  gettimeofday(&tv_now, nullptr);

  if (tv_now.tv_sec != tv_base_.tv_sec)
  {
    const int new_sec = tm_base_.tm_sec + static_cast<int>(tv_now.tv_sec - tv_base_.tv_sec);
    if (new_sec >= 60)
    {
      tm_base_.tm_sec = new_sec % 60;
      const int new_min = tm_base_.tm_min + new_sec / 60;
      if (new_min >= 60)
      {
        tm_base_.tm_min = new_min % 60;
        const int new_hour = tm_base_.tm_hour + new_min / 60;
        if (new_hour >= 24)
        {
          Utils::CurrentTime(&tv_now, &tm_base_);
        }
        else
        {
          tm_base_.tm_hour = new_hour;
        }
      }
      else
      {
        tm_base_.tm_min = new_min;
      }
    }
    else
    {
      tm_base_.tm_sec = new_sec;
    }

    tv_base_ = tv_now;
  }
  else
  {
    tv_base_.tv_usec = tv_now.tv_usec;
  }
}

void LogStream::SetPrefix(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level)
{
  file_name_ = tinylog::past_last_slash(file_name);
  line_  = line;
  func_name_ = func_name;

  switch (log_level)
  {
  case Utils::LEVEL_DEBUG:
    str_log_level_ = "[DEBUG  ]";
    break;
  case Utils::LEVEL_INFO:
    str_log_level_ = "[INFO   ]";
    break;
  case Utils::LEVEL_WARNING:
    str_log_level_ = "[WARNING]";
    break;
  case Utils::LEVEL_ERROR:
    str_log_level_ = "[ERROR  ]";
    break;
  case Utils::LEVEL_FATAL:
    str_log_level_ = "[FATAL  ]";
    break;
  default:
    str_log_level_ = "[INFO   ]";
    break;
  }
}

} // namespace tinylog
