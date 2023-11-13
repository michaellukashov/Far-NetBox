
#include <vcl.h>
#pragma hdrstop

#include "Terminal.h"

#include <Common.h>
#include <Sysutils.hpp>
#include <System.IOUtils.hpp>
#include <StrUtils.hpp>

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
#include "NeonIntf.h"
#include <PuttyTools.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>
#include <algorithm>

#ifndef AUTO_WINSOCK
#include <WinSock2.h>
#endif

__removed #pragma package(smart_init)

__removed #define FILE_OPERATION_LOOP_TERMINAL this


class TLoopDetector : public TObject
{
public:
  TLoopDetector() noexcept;
  void RecordVisitedDirectory(const UnicodeString & Directory);
  bool IsUnvisitedDirectory(const UnicodeString & Directory);

private:
  std::unique_ptr<TStringList> FVisitedDirectories;
};

TLoopDetector::TLoopDetector() noexcept
{
  FVisitedDirectories.reset(CreateSortedStringList());
}

void TLoopDetector::RecordVisitedDirectory(const UnicodeString & ADirectory)
{
  UnicodeString VisitedDirectory = ::ExcludeTrailingBackslash(ADirectory);
  FVisitedDirectories->Add(VisitedDirectory);
}

bool TLoopDetector::IsUnvisitedDirectory(const UnicodeString & Directory)
{
  bool Result = (FVisitedDirectories->IndexOf(Directory) < 0);

  if (Result)
  {
    RecordVisitedDirectory(Directory);
  }

  return Result;
}


NB_DEFINE_CLASS_ID(TMoveFileParams);
struct TMoveFileParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TMoveFileParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TMoveFileParams) || TObject::is(Kind); }
public:
  TMoveFileParams() : TObject(OBJECT_CLASS_TMoveFileParams) {}
  UnicodeString Target;
  UnicodeString FileMask;
  bool DontOverwrite{false};
};

NB_DEFINE_CLASS_ID(TFilesFindParams);
struct TFilesFindParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFilesFindParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFilesFindParams) || TObject::is(Kind); }
public:
  TFilesFindParams() : TObject(OBJECT_CLASS_TFilesFindParams) {}

  TFileMasks FileMask;
  TFileFoundEvent OnFileFound;
  TFindingFileEvent OnFindingFile;
  bool Cancel{false};
  TLoopDetector LoopDetector;
  UnicodeString RealDirectory;
};
 
TCalculateSizeStats::TCalculateSizeStats() noexcept
{
__removed memset(this, 0, sizeof(*this));
}

TCalculateSizeParams::TCalculateSizeParams() noexcept : TObject(OBJECT_CLASS_TCalculateSizeParams)
{
  __removed memset(this, 0, sizeof(*this));
  Result = true;
  AllowDirs = true;
}

TSynchronizeOptions::TSynchronizeOptions() noexcept
{
  __removed memset(this, 0, sizeof(*this));
}

TSynchronizeOptions::~TSynchronizeOptions() noexcept
{
  SAFE_DESTROY(Filter);
}

bool TSynchronizeOptions::MatchesFilter(const UnicodeString & AFileName) const
{
  bool Result = true;
  if (Filter)
  {
    int32_t FoundIndex = 0;
    Result = Filter->Find(AFileName, FoundIndex);
  }
  return Result;
}

TSpaceAvailable::TSpaceAvailable()
{
__removed memset(this, 0, sizeof(*this));
}

#if 0
TOverwriteFileParams::TOverwriteFileParams()
{
  SourceSize = 0;
  DestSize = 0;
  SourcePrecision = mfFull;
  DestPrecision = mfFull;
}
#endif // if 0


NB_DEFINE_CLASS_ID(TTunnelThread);
class TTunnelThread : public TSimpleThread
{
  NB_DISABLE_COPY(TTunnelThread)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTunnelThread); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTunnelThread) || TSimpleThread::is(Kind); }
public:
  TTunnelThread() = delete;
  explicit TTunnelThread(TSecureShell *SecureShell) noexcept;
  virtual ~TTunnelThread() noexcept;
  virtual void InitTunnelThread();

  virtual void Terminate() override;

protected:
  virtual void Execute() override;

private:
  TSecureShell *FSecureShell{nullptr};
  bool FTerminated{false};
};

TTunnelThread::TTunnelThread(TSecureShell * SecureShell) noexcept :
  TSimpleThread(OBJECT_CLASS_TTunnelThread),
  FSecureShell(SecureShell)
{
}

void TTunnelThread::InitTunnelThread()
{
  TSimpleThread::InitSimpleThread();
  Start();
}

TTunnelThread::~TTunnelThread() noexcept
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}

void TTunnelThread::Terminate()
{
  FTerminated = true;
}

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


NB_DEFINE_CLASS_ID(TTunnelUI);
class TTunnelUI : public TSessionUI
{
  NB_DISABLE_COPY(TTunnelUI)
public:
  TTunnelUI() = delete;
  explicit TTunnelUI(TTerminal *Terminal) noexcept;
  virtual ~TTunnelUI() = default;

  virtual void Information(const UnicodeString & AStr, bool Status) override;
  virtual uint32_t QueryUser(const UnicodeString & AQuery,
    TStrings * MoreMessages, uint32_t Answers, const TQueryParams * Params,
    TQueryType QueryType) override;
  virtual uint32_t QueryUserException(const UnicodeString & AQuery,
    Exception * E, uint32_t Answers, const TQueryParams * Params,
    TQueryType QueryType) override;
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & AInstructions, TStrings *Prompts,
    TStrings *Results) override;
  virtual void DisplayBanner(const UnicodeString & Banner) override;
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpContext) override;
  virtual void HandleExtendedException(Exception *E) override;
  virtual void Closed() override;
  virtual void ProcessGUI() override;

private:
  TTerminal *FTerminal{nullptr};
  uint32_t FTerminalThreadID{0};
};

TTunnelUI::TTunnelUI(TTerminal *Terminal) noexcept :
  TSessionUI(OBJECT_CLASS_TTunnelUI),
  FTerminal(Terminal)
{
  FTerminalThreadID = GetCurrentThreadId();
}

void TTunnelUI::Information(const UnicodeString & AStr, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->Information(AStr, Status);
  }
}

uint32_t TTunnelUI::QueryUser(const UnicodeString & AQuery,
  TStrings * MoreMessages, uint32_t Answers, const TQueryParams * Params,
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

uint32_t TTunnelUI::QueryUserException(const UnicodeString & AQuery,
  Exception * E, uint32_t Answers, const TQueryParams * Params,
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

bool TTunnelUI::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & AInstructions, TStrings * Prompts, TStrings * Results)
{
  bool Result = false;
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    UnicodeString Instructions = AInstructions;
    if (IsAuthenticationPrompt(Kind))
    {
      Instructions =
        FMTLOAD(TUNNEL_INSTRUCTION2, Data->GetHostName()) +
        (AInstructions.IsEmpty() ? "" : "\n") +
        AInstructions;
    }

    Result = FTerminal->PromptUser(Data, Kind, AName, Instructions, Prompts, Results);
  }
  return Result;
}

void TTunnelUI::DisplayBanner(const UnicodeString & Banner)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->DisplayBanner(Banner);
  }
}

void TTunnelUI::FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpContext)
{
  throw ESshFatal(E, Msg, HelpContext);
}

void TTunnelUI::HandleExtendedException(Exception *E)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->HandleExtendedException(E);
  }
}

void TTunnelUI::Closed()
{
  // noop
}

void TTunnelUI::ProcessGUI()
{
  // noop
}


class TCallbackGuard : public TObject
{
  NB_DISABLE_COPY(TCallbackGuard)
public:
  explicit TCallbackGuard(TTerminal * ATerminal) noexcept;
  virtual ~TCallbackGuard() noexcept;

  void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword);
  void Verify();
  bool Verify(Exception * E);
  void Dismiss();

private:
  ExtException *FFatalError{nullptr};
  TTerminal *FTerminal{nullptr};
  bool FGuarding{false};
};

TCallbackGuard::TCallbackGuard(TTerminal * ATerminal) noexcept :
  FTerminal(ATerminal),
  FFatalError(nullptr),
  FGuarding(FTerminal->FCallbackGuard == nullptr)
{
  if (FGuarding)
  {
    FTerminal->FCallbackGuard = this;
  }
}

TCallbackGuard::~TCallbackGuard() noexcept
{
  if (FGuarding)
  {
    DebugAssert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == nullptr));
    FTerminal->FCallbackGuard = nullptr;
  }

  SAFE_DESTROY_EX(Exception, FFatalError);
}

void TCallbackGuard::FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword)
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

void TCallbackGuard::Dismiss()
{
  DebugAssert(FFatalError == nullptr);
  FGuarding = false;
}

void TCallbackGuard::Verify()
{
  if (FGuarding)
  {
    FGuarding = false;
    DebugAssert(FTerminal->FCallbackGuard == this);
    FTerminal->FCallbackGuard = nullptr;

    if (FFatalError != nullptr)
    {
      throw ESshFatal(FFatalError, "");
    }
  }
}

bool TCallbackGuard::Verify(Exception * E)
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


TRobustOperationLoop::TRobustOperationLoop(
  TTerminal * Terminal, TFileOperationProgressType * OperationProgress, bool * AnyTransfer, bool CanRetry) noexcept :
  FTerminal(Terminal),
  FOperationProgress(OperationProgress),
  FRetry(false),
  FAnyTransfer(AnyTransfer),
  FCanRetry(CanRetry)
{
  if (FAnyTransfer != nullptr)
  {
    FPrevAnyTransfer = *FAnyTransfer;
    *FAnyTransfer = false;
    FStart = Now();
  }
}

TRobustOperationLoop::~TRobustOperationLoop() noexcept
{
  if (FAnyTransfer != nullptr)
  {
    *FAnyTransfer = FPrevAnyTransfer;
  }
}

bool TRobustOperationLoop::TryReopen(Exception & E)
{
  if (!FCanRetry)
  {
    FRetry = false;
  }
  else if (dyn_cast<ESkipFile>(&E) != nullptr)
  {
    FRetry = false;
  }
  else if (FTerminal->Active)
  {
    FTerminal->LogEvent(1, L"Session is open, will not retry transfer");
    FRetry = false;
  }
  else
  {
    FRetry = true;
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
        if (!FRetry)
        {
          FTerminal->LogEvent(L"Retry interval expired, will not retry transfer");
        }
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

bool TRobustOperationLoop::ShouldRetry() const
{
  return FRetry;
}

bool TRobustOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  return Result;
}

class TRetryOperationLoop
{
public:
  explicit TRetryOperationLoop(TTerminal * Terminal) noexcept;

  void Error(Exception & E);
  void Error(Exception & E, TSessionAction & Action);
  void Error(Exception & E, const UnicodeString & Message);
  void Error(Exception & E, TSessionAction & Action, const UnicodeString & Message);
  bool Retry();
  bool Succeeded() const;

private:
  TTerminal * FTerminal{nullptr};
  bool FRetry{false};
  bool FSucceeded{false};

  void DoError(Exception & E, TSessionAction * Action, const UnicodeString & Message);
};

TRetryOperationLoop::TRetryOperationLoop(TTerminal *Terminal) noexcept
{
  FTerminal = Terminal;
  FRetry = false;
  FSucceeded = true;
}

void TRetryOperationLoop::DoError(Exception & E, TSessionAction * Action, const UnicodeString & Message)
{
  FSucceeded = false;
  // Note that the action may already be canceled when RollbackAction is called
  uint32_t Result;
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

void TRetryOperationLoop::Error(Exception & E)
{
  DoError(E, nullptr, UnicodeString());
}

void TRetryOperationLoop::Error(Exception & E, TSessionAction & Action)
{
  DoError(E, &Action, UnicodeString());
}

void TRetryOperationLoop::Error(Exception & E, const UnicodeString & Message)
{
  DoError(E, nullptr, Message);
}

void TRetryOperationLoop::Error(Exception & E, TSessionAction & Action, const UnicodeString & Message)
{
  DoError(E, &Action, Message);
}

bool TRetryOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  if (Result)
  {
    FSucceeded = true;
  }
  return Result;
}

bool TRetryOperationLoop::Succeeded() const
{
  return FSucceeded;
}

TCollectedFileList::TCollectedFileList() noexcept :
  TObject(OBJECT_CLASS_TCollectedFileList)
{
}

TCollectedFileList::~TCollectedFileList()
{
  for (size_t Index = 0; Index < FList.size(); Index++)
  {
    Deleting(Index);
  }
}

void TCollectedFileList::Deleting(int32_t Index)
{
  delete FList[Index].Object;
}

int32_t TCollectedFileList::Add(const UnicodeString & FileName, TObject * Object, bool Dir)
{
  TFileData Data;
  Data.FileName = FileName;
  Data.Object = Object;
  Data.Dir = Dir;
  Data.Recursed = true;
  Data.State = 0;
  FList.push_back(Data);
  return GetCount() - 1;
}

void TCollectedFileList::DidNotRecurse(int32_t Index)
{
  FList[Index].Recursed = false;
}

void TCollectedFileList::Delete(int32_t Index)
{
  Deleting(Index);
  FList.erase(FList.begin() + Index);
}

int32_t TCollectedFileList::GetCount() const
{
  return FList.size();
}

UnicodeString TCollectedFileList::GetFileName(int32_t Index) const
{
  return FList[Index].FileName;
}

TObject * TCollectedFileList::GetObj(int32_t Index) const
{
  return FList[Index].Object;
}

bool TCollectedFileList::IsDir(int32_t Index) const
{
  return FList[Index].Dir;
}

bool TCollectedFileList::IsRecursed(int32_t Index) const
{
  return FList[Index].Recursed;
}

int32_t TCollectedFileList::GetState(int32_t Index) const
{
  return FList[Index].State;
}

void TCollectedFileList::SetState(int32_t Index, int32_t State)
{
  FList[Index].State = State;
}


TParallelOperation::TParallelOperation(TOperationSide Side) noexcept
{
  FCopyParam = nullptr;
  FParams = 0;
  FProbablyEmpty = false;
  FClients = 0;
  FMainOperationProgress = nullptr;
  DebugAssert((Side == osLocal) || (Side == osRemote));
  FSide = Side;
  FVersion = 0;
}

void TParallelOperation::Init(
  TStrings * AFileList, const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int32_t Params,
  TFileOperationProgressType * MainOperationProgress, const UnicodeString & MainName,
  int64_t ParallelFileSize)
{
  DebugAssert(FFileList == nullptr);
  // More lists should really happen in scripting only, which does not support parallel transfers atm.
  // But in general the code should work with more lists anyway, it just was not tested for it.
  DebugAssert(AFileList->GetCount() == 1);
  FFileList.reset(AFileList);
  FSection = std::make_unique<TCriticalSection>();
  FTargetDir = TargetDir;
  FCopyParam = CopyParam;
  FParams = Params;
  FMainOperationProgress = MainOperationProgress;
  FMainName = MainName;
  FListIndex = 0;
  FIndex = 0;
  FIsParallelFileTransfer = (ParallelFileSize >= 0);
  FParallelFileSize = ParallelFileSize;
  FParallelFileOffset = 0;
  FParallelFileCount = 0;
  FParallelFileMerging = false;
  FParallelFileMerged = 0;
}

TParallelOperation::~TParallelOperation() noexcept
{
  WaitFor();
}

bool TParallelOperation::IsInitialized() const
{
  return (FMainOperationProgress != nullptr);
}

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
    TGuard Guard(*FSection.get()); nb::used(Guard);
    Result = !FProbablyEmpty && (FMainOperationProgress->GetCancel() < csCancel);
  }
  return Result;
}

void TParallelOperation::AddClient()
{
  TGuard Guard(*FSection.get()); nb::used(Guard);
  FClients++;
}

void TParallelOperation::RemoveClient()
{
  TGuard Guard(*FSection.get()); nb::used(Guard);
  FClients--;
}

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
        TGuard Guard(*FSection.get()); nb::used(Guard);
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

