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
