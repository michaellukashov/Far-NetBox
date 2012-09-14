#pragma once

#include <Classes.hpp>

namespace Masks
{

class TMask
{
public:
  explicit TMask(const UnicodeString Mask) :
    FMask(Mask)
  {
  }
  bool Matches(const UnicodeString Str)
  {
    return Sysutils::AnsiCompareIC(FMask, Str) == 0;
  }
private:
  UnicodeString FMask;
};

} // namespace Masks

