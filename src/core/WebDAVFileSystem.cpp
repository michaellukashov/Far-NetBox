#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <wincrypt.h>

#define NE_LFS
#define WINSCP
#include <neon/src/ne_basic.h>
#include <neon/src/ne_auth.h>
#include <neon/src/ne_props.h>
#include <neon/src/ne_uri.h>
#include <neon/src/ne_session.h>
#include <neon/src/ne_request.h>
#include <neon/src/ne_xml.h>
#include <neon/src/ne_redirect.h>
#include <neon/src/ne_xmlreq.h>
#include <neon/src/ne_locks.h>
#include <expat/lib/expat.h>

/*#include <apr_hash.h>
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_file_io.h>
#include <apr_portable.h>
#include <apr_atomic.h>

#include <neon/src/ne_compress.h>
#include <neon/src/ne_defs.h>
#include <neon/src/ne_utils.h>
#include <neon/src/ne_pkcs11.h>*/


#include "WebDAVFileSystem.h"

#include "Interface.h"
#include "Common.h"
#include "Exceptions.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "SecureShell.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Security.h"
#include <StrUtils.hpp>
#include <NeonIntf.h>
#include <openssl/ssl.h>

class TWebDAVFileListHelper : public TObject
{
NB_DISABLE_COPY(TWebDAVFileListHelper)
public:
  explicit TWebDAVFileListHelper(TWebDAVFileSystem * FileSystem, TRemoteFileList * FileList,
    bool IgnoreFileList) :
    FFileSystem(FileSystem),
    FFileList(FFileSystem->FFileList),
    FIgnoreFileList(FFileSystem->FIgnoreFileList)
  {
    FFileSystem->FFileList = FileList;
    FFileSystem->FIgnoreFileList = IgnoreFileList;
  }

  ~TWebDAVFileListHelper()
  {
    FFileSystem->FFileList = FFileList;
    FFileSystem->FIgnoreFileList = FIgnoreFileList;
  }

private:
  TWebDAVFileSystem * FFileSystem;
  TRemoteFileList * FFileList;
  bool FIgnoreFileList;
};

#define CONST_WEBDAV_PROTOCOL_BASE_NAME L"WebDAV"

void NeonInitialize()
{
  // Even if this fails, we do not want to interrupt WinSCP starting for that.
  // We may possibly remember that and fail opening session later.
  // Anyway, it can hardly fail.
  DebugAlwaysTrue(ne_sock_init());
}

void NeonFinalize()
{
  ne_sock_exit();
}

UnicodeString NeonVersion()
{
  UnicodeString Str = StrFromNeon(ne_version_string());
  CutToChar(Str, L' ', true); // "neon"
  UnicodeString Result = CutToChar(Str, L':', true);
  return Result;
}

UnicodeString ExpatVersion()
{
  return FORMAT(L"%d.%d.%d", XML_MAJOR_VERSION, XML_MINOR_VERSION, XML_MICRO_VERSION);
}

TWebDAVFileSystem::TWebDAVFileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(ATerminal),
  FFileList(nullptr),
  FOnCaptureOutput(nullptr),
  FIgnoreAuthenticationFailure(iafNo),
  FPortNumber(0),
  FStoredPasswordTried(false),
  FPasswordFailed(false),
  FActive(false),
  FFileTransferAbort(ftaNone),
  FIgnoreFileList(false),
  FFileTransferCancelled(false),
  FFileTransferResumed(0),
  FFileTransferPreserveTime(false),
  FHasTrailingSlash(false),
  FFileTransferCPSLimit(0),
  FLastReadDirectoryProgress(0),
  FCurrentOperationProgress(nullptr),
  webdav_pool(nullptr),
  FSession(nullptr)
{
}

void TWebDAVFileSystem::Init(void *)
{
  FFileSystemInfo.ProtocolBaseName = CONST_WEBDAV_PROTOCOL_BASE_NAME;
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;

  if (apr_initialize() != APR_SUCCESS)
    throw ExtException(UnicodeString(L"Cannot init APR"));
  apr_pool_create(&webdav_pool, nullptr);
}

TWebDAVFileSystem::~TWebDAVFileSystem()
{
  webdav_pool_destroy(webdav_pool);
  apr_terminate();
  webdav_pool = nullptr;
  ne_sock_exit();
}

void TWebDAVFileSystem::Open()
{
  FCurrentDirectory.Clear();
  FHasTrailingSlash = false;
  FStoredPasswordTried = false;
  FTlsVersionStr.Clear();

  TSessionData * Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();
  FSessionInfo.ProtocolBaseName = CONST_WEBDAV_PROTOCOL_BASE_NAME;
  FSessionInfo.ProtocolName = FSessionInfo.ProtocolBaseName;

  bool Ssl = (FTerminal->GetSessionData()->GetFtps() != ftpsNone);
  if (Ssl)
  {
    FSessionInfo.SecurityProtocolName = LoadStr(FTPS_IMPLICIT);
  }

  UnicodeString HostName = Data->GetHostNameExpanded();
  size_t Port = Data->GetPortNumber();
  UnicodeString ProtocolName = !Ssl ? L"http" : L"https";
  UnicodeString UserName = Data->GetUserNameExpanded();
  UnicodeString Path = Data->GetRemoteDirectory();
  UnicodeString Url = FORMAT(L"%s://%s:%d%s", ProtocolName.c_str(), HostName.c_str(), Port, Path.c_str());

  FPasswordFailed = false;

  FTerminal->Information(LoadStr(STATUS_CONNECT), true);
  for (int I = 0; I < 5; I++)
  {
    FActive = false;
    try
    {
      FActive = (WEBDAV_NO_ERROR == OpenURL(Url, webdav_pool));
      if (FActive)
      {
        break;
      }
    }
    catch (ExtException & /*E*/)
    {
      throw;
    }
    catch (...)
    {
      if (webdav::cancelled || FFileTransferCancelled)
        break;
      apr_sleep(200000); // 0.2 sec
    }
  }
  if (!FActive)
  {
    FTerminal->Closed();
    throw Exception(LoadStr(CONNECTION_FAILED));
  }
}

void TWebDAVFileSystem::Close()
{
  DebugAssert(FActive);
  FTerminal->Closed();
  FActive = false;
}

void TWebDAVFileSystem::CollectUsage()
{
//  if (!FTlsVersionStr.IsEmpty())
//  {
//    FTerminal->CollectTlsUsage(FTlsVersionStr);
//  }
}

const TSessionInfo & TWebDAVFileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}

const TFileSystemInfo & TWebDAVFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  return FFileSystemInfo;
}

bool TWebDAVFileSystem::TemporaryTransferFile(const UnicodeString & /*AFileName*/)
{
  return false;
}

bool TWebDAVFileSystem::GetStoredCredentialsTried() const
{
  return false;
}

UnicodeString TWebDAVFileSystem::FSGetUserName() const
{
  return FUserName;
}

void TWebDAVFileSystem::GetSupportedChecksumAlgs(TStrings * /*Algs*/)
{
  // NOOP
}

void TWebDAVFileSystem::Idle()
{
  // TODO: Keep session alive
  return;
}