void TParallelOperation::Done(
  const UnicodeString & FileName, bool Dir, bool Success, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, TTerminal * Terminal)
{
  if (Dir)
  {
    TGuard Guard(*FSection.get()); nb::used(Guard);

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
        if (FFileList->GetCount() > FListIndex)
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
          TCollectedFileList * Files = GetFileList(FListIndex);
          int32_t Index = 0;
          while (Index < Files->GetCount())
          {
            if (StartsText(FileNameWithSlash, Files->GetFileName(Index)))
            {
              // We should add the file to "skip" counters in the OperationProgress,
              // but an interactive foreground transfer is not doing that either yet.
              Files->Delete(Index);
              // Force re-update
              FVersion++;
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
  else
  {
    if (IsParallelFileTransfer)
    {
      TGuard Guard(*FSection.get());

      try__finally
      {
        if (Success && DebugAlwaysTrue(FSide == osRemote))
        {
          TParallelFileOffsets::const_iterator I = std::find(FParallelFileOffsets.begin(), FParallelFileOffsets.end(), CopyParam->PartOffset);
          if (DebugAlwaysTrue(I != FParallelFileOffsets.end()))
          {
            int32_t Index = I - FParallelFileOffsets.begin();
            DebugAssert(!FParallelFileDones[Index]);
            FParallelFileDones[Index] = true;

            if (!FParallelFileMerging)
            {
              // Once we obtain "merging" semaphor, we won't leave until everything is merged
              TAutoFlag MergingFlag(FParallelFileMerging); nb::used(MergingFlag);

              try
              {
                UnicodeString TargetName = TPath::Combine(TargetDir, FParallelFileTargetName);
                UnicodeString TargetNamePartial = TargetName + PartialExt;
                UnicodeString TargetNamePartialOnly = base::UnixExtractFileName(TargetNamePartial);

                while (true)
                {
                  if ((FParallelFileMerged >= FParallelFileCount) || !FParallelFileDones[FParallelFileMerged])
                  {
                    break;
                  }
                  else
                  {
                    TUnguard Unguard(*FSection.get()); nb::used(Unguard);

                    UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
                    // Safe as write access to FParallelFileMerged is guarded by FParallelFileMerging
                    int32_t Index = FParallelFileMerged;
                    UnicodeString TargetPartName = GetPartPrefix(TPath::Combine(TargetDir, FileNameOnly)) + IntToStr(Index);

                    if ((CopyParam->PartSize >= 0) && (Terminal->OperationProgress->TransferredSize != CopyParam->PartSize))
                    {
                      UnicodeString TransferredSizeStr = IntToStr(Terminal->OperationProgress->TransferredSize);
                      UnicodeString PartSizeStr = IntToStr(CopyParam->PartSize);
                      UnicodeString Message =
                        FMTLOAD(INCONSISTENT_SIZE, TargetPartName, TransferredSizeStr, PartSizeStr);
                      Terminal->TerminalError(nullptr, Message);
                    }

                    if (Index == 0)
                    {
                      Terminal->LogEvent(FORMAT(L"Renaming part %d of \"%s\" to \"%s\"...", Index, FileNameOnly, TargetNamePartialOnly));
                      Terminal->DoRenameLocalFileForce(TargetPartName, TargetNamePartial);
                    }
                    else
                    {
                      {
                        Terminal->LogEvent(FORMAT(L"Appending part %d of \"%s\" to \"%s\"...", Index, FileNameOnly, TargetNamePartialOnly));
                        std::unique_ptr<THandleStream> SourceStream(TSafeHandleStream::CreateFromFile(TargetPartName, fmOpenRead | fmShareDenyWrite));
                        std::unique_ptr<THandleStream> DestStream(TSafeHandleStream::CreateFromFile(TargetNamePartial, fmOpenWrite | fmShareDenyWrite));
                        HANDLE DestHandle = reinterpret_cast<HANDLE>(DestStream->Handle());
                        FILETIME WrTime;
                        bool GotWrTime = GetFileTime(DestHandle, nullptr, nullptr, &WrTime);
                        DestStream->Seek(0L, TSeekOrigin::soEnd);
                        DestStream->CopyFrom(SourceStream.get(), SourceStream->Size);
                        if (GotWrTime)
                        {
                          SetFileTime(DestHandle, nullptr, nullptr, &WrTime);
                        }
                      }

                      Terminal->DoDeleteLocalFile(TargetPartName);
                    }

                    FParallelFileMerged++;
                  }
                }

                if ((FParallelFileMerged == FParallelFileCount) && (FParallelFileOffset == FParallelFileSize))
                {
                  Terminal->LogEvent(FORMAT(L"Renaming completed \"%s\" to \"%s\"...", TargetNamePartialOnly, FParallelFileTargetName));
                  Terminal->DoRenameLocalFileForce(TargetNamePartial, TargetName);
                }
              }
              catch (...)
              {
                Success = false;
                throw;
              }
            }
          }
        }
      },
      __finally
      {
        if (!Success)
        {
          // If any part fails, cancel whole transfer.
          // We should get here only after the user sees/acknowledges the failure.
          FMainOperationProgress->SetCancel(csCancel);
        }
      } end_try__finally
    }
  }
}

bool TParallelOperation::CheckEnd(TCollectedFileList *Files)
{
  bool Result = (FIndex >= Files->GetCount());
  if (Result)
  {
    FListIndex++;
    FIndex = 0;
  }
  return Result;
}

bool TParallelOperation::GetOnlyFile(TStrings * FileList, UnicodeString & FileName, TObject *& Object)
{
  bool Result = (FileList->Count == 1);
  if (Result)
  {
    TCollectedFileList * OnlyFileList = GetFileList(FileList, 0);
    Result = (OnlyFileList->GetCount() == 1) && !OnlyFileList->IsDir(0);
    if (Result)
    {
      FileName = OnlyFileList->GetFileName(0);
      Object = OnlyFileList->GetObj(0);
    }
  }
  return Result;
}

TCollectedFileList * TParallelOperation::GetFileList(TStrings * FileList, int32_t Index)
{
  return DebugNotNull(dynamic_cast<TCollectedFileList *>(FileList->GetObj(Index)));
}

TCollectedFileList * TParallelOperation::GetFileList(int32_t Index)
{
  return GetFileList(FFileList.get(), Index);
}

UnicodeString TParallelOperation::GetPartPrefix(const UnicodeString & FileName)
{
  return FORMAT(L"%s%s.", FileName, PartialExt);
}

int32_t TParallelOperation::GetNext(
  TTerminal * Terminal, UnicodeString & FileName, TObject *& Object, UnicodeString & TargetDir, bool & Dir,
  bool & Recursed, TCopyParamType *& CustomCopyParam)
{
  TGuard Guard(*FSection.get()); nb::used(Guard);
  int32_t Result = 1;
  TCollectedFileList * Files;
  do
  {
    if (FFileList->GetCount() > FListIndex)
    {
      Files = GetFileList(FListIndex);
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
    UnicodeString RootPath = FFileList->GetString(FListIndex);

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
      const TDirectoryData & DirectoryData = DirectoryIterator->second;
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
      UnicodeString OnlyFileName;
      if (Dir || IsParallelFileTransfer) // optimization
      {
        if (FSide == osLocal)
        {
          OnlyFileName = base::ExtractFileName(FileName, false);
        }
        else
        {
          OnlyFileName = base::UnixExtractFileName(FileName);
        }
        OnlyFileName = Terminal->ChangeFileName(FCopyParam, OnlyFileName, osRemote, FirstLevel);
      }

      if (Dir)
      {
        DebugAssert(!OnlyFileName.IsEmpty());
        TDirectoryData DirectoryData;
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

      bool Processed = true;
      if (IsParallelFileTransfer)
      {
        CustomCopyParam = new TCopyParamType(*FCopyParam);
        CustomCopyParam->PartOffset = FParallelFileOffset;
        int64_t Remaining = FParallelFileSize - CustomCopyParam->PartOffset;
        CustomCopyParam->PartSize = FParallelFileSize / GetConfiguration()->QueueTransfersLimit();
        DebugAssert(!OnlyFileName.IsEmpty());
        if (FParallelFileTargetName.IsEmpty())
        {
          FParallelFileTargetName = OnlyFileName;
        }
        DebugAssert(FParallelFileTargetName == OnlyFileName);
        int32_t Index = FParallelFileCount;
        UnicodeString PartFileName = GetPartPrefix(OnlyFileName) + IntToStr(Index);
        FParallelFileCount++;
        FParallelFileOffsets.push_back(CustomCopyParam->PartOffset);
        FParallelFileDones.push_back(false);
        DebugAssert(FParallelFileOffsets.size() == static_cast<size_t>(FParallelFileCount));
        CustomCopyParam->FileMask = DelimitFileNameMask(PartFileName);
        if ((CustomCopyParam->PartSize >= Remaining) ||
            (Remaining - CustomCopyParam->PartSize < CustomCopyParam->PartSize / 10))
        {
          CustomCopyParam->PartSize = -1; // Until the end
          FParallelFileOffset = FParallelFileSize;
          Terminal->LogEvent(FORMAT(L"Starting transfer of \"%s\" part %d from %s until the EOF", OnlyFileName, Index, IntToStr(CustomCopyParam->PartOffset)));
        }
        else
        {
          Processed = false;
          FParallelFileOffset += CustomCopyParam->PartSize;
          Terminal->LogEvent(FORMAT(L"Starting transfer of \"%s\" part %d from %s, length %s", OnlyFileName, Index, IntToStr(CustomCopyParam->PartOffset), IntToStr(CustomCopyParam->PartSize)));
        }
      }

      if (Processed)
      {
        // The current implementation of UpdateFileList relies on this specific way of changing state
        // (all files before FIndex one-by-one)
        Files->SetState(FIndex, qfsProcessed);
        FIndex++;
        CheckEnd(Files);
      }
    }
  }

  FProbablyEmpty = (FFileList->GetCount() == FListIndex);

  return Result;
}

bool TParallelOperation::UpdateFileList(TQueueFileList * UpdateFileList)
{
  TGuard Guard(*FSection.get());

  bool Result =
    (UpdateFileList->FLastParallelOperation != this) ||
    (UpdateFileList->FLastParallelOperationVersion != FVersion);

  DebugAssert(FFileList->Count == 1);
  TCollectedFileList * Files = GetFileList(0);

  if (!Result && (UpdateFileList->GetCount() != Files->GetCount()))
  {
    DebugAssert(false);
    Result = true;
  }

  if (Result)
  {
    UpdateFileList->Clear();
    for (int32_t Index = 0; Index < Files->GetCount(); Index++)
    {
      UpdateFileList->Add(Files->GetFileName(Index), Files->GetState(Index));
    }
  }
  else
  {
    int32_t Index = ((FListIndex == 0) ? FIndex : Files->GetCount()) - 1;
    while ((Index >= 0) && (UpdateFileList->GetState(Index) != Files->GetState(Index)))
    {
      UpdateFileList->SetState(Index, Files->GetState(Index));
      Index--;
      Result = true;
    }
  }

  UpdateFileList->FLastParallelOperation = this;
  UpdateFileList->FLastParallelOperationVersion = FVersion;

  return Result;
}

TTerminal::TTerminal(TObjectClassId Kind) noexcept :
  TSessionUI(Kind),
  FSessionData(std::make_unique<TSessionData>("")),
  FFSProtocol(cfsUnknown),
  FStatus(ssClosed)
{
}

void TTerminal::Init(TSessionData * ASessionData, TConfiguration * AConfiguration, TActionLog * AActionLog)
{
  FConfiguration = AConfiguration;
  //FSessionData = new TSessionData(L"");
  FSessionData->Assign(ASessionData);
  TDateTime Started = Now(); // use the same time for session and XML log
  FLog = std::make_unique<TSessionLog>(this, Started, FSessionData.get(), FConfiguration);
  if (AActionLog != nullptr)
  {
    FActionLog.reset(AActionLog);
    FActionLogOwned = false;
  }
  else
  {
    FActionLog = std::make_unique<TActionLog>(this, Started, FSessionData.get(), FConfiguration);
    FActionLogOwned = true;
  }
  FFiles = std::make_unique<TRemoteDirectory>(this);
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
  FOnDisplayBanner = nullptr;
  FOnShowExtendedException = nullptr;
  FOperationProgressPersistence = nullptr;
  FOperationProgressOnceDoneOperation = odoIdle;
  FOnInformation = nullptr;
  FOnCustomCommand = nullptr;
  FOnClose = nullptr;
  FOnFindingFile = nullptr;
  FOperationProgressPersistence = nullptr;
  FOperationProgressOnceDoneOperation = odoIdle;

  FUseBusyCursor = True;
  FDirectoryCache = std::make_unique<TRemoteDirectoryCache>();
//  FDirectoryChangesCache = nullptr;
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
  FSecondaryTerminals = 0;
  FCollectFileSystemUsage = false;
  FSuspendTransaction = false;
  FOperationProgress = nullptr;
  FClosedOnCompletion = nullptr;
}

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
      (FDirectoryChangesCache.get() != nullptr))
  {
    FConfiguration->SaveDirectoryChangesCache(GetSessionData()->GetSessionKey(),
      FDirectoryChangesCache.get());
  }

//  SAFE_DESTROY_EX(TCustomFileSystem, FFileSystem);
//  SAFE_DESTROY_EX(TSessionLog, FLog);
  if (FActionLogOwned)
  {
    TActionLog * ActionLog = FActionLog.release();
    SAFE_DESTROY_EX(TActionLog, ActionLog);
  }
  else
  {
    FActionLog.release();
  }
//  SAFE_DESTROY_EX(TActionLog, FActionLog);
//  SAFE_DESTROY(FFiles);
//  SAFE_DESTROY_EX(TRemoteDirectoryCache, FDirectoryCache);
//  SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
//  SAFE_DESTROY(FSessionData);
//  SAFE_DESTROY(FOldFiles);
}

void TTerminal::Idle()
{
  // Once we disconnect, do nothing, until reconnect handler
  // "receives the information".
  // Never go idle when called from within ::ProcessGUI() call
  // as we may recurse for good, timeouting eventually.
  if (GetActive() && (FNesting == 0))
  {
    TAutoNestingCounter NestingCounter(FNesting); nb::used(NestingCounter);

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

RawByteString TTerminal::EncryptPassword(const UnicodeString & APassword) const
{
  return FConfiguration->EncryptPassword(APassword, GetSessionData()->GetSessionPasswordEncryptionKey());
}

UnicodeString TTerminal::DecryptPassword(const RawByteString & APassword) const
{
  UnicodeString Result;
  try
  {
    Result = FConfiguration->DecryptPassword(APassword, GetSessionData()->GetSessionPasswordEncryptionKey());
  }
  catch (EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}

void TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FRememberedPassword = EncryptPassword(GetRememberedPassword());
  FRememberedTunnelPassword = EncryptPassword(GetRememberedTunnelPassword());
}

UnicodeString TTerminal::ExpandFileName(const UnicodeString & APath,
  const UnicodeString & BasePath)
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

bool TTerminal::GetActive() const
{
  return (this != nullptr) && (FFileSystem != nullptr) && FFileSystem->GetActive();
}

void TTerminal::Close()
{
  FFileSystem->Close();

  // Cannot rely on CommandSessionOpened here as Status is set to ssClosed too late
  if ((FCommandSession != nullptr) && FCommandSession->GetActive())
  {
    // prevent recursion
    FCommandSession->SetOnClose(nullptr);
    FCommandSession->Close();
  }
}

void TTerminal::ResetConnection()
{
  // used to be called from Reopen(), why?
  FTunnelError.Clear();

  FRememberedPasswordTried = false;
  FRememberedTunnelPasswordTried = false;

  if (FDirectoryChangesCache.get() != nullptr)
  {
//    SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
    FDirectoryChangesCache.reset();
  }

  FFiles->SetDirectory("");
  // note that we cannot clear contained files
  // as they can still be referenced in the GUI atm
}

void TTerminal::FingerprintScan(UnicodeString & SHA256, UnicodeString & SHA1, UnicodeString & MD5)
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
    if (!FFingerprintScannedSHA256.IsEmpty() ||
        !FFingerprintScannedSHA1.IsEmpty() ||
        !FFingerprintScannedMD5.IsEmpty())
    {
      SHA256 = FFingerprintScannedSHA256;
      SHA1 = FFingerprintScannedSHA1;
      MD5 = FFingerprintScannedMD5;
    }
    else
    {
      throw;
    }
  }
}

void TTerminal::Open()
{
  AnySession = true;
  GetConfiguration()->Usage->Inc(L"SessionOpens");
  TAutoNestingCounter OpeningCounter(FOpening);
  ReflectSettings();
  try
  {
    FEncryptKey = HexToBytes(FSessionData->EncryptKey);
    if (IsEncryptingFiles())
    {
      ValidateEncryptKey(FEncryptKey);
    }

    DoInformation("", true, 1);
    try__finally
    {
      FRememberedPasswordUsed = false;
      InternalTryOpen();
    },
    __finally
    {
      // This does not make it through, if terminal thread is abandonded,
      // see also TTerminalManager::DoConnectTerminal
      DoInformation("", true, 0);
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
    FatalError(&E, "");
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
      DebugAssert(FDirectoryChangesCache.get() == nullptr);
      FDirectoryChangesCache = std::make_unique<TRemoteDirectoryChangesCache>(
        FConfiguration->GetCacheDirectoryChangesMaxSize());
      if (GetSessionData()->GetPreserveDirectoryChanges())
      {
        FConfiguration->LoadDirectoryChangesCache(GetSessionData()->GetSessionKey(),
          FDirectoryChangesCache.get());
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
//      SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
      FDirectoryChangesCache.reset();
    }
    if (GetSessionData()->GetFingerprintScan() && (FFileSystem != nullptr) &&
        DebugAlwaysTrue(GetSessionData()->GetFtps() != ftpsNone))
    {
      const TSessionInfo & SessionInfo = FFileSystem->GetSessionInfo();
      FFingerprintScannedSHA256 = SessionInfo.CertificateFingerprintSHA256;
      FFingerprintScannedSHA1 = SessionInfo.CertificateFingerprintSHA1;
      FFingerprintScannedMD5 = UnicodeString();
    }
    if (FRememberedPasswordUsed)
    {
      // Particularly to prevent reusing a wrong client certificate passphrase
      // in the next login attempt
      FRememberedPassword = UnicodeString();
      FRememberedPasswordKind = TPromptKind(-1);
      FRememberedTunnelPassword = UnicodeString();
    }
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
    FFileSystem = std::make_unique<TFTPFileSystem>(this);
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
#endif
  }
  else if (FSProtocol == fsWebDAV)
  {
    FFSProtocol = cfsWebDAV;
    FFileSystem = std::make_unique<TWebDAVFileSystem>(this);
    FFileSystem->Init(nullptr);
    FFileSystem->Open();
    GetLog()->AddSeparator();
    LogEvent("Using WebDAV protocol.");
  }
  else if (GetSessionData()->GetFSProtocol() == fsS3)
  {
    FFSProtocol = cfsS3;
    FFileSystem = std::make_unique<TS3FileSystem>(this);
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
      FSecureShell = std::make_unique<TSecureShell>(this, FSessionData.get(), GetLog(), FConfiguration);
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
          FFingerprintScannedSHA1 = UnicodeString();
        }
        // Tunnel errors that happens only once we connect to the local port (server-side forwarding problems).
        // Contrary to local side problems handled already in OpenTunnel.
        if (!FSecureShell->GetActive() && !FTunnelError.IsEmpty())
        {
          // the only case where we expect this to happen (first in GUI, the latter in scripting)
          DebugAssert((E.Message == LoadStr(UNEXPECTED_CLOSE_ERROR)) || (E.Message == LoadStr(NET_TRANSL_CONN_ABORTED)) || (E.Message.Pos(LoadStr(NOT_CONNECTED)) > 0));
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
        FFileSystem = std::make_unique<TSCPFileSystem>(this);
        FFileSystem->Init(FSecureShell.release());
        FSecureShell = nullptr; // ownership passed
        LogEvent("Using SCP protocol.");
      }
      else
      {
        FFSProtocol = cfsSFTP;
        FFileSystem = std::make_unique<TSFTPFileSystem>(this);
        FFileSystem->Init(FSecureShell.release());
        FSecureShell = nullptr; // ownership passed
        LogEvent("Using SFTP protocol.");
      }
    },
    __finally
    {
      FSecureShell = nullptr;
    } end_try__finally
  }
}

bool TTerminal::IsListenerFree(uint32_t PortNumber) const
{
  SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
  bool Result = (Socket != INVALID_SOCKET);
  if (Result)
  {
    SOCKADDR_IN Address{};

    nb::ClearStruct(Address);
    Address.sin_family = AF_INET;
    Address.sin_port = htons(static_cast<short>(PortNumber));
    Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Result = (bind(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) == 0);
    closesocket(Socket);
  }
  return Result;
}

void TTerminal::SetupTunnelLocalPortNumber()
{
  DebugAssert(FTunnelData == nullptr);

  FTunnelLocalPortNumber = FSessionData->GetTunnelLocalPortNumber();
  if (FTunnelLocalPortNumber == 0)
  {
    // Randomizing the port selection to reduce a chance of conflicts.
    rde::vector<int32_t> Ports;
    for (int32_t Port = GetConfiguration()->FTunnelLocalPortNumberLow; Port <= GetConfiguration()->FTunnelLocalPortNumberHigh; Port++)
    {
      Ports.push_back(Port);
    }

    do
    {
      if (Ports.empty())
      {
        FatalError(nullptr, FMTLOAD(TUNNEL_NO_FREE_PORT,
          FConfiguration->GetTunnelLocalPortNumberLow(), FConfiguration->GetTunnelLocalPortNumberHigh()));
      }
      int32_t Index = Random(Ports.size());
      int32_t Port = Ports[Index];
      Ports.erase(&Ports.at(Index));
      if (IsListenerFree(Port))
      {
        FTunnelLocalPortNumber = Port;
        LogEvent(FORMAT(L"Autoselected tunnel local port number %d", FTunnelLocalPortNumber));
      }
    }
    while (FTunnelLocalPortNumber == 0);
  }
}

void TTerminal::OpenTunnel()
{
  SetupTunnelLocalPortNumber();

  try
  {
    FTunnelData.reset(FSessionData->CreateTunnelData(FTunnelLocalPortNumber));

    // The Started argument is not used with Parent being set
    FTunnelLog = std::make_unique<TSessionLog>(this, TDateTime(), FTunnelData.get(), FConfiguration);
    FTunnelLog->SetParent(FLog.get(), "Tunnel");
    FTunnelLog->ReflectSettings();
    FTunnelUI = std::make_unique<TTunnelUI>(this);
    FTunnel = std::make_unique<TSecureShell>(FTunnelUI.get(), FTunnelData.get(), FTunnelLog.get(), FConfiguration);

    FTunnelOpening = true;
    try__finally
    {
      FTunnel->Open();
    },
    __finally
    {
      FTunnelOpening = false;
    } end_try__finally

    // When forwarding fails due to a local problem (e.g. port in use), we get the error already here, so abort asap.
    // If we won't abort, particularly in case of "port in use", we may not even detect the problem,
    // as the connection to the port may succeed.
    // Remote-side tunnel problems are handled in TTerminal::Open().
    if (!FTunnel->FLastTunnelError.IsEmpty())
    {
      // Throw and handle with any other theoretical forwarding errors that may cause tunnel connection close
      // (probably cannot happen)
      EXCEPTION;
    }

    FTunnelThread = std::make_unique<TTunnelThread>(FTunnel.get());
    FTunnelThread->InitTunnelThread();
  }
  catch (Exception & E)
  {
    LogEvent(L"Error opening tunnel.");
    CloseTunnel();
    if (!FTunnelError.IsEmpty())
    {
      FatalError(&E, FMTLOAD(TUNNEL_ERROR, FTunnelError));
    }
    else
    {
      throw;
    }
  }
}

void TTerminal::CloseTunnel()
{
  FTunnelThread.reset();
  FTunnelError = FTunnel->GetLastTunnelError();
  FTunnel.reset();
  FTunnelUI.reset();
  FTunnelLog.reset();
  FTunnelData.reset();

  FTunnelLocalPortNumber = 0;
}

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
    catch (Exception & E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }

  FStatus = ssClosed;
}

void TTerminal::ProcessGUI()
{
  // Do not process GUI here, as we are called directly from a GUI loop and may
  // recurse for good.
  // Alternatively we may check for (FOperationProgress == nullptr)
  if (FNesting == 0)
  {
    TAutoNestingCounter NestingCounter(FNesting); nb::used(NestingCounter);
    ::ProcessGUI();
  }
}

void TTerminal::Progress(TFileOperationProgressType * OperationProgress)
{
  if (FNesting == 0)
  {
    TAutoNestingCounter NestingCounter(FNesting); nb::used(NestingCounter);
    OperationProgress->Progress();
  }
}

void TTerminal::Reopen(int32_t Params)
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
  int32_t PrevExceptionOnFail = FExceptionOnFail;
  bool WasInTransaction = InTransaction();
  try__finally
  {
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
    // Not sure why we are suspeding the transaction in the first place,
    // but definitelly when set while connecting auto loaded workspace session, it causes loading the directory twice.
    // (when reconnecting lost connection, it's usually prevented by cached directory)
    // Preventing that by suspeding transaction only when there is one.
    FSuspendTransaction = (FInTransaction > 0);
    /*if (InTransaction())
      EndTransaction();
    FSuspendTransaction = true;*/
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

bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & Instructions, const UnicodeString & Prompt, bool Echo, int32_t MaxLen, UnicodeString &AResult)
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

bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results)
{
  // If PromptUser is overridden in descendant class, the overridden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  // Actually no longer needed as we do not override DoPromptUser
  // anymore in TSecondaryTerminal.
  return DoPromptUser(Data, Kind, AName, Instructions, Prompts, Results);
}

TTerminal * TTerminal::GetPrimaryTerminal()
{
  return this;
}

bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & AInstructions, TStrings * Prompts, TStrings * Results)
{
  bool Result = false;

  bool PasswordOrPassphrasePrompt = ::IsPasswordOrPassphrasePrompt(Kind, Prompts);
  if (PasswordOrPassphrasePrompt)
  {
    bool & PasswordTried =
      FTunnelOpening ? FRememberedTunnelPasswordTried : FRememberedPasswordTried;
    if (!PasswordTried)
    {
      // let's expect that the main session is already authenticated and its password
      // is not written after, so no locking is necessary
      // (no longer true, once the main session can be reconnected)
      UnicodeString Password;
      if (FTunnelOpening)
      {
        Password = PrimaryTerminal->GetRememberedTunnelPassword();
      }
      else
      {
        Password = PrimaryTerminal->GetRememberedPassword();
      }
      FRememberedPasswordUsed = true;
      Results->SetString(0, Password);
      if (!Results->GetString(0).IsEmpty())
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
      Prompts->SetObj(0, ToObj(nb::ToIntPtr(Prompts->GetObj(0)) | pupRemember));
    }

    if (GetOnPromptUser() != nullptr)
    {
      TCallbackGuard Guard(this);
      try
      {
        GetOnPromptUser()(this, Kind, AName, AInstructions, Prompts, Results, Result, nullptr);
        Guard.Verify();
      }
      catch (Exception & E)
      {
        if (!Guard.Verify(&E))
        {
          throw;
        }
      }
    }

    if (Result && PasswordOrPassphrasePrompt &&
        (GetConfiguration()->GetRememberPassword() || FLAGSET(nb::ToIntPtr(Prompts->GetObj(0)), pupRemember)))
    {
      RawByteString EncryptedPassword = EncryptPassword(Results->GetString(0));
      if (FTunnelOpening)
      {
        PrimaryTerminal->SetRememberedTunnelPassword(EncryptedPassword);
      }
      else
      {
        PrimaryTerminal->SetRememberedPassword(EncryptedPassword);
        PrimaryTerminal->FRememberedPasswordKind = Kind;
      }
    }
  }

  return Result;
}

uint32_t TTerminal::QueryUser(const UnicodeString & AQuery,
  TStrings * MoreMessages, uint32_t Answers, const TQueryParams * AParams,
  TQueryType QueryType)
{
  LogEvent(FORMAT("Asking user:\n%s (%s)", AQuery, UnicodeString(MoreMessages ? MoreMessages->GetCommaText() : "")));
  uint32_t Answer = AbortAnswer(Answers);
  if (FOnQueryUser)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnQueryUser(this, AQuery, MoreMessages, Answers, AParams, Answer, QueryType, nullptr);
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
  UnicodeString Name;
  if (Answer == qaNeverAskAgain)
  {
    Name = L"NeverAskAgain";
  }
  else
  {
    UnicodeString Caption;
    AnswerNameAndCaption(Answer, Name, Caption);
  }
  LogEvent(FORMAT(L"Answer: %s", Name));
  return Answer;
}

