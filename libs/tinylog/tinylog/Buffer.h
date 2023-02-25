#pragma once

#include <stdio.h>
#include <string>

#include "Utils.h"

namespace tinylog {

class Buffer
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  Buffer() = delete;
  explicit Buffer(uint64_t capacity);
  ~Buffer();

  int32_t TryAppend(struct tm* pt_time, int64_t u_sec, const char* file_name, int32_t line,
    const char* func_name, std::string& str_log_level, const char* log_data);
  int32_t TryAppend(const void* pt_log, int32_t ToWrite);

  void Clear();
  uint64_t Size() const;
  uint64_t Capacity() const;
  int32_t Flush(FILE* file);

private:
  Buffer(const Buffer&);
  Buffer& operator=(const Buffer&);

  char* pt_data_{nullptr};
  uint64_t size_{0};
  uint64_t capacity_{0};
};

} // namespace tinylog
