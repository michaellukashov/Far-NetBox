#pragma once

#include <headers.hpp>


class TCriticalSection
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TCriticalSection)
public:
  TCriticalSection();
  ~TCriticalSection();

  void Enter() const;
  void Leave() const;

  intptr_t GetAcquired() const { return FAcquired; }

private:
  mutable CRITICAL_SECTION FSection;
  mutable intptr_t FAcquired;
};