uint32_t TTerminal::QueryUserException(const UnicodeString & AQuery,
  Exception * E, uint32_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uint32_t Result = 0;
  UnicodeString ExMessage;
  if (DebugAlwaysTrue(ExceptionMessageFormatted(E, ExMessage) || !AQuery.IsEmpty()))
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
      TStrings * MoreMessagesPtr = MoreMessages.release();
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

void TTerminal::DisplayBanner(const UnicodeString & ABanner)
{
  if (GetOnDisplayBanner() != nullptr)
  {
    uint32_t OrigParams, Params = 0;
    if (GetConfiguration()->GetForceBanners() ||
        GetConfiguration()->ShowBanner(GetSessionData()->GetSessionKey(), ABanner, Params))
    {
      bool NeverShowAgain = false;
      int32_t Options =
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

void TTerminal::HandleExtendedException(Exception * E)
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

void TTerminal::ShowExtendedException(Exception *E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    GetOnShowExtendedException()(this, E, nullptr);
  }
}

void TTerminal::DoInformation(
  const UnicodeString & AStr, bool Status, int32_t Phase, const UnicodeString & Additional)
{
  if (GetOnInformation())
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnInformation()(this, AStr, Status, Phase, Additional);
      Guard.Verify();
    }
    catch (Exception & E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}

void TTerminal::Information(const UnicodeString & AStr, bool Status)
{
  DoInformation(AStr, Status);
}

void TTerminal::DoProgress(TFileOperationProgressType & ProgressData)
{
  if ((FConfiguration->GetActualLogProtocol() >= 1) &&
      ProgressData.IsTransfer())
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

void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString & AFileName, bool Success, TOnceDoneOperation & OnceDoneOperation)
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

void TTerminal::SaveCapabilities(TFileSystemInfo &FileSystemInfo)
{
  for (int32_t Index = 0; Index < fcCount; ++Index)
  {
    FileSystemInfo.IsCapable[Index] = GetIsCapable(static_cast<TFSCapability>(Index));
  }
}

bool TTerminal::GetIsCapableProtected(TFSCapability Capability) const
{
  if (DebugAlwaysTrue(FFileSystem != nullptr))
  {
    switch (Capability)
    {
      case fcBackgroundTransfers:
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

UnicodeString TTerminal::GetAbsolutePath(const UnicodeString & APath, bool Local) const
{
  return FFileSystem->GetAbsolutePath(APath, Local);
}

void TTerminal::ReactOnCommand(int32_t ACmd)
{
  bool ChangesDirectory = false;
  bool ModifiesFiles = false;

  switch (static_cast<TFSCommand>(ACmd)) {
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

void TTerminal::TerminalError(const UnicodeString & Msg)
{
  TerminalError(nullptr, Msg);
}

void TTerminal::TerminalError(
  Exception * E, const UnicodeString & AMsg, const UnicodeString & AHelpKeyword)
{
  throw ETerminal(E, AMsg, AHelpKeyword);
}

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
    int32_t NumberOfRetries = FSessionData->GetNumberOfRetries();
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
      Params.TimeoutResponse = Params.TimeoutAnswer;
      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);
      Aliases[0].Default = true;
      Params.Aliases = Aliases;
      Params.AliasesCount = _countof(Aliases);
      Result = (QueryUserException("", E, qaRetry | qaAbort, &Params, qtError) == qaRetry);
    }

    if (Fatal != nullptr)
    {
      Fatal->SetReopenQueried(true);
    }
  }
  return Result;
}

bool TTerminal::ContinueReopen(TDateTime Start) const
{
  return
    (FConfiguration->GetSessionReopenTimeout() == 0) ||
    (int32_t(double(Now() - Start) * MSecsPerDay) < FConfiguration->GetSessionReopenTimeout());
}

bool TTerminal::QueryReopen(Exception * E, int32_t AParams,
  TFileOperationProgressType * OperationProgress)
{
  TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);

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
      catch (Exception & E2)
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

bool TTerminal::FileOperationLoopQuery(Exception & E,
  TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
  uint32_t AFlags, const UnicodeString & SpecialRetry, const UnicodeString & HelpKeyword)
{
  bool Result{false};
  GetLog()->AddException(&E);
  uint32_t Answer{0};
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
    TQueryButtonAlias Aliases[2]{};
    int32_t AliasCount = 0;

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
      TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);
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

void TTerminal::FileOperationLoopEnd(Exception & E,
  TFileOperationProgressType *OperationProgress, const UnicodeString & AMessage,
  uint32_t AFlags, const UnicodeString & ASpecialRetry, const UnicodeString & AHelpKeyword)
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

int32_t TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType * OperationProgress, uint32_t AFlags,
  const UnicodeString & AMessage, void * Param1, void * Param2)
{
  DebugAssert(CallBackFunc);
  int32_t Result = 0;
  FileOperationLoopCustom(this, OperationProgress, AFlags,
    AMessage, "",
  [&]()
  {
    Result = CallBackFunc(Param1, Param2);
  });
  __removed FILE_OPERATION_LOOP_END_EX(Message, Flags);

  return Result;
}

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

void TTerminal::ClearCachedFileList(const UnicodeString & APath,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(APath, SubDirs);
}

void TTerminal::AddCachedFileList(TRemoteFileList * FileList)
{
  FDirectoryCache->AddFileList(FileList);
}

TRemoteFileList * TTerminal::DirectoryFileList(const UnicodeString & APath, TDateTime Timestamp, bool CanLoad)
{
  std::unique_ptr<TRemoteFileList> Result;
  if (base::UnixSamePath(FFiles->GetDirectory(), APath))
  {
    if (Timestamp < FFiles->GetTimestamp())
    {
      Result = std::make_unique<TRemoteFileList>();
      FFiles->DuplicateTo(Result.get());
    }
  }
  else
  {
    if (FDirectoryCache->HasNewerFileList(APath, Timestamp))
    {
      Result = std::make_unique<TRemoteFileList>();
      DebugAlwaysTrue(FDirectoryCache->GetFileList(APath, Result.get()));
    }
    // do not attempt to load file list if there is cached version,
    // only absence of cached version indicates that we consider
    // the directory content obsolete
    else if (CanLoad && !FDirectoryCache->HasFileList(APath))
    {
      Result = std::make_unique<TRemoteFileList>();
      Result->SetDirectory(APath);

      try
      {
        ReadDirectory(Result.get());
      }
      catch (...)
      {
//        SAFE_DESTROY(Result);
        throw;
      }
    }
  }

  return Result.release();
}

void TTerminal::TerminalSetCurrentDirectory(const UnicodeString & AValue)
{
  DebugAssert(FFileSystem);
  if (AValue != FFileSystem->RemoteCurrentDirectory)
  {
    RemoteChangeDirectory(AValue);
  }
}

UnicodeString TTerminal::RemoteGetCurrentDirectory()
{
  if (FFileSystem != nullptr)
  {
    // there's occasional crash when assigning FFileSystem->CurrentDirectory
    // to FCurrentDirectory, splitting the assignment to two statements
    // to locate the crash more closely
    UnicodeString CurrentDirectory = FFileSystem->RemoteCurrentDirectory();
    FCurrentDirectory = CurrentDirectory;
    if (FCurrentDirectory.IsEmpty())
    {
      ReadCurrentDirectory();
    }
  }

  return FCurrentDirectory;
}

UnicodeString TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->RemoteCurrentDirectory();
  }

  return FCurrentDirectory;
}

TRemoteTokenList * TTerminal::GetGroups()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}

TRemoteTokenList * TTerminal::GetUsers()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}

TRemoteTokenList * TTerminal::GetMembership()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}

UnicodeString TTerminal::TerminalGetUserName() const
{
  // in future might also be implemented to detect username similar to GetUserGroups
  DebugAssert(FFileSystem != nullptr);
  UnicodeString Result = FFileSystem ? FFileSystem->RemoteGetUserName() : "";
  // Is empty also when stored username was used
  if (Result.IsEmpty())
  {
    Result = GetSessionData()->GetUserNameExpanded();
  }
  return Result;
}

bool TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == nullptr) || FDirectoryChangesCache->GetIsEmpty());
}

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
    catch (Exception & E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}

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
    catch (Exception & E)
    {
      if (!Guard.Verify(&E))
      {
        throw;
      }
    }
  }
}

void TTerminal::DoReadDirectoryProgress(int32_t Progress, int32_t ResolvedLinks, bool &Cancel)
{
  if (FReadingCurrentDirectory && (FOnReadDirectoryProgress != nullptr))
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnReadDirectoryProgress(this, Progress, ResolvedLinks, Cancel);
      Guard.Verify();
    }
    catch (Exception & E)
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
      FOnFindingFile(this, "", Cancel);
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

bool TTerminal::InTransaction() const
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}

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

void TTerminal::EndTransaction()
{
  DoEndTransaction(false);
}

void TTerminal::DoEndTransaction(bool Inform)
{
  if (FInTransaction == 0)
    TerminalError("Can't end transaction, not in transaction");
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
            DoInformation(LoadStr(STATUS_OPEN_DIRECTORY), true, -1, CurrentDirectory());
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

void TTerminal::SetExceptionOnFail(bool Value)
{
  if (Value)
  {
    FExceptionOnFail++;
  }
  else
  {
    if (FExceptionOnFail == 0)
      throw Exception("ExceptionOnFail is already zero.");
    FExceptionOnFail--;
  }

  if (FCommandSession != nullptr)
  {
    FCommandSession->FExceptionOnFail = FExceptionOnFail;
  }
}

bool TTerminal::GetExceptionOnFail() const
{
  return static_cast<bool>(FExceptionOnFail > 0);
}

void TTerminal::FatalAbort()
{
  FatalError(nullptr, "");
}

void TTerminal::FatalError(Exception * E, const UnicodeString & AMsg, const UnicodeString & AHelpKeyword)
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

void TTerminal::CommandError(Exception * E, const UnicodeString & AMsg)
{
  CommandError(E, AMsg, 0);
}

uint32_t TTerminal::CommandError(Exception * E, const UnicodeString & AMsg,
  uint32_t Answers, const UnicodeString & AHelpKeyword)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  DebugAssert(FCallbackGuard == nullptr);
  uint32_t Result = 0;
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

bool TTerminal::HandleException(Exception * E)
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

void TTerminal::CloseOnCompletion(
  TOnceDoneOperation Operation, const UnicodeString & AMessage,
  const UnicodeString & ATargetLocalPath, const UnicodeString & ADestLocalFileName)
{
  if (FOperationProgressPersistence != nullptr)
  {
    DebugAssert(AMessage.IsEmpty());
    FOperationProgressOnceDoneOperation = Operation;
  }
  else
  {
    Configuration->Usage()->Inc("ClosesOnCompletion");
    LogEvent(L"Closing session after completed operation (as requested by user)");
    Close();
    throw ESshTerminate(nullptr,
      MainInstructions(AMessage.IsEmpty() ? LoadStr(CLOSED_ON_COMPLETION) : AMessage),
      Operation, ATargetLocalPath, ADestLocalFileName);
  }
}

TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
  const UnicodeString & ASourceFullFileName, const TCopyParamType * CopyParam, int32_t Params, TFileOperationProgressType * OperationProgress, bool Special) const
{
  TBatchOverwrite Result;
  if (Special &&
      (FLAGSET(Params, cpResume) || CopyParam->ResumeTransfer(ASourceFullFileName)) &&
      (CopyParam->PartOffset < 0))
  {
    Result = boResume;
  }
  else if (FLAGSET(Params, cpAppend))
  {
    Result = boAppend;
  }
  else if (CopyParam->GetNewerOnly() &&
           (((OperationProgress->GetSide() == osLocal) && GetIsCapable(fcNewerOnlyUpload)) ||
            (OperationProgress->GetSide() != osLocal)) &&
           DebugAlwaysTrue(CopyParam->PartOffset < 0))
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

bool TTerminal::CheckRemoteFile(
  const UnicodeString & AFileName, const TCopyParamType * CopyParam, int32_t Params, TFileOperationProgressType *OperationProgress) const
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

uint32_t TTerminal::ConfirmFileOverwrite(
  const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
  const TOverwriteFileParams * FileParams, uint32_t Answers, TQueryParams * QueryParams,
  TOperationSide Side, const TCopyParamType * CopyParam, int32_t Params, TFileOperationProgressType * OperationProgress,
  const UnicodeString & AMessage)
{
  UnicodeString Message = AMessage;
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

    if (BatchOverwrite != boNo)
    {
      LogEvent(1, FORMAT(L"Batch operation mode [%d] is effective", int(BatchOverwrite)));
    }
    else
    {
      // particularly with parallel transfers, the overal operation can be already cancelled by other parallel operation
      if (OperationProgress->GetCancel() > csContinue)
      {
        LogEvent(1, L"Transfer cancelled in parallel operation");
        Result = qaCancel;
      }
      else
      {
        if (Message.IsEmpty())
        {
          // Side refers to destination side here
          Message = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE2 :
            REMOTE_FILE_OVERWRITE2), ATargetFileName, ATargetFileName);
        }
        if (FileParams != nullptr)
        {
          Message = FMTLOAD(FILE_OVERWRITE_DETAILS, Message,
            FormatSize(FileParams->SourceSize),
            base::UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision),
            FormatSize(FileParams->DestSize),
            base::UserModificationStr(FileParams->DestTimestamp, FileParams->DestPrecision));
        }
        if (DebugAlwaysTrue(QueryParams->HelpKeyword.IsEmpty()))
        {
          QueryParams->HelpKeyword = HELP_OVERWRITE;
        }
        Result = QueryUser(Message, nullptr, Answers, QueryParams);
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
        LogEvent(1, L"Overwritting all files");
        Result = qaYes;
        break;

      case boNone:
        LogEvent(1, L"Not overwritting any file");
        Result = qaNo;
        break;

      case boOlder:
        if (FileParams == nullptr)
        {
          LogEvent(1, L"Not overwritting due to lack of file information");
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
            (CompareFileTime(ReducedSourceTimestamp, ReducedDestTimestamp) > 0) ?
            qaYes : qaNo;

          LogEvent(FORMAT("Source file timestamp is [%s], destination timestamp is [%s], will%s overwrite",
            StandardTimestamp(ReducedSourceTimestamp),
            StandardTimestamp(ReducedDestTimestamp),
            UnicodeString(Result == qaYes ? "" : " not")));
        }
        break;

      case boAlternateResume:
        DebugAssert(CanAlternateResume);
        LogEvent(1, L"Alternate resume mode");
        Result = qaSkip; // ugh
        break;

      case boAppend:
        LogEvent(1, L"Appending file");
        Result = qaRetry;
        break;

      case boResume:
        LogEvent(1, L"Resuming file transfer");
        Result = qaRetry;
        break;
    }
  }

  return Result;
}

void TTerminal::FileModified(const TRemoteFile * AFile,
  const UnicodeString & AFileName, bool ClearDirectoryChange)
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

void TTerminal::DirectoryModified(const UnicodeString & APath, bool SubDirs)
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

void TTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
  AddCachedFileList(FileList);
}

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

void TTerminal::EnsureNonExistence(const UnicodeString & AFileName)
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

void TTerminal::LogEvent(const UnicodeString & AStr)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, AStr);
  }
}

void TTerminal::LogEvent(int32_t Level, const UnicodeString & AStr)
{
  if (Log->Logging && (Configuration->ActualLogProtocol >= Level))
  {
    Log->Add(llMessage, AStr);
  }
}

void TTerminal::RollbackAction(TSessionAction & Action,
  TFileOperationProgressType * OperationProgress, Exception * E)
{
  // ESkipFile without "cancel" is file skip,
  // and we do not want to record skipped actions.
  // But ESkipFile with "cancel" is abort and we want to record that.
  // Note that TScpFileSystem modifies the logic of RollbackAction little bit.
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

    if (!GetSessionData()->RemoteDirectory().IsEmpty())
    {
      if (SessionData->UpdateDirectories)
      {
        ExceptionOnFail = true;
        try
        {
          RemoteChangeDirectory(SessionData->RemoteDirectory());
        }
        catch (...)
        {
          if (!Active || SessionData->FRequireDirectories)
          {
            ExceptionOnFail = false;
            throw;
          }
          else
          {
            LogEvent(L"Configured initial remote directory cannot be opened, staying in the home directory.");
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
      // not to break the cache, if the next directory change would not
      // be initialized by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange.Clear();
    }

    if (OldDirectory != FFileSystem->RemoteCurrentDirectory()) DoChangeDirectory();
  }
  catch (Exception &E)
  {
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
}

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
        LoadedFromCache = FDirectoryCache->GetFileList(RemoteGetCurrentDirectory(), FFiles.get());
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
      std::unique_ptr<TRemoteDirectory> Files = std::make_unique<TRemoteDirectory>(this, FFiles.get());
      try__finally
      {
        Files->SetDirectory(RemoteGetCurrentDirectory());
        CustomReadDirectory(Files.get());
      },
      __finally
      {
        DoReadDirectoryProgress(-1, 0, Cancel);
        FReadingCurrentDirectory = false;
        std::unique_ptr<TRemoteDirectory> OldFiles(FFiles.release());
        FFiles.reset(Files.release());
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
          OldFiles->Reset();
        } end_try__finally
        if (GetActive())
        {
          if (GetSessionData()->GetCacheDirectories())
          {
            DirectoryLoaded(FFiles.get());
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

UnicodeString TTerminal::GetRemoteFileInfo(TRemoteFile * AFile) const
{
  return
    FORMAT("%s;%s;%lld;%s;%d;%s;%s;%s;%d",
       AFile->GetFileName(), AFile->GetType(), AFile->GetSize(), StandardTimestamp(AFile->GetModification()), int(AFile->GetModificationFmt()),
       AFile->GetFileOwner().GetLogText(), AFile->GetFileGroup().GetLogText(), AFile->GetRights()->GetText(),
       AFile->GetAttr());
}

void TTerminal::LogRemoteFile(TRemoteFile * AFile)
{
  // optimization
  if (GetLog()->GetLogging() && AFile)
  {
    LogEvent(GetRemoteFileInfo(AFile));
  }
}

UnicodeString TTerminal::FormatFileDetailsForLog(
  const UnicodeString & AFileName, TDateTime AModification, int64_t Size, const TRemoteFile * LinkedFile) const
{
  UnicodeString Result;
  // optimization
  if (GetLogConst()->GetLogging())
  {
    Result = FORMAT("'%s' [%s] [%s]", AFileName, UnicodeString(AModification != TDateTime() ? StandardTimestamp(AModification) : UnicodeString(L"n/a")), ::Int64ToStr(Size));
    if (LinkedFile != nullptr)
    {
      LinkedFile = LinkedFile->Resolve(); // in case it's symlink to symlink
      Result += FORMAT(L" - Link to: %s", FormatFileDetailsForLog(LinkedFile->FileName, LinkedFile->Modification, LinkedFile->Size, nullptr));
    }
  }
  return Result;
}

void TTerminal::LogFileDetails(const UnicodeString & AFileName, TDateTime AModification, int64_t Size, const TRemoteFile * LinkedFile)
{
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT("File: %s", FormatFileDetailsForLog(AFileName, AModification, Size, LinkedFile)));
  }
}

void TTerminal::LogFileDone(
  TFileOperationProgressType * OperationProgress, const UnicodeString & DestFileName,
  TTransferSessionAction & Action)
{
  if (FDestFileName.IsEmpty())
  {
    FDestFileName = DestFileName;
  }
  else
  {
    FMultipleDestinationFiles = true;
  }

  int64_t Size = OperationProgress->TransferredSize;
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT("Transfer done: '%s' => '%s' [%s]", OperationProgress->GetFullFileName(), DestFileName, Int64ToStr(Size)));
  }

  Action.Size(Size);
}

void TTerminal::CustomReadDirectory(TRemoteFileList * AFileList)
{
  DebugAssert(AFileList);
  DebugAssert(FFileSystem);

  // To match FTP upload/download, we also limit directory listing.
  // For simplicity, we limit it unconditionally, for all protocols for any kind of errors.
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

      FFileSystem->ReadDirectory(AFileList);
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


  if (Log->Logging && (Configuration->ActualLogProtocol >= 0))
  {
    for (int32_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      LogRemoteFile(AFileList->GetFile(Index));
    }
  }

  ReactOnCommand(fsListDirectory);
}

TRemoteFileList * TTerminal::ReadDirectoryListing(const UnicodeString & ADirectory, const TFileMasks & Mask)
{
  TRemoteFileList * FileList;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    FileList = nullptr;
    TLsSessionAction Action(GetActionLog(), GetAbsolutePath(ADirectory, true));

    try
    {
      FileList = DoReadDirectoryListing(ADirectory, false);
      if (FileList != nullptr)
      {
        int32_t Index = 0;
        while (Index < FileList->GetCount())
        {
          TRemoteFile * File = FileList->GetFile(Index);
          TFileMasks::TParams Params;
          Params.Size = File->Resolve()->GetSize();
          Params.Modification = File->GetModification();
          if (!Mask.MatchesFileName(File->GetFileName(), false, &Params))
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

TRemoteFile * TTerminal::ReadFileListing(const UnicodeString & APath)
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
      File = ReadFile(APath);
      Action.File(File);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return File;
}

TRemoteFileList * TTerminal::CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache)
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

TRemoteFileList * TTerminal::DoReadDirectoryListing(const UnicodeString & ADirectory, bool UseCache)
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

bool TTerminal::DeleteContentsIfDirectory(
  const UnicodeString & AFileName, const TRemoteFile * AFile, int32_t AParams, TRmSessionAction & Action)
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

void TTerminal::ProcessDirectory(const UnicodeString & ADirName,
  TProcessFileEvent CallBackFunc, void * AParam, bool UseCache, bool IgnoreErrors)
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

      for (int32_t Index = 0; Index < FileList->GetCount(); ++Index)
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

void TTerminal::ReadDirectory(TRemoteFileList * AFileList)
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

void TTerminal::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
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

TRemoteFile * TTerminal::ReadFile(const UnicodeString & AFileName)
{
  DebugAssert(FFileSystem);
  std::unique_ptr<TRemoteFile> File;
  try
  {
    LogEvent(FORMAT("Listing file \"%s\".", AFileName));
    TRemoteFile * AFile = nullptr;
    FFileSystem->ReadFile(AFileName, AFile);
    File.reset(AFile);
    ReactOnCommand(fsListFile);
    LogRemoteFile(File.get());
  }
  catch (Exception &E)
  {
    File.reset(nullptr);
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, AFileName));
  }
  return File.release();
}

TRemoteFile * TTerminal::TryReadFile(const UnicodeString & AFileName)
{
  TRemoteFile * File;
  try
  {
    SetExceptionOnFail(true);
    try__finally
    {
      File = ReadFile(base::UnixExcludeTrailingBackslash(AFileName));
    },
    __finally
    {
      SetExceptionOnFail(false);
    } end_try__finally
  }
  catch(...)
  {
    if (GetActive())
    {
      File = nullptr;
    }
    else
    {
      throw;
    }
  }
  return File;
}

bool TTerminal::FileExists(const UnicodeString & FileName)
{
  std::unique_ptr<TRemoteFile> File(TryReadFile(FileName));
  return (File.get() != nullptr);
}

void TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}