UnicodeString TWebDAVFileSystem::GetAbsolutePath(const UnicodeString & APath, bool Local)
{
  return static_cast<const TWebDAVFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TWebDAVFileSystem::GetAbsolutePath(const UnicodeString & APath, bool /*Local*/) const
{
  return core::AbsolutePath(GetCurrDirectory(), APath);
}

bool TWebDAVFileSystem::IsCapable(intptr_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
    case fcUserGroupListing:
    case fcModeChanging:
    case fcModeChangingUpload:
    case fcPreservingTimestampUpload:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcSymbolicLink:
    case fcResolveSymlink:
      return false;
    case fcRename:
    case fcRemoteMove:
    case fcRemoteCopy:
      return true;

    case fcTextMode:
    case fcNativeTextMode:
    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcLoadingAdditionalProperties:
    case fcCheckingSpaceAvailable:
    case fcIgnorePermErrors:
    case fcCalculatingChecksum:
    case fcSecondaryShell: // has fcShellAnyCommand
    case fcGroupOwnerChangingByID: // by name
    case fcRemoveCtrlZUpload:
    case fcRemoveBOMUpload:
      return false;

    default:
      DebugAssert(false);
      return false;
  }
}

void TWebDAVFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.IsEmpty())
  {
    FTerminal->LogEvent(FORMAT(L"Locating to cached directory \"%s\".",
      FCachedDirectoryChange.c_str()));
    UnicodeString Directory = FCachedDirectoryChange;
    FCachedDirectoryChange.Clear();
    try
    {
      ChangeDirectory(Directory);
    }
    catch (...)
    {
      // when location to cached directory fails, pretend again
      // location in cached directory
      // here used to be check (CurrentDirectory != Directory), but it is
      // false always (current directory is already set to cached directory),
      // making the condition below useless. check removed.
      if (FTerminal->GetActive())
      {
        FCachedDirectoryChange = Directory;
      }
      throw;
    }
  }
}

UnicodeString TWebDAVFileSystem::GetCurrDirectory() const
{
  return FCurrentDirectory;
}

void TWebDAVFileSystem::DoStartup()
{
  FTerminal->SetExceptionOnFail(true);
  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FTerminal->SetExceptionOnFail(false);
}

void TWebDAVFileSystem::LookupUsersGroups()
{
}

void TWebDAVFileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    FCurrentDirectory = FCurrentDirectory.IsEmpty() ? UnicodeString(ROOTDIRECTORY) : FCurrentDirectory;
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
  }
}

void TWebDAVFileSystem::HomeDirectory()
{
  Error(SNotImplemented, 1009);
}

void TWebDAVFileSystem::AnnounceFileListOperation()
{
  // noop
}

void TWebDAVFileSystem::DoChangeDirectory(const UnicodeString & /*Directory*/)
{
}

void TWebDAVFileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  UnicodeString Directory = ADirectory;
  bool HasTrailingSlash = (Directory.Length() > 0) && (Directory[Directory.Length()] == L'/');
  try
  {
    // For changing directory, we do not make paths absolute, instead we
    // delegate this to the server, hence we synchronize current working
    // directory with the server and only then we ask for the change with
    // relative path.
    // But if synchronization fails, typically because current working directory
    // no longer exists, we fall back to out own resolution, to give
    // user chance to leave the non-existing directory.
    EnsureLocation();
  }
  catch (...)
  {
    if (FTerminal->GetActive())
    {
      Directory = GetAbsolutePath(Directory, false);
      if (HasTrailingSlash)
        Directory = core::UnixIncludeTrailingBackslash(Directory);
    }
    else
    {
      throw;
    }
  }

  FCurrentDirectory = GetAbsolutePath(Directory, false);
  if (HasTrailingSlash)
    FCurrentDirectory = core::UnixIncludeTrailingBackslash(FCurrentDirectory);

  // make next ReadCurrentDirectory retrieve actual server-side current directory
  FCachedDirectoryChange.Clear();
}

void TWebDAVFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCachedDirectoryChange = core::UnixExcludeTrailingBackslash(Directory);
}

void TWebDAVFileSystem::DoReadDirectory(TRemoteFileList * FileList)
{
  FileList->Reset();
  // add parent directory
  FileList->AddFile(new TRemoteParentDirectory(FTerminal));

  FLastReadDirectoryProgress = 0;

  TWebDAVFileListHelper Helper(this, FileList, false);

  // always specify path to list, do not attempt to
  // list "current" dir as:
  // 1) List() lists again the last listed directory, not the current working directory
  // 2) we handle this way the cached directory change
  UnicodeString Directory = GetAbsolutePath(FileList->GetDirectory(), false);
  if (FHasTrailingSlash)
    Directory = core::UnixIncludeTrailingBackslash(Directory);
  WebDAVGetList(Directory);
}

void TWebDAVFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  DebugAssert(FileList);
  webdav::cancelled = 0;
  bool Repeat = false;

  do
  {
    Repeat = false;
    try
    {
      DoReadDirectory(FileList);
    }
    catch (Exception &)
    {
      if (!FTerminal->GetActive())
      {
        FTerminal->Reopen(ropNoReadDirectory);
        Repeat = true;
      }
      else
      {
        throw;
      }
    }
  }
  while (Repeat);
}

void TWebDAVFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
}

void TWebDAVFileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& File)
{
  CustomReadFile(AFileName, File, nullptr);
}

void TWebDAVFileSystem::CustomReadFile(const UnicodeString & AFileName,
  TRemoteFile *& File, TRemoteFile * /*ALinkedByFile*/)
{
  File = nullptr;
  int IsDir = 0;
  bool isExist = WebDAVCheckExisting(AFileName.c_str(), IsDir);
  if (isExist)
  {
    File = new TRemoteFile();
    if (IsDir)
      File->SetType(FILETYPE_DIRECTORY);
  }
}

void TWebDAVFileSystem::RemoteDeleteFile(const UnicodeString & /*AFileName*/,
  const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & /*Action*/)
{
  DebugUsedParam(AFile);
  DebugUsedParam(Params);
  UnicodeString FullFileName = AFile->GetFullFileName();
  bool res = WebDAVDeleteFile(FullFileName.c_str());
  if (!res)
  {
    ThrowSkipFileNull();
  }
}

void TWebDAVFileSystem::RemoteRenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  UnicodeString FullFileName = core::UnixIncludeTrailingBackslash(FCurrentDirectory) + AFileName;
  bool res = WebDAVRenameFile(FullFileName.c_str(), ANewName.c_str());
  if (!res)
  {
    ThrowSkipFileNull();
  }
}

void TWebDAVFileSystem::RemoteCopyFile(const UnicodeString & /*AFileName*/,
  const UnicodeString & /*ANewName*/)
{
  Error(SNotImplemented, 1012);
}

void TWebDAVFileSystem::RemoteCreateDirectory(const UnicodeString & ADirName)
{
  UnicodeString FullDirName = GetAbsolutePath(ADirName, true);
  bool res = WebDAVMakeDirectory(FullDirName.c_str());
  if (!res)
  {
    TStringList Strings;
    Strings.SetDelimiter(L'/');
    Strings.SetDelimitedText(ADirName);
    UnicodeString CurDir;
    for (intptr_t Index = 0; Index < Strings.GetCount(); ++Index)
    {
      if (Strings.GetString(Index).IsEmpty())
      {
        continue;
      }
      CurDir += L"/" + Strings.GetString(Index);
      res = WebDAVMakeDirectory(CurDir.c_str());
    }
    if (!res)
    {
      ThrowSkipFile(nullptr, L"");
    }
  }
}

