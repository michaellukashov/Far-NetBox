
#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FileSystems.h"
#include "RemoteFiles.h"
//#include "CopyParam.h"

#if defined(__BORLANDC__)
#pragma package(smart_init)
#endif // defined(__BORLANDC__)

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
  UNREACHABLE_AFTER_NORETURN(return EmptyStr);
}

UnicodeString TCustomFileSystem::CalculateFilesChecksumInitialize(const UnicodeString & DebugUsedArg(Alg))
{
  NotImplemented();
  UNREACHABLE_AFTER_NORETURN(return EmptyStr);
}

void TCustomFileSystem::TransferOnDirectory(
  const UnicodeString & ADirectory, const TCopyParamType *, int32_t AParams)
{
  DebugUsedParam(ADirectory);
  DebugUsedParam(AParams);
}

void TCustomFileSystem::DirectorySunk(
  const UnicodeString & DestFullName, const TRemoteFile *, const TCopyParamType *)
{
  DebugUsedParam(DestFullName);
}