void TTerminal::OperationFinish(
  TFileOperationProgressType * Progress, const void * /*Item*/, const UnicodeString & AFileName,
  bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  Progress->Finish(AFileName, Success, OnceDoneOperation);
}

void TTerminal::OperationStart(
  TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int32_t Count)
{
  OperationStart(Progress, Operation, Side, Count, false, UnicodeString(), 0, odoIdle);
}

void TTerminal::OperationStart(
  TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int32_t Count,
  bool Temp, const UnicodeString & ADirectory, uint32_t CPSLimit, TOnceDoneOperation OnceDoneOperation)
{
  if (FOperationProgressPersistence != nullptr)
  {
    Progress.Restore(*FOperationProgressPersistence);
  }
  Progress.Start(Operation, Side, Count, Temp, ADirectory, CPSLimit, OnceDoneOperation);
  DebugAssert(FOperationProgress == nullptr);
  FOperationProgress = &Progress;
}

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

bool TTerminal::ProcessFiles(TStrings * AFileList,
  TFileOperation Operation, TProcessFileEvent ProcessFile, void * Param,
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
        int32_t Index = 0;
        while ((Index < AFileList->GetCount()) && (Progress.GetCancel() == csContinue))
        {
          UnicodeString FileName = AFileList->GetString(Index);
          TRemoteFile * File = AFileList->GetAs<TRemoteFile>(Index);
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
            TSuspendFileOperationProgress Suspend(GetOperationProgress()); nb::used(Suspend);
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

// not used anymore
#if 0
bool TTerminal::ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void * Param, TOperationSide Side)
{
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
}
#endif

TStrings * TTerminal::GetFixedPaths() const
{
  DebugAssert(FFileSystem != nullptr);
  return FFileSystem ? FFileSystem->GetFixedPaths() : nullptr;
}

bool  TTerminal::GetResolvingSymlinks() const
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}

TUsableCopyParamAttrs  TTerminal::UsableCopyParamAttrs(int32_t Params) const
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

bool TTerminal::IsRecycledFile(const UnicodeString & AFileName)
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

bool TTerminal::RecycleFile(const UnicodeString & AFileName, const TRemoteFile * AFile)
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

  bool Result;
  if (IsRecycledFile(FileName))
  {
    Result = true;
  }
  else
  {
    LogEvent(FORMAT("Moving file \"%s\" to remote recycle bin '%s'.",
      FileName, GetSessionData()->GetRecycleBinPath()));

    TMoveFileParams Params;
    Params.Target = GetSessionData()->GetRecycleBinPath();
#if defined(__BORLANDC__)
    Params.FileMask = FORMAT("*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()));
#else
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = Now();
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT("%04d%02d%02d-%02d%02d%02d", Y, M, D, H, N, S);
      // Params.FileMask = FORMAT("*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()));
      Params.FileMask = FORMAT("*-%s.*", dt);
    }
#endif
    Params.DontOverwrite = false;

    Result = DoMoveFile(FileName, AFile, &Params);

    if (Result && (GetOperationProgress() != nullptr) && (GetOperationProgress()->Operation() == foDelete))
    {
      GetOperationProgress()->Succeeded();
    }
  }

  return Result;
}

bool TTerminal::TryStartOperationWithFile(
  const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2)
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

void TTerminal::StartOperationWithFile(
  const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  if (!TryStartOperationWithFile(AFileName, Operation1, Operation2))
  {
    Abort();
  }
}

void TTerminal::RemoteDeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * AParams)
{
  UnicodeString FileName = AFileName;
  if (FileName.IsEmpty() && AFile)
  {
    FileName = AFile->GetFileName();
  }
  StartOperationWithFile(FileName, foDelete);
  int32_t Params = (AParams != nullptr) ? *(static_cast<int32_t *>(AParams)) : 0;
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
    DoDeleteFile(FFileSystem.get(), FileName, AFile, Params);
  }
}

void TTerminal::DoDeleteFile(
  TCustomFileSystem * FileSystem, const UnicodeString & AFileName, const TRemoteFile * AFile, int32_t Params)
{
  LogEvent(FORMAT(L"Deleting file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  TRetryOperationLoop RetryLoop(this);
  do
  {
    TRmSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true));
    try
    {
      DebugAssert(FileSystem != nullptr);
      // 'File' parameter: SFTPFileSystem needs to know if file is file or directory
      FileSystem->RemoteDeleteFile(AFileName, AFile, Params, Action);
      if ((OperationProgress != nullptr) && (OperationProgress->Operation() == foDelete))
      {
        OperationProgress->Succeeded();
      }
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(DELETE_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());

  // Forget if file was or was not encrypted and use user preferences, if we ever recreate it.
  FEncryptedFileNames.erase(GetAbsolutePath(AFileName, true));
  ReactOnCommand(fsDeleteFile);
}

bool TTerminal::RemoteDeleteFiles(TStrings * AFilesToDelete, int32_t Params)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor); nb::used(UseBusyCursorRestorer);
  FUseBusyCursor = false;

  TODO("avoid resolving symlinks while reading subdirectories.");
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return this->ProcessFiles(AFilesToDelete, foDelete, nb::bind(&TTerminal::RemoteDeleteFile, this), &Params);
}

void TTerminal::DeleteLocalFile(const UnicodeString & AFileName,
  const TRemoteFile * /*AFile*/, void * Params)
{
  StartOperationWithFile(AFileName, foDelete);
  int32_t Deleted;
  if (OnDeleteLocalFile == nullptr)
  {
    Deleted = RecursiveDeleteFileChecked(AFileName, false);
  }
  else
  {
    OnDeleteLocalFile(AFileName, FLAGSET(*((int32_t *)Params), dfAlternative), Deleted);
  }
  if (DebugAlwaysTrue((OperationProgress != nullptr) && (OperationProgress->Operation() == foDelete)))
  {
    OperationProgress->Succeeded(Deleted);
  }
}

bool TTerminal::DeleteLocalFiles(TStrings * AFileList, int32_t Params)
{
  return ProcessFiles(AFileList, foDelete, nb::bind(&TTerminal::DeleteLocalFile, this), &Params, osLocal);
}

void TTerminal::CustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * AParams)
{
  TCustomCommandParams * Params = cast_to<TCustomCommandParams>(AParams);
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

void TTerminal::PrepareCommandSession(bool NeedCurrentDirectory)
{
  DebugAssert(GetCommandSessionOpened());
  DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
  LogEvent(L"Performing operation on command session.");

  if (FCommandSession->CurrentDirectory != CurrentDirectory)
  {
    FCommandSession->CurrentDirectory = CurrentDirectory;
    // We are likely in transaction, so ReadCurrentDirectory won't get called
    // until transaction ends. But we need to know CurrentDirectory to
    // expand !/ pattern when in CustomCommandOnFiles.
    // Doing this only, when current directory of the main and secondary shell differs,
    // what would be the case before the first file in transaction.
    // Otherwise we would be reading pwd before every time as the
    // CustomCommandOnFile on its own sets FReadCurrentDirectoryPending
    if (NeedCurrentDirectory && FCommandSession->FReadCurrentDirectoryPending)
    {
      FCommandSession->ReadCurrentDirectory();
    }
  }
}

TCustomFileSystem * TTerminal::GetFileSystemForCapability(TFSCapability Capability, bool NeedCurrentDirectory)
{
  TCustomFileSystem * Result;
  if (GetIsCapable(Capability))
  {
    DebugAssert(FFileSystem != nullptr);
    Result = FFileSystem.get();
  }
  else
  {
    PrepareCommandSession(NeedCurrentDirectory);

    Result = FCommandSession->FFileSystem.get();
  }
  return Result;
}

void TTerminal::DoCustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & ACommand, int32_t AParams,
  TCaptureOutputEvent OutputEvent)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      TCustomFileSystem * FileSystem = GetFileSystemForCapability(fcAnyCommand, true);
      DebugAssert((FileSystem != FFileSystem.get()) || GetIsCapable(fcShellAnyCommand));

      FileSystem->CustomCommandOnFile(AFileName, AFile, ACommand, AParams, OutputEvent);
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, FMTLOAD(CUSTOM_COMMAND_ERROR, ACommand, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::CustomCommandOnFiles(const UnicodeString & ACommand,
  int32_t AParams, TStrings * AFiles, TCaptureOutputEvent OutputEvent)
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
    for (int32_t Index = 0; Index < AFiles->GetCount(); ++Index)
    {
      TRemoteFile * File = AFiles->GetAs<TRemoteFile>(Index);
      bool Dir = File->GetIsDirectory() && CanRecurseToDirectory(File);

      if (!Dir || FLAGSET(AParams, ccApplyToDirectories))
      {
        AddToShellFileListCommandLine(FileList, AFiles->GetString(Index));
      }
    }

    TCustomCommandData Data(this);
    UnicodeString Cmd =
      TRemoteCustomCommand(Data, RemoteGetCurrentDirectory(), "", FileList).
        Complete(ACommand, true);
    if (!DoOnCustomCommand(Cmd))
    {
      DoAnyCommand(Cmd, OutputEvent, nullptr);
    }
  }
}

bool TTerminal::DoOnCustomCommand(const UnicodeString & Command)
{
  bool Result = false;
  if (FOnCustomCommand != nullptr)
  {
    FOnCustomCommand(this, Command, Result);
  }
  return Result;
}

void TTerminal::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*const TRemoteProperties*/ void * Properties)
{
  TRemoteProperties *RProperties = cast_to<TRemoteProperties>(Properties);
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

void TTerminal::DoChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const TRemoteProperties * Properties)
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

void TTerminal::ChangeFilesProperties(TStrings * AFileList,
  const TRemoteProperties * Properties)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor); nb::used(UseBusyCursorRestorer);
  FUseBusyCursor = false;

  AnnounceFileListOperation();
  ProcessFiles(AFileList, foSetProperties, nb::bind(&TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}

bool TTerminal::LoadFilesProperties(TStrings * AFileList)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  // see comment in TSFTPFileSystem::IsCapable
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(AFileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (AFileList->GetCount() > 0) &&
      (AFileList->GetAs<TRemoteFile>(0)->GetDirectory() == FFiles.get()))
  {
    AddCachedFileList(FFiles.get());
  }
  return Result;
}

void TTerminal::DoCalculateFileSize(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * AParam)
{
  // This is called for top-level entries only
  TCalculateSizeParams * AParams = cast_to<TCalculateSizeParams>(AParam);
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
  CalculateFileSize(AFileName, AFile, AParam);

  if (AParams->Stats->CalculatedSizes != nullptr)
  {
    int64_t Size = AParams->Size - PrevSize;
    AParams->Stats->CalculatedSizes->push_back(Size);
  }
}

void TTerminal::CalculateFileSize(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * AParam)
{
  DebugAssert(AParam);
  DebugAssert(AFile);
  UnicodeString FileName = AFileName;
  TCalculateSizeParams * Params = cast_to<TCalculateSizeParams>(AParam);
  Expects(Params != nullptr);
  if (FileName.IsEmpty())
  {
    FileName = AFile->GetFileName();
  }

  if (!TryStartOperationWithFile(FileName, foCalculateSize))
  {
    Params->Result = false;
    // Combined with csIgnoreErrors, this will not abort completely, but we get called again
    // with next sibling of our parent directory (and we get again to this branch, throwing again)
    Abort();
  }

  if ((Params->CopyParam == nullptr) ||
      DoAllowRemoteFileTransfer(AFile, Params->CopyParam, FLAGSET(Params->Params, csDisallowTemporaryTransferFiles)))
  {
    int32_t CollectionIndex = -1;
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
        else if (FLAGSET(Params->Params, csStopOnFirstFile) && Params->Stats && (Params->Stats->Files > 0))
        {
          // do not waste time recursing into a folder, if we already found some files
        }
        else
        {
          if (FLAGCLEAR(Params->Params, csStopOnFirstFile))
          {
            LogEvent(FORMAT("Getting size of directory \"%s\"", FileName));
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
      Params->Size += AFile->Resolve()->GetSize();

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

bool TTerminal::DoCalculateDirectorySize(const UnicodeString & FileName, TCalculateSizeParams * Params)
{
  bool Result = false;
  if (FLAGSET(Params->Params, csStopOnFirstFile) && (Configuration->ActualLogProtocol >= 1))
  {
    LogEvent(FORMAT("Checking if remote directory \"%s\" is empty", FileName));
  }

  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      ProcessDirectory(FileName, nb::bind(&TTerminal::CalculateFileSize, this), Params, Params->UseCache);
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
      LogEvent(FORMAT("Remote directory \"%s\" is empty", FileName));
    }
    else
    {
      LogEvent(FORMAT("Remote directory \"%s\" is not empty", FileName));
    }
  }

  return Result;
}

bool TTerminal::CalculateFilesSize(TStrings * AFileList, int64_t & Size, TCalculateSizeParams & Params)
{
  // With FTP protocol, we may use DSIZ command from
  // draft-peterson-streamlined-ftp-command-extensions-10
  // Implemented by Serv-U FTP.

  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor); nb::used(UseBusyCursorRestorer);
  FUseBusyCursor = false;

  ProcessFiles(AFileList, foCalculateSize, nb::bind(&TTerminal::DoCalculateFileSize, this), &Params);
  Size = Params.Size;
  if (Configuration->ActualLogProtocol >= 1)
  {
    LogEvent(FORMAT(L"Size of %d remote files/folders calculated as %s", AFileList->Count, IntToStr(Size)));
  }
  return Params.Result;
}

void TTerminal::CalculateSubFoldersChecksum(
  const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  // recurse into subdirectories only if we have callback function
  if (OnCalculatedChecksum)
  {
    int32_t Index = 0;
    TOnceDoneOperation OnceDoneOperation; // unused
    while ((Index < FileList->Count) && !OperationProgress->Cancel)
    {
      UnicodeString FileName = FileList->Strings[Index];
      TRemoteFile * File = DebugNotNull(FileList->GetAs<TRemoteFile>(Index));

      if (File->IsDirectory &&
          CanRecurseToDirectory(File) &&
          IsRealFile(File->FileName))
      {
        OperationProgress->SetFile(File->FileName);
        std::unique_ptr<TRemoteFileList> SubFiles(CustomReadDirectoryListing(File->FullFileName, false));

        if (SubFiles.get() != nullptr)
        {
          std::unique_ptr<TStrings> SubFileList(std::make_unique<TStringList>());
          bool Success = false;
          try__finally
          {
            OperationProgress->SetFile(File->FileName);

            for (int32_t Index = 0; Index < SubFiles->Count; Index++)
            {
              TRemoteFile * SubFile = SubFiles->Files[Index];
              UnicodeString SubFileName = base::UnixCombinePaths(FileName, SubFile->FileName);
              SubFileList->AddObject(SubFileName, SubFile);
            }

            FFileSystem->CalculateFilesChecksum(Alg, SubFileList.get(), OnCalculatedChecksum, OperationProgress, false);

            Success = true;
          },
          __finally
          {
            if (FirstLevel)
            {
              OperationProgress->Finish(File->FileName, Success, OnceDoneOperation);
            }
          } end_try__finally
        }
      }
      Index++;
    }
  }
}

void TTerminal::CalculateFilesChecksum(
  const UnicodeString & Alg, TStrings * AFileList, TCalculatedChecksumEvent OnCalculatedChecksum)
{
  TFileOperationProgressType Progress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));
  OperationStart(Progress, foCalculateChecksum, osRemote, AFileList->Count);

  try__finally
  {
    TCustomFileSystem * FileSystem = GetFileSystemForCapability(fcCalculatingChecksum);

    UnicodeString NormalizedAlg = FileSystem->CalculateFilesChecksumInitialize(Alg);

    FileSystem->CalculateFilesChecksum(NormalizedAlg, AFileList, OnCalculatedChecksum, &Progress, true);
  },
  __finally
  {
    OperationStop(Progress);
  } end_try__finally
}

void TTerminal::TerminalRenameFile(const TRemoteFile * File, const UnicodeString & NewName)
{
  // Already checked in TUnixDirView::InternalEdit
  if (DebugAlwaysTrue(File->FileName() != NewName))
  {
    FileModified(File, File->FileName);
    LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", File->FileName(), NewName));
    if (DoRenameFile(File->FileName, File, NewName, false, false))
    {
      ReactOnCommand(fsRenameFile);
    }
  }
}

bool TTerminal::DoRenameOrCopyFile(
  bool Rename, const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & NewName,
  bool Move, bool DontOverwrite, bool IsBatchOperation)
{
  TBatchOverwrite BatchOverwrite = (IsBatchOperation ? OperationProgress->BatchOverwrite : boNo);
  UnicodeString AbsoluteNewName = GetAbsolutePath(NewName, true);
  bool Result = true;
  bool ExistenceKnown = false;
  std::unique_ptr<TRemoteFile> DuplicateFile;
  if (BatchOverwrite == boNone)
  {
    DebugAssert(!DontOverwrite); // unsupported combination
    Result = !this->FileExists(AbsoluteNewName);
    ExistenceKnown = true;
  }
  else if (BatchOverwrite == boAll)
  {
    // noop
  }
  else if (DebugAlwaysTrue(BatchOverwrite == boNo) &&
           Configuration->GetConfirmOverwriting() &&
           !DontOverwrite)
  {
    DuplicateFile.reset(TryReadFile(AbsoluteNewName));
    ExistenceKnown = true;

    if (DuplicateFile.get() != nullptr)
    {
      UnicodeString QuestionFmt;
      TQueryType QueryType;
      if (DuplicateFile->IsDirectory)
      {
        QuestionFmt = MainInstructions(LoadStr(DIRECTORY_OVERWRITE)) + LoadStr(DIRECTORY_OVERWRITE_WARNING);
        QueryType = qtWarning;
      }
      else
      {
        QuestionFmt = MainInstructions(LoadStr(CORE_PROMPT_FILE_OVERWRITE));
        QueryType = qtConfirmation;
      }
      TQueryParams Params(qpNeverAskAgainCheck);
      UnicodeString Question = FORMAT(QuestionFmt, NewName);
      uint32_t Answers = qaYes | qaNo | FLAGMASK(OperationProgress != nullptr, qaCancel) | FLAGMASK(IsBatchOperation, qaYesToAll | qaNoToAll);
      uint32_t Answer = QueryUser(Question, nullptr, Answers, &Params, QueryType);
      switch (Answer)
      {
        case qaNeverAskAgain:
          FConfiguration->FConfirmOverwriting = false;
          Result = true;
          break;

        case qaYes:
          Result = true;
          break;

        case qaNo:
          Result = false;
          break;

        case qaYesToAll:
          Result = true;
          if (DebugAlwaysTrue(IsBatchOperation))
          {
            OperationProgress->SetBatchOverwrite(boAll);
          }
          break;

        case qaNoToAll:
          Result = false;
          if (DebugAlwaysTrue(IsBatchOperation))
          {
            OperationProgress->SetBatchOverwrite(boNone);
          }
          break;

        case qaCancel:
          Result = false;
          OperationProgress->SetCancel(csCancel);
          break;

        default:
          Result = false;
          DebugFail();
          break;
      }
    }
  }

  if (Result)
  {
    // Prevent destroying TRemoteFile between delete and rename
    BeginTransaction();
    try__finally
    {
      TCustomFileSystem * FileSystem;
      if (!Rename)
      {
        if (GetIsCapable(fcSecondaryShell) &&
            File->IsDirectory &&
            (FCommandSession != nullptr)) // Won't be in scripting, there we let it fail later
        {
          PrepareCommandSession();
          FileSystem = FCommandSession->FFileSystem.get();
        }
        else
        {
          FileSystem = GetFileSystemForCapability(fcRemoteCopy);
        }
      }
      else
      {
        FileSystem = FFileSystem.get();
      }

      if (!GetIsCapable(fcMoveOverExistingFile) && !DontOverwrite)
      {
        if (!ExistenceKnown)
        {
          DuplicateFile.reset(TryReadFile(AbsoluteNewName));
        }

        if (DuplicateFile.get() != nullptr)
        {
          DoDeleteFile(FileSystem, AbsoluteNewName, DuplicateFile.get(), 0);
        }
      }

      TRetryOperationLoop RetryLoop(this);
      do
      {
        UnicodeString AbsoluteFileName = GetAbsolutePath(FileName, true);
        DebugAssert(FileSystem != nullptr);
        if (Rename)
        {
          TMvSessionAction Action(ActionLog, AbsoluteFileName, AbsoluteNewName);
          try
          {
            FileSystem->RemoteRenameFile(FileName, File, NewName, !DontOverwrite);
          }
          catch (Exception & E)
          {
            UnicodeString Message = FMTLOAD(Move ? MOVE_FILE_ERROR : RENAME_FILE_ERROR, FileName, NewName);
            RetryLoop.Error(E, Action, Message);
          }
        }
        else
        {
          TCpSessionAction Action(ActionLog, AbsoluteFileName, AbsoluteNewName);
          try
          {
            FileSystem->RemoteCopyFile(FileName, File, NewName, !DontOverwrite);
          }
          catch (Exception & E)
          {
            RetryLoop.Error(E, Action, FMTLOAD(COPY_FILE_ERROR, FileName, NewName));
          }
        }
      }
      while (RetryLoop.Retry());
      Result = RetryLoop.Succeeded();
    },
    __finally
    {
      EndTransaction();
    } end_try__finally
  }
  return Result;
}

