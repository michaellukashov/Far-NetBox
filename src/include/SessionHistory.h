#pragma once\r
\r
#include <nbstring.h>\r
\r
namespace nb {\r
\r
struct TSessionHistoryEntry\r
{\r
  UnicodeString SessionName;\r
  UnicodeString RemoteDirectory;\r
  bool Valid{false};\r
};\r
\r
NB_CORE_EXPORT UnicodeString EncodeSessionParam(\r
  const UnicodeString & SessionName, const UnicodeString & RemoteDirectory);\r
NB_CORE_EXPORT TSessionHistoryEntry DecodeSessionParam(\r
  const UnicodeString & Param);\r
\r
} // namespace nb\r