void TWebDAVFileSystem::CreateLink(const UnicodeString & /*AFileName*/,
  const UnicodeString & /*PointTo*/, bool /*Symbolic*/)
{
  Error(SNotImplemented, 1014);
}

void TWebDAVFileSystem::ChangeFileProperties(const UnicodeString & /*AFileName*/,
  const TRemoteFile * /*AFile*/, const TRemoteProperties * /*Properties*/,
  TChmodSessionAction & /*Action*/)
{
  Error(SNotImplemented, 1006);
//  DebugAssert(Properties);
}

bool TWebDAVFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugAssert(false);
  return false;
}

void TWebDAVFileSystem::CalculateFilesChecksum(const UnicodeString & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  DebugAssert(false);
}

bool TWebDAVFileSystem::ConfirmOverwrite(
  const UnicodeString & ASourceFullFileName, UnicodeString & ADestFileName,
  TFileOperationProgressType * OperationProgress,
  const TOverwriteFileParams * FileParams,
  const TCopyParamType * CopyParam, intptr_t Params,
  bool AutoResume,
  OUT TOverwriteMode & OverwriteMode,
  OUT uintptr_t & Answer)
{
  bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  bool CanResume = false; // disable resume

  Answer = 0;
  if (CanAutoResume && CanResume)
  {
    Answer = qaRetry;
  }
  else
  {
    // retry = "resume"
    // all = "yes to newer"
    // ignore = "rename"
    uintptr_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;
    if (CanResume)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(RESUME_BUTTON);
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = _countof(Aliases);

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      Answer = FTerminal->ConfirmFileOverwrite(
         ASourceFullFileName, ADestFileName, FileParams,
         Answers, &QueryParams,
         OperationProgress->Side == osLocal ? osRemote : osLocal,
         CopyParam, Params, OperationProgress);
    }
  }

  bool Result = true;

  switch (Answer)
  {
      // resume
    case qaRetry:
      OverwriteMode = omResume;
      DebugAssert(FileParams != nullptr);
      DebugAssert(CanResume);
      FFileTransferResumed = FileParams->DestSize;
      break;

      // rename
    case qaIgnore:
      if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName,
            LoadStr(RENAME_TITLE), L"", LoadStr(RENAME_PROMPT2), true, 0, ADestFileName))
      {
        OverwriteMode = omOverwrite;
      }
      else
      {
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
        FFileTransferAbort = ftaCancel;
        Result = false;
      }
      break;

    case qaYes:
      OverwriteMode = omOverwrite;
      break;

    case qaNo:
      FFileTransferAbort = ftaSkip;
      Result = false;
      break;

    case qaCancel:
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      FFileTransferAbort = ftaCancel;
      Result = false;
      break;

    default:
      DebugAssert(false);
      Result = false;
      break;
  }
  return Result;
}

void TWebDAVFileSystem::CustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent)
{
  DebugAssert(AFile);
  bool Dir = AFile->GetIsDirectory() && !AFile->GetIsSymLink();
  if (Dir && (Params & ccRecursive))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TTerminal::CustomCommandOnFile, FTerminal), &AParams);
  }

  if (!Dir || (Params & ccApplyToDirectories))
  {
    TCustomCommandData Data(FTerminal);
    UnicodeString Cmd = TRemoteCustomCommand(
      Data, FTerminal->GetCurrDirectory(), AFileName, L"").
      Complete(Command, true);
  }
}

void TWebDAVFileSystem::AnyCommand(const UnicodeString & /*Command*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  Error(SNotImplemented, 1008);
}

TStrings * TWebDAVFileSystem::GetFixedPaths()
{
  return nullptr;
}

void TWebDAVFileSystem::SpaceAvailable(const UnicodeString & /*APath*/,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  DebugAssert(false);
}

void TWebDAVFileSystem::CopyToRemote(const TStrings * AFilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  Params &= ~cpAppend;
  UnicodeString FileName, FileNameOnly;
  UnicodeString TargetDir = GetAbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = core::UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && !OperationProgress->Cancel)
  {
    FileName = AFilesToCopy->GetString(Index);
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFilesToCopy->GetObj(Index));
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;
    FileNameOnly = base::ExtractFileName(RealFileName, false);

    {
      bool Success = false;
      SCOPE_EXIT
      {
        OperationProgress->Finish(RealFileName, Success, OnceDoneOperation);
      };
      try
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (::DirectoryExists(ApiPath(::ExtractFilePath(FileName))))
          {
            FTerminal->DirectoryModified(FullTargetDir + FileNameOnly, true);
          }
        }
        WebDAVSourceRobust(FileName, File, FullTargetDir, CopyParam, Params, OperationProgress,
          tfFirstLevel);
        Success = true;
      }
      catch (ESkipFile & E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    ++Index;
  }
}

void TWebDAVFileSystem::WebDAVSourceRobust(const UnicodeString & AFileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  bool Retry = false;

  TUploadSessionAction Action(FTerminal->GetActionLog());

  do
  {
    Retry = false;
    try
    {
      WebDAVSource(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action);
    }
    catch (Exception & E)
    {
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);
        throw;
      }
    }

    if (Retry)
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite confirmations
      // (should not be set for directories!)
      Params |= cpNoConfirmation;
      Flags |= tfAutoResume;
    }
  }
  while (Retry);
}

void TWebDAVFileSystem::WebDAVSource(const UnicodeString & AFileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TUploadSessionAction & Action)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : AFileName;
  Action.SetFileName(::ExpandUNCFileName(RealFileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(AFileName, CopyParam, OperationProgress))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", RealFileName.c_str()));
    ThrowSkipFileNull();
  }

  int64_t Size = 0;
  uintptr_t LocalFileAttrs = 0;

  FTerminal->OpenLocalFile(AFileName, GENERIC_READ,
    nullptr, &LocalFileAttrs, nullptr, nullptr, nullptr, &Size);

  OperationProgress->SetFileInProgress();

  bool Dir = FLAGSET(LocalFileAttrs, faDirectory);
  if (Dir)
  {
    Action.Cancel();
    WebDAVDirectorySource(::IncludeTrailingBackslash(AFileName), TargetDir,
      LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
  }
  else
  {
    UnicodeString DestFileName = CopyParam->ChangeFileName(base::ExtractFileName(RealFileName, false),
      osLocal, FLAGSET(Flags, tfFirstLevel));

    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", RealFileName.c_str()));

    OperationProgress->SetLocalSize(Size);

    // Suppose same data size to transfer as to read
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(OperationProgress->LocalSize);
    OperationProgress->TransferingFile = false;

    // Will we use ASCII of BINARY file transfer?
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;

    ResetFileTransfer();

    TFileTransferData UserData;

    {
      uint32_t TransferType = 2; // OperationProgress->AsciiTransfer = false
      // ignore file list
      TWebDAVFileListHelper Helper(this, nullptr, true);

      FFileTransferCPSLimit = OperationProgress->CPSLimit;
      // not used for uploads anyway
      FFileTransferPreserveTime = CopyParam->GetPreserveTime();
      // not used for uploads, but we get new name (if any) back in this field
      UserData.FileName = DestFileName;
      UserData.Params = Params;
      UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
      UserData.CopyParam = CopyParam;
      FileTransfer(RealFileName, AFileName, DestFileName,
        TargetDir, false, Size, TransferType, UserData, OperationProgress);
    }

    UnicodeString DestFullName = TargetDir + UserData.FileName;
    // only now, we know the final destination
    Action.Destination(DestFullName);
  }

  // TODO : Delete also read-only files.
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(::RemoveFile(AFileName));
      });
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
  {
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, AFileName.c_str()), "",
    [&]()
    {
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(AFileName), LocalFileAttrs & ~faArchive) == 0);
    });
  }
}

