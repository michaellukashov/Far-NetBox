#pragma once

#include <nbstring.h>

namespace nb {

struct TSessionHistoryEntry
{
  UnicodeString SessionName;
  UnicodeString RemoteDirectory;
  bool Valid{false};
};

NB_CORE_EXPORT UnicodeString EncodeSessionParam(
  const UnicodeString & SessionName, const UnicodeString & RemoteDirectory);
NB_CORE_EXPORT TSessionHistoryEntry DecodeSessionParam(
  const UnicodeString & Param);

} // namespace nb
