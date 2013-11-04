//---------------------------------------------------------------------------
#include <headers.hpp>
#include <Classes.hpp>
#include <StrUtils.hpp>

//---------------------------------------------------------------------------

UnicodeString ReplaceStr(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  return StringReplace(Str, What, ByWhat, TReplaceFlags() << rfReplaceAll);
}

bool StartsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  return Str.Pos(SubStr) == 1;
}
