
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
#ifndef NO_FILEZILLA
#include "FtpFileSystem.h"
#endif
#include "WebDAVFileSystem.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Queue.h"
#include <openssl/pkcs12.h>
#include <openssl/err.h>

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif

///* TODO : Better user interface (query to user) */
void FileOperationLoopCustom(TTerminal * Terminal,
  TFileOperationProgressType * OperationProgress,
  bool AllowSkip, UnicodeString Message,
  UnicodeString HelpKeyword,
  const std::function<void()> & Operation)
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
    catch (Exception & E)
    {
      Terminal->FileOperationLoopQuery(
        E, OperationProgress, Message, AllowSkip, L"", HelpKeyword);
      DoRepeat = true;
    }
  }
  while (DoRepeat);
}

#if 0
#pragma package(smart_init)

#define FILE_OPERATION_LOOP_TERMINAL this
#endif // #if 0


class TLoopDetector : public TObject
{
public:
  TLoopDetector();
  void RecordVisitedDirectory(UnicodeString Directory);
  bool IsUnvisitedDirectory(UnicodeString Directory);

private:
  std::unique_ptr<TStringList> FVisitedDirectories;
};

TLoopDetector::TLoopDetector()
{
  FVisitedDirectories.reset(CreateSortedStringList());
}

void TLoopDetector::RecordVisitedDirectory(UnicodeString Directory)
{
  UnicodeString VisitedDirectory = ::ExcludeTrailingBackslash(Directory);
  FVisitedDirectories->Add(VisitedDirectory);
}

bool TLoopDetector::IsUnvisitedDirectory(UnicodeString Directory)
{
  bool Result = (FVisitedDirectories->IndexOf(Directory) < 0);

  if (Result)
  {
    RecordVisitedDirectory(Directory);
  }

  return Result;
}


struct TMoveFileParams : public TObject
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TMoveFileParams); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TMoveFileParams) || TObject::is(Kind); }
public:
  TMoveFileParams() : TObject(OBJECT_CLASS_TMoveFileParams)
  {
  }

  UnicodeString Target;
  UnicodeString FileMask;
};

struct TFilesFindParams : public TObject
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFilesFindParams); }
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

TCalculateSizeStats::TCalculateSizeStats() :
  Files(0),
  Directories(0),
  SymLinks(0),
  FoundFiles(nullptr)
{
//  memset(this, 0, sizeof(*this));
}

TSynchronizeOptions::TSynchronizeOptions() :
  Filter(nullptr)
{
//  memset(this, 0, sizeof(*this));
}

TSynchronizeOptions::~TSynchronizeOptions()
{
  SAFE_DESTROY(Filter);
}

bool TSynchronizeOptions::MatchesFilter(UnicodeString AFileName) const
{
  bool Result = true;
  if (Filter)
  {
    intptr_t FoundIndex = 0;
    Result = Filter->Find(AFileName, FoundIndex);
  }
  return Result;
}

TSpaceAvailable::TSpaceAvailable() :
  BytesOnDevice(0),
  UnusedBytesOnDevice(0),
  BytesAvailableToUser(0),
  UnusedBytesAvailableToUser(0),
  BytesPerAllocationUnit(0)
{
//  memset(this, 0, sizeof(*this));
}

TChecklistItem::TChecklistItem() :
  TObject(OBJECT_CLASS_TChecklistItem),
  Action(saNone),
  IsDirectory(false),
  ImageIndex(-1),
  Checked(true),
  RemoteFile(nullptr)
{
  Local.ModificationFmt = mfFull;
  Local.Modification = 0;
  Local.Size = 0;
  Remote.ModificationFmt = mfFull;
  Remote.Modification = 0;
  Remote.Size = 0;
  FLocalLastWriteTime.dwHighDateTime = 0;
  FLocalLastWriteTime.dwLowDateTime = 0;
}

TChecklistItem::~TChecklistItem()
{
  SAFE_DESTROY(RemoteFile);
}

UnicodeString TChecklistItem::GetFileName() const
{
  if (!Remote.FileName.IsEmpty())
  {
    return Remote.FileName;
  }
  else
  {
    DebugAssert(!Local.FileName.IsEmpty());
    return Local.FileName;
  }
}


TSynchronizeChecklist::TSynchronizeChecklist()
{
}

TSynchronizeChecklist::~TSynchronizeChecklist()
{
  for (intptr_t Index = 0; Index < FList.GetCount(); ++Index)
  {
    TChecklistItem * Item = FList.GetAs<TChecklistItem>(Index);
    SAFE_DESTROY(Item);
  }
}

void TSynchronizeChecklist::Add(TChecklistItem * Item)
{
  FList.Add(Item);
}

intptr_t TSynchronizeChecklist::Compare(const void * AItem1, const void * AItem2)
{
  const TChecklistItem * Item1 = get_as<TChecklistItem>(AItem1);
  const TChecklistItem * Item2 = get_as<TChecklistItem>(AItem2);

  intptr_t Result;
  if (!Item1->Local.Directory.IsEmpty())
  {
    Result = ::AnsiCompareText(Item1->Local.Directory, Item2->Local.Directory);
  }
  else
  {
    DebugAssert(!Item1->Remote.Directory.IsEmpty());
    Result = ::AnsiCompareText(Item1->Remote.Directory, Item2->Remote.Directory);
  }

  if (Result == 0)
  {
    Result = ::AnsiCompareText(Item1->GetFileName(), Item2->GetFileName());
  }

  return Result;
}

void TSynchronizeChecklist::Sort()
{
  FList.Sort(Compare);
}

intptr_t TSynchronizeChecklist::GetCount() const
{
  return FList.GetCount();
}

const TChecklistItem * TSynchronizeChecklist::GetItem(intptr_t Index) const
{
  return FList.GetAs<TChecklistItem>(Index);
}

void TSynchronizeChecklist::Update(const TChecklistItem * Item, bool Check, TChecklistAction Action)
{
  // TSynchronizeChecklist owns non-const items so it can manipulate them freely,
  // const_cast here is just an optimization
  TChecklistItem * MutableItem = const_cast<TChecklistItem *>(Item);
  DebugAssert(FList.IndexOf(MutableItem) >= 0);
  MutableItem->Checked = Check;
  MutableItem->Action = Action;
}

TChecklistAction TSynchronizeChecklist::Reverse(TChecklistAction Action)
{
  switch (Action)
  {
  case saUploadNew:
    return saDeleteLocal;

  case saDownloadNew:
    return saDeleteRemote;

  case saUploadUpdate:
    return saDownloadUpdate;

  case saDownloadUpdate:
    return saUploadUpdate;

  case saDeleteRemote:
    return saDownloadNew;

  case saDeleteLocal:
    return saUploadNew;

  default:
  case saNone:
    DebugFail();
    return saNone;
  }
}


class TTunnelThread : public TSimpleThread
{
NB_DISABLE_COPY(TTunnelThread)
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TTunnelThread); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTunnelThread) || TSimpleThread::is(Kind); }
public:
  explicit TTunnelThread(TSecureShell * SecureShell);
  virtual ~TTunnelThread();

  virtual void InitTunnelThread();
  virtual void Terminate() override;

protected:
  virtual void Execute() override;

private:
  TSecureShell * FSecureShell;
  bool FTerminated;
};

TTunnelThread::TTunnelThread(TSecureShell * SecureShell) :
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

TTunnelThread::~TTunnelThread()
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


class TTunnelUI : public TSessionUI
{
NB_DISABLE_COPY(TTunnelUI)
public:
  explicit TTunnelUI(TTerminal * Terminal);

  virtual ~TTunnelUI()
  {
  }

  virtual void Information(UnicodeString Str, bool Status) override;
  virtual uintptr_t QueryUser(UnicodeString Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType) override;
  virtual uintptr_t QueryUserException(UnicodeString Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType) override;
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString AName, UnicodeString AInstructions, TStrings * Prompts,
    TStrings * Results) override;
  virtual void DisplayBanner(UnicodeString Banner) override;
  virtual void FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpContext) override;
  virtual void HandleExtendedException(Exception * E) override;
  virtual void Closed() override;
  virtual void ProcessGUI() override;

private:
  TTerminal * FTerminal;
  uint32_t FTerminalThreadID;
};

TTunnelUI::TTunnelUI(TTerminal * Terminal) :
  TSessionUI(OBJECT_CLASS_TTunnelUI),
  FTerminal(Terminal)
{
  FTerminalThreadID = GetCurrentThreadId();
}

void TTunnelUI::Information(UnicodeString Str, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->Information(Str, Status);
  }
}

uintptr_t TTunnelUI::QueryUser(UnicodeString Query,
  TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result;
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    Result = FTerminal->QueryUser(Query, MoreMessages, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(Answers);
  }
  return Result;
}

uintptr_t TTunnelUI::QueryUserException(UnicodeString Query,
  Exception * E, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result;
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    Result = FTerminal->QueryUserException(Query, E, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(static_cast<intptr_t>(Answers));
  }
  return Result;
}

bool TTunnelUI::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString AName, UnicodeString AInstructions, TStrings * Prompts, TStrings * Results)
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

void TTunnelUI::DisplayBanner(UnicodeString Banner)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->DisplayBanner(Banner);
  }
}

void TTunnelUI::FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpContext)
{
  throw ESshFatal(E, Msg, HelpContext);
}

void TTunnelUI::HandleExtendedException(Exception * E)
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
  explicit TCallbackGuard(TTerminal * ATerminal);
  inline ~TCallbackGuard();

  void FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword);
  inline void Verify();
  inline bool Verify(Exception * E);
  void Dismiss();

private:
  ExtException * FFatalError;
  TTerminal * FTerminal;
  bool FGuarding;
};

TCallbackGuard::TCallbackGuard(TTerminal * ATerminal) :
  FFatalError(nullptr),
  FTerminal(ATerminal),
  FGuarding(FTerminal->FCallbackGuard == nullptr)
{
  if (FGuarding)
  {
    FTerminal->FCallbackGuard = this;
  }
}

TCallbackGuard::~TCallbackGuard()
{
  if (FGuarding)
  {
    DebugAssert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == nullptr));
    FTerminal->FCallbackGuard = nullptr;
  }

  SAFE_DESTROY_EX(Exception, FFatalError);
}

void TCallbackGuard::FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword)
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
      throw ESshFatal(FFatalError, L"");
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


TRobustOperationLoop::TRobustOperationLoop(TTerminal * Terminal, TFileOperationProgressType * OperationProgress, bool * AnyTransfer) :
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

TRobustOperationLoop::~TRobustOperationLoop()
{
  if (FAnyTransfer != nullptr)
  {
    *FAnyTransfer = FPrevAnyTransfer;
  }
}

bool TRobustOperationLoop::TryReopen(Exception & E)
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
  explicit TRetryOperationLoop(TTerminal * Terminal);

  void Error(Exception & E);
  void Error(Exception & E, TSessionAction & Action);
  void Error(Exception & E, UnicodeString Message);
  void Error(Exception & E, TSessionAction & Action, UnicodeString Message);
  bool Retry();

private:
  TTerminal * FTerminal;
  bool FRetry;

  void DoError(Exception & E, TSessionAction * Action, UnicodeString Message);
};

TRetryOperationLoop::TRetryOperationLoop(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FRetry = false;
}

void TRetryOperationLoop::DoError(Exception & E, TSessionAction * Action, UnicodeString Message)
{
  // Note that the action may already be canceled when RollbackAction is called
  uintptr_t Result;
  try
  {
    Result = FTerminal->CommandError(&E, Message, qaRetry | qaSkip | qaAbort);
  }
  catch (Exception & E2)
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

void TRetryOperationLoop::Error(Exception & E, UnicodeString Message)
{
  DoError(E, nullptr, Message);
}

void TRetryOperationLoop::Error(Exception & E, TSessionAction & Action, UnicodeString Message)
{
  DoError(E, &Action, Message);
}

bool TRetryOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  return Result;
}


TCollectedFileList::TCollectedFileList() :
  TObject(OBJECT_CLASS_TCollectedFileList)
{
}

intptr_t TCollectedFileList::Add(UnicodeString FileName, TObject * Object, bool Dir)
{
  TFileData Data;
  Data.FileName = FileName;
  Data.Object = Object;
  Data.Dir = Dir;
  Data.Recursed = true;
  FList.push_back(Data);
  return GetCount() - 1;
}

void TCollectedFileList::DidNotRecurse(intptr_t Index)
{
  FList[Index].Recursed = false;
}

void TCollectedFileList::Delete(intptr_t Index)
{
  FList.erase(FList.begin() + Index);
}

intptr_t TCollectedFileList::GetCount() const
{
  return FList.size();
}

UnicodeString TCollectedFileList::GetFileName(intptr_t Index) const
{
  return FList[Index].FileName;
}

TObject * TCollectedFileList::GetObj(intptr_t Index) const
{
  return FList[Index].Object;
}

bool TCollectedFileList::IsDir(intptr_t Index) const
{
  return FList[Index].Dir;
}

bool TCollectedFileList::IsRecursed(intptr_t Index) const
{
  return FList[Index].Recursed;
}


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

void TParallelOperation::Init(
  TStrings * AFileList, UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * MainOperationProgress, UnicodeString MainName)
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

TParallelOperation::~TParallelOperation()
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
    TGuard Guard(*FSection.get());
    Result = !FProbablyEmpty && (FMainOperationProgress->GetCancel() < csCancel);
  }
  return Result;
}

void TParallelOperation::AddClient()
{
  TGuard Guard(*FSection.get());
  FClients++;
}