void TWebDAVFileSystem::WebDAVDirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, uintptr_t Attrs, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  UnicodeString DestDirectoryName = CopyParam->ChangeFileName(
    base::ExtractFileName(::ExcludeTrailingBackslash(DirectoryName), false), osLocal,
    FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = core::UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);
  // create DestFullName if it does not exist
  if (!FTerminal->FileExists(DestFullName))
  {
    TRemoteProperties Properties;
    if (CopyParam->GetPreserveRights())
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }
    FTerminal->RemoteCreateDirectory(DestFullName, &Properties);
  }

  OperationProgress->SetFile(DirectoryName);

  WIN32_FIND_DATA SearchRec;
  bool FindOK = false;
  HANDLE FindHandle = INVALID_HANDLE_VALUE;

  UnicodeString FindPath = DirectoryName + L"*.*";

  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()), "",
  [&]()
  {
    FindHandle = ::FindFirstFile(ApiPath(FindPath).c_str(), &SearchRec);
    FindOK = FindHandle != INVALID_HANDLE_VALUE;
    if (!FindOK)
    {
      FindCheck(::GetLastError(), FindPath);
    }
  });

  bool CreateDir = true;

  {
    SCOPE_EXIT
    {
      ::FindClose(FindHandle);
    };
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = DirectoryName + SearchRec.cFileName;
      try
      {
        if ((wcscmp(SearchRec.cFileName, THISDIRECTORY) != 0) && (wcscmp(SearchRec.cFileName, PARENTDIRECTORY) != 0))
        {
          WebDAVSourceRobust(FileName, nullptr, DestFullName, CopyParam, Params, OperationProgress,
            Flags & ~(tfFirstLevel | tfAutoResume));
          // if any file got uploaded (i.e. there were any file in the
          // directory and at least one was not skipped),
          // do not try to create the directory,
          // as it should be already created by FZAPI during upload
          CreateDir = false;
        }
      }
      catch (ESkipFile & E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        TSuspendFileOperationProgress Suspend(OperationProgress);
        // here a message to user was displayed, which was not appropriate
        // when user refused to overwrite the file in subdirectory.
        // hopefully it won't be missing in other situations.
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }

      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()), "",
      [&]()
      {
        FindOK = ::FindNextFile(FindHandle, &SearchRec) != FALSE;
        if (!FindOK)
        {
          ::FindCheck(::GetLastError(), FindPath);
        }
      });
    }
  }
  if (CreateDir)
  {
    TRemoteProperties Properties;
    if (CopyParam->GetPreserveRights())
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }

    try
    {
      FTerminal->SetExceptionOnFail(true);
      {
        SCOPE_EXIT
        {
          FTerminal->SetExceptionOnFail(false);
        };
        FTerminal->RemoteCreateDirectory(DestFullName, &Properties);
      }
    }
    catch (...)
    {
      TRemoteFile * File = nullptr;
      // ignore non-fatal error when the directory already exists
      UnicodeString fn = core::UnixExcludeTrailingBackslash(DestFullName);
      if (fn.IsEmpty())
      {
        fn = ROOTDIRECTORY;
      }
      bool Rethrow =
        !FTerminal->GetActive() ||
        !FTerminal->FileExists(fn, &File) ||
        (File && !File->GetIsDirectory());
      SAFE_DESTROY(File);
      if (Rethrow)
      {
        throw;
      }
    }
  }

  // TODO : Delete also read-only directories.
  // TODO : Show error message on failure.
  if (!OperationProgress->Cancel)
  {
    if (FLAGSET(Params, cpDelete))
    {
      FTerminal->RemoveLocalDirectory(ApiPath(DirectoryName));
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DirectoryName), Attrs & ~faArchive) == 0);
      });
    }
  }
}

void TWebDAVFileSystem::CopyToLocal(const TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  Params &= ~cpAppend;
  UnicodeString FullTargetDir = ::IncludeTrailingBackslash(TargetDir);

  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile * File = NB_STATIC_DOWNCAST_CONST(TRemoteFile, AFilesToCopy->GetObj(Index));
    FTerminal->SetExceptionOnFail(true);
    {
      bool Success = false;
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
        FTerminal->SetExceptionOnFail(false);
      };
      UnicodeString AbsoluteFilePath = GetAbsolutePath(FileName, false);
      UnicodeString TargetDirectory = CreateTargetDirectory(File->GetFileName(), FullTargetDir, CopyParam);
      try
      {
        SinkRobust(AbsoluteFilePath, File, TargetDirectory, CopyParam, Params,
          OperationProgress, tfFirstLevel);
        Success = true;
      }
      catch (ESkipFile & E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    ++Index;
  }
}

void TWebDAVFileSystem::SinkRobust(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem
  bool Retry;

  TDownloadSessionAction Action(FTerminal->GetActionLog());

  do
  {
    Retry = false;
    try
    {
      Sink(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action);
    }
    catch (Exception & E)
    {
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);
        throw;
      }
    }

    if (Retry)
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      DebugAssert(AFile != nullptr);
      if (!AFile->GetIsDirectory())
      {
        // prevent overwrite confirmations
        Params |= cpNoConfirmation;
        Flags |= tfAutoResume;
      }
    }
  }
  while (Retry);
}

