#pragma once

#include <nbsystem.h>

class TCriticalSection
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TCriticalSection)
public:
  TCriticalSection() noexcept;
  ~TCriticalSection() noexcept;

  void Enter() const;
  void Leave() const;
  bool TryEnter();
  void Release();

  int32_t GetAcquired() const { return FAcquired; }

private:
  mutable CRITICAL_SECTION FSection{};
  mutable int32_t FAcquired{0};
};
