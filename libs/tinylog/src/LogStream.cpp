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

size_t LogStream::Write(const char * data, size_t ToWrite)
{
  return FormattedWrite(data, ToWrite);
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

size_t LogStream::FormattedWrite(const char * log_data, size_t to_write)
{
  // max length calculation. assume max line number is 999999
  // "2000-01-01 00:00:00.000 <file_name>:999999 <func_name> <level> <log_data>\n\0"
  // 24 + max(file, 10) + 1 + 6 + 1 + max(func, 16) + 1 + max(level, 10) + 1 + to_write + 2
  // 36 + max(file, 10) + max(func, 16) + max(level, 10) + to_write

  size_t Result = 0;

  UpdateBaseTime();

#define MAX(STR, DEF) std::max(((STR) ? strlen(STR) : 0ULL), (DEF))
  const size_t prefix_len = 36 + MAX(file_name_, 10ULL) +
    MAX(func_name_, 16ULL) + MAX(str_log_level_, 10ULL);
  const size_t append_len = prefix_len + to_write;
#undef MAX

  char * buf = nb::chcalloc(append_len);
  if (buf != nullptr)
  {
    int n_append = 0;
    if (file_name_ && *file_name_ && line_ > 0 && func_name_ && *func_name_ && str_log_level_ && *str_log_level_)
    {
      const int32_t line_number = line_ > 999999 ? 999999 : line_;

      n_append = sprintf_s(buf, append_len,
        "%d-%02d-%02d %02d:%02d:%02d.%.03d %10s:%3d %16s %10s %.*s\n",
        tm_base_.tm_year + 1900, tm_base_.tm_mon + 1, tm_base_.tm_mday,
        tm_base_.tm_hour, tm_base_.tm_min, tm_base_.tm_sec, static_cast<int>(tv_base_.tv_usec / 1000),
        file_name_ ? file_name_ : "", line_number, func_name_ ? func_name_ : "", str_log_level_,
        static_cast<int>(to_write), log_data);
    }
    else
    {
      n_append = sprintf_s(buf, append_len,
        "%d-%02d-%02d %02d:%02d:%02d.%.03d %.*s\n",
        tm_base_.tm_year + 1900, tm_base_.tm_mon + 1, tm_base_.tm_mday,
        tm_base_.tm_hour, tm_base_.tm_min, tm_base_.tm_sec, static_cast<int>(tv_base_.tv_usec / 1000),
        static_cast<int>(to_write), log_data);
    }

    if (n_append > 0)
    {
      Result = InternalWrite(buf, n_append);
    }
    nb_free(buf);
  }
  return Result;
}

size_t LogStream::InternalWrite(const char * log_data, size_t to_write)
{
  const size_t Result = to_write;
  size_t n_append;

  pthread_mutex_lock(&mutex_);
  // DEBUG_PRINTF("ToWrite: %d", (int)to_write);
  while (to_write)
  {
    auto & buff = drain_buffer_ ? back_buff_ : front_buff_;
    // append to the current buffer
    n_append = buff->TryAppend(log_data, to_write);
    if (n_append == 0)
    {
      if (drain_buffer_)
      {
        // we are appending to the back_buff_ and there is no more space there
        // wait until buffer is drained (very rare situation)
        pthread_cond_signal(&cond_);
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
      log_data += n_append;
      to_write -= n_append;
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
  FormattedWrite(log_data, strlen(log_data));

  return *this;
}

LogStream & LogStream::operator<<(const std::string & ref_log)
{
  FormattedWrite(ref_log.c_str(), ref_log.size());
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