void TParallelOperation::RemoveClient()
{
  TGuard Guard(*FSection.get());
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
        TGuard Guard(*FSection.get());
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

void TParallelOperation::Done(UnicodeString FileName, bool Dir, bool Success)
{
  if (Dir)
  {
    TGuard Guard(*FSection.get());

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
          TCollectedFileList * Files = DebugNotNull(dyn_cast<TCollectedFileList>(FFileList->GetObj(0)));
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

bool TParallelOperation::CheckEnd(TCollectedFileList * Files)
{
  bool Result = (FIndex >= Files->GetCount());
  if (Result)
  {
    FFileList->Delete(0);
    FIndex = 0;
  }
  return Result;
}

intptr_t TParallelOperation::GetNext(TTerminal * Terminal, UnicodeString & FileName, TObject *& Object, UnicodeString & TargetDir, bool & Dir, bool & Recursed)
{
  TGuard Guard(*FSection.get());
  intptr_t Result = 1;
  TCollectedFileList * Files;
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


TTerminal::TTerminal(TObjectClassId Kind) :
  TSessionUI(Kind),
  FSessionData(nullptr),
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

void TTerminal::Init(TSessionData * SessionData,
  TConfiguration * Configuration)
{
  FConfiguration = Configuration;
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(SessionData);
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
  FOnDisplayBanner = nullptr;
  FOnShowExtendedException = nullptr;
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
  FEnableSecureShellUsage = false;
  FCollectFileSystemUsage = false;
  FSuspendTransaction = false;
  FOperationProgress = nullptr;
  FClosedOnCompletion = nullptr;
}

TTerminal::~TTerminal()
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

void TTerminal::Idle()
{
  // Once we disconnect, do nothing, until reconnect handler
  // "receives the information".
  // Never go idle when called from within ::ProcessGUI() call
  // as we may recurse for good, timeouting eventually.
  if (GetActive() && (FNesting == 0))
  {
    TAutoNestingCounter NestingCounter(FNesting);

    if (FConfiguration->GetActualLogProtocol() >= 1)
    {
#if 0
      LogEvent("Session upkeep");
#endif // #if 0
    }

    DebugAssert(FFileSystem != nullptr);
    FFileSystem->Idle();

    if (GetCommandSessionOpened())
    {
      try
      {
        FCommandSession->Idle();
      }
      catch (Exception & E)
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

RawByteString TTerminal::EncryptPassword(UnicodeString APassword) const
{
  return FConfiguration->EncryptPassword(APassword, GetSessionData()->GetSessionName());
}

UnicodeString TTerminal::DecryptPassword(RawByteString APassword) const
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

void TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FRememberedPassword = EncryptPassword(DecryptPassword(FRememberedPassword));
  FRememberedTunnelPassword = EncryptPassword(DecryptPassword(FRememberedTunnelPassword));
}

UnicodeString TTerminal::ExpandFileName(UnicodeString APath,
  UnicodeString BasePath)
{
  // replace this by AbsolutePath()
  UnicodeString Result = base::UnixExcludeTrailingBackslash(APath);
  if (!base::UnixIsAbsolutePath(Result) && !BasePath.IsEmpty())
  {
    TODO("Handle more complicated cases like '../../xxx'");
    if (Result == PARENTDIRECTORY)
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

UnicodeString TTerminal::FingerprintScan()
{
  GetSessionData()->SetFingerprintScan(true);
  try
  {
    Open();
    // we should never get here
    Abort();
  }
  catch (...)
  {
    if (!FFingerprintScanned.IsEmpty())
    {
      return FFingerprintScanned;
    }
    throw;
  }
  DebugFail();
  return UnicodeString();
}

void TTerminal::Open()
{
  TAutoNestingCounter OpeningCounter(FOpening);
  ReflectSettings();
  try
  {
    DoInformation(L"", true, 1);
    try__finally
    {
      SCOPE_EXIT
      {
        DoInformation(L"", true, 0);
      };
      {
        InternalTryOpen();
      }
    }
    __finally
    {
#if 0
      DoInformation(L"", true, 0);
#endif // #if 0
    };
  }
  catch (EFatal &)
  {
    throw;
  }
  catch (Exception & E)
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
      SCOPE_EXIT
      {
        if (FSessionData->GetTunnel())
        {
          FSessionData->RollbackTunnel();
        }
      };
      InternalDoTryOpen();
    }
    __finally
    {
#if 0
      if (FSessionData->Tunnel)
      {
        FSessionData->RollbackTunnel();
      }
#endif // #if 0
    };

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
      FFingerprintScanned = FFileSystem->GetSessionInfo().CertificateFingerprint;
    }
    // Particularly to prevent reusing a wrong client certificate passphrase
    // in the next login attempt
    FRememberedPassword = UnicodeString();
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
#ifdef NO_FILEZILLA
    LogEvent("FTP protocol is not supported by this build.");
    FatalError(nullptr, LoadStr(FTP_UNSUPPORTED));
#else
    FFSProtocol = cfsFTP;
    FFileSystem = new TFTPFileSystem(this);
    FFileSystem->Init(nullptr);
    FFileSystem->Open();
    GetLog()->AddSeparator();
    LogEvent("Using FTP protocol.");
#endif
  }
  else if ((FSProtocol == fsFTP) && (GetSessionData()->GetFtps() != ftpsNone))
  {
#if defined(NO_FILEZILLA) && defined(MPEXT_NO_SSLDLL)
    LogEvent("FTPS protocol is not supported by this build.");
    FatalError(nullptr, LoadStr(FTPS_UNSUPPORTED));
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
  else
  {
    DebugAssert(FSecureShell == nullptr);
    try__finally
    {
      SCOPE_EXIT
      {
        SAFE_DESTROY(FSecureShell);
      };
      FSecureShell = new TSecureShell(this, FSessionData, GetLog(), FConfiguration);
      try
      {
        // there will be only one channel in this session
        FSecureShell->SetSimple(true);
        FSecureShell->Open();
      }
      catch (Exception & E)
      {
        DebugAssert(!FSecureShell->GetActive());
        if (FSessionData->GetFingerprintScan())
        {
          FFingerprintScanned = FSecureShell->GetHostKeyFingerprint();
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
    }
    __finally
    {
#if 0
      delete FSecureShell;
      FSecureShell = nullptr;
#endif // #if 0
    };
  }
}

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
    FTunnelData->SetUserName(FSessionData->GetTunnelUserName());
    FTunnelData->SetPassword(FSessionData->GetTunnelPassword());
    FTunnelData->SetPublicKeyFile(FSessionData->GetTunnelPublicKeyFile());
    FTunnelData->SetTunnelPortFwd(FORMAT("L%d\t%s:%d",
      FTunnelLocalPortNumber, FSessionData->GetHostNameExpanded(), FSessionData->GetPortNumber()));
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
      SCOPE_EXIT
      {
        FTunnelOpening = false;
      };
      FTunnel->Open();
    }
    __finally
    {
#if 0
      FTunnelOpening = false;
#endif
    };

    FTunnelThread = new TTunnelThread(FTunnel);
    FTunnelThread->InitTunnelThread();
  }
  catch (...)
  {
    CloseTunnel();
    throw;
  }
}

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
    TAutoNestingCounter NestingCounter(FNesting);
    ::ProcessGUI();
  }
}

void TTerminal::Progress(TFileOperationProgressType * OperationProgress)
{
  if (FNesting == 0)
  {
    TAutoNestingCounter NestingCounter(FNesting);
    OperationProgress->Progress();
  }
}

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
  try__finally
  {
    bool WasInTransaction = InTransaction();
    SCOPE_EXIT
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
    };
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
  }
  __finally
  {
#if 0
    SessionData->RemoteDirectory = PrevRemoteDirectory;
    SessionData->FSProtocol = OrigFSProtocol;
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
#endif // if 0
  };
}

bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString AName, UnicodeString Instructions, UnicodeString Prompt, bool Echo, intptr_t MaxLen, UnicodeString & AResult)
{
  bool Result;
  std::unique_ptr<TStrings> Prompts(new TStringList());
  std::unique_ptr<TStrings> Results(new TStringList());
  try__finally
  {
    Prompts->AddObject(Prompt, ToObj(FLAGMASK(Echo, pupEcho)));
    Results->AddObject(AResult, ToObj(MaxLen));
    Result = PromptUser(Data, Kind, AName, Instructions, Prompts.get(), Results.get());
    AResult = Results->GetString(0);
  }
  __finally
  {
#if 0
    delete Prompts;
    delete Results;
#endif
  };
  return Result;
}

bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString AName, UnicodeString Instructions, TStrings * Prompts, TStrings * Results)
{
  // If PromptUser is overridden in descendant class, the overridden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  // Actually no longer needed as we do not override DoPromptUser
  // anymore in TSecondaryTerminal.
  return DoPromptUser(Data, Kind, AName, Instructions, Prompts, Results);
}

TTerminal * TTerminal::GetPasswordSource()
{
  return this;
}

bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Response)
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
        GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Response, Result, nullptr);
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
      }
    }
  }

  return Result;
}

