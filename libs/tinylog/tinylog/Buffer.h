#pragma once

// #include <stdio.h>
// #include <string>

#include "Utils.h"

namespace tinylog {

class Buffer
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  Buffer() = delete;
  explicit Buffer(size_t capacity);
  ~Buffer();

  int32_t TryAppend(struct tm* pt_time, int64_t u_sec, const char* file_name, int32_t line,
    const char* func_name, const char* str_log_level, const char* log_data, int64_t to_write);
  int32_t TryAppend(const void* pt_log, int32_t ToWrite);

  void Clear();
  uint64_t Size() const;
  uint64_t Capacity() const;
  char * Data() const { return data_; }
  int32_t Flush(FILE* file);

private:
  Buffer(const Buffer&) = delete;
  Buffer& operator =(const Buffer&) = delete;

  char * data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};

} // namespace tinylog
