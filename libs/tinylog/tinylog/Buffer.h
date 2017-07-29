#pragma once

#include <cstdint>
#include <string>

#include "Utils.h"

namespace tinylog {

class Buffer
{
public:
  explicit Buffer(uint32_t l_capacity);
  ~Buffer();

  int32_t TryAppend(struct tm *pt_time, long u_sec, const char *pt_file, int i_line,
    const char *pt_func, std::string &str_log_level, const char *pt_log);

  void Clear();
  size_t Size() const;
  size_t Capacity() const;
  int32_t Flush(FILE *file);

private:
  Buffer(const Buffer &);
  Buffer &operator=(const Buffer &);

  char *pt_data_;
  size_t l_size_;
  size_t l_capacity_;
};

} // namespace tinylog
