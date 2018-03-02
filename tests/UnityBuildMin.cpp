#define PUTTY_DO_GLOBALS
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif

#include <disable_warnings_in_std_begin.hpp>

#include "../nbcore/nbmemory.cpp"
#include "../nbcore/nbstring.cpp"
#include "../nbcore/nbutils.cpp"

#include "../base/UnicodeString.cpp"
#include "../base/Classes.cpp"
#include "../base/Global.cpp"
#include "../base/Common.cpp"
#include "../base/FormatUtils.cpp"
#include "../base/Sysutils.cpp"
#include "../base/StrUtils.cpp"
#include "../base/Exceptions.cpp"
#include "../base/System.SyncObjs.cpp"
#include "../base/LibraryLoader.cpp"
#include "../base/FileBuffer.cpp"
// #include "../windows/WinInterface.cpp"
// stub
bool AppendExceptionStackTraceAndForget(TStrings *&MoreMessages)
{
  return false;
}

#include <disable_warnings_in_std_end.hpp>
