#pragma once

#include <stdio.h>

#include "Utils.h"

namespace tinylog {

class Buffer
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  Buffer() = delete;
  explicit Buffer(uint32_t capacity);
  ~Buffer();

  int32_t TryAppend(const void *pt_log, intptr_t ToWrite);

  void Clear();
  size_t Size() const;
  size_t Capacity() const;
  int32_t Flush(FILE *file);

private:
  Buffer(const Buffer &);
  Buffer &operator=(const Buffer &);

  char *pt_data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};

} // namespace tinylog
