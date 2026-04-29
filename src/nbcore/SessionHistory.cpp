#include "stdafx.h"\r
\r
#include <Common.h>\r
#include "SessionHistory.h"\r
\r
namespace nb {\r
\r
UnicodeString EncodeSessionParam(\r
  const UnicodeString & SessionName, const UnicodeString & RemoteDirectory)\r
{\r
  UnicodeString Result = L"netbox://" + EncodeUrlString(SessionName);\r
  if (!RemoteDirectory.IsEmpty())\r
  {\r
    Result += L"/" + EncodeUrlPath(RemoteDirectory);\r
  }\r
  return Result;\r
}\r
\r
TSessionHistoryEntry DecodeSessionParam(const UnicodeString & Param)\r
{\r
  TSessionHistoryEntry Result;\r
  constexpr const wchar_t * NetboxPrefix = L"netbox://";\r
  constexpr const int32_t PrefixLen = 9;\r
\r
  if (Param.SubString(1, PrefixLen).LowerCase() == NetboxPrefix)\r
  {\r
    UnicodeString Remaining = Param.SubString(PrefixLen + 1, Param.Length() - PrefixLen);\r
    const int32_t SlashPos = Remaining.Pos(L'/');\r
    if (SlashPos > 0)\r
    {\r
      Result.SessionName = DecodeUrlChars(Remaining.SubString(1, SlashPos - 1));\r
      Result.RemoteDirectory = DecodeUrlChars(Remaining.SubString(SlashPos + 1, Remaining.Length() - SlashPos));\r
    }\r
    else\r
    {\r
      Result.SessionName = DecodeUrlChars(Remaining);\r
    }\r
    Result.Valid = !Result.SessionName.IsEmpty();\r
  }\r
  else if (Param.SubString(1, 7).LowerCase() == L"netbox:")\r
  {\r
    // Fallback to old format: netbox:SessionName\1RemoteDirectory\r
    UnicodeString Remaining = Param.SubString(8, Param.Length() - 7);\r
    const int32_t DelimPos = Remaining.Pos(L'\1');\r
    if (DelimPos > 0)\r
    {\r
      Result.SessionName = Remaining.SubString(1, DelimPos - 1);\r
      Result.RemoteDirectory = Remaining.SubString(DelimPos + 1, Remaining.Length() - DelimPos);\r
    }\r
    else\r
    {\r
      Result.SessionName = Remaining;\r
    }\r
    Result.Valid = !Result.SessionName.IsEmpty();\r
  }\r
  return Result;\r
}\r
\r
} // namespace nb\r