uintptr_t TTerminal::QueryUser(UnicodeString Query,
  TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  LogEvent(FORMAT("Asking user:\n%s (%s)", Query, UnicodeString(MoreMessages ? MoreMessages->GetCommaText() : L"")));
  uintptr_t Answer = AbortAnswer(Answers);
  if (FOnQueryUser)
  {
    TCallbackGuard Guard(this);
    try
    {
      FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, nullptr);
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
  return Answer;
}

uintptr_t TTerminal::QueryUserException(UnicodeString Query,
  Exception * E, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result = 0;
  UnicodeString ExMessage;
  if (DebugAlwaysTrue(ExceptionMessage(E, ExMessage) || !Query.IsEmpty()))
  {
    std::unique_ptr<TStrings> MoreMessages(new TStringList());
    try__finally
    {
      if (!ExMessage.IsEmpty() && !Query.IsEmpty())
      {
        MoreMessages->Add(UnformatMessage(ExMessage));
      }

      ExtException * EE = dyn_cast<ExtException>(E);
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

      Result = QueryUser(!Query.IsEmpty() ? Query : ExMessage,
        MoreMessages->GetCount() ? MoreMessages.get() : nullptr,
        Answers, &HelpKeywordOverrideParams, QueryType);
    }
    __finally
    {
#if 0
      delete MoreMessages;
#endif
    };
  }
  return Result;
}

void TTerminal::DisplayBanner(UnicodeString Banner)
{
  if (GetOnDisplayBanner() != nullptr)
  {
    if (FConfiguration->GetForceBanners() ||
      FConfiguration->ShowBanner(GetSessionData()->GetSessionKey(), Banner))
    {
      bool NeverShowAgain = false;
      intptr_t Options =
        FLAGMASK(FConfiguration->GetForceBanners(), boDisableNeverShowAgain);

      TCallbackGuard Guard(this);
      try
      {
        GetOnDisplayBanner()(this, GetSessionData()->GetSessionName(), Banner,
          NeverShowAgain, Options);
        Guard.Verify();
      }
      catch (Exception & E)
      {
        if (!Guard.Verify(&E))
        {
          throw;
        }
      }

      if (!FConfiguration->GetForceBanners() && NeverShowAgain)
      {
        FConfiguration->NeverShowBanner(GetSessionData()->GetSessionKey(), Banner);
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
    catch (Exception & E2)
    {
      if (!Guard.Verify(&E2))
      {
        throw;
      }
    }
  }
}

void TTerminal::ShowExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    GetOnShowExtendedException()(this, E, nullptr);
  }
}

void TTerminal::DoInformation(UnicodeString Str, bool Status,
  intptr_t Phase)
{
  if (GetOnInformation())
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnInformation()(this, Str, Status, Phase);
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

void TTerminal::Information(UnicodeString Str, bool Status)
{
  DoInformation(Str, Status);
}

void TTerminal::DoProgress(TFileOperationProgressType & ProgressData)
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

  if (GetOnProgress() != nullptr)
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnProgress()(ProgressData);
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

void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  UnicodeString AFileName, bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  if (GetOnFinished() != nullptr)
  {
    TCallbackGuard Guard(this);
    try
    {
      GetOnFinished()(Operation, Side, Temp, AFileName, Success, OnceDoneOperation);
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

void TTerminal::SaveCapabilities(TFileSystemInfo & FileSystemInfo)
{
  for (intptr_t Index = 0; Index < fcCount; ++Index)
  {
    FileSystemInfo.IsCapable[Index] = GetIsCapable(static_cast<TFSCapability>(Index));
  }
}

bool TTerminal::GetIsCapableProtected(TFSCapability Capability) const
{
  DebugAssert(FFileSystem);
  return FFileSystem->IsCapable(Capability);
}

UnicodeString TTerminal::GetAbsolutePath(UnicodeString APath, bool Local) const
{
  return FFileSystem->GetAbsolutePath(APath, Local);
}

void TTerminal::ReactOnCommand(intptr_t Cmd)
{
  bool ChangesDirectory = false;
  bool ModifiesFiles = false;

  switch (static_cast<TFSCommand>(Cmd))
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

void TTerminal::TerminalError(UnicodeString Msg)
{
  TerminalError(nullptr, Msg);
}

void TTerminal::TerminalError(
  Exception * E, UnicodeString Msg, UnicodeString HelpKeyword)
{
  throw ETerminal(E, Msg, HelpKeyword);
}

bool TTerminal::DoQueryReopen(Exception * E)
{
  EFatal * Fatal = dyn_cast<EFatal>(E);
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

bool TTerminal::ContinueReopen(TDateTime Start) const
{
  return
    (FConfiguration->GetSessionReopenTimeout() == 0) ||
    (intptr_t(double(Now() - Start) * MSecsPerDay) < FConfiguration->GetSessionReopenTimeout());
}

bool TTerminal::QueryReopen(Exception * E, intptr_t Params,
  TFileOperationProgressType * OperationProgress)
{
  TSuspendFileOperationProgress Suspend(OperationProgress);

  bool Result = DoQueryReopen(E);

  if (Result)
  {
    TDateTime Start = Now();
    do
    {
      try
      {
        Reopen(Params);
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
  TFileOperationProgressType * OperationProgress, UnicodeString Message,
  bool AllowSkip, UnicodeString SpecialRetry, UnicodeString HelpKeyword)
{
  bool Result = false;
  GetLog()->AddException(&E);
  uintptr_t Answer;
  bool SkipToAllPossible = AllowSkip && (OperationProgress != nullptr);

  if (SkipToAllPossible && OperationProgress->GetSkipToAll())
  {
    Answer = qaSkip;
  }
  else
  {
    uintptr_t Answers = qaRetry | qaAbort |
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
      TSuspendFileOperationProgress Suspend(OperationProgress);
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
      ThrowSkipFile(&E, Message);
    }
    else
    {
      // this can happen only during file transfer with SCP
      throw ExtException(&E, Message);
    }
  }

  return Result;
}

intptr_t TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType * OperationProgress, bool AllowSkip,
  UnicodeString Message, void * Param1, void * Param2)
{
  DebugAssert(CallBackFunc);
  intptr_t Result = 0;
  FileOperationLoopCustom(this, OperationProgress, AllowSkip, Message, "",
  [&]()
  {
    Result = CallBackFunc(Param1, Param2);
  });

  return Result;
}

UnicodeString TTerminal::TranslateLockedPath(UnicodeString APath, bool Lock)
{
  UnicodeString Result = APath;
  if (GetSessionData()->GetLockInHome() && !Result.IsEmpty() && (Result[1] == L'/'))
  {
    if (Lock)
    {
      if (Result.SubString(1, FLockDirectory.Length()) == FLockDirectory)
      {
        Result.Delete(1, FLockDirectory.Length());
        if (Result.IsEmpty())
        {
          Result = ROOTDIRECTORY;
        }
      }
    }
    else
    {
      Result = base::UnixExcludeTrailingBackslash(FLockDirectory + Result);
    }
  }
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
}

void TTerminal::ClearCachedFileList(UnicodeString APath,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(APath, SubDirs);
}

void TTerminal::AddCachedFileList(TRemoteFileList * FileList)
{
  FDirectoryCache->AddFileList(FileList);
}

TRemoteFileList * TTerminal::DirectoryFileList(UnicodeString APath, TDateTime Timestamp, bool CanLoad)
{
  TRemoteFileList * Result = nullptr;
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

void TTerminal::TerminalSetCurrentDirectory(UnicodeString AValue)
{
  DebugAssert(FFileSystem);
  UnicodeString Value = TranslateLockedPath(AValue, false);
  if (Value != FFileSystem->RemoteCurrentDirectory())
  {
    RemoteChangeDirectory(Value);
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

UnicodeString TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->RemoteCurrentDirectory();
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
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
  UnicodeString Result = FFileSystem ? FFileSystem->RemoteGetUserName() : L"";
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
    catch (Exception & E)
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
    catch (Exception & E)
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

void TTerminal::DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool & Cancel)
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
      FOnFindingFile(this, L"", Cancel);
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
        SCOPE_EXIT
        {
          FReadCurrentDirectoryPending = false;
          FReadDirectoryPending = false;
        };
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
      }
      __finally
      {
#if 0
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
#endif
      };
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
      throw Exception(L"ExceptionOnFail is already zero.");
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
  FatalError(nullptr, L"");
}

void TTerminal::FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword)
{
  bool SecureShellActive = (FSecureShell != nullptr) && FSecureShell->GetActive();
  if (GetActive() || SecureShellActive)
  {
    // We log this instead of exception handler, because Close() would
    // probably cause exception handler to loose pointer to TShellLog()
    LogEvent("Attempt to close connection due to fatal exception:");
    GetLog()->Add(llException, Msg);
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
    FCallbackGuard->FatalError(E, Msg, HelpKeyword);
  }
  else
  {
    throw ESshFatal(E, Msg, HelpKeyword);
  }
}

void TTerminal::CommandError(Exception * E, UnicodeString Msg)
{
  CommandError(E, Msg, 0);
}

uintptr_t TTerminal::CommandError(Exception * E, UnicodeString Msg,
  uintptr_t Answers, UnicodeString HelpKeyword)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  DebugAssert(FCallbackGuard == nullptr);
  uintptr_t Result = 0;
  if (E && isa<EFatal>(E))
  {
    FatalError(E, Msg, HelpKeyword);
  }
  else if (E && isa<EAbort>(E))
  {
    // resent EAbort exception
    Abort();
  }
  else if (GetExceptionOnFail())
  {
    throw ECommand(E, Msg, HelpKeyword);
  }
  else if (!Answers)
  {
    ECommand ECmd(E, Msg, HelpKeyword);
    try__finally
    {
      HandleExtendedException(&ECmd);
    }
    __finally
    {
#if 0
      delete ECmd;
#endif
    };
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
      TQueryParams Params(qpAllowContinueOnError, HelpKeyword);
      TQueryButtonAlias Aliases[1];
      if (CanSkip)
      {
        Aliases[0].Button = qaAll;
        Aliases[0].Alias = LoadStr(SKIP_ALL_BUTTON);
        Params.Aliases = Aliases;
        Params.AliasesCount = _countof(Aliases);
        Answers |= qaAll;
      }
      Result = QueryUserException(Msg, E, Answers, &Params, qtError);
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
  TOnceDoneOperation Operation, UnicodeString Message,
  UnicodeString TargetLocalPath, UnicodeString DestLocalFileName)
{
//  Configuration->Usage->Inc(L"ClosesOnCompletion");
  LogEvent("Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(nullptr,
    Message.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : Message,
    Operation, TargetLocalPath, DestLocalFileName);
}

TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
  UnicodeString ASourceFullFileName, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress, bool Special) const
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

bool TTerminal::CheckRemoteFile(
  UnicodeString AFileName, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress) const
{
  bool Result;
  OperationProgress->LockUserSelections();
  try__finally
  {
    SCOPE_EXIT
    {
      OperationProgress->UnlockUserSelections();
    };
    Result = (EffectiveBatchOverwrite(AFileName, CopyParam, Params, OperationProgress, true) != boAll);
  }
  __finally
  {
#if 0
    OperationProgress->UnlockUserSelections();
#endif
  };
  return Result;
}

uintptr_t TTerminal::ConfirmFileOverwrite(
  UnicodeString ASourceFullFileName, UnicodeString ATargetFileName,
  const TOverwriteFileParams * FileParams, uintptr_t Answers, TQueryParams * QueryParams,
  TOperationSide Side, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
  UnicodeString AMessage)
{
  UnicodeString Message = AMessage;
  uintptr_t Result = 0;
  TBatchOverwrite BatchOverwrite;
  bool CanAlternateResume =
    (FileParams != nullptr) &&
    (FileParams->DestSize < FileParams->SourceSize) &&
    !OperationProgress->GetAsciiTransfer();

  OperationProgress->LockUserSelections();
  try__finally
  {
    SCOPE_EXIT
    {
      OperationProgress->UnlockUserSelections();
    };
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
        if (Message.IsEmpty())
        {
          // Side refers to destination side here
          Message = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE2 :
            REMOTE_FILE_OVERWRITE2), ATargetFileName, ATargetFileName);
        }
        if (FileParams != nullptr)
        {
          Message = FMTLOAD(FILE_OVERWRITE_DETAILS, Message,
            ::Int64ToStr(FileParams->SourceSize),
            base::UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision),
            ::Int64ToStr(FileParams->DestSize),
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
  }
  __finally
  {
#if 0
    OperationProgress->UnlockUserSelections();
#endif
  };

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

          LogEvent(FORMAT(L"Source file timestamp is [%s], destination timestamp is [%s], will%s overwrite",
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

void TTerminal::FileModified(const TRemoteFile * AFile,
  UnicodeString AFileName, bool ClearDirectoryChange)
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

void TTerminal::DirectoryModified(UnicodeString APath, bool SubDirs)
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

void TTerminal::DirectoryLoaded(TRemoteFileList * FileList)
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

void TTerminal::EnsureNonExistence(UnicodeString AFileName)
{
  // if filename doesn't contain path, we check for existence of file
  if ((base::UnixExtractFileDir(AFileName).IsEmpty()) &&
      base::UnixSamePath(RemoteGetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * File = FFiles->FindFile(AFileName);
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

void TTerminal::LogEvent(UnicodeString Str)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, Str);
  }
}

void TTerminal::RollbackAction(TSessionAction & Action,
  TFileOperationProgressType * OperationProgress, Exception * E)
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

void TTerminal::DoStartup()
{
  LogEvent("Doing startup conversation with host.");
  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      DoEndTransaction(true);
    };
    DoInformation(LoadStr(STATUS_STARTUP), true);

    // Make sure that directory would be loaded at last
    FReadCurrentDirectoryPending = true;
    FReadDirectoryPending = GetAutoReadDirectory();

    FFileSystem->DoStartup();

    LookupUsersGroups();

    if (!GetSessionData()->GetRemoteDirectory().IsEmpty())
    {
      RemoteChangeDirectory(GetSessionData()->GetRemoteDirectory());
    }
  }
  __finally
  {
#if 0
    DoEndTransaction(true);
#endif // #if 0
  };
  LogEvent("Startup conversation with host finished.");
}

void TTerminal::ReadCurrentDirectory()
{
  DebugAssert(FFileSystem);
  try
  {
    // reset flag is case we are called externally (like from console dialog)
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
      // not to broke the cache, if the next directory change would not
      // be initialized by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange.Clear();
    }

    if (OldDirectory.IsEmpty())
    {
      FLockDirectory = (GetSessionData()->GetLockInHome() ?
        FFileSystem->RemoteCurrentDirectory() : UnicodeString(L""));
    }
    // if (OldDirectory != FFileSystem->GetCurrDirectory())
    {
      DoChangeDirectory();
    }
  }
  catch (Exception & E)
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
        SCOPE_EXIT
        {
          DoReadDirectory(ReloadOnly);
        };
        LoadedFromCache = FDirectoryCache->GetFileList(RemoteGetCurrentDirectory(), FFiles);
      }
      __finally
      {
#if 0
        DoReadDirectory(ReloadOnly);
#endif // #if 0
      };

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
      TRemoteDirectory * Files = new TRemoteDirectory(this, FFiles);
      try__finally
      {
        SCOPE_EXIT
        {
          DoReadDirectoryProgress(-1, 0, Cancel);
          FReadingCurrentDirectory = false;
          FOldFiles->Reset();
          FOldFiles->AddFiles(FFiles);
          FFiles = Files;
          try__finally
          {
            SCOPE_EXIT
            {
              // delete only after loading new files to dir view,
              // not to destroy the file objects that the view holds
              // (can be issue in multi threaded environment, such as when the
              // terminal is reconnecting in the terminal thread)
              FOldFiles->Reset();
            };
            DoReadDirectory(ReloadOnly);
          }
          __finally
          {
#if 0
            // delete only after loading new files to dir view,
            // not to destroy the file objects that the view holds
            // (can be issue in multi threaded environment, such as when the
            // terminal is reconnecting in the terminal thread)
            delete OldFiles;
#endif // #if 0
          };
          if (GetActive())
          {
            if (GetSessionData()->GetCacheDirectories())
            {
              DirectoryLoaded(FFiles);
            }
          }
        };
        Files->SetDirectory(RemoteGetCurrentDirectory());
        CustomReadDirectory(Files);
      }
      __finally
      {
#if 0
        DoReadDirectoryProgress(-1, 0, Cancel);
        FReadingCurrentDirectory = false;
        TRemoteDirectory * OldFiles = FFiles;
        FFiles = Files;
        try
        {
          DoReadDirectory(ReloadOnly);
        }
        __finally
        {
          // delete only after loading new files to dir view,
          // not to destroy the file objects that the view holds
          // (can be issue in multi threaded environment, such as when the
          // terminal is reconnecting in the terminal thread)
          delete OldFiles;
        }
        if (Active)
        {
          if (SessionData->CacheDirectories)
          {
            DirectoryLoaded(FFiles);
          }
        }
#endif // #if 0
      };
    }
    catch (Exception & E)
    {
      CommandError(&E, FMTLOAD(LIST_DIR_ERROR, FFiles->GetDirectory()));
    }
  }
}

UnicodeString TTerminal::GetRemoteFileInfo(TRemoteFile * AFile) const
{
  return
    FORMAT(L"%s;%s;%lld;%s;%d;%s;%s;%s;%d",
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

UnicodeString TTerminal::FormatFileDetailsForLog(UnicodeString AFileName, const TDateTime & AModification, int64_t Size)
{
  UnicodeString Result;
  // optimization
  if (GetLog()->GetLogging())
  {
    Result = FORMAT("'%s' [%s] [%s]", AFileName, UnicodeString(AModification != TDateTime() ? StandardTimestamp(AModification) : UnicodeString(L"n/a")), ::Int64ToStr(Size));
  }
  return Result;
}

void TTerminal::LogFileDetails(UnicodeString AFileName, const TDateTime & AModification, int64_t Size)
{
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT("File: %s", FormatFileDetailsForLog(AFileName, AModification, Size)));
  }
}

void TTerminal::LogFileDone(TFileOperationProgressType * OperationProgress, UnicodeString DestFileName)
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
    LogEvent(FORMAT(L"Transfer done: '%s' => '%s' [%s]", OperationProgress->GetFullFileName(), DestFileName, Int64ToStr(OperationProgress->GetTransferredSize())));
  }
}

void TTerminal::CustomReadDirectory(TRemoteFileList * AFileList)
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
      FFileSystem->ReadDirectory(AFileList);
    }
    catch (Exception & E)
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
  {
    for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      LogRemoteFile(AFileList->GetFile(Index));
    }
  }

  ReactOnCommand(fsListDirectory);
}

TRemoteFileList * TTerminal::ReadDirectoryListing(UnicodeString Directory, const TFileMasks & Mask)
{
  TRemoteFileList * FileList;
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
          TRemoteFile * File = FileList->GetFile(Index);
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
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return FileList;
}

TRemoteFile * TTerminal::ReadFileListing(UnicodeString APath)
{
  TRemoteFile * File = nullptr;
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
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return File;
}

TRemoteFileList * TTerminal::CustomReadDirectoryListing(UnicodeString Directory, bool UseCache)
{
  TRemoteFileList * FileList = nullptr;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FileList = DoReadDirectoryListing(Directory, UseCache);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E);
    }
  }
  while (RetryLoop.Retry());
  return FileList;
}

TRemoteFileList * TTerminal::DoReadDirectoryListing(UnicodeString ADirectory, bool UseCache)
{
  std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
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
        SCOPE_EXIT
        {
          SetExceptionOnFail(false);
        };
        ReadDirectory(FileList.get());
      }
      __finally
      {
#if 0
        SetExceptionOnFail(false);
#endif
      };

      if (Cache)
      {
        AddCachedFileList(FileList.get());
      }
    }
  }
#if 0
  catch(...)
  {
    delete FileList;
    throw;
  }
#endif // #if 0
  return FileList.release();
}

