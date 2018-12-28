
#include <vcl.h>
#pragma hdrstop

#include "Terminal.h"

#include <Common.h>
#include <Sysutils.hpp>
#include <System.IOUtils.hpp>

#include "Interface.h"
#include "RemoteFiles.h"
#include "SecureShell.h"
#include "ScpFileSystem.h"
#include "SftpFileSystem.h"
#include "FtpFileSystem.h"
#include "WebDAVFileSystem.h"
#include "S3FileSystem.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Queue.h"
#include "Cryptography.h"
#include <openssl/pkcs12.h>
#include <openssl/err.h>

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif

///* TODO : Better user interface (query to user) */
void FileOperationLoopCustom(TTerminal *Terminal,
  TFileOperationProgressType *OperationProgress,
  uintptr_t Flags, const UnicodeString Message,
  const UnicodeString HelpKeyword,
  fu2::function<void()> Operation)
{
  bool DoRepeat;
  do
  {
    DoRepeat = false;
    try
    {
      Operation();
    }
    catch (EAbort &)
    {
      throw;
    }
    catch (ESkipFile &)
    {
      throw;
    }
    catch (EFatal &)
    {
      throw;
    }
    catch (EFileNotFoundError &)
    {
      throw;
    }
    catch (EOSError &)
    {
      throw;
    }
    catch (Exception &E)
    {
      Terminal->FileOperationLoopQuery(
        E, OperationProgress, Message, Flags & folAllowSkip, L"", HelpKeyword);
      DoRepeat = true;
    }
  }
  while (DoRepeat);
}
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
__removed #define FILE_OPERATION_LOOP_TERMINAL this
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TLoopDetector : public TObject
{
public:
  TLoopDetector();
  void RecordVisitedDirectory(const UnicodeString Directory);
  bool IsUnvisitedDirectory(const UnicodeString Directory);

private:
  std::unique_ptr<TStringList> FVisitedDirectories;
};
//---------------------------------------------------------------------------
TLoopDetector::TLoopDetector()
{
  FVisitedDirectories.reset(CreateSortedStringList());
}
//---------------------------------------------------------------------------
void TLoopDetector::RecordVisitedDirectory(const UnicodeString ADirectory)
{
  UnicodeString VisitedDirectory = ::ExcludeTrailingBackslash(ADirectory);
  FVisitedDirectories->Add(VisitedDirectory);
}
//---------------------------------------------------------------------------
bool TLoopDetector::IsUnvisitedDirectory(const UnicodeString Directory)
{
  bool Result = (FVisitedDirectories->IndexOf(Directory) < 0);

  if (Result)
  {
    RecordVisitedDirectory(Directory);
  }

  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TMoveFileParams);
struct TMoveFileParams : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TMoveFileParams); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TMoveFileParams) || TObject::is(Kind); }
public:
  TMoveFileParams() : TObject(OBJECT_CLASS_TMoveFileParams)
  {
  }
  UnicodeString Target;
  UnicodeString FileMask;
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TFilesFindParams);
struct TFilesFindParams : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFilesFindParams); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFilesFindParams) || TObject::is(Kind); }
public:
  TFilesFindParams() :
    TObject(OBJECT_CLASS_TFilesFindParams),
    OnFileFound(nullptr),
    OnFindingFile(nullptr),
    Cancel(false)
  {
  }

  TFileMasks FileMask;
  TFileFoundEvent OnFileFound;
  TFindingFileEvent OnFindingFile;
  bool Cancel;
  TLoopDetector LoopDetector;
  UnicodeString RealDirectory;
};
//---------------------------------------------------------------------------
TCalculateSizeStats::TCalculateSizeStats() noexcept :
  Files(0),
  Directories(0),
  SymLinks(0),
  FoundFiles(nullptr)
{
__removed memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
TCalculateSizeParams::TCalculateSizeParams() noexcept : TObject(OBJECT_CLASS_TCalculateSizeParams)
{
  __removed memset(this, 0, sizeof(*this));
  Result = true;
  AllowDirs = true;
}
//---------------------------------------------------------------------------
TSynchronizeOptions::~TSynchronizeOptions() noexcept
{
  SAFE_DESTROY(Filter);
}
//---------------------------------------------------------------------------
bool TSynchronizeOptions::MatchesFilter(const UnicodeString AFileName) const
{
  bool Result = true;
  if (Filter)
  {
    intptr_t FoundIndex = 0;
    Result = Filter->Find(AFileName, FoundIndex);
  }
  return Result;
}
//---------------------------------------------------------------------------
TSpaceAvailable::TSpaceAvailable() :
  BytesOnDevice(0),
  UnusedBytesOnDevice(0),
  BytesAvailableToUser(0),
  UnusedBytesAvailableToUser(0),
  BytesPerAllocationUnit(0)
{
__removed memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
#if 0
TOverwriteFileParams::TOverwriteFileParams()
{
  SourceSize = 0;
  DestSize = 0;
  SourcePrecision = mfFull;
  DestPrecision = mfFull;
}
#endif // if 0
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TTunnelThread);
class TTunnelThread : public TSimpleThread
{
  NB_DISABLE_COPY(TTunnelThread)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTunnelThread); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTunnelThread) || TSimpleThread::is(Kind); }
public:
  explicit TTunnelThread(TSecureShell *SecureShell);
  virtual ~TTunnelThread();
  virtual void InitTunnelThread();

  virtual void Terminate() override;

protected:
  virtual void Execute() override;

private:
  TSecureShell *FSecureShell;
  bool FTerminated;
};
//---------------------------------------------------------------------------
TTunnelThread::TTunnelThread(TSecureShell *SecureShell) :
  TSimpleThread(OBJECT_CLASS_TTunnelThread),
  FSecureShell(SecureShell),
  FTerminated(false)
{
}

void TTunnelThread::InitTunnelThread()
{
  TSimpleThread::InitSimpleThread();
  Start();
}
//---------------------------------------------------------------------------
TTunnelThread::~TTunnelThread()
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}
//---------------------------------------------------------------------------
void TTunnelThread::Terminate()
{
  FTerminated = true;
}
//---------------------------------------------------------------------------
void TTunnelThread::Execute()
{
  try
  {
    while (!FTerminated)
    {
      FSecureShell->Idle(250);
    }
  }
  catch (...)
  {
    if (FSecureShell->GetActive())
    {
      FSecureShell->Close();
    }
    // do not pass exception out of thread's proc
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TTunnelUI);
class TTunnelUI : public TSessionUI
{
  NB_DISABLE_COPY(TTunnelUI)
public:
  explicit TTunnelUI(TTerminal *Terminal);

  virtual ~TTunnelUI()
  {
  }

  virtual void Information(const UnicodeString AStr, bool Status) override;
  virtual uint32_t QueryUser(const UnicodeString AQuery,
    TStrings *MoreMessages, uint32_t Answers, const TQueryParams *Params,
    TQueryType QueryType) override;
  virtual uint32_t QueryUserException(const UnicodeString AQuery,
    Exception *E, uint32_t Answers, const TQueryParams *Params,
    TQueryType QueryType) override;
  virtual bool PromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts,
    TStrings *Results) override;
  virtual void DisplayBanner(const UnicodeString Banner) override;
  virtual void FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpContext) override;
  virtual void HandleExtendedException(Exception *E) override;
  virtual void Closed() override;
  virtual void ProcessGUI() override;

