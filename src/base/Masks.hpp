#pragma once

#include <Classes.hpp>

namespace Masks
{

class TMask : public TObject
{
public:
  explicit TMask(const UnicodeString & Mask)
  {
    FMask = ::ExtractFileExtension(Mask, L'\\');
  }
  bool Matches(const UnicodeString & Str)
  {
    UnicodeString Ext = ::ExtractFileExtension(Str, L'\\');
    return Sysutils::AnsiCompareIC(FMask, Ext) == 0;
  }

private:
  UnicodeString FMask;
};

} // namespace Masks

