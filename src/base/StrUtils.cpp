
#include <vcl.h>

#include <Classes.hpp>
#include <StrUtils.hpp>
#include <Sysutils.hpp>

UnicodeString ReplaceStr(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  return ::StringReplaceAll(Str, What, ByWhat);
}

bool StartsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  return Str.Pos(SubStr) == 1;
}

bool EndsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  if (SubStr.Length() > Str.Length())
    return false;
  return Str.SubStr(Str.Length() - SubStr.Length() + 1, SubStr.Length()) == SubStr;
}

NB_CORE_EXPORT bool EndsText(const UnicodeString & SubStr, const UnicodeString & Str)
{
  return EndsStr(SubStr, Str);
}

NB_CORE_EXPORT UnicodeString LeftStr(const UnicodeString & AStr, int32_t Len)
{
  return AStr.SubString(Len);
}

NB_CORE_EXPORT UnicodeString EncodeBase64(const char * AStr, int32_t Len)
{
  UnicodeString Result;
  return Result;
}

NB_CORE_EXPORT TBytes DecodeBase64(const UnicodeString & AStr)
{
  nb::vector_t<uint8_t> Result;
  return Result;
}