private:
  TTerminal *FTerminal;
  uint32_t FTerminalThreadID;
};
//---------------------------------------------------------------------------
TTunnelUI::TTunnelUI(TTerminal *Terminal) :
  TSessionUI(OBJECT_CLASS_TTunnelUI),
  FTerminal(Terminal)
{
  FTerminalThreadID = GetCurrentThreadId();
}
//---------------------------------------------------------------------------
void TTunnelUI::Information(const UnicodeString AStr, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->Information(AStr, Status);
  }
}
//---------------------------------------------------------------------------
uint32_t TTunnelUI::QueryUser(const UnicodeString AQuery,
  TStrings *MoreMessages, uint32_t Answers, const TQueryParams *Params,
  TQueryType QueryType)
{
  uint32_t Result;
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    Result = FTerminal->QueryUser(AQuery, MoreMessages, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(Answers);
  }
  return Result;
}
//---------------------------------------------------------------------------
uint32_t TTunnelUI::QueryUserException(const UnicodeString AQuery,
  Exception *E, uint32_t Answers, const TQueryParams *Params,
  TQueryType QueryType)
{
  uint32_t Result;
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    Result = FTerminal->QueryUserException(AQuery, E, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(Answers);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTunnelUI::PromptUser(TSessionData *Data, TPromptKind Kind,
  const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts, TStrings *Results)
{
  bool Result = false;
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    UnicodeString Instructions = AInstructions;
    if (IsAuthenticationPrompt(Kind))
    {
      Instructions =
        FMTLOAD(TUNNEL_INSTRUCTION2, Data->GetHostName()) +
        (AInstructions.IsEmpty() ? L"" : L"\n") +
        AInstructions;
    }

    Result = FTerminal->PromptUser(Data, Kind, AName, Instructions, Prompts, Results);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTunnelUI::DisplayBanner(const UnicodeString Banner)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->DisplayBanner(Banner);
  }
}
//---------------------------------------------------------------------------
void TTunnelUI::FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpContext)
{
  throw ESshFatal(E, Msg, HelpContext);
}
//---------------------------------------------------------------------------
void TTunnelUI::HandleExtendedException(Exception *E)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->HandleExtendedException(E);
  }
}
//---------------------------------------------------------------------------
void TTunnelUI::Closed()
{
  // noop
}
//---------------------------------------------------------------------------
void TTunnelUI::ProcessGUI()
{
  // noop
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCallbackGuard : public TObject
{
  NB_DISABLE_COPY(TCallbackGuard)
public:
  explicit TCallbackGuard(TTerminal *ATerminal);
  ~TCallbackGuard();

  void FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpKeyword);
  void Verify();
  bool Verify(Exception *E);
  void Dismiss();

private:
  ExtException *FFatalError;
  TTerminal *FTerminal;
  bool FGuarding;
};
//---------------------------------------------------------------------------
TCallbackGuard::TCallbackGuard(TTerminal *ATerminal) :
  FFatalError(nullptr),
  FTerminal(ATerminal),
  FGuarding(FTerminal->FCallbackGuard == nullptr)
{
  if (FGuarding)
  {
    FTerminal->FCallbackGuard = this;
  }
}
//---------------------------------------------------------------------------
TCallbackGuard::~TCallbackGuard()
{
  if (FGuarding)
  {
    DebugAssert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == nullptr));
    FTerminal->FCallbackGuard = nullptr;
  }

  SAFE_DESTROY_EX(Exception, FFatalError);
}
//---------------------------------------------------------------------------
void TCallbackGuard::FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpKeyword)
{
  DebugAssert(FGuarding);

  // make sure we do not bother about getting back the silent abort exception
  // we issued ourselves. this may happen when there is an exception handler
  // that converts any exception to fatal one (such as in TTerminal::Open).
  if (isa<ECallbackGuardAbort>(E))
  {
    SAFE_DESTROY_EX(Exception, FFatalError);
    FFatalError = new ExtException(E, Msg, HelpKeyword);
  }

  // silently abort what we are doing.
  // non-silent exception would be caught probably by default application
  // exception handler, which may not do an appropriate action
  // (particularly it will not resume broken transfer).
  throw ECallbackGuardAbort();
}
//---------------------------------------------------------------------------
void TCallbackGuard::Dismiss()
{
  DebugAssert(FFatalError == nullptr);
  FGuarding = false;
}
//---------------------------------------------------------------------------
void TCallbackGuard::Verify()
{
  if (FGuarding)
  {
    FGuarding = false;
    DebugAssert(FTerminal->FCallbackGuard == this);
    FTerminal->FCallbackGuard = nullptr;

    if (FFatalError != nullptr)
    {
      throw ESshFatal(FFatalError, L"");
    }
  }
}
//---------------------------------------------------------------------------
bool TCallbackGuard::Verify(Exception *E)
{
  bool Result =
    (dyn_cast<ECallbackGuardAbort>(E) != nullptr);
  if (Result)
  {
    DebugAssert(FGuarding && (FFatalError != nullptr));
    Verify();
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRobustOperationLoop::TRobustOperationLoop(TTerminal *Terminal, TFileOperationProgressType *OperationProgress, bool *AnyTransfer) :
  FTerminal(Terminal),
  FOperationProgress(OperationProgress),
  FRetry(false),
  FAnyTransfer(AnyTransfer),
  FPrevAnyTransfer(false)
{
  if (FAnyTransfer != nullptr)
  {
    FPrevAnyTransfer = *FAnyTransfer;
    *FAnyTransfer = false;
    FStart = Now();
  }
}
//---------------------------------------------------------------------------
TRobustOperationLoop::~TRobustOperationLoop()
{
  if (FAnyTransfer != nullptr)
  {
    *FAnyTransfer = FPrevAnyTransfer;
  }
}
//---------------------------------------------------------------------------
bool TRobustOperationLoop::TryReopen(Exception &E)
{
  FRetry = FTerminal && !FTerminal->GetActive();
  if (FRetry)
  {
    if (FAnyTransfer != nullptr)
    {
      // if there was any transfer, reset the global timestamp
      if (*FAnyTransfer)
      {
        FStart = Now();
        FPrevAnyTransfer = true;
        *FAnyTransfer = false;
      }
      else
      {
        FRetry = FTerminal->ContinueReopen(FStart);
      }
    }

    if (FRetry)
    {
      FRetry =
        FTerminal->QueryReopen(&E, ropNoReadDirectory, FOperationProgress);
    }
  }
  return FRetry;
}
//---------------------------------------------------------------------------
bool TRobustOperationLoop::ShouldRetry() const
{
  return FRetry;
}
//---------------------------------------------------------------------------
bool TRobustOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  return Result;
}
//---------------------------------------------------------------------------
class TRetryOperationLoop
{
public:
  explicit TRetryOperationLoop(TTerminal *Terminal);

  void Error(Exception &E);
  void Error(Exception &E, TSessionAction &Action);
  void Error(Exception &E, const UnicodeString Message);
  void Error(Exception &E, TSessionAction &Action, const UnicodeString Message);
  bool Retry();

private:
  TTerminal *FTerminal;
  bool FRetry;

  void DoError(Exception &E, TSessionAction *Action, const UnicodeString Message);
};
//---------------------------------------------------------------------------
TRetryOperationLoop::TRetryOperationLoop(TTerminal *Terminal)
{
  FTerminal = Terminal;
  FRetry = false;
}
//---------------------------------------------------------------------------
void TRetryOperationLoop::DoError(Exception &E, TSessionAction *Action, const UnicodeString Message)
{
  // Note that the action may already be canceled when RollbackAction is called
  uintptr_t Result;
  try
  {
    Result = FTerminal->CommandError(&E, Message, qaRetry | qaSkip | qaAbort);
  }
  catch (Exception &E2)
  {
    if (Action != nullptr)
    {
      FTerminal->RollbackAction(*Action, nullptr, &E2);
    }
    throw;
  }

  switch (Result)
  {
  case qaRetry:
    FRetry = true;
    if (Action != nullptr)
    {
      Action->Cancel();
    }
    break;

  case qaAbort:
    if (Action != nullptr)
    {
      FTerminal->RollbackAction(*Action, nullptr, &E);
    }
    Abort();
    break;

  case qaSkip:
    if (Action != nullptr)
    {
      Action->Cancel();
    }
    break;

  default:
    DebugFail();
    break;
  }
}
//---------------------------------------------------------------------------
void TRetryOperationLoop::Error(Exception &E)
{
  DoError(E, nullptr, UnicodeString());
}
//---------------------------------------------------------------------------
void TRetryOperationLoop::Error(Exception &E, TSessionAction &Action)
{
  DoError(E, &Action, UnicodeString());
}
//---------------------------------------------------------------------------
void TRetryOperationLoop::Error(Exception &E, const UnicodeString Message)
{
  DoError(E, nullptr, Message);
}
//---------------------------------------------------------------------------
void TRetryOperationLoop::Error(Exception &E, TSessionAction &Action, const UnicodeString Message)
{
  DoError(E, &Action, Message);
}
//---------------------------------------------------------------------------
bool TRetryOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TCollectedFileList::TCollectedFileList() :
  TObject(OBJECT_CLASS_TCollectedFileList)
{
}
//---------------------------------------------------------------------------
intptr_t TCollectedFileList::Add(const UnicodeString FileName, TObject *Object, bool Dir)
{
  TFileData Data;
  Data.FileName = FileName;
  Data.Object = Object;
  Data.Dir = Dir;
  Data.Recursed = true;
  FList.push_back(Data);
  return GetCount() - 1;
}
//---------------------------------------------------------------------------
void TCollectedFileList::DidNotRecurse(intptr_t Index)
{
  FList[Index].Recursed = false;
}
//---------------------------------------------------------------------------
void TCollectedFileList::Delete(intptr_t Index)
{
  FList.erase(FList.begin() + Index);
}
//---------------------------------------------------------------------------
intptr_t TCollectedFileList::GetCount() const
{
  return FList.size();
}
//---------------------------------------------------------------------------
UnicodeString TCollectedFileList::GetFileName(intptr_t Index) const
{
  return FList[Index].FileName;
}
//---------------------------------------------------------------------------
TObject *TCollectedFileList::GetObj(intptr_t Index) const
{
  return FList[Index].Object;
}
//---------------------------------------------------------------------------
bool TCollectedFileList::IsDir(intptr_t Index) const
{
  return FList[Index].Dir;
}
//---------------------------------------------------------------------------
bool TCollectedFileList::IsRecursed(intptr_t Index) const
{
  return FList[Index].Recursed;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TParallelOperation::TParallelOperation(TOperationSide Side) :
  FIndex(0),
  FCopyParam(nullptr),
  FParams(0),
  FProbablyEmpty(false),
  FClients(0),
  FMainOperationProgress(nullptr),
  FSide(Side)
{
  DebugAssert((Side == osLocal) || (Side == osRemote));
}
//---------------------------------------------------------------------------
void TParallelOperation::Init(
  TStrings *AFileList, const UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *MainOperationProgress, const UnicodeString MainName)
{
  DebugAssert(FFileList.get() == nullptr);
  // More lists should really happen in scripting only, which does not support parallel transfers atm.
  // But in general the code should work with more lists anyway, it just was not tested for it.
  DebugAssert(AFileList->GetCount() == 1);
  FFileList.reset(AFileList);
  FSection.reset(new TCriticalSection());
  FTargetDir = TargetDir;
  FCopyParam = CopyParam;
  FParams = Params;
  FMainOperationProgress = MainOperationProgress;
  FMainName = MainName;
  FIndex = 0;
}
//---------------------------------------------------------------------------
TParallelOperation::~TParallelOperation()
{
  WaitFor();
}
//---------------------------------------------------------------------------
bool TParallelOperation::IsInitialized() const
{
  return (FMainOperationProgress != nullptr);
}
//---------------------------------------------------------------------------
bool TParallelOperation::ShouldAddClient() const
{
  bool Result;
  // TTransferQueueItem::ProgressUpdated() already checks IsInitialized() before calling this
  if (!IsInitialized())
  {
    Result = false;
  }
  else
  {
    volatile TGuard Guard(*FSection.get());
    Result = !FProbablyEmpty && (FMainOperationProgress->GetCancel() < csCancel);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TParallelOperation::AddClient()
{
  volatile TGuard Guard(*FSection.get());
  FClients++;
}
//---------------------------------------------------------------------------
void TParallelOperation::RemoveClient()
{
  volatile TGuard Guard(*FSection.get());
  FClients--;
}
//---------------------------------------------------------------------------
void TParallelOperation::WaitFor()
{
  // Even initialized?
  // Won't be, when parallel transfers were not possible (like when preserving of directory timestamps is enabled)
  if (FSection.get() != nullptr)
  {
    bool Done;

    do
    {
      {
        volatile TGuard Guard(*FSection.get());
        Done = (FClients == 0);
      }

      if (!Done)
      {
        // propagate the total progress incremented by the parallel operations
        FMainOperationProgress->Progress();
        Sleep(200);
      }
    }
    while (!Done);
    FProbablyEmpty = true;
  }
  else
  {
    DebugAssert(FClients == 0);
  }

}
//---------------------------------------------------------------------------
void TParallelOperation::Done(const UnicodeString FileName, bool Dir, bool Success)
{
  if (Dir)
  {
    volatile TGuard Guard(*FSection.get());

    TDirectories::iterator DirectoryIterator = FDirectories.find(FileName);
    if (DebugAlwaysTrue(DirectoryIterator != FDirectories.end()))
    {
      if (Success)
      {
        DebugAssert(!DirectoryIterator->second.Exists);
        DirectoryIterator->second.Exists = true;
      }
      else
      {
        // This is actually not useful at the moment, as when creating directory fails and "Skip" is pressed,
        // the current code in CopyToRemote/CreateDirectory will behave as, if it succedded, so Successs will be true here.
        FDirectories.erase(DirectoryIterator->first);
        if (FFileList->GetCount() > 0)
        {
          UnicodeString FileNameWithSlash;
          if (FSide == osLocal)
          {
            FileNameWithSlash = ::IncludeTrailingBackslash(FileName);
          }
          else
          {
            FileNameWithSlash = base::UnixIncludeTrailingBackslash(FileName);
          }

          // It can actually be a different list than the one the directory was taken from,
          // but that does not matter that much. It should not happen anyway, as more lists should be in scripting only.
          TCollectedFileList *Files = DebugNotNull(dyn_cast<TCollectedFileList>(FFileList->GetObj(0)));
          intptr_t Index = 0;
          while (Index < Files->GetCount())
          {
            if (StartsText(FileNameWithSlash, Files->GetFileName(Index)))
            {
              // We should add the file to "skip" counters in the OperationProgress,
              // but an interactive foreground transfer is not doing that either yet.
              Files->Delete(Index);
              if (Index < FIndex)
              {
                FIndex--;
              }
            }
            else
            {
              Index++;
            }
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TParallelOperation::CheckEnd(TCollectedFileList *Files)
{
  bool Result = (FIndex >= Files->GetCount());
  if (Result)
  {
    FFileList->Delete(0);
    FIndex = 0;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TParallelOperation::GetNext(TTerminal *Terminal, UnicodeString &FileName, TObject *&Object, UnicodeString &TargetDir, bool &Dir, bool &Recursed)
{
  volatile TGuard Guard(*FSection.get());
  intptr_t Result = 1;
  TCollectedFileList *Files;
  do
  {
    if (FFileList->GetCount() > 0)
    {
      Files = DebugNotNull(dyn_cast<TCollectedFileList>(FFileList->GetObj(0)));
      // can happen if the file was excluded by file mask
      if (CheckEnd(Files))
      {
        Files = nullptr;
      }
    }
    else
    {
      Files = nullptr;
      Result = -1;
    }
  }
  while ((Result == 1) && (Files == nullptr));

  if (Files != nullptr)
  {
    UnicodeString RootPath = FFileList->GetString(0);

    FileName = Files->GetFileName(FIndex);
    Object = Files->GetObj(FIndex);
    Dir = Files->IsDir(FIndex);
    Recursed = Files->IsRecursed(FIndex);
    UnicodeString DirPath;
    bool FirstLevel;
    if (FSide == osLocal)
    {
      DirPath = ::ExtractFileDir(FileName);
      FirstLevel = ::SamePaths(DirPath, RootPath);
    }
    else
    {
      DirPath = base::UnixExtractFileDir(FileName);
      FirstLevel = base::UnixSamePath(DirPath, RootPath);
    }

    if (FirstLevel)
    {
      TargetDir = FTargetDir;
    }
    else
    {
      TDirectories::const_iterator DirectoryIterator = FDirectories.find(DirPath);
      if (DebugAlwaysFalse(DirectoryIterator == FDirectories.end()))
      {
        throw EInvalidOperation(L"Parent path not known");
      }
      const TDirectoryData &DirectoryData = DirectoryIterator->second;
      if (!DirectoryData.Exists)
      {
        Result = 0; // wait for parent directory to be created
      }
      else
      {
        TargetDir = DirectoryData.OppositePath;
      }
    }

    if (!TargetDir.IsEmpty())
    {
      if (Dir)
      {
        TDirectoryData DirectoryData;

        UnicodeString OnlyFileName;
        if (FSide == osLocal)
        {
          OnlyFileName = base::ExtractFileName(FileName, false);
        }
        else
        {
          OnlyFileName = base::UnixExtractFileName(FileName);
        }
        OnlyFileName = Terminal->ChangeFileName(FCopyParam, OnlyFileName, osRemote, FirstLevel);
        if (FSide == osLocal)
        {
          DirectoryData.OppositePath = base::UnixCombinePaths(TargetDir, OnlyFileName);
        }
        else
        {
          DirectoryData.OppositePath = TPath::Combine(TargetDir, OnlyFileName);
        }

        DirectoryData.Exists = false;

        FDirectories.insert(TDirectories::value_type(FileName, DirectoryData));
      }

      FIndex++;
      CheckEnd(Files);
    }
  }

  FProbablyEmpty = (FFileList->GetCount() == 0);

  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TTerminal::TTerminal(TObjectClassId Kind) noexcept :
  TSessionUI(Kind),
  FSessionData(new TSessionData(L"")),
  FLog(nullptr),
  FActionLog(nullptr),
  FConfiguration(nullptr),
  FExceptionOnFail(0),
  FFiles(nullptr),
  FInTransaction(0),
  FSuspendTransaction(false),
  FOnChangeDirectory(nullptr),
  FOnReadDirectory(nullptr),
  FOnStartReadDirectory(nullptr),
  FOnReadDirectoryProgress(nullptr),
  FOnDeleteLocalFile(nullptr),
  FOnCreateLocalFile(nullptr),
  FOnGetLocalFileAttributes(nullptr),
  FOnSetLocalFileAttributes(nullptr),
  FOnMoveLocalFile(nullptr),
  FOnRemoveLocalDirectory(nullptr),
  FOnCreateLocalDirectory(nullptr),
  FOnInitializeLog(nullptr),
  FUsersGroupsLookedup(false),
  FOperationProgress(nullptr),
  FUseBusyCursor(false),
  FDirectoryCache(nullptr),
  FDirectoryChangesCache(nullptr),
  FSecureShell(nullptr),
  FFSProtocol(cfsUnknown),
  FCommandSession(nullptr),
  FAutoReadDirectory(false),
  FReadingCurrentDirectory(false),
  FClosedOnCompletion(nullptr),
  FStatus(ssClosed),
  FOpening(0),
  FTunnelThread(nullptr),
  FTunnel(nullptr),
  FTunnelData(nullptr),
  FTunnelLog(nullptr),
  FTunnelUI(nullptr),
  FTunnelLocalPortNumber(0),
  FCallbackGuard(nullptr),
  FEnableSecureShellUsage(false),
  FCollectFileSystemUsage(false),
  FRememberedPasswordTried(false),
  FRememberedTunnelPasswordTried(false),
  FNesting(0),
  FLastProgressLogged(0),
  FMultipleDestinationFiles(false),
  FReadCurrentDirectoryPending(false),
  FReadDirectoryPending(false),
  FTunnelOpening(false),
  FFileSystem(nullptr)
{
  FOldFiles = new TRemoteDirectory(this);
}

void TTerminal::Init(TSessionData *ASessionData,
  TConfiguration *AConfiguration)
{
  FConfiguration = AConfiguration;
  FSessionData->Assign(ASessionData);
  TDateTime Started = Now(); // use the same time for session and XML log
  FLog = new TSessionLog(this, Started, FSessionData, FConfiguration);
  FActionLog = new TActionLog(this, Started, FSessionData, FConfiguration);
  FFiles = new TRemoteDirectory(this);
  FExceptionOnFail = 0;
  FInTransaction = 0;
  FReadCurrentDirectoryPending = false;
  FReadDirectoryPending = false;
  FUsersGroupsLookedup = False;
  FTunnelLocalPortNumber = 0;
  FFileSystem = nullptr;
  FSecureShell = nullptr;
  FOnProgress = nullptr;
  FOnFinished = nullptr;
  FOnDeleteLocalFile = nullptr;
  FOnCreateLocalFile = nullptr;
  FOnGetLocalFileAttributes = nullptr;
  FOnSetLocalFileAttributes = nullptr;
  FOnMoveLocalFile = nullptr;
  FOnRemoveLocalDirectory = nullptr;
  FOnCreateLocalDirectory = nullptr;
  FOnReadDirectoryProgress = nullptr;
  FOnQueryUser = nullptr;
  FOnPromptUser = nullptr;
  FOperationProgressPersistence = nullptr;
  FOperationProgressOnceDoneOperation = odoIdle;
  FOnInformation = nullptr;
  FOnCustomCommand = nullptr;
  FOnClose = nullptr;
  FOnFindingFile = nullptr;

  FUseBusyCursor = True;
  FLockDirectory.Clear();
  FDirectoryCache = new TRemoteDirectoryCache();
  FDirectoryChangesCache = nullptr;
  FFSProtocol = cfsUnknown;
  FCommandSession = nullptr;
  FAutoReadDirectory = true;
  FReadingCurrentDirectory = false;
  FStatus = ssClosed;
  FOpening = 0;
  FTunnelThread = nullptr;
  FTunnel = nullptr;
  FTunnelData = nullptr;
  FTunnelLog = nullptr;
  FTunnelUI = nullptr;
  FTunnelOpening = false;
  FCallbackGuard = nullptr;
  FNesting = 0;
  FRememberedPasswordKind = TPromptKind(-1);
  FCollectFileSystemUsage = false;
  FSuspendTransaction = false;
  FOperationProgress = nullptr;
  FClosedOnCompletion = nullptr;
}
//---------------------------------------------------------------------------
TTerminal::~TTerminal() noexcept
{
  if (GetActive())
  {
    Close();
  }

  if (FCallbackGuard != nullptr)
  {
    // see TTerminal::HandleExtendedException
    FCallbackGuard->Dismiss();
  }
  DebugAssert(FTunnel == nullptr);

  SAFE_DESTROY(FCommandSession);

  if (GetSessionData()->GetCacheDirectoryChanges() && GetSessionData()->GetPreserveDirectoryChanges() &&
    (FDirectoryChangesCache != nullptr))
  {
    FConfiguration->SaveDirectoryChangesCache(GetSessionData()->GetSessionKey(),
      FDirectoryChangesCache);
  }

  SAFE_DESTROY_EX(TCustomFileSystem, FFileSystem);
  SAFE_DESTROY_EX(TSessionLog, FLog);
  SAFE_DESTROY_EX(TActionLog, FActionLog);
  SAFE_DESTROY(FFiles);
  SAFE_DESTROY_EX(TRemoteDirectoryCache, FDirectoryCache);
  SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
  SAFE_DESTROY(FSessionData);
  SAFE_DESTROY(FOldFiles);
}
//---------------------------------------------------------------------------
void TTerminal::Idle()
{
  // Once we disconnect, do nothing, until reconnect handler
  // "receives the information".
  // Never go idle when called from within ::ProcessGUI() call
  // as we may recurse for good, timeouting eventually.
  if (GetActive() && (FNesting == 0))
  {
    volatile TAutoNestingCounter NestingCounter(FNesting);

    if (FConfiguration->GetActualLogProtocol() >= 1)
    {
      __removed LogEvent("Session upkeep");
    }

    DebugAssert(FFileSystem != nullptr);
    FFileSystem->Idle();

    if (GetCommandSessionOpened())
    {
      try
      {
        FCommandSession->Idle();
      }
      catch (Exception &E)
      {
        // If the secondary session is dropped, ignore the error and let
        // it be reconnected when needed.
        // BTW, non-fatal error can hardly happen here, that's why
        // it is displayed, because it can be useful to know.
        if (FCommandSession->GetActive())
        {
          FCommandSession->HandleExtendedException(&E);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
RawByteString TTerminal::EncryptPassword(const UnicodeString APassword) const
{
  return FConfiguration->EncryptPassword(APassword, GetSessionData()->GetSessionName());
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::DecryptPassword(const RawByteString APassword) const
{
  UnicodeString Result;
  try
  {
    Result = FConfiguration->DecryptPassword(APassword, GetSessionData()->GetSessionName());
  }
  catch (EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FRememberedPassword = EncryptPassword(GetRememberedPassword());
  FRememberedTunnelPassword = EncryptPassword(GetRememberedTunnelPassword());
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::ExpandFileName(const UnicodeString APath,
  const UnicodeString BasePath)
{
  // replace this by AbsolutePath()
  UnicodeString Result = base::UnixExcludeTrailingBackslash(APath);
  if (!base::UnixIsAbsolutePath(Result) && !BasePath.IsEmpty())
  {
    TODO("Handle more complicated cases like '../../xxx'");
    if (APath == PARENTDIRECTORY)
    {
      Result = base::UnixExcludeTrailingBackslash(base::UnixExtractFilePath(
            base::UnixExcludeTrailingBackslash(BasePath)));
    }
    else
    {
      Result = base::UnixIncludeTrailingBackslash(BasePath) + APath;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::GetActive() const
{
  return (this != nullptr) && (FFileSystem != nullptr) && FFileSystem->GetActive();
}
//---------------------------------------------------------------------------
void TTerminal::Close()
{
  if (FStatus == ssClosing)
    return;
  FStatus = ssClosing;
  FFileSystem->Close();

  // Cannot rely on CommandSessionOpened here as Status is set to ssClosed too late
  if ((FCommandSession != nullptr) && FCommandSession->GetActive())
  {
    // prevent recursion
    FCommandSession->SetOnClose(nullptr);
    FCommandSession->Close();
  }
}
//---------------------------------------------------------------------------
void TTerminal::ResetConnection()
{
  // used to be called from Reopen(), why?
  FTunnelError.Clear();

  FRememberedPasswordTried = false;
  FRememberedTunnelPasswordTried = false;

  if (FDirectoryChangesCache != nullptr)
  {
    SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
  }

  FFiles->SetDirectory(L"");
  // note that we cannot clear contained files
  // as they can still be referenced in the GUI atm
}
//---------------------------------------------------------------------------
void TTerminal::FingerprintScan(UnicodeString &SHA256, UnicodeString &MD5)
{
  GetSessionData()->SetFingerprintScan(true);
  try
  {
    Open();
    // we should never get here
    DebugFail();
    Abort();
  }
  catch (...)
  {
    if (!FFingerprintScannedSHA256.IsEmpty() || !FFingerprintScannedMD5.IsEmpty())
    {
      SHA256 = FFingerprintScannedSHA256;
      MD5 = FFingerprintScannedMD5;
    }
    else
    {
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::Open()
{
  TAutoNestingCounter OpeningCounter(FOpening);
  ReflectSettings();
  try
  {
    FEncryptKey = HexToBytes(FSessionData->EncryptKey);

    DoInformation(L"", true, 1);
    try__finally
    {
      InternalTryOpen();
    },
    __finally
    {
      DoInformation(L"", true, 0);
    } end_try__finally
  }
  catch (EFatal &)
  {
    throw;
  }
  catch (Exception &E)
  {
    LogEvent(FORMAT("Got error: \"%s\"", E.Message));
    // any exception while opening session is fatal
    FatalError(&E, L"");
  }
  FSessionData->SetNumberOfRetries(0);
}

void TTerminal::InternalTryOpen()
{
  try
  {
    ResetConnection();
    FStatus = ssOpening;

    try__finally
    {
      InternalDoTryOpen();
    },
    __finally
    {
      if (FSessionData->GetTunnel())
      {
        FSessionData->RollbackTunnel();
      }
    } end_try__finally

    if (GetSessionData()->GetCacheDirectoryChanges())
    {
      DebugAssert(FDirectoryChangesCache == nullptr);
      FDirectoryChangesCache = new TRemoteDirectoryChangesCache(
        FConfiguration->GetCacheDirectoryChangesMaxSize());
      if (GetSessionData()->GetPreserveDirectoryChanges())
      {
        FConfiguration->LoadDirectoryChangesCache(GetSessionData()->GetSessionKey(),
          FDirectoryChangesCache);
      }
    }

    DoStartup();

    if (FCollectFileSystemUsage)
    {
      FFileSystem->CollectUsage();
      FCollectFileSystemUsage = false;
    }

    DoInformation(LoadStr(STATUS_READY), true);
    FStatus = ssOpened;
  }
  catch (...)
  {
    // rollback
    if (FDirectoryChangesCache != nullptr)
    {
      SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
    }
    if (GetSessionData()->GetFingerprintScan() && (FFileSystem != nullptr) &&
        DebugAlwaysTrue(GetSessionData()->GetFtps() != ftpsNone))
    {
      FFingerprintScannedSHA256 = UnicodeString();
      FFingerprintScannedMD5 = FFileSystem->GetSessionInfo().CertificateFingerprint;
    }
    // Particularly to prevent reusing a wrong client certificate passphrase
    // in the next login attempt
    FRememberedPassword = UnicodeString();
    FRememberedPasswordKind = TPromptKind(-1);
    FRememberedTunnelPassword = UnicodeString();
    throw;
  }
}

void TTerminal::InternalDoTryOpen()
{
  if (FFileSystem == nullptr)
  {
    GetLog()->AddSystemInfo();
    DoInitializeLog();
    GetLog()->AddStartupInfo();
  }

  DebugAssert(FTunnel == nullptr);
  if (FSessionData->GetTunnel())
  {
    DoInformation(LoadStr(OPEN_TUNNEL), true);
    LogEvent("Opening tunnel.");
    OpenTunnel();
    GetLog()->AddSeparator();

    FSessionData->ConfigureTunnel(FTunnelLocalPortNumber);

    DoInformation(LoadStr(USING_TUNNEL), false);
    LogEvent(FORMAT("Connecting via tunnel interface %s:%d.",
      FSessionData->GetHostNameExpanded(), FSessionData->GetPortNumber()));
  }
  else
  {
    DebugAssert(FTunnelLocalPortNumber == 0);
  }

  if (FFileSystem == nullptr)
  {
    InitFileSystem();
  }
  else
  {
    FFileSystem->Open();
  }
}

void TTerminal::InitFileSystem()
{
  DebugAssert(FFileSystem == nullptr);
  TFSProtocol FSProtocol = GetSessionData()->GetFSProtocol();
  if ((FSProtocol == fsFTP) && (GetSessionData()->GetFtps() == ftpsNone))
  {
    FFSProtocol = cfsFTP;
    FFileSystem = new TFTPFileSystem(this);
    FFileSystem->Init(nullptr);
    FFileSystem->Open();
    GetLog()->AddSeparator();
    LogEvent("Using FTP protocol.");
  }
  else if ((FSProtocol == fsFTP) && (GetSessionData()->GetFtps() != ftpsNone))
  {
#if defined(NO_FILEZILLA)
    LogEvent("FTP protocol is not supported by this build.");
    FatalError(nullptr, "FTP is not supported");
#else
    FFSProtocol = cfsFTPS;
    FFileSystem = new TFTPFileSystem(this);
    FFileSystem->Init(nullptr);
    FFileSystem->Open();
    GetLog()->AddSeparator();
    LogEvent("Using FTPS protocol.");
#endif
  }
  else if (FSProtocol == fsWebDAV)
  {
    FFSProtocol = cfsWebDAV;
    FFileSystem = new TWebDAVFileSystem(this);
    FFileSystem->Init(nullptr);
    FFileSystem->Open();
    GetLog()->AddSeparator();
    LogEvent("Using WebDAV protocol.");
  }
  else if (GetSessionData()->GetFSProtocol() == fsS3)
  {
    FFSProtocol = cfsS3;
    FFileSystem = new TS3FileSystem(this);
    FFileSystem->Init(nullptr);
    FFileSystem->Open();
    GetLog()->AddSeparator();
    LogEvent("Using S3 protocol.");
  }
  else
  {
    DebugAssert(FSecureShell == nullptr);
    try__finally
    {
      FSecureShell = new TSecureShell(this, FSessionData, GetLog(), FConfiguration);
      try
      {
        // there will be only one channel in this session
        FSecureShell->SetSimple(true);
        FSecureShell->Open();
      }
      catch (Exception &E)
      {
        DebugAssert(!FSecureShell->GetActive());
        if (FSessionData->GetFingerprintScan())
        {
          FSecureShell->GetHostKeyFingerprint(FFingerprintScannedSHA256, FFingerprintScannedMD5);
        }
        if (!FSecureShell->GetActive() && !FTunnelError.IsEmpty())
        {
          // the only case where we expect this to happen
          UnicodeString ErrorMessage = LoadStr(UNEXPECTED_CLOSE_ERROR);
          DebugAssert(E.Message == ErrorMessage);
          FatalError(&E, FMTLOAD(TUNNEL_ERROR, FTunnelError));
        }
        else
        {
          throw;
        }
      }

      GetLog()->AddSeparator();

      if ((FSProtocol == fsSCPonly) ||
          (FSProtocol == fsSFTP && FSecureShell->SshFallbackCmd()))
      {
        FFSProtocol = cfsSCP;
        FFileSystem = new TSCPFileSystem(this);
        FFileSystem->Init(FSecureShell);
        FSecureShell = nullptr; // ownership passed
        LogEvent("Using SCP protocol.");
      }
      else
      {
        FFSProtocol = cfsSFTP;
        FFileSystem = new TSFTPFileSystem(this);
        FFileSystem->Init(FSecureShell);
        FSecureShell = nullptr; // ownership passed
        LogEvent("Using SFTP protocol.");
      }
    },
    __finally
    {
      SAFE_DESTROY(FSecureShell);
      FSecureShell = nullptr;
      // This does not make it through, if terminal thread is abandonded,
      // see also TTerminalManager::DoConnectTerminal
      DoInformation(L"", true, 0);
    } end_try__finally
  }
}
//---------------------------------------------------------------------------
bool TTerminal::IsListenerFree(uintptr_t PortNumber) const
{
  SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
  bool Result = (Socket != INVALID_SOCKET);
  if (Result)
  {
    SOCKADDR_IN Address;

    ClearStruct(Address);
    Address.sin_family = AF_INET;
    Address.sin_port = htons(static_cast<short>(PortNumber));
    Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Result = (bind(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) == 0);
    closesocket(Socket);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::SetupTunnelLocalPortNumber()
{
  DebugAssert(FTunnelData == nullptr);

  FTunnelLocalPortNumber = FSessionData->GetTunnelLocalPortNumber();
  if (FTunnelLocalPortNumber == 0)
  {
    FTunnelLocalPortNumber = FConfiguration->GetTunnelLocalPortNumberLow();
    while (!IsListenerFree(FTunnelLocalPortNumber))
    {
      FTunnelLocalPortNumber++;
      if (FTunnelLocalPortNumber > FConfiguration->GetTunnelLocalPortNumberHigh())
      {
        FTunnelLocalPortNumber = 0;
        FatalError(nullptr, FMTLOAD(TUNNEL_NO_FREE_PORT,
          FConfiguration->GetTunnelLocalPortNumberLow(), FConfiguration->GetTunnelLocalPortNumberHigh()));
      }
    }
    LogEvent(FORMAT("Autoselected tunnel local port number %d", FTunnelLocalPortNumber));
  }
}
//---------------------------------------------------------------------------
void TTerminal::OpenTunnel()
{
  SetupTunnelLocalPortNumber();

  try
  {
    FTunnelData = new TSessionData(L"");
    FTunnelData->Assign(StoredSessions->GetDefaultSettings());
    FTunnelData->SetName(FMTLOAD(TUNNEL_SESSION_NAME, FSessionData->GetSessionName()));
    FTunnelData->SetTunnel(false);
    FTunnelData->SetHostName(FSessionData->GetTunnelHostName());
    FTunnelData->SetPortNumber(FSessionData->GetTunnelPortNumber());
    FTunnelData->SessionSetUserName(FSessionData->GetTunnelUserName());
    FTunnelData->SetPassword(FSessionData->GetTunnelPassword());
    FTunnelData->SetPublicKeyFile(FSessionData->GetTunnelPublicKeyFile());
    UnicodeString HostName = FSessionData->GetHostNameExpanded();
    if (IsIPv6Literal(HostName))
    {
      HostName = EscapeIPv6Literal(HostName);
    }
    FTunnelData->SetTunnelPortFwd(FORMAT("L%d\t%s:%d",
      FTunnelLocalPortNumber, HostName, FSessionData->GetPortNumber()));
    FTunnelData->SetHostKey(FSessionData->GetTunnelHostKey());

    // inherit proxy options on the main session
    FTunnelData->SetProxyMethod(FSessionData->GetProxyMethod());
    FTunnelData->SetProxyHost(FSessionData->GetProxyHost());
    FTunnelData->SetProxyPort(FSessionData->GetProxyPort());
    FTunnelData->SetProxyUsername(FSessionData->GetProxyUsername());
    FTunnelData->SetProxyPassword(FSessionData->GetProxyPassword());
    FTunnelData->SetProxyTelnetCommand(FSessionData->GetProxyTelnetCommand());
    FTunnelData->SetProxyLocalCommand(FSessionData->GetProxyLocalCommand());
    FTunnelData->SetProxyDNS(FSessionData->GetProxyDNS());
    FTunnelData->SetProxyLocalhost(FSessionData->GetProxyLocalhost());

    // inherit most SSH options of the main session (except for private key and bugs)
    FTunnelData->SetCompression(FSessionData->GetCompression());
    FTunnelData->SetSshProt(FSessionData->GetSshProt());
    FTunnelData->SetCipherList(FSessionData->GetCipherList());
    FTunnelData->SetSsh2DES(FSessionData->GetSsh2DES());

    FTunnelData->SetKexList(FSessionData->GetKexList());
    FTunnelData->SetRekeyData(FSessionData->GetRekeyData());
    FTunnelData->SetRekeyTime(FSessionData->GetRekeyTime());

    FTunnelData->SetSshNoUserAuth(FSessionData->GetSshNoUserAuth());
    FTunnelData->SetAuthGSSAPI(FSessionData->GetAuthGSSAPI());
    FTunnelData->SetGSSAPIFwdTGT(FSessionData->GetGSSAPIFwdTGT());
    FTunnelData->SetTryAgent(FSessionData->GetTryAgent());
    FTunnelData->SetAgentFwd(FSessionData->GetAgentFwd());
    FTunnelData->SetAuthTIS(FSessionData->GetAuthTIS());
    FTunnelData->SetAuthKI(FSessionData->GetAuthKI());
    FTunnelData->SetAuthKIPassword(FSessionData->GetAuthKIPassword());

    // The Started argument is not used with Parent being set
    FTunnelLog = new TSessionLog(this, TDateTime(), FTunnelData, FConfiguration);
    FTunnelLog->SetParent(FLog, L"Tunnel");
    FTunnelLog->ReflectSettings();
    FTunnelUI = new TTunnelUI(this);
    FTunnel = new TSecureShell(FTunnelUI, FTunnelData, FTunnelLog, FConfiguration);

    FTunnelOpening = true;
    try__finally
    {
      FTunnel->Open();
    },
    __finally
    {
      FTunnelOpening = false;
    } end_try__finally

    FTunnelThread = new TTunnelThread(FTunnel);
    FTunnelThread->InitTunnelThread();
  }
  catch (...)
  {
    CloseTunnel();
    throw;
  }
}
//---------------------------------------------------------------------------
void TTerminal::CloseTunnel()
{
  SAFE_DESTROY_EX(TTunnelThread, FTunnelThread);
  FTunnelError = FTunnel->GetLastTunnelError();
  SAFE_DESTROY_EX(TSecureShell, FTunnel);
  SAFE_DESTROY_EX(TTunnelUI, FTunnelUI);
  SAFE_DESTROY_EX(TSessionLog, FTunnelLog);
  SAFE_DESTROY(FTunnelData);

  FTunnelLocalPortNumber = 0;
}
//---------------------------------------------------------------------------
void TTerminal::Closed()
{
  if (FTunnel != nullptr)
  {
    CloseTunnel();
  }

  if (GetOnClose())
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnClose()(this);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }

  FStatus = ssClosed;
}
//---------------------------------------------------------------------------
void TTerminal::ProcessGUI()
{
  // Do not process GUI here, as we are called directly from a GUI loop and may
  // recurse for good.
  // Alternatively we may check for (FOperationProgress == nullptr)
  if (FNesting == 0)
  {
    volatile TAutoNestingCounter NestingCounter(FNesting);
    ::ProcessGUI();
  }
}
//---------------------------------------------------------------------------
void TTerminal::Progress(TFileOperationProgressType *OperationProgress)
{
  if (FNesting == 0)
  {
    volatile TAutoNestingCounter NestingCounter(FNesting);
    OperationProgress->Progress();
  }
}
//---------------------------------------------------------------------------
void TTerminal::Reopen(intptr_t Params)
{
  TFSProtocol OrigFSProtocol = GetSessionData()->GetFSProtocol();
  UnicodeString PrevRemoteDirectory = GetSessionData()->GetRemoteDirectory();
  bool PrevReadCurrentDirectoryPending = FReadCurrentDirectoryPending;
  bool PrevReadDirectoryPending = FReadDirectoryPending;
  DebugAssert(!FSuspendTransaction);
  bool PrevAutoReadDirectory = FAutoReadDirectory;
  // here used to be a check for FExceptionOnFail being 0
  // but it can happen, e.g. when we are downloading file to execute it.
  // however I'm not sure why we mind having exception-on-fail enabled here
  intptr_t PrevExceptionOnFail = FExceptionOnFail;
  bool WasInTransaction = InTransaction();
  try__finally
  {
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
    // reset transactions
    if (InTransaction())
      EndTransaction();
    FSuspendTransaction = true;
    FExceptionOnFail = 0;
    // typically, we avoid reading directory, when there is operation ongoing,
    // for file list which may reference files from current directory
    if (FLAGSET(Params, ropNoReadDirectory))
    {
      SetAutoReadDirectory(false);
    }

    // only peek, we may not be connected at all atm,
    // so make sure we do not try retrieving current directory from the server
    // (particularly with FTP)
    UnicodeString ACurrentDirectory = PeekCurrentDirectory();
    if (!ACurrentDirectory.IsEmpty())
    {
      GetSessionData()->SetRemoteDirectory(ACurrentDirectory);
    }
    if (GetSessionData()->GetFSProtocol() == fsSFTP)
    {
      GetSessionData()->SetFSProtocol(FFSProtocol == cfsSCP ? fsSCPonly : fsSFTPonly);
    }

    if (DebugAlwaysFalse(GetActive()))
    {
      Close();
    }

    Open();
  },
  __finally
  {
    GetSessionData()->SetRemoteDirectory(PrevRemoteDirectory);
    GetSessionData()->SetFSProtocol(OrigFSProtocol);
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
    if (WasInTransaction)
      BeginTransaction();
  } end_try__finally
}
//---------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData *Data, TPromptKind Kind,
  const UnicodeString AName, const UnicodeString Instructions, const UnicodeString Prompt,
  bool Echo, intptr_t MaxLen, UnicodeString &AResult)
{
  bool Result;
  std::unique_ptr<TStrings> Prompts(std::make_unique<TStringList>());
  std::unique_ptr<TStrings> Results(std::make_unique<TStringList>());
  try__finally
  {
    Prompts->AddObject(Prompt, ToObj(FLAGMASK(Echo, pupEcho)));
    Results->AddObject(AResult, ToObj(MaxLen));
    Result = PromptUser(Data, Kind, AName, Instructions, Prompts.get(), Results.get());
    AResult = Results->GetString(0);
  },
  __finally__removed
  ({
    delete Prompts;
    delete Results;
  }) end_try__finally
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData *Data, TPromptKind Kind,
  const UnicodeString AName, const UnicodeString Instructions, TStrings *Prompts, TStrings *Results)
{
  // If PromptUser is overridden in descendant class, the overridden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  // Actually no longer needed as we do not override DoPromptUser
  // anymore in TSecondaryTerminal.
  return DoPromptUser(Data, Kind, AName, Instructions, Prompts, Results);
}
//---------------------------------------------------------------------------
TTerminal * TTerminal::GetPasswordSource()
{
  return this;
}
//---------------------------------------------------------------------------
bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts, TStrings *Response)
{
  bool Result = false;

  bool PasswordOrPassphrasePrompt = ::IsPasswordOrPassphrasePrompt(Kind, Prompts);
  if (PasswordOrPassphrasePrompt)
  {
    bool &PasswordTried =
      FTunnelOpening ? FRememberedTunnelPasswordTried : FRememberedPasswordTried;
    if (!PasswordTried)
    {
      // let's expect that the main session is already authenticated and its password
      // is not written after, so no locking is necessary
      // (no longer true, once the main session can be reconnected)
      UnicodeString Password;
      if (FTunnelOpening)
      {
        Password = GetPasswordSource()->GetRememberedTunnelPassword();
      }
      else
      {
        Password = GetPasswordSource()->GetRememberedPassword();
      }
      Response->SetString(0, Password);
      if (!Response->GetString(0).IsEmpty())
      {
        LogEvent("Using remembered password.");
        Result = true;
      }
      PasswordTried = true;
    }
  }

  if (!Result)
  {
    if (PasswordOrPassphrasePrompt && !GetConfiguration()->GetRememberPassword())
    {
      Prompts->SetObj(0, ToObj(ToIntPtr(Prompts->GetObj(0)) | pupRemember));
    }

    if (GetOnPromptUser() != nullptr)
    {
      TCallbackGuard Guard(this);
      try
      {
        GetOnPromptUser()(this, Kind, AName, AInstructions, Prompts, Response, Result, nullptr);
        Guard.Verify();
      }
      catch (Exception &E)
      {
        if (!Guard.Verify(&E))
        {
          throw;
        }
      }
    }

    if (Result && PasswordOrPassphrasePrompt &&
        (GetConfiguration()->GetRememberPassword() || FLAGSET(ToIntPtr(Prompts->GetObj(0)), pupRemember)))
    {
      RawByteString EncryptedPassword = EncryptPassword(Response->GetString(0));
      if (FTunnelOpening)
      {
        GetPasswordSource()->SetRememberedTunnelPassword(EncryptedPassword);
      }
      else
      {
        GetPasswordSource()->SetRememberedPassword(EncryptedPassword);
        GetPasswordSource()->FRememberedPasswordKind = Kind;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
uint32_t TTerminal::QueryUser(const UnicodeString AQuery,
  TStrings *MoreMessages, uint32_t Answers, const TQueryParams *Params,
  TQueryType QueryType)
{
  LogEvent(FORMAT("Asking user:\n%s (%s)", AQuery, UnicodeString(MoreMessages ? MoreMessages->GetCommaText() : L"")));
  uint32_t Answer = AbortAnswer(Answers);
  if (FOnQueryUser)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnQueryUser(this, AQuery, MoreMessages, Answers, Params, Answer, QueryType, nullptr);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
  return Answer;
}
//---------------------------------------------------------------------------
uint32_t TTerminal::QueryUserException(const UnicodeString AQuery,
  Exception *E, uint32_t Answers, const TQueryParams *Params,
  TQueryType QueryType)
{
  uint32_t Result = 0;
  UnicodeString ExMessage;
  if (DebugAlwaysTrue(ExceptionMessage(E, ExMessage) || !AQuery.IsEmpty()))
  {
    std::unique_ptr<TStrings> MoreMessages(std::make_unique<TStringList>());
    try__finally
    {
      if (!ExMessage.IsEmpty() && !AQuery.IsEmpty())
      {
        MoreMessages->Add(UnformatMessage(ExMessage));
      }

      ExtException *EE = dyn_cast<ExtException>(E);
      if ((EE != nullptr) && (EE->GetMoreMessages() != nullptr))
      {
        MoreMessages->AddStrings(EE->GetMoreMessages());
      }

      // We know MoreMessages not to be NULL here,
      // AppendExceptionStackTraceAndForget should never return true
      // (indicating it had to create the string list)
      TStrings *MoreMessagesPtr = MoreMessages.release();
      DebugAlwaysFalse(AppendExceptionStackTraceAndForget(MoreMessagesPtr));
      MoreMessages.reset(MoreMessagesPtr);

      TQueryParams HelpKeywordOverrideParams;
      if (Params != nullptr)
      {
        HelpKeywordOverrideParams.Assign(*Params);
      }
      HelpKeywordOverrideParams.HelpKeyword =
        MergeHelpKeyword(HelpKeywordOverrideParams.HelpKeyword, GetExceptionHelpKeyword(E));

      Result = QueryUser(!AQuery.IsEmpty() ? AQuery : ExMessage,
        MoreMessages->GetCount() ? MoreMessages.get() : nullptr,
        Answers, &HelpKeywordOverrideParams, QueryType);
    },
    __finally__removed
    ({
      delete MoreMessages;
    }) end_try__finally
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DisplayBanner(const UnicodeString ABanner)
{
  if (GetOnDisplayBanner() != nullptr)
  {
    uintptr_t OrigParams, Params = 0;
    if (GetConfiguration()->GetForceBanners() ||
        GetConfiguration()->ShowBanner(GetSessionData()->GetSessionKey(), ABanner, Params))
    {
      bool NeverShowAgain = false;
      intptr_t Options =
        FLAGMASK(FConfiguration->GetForceBanners(), boDisableNeverShowAgain);
      OrigParams = Params;

      TCallbackGuard Guard(this);
      try
      {
        GetOnDisplayBanner()(this, GetSessionData()->GetSessionName(), ABanner, NeverShowAgain, Options, Params);
        Guard.Verify();
      }
      catch (Exception &E)
      {
        if (!Guard.Verify(&E))
        {
          throw;
        }
      }

      if (!FConfiguration->GetForceBanners() && NeverShowAgain)
      {
        FConfiguration->NeverShowBanner(GetSessionData()->GetSessionKey(), ABanner);
      }
      if (OrigParams != Params)
      {
        FConfiguration->SetBannerParams(GetSessionData()->GetSessionKey(), Params);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::HandleExtendedException(Exception *E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    TCallbackGuard Guard(this);
    try
    {
      // the event handler may destroy 'this' ...
      GetOnShowExtendedException()(this, E, nullptr);
      // .. hence guard is dismissed from destructor, to make following call no-op
      Guard.Verify();
    }
    catch (Exception &E2)
    {
      if (!Guard.Verify(&E2))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::ShowExtendedException(Exception *E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    GetOnShowExtendedException()(this, E, nullptr);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoInformation(const UnicodeString AStr, bool Status,
  intptr_t Phase)
{
  if (GetOnInformation())
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnInformation()(this, AStr, Status, Phase);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::Information(const UnicodeString AStr, bool Status)
{
  DoInformation(AStr, Status);
}
//---------------------------------------------------------------------------
void TTerminal::DoProgress(TFileOperationProgressType &ProgressData)
{
  if ((FConfiguration->GetActualLogProtocol() >= 1) &&
      ((ProgressData.GetOperation() == foCopy) || (ProgressData.GetOperation() == foMove)))
  {
    DWORD Now = GetTickCount();
    if (Now - FLastProgressLogged >= 1000)
    {
      LogEvent(L"Transfer progress: " + ProgressData.GetLogStr(false));
      FLastProgressLogged = Now;
    }
  }

  if (ProgressData.GetTransferredSize() > 0)
  {
    FFileTransferAny = true;
  }

  if (GetOnProgress() != nullptr)
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnProgress()(ProgressData);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString AFileName, bool Success, TOnceDoneOperation &OnceDoneOperation)
{
  if (GetOnFinished() != nullptr)
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnFinished()(Operation, Side, Temp, AFileName, Success, OnceDoneOperation);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::SaveCapabilities(TFileSystemInfo &FileSystemInfo)
{
  for (intptr_t Index = 0; Index < fcCount; ++Index)
  {
    FileSystemInfo.IsCapable[Index] = GetIsCapable(static_cast<TFSCapability>(Index));
  }
}
//---------------------------------------------------------------------------
bool TTerminal::GetIsCapableProtected(TFSCapability Capability) const
{
  if (DebugAlwaysTrue(FFileSystem != nullptr))
  {
    switch (Capability)
    {
      case fsBackgroundTransfers:
        return !IsEncryptingFiles();

      default:
        return FFileSystem->IsCapable(Capability);
    }
  }
  else
  {
    return false;
  }
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetAbsolutePath(const UnicodeString APath, bool Local) const
{
  return FFileSystem->GetAbsolutePath(APath, Local);
}
//---------------------------------------------------------------------------
void TTerminal::ReactOnCommand(intptr_t ACmd)
{
  bool ChangesDirectory = false;
  bool ModifiesFiles = false;

  switch (static_cast<TFSCommand>(ACmd))
  {
  case fsChangeDirectory:
  case fsHomeDirectory:
    ChangesDirectory = true;
    break;

  case fsCopyToRemote:
  case fsDeleteFile:
  case fsRenameFile:
  case fsMoveFile:
  case fsCopyFile:
  case fsCreateDirectory:
  case fsChangeMode:
  case fsChangeGroup:
  case fsChangeOwner:
  case fsChangeProperties:
  case fsLock:
    ModifiesFiles = true;
    break;

  case fsAnyCommand:
    ChangesDirectory = true;
    ModifiesFiles = true;
    break;
  }

  if (ChangesDirectory)
  {
    if (!InTransaction())
    {
      ReadCurrentDirectory();
      if (GetAutoReadDirectory())
      {
        ReadDirectory(false);
      }
    }
    else
    {
      FReadCurrentDirectoryPending = true;
      if (GetAutoReadDirectory())
      {
        FReadDirectoryPending = true;
      }
    }
  }
  else if (ModifiesFiles && GetAutoReadDirectory() && FConfiguration->GetAutoReadDirectoryAfterOp())
  {
    if (!InTransaction())
    {
      ReadDirectory(true);
    }
    else
    {
      FReadDirectoryPending = true;
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::TerminalError(const UnicodeString Msg)
{
  TerminalError(nullptr, Msg);
}
//---------------------------------------------------------------------------
void TTerminal::TerminalError(
  Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword)
{
  throw ETerminal(E, AMsg, AHelpKeyword);
}
//---------------------------------------------------------------------------
bool TTerminal::DoQueryReopen(Exception *E)
{
  EFatal *Fatal = dyn_cast<EFatal>(E);
  DebugAssert(Fatal != nullptr);
  bool Result = false;
  if ((Fatal != nullptr) && Fatal->GetReopenQueried())
  {
    Result = false;
  }
  else
  {
    intptr_t NumberOfRetries = FSessionData->GetNumberOfRetries();
    if (FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries() > 0 && NumberOfRetries >= FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries())
    {
      LogEvent(FORMAT("Reached maximum number of retries: %d", FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries()));
    }
    else
    {
      LogEvent("Connection was lost, asking what to do.");

      NumberOfRetries++;
      FSessionData->SetNumberOfRetries(NumberOfRetries);

      TQueryParams Params(qpAllowContinueOnError);
      Params.Timeout = FConfiguration->GetSessionReopenAuto();
      Params.TimeoutAnswer = qaRetry;
      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);
      Aliases[0].Default = true;
      Params.Aliases = Aliases;
      Params.AliasesCount = _countof(Aliases);
      Result = (QueryUserException(L"", E, qaRetry | qaAbort, &Params, qtError) == qaRetry);
    }

    if (Fatal != nullptr)
    {
      Fatal->SetReopenQueried(true);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::ContinueReopen(TDateTime Start) const
{
  return
    (FConfiguration->GetSessionReopenTimeout() == 0) ||
    (intptr_t(double(Now() - Start) * MSecsPerDay) < FConfiguration->GetSessionReopenTimeout());
}
//---------------------------------------------------------------------------
bool TTerminal::QueryReopen(Exception *E, intptr_t AParams,
  TFileOperationProgressType *OperationProgress)
{
  volatile TSuspendFileOperationProgress Suspend(OperationProgress);

  bool Result = DoQueryReopen(E);

  if (Result)
  {
    TDateTime Start = Now();
    do
    {
      try
      {
        Reopen(AParams);
        FSessionData->SetNumberOfRetries(0);
      }
      catch (Exception &E2)
      {
        if (!GetActive())
        {
          Result =
            ContinueReopen(Start) &&
            DoQueryReopen(&E2);
        }
        else
        {
          throw;
        }
      }
    }
    while (!GetActive() && Result);
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::FileOperationLoopQuery(Exception &E,
  TFileOperationProgressType *OperationProgress, const UnicodeString Message,
  uintptr_t AFlags, const UnicodeString SpecialRetry, const UnicodeString HelpKeyword)
{
  bool Result = false;
  GetLog()->AddException(&E);
  uint32_t Answer;
  bool AllowSkip = FLAGSET(AFlags, folAllowSkip);
  bool SkipToAllPossible = AllowSkip && (OperationProgress != nullptr);

  if (SkipToAllPossible && OperationProgress->GetSkipToAll())
  {
    Answer = qaSkip;
  }
  else
  {
    uint32_t Answers = qaRetry | qaAbort |
      FLAGMASK(AllowSkip, qaSkip) |
      FLAGMASK(SkipToAllPossible, qaAll) |
      FLAGMASK(!SpecialRetry.IsEmpty(), qaYes);
    TQueryParams Params(qpAllowContinueOnError | FLAGMASK(!AllowSkip, qpFatalAbort));
    Params.HelpKeyword = HelpKeyword;
    TQueryButtonAlias Aliases[2];
    intptr_t AliasCount = 0;

    if (FLAGSET(Answers, qaAll))
    {
      Aliases[AliasCount].Button = qaAll;
      Aliases[AliasCount].Alias = LoadStr(SKIP_ALL_BUTTON);
      AliasCount++;
    }
    if (FLAGSET(Answers, qaYes))
    {
      Aliases[AliasCount].Button = qaYes;
      Aliases[AliasCount].Alias = SpecialRetry;
      AliasCount++;
    }

    if (AliasCount > 0)
    {
      Params.Aliases = Aliases;
      Params.AliasesCount = AliasCount;
    }

    {
      volatile TSuspendFileOperationProgress Suspend(OperationProgress);
      Answer = QueryUserException(Message, &E, Answers, &Params, qtError);
    }

    if (Answer == qaAll)
    {
      DebugAssert(OperationProgress != nullptr);
      OperationProgress->SetSkipToAll();
      Answer = qaSkip;
    }
    if (Answer == qaYes)
    {
      Result = true;
      Answer = qaRetry;
    }
  }

  if (Answer != qaRetry)
  {
    if ((Answer == qaAbort) && (OperationProgress != nullptr))
    {
      OperationProgress->SetCancel(csCancel);
    }

    if (AllowSkip)
    {
      throw ESkipFile(&E, Message);
    }
    else
    {
      // this can happen only during file transfer with SCP
      throw ExtException(&E, Message);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::FileOperationLoopEnd(Exception &E,
  TFileOperationProgressType *OperationProgress, const UnicodeString AMessage,
  uintptr_t AFlags, const UnicodeString ASpecialRetry, const UnicodeString AHelpKeyword)
{
  if (isa<EAbort>(&E) ||
      isa<ESkipFile>(&E) ||
      isa<ESkipFile>(&E))
  {
    RethrowException(&E);
  }
  else if (isa<EFatal>(&E))
  {
    if (FLAGCLEAR(AFlags, folRetryOnFatal))
    {
      RethrowException(&E);
    }
    else
    {
      DebugAssert(ASpecialRetry.IsEmpty());
      std::unique_ptr<Exception> E2(std::make_unique<EFatal>(&E, AMessage, AHelpKeyword));
      if (!DoQueryReopen(E2.get()))
      {
        RethrowException(E2.get());
      }
    }
  }
  else
  {
    FileOperationLoopQuery(E, OperationProgress, AMessage, AFlags, ASpecialRetry, AHelpKeyword);
  }
}
//---------------------------------------------------------------------------
intptr_t TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
  const UnicodeString AMessage, void *Param1, void *Param2)
{
  DebugAssert(CallBackFunc);
  intptr_t Result = 0;
  FileOperationLoopCustom(this, OperationProgress, AFlags,
    AMessage, "",
  [&]()
  {
    Result = CallBackFunc(Param1, Param2);
  });
  __removed FILE_OPERATION_LOOP_END_EX(Message, Flags);

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::TranslateLockedPath(const UnicodeString APath, bool Lock)
{
  UnicodeString Path = APath;
  if (GetSessionData()->GetLockInHome() && !Path.IsEmpty() && (Path[1] == L'/'))
  {
    if (Lock)
    {
      if (Path.SubString(1, FLockDirectory.Length()) == FLockDirectory)
      {
        Path.Delete(1, FLockDirectory.Length());
        if (Path.IsEmpty())
        {
          Path = ROOTDIRECTORY;
        }
      }
    }
    else
    {
      Path = base::UnixExcludeTrailingBackslash(FLockDirectory + Path);
    }
  }
  return Path;
}
//---------------------------------------------------------------------------
void TTerminal::ClearCaches()
{
  FDirectoryCache->Clear();
  if (FDirectoryChangesCache != nullptr)
  {
    FDirectoryChangesCache->Clear();
  }
  if (FCommandSession != nullptr)
  {
    FCommandSession->ClearCaches();
  }
  FFileSystem->ClearCaches();
  FEncryptedFileNames.clear();
  FFoldersScannedForEncryptedFiles.clear();
}
//---------------------------------------------------------------------------
void TTerminal::ClearCachedFileList(const UnicodeString APath,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(APath, SubDirs);
}
//---------------------------------------------------------------------------
void TTerminal::AddCachedFileList(TRemoteFileList *FileList)
{
  FDirectoryCache->AddFileList(FileList);
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::DirectoryFileList(const UnicodeString APath, TDateTime Timestamp, bool CanLoad)
{
  TRemoteFileList *Result = nullptr;
  if (base::UnixSamePath(FFiles->GetDirectory(), APath))
  {
    if (Timestamp < FFiles->GetTimestamp())
    {
      Result = new TRemoteFileList();
      FFiles->DuplicateTo(Result);
    }
  }
  else
  {
    if (FDirectoryCache->HasNewerFileList(APath, Timestamp))
    {
      Result = new TRemoteFileList();
      DebugAlwaysTrue(FDirectoryCache->GetFileList(APath, Result));
    }
    // do not attempt to load file list if there is cached version,
    // only absence of cached version indicates that we consider
    // the directory content obsolete
    else if (CanLoad && !FDirectoryCache->HasFileList(APath))
    {
      Result = new TRemoteFileList();
      Result->SetDirectory(APath);

      try
      {
        ReadDirectory(Result);
      }
      catch (...)
      {
        SAFE_DESTROY(Result);
        throw;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::TerminalSetCurrentDirectory(const UnicodeString AValue)
{
  DebugAssert(FFileSystem);
  UnicodeString Value = TranslateLockedPath(AValue, false);
  if (AValue != FFileSystem->RemoteCurrentDirectory)
  {
    RemoteChangeDirectory(Value);
  }
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::RemoteGetCurrentDirectory()
{
  if (FFileSystem != nullptr)
  {
    // there's occasional crash when assigning FFileSystem->CurrentDirectory
    // to FCurrentDirectory, splitting the assignment to two statements
    // to locate the crash more closely
    UnicodeString CurrentDirectory = FFileSystem->RemoteCurrentDirectory();
    if (FCurrentDirectory != CurrentDirectory)
    {
      FCurrentDirectory = CurrentDirectory;
      if (FCurrentDirectory.IsEmpty())
      {
        ReadCurrentDirectory();
      }
    }
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->RemoteCurrentDirectory();
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}
//---------------------------------------------------------------------------
TRemoteTokenList * TTerminal::GetGroups()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}
//---------------------------------------------------------------------------
TRemoteTokenList * TTerminal::GetUsers()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}
//---------------------------------------------------------------------------
TRemoteTokenList * TTerminal::GetMembership()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::TerminalGetUserName() const
{
  // in future might also be implemented to detect username similar to GetUserGroups
  DebugAssert(FFileSystem != nullptr);
  UnicodeString Result = FFileSystem ? FFileSystem->RemoteGetUserName() : L"";
  // Is empty also when stored username was used
  if (Result.IsEmpty())
  {
    Result = GetSessionData()->GetUserNameExpanded();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == nullptr) || FDirectoryChangesCache->GetIsEmpty());
}
//---------------------------------------------------------------------------
void TTerminal::DoInitializeLog()
{
  if (FOnInitializeLog)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnInitializeLog(this);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoChangeDirectory()
{
  if (FOnChangeDirectory)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnChangeDirectory(this);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoReadDirectory(bool ReloadOnly)
{
  if (FOnReadDirectory)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnReadDirectory(this, ReloadOnly);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoStartReadDirectory()
{
  if (FOnStartReadDirectory)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnStartReadDirectory(this);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool &Cancel)
{
  if (FReadingCurrentDirectory && (FOnReadDirectoryProgress != nullptr))
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnReadDirectoryProgress(this, Progress, ResolvedLinks, Cancel);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
  if (FOnFindingFile != nullptr)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnFindingFile(this, L"", Cancel);
      Guard.Verify();
    }
    catch (Exception &E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::InTransaction() const
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}
//---------------------------------------------------------------------------
void TTerminal::BeginTransaction()
{
  if (FInTransaction == 0)
  {
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
  }
  FInTransaction++;

  if (FCommandSession != nullptr)
  {
    FCommandSession->BeginTransaction();
  }
}
//---------------------------------------------------------------------------
void TTerminal::EndTransaction()
{
  DoEndTransaction(false);
}
//---------------------------------------------------------------------------
void TTerminal::DoEndTransaction(bool Inform)
{
  if (FInTransaction == 0)
    TerminalError(L"Can't end transaction, not in transaction");
  DebugAssert(FInTransaction > 0);
  FInTransaction--;

  // it connection was closed due to fatal error during transaction, do nothing
  if (GetActive())
  {
    if (FInTransaction == 0)
    {
      try__finally
      {
        if (FReadCurrentDirectoryPending)
        {
          ReadCurrentDirectory();
        }

        if (FReadDirectoryPending)
        {
          if (Inform)
          {
            DoInformation(LoadStr(STATUS_OPEN_DIRECTORY), true);
          }
          ReadDirectory(!FReadCurrentDirectoryPending);
        }
      },
      __finally
      {
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
      } end_try__finally
    }
  }

  if (FCommandSession != nullptr)
  {
    FCommandSession->EndTransaction();
  }
}
//---------------------------------------------------------------------------
void TTerminal::SetExceptionOnFail(bool Value)
{
  if (Value)
  {
    FExceptionOnFail++;
  }
  else
  {
    if (FExceptionOnFail == 0)
      throw Exception(L"ExceptionOnFail is already zero.");
    FExceptionOnFail--;
  }

  if (FCommandSession != nullptr)
  {
    FCommandSession->FExceptionOnFail = FExceptionOnFail;
  }
}
//---------------------------------------------------------------------------
bool TTerminal::GetExceptionOnFail() const
{
  return static_cast<bool>(FExceptionOnFail > 0);
}
//---------------------------------------------------------------------------
void TTerminal::FatalAbort()
{
  FatalError(nullptr, L"");
}
//---------------------------------------------------------------------------
void TTerminal::FatalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword)
{
  bool SecureShellActive = (FSecureShell != nullptr) && FSecureShell->GetActive();
  if (GetActive() || SecureShellActive)
  {
    // We log this instead of exception handler, because Close() would
    // probably cause exception handler to loose pointer to TShellLog()
    LogEvent("Attempt to close connection due to fatal exception:");
    GetLog()->Add(llException, AMsg);
    GetLog()->AddException(E);

    if (GetActive())
    {
      Close();
    }

    // this may happen if failure of authentication of SSH, owned by terminal yet
    // (because the protocol was not decided yet), is detected by us (not by putty).
    // e.g. not verified host key
    if (SecureShellActive)
    {
      FSecureShell->Close();
    }
  }

  if (FCallbackGuard != nullptr)
  {
    FCallbackGuard->FatalError(E, AMsg, AHelpKeyword);
  }
  else
  {
    throw ESshFatal(E, AMsg, AHelpKeyword);
  }
}
//---------------------------------------------------------------------------
void TTerminal::CommandError(Exception *E, const UnicodeString AMsg)
{
  CommandError(E, AMsg, 0);
}
//---------------------------------------------------------------------------
uintptr_t TTerminal::CommandError(Exception *E, const UnicodeString AMsg,
  uint32_t Answers, const UnicodeString AHelpKeyword)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  DebugAssert(FCallbackGuard == nullptr);
  uintptr_t Result = 0;
  if (E && isa<EFatal>(E))
  {
    FatalError(E, AMsg, AHelpKeyword);
  }
  else if (E && isa<EAbort>(E))
  {
    // resent EAbort exception
    Abort();
  }
  else if (GetExceptionOnFail())
  {
    throw ECommand(E, AMsg, AHelpKeyword);
  }
  else if (!Answers)
  {
    ECommand ECmd(E, AMsg, AHelpKeyword);
    try__finally
    {
      HandleExtendedException(&ECmd);
    },
    __finally__removed
    ({
      delete ECmd;
    }) end_try__finally
  }
  else
  {
    // small hack to enable "skip to all" for TRetryOperationLoop
    bool CanSkip = FLAGSET(Answers, qaSkip) && (GetOperationProgress() != nullptr);
    if (CanSkip && GetOperationProgress()->GetSkipToAll())
    {
      Result = qaSkip;
    }
    else
    {
      TQueryParams Params(qpAllowContinueOnError, AHelpKeyword);
      TQueryButtonAlias Aliases[1];
      if (CanSkip)
      {
        Aliases[0].Button = qaAll;
        Aliases[0].Alias = LoadStr(SKIP_ALL_BUTTON);
        Params.Aliases = Aliases;
        Params.AliasesCount = _countof(Aliases);
        Answers |= qaAll;
      }
      Result = QueryUserException(AMsg, E, Answers, &Params, qtError);
      if (Result == qaAll)
      {
        DebugAssert(GetOperationProgress() != nullptr);
        GetOperationProgress()->SetSkipToAll();
        Result = qaSkip;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::HandleException(Exception *E)
{
  if (GetExceptionOnFail())
  {
    return false;
  }
  else
  {
    GetLog()->AddException(E);
    return true;
  }
}
//---------------------------------------------------------------------------
void TTerminal::CloseOnCompletion(
  TOnceDoneOperation Operation, const UnicodeString AMessage,
  const UnicodeString ATargetLocalPath, const UnicodeString ADestLocalFileName)
{
  if (FOperationProgressPersistence != nullptr)
  {
    DebugAssert(AMessage.IsEmpty());
    FOperationProgressOnceDoneOperation = Operation;
  }
  else
  {
    Configuration->Usage()->Inc(L"ClosesOnCompletion");
    LogEvent(L"Closing session after completed operation (as requested by user)");
    Close();
    throw ESshTerminate(nullptr,
      AMessage.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : AMessage,
      Operation, ATargetLocalPath, ADestLocalFileName);
  }
}
//---------------------------------------------------------------------------
TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
  const UnicodeString ASourceFullFileName, const TCopyParamType *CopyParam, intptr_t Params, TFileOperationProgressType *OperationProgress, bool Special) const
{
  TBatchOverwrite Result;
  if (Special &&
      (FLAGSET(Params, cpResume) || CopyParam->ResumeTransfer(ASourceFullFileName)))
  {
    Result = boResume;
  }
  else if (FLAGSET(Params, cpAppend))
  {
    Result = boAppend;
  }
  else if (CopyParam->GetNewerOnly() &&
           (((OperationProgress->GetSide() == osLocal) && GetIsCapable(fcNewerOnlyUpload)) ||
            (OperationProgress->GetSide() != osLocal)))
  {
    // no way to change batch overwrite mode when CopyParam->NewerOnly is on
    Result = boOlder;
  }
  else if (FLAGSET(Params, cpNoConfirmation) || !FConfiguration->GetConfirmOverwriting())
  {
    // no way to change batch overwrite mode when overwrite confirmations are off
    DebugAssert(OperationProgress->GetBatchOverwrite() == boNo);
    Result = boAll;
  }
  else
  {
    Result = OperationProgress->GetBatchOverwrite();
    if (!Special &&
        ((Result == boOlder) || (Result == boAlternateResume) || (Result == boResume)))
    {
      Result = boNo;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CheckRemoteFile(
  const UnicodeString AFileName, const TCopyParamType *CopyParam, intptr_t Params, TFileOperationProgressType *OperationProgress) const
{
  bool Result;
  OperationProgress->LockUserSelections();
  try__finally
  {
    Result = (EffectiveBatchOverwrite(AFileName, CopyParam, Params, OperationProgress, true) != boAll);
  },
  __finally
  {
    OperationProgress->UnlockUserSelections();
  } end_try__finally
  return Result;
}
//---------------------------------------------------------------------------
uint32_t TTerminal::ConfirmFileOverwrite(
  const UnicodeString ASourceFullFileName, const UnicodeString ATargetFileName,
  const TOverwriteFileParams *FileParams, uint32_t Answers, TQueryParams *QueryParams,
  TOperationSide Side, const TCopyParamType *CopyParam, intptr_t Params, TFileOperationProgressType *OperationProgress,
  UnicodeString AMessage)
{
  uint32_t Result = 0;
  TBatchOverwrite BatchOverwrite;
  bool CanAlternateResume =
    (FileParams != nullptr) &&
    (FileParams->DestSize < FileParams->SourceSize) &&
    !OperationProgress->GetAsciiTransfer();

  OperationProgress->LockUserSelections();
  try__finally
  {
    // duplicated in TSFTPFileSystem::SFTPConfirmOverwrite
    BatchOverwrite = EffectiveBatchOverwrite(ASourceFullFileName, CopyParam, Params, OperationProgress, true);
    bool Applicable = true;
    switch (BatchOverwrite)
    {
    case boOlder:
      Applicable = (FileParams != nullptr);
      break;

    case boAlternateResume:
      Applicable = CanAlternateResume;
      break;

    case boResume:
      Applicable = CanAlternateResume;
      break;
    }

    if (!Applicable)
    {
      TBatchOverwrite EffBatchOverwrite = EffectiveBatchOverwrite(ASourceFullFileName, CopyParam, Params, OperationProgress, false);
      DebugAssert(BatchOverwrite != EffBatchOverwrite);
      BatchOverwrite = EffBatchOverwrite;
    }

    if (BatchOverwrite == boNo)
    {
      // particularly with parallel transfers, the overal operation can be already cancelled by other parallel operation
      if (OperationProgress->GetCancel() > csContinue)
      {
        Result = qaCancel;
      }
      else
      {
        if (AMessage.IsEmpty())
        {
          // Side refers to destination side here
          AMessage = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE2 :
            REMOTE_FILE_OVERWRITE2), ATargetFileName, ATargetFileName);
        }
        if (FileParams != nullptr)
        {
          AMessage = FMTLOAD(FILE_OVERWRITE_DETAILS, AMessage,
            FormatSize(FileParams->SourceSize),
            base::UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision),
            FormatSize(FileParams->DestSize),
            base::UserModificationStr(FileParams->DestTimestamp, FileParams->DestPrecision));
        }
        if (DebugAlwaysTrue(QueryParams->HelpKeyword.IsEmpty()))
        {
          QueryParams->HelpKeyword = HELP_OVERWRITE;
        }
        Result = QueryUser(AMessage, nullptr, Answers, QueryParams);
        switch (Result)
        {
        case qaNeverAskAgain:
          FConfiguration->SetConfirmOverwriting(false);
          Result = qaYes;
          break;

        case qaYesToAll:
          BatchOverwrite = boAll;
          break;

        case qaAll:
          BatchOverwrite = boOlder;
          break;

        case qaNoToAll:
          BatchOverwrite = boNone;
          break;
        }

        // we user has not selected another batch overwrite mode,
        // keep the current one. note that we may get here even
        // when batch overwrite was selected already, but it could not be applied
        // to current transfer (see condition above)
        if (BatchOverwrite != boNo)
        {
          GetOperationProgress()->SetBatchOverwrite(BatchOverwrite);
        }
      }
    }
  },
  __finally
  {
    OperationProgress->UnlockUserSelections();
  } end_try__finally

  if (BatchOverwrite != boNo)
  {
    switch (BatchOverwrite)
    {
    case boAll:
      Result = qaYes;
      break;

    case boNone:
      Result = qaNo;
      break;

    case boOlder:
      {
        if (FileParams == nullptr)
        {
          Result = qaNo;
        }
        else
        {
          TModificationFmt Precision = base::LessDateTimePrecision(FileParams->SourcePrecision, FileParams->DestPrecision);
          TDateTime ReducedSourceTimestamp =
            base::ReduceDateTimePrecision(FileParams->SourceTimestamp, Precision);
          TDateTime ReducedDestTimestamp =
            base::ReduceDateTimePrecision(FileParams->DestTimestamp, Precision);

          Result =
            CompareFileTime(ReducedSourceTimestamp, ReducedDestTimestamp) > 0 ?
              qaYes : qaNo;

          LogEvent(FORMAT("Source file timestamp is [%s], destination timestamp is [%s], will%s overwrite",
            StandardTimestamp(ReducedSourceTimestamp),
            StandardTimestamp(ReducedDestTimestamp),
            UnicodeString(Result == qaYes ? L"" : L" not")));
        }
      }
      break;

    case boAlternateResume:
      DebugAssert(CanAlternateResume);
      Result = qaSkip; // ugh
      break;

    case boAppend:
      Result = qaRetry;
      break;

    case boResume:
      Result = qaRetry;
      break;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::FileModified(const TRemoteFile *AFile,
  const UnicodeString AFileName, bool ClearDirectoryChange)
{
  UnicodeString ParentDirectory;
  UnicodeString Directory;

  if (GetSessionData()->GetCacheDirectories() || GetSessionData()->GetCacheDirectoryChanges())
  {
    if ((AFile != nullptr) && (AFile->GetDirectory() != nullptr))
    {
      if (AFile->GetIsDirectory())
      {
        Directory = AFile->GetDirectory()->GetFullDirectory() + AFile->GetFileName();
      }
      ParentDirectory = AFile->GetDirectory()->GetDirectory();
    }
    else if (!AFileName.IsEmpty())
    {
      ParentDirectory = base::UnixExtractFilePath(AFileName);
      if (ParentDirectory.IsEmpty())
      {
        ParentDirectory = RemoteGetCurrentDirectory();
      }

      // this case for scripting
      if ((AFile != nullptr) && AFile->GetIsDirectory())
      {
        Directory = base::UnixIncludeTrailingBackslash(ParentDirectory) +
          base::UnixExtractFileName(AFile->GetFileName());
      }
    }
  }

  if (GetSessionData()->GetCacheDirectories())
  {
    if (!Directory.IsEmpty())
    {
      DirectoryModified(Directory, true);
    }
    if (!ParentDirectory.IsEmpty())
    {
      DirectoryModified(ParentDirectory, false);
    }
  }

  if (GetSessionData()->GetCacheDirectoryChanges() && ClearDirectoryChange)
  {
    if (!Directory.IsEmpty())
    {
      FDirectoryChangesCache->ClearDirectoryChange(Directory);
      FDirectoryChangesCache->ClearDirectoryChangeTarget(Directory);
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DirectoryModified(const UnicodeString APath, bool SubDirs)
{
  if (APath.IsEmpty())
  {
    ClearCachedFileList(RemoteGetCurrentDirectory(), SubDirs);
  }
  else
  {
    ClearCachedFileList(APath, SubDirs);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
  AddCachedFileList(FileList);
}
//---------------------------------------------------------------------------
void TTerminal::ReloadDirectory()
{
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(RemoteGetCurrentDirectory(), false);
  }
  if (GetSessionData()->GetCacheDirectoryChanges())
  {
    DebugAssert(FDirectoryChangesCache != nullptr);
    FDirectoryChangesCache->ClearDirectoryChange(RemoteGetCurrentDirectory());
  }

  ReadCurrentDirectory();
  FReadCurrentDirectoryPending = false;
  ReadDirectory(true);
  FReadDirectoryPending = false;
}
//---------------------------------------------------------------------------
void TTerminal::RefreshDirectory()
{
  if (GetSessionData()->GetCacheDirectories())
  {
    LogEvent("Not refreshing directory, caching is off.");
  }
  else if (FDirectoryCache->HasNewerFileList(RemoteGetCurrentDirectory(), FFiles->GetTimestamp()))
  {
    // Second parameter was added to allow (rather force) using the cache.
    // Before, the directory was reloaded always, it seems useless,
    // has it any reason?
    ReadDirectory(true, true);
    FReadDirectoryPending = false;
  }
}
//---------------------------------------------------------------------------
void TTerminal::EnsureNonExistence(const UnicodeString AFileName)
{
  // if filename doesn't contain path, we check for existence of file
  if ((base::UnixExtractFileDir(AFileName).IsEmpty()) &&
      base::UnixSamePath(RemoteGetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile *File = FFiles->FindFile(AFileName);
    if (File)
    {
      if (File->GetIsDirectory())
      {
        throw ECommand(nullptr, FMTLOAD(RENAME_CREATE_DIR_EXISTS, AFileName));
      }
      throw ECommand(nullptr, FMTLOAD(RENAME_CREATE_FILE_EXISTS, AFileName));
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::LogEvent(const UnicodeString AStr)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, AStr);
  }
}
//---------------------------------------------------------------------------
void TTerminal::LogEvent(int Level, const UnicodeString AStr)
{
  if (Log->Logging && (Configuration->ActualLogProtocol >= Level))
  {
    Log->Add(llMessage, AStr);
  }
}
//---------------------------------------------------------------------------
void TTerminal::RollbackAction(TSessionAction &Action,
  TFileOperationProgressType *OperationProgress, Exception *E)
{
  // ESkipFile without "cancel" is file skip,
  // and we do not want to record skipped actions.
  // But ESkipFile with "cancel" is abort and we want to record that.
  // Note that TSCPFileSystem modifies the logic of RollbackAction little bit.
  if (isa<ESkipFile>(E) &&
      ((OperationProgress == nullptr) ||
       (OperationProgress->GetCancel() == csContinue)))
  {
    Action.Cancel();
  }
  else
  {
    Action.Rollback(E);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoStartup()
{
  LogEvent("Doing startup conversation with host.");
  BeginTransaction();
  try__finally
  {
    DoInformation(LoadStr(STATUS_STARTUP), true);

    // Make sure that directory would be loaded at last
    FReadCurrentDirectoryPending = true;
    FReadDirectoryPending = GetAutoReadDirectory();

    FFileSystem->DoStartup();

    LookupUsersGroups();

    if (!GetSessionData()->GetRemoteDirectory().IsEmpty())
    {
      if (SessionData->UpdateDirectories)
      {
        ExceptionOnFail = true;
        try
        {
          RemoteChangeDirectory(SessionData->RemoteDirectory);
        }
        catch (...)
        {
          if (!Active)
          {
            LogEvent(L"Configured initial remote directory cannot be opened, staying in the home directory.");
            throw;
          }
        }
        ExceptionOnFail = false;
      }
      else
      {
        RemoteChangeDirectory(SessionData->RemoteDirectory);
      }
    }
  },
  __finally
  {
    DoEndTransaction(true);
  } end_try__finally
  LogEvent("Startup conversation with host finished.");
}
//---------------------------------------------------------------------------
void TTerminal::ReadCurrentDirectory()
{
  DebugAssert(FFileSystem);
  try
  {
    // reset flag in case we are called externally (like from console dialog)
    FReadCurrentDirectoryPending = false;

    LogEvent("Getting current directory name.");
    UnicodeString OldDirectory = FFileSystem->RemoteCurrentDirectory();

    FFileSystem->ReadCurrentDirectory();
    ReactOnCommand(fsCurrentDirectory);

    if (GetSessionData()->GetCacheDirectoryChanges())
    {
      DebugAssert(FDirectoryChangesCache != nullptr);
      UnicodeString CurrentDirectory = RemoteGetCurrentDirectory();
      if (!CurrentDirectory.IsEmpty() && !FLastDirectoryChange.IsEmpty() && (CurrentDirectory != OldDirectory))
      {
        FDirectoryChangesCache->AddDirectoryChange(OldDirectory,
          FLastDirectoryChange, CurrentDirectory);
      }
      // not to breake the cache, if the next directory change would not
      // be initialized by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange.Clear();
    }

    if (OldDirectory.IsEmpty())
    {
      FLockDirectory = (GetSessionData()->GetLockInHome() ?
        FFileSystem->RemoteCurrentDirectory() : UnicodeString(""));
    }
    // if (OldDirectory != FFileSystem->GetCurrDirectory())
    {
      DoChangeDirectory();
    }
  }
  catch (Exception &E)
  {
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
{
  bool LoadedFromCache = false;

  if (GetSessionData()->GetCacheDirectories() && FDirectoryCache->HasFileList(RemoteGetCurrentDirectory()))
  {
    if (ReloadOnly && !ForceCache)
    {
      LogEvent("Cached directory not reloaded.");
    }
    else
    {
      DoStartReadDirectory();
      try__finally
      {
        LoadedFromCache = FDirectoryCache->GetFileList(RemoteGetCurrentDirectory(), FFiles);
      },
      __finally
      {
        DoReadDirectory(ReloadOnly);
      } end_try__finally

      if (LoadedFromCache)
      {
        LogEvent("Directory content loaded from cache.");
      }
      else
      {
        LogEvent("Cached Directory content has been removed.");
      }
    }
  }

  if (!LoadedFromCache)
  {
    DoStartReadDirectory();
    FReadingCurrentDirectory = true;
    bool Cancel = false; // dummy
    DoReadDirectoryProgress(0, 0, Cancel);

    try
    {
      TRemoteDirectory *Files = new TRemoteDirectory(this, FFiles);
      try__finally
      {
        Files->SetDirectory(RemoteGetCurrentDirectory());
        CustomReadDirectory(Files);
      },
      __finally
      {
        DoReadDirectoryProgress(-1, 0, Cancel);
        FReadingCurrentDirectory = false;
        FOldFiles->Reset();
        FOldFiles->AddFiles(FFiles);
        FFiles = Files;
        try__finally
        {
          DoReadDirectory(ReloadOnly);
        },
        __finally
        {
          // delete only after loading new files to dir view,
          // not to destroy the file objects that the view holds
          // (can be issue in multi threaded environment, such as when the
          // terminal is reconnecting in the terminal thread)
          FOldFiles->Reset();
        } end_try__finally
        if (GetActive())
        {
          if (GetSessionData()->GetCacheDirectories())
          {
            DirectoryLoaded(FFiles);
          }
        }
      } end_try__finally
    }
    catch (Exception &E)
    {
      CommandError(&E, FMTLOAD(LIST_DIR_ERROR, FFiles->GetDirectory()));
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetRemoteFileInfo(TRemoteFile *AFile) const
{
  return
    FORMAT("%s;%s;%lld;%s;%d;%s;%s;%s;%d",
       AFile->GetFileName(), AFile->GetType(), AFile->GetSize(), StandardTimestamp(AFile->GetModification()), int(AFile->GetModificationFmt()),
       AFile->GetFileOwner().GetLogText(), AFile->GetFileGroup().GetLogText(), AFile->GetRights()->GetText(),
       AFile->GetAttr());
}
//---------------------------------------------------------------------------
void TTerminal::LogRemoteFile(TRemoteFile *AFile)
{
  // optimization
  if (GetLog()->GetLogging() && AFile)
  {
    LogEvent(GetRemoteFileInfo(AFile));
  }
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::FormatFileDetailsForLog(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size) const
{
  UnicodeString Result;
  // optimization
  if (GetLogConst()->GetLogging())
  {
    Result = FORMAT("'%s' [%s] [%s]", AFileName, UnicodeString(AModification != TDateTime() ? StandardTimestamp(AModification) : UnicodeString(L"n/a")), ::Int64ToStr(Size));
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::LogFileDetails(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size)
{
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT("File: %s", FormatFileDetailsForLog(AFileName, AModification, Size)));
  }
}
//---------------------------------------------------------------------------
void TTerminal::LogFileDone(TFileOperationProgressType *OperationProgress, const UnicodeString DestFileName)
{
  if (FDestFileName.IsEmpty())
  {
    FDestFileName = DestFileName;
  }
  else
  {
    FMultipleDestinationFiles = true;
  }

  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT("Transfer done: '%s' => '%s' [%s]", OperationProgress->GetFullFileName(), DestFileName, Int64ToStr(OperationProgress->GetTransferredSize())));
  }
}
//---------------------------------------------------------------------------
void TTerminal::CustomReadDirectory(TRemoteFileList *AFileList)
{
  DebugAssert(AFileList);
  DebugAssert(FFileSystem);

  // To match FTP upload/download, we also limit directory listing.
  // For simiplicity, we limit it unconditionally, for all protocols for any kind of errors.
  bool FileTransferAny = false;
  TRobustOperationLoop RobustLoop(this, FOperationProgress, &FileTransferAny);

  do
  {
    try
    {
      // Record even an attempt, to avoid listing a directory over and over again, when it is not readable actually
      if (IsEncryptingFiles())
      {
        FFoldersScannedForEncryptedFiles.insert(AFileList->Directory());
      }

    }
    catch (Exception &E)
    {
      // Do not retry for initial listing of directory,
      // we instead retry whole connection attempt,
      // what would be done anyway (but Open is not ready for recursion).
      if ((FOpening > 0) ||
          !RobustLoop.TryReopen(E))
      {
        throw;
      }
    }
  }
  while (RobustLoop.Retry());

  if (GetLog()->GetLogging())
  if (Log->Logging && (Configuration->ActualLogProtocol >= 0))
  {
    for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      LogRemoteFile(AFileList->GetFile(Index));
    }
  }

  ReactOnCommand(fsListDirectory);
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::ReadDirectoryListing(const UnicodeString Directory, const TFileMasks &Mask)
{
  TRemoteFileList *FileList;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    FileList = nullptr;
    TLsSessionAction Action(GetActionLog(), GetAbsolutePath(Directory, true));

    try
    {
      FileList = DoReadDirectoryListing(Directory, false);
      if (FileList != nullptr)
      {
        intptr_t Index = 0;
        while (Index < FileList->GetCount())
        {
          TRemoteFile *File = FileList->GetFile(Index);
          TFileMasks::TParams Params;
          Params.Size = File->GetSize();
          Params.Modification = File->GetModification();
          // Have to use UnicodeString(), instead of L"", as with that
          // overload with (UnicodeString, bool, bool, TParams*) wins
          if (!Mask.Matches(File->GetFileName(), false, UnicodeString(), &Params))
          {
            FileList->Delete(Index);
          }
          else
          {
            ++Index;
          }
        }

        Action.FileList(FileList);
      }
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFile * TTerminal::ReadFileListing(const UnicodeString APath)
{
  TRemoteFile *File = nullptr;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    File = nullptr;
    TStatSessionAction Action(GetActionLog(), GetAbsolutePath(APath, true));
    try
    {
      // reset caches
      AnnounceFileListOperation();
      ReadFile(APath, File);
      Action.File(File);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return File;
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::CustomReadDirectoryListing(const UnicodeString Directory, bool UseCache)
{
  TRemoteFileList *FileList = nullptr;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FileList = DoReadDirectoryListing(Directory, UseCache);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E);
    }
  }
  while (RetryLoop.Retry());
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::DoReadDirectoryListing(const UnicodeString ADirectory, bool UseCache)
{
  std::unique_ptr<TRemoteFileList> FileList(std::make_unique<TRemoteFileList>());
  try__catch
  {
    bool Cache = UseCache && GetSessionData()->GetCacheDirectories();
    bool LoadedFromCache = Cache && FDirectoryCache->HasFileList(ADirectory);
    if (LoadedFromCache)
    {
      LoadedFromCache = FDirectoryCache->GetFileList(ADirectory, FileList.get());
    }

    if (!LoadedFromCache)
    {
      FileList->SetDirectory(ADirectory);

      SetExceptionOnFail(true);
      try__finally
      {
        ReadDirectory(FileList.get());
      },
      __finally
      {
        SetExceptionOnFail(false);
      } end_try__finally

      if (Cache)
      {
        AddCachedFileList(FileList.get());
      }
    }
  }
  catch__removed
  ({
    delete FileList;
    throw;
  })
  return FileList.release();
}
//---------------------------------------------------------------------------
bool TTerminal::DeleteContentsIfDirectory(
  const UnicodeString AFileName, const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction &Action)
{
  bool Dir = (AFile != nullptr) && AFile->GetIsDirectory() && CanRecurseToDirectory(AFile);

  if (Dir && FLAGCLEAR(AParams, dfNoRecursive))
  {
    try
    {
      ProcessDirectory(AFileName, nb::bind(&TTerminal::RemoteDeleteFile, this), &AParams);
    }
    catch(...)
    {
      Action.Cancel();
      throw;
    }
  }
  return Dir && !AFile->IsSymLink;
}
//---------------------------------------------------------------------------
void TTerminal::ProcessDirectory(const UnicodeString ADirName,
  TProcessFileEvent CallBackFunc, void *AParam, bool UseCache, bool IgnoreErrors)
{
  std::unique_ptr<TRemoteFileList> FileList;
  if (IgnoreErrors)
  {
    SetExceptionOnFail(true);
    try__finally
    {
      try
      {
        FileList.reset(CustomReadDirectoryListing(ADirName, UseCache));
      }
      catch (...)
      {
        if (!GetActive())
        {
          throw;
        }
      }
    },
    __finally
    {
      SetExceptionOnFail(false);
    } end_try__finally
  }
  else
  {
    FileList.reset(CustomReadDirectoryListing(ADirName, UseCache));
  }

  // skip if directory listing fails and user selects "skip"
  if (FileList.get())
  {
    try__finally
    {
      UnicodeString Directory = base::UnixIncludeTrailingBackslash(ADirName);

      for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
      {
        TRemoteFile *File = FileList->GetFile(Index);
        if (IsRealFile(File->FileName))
        {
          CallBackFunc(Directory + File->GetFileName(), File, AParam);
          // We should catch ESkipFile here as we do in ProcessFiles.
          // Now we have to handle ESkipFile in every callback implementation.
        }
      }
    },
    __finally__removed
    ({
      delete FileList;
    }) end_try__finally
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadDirectory(TRemoteFileList *AFileList)
{
  DebugAssert(AFileList);

  try
  {
    CustomReadDirectory(AFileList);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(LIST_DIR_ERROR, AFileList->GetDirectory()));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadSymlink(TRemoteFile *SymlinkFile,
  TRemoteFile *&File)
{
  DebugAssert(FFileSystem);
  try
  {
    LogEvent(FORMAT("Reading symlink \"%s\".", SymlinkFile->GetFileName()));
    FFileSystem->ReadSymlink(SymlinkFile, File);
    ReactOnCommand(fsReadSymlink);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName()));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadFile(const UnicodeString AFileName,
  TRemoteFile *&AFile)
{
  DebugAssert(FFileSystem);
  AFile = nullptr;
  try
  {
    LogEvent(FORMAT("Listing file \"%s\".", AFileName));
    FFileSystem->ReadFile(AFileName, AFile);
    ReactOnCommand(fsListFile);
    LogRemoteFile(AFile);
  }
  catch (Exception &E)
  {
    if (AFile)
    {
      SAFE_DESTROY(AFile);
    }
    AFile = nullptr;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, AFileName));
  }
}
//---------------------------------------------------------------------------
bool TTerminal::FileExists(const UnicodeString AFileName, TRemoteFile **AFile)
{
  bool Result;
  TRemoteFile *File = nullptr;
  try
  {
    SetExceptionOnFail(true);
    try__finally
    {
      ReadFile(base::UnixExcludeTrailingBackslash(AFileName), File);
    },
    __finally
    {
      SetExceptionOnFail(false);
    } end_try__finally

    if (AFile != nullptr)
    {
      *AFile = File;
    }
    else
    {
      SAFE_DESTROY(File);
    }
    Result = true;
  }
  catch (...)
  {
    if (GetActive())
    {
      Result = false;
    }
    else
    {
      throw;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}
//---------------------------------------------------------------------------
void TTerminal::OperationFinish(
  TFileOperationProgressType * Progress, const void * /*Item*/, const UnicodeString AFileName,
  bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  Progress->Finish(AFileName, Success, OnceDoneOperation);
}
//---------------------------------------------------------------------------
void TTerminal::OperationStart(
  TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, intptr_t Count)
{
  OperationStart(Progress, Operation, Side, Count, false, UnicodeString(), 0);
}
//---------------------------------------------------------------------------
void TTerminal::OperationStart(
  TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, intptr_t Count,
  bool Temp, const UnicodeString ADirectory, uint64_t CPSLimit)
{
  if (FOperationProgressPersistence != nullptr)
  {
    Progress.Restore(*FOperationProgressPersistence);
  }
  Progress.Start(Operation, Side, Count, Temp, ADirectory, CPSLimit);
  DebugAssert(FOperationProgress == nullptr);
  FOperationProgress = &Progress;
}
//---------------------------------------------------------------------------
void TTerminal::OperationStop(TFileOperationProgressType & Progress)
{
  DebugAssert(FOperationProgress == &Progress);
  if (FOperationProgressPersistence != nullptr)
  {
    Progress.Store(*FOperationProgressPersistence);
  }
  FOperationProgress = nullptr;
  Progress.Stop();
}
//---------------------------------------------------------------------------
bool TTerminal::ProcessFiles(TStrings * AFileList,
  TFileOperation Operation, TProcessFileEvent ProcessFile, void *Param,
  TOperationSide Side, bool Ex)
{
  DebugAssert(FFileSystem);
  DebugAssert(AFileList);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try
  {
    TFileOperationProgressType Progress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));
    OperationStart(Progress, Operation, Side, AFileList->Count);

    FOperationProgress = &Progress; //-V506
    try__finally
    {
      if (Side == osRemote)
      {
        // DebugAssert(FFileSystem != nullptr);
        BeginTransaction();
      }

      try__finally
      {
        intptr_t Index = 0;
        while ((Index < AFileList->GetCount()) && (Progress.GetCancel() == csContinue))
        {
          UnicodeString FileName = AFileList->GetString(Index);
          TRemoteFile * File = reinterpret_cast<TRemoteFile *>(AFileList->GetObj(Index));
          try
          {
            bool Success = false;
            try__finally
            {
              Expects(!Ex);
              if (!Ex)
              {
                ProcessFile(FileName, File, Param);
              }
              else
              {
#if 0
                // not used anymore
                TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                ProcessFileEx(FileName, File, Param, Index);
#endif //if 0
              }
              Success = true;
            },
            __finally
            {
              OperationFinish(&Progress, File, FileName, Success, OnceDoneOperation);
            } end_try__finally
          }
          catch (ESkipFile &E)
          {
            DEBUG_PRINTF("before HandleException");
            volatile TSuspendFileOperationProgress Suspend(GetOperationProgress());
            if (!HandleException(&E))
            {
              throw;
            }
          }
          ++Index;
        }
      },
      __finally
      {
         if (Side == osRemote)
         {
           EndTransaction();
         }
      } end_try__finally

      if (Progress.GetCancel() == csContinue)
      {
        Result = true;
      }
    },
    __finally
    {
      OperationStop(Progress);
    } end_try__finally
  }
  catch (...)
  {
    OnceDoneOperation = odoIdle;
    // this was missing here. was it by purpose?
    // without it any error message is lost
    throw;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
// not used anymore
#if 0
bool TTerminal::ProcessFilesEx(TStrings *FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void *Param, TOperationSide Side)
{
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
}
#endif
//---------------------------------------------------------------------------
TStrings * TTerminal::GetFixedPaths() const
{
  DebugAssert(FFileSystem != nullptr);
  return FFileSystem ? FFileSystem->GetFixedPaths() : nullptr;
}
//---------------------------------------------------------------------------
bool  TTerminal::GetResolvingSymlinks() const
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}
//---------------------------------------------------------------------------
TUsableCopyParamAttrs  TTerminal::UsableCopyParamAttrs(intptr_t Params) const
{
  TUsableCopyParamAttrs Result;
  Result.General =
    FLAGMASK(!GetIsCapable(fcTextMode), cpaNoTransferMode) |
    FLAGMASK(!GetIsCapable(fcModeChanging), cpaNoRights) |
    FLAGMASK(!GetIsCapable(fcModeChanging), cpaNoPreserveReadOnly) |
    FLAGMASK(FLAGSET(Params, cpDelete), cpaNoClearArchive) |
    FLAGMASK(!GetIsCapable(fcIgnorePermErrors), cpaNoIgnorePermErrors) |
    // the following three are never supported for download,
    // so when they are not suppored for upload too,
    // set them in General flags, so that they do not get enabled on
    // Synchronize dialog.
    FLAGMASK(!GetIsCapable(fcModeChangingUpload), cpaNoRights) |
    FLAGMASK(!GetIsCapable(fcRemoveCtrlZUpload), cpaNoRemoveCtrlZ) |
    FLAGMASK(!GetIsCapable(fcRemoveBOMUpload), cpaNoRemoveBOM) |
    FLAGMASK(!GetIsCapable(fcPreservingTimestampDirs), cpaNoPreserveTimeDirs) |
    FLAGMASK(!GetIsCapable(fcResumeSupport), cpaNoResumeSupport) |
    FLAGMASK(!IsEncryptingFiles(), cpaNoEncryptNewFiles);
  Result.Download = Result.General | cpaNoClearArchive |
    cpaNoIgnorePermErrors |
    // May be already set in General flags, but it's unconditional here
    cpaNoRights | cpaNoRemoveCtrlZ | cpaNoRemoveBOM | cpaNoEncryptNewFiles;
  Result.Upload = Result.General | cpaNoPreserveReadOnly |
    FLAGMASK(!GetIsCapable(fcPreservingTimestampUpload), cpaNoPreserveTime);
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::IsRecycledFile(const UnicodeString AFileName)
{
  bool Result = !GetSessionData()->GetRecycleBinPath().IsEmpty();
  if (Result)
  {
    UnicodeString Path = base::UnixExtractFilePath(AFileName);
    if (Path.IsEmpty())
    {
      Path = RemoteGetCurrentDirectory();
    }
    Result = base::UnixSamePath(Path, GetSessionData()->GetRecycleBinPath());
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::RecycleFile(UnicodeString AFileName,
  const TRemoteFile *AFile)
{
  UnicodeString FileName = AFileName;
  if (FileName.IsEmpty())
  {
    DebugAssert(AFile != nullptr);
    if (AFile)
    {
      FileName = AFile->GetFileName();
    }
  }

  if (!IsRecycledFile(FileName))
  {
    LogEvent(FORMAT("Moving file \"%s\" to remote recycle bin '%s'.",
      FileName, GetSessionData()->GetRecycleBinPath()));

    TMoveFileParams Params;
    Params.Target = GetSessionData()->GetRecycleBinPath();
#if defined(__BORLANDC__)
    Params.FileMask = FORMAT("*-%s.*", (FormatDateTime(L"yyyymmdd-hhnnss", Now())));
#else
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = Now();
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L"%04d%02d%02d-%02d%02d%02d", Y, M, D, H, N, S);
      // Params.FileMask = FORMAT(L"*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()));
      Params.FileMask = FORMAT(L"*-%s.*", dt);
    }
#endif

    TerminalMoveFile(FileName, AFile, &Params);

    if ((GetOperationProgress() != nullptr) && (GetOperationProgress()->Operation() == foDelete))
    {
      GetOperationProgress()->Succeeded();
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::TryStartOperationWithFile(
  const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  bool Result = true;
  if ((GetOperationProgress() != nullptr) &&
      ((GetOperationProgress()->GetOperation() == Operation1) ||
       ((Operation2 != foNone) && (GetOperationProgress()->GetOperation() == Operation2))))
  {
    if (GetOperationProgress()->GetCancel() != csContinue)
    {
      Result = false;
    }
    else
    {
      GetOperationProgress()->SetFile(AFileName);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::StartOperationWithFile(
  const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  if (!TryStartOperationWithFile(AFileName, Operation1, Operation2))
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
void TTerminal::RemoteDeleteFile(UnicodeString AFileName,
  const TRemoteFile *AFile, void *AParams)
{
  UnicodeString FileName = AFileName;
  if (FileName.IsEmpty() && AFile)
  {
    FileName = AFile->GetFileName();
  }
  StartOperationWithFile(FileName, foDelete);
  intptr_t Params = (AParams != nullptr) ? *(static_cast<intptr_t *>(AParams)) : 0;
  bool Recycle =
    FLAGCLEAR(Params, dfForceDelete) &&
    (GetSessionData()->GetDeleteToRecycleBin() != FLAGSET(Params, dfAlternative)) &&
    !GetSessionData()->GetRecycleBinPath().IsEmpty();
  if (Recycle && !IsRecycledFile(FileName))
  {
    RecycleFile(FileName, AFile);
  }
  else
  {
    LogEvent(FORMAT("Deleting file \"%s\".", FileName));
    FileModified(AFile, FileName, true);
    DoDeleteFile(FileName, AFile, Params);
    // Forget if file was or was not encrypted and use user preferences, if we ever recreate it.
    FEncryptedFileNames.erase(GetAbsolutePath(FileName, true));
    ReactOnCommand(fsDeleteFile);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoDeleteFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, intptr_t Params)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TRmSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true));
    try
    {
      DebugAssert(FFileSystem);
      // 'File' parameter: SFTPFileSystem needs to know if file is file or directory
      FFileSystem->RemoteDeleteFile(AFileName, AFile, Params, Action);
      if ((OperationProgress != nullptr) && (OperationProgress->Operation() == foDelete))
      {
        OperationProgress->Succeeded();
      }
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(DELETE_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
bool TTerminal::RemoteDeleteFiles(TStrings *AFilesToDelete, intptr_t Params)
{
  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TODO("avoid resolving symlinks while reading subdirectories.");
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return this->ProcessFiles(AFilesToDelete, foDelete, nb::bind(&TTerminal::RemoteDeleteFile, this), &Params);
}
//---------------------------------------------------------------------------
void TTerminal::DeleteLocalFile(UnicodeString AFileName,
  const TRemoteFile * /*AFile*/, void *Params)
{
  StartOperationWithFile(AFileName, foDelete);
  intptr_t Deleted;
  if (OnDeleteLocalFile == nullptr)
  {
    Deleted = RecursiveDeleteFileChecked(AFileName, false);
  }
  else
  {
    OnDeleteLocalFile(AFileName, FLAGSET(*((intptr_t *)Params), dfAlternative), Deleted);
  }
  if (DebugAlwaysTrue((OperationProgress != nullptr) && (OperationProgress->Operation() == foDelete)))
  {
    OperationProgress->Succeeded(Deleted);
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DeleteLocalFiles(TStrings *AFileList, intptr_t Params)
{
  return ProcessFiles(AFileList, foDelete, nb::bind(&TTerminal::DeleteLocalFile, this), &Params, osLocal);
}
//---------------------------------------------------------------------------
void TTerminal::CustomCommandOnFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, void *AParams)
{
  TCustomCommandParams *Params = get_as<TCustomCommandParams>(AParams);
  UnicodeString LocalFileName = AFileName;
  if (AFileName.IsEmpty() && AFile)
  {
    LocalFileName = AFile->GetFileName();
  }
  StartOperationWithFile(LocalFileName, foCustomCommand);
  LogEvent(FORMAT("Executing custom command \"%s\" (%d) on file \"%s\".",
      Params->Command, Params->Params, LocalFileName));
  FileModified(AFile, LocalFileName);
  DoCustomCommandOnFile(LocalFileName, AFile, Params->Command, Params->Params,
    Params->OutputEvent);
  ReactOnCommand(fsAnyCommand);
}
//---------------------------------------------------------------------------
void TTerminal::DoCustomCommandOnFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, const UnicodeString ACommand, intptr_t AParams,
  TCaptureOutputEvent OutputEvent)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      if (GetIsCapable(fcAnyCommand))
      {
        DebugAssert(FFileSystem);
        DebugAssert(GetIsCapable(fcShellAnyCommand));
        FFileSystem->CustomCommandOnFile(AFileName, AFile, ACommand, AParams, OutputEvent);
      }
      else
      {
        DebugAssert(GetCommandSessionOpened());
        DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
        LogEvent("Executing custom command on command session.");

        if (FCommandSession->RemoteGetCurrentDirectory() != RemoteGetCurrentDirectory())
        {
          FCommandSession->TerminalSetCurrentDirectory(RemoteGetCurrentDirectory());
          // We are likely in transaction, so ReadCurrentDirectory won't get called
          // until transaction ends. But we need to know CurrentDirectory to
          // expand !/ pattern.
          // Doing this only, when current directory of the main and secondary shell differs,
          // what would be the case before the first file in transaction.
          // Otherwise we would be reading pwd before every time as the
          // CustomCommandOnFile on its own sets FReadCurrentDirectoryPending
          if (FCommandSession->FReadCurrentDirectoryPending)
          {
            FCommandSession->ReadCurrentDirectory();
          }
        }
        FCommandSession->FFileSystem->CustomCommandOnFile(AFileName, AFile, ACommand,
          AParams, OutputEvent);
      }
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, FMTLOAD(CUSTOM_COMMAND_ERROR, ACommand, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::CustomCommandOnFiles(const UnicodeString ACommand,
  intptr_t AParams, TStrings *AFiles, TCaptureOutputEvent OutputEvent)
{
  if (!TRemoteCustomCommand().IsFileListCommand(ACommand))
  {
    TCustomCommandParams Params;
    Params.Command = ACommand;
    Params.Params = AParams;
    Params.OutputEvent = OutputEvent;
    ProcessFiles(AFiles, foCustomCommand, nb::bind(&TTerminal::CustomCommandOnFile, this), &Params);
  }
  else
  {
    UnicodeString FileList;
    for (intptr_t Index = 0; Index < AFiles->GetCount(); ++Index)
    {
      TRemoteFile *File = AFiles->GetAs<TRemoteFile>(Index);
      bool Dir = File->GetIsDirectory() && CanRecurseToDirectory(File);

      if (!Dir || FLAGSET(AParams, ccApplyToDirectories))
      {
        if (!FileList.IsEmpty())
        {
          FileList += L" ";
        }

        FileList += L"\"" + ShellDelimitStr(AFiles->GetString(Index), L'"') + L"\"";
      }
    }

    TCustomCommandData Data(this);
    UnicodeString Cmd =
      TRemoteCustomCommand(Data, RemoteGetCurrentDirectory(), L"", FileList).
      Complete(ACommand, true);
    if (!DoOnCustomCommand(Cmd))
    {
      DoAnyCommand(Cmd, OutputEvent, nullptr);
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DoOnCustomCommand(const UnicodeString Command)
{
  bool Result = false;
  if (FOnCustomCommand != nullptr)
  {
    FOnCustomCommand(this, Command, Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::ChangeFileProperties(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*const TRemoteProperties*/ void *Properties)
{
  TRemoteProperties *RProperties = get_as<TRemoteProperties>(Properties);
  DebugAssert(RProperties && !RProperties->Valid.Empty());
  UnicodeString LocalFileName = AFileName;
  if (AFileName.IsEmpty() && AFile)
  {
    LocalFileName = AFile->GetFileName();
  }
  StartOperationWithFile(LocalFileName, foSetProperties);
  if (GetLog()->GetLogging() && RProperties)
  {
    LogEvent(FORMAT("Changing properties of \"%s\" (%s)",
      LocalFileName, BooleanToEngStr(RProperties->Recursive)));
    if (RProperties->Valid.Contains(vpRights))
    {
      LogEvent(FORMAT(" - mode: \"%s\"", RProperties->Rights.GetModeStr()));
    }
    if (RProperties->Valid.Contains(vpGroup))
    {
      LogEvent(FORMAT(" - group: %s", RProperties->Group.GetLogText()));
    }
    if (RProperties->Valid.Contains(vpOwner))
    {
      LogEvent(FORMAT(" - owner: %s", RProperties->Owner.GetLogText()));
    }
    if (RProperties->Valid.Contains(vpModification))
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = ::UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT("%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
#if 0
      LogEvent(FORMAT(" - modification: \"%s\"",
        (FormatDateTime(L"dddddd tt",
           UnixToDateTime(RProperties->Modification, SessionData->DSTMode)))));
#endif // #if 0
      LogEvent(FORMAT(" - modification: \"%s\"", dt));
    }
    if (RProperties->Valid.Contains(vpLastAccess))
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = ::UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT("%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
#if 0
      LogEvent(FORMAT(" - last access: \"%s\"",
        (FormatDateTime(L"dddddd tt",
           UnixToDateTime(RProperties->LastAccess, SessionData->DSTMode)))));
#endif // #if 0
      LogEvent(FORMAT(" - last access: \"%s\"", dt));
    }
  }
  FileModified(AFile, LocalFileName);
  DoChangeFileProperties(LocalFileName, AFile, RProperties);
  ReactOnCommand(fsChangeProperties);
}
//---------------------------------------------------------------------------
void TTerminal::DoChangeFileProperties(const UnicodeString AFileName,
  const TRemoteFile *AFile, const TRemoteProperties *Properties)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TChmodSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->ChangeFileProperties(AFileName, AFile, Properties, Action);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CHANGE_PROPERTIES_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::ChangeFilesProperties(TStrings *AFileList,
  const TRemoteProperties *Properties)
{
  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  AnnounceFileListOperation();
  ProcessFiles(AFileList, foSetProperties, nb::bind(&TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}
//---------------------------------------------------------------------------
bool TTerminal::LoadFilesProperties(TStrings *AFileList)
{
  // see comment in TSFTPFileSystem::IsCapable
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(AFileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (AFileList->GetCount() > 0) &&
      (AFileList->GetAs<TRemoteFile>(0)->GetDirectory() == FFiles))
  {
    AddCachedFileList(FFiles);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DoCalculateFileSize(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *AParam)
{
  // This is called for top-level entries only
  TCalculateSizeParams *AParams = get_as<TCalculateSizeParams>(AParam);
  Expects(AParams && AParams->Stats);

  if (AParams->Stats->FoundFiles != nullptr)
  {
    UnicodeString DirPath = base::UnixExtractFilePath(base::UnixExcludeTrailingBackslash(AFile->GetFullFileName()));
    if ((DirPath != AParams->LastDirPath) ||
        (AParams->Files == nullptr))
    {
      AParams->Files = new TCollectedFileList();
      AParams->LastDirPath = DirPath;
      AParams->Stats->FoundFiles->AddObject(AParams->LastDirPath, AParams->Files);
    }
  }

  int64_t PrevSize = AParams->Size;

  if (AParams->Stats->CalculatedSizes != nullptr)
  {
    int64_t Size = AParams->Size - PrevSize;
    AParams->Stats->CalculatedSizes->push_back(Size);
  }
}
//---------------------------------------------------------------------------
void TTerminal::CalculateFileSize(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *AParam)
{
  DebugAssert(AParam);
  DebugAssert(AFile);
  UnicodeString FileName = AFileName;
  TCalculateSizeParams *Params = get_as<TCalculateSizeParams>(AParam);
  Expects(Params != nullptr);
  if (FileName.IsEmpty())
  {
    FileName = AFile->GetFileName();
  }

  if (!TryStartOperationWithFile(FileName, foCalculateSize))
  {
    Params->Result = false;
    // Combined with csIgnoreErrors, this will not abort completelly, but we get called again
    // with next sibling of our parent directory (and we get again to this branch, throwing again)
    Abort();
  }

  if ((Params->CopyParam == nullptr) ||
      DoAllowRemoteFileTransfer(AFile, Params->CopyParam, FLAGSET(Params->Params, csDisallowTemporaryTransferFiles)))
  {
    intptr_t CollectionIndex = -1;
    if (Params->Files != nullptr)
    {
      UnicodeString FullFileName = base::UnixExcludeTrailingBackslash(AFile->GetFullFileName());
      CollectionIndex = Params->Files->Add(FullFileName, AFile->Duplicate(), AFile->GetIsDirectory());
    }

    if (AFile->GetIsDirectory())
    {
      if (CanRecurseToDirectory(AFile))
      {
        if (!Params->AllowDirs)
        {
          Params->Result = false;
        }
        else if (FLAGSET(Params->Params, csStopOnFirstFile) && (Params->Stats->Files > 0))
        {
          // do not waste time recursing into a folder, if we already found some files
        }
        else
        {
          if (FLAGCLEAR(Params->Params, csStopOnFirstFile))
          {
            LogEvent(FORMAT(L"Getting size of directory \"%s\"", FileName));
          }

          // pass in full path so we get it back in file list for AllowTransfer() exclusion
          if (!DoCalculateDirectorySize(AFile->FullFileName, Params))
          {
            if (CollectionIndex >= 0)
            {
              Params->Files->DidNotRecurse(CollectionIndex);
            }
          }
        }
      }

      if (Params->Stats != nullptr)
      {
        Params->Stats->Directories++;
      }
    }
    else
    {
      Params->Size += AFile->GetSize();

      if (Params->Stats != nullptr)
      {
        Params->Stats->Files++;
      }
    }

    if ((Params->Stats != nullptr) && AFile->GetIsSymLink())
    {
      Params->Stats->SymLinks++;
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DoCalculateDirectorySize(const UnicodeString FileName, TCalculateSizeParams * Params)
{
  bool Result = false;
  if (FLAGSET(Params->Params, csStopOnFirstFile) && (Configuration->ActualLogProtocol >= 1))
  {
    LogEvent(FORMAT(L"Checking if remote directory \"%s\" is empty", FileName));
  }

  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      ProcessDirectory(FileName, nb::bind(&TTerminal::CalculateFileSize, this), Params);
      Result = true;
    }
    catch (Exception &E)
    {
      // We can probably replace the csIgnoreErrors with IgnoreErrors argument of the ProcessDirectory
      if (!GetActive() || ((Params->Params & csIgnoreErrors) == 0))
      {
        RetryLoop.Error(E, FMTLOAD(CALCULATE_SIZE_ERROR, FileName));
      }
    }
  }
  while (RetryLoop.Retry());

  if (Configuration->ActualLogProtocol >= 1)
  {
    if (Params->Stats->Files == 0)
    {
      LogEvent(FORMAT(L"Remote directory \"%s\" is empty", FileName));
    }
    else
    {
      LogEvent(FORMAT(L"Remote directory \"%s\" is not empty", FileName));
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CalculateFilesSize(TStrings *AFileList,
  int64_t &Size, intptr_t Params, const TCopyParamType *CopyParam,
  bool AllowDirs, TCalculateSizeStats &Stats)
{
  // With FTP protocol, we may use DSIZ command from
  // draft-peterson-streamlined-ftp-command-extensions-10
  // Implemented by Serv-U FTP.

  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TCalculateSizeParams Param;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = &Stats;
  Param.AllowDirs = AllowDirs;
  ProcessFiles(AFileList, foCalculateSize, nb::bind(&TTerminal::DoCalculateFileSize, this), &Param);
  Size = Param.Size;
  return Param.Result;
}
//---------------------------------------------------------------------------
void TTerminal::CalculateFilesChecksum(const UnicodeString Alg,
  TStrings *AFileList, TStrings *Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, AFileList, Checksums, OnCalculatedChecksum);
}
//---------------------------------------------------------------------------
void TTerminal::TerminalRenameFile(const TRemoteFile *AFile,
  const UnicodeString ANewName, bool CheckExistence)
{
  DebugAssert(AFile && AFile->GetDirectory() == FFiles);
  bool Proceed = true;
  // if filename doesn't contain path, we check for existence of file
  if ((AFile && AFile->GetFileName() != ANewName) && CheckExistence &&
      FConfiguration->GetConfirmOverwriting() &&
      base::UnixSamePath(RemoteGetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile *DuplicateFile = FFiles->FindFile(ANewName);
    if (DuplicateFile)
    {
      UnicodeString QuestionFmt;
      if (DuplicateFile->GetIsDirectory())
      {
        QuestionFmt = LoadStr(DIRECTORY_OVERWRITE);
      }
      else
      {
        QuestionFmt = LoadStr(PROMPT_FILE_OVERWRITE);
      }
      TQueryParams Params(qpNeverAskAgainCheck);
      UnicodeString Question = MainInstructions(FORMAT(QuestionFmt, ANewName));
      intptr_t Result = QueryUser(Question, nullptr,
        qaYes | qaNo, &Params);
      if (Result == qaNeverAskAgain)
      {
        Proceed = true;
        FConfiguration->SetConfirmOverwriting(false);
      }
      else
      {
        Proceed = (Result == qaYes);
      }
    }
  }

  if (Proceed && AFile)
  {
    FileModified(AFile, AFile->GetFileName());
    LogEvent(FORMAT("Renaming file \"%s\" to \"%s\".", AFile->GetFileName(), ANewName));
    DoRenameFile(AFile->GetFileName(), AFile, ANewName, false);
    ReactOnCommand(fsRenameFile);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
  const UnicodeString ANewName, bool Move)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TMvSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true), GetAbsolutePath(ANewName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteRenameFile(AFileName, AFile, ANewName);
    }
    catch (Exception &E)
    {
      UnicodeString Message = FMTLOAD(Move ? MOVE_FILE_ERROR : RENAME_FILE_ERROR, AFileName, ANewName);
      RetryLoop.Error(E, Action, Message);
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::TerminalMoveFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*const TMoveFileParams*/ void *Param)
{
  StartOperationWithFile(AFileName, foRemoteMove, foDelete);
  DebugAssert(Param != nullptr);
  const TMoveFileParams &Params = *get_as<TMoveFileParams>(Param);
  UnicodeString NewName = base::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT("Moving file \"%s\" to \"%s\".", AFileName, NewName));
  FileModified(AFile, AFileName);
  DoRenameFile(AFileName, AFile, NewName, true);
  ReactOnCommand(fsMoveFile);
}
//---------------------------------------------------------------------------
bool TTerminal::TerminalMoveFiles(TStrings *AFileList, const UnicodeString ATarget,
  const UnicodeString AFileMask)
{
  TMoveFileParams Params;
  Params.Target = ATarget;
  Params.FileMask = AFileMask;
  DirectoryModified(ATarget, true);
  bool Result;
  BeginTransaction();
  try__finally
  {
    // ON_SCOPE_EXIT(TTerminal::AfterMoveFiles, TStrings *, AFileList);
    Result = ProcessFiles(AFileList, foRemoteMove, nb::bind(&TTerminal::TerminalMoveFile, this), &Params);
  },
  __finally
  {
   if (GetActive())
   {
     __removed UnicodeString WithTrailing = base::UnixIncludeTrailingBackslash(this->GetCurrDirectory());
     bool PossiblyMoved = false;
     // check if we was moving current directory.
     // this is just optimization to avoid checking existence of current
     // directory after each move operation.
     UnicodeString CurrentDirectory = this->RemoteGetCurrentDirectory();
     for (intptr_t Index = 0; !PossiblyMoved && (Index < AFileList->GetCount()); ++Index)
     {
       const TRemoteFile *File =
         AFileList->GetAs<TRemoteFile>(Index);
       // File can be nullptr, and filename may not be full path,
       // but currently this is the only way we can move (at least in GUI)
       // current directory
       UnicodeString Str = AFileList->GetString(Index);
       if ((File != nullptr) &&
           File->GetIsDirectory() &&
           ((CurrentDirectory.SubString(1, Str.Length()) == Str) &&
            ((Str.Length() == CurrentDirectory.Length()) ||
             (CurrentDirectory[Str.Length() + 1] == L'/'))))
       {
         PossiblyMoved = true;
       }
     }

     if (PossiblyMoved && !this->FileExists(CurrentDirectory))
     {
       UnicodeString NearestExisting = CurrentDirectory;
       do
       {
         NearestExisting = base::UnixExtractFileDir(NearestExisting);
       }
       while (!base::IsUnixRootPath(NearestExisting) && !this->FileExists(NearestExisting));

       RemoteChangeDirectory(NearestExisting);
     }
   }
   EndTransaction();
  } end_try__finally
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::AfterMoveFiles(TStrings *AFileList)
{
  if (GetActive())
  {
    __removed UnicodeString WithTrailing = base::UnixIncludeTrailingBackslash(this->GetCurrDirectory());
    bool PossiblyMoved = false;
    // check if we was moving current directory.
    // this is just optimization to avoid checking existence of current
    // directory after each move operation.
    UnicodeString CurrentDirectory = this->RemoteGetCurrentDirectory();
    for (intptr_t Index = 0; !PossiblyMoved && (Index < AFileList->GetCount()); ++Index)
    {
      const TRemoteFile *File =
        AFileList->GetAs<TRemoteFile>(Index);
      // File can be nullptr, and filename may not be full path,
      // but currently this is the only way we can move (at least in GUI)
      // current directory
      UnicodeString Str = AFileList->GetString(Index);
      if ((File != nullptr) &&
          File->GetIsDirectory() &&
          ((CurrentDirectory.SubString(1, Str.Length()) == Str) &&
           ((Str.Length() == CurrentDirectory.Length()) ||
            (CurrentDirectory[Str.Length() + 1] == L'/'))))
      {
        PossiblyMoved = true;
      }
    }

    if (PossiblyMoved && !this->FileExists(CurrentDirectory))
    {
      UnicodeString NearestExisting = CurrentDirectory;
      do
      {
        NearestExisting = base::UnixExtractFileDir(NearestExisting);
      }
      while (!base::IsUnixRootPath(NearestExisting) && !this->FileExists(NearestExisting));

      RemoteChangeDirectory(NearestExisting);
    }
  }
  EndTransaction();
}
//---------------------------------------------------------------------------
void TTerminal::DoCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
  const UnicodeString ANewName)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    volatile TCpSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true), GetAbsolutePath(ANewName, true));
    try
    {
      DebugAssert(FFileSystem);
      if (GetIsCapable(fcRemoteCopy))
      {
        FFileSystem->RemoteCopyFile(AFileName, AFile, ANewName);
      }
      else
      {
        DebugAssert(GetCommandSessionOpened());
        DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
        LogEvent("Copying file on command session.");
        FCommandSession->TerminalSetCurrentDirectory(RemoteGetCurrentDirectory());
        FCommandSession->FFileSystem->RemoteCopyFile(AFileName, AFile, ANewName);
      }
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, FMTLOAD(COPY_FILE_ERROR, AFileName, ANewName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::TerminalCopyFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(AFileName, foRemoteCopy);
  DebugAssert(Param != nullptr);
  const TMoveFileParams &Params = *get_as<TMoveFileParams>(Param);
  UnicodeString NewName = base::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT("Copying file \"%s\" to \"%s\".", AFileName, NewName));
  DoCopyFile(AFileName, AFile, NewName);
  ReactOnCommand(fsCopyFile);
}
//---------------------------------------------------------------------------
bool TTerminal::TerminalCopyFiles(TStrings *AFileList, const UnicodeString ATarget,
  const UnicodeString AFileMask)
{
  TMoveFileParams Params;
  Params.Target = ATarget;
  Params.FileMask = AFileMask;
  DirectoryModified(ATarget, true);
  return ProcessFiles(AFileList, foRemoteCopy, nb::bind(&TTerminal::TerminalCopyFile, this), &Params);
}
//---------------------------------------------------------------------------
void TTerminal::RemoteCreateDirectory(const UnicodeString ADirName, const TRemoteProperties * Properties)
{
  DebugAssert(FFileSystem);
  DebugAssert(Properties != nullptr);
  FileModified(nullptr, ADirName);

  LogEvent(FORMAT("Creating directory \"%s\".", ADirName));
  bool Encrypt = Properties->Valid.Contains(vpEncrypt) && Properties->Encrypt;
  DoCreateDirectory(ADirName, Encrypt);

  TValidProperties RemainingPropeties = Properties->Valid;
  RemainingPropeties >> vpEncrypt;
  if (!RemainingPropeties.Empty())
  {
    DoChangeFileProperties(ADirName, nullptr, Properties);
  }

  ReactOnCommand(fsCreateDirectory);
}
//---------------------------------------------------------------------------
void TTerminal::DoCreateDirectory(const UnicodeString ADirName, bool Encrypt)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TMkdirSessionAction Action(GetActionLog(), GetAbsolutePath(ADirName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteCreateDirectory(ADirName, Encrypt);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CREATE_DIR_ERROR, ADirName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::RemoteCreateLink(UnicodeString AFileName,
  const UnicodeString APointTo, bool Symbolic)
{
  DebugAssert(FFileSystem);
  EnsureNonExistence(AFileName);
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(RemoteGetCurrentDirectory(), false);
  }

  LogEvent(FORMAT("Creating link \"%s\" to \"%s\" (symbolic: %s).",
      AFileName, APointTo, BooleanToEngStr(Symbolic)));
  DoCreateLink(AFileName, APointTo, Symbolic);
  ReactOnCommand(fsCreateDirectory);
}
//---------------------------------------------------------------------------
void TTerminal::DoCreateLink(const UnicodeString AFileName,
  const UnicodeString APointTo, bool Symbolic)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteCreateLink(AFileName, APointTo, Symbolic);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, FMTLOAD(CREATE_LINK_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::HomeDirectory()
{
  DebugAssert(FFileSystem);
  try
  {
    LogEvent("Changing directory to home directory.");
    FFileSystem->HomeDirectory();
    ReactOnCommand(fsHomeDirectory);
  }
  catch (Exception &E)
  {
    CommandError(&E, LoadStr(CHANGE_HOMEDIR_ERROR));
  }
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetHomeDirectory()
{
  return FFileSystem->GetHomeDirectory();
}
//---------------------------------------------------------------------------
void TTerminal::RemoteChangeDirectory(const UnicodeString ADirectory)
{
  UnicodeString DirectoryNormalized = base::ToUnixPath(ADirectory);
  DebugAssert(FFileSystem);
  try
  {
    UnicodeString CachedDirectory;
    DebugAssert(!GetSessionData()->GetCacheDirectoryChanges() || (FDirectoryChangesCache != nullptr));
    // never use directory change cache during startup, this ensures, we never
    // end-up initially in non-existing directory
    if ((GetStatus() == ssOpened) &&
        GetSessionData()->GetCacheDirectoryChanges() &&
        FDirectoryChangesCache->GetDirectoryChange(PeekCurrentDirectory(),
          DirectoryNormalized, CachedDirectory))
    {
      LogEvent(FORMAT("Cached directory change via \"%s\" to \"%s\".",
        DirectoryNormalized, CachedDirectory));
      FFileSystem->CachedChangeDirectory(CachedDirectory);
    }
    else
    {
      LogEvent(FORMAT("Changing directory to \"%s\".", DirectoryNormalized));
      FFileSystem->ChangeDirectory(DirectoryNormalized);
    }
    FLastDirectoryChange = DirectoryNormalized;
    ReactOnCommand(fsChangeDirectory);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(CHANGE_DIR_ERROR, DirectoryNormalized));
  }
}
//---------------------------------------------------------------------------
void TTerminal::LookupUsersGroups()
{
  if (!FUsersGroupsLookedup && (GetSessionData()->GetLookupUserGroups() != asOff) &&
    GetIsCapable(fcUserGroupListing))
  {
    DebugAssert(FFileSystem);

    try
    {
      FUsersGroupsLookedup = true;
      LogEvent("Looking up groups and users.");
      FFileSystem->LookupUsersGroups();
      ReactOnCommand(fsLookupUsersGroups);

      if (GetLog()->GetLogging())
      {
        FGroups.Log(this, L"groups");
        FMembership.Log(this, L"membership");
        FUsers.Log(this, L"users");
      }
    }
    catch (Exception &E)
    {
      if (!GetActive() || (GetSessionData()->GetLookupUserGroups() == asOn))
      {
        CommandError(&E, LoadStr(LOOKUP_GROUPS_ERROR));
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::AllowedAnyCommand(const UnicodeString Command) const
{
  return !Command.Trim().IsEmpty();
}
//---------------------------------------------------------------------------
bool TTerminal::GetCommandSessionOpened() const
{
  // consider secondary terminal open in "ready" state only
  // so we never do keepalives on it until it is completely initialized
  return (FCommandSession != nullptr) &&
    (FCommandSession->GetStatus() == ssOpened);
}
//---------------------------------------------------------------------------
TTerminal * TTerminal::CreateSecondarySession(const UnicodeString Name, TSessionData *ASessionData)
{
  std::unique_ptr<TSecondaryTerminal> Result(std::make_unique<TSecondaryTerminal>(this));
  Result->Init(ASessionData, FConfiguration, Name);

  Result->SetAutoReadDirectory(false);

  Result->SetExceptionOnFail(FExceptionOnFail > 0);

  Result->SetOnQueryUser(GetOnQueryUser());
  Result->SetOnPromptUser(GetOnPromptUser());
  Result->SetOnShowExtendedException(GetOnShowExtendedException());
  Result->SetOnProgress(GetOnProgress());
  Result->SetOnFinished(GetOnFinished());
  Result->SetOnInformation(GetOnInformation());
  Result->SetOnCustomCommand(GetOnCustomCommand());
  // do not copy OnDisplayBanner to avoid it being displayed
  return Result.release();
}
//---------------------------------------------------------------------------
void TTerminal::FillSessionDataForCode(TSessionData *Data) const
{
  const TSessionInfo &SessionInfo = GetSessionInfo();
  if (!SessionInfo.HostKeyFingerprintSHA256.IsEmpty())
  {
    Data->SetHostKey(SessionInfo.HostKeyFingerprintSHA256);
  }
  else if (SessionInfo.CertificateVerifiedManually && DebugAlwaysTrue(!SessionInfo.CertificateFingerprint.IsEmpty()))
  {
    Data->SetHostKey(SessionInfo.CertificateFingerprint);
  }
}
//---------------------------------------------------------------------------
void TTerminal::UpdateSessionCredentials(TSessionData * Data)
{
  Data->UserName = UserName;
  Data->Password = Password;
  if (!FRememberedPassword.IsEmpty() && (FRememberedPasswordKind == pkPassphrase))
  {
    Data->Passphrase = GetRememberedPassword();
  }
}
//---------------------------------------------------------------------------
TTerminal * TTerminal::GetCommandSession()
{
  if ((FCommandSession != nullptr) && !FCommandSession->GetActive())
  {
    SAFE_DESTROY(FCommandSession);
  }

  if (FCommandSession == nullptr)
  {
    // transaction cannot be started yet to allow proper matching transaction
    // levels between main and command session
    DebugAssert(FInTransaction == 0);

    std::unique_ptr<TSessionData> CommandSessionData(FSessionData->Clone());
    CommandSessionData->SetRemoteDirectory(RemoteGetCurrentDirectory());
    CommandSessionData->SetFSProtocol(fsSCPonly);
    CommandSessionData->SetClearAliases(false);
    CommandSessionData->SetUnsetNationalVars(false);
    CommandSessionData->SetLookupUserGroups(asOn);

    std::unique_ptr<TTerminal> CommandSession(CreateSecondarySession(L"Shell", CommandSessionData.get()));

    CommandSession->SetAutoReadDirectory(false);

    CommandSession->SetExceptionOnFail(FExceptionOnFail > 0);

    CommandSession->SetOnQueryUser(GetOnQueryUser());
    CommandSession->SetOnPromptUser(GetOnPromptUser());
    CommandSession->SetOnShowExtendedException(GetOnShowExtendedException());
    CommandSession->SetOnProgress(GetOnProgress());
    CommandSession->SetOnFinished(GetOnFinished());
    CommandSession->SetOnInformation(GetOnInformation());
    CommandSession->SetOnCustomCommand(GetOnCustomCommand());
    // do not copy OnDisplayBanner to avoid it being displayed
    FCommandSession = CommandSession.release();
  }

  return FCommandSession;
}
//---------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated

class TOutputProxy : public TObject
{
public:
  TOutputProxy(TCallSessionAction &Action, TCaptureOutputEvent OutputEvent) :
    FAction(Action),
    FOutputEvent(OutputEvent)
  {
  }

  void Output(const UnicodeString Str, TCaptureOutputType OutputType)
  {
    switch (OutputType)
    {
    case cotOutput:
      FAction.AddOutput(Str, false);
      break;
    case cotError:
      FAction.AddOutput(Str, true);
      break;
    case cotExitCode:
      FAction.ExitCode(ToInt(::StrToInt64(Str)));
      break;
    }

    if (FOutputEvent != nullptr)
    {
      FOutputEvent(Str, OutputType);
    }
  }

private:
  TCallSessionAction &FAction;
  TCaptureOutputEvent FOutputEvent;
};

#pragma warning(pop)
//---------------------------------------------------------------------------
void TTerminal::AnyCommand(const UnicodeString Command,
  TCaptureOutputEvent OutputEvent)
{
#if 0
#pragma warn -inl
  class TOutputProxy
  {
  public:
    TOutputProxy(TCallSessionAction &Action, TCaptureOutputEvent OutputEvent) :
      FAction(Action),
      FOutputEvent(OutputEvent)
    {
    }

    void Output(const UnicodeString AStr, TCaptureOutputType OutputType)
    {
      switch (OutputType)
      {
        case cotOutput:
          FAction.AddOutput(Str, false);
          break;
        case cotError:
          FAction.AddOutput(Str, true);
          break;
        case cotExitCode:
          FAction.ExitCode(StrToIntPtr(Str));
          break;
      }

      if (FOutputEvent != nullptr)
      {
        FOutputEvent(Str, OutputType);
      }
    }

  private:
    TCallSessionAction &FAction;
    TCaptureOutputEvent FOutputEvent;
  };
#pragma warn .inl
#endif // #if 0

  TCallSessionAction Action(GetActionLog(), Command, RemoteGetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, nb::bind(&TOutputProxy::Output, &ProxyOutputEvent), &Action);
}
//---------------------------------------------------------------------------
void TTerminal::DoAnyCommand(const UnicodeString ACommand,
  TCaptureOutputEvent OutputEvent, TCallSessionAction *Action)
{
  DebugAssert(FFileSystem);
  try
  {
    DirectoryModified(RemoteGetCurrentDirectory(), false);
    if (GetIsCapable(fcAnyCommand))
    {
      LogEvent("Executing user defined command.");
      FFileSystem->AnyCommand(ACommand, OutputEvent);
    }
    else
    {
      DebugAssert(GetCommandSessionOpened());
      DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent("Executing user defined command on command session.");

      FCommandSession->TerminalSetCurrentDirectory(RemoteGetCurrentDirectory());
      FCommandSession->FFileSystem->AnyCommand(ACommand, OutputEvent);

      FCommandSession->GetFileSystem()->ReadCurrentDirectory();

      // synchronize pwd (by purpose we lose transaction optimization here)
      RemoteChangeDirectory(FCommandSession->RemoteGetCurrentDirectory());
    }
    ReactOnCommand(fsAnyCommand);
  }
  catch (Exception &E)
  {
    if (Action != nullptr)
    {
      RollbackAction(*Action, nullptr, &E);
    }
    if (GetExceptionOnFail() || isa<EFatal>(&E))
    {
      throw;
    }
    HandleExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DoCreateLocalFile(const UnicodeString AFileName,
  TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
  bool NoConfirmation)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  bool Done;
  DWORD DesiredAccess = GENERIC_WRITE;
  DWORD ShareMode = FILE_SHARE_READ;
  DWORD CreationDisposition = CREATE_ALWAYS; // Resume ? OPEN_ALWAYS : CREATE_ALWAYS;
  DWORD FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
  do
  {
    *AHandle = this->TerminalCreateLocalFile(ApiPath(AFileName), DesiredAccess, ShareMode,
        CreationDisposition, FlagsAndAttributes);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      // save the error, otherwise it gets overwritten by call to FileExists
      int LastError = ::GetLastError();
      DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
      if (::SysUtulsFileExists(ApiPath(AFileName)) &&
        (((LocalFileAttrs = GetLocalFileAttributes(ApiPath(AFileName))) & (faReadOnly | faHidden)) != 0))
      {
        if (FLAGSET(LocalFileAttrs, faReadOnly))
        {
          OperationProgress->LockUserSelections();
          try__finally
          {
            if (OperationProgress->GetBatchOverwrite() == boNone)
            {
              Result = false;
            }
            else if ((OperationProgress->GetBatchOverwrite() != boAll) && !NoConfirmation)
            {
              uint32_t Answer;
              {
                volatile TSuspendFileOperationProgress Suspend(OperationProgress);
                Answer = QueryUser(
                  MainInstructions(FMTLOAD(READ_ONLY_OVERWRITE, AFileName)), nullptr,
                  qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll, nullptr);
              }

              switch (Answer)
              {
              case qaYesToAll:
                OperationProgress->SetBatchOverwrite(boAll);
                break;
              case qaCancel:
                OperationProgress->SetCancel(csCancel); // continue on next case
                Result = false;
                break;
              case qaNoToAll:
                OperationProgress->SetBatchOverwrite(boNone);
                Result = false;
                break;
              case qaNo:
                Result = false;
                break;
              default:
                Result = false;
                break;
              }
            }
          },
          __finally
          {
            OperationProgress->UnlockUserSelections();
          } end_try__finally
        }
        else
        {
          DebugAssert(FLAGSET(LocalFileAttrs, faHidden));
          Result = true;
        }

        if (Result)
        {
          FlagsAndAttributes |=
            FLAGMASK(FLAGSET(LocalFileAttrs, faHidden), FILE_ATTRIBUTE_HIDDEN) |
            FLAGMASK(FLAGSET(LocalFileAttrs, faReadOnly), FILE_ATTRIBUTE_READONLY);

          FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
            FMTLOAD(CANT_SET_ATTRS, AFileName), "",
          [&]()
          {
            if (!this->SetLocalFileAttributes(ApiPath(AFileName), LocalFileAttrs & ~(faReadOnly | faHidden)))
            {
              ::RaiseLastOSError();
            }
          });
          __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (FileName)));
        }
        else
        {
          Done = true;
        }
      }
      else
      {
        ::RaiseLastOSError(LastError);
      }
    }
  }
  while (!Done);

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::TerminalCreateLocalFile(const UnicodeString ATargetFileName,
  TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
  bool NoConfirmation)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(CREATE_FILE_ERROR, ATargetFileName), "",
  [&]()
  {
    Result = DoCreateLocalFile(ATargetFileName, OperationProgress, AHandle, NoConfirmation);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_FILE_ERROR, (FileName)));

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::TerminalOpenLocalFile(const UnicodeString ATargetFileName,
  DWORD Access, DWORD *AAttrs, HANDLE *AHandle, int64_t *ACTime,
  int64_t *AMTime, int64_t *AATime, int64_t *ASize,
  bool TryWriteReadOnly)
{
  DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  TFileOperationProgressType *OperationProgress = GetOperationProgress();

  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(FILE_NOT_EXISTS, ATargetFileName), "",
  [&]()
  {
    UnicodeString FileNameApi = ApiPath(ATargetFileName);
    LocalFileAttrs = this->GetLocalFileAttributes(FileNameApi);
    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      ::RaiseLastOSError();
    }
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(FILE_NOT_EXISTS, (FileName)));

  if (FLAGCLEAR(LocalFileAttrs, faDirectory) || (AHandle == INVALID_HANDLE_VALUE))
  {
    bool NoHandle = false;
    if (!TryWriteReadOnly && (Access == GENERIC_WRITE) &&
        ((LocalFileAttrs & faReadOnly) != 0))
    {
      Access = GENERIC_READ;
      NoHandle = true;
    }

    FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
      FMTLOAD(OPENFILE_ERROR, ATargetFileName), "",
    [&]()
    {
      DWORD Flags = FLAGMASK(FLAGSET(LocalFileAttrs, faDirectory), FILE_FLAG_BACKUP_SEMANTICS);
      LocalFileHandle = this->TerminalCreateLocalFile(ApiPath(ATargetFileName), Access,
        Access == GENERIC_READ ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ,
        OPEN_EXISTING, Flags);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        ::RaiseLastOSError();
      }
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(OPENFILE_ERROR, (FileName)));

    try
    {
      if (AATime || AMTime || ACTime)
      {
        FILETIME ATime;
        FILETIME MTime;
        FILETIME CTime;

        // Get last file access and modification time
        FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
          FMTLOAD(CANT_GET_ATTRS, ATargetFileName), "",
        [&]()
        {
          THROWOSIFFALSE(::GetFileTime(LocalFileHandle, &CTime, &ATime, &MTime));
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_GET_ATTRS, (FileName)));

        if (ACTime)
        {
          *ACTime = ::ConvertTimestampToUnixSafe(CTime, GetSessionData()->GetDSTMode());
        }
        if (AATime)
        {
          *AATime = ::ConvertTimestampToUnixSafe(ATime, GetSessionData()->GetDSTMode());
        }
        if (AMTime)
        {
          *AMTime = ::ConvertTimestampToUnix(MTime, GetSessionData()->GetDSTMode());
        }
      }

      if (ASize)
      {
        // Get file size
        FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
          FMTLOAD(CANT_GET_ATTRS, ATargetFileName), "",
        [&]()
        {
          uint32_t LSize;
          DWORD HSize;
          LSize = ::GetFileSize(LocalFileHandle, &HSize);
          if ((LSize == ToUInt32(-1)) && (::GetLastError() != NO_ERROR))
          {
            ::RaiseLastOSError();
          }
          *ASize = (ToInt64(HSize) << 32) + LSize;
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_GET_ATTRS, (FileName)));
      }

      if ((AHandle == nullptr) || NoHandle)
      {
        SAFE_CLOSE_HANDLE(LocalFileHandle);
        LocalFileHandle = INVALID_HANDLE_VALUE;
      }
    }
    catch (...)
    {
      SAFE_CLOSE_HANDLE(LocalFileHandle);
      throw;
    }
  }

  if (AAttrs)
  {
    *AAttrs = LocalFileAttrs;
  }
  if (AHandle)
  {
    *AHandle = LocalFileHandle;
  }
}
//---------------------------------------------------------------------------
void TTerminal::TerminalOpenLocalFile(
  const UnicodeString AFileName, uintptr_t Access, TLocalFileHandle &AHandle, bool TryWriteReadOnly)
{
  AHandle.FileName = AFileName;
  this->TerminalOpenLocalFile(
    AFileName, ToDWord(Access), &AHandle.Attrs, &AHandle.Handle, nullptr, &AHandle.MTime, &AHandle.ATime, &AHandle.Size,
    TryWriteReadOnly);
  AHandle.Modification = ::UnixToDateTime(AHandle.MTime, GetSessionData()->GetDSTMode());
  AHandle.Directory = FLAGSET(AHandle.Attrs, faDirectory);
}
//---------------------------------------------------------------------------
bool TTerminal::DoAllowLocalFileTransfer(
  const UnicodeString FileName, const TSearchRecSmart & SearchRec, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles)
{
  TFileMasks::TParams Params;
  Params.Size = SearchRec.Size;
  Params.Modification = SearchRec.GetLastWriteTime();
  UnicodeString BaseFileName = GetBaseFileName(FileName);
  // Note that for synchronization, we do not need to check "TSynchronizeOptions::MatchesFilter" here as
  // that is checked for top-level entries only and we are never top-level here.
  return
    CopyParam->AllowTransfer(BaseFileName, osLocal, SearchRec.IsDirectory(), Params, SearchRec.IsHidden()) &&
    (!DisallowTemporaryTransferFiles || !FFileSystem->TemporaryTransferFile(FileName)) &&
    (!SearchRec.IsDirectory() || !CopyParam->ExcludeEmptyDirectories ||
       !IsEmptyLocalDirectory(FileName, CopyParam, DisallowTemporaryTransferFiles));
}
//---------------------------------------------------------------------------
bool TTerminal::DoAllowRemoteFileTransfer(
  const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles)
{
  TFileMasks::TParams MaskParams;
  MaskParams.Size = File->Size;
  MaskParams.Modification = File->Modification;
  UnicodeString FullRemoteFileName = base::UnixExcludeTrailingBackslash(File->FullFileName);
  UnicodeString BaseFileName = GetBaseFileName(FullRemoteFileName);
  return
    CopyParam->AllowTransfer(BaseFileName, osRemote, File->IsDirectory, MaskParams, File->IsHidden) &&
    (!DisallowTemporaryTransferFiles || !FFileSystem->TemporaryTransferFile(File->FileName)) &&
    (!File->IsDirectory || !CopyParam->ExcludeEmptyDirectories ||
       !IsEmptyRemoteDirectory(File, CopyParam, DisallowTemporaryTransferFiles));
}
//---------------------------------------------------------------------------
bool TTerminal::AllowLocalFileTransfer(
  const UnicodeString AFileName, const TSearchRecSmart * SearchRec,
  const TCopyParamType *CopyParam, TFileOperationProgressType *OperationProgress)
{
  bool Result = true;
  // optimization (though in most uses of the method, the caller actually knows TSearchRec already, so it passes it here
  if (GetLog()->GetLogging() || !CopyParam->AllowAnyTransfer())
  {
    TSearchRecSmart ASearchRec;
    if (SearchRec == nullptr)
    {
      FileOperationLoopCustom(this, OperationProgress, 0,
        FMTLOAD(FILE_NOT_EXISTS, AFileName), "",
      [&]()
      {
        if (!FileSearchRec(AFileName, ASearchRec))
        {
          RaiseLastOSError();
        }
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(FILE_NOT_EXISTS, (FileName)));
      SearchRec = &ASearchRec;
    }

    if (!DoAllowLocalFileTransfer(AFileName, *SearchRec, CopyParam, false))
    {
      LogEvent(FORMAT("File \"%s\" excluded from transfer", AFileName));
      Result = false;
    }
    else if (CopyParam->SkipTransfer(AFileName, SearchRec->IsDirectory()))
    {
      OperationProgress->AddSkippedFileSize(SearchRec->Size);
      Result = false;
    }

    if (Result)
    {
      LogFileDetails(AFileName, SearchRec->GetLastWriteTime(), SearchRec->Size);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::MakeLocalFileList(
  UnicodeString AFileName, const TSearchRecSmart & Rec, void * Param)
{
  TMakeLocalFileListParams &Params = *get_as<TMakeLocalFileListParams>(Param);
  Expects(Params.FileList != nullptr);

  if (Rec.IsDirectory() && Params.Recursive)
  {
    ProcessLocalDirectory(AFileName, nb::bind(&TTerminal::MakeLocalFileList, this), &Params);
  }

  if (!Rec.IsDirectory() || Params.IncludeDirs)
  {
    Params.FileList->Add(AFileName);
    if (Params.FileTimes != nullptr)
    {
      Params.FileTimes->push_back(const_cast<TSearchRecSmart &>(Rec).GetLastWriteTime());
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::CalculateLocalFileSize(
  UnicodeString AFileName, const TSearchRecSmart & Rec, /*TCalculateSizeParams*/ void * AParams)
{
  TCalculateSizeParams *Params = get_as<TCalculateSizeParams>(AParams);
  Expects(Params != nullptr);

  if (!TryStartOperationWithFile(AFileName, foCalculateSize))
  {
    Params->Result = false;
  }
  else
  {
    try
    {
      if ((Params->CopyParam == nullptr) ||
          DoAllowLocalFileTransfer(AFileName, Rec, Params->CopyParam, false))
      {
        intptr_t CollectionIndex = -1;
        if (Params->Files != nullptr)
        {
          UnicodeString FullFileName = ::ExpandUNCFileName(AFileName);
          CollectionIndex = Params->Files->Add(FullFileName, nullptr, Rec.IsDirectory());
        }

        if (!Rec.IsDirectory())
        {
          Params->Size += Rec.Size;
        }
        else
        {
          try
          {
            ProcessLocalDirectory(AFileName, nb::bind(&TTerminal::CalculateLocalFileSize, this), AParams);
          }
          catch (...)
          {
            if (CollectionIndex >= 0)
            {
              Params->Files->DidNotRecurse(CollectionIndex);
            }
          }
        }
      }
    }
    catch (...)
    {
      // ignore
      DEBUG_PRINTF("TTerminal::CalculateLocalFileSize: error occured");
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::CalculateLocalFilesSize(TStrings *AFileList,
  int64_t & Size, const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files,
  TCalculatedSizes * CalculatedSizes)
{
  bool Result;
  TFileOperationProgressType OperationProgress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationStart(OperationProgress, foCalculateSize, osLocal, AFileList->Count);
  try__finally
  {
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;
    Params.Files = nullptr;
    Params.Result = true;

    UnicodeString LastDirPath;
    for (intptr_t Index = 0; Params.Result && (Index < AFileList->GetCount()); ++Index)
    {
      UnicodeString FileName = AFileList->GetString(Index);
      TSearchRecSmart Rec;
      if (FileSearchRec(FileName, Rec))
      {
        if (Rec.IsDirectory() && !AllowDirs)
        {
          Params.Result = false;
        }
        else
        {
          if (Files != nullptr)
          {
            UnicodeString FullFileName = ::ExpandUNCFileName(FileName);
            UnicodeString DirPath = ::ExtractFilePath(FullFileName);
            if (DirPath != LastDirPath)
            {
              Params.Files = new TCollectedFileList();
              LastDirPath = DirPath;
              Files->AddObject(LastDirPath, Params.Files);
            }
          }

          int64_t PrevSize = Params.Size;

          CalculateLocalFileSize(FileName, Rec, &Params);

          if (CalculatedSizes != nullptr)
          {
            int64_t Size = Params.Size - PrevSize;
            CalculatedSizes->push_back(Size);
          }

          OperationFinish(&OperationProgress, AFileList->GetObj(Index), FileName, true, OnceDoneOperation);
        }
      }
    }

    Size = Params.Size;
    Result = Params.Result;
  },
  __finally
  {
    OperationStop(OperationProgress);
  } end_try__finally

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
  return Result;
}
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TSynchronizeFileData);
struct TSynchronizeFileData : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSynchronizeFileData); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSynchronizeFileData) || TObject::is(Kind); }
public:
  TSynchronizeFileData() :
    TObject(OBJECT_CLASS_TSynchronizeFileData),
    Modified(false),
    New(false),
    IsDirectory(false),
    MatchingRemoteFileFile(nullptr),
    MatchingRemoteFileImageIndex(0)
  {
    ClearStruct(LocalLastWriteTime);
  }

  bool Modified;
  bool New;
  bool IsDirectory;
  TChecklistItem::TFileInfo Info;
  TChecklistItem::TFileInfo MatchingRemoteFile;
  TRemoteFile *MatchingRemoteFileFile;
  intptr_t MatchingRemoteFileImageIndex;
  FILETIME LocalLastWriteTime;
};
//---------------------------------------------------------------------------
const intptr_t sfFirstLevel = 0x01;
NB_DEFINE_CLASS_ID(TSynchronizeData);
struct TSynchronizeData : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSynchronizeData); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSynchronizeData) || TObject::is(Kind); }
public:
  TSynchronizeData() : TObject(OBJECT_CLASS_TSynchronizeData)
  {
  }

  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  TTerminal::TSynchronizeMode Mode{};
  intptr_t Params{0};
  TSynchronizeDirectoryEvent OnSynchronizeDirectory{nullptr};
  TSynchronizeOptions *Options{nullptr};
  intptr_t Flags{0};
  TStringList *LocalFileList{nullptr};
  const TCopyParamType *CopyParam{nullptr};
  TSynchronizeChecklist *Checklist{nullptr};

  void DeleteLocalFileList()
  {
    if (LocalFileList != nullptr)
    {
      for (intptr_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
      {
        TSynchronizeFileData *FileData = LocalFileList->GetAs<TSynchronizeFileData>(Index);
        SAFE_DESTROY(FileData);
      }
      SAFE_DESTROY(LocalFileList);
    }
  }
};
//---------------------------------------------------------------------------
TSynchronizeChecklist * TTerminal::SynchronizeCollect(const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType *CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
  TSynchronizeOptions *Options)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor); nb::used(UseBusyCursorRestorer);
  FUseBusyCursor = false;

  std::unique_ptr<TSynchronizeChecklist> Checklist(new TSynchronizeChecklist());
  try__catch
  {
    DoSynchronizeCollectDirectory(LocalDirectory, RemoteDirectory, Mode,
      CopyParam, Params, OnSynchronizeDirectory, Options, sfFirstLevel,
      Checklist.get());
    Checklist->Sort();
  }
  catch__removed
  ({
    delete Checklist;
    throw;
  })
  return Checklist.release();
}
//---------------------------------------------------------------------------
static void AddFlagName(UnicodeString &ParamsStr, intptr_t &Params, intptr_t Param, const UnicodeString Name)
{
  if (FLAGSET(Params, Param))
  {
    AddToList(ParamsStr, Name, L", ");
  }
  Params &= ~Param;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::SynchronizeModeStr(TSynchronizeMode Mode)
{
  UnicodeString ModeStr;
  switch (Mode)
  {
  case smRemote:
    ModeStr = L"Remote";
    break;
  case smLocal:
    ModeStr = L"Local";
    break;
  case smBoth:
    ModeStr = L"Both";
    break;
  default:
    ModeStr = L"Unknown";
    break;
  }
  return ModeStr;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::SynchronizeParamsStr(intptr_t Params)
{
  UnicodeString ParamsStr;
  AddFlagName(ParamsStr, Params, spDelete, L"Delete");
  AddFlagName(ParamsStr, Params, spNoConfirmation, L"NoConfirmation");
  AddFlagName(ParamsStr, Params, spExistingOnly, L"ExistingOnly");
  AddFlagName(ParamsStr, Params, spNoRecurse, L"NoRecurse");
  AddFlagName(ParamsStr, Params, spUseCache, L"UseCache");
  AddFlagName(ParamsStr, Params, spDelayProgress, L"DelayProgress");
  AddFlagName(ParamsStr, Params, spPreviewChanges, L"*PreviewChanges"); // GUI only
  AddFlagName(ParamsStr, Params, spSubDirs, L"SubDirs");
  AddFlagName(ParamsStr, Params, spTimestamp, L"Timestamp");
  AddFlagName(ParamsStr, Params, spNotByTime, L"NotByTime");
  AddFlagName(ParamsStr, Params, spBySize, L"BySize");
  AddFlagName(ParamsStr, Params, spSelectedOnly, L"*SelectedOnly"); // GUI only
  AddFlagName(ParamsStr, Params, spMirror, L"Mirror");
  if (Params > 0)
  {
    AddToList(ParamsStr, FORMAT("0x%x", ToInt(Params)), L", ");
  }
  return ParamsStr;
}
//---------------------------------------------------------------------------
bool TTerminal::LocalFindFirstLoop(const UnicodeString APath, TSearchRecChecked & SearchRec)
{
  bool Result;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(LIST_DIR_ERROR, APath), "",
  [&]()
  {
    DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    Result = (FindFirstChecked(APath, FindAttrs, SearchRec) == 0);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, (Path)));
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::LocalFindNextLoop(TSearchRecChecked & SearchRec)
{
  bool Result;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(LIST_DIR_ERROR, SearchRec.Path), "",
  [&]()
  {
    Result = (FindNextChecked(SearchRec) == 0);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, (SearchRec.Path)));
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::IsEmptyLocalDirectory(
  const UnicodeString Path, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles)
{
  UnicodeString Contents;
  if (Configuration->ActualLogProtocol >= 1)
  {
    LogEvent(FORMAT(L"Checking if local directory \"%s\" is empty", Path));
  }

  TSearchRecOwned SearchRec;
  if (LocalFindFirstLoop(IncludeTrailingBackslash(Path) + L"*.*", SearchRec))
  {
    do
    {
      UnicodeString FullLocalFileName = IncludeTrailingBackslash(Path) + SearchRec.Name;
      if (SearchRec.IsRealFile() &&
          DoAllowLocalFileTransfer(FullLocalFileName, SearchRec, CopyParam, true))
      {
        if (!SearchRec.IsDirectory() ||
            !IsEmptyLocalDirectory(FullLocalFileName, CopyParam, DisallowTemporaryTransferFiles))
        {
          Contents = SearchRec.Name;
        }
      }
    }
    while (Contents.IsEmpty() && LocalFindNextLoop(SearchRec));
  }

  if (Configuration->ActualLogProtocol >= 1)
  {
    if (Contents.IsEmpty())
    {
      LogEvent(FORMAT(L"Local directory \"%s\" is empty", Path));
    }
    else
    {
      LogEvent(FORMAT(L"Local directory \"%s\" is not empty, it contains \"%s\"", Path, Contents));
    }
  }

  return Contents.IsEmpty();
}
//---------------------------------------------------------------------------
void TTerminal::DoSynchronizeCollectDirectory(const UnicodeString ALocalDirectory,
  const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t AParams,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options,
  intptr_t AFlags, TSynchronizeChecklist * Checklist)
{
  TSynchronizeData Data;

  Data.LocalDirectory = ::IncludeTrailingBackslash(ALocalDirectory);
  Data.RemoteDirectory = base::UnixIncludeTrailingBackslash(ARemoteDirectory);
  Data.Mode = Mode;
  Data.Params = AParams;
  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;
  Data.LocalFileList = nullptr;
  Data.CopyParam = CopyParam;
  Data.Options = Options;
  Data.Flags = AFlags;
  Data.Checklist = Checklist;

  LogEvent(FORMAT("Collecting synchronization list for local directory '%s' and remote directory '%s', "
    "mode = %s, params = 0x%x (%s), file mask = '%s'", ALocalDirectory, ARemoteDirectory,
    SynchronizeModeStr(Mode), int(AParams), SynchronizeParamsStr(AParams), CopyParam->GetIncludeFileMask().GetMasks()));

  if (FLAGCLEAR(AParams, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  try__finally
  {
    Data.LocalFileList = CreateSortedStringList();

    TSearchRecOwned SearchRec;
    if (LocalFindFirstLoop(Data.LocalDirectory + L"*.*", SearchRec))
    {
      do
      {
        UnicodeString FileName = SearchRec.Name;
        UnicodeString FullLocalFileName = Data.LocalDirectory + FileName;
        UnicodeString RemoteFileName = ChangeFileName(CopyParam, FileName, osLocal, false);
        if (SearchRec.IsRealFile() &&
            DoAllowLocalFileTransfer(FullLocalFileName, SearchRec, CopyParam, true) &&
            (FLAGCLEAR(AFlags, sfFirstLevel) ||
             (Options == nullptr) ||
             Options->MatchesFilter(FileName) ||
             Options->MatchesFilter(RemoteFileName)))
        {
          TSynchronizeFileData * FileData = new TSynchronizeFileData;

          FileData->IsDirectory = SearchRec.IsDirectory();
          FileData->Info.FileName = FileName;
          FileData->Info.Directory = Data.LocalDirectory;
          FileData->Info.Modification = SearchRec.GetLastWriteTime();
          FileData->Info.ModificationFmt = mfFull;
          FileData->Info.Size = SearchRec.Size;
          FileData->LocalLastWriteTime = SearchRec.FindData.ftLastWriteTime;
          FileData->New = true;
          FileData->Modified = false;
          Data.LocalFileList->AddObject(FileName, reinterpret_cast<TObject*>(FileData));
          LogEvent(0, FORMAT(L"Local file %s included to synchronization",
            FormatFileDetailsForLog(FullLocalFileName, SearchRec.GetLastWriteTime(), SearchRec.Size)));
        }
        else
        {
          LogEvent(0, FORMAT(L"Local file %s excluded from synchronization",
            FormatFileDetailsForLog(FullLocalFileName, SearchRec.GetLastWriteTime(), SearchRec.Size)));
        }

      }
      while (LocalFindNextLoop(SearchRec));

      SearchRec.Close();

      // can we expect that ProcessDirectory would take so little time
      // that we can postpone showing progress window until anything actually happens?
      bool Cached = FLAGSET(AParams, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(ARemoteDirectory);

      if (!Cached && FLAGSET(AParams, spDelayProgress))
      {
        DoSynchronizeProgress(Data, true);
      }

      ProcessDirectory(ARemoteDirectory, nb::bind(&TTerminal::SynchronizeCollectFile, this), &Data,
        FLAGSET(AParams, spUseCache));

      TSynchronizeFileData *FileData;
      for (intptr_t Index = 0; Index < Data.LocalFileList->GetCount(); ++Index)
      {
        FileData = Data.LocalFileList->GetAs<TSynchronizeFileData>(Index);
        // add local file either if we are going to upload it
        // (i.e. if it is updated or we want to upload even new files)
        // or if we are going to delete it (i.e. all "new"=obsolete files)
        bool Modified = (FileData->Modified && ((Mode == smBoth) || (Mode == smRemote)));
        bool New = (FileData->New &&
          ((Mode == smLocal) ||
           (((Mode == smBoth) || (Mode == smRemote)) && FLAGCLEAR(AParams, spTimestamp))));

        if (New)
        {
          LogEvent(FORMAT("Local file %s is new",
            FormatFileDetailsForLog(UnicodeString(FileData->Info.Directory) + UnicodeString(FileData->Info.FileName),
             FileData->Info.Modification, FileData->Info.Size)));
        }

        if (Modified || New)
        {
          std::unique_ptr<TChecklistItem> ChecklistItem(new TChecklistItem());
          try__finally
          {
            ChecklistItem->IsDirectory = FileData->IsDirectory;

            ChecklistItem->Local = FileData->Info;
            ChecklistItem->FLocalLastWriteTime = FileData->LocalLastWriteTime;

            if (Modified)
            {
              DebugAssert(!FileData->MatchingRemoteFile.Directory.IsEmpty());
              ChecklistItem->Remote = FileData->MatchingRemoteFile;
              ChecklistItem->ImageIndex = FileData->MatchingRemoteFileImageIndex;
              ChecklistItem->RemoteFile = FileData->MatchingRemoteFileFile;
            }
            else
            {
              ChecklistItem->Remote.Directory = Data.RemoteDirectory;
            }

            if ((Mode == smBoth) || (Mode == smRemote))
            {
              ChecklistItem->Action =
                (Modified ? TChecklistAction::saUploadUpdate : TChecklistAction::saUploadNew);
              ChecklistItem->Checked =
                (Modified || FLAGCLEAR(AParams, spExistingOnly)) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(AParams, spNoRecurse) ||
                 FLAGSET(AParams, spSubDirs));
            }
            else if ((Mode == smLocal) && FLAGCLEAR(AParams, spTimestamp))
            {
              ChecklistItem->Action = saDeleteLocal;
              ChecklistItem->Checked =
                FLAGSET(AParams, spDelete) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(AParams, spNoRecurse) ||
                 FLAGSET(AParams, spSubDirs));
            }

            if (ChecklistItem->Action != saNone)
            {
              Data.Checklist->Add(ChecklistItem.get());
              ChecklistItem.release();
            }
          },
          __finally__removed
          ({
            delete ChecklistItem;
          }) end_try__finally
        }
        else
        {
          if (FileData->Modified)
          {
            SAFE_DESTROY(FileData->MatchingRemoteFileFile);
          }
        }
      }
    }
  },
  __finally
  {
     Data.DeleteLocalFileList();
  } end_try__finally
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeCollectFile(UnicodeString AFileName,
  const TRemoteFile *AFile, /*TSynchronizeData*/ void *Param)
{
  try
  {
    DoSynchronizeCollectFile(AFileName, AFile, Param);
  }
  catch (ESkipFile &E)
  {
    volatile TSuspendFileOperationProgress Suspend(OperationProgress);
    nb::used(Suspend);
    if (!HandleException(&E))
    {
      throw;
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::IsEmptyRemoteDirectory(
  const TRemoteFile * File, const TCopyParamType * ACopyParam, bool DisallowTemporaryTransferFiles)
{
  TCalculateSizeStats Stats;

  TCopyParamType CopyParam(*ACopyParam);
  CopyParam.ExcludeEmptyDirectories = false; // to avoid endless recursion

  TCalculateSizeParams Params;
  Params.Params = csStopOnFirstFile | csIgnoreErrors | FLAGMASK(DisallowTemporaryTransferFiles, csDisallowTemporaryTransferFiles);
  Params.CopyParam = &CopyParam;
  Params.Stats = &Stats;

  DoCalculateDirectorySize(base::UnixExcludeTrailingBackslash(File->FullFileName), &Params);

  return Params.Result && (Stats.Files == 0);
}
//---------------------------------------------------------------------------
void TTerminal::DoSynchronizeCollectFile(UnicodeString AFileName,
  const TRemoteFile *AFile, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData *Data = get_as<TSynchronizeData>(Param);
  Expects(Data != nullptr);

  UnicodeString LocalFileName = ChangeFileName(Data->CopyParam, AFile->FileName, osRemote, false);
  UnicodeString FullRemoteFileName = base::UnixExcludeTrailingBackslash(AFile->FullFileName);
  if (DoAllowRemoteFileTransfer(AFile, Data->CopyParam, true) &&
      (FLAGCLEAR(Data->Flags, sfFirstLevel) ||
       (Data->Options == nullptr) ||
        Data->Options->MatchesFilter(AFile->GetFileName()) ||
        Data->Options->MatchesFilter(LocalFileName)))
  {
    std::unique_ptr<TChecklistItem> ChecklistItem(new TChecklistItem());
    try__finally
    {
      ChecklistItem->IsDirectory = AFile->GetIsDirectory();
      ChecklistItem->ImageIndex = AFile->GetIconIndex();

      ChecklistItem->Remote.FileName = AFile->GetFileName();
      ChecklistItem->Remote.Directory = Data->RemoteDirectory;
      ChecklistItem->Remote.Modification = AFile->GetModification();
      ChecklistItem->Remote.ModificationFmt = AFile->GetModificationFmt();
      ChecklistItem->Remote.Size = AFile->GetSize();

      bool Modified = false;
      bool New = false;
      if (AFile->GetIsDirectory() && !CanRecurseToDirectory(AFile))
      {
        LogEvent(FORMAT("Skipping symlink to directory \"%s\".", AFile->GetFileName()));
      }
      else
      {
        intptr_t LocalIndex = Data->LocalFileList->IndexOf(LocalFileName);
        New = (LocalIndex < 0);
        if (!New)
        {
          TSynchronizeFileData *LocalData =
          Data->LocalFileList->GetAs<TSynchronizeFileData>(LocalIndex);

          LocalData->New = false;

          if (AFile->GetIsDirectory() != LocalData->IsDirectory)
          {
            LogEvent(FORMAT("%s is directory on one side, but file on the another",
              AFile->GetFileName()));
          }
          else if (!AFile->GetIsDirectory())
          {
            ChecklistItem->Local = LocalData->Info;

            ChecklistItem->Local.Modification =
              base::ReduceDateTimePrecision(ChecklistItem->Local.Modification, AFile->GetModificationFmt());

            bool LocalModified = false;
            // for spTimestamp+spBySize require that the file sizes are the same
            // before comparing file time
            intptr_t TimeCompare;
            if (FLAGCLEAR(Data->Params, spNotByTime) &&
                (FLAGCLEAR(Data->Params, spTimestamp) ||
                 FLAGCLEAR(Data->Params, spBySize) ||
                 (ChecklistItem->Local.Size == ChecklistItem->Remote.Size)))
            {
              TimeCompare = CompareFileTime(ChecklistItem->Local.Modification,
                   ChecklistItem->Remote.Modification);
            }
            else
            {
              TimeCompare = 0;
            }
            if (TimeCompare < 0)
            {
              if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                  (Data->Mode == smBoth) || (Data->Mode == smLocal))
              {
                Modified = true;
              }
              else
              {
                LocalModified = true;
              }
            }
            else if (TimeCompare > 0)
            {
              if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                  (Data->Mode == smBoth) || (Data->Mode == smRemote))
              {
                LocalModified = true;
              }
              else
              {
                Modified = true;
              }
            }
            else if (FLAGSET(Data->Params, spBySize) &&
                     (ChecklistItem->Local.Size != ChecklistItem->Remote.Size) &&
                     FLAGCLEAR(Data->Params, spTimestamp))
            {
              Modified = true;
              LocalModified = true;
            }

            if (LocalModified)
            {
              LocalData->Modified = true;
              LocalData->MatchingRemoteFile = ChecklistItem->Remote;
              LocalData->MatchingRemoteFileImageIndex = ChecklistItem->ImageIndex;
              // we need this for custom commands over checklist only,
              // not for sync itself
              LocalData->MatchingRemoteFileFile = AFile->Duplicate();
              LogEvent(FORMAT("Local file %s is modified comparing to remote file %s",
                FormatFileDetailsForLog(UnicodeString(LocalData->Info.Directory) + UnicodeString(LocalData->Info.FileName),
                   LocalData->Info.Modification, LocalData->Info.Size),
                 FormatFileDetailsForLog(FullRemoteFileName,
                   AFile->GetModification(), AFile->GetSize())));
            }

            if (Modified)
            {
              LogEvent(FORMAT("Remote file %s is modified comparing to local file %s",
                FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize()),
                 FormatFileDetailsForLog(UnicodeString(LocalData->Info.Directory) + UnicodeString(LocalData->Info.FileName),
                   LocalData->Info.Modification, LocalData->Info.Size)));
            }
          }
          else if (FLAGCLEAR(Data->Params, spNoRecurse))
          {
            DoSynchronizeCollectDirectory(
              Data->LocalDirectory + LocalData->Info.FileName,
              Data->RemoteDirectory + AFile->GetFileName(),
              Data->Mode, Data->CopyParam, Data->Params, Data->OnSynchronizeDirectory,
              Data->Options, (Data->Flags & ~sfFirstLevel),
              Data->Checklist);
          }
        }
        else
        {
          ChecklistItem->Local.Directory = Data->LocalDirectory;
          LogEvent(FORMAT("Remote file %s is new",
            FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize())));
        }
      }

      if (New || Modified)
      {
        DebugAssert(!New || !Modified);

        // download the file if it changed or is new and we want to have it locally
        if ((Data->Mode == smBoth) || (Data->Mode == smLocal))
        {
          if (FLAGCLEAR(Data->Params, spTimestamp) || Modified)
          {
            ChecklistItem->Action =
              (Modified ? TChecklistAction::saDownloadUpdate : TChecklistAction::saDownloadNew);
            ChecklistItem->Checked =
              (Modified || FLAGCLEAR(Data->Params, spExistingOnly)) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
                FLAGSET(Data->Params, spSubDirs));
          }
        }
        else if ((Data->Mode == smRemote) && New)
        {
          if (FLAGCLEAR(Data->Params, spTimestamp))
          {
            ChecklistItem->Action = saDeleteRemote;
            ChecklistItem->Checked =
              FLAGSET(Data->Params, spDelete) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
               FLAGSET(Data->Params, spSubDirs));
          }
        }

        if (ChecklistItem->Action != saNone)
        {
          ChecklistItem->RemoteFile = AFile->Duplicate();
          Data->Checklist->Add(ChecklistItem.get());
          ChecklistItem.release();
        }
      }
    },
    __finally__removed
    ({
      delete ChecklistItem;
    }) end_try__finally
  }
  else
  {
    LogEvent(0, FORMAT(L"Remote file %s excluded from synchronization",
      FormatFileDetailsForLog(FullRemoteFileName, AFile->Modification, AFile->Size)));
  }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeApply(
  TSynchronizeChecklist * Checklist,
  const TCopyParamType *CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TProcessedSynchronizationChecklistItem OnProcessedItem,
  TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems, void * Token,
  TFileOperationStatistics * Statistics)
{
  TSynchronizeData Data;

  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;

  intptr_t CopyParams =
    FLAGMASK(FLAGSET(Params, spNoConfirmation), cpNoConfirmation);

  TCopyParamType SyncCopyParam = *CopyParam;
  // when synchronizing by time, we force preserving time,
  // otherwise it does not make any sense
  if (FLAGCLEAR(Params, spNotByTime))
  {
    SyncCopyParam.SetPreserveTime(true);
  }

  if (SyncCopyParam.CalculateSize)
  {
    // If we fail to collect the sizes, do not try again during an actual transfer
    SyncCopyParam.CalculateSize = false;

    TSynchronizeChecklist::TItemList Items;
    for (int Index = 0; Index < Checklist->Count; Index++)
    {
      const TChecklistItem* ChecklistItem = Checklist->GetItem(Index);
      // TSynchronizeChecklistDialog relies on us not to update a size of an item that had size already
      // See TSynchronizeChecklistDialog::UpdatedSynchronizationChecklistItems
      if (ChecklistItem->Checked && !TSynchronizeChecklist::IsItemSizeIrrelevant(ChecklistItem->Action) &&
          !ChecklistItem->HasSize() && DebugAlwaysTrue(ChecklistItem->IsDirectory))
      {
        Items.push_back(ChecklistItem);
      }
    }

    SynchronizeChecklistCalculateSize(Checklist, Items, &SyncCopyParam);
    if (OnUpdatedSynchronizationChecklistItems != nullptr)
    {
      OnUpdatedSynchronizationChecklistItems(Items);
    }
  }

  BeginTransaction();
  TValueRestorer<TFileOperationProgressType::TPersistence *> OperationProgressPersistenceRestorer(FOperationProgressPersistence);  nb::used(OperationProgressPersistenceRestorer);
  TValueRestorer<TOnceDoneOperation> OperationProgressOnceDoneOperationRestorer(FOperationProgressOnceDoneOperation); nb::used(OperationProgressOnceDoneOperationRestorer);
  TFileOperationProgressType::TPersistence OperationProgressPersistence;
  OperationProgressPersistence.Statistics = Statistics;
  FOperationProgressPersistence = &OperationProgressPersistence;

  try__finally
  {
    int Index = 0;
    while (Index < Checklist->Count)
    {
      const TChecklistItem* ChecklistItem = Checklist->GetItem(Index);
      if (ChecklistItem->Checked)
      {
        if (!SamePaths(Data.LocalDirectory, ChecklistItem->Local.Directory) ||
            !base::UnixSamePath(Data.RemoteDirectory, ChecklistItem->Remote.Directory))
        {
          Data.LocalDirectory = IncludeTrailingBackslash(ChecklistItem->Local.Directory);
          Data.RemoteDirectory = base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory);

          LogEvent(
            FORMAT(L"Synchronizing local directory '%s' with remote directory '%s', params = 0x%x (%s)",
              Data.LocalDirectory, Data.RemoteDirectory, int(Params), SynchronizeParamsStr(Params)));

          DoSynchronizeProgress(Data, false);
        }

        std::unique_ptr<TStringList> FileList(std::make_unique<TStringList>());

        UnicodeString LocalPath = IncludeTrailingBackslash(ChecklistItem->Local.Directory) + ChecklistItem->Local.FileName;
        UnicodeString RemotePath = base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) + ChecklistItem->Remote.FileName;
        bool Result = true;

        if (FLAGSET(Params, spTimestamp))
        {
          // used by SynchronizeLocalTimestamp and SynchronizeRemoteTimestamp
          TObject * ChecklistItemToken = const_cast<TObject *>(reinterpret_cast<const TObject *>(ChecklistItem));
          switch (ChecklistItem->Action)
          {
            case TChecklistAction::saDownloadUpdate:
              FileList->AddObject(RemotePath, ChecklistItemToken);
              ProcessFiles(FileList.get(), foSetProperties, nb::bind(&TTerminal::SynchronizeLocalTimestamp, this), nullptr, osLocal);
              break;

            case TChecklistAction::saUploadUpdate:
              FileList->AddObject(LocalPath, ChecklistItemToken);
              ProcessFiles(FileList.get(), foSetProperties, nb::bind(&TTerminal::SynchronizeRemoteTimestamp, this));
              break;

            default:
              DebugFail();
              Result = false;
              break;
          }
        }
        else
        {
          TCopyParamType ItemCopyParam = SyncCopyParam;
          ItemCopyParam.Size = ChecklistItem->HasSize() ? ChecklistItem->GetSize() : -1;
          switch (ChecklistItem->Action)
          {
            case TChecklistAction::saDownloadNew:
            case TChecklistAction::saDownloadUpdate:
              FileList->AddObject(RemotePath, ChecklistItem->RemoteFile);
              Result = CopyToLocal(FileList.get(), Data.LocalDirectory, &ItemCopyParam, CopyParams, nullptr);
              break;

            case TChecklistAction::saDeleteRemote:
              FileList->AddObject(RemotePath, ChecklistItem->RemoteFile);
              Result = RemoteDeleteFiles(FileList.get());
              break;

            case TChecklistAction::saUploadNew:
            case TChecklistAction::saUploadUpdate:
              FileList->Add(LocalPath);
              Result = CopyToRemote(FileList.get(), Data.RemoteDirectory, &ItemCopyParam, CopyParams, nullptr);
              break;

            case TChecklistAction::saDeleteLocal:
              FileList->Add(LocalPath);
              Result = DeleteLocalFiles(FileList.get());
              break;

            default:
              DebugFail();
              Result = false;
              break;
          }
        }

        if (!Result)
        {
          Abort();
        }

        if (OnProcessedItem != nullptr)
        {
          OnProcessedItem(Token, ChecklistItem);
        }
      }

      Index++;
    }
  },
  __finally
  {
    EndTransaction();
  } end_try__finally

  if (FOperationProgressOnceDoneOperation != odoIdle)
  {
    // Otherwise CloseOnCompletion would have no effect
    OperationProgressPersistenceRestorer.Release();
    CloseOnCompletion(FOperationProgressOnceDoneOperation);
  }

}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeChecklistCalculateSize(
  TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items,
  const TCopyParamType * CopyParam)
{
  std::unique_ptr<TStrings> RemoteFileList(std::make_unique<TStringList>());
  std::unique_ptr<TStrings> LocalFileList(std::make_unique<TStringList>());

  for (size_t Index = 0; Index < Items.size(); Index++)
  {
    const TChecklistItem* ChecklistItem = Items[Index];
    if (ChecklistItem->IsDirectory)
    {
      if (ChecklistItem->IsRemoteOnly())
      {
        RemoteFileList->AddObject(ChecklistItem->RemoteFile->FullFileName, ChecklistItem->RemoteFile);
      }
      else if (ChecklistItem->IsLocalOnly())
      {
        LocalFileList->Add(IncludeTrailingBackslash(ChecklistItem->Local.Directory) + ChecklistItem->Local.FileName);
      }
      else
      {
        // "update" actions are not relevant for directories
        DebugFail();
      }
    }
  }

  TCalculatedSizes RemoteCalculatedSizes;
  TCalculatedSizes LocalCalculatedSizes;

  try__finally
  {
    bool Result = true;
    if (LocalFileList->Count > 0)
    {
      int64_t LocalSize = 0;
      Result = CalculateLocalFilesSize(LocalFileList.get(), LocalSize, CopyParam, true, nullptr, &LocalCalculatedSizes);
    }
    if (Result && (RemoteFileList->Count > 0))
    {
      int64_t RemoteSize = 0;
      TCalculateSizeStats RemoteStats;
      RemoteStats.CalculatedSizes = &RemoteCalculatedSizes;
      CalculateFilesSize(RemoteFileList.get(), RemoteSize, 0, CopyParam, true, RemoteStats);
    }
  },
  __finally
  {
    size_t LocalIndex = 0;
    size_t RemoteIndex = 0;

    for (size_t Index = 0; Index < Items.size(); Index++)
    {
      const TChecklistItem* ChecklistItem = Items[Index];
      if (ChecklistItem->IsDirectory)
      {
        int64_t Size = -1;
        if (ChecklistItem->IsRemoteOnly())
        {
          if (RemoteIndex < RemoteCalculatedSizes.size())
          {
            Size = RemoteCalculatedSizes[RemoteIndex];
          }
          RemoteIndex++;
        }
        else if (ChecklistItem->IsLocalOnly())
        {
          if (LocalIndex < LocalCalculatedSizes.size())
          {
            Size = LocalCalculatedSizes[LocalIndex];
          }
          LocalIndex++;
        }
        else
        {
          // "update" actions are not relevant for directories
          DebugFail();
        }

        if (Size >= 0)
        {
          Checklist->UpdateDirectorySize(ChecklistItem, Size);
        }
      }
    }

    DebugAssert(RemoteIndex >= RemoteCalculatedSizes.size());
    DebugAssert(LocalIndex >= LocalCalculatedSizes.size());
  } end_try__finally
}
//---------------------------------------------------------------------------
void TTerminal::DoSynchronizeProgress(const TSynchronizeData &Data,
  bool Collect)
{
  if (Data.OnSynchronizeDirectory)
  {
    bool Continue = true;
    Data.OnSynchronizeDirectory(Data.LocalDirectory, Data.RemoteDirectory,
      Continue, Collect);

    if (!Continue)
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeLocalTimestamp(UnicodeString /*AFileName*/,
  const TRemoteFile *AFile, void * /*Param*/)
{
  const TChecklistItem *ChecklistItem =
    reinterpret_cast<const TChecklistItem *>(AFile);

  UnicodeString LocalFile =
    ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
    ChecklistItem->Local.FileName;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(CANT_SET_ATTRS, LocalFile), "",
  [&]()
  {
    // this->SetLocalFileTime(LocalFile, ChecklistItem->Remote.Modification);
    HANDLE Handle;
    this->TerminalOpenLocalFile(LocalFile, GENERIC_WRITE, nullptr, &Handle,
      nullptr, nullptr, nullptr, nullptr);
    FILETIME WrTime = ::DateTimeToFileTime(ChecklistItem->Remote.Modification,
      SessionData->GetDSTMode());
    bool Result = ::SetFileTime(Handle, nullptr, nullptr, &WrTime) != FALSE;
    int Error = ::GetLastError();
    ::CloseHandle(Handle);
    if (!Result)
    {
      ::RaiseLastOSError(Error);
    }
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (LocalFile)));
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeRemoteTimestamp(UnicodeString /*AFileName*/,
  const TRemoteFile *AFile, void * /*Param*/)
{
  const TChecklistItem *ChecklistItem =
    reinterpret_cast<const TChecklistItem *>(AFile);

  TRemoteProperties Properties;
  Properties.Valid << vpModification;
  Properties.Modification = ::ConvertTimestampToUnix(ChecklistItem->FLocalLastWriteTime,
    GetSessionData()->GetDSTMode());

  ChangeFileProperties(
    base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) + ChecklistItem->Remote.FileName,
    nullptr, &Properties);
}
//---------------------------------------------------------------------------
void TTerminal::FileFind(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*TFilesFindParams*/ void *Param)
{
  // see DoFilesFind
  FOnFindingFile = nullptr;

  DebugAssert(Param);
  DebugAssert(AFile);
  TFilesFindParams *AParams = get_as<TFilesFindParams>(Param);

  if (!AParams->Cancel)
  {
    UnicodeString LocalFileName = AFileName;
    if (AFileName.IsEmpty())
    {
      LocalFileName = AFile->GetFileName();
    }

    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->GetSize();
    MaskParams.Modification = AFile->GetModification();

    UnicodeString FullFileName = base::UnixExcludeTrailingBackslash(AFile->GetFullFileName());
    bool ImplicitMatch = false;
    // Do not use recursive include match
    if (AParams->FileMask.Matches(FullFileName, false,
         AFile->GetIsDirectory(), &MaskParams, false, ImplicitMatch))
    {
      if (!ImplicitMatch)
      {
        AParams->OnFileFound(this, LocalFileName, AFile, AParams->Cancel);
      }

      if (AFile->GetIsDirectory())
      {
        UnicodeString RealDirectory;
        if (!AFile->GetIsSymLink() || AFile->GetLinkTo().IsEmpty())
        {
          RealDirectory = base::UnixIncludeTrailingBackslash(AParams->RealDirectory) + AFile->GetFileName();
        }
        else
        {
          RealDirectory = base::AbsolutePath(AParams->RealDirectory, AFile->GetLinkTo());
        }

        if (!AParams->LoopDetector.IsUnvisitedDirectory(RealDirectory))
        {
          LogEvent(FORMAT("Already searched \"%s\" directory, link loop detected", FullFileName));
        }
        else
        {
          DoFilesFind(FullFileName, *AParams, RealDirectory);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoFilesFind(const UnicodeString ADirectory, TFilesFindParams &Params, const UnicodeString ARealDirectory)
{
  LogEvent(FORMAT("Searching directory \"%s\" (real path \"%s\")", ADirectory, ARealDirectory));
  Params.OnFindingFile(this, ADirectory, Params.Cancel);
  if (!Params.Cancel)
  {
    DebugAssert(FOnFindingFile == nullptr);
    // ideally we should set the handler only around actually reading
    // of the directory listing, so we at least reset the handler in
    // FileFind
    FOnFindingFile = Params.OnFindingFile;
    UnicodeString PrevRealDirectory = Params.RealDirectory;
    try__finally
    {
      Params.RealDirectory = ARealDirectory;
      ProcessDirectory(ADirectory, nb::bind(&TTerminal::FileFind, this), &Params, false, true);
    },
    __finally
    {
      Params.RealDirectory = PrevRealDirectory;
      FOnFindingFile = nullptr;
    } end_try__finally
  }
}
//---------------------------------------------------------------------------
void TTerminal::FilesFind(const UnicodeString ADirectory, const TFileMasks &FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;

  Params.LoopDetector.RecordVisitedDirectory(ADirectory);

  DoFilesFind(ADirectory, Params, ADirectory);
}
//---------------------------------------------------------------------------
void TTerminal::SpaceAvailable(const UnicodeString APath,
  TSpaceAvailable &ASpaceAvailable)
{
  DebugAssert(GetIsCapable(fcCheckingSpaceAvailable));

  try
  {
    FFileSystem->SpaceAvailable(APath, ASpaceAvailable);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(SPACE_AVAILABLE_ERROR, APath));
  }
}
//---------------------------------------------------------------------------
void TTerminal::LockFile(UnicodeString AFileName,
  const TRemoteFile *AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foLock);

  LogEvent(FORMAT("Locking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoLockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}
//---------------------------------------------------------------------------
void TTerminal::DoLockFile(UnicodeString AFileName, const TRemoteFile *AFile)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FFileSystem->LockFile(AFileName, AFile);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, FMTLOAD(LOCK_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::UnlockFile(UnicodeString AFileName,
  const TRemoteFile *AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foUnlock);

  LogEvent(FORMAT("Unlocking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoUnlockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}
//---------------------------------------------------------------------------
void TTerminal::DoUnlockFile(UnicodeString AFileName, const TRemoteFile *AFile)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FFileSystem->UnlockFile(AFileName, AFile);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, FMTLOAD(UNLOCK_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::LockFiles(TStrings *AFileList)
{
  BeginTransaction();
  try__finally
  {
    ProcessFiles(AFileList, foLock, nb::bind(&TTerminal::LockFile, this), nullptr);
  },
  __finally
  {
    EndTransaction();
  } end_try__finally
}
//---------------------------------------------------------------------------
void TTerminal::UnlockFiles(TStrings *AFileList)
{
  BeginTransaction();
  try__finally
  {
    ProcessFiles(AFileList, foUnlock, nb::bind(&TTerminal::UnlockFile, this), nullptr);
  },
  __finally
  {
    EndTransaction();
  } end_try__finally
}
//---------------------------------------------------------------------------
const TSessionInfo & TTerminal::GetSessionInfo() const
{
  return FFileSystem->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}
//---------------------------------------------------------------------------
void TTerminal::GetSupportedChecksumAlgs(TStrings *Algs) const
{
  FFileSystem->GetSupportedChecksumAlgs(Algs);
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetPassword() const
{
  UnicodeString Result;
  // FRememberedPassword is empty also when stored password was used
  if (FRememberedPassword.IsEmpty())
  {
    Result = GetSessionData()->GetPassword();
  }
  else if (FRememberedPasswordKind != pkPassphrase)
  {
    Result = GetRememberedPassword();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetRememberedPassword() const
{
  return DecryptPassword(FRememberedPassword);
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetRememberedTunnelPassword() const
{
  return DecryptPassword(FRememberedTunnelPassword);
}
//---------------------------------------------------------------------------
bool TTerminal::GetStoredCredentialsTried() const
{
  bool Result;
  if (FFileSystem != nullptr)
  {
    Result = FFileSystem->GetStoredCredentialsTried();
  }
  else if (FSecureShell != nullptr)
  {
    Result = FSecureShell->GetStoredCredentialsTried();
  }
  else
  {
    DebugAssert(FTunnelOpening);
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TTerminal::CopyToParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress)
{
  UnicodeString FileName;
  TObject *Object;
  UnicodeString TargetDir;
  bool Dir;
  bool Recursed;

  intptr_t Result = ParallelOperation->GetNext(this, FileName, Object, TargetDir, Dir, Recursed);
  if (Result > 0)
  {
    std::unique_ptr<TStrings> FilesToCopy(std::make_unique<TStringList>());
    FilesToCopy->AddObject(FileName, Object);

    if (ParallelOperation->GetSide() == osLocal)
    {
      TargetDir = TranslateLockedPath(TargetDir, false);
    }
    // OnceDoneOperation is not supported
    TOnceDoneOperation OnceDoneOperation = odoIdle;

    intptr_t Params = ParallelOperation->GetParams();
    // If we failed to recurse the directory when enumerating, recurse now (typically only to show the recursion error)
    if (Dir && Recursed)
    {
      Params = Params | cpNoRecurse;
    }

    intptr_t Prev = OperationProgress->GetFilesFinishedSuccessfully();
    DebugAssert((FOperationProgress == OperationProgress) || (FOperationProgress == nullptr));
    TFileOperationProgressType * PrevOperationProgress = FOperationProgress;
    try__finally
    {
      FOperationProgress = OperationProgress;
      if (ParallelOperation->GetSide() == osLocal)
      {
        FFileSystem->CopyToRemote(
          FilesToCopy.get(), TargetDir, ParallelOperation->GetCopyParam(), Params, OperationProgress, OnceDoneOperation);
      }
      else if (DebugAlwaysTrue(ParallelOperation->GetSide() == osRemote))
      {
        FFileSystem->CopyToLocal(
          FilesToCopy.get(), TargetDir, ParallelOperation->GetCopyParam(), Params, OperationProgress, OnceDoneOperation);
      }
    },
    __finally
    {
      bool Success = (Prev < OperationProgress->GetFilesFinishedSuccessfully());
      ParallelOperation->Done(FileName, Dir, Success);
      // Not to fail an assertion in OperationStop when called from CopyToRemote or CopyToLocal,
      // when FOperationProgress is already OperationProgress.
      FOperationProgress = PrevOperationProgress;
    } end_try__finally
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CanParallel(
  const TCopyParamType *CopyParam, intptr_t Params, TParallelOperation *ParallelOperation) const
{
  return
    (ParallelOperation != nullptr) &&
    FFileSystem->IsCapable(fsParallelTransfers) &&
    // parallel transfer is not implemented for operations needed to be done on a folder
    // after all its files are processed
    FLAGCLEAR(Params, cpDelete) &&
    (!CopyParam->GetPreserveTime() || !CopyParam->GetPreserveTimeDirs());
}
//---------------------------------------------------------------------------
void TTerminal::CopyParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress)
{
  try__finally
  {
    bool Continue = true;
    do
    {
      intptr_t GotNext = CopyToParallel(ParallelOperation, OperationProgress);
      if (GotNext < 0)
      {
        Continue = false;
      }
      else if (GotNext == 0)
      {
        Sleep(100);
      }
    }
    while (Continue && !OperationProgress->GetCancel());
  },
  __finally
  {
     OperationProgress->SetDone();
     ParallelOperation->WaitFor();
  } end_try__finally
}
//---------------------------------------------------------------------------
void TTerminal::LogParallelTransfer(TParallelOperation *ParallelOperation)
{
  LogEvent(
    FORMAT("Adding a parallel transfer to the transfer started on the connection \"%s\"",
      ParallelOperation->GetMainName()));
}
//---------------------------------------------------------------------------
void TTerminal::LogTotalTransferDetails(
  const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
  TFileOperationProgressType *OperationProgress, bool Parallel, TStrings *AFiles)
{
  if (FLog->GetLogging())
  {
    UnicodeString TargetSide = ((OperationProgress->GetSide() == osLocal) ? L"remote" : L"local");
    UnicodeString S =
      FORMAT(
        "Copying %d files/directories to %s directory \"%s\"",
        OperationProgress->GetCount(), TargetSide, ATargetDir);
    if (Parallel && DebugAlwaysTrue(AFiles != nullptr))
    {
      intptr_t Count = 0;
      for (intptr_t Index = 0; Index < AFiles->GetCount(); ++Index)
      {
        TCollectedFileList *FileList = dyn_cast<TCollectedFileList>(AFiles->GetObj(Index));
        Count += FileList->GetCount();
      }
      S += FORMAT(" - in parallel, with %d total files", Count);
    }
    if (OperationProgress->GetTotalSizeSet())
    {
      S += FORMAT(" - total size: %s", FormatSize(OperationProgress->GetTotalSize()));
    }
    LogEvent(S);
    LogEvent(0, CopyParam->LogStr);
  }
}
//---------------------------------------------------------------------------
void TTerminal::LogTotalTransferDone(TFileOperationProgressType *OperationProgress)
{
  LogEvent(L"Copying finished: " + OperationProgress->GetLogStr(true));
}
//---------------------------------------------------------------------------
bool TTerminal::CopyToRemote(
  TStrings * AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
  TParallelOperation * ParallelOperation)
{
  DebugAssert(FFileSystem);
  DebugAssert(AFilesToCopy);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TFileOperationProgressType OperationProgress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));
  try
  {
    int64_t Size = 0;
    std::unique_ptr<TStringList> Files;
    bool ACanParallel =
      CanParallel(CopyParam, AParams, ParallelOperation) &&
      !CopyParam->ClearArchive;
    if (ACanParallel)
    {
      Files = std::make_unique<TStringList>();
      Files->SetOwnsObjects(true);
    }
    bool CalculatedSize;
    if ((CopyParam->Size >= 0) &&
        DebugAlwaysTrue(!ACanParallel)) // Size is set for sync only and we never use parallel transfer for sync
    {
      Size = CopyParam->Size;
      CalculatedSize = true;
    }
    else
    {
      bool CalculateSize = ACanParallel || CopyParam->CalculateSize;
      CalculatedSize = CalculateLocalFilesSize(AFilesToCopy, Size, CopyParam, CalculateSize, Files.get(), nullptr);
    }

    FLastProgressLogged = GetTickCount();
    OperationStart(
      OperationProgress, (AParams & cpDelete ? foMove : foCopy), osLocal,
      AFilesToCopy->Count, AParams & cpTemporary, ATargetDir, CopyParam->CPSLimit);

    __removed bool CollectingUsage = false;
    try__finally
    {
      if (CalculatedSize)
      {
#if 0
        if (Configuration->Usage->Collect)
        {
          int CounterSize = TUsage::CalculateCounterSize(Size);
          Configuration->Usage->Inc(L"Uploads");
          Configuration->Usage->Inc(L"UploadedBytes", CounterSize);
          Configuration->Usage->SetMax(L"MaxUploadSize", CounterSize);
          CollectingUsage = true;
        }
#endif // #if 0
        OperationProgress.SetTotalSize(Size);
      }

      UnicodeString UnlockedTargetDir = TranslateLockedPath(ATargetDir, false);
      BeginTransaction();
      try__finally
      {
        bool Parallel = ACanParallel && CalculatedSize;
        LogTotalTransferDetails(ATargetDir, CopyParam, &OperationProgress, Parallel, Files.get());

        if (Parallel)
        {
          // OnceDoneOperation is not supported
          ParallelOperation->Init(Files.release(), UnlockedTargetDir, CopyParam, AParams, &OperationProgress, GetLog()->GetName());
          CopyParallel(ParallelOperation, &OperationProgress);
        }
        else
        {
          FFileSystem->CopyToRemote(AFilesToCopy, UnlockedTargetDir,
            CopyParam, AParams, &OperationProgress, OnceDoneOperation);
        }

        LogTotalTransferDone(&OperationProgress);
      },
      __finally
      {
        if (GetActive())
        {
          ReactOnCommand(fsCopyToRemote);
        }
        EndTransaction();
      } end_try__finally

      if (OperationProgress.GetCancel() == csContinue)
      {
        Result = true;
      }
    },
    __finally
    {
#if 0
      if (CollectingUsage)
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"UploadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxUploadTime", CounterTime);
      }
#endif //if 0
      OperationStop(OperationProgress);
    } end_try__finally
  }
  catch (Exception &E)
  {
    if (OperationProgress.GetCancel() != csCancel)
    {
      CommandError(&E, MainInstructions(LoadStr(TOREMOTE_COPY_ERROR)));
    }
    OnceDoneOperation = odoIdle;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DoCopyToRemote(
  TStrings *AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
  TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TOnceDoneOperation &OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  FFileSystem->TransferOnDirectory(ATargetDir, CopyParam, AParams);

  UnicodeString TargetDir = GetAbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = base::UnixIncludeTrailingBackslash(ATargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && !OperationProgress->GetCancel())
  {
    bool Success = false;
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    TSearchRecSmart * SearchRec = nullptr;
    if (AFilesToCopy->GetObj(Index) != nullptr)
    {
      TLocalFile * LocalFile = AFilesToCopy->GetAs<TLocalFile>(Index);
      SearchRec = &LocalFile->SearchRec;
    }

    try__finally
    {
      try
      {
        if (GetSessionData()->GetCacheDirectories())
        {
          DirectoryModified(TargetDir, false);

          if (::SysUtulsDirectoryExists(ApiPath(FileName)))
          {
            UnicodeString FileNameOnly = base::ExtractFileName(FileName, false);
            DirectoryModified(FullTargetDir + FileNameOnly, true);
          }
        }
        SourceRobust(FileName, SearchRec, FullTargetDir, CopyParam, AParams, OperationProgress, AFlags | tfFirstLevel);
        Success = true;
      }
      catch (ESkipFile &E)
      {
        volatile TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!HandleException(&E))
        {
          throw;
        }
      }
    },
    __finally
    {
      OperationFinish(OperationProgress, AFilesToCopy->GetObj(Index), FileName, Success, OnceDoneOperation);
    } end_try__finally
    Index++;
  }
}
//---------------------------------------------------------------------------
void TTerminal::SourceRobust(
  const UnicodeString AFileName, const TSearchRecSmart * SearchRec,
  const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
  TFileOperationProgressType * OperationProgress, uintptr_t AFlags)
{
  TUploadSessionAction Action(GetActionLog());
  bool * AFileTransferAny = FLAGSET(AFlags, tfUseFileTransferAny) ? &FFileTransferAny : nullptr;
  TRobustOperationLoop RobustLoop(this, OperationProgress, AFileTransferAny);

  do
  {
    bool ChildError = false;
    try
    {
      Source(AFileName, SearchRec, ATargetDir, CopyParam, AParams, OperationProgress, AFlags, Action, ChildError);
    }
    catch (Exception & E)
    {
      if (!RobustLoop.TryReopen(E))
      {
        if (!ChildError)
        {
          RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (RobustLoop.ShouldRetry())
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite and resume confirmations (should not be set for directories!)
      AParams |= cpNoConfirmation;
      // enable resume even if we are uploading into new directory
      AFlags &= ~tfNewDirectory;
      AFlags |= tfAutoResume;
    }
  }
  while (RobustLoop.Retry());
}
//---------------------------------------------------------------------------
void TTerminal::CreateTargetDirectory(
  const UnicodeString ADirectoryPath, uintptr_t Attrs, const TCopyParamType *CopyParam)
{
  TRemoteProperties Properties;
  if (CopyParam->GetPreserveRights())
  {
    Properties.Valid = Properties.Valid << vpRights;
    Properties.Rights = CopyParam->RemoteFileRights(Attrs);
  }
  Properties.Valid = Properties.Valid << vpEncrypt;
  Properties.Encrypt = CopyParam->EncryptNewFiles;
  RemoteCreateDirectory(ADirectoryPath, &Properties);
}
//---------------------------------------------------------------------------
void TTerminal::DirectorySource(
  const UnicodeString ADirectoryName, const UnicodeString ATargetDir, const UnicodeString ADestDirectoryName,
  uintptr_t Attrs, const TCopyParamType *CopyParam, intptr_t AParams,
  TFileOperationProgressType *OperationProgress, uintptr_t AFlags)
{
  FFileSystem->TransferOnDirectory(ATargetDir, CopyParam, AParams);

  UnicodeString DestFullName = base::UnixIncludeTrailingBackslash(ATargetDir + ADestDirectoryName);

  OperationProgress->SetFile(ADirectoryName);

  bool PostCreateDir = FLAGCLEAR(AFlags, tfPreCreateDir);
  // WebDAV and SFTP
  if (!PostCreateDir)
  {
    // This is originally a code for WebDAV, SFTP used a slightly different logic,
    // but functionally it should be very similar.
    if (!FileExists(DestFullName))
    {
      CreateTargetDirectory(DestFullName, Attrs, CopyParam);
      AFlags |= tfNewDirectory;
    }
  }

  if (FLAGCLEAR(AParams, cpNoRecurse))
  {
    TSearchRecOwned SearchRec;
    bool FindOK = LocalFindFirstLoop(ADirectoryName + L"*.*", SearchRec);
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = ADirectoryName + SearchRec.Name;
      try
      {
        if (SearchRec.IsRealFile())
        {
          SourceRobust(FileName, &SearchRec, DestFullName, CopyParam, AParams, OperationProgress, (AFlags & ~(tfFirstLevel | tfAutoResume)));
          // FTP: if any file got uploaded (i.e. there were any file in the directory and at least one was not skipped),
          // do not try to create the directory, as it should be already created by FZAPI during upload
          PostCreateDir = false;
        }
      }
      catch (ESkipFile &E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        TSuspendFileOperationProgress Suspend(OperationProgress);
        // here a message to user was displayed, which was not appropriate
        // when user refused to overwrite the file in subdirectory.
        // hopefully it won't be missing in other situations.
        if (!HandleException(&E))
        {
          throw;
        }
      }

      FindOK = LocalFindNextLoop(SearchRec);
    }

    SearchRec.Close();

    // FTP
    if (PostCreateDir)
    {
      TRemoteFile * File = nullptr;
      // ignore non-fatal error when the directory already exists
      bool DoCreate =
        !FileExists(DestFullName, &File) ||
        !File->GetIsDirectory(); // just try to create and make it fail
      delete File;

      if (DoCreate)
      {
        CreateTargetDirectory(DestFullName, Attrs, CopyParam);
      }
    }

    // TODO : Delete also read-only directories.
    // TODO : Show error message on failure.
    if (!OperationProgress->GetCancel())
    {
      if (GetIsCapable(fcPreservingTimestampDirs) && CopyParam->GetPreserveTime() && CopyParam->GetPreserveTimeDirs())
      {
        TRemoteProperties Properties;
        Properties.Valid << vpModification;

        this->TerminalOpenLocalFile(
          ::ExcludeTrailingBackslash(ADirectoryName), GENERIC_READ, nullptr, nullptr, nullptr,
          &Properties.Modification, &Properties.LastAccess, nullptr);

        ChangeFileProperties(DestFullName, nullptr, &Properties);
      }

      if (FLAGSET(AParams, cpDelete))
      {
        DebugAssert(FLAGCLEAR(AParams, cpNoRecurse));
        ::SysUtulsRemoveDir(ApiPath(ADirectoryName));
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
      {
        FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
          FMTLOAD(CANT_SET_ATTRS, ADirectoryName), "",
        [&]()
        {
          THROWOSIFFALSE(::SysUtulsFileSetAttr(ApiPath(ADirectoryName), (DWORD)(Attrs & ~faArchive)) == 0);
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (DirectoryName)));
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::SelectTransferMode(
  const UnicodeString ABaseFileName, TOperationSide Side, const TCopyParamType *CopyParam,
  const TFileMasks::TParams &MaskParams)
{
  bool AsciiTransfer = GetIsCapable(fcTextMode) && CopyParam->UseAsciiTransfer(ABaseFileName, Side, MaskParams);
  OperationProgress->SetAsciiTransfer(AsciiTransfer);
  UnicodeString ModeName = (OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary");
  LogEvent(0, FORMAT(L"%s transfer mode selected.", ModeName));
}
//---------------------------------------------------------------------------
void TTerminal::SelectSourceTransferMode(const TLocalFileHandle &Handle, const TCopyParamType *CopyParam)
{
  // Will we use ASCII or BINARY file transfer?
  TFileMasks::TParams MaskParams;
  MaskParams.Size = Handle.Size;
  MaskParams.Modification = Handle.Modification;
  UnicodeString BaseFileName = GetBaseFileName(Handle.FileName);
  SelectTransferMode(BaseFileName, osLocal, CopyParam, MaskParams);
}
//---------------------------------------------------------------------------
void TTerminal::UpdateSource(const TLocalFileHandle &AHandle, const TCopyParamType *CopyParam, intptr_t AParams)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  // TODO: Delete also read-only files.
  if (FLAGSET(AParams, cpDelete))
  {
    if (!AHandle.Directory)
    {
      FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
        FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AHandle.FileName), "",
      [&]()
      {
        THROWOSIFFALSE(::SysUtulsRemoveFile(ApiPath(AHandle.FileName)));
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (AHandle.FileName)));
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(AHandle.Attrs, faArchive))
  {
    FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
      FMTLOAD(CANT_SET_ATTRS, AHandle.FileName), "",
    [&]()
    {
      THROWOSIFFALSE(::SysUtulsFileSetAttr(ApiPath(AHandle.FileName), (DWORD)(AHandle.Attrs & ~faArchive)) == 0);
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (Handle.FileName)));
  }
}
//---------------------------------------------------------------------------
void TTerminal::Source(
  const UnicodeString AFileName, const TSearchRecSmart * SearchRec,
  const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
  TFileOperationProgressType * OperationProgress, uintptr_t AFlags, TUploadSessionAction & Action, bool & ChildError)
{
  Action.SetFileName(::ExpandUNCFileName(AFileName));

  OperationProgress->SetFile(AFileName, false);

  if (!AllowLocalFileTransfer(AFileName, SearchRec, CopyParam, OperationProgress))
  {
    throw ESkipFile();
  }

  TLocalFileHandle Handle;
  TerminalOpenLocalFile(AFileName, GENERIC_READ, Handle);

  OperationProgress->SetFileInProgress();

  UnicodeString DestFileName =
    ChangeFileName(CopyParam, base::ExtractFileName(AFileName, false), osLocal, FLAGSET(AFlags, tfFirstLevel));

  if (Handle.Directory)
  {
    Action.Cancel();
    DirectorySource(
      IncludeTrailingBackslash(AFileName), ATargetDir, DestFileName, Handle.Attrs,
      CopyParam, AParams, OperationProgress, AFlags);
  }
  else
  {
    LogEvent(FORMAT("Copying \"%s\" to remote directory started.", AFileName));

    OperationProgress->SetLocalSize(Handle.Size);

    // Suppose same data size to transfer as to read
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(OperationProgress->GetLocalSize());

    if (GetIsCapable(fcTextMode))
    {
      SelectSourceTransferMode(Handle, CopyParam);
    }

    FFileSystem->Source(
      Handle, ATargetDir, DestFileName, CopyParam, AParams, OperationProgress, AFlags, Action, ChildError);

    LogFileDone(OperationProgress, GetAbsolutePath(ATargetDir + DestFileName, true));
    OperationProgress->Succeeded();
  }

  Handle.Release();

  UpdateSource(Handle, CopyParam, AParams);
}
//---------------------------------------------------------------------------
bool TTerminal::CopyToLocal(
  TStrings * AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
  TParallelOperation *ParallelOperation)
{
  DebugAssert(FFileSystem);

  // see scp.c: sink(), tolocal()

  bool Result = false;
  DebugAssert(AFilesToCopy != nullptr);
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  FDestFileName = L"";
  FMultipleDestinationFiles = false;

  BeginTransaction();
  try__finally
  {
    int64_t TotalSize = 0;
    bool TotalSizeKnown = false;
    FLastProgressLogged = GetTickCount();
    TFileOperationProgressType OperationProgress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));

    std::unique_ptr<TStringList> Files;
    bool ACanParallel = CanParallel(CopyParam, AParams, ParallelOperation);
    if (ACanParallel)
    {
      Files = std::make_unique<TStringList>();
      Files->SetOwnsObjects(true);
    }

    if ((CopyParam->Size >= 0) &&
        DebugAlwaysTrue(!ACanParallel)) // Size is set for sync only and we never use parallel transfer for sync
    {
      TotalSize = CopyParam->Size;
      TotalSizeKnown = true;
    }
    else
    {
      ExceptionOnFail = true;
      try__finally
      {
        TCalculateSizeStats Stats;
        Stats.FoundFiles = Files.get();
        bool CalculateSize = ACanParallel || CopyParam->CalculateSize;
        if (CalculateFilesSize(AFilesToCopy, TotalSize, csIgnoreErrors, CopyParam, CalculateSize, Stats))
        {
          TotalSizeKnown = true;
        }
      },
      __finally
      {
        ExceptionOnFail = false;
      } end_try__finally
    }

    OperationStart(OperationProgress, (AParams & cpDelete ? foMove : foCopy), osRemote,
      AFilesToCopy ? AFilesToCopy->GetCount() : 0, (AParams & cpTemporary) != 0, ATargetDir, CopyParam->GetCPSLimit());

    //bool CollectingUsage = false;
    try__finally
    {
      if (TotalSizeKnown)
      {
#if 0
        if (Configuration->Usage->Collect)
        {
          int CounterTotalSize = TUsage::CalculateCounterSize(TotalSize);
          Configuration->Usage->Inc(L"Downloads");
          Configuration->Usage->Inc(L"DownloadedBytes", CounterTotalSize);
          Configuration->Usage->SetMax(L"MaxDownloadSize", CounterTotalSize);
          CollectingUsage = true;
        }
#endif // #if 0
        OperationProgress.SetTotalSize(TotalSize);
      }

      try
      {
        try__finally
        {
          bool Parallel = ACanParallel && TotalSizeKnown;
          LogTotalTransferDetails(ATargetDir, CopyParam, &OperationProgress, Parallel, Files.get());

          if (Parallel)
          {
            // OnceDoneOperation is not supported
            ParallelOperation->Init(Files.release(), ATargetDir, CopyParam, AParams, &OperationProgress, GetLog()->GetName());
            CopyParallel(ParallelOperation, &OperationProgress);
          }
          else
          {
            FFileSystem->CopyToLocal(AFilesToCopy, ATargetDir, CopyParam, AParams,
              &OperationProgress, OnceDoneOperation);
          }

          LogTotalTransferDone(&OperationProgress);
        },
        __finally
        {
          if (GetActive())
          {
            ReactOnCommand(fsCopyToLocal);
          }
        } end_try__finally
      }
      catch (Exception &E)
      {
        if (OperationProgress.GetCancel() != csCancel)
        {
          CommandError(&E, MainInstructions(LoadStr(TOLOCAL_COPY_ERROR)));
        }
        OnceDoneOperation = odoIdle;
      }

      if (OperationProgress.GetCancel() == csContinue)
      {
        Result = true;
      }
    },
    __finally
    {
#if 0
      if (CollectingUsage)
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"DownloadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxDownloadTime", CounterTime);
      }
#endif // #if 0
      OperationStop(OperationProgress);
      FOperationProgress = nullptr;
    } end_try__finally
  },
  __finally
  {
    // If session is still active (no fatal error) we reload directory
    // by calling EndTransaction
    EndTransaction();
  } end_try__finally

  if (OnceDoneOperation != odoIdle)
  {
    UnicodeString DestFileName;
    if (!FMultipleDestinationFiles)
    {
      DestFileName = FDestFileName;
    }
    CloseOnCompletion(OnceDoneOperation, UnicodeString(), ATargetDir, DestFileName);
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DoCopyToLocal(
  TStrings *AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TOnceDoneOperation &OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  UnicodeString FullTargetDir = IncludeTrailingBackslash(ATargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && !OperationProgress->GetCancel())
  {
    bool Success = false;
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile * File = AFilesToCopy->GetAs<const TRemoteFile>(Index);

    try__finally
    {
      try
      {
        UnicodeString AbsoluteFileName = GetAbsolutePath(FileName, true);
        SinkRobust(AbsoluteFileName, File, FullTargetDir, CopyParam, Params, OperationProgress, AFlags | tfFirstLevel);
        Success = true;
      }
      catch (ESkipFile &E)
      {
        volatile TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!HandleException(&E))
        {
          throw;
        }
      }
    },
    __finally
    {
      OperationFinish(OperationProgress, File, FileName, Success, OnceDoneOperation);
    } end_try__finally
    Index++;
  }
}
//---------------------------------------------------------------------------
void TTerminal::SinkRobust(
  const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ATargetDir,
  const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress, uintptr_t AFlags)
{
  TDownloadSessionAction Action(GetActionLog());
  bool *FileTransferAny = FLAGSET(AFlags, tfUseFileTransferAny) ? &FFileTransferAny : nullptr;
  TRobustOperationLoop RobustLoop(this, OperationProgress, FileTransferAny);
  bool Sunk = false;

  do
  {
    try
    {
      // If connection is lost while deleting the file, so not retry download on the next round.
      // The file may not exist anymore and the download attempt might overwrite the (only) local copy.
      if (!Sunk)
      {
        Sink(AFileName, AFile, ATargetDir, CopyParam, AParams, OperationProgress, AFlags, Action);
        Sunk = true;
      }

      if (FLAGSET(AParams, cpDelete))
      {
        DebugAssert(FLAGCLEAR(AParams, cpNoRecurse));
        // If file is directory, do not delete it recursively, because it should be
        // empty already. If not, it should not be deleted (some files were
        // skipped or some new files were copied to it, while we were downloading)
        intptr_t Params = dfNoRecursive;
        RemoteDeleteFile(AFileName, AFile, &Params);
      }
    }
    catch (Exception & E)
    {
      if (!RobustLoop.TryReopen(E))
      {
        if (!Sunk)
        {
          RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (RobustLoop.ShouldRetry())
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      DebugAssert(AFile != nullptr);
      if (!AFile->GetIsDirectory())
      {
        // prevent overwrite and resume confirmations
        AParams |= cpNoConfirmation;
        AFlags |= tfAutoResume;
      }
    }
  }
  while (RobustLoop.Retry());
}
#if 0
//---------------------------------------------------------------------------
struct TSinkFileParams
{
  UnicodeString TargetDir;
  const TCopyParamType * CopyParam;
  int Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  unsigned int Flags;
};
#endif //if 0
//---------------------------------------------------------------------------
void TTerminal::Sink(
  const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ATargetDir,
  const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress, uintptr_t Flags,
  TDownloadSessionAction &Action)
{
  Action.SetFileName(AFileName);
  DebugAssert(AFile);

  if (!DoAllowRemoteFileTransfer(AFile, CopyParam, false))
  {
    LogEvent(FORMAT("File \"%s\" excluded from transfer", AFileName));
    throw ESkipFile();
  }

  if (CopyParam->SkipTransfer(AFileName, AFile->GetIsDirectory()))
  {
    OperationProgress->AddSkippedFileSize(AFile->GetSize());
    throw ESkipFile();
  }

  LogFileDetails(AFileName, AFile->GetModification(), AFile->GetSize());

  OperationProgress->SetFile(AFileName);

  UnicodeString OnlyFileName = base::UnixExtractFileName(AFileName);
  UnicodeString DestFileName = ChangeFileName(CopyParam, OnlyFileName, osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = ATargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    Action.Cancel();
    if (CanRecurseToDirectory(AFile))
    {
      FileOperationLoopCustom(this, OperationProgress, Flags,
        FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName), "",
      [&]()
      {
        int Attrs = ::FileGetAttrFix(ApiPath(DestFullName));
        if (FLAGCLEAR(Attrs, faDirectory))
        {
          ThrowExtException();
        }
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(NOT_DIRECTORY_ERROR, (DestFullName)));

      FileOperationLoopCustom(this, OperationProgress, Flags,
        FMTLOAD(CREATE_DIR_ERROR, DestFullName), "",
      [&]()
      {
        THROWOSIFFALSE(::SysUtulsForceDirectories(ApiPath(DestFullName)));
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_DIR_ERROR, (DestFullName)));

      if (FLAGCLEAR(AParams, cpNoRecurse))
      {
        TSinkFileParams SinkFileParams;
        SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
        SinkFileParams.CopyParam = CopyParam;
        SinkFileParams.Params = AParams;
        SinkFileParams.OperationProgress = OperationProgress;
        SinkFileParams.Skipped = false;
        SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

        ProcessDirectory(AFileName, nb::bind(&TTerminal::SinkFile, this), &SinkFileParams);

        FFileSystem->DirectorySunk(DestFullName, AFile, CopyParam);

        // Do not delete directory if some of its files were skip.
        // Throw "skip file" for the directory to avoid attempt to deletion
        // of any parent directory
        if (FLAGSET(AParams, cpDelete) && SinkFileParams.Skipped)
        {
          throw ESkipFile();
        }
      }
    }
    else
    {
      LogEvent(FORMAT("Skipping symlink to directory \"%s\".", AFileName));
    }
  }
  else
  {
    LogEvent(FORMAT("Copying \"%s\" to local directory started.", AFileName));

    // Will we use ASCII of BINARY file transfer?
    UnicodeString BaseFileName = GetBaseFileName(AFileName);
    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->Size;
    MaskParams.Modification = AFile->Modification;
    SelectTransferMode(BaseFileName, osRemote, CopyParam, MaskParams);

    // Suppose same data size to transfer as to write
    // (not true with ASCII transfer)
    int64_t TransferSize = AFile->Size;
    OperationProgress->SetLocalSize(TransferSize);
    if (IsFileEncrypted(AFileName))
    {
      TransferSize += TEncryption::GetOverhead();
    }
    OperationProgress->SetTransferSize(TransferSize);

    intptr_t Attrs = 0;
    FileOperationLoopCustom(this, OperationProgress, Flags,
      FMTLOAD(NOT_FILE_ERROR, DestFullName), "",
    [&]()
    {
      Attrs = ::FileGetAttrFix(ApiPath(DestFullName));
      if ((Attrs >= 0) && FLAGSET(Attrs, faDirectory))
      {
        ThrowExtException();
      }
    });
   __removed FILE_OPERATION_LOOP_END(FMTLOAD(NOT_FILE_ERROR, (DestFullName)));

    FFileSystem->Sink(
      AFileName, AFile, ATargetDir, DestFileName, Attrs, CopyParam, AParams, OperationProgress, Flags, Action);

    LogFileDone(OperationProgress, ::ExpandUNCFileName(DestFullName));
    OperationProgress->Succeeded();
  }
}
//---------------------------------------------------------------------------
void TTerminal::UpdateTargetAttrs(
  const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType *CopyParam, uintptr_t Attrs)
{
  if (Attrs == -1)
  {
    Attrs = faArchive;
  }
  uintptr_t NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
  if ((NewAttrs & Attrs) != NewAttrs)
  {
    FileOperationLoopCustom(this, FOperationProgress, folAllowSkip,
      FMTLOAD(CANT_SET_ATTRS, ADestFullName), "",
    [&]()
    {
      THROWOSIFFALSE(::SysUtulsFileSetAttr(ApiPath(ADestFullName), (DWORD)(Attrs | NewAttrs)) == 0);
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (ADestFullName)));
  }
}
//---------------------------------------------------------------------------
void TTerminal::UpdateTargetTime(HANDLE Handle, TDateTime Modification, TDSTMode DSTMode)
{
  LogEvent(FORMAT("Preserving timestamp [%s]", ::StandardTimestamp(Modification)));
  FILETIME WrTime = DateTimeToFileTime(Modification, DSTMode);
  if (!::SetFileTime(Handle, nullptr, nullptr, &WrTime))
  {
    int Error = GetLastError();
    LogEvent(FORMAT("Preserving timestamp failed, ignoring: %s", ::SysErrorMessageForError(Error)));
  }
}
//---------------------------------------------------------------------------
void TTerminal::SinkFile(const UnicodeString AFileName, const TRemoteFile *AFile, void *AParam)
{
  TSinkFileParams * Params = get_as<TSinkFileParams>(AParam);
  DebugAssert(Params->OperationProgress != nullptr);
  try
  {
    SinkRobust(AFileName, AFile, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch (ESkipFile &E)
  {
    Params->Skipped = true;

    {
      volatile TSuspendFileOperationProgress Suspend(Params->OperationProgress);
      if (!HandleException(&E))
      {
        throw;
      }
    }

    if (Params->OperationProgress->GetCancel())
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReflectSettings() const
{
  DebugAssert(FLog != nullptr);
  FLog->ReflectSettings();
  DebugAssert(FActionLog != nullptr);
  FActionLog->ReflectSettings();
  // also FTunnelLog ?
}
//---------------------------------------------------------------------------
void TTerminal::CollectUsage()
{
  switch (GetSessionData()->GetFSProtocol())
  {
  case fsSCPonly:
//      Configuration->Usage->Inc(L"OpenedSessionsSCP");
    break;

  case fsSFTP:
  case fsSFTPonly:
//      Configuration->Usage->Inc(L"OpenedSessionsSFTP");
    break;

  case fsFTP:
    if (GetSessionData()->GetFtps() == ftpsNone)
    {
//        Configuration->Usage->Inc(L"OpenedSessionsFTP");
    }
    else
    {
//        Configuration->Usage->Inc(L"OpenedSessionsFTPS");
    }
    break;

  case fsWebDAV:
    if (GetSessionData()->GetFtps() == ftpsNone)
    {
//        Configuration->Usage->Inc(L"OpenedSessionsWebDAV");
    }
    else
    {
//        Configuration->Usage->Inc(L"OpenedSessionsWebDAVS");
    }
    break;

  case fsS3:
    // Configuration->Usage->Inc(L"OpenedSessionsS3");
    break;
  }

  if (GetConfiguration()->GetLogging() && GetConfiguration()->GetLogToFile())
  {
//    Configuration->Usage->Inc(L"OpenedSessionsLogToFile2");
  }

  if (GetConfiguration()->GetLogActions())
  {
//    Configuration->Usage->Inc(L"OpenedSessionsXmlLog");
  }

  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(L""));
  if (!GetSessionData()->IsSame(FactoryDefaults.get(), true))
  {
//    Configuration->Usage->Inc(L"OpenedSessionsAdvanced");
  }

  if (GetSessionData()->GetProxyMethod() != ::pmNone)
  {
//    Configuration->Usage->Inc(L"OpenedSessionsProxy");
  }
  if (GetSessionData()->GetFtpProxyLogonType() > 0)
  {
//    Configuration->Usage->Inc(L"OpenedSessionsFtpProxy");
  }
  if (IsEncryptingFiles())
  {
    Configuration->Usage->Inc(L"OpenedSessionsEncrypted");
  }

  FCollectFileSystemUsage = true;
}

bool TTerminal::CheckForEsc() const
{
  if (FOnCheckForEsc)
  {
    return FOnCheckForEsc();
  }
  return (FOperationProgress && FOperationProgress->GetCancel() == csCancel);
}
//---------------------------------------------------------------------------
static UnicodeString FormatCertificateData(const UnicodeString Fingerprint, intptr_t Failures)
{
  return FORMAT("%s;%2.2X", Fingerprint, Failures);
}
//---------------------------------------------------------------------------
bool TTerminal::VerifyCertificate(
  const UnicodeString CertificateStorageKey, const UnicodeString SiteKey,
  const UnicodeString Fingerprint,
  const UnicodeString CertificateSubject, int Failures)
{
  bool Result = false;

  UnicodeString CertificateData = FormatCertificateData(Fingerprint, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smRead);

  if (Storage->OpenSubKey(CertificateStorageKey, false))
  {
    if (Storage->ValueExists(SiteKey))
    {
      UnicodeString CachedCertificateData = Storage->ReadString(SiteKey, L"");
      if (CertificateData == CachedCertificateData)
      {
        LogEvent(FORMAT("Certificate for \"%s\" matches cached fingerprint and failures", CertificateSubject));
        Result = true;
      }
    }
    else if (Storage->ValueExists(Fingerprint))
    {
      LogEvent(FORMAT("Certificate for \"%s\" matches legacy cached fingerprint", CertificateSubject));
      Result = true;
    }
  }

  if (!Result)
  {
    UnicodeString Buf = GetSessionData()->GetHostKey();
    while (!Result && !Buf.IsEmpty())
    {
      UnicodeString ExpectedKey = CutToChar(Buf, L';', false);
      if (ExpectedKey == L"*")
      {
        UnicodeString Message = LoadStr(ANY_CERTIFICATE);
        Information(Message, true);
        GetLog()->Add(llException, Message);
        Result = true;
      }
      else if (ExpectedKey == Fingerprint)
      {
        LogEvent(FORMAT("Certificate for \"%s\" matches configured fingerprint", CertificateSubject));
        Result = true;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::ConfirmCertificate(
  TSessionInfo &SessionInfo, intptr_t Failures, const UnicodeString CertificateStorageKey, bool CanRemember)
{
  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text = SessionInfo.CertificateFingerprint;

  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
  Aliases[0].ActionAlias = LoadStr(COPY_CERTIFICATE_ACTION);
  Aliases[0].OnSubmit = nb::bind(&TClipboardHandler::Copy, &ClipboardHandler);

  TQueryParams Params(qpWaitInBatch);
  Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
  Params.NoBatchAnswers = qaYes | qaRetry;
  Params.Aliases = Aliases;
  Params.AliasesCount = _countof(Aliases);
  uint32_t Answer =
    QueryUser(
      FMTLOAD(VERIFY_CERT_PROMPT3, SessionInfo.Certificate),
      nullptr, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);

  bool Result;
  switch (Answer)
  {
  case qaYes:
    CacheCertificate(
      CertificateStorageKey, GetSessionData()->GetSiteKey(), SessionInfo.CertificateFingerprint, Failures);
    Result = true;
    break;

  case qaNo:
    Result = true;
    break;

  case qaCancel:
    // Configuration->Usage->Inc(L"HostNotVerified");
    Result = false;
    break;

  default:
    DebugFail();
    Result = false;
    break;
  }

  // Cache only if the certificate was accepted manually
  if (Result && CanRemember)
  {
    GetConfiguration()->RememberLastFingerprint(
      GetSessionData()->GetSiteKey(), TlsFingerprintType, SessionInfo.CertificateFingerprint);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::CacheCertificate(const UnicodeString CertificateStorageKey,
  const UnicodeString SiteKey, const UnicodeString Fingerprint, intptr_t Failures)
{
  UnicodeString CertificateData = FormatCertificateData(Fingerprint, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(CertificateStorageKey, true))
  {
    Storage->WriteString(SiteKey, CertificateData);
  }
}
//---------------------------------------------------------------------------
void TTerminal::CollectTlsUsage(const UnicodeString /*TlsVersionStr*/)
{
  // see SSL_get_version() in OpenSSL ssl_lib.c
#if 0
  if (TlsVersionStr == L"TLSv1.2")
  {
    Configuration->Usage->Inc(L"OpenedSessionsTLS12");
  }
  else if (TlsVersionStr == L"TLSv1.1")
  {
    Configuration->Usage->Inc(L"OpenedSessionsTLS11");
  }
  else if (TlsVersionStr == L"TLSv1")
  {
    Configuration->Usage->Inc(L"OpenedSessionsTLS10");
  }
  else if (TlsVersionStr == L"SSLv3")
  {
    Configuration->Usage->Inc(L"OpenedSessionsSSL30");
  }
  else if (TlsVersionStr == L"SSLv2")
  {
    Configuration->Usage->Inc(L"OpenedSessionsSSL20");
  }
  else
  {
    DebugFail();
  }
#endif // #if 0
}
//---------------------------------------------------------------------------
bool TTerminal::LoadTlsCertificate(X509 *&Certificate, EVP_PKEY *&PrivateKey)
{
  bool Result = !GetSessionData()->GetTlsCertificateFile().IsEmpty();
  if (Result)
  {
    UnicodeString Passphrase = GetSessionData()->GetPassphrase();

    // Inspired by neon's ne_ssl_clicert_read
    bool Retry;

    do
    {
      Retry = false;

      bool WrongPassphrase = false;
      ParseCertificate(GetSessionData()->GetTlsCertificateFile(), Passphrase, Certificate, PrivateKey, WrongPassphrase);
      if (WrongPassphrase)
      {
        if (Passphrase.IsEmpty())
        {
          LogEvent("Certificate is encrypted, need passphrase");
          Information(LoadStr(CLIENT_CERTIFICATE_LOADING), false);
        }
        else
        {
          Information(LoadStr(CERTIFICATE_DECODE_ERROR_INFO), false);
        }

        Passphrase = L"";
        if (PromptUser(
              GetSessionData(), pkPassphrase,
              LoadStr(CERTIFICATE_PASSPHRASE_TITLE), L"",
              LoadStr(CERTIFICATE_PASSPHRASE_PROMPT), false, 0, Passphrase))
        {
          Retry = true;
        }
        else
        {
          Result = false;
        }
      }
    }
    while (Retry);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetBaseFileName(const UnicodeString AFileName) const
{
  UnicodeString FileName = AFileName;

  if (FSessionData->GetTrimVMSVersions())
  {
    intptr_t P = FileName.LastDelimiter(L";");
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
  }
  return FileName;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::ChangeFileName(const TCopyParamType *CopyParam,
  const UnicodeString AFileName, TOperationSide Side, bool FirstLevel) const
{
  UnicodeString FileName = GetBaseFileName(AFileName);
  if (CopyParam)
  {
    FileName = CopyParam->ChangeFileName(FileName, Side, FirstLevel);
  }
  return FileName;
}
//---------------------------------------------------------------------------
bool TTerminal::CanRecurseToDirectory(const TRemoteFile *AFile) const
{
  return !AFile->GetIsSymLink() || FSessionData->GetFollowDirectorySymlinks();
}
//---------------------------------------------------------------------------
typename TTerminal::TEncryptedFileNames::const_iterator TTerminal::GetEncryptedFileName(const UnicodeString APath)
{
  UnicodeString FileDir = base::UnixExtractFileDir(APath);

  // If we haven't been in this folder yet, read it to collect mapping to encrypted file names.
  if (FFoldersScannedForEncryptedFiles.find(FileDir) == FFoldersScannedForEncryptedFiles.end())
  {
    try
    {
      delete DoReadDirectoryListing(FileDir, true);
    }
    catch (Exception & /*E*/)
    {
      if (!Active)
      {
        throw;
      }
    }

    FFoldersScannedForEncryptedFiles.insert(FileDir);
  }

  TEncryptedFileNames::const_iterator Result = FEncryptedFileNames.find(APath);
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::IsFileEncrypted(const UnicodeString APath, bool EncryptNewFiles)
{
  // can be optimized
  bool Result = (EncryptFileName(APath, EncryptNewFiles) != APath);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::EncryptFileName(const UnicodeString APath, bool EncryptNewFiles)
{
  UnicodeString Result = APath;
  if (IsEncryptingFiles() && !base::IsUnixRootPath(APath))
  {
    UnicodeString FileName = base::UnixExtractFileName(APath);
    UnicodeString FileDir = base::UnixExtractFileDir(APath);

    if (!FileName.IsEmpty() && IsRealFile(FileName))
    {
      TEncryptedFileNames::const_iterator I = GetEncryptedFileName(APath);
      if (I != FEncryptedFileNames.end())
      {
        FileName = I->second;
      }
      else if (EncryptNewFiles)
      {
        TEncryption Encryption(FEncryptKey);
        FileName = Encryption.EncryptFileName(FileName);
        FEncryptedFileNames[APath] = FileName;
      }
    }

    FileDir = EncryptFileName(FileDir, EncryptNewFiles);
    Result = base::UnixCombinePaths(FileDir, FileName);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::DecryptFileName(const UnicodeString Path)
{
  UnicodeString Result = Path;
  if (IsEncryptingFiles() && !base::IsUnixRootPath(Path))
  {
    UnicodeString FileName = base::UnixExtractFileName(Path);
    UnicodeString FileNameEncrypted = FileName;

    bool Encrypted = TEncryption::IsEncryptedFileName(FileName);
    if (Encrypted)
    {
      TEncryption Encryption(FEncryptKey);
      FileName = Encryption.DecryptFileName(FileName);

      UnicodeString FileDir = base::UnixExtractFileDir(Path);
      FileDir = DecryptFileName(FileDir);
      Result = base::UnixCombinePaths(FileDir, FileName);
    }

    if (Encrypted || (FEncryptedFileNames.find(Result) == FEncryptedFileNames.end()))
    {
      // This may overwrite another variant of encryption
      FEncryptedFileNames[Result] = FileNameEncrypted;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSecondaryTerminal::TSecondaryTerminal(TTerminal *MainTerminal) noexcept :
  TTerminal(OBJECT_CLASS_TSecondaryTerminal),
  FMainTerminal(MainTerminal)
{
}

TSecondaryTerminal::TSecondaryTerminal(TObjectClassId Kind, TTerminal *MainTerminal) noexcept :
  TTerminal(Kind),
  FMainTerminal(MainTerminal)
{
}

void TSecondaryTerminal::Init(
  TSessionData *ASessionData, TConfiguration *AConfiguration, const UnicodeString AName)
{
  TTerminal::Init(ASessionData, AConfiguration);
  DebugAssert(FMainTerminal != nullptr);
  GetLog()->SetParent(FMainTerminal->GetLog(), AName);
  GetActionLog()->SetEnabled(false);
  GetSessionData()->NonPersistant();
  if (!FMainTerminal->TerminalGetUserName().IsEmpty())
  {
    GetSessionData()->SessionSetUserName(FMainTerminal->TerminalGetUserName());
  }
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::UpdateFromMain()
{
  if ((FFileSystem != nullptr) && (FMainTerminal->FFileSystem != nullptr))
  {
    FFileSystem->UpdateFromMain(FMainTerminal->FFileSystem);
  }
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  DebugAssert(FileList != nullptr);
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryModified(const UnicodeString APath,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(APath, SubDirs);
}
//---------------------------------------------------------------------------
TTerminal * TSecondaryTerminal::GetPasswordSource()
{
  return FMainTerminal;
}
//---------------------------------------------------------------------------
TTerminalList::TTerminalList(TConfiguration *AConfiguration) noexcept :
  TObjectList(OBJECT_CLASS_TTerminalList),
  FConfiguration(AConfiguration)
{
  DebugAssert(FConfiguration);
}
//---------------------------------------------------------------------------
TTerminalList::~TTerminalList() noexcept
{
  DebugAssert(GetCount() == 0);
}
//---------------------------------------------------------------------------
TTerminal * TTerminalList::CreateTerminal(TSessionData *Data)
{
  TTerminal *Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}
//---------------------------------------------------------------------------
TTerminal * TTerminalList::NewTerminal(TSessionData *Data)
{
  TTerminal *Result = CreateTerminal(Data);
  Add(Result);
  return Result;
}
//---------------------------------------------------------------------------
void TTerminalList::FreeTerminal(TTerminal *Terminal)
{
  DebugAssert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}
//---------------------------------------------------------------------------
void TTerminalList::FreeAndNullTerminal(TTerminal *& Terminal)
{
  TTerminal *T = Terminal;
  Terminal = nullptr;
  FreeTerminal(T);
}
//---------------------------------------------------------------------------
TTerminal * TTerminalList::GetTerminal(intptr_t Index)
{
  return GetAs<TTerminal>(Index);
}
//---------------------------------------------------------------------------
void TTerminalList::RecryptPasswords()
{
  for (intptr_t Index = 0; Index < GetCount(); Index++)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TLocalFileHandle::TLocalFileHandle() :
  Handle(0),
  Attrs(0),
  MTime(0),
  ATime(0),
  Size(0),
  Directory(false)
{
}
//---------------------------------------------------------------------------
TLocalFileHandle::~TLocalFileHandle()
{
  Release();
}
//---------------------------------------------------------------------------
void TLocalFileHandle::Release()
{
  if (Handle != 0)
  {
    Close();
  }
}
//---------------------------------------------------------------------------
void TLocalFileHandle::Dismiss()
{
  if (DebugAlwaysTrue(Handle != 0))
  {
    Handle = 0;
  }
}
//---------------------------------------------------------------------------
void TLocalFileHandle::Close()
{
  __removed if (DebugAlwaysTrue(Handle != 0))
  {
    SAFE_CLOSE_HANDLE(Handle);
    Dismiss();
  }
}
//---------------------------------------------------------------------------
UnicodeString GetSessionUrl(const TTerminal *Terminal, bool WithUserName)
{
  UnicodeString Result;
  const TSessionInfo &SessionInfo = Terminal->GetSessionInfo();
  const TSessionData *SessionData = Terminal->GetSessionData();
  UnicodeString Protocol = SessionInfo.ProtocolBaseName;
  UnicodeString HostName = SessionData->GetHostNameExpanded();
  UnicodeString UserName = SessionData->GetUserNameExpanded();
  intptr_t Port = Terminal->GetSessionData()->GetPortNumber();
  if (WithUserName && !UserName.IsEmpty())
  {
    Result = FORMAT("%s://%s:@%s:%d", Protocol.Lower(), UserName, HostName, Port);
  }
  else
  {
    Result = FORMAT("%s://%s:%d", Protocol.Lower(), HostName, Port);
  }
  return Result;
}