void TTerminal::ProcessDirectory(UnicodeString ADirName,
  TProcessFileEvent CallBackFunc, void * Param, bool UseCache, bool IgnoreErrors)
{
  std::unique_ptr<TRemoteFileList> FileList;
  if (IgnoreErrors)
  {
    SetExceptionOnFail(true);
    try__finally
    {
      SCOPE_EXIT
      {
        SetExceptionOnFail(false);
      };
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
    }
    __finally
    {
#if 0
      SetExceptionOnFail(false);
#endif
    };
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
        TRemoteFile * File = FileList->GetFile(Index);
        if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
        {
          CallBackFunc(Directory + File->GetFileName(), File, Param);
          // We should catch EScpSkipFile here as we do in ProcessFiles.
          // Now we have to handle EScpSkipFile in every callback implementation.
        }
      }
    }
    __finally
    {
#if 0
      delete FileList;
#endif // #if 0
    };
  }
}

void TTerminal::ReadDirectory(TRemoteFileList * AFileList)
{
  DebugAssert(AFileList);

  try
  {
    CustomReadDirectory(AFileList);
  }
  catch (Exception & E)
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
  catch (Exception & E)
  {
    CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName()));
  }
}

void TTerminal::ReadFile(UnicodeString AFileName,
  TRemoteFile *& AFile)
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
  catch (Exception & E)
  {
    if (AFile)
    {
      SAFE_DESTROY(AFile);
    }
    AFile = nullptr;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, AFileName));
  }
}

bool TTerminal::FileExists(UnicodeString AFileName, TRemoteFile ** AFile)
{
  bool Result;
  TRemoteFile * File = nullptr;
  try
  {
    SetExceptionOnFail(true);
    try__finally
    {
      SCOPE_EXIT
      {
        SetExceptionOnFail(false);
      };
      ReadFile(base::UnixExcludeTrailingBackslash(AFileName), File);
    }
    __finally
    {
#if 0
      SetExceptionOnFail(false);
#endif
    };

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

void TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}

bool TTerminal::ProcessFiles(const TStrings * AFileList,
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
    Progress.Start(Operation, Side, AFileList->GetCount());

    TFileOperationProgressType * OperationProgress(&Progress);

    FOperationProgress = &Progress; //-V506
    try__finally
    {
      SCOPE_EXIT
      {
        FOperationProgress = nullptr;
        Progress.Stop();
      };
      if (Side == osRemote)
      {
        BeginTransaction();
      }

      try__finally
      {
        SCOPE_EXIT
        {
          if (Side == osRemote)
          {
            EndTransaction();
          }
        };
        intptr_t Index = 0;
        while ((Index < AFileList->GetCount()) && (Progress.GetCancel() == csContinue))
        {
          UnicodeString FileName = AFileList->GetString(Index);
          try
          {
            bool Success = false;
            try__finally
            {
              SCOPE_EXIT
              {
                Progress.Finish(FileName, Success, OnceDoneOperation);
              };
              if (!Ex)
              {
                TRemoteFile * RemoteFile = AFileList->GetAs<TRemoteFile>(Index);
                ProcessFile(FileName, RemoteFile, Param);
              }
              else
              {
                // not used anymore
#if 0
                TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                ProcessFileEx(FileName, (TRemoteFile *)FileList->GetObj(Index), Param, Index);
#endif
              }
              Success = true;
            }
            __finally
            {
#if 0
              Progress.Finish(FileName, Success, OnceDoneOperation);
#endif
            };
          }
          catch (ESkipFile & E)
          {
            DEBUG_PRINTF("before HandleException");
            TSuspendFileOperationProgress Suspend(OperationProgress);
            if (!HandleException(&E))
            {
              throw;
            }
          }
          ++Index;
        }
      }
      __finally
      {
#if 0
        if (Side == osRemote)
        {
          EndTransaction();
        }
#endif
      };

      if (Progress.GetCancel() == csContinue)
      {
        Result = true;
      }
    }
    __finally
    {
#if 0
      FOperationProgress = nullptr;
      Progress.Stop();
#endif
    };
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

bool TTerminal::GetResolvingSymlinks() const
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}

TUsableCopyParamAttrs TTerminal::UsableCopyParamAttrs(intptr_t Params) const
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
    FLAGMASK(!GetIsCapable(fcResumeSupport), cpaNoResumeSupport);
  Result.Download = Result.General | cpaNoClearArchive |
    cpaNoIgnorePermErrors |
    // May be already set in General flags, but it's unconditional here
    cpaNoRights | cpaNoRemoveCtrlZ | cpaNoRemoveBOM;
  Result.Upload = Result.General | cpaNoPreserveReadOnly |
    FLAGMASK(!GetIsCapable(fcPreservingTimestampUpload), cpaNoPreserveTime);
  return Result;
}

bool TTerminal::IsRecycledFile(UnicodeString AFileName)
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

void TTerminal::RecycleFile(UnicodeString AFileName,
  const TRemoteFile * AFile)
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
  }
}

bool TTerminal::TryStartOperationWithFile(
  UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2)
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
  UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  if (!TryStartOperationWithFile(AFileName, Operation1, Operation2))
  {
    Abort();
  }
}

void TTerminal::RemoteDeleteFile(const UnicodeString AFileName,
  const TRemoteFile * AFile, void * AParams)
{
  UnicodeString FileName = AFileName;
  if (AFileName.IsEmpty() && AFile)
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
    ReactOnCommand(fsDeleteFile);
  }
}

void TTerminal::DoDeleteFile(UnicodeString AFileName,
  const TRemoteFile * AFile, intptr_t Params)
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
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(DELETE_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

bool TTerminal::RemoteDeleteFiles(TStrings * AFilesToDelete, intptr_t Params)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TODO("avoid resolving symlinks while reading subdirectories.");
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return ProcessFiles(AFilesToDelete, foDelete, nb::bind(&TTerminal::RemoteDeleteFile, this), &Params);
}

void TTerminal::DeleteLocalFile(UnicodeString AFileName,
  const TRemoteFile * /*AFile*/, void * Params)
{
  StartOperationWithFile(AFileName, foDelete);
  if (GetOnDeleteLocalFile() == nullptr)
  {
    RecursiveDeleteFileChecked(AFileName, false);
  }
  else
  {
    GetOnDeleteLocalFile()(AFileName, FLAGSET(*(static_cast<intptr_t *>(Params)), dfAlternative));
  }
}

bool TTerminal::DeleteLocalFiles(TStrings * AFileList, intptr_t Params)
{
  return ProcessFiles(AFileList, foDelete, nb::bind(&TTerminal::DeleteLocalFile, this), &Params, osLocal);
}

void TTerminal::CustomCommandOnFile(UnicodeString AFileName,
  const TRemoteFile * AFile, void * AParams)
{
  TCustomCommandParams * Params = get_as<TCustomCommandParams>(AParams);
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

void TTerminal::DoCustomCommandOnFile(UnicodeString AFileName,
  const TRemoteFile * AFile, UnicodeString Command, intptr_t Params,
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
        FFileSystem->CustomCommandOnFile(AFileName, AFile, Command, Params, OutputEvent);
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
        FCommandSession->FFileSystem->CustomCommandOnFile(AFileName, AFile, Command,
          Params, OutputEvent);
      }
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(CUSTOM_COMMAND_ERROR, Command, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::CustomCommandOnFiles(UnicodeString Command,
  intptr_t Params, TStrings * AFiles, TCaptureOutputEvent OutputEvent)
{
  if (!TRemoteCustomCommand().IsFileListCommand(Command))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    ProcessFiles(AFiles, foCustomCommand, nb::bind(&TTerminal::CustomCommandOnFile, this), &AParams);
  }
  else
  {
    UnicodeString FileList;
    for (intptr_t Index = 0; Index < AFiles->GetCount(); ++Index)
    {
      TRemoteFile * File = AFiles->GetAs<TRemoteFile>(Index);
      bool Dir = File->GetIsDirectory() && CanRecurseToDirectory(File);

      if (!Dir || FLAGSET(Params, ccApplyToDirectories))
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
        Complete(Command, true);
    if (!DoOnCustomCommand(Cmd))
    {
      DoAnyCommand(Cmd, OutputEvent, nullptr);
    }
  }
}

bool TTerminal::DoOnCustomCommand(UnicodeString Command)
{
  bool Result = false;
  if (FOnCustomCommand != nullptr)
  {
    FOnCustomCommand(this, Command, Result);
  }
  return Result;
}

void TTerminal::ChangeFileProperties(UnicodeString AFileName,
  const TRemoteFile * AFile, /*const TRemoteProperties*/ void * Properties)
{
  TRemoteProperties * RProperties = get_as<TRemoteProperties>(Properties);
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
      LogEvent(FORMAT(" - modification: \"%s\"",
//       FormatDateTime(L"dddddd tt",
//         ::UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode()))));
         dt));
    }
    if (RProperties->Valid.Contains(vpLastAccess))
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = ::UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT("%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
      LogEvent(FORMAT(" - last access: \"%s\"",
//       FormatDateTime(L"dddddd tt",
//         ::UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode()))));
         dt));
    }
  }
  FileModified(AFile, LocalFileName);
  DoChangeFileProperties(LocalFileName, AFile, RProperties);
  ReactOnCommand(fsChangeProperties);
}

void TTerminal::DoChangeFileProperties(UnicodeString AFileName,
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
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CHANGE_PROPERTIES_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::ChangeFilesProperties(TStrings * AFileList,
  const TRemoteProperties * Properties)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  AnnounceFileListOperation();
  ProcessFiles(AFileList, foSetProperties, nb::bind(&TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}

bool TTerminal::LoadFilesProperties(TStrings * AFileList)
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

void TTerminal::DoCalculateFileSize(UnicodeString AFileName,
  const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * AParam)
{
  TCalculateSizeParams * AParams = get_as<TCalculateSizeParams>(AParam);

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

  CalculateFileSize(AFileName, AFile, AParam);
}


void TTerminal::CalculateFileSize(UnicodeString AFileName,
  const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * AParam)
{
  DebugAssert(AParam);
  DebugAssert(AFile);
  TCalculateSizeParams * AParams = get_as<TCalculateSizeParams>(AParam);
  UnicodeString LocalFileName = AFileName;
  if (AFileName.IsEmpty())
  {
    LocalFileName = AFile->GetFileName();
  }

  if (!TryStartOperationWithFile(LocalFileName, foCalculateSize))
  {
    AParams->Result = false;
    // Combined with csIgnoreErrors, this will not abort completelly, but we get called again
    // with next sibling of our parent directory (and we get again to this branch, throwing again)
    Abort();
  }

  bool AllowTransfer = (AParams->CopyParam == nullptr);
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->GetSize();
    MaskParams.Modification = AFile->GetModification();

    UnicodeString BaseFileName =
      GetBaseFileName(base::UnixExcludeTrailingBackslash(AFile->GetFullFileName()));
    AllowTransfer = AParams->CopyParam->AllowTransfer(
      BaseFileName, osRemote, AFile->GetIsDirectory(), MaskParams);
  }

  if (AllowTransfer)
  {
    intptr_t CollectionIndex = -1;
    if (AParams->Files != nullptr)
    {
      UnicodeString FullFileName = base::UnixExcludeTrailingBackslash(AFile->GetFullFileName());
      CollectionIndex = AParams->Files->Add(FullFileName, AFile->Duplicate(), AFile->GetIsDirectory());
    }

    if (AFile->GetIsDirectory())
    {
      if (CanRecurseToDirectory(AFile))
      {
        if (!AParams->AllowDirs)
        {
          AParams->Result = false;
        }
        else
        {
          LogEvent(FORMAT("Getting size of directory \"%s\"", LocalFileName));
          // pass in full path so we get it back in file list for AllowTransfer() exclusion
          if (!DoCalculateDirectorySize(AFile->GetFullFileName(), AFile, AParams))
          {
            if (CollectionIndex >= 0)
            {
              AParams->Files->DidNotRecurse(CollectionIndex);
            }
          }
        }
      }

      if (AParams->Stats != nullptr)
      {
        AParams->Stats->Directories++;
      }
    }
    else
    {
      AParams->Size += AFile->GetSize();

      if (AParams->Stats != nullptr)
      {
        AParams->Stats->Files++;
      }
    }

    if ((AParams->Stats != nullptr) && AFile->GetIsSymLink())
    {
      AParams->Stats->SymLinks++;
    }
  }
}

bool TTerminal::DoCalculateDirectorySize(UnicodeString AFileName,
  const TRemoteFile * /*AFile*/, TCalculateSizeParams * Params)
{
  bool Result = false;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      ProcessDirectory(AFileName, nb::bind(&TTerminal::CalculateFileSize, this), Params);
      Result = true;
    }
    catch (Exception & E)
    {
      // We can probably replace the csIgnoreErrors with IgnoreErrors argument of the ProcessDirectory
      if (!GetActive() || ((Params->Params & csIgnoreErrors) == 0))
      {
        RetryLoop.Error(E, FMTLOAD(CALCULATE_SIZE_ERROR, AFileName));
      }
    }
  }
  while (RetryLoop.Retry());
  return Result;
}

bool TTerminal::CalculateFilesSize(const TStrings * AFileList,
  int64_t & Size, intptr_t Params, const TCopyParamType * CopyParam,
  bool AllowDirs, TCalculateSizeStats & Stats)
{
  // With FTP protocol, we may use DSIZ command from
  // draft-peterson-streamlined-ftp-command-extensions-10
  // Implemented by Serv-U FTP.

  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  std::unique_ptr<TCalculateSizeParams> Param(new TCalculateSizeParams);
  Param->Size = 0;
  Param->Params = Params;
  Param->CopyParam = CopyParam;
  Param->Stats = &Stats;
  Param->AllowDirs = AllowDirs;
  Param->Result = true;
  ProcessFiles(AFileList, foCalculateSize, nb::bind(&TTerminal::DoCalculateFileSize, this), Param.get());
  Size = Param->Size;
  return Param->Result;
}

void TTerminal::CalculateFilesChecksum(UnicodeString Alg,
  TStrings * AFileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, AFileList, Checksums, OnCalculatedChecksum);
}

void TTerminal::TerminalRenameFile(UnicodeString AFileName,
  UnicodeString ANewName)
{
  LogEvent(FORMAT("Renaming file \"%s\" to \"%s\".", AFileName, ANewName));
  DoRenameFile(AFileName, ANewName, false);
  ReactOnCommand(fsRenameFile);
}

