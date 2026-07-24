#pragma once

#include <SessionContext.h>

namespace nb {

struct TSessionHistoryEntry
{
  UnicodeString SessionName;
  UnicodeString RemoteDirectory;
  bool Valid{false};
};

UnicodeString EncodeSessionParam(const UnicodeString & SessionName, const UnicodeString & RemoteDirectory);
TSessionHistoryEntry DecodeSessionParam(const UnicodeString & Param);
void LogSessionEvent(ISessionContext * SessionContext, const UnicodeString & Text);

} // namespace nb