bool TTerminal::DoRenameFile(
  const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & NewName, bool Move, bool DontOverwrite)
{
  // Can be foDelete when recycling (and overwrite should not happen in this case)
  bool IsBatchOperation = (OperationProgress != nullptr) && (OperationProgress->Operation == foRemoteMove);
  return DoRenameOrCopyFile(true, FileName, File, NewName, Move, DontOverwrite, IsBatchOperation);
}

bool TTerminal::DoMoveFile(const UnicodeString & FileName, const TRemoteFile * File, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(FileName, foRemoteMove, foDelete);
  DebugAssert(Param != nullptr);
  const TMoveFileParams & Params = *static_cast<const TMoveFileParams*>(Param);
  UnicodeString NewName = base::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(FileName), Params.FileMask);
  LogEvent(FORMAT(L"Moving file \"%s\" to \"%s\".", FileName, NewName));
  FileModified(File, FileName);
  bool Result = DoRenameFile(FileName, File, NewName, true, Params.DontOverwrite);
  if (Result)
  {
    ReactOnCommand(fsMoveFile);
  }
  return Result;
}

void TTerminal::TerminalMoveFile(const UnicodeString & FileName, const TRemoteFile * File, /*const TMoveFileParams*/ void * Param)
{
  DoMoveFile(FileName, File, Param);
}

bool TTerminal::TerminalMoveFiles(
  TStrings * AFileList, const UnicodeString & ATarget, const UnicodeString & AFileMask, bool DontOverwrite)
{
  TMoveFileParams Params;
  Params.Target = ATarget;
  Params.FileMask = AFileMask;
  Params.DontOverwrite = DontOverwrite;
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
      // Only after the move, as with encryption, the folders can be read and cached before the move
      // (when determining if the target exists and is encrypted)
      DirectoryModified(ATarget, true);

      UnicodeString WithTrailing = base::UnixIncludeTrailingBackslash(CurrentDirectory);
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      for (int32_t Index = 0; !PossiblyMoved && (Index < AFileList->GetCount()); ++Index)
      {
        const TRemoteFile * File =
          AFileList->GetAs<TRemoteFile>(Index);
        // File can be nullptr, and filename may not be full path,
        // but currently this is the only way we can move (at least in GUI)
        // current directory
        UnicodeString Str = AFileList->GetString(Index);
        if ((File != nullptr) &&
            File->GetIsDirectory() &&
            ((CurrentDirectory().SubString(1, Str.Length()) == Str) &&
             ((Str.Length() == CurrentDirectory().Length()) ||
              (CurrentDirectory()[Str.Length() + 1] == L'/'))))
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

void TTerminal::DoCopyFile(
  const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool DontOverwrite)
{
  bool IsBatchOperation = DebugAlwaysTrue(OperationProgress != nullptr);
  bool Move = false; // not used
  DoRenameOrCopyFile(false, AFileName, AFile, ANewName, Move, DontOverwrite, IsBatchOperation);
}

void TTerminal::TerminalCopyFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(AFileName, foRemoteCopy);
  DebugAssert(Param != nullptr);
  const TMoveFileParams & Params = *cast_to<TMoveFileParams>(Param);
  UnicodeString NewName = base::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT("Copying file \"%s\" to \"%s\".", AFileName, NewName));
  DoCopyFile(AFileName, AFile, NewName, Params.DontOverwrite);
  ReactOnCommand(fsCopyFile);
}

bool TTerminal::TerminalCopyFiles(
  TStrings * AFileList, const UnicodeString & ATarget, const UnicodeString & AFileMask, bool DontOverwrite)
{
  TMoveFileParams Params;
  Params.Target = ATarget;
  Params.FileMask = AFileMask;
  Params.DontOverwrite = DontOverwrite;
  DirectoryModified(ATarget, true);
  return ProcessFiles(AFileList, foRemoteCopy, nb::bind(&TTerminal::TerminalCopyFile, this), &Params);
}

void TTerminal::RemoteCreateDirectory(const UnicodeString & ADirName, const TRemoteProperties * Properties)
{
  DebugAssert(FFileSystem);
  DebugAssert(Properties != nullptr);
  EnsureNonExistence(ADirName);
  FileModified(nullptr, ADirName);

  LogEvent(FORMAT("Creating directory \"%s\".", ADirName));
  bool Encrypt = Properties->Valid.Contains(vpEncrypt) && Properties->Encrypt;
  DoCreateDirectory(ADirName, Encrypt);

  TValidProperties RemainingPropeties = Properties->Valid;
  RemainingPropeties >> vpEncrypt;
  if (!RemainingPropeties.Empty() &&
      (GetIsCapable(fcModeChanging) || GetIsCapable(fcOwnerChanging) || GetIsCapable(fcGroupChanging)))
  {
    DoChangeFileProperties(ADirName, nullptr, Properties);
  }

  ReactOnCommand(fsCreateDirectory);
}

void TTerminal::DoCreateDirectory(const UnicodeString & ADirName, bool Encrypt)
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
    catch(Exception &E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CREATE_DIR_ERROR, ADirName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::RemoteCreateLink(const UnicodeString & AFileName,
  const UnicodeString & APointTo, bool Symbolic)
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

void TTerminal::DoCreateLink(const UnicodeString & AFileName,
  const UnicodeString & APointTo, bool Symbolic)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteCreateLink(AFileName, APointTo, Symbolic);
    }
    catch(Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(CREATE_LINK_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

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

UnicodeString TTerminal::GetHomeDirectory()
{
  return FFileSystem->GetHomeDirectory();
}

void TTerminal::RemoteChangeDirectory(const UnicodeString & ADirectory)
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

bool TTerminal::AllowedAnyCommand(const UnicodeString & Command) const
{
  return !Command.Trim().IsEmpty();
}

bool TTerminal::GetCommandSessionOpened() const
{
  // consider secondary terminal open in "ready" state only
  // so we never do keepalives on it until it is completely initialized
  return (FCommandSession != nullptr) &&
    (FCommandSession->GetStatus() == ssOpened);
}

TTerminal * TTerminal::CreateSecondarySession(const UnicodeString & Name, TSessionData * ASessionData)
{
  std::unique_ptr<TSecondaryTerminal> Result(std::make_unique<TSecondaryTerminal>());
  Result->Init(this, ASessionData, FConfiguration, Name, ActionLog);

  Result->SetAutoReadDirectory(false);

  Result->FExceptionOnFail = FExceptionOnFail;

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

void TTerminal::FillSessionDataForCode(TSessionData * Data) const
{
  const TSessionInfo & SessionInfo = GetSessionInfo();
  if (!SessionInfo.HostKeyFingerprintSHA256.IsEmpty())
  {
    Data->SetHostKey(SessionInfo.HostKeyFingerprintSHA256);
  }
  else if (SessionInfo.CertificateVerifiedManually && DebugAlwaysTrue(!SessionInfo.CertificateFingerprintSHA256.IsEmpty()))
  {
    Data->SetHostKey(SessionInfo.CertificateFingerprintSHA256);
  }

  if (FTunnel != nullptr)
  {
    const TSessionInfo & TunnelSessionInfo = FTunnel->GetSessionInfo();
    if (DebugAlwaysTrue(!TunnelSessionInfo.HostKeyFingerprintSHA256.IsEmpty()))
    {
      Data->FTunnelHostKey = TunnelSessionInfo.HostKeyFingerprintSHA256;
    }
  }
}

void TTerminal::UpdateSessionCredentials(TSessionData * Data)
{
  // Only if it differs from the original session data?
  // Because this way, we persist the username redundantly in workspace session linked to a stored site.
  Data->UserName = UserName;
  Data->Password = Password;
  if (!FRememberedPassword.IsEmpty() && (FRememberedPasswordKind == pkPassphrase))
  {
    Data->Passphrase = GetRememberedPassword();
  }
}

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

    CommandSession->FExceptionOnFail = FExceptionOnFail;

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

void TTerminal::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
#if 0
// moved to Terminal.h
  #pragma warn -inl
  class TOutputProxy
  {
  public:
    TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) :
      FAction(Action),
      FOutputEvent(OutputEvent)
    {
    }

    void Output(const UnicodeString & Str, TCaptureOutputType OutputType)
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
          FAction.ExitCode(StrToInt(Str));
          break;
      }

      if (FOutputEvent != nullptr)
      {
        FOutputEvent(Str, OutputType);
      }
    }

  private:
    TCallSessionAction & FAction;
    TCaptureOutputEvent FOutputEvent;
  };
#pragma warn .inl
#endif // #if 0

  TCallSessionAction Action(GetActionLog(), Command, RemoteGetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, nb::bind(&TOutputProxy::Output, &ProxyOutputEvent), &Action);
}

void TTerminal::DoAnyCommand(const UnicodeString & ACommand,
  TCaptureOutputEvent OutputEvent, TCallSessionAction * Action)
{
  DebugAssert(FFileSystem);
  try
  {
    DirectoryModified(RemoteGetCurrentDirectory(), false);
    LogEvent(L"Executing user defined command.");
    TCustomFileSystem * FileSystem = GetFileSystemForCapability(fcAnyCommand);
    FileSystem->AnyCommand(ACommand, OutputEvent);
    if (GetCommandSessionOpened() && (FileSystem == FCommandSession->FFileSystem.get()))
    {
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
//    if (GetExceptionOnFail() || isa<EFatal>(&E))
//    {
//      throw;
//    }
    if (ExceptionOnFail || (isa<EFatal>(&E))) throw;
      else HandleExtendedException(&E);
  }
}

bool TTerminal::DoCreateLocalFile(const UnicodeString & AFileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
  bool NoConfirmation)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  bool Done = false;
  DWORD DesiredAccess = GENERIC_WRITE;
  DWORD ShareMode = FILE_SHARE_READ;
  DWORD CreationDisposition = CREATE_ALWAYS; // Resume ? OPEN_ALWAYS : CREATE_ALWAYS;
  DWORD FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
  do
  {
    *AHandle = this->TerminalCreateLocalFile(ApiPath(AFileName),
      DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      // save the error, otherwise it gets overwritten by call to FileExists
      int32_t LastError = ::GetLastError();
      DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
      if (base::FileExists(ApiPath(AFileName)) &&
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
                TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);
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
          __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, FileName));
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

bool TTerminal::TerminalCreateLocalFile(const UnicodeString & ATargetFileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
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
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_FILE_ERROR, FileName));

  return Result;
}

void TTerminal::TerminalOpenLocalFile(const UnicodeString & ATargetFileName,
  DWORD Access, DWORD * AAttrs, HANDLE * AHandle, int64_t * ACTime,
  int64_t * AMTime, int64_t * AATime, int64_t * ASize,
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
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(FILE_NOT_EXISTS, FileName));

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
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(OPENFILE_ERROR, FileName));

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
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_GET_ATTRS, FileName));

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
          if ((LSize == nb::ToUInt32(-1)) && (::GetLastError() != NO_ERROR))
          {
            ::RaiseLastOSError();
          }
          *ASize = (nb::ToInt64(HSize) << 32) + LSize;
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_GET_ATTRS, FileName));
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

void TTerminal::TerminalOpenLocalFile(
  const UnicodeString & AFileName, DWORD Access, TLocalFileHandle & AHandle, bool TryWriteReadOnly)
{
  AHandle.FileName = AFileName;
  this->TerminalOpenLocalFile(
    AFileName, Access, &AHandle.Attrs, &AHandle.Handle, nullptr, &AHandle.MTime, &AHandle.ATime, &AHandle.Size,
    TryWriteReadOnly);
  AHandle.Modification = ::UnixToDateTime(AHandle.MTime, GetSessionData()->GetDSTMode());
  AHandle.Directory = FLAGSET(AHandle.Attrs, faDirectory);
}

bool TTerminal::DoAllowLocalFileTransfer(
  const UnicodeString & FileName, const TSearchRecSmart & SearchRec, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles)
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

bool TTerminal::DoAllowRemoteFileTransfer(
  const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles)
{
  TFileMasks::TParams MaskParams;
  MaskParams.Size = File->Resolve()->Size;
  MaskParams.Modification = File->Modification;
  UnicodeString FullRemoteFileName = base::UnixExcludeTrailingBackslash(File->FullFileName);
  UnicodeString BaseFileName = GetBaseFileName(FullRemoteFileName);
  return
    CopyParam->AllowTransfer(BaseFileName, osRemote, File->IsDirectory, MaskParams, File->IsHidden) &&
    (!DisallowTemporaryTransferFiles || !FFileSystem->TemporaryTransferFile(File->FileName)) &&
    (!File->IsDirectory || !CopyParam->ExcludeEmptyDirectories ||
       !IsEmptyRemoteDirectory(File, CopyParam, DisallowTemporaryTransferFiles));
}

bool TTerminal::AllowLocalFileTransfer(
  const UnicodeString & AFileName, const TSearchRecSmart * SearchRec,
  const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress)
{
  bool Result = true;
  // optimization (though in most uses of the method, the caller actually knows TSearchRec already, so it passes it here
  if (GetLog()->GetLogging() || !CopyParam->AllowAnyTransfer())
  {
    TSearchRecSmart ASearchRec;
    if (SearchRec == nullptr)
    {
      if (CopyParam->FOnTransferIn)
      {
        ASearchRec.Clear();
      }
      else
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
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(FILE_NOT_EXISTS, FileName));
      }
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

void TTerminal::MakeLocalFileList(
  const UnicodeString & AFileName, const TSearchRecSmart & Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *cast_to<TMakeLocalFileListParams>(Param);
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

void TTerminal::CalculateLocalFileSize(
  const UnicodeString & AFileName, const TSearchRecSmart & Rec, /*TCalculateSizeParams*/ void * AParams)
{
  TCalculateSizeParams * Params = cast_to<TCalculateSizeParams>(AParams);
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
        int32_t CollectionIndex = -1;
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

bool TTerminal::CalculateLocalFilesSize(TStrings * AFileList,
  int64_t & Size, const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files,
  TCalculatedSizes * CalculatedSizes)
{
  bool Result = false;
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
    for (int32_t Index = 0; Params.Result && (Index < AFileList->GetCount()); ++Index)
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

  if (Configuration->ActualLogProtocol >= 1)
  {
    LogEvent(FORMAT(L"Size of %d local files/folders calculated as %s", AFileList->Count, IntToStr(Size)));
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
  return Result;
}

NB_DEFINE_CLASS_ID(TSynchronizeFileData);
struct TSynchronizeFileData : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSynchronizeFileData); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSynchronizeFileData) || TObject::is(Kind); }
public:
  TSynchronizeFileData() :
    TObject(OBJECT_CLASS_TSynchronizeFileData)
  {
    nb::ClearStruct(LocalLastWriteTime);
  }

  bool Modified{false};
  bool New{false};
  bool IsDirectory{false};
  TChecklistItem::TFileInfo Info;
  TChecklistItem::TFileInfo MatchingRemoteFile;
  TRemoteFile * MatchingRemoteFileFile{nullptr};
  int32_t MatchingRemoteFileImageIndex{0};
  FILETIME LocalLastWriteTime;
};

constexpr const int32_t sfFirstLevel = 0x01;
NB_DEFINE_CLASS_ID(TSynchronizeData);
struct TSynchronizeData : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSynchronizeData); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSynchronizeData) || TObject::is(Kind); }
public:
  TSynchronizeData() : TObject(OBJECT_CLASS_TSynchronizeData) {}

  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  TTerminal::TSynchronizeMode Mode{};
  int32_t Params{0};
  TSynchronizeDirectoryEvent OnSynchronizeDirectory;
  TSynchronizeOptions * Options{nullptr};
  int32_t Flags{0};
  TStringList * LocalFileList{nullptr};
  const TCopyParamType *CopyParam{nullptr};
  TSynchronizeChecklist *Checklist{nullptr};

  void DeleteLocalFileList()
  {
    if (LocalFileList != nullptr)
    {
      for (int32_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
      {
        TSynchronizeFileData *FileData = LocalFileList->GetAs<TSynchronizeFileData>(Index);
        SAFE_DESTROY(FileData);
      }
      SAFE_DESTROY(LocalFileList);
    }
  }
};

TSynchronizeChecklist * TTerminal::SynchronizeCollect(const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, int32_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
  TSynchronizeOptions * Options)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor); nb::used(UseBusyCursorRestorer);
  FUseBusyCursor = false;

  std::unique_ptr<TSynchronizeChecklist> Checklist(std::make_unique<TSynchronizeChecklist>());
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

static void AddFlagName(UnicodeString & ParamsStr, int32_t & Params, int32_t Param, const UnicodeString & Name)
{
  if (FLAGSET(Params, Param))
  {
    AddToList(ParamsStr, Name, L", ");
  }
  Params &= ~Param;
}

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

UnicodeString TTerminal::SynchronizeParamsStr(int32_t Params)
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
  AddFlagName(ParamsStr, Params, spByChecksum, L"ByChecksum");
  AddFlagName(ParamsStr, Params, spCaseSensitive, L"CaseSensitive");
  AddFlagName(ParamsStr, Params, spSelectedOnly, L"*SelectedOnly"); // GUI only
  AddFlagName(ParamsStr, Params, spMirror, L"Mirror");
  if (Params > 0)
  {
    AddToList(ParamsStr, FORMAT("0x%x", nb::ToInt(Params)), L", ");
  }
  return ParamsStr;
}

bool TTerminal::LocalFindFirstLoop(const UnicodeString & APath, TSearchRecChecked & SearchRec)
{
  bool Result;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(LIST_DIR_ERROR, APath), "",
  [&]()
  {
    DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    Result = (FindFirstChecked(APath, FindAttrs, SearchRec) == 0);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, Path));
  return Result;
}

bool TTerminal::LocalFindNextLoop(TSearchRecChecked & SearchRec)
{
  bool Result;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(LIST_DIR_ERROR, SearchRec.Path), "",
  [&]()
  {
    Result = (FindNextChecked(SearchRec) == 0);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, SearchRec.Path));
  return Result;
}

bool TTerminal::IsEmptyLocalDirectory(
  const UnicodeString & Path, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles)
{
  UnicodeString Contents;
  if (Configuration->ActualLogProtocol >= 1)
  {
    LogEvent(FORMAT("Checking if local directory \"%s\" is empty", Path));
  }

  TSearchRecOwned SearchRec;
  if (LocalFindFirstLoop(IncludeTrailingBackslash(Path) + L"*.*", SearchRec))
  {
    do
    {
      UnicodeString FullLocalFileName = SearchRec.GetFilePath();
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
      LogEvent(FORMAT("Local directory \"%s\" is empty", Path));
    }
    else
    {
      LogEvent(FORMAT("Local directory \"%s\" is not empty, it contains \"%s\"", Path, Contents));
    }
  }

  return Contents.IsEmpty();
}