void TTerminal::TerminalRenameFile(const TRemoteFile * AFile,
  UnicodeString ANewName, bool CheckExistence)
{
  DebugAssert(AFile && AFile->GetDirectory() == FFiles);
  bool Proceed = true;
  // if filename doesn't contain path, we check for existence of file
  if ((AFile && AFile->GetFileName() != ANewName) && CheckExistence &&
    FConfiguration->GetConfirmOverwriting() &&
    base::UnixSamePath(RemoteGetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * DuplicateFile = FFiles->FindFile(ANewName);
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
    TerminalRenameFile(AFile->GetFileName(), ANewName);
  }
}

void TTerminal::DoRenameFile(UnicodeString AFileName,
  UnicodeString ANewName, bool Move)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TMvSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true), GetAbsolutePath(ANewName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteRenameFile(AFileName, ANewName);
    }
    catch (Exception & E)
    {
      UnicodeString Message = FMTLOAD(Move ? MOVE_FILE_ERROR : RENAME_FILE_ERROR, AFileName, ANewName);
      RetryLoop.Error(E, Action, Message);
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::TerminalMoveFile(UnicodeString AFileName,
  const TRemoteFile * AFile, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(AFileName, foRemoteMove, foDelete);
  DebugAssert(Param != nullptr);
  const TMoveFileParams & Params = *get_as<TMoveFileParams>(Param);
  UnicodeString NewName = base::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT("Moving file \"%s\" to \"%s\".", AFileName, NewName));
  FileModified(AFile, AFileName);
  DoRenameFile(AFileName, NewName, true);
  ReactOnCommand(fsMoveFile);
}

bool TTerminal::MoveFiles(TStrings * AFileList, UnicodeString Target,
  UnicodeString FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  bool Result;

  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT2(TTerminal::AfterMoveFiles, (void *)AFileList);
    // nb::FastDelegate1<void, TStrings *> func = nb::bind(&TTerminal::AfterMoveFiles, this, AFileList);
    Result = ProcessFiles(AFileList, foRemoteMove, nb::bind(&TTerminal::TerminalMoveFile, this), &Params);
  }
  __finally
  {
#if 0
    if (Active)
    {
      UnicodeString WithTrailing = UnixIncludeTrailingBackslash(CurrentDirectory);
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      for (int Index = 0; !PossiblyMoved && (Index < FileList->Count); Index++)
      {
        const TRemoteFile * File =
          dynamic_cast<const TRemoteFile *>(FileList->Objects[Index]);
        // File can be nullptr, and filename may not be full path,
        // but currently this is the only way we can move (at least in GUI)
        // current directory
        if ((File != nullptr) &&
            File->IsDirectory &&
            ((CurrentDirectory.SubString(1, FileList->Strings[Index].Length()) == FileList->Strings[Index]) &&
             ((FileList->Strings[Index].Length() == CurrentDirectory.Length()) ||
              (CurrentDirectory[FileList->Strings[Index].Length() + 1] == L'/'))))
        {
          PossiblyMoved = true;
        }
      }

      if (PossiblyMoved && !FileExists(CurrentDirectory))
      {
        UnicodeString NearestExisting = CurrentDirectory;
        do
        {
          NearestExisting = UnixExtractFileDir(NearestExisting);
        }
        while (!IsUnixRootPath(NearestExisting) && !FileExists(NearestExisting));

        ChangeDirectory(NearestExisting);
      }
    }
    EndTransaction();
#endif // #if 0
  };
  return Result;
}

void TTerminal::AfterMoveFiles(void * Params)
{
  TStrings * AFileList = reinterpret_cast<TStrings *>(Params);
  {
    if (GetActive())
    {
      // UnicodeString WithTrailing = base::UnixIncludeTrailingBackslash(this->GetCurrDirectory());
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      UnicodeString CurrentDirectory = this->RemoteGetCurrentDirectory();
      for (intptr_t Index = 0; !PossiblyMoved && (Index < AFileList->GetCount()); ++Index)
      {
        const TRemoteFile * File = AFileList->GetAs<TRemoteFile>(Index);
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
}

void TTerminal::DoCopyFile(UnicodeString AFileName,
  UnicodeString ANewName)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      DebugAssert(FFileSystem);
      if (GetIsCapable(fcRemoteCopy))
      {
        FFileSystem->RemoteCopyFile(AFileName, ANewName);
      }
      else
      {
        DebugAssert(GetCommandSessionOpened());
        DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
        LogEvent("Copying file on command session.");
        FCommandSession->TerminalSetCurrentDirectory(RemoteGetCurrentDirectory());
        FCommandSession->FFileSystem->RemoteCopyFile(AFileName, ANewName);
      }
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(COPY_FILE_ERROR, AFileName, ANewName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::TerminalCopyFile(UnicodeString AFileName,
  const TRemoteFile * /*File*/, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(AFileName, foRemoteCopy);
  DebugAssert(Param != nullptr);
  const TMoveFileParams & Params = *get_as<TMoveFileParams>(Param);
  UnicodeString NewName = base::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT("Copying file \"%s\" to \"%s\".", AFileName, NewName));
  DoCopyFile(AFileName, NewName);
  ReactOnCommand(fsCopyFile);
}

bool TTerminal::CopyFiles(const TStrings * AFileList, UnicodeString Target,
  UnicodeString FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  return ProcessFiles(AFileList, foRemoteCopy, nb::bind(&TTerminal::TerminalCopyFile, this), &Params);
}

void TTerminal::RemoteCreateDirectory(UnicodeString ADirName,
  const TRemoteProperties * Properties)
{
  DebugAssert(FFileSystem);
  EnsureNonExistence(ADirName);
  FileModified(nullptr, ADirName);

  LogEvent(FORMAT("Creating directory \"%s\".", ADirName));
  DoCreateDirectory(ADirName);

  if ((Properties != nullptr) && !Properties->Valid.Empty())
  {
    DoChangeFileProperties(ADirName, nullptr, Properties);
  }

  ReactOnCommand(fsCreateDirectory);
}

void TTerminal::DoCreateDirectory(UnicodeString ADirName)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TMkdirSessionAction Action(GetActionLog(), GetAbsolutePath(ADirName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteCreateDirectory(ADirName);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CREATE_DIR_ERROR, ADirName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::CreateLink(UnicodeString AFileName,
  UnicodeString PointTo, bool Symbolic, bool IsDirectory)
{
  DebugAssert(FFileSystem);
  EnsureNonExistence(AFileName);
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(RemoteGetCurrentDirectory(), false);
  }

  LogEvent(FORMAT("Creating link \"%s\" to \"%s\" (symbolic: %s).",
    AFileName, PointTo, BooleanToEngStr(Symbolic)));
  DoCreateLink(AFileName, PointTo, Symbolic);
  ReactOnCommand(fsCreateDirectory);
}

void TTerminal::DoCreateLink(UnicodeString AFileName,
  UnicodeString PointTo, bool Symbolic)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->CreateLink(AFileName, PointTo, Symbolic);
    }
    catch (Exception & E)
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
  catch (Exception & E)
  {
    CommandError(&E, LoadStr(CHANGE_HOMEDIR_ERROR));
  }
}

void TTerminal::RemoteChangeDirectory(UnicodeString Directory)
{
  UnicodeString DirectoryNormalized = base::ToUnixPath(Directory);
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
  catch (Exception & E)
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
    catch (Exception & E)
    {
      if (!GetActive() || (GetSessionData()->GetLookupUserGroups() == asOn))
      {
        CommandError(&E, LoadStr(LOOKUP_GROUPS_ERROR));
      }
    }
  }
}

bool TTerminal::AllowedAnyCommand(UnicodeString Command) const
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

TTerminal * TTerminal::CreateSecondarySession(UnicodeString Name, TSessionData * SessionData)
{
  std::unique_ptr<TSecondaryTerminal> Result(new TSecondaryTerminal(this));
  Result->Init(SessionData, FConfiguration, Name);

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

void TTerminal::FillSessionDataForCode(TSessionData * Data) const
{
  const TSessionInfo & SessionInfo = GetSessionInfo();
  Data->SetHostKey(SessionInfo.HostKeyFingerprint);
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

    TTerminal * CommandSession = CreateSecondarySession(L"Shell", CommandSessionData.get());
    try
    {
      CommandSession->SetAutoReadDirectory(false);

      CommandSession->SetExceptionOnFail(FExceptionOnFail > 0);

      CommandSession->SetOnQueryUser(GetOnQueryUser());
      CommandSession->SetOnPromptUser(GetOnPromptUser());
      CommandSession->SetOnShowExtendedException(GetOnShowExtendedException());
      CommandSession->SetOnProgress(GetOnProgress());
      CommandSession->SetOnFinished(GetOnFinished());
      CommandSession->SetOnInformation(GetOnInformation());
      CommandSession->SetOnCustomCommand(GetOnCustomCommand());
    }
    catch (...)
    {
      SAFE_DESTROY(CommandSession);
    }

    FCommandSession = CommandSession;
    // do not copy OnDisplayBanner to avoid it being displayed
  }

  return FCommandSession;
}

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated

class TOutputProxy : public TObject
{
public:
  TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) :
    FAction(Action),
    FOutputEvent(OutputEvent)
  {
  }

  void Output(UnicodeString Str, TCaptureOutputType OutputType)
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
  TCallSessionAction & FAction;
  TCaptureOutputEvent FOutputEvent;
};

#pragma warning(pop)

void TTerminal::AnyCommand(UnicodeString Command,
  TCaptureOutputEvent OutputEvent)
{
#if 0
  #pragma warn -inl
  class TOutputProxy
  {
  public:
    TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) :
      FAction(Action),
      FOutputEvent(OutputEvent)
    {
    }

    void Output(UnicodeString Str, TCaptureOutputType OutputType)
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
    TCallSessionAction & FAction;
    TCaptureOutputEvent FOutputEvent;
  };
  #pragma warn .inl
#endif // #if 0

  TCallSessionAction Action(GetActionLog(), Command, RemoteGetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, nb::bind(&TOutputProxy::Output, &ProxyOutputEvent), &Action);
}

void TTerminal::DoAnyCommand(UnicodeString ACommand,
  TCaptureOutputEvent OutputEvent, TCallSessionAction * Action)
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
  catch (Exception & E)
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

bool TTerminal::DoCreateLocalFile(UnicodeString AFileName,
  TFileOperationProgressType * OperationProgress,
  bool Resume,
  bool NoConfirmation,
  HANDLE * AHandle)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  bool Done;
  DWORD DesiredAccess = GENERIC_WRITE;
  DWORD ShareMode = FILE_SHARE_READ;
  DWORD CreationDisposition = Resume ? OPEN_ALWAYS : CREATE_ALWAYS;
  DWORD FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
  do
  {
    *AHandle = TerminalCreateLocalFile(ApiPath(AFileName), DesiredAccess, ShareMode,
      CreationDisposition, FlagsAndAttributes);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      // save the error, otherwise it gets overwritten by call to FileExists
      int LastError = ::GetLastError();
      DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
      if (::FileExists(ApiPath(AFileName)) &&
        (((LocalFileAttrs = GetLocalFileAttributes(ApiPath(AFileName))) & (faReadOnly | faHidden)) != 0))
      {
        if (FLAGSET(LocalFileAttrs, faReadOnly))
        {
          OperationProgress->LockUserSelections();
          try__finally
          {
            SCOPE_EXIT
            {
              OperationProgress->UnlockUserSelections();
            };
            if (OperationProgress->GetBatchOverwrite() == boNone)
            {
              Result = false;
            }
            else if ((OperationProgress->GetBatchOverwrite() != boAll) && !NoConfirmation)
            {
              uintptr_t Answer;
              {
                TSuspendFileOperationProgress Suspend(OperationProgress);
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
          }
          __finally
          {
#if 0
            OperationProgress->UnlockUserSelections();
#endif
          };
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

          FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, AFileName), "",
          [&]()
          {
            if (!this->SetLocalFileAttributes(ApiPath(AFileName), LocalFileAttrs & ~(faReadOnly | faHidden)))
            {
              ::RaiseLastOSError();
            }
          });
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

bool TTerminal::TerminalCreateLocalFile(UnicodeString ATargetFileName,
  TFileOperationProgressType * OperationProgress,
  bool Resume,
  bool NoConfirmation,
  HANDLE * AHandle)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CREATE_FILE_ERROR, ATargetFileName), "",
  [&]()
  {
    Result = DoCreateLocalFile(ATargetFileName, OperationProgress, Resume, NoConfirmation,
      AHandle);
  });

  return Result;
}

void TTerminal::TerminalOpenLocalFile(UnicodeString ATargetFileName,
  DWORD Access, uintptr_t * AAttrs, HANDLE * AHandle, int64_t * ACTime,
  int64_t * AMTime, int64_t * AATime, int64_t * ASize,
  bool TryWriteReadOnly)
{
  DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();

  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(FILE_NOT_EXISTS, ATargetFileName), "",
  [&]()
  {
    UnicodeString FileNameApi = ApiPath(ATargetFileName);
    LocalFileAttrs = this->GetLocalFileAttributes(FileNameApi);
    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      ::RaiseLastOSError();
    }
  });

  if (FLAGCLEAR(LocalFileAttrs, faDirectory) || (AHandle == INVALID_HANDLE_VALUE))
  {
    bool NoHandle = false;
    if (!TryWriteReadOnly && (Access == GENERIC_WRITE) &&
      ((LocalFileAttrs & faReadOnly) != 0))
    {
      Access = GENERIC_READ;
      NoHandle = true;
    }

    FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(OPENFILE_ERROR, ATargetFileName), "",
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

    try
    {
      if (AATime || AMTime || ACTime)
      {
        FILETIME ATime;
        FILETIME MTime;
        FILETIME CTime;

        // Get last file access and modification time
        FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_GET_ATTRS, ATargetFileName), "",
        [&]()
        {
          THROWOSIFFALSE(::GetFileTime(LocalFileHandle, &CTime, &ATime, &MTime));
        });
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
        FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_GET_ATTRS, ATargetFileName), "",
        [&]()
        {
          uint32_t LSize;
          DWORD HSize;
          LSize = ::GetFileSize(LocalFileHandle, &HSize);
          if ((LSize == static_cast<uint32_t>(-1)) && (::GetLastError() != NO_ERROR))
          {
            ::RaiseLastOSError();
          }
          *ASize = (static_cast<int64_t>(HSize) << 32) + LSize;
        });
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

