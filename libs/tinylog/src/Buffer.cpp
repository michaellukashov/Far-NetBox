#include <io.h>
#include <algorithm>
#include <cassert>
#include <memory>

#include <tinylog/Buffer.h>

namespace tinylog {

Buffer::Buffer(size_t capacity)
{
  data_ = nb::chcalloc(capacity);
  size_ = 0;
  capacity_ = capacity;
}

Buffer::~Buffer()
{
  nb_free(data_);
}

/*
 * Append data to the buffer.
 * This function should be locked.
 * return value:
 * number of bytes copied to the buffer
 */
size_t Buffer::TryAppend(const void * pt_log, size_t ToWrite)
{
  const size_t available_space = capacity_ - size_;
  size_t to_write = std::min(available_space, ToWrite);
  if (to_write > 0)
  {
    memmove(data_ + size_, pt_log, to_write);
    size_ += to_write;
  }
  return to_write;
}

void Buffer::Clear()
{
  size_ = 0;
}

size_t Buffer::Size() const
{
  return size_;
}

size_t Buffer::Capacity() const
{
  return capacity_;
}

int32_t Buffer::Flush(FILE* file)
{
  // DebugCheck(file);
  assert(file);
  if (!size_)
    return 0;
  size_t n_write = 0;
  while ((n_write = fwrite(data_, 1, size_ - n_write, file)) != 0)
  {
    if ((static_cast<intptr_t>(n_write) < 0) && (errno != EINTR))
    {
      // error
      break;
    }
    else if (n_write == size_)
    {
      // All write
      break;
    }
    else if (n_write > 0)
    {
      // Half write
    }
  }

  // error
  if (static_cast<intptr_t>(n_write) < 0)
    return -1;

  fflush(file);
  return 0;
}

} // namespace tinylog