void TTerminal::DoSynchronizeCollectDirectory(const UnicodeString & ALocalDirectory,
  const UnicodeString & ARemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, int32_t AParams,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options,
  int32_t AFlags, TSynchronizeChecklist * Checklist)
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
    SynchronizeModeStr(Mode), int(AParams), SynchronizeParamsStr(AParams), CopyParam->GetIncludeFileMask().Masks()));

  if (FLAGCLEAR(AParams, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  try__finally
  {
    Data.LocalFileList = CreateSortedStringList(FLAGSET(AParams, spCaseSensitive));

    TSearchRecOwned SearchRec;
    if (LocalFindFirstLoop(Data.LocalDirectory + L"*.*", SearchRec))
    {
      do
      {
        UnicodeString FileName = SearchRec.Name;
        UnicodeString FullLocalFileName = SearchRec.GetFilePath();
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
          LogEvent(0, FORMAT("Local file %s included to synchronization",
            FormatFileDetailsForLog(FullLocalFileName, SearchRec.GetLastWriteTime(), SearchRec.Size)));
        }
        else
        {
          LogEvent(0, FORMAT("Local file %s excluded from synchronization",
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
      for (int32_t Index = 0; Index < Data.LocalFileList->GetCount(); ++Index)
      {
        FileData = Data.LocalFileList->GetAs<TSynchronizeFileData>
          (Index);
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
          std::unique_ptr<TChecklistItem> ChecklistItem(std::make_unique<TChecklistItem>());
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

void TTerminal::SynchronizeCollectFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param)
{
  try
  {
    DoSynchronizeCollectFile(AFileName, AFile, Param);
  }
  catch (ESkipFile & E)
  {
    TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);
    nb::used(Suspend);
    if (!HandleException(&E))
    {
      throw;
    }
  }
}

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

void TTerminal::CollectCalculatedChecksum(
  const UnicodeString & DebugUsedArg(FileName), const UnicodeString & DebugUsedArg(Alg), const UnicodeString & Hash)
{
  DebugAssert(FCollectedCalculatedChecksum.IsEmpty());
  FCollectedCalculatedChecksum = Hash;
}

bool TTerminal::SameFileChecksum(const UnicodeString & LocalFileName, const TRemoteFile * File)
{
  UnicodeString DefaultAlg = Sha256ChecksumAlg;
  UnicodeString Algs =
    DefaultStr(Configuration->SynchronizationChecksumAlgs, DefaultAlg + L"," + Sha1ChecksumAlg);
  std::unique_ptr<TStrings> SupportedAlgs(CreateSortedStringList());
  GetSupportedChecksumAlgs(SupportedAlgs.get());
  UnicodeString Alg;
  while (Alg.IsEmpty() && !Algs.IsEmpty())
  {
    UnicodeString A = CutToChar(Algs, L',', true);

    if (SupportedAlgs->IndexOf(A) >= 0)
    {
      Alg = A;
    }
  }

  if (Alg.IsEmpty())
  {
    Alg = DefaultAlg;
  }

  std::unique_ptr<TStrings> FileList(std::make_unique<TStringList>());
  FileList->AddObject(File->FullFileName, File);
  DebugAssert(FCollectedCalculatedChecksum.IsEmpty());
  FCollectedCalculatedChecksum = EmptyStr;
  CalculateFilesChecksum(Alg, FileList.get(), nb::bind(&TTerminal::CollectCalculatedChecksum, this));
  UnicodeString RemoteChecksum = FCollectedCalculatedChecksum;
  FCollectedCalculatedChecksum = EmptyStr;

  UnicodeString LocalChecksum;
  __removed FILE_OPERATION_LOOP_BEGIN
  FileOperationLoopCustom(this, OperationProgress, folNone,
  FMTLOAD(CHECKSUM_ERROR, LocalFileName), "",
  [&]()
  {
    std::unique_ptr<THandleStream> Stream(TSafeHandleStream::CreateFromFile(LocalFileName, fmOpenRead | fmShareDenyWrite));
    LocalChecksum = CalculateFileChecksum(Stream.get(), Alg);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CHECKSUM_ERROR, (LocalFileName)));

  return SameText(RemoteChecksum, LocalChecksum);
}

void TTerminal::DoSynchronizeCollectFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData * Data = cast_to<TSynchronizeData>(Param);
  Expects(Data != nullptr);

  // Can be nullptr in scripting
  if (Data->Options != nullptr)
  {
    Data->Options->Files++;
  }

  UnicodeString LocalFileName = ChangeFileName(Data->CopyParam, AFile->FileName, osRemote, false);
  UnicodeString FullRemoteFileName = base::UnixExcludeTrailingBackslash(AFile->FullFileName);
  if (DoAllowRemoteFileTransfer(AFile, Data->CopyParam, true) &&
      (FLAGCLEAR(Data->Flags, sfFirstLevel) ||
       (Data->Options == nullptr) ||
        Data->Options->MatchesFilter(AFile->GetFileName()) ||
        Data->Options->MatchesFilter(LocalFileName)))
  {
    std::unique_ptr<TChecklistItem> ChecklistItem(std::make_unique<TChecklistItem>());
    try__finally
    {
      ChecklistItem->IsDirectory = AFile->GetIsDirectory();
      ChecklistItem->ImageIndex = AFile->GetIconIndex();

      ChecklistItem->Remote.FileName = AFile->GetFileName();
      ChecklistItem->Remote.Directory = Data->RemoteDirectory;
      ChecklistItem->Remote.Modification = AFile->GetModification();
      ChecklistItem->Remote.ModificationFmt = AFile->GetModificationFmt();
      ChecklistItem->Remote.Size = AFile->Resolve()->GetSize();

      bool Modified = false;
      bool New = false;
      if (AFile->GetIsDirectory() && !CanRecurseToDirectory(AFile))
      {
        LogEvent(FORMAT("Skipping symlink to directory \"%s\".", AFile->GetFileName()));
      }
      else
      {
        int32_t LocalIndex = Data->LocalFileList->IndexOf(LocalFileName);
        New = (LocalIndex < 0);
        if (!New)
        {
          TSynchronizeFileData * LocalData =
          Data->LocalFileList->GetAs<TSynchronizeFileData>(LocalIndex);

          LocalData->New = false;
          UnicodeString FullLocalFileName = LocalData->Info.Directory + LocalData->Info.FileName;

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
            int32_t TimeCompare;
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
            else if (FLAGSET(Data->Params, spByChecksum) &&
                     FLAGCLEAR(Data->Params, spTimestamp) &&
                     !SameFileChecksum(FullLocalFileName, AFile) &&
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
                FormatFileDetailsForLog(FullLocalFileName, LocalData->Info.Modification, LocalData->Info.Size),
                FormatFileDetailsForLog(FullRemoteFileName, AFile->Modification, AFile->Size, AFile->LinkedFile)));
            }

            if (Modified)
            {
              LogEvent(FORMAT("Remote file %s is modified comparing to local file %s",
                FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize(), AFile->LinkedFile),
                FormatFileDetailsForLog(FullLocalFileName, LocalData->Info.Modification, LocalData->Info.Size)));
            }
          }
          else if (FLAGCLEAR(Data->Params, spNoRecurse))
          {
            DoSynchronizeCollectDirectory(
              FullLocalFileName, FullRemoteFileName,
              Data->Mode, Data->CopyParam, Data->Params, Data->OnSynchronizeDirectory,
              Data->Options, (Data->Flags & ~sfFirstLevel),
              Data->Checklist);
          }
        }
        else
        {
          ChecklistItem->Local.Directory = Data->LocalDirectory;
          LogEvent(FORMAT("Remote file %s is new",
            FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize(), AFile->LinkedFile)));
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
    LogEvent(0, FORMAT("Remote file %s excluded from synchronization",
      FormatFileDetailsForLog(FullRemoteFileName, AFile->Modification, AFile->Size, AFile->LinkedFile)));
  }
}

void TTerminal::SynchronizeApply(
  TSynchronizeChecklist * Checklist,
  const TCopyParamType * CopyParam, int32_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TProcessedSynchronizationChecklistItem OnProcessedItem,
  TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems, void * Token,
  TFileOperationStatistics * Statistics)
{
  TSynchronizeData Data;

  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;

  int32_t CopyParams =
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
    for (int32_t Index = 0; Index < Checklist->Count; Index++)
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
    int32_t Index = 0;
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
            FORMAT("Synchronizing local directory '%s' with remote directory '%s', params = 0x%x (%s)",
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
      TCalculateSizeParams Params;
      Params.CopyParam = CopyParam;
      Params.Stats = &RemoteStats;
      CalculateFilesSize(RemoteFileList.get(), RemoteSize, Params);
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

void TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
  bool Collect)
{
  if (Data.OnSynchronizeDirectory)
  {
    bool Continue = true;
    Data.OnSynchronizeDirectory(
      Data.LocalDirectory, Data.RemoteDirectory, Continue, Collect, Data.Options);

    if (!Continue)
    {
      Abort();
    }
  }
}

void TTerminal::SynchronizeLocalTimestamp(const UnicodeString & /*AFileName*/,
  const TRemoteFile * AFile, void * /*Param*/)
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
    int32_t Error = ::GetLastError();
    ::CloseHandle(Handle);
    if (!Result)
    {
      ::RaiseLastOSError(Error);
    }
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, LocalFile));
}

void TTerminal::SynchronizeRemoteTimestamp(const UnicodeString & /*AFileName*/,
  const TRemoteFile * AFile, void * /*Param*/)
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

void TTerminal::FileFind(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TFilesFindParams*/ void * Param)
{
  // see DoFilesFind
  FOnFindingFile = nullptr;

  DebugAssert(Param);
  DebugAssert(AFile);
  TFilesFindParams * AParams = cast_to<TFilesFindParams>(Param);

  if (!AParams->Cancel)
  {
    UnicodeString LocalFileName = AFileName;
    if (AFileName.IsEmpty())
    {
      LocalFileName = AFile->GetFileName();
    }

    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->Resolve()->GetSize();
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
          LogEvent(FORMAT("Already searched \"%s\" directory (real path \"%s\"), link loop detected", FullFileName, RealDirectory));
        }
        else
        {
          DoFilesFind(FullFileName, *AParams, RealDirectory);
        }
      }
    }
  }
}

void TTerminal::DoFilesFind(const UnicodeString & ADirectory, TFilesFindParams & Params, const UnicodeString & ARealDirectory)
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

void TTerminal::FilesFind(const UnicodeString & ADirectory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.FileMask.FNoImplicitMatchWithDirExcludeMask = true;
  Params.FileMask.FAllDirsAreImplicitlyIncluded = true;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;

  Params.LoopDetector.RecordVisitedDirectory(ADirectory);

  DoFilesFind(ADirectory, Params, ADirectory);
}

void TTerminal::SpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & ASpaceAvailable)
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

