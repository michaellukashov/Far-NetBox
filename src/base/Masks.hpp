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
  bool GetMatches(const UnicodeString Str);
private:
  UnicodeString FMask;
};

} // namespace Masks

//---------------------------------------------------------------------------