bool TTerminal::AllowLocalFileTransfer(UnicodeString AFileName,
  const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress)
{
  bool Result = true;
  // optimization
  if (GetLog()->GetLogging() || !CopyParam->AllowAnyTransfer())
  {
    WIN32_FIND_DATA FindData = {};
    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
    FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(FILE_NOT_EXISTS, AFileName), "",
    [&]()
    {
      LocalFileHandle = ::FindFirstFileW(ApiPath(::ExcludeTrailingBackslash(AFileName)).c_str(), &FindData);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        ::RaiseLastOSError();
      }
    });
    ::FindClose(LocalFileHandle);
    bool Directory = FLAGSET(FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    TFileMasks::TParams Params;
    // SearchRec.Size in C++B2010 is int64_t,
    // so we should be able to use it instead of FindData.nFileSize*
    Params.Size =
      (static_cast<int64_t>(FindData.nFileSizeHigh) << 32) +
      FindData.nFileSizeLow;
    Params.Modification = ::FileTimeToDateTime(FindData.ftLastWriteTime);
    UnicodeString BaseFileName = GetBaseFileName(AFileName);
    if (!CopyParam->AllowTransfer(BaseFileName, osLocal, Directory, Params))
    {
      LogEvent(FORMAT("File \"%s\" excluded from transfer", AFileName));
      Result = false;
    }
    else if (CopyParam->SkipTransfer(AFileName, Directory))
    {
      OperationProgress->AddSkippedFileSize(Params.Size);
      Result = false;
    }

    if (Result)
    {
      LogFileDetails(AFileName, Params.Modification, Params.Size);
    }
  }
  return Result;
}

void TTerminal::MakeLocalFileList(UnicodeString AFileName,
  const TSearchRec & Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *get_as<TMakeLocalFileListParams>(Param);

  bool Directory = FLAGSET(Rec.Attr, faDirectory);
  if (Directory && Params.Recursive)
  {
    ProcessLocalDirectory(AFileName, nb::bind(&TTerminal::MakeLocalFileList, this), &Params);
  }

  if (!Directory || Params.IncludeDirs)
  {
    Params.FileList->Add(AFileName);
    if (Params.FileTimes != nullptr)
    {
      TODO("Add TSearchRec::TimeStamp");
      // Params.FileTimes->push_back(const_cast<TSearchRec &>(Rec).TimeStamp);
    }
  }
}

void TTerminal::CalculateLocalFileSize(UnicodeString AFileName,
  const TSearchRec & Rec, /*int64_t*/ void * AParams)
{
  TCalculateSizeParams * Params = get_as<TCalculateSizeParams>(AParams);

  if (!TryStartOperationWithFile(AFileName, foCalculateSize))
  {
    Params->Result = false;
  }
  else
  {
    try
    {
      bool Dir = FLAGSET(Rec.Attr, faDirectory);

      bool AllowTransfer = (Params->CopyParam == nullptr);
      // SearchRec.Size in C++B2010 is int64_t,
      // so we should be able to use it instead of FindData.nFileSize*
      int64_t Size =
        (static_cast<int64_t>(Rec.FindData.nFileSizeHigh) << 32) +
        Rec.FindData.nFileSizeLow;
      if (!AllowTransfer)
      {
        TFileMasks::TParams MaskParams;
        MaskParams.Size = Size;
        MaskParams.Modification = ::FileTimeToDateTime(Rec.FindData.ftLastWriteTime);

        UnicodeString BaseFileName = GetBaseFileName(AFileName);
        AllowTransfer = Params->CopyParam->AllowTransfer(BaseFileName, osLocal, Dir, MaskParams);
      }

      if (AllowTransfer)
      {
        intptr_t CollectionIndex = -1;
        if (Params->Files != nullptr)
        {
          UnicodeString FullFileName = ::ExpandUNCFileName(AFileName);
          CollectionIndex = Params->Files->Add(FullFileName, nullptr, Dir);
        }

        if (!Dir)
        {
          Params->Size += Size;
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

bool TTerminal::CalculateLocalFilesSize(const TStrings * AFileList,
  const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files,
  int64_t & Size)
{
  bool Result;
  TFileOperationProgressType OperationProgress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationProgress.Start(foCalculateSize, osLocal, AFileList->GetCount());
  try__finally
  {
    SCOPE_EXIT
    {
      FOperationProgress = nullptr;
      OperationProgress.Stop();
    };
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;
    Params.Files = nullptr;
    Params.Result = true;

    DebugAssert(!FOperationProgress);
    FOperationProgress = &OperationProgress; //-V506
    UnicodeString LastDirPath;
    for (intptr_t Index = 0; Params.Result && (Index < AFileList->GetCount()); ++Index)
    {
      UnicodeString FileName = AFileList->GetString(Index);
      TSearchRec Rec;
      if (FileSearchRec(FileName, Rec))
      {
        bool Dir = FLAGSET(Rec.Attr, faDirectory);
        if (Dir && !AllowDirs)
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

          CalculateLocalFileSize(FileName, Rec, &Params);

          OperationProgress.Finish(FileName, true, OnceDoneOperation);
        }
      }
    }

    Size = Params.Size;
    Result = Params.Result;
  }
  __finally
  {
#if 0
    FOperationProgress = nullptr;
    OperationProgress.Stop();
#endif
  };

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
  return Result;
}

struct TSynchronizeFileData : public TObject
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSynchronizeFileData); }
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
  TRemoteFile * MatchingRemoteFileFile;
  intptr_t MatchingRemoteFileImageIndex;
  FILETIME LocalLastWriteTime;
};

const intptr_t sfFirstLevel = 0x01;

struct TSynchronizeData : public TObject
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSynchronizeData); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSynchronizeData) || TObject::is(Kind); }
public:
  TSynchronizeData() : TObject(OBJECT_CLASS_TSynchronizeData)
  {
  }

  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  TTerminal::TSynchronizeMode Mode;
  intptr_t Params;
  TSynchronizeDirectoryEvent OnSynchronizeDirectory;
  TSynchronizeOptions * Options;
  intptr_t Flags;
  TStringList * LocalFileList;
  const TCopyParamType * CopyParam;
  TSynchronizeChecklist * Checklist;

  void DeleteLocalFileList()
  {
    if (LocalFileList != nullptr)
    {
      for (intptr_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
      {
        TSynchronizeFileData * FileData = LocalFileList->GetAs<TSynchronizeFileData>(Index);
        SAFE_DESTROY(FileData);
      }
      SAFE_DESTROY(LocalFileList);
    }
  }
};

TSynchronizeChecklist * TTerminal::SynchronizeCollect(UnicodeString LocalDirectory,
  UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
  TSynchronizeOptions * Options)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  std::unique_ptr<TSynchronizeChecklist> Checklist(new TSynchronizeChecklist());
  try__catch
  {
    DoSynchronizeCollectDirectory(LocalDirectory, RemoteDirectory, Mode,
      CopyParam, Params, OnSynchronizeDirectory, Options, sfFirstLevel,
      Checklist.get());
    Checklist->Sort();
  }
#if 0
  catch(...)
  {
    delete Checklist;
    throw;
  }
#endif // #if 0
  return Checklist.release();
}

static void AddFlagName(UnicodeString & ParamsStr, intptr_t & Params, intptr_t Param, UnicodeString Name)
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

void TTerminal::DoSynchronizeCollectDirectory(UnicodeString ALocalDirectory,
  UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options,
  intptr_t Level, TSynchronizeChecklist * Checklist)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  TSynchronizeData Data;

  Data.LocalDirectory = ::IncludeTrailingBackslash(ALocalDirectory);
  Data.RemoteDirectory = base::UnixIncludeTrailingBackslash(ARemoteDirectory);
  Data.Mode = Mode;
  Data.Params = Params;
  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;
  Data.LocalFileList = nullptr;
  Data.CopyParam = CopyParam;
  Data.Options = Options;
  Data.Flags = Level;
  Data.Checklist = Checklist;

  LogEvent(FORMAT("Collecting synchronization list for local directory '%s' and remote directory '%s', "
    "mode = %s, params = 0x%x (%s), file mask = '%s'", ALocalDirectory, ARemoteDirectory,
    SynchronizeModeStr(Mode), int(Params), SynchronizeParamsStr(Params), CopyParam->GetIncludeFileMask().GetMasks())
  );

  if (FLAGCLEAR(Params, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  try__finally
  {
    SCOPE_EXIT
    {
      Data.DeleteLocalFileList();
    };
    bool Found = false;
    TSearchRecChecked SearchRec;
    Data.LocalFileList = CreateSortedStringList();

    FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, ALocalDirectory), "",
    [&]()
    {
      DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      Found = ::FindFirstChecked(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0;
    });

    if (Found)
    {
      try__finally
      {
        SCOPE_EXIT
        {
          base::FindClose(SearchRec);
        };
        while (Found)
        {
          UnicodeString FileName = SearchRec.Name;
          // add dirs for recursive mode or when we are interested in newly
          // added subdirs
          // SearchRec.Size in C++B2010 is int64_t,
          // so we should be able to use it instead of FindData.nFileSize*
          int64_t Size =
            (static_cast<int64_t>(SearchRec.FindData.nFileSizeHigh) << 32) +
            SearchRec.FindData.nFileSizeLow;
          TDateTime Modification = ::FileTimeToDateTime(SearchRec.FindData.ftLastWriteTime);
          TFileMasks::TParams MaskParams;
          MaskParams.Size = Size;
          MaskParams.Modification = Modification;
          UnicodeString RemoteFileName =
            ChangeFileName(CopyParam, FileName, osLocal, false);
          UnicodeString FullLocalFileName = Data.LocalDirectory + FileName;
          UnicodeString BaseFileName = GetBaseFileName(FullLocalFileName);
          if ((FileName != THISDIRECTORY) && (FileName != PARENTDIRECTORY) &&
              CopyParam->AllowTransfer(BaseFileName, osLocal,
                FLAGSET(SearchRec.Attr, faDirectory), MaskParams) &&
              !FFileSystem->TemporaryTransferFile(FileName) &&
              (FLAGCLEAR(Level, sfFirstLevel) ||
               (Options == nullptr) ||
               Options->MatchesFilter(FileName) ||
               Options->MatchesFilter(RemoteFileName)))
          {
            TSynchronizeFileData * FileData = new TSynchronizeFileData();

            FileData->IsDirectory = FLAGSET(SearchRec.Attr, faDirectory);
            FileData->Info.FileName = FileName;
            FileData->Info.Directory = Data.LocalDirectory;
            FileData->Info.Modification = Modification;
            FileData->Info.ModificationFmt = mfFull;
            FileData->Info.Size = Size;
            FileData->LocalLastWriteTime = SearchRec.FindData.ftLastWriteTime;
            FileData->New = true;
            FileData->Modified = false;
            Data.LocalFileList->AddObject(FileName, FileData);
            LogEvent(FORMAT("Local file %s included to synchronization",
              FormatFileDetailsForLog(FullLocalFileName, Modification, Size)));
          }
          else
          {
            LogEvent(FORMAT("Local file %s excluded from synchronization",
              FormatFileDetailsForLog(FullLocalFileName, Modification, Size)));
          }

          FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, ALocalDirectory), "",
          [&]()
          {
            Found = (::FindNextChecked(SearchRec) == 0);
          });
        }
      }
      __finally
      {
#if 0
        FindClose(SearchRec);
#endif // #if 0
      };

      // can we expect that ProcessDirectory would take so little time
      // that we can postpone showing progress window until anything actually happens?
      bool Cached = FLAGSET(Params, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(ARemoteDirectory);

      if (!Cached && FLAGSET(Params, spDelayProgress))
      {
        DoSynchronizeProgress(Data, true);
      }

      ProcessDirectory(ARemoteDirectory, nb::bind(&TTerminal::SynchronizeCollectFile, this), &Data,
        FLAGSET(Params, spUseCache));

      TSynchronizeFileData * FileData;
      for (intptr_t Index = 0; Index < Data.LocalFileList->GetCount(); ++Index)
      {
        FileData = Data.LocalFileList->GetAs<TSynchronizeFileData>(Index);
        // add local file either if we are going to upload it
        // (i.e. if it is updated or we want to upload even new files)
        // or if we are going to delete it (i.e. all "new"=obsolete files)
        bool Modified = (FileData->Modified && ((Mode == smBoth) || (Mode == smRemote)));
        bool New = (FileData->New &&
          ((Mode == smLocal) ||
           (((Mode == smBoth) || (Mode == smRemote)) && FLAGCLEAR(Params, spTimestamp))));

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
                (Modified ? saUploadUpdate : saUploadNew);
              ChecklistItem->Checked =
                (Modified || FLAGCLEAR(Params, spExistingOnly)) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }
            else if ((Mode == smLocal) && FLAGCLEAR(Params, spTimestamp))
            {
              ChecklistItem->Action = saDeleteLocal;
              ChecklistItem->Checked =
                FLAGSET(Params, spDelete) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }

            if (ChecklistItem->Action != saNone)
            {
              Data.Checklist->Add(ChecklistItem.get());
              ChecklistItem.release();
            }
          }
          __finally
          {
#if 0
            delete ChecklistItem;
#endif // #if 0
          };
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
  }
  __finally
  {
#if 0
    if (Data.LocalFileList != nullptr)
    {
      for (int Index = 0; Index < Data.LocalFileList->Count; Index++)
      {
        TSynchronizeFileData * FileData = reinterpret_cast<TSynchronizeFileData*>
          (Data.LocalFileList->Objects[Index]);
        delete FileData;
      }
      delete Data.LocalFileList;
    }
#endif // #if 0
  };
}

void TTerminal::SynchronizeCollectFile(UnicodeString AFileName,
  const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  try
  {
    DoSynchronizeCollectFile(AFileName, AFile, Param);
  }
  catch (ESkipFile & E)
  {
    TSuspendFileOperationProgress Suspend(OperationProgress);
    if (!HandleException(&E))
    {
      throw;
    }
  }
}