void TTerminal::LockFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foLock);

  LogEvent(FORMAT("Locking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoLockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}

void TTerminal::DoLockFile(const UnicodeString & AFileName, const TRemoteFile * AFile)
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

void TTerminal::UnlockFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foUnlock);

  LogEvent(FORMAT("Unlocking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoUnlockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}

void TTerminal::DoUnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FFileSystem->UnlockFile(AFileName, AFile);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(UNLOCK_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::LockFiles(TStrings * AFileList)
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

void TTerminal::UnlockFiles(TStrings * AFileList)
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

const TSessionInfo & TTerminal::GetSessionInfo() const
{
  return FFileSystem->GetSessionInfo();
}

const TFileSystemInfo & TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}

TStrings * TTerminal::GetShellChecksumAlgDefs()
{
  if (FShellChecksumAlgDefs.get() == nullptr)
  {
    UnicodeString ChecksumCommandsDef = Configuration->FChecksumCommands;
    if (ChecksumCommandsDef.IsEmpty())
    {
      UnicodeString Delimiter(L",");
      AddToList(ChecksumCommandsDef, Sha512ChecksumAlg + L"=sha512sum", Delimiter);
      AddToList(ChecksumCommandsDef, Sha384ChecksumAlg + L"=sha384sum", Delimiter);
      AddToList(ChecksumCommandsDef, Sha256ChecksumAlg + L"=sha256sum", Delimiter);
      AddToList(ChecksumCommandsDef, Sha224ChecksumAlg + L"=sha224sum", Delimiter);
      AddToList(ChecksumCommandsDef, Sha1ChecksumAlg + L"=sha1sum", Delimiter);
      AddToList(ChecksumCommandsDef, Md5ChecksumAlg + L"=md5sums", Delimiter);
    }

    FShellChecksumAlgDefs.reset(CommaTextToStringList(ChecksumCommandsDef));
  }
  return FShellChecksumAlgDefs.get();
}

void TTerminal::GetShellChecksumAlgs(TStrings * Algs)
{
  TStrings * AlgDefs = GetShellChecksumAlgDefs();
  for (int32_t Index = 0; Index < AlgDefs->Count; Index++)
  {
    Algs->Add(AlgDefs->GetName(Index));
  }
}

void TTerminal::GetSupportedChecksumAlgs(TStrings * Algs)
{
  if (!GetIsCapable(fcCalculatingChecksum) && GetIsCapable(fcSecondaryShell))
  {
    // Both return the same anyway
    if (GetCommandSessionOpened())
    {
      FCommandSession->GetSupportedChecksumAlgs(Algs);
    }
    else
    {
      GetShellChecksumAlgs(Algs);
    }
  }
  else
  {
    FFileSystem->GetSupportedChecksumAlgs(Algs);
  }
}

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

UnicodeString TTerminal::GetRememberedPassword() const
{
  return DecryptPassword(FRememberedPassword);
}

UnicodeString TTerminal::GetRememberedTunnelPassword() const
{
  return DecryptPassword(FRememberedTunnelPassword);
}

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

int32_t TTerminal::CopyToParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress)
{
  UnicodeString FileName;
  TObject * Object{nullptr};
  UnicodeString TargetDir;
  bool Dir{false};
  bool Recursed{false};

  TCopyParamType * CustomCopyParam = nullptr;
  int32_t Result = ParallelOperation->GetNext(this, FileName, Object, TargetDir, Dir, Recursed, CustomCopyParam);
  std::unique_ptr<TCopyParamType> CustomCopyParamOwner(CustomCopyParam);

  if (Result > 0)
  {
    std::unique_ptr<TStrings> FilesToCopy(std::make_unique<TStringList>());
    FilesToCopy->AddObject(FileName, Object);

    // OnceDoneOperation is not supported
    TOnceDoneOperation OnceDoneOperation = odoIdle;

    int32_t Params = ParallelOperation->GetParams();
    // If we failed to recurse the directory when enumerating, recurse now (typically only to show the recursion error)
    if (Dir && Recursed)
    {
      Params = Params | cpNoRecurse;
    }

    int32_t Prev = OperationProgress->GetFilesFinishedSuccessfully();
    DebugAssert((FOperationProgress == OperationProgress) || (FOperationProgress == nullptr));
    TFileOperationProgressType * PrevOperationProgress = FOperationProgress;
    const TCopyParamType * CopyParam = (CustomCopyParam != nullptr) ? CustomCopyParam : ParallelOperation->GetCopyParam();
    try__finally
    {
      FOperationProgress = OperationProgress;
      if (ParallelOperation->GetSide() == osLocal)
      {
        FFileSystem->CopyToRemote(
          FilesToCopy.get(), TargetDir, CopyParam, Params, OperationProgress, OnceDoneOperation);
      }
      else if (DebugAlwaysTrue(ParallelOperation->GetSide() == osRemote))
      {
        FFileSystem->CopyToLocal(
          FilesToCopy.get(), TargetDir, CopyParam, Params, OperationProgress, OnceDoneOperation);
      }
    },
    __finally
    {
      bool Success = (Prev < OperationProgress->GetFilesFinishedSuccessfully());
      ParallelOperation->Done(FileName, Dir, Success, TargetDir, CopyParam, this);
      // Not to fail an assertion in OperationStop when called from CopyToRemote or CopyToLocal,
      // when FOperationProgress is already OperationProgress.
      FOperationProgress = PrevOperationProgress;
    } end_try__finally
  }

  return Result;
}

bool TTerminal::CanParallel(
  const TCopyParamType * CopyParam, int32_t Params, TParallelOperation * ParallelOperation) const
{
  return
    (ParallelOperation != nullptr) &&
    FFileSystem->IsCapable(fcParallelTransfers) &&
    // parallel transfer is not implemented for operations needed to be done on a folder
    // after all its files are processed
    FLAGCLEAR(Params, cpDelete) &&
    (!CopyParam->GetPreserveTime() || !CopyParam->GetPreserveTimeDirs());
}

void TTerminal::CopyParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress)
{
  try__finally
  {
    bool Continue = true;
    do
    {
      int32_t GotNext = CopyToParallel(ParallelOperation, OperationProgress);
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

void TTerminal::LogParallelTransfer(TParallelOperation * ParallelOperation)
{
  LogEvent(
    FORMAT("Adding a parallel transfer to the transfer started on the connection \"%s\"",
      ParallelOperation->GetMainName()));
}

void TTerminal::LogTotalTransferDetails(
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  TFileOperationProgressType * OperationProgress, bool Parallel, TStrings * AFiles)
{
  if (FLog->GetLogging())
  {
    UnicodeString TargetSide = (OperationProgress->GetSide() == osLocal) ? L"remote" : L"local";
    UnicodeString S =
      FORMAT(
        "Copying %d files/directories to %s directory \"%s\"",
        OperationProgress->GetCount(), TargetSide, ATargetDir);
    if (Parallel && DebugAlwaysTrue(AFiles != nullptr))
    {
      int32_t Count = 0;
      for (int32_t Index = 0; Index < AFiles->GetCount(); ++Index)
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

void TTerminal::LogTotalTransferDone(TFileOperationProgressType * OperationProgress)
{
  LogEvent(L"Copying finished: " + OperationProgress->GetLogStr(true));
}

bool TTerminal::CopyToRemote(
  TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TParallelOperation * ParallelOperation)
{
  DebugAssert(FFileSystem);
  DebugAssert(AFilesToCopy);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  if ((CopyParam->FOnTransferIn) && !FFileSystem->IsCapable(fcTransferIn))
  {
    throw Exception(LoadStr(NOTSUPPORTED));
  }

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
    // TFileOperationProgressType OperationProgress(&DoProgress, &DoFinished);
    OperationStart(
      OperationProgress, (AParams & cpDelete ? foMove : foCopy), osLocal,
      AFilesToCopy->Count, AParams & cpTemporary, ATargetDir, CopyParam->CPSLimit, CopyParam->OnceDoneOperation);

    __removed bool CollectingUsage = false;
    try__finally
    {
      if (CalculatedSize)
      {
#if 0
        if (Configuration->Usage->Collect)
        {
          int32_t CounterSize = TUsage::CalculateCounterSize(Size);
          Configuration->Usage->Inc("Uploads");
          Configuration->Usage->Inc("UploadedBytes", CounterSize);
          Configuration->Usage->SetMax(L"MaxUploadSize", CounterSize);
          CollectingUsage = true;
        }
#endif // #if 0
        OperationProgress.SetTotalSize(Size);
      }

      BeginTransaction();
      try__finally
      {
        bool Parallel = ACanParallel && CalculatedSize;
        LogTotalTransferDetails(ATargetDir, CopyParam, &OperationProgress, Parallel, Files.get());

        if (Parallel)
        {
          // OnceDoneOperation is not supported
          ParallelOperation->Init(Files.release(), ATargetDir, CopyParam, AParams, &OperationProgress, GetLog()->GetName(), -1);
          CopyParallel(ParallelOperation, &OperationProgress);
        }
        else
        {
          FFileSystem->CopyToRemote(
            AFilesToCopy, ATargetDir, CopyParam, AParams, &OperationProgress, OnceDoneOperation);
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
        int32_t CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc("UploadTime", CounterTime);
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

void TTerminal::DoCopyToRemote(
  TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TFileOperationProgressType * OperationProgress, uint32_t AFlags, TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  FFileSystem->TransferOnDirectory(ATargetDir, CopyParam, AParams);

  // Must be local resolving, as this is outside of robust loop
  UnicodeString TargetDir = GetAbsolutePath(ATargetDir, true);
  UnicodeString FullTargetDir = base::UnixIncludeTrailingBackslash(ATargetDir);
  int32_t Index = 0;
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

          if (base::DirectoryExists(ApiPath(FileName)))
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
        TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);
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

void TTerminal::SourceRobust(
  const UnicodeString & AFileName, const TSearchRecSmart * SearchRec,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TFileOperationProgressType * OperationProgress, uint32_t AFlags)
{
  TUploadSessionAction Action(GetActionLog());
  bool * AFileTransferAny = FLAGSET(AFlags, tfUseFileTransferAny) ? &FFileTransferAny : nullptr;
  bool CanRetry = (!CopyParam->FOnTransferIn);
  TRobustOperationLoop RobustLoop(this, OperationProgress, AFileTransferAny, CanRetry);

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

bool TTerminal::CreateTargetDirectory(
  const UnicodeString & ADirectoryPath, uint32_t Attrs, const TCopyParamType * CopyParam)
{
  std::unique_ptr<TRemoteFile> File(TryReadFile(ADirectoryPath));
  bool DoCreate =
    (File.get() == nullptr) ||
    !File->IsDirectory; // just try to create and make it fail
  File.reset(nullptr);
  if (DoCreate)
  {
    TRemoteProperties Properties;
    // Do not even try with FTP, as while theoretically it supports setting permissions of created folders and it will try,
    // it most likely fail as most FTP servers do not support SITE CHMOD.
    if (CopyParam->GetPreserveRights() && GetIsCapable(fcModeChangingUpload))
    {
      Properties.Valid = Properties.Valid << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }
    Properties.Valid = Properties.Valid << vpEncrypt;
    Properties.Encrypt = CopyParam->EncryptNewFiles;
    RemoteCreateDirectory(ADirectoryPath, &Properties);
  }
  return DoCreate;
}

void TTerminal::DirectorySource(
  const UnicodeString & ADirectoryName, const UnicodeString & ATargetDir, const UnicodeString & ADestDirectoryName,
  uint32_t Attrs, const TCopyParamType * CopyParam, int32_t AParams,
  TFileOperationProgressType * OperationProgress, uint32_t AFlags)
{
  FFileSystem->TransferOnDirectory(ATargetDir, CopyParam, AParams);

  //UnicodeString DestFullName = base::UnixIncludeTrailingBackslash(ATargetDir + ADestDirectoryName);
  UnicodeString DestFullName = ATargetDir + ADestDirectoryName;

  OperationProgress->SetFile(ADirectoryName);

  bool PostCreateDir = FLAGCLEAR(AFlags, tfPreCreateDir);
  // WebDAV, SFTP and S3
  if (!PostCreateDir)
  {
    // This is originally a code for WebDAV, SFTP used a slightly different logic,
    // but functionally it should be very similar.
    if (CreateTargetDirectory(DestFullName, Attrs, CopyParam))
    {
      AFlags |= tfNewDirectory;
    }
  }

  bool DoRecurse = FLAGCLEAR(AParams, cpNoRecurse);
  if (DoRecurse)
  {
    TSearchRecOwned SearchRec;
    bool FindOK = LocalFindFirstLoop(ADirectoryName + L"*.*", SearchRec);
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = SearchRec.GetFilePath();
      try
      {
        if (SearchRec.IsRealFile())
        {
          // Not sure if we need the trailing slash here, but we cannot use it in CreateTargetDirectory.
          // At least FTP cannot handle it, when setting the new directory permissions.
          UnicodeString ATargetDir = base::UnixIncludeTrailingBackslash(DestFullName);
          SourceRobust(FileName, &SearchRec, ATargetDir, CopyParam, AParams, OperationProgress, (AFlags & ~(tfFirstLevel | tfAutoResume)));
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
  }

  // FTP
  if (PostCreateDir)
  {
    CreateTargetDirectory(DestFullName, Attrs, CopyParam);
  }

  // Paralell transfers (cpNoRecurse) won't be allowed if any of these are set anyway (see CanParallel).
  // Exception is ClearArchive, which is does not prevent parallel transfer, but is silently ignored for directories.
  if (DoRecurse && !OperationProgress->GetCancel())
  {
    // TODO : Delete also read-only directories.
    // TODO : Show error message on failure.
    if (GetIsCapable(fcPreservingTimestampDirs) && CopyParam->PreserveTime && CopyParam->PreserveTimeDirs)
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
        THROWOSIFFALSE(::SysUtulsFileSetAttr(ApiPath(ADirectoryName), static_cast<DWORD>(Attrs & ~faArchive)) == 0);
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, DirectoryName));
    }
  }
}

bool TTerminal::UseAsciiTransfer(
  const UnicodeString & BaseFileName, TOperationSide Side, const TCopyParamType * CopyParam,
  const TFileMasks::TParams & MaskParams)
{
  return
      GetIsCapable(fcTextMode) &&
    CopyParam->UseAsciiTransfer(BaseFileName, Side, MaskParams) &&
    // Used either before PartSize is set (to check that file is to be transfered in binary mode, hence allows parallel file transfer),
    // or later during parallel transfer (which never happens for ascii mode).
    DebugAlwaysTrue(CopyParam->PartSize < 0);
}

void TTerminal::SelectTransferMode(
  const UnicodeString & ABaseFileName, TOperationSide Side, const TCopyParamType * CopyParam,
  const TFileMasks::TParams & MaskParams)
{
  bool AsciiTransfer = UseAsciiTransfer(ABaseFileName, Side, CopyParam, MaskParams);
  OperationProgress->SetAsciiTransfer(AsciiTransfer);
  UnicodeString ModeName = OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary";
  LogEvent(0, FORMAT("%s transfer mode selected.", ModeName));
}

void TTerminal::SelectSourceTransferMode(const TLocalFileHandle & Handle, const TCopyParamType * CopyParam)
{
  // Will we use ASCII or BINARY file transfer?
  TFileMasks::TParams MaskParams;
  MaskParams.Size = Handle.Size;
  MaskParams.Modification = Handle.Modification;
  UnicodeString BaseFileName = GetBaseFileName(Handle.FileName);
  SelectTransferMode(BaseFileName, osLocal, CopyParam, MaskParams);
}

void TTerminal::DoDeleteLocalFile(const UnicodeString & FileName)
{
  __removed FILE_OPERATION_LOOP_BEGIN
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
  FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, FileName), "",
  [&]()
  {
    DeleteFileChecked(FileName);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, FileName));
}

void TTerminal::DoRenameLocalFileForce(const UnicodeString & OldName, const UnicodeString & NewName)
{
  if (base::FileExists(ApiPath(NewName)))
  {
    DoDeleteLocalFile(NewName);
  }

  __removed FILE_OPERATION_LOOP_BEGIN
  FileOperationLoopCustom(this, FOperationProgress, folNone, FMTLOAD(RENAME_FILE_ERROR, OldName, NewName), "",
  [&]()
  {
    THROWOSIFFALSE(SysUtulsRenameFile(ApiPath(OldName), ApiPath(NewName)));
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(RENAME_FILE_ERROR, OldName, NewName));
}

void TTerminal::UpdateSource(const TLocalFileHandle & AHandle, const TCopyParamType * CopyParam, int32_t AParams)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  // TODO: Delete also read-only files.
  if (FLAGSET(AParams, cpDelete))
  {
    if (!AHandle.Directory)
    {
      LogEvent(FORMAT(L"Deleting successfully uploaded source file \"%s\".", AHandle.FileName));

      DoDeleteLocalFile(AHandle.FileName);
    }
  }
  else if (CopyParam->ClearArchive && FLAGSET(AHandle.Attrs, faArchive))
  {
    __removed FILE_OPERATION_LOOP_BEGIN
    FileOperationLoopCustom(this, OperationProgress, folNone, FMTLOAD(CANT_SET_ATTRS, AHandle.FileName), "",
    [&]()
    {
      THROWOSIFFALSE(::SysUtulsFileSetAttr(ApiPath(AHandle.FileName), (AHandle.Attrs & ~faArchive)) == 0);
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, Handle.FileName));
  }
}

void TTerminal::Source(
  const UnicodeString & AFileName, const TSearchRecSmart * SearchRec,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TFileOperationProgressType * OperationProgress, uint32_t AFlags, TUploadSessionAction & Action, bool & ChildError)
{
  UnicodeString ActionFileName = AFileName;
  if (!CopyParam->FOnTransferIn)
  {
    ActionFileName = ::ExpandUNCFileName(ActionFileName);
  }
  Action.SetFileName(ActionFileName);

  OperationProgress->SetFile(AFileName, false);

  if (!AllowLocalFileTransfer(AFileName, SearchRec, CopyParam, OperationProgress))
  {
    throw ESkipFile();
  }

  TLocalFileHandle Handle;
  if (CopyParam->FOnTransferIn.empty())
  {
    TerminalOpenLocalFile(AFileName, GENERIC_READ, Handle);
  }
  else
  {
    Handle.FileName = AFileName;
  }

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
    if (!CopyParam->FOnTransferIn.empty())
    {
      LogEvent(FORMAT(L"Streaming \"%s\" to remote directory started.", AFileName));
    }
    else
    {
      LogEvent(FORMAT("Copying \"%s\" to remote directory started.", AFileName));

      OperationProgress->SetLocalSize(Handle.Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->GetLocalSize());
    }

    if (GetIsCapable(fcTextMode))
    {
      SelectSourceTransferMode(Handle, CopyParam);
    }

    FFileSystem->Source(
      Handle, ATargetDir, DestFileName, CopyParam, AParams, OperationProgress, AFlags, Action, ChildError);

    LogFileDone(OperationProgress, GetAbsolutePath(ATargetDir + DestFileName, true), Action);
    OperationProgress->Succeeded();
  }

  Handle.Release();

  UpdateSource(Handle, CopyParam, AParams);
}

void TTerminal::CheckParallelFileTransfer(
  const UnicodeString & TargetDir, TStringList * Files, const TCopyParamType * CopyParam, int32_t Params,
  UnicodeString & ParallelFileName, int64_t & ParallelFileSize, TFileOperationProgressType * OperationProgress)
{
  if ((Configuration->ParallelTransferThreshold > 0) &&
      FFileSystem->IsCapable(fcParallelFileTransfers))
  {
    TObject * ParallelObject = nullptr;
    if (TParallelOperation::GetOnlyFile(Files, ParallelFileName, ParallelObject))
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(ParallelObject);
      const TRemoteFile * UltimateFile = File->Resolve();
      if ((UltimateFile == File) && // not tested with symlinks
          (UltimateFile->Size >= nb::ToInt64(Configuration->ParallelTransferThreshold) * 1024))
      {
        UnicodeString BaseFileName = GetBaseFileName(ParallelFileName);
        TFileMasks::TParams MaskParams;
        MaskParams.Size = UltimateFile->Size;
        MaskParams.Modification = File->Modification;
        if (!UseAsciiTransfer(BaseFileName, osRemote, CopyParam, MaskParams))
        {
          ParallelFileSize = UltimateFile->Size;
          UnicodeString TargetFileName = CopyParam->ChangeFileName(base::UnixExtractFileName(ParallelFileName), osRemote, true);
          UnicodeString DestFullName = TPath::Combine(TargetDir, TargetFileName);

          if (base::FileExists(ApiPath(DestFullName)))
          {
            TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);

            TOverwriteFileParams FileParams;
            int64_t MTime;
            TerminalOpenLocalFile(DestFullName, GENERIC_READ, nullptr, nullptr, nullptr, &MTime, nullptr, &FileParams.DestSize, false);
            FileParams.SourceSize = ParallelFileSize;
            FileParams.SourceTimestamp = UltimateFile->Modification;
            FileParams.SourcePrecision = UltimateFile->ModificationFmt;
            FileParams.DestTimestamp = UnixToDateTime(MTime, SessionData->DSTMode);
            int32_t Answers = qaYes | qaNo | qaCancel;
            TQueryParams QueryParams(qpNeverAskAgainCheck);
            uint32_t Answer =
              ConfirmFileOverwrite(
                ParallelFileName, TargetFileName, &FileParams, Answers, &QueryParams, osRemote,
                CopyParam, Params, OperationProgress, EmptyStr);
            switch (Answer)
            {
              case qaCancel:
              case qaNo:
                OperationProgress->SetCancelAtLeast(csCancel);
                break;
            }
          }
        }
      }
    }
  }
}

bool TTerminal::CopyToLocal(
  TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TParallelOperation * ParallelOperation)
{
  DebugAssert(FFileSystem);

  // see scp.c: sink(), tolocal()

  bool Result = false;
  DebugAssert(AFilesToCopy != nullptr);
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  if ((CopyParam->FOnTransferOut) && !FFileSystem->IsCapable(fcTransferOut))
  {
    throw Exception(LoadStr(NOTSUPPORTED));
  }

  FDestFileName = "";
  FMultipleDestinationFiles = false;

  BeginTransaction();
  try__finally
  {
    int64_t TotalSize = 0;
    bool TotalSizeKnown = false;
    FLastProgressLogged = GetTickCount();
    TFileOperationProgressType OperationProgress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));

    std::unique_ptr<TStringList> Files;
    bool ACanParallel =
      CanParallel(CopyParam, AParams, ParallelOperation) &&
      DebugAlwaysTrue(!CopyParam->FOnTransferOut);
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
        TCalculateSizeParams Params;
        Params.Params = csIgnoreErrors;
        Params.CopyParam = CopyParam;
        Params.AllowDirs = ACanParallel || CopyParam->CalculateSize;
        Params.Stats = &Stats;
        if (CalculateFilesSize(AFilesToCopy, TotalSize, Params))
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
      AFilesToCopy ? AFilesToCopy->GetCount() : 0, (AParams & cpTemporary) != 0, ATargetDir, CopyParam->GetCPSLimit(), CopyParam->OnceDoneOperation);

    bool CollectingUsage = false;
    try__finally
    {
      if (TotalSizeKnown)
      {
        if (Configuration->Usage->Collect)
        {
          int32_t CounterTotalSize = TUsage::CalculateCounterSize(TotalSize);
          Configuration->Usage->Inc("Downloads");
          Configuration->Usage->Inc("DownloadedBytes", CounterTotalSize);
          Configuration->Usage->SetMax("MaxDownloadSize", CounterTotalSize);
          CollectingUsage = true;
        }

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
            UnicodeString ParallelFileName;
            int64_t ParallelFileSize = -1;
            CheckParallelFileTransfer(ATargetDir, Files.get(), CopyParam, AParams, ParallelFileName, ParallelFileSize, &OperationProgress);

            if (OperationProgress.Cancel == csContinue)
            {
              if (ParallelFileSize >= 0)
              {
                DebugAssert(ParallelFileSize == TotalSize);
                AParams |= cpNoConfirmation;
              }

              // OnceDoneOperation is not supported
              ParallelOperation->Init(
                Files.release(), ATargetDir, CopyParam, AParams, &OperationProgress, Log->Name, ParallelFileSize);
              UnicodeString ParallelFilePrefix;
              CopyParallel(ParallelOperation, &OperationProgress);
            }
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
      if (CollectingUsage)
      {
        int32_t CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc("DownloadTime", CounterTime);
        Configuration->Usage->SetMax("MaxDownloadTime", CounterTime);
      }
      OperationStop(OperationProgress);
      FOperationProgress = nullptr;
    } end_try__finally
    Files.reset(nullptr);
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

void TTerminal::DoCopyToLocal(
  TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TFileOperationProgressType * OperationProgress, uint32_t AFlags, TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  UnicodeString FullTargetDir = IncludeTrailingBackslash(ATargetDir);
  int32_t Index = 0;
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
        SinkRobust(AbsoluteFileName, File, FullTargetDir, CopyParam, AParams, OperationProgress, AFlags | tfFirstLevel);
        Success = true;
      }
      catch (ESkipFile &E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);
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

void TTerminal::SinkRobust(
  const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ATargetDir,
  const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, uint32_t AFlags)
{
  TDownloadSessionAction Action(GetActionLog());
  try__finally
  {
    bool *FileTransferAny = FLAGSET(AFlags, tfUseFileTransferAny) ? &FFileTransferAny : nullptr;
    bool CanRetry = (!CopyParam->FOnTransferOut);
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
          int32_t Params = dfNoRecursive;
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
  },
  __finally
  {
    // Once we issue <download> we must terminate the data stream
    if (Action.IsValid() && (CopyParam->FOnTransferOut) && DebugAlwaysTrue(GetIsCapable(fcTransferOut)))
    {
      CopyParam->FOnTransferOut(this, nullptr, 0);
    }
  } end_try__finally
}

#if 0

struct TSinkFileParams
{
  UnicodeString TargetDir;
  const TCopyParamType * CopyParam;
  int32_t Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  uint32_t Flags;
};

#endif //if 0

void TTerminal::Sink(
  const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ATargetDir,
  const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, uint32_t AFlags,
  TDownloadSessionAction & Action)
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
    OperationProgress->AddSkippedFileSize(AFile->Resolve()->GetSize());
    throw ESkipFile();
  }

  LogFileDetails(AFileName, AFile->GetModification(), AFile->GetSize(), AFile->LinkedFile);

  OperationProgress->SetFile(AFileName);

  UnicodeString OnlyFileName = base::UnixExtractFileName(AFileName);
  UnicodeString DestFileName = ChangeFileName(CopyParam, OnlyFileName, osRemote, FLAGSET(AFlags, tfFirstLevel));
  UnicodeString DestFullName = ATargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    Action.Cancel();
    if (CanRecurseToDirectory(AFile))
    {
      FileOperationLoopCustom(this, OperationProgress, AFlags, FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName), "",
      [&]()
      {
        DWORD Attrs = ::FileGetAttrFix(ApiPath(DestFullName));
        if (FLAGCLEAR(Attrs, faDirectory))
        {
          ThrowExtException();
        }
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName));

      FileOperationLoopCustom(this, OperationProgress, AFlags, FMTLOAD(CREATE_DIR_ERROR, DestFullName), "",
      [&]()
      {
        THROWOSIFFALSE(::SysUtulsForceDirectories(ApiPath(DestFullName)));
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_DIR_ERROR, DestFullName));

      if (FLAGCLEAR(AParams, cpNoRecurse))
      {
        TSinkFileParams SinkFileParams;
        SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
        SinkFileParams.CopyParam = CopyParam;
        SinkFileParams.Params = AParams;
        SinkFileParams.OperationProgress = OperationProgress;
        SinkFileParams.Skipped = false;
        SinkFileParams.Flags = AFlags & ~(tfFirstLevel | tfAutoResume);

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
    if (CopyParam->FOnTransferOut)
    {
      LogEvent(FORMAT(L"Streaming \"%s\" to local machine started.", AFileName));
    }
    else
    {
      LogEvent(FORMAT("Copying \"%s\" to local directory started.", AFileName));
    }

    // Will we use ASCII or BINARY file transfer?
    UnicodeString BaseFileName = GetBaseFileName(AFileName);
    const TRemoteFile * UltimateFile = AFile->Resolve();
    TFileMasks::TParams MaskParams;
    MaskParams.Size = UltimateFile->Size;
    MaskParams.Modification = AFile->Modification;
    SelectTransferMode(BaseFileName, osRemote, CopyParam, MaskParams);

    // Suppose same data size to transfer as to write
    // (not true with ASCII transfer)
    int64_t TransferSize = (CopyParam->PartSize >= 0) ? CopyParam->PartSize : (UltimateFile->Size - std::max(0LL, CopyParam->PartOffset));
    OperationProgress->SetLocalSize(TransferSize);
    if (IsFileEncrypted(AFileName))
    {
      TransferSize += TEncryption::GetOverhead();
    }
    OperationProgress->SetTransferSize(TransferSize);

    int32_t Attrs = 0;
    UnicodeString LogFileName;
    if (CopyParam->FOnTransferOut)
    {
      Attrs = -1;
      LogFileName = L"-";
    }
    else
    {
      FileOperationLoopCustom(this, OperationProgress, AFlags, FMTLOAD(NOT_FILE_ERROR, DestFullName), "",
      [&]()
      {
        Attrs = ::FileGetAttrFix(ApiPath(DestFullName));
        if ((Attrs >= 0) && FLAGSET(Attrs, faDirectory))
        {
          ThrowExtException();
        }
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(NOT_FILE_ERROR, DestFullName));
      LogFileName = ::ExpandUNCFileName(DestFullName);
    }

    FFileSystem->Sink(
      AFileName, AFile, ATargetDir, DestFileName, Attrs, CopyParam, AParams, OperationProgress, AFlags, Action);

    LogFileDone(OperationProgress, LogFileName, Action);
    OperationProgress->Succeeded();
  }
}

void TTerminal::UpdateTargetAttrs(
  const UnicodeString & ADestFullName, const TRemoteFile * AFile, const TCopyParamType * CopyParam, int32_t Attrs)
{
  if (Attrs == static_cast<int32_t>(-1))
  {
    Attrs = faArchive;
  }
  uint32_t NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
  if ((NewAttrs & Attrs) != NewAttrs)
  {
    FileOperationLoopCustom(this, FOperationProgress, folAllowSkip,
      FMTLOAD(CANT_SET_ATTRS, ADestFullName), "",
    [&]()
    {
      THROWOSIFFALSE(::SysUtulsFileSetAttr(ApiPath(ADestFullName), static_cast<DWORD>(Attrs | NewAttrs)) == 0);
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, ADestFullName));
  }
}

void TTerminal::UpdateTargetTime(HANDLE Handle, TDateTime Modification, TDSTMode DSTMode)
{
  LogEvent(FORMAT("Preserving timestamp [%s]", ::StandardTimestamp(Modification)));
  FILETIME WrTime = DateTimeToFileTime(Modification, DSTMode);
  if (!::SetFileTime(Handle, nullptr, nullptr, &WrTime))
  {
    int32_t Error = GetLastError();
    LogEvent(FORMAT("Preserving timestamp failed, ignoring: %s", ::SysErrorMessageForError(Error)));
  }
}

void TTerminal::SinkFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * AParam)
{
  TSinkFileParams * Params = cast_to<TSinkFileParams>(AParam);
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
      TSuspendFileOperationProgress Suspend(Params->OperationProgress); nb::used(Suspend);
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

void TTerminal::ReflectSettings() const
{
  DebugAssert(FLog != nullptr);
  FLog->ReflectSettings();
  if (FActionLogOwned)
  {
    DebugAssert(FActionLog != nullptr);
    FActionLog->ReflectSettings();
  }
  // also FTunnelLog ?
}

void TTerminal::CollectUsage()
{
  switch (GetSessionData()->GetFSProtocol())
  {
    case fsSCPonly:
//      Configuration->Usage->Inc("OpenedSessionsSCP");
      break;

    case fsSFTP:
    case fsSFTPonly:
//      Configuration->Usage->Inc("OpenedSessionsSFTP");
      break;

    case fsFTP:
      if (GetSessionData()->GetFtps() == ftpsNone)
      {
//        Configuration->Usage->Inc("OpenedSessionsFTP");
      }
      else
      {
//        Configuration->Usage->Inc("OpenedSessionsFTPS");
      }
      break;

    case fsWebDAV:
      if (GetSessionData()->GetFtps() == ftpsNone)
      {
//        Configuration->Usage->Inc("OpenedSessionsWebDAV");
      }
      else
      {
//        Configuration->Usage->Inc("OpenedSessionsWebDAVS");
      }
      break;

    case fsS3:
      // Configuration->Usage->Inc("OpenedSessionsS3");
      break;
  }

  if (GetConfiguration()->GetLogging() && GetConfiguration()->GetLogToFile())
  {
//    Configuration->Usage->Inc("OpenedSessionsLogToFile2");
  }

  if (GetConfiguration()->GetLogActions())
  {
//    Configuration->Usage->Inc("OpenedSessionsXmlLog");
  }

  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(""));
  if (!GetSessionData()->IsSame(FactoryDefaults.get(), true))
  {
//    Configuration->Usage->Inc("OpenedSessionsAdvanced");
  }

  if (GetSessionData()->GetProxyMethod() != ::pmNone)
  {
//    Configuration->Usage->Inc("OpenedSessionsProxy");
  }
  if (GetSessionData()->GetFtpProxyLogonType() > 0)
  {
//    Configuration->Usage->Inc("OpenedSessionsFtpProxy");
  }
  if (IsEncryptingFiles())
  {
    Configuration->Usage->Inc("OpenedSessionsEncrypted");
  }

  if (GetSessionData()->GetSendBuf() == 0)
  {
    Configuration->Usage->Inc(L"OpenedSessionsNoSendBuf");
  }

  FCollectFileSystemUsage = true;
}

static UnicodeString FormatCertificateData(const UnicodeString & Fingerprint, int32_t Failures)
{
  return FORMAT("%s;%2.2X", Fingerprint, Failures);
}

bool TTerminal::VerifyCertificate(
  const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
  const UnicodeString & FingerprintSHA1, const UnicodeString & FingerprintSHA256,
  const UnicodeString & CertificateSubject, int32_t Failures)
{
  bool Result = false;

  UnicodeString CertificateDataSHA1 = FormatCertificateData(FingerprintSHA1, Failures);
  UnicodeString CertificateDataSHA256 = FormatCertificateData(FingerprintSHA256, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smRead);

  if (Storage->OpenSubKey(CertificateStorageKey, false))
  {
    if (Storage->ValueExists(SiteKey))
    {
      UnicodeString CachedCertificateData = Storage->ReadString(SiteKey, "");
      if (SameChecksum(CertificateDataSHA1, CachedCertificateData, false) ||
          SameChecksum(CertificateDataSHA256, CachedCertificateData, false))
      {
        LogEvent(FORMAT("Certificate for \"%s\" matches cached fingerprint and failures", CertificateSubject));
        Result = true;
      }
    }
    else if (Storage->ValueExists(FingerprintSHA1) || Storage->ValueExists(FingerprintSHA256))
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
      else if (SameChecksum(ExpectedKey, FingerprintSHA1, false) ||
               SameChecksum(ExpectedKey, FingerprintSHA256, false))
      {
        LogEvent(FORMAT("Certificate for \"%s\" matches configured fingerprint", CertificateSubject));
        Result = true;
      }
    }
  }

  return Result;
}

bool TTerminal::ConfirmCertificate(
  TSessionInfo & SessionInfo, int32_t Failures, const UnicodeString & CertificateStorageKey, bool CanRemember)
{
  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text =
    FORMAT(L"SHA-256: %s\nSHA-1: %s", SessionInfo.CertificateFingerprintSHA256, SessionInfo.CertificateFingerprintSHA1);

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
      CertificateStorageKey, GetSessionData()->GetSiteKey(),
        SessionInfo.CertificateFingerprintSHA1, SessionInfo.CertificateFingerprintSHA256, Failures);
    Result = true;
    break;

  case qaNo:
    Result = true;
    break;

  case qaCancel:
    // Configuration->Usage->Inc("HostNotVerified");
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
      GetSessionData()->GetSiteKey(), TlsFingerprintType, SessionInfo.CertificateFingerprintSHA256);
  }
  return Result;
}

void TTerminal::CacheCertificate(
  const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
  const UnicodeString & DebugUsedArg(FingerprintSHA1), const UnicodeString & FingerprintSHA256, int32_t Failures)
{
  UnicodeString CertificateData = FormatCertificateData(FingerprintSHA256, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(CertificateStorageKey, true))
  {
    Storage->WriteString(SiteKey, CertificateData);
  }
}

// Shared implementation for WebDAV and S3
bool TTerminal::VerifyOrConfirmHttpCertificate(
  const UnicodeString & AHostName, int32_t APortNumber, const TNeonCertificateData & AData, bool CanRemember,
  TSessionInfo & SessionInfo)
{
  TNeonCertificateData Data = AData;
  SessionInfo.CertificateFingerprintSHA1 = Data.FingerprintSHA1;
  SessionInfo.CertificateFingerprintSHA256 = Data.FingerprintSHA256;

  bool Result;
  if (SessionData->FingerprintScan)
  {
    Result = false;
  }
  else
  {
    LogEvent(0, CertificateVerificationMessage(Data));

    UnicodeString WindowsValidatedMessage;
    // Side effect is that NE_SSL_UNTRUSTED is removed from Failure, before we call VerifyCertificate,
    // as it compares failures against a cached failures that do not include NE_SSL_UNTRUSTED
    // (if the certificate is trusted by Windows certificate store).
    // But we will log that result only if we actually use it for the decision.
    bool WindowsValidated = NeonWindowsValidateCertificateWithMessage(Data, WindowsValidatedMessage);

    UnicodeString SiteKey = TSessionData::FormatSiteKey(AHostName, APortNumber);
    Result =
      VerifyCertificate(
        HttpsCertificateStorageKey, SiteKey, Data.FingerprintSHA1, Data.FingerprintSHA256, Data.Subject, Data.Failures);

    if (Result)
    {
      SessionInfo.CertificateVerifiedManually = true;
    }
    else
    {
      Result = WindowsValidated;
      LogEvent(0, WindowsValidatedMessage);
    }

    SessionInfo.Certificate = CertificateSummary(Data, AHostName);

    if (!Result)
    {
      if (ConfirmCertificate(SessionInfo, Data.Failures, HttpsCertificateStorageKey, CanRemember))
      {
        Result = true;
        SessionInfo.CertificateVerifiedManually = true;
      }
    }
  }

  return Result;
}