void TWebDAVFileSystem::Sink(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction & Action)
{
  UnicodeString FileNameOnly = base::UnixExtractFileName(AFileName);

  Action.SetFileName(AFileName);

  DebugAssert(AFile);
  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();

  if (!CopyParam->AllowTransfer(AFileName, osRemote, AFile->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", AFileName.c_str()));
    ThrowSkipFileNull();
  }

  FTerminal->LogFileDetails(AFileName, TDateTime(), AFile->GetSize());

  OperationProgress->SetFile(FileNameOnly);

  UnicodeString DestFileName = CopyParam->ChangeFileName(base::UnixExtractFileName(AFile->GetFileName()),
    osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    bool CanProceed = true;
    if (::DirectoryExists(DestFullName))
    {
      uintptr_t Answer = 0;
      UnicodeString Message = FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly.c_str());
      TQueryParams QueryParams(qpNeverAskAgainCheck);

      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        Answer = FTerminal->ConfirmFileOverwrite(
          AFileName, DestFileName, nullptr,
          qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
          &QueryParams, osRemote, CopyParam, Params, OperationProgress, Message);
      }
      switch (Answer)
      {
        case qaCancel:
          OperationProgress->Cancel = csCancel; // continue on next case
          // FALLTHROUGH
        case qaNo:
          CanProceed = false;
        default:
          break;
      }
    }
    if (CanProceed)
    {
      Action.Cancel();
      if (!AFile->GetIsSymLink())
      {
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()), "",
        [&]()
        {
          DWORD LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
          if (FLAGCLEAR(LocalFileAttrs, faDirectory))
          {
            ThrowExtException();
          }
        });

        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()), "",
        [&]()
        {
          THROWOSIFFALSE(::ForceDirectories(ApiPath(DestFullName)));
        });

        TSinkFileParams SinkFileParams;
        SinkFileParams.TargetDir = ApiPath(::IncludeTrailingBackslash(DestFullName));
        SinkFileParams.CopyParam = CopyParam;
        SinkFileParams.Params = Params;
        SinkFileParams.OperationProgress = OperationProgress;
        SinkFileParams.Skipped = false;
        SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

        FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TWebDAVFileSystem::SinkFile, this), &SinkFileParams);

        // Do not delete directory if some of its files were skipped.
        // Throw "skip file" for the directory to avoid attempt to deletion
        // of any parent directory
        if (FLAGSET(Params, cpDelete) && SinkFileParams.Skipped)
        {
          ThrowSkipFileNull();
        }
      }
      else
      {
        // file is symlink to directory, currently do nothing, but it should be
        // reported to user
      }
    }
  }
  else
  {
    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to local directory started.", AFileName.c_str()));
    bool CanProceed = true;
    if (::FileExists(ApiPath(DestFullName)))
    {
      int64_t Size = 0;
      int64_t MTime = 0;
      FTerminal->OpenLocalFile(DestFullName, GENERIC_READ,
        nullptr, nullptr, nullptr, &MTime, nullptr, &Size);
      TOverwriteFileParams FileParams;

      FileParams.SourceSize = AFile->GetSize();
      FileParams.SourceTimestamp = AFile->GetModification();
      FileParams.DestSize = Size;
      FileParams.DestTimestamp = ::UnixToDateTime(MTime,
        FTerminal->GetSessionData()->GetDSTMode());

      uintptr_t Answer = 0;
      TOverwriteMode OverwriteMode = omOverwrite;
      bool AutoResume = false;
      ConfirmOverwrite(AFileName, DestFullName, OperationProgress,
          &FileParams, CopyParam, Params, AutoResume,
          OverwriteMode, Answer);
      switch (Answer)
      {
        case qaCancel:
          OperationProgress->Cancel = csCancel; // continue on next case
          // FALLTHROUGH
        case qaNo:
          CanProceed = false;
        default:
          break;
      }
    }
    if (CanProceed)
    {
      // Suppose same data size to transfer as to write
      OperationProgress->SetTransferSize(AFile->GetSize());
      OperationProgress->SetLocalSize(OperationProgress->TransferSize);

      DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()), "",
      [&]()
      {
        LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
        if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, faDirectory))
        {
          ThrowExtException();
        }
      });

      OperationProgress->TransferingFile = false; // not set with FTP protocol

      ResetFileTransfer();

      TFileTransferData UserData;

      UnicodeString FilePath = core::UnixExtractFilePath(AFileName);
      if (FilePath.IsEmpty())
      {
        FilePath = ROOTDIRECTORY;
      }

      {
        int TransferType = 2; // OperationProgress->AsciiTransfer = false
        // ignore file list
        TWebDAVFileListHelper Helper(this, nullptr, true);

        FFileTransferCPSLimit = OperationProgress->CPSLimit;
        FFileTransferPreserveTime = CopyParam->GetPreserveTime();
        UserData.FileName = DestFileName;
        UserData.Params = Params;
        UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
        UserData.CopyParam = CopyParam;
        FileTransfer(AFileName, DestFullName, FileNameOnly,
          FilePath, true, AFile->GetSize(), TransferType, UserData, OperationProgress);
      }

      // in case dest filename is changed from overwrite dialog
      if (DestFileName != UserData.FileName)
      {
        DestFullName = TargetDir + UserData.FileName;
        LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
      }

      Action.Destination(::ExpandUNCFileName(DestFullName));

      if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
      {
        LocalFileAttrs = faArchive;
      }
      DWORD NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
      if ((NewAttrs & LocalFileAttrs) != NewAttrs)
      {
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()), "",
        [&]()
        {
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DestFullName), LocalFileAttrs | NewAttrs) == 0);
        });
      }
      // set time
      FTerminal->SetLocalFileTime(DestFullName, AFile->GetModification());
    }
  }

  if (FLAGSET(Params, cpDelete))
  {
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params = dfNoRecursive;
    FTerminal->RemoteDeleteFile(AFileName, AFile, &Params);
  }
}

void TWebDAVFileSystem::SinkFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * Param)
{
  TSinkFileParams * Params = NB_STATIC_DOWNCAST(TSinkFileParams, Param);
  DebugAssert(Params->OperationProgress);
  try
  {
    SinkRobust(AFileName, AFile, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch (ESkipFile & E)
  {
    TFileOperationProgressType * OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      if (!FTerminal->HandleException(&E))
      {
        throw;
      }
    }

    if (OperationProgress->Cancel)
    {
      Abort();
    }
  }
}

bool TWebDAVFileSystem::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, intptr_t Count)
{
  if (!FActive)
  {
    return false;
  }
  else if (FIgnoreFileList)
  {
    // directory listing provided implicitly by FZAPI during certain operations is ignored
    DebugAssert(FFileList == nullptr);
    return false;
  }
  else
  {
    DebugAssert(FFileList != nullptr);
    // this can actually fail in real life,
    // when connected to server with case insensitive paths
    UnicodeString AbsolutePath = GetAbsolutePath(FFileList->GetDirectory(), false);
    DebugAssert(core::UnixSamePath(AbsolutePath, Path));
    DebugUsedParam(Path);

    for (intptr_t Index = 0; Index < Count; ++Index)
    {
      const TListDataEntry * Entry = &Entries[Index];
      std::unique_ptr<TRemoteFile> File(new TRemoteFile());
      try
      {
        File->SetTerminal(FTerminal);

        File->SetFileName(UnicodeString(Entry->Name));
        if (wcslen(Entry->Permissions) >= 10)
        {
          try
          {
            File->GetRights()->SetText(Entry->Permissions + 1);
          }
          catch (...)
          {
            // ignore permissions errors with WebDAV
          }
        }
        // FIXME
        UnicodeString Own = Entry->OwnerGroup;
        const wchar_t * Space = wcschr(Own.c_str(), ' ');
        if (Space != nullptr)
        {
          File->GetFileOwner().SetName(UnicodeString(Own.c_str(), (intptr_t)(Space - Own.c_str())));
          File->GetFileGroup().SetName(Space + 1);
        }
        else
        {
          File->GetFileOwner().SetName(Entry->OwnerGroup);
        }

        File->SetSize(Entry->Size);

        if (Entry->Link)
        {
          File->SetType(FILETYPE_SYMLINK);
        }
        else if (Entry->Dir)
        {
          File->SetType(FILETYPE_DIRECTORY);
        }
        else
        {
          File->SetType(L'-');
        }

        // ModificationFmt must be set after Modification
        if (Entry->Time.HasDate)
        {
          // should be the same as ConvertRemoteTimestamp
          TDateTime Modification =
            EncodeDateVerbose(static_cast<uint16_t>(Entry->Time.Year), static_cast<uint16_t>(Entry->Time.Month),
              static_cast<uint16_t>(Entry->Time.Day));
          if (Entry->Time.HasTime)
          {
            uint16_t seconds = 0;
            if (Entry->Time.HasSeconds)
              seconds = static_cast<uint16_t>(Entry->Time.Second);
            File->SetModification(Modification +
              EncodeTimeVerbose(static_cast<uint16_t>(Entry->Time.Hour),
                static_cast<uint16_t>(Entry->Time.Minute),
                seconds, 0));
            // not exact as we got year as well, but it is most probably
            // guessed by FZAPI anyway
            File->SetModificationFmt(mfMDHM);
          }
          else
          {
            File->SetModification(Modification);
            File->SetModificationFmt(mfMDY);
          }
        }
        else
        {
          // We estimate date to be today, if we have at least time
          File->SetModification(TDateTime(0.0));
          File->SetModificationFmt(mfNone);
        }
        File->SetLastAccess(File->GetModification());

        File->SetLinkTo(Entry->LinkTarget);

        File->Complete();
      }
      catch (Exception & E)
      {
        UnicodeString EntryData =
          FORMAT(L"%s/%s/%s/%lld/%d/%d/%d/%d/%d/%d/%d/%d/%d",
                 Entry->Name,
                 Entry->Permissions,
                 Entry->OwnerGroup,
                 Entry->Size,
                 int(Entry->Dir), int(Entry->Link), Entry->Time.Year, Entry->Time.Month, Entry->Time.Day,
                 Entry->Time.Hour, Entry->Time.Minute, int(Entry->Time.HasTime), int(Entry->Time.HasDate));
        throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, EntryData.c_str()), HELP_LIST_LINE_ERROR);
      }

      FFileList->AddFile(File.release());
    }
    return true;
  }
}