void TTerminal::DoSynchronizeCollectFile(UnicodeString /*AFileName*/,
  const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData * Data = get_as<TSynchronizeData>(Param);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();
  MaskParams.Modification = AFile->GetModification();
  UnicodeString LocalFileName =
    ChangeFileName(Data->CopyParam, AFile->GetFileName(), osRemote, false);
  UnicodeString FullRemoteFileName =
    base::UnixExcludeTrailingBackslash(AFile->GetFullFileName());
  UnicodeString BaseFileName = GetBaseFileName(FullRemoteFileName);
  if (Data->CopyParam->AllowTransfer(
      BaseFileName, osRemote,
      AFile->GetIsDirectory(), MaskParams) &&
    !FFileSystem->TemporaryTransferFile(AFile->GetFileName()) &&
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
          TSynchronizeFileData * LocalData =
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
                 FormatFileDetailsForLog(FullRemoteFileName,
                 AFile->GetModification(),
                 AFile->GetSize()),
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
              (Modified ? saDownloadUpdate : saDownloadNew);
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
    }
    __finally
    {
#if 0
      delete ChecklistItem;
#endif // #if 0
    };
  }
  else
  {
    LogEvent(FORMAT("Remote file %s excluded from synchronization",
       FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize())));
  }
}

void TTerminal::SynchronizeApply(TSynchronizeChecklist * Checklist,
  UnicodeString /*LocalDirectory*/, UnicodeString /*RemoteDirectory*/,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory)
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

  std::unique_ptr<TStringList> DownloadList(new TStringList());
  std::unique_ptr<TStringList> DeleteRemoteList(new TStringList());
  std::unique_ptr<TStringList> UploadList(new TStringList());
  std::unique_ptr<TStringList> DeleteLocalList(new TStringList());

  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      EndTransaction();
    };
    intptr_t IIndex = 0;
    while (IIndex < Checklist->GetCount())
    {
      DownloadList->Clear();
      DeleteRemoteList->Clear();
      UploadList->Clear();
      DeleteLocalList->Clear();

      const TChecklistItem * ChecklistItem = Checklist->GetItem(IIndex);

      UnicodeString CurrentLocalDirectory = ChecklistItem->Local.Directory;
      UnicodeString CurrentRemoteDirectory = ChecklistItem->Remote.Directory;

      LogEvent(FORMAT("Synchronizing local directory '%s' with remote directory '%s', "
        "params = 0x%x (%s)", CurrentLocalDirectory, CurrentRemoteDirectory,
        int(Params), SynchronizeParamsStr(Params)));

      intptr_t Count = 0;

      while ((IIndex < Checklist->GetCount()) &&
        (Checklist->GetItem(IIndex)->Local.Directory == CurrentLocalDirectory) &&
        (Checklist->GetItem(IIndex)->Remote.Directory == CurrentRemoteDirectory))
      {
        ChecklistItem = Checklist->GetItem(IIndex);
        if (ChecklistItem->Checked)
        {
          Count++;

          if (FLAGSET(Params, spTimestamp))
          {
            switch (ChecklistItem->Action)
            {
            case saDownloadUpdate:
              DownloadList->AddObject(
                base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                ChecklistItem->Remote.FileName,
                const_cast<TChecklistItem *>(ChecklistItem));
              break;

            case saUploadUpdate:
              UploadList->AddObject(
                ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                ChecklistItem->Local.FileName,
                const_cast<TChecklistItem *>(ChecklistItem));
              break;

            default:
              DebugFail();
              break;
            }
          }
          else
          {
            switch (ChecklistItem->Action)
            {
            case saDownloadNew:
            case saDownloadUpdate:
              DownloadList->AddObject(
                base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                ChecklistItem->Remote.FileName,
                ChecklistItem->RemoteFile);
              break;

            case saDeleteRemote:
              DeleteRemoteList->AddObject(
                base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                ChecklistItem->Remote.FileName,
                ChecklistItem->RemoteFile);
              break;

            case saUploadNew:
            case saUploadUpdate:
              UploadList->Add(
                ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                ChecklistItem->Local.FileName);
              break;

            case saDeleteLocal:
              DeleteLocalList->Add(
                ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                ChecklistItem->Local.FileName);
              break;

            default:
              DebugFail();
              break;
            }
          }
        }
        ++IIndex;
      }

      // prevent showing/updating of progress dialog if there's nothing to do
      if (Count > 0)
      {
        Data.LocalDirectory = ::IncludeTrailingBackslash(CurrentLocalDirectory);
        Data.RemoteDirectory = base::UnixIncludeTrailingBackslash(CurrentRemoteDirectory);
        DoSynchronizeProgress(Data, false);

        if (FLAGSET(Params, spTimestamp))
        {
          if (DownloadList->GetCount() > 0)
          {
            ProcessFiles(DownloadList.get(), foSetProperties,
              nb::bind(&TTerminal::SynchronizeLocalTimestamp, this), nullptr, osLocal);
          }

          if (UploadList->GetCount() > 0)
          {
            ProcessFiles(UploadList.get(), foSetProperties,
              nb::bind(&TTerminal::SynchronizeRemoteTimestamp, this));
          }
        }
        else
        {
          if ((DownloadList->GetCount() > 0) &&
              !CopyToLocal(DownloadList.get(), Data.LocalDirectory, &SyncCopyParam, CopyParams, nullptr))
          {
            Abort();
          }

          if ((DeleteRemoteList->GetCount() > 0) &&
              !RemoteDeleteFiles(DeleteRemoteList.get()))
          {
            Abort();
          }

          if ((UploadList->GetCount() > 0) &&
              !CopyToRemote(UploadList.get(), Data.RemoteDirectory, &SyncCopyParam, CopyParams, nullptr))
          {
            Abort();
          }

          if ((DeleteLocalList->GetCount() > 0) &&
              !DeleteLocalFiles(DeleteLocalList.get()))
          {
            Abort();
          }
        }
      }
    }
  }
  __finally
  {
#if 0
    delete DownloadList;
    delete DeleteRemoteList;
    delete UploadList;
    delete DeleteLocalList;

    EndTransaction();
#endif // #if 0
  };
}

void TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
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

void TTerminal::SynchronizeLocalTimestamp(UnicodeString /*FileName*/,
  const TRemoteFile * AFile, void * /*Param*/)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();

  const TChecklistItem * ChecklistItem =
    reinterpret_cast<const TChecklistItem *>(AFile);

  UnicodeString LocalFile =
    ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
      ChecklistItem->Local.FileName;
  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, LocalFile), "",
  [&]()
  {
    this->SetLocalFileTime(LocalFile, ChecklistItem->Remote.Modification);
#if 0
    HANDLE LocalFileHandle;
    this->TerminalOpenLocalFile(LocalFile, GENERIC_WRITE, &LocalFileHandle, nullptr,
      nullptr, nullptr, nullptr, nullptr);
    FILETIME WrTime = DateTimeToFileTime(ChecklistItem->Remote.Modification,
      GetSessionData()->GetDSTMode());
    bool Result = ::SetFileTime(LocalFileHandle, nullptr, nullptr, &WrTime);
    int Error = ::GetLastError();
    SAFE_CLOSE_HANDLE(LocalFileHandle);
    if (!Result)
    {
      ::RaiseLastOSError(Error);
    }
#endif // #if 0
  });
}

void TTerminal::SynchronizeRemoteTimestamp(UnicodeString /*AFileName*/,
  const TRemoteFile * AFile, void * /*Param*/)
{
  const TChecklistItem * ChecklistItem =
    reinterpret_cast<const TChecklistItem *>(AFile);

  TRemoteProperties Properties;
  Properties.Valid << vpModification;
  Properties.Modification = ::ConvertTimestampToUnix(ChecklistItem->FLocalLastWriteTime,
    GetSessionData()->GetDSTMode());

  ChangeFileProperties(
    base::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) + ChecklistItem->Remote.FileName,
    nullptr, &Properties);
}

void TTerminal::FileFind(UnicodeString AFileName,
  const TRemoteFile * AFile, /*TFilesFindParams*/ void * Param)
{
  // see DoFilesFind
  FOnFindingFile = nullptr;

  DebugAssert(Param);
  DebugAssert(AFile);
  TFilesFindParams * AParams = get_as<TFilesFindParams>(Param);

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

void TTerminal::DoFilesFind(UnicodeString Directory, TFilesFindParams & Params, UnicodeString RealDirectory)
{
  LogEvent(FORMAT("Searching directory \"%s\" (real path \"%s\")", Directory, RealDirectory));
  Params.OnFindingFile(this, Directory, Params.Cancel);
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
      SCOPE_EXIT
      {
        Params.RealDirectory = PrevRealDirectory;
        FOnFindingFile = nullptr;
      };
      Params.RealDirectory = RealDirectory;
      ProcessDirectory(Directory, nb::bind(&TTerminal::FileFind, this), &Params, false, true);
    }
    __finally
    {
#if 0
      Params.RealDirectory = PrevRealDirectory;
      FOnFindingFile = nullptr;
#endif // #if 0
    };
  }
}

void TTerminal::FilesFind(UnicodeString Directory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;

  Params.LoopDetector.RecordVisitedDirectory(Directory);

  DoFilesFind(Directory, Params, Directory);
}

void TTerminal::SpaceAvailable(UnicodeString APath,
  TSpaceAvailable & ASpaceAvailable)
{
  DebugAssert(GetIsCapable(fcCheckingSpaceAvailable));

  try
  {
    FFileSystem->SpaceAvailable(APath, ASpaceAvailable);
  }
  catch (Exception & E)
  {
    CommandError(&E, FMTLOAD(SPACE_AVAILABLE_ERROR, APath));
  }
}

void TTerminal::LockFile(UnicodeString AFileName,
  const TRemoteFile * AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foLock);

  LogEvent(FORMAT("Locking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoLockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}

void TTerminal::DoLockFile(UnicodeString AFileName, const TRemoteFile * AFile)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FFileSystem->LockFile(AFileName, AFile);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(LOCK_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::UnlockFile(UnicodeString AFileName,
  const TRemoteFile * AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foUnlock);

  LogEvent(FORMAT("Unlocking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoUnlockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}

void TTerminal::DoUnlockFile(UnicodeString AFileName, const TRemoteFile * AFile)
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
    SCOPE_EXIT
    {
      EndTransaction();
    };
    ProcessFiles(AFileList, foLock, nb::bind(&TTerminal::LockFile, this), nullptr);
  }
  __finally
  {
#if 0
    EndTransaction();
#endif // #if 0
  };
}

void TTerminal::UnlockFiles(TStrings * AFileList)
{
  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      EndTransaction();
    };
    ProcessFiles(AFileList, foUnlock, nb::bind(&TTerminal::UnlockFile, this), nullptr);
  }
  __finally
  {
#if 0
    EndTransaction();
#endif
  };
}

const TSessionInfo & TTerminal::GetSessionInfo() const
{
  return FFileSystem->GetSessionInfo();
}

const TFileSystemInfo & TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}

void TTerminal::GetSupportedChecksumAlgs(TStrings * Algs) const
{
  FFileSystem->GetSupportedChecksumAlgs(Algs);
}

UnicodeString TTerminal::GetPassword() const
{
  UnicodeString Result;
  // FRememberedPassword is empty also when stored password was used
  if (FRememberedPassword.IsEmpty())
  {
    Result = GetSessionData()->GetPassword();
  }
  else
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

intptr_t TTerminal::CopyToParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress)
{
  UnicodeString FileName;
  TObject * Object;
  UnicodeString TargetDir;
  bool Dir;
  bool Recursed;

  intptr_t Result = ParallelOperation->GetNext(this, FileName, Object, TargetDir, Dir, Recursed);
  if (Result > 0)
  {
    std::unique_ptr<TStrings> FilesToCopy(new TStringList());
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
    try__finally
    {
      SCOPE_EXIT
      {
        bool Success = (Prev < OperationProgress->GetFilesFinishedSuccessfully());
        ParallelOperation->Done(FileName, Dir, Success);
        FOperationProgress = nullptr;
      };
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
    }
    __finally
    {
#if 0
      bool Success = (Prev < OperationProgress->FilesFinishedSuccessfully);
      ParallelOperation->Done(FileName, Dir, Success);
      FOperationProgress = nullptr;
#endif // #if 0
    };
  }

  return Result;
}

bool TTerminal::CanParallel(
  const TCopyParamType * CopyParam, intptr_t Params, TParallelOperation * ParallelOperation) const
{
  return
    (ParallelOperation != nullptr) &&
    FFileSystem->IsCapable(fsParallelTransfers) &&
    // parallel transfer is not implemented for operations needed to be done on a folder
    // after all its files are processed
    FLAGCLEAR(Params, cpDelete) &&
    (!CopyParam->GetPreserveTime() || !CopyParam->GetPreserveTimeDirs());
}

void TTerminal::CopyParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress)
{
  try__finally
  {
    SCOPE_EXIT
    {
      OperationProgress->SetDone();
      ParallelOperation->WaitFor();
    };
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
  }
  __finally
  {
#if 0
    OperationProgress->SetDone();
    ParallelOperation->WaitFor();
#endif // #if 0
  };
}

void TTerminal::LogParallelTransfer(TParallelOperation * ParallelOperation)
{
  LogEvent(
    FORMAT(L"Adding a parallel transfer to the transfer started on the connection \"%s\"",
    ParallelOperation->GetMainName()));
}

void TTerminal::LogTotalTransferDetails(
  const UnicodeString TargetDir, const TCopyParamType * CopyParam,
  TFileOperationProgressType * OperationProgress, bool Parallel, TStrings * Files)
{
  if (FLog->GetLogging())
  {
    UnicodeString TargetSide = ((OperationProgress->GetSide() == osLocal) ? L"remote" : L"local");
    UnicodeString S =
      FORMAT(
        L"Copying %d files/directories to %s directory \"%s\"",
        OperationProgress->GetCount(), TargetSide, TargetDir);
    if (Parallel && DebugAlwaysTrue(Files != nullptr))
    {
      intptr_t Count = 0;
      for (intptr_t Index = 0; Index < Files->GetCount(); ++Index)
      {
        TCollectedFileList * FileList = dyn_cast<TCollectedFileList>(Files->GetObj(Index));
        Count += FileList->GetCount();
      }
      S += FORMAT(L" - in parallel, with %d total files", Count);
    }
    if (OperationProgress->GetTotalSizeSet())
    {
      S += FORMAT(L" - total size: %s", FormatSize(OperationProgress->GetTotalSize()));
    }
    LogEvent(S);
    LogEvent(CopyParam->GetLogStr());
  }
}

