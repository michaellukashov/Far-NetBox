#include <string.h>
#include <string>
#include <cerrno>
#include <io.h>

#include "Buffer.h"

namespace tinylog {

Buffer::Buffer(uint32_t l_capacity)
{
  pt_data_ = new char[l_capacity];
  l_size_ = 0;
  l_capacity_ = l_capacity;
}

Buffer::~Buffer()
{
  // TODO: custom allocator/deallocator
  delete[] pt_data_;
}

/*
 * Append time and log to buffer.
 * This function should be locked.
 * return value:
 * 0  : success
 * -1 : fail, buffer full
 */
int32_t Buffer::TryAppend(
  struct tm *pt_time, long u_sec, const char *pt_file, int i_line,
  const char *pt_func, std::string &str_log_level, const char *pt_log)
{
  /*
   * date: 11 byte
   * time: 13 byte
   * line number: at most 5 byte
   * log level: 9 byte
  */
  size_t append_len = 24 + strlen(pt_file) + 5 + strlen(pt_func) + 9 + strlen(pt_log);

  if (append_len + l_size_ > l_capacity_)
  {
    return -1;
  }

  int n_append = sprintf(pt_data_ + l_size_, "%d-%02d-%02d %02d:%02d:%02d.%.03ld %s %d %s %s %s\n",
      pt_time->tm_year + 1900, pt_time->tm_mon + 1, pt_time->tm_mday,
      pt_time->tm_hour, pt_time->tm_min, pt_time->tm_sec, u_sec / 1000,
      pt_file, i_line, pt_func, str_log_level.c_str(),
      pt_log);

  if (n_append > 0)
  {
    l_size_ += n_append;
  }

  return 0;
}

void Buffer::Clear()
{
  l_size_ = 0;
}

size_t Buffer::Size() const
{
  return l_size_;
}

size_t Buffer::Capacity() const
{
  return l_capacity_;
}

int32_t Buffer::Flush(FILE *file)
{
  size_t n_write = fwrite(pt_data_, 1, l_size_, file);
  if (n_write != l_size_)
  {
    // error
    return -1;
  }

  return 0;
}

} // namespace tinylog