void TWebDAVFileSystem::ResetFileTransfer()
{
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
  webdav::cancelled = 0;
}

void TWebDAVFileSystem::ReadDirectoryProgress(int64_t Bytes)
{
  // with WebDAV we do not know exactly how many entries we have received,
  // instead we know number of bytes received only.
  // so we report approximation based on average size of entry.
  int Progress = static_cast<int>(Bytes / 80);
  if (Progress - FLastReadDirectoryProgress >= 10)
  {
    bool Cancel = false;
    FLastReadDirectoryProgress = Progress;
    FTerminal->DoReadDirectoryProgress(Progress, 0, Cancel);
    if (Cancel)
    {
      FTerminal->DoReadDirectoryProgress(-2, 0, Cancel);
    }
  }
}

void TWebDAVFileSystem::DoFileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();
  if (!OperationProgress)
  {
    return;
  }

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    OperationProgress->AddResumed(FFileTransferResumed);
    FFileTransferResumed = 0;
  }

  int64_t Diff = Bytes - OperationProgress->TransferedSize;
  if (Diff >= 0)
  {
    OperationProgress->AddTransfered(Diff);
  }

  if (!FFileTransferCancelled && OperationProgress->Cancel == csCancel)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    webdav::cancelled = 1;
  }

  if (FFileTransferCPSLimit != OperationProgress->CPSLimit)
  {
    FFileTransferCPSLimit = OperationProgress->CPSLimit;
  }
}

void TWebDAVFileSystem::FileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TGuard Guard(FTransferStatusCriticalSection);

  DoFileTransferProgress(TransferSize, Bytes);
}

void TWebDAVFileSystem::FileTransfer(const UnicodeString & AFileName,
  const UnicodeString & LocalFile, const UnicodeString & RemoteFile,
  const UnicodeString & RemotePath, bool Get, int64_t Size, int /*Type*/,
  TFileTransferData & /*UserData*/, TFileOperationProgressType * OperationProgress)
{
  FCurrentOperationProgress = OperationProgress;
  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(TRANSFER_ERROR, AFileName.c_str()), "",
  [&]()
  {
    UnicodeString FullRemoteFileName = RemotePath + RemoteFile;
    bool Result;
    if (Get)
    {
      Result = WebDAVGetFile(FullRemoteFileName.c_str(), LocalFile.c_str());
    }
    else
    {
      Result = WebDAVPutFile(FullRemoteFileName.c_str(), LocalFile.c_str(), Size);
    }
    // if (!Result)
      // ThrowExtException();
  });

  switch (FFileTransferAbort)
  {
    case ftaSkip:
      ThrowSkipFile(nullptr, L"");

    case ftaCancel:
      Abort();
      break;
  }

  if (!FFileTransferCancelled)
  {
    // show completion of transfer
    // call non-guarded variant to avoid deadlock with keepalives
    // (we are not waiting for reply anymore so keepalives are free to proceed)
    DoFileTransferProgress(OperationProgress->TransferSize, OperationProgress->TransferSize);
  }
}