void TTerminal::LogTotalTransferDone(TFileOperationProgressType * OperationProgress)
{
  LogEvent(L"Copying finished: " + OperationProgress->GetLogStr(true));
}

bool TTerminal::CopyToRemote(const TStrings * AFilesToCopy,
  UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params, TParallelOperation * ParallelOperation)
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
    if (CanParallel(CopyParam, Params, ParallelOperation) &&
        !CopyParam->GetClearArchive())
    {
      Files.reset(new TStringList());
      Files->SetOwnsObjects(true);
    }
    // dirty trick: when moving, do not pass copy param to avoid exclude mask
    bool CalculatedSize =
      CalculateLocalFilesSize(
        AFilesToCopy,
        (FLAGCLEAR(Params, cpDelete) ? CopyParam : nullptr),
        CopyParam->GetCalculateSize(), Files.get(),
        Size);

    FLastProgressLogged = GetTickCount();
    OperationProgress.Start((Params & cpDelete) ? foMove : foCopy, osLocal,
      AFilesToCopy->GetCount(), (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = &OperationProgress; //-V506
#if 0
    bool CollectingUsage = false;
#endif // #if 0
    try__finally
    {
      SCOPE_EXIT
      {
#if 0
        if (GetCollectingUsage())
        {
          int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
          Configuration->Usage->Inc(L"UploadTime", CounterTime);
          Configuration->Usage->SetMax(L"MaxUploadTime", CounterTime);
        }
#endif // #if 0
        OperationProgress.Stop();
        FOperationProgress = nullptr;
      };
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

      UnicodeString UnlockedTargetDir = TranslateLockedPath(TargetDir, false);
      BeginTransaction();
      try__finally
      {
        SCOPE_EXIT
        {
          if (GetActive())
          {
            ReactOnCommand(fsCopyToRemote);
          }
          EndTransaction();
        };

        bool Parallel = CalculatedSize && (Files.get() != nullptr);
        LogTotalTransferDetails(TargetDir, CopyParam, &OperationProgress, Parallel, Files.get());

        if (Parallel)
        {
          // OnceDoneOperation is not supported
          ParallelOperation->Init(Files.release(), UnlockedTargetDir, CopyParam, Params, &OperationProgress, GetLog()->GetName());
          CopyParallel(ParallelOperation, &OperationProgress);
        }
        else
        {
          FFileSystem->CopyToRemote(AFilesToCopy, UnlockedTargetDir,
            CopyParam, Params, &OperationProgress, OnceDoneOperation);
        }

        LogTotalTransferDone(&OperationProgress);
      }
      __finally
      {
#if 0
        if (Active)
        {
          ReactOnCommand(fsCopyToRemote);
        }
        EndTransaction();
#endif // #if 0
      };

      if (OperationProgress.GetCancel() == csContinue)
      {
        Result = true;
      }
    }
    __finally
    {
#if 0
      if (CollectingUsage)
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"UploadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxUploadTime", CounterTime);
      }
      OperationProgress.Stop();
      FOperationProgress = nullptr;
#endif // #if 0
    };
  }
  catch (Exception & E)
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

bool TTerminal::CopyToLocal(const TStrings * AFilesToCopy,
  UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TParallelOperation * ParallelOperation)
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
    SCOPE_EXIT
    {
      // If session is still active (no fatal error) we reload directory
      // by calling EndTransaction
      EndTransaction();
    };
    int64_t TotalSize = 0;
    bool TotalSizeKnown = false;
    FLastProgressLogged = GetTickCount();
    TFileOperationProgressType OperationProgress(nb::bind(&TTerminal::DoProgress, this), nb::bind(&TTerminal::DoFinished, this));

    std::unique_ptr<TStringList> Files;
    if (CanParallel(CopyParam, Params, ParallelOperation))
    {
      Files.reset(new TStringList());
      Files->SetOwnsObjects(true);
    }

    SetExceptionOnFail(true);
    try__finally
    {
      SCOPE_EXIT
      {
        SetExceptionOnFail(false);
      };
      // dirty trick: when moving, do not pass copy param to avoid exclude mask
      TCalculateSizeStats Stats;
      Stats.FoundFiles = Files.get();
      if (CalculateFilesSize(
          AFilesToCopy, TotalSize, csIgnoreErrors,
          (FLAGCLEAR(Params, cpDelete) ? CopyParam : nullptr),
          CopyParam->GetCalculateSize(), Stats))
      {
        TotalSizeKnown = true;
      }
    }
    __finally
    {
#if 0
      ExceptionOnFail = false;
#endif // #if 0
    };

    OperationProgress.Start(((Params & cpDelete) != 0 ? foMove : foCopy), osRemote,
      AFilesToCopy ? AFilesToCopy->GetCount() : 0, (Params & cpTemporary) != 0, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = &OperationProgress; //-V506
    //bool CollectingUsage = false;
    try__finally
    {
      SCOPE_EXIT
      {
#if 0
          if (CollectingUsage)
          {
            int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
            Configuration->Usage->Inc(L"DownloadTime", CounterTime);
            Configuration->Usage->SetMax(L"MaxDownloadTime", CounterTime);
          }
#endif // #if 0
          FOperationProgress = nullptr;
          OperationProgress.Stop();
        };
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
          SCOPE_EXIT
          {
            if (GetActive())
            {
              ReactOnCommand(fsCopyToLocal);
            }
          };
          bool Parallel = TotalSizeKnown && (Files.get() != nullptr);
          LogTotalTransferDetails(TargetDir, CopyParam, &OperationProgress, Parallel, Files.get());

          if (Parallel)
          {
            // OnceDoneOperation is not supported
            ParallelOperation->Init(Files.release(), TargetDir, CopyParam, Params, &OperationProgress, GetLog()->GetName());
            CopyParallel(ParallelOperation, &OperationProgress);
          }
          else
          {
            FFileSystem->CopyToLocal(AFilesToCopy, TargetDir, CopyParam, Params,
              &OperationProgress, OnceDoneOperation);
          }

          LogTotalTransferDone(&OperationProgress);
        }
        __finally
        {
#if 0
          if (Active)
          {
            ReactOnCommand(fsCopyToLocal);
          }
#endif // #if 0
        };
      }
      catch (Exception & E)
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
    }
    __finally
    {
#if 0
      if (CollectingUsage)
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"DownloadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxDownloadTime", CounterTime);
      }
      FOperationProgress = nullptr;
      OperationProgress.Stop();
#endif // #if 0
    };
  }
  __finally
  {
#if 0
    // If session is still active (no fatal error) we reload directory
    // by calling EndTransaction
    EndTransaction();
#endif // #if 0
  };

  if (OnceDoneOperation != odoIdle)
  {
    UnicodeString DestFileName;
    if (!FMultipleDestinationFiles)
    {
      DestFileName = FDestFileName;
    }
    CloseOnCompletion(OnceDoneOperation, UnicodeString(), TargetDir, DestFileName);
  }

  return Result;
}

void TTerminal::SetLocalFileTime(UnicodeString LocalFileName,
  const TDateTime & Modification)
{
  FILETIME WrTime = ::DateTimeToFileTime(Modification,
    GetSessionData()->GetDSTMode());
  SetLocalFileTime(LocalFileName, nullptr, &WrTime);
}

void TTerminal::SetLocalFileTime(UnicodeString LocalFileName,
  FILETIME * AcTime, FILETIME * WrTime)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
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

HANDLE TTerminal::TerminalCreateLocalFile(UnicodeString LocalFileName, DWORD DesiredAccess,
  DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (GetOnCreateLocalFile())
  {
    return GetOnCreateLocalFile()(ApiPath(LocalFileName), DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes);
  }
  return ::CreateFile(ApiPath(LocalFileName).c_str(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, nullptr);
}

DWORD TTerminal::GetLocalFileAttributes(UnicodeString LocalFileName) const
{
  if (GetOnGetLocalFileAttributes())
  {
    return GetOnGetLocalFileAttributes()(ApiPath(LocalFileName));
  }
  return ::FileGetAttrFix(LocalFileName);
}

bool TTerminal::SetLocalFileAttributes(UnicodeString LocalFileName, DWORD FileAttributes)
{
  if (GetOnSetLocalFileAttributes())
  {
    return GetOnSetLocalFileAttributes()(ApiPath(LocalFileName), FileAttributes);
  }
  return ::FileSetAttr(LocalFileName, FileAttributes);
}

bool TTerminal::MoveLocalFile(UnicodeString LocalFileName, UnicodeString NewLocalFileName, DWORD Flags)
{
  if (GetOnMoveLocalFile())
  {
    return GetOnMoveLocalFile()(LocalFileName, NewLocalFileName, Flags);
  }
  return ::MoveFileEx(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), Flags) != FALSE;
}

bool TTerminal::RemoveLocalDirectory(UnicodeString LocalDirName)
{
  if (GetOnRemoveLocalDirectory())
  {
    return GetOnRemoveLocalDirectory()(LocalDirName);
  }
  return RemoveDir(LocalDirName);
}

bool TTerminal::CreateLocalDirectory(UnicodeString LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (GetOnCreateLocalDirectory())
  {
    return GetOnCreateLocalDirectory()(LocalDirName, SecurityAttributes);
  }
  return ::CreateDir(LocalDirName, SecurityAttributes);
}

void TTerminal::ReflectSettings() const
{
  DebugAssert(FLog != nullptr);
  FLog->ReflectSettings();
  DebugAssert(FActionLog != nullptr);
  FActionLog->ReflectSettings();
  // also FTunnelLog ?
}

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
  }

  if (GetConfiguration()->GetLogging() && GetConfiguration()->GetLogToFile())
  {
//    Configuration->Usage->Inc(L"OpenedSessionsLogToFile2");
  }

  if (GetConfiguration()->GetLogActions())
  {
//    Configuration->Usage->Inc(L"OpenedSessionsXmlLog");
  }

  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
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

  FCollectFileSystemUsage = true;
}

bool TTerminal::CheckForEsc() const
{
  if (FOnCheckForEsc)
    return FOnCheckForEsc();
  return (FOperationProgress && FOperationProgress->GetCancel() == csCancel);
}

static UnicodeString FormatCertificateData(UnicodeString Fingerprint, int Failures)
{
  return FORMAT("%s;%2.2X", Fingerprint, Failures);
}

bool TTerminal::VerifyCertificate(
  UnicodeString CertificateStorageKey, UnicodeString SiteKey,
  UnicodeString Fingerprint,
  UnicodeString CertificateSubject, int Failures)
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

void TTerminal::CacheCertificate(UnicodeString CertificateStorageKey,
  UnicodeString SiteKey, UnicodeString Fingerprint, int Failures)
{
  UnicodeString CertificateData = FormatCertificateData(Fingerprint, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(CertificateStorageKey, true))
  {
    Storage->WriteString(SiteKey, CertificateData);
  }
}

void TTerminal::CollectTlsUsage(UnicodeString TlsVersionStr)
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
          LogEvent(L"Certificate is encrypted, need passphrase");
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

UnicodeString TTerminal::GetBaseFileName(UnicodeString AFileName) const
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

UnicodeString TTerminal::ChangeFileName(const TCopyParamType * CopyParam,
  UnicodeString AFileName, TOperationSide Side, bool FirstLevel) const
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

bool TTerminal::IsThisOrChild(TTerminal * Terminal) const
{
  return
    (this == Terminal) ||
    ((FCommandSession != nullptr) && (FCommandSession == Terminal));
}


TSecondaryTerminal::TSecondaryTerminal(TTerminal * MainTerminal) :
  TTerminal(OBJECT_CLASS_TSecondaryTerminal),
  FMainTerminal(MainTerminal)
{
}

TSecondaryTerminal::TSecondaryTerminal(TObjectClassId Kind, TTerminal * MainTerminal) :
  TTerminal(Kind),
  FMainTerminal(MainTerminal)
{
}

void TSecondaryTerminal::Init(
  TSessionData * ASessionData, TConfiguration * AConfiguration, UnicodeString Name)
{
  TTerminal::Init(ASessionData, AConfiguration);
  DebugAssert(FMainTerminal != nullptr);
  GetLog()->SetParent(FMainTerminal->GetLog(), Name);
  GetActionLog()->SetEnabled(false);
  GetSessionData()->NonPersistant();
  if (!FMainTerminal->TerminalGetUserName().IsEmpty())
  {
    GetSessionData()->SetUserName(FMainTerminal->TerminalGetUserName());
  }
}

void TSecondaryTerminal::UpdateFromMain()
{
  if ((FFileSystem != nullptr) && (FMainTerminal->FFileSystem != nullptr))
  {
    FFileSystem->UpdateFromMain(FMainTerminal->FFileSystem);
  }
}

void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  DebugAssert(FileList != nullptr);
}

void TSecondaryTerminal::DirectoryModified(UnicodeString APath,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(APath, SubDirs);
}

TTerminal * TSecondaryTerminal::GetPasswordSource()
{
  return FMainTerminal;
}

TTerminalList::TTerminalList(TConfiguration * AConfiguration) :
  TObjectList(OBJECT_CLASS_TTerminalList),
  FConfiguration(AConfiguration)
{
  DebugAssert(FConfiguration);
}

TTerminalList::~TTerminalList()
{
  DebugAssert(GetCount() == 0);
}

TTerminal * TTerminalList::CreateTerminal(TSessionData * Data)
{
  TTerminal * Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}

TTerminal * TTerminalList::NewTerminal(TSessionData * Data)
{
  TTerminal * Result = CreateTerminal(Data);
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
  TTerminal * T = Terminal;
  Terminal = nullptr;
  FreeTerminal(T);
}

TTerminal * TTerminalList::GetTerminal(intptr_t Index)
{
  return GetAs<TTerminal>(Index);
}

void TTerminalList::Idle()
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TTerminal * Terminal = GetTerminal(Index);
    if (Terminal->GetStatus() == ssOpened)
    {
      Terminal->Idle();
    }
  }
}

void TTerminalList::RecryptPasswords()
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    GetTerminal(Index)->RecryptPasswords();
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

