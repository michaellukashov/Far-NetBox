#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FileSystems.h"
#include "RemoteFiles.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
TCustomFileSystem::TCustomFileSystem(TObjectClassId Kind, TTerminal *ATerminal) noexcept :
  TObject(Kind),
  FTerminal(ATerminal)
{
  DebugAssert(FTerminal);
}
//---------------------------------------------------------------------------
TCustomFileSystem::~TCustomFileSystem() noexcept
{
#ifdef USE_DLMALLOC
  dlmalloc_trim(0); // 64 * 1024);
#endif
}
//---------------------------------------------------------------------------
UnicodeString TCustomFileSystem::CreateTargetDirectory(
  IN UnicodeString AFileName,
  IN UnicodeString ADirectory,
  IN const TCopyParamType *CopyParam)
{
  UnicodeString Result = ADirectory;
  UnicodeString DestFileName = CopyParam->ChangeFileName(base::UnixExtractFileName(AFileName),
      osRemote, true);
  UnicodeString FileNamePath = ::ExtractFilePath(DestFileName);
  if (!FileNamePath.IsEmpty())
  {
    Result = ::IncludeTrailingBackslash(ADirectory + FileNamePath);
    if (!::SysUtulsForceDirectories(ApiPath(Result)))
      Result.Clear();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TCustomFileSystem::GetHomeDirectory()
{
  throw Exception("Not implemented");
}
