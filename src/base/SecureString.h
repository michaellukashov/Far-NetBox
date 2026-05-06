#pragma once

#include <Common.h>
#include <cstddef>

class NB_CORE_EXPORT TSecureString
{
public:
  TSecureString() noexcept = default;
  explicit TSecureString(const UnicodeString & Value);
  ~TSecureString() noexcept;

  TSecureString(const TSecureString &) = delete;
  TSecureString & operator=(const TSecureString &) = delete;
  TSecureString(TSecureString && Other) noexcept;
  TSecureString & operator=(TSecureString && Other) noexcept;

  void SetValue(const UnicodeString & Value);
  UnicodeString GetValue() const;
  void Clear();
  bool IsEmpty() const { return FSize == 0; }

private:
  wchar_t * FData{nullptr};
  size_t FSize{0};
  bool FLocked{false};

  void SecureZero();
  void Allocate(size_t Size);
  void Free();
};