void TTerminal::CollectTlsUsage(const UnicodeString & TlsVersionStr)
{
  // see SSL_get_version() in OpenSSL ssl_lib.c
  if (TlsVersionStr == L"TLSv1.3")
  {
    Configuration->Usage->Inc(L"OpenedSessionsTLS13");
  }
  else if (TlsVersionStr == L"TLSv1.2")
  {
    Configuration->Usage->Inc("OpenedSessionsTLS12");
  }
  else if (TlsVersionStr == "TLSv1.1")
  {
    Configuration->Usage->Inc("OpenedSessionsTLS11");
  }
  else if (TlsVersionStr == "TLSv1")
  {
    Configuration->Usage->Inc("OpenedSessionsTLS10");
  }
  else if (TlsVersionStr == "SSLv3")
  {
    Configuration->Usage->Inc("OpenedSessionsSSL30");
  }
  else if (TlsVersionStr == "SSLv2")
  {
    Configuration->Usage->Inc("OpenedSessionsSSL20");
  }
  else
  {
    DebugFail();
  }
}

bool TTerminal::LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey)
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

        Passphrase = "";
        if (PromptUser(
              GetSessionData(), pkPassphrase,
              LoadStr(CERTIFICATE_PASSPHRASE_TITLE), "",
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

UnicodeString TTerminal::GetBaseFileName(const UnicodeString & AFileName) const
{
  UnicodeString FileName = AFileName;

  if (FSessionData->GetTrimVMSVersions())
  {
    int32_t P = FileName.LastDelimiter(L";");
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
  }
  return FileName;
}

UnicodeString TTerminal::ChangeFileName(const TCopyParamType * CopyParam,
  const UnicodeString & AFileName, TOperationSide Side, bool FirstLevel) const
{
  UnicodeString FileName = GetBaseFileName(AFileName);
  if (CopyParam)
  {
    FileName = CopyParam->ChangeFileName(FileName, Side, FirstLevel);
  }
  return FileName;
}

bool TTerminal::CanRecurseToDirectory(const TRemoteFile * AFile) const
{
  return !AFile->GetIsSymLink() || FSessionData->GetFollowDirectorySymlinks();
}

typename TTerminal::TEncryptedFileNames::const_iterator TTerminal::GetEncryptedFileName(const UnicodeString & APath)
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

      if (FEncryptedFileNames.find(APath) == FEncryptedFileNames.end())
      {
        FEncryptedFileNames[APath] = base::UnixExtractFileName(APath);
        LogEvent(2, FORMAT("Name of file '%s' assumed not to be encrypted", APath));
      }
    }

    FFoldersScannedForEncryptedFiles.insert(FileDir);
  }

  TEncryptedFileNames::const_iterator Result = FEncryptedFileNames.find(APath);
  return Result;
}

bool TTerminal::IsFileEncrypted(const UnicodeString & APath, bool EncryptNewFiles)
{
  // can be optimized
  bool Result = (EncryptFileName(APath, EncryptNewFiles) != APath);
  return Result;
}

UnicodeString TTerminal::EncryptFileName(const UnicodeString & APath, bool EncryptNewFiles)
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
        LogEvent(2, FORMAT("Name of file '%s' encrypted as '%s'", APath, FileName));
      }
    }

    FileDir = EncryptFileName(FileDir, EncryptNewFiles);
    Result = base::UnixCombinePaths(FileDir, FileName);
  }
  return Result;
}

UnicodeString TTerminal::DecryptFileName(const UnicodeString & Path, bool DecryptFullPath, bool DontCache)
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
    }

    if (Encrypted || DecryptFullPath) // DecryptFullPath is just an optimization
    {
      UnicodeString FileDir = base::UnixExtractFileDir(Path);
      FileDir = DecryptFileName(FileDir, DecryptFullPath, DontCache);
      Result = base::UnixCombinePaths(FileDir, FileName);
      // Encryption.FreeContext();
    }

    TEncryptedFileNames::iterator Iter = FEncryptedFileNames.find(Result);
    bool NotCached = (Iter == FEncryptedFileNames.end());
    if (!DontCache && (Encrypted || NotCached))
    {
      if (Encrypted && (NotCached || (Iter->second != FileNameEncrypted)))
      {
        LogEvent(2, FORMAT("Name of file '%s' decrypted from '%s'", Result, FileNameEncrypted));
      }
      // This may overwrite another variant of encryption
      FEncryptedFileNames[Result] = FileNameEncrypted;
    }
  }
  return Result;
}

TRemoteFile * TTerminal::CheckRights(const UnicodeString & EntryType, const UnicodeString & FileName, bool & WrongRights)
{
  std::unique_ptr<TRemoteFile> File;
  try
  {
    LogEvent(FORMAT(L"Checking %s \"%s\"...", LowerCase(EntryType), FileName));
    File.reset(ReadFile(FileName));
    int32_t ForbiddenRights = TRights::rfGroupWrite | TRights::rfOtherWrite;
    if ((File->Rights->Number & ForbiddenRights) != 0)
    {
      LogEvent(FORMAT(L"%s \"%s\" exists, but has incorrect permissions %s.", EntryType, FileName, File->Rights->Octal));
      WrongRights = true;
    }
    else
    {
      LogEvent(FORMAT(L"%s \"%s\" exists and has correct permissions %s.", EntryType, FileName, File->Rights->Octal));
    }
  }
  catch (Exception & DebugUsedArg(E))
  {
  }
  return File.release();
}

UnicodeString TTerminal::UploadPublicKey(const UnicodeString & FileName)
{
  UnicodeString Result;

  UnicodeString TemporaryDir;
  bool PrevAutoReadDirectory = FAutoReadDirectory;
  bool PrevExceptionOnFail = ExceptionOnFail;

  UnicodeString AuthorizedKeysFilePath = FORMAT(L"%s/%s", OpensshFolderName, OpensshAuthorizedKeysFileName);

  try__finally
  {
    FAutoReadDirectory = false;
    ExceptionOnFail = true;

    Log->AddSeparator();

    UnicodeString Comment;
    bool UnusedHasCertificate;
    UnicodeString Line = GetPublicKeyLine(FileName, Comment, UnusedHasCertificate);

    LogEvent(FORMAT(L"Adding public key line to \"%s\" file:\n%s", AuthorizedKeysFilePath, Line));

    UnicodeString SshFolderAbsolutePath = base::UnixIncludeTrailingBackslash(GetHomeDirectory()) + OpensshFolderName;
    bool WrongRights = false;
    std::unique_ptr<TRemoteFile> SshFolderFile(CheckRights(L"Folder", SshFolderAbsolutePath, WrongRights));
    if (SshFolderFile.get() == nullptr)
    {
      TRights SshFolderRights;
      SshFolderRights.Number = TRights::rfUserRead | TRights::rfUserWrite | TRights::rfUserExec;
      TRemoteProperties SshFolderProperties;
      SshFolderProperties.Rights = SshFolderRights;
      SshFolderProperties.Valid = TValidProperties<TValidProperty>() << vpRights;

      LogEvent(FORMAT(L"Trying to create \"%s\" folder with permissions %s...", OpensshFolderName, SshFolderRights.Octal));
      RemoteCreateDirectory(SshFolderAbsolutePath, &SshFolderProperties);
    }

    TemporaryDir = ExcludeTrailingBackslash(Configuration->TemporaryDir());
    if (!::SysUtulsForceDirectories(ApiPath(TemporaryDir)))
    {
      throw EOSExtException(FMTLOAD(CREATE_TEMP_DIR_ERROR, TemporaryDir));
    }
    UnicodeString TemporaryAuthorizedKeysFile = IncludeTrailingBackslash(TemporaryDir) + OpensshAuthorizedKeysFileName;

    UnicodeString AuthorizedKeysFileAbsolutePath = base::UnixIncludeTrailingBackslash(SshFolderAbsolutePath) + OpensshAuthorizedKeysFileName;

    bool Updated = true;
    TCopyParamType CopyParam; // Use factory defaults
    CopyParam.ResumeSupport = rsOff; // not to break the permissions
    CopyParam.PreserveTime = false; // not needed

    UnicodeString AuthorizedKeys;
    std::unique_ptr<TRemoteFile> AuthorizedKeysFileFile(CheckRights(L"File", AuthorizedKeysFileAbsolutePath, WrongRights));
    if (AuthorizedKeysFileFile.get() != nullptr)
    {
      AuthorizedKeysFileFile->FullFileName = AuthorizedKeysFileAbsolutePath;
      std::unique_ptr<TStrings> Files(std::make_unique<TStringList>());
      Files->AddObject(AuthorizedKeysFileAbsolutePath, AuthorizedKeysFileFile.get());
      LogEvent(FORMAT(L"Downloading current \"%s\" file...", OpensshAuthorizedKeysFileName));
      CopyToLocal(Files.get(), TemporaryDir, &CopyParam, cpNoConfirmation, nullptr);
      // Overload with Encoding parameter work incorrectly, when used on a file without BOM
      AuthorizedKeys = ReadAllText(TemporaryAuthorizedKeysFile);

      std::unique_ptr<TStrings> AuthorizedKeysLines(TextToStringList(AuthorizedKeys));
      int32_t P = Line.Pos(L" ");
      if (DebugAlwaysTrue(P > 0))
      {
        P = PosEx(L" ", Line, P + 1);
      }
      UnicodeString Prefix = Line.SubString(1, P); // including the space
      for (int32_t Index = 0; Index < AuthorizedKeysLines->Count; Index++)
      {
        if (::StartsStr(Prefix, AuthorizedKeysLines->GetString(Index)))
        {
          LogEvent(FORMAT(L"\"%s\" file already contains public key line:\n%s", OpensshAuthorizedKeysFileName, AuthorizedKeysLines->GetString(Index)));
          Updated = false;
        }
      }

      if (Updated)
      {
        LogEvent(FORMAT(L"\"%s\" file does not contain the public key line yet.", OpensshAuthorizedKeysFileName));
        if (!::EndsStr(L"\n", AuthorizedKeys))
        {
          LogEvent(FORMAT(L"Adding missing trailing new line to \"%s\" file...", OpensshAuthorizedKeysFileName));
          AuthorizedKeys += L"\n";
        }
      }
    }
    else
    {
      LogEvent(FORMAT(L"Creating new \"%s\" file...", OpensshAuthorizedKeysFileName));
      CopyParam.PreserveRights = true;
      CopyParam.Rights.Number = TRights::rfUserRead | TRights::rfUserWrite;
    }

    if (Updated)
    {
      AuthorizedKeys += Line + L"\n";
      // Overload without Encoding parameter uses TEncoding::UTF8, but does not write BOM, what we want
      WriteAllText(TemporaryAuthorizedKeysFile, AuthorizedKeys);
      std::unique_ptr<TStrings> Files(std::make_unique<TStringList>());
      Files->Add(TemporaryAuthorizedKeysFile);
      LogEvent(FORMAT(L"Uploading updated \"%s\" file...", OpensshAuthorizedKeysFileName));
      CopyToRemote(Files.get(), SshFolderAbsolutePath, &CopyParam, cpNoConfirmation, nullptr);
    }

    Result = FMTLOAD(PUBLIC_KEY_UPLOADED, Comment);
    if (WrongRights)
    {
      Result += L"\n\n" + FMTLOAD(PUBLIC_KEY_PERMISSIONS, AuthorizedKeysFilePath);
    }
  },
  __finally
  {
    FAutoReadDirectory = PrevAutoReadDirectory;
    ExceptionOnFail = PrevExceptionOnFail;
    if (!TemporaryDir.IsEmpty())
    {
      RecursiveDeleteFile(ExcludeTrailingBackslash(TemporaryDir));
    }
  } end_try__finally
  return Result;
}

bool TTerminal::IsValidFile(TRemoteFile * File) const
{
  return
    !File->FileName().IsEmpty() &&
    (base::IsUnixRootPath(File->FileName()) || base::UnixExtractFileDir(File->FileName()).IsEmpty());
}

UnicodeString TTerminal::CutFeature(UnicodeString & Buf)
{
  UnicodeString Result;
  if (Buf.SubString(1, 1) == L"\"")
  {
    Buf.Delete(1, 1);
    int32_t P = Buf.Pos(L"\",");
    if (P == 0)
    {
      Result = Buf;
      Buf = UnicodeString();
      // there should be the ending quote, but if not, just do nothing
      if (Result.SubString(Result.Length(), 1) == L"\"")
      {
        Result.SetLength(Result.Length() - 1);
      }
    }
    else
    {
      Result = Buf.SubString(1, P - 1);
      Buf.Delete(1, P + 1);
    }
    Buf = Buf.TrimLeft();
  }
  else
  {
    Result = CutToChar(Buf, L',', true);
  }
  return Result;
}

TStrings * TTerminal::ProcessFeatures(TStrings * Features)
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  UnicodeString FeaturesOverride = SessionData->ProtocolFeatures().Trim();
  if (FeaturesOverride.SubString(1, 1) == L"*")
  {
    FeaturesOverride.Delete(1, 1);
    while (!FeaturesOverride.IsEmpty())
    {
      UnicodeString Feature = CutFeature(FeaturesOverride);
      Result->Add(Feature);
    }
  }
  else
  {
    std::unique_ptr<TStrings> DeleteFeatures(CreateSortedStringList());
    std::unique_ptr<TStrings> AddFeatures(std::make_unique<TStringList>());
    while (!FeaturesOverride.IsEmpty())
    {
      UnicodeString Feature = CutFeature(FeaturesOverride);
      if (Feature.SubString(1, 1) == L"-")
      {
        Feature.Delete(1, 1);
        DeleteFeatures->Add(Feature.LowerCase());
      }
      else
      {
        if (Feature.SubString(1, 1) == L"+")
        {
          Feature.Delete(1, 1);
        }
        AddFeatures->Add(Feature);
      }
    }

    for (int32_t Index = 0; Index < Features->Count; Index++)
    {
      UnicodeString Feature = Features->Strings[Index];
      if (DeleteFeatures->IndexOf(Feature) < 0)
      {
        Result->Add(Feature);
      }
    }

    Result->AddStrings(AddFeatures.get());
  }
  return Result.release();
}

void TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  const TDateTime & Modification)
{
  FILETIME WrTime = ::DateTimeToFileTime(Modification,
      GetSessionData()->GetDSTMode());
  SetLocalFileTime(LocalFileName, nullptr, &WrTime);
}

void TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  FILETIME * AcTime, FILETIME * WrTime)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, LocalFileName), "",
  [&]()
  {
    HANDLE LocalFileHandle;
    SCOPE_EXIT
    {
      SAFE_CLOSE_HANDLE(LocalFileHandle);
    };
    this->TerminalOpenLocalFile(LocalFileName, GENERIC_WRITE, nullptr,
      &LocalFileHandle, nullptr, nullptr, nullptr, nullptr);
    THROWOSIFFALSE(::SetFileTime(LocalFileHandle, nullptr, AcTime, WrTime));
  });
}

HANDLE TTerminal::TerminalCreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
  DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (GetOnCreateLocalFile())
  {
    return GetOnCreateLocalFile()(ApiPath(LocalFileName), DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes);
  }
  return ::CreateFile(ApiPath(LocalFileName).c_str(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, nullptr);
}

DWORD TTerminal::GetLocalFileAttributes(const UnicodeString & LocalFileName) const
{
  if (GetOnGetLocalFileAttributes())
  {
    return GetOnGetLocalFileAttributes()(ApiPath(LocalFileName));
  }
  return ::FileGetAttrFix(LocalFileName);
}

bool TTerminal::SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
{
  if (GetOnSetLocalFileAttributes())
  {
    return GetOnSetLocalFileAttributes()(ApiPath(LocalFileName), FileAttributes);
  }
  return SysUtulsFileSetAttr(LocalFileName, FileAttributes);
}

bool TTerminal::MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
{
  if (GetOnMoveLocalFile())
  {
    return GetOnMoveLocalFile()(LocalFileName, NewLocalFileName, Flags);
  }
  return ::MoveFileEx(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), Flags) != FALSE;
}

bool TTerminal::RemoveLocalDirectory(const UnicodeString & LocalDirName)
{
  if (GetOnRemoveLocalDirectory())
  {
    return GetOnRemoveLocalDirectory()(LocalDirName);
  }
  return SysUtulsRemoveDir(LocalDirName);
}

bool TTerminal::CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (GetOnCreateLocalDirectory())
  {
    return GetOnCreateLocalDirectory()(LocalDirName, SecurityAttributes);
  }
  return SysUtulsCreateDir(LocalDirName, SecurityAttributes);
}

bool TTerminal::CheckForEsc() const
{
  if (FOnCheckForEsc)
  {
    return FOnCheckForEsc();
  }
  return (FOperationProgress && FOperationProgress->GetCancel() == csCancel);
}

bool TTerminal::IsThisOrChild(TTerminal * ATerminal) const
{
  return
    (this == ATerminal) ||
    ((FCommandSession != nullptr) && (FCommandSession == ATerminal));
}


TSecondaryTerminal::TSecondaryTerminal(TObjectClassId Kind) noexcept :
  TTerminal(Kind)
{
}

void TSecondaryTerminal::Init(
  TTerminal * MainTerminal, TSessionData * ASessionData, TConfiguration * AConfiguration,
  const UnicodeString & AName, TActionLog * ActionLog)
{
  FMainTerminal = MainTerminal;
  TTerminal::Init(ASessionData, AConfiguration, ActionLog);
  GetLog()->SetParent(FMainTerminal->GetLog(), AName);
  GetSessionData()->NonPersistent();
  DebugAssert(FMainTerminal != nullptr);
  FMainTerminal->FSecondaryTerminals++;
  if (SessionData->FTunnelLocalPortNumber != 0)
  {
    SessionData->FTunnelLocalPortNumber = SessionData->FTunnelLocalPortNumber + FMainTerminal->FSecondaryTerminals;
  }
  if (!FMainTerminal->TerminalGetUserName().IsEmpty())
  {
    GetSessionData()->SessionSetUserName(FMainTerminal->TerminalGetUserName());
  }
}

void TSecondaryTerminal::UpdateFromMain()
{
  if ((FFileSystem != nullptr) && (FMainTerminal->FFileSystem != nullptr))
  {
    FFileSystem->UpdateFromMain(FMainTerminal->FFileSystem.get());
  }
}

void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  DebugAssert(FileList != nullptr);
}

void TSecondaryTerminal::DirectoryModified(const UnicodeString & APath,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(APath, SubDirs);
}

TTerminal * TSecondaryTerminal::GetPrimaryTerminal()
{
  return FMainTerminal;
}

TTerminalList::TTerminalList(TConfiguration * AConfiguration) noexcept :
  TObjectList(OBJECT_CLASS_TTerminalList),
  FConfiguration(AConfiguration)
{
  DebugAssert(FConfiguration);
}

TTerminalList::~TTerminalList() noexcept
{
  DebugAssert(GetCount() == 0);
}

TTerminal * TTerminalList::CreateTerminal(TSessionData * Data)
{
  TTerminal *Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}

TTerminal * TTerminalList::NewTerminal(TSessionData * Data)
{
  TTerminal *Result = CreateTerminal(Data);
  Add(Result);
  return Result;
}

void TTerminalList::FreeTerminal(TTerminal * Terminal)
{
  DebugAssert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}

void TTerminalList::FreeAndNullTerminal(TTerminal *& Terminal)
{
  TTerminal *T = Terminal;
  Terminal = nullptr;
  FreeTerminal(T);
}

TTerminal * TTerminalList::GetTerminal(int32_t Index)
{
  return GetAs<TTerminal>(Index);
}

void TTerminalList::RecryptPasswords()
{
  for (int32_t Index = 0; Index < GetCount(); Index++)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}


TLocalFileHandle::TLocalFileHandle() noexcept
{
#if 0
  Handle = 0;
  Attrs = 0;
  MTime = 0;
  ATime = 0;
  Size = 0;
  Directory = false;
#endif // #if 0
}

TLocalFileHandle::~TLocalFileHandle() noexcept
{
  Release();
}

void TLocalFileHandle::Release()
{
  if (Handle != 0)
  {
    Close();
  }
}

void TLocalFileHandle::Dismiss()
{
  if (DebugAlwaysTrue(Handle != 0))
  {
    Handle = 0;
  }
}

void TLocalFileHandle::Close()
{
  __removed if (DebugAlwaysTrue(Handle != 0))
  {
    SAFE_CLOSE_HANDLE(Handle);
    Dismiss();
  }
}

UnicodeString GetSessionUrl(const TTerminal * Terminal, bool WithUserName)
{
  UnicodeString Result;
  const TSessionInfo & SessionInfo = Terminal->GetSessionInfo();
  const TSessionData * SessionData = Terminal->GetSessionData();
  UnicodeString Protocol = SessionInfo.ProtocolBaseName;
  UnicodeString HostName = SessionData->GetHostNameExpanded();
  UnicodeString UserName = SessionData->GetUserNameExpanded();
  int32_t Port = Terminal->GetSessionData()->GetPortNumber();
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

///* TODO : Better user interface (query to user) */
void FileOperationLoopCustom(TTerminal * Terminal,
  TFileOperationProgressType * OperationProgress,
  uint32_t Flags, const UnicodeString & Message,
  const UnicodeString & HelpKeyword,
  std::function<void()> Operation)
{
  bool DoRepeat;
  do
  {
    DoRepeat = false;
    try
    {
      Operation();
    }
    catch(EAbort &)
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
        E, OperationProgress, Message, Flags & folAllowSkip, "", HelpKeyword);
      DoRepeat = true;
    }
  }
  while (DoRepeat);
}
