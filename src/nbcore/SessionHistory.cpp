#include "stdafx.h"

#include <Common.h>
#include "SessionHistory.h"

namespace nb {

UnicodeString EncodeSessionParam(
  const UnicodeString & SessionName, const UnicodeString & RemoteDirectory)
{
  UnicodeString Result = L"netbox://" + EncodeUrlString(SessionName);
  if (!RemoteDirectory.IsEmpty())
  {
    Result += L"/" + EncodeUrlPath(RemoteDirectory);
  }
  return Result;
}

TSessionHistoryEntry DecodeSessionParam(const UnicodeString & Param)
{
  TSessionHistoryEntry Result;
  constexpr const wchar_t * NetboxPrefix = L"netbox://";
  constexpr const int32_t PrefixLen = 9;

  if (Param.SubString(1, PrefixLen).LowerCase() == NetboxPrefix)
  {
    UnicodeString Remaining = Param.SubString(PrefixLen + 1, Param.Length() - PrefixLen);
    const int32_t SlashPos = Remaining.Pos(L'/');
    if (SlashPos > 0)
    {
      Result.SessionName = DecodeUrlChars(Remaining.SubString(1, SlashPos - 1));
      Result.RemoteDirectory = DecodeUrlChars(Remaining.SubString(SlashPos + 1, Remaining.Length() - SlashPos));
    }
    else
    {
      Result.SessionName = DecodeUrlChars(Remaining);
    }
    Result.Valid = !Result.SessionName.IsEmpty();
  }
  else if (Param.SubString(1, 7).LowerCase() == L"netbox:")
  {
    // Fallback to old format: netbox:SessionName\1RemoteDirectory
    UnicodeString Remaining = Param.SubString(8, Param.Length() - 7);
    const int32_t DelimPos = Remaining.Pos(L'\1');
    if (DelimPos > 0)
    {
      Result.SessionName = Remaining.SubString(1, DelimPos - 1);
      Result.RemoteDirectory = Remaining.SubString(DelimPos + 1, Remaining.Length() - DelimPos);
    }
    else
    {
      Result.SessionName = Remaining;
    }
    Result.Valid = !Result.SessionName.IsEmpty();
  }
  return Result;
}

} // namespace nb
