#pragma once

#include "Utils.h"

namespace tinylog {

class Buffer
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit Buffer(uint32_t l_capacity);
  ~Buffer();

  int32_t TryAppend(const void *pt_log, intptr_t ToWrite);

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
