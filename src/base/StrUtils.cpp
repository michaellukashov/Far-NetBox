
#include <vcl.h>

#include <Classes.hpp>
#include <StrUtils.hpp>
#include <Sysutils.hpp>

UnicodeString ReplaceStr(UnicodeString Str, UnicodeString What, UnicodeString ByWhat)
{
  return ::StringReplaceAll(Str, What, ByWhat);
}

bool StartsStr(UnicodeString SubStr, UnicodeString Str)
{
  return Str.Pos(SubStr) == 1;
}

bool EndsStr(UnicodeString SubStr, UnicodeString Str)
{
  if (SubStr.Length() > Str.Length())
    return false;
  return Str.SubStr(Str.Length() - SubStr.Length() + 1, SubStr.Length()) == SubStr;
}

NB_CORE_EXPORT bool EndsText(UnicodeString SubStr, UnicodeString Str)
{
  return EndsStr(SubStr, Str);
}

NB_CORE_EXPORT UnicodeString LeftStr(UnicodeString AStr, intptr_t Len)
{
  return AStr.SubString(Len);
}

NB_CORE_EXPORT UnicodeString EncodeBase64(const char* AStr, intptr_t Len)
{
  UnicodeString Result;
  return Result;
}

NB_CORE_EXPORT rde::vector<uint8_t> DecodeBase64(UnicodeString AStr)
{
  rde::vector<uint8_t> Result;
  return Result;
}
