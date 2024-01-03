
#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FileSystems.h"
#include "RemoteFiles.h"
#include "CopyParam.h"

// #pragma package(smart_init)

const TObjectClassId OBJECT_CLASS_TSinkFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFileTransferData = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TOverwriteFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TOpenRemoteFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCustomFileSystem = static_cast<TObjectClassId>(nb::counter_id());

TCustomFileSystem::TCustomFileSystem(TObjectClassId Kind, TTerminal * ATerminal) noexcept :
  TObject(Kind),
  FTerminal(ATerminal)
{
  DebugAssert(FTerminal);
}

TCustomFileSystem::~TCustomFileSystem() noexcept
{
#ifdef USE_DLMALLOC
  dlmalloc_trim(0); // 64 * 1024);
#endif
}

UnicodeString TCustomFileSystem::CreateTargetDirectory(
  IN const UnicodeString & AFileName,
  IN const UnicodeString & ADirectory,
  IN const TCopyParamType * CopyParam)
{
  UnicodeString Result = ADirectory;
  const UnicodeString DestFileName = CopyParam->ChangeFileName(base::UnixExtractFileName(AFileName),
    osRemote, true);
  const UnicodeString FileNamePath = ::ExtractFilePath(DestFileName);
  if (!FileNamePath.IsEmpty())
  {
    Result = ::IncludeTrailingBackslash(ADirectory + FileNamePath);
    if (!::SysUtulsForceDirectories(ApiPath(Result)))
      Result.Clear();
  }
  return Result;
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
