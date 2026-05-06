#include "SecureString.h"
#include <windows.h>

TSecureString::TSecureString(const UnicodeString & Value)
{
  SetValue(Value);
}

TSecureString::~TSecureString() noexcept
{
  Free();
}

TSecureString::TSecureString(TSecureString && Other) noexcept :
  FData(Other.FData),
  FSize(Other.FSize),
  FLocked(Other.FLocked)
{
  Other.FData = nullptr;
  Other.FSize = 0;
  Other.FLocked = false;
}

TSecureString & TSecureString::operator=(TSecureString && Other) noexcept
{
  if (this != &Other)
  {
    Free();
    FData = Other.FData;
    FSize = Other.FSize;
    FLocked = Other.FLocked;
    Other.FData = nullptr;
    Other.FSize = 0;
    Other.FLocked = false;
  }
  return *this;
}

void TSecureString::SetValue(const UnicodeString & Value)
{
  Clear();
  if (!Value.IsEmpty())
  {
    const size_t Len = Value.Length();
    Allocate(Len + 1);
    if (FData)
    {
      memcpy(FData, Value.c_str(), Len * sizeof(wchar_t));
      FData[Len] = L'\0';
    }
  }
}

UnicodeString TSecureString::GetValue() const
{
  if (FData && FSize > 0)
  {
    return UnicodeString(FData, static_cast<int32_t>(FSize - 1));
  }
  return UnicodeString();
}

void TSecureString::Clear()
{
  Free();
}

void TSecureString::SecureZero()
{
  if (FData && FSize > 0)
  {
    SecureZeroMemory(FData, FSize * sizeof(wchar_t));
  }
}

void TSecureString::Allocate(size_t Size)
{
  FData = new wchar_t[Size];
  FSize = Size;
  // Attempt to lock the memory page to prevent swapping to disk.
  // This requires SE_LOCK_MEMORY privilege; if unavailable, we
  // fall back to normal heap but still securely wipe on free.
  FLocked = (::VirtualLock(FData, FSize * sizeof(wchar_t)) != FALSE);
}

void TSecureString::Free()
{
  if (FData)
  {
    SecureZero();
    if (FLocked)
    {
      ::VirtualUnlock(FData, FSize * sizeof(wchar_t));
    }
    delete[] FData;
    FData = nullptr;
    FSize = 0;
    FLocked = false;
  }
}
