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

  size_t TryAppend(const void* pt_log, size_t ToWrite);

  void Clear();
  size_t Size() const;
  size_t Capacity() const;
  char * Data() const { return data_; }
  bool Flush(FILE* file);

private:
  Buffer(const Buffer&) = delete;
  Buffer& operator =(const Buffer&) = delete;

  char * data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};

} // namespace tinylog
