
#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FileSystems.h"
#include "RemoteFiles.h"
#include "CopyParam.h"

// #pragma package(smart_init)

TCustomFileSystem::TCustomFileSystem(TObjectClassId Kind, TTerminal * ATerminal) noexcept : TObject(Kind),
  FTerminal(ATerminal)
{
  DebugAssert(FTerminal);
}

TCustomFileSystem::~TCustomFileSystem() noexcept
{
  // DEBUG_PRINTF("begin");
#ifdef USE_DLMALLOC
  dlmalloc_trim(0); // 64 * 1024);
#endif
}

UnicodeString TCustomFileSystem::GetHomeDirectory()
{
  NotImplemented();
  return EmptyStr;
}

UnicodeString TCustomFileSystem::CalculateFilesChecksumInitialize(const UnicodeString & DebugUsedArg(Alg))
{
  NotImplemented();
  return EmptyStr;
}