bool TWebDAVFileSystem::SendPropFindRequest(const wchar_t * Path, int & ResponseCode)
{
  DebugAssert(Path);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_send_propfind_request(
    FSession,
    remote_path,
    &ResponseCode,
    pool);

  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVCheckExisting(const wchar_t * Path, int & IsDir)
{
  DebugAssert(Path);
  IsDir = 0;
  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  webdav::node_kind_t kind = webdav::node_none;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_check_path(
    FSession,
    remote_path,
    &kind,
    pool);

  if (kind != webdav::node_none)
    IsDir = kind == webdav::node_dir;
  webdav_pool_destroy(pool);
  return (err == WEBDAV_NO_ERROR) && (kind != webdav::node_none);
}

bool TWebDAVFileSystem::WebDAVMakeDirectory(const wchar_t * Path)
{
  DebugAssert(Path);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_make_directory(
    FSession,
    remote_path,
    nullptr,
    pool);
  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVGetList(const UnicodeString & Directory)
{
  webdav::listdataentry_vector_t Entries;

  DebugAssert(FSession);
  webdav::list_func_baton_t baton = {0};
  baton.verbose = true;
  baton.entries = &Entries;
  baton.session = FSession;
  baton.pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Directory), baton.pool);
  if (err)
    return false;
  err = webdav::client_list(
    FSession,
    remote_path,
    webdav::depth_immediates,
    WEBDAV_DIRENT_ALL,
    webdav::list_func,
    &baton,
    baton.pool);

  TListDataEntry * pEntries = !Entries.empty() ? &Entries[0] : nullptr;
  HandleListData(Directory.c_str(), pEntries, Entries.size());
  webdav_pool_destroy(baton.pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVGetFile(
  const wchar_t * RemotePath, const wchar_t * LocalPath)
{
  DebugAssert(RemotePath && *RemotePath);
  DebugAssert(FSession);

  bool Result = false;
  HANDLE LocalFileHandle = FTerminal->CreateLocalFile(LocalPath,
    GENERIC_WRITE, 0, CREATE_ALWAYS, 0);
  if (LocalFileHandle != INVALID_HANDLE_VALUE)
  try
  {
    apr_pool_t * pool = webdav_pool_create(webdav_pool);
    webdav::error_t err = WEBDAV_NO_ERROR;
    const char * remote_path = nullptr;
    err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(RemotePath), pool);
    if (err)
    {
      ::CloseHandle(LocalFileHandle);
      return false;
    }
    err = webdav::client_get_file(
      FSession,
      remote_path,
      &LocalFileHandle,
      pool);

    webdav_pool_destroy(pool);
    Result = err == WEBDAV_NO_ERROR;
    if (!Result)
    {
      ::CloseHandle(LocalFileHandle);
      LocalFileHandle = INVALID_HANDLE_VALUE;
    }
  }
  catch (...)
  {
    if (LocalFileHandle != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(LocalFileHandle);
      LocalFileHandle = INVALID_HANDLE_VALUE;
    }
    Result = false;
    throw;
  }

  return Result;
}

bool TWebDAVFileSystem::WebDAVPutFile(const wchar_t * RemotePath,
  const wchar_t * LocalPath, const uint64_t /*FileSize*/)
{
  DebugAssert(RemotePath && *RemotePath);
  DebugAssert(LocalPath && *LocalPath);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  const char * local_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(RemotePath), pool);
  if (err)
    return false;
  err = webdav::path_cstring_to_utf8(&local_path, StrToNeon(ApiPath(LocalPath)), pool);
  if (err)
    return false;
  try
  {
    err = webdav::client_put_file(
      FSession,
      remote_path,
      local_path,
      pool);
  }
  catch (ESkipFile &)
  {
    err = WEBDAV_ERR_CANCELLED;
  }

  webdav_pool_destroy(pool);
  if (err == WEBDAV_ERR_CANCELLED)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
  }
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVRenameFile(const wchar_t * SrcPath, const wchar_t * DstPath)
{
  DebugAssert(SrcPath && *SrcPath);
  DebugAssert(DstPath && *DstPath);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * src_path = nullptr;
  const char * dst_path = nullptr;
  err = webdav::path_cstring_to_utf8(&src_path, StrToNeon(SrcPath), pool);
  if (err)
    return false;
  err = webdav::path_cstring_to_utf8(&dst_path, StrToNeon(DstPath), pool);
  if (err)
    return false;
  err = webdav::client_move_file_or_directory(
    FSession,
    src_path,
    dst_path,
    nullptr,
    pool);

  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVDeleteFile(const wchar_t * Path)
{
  DebugAssert(Path);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_delete_file(
    FSession,
    remote_path,
    nullptr,
    pool);

  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::OpenURL(
  const UnicodeString & SessionURL,
  apr_pool_t * pool)
{
  webdav::client_ctx_t * ctx = nullptr;
  WEBDAV_ERR(client_create_context(&ctx, pool));

  const char * auth_username = nullptr;
  const char * auth_password = nullptr;
  WEBDAV_ERR(webdav::utf_cstring_to_utf8(&auth_username,
    StrToNeon(FTerminal->GetSessionData()->GetUserNameExpanded()), pool));
  WEBDAV_ERR(webdav::utf_cstring_to_utf8(&auth_password,
    StrToNeon(FTerminal->GetSessionData()->GetPassword()), pool));
  webdav::auth_baton_t * ab = nullptr;
  webdav::auth_baton_create(&ab, pool);
  webdav::auth_baton_init(
    ab,
    false, // non_interactive
    auth_username,
    auth_password,
    false, // no_auth_cache
    true, // trust_server_cert
    this,
    webdav::check_cancel, ab,
    pool);
  ctx->auth_baton = ab;

  // Set up our cancellation support.
  ctx->cancel_func = webdav::check_cancel;
  ctx->cancel_baton = ab;

  ctx->progress_func = webdav::progress_func;
  ctx->progress_baton = ctx;

  webdav::session_t * session_p = nullptr;
  const char * corrected_url = nullptr;
  UTF8String base_url(SessionURL);
  const char * base_url_encoded = webdav::path_uri_encode(base_url.c_str(), pool);
  WEBDAV_ERR(webdav::client_open_session_internal(
    &session_p,
    &corrected_url,
    base_url_encoded,
    ctx,
    pool));

  const char * url = nullptr;
  if (corrected_url)
  {
    url = apr_pstrdup(pool, corrected_url);
  }
  else
  {
    url = apr_pstrdup(pool, base_url_encoded);
  }
  ne_uri * uri = nullptr;
  if (WEBDAV_NO_ERROR == webdav::parse_ne_uri(&uri, url, pool))
  {
    FCurrentDirectory = uri->path;
    FHasTrailingSlash = (FCurrentDirectory.Length() > 0) && (FCurrentDirectory[FCurrentDirectory.Length()] == L'/');
  }
  FSession = session_p;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::GetServerSettings(
  int * proxy_method,
  const char ** proxy_host,
  uint32_t * proxy_port,
  const char ** proxy_username,
  const char ** proxy_password,
  int * timeout_seconds,
  int * neon_debug,
  const char ** neon_debug_file_name,
  bool * compression,
  const char ** pk11_provider,
  const char ** ssl_authority_file,
  apr_pool_t * pool)
{
  // If we find nothing, default to nulls.
  *proxy_method = 0;
  *proxy_host = nullptr;
  *proxy_port = static_cast<uint32_t>(-1);
  *proxy_username = nullptr;
  *proxy_password = nullptr;
  *pk11_provider = nullptr;
  *ssl_authority_file = nullptr;

  TSessionData * Data = FTerminal->GetSessionData();
  TConfiguration * Configuration = FTerminal->GetConfiguration();
  {
    TProxyMethod ProxyMethod = Data->GetProxyMethod();
    *proxy_method = static_cast<int>(ProxyMethod);
    if (ProxyMethod != ::pmNone)
    {
      WEBDAV_ERR(webdav::path_cstring_to_utf8(proxy_host, StrToNeon(Data->GetProxyHost()), pool));
      WEBDAV_ERR(webdav::path_cstring_to_utf8(proxy_username, StrToNeon(Data->GetProxyUsername()), pool));
      WEBDAV_ERR(webdav::path_cstring_to_utf8(proxy_password, StrToNeon(Data->GetProxyPassword()), pool));
    }
  }

  // Apply non-proxy-specific settings regardless of exceptions:
  if (compression)
    *compression = Data->GetCompression();

  int l_debug = Configuration->GetActualLogProtocol() >= 1 ? 1 : 0;
  *pk11_provider = "";

  *ssl_authority_file = apr_pstrdup(pool, StrToNeon(Data->GetPublicKeyFile()));

  {
    intptr_t l_proxy_port = Data->GetProxyPort();
    if (l_proxy_port < 0)
    {
      return webdav::error_create(WEBDAV_ERR_ILLEGAL_URL, nullptr,
        "Invalid URL: negative proxy port number");
    }
    if (l_proxy_port > 65535)
    {
      return webdav::error_create(WEBDAV_ERR_ILLEGAL_URL, nullptr,
        "Invalid URL: proxy port number greater "
        "than maximum TCP port number 65535");
    }
    *proxy_port = static_cast<uint32_t>(l_proxy_port);
  }

  {
    intptr_t l_timeout = Data->GetTimeout();
    if (l_timeout < 0)
      return webdav::error_create(WEBDAV_ERR_BAD_CONFIG_VALUE, nullptr,
        "Invalid config: negative timeout value");
    *timeout_seconds = static_cast<int>(l_timeout);
  }

  if (l_debug)
  {
    *neon_debug = l_debug;
    if (Configuration->GetLogToFile())
    {
      WEBDAV_ERR(webdav::path_cstring_to_utf8(neon_debug_file_name,
        StrToNeon(GetExpandedLogFileName(Configuration->GetLogFileName(), Data)), pool));
    }
    else
    {
      *neon_debug_file_name = nullptr;
    }
  }
  else
  {
    *neon_debug = 0;
    *neon_debug_file_name = nullptr;
  }

  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::VerifyCertificate(
  const char * Prompt, const char * fingerprint,
  uintptr_t & RequestResult)
{
  RequestResult = 0;
  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text = fingerprint;

  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
  Aliases[0].OnClick = MAKE_CALLBACK(TClipboardHandler::Copy, &ClipboardHandler);

  TQueryParams Params;
  Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
  Params.NoBatchAnswers = qaYes | qaRetry;
  Params.Aliases = Aliases;
  Params.AliasesCount = _countof(Aliases);
  uintptr_t Answer = FTerminal->QueryUser(
    FMTLOAD(VERIFY_CERT_PROMPT3, UnicodeString(Prompt).c_str()),
    nullptr, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);
  RequestResult = Answer;
  switch (RequestResult)
  {
    case qaCancel:
      // FTerminal->Configuration->Usage->Inc(L"HostNotVerified");
      FFileTransferCancelled = true;
      FFileTransferAbort = ftaCancel;
      break;
  }
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::AskForClientCertificateFilename(
  const char ** cert_file, uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
#if 0
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString FileName;
  if (!FTerminal->PromptUser(Data, pkFileName, LoadStr(CERT_FILENAME_PROMPT_TITLE), L"",
    LoadStr(CERT_FILENAME_PROMPT), true, 0, FileName))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(cert_file, StrToNeon(FileName), pool));
  RequestResult = qaOK;
#endif
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::NeonRequestAuth(
  const char ** user_name,
  const char ** password,
  uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  bool Result = true;
  RequestResult = 0;
  TSessionData * SessionData = FTerminal->GetSessionData();
  UnicodeString UserName = SessionData->GetUserNameExpanded();
  // will ask for username only once
  if (this->FUserName.IsEmpty())
  {
    if (!UserName.IsEmpty())
    {
      this->FUserName = UserName;
    }
    else
    {
      if (!FTerminal->PromptUser(SessionData, pkUserName, LoadStr(USERNAME_TITLE), L"",
        LoadStr(USERNAME_PROMPT2), true, NE_ABUFSIZ, this->FUserName))
      {
        // note that we never get here actually
        Result = false;
      }
    }
  }

  //if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), L"",
  //  LoadStr(USERNAME_PROMPT2), true, 0, UserName))
  //{
  //  FFileTransferCancelled = true;
  //  FFileTransferAbort = ftaCancel;
  //  return WEBDAV_ERR_CANCELLED;
  //}
  //WEBDAV_ERR(webdav::path_cstring_to_utf8(user_name, AnsiString(UserName).c_str(), pool));
  //RequestResult = qaOK;

  UnicodeString Password;
  if (Result)
  {
    // Some servers (Gallery2 on https://g2.pixi.me/w/webdav/)
    // return authentication error (401) on PROPFIND request for
    // non-existing files.
    // When we already tried password before, do not try anymore.
    // When we did not try password before (possible only when
    // server does not require authentication for any previous request,
    // such as when read access is not authenticated), try it now,
    // but use special flag for the try, because when it fails
    // we still want to try password for future requests (such as PUT).

    if (!this->FPassword.IsEmpty() && !this->FStoredPasswordTried)
    {
      if (this->FIgnoreAuthenticationFailure == iafPasswordFailed)
      {
        // Fail PROPFIND /nonexisting request...
        Result = false;
      }
      else
      {
        Password = FTerminal->DecryptPassword(this->FPassword);
      }
    }
    else
    {
      if (!SessionData->GetPassword().IsEmpty() && !this->FStoredPasswordTried)
      {
        Password = SessionData->GetPassword();
        this->FStoredPasswordTried = true;
      }
      else
      {
        // Asking for password (or using configured password) the first time,
        // and asking for password.
        // Note that we never get false here actually
        Result =
          FTerminal->PromptUser(
          SessionData, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, NE_ABUFSIZ, Password);
      }

      if (Result)
      {
        // While neon remembers the password on its own,
        // we need to keep a copy in case neon store gets reset by
        // 401 response to PROPFIND /nonexisting on G2, see above.
        // Possibly we can do this for G2 servers only.
        this->FPassword = FTerminal->EncryptPassword(Password);
      }
    }
  }

  if (Result)
  {
    WEBDAV_ERR(webdav::path_cstring_to_utf8(user_name, StrToNeon(this->FUserName), pool));
    WEBDAV_ERR(webdav::path_cstring_to_utf8(password, StrToNeon(Password), pool));
    RequestResult = qaOK;
  }

  return Result ? WEBDAV_NO_ERROR : WEBDAV_ERROR_AUTH;
}

webdav::error_t TWebDAVFileSystem::AskForUsername(
  const char ** user_name, uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString UserName = Data->GetUserNameExpanded();
  if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), L"",
    LoadStr(USERNAME_PROMPT2), true, 0, UserName))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(user_name, StrToNeon(UserName), pool));
  RequestResult = qaOK;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::AskForUserPassword(
  const char ** password,
  uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString Password = Data->GetPassword();
  if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), L"",
    LoadStr(PASSWORD_PROMPT), false, 0, Password))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(password, StrToNeon(Password), pool));
  RequestResult = qaOK;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::AskForPassphrase(
  const char ** passphrase,
  const char * realm,
  uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString Passphrase = Data->GetUserNameExpanded();
  UnicodeString Prompt = FORMAT(LoadStr(PROMPT_KEY_PASSPHRASE).c_str(), UnicodeString(realm).c_str());
  if (!FTerminal->PromptUser(Data, pkPassphrase, LoadStr(PASSPHRASE_TITLE), L"",
    Prompt, false, 0, Passphrase))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(passphrase, StrToNeon(Passphrase), pool));
  RequestResult = qaOK;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::SimplePrompt(
  const char * prompt_text,
  const char * prompt_string,
  uintptr_t & RequestResult)
{
  RequestResult = 0;
  std::unique_ptr<TStrings> MoreMessages(new TStringList());
  MoreMessages->Add(UnicodeString(prompt_string));
  uintptr_t Answer = FTerminal->QueryUser(
    UnicodeString(prompt_text),
    MoreMessages.get(), qaYes | qaNo | qaCancel, nullptr, qtConfirmation);
  RequestResult = Answer;
  return RequestResult == qaCancel ? WEBDAV_ERR_CANCELLED : WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::CreateStorage(
  THierarchicalStorage *& Storage)
{
  Storage =
    FTerminal->GetConfiguration()->CreateConfigStorage();
  return WEBDAV_NO_ERROR;
}

uintptr_t TWebDAVFileSystem::AdjustToCPSLimit(uintptr_t Len)
{
  return FCurrentOperationProgress ? (uintptr_t)FCurrentOperationProgress->AdjustToCPSLimit(Len) : Len;
}

NB_IMPLEMENT_CLASS(TWebDAVFileSystem, NB_GET_CLASS_INFO(TCustomFileSystem), nullptr)

