
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
  const std::function<void()> &Operation)
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
  __fastcall TLoopDetector();
  void __fastcall RecordVisitedDirectory(const UnicodeString Directory);
  bool __fastcall IsUnvisitedDirectory(const UnicodeString Directory);

private:
  std::unique_ptr<TStringList> FVisitedDirectories;
};
//---------------------------------------------------------------------------
__fastcall TLoopDetector::TLoopDetector()
{
  FVisitedDirectories.reset(CreateSortedStringList());
}
//---------------------------------------------------------------------------
void __fastcall TLoopDetector::RecordVisitedDirectory(const UnicodeString ADirectory)
{
  UnicodeString VisitedDirectory = ::ExcludeTrailingBackslash(ADirectory);
  FVisitedDirectories->Add(VisitedDirectory);
}
//---------------------------------------------------------------------------
bool __fastcall TLoopDetector::IsUnvisitedDirectory(const UnicodeString Directory)
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
TCalculateSizeStats::TCalculateSizeStats() :
  Files(0),
  Directories(0),
  SymLinks(0),
  FoundFiles(nullptr)
{
__removed memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
TSynchronizeOptions::TSynchronizeOptions() :
  Filter(nullptr)
{
__removed memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
TSynchronizeOptions::~TSynchronizeOptions()
{
  SAFE_DESTROY(Filter);
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeOptions::MatchesFilter(UnicodeString AFileName) const
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
//---------------------------------------------------------------------------
TChecklistItem::~TChecklistItem()
{
  SAFE_DESTROY(RemoteFile);
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSynchronizeChecklist::TSynchronizeChecklist()
{
}
//---------------------------------------------------------------------------
TSynchronizeChecklist::~TSynchronizeChecklist()
{
  for (intptr_t Index = 0; Index < FList.GetCount(); ++Index)
  {
    TChecklistItem *Item = FList.GetAs<TChecklistItem>(Index);
    SAFE_DESTROY(Item);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Add(TChecklistItem *Item)
{
  FList.Add(Item);
}
//---------------------------------------------------------------------------
intptr_t __fastcall TSynchronizeChecklist::Compare(const void *AItem1, const void *AItem2)
{
  const TChecklistItem *Item1 = get_as<TChecklistItem>(AItem1);
  const TChecklistItem *Item2 = get_as<TChecklistItem>(AItem2);

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
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Sort()
{
  FList.Sort(Compare);
}
//---------------------------------------------------------------------------
intptr_t TSynchronizeChecklist::GetCount() const
{
  return FList.GetCount();
}
//---------------------------------------------------------------------------
const TChecklistItem *TSynchronizeChecklist::GetItem(intptr_t Index) const
{
  return FList.GetAs<TChecklistItem>(Index);
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklist::Update(const TChecklistItem *Item, bool Check, TChecklistAction Action)
{
  // TSynchronizeChecklist owns non-const items so it can manipulate them freely,
  // const_cast here is just an optimization
  TChecklistItem *MutableItem = const_cast<TChecklistItem *>(Item);
  DebugAssert(FList.IndexOf(MutableItem) >= 0);
  MutableItem->Checked = Check;
  MutableItem->Action = Action;
}
//---------------------------------------------------------------------------
TChecklistAction __fastcall TSynchronizeChecklist::Reverse(TChecklistAction Action)
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelThread : public TSimpleThread
{
  NB_DISABLE_COPY(TTunnelThread)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTunnelThread); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTunnelThread) || TSimpleThread::is(Kind); }
public:
  explicit __fastcall TTunnelThread(TSecureShell *SecureShell);
  virtual __fastcall ~TTunnelThread();
  virtual void InitTunnelThread();

  virtual void __fastcall Terminate() override;

protected:
  virtual void __fastcall Execute() override;

private:
  TSecureShell *FSecureShell;
  bool FTerminated;
};
//---------------------------------------------------------------------------
__fastcall TTunnelThread::TTunnelThread(TSecureShell *SecureShell) :
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
__fastcall TTunnelThread::~TTunnelThread()
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}
//---------------------------------------------------------------------------
void __fastcall TTunnelThread::Terminate()
{
  FTerminated = true;
}
//---------------------------------------------------------------------------
void __fastcall TTunnelThread::Execute()
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
class TTunnelUI : public TSessionUI
{
  NB_DISABLE_COPY(TTunnelUI)
public:
  explicit __fastcall TTunnelUI(TTerminal *Terminal);

  virtual __fastcall ~TTunnelUI()
  {
  }

  virtual void __fastcall Information(const UnicodeString AStr, bool Status) override;
  virtual uint32_t __fastcall QueryUser(const UnicodeString AQuery,
    TStrings *MoreMessages, uint32_t Answers, const TQueryParams *Params,
    TQueryType QueryType) override;
  virtual uint32_t __fastcall QueryUserException(const UnicodeString AQuery,
    Exception *E, uint32_t Answers, const TQueryParams *Params,
    TQueryType QueryType) override;
  virtual bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts,
    TStrings *Results) override;
  virtual void __fastcall DisplayBanner(const UnicodeString Banner) override;
  virtual void __fastcall FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpContext) override;
  virtual void __fastcall HandleExtendedException(Exception *E) override;
  virtual void __fastcall Closed() override;
  virtual void __fastcall ProcessGUI() override;

private:
  TTerminal *FTerminal;
  uint32_t FTerminalThreadID;
};
//---------------------------------------------------------------------------
__fastcall TTunnelUI::TTunnelUI(TTerminal *Terminal) :
  TSessionUI(OBJECT_CLASS_TTunnelUI),
  FTerminal(Terminal)
{
  FTerminalThreadID = GetCurrentThreadId();
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::Information(const UnicodeString AStr, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->Information(AStr, Status);
  }
}
//---------------------------------------------------------------------------
uint32_t __fastcall TTunnelUI::QueryUser(const UnicodeString AQuery,
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
uint32_t __fastcall TTunnelUI::QueryUserException(const UnicodeString AQuery,
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
bool __fastcall TTunnelUI::PromptUser(TSessionData *Data, TPromptKind Kind,
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
void __fastcall TTunnelUI::DisplayBanner(const UnicodeString Banner)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->DisplayBanner(Banner);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpContext)
{
  throw ESshFatal(E, Msg, HelpContext);
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::HandleExtendedException(Exception *E)
{
  if (GetCurrentThreadId() == FTerminalThreadID)
  {
    FTerminal->HandleExtendedException(E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::Closed()
{
  // noop
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::ProcessGUI()
{
  // noop
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCallbackGuard : public TObject
{
  NB_DISABLE_COPY(TCallbackGuard)
public:
  explicit __fastcall TCallbackGuard(TTerminal *ATerminal);
  inline __fastcall ~TCallbackGuard();

  void __fastcall FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpKeyword);
  inline void __fastcall Verify();
  inline bool __fastcall Verify(Exception *E);
  void __fastcall Dismiss();

private:
  ExtException *FFatalError;
  TTerminal *FTerminal;
  bool FGuarding;
};
//---------------------------------------------------------------------------
__fastcall TCallbackGuard::TCallbackGuard(TTerminal *ATerminal) :
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
__fastcall TCallbackGuard::~TCallbackGuard()
{
  if (FGuarding)
  {
    DebugAssert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == nullptr));
    FTerminal->FCallbackGuard = nullptr;
  }

  SAFE_DESTROY_EX(Exception, FFatalError);
}
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::FatalError(Exception *E, const UnicodeString Msg, const UnicodeString HelpKeyword)
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
void __fastcall TCallbackGuard::Dismiss()
{
  DebugAssert(FFatalError == nullptr);
  FGuarding = false;
}
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::Verify()
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
void TRetryOperationLoop::DoError(Exception &E, TSessionAction *Action, UnicodeString Message)
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
void TRetryOperationLoop::Error(Exception &E, TSessionAction &Action, UnicodeString Message)
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
  TGuard Guard(*FSection.get());
  FClients++;
}
//---------------------------------------------------------------------------
void TParallelOperation::RemoveClient()
{
  TGuard Guard(*FSection.get());
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
  TGuard Guard(*FSection.get());
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
__fastcall TTerminal::TTerminal(TObjectClassId Kind) :
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

void TTerminal::Init(TSessionData *SessionData,
  TConfiguration *Configuration)
{
  FConfiguration = Configuration;
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
//---------------------------------------------------------------------------
__fastcall TTerminal::~TTerminal()
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
void __fastcall TTerminal::Idle()
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
RawByteString __fastcall TTerminal::EncryptPassword(const UnicodeString APassword) const
{
  return FConfiguration->EncryptPassword(APassword, GetSessionData()->GetSessionName());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::DecryptPassword(const RawByteString APassword) const
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
void __fastcall TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FRememberedPassword = EncryptPassword(DecryptPassword(FRememberedPassword));
  FRememberedTunnelPassword = EncryptPassword(DecryptPassword(FRememberedTunnelPassword));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::ExpandFileName(const UnicodeString APath,
  const UnicodeString BasePath)
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
//---------------------------------------------------------------------------
bool __fastcall TTerminal::GetActive() const
{
  return (this != nullptr) && (FFileSystem != nullptr) && FFileSystem->GetActive();
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::Close()
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
void __fastcall TTerminal::ResetConnection()
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
void __fastcall TTerminal::FingerprintScan(UnicodeString &SHA256, UnicodeString &MD5)
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
void __fastcall TTerminal::Open()
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
    __finally__removed
    ({
      DoInformation(L"", true, 0);
    })
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
      SCOPE_EXIT
      {
        if (FSessionData->GetTunnel())
        {
          FSessionData->RollbackTunnel();
        }
      };
      InternalDoTryOpen();
    }
    __finally__removed
    ({
      if (FSessionData->Tunnel)
      {
        FSessionData->RollbackTunnel();
      }
    })

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
    }
    __finally__removed
    ({
      delete FSecureShell;
      FSecureShell = nullptr;
    })
  }
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::IsListenerFree(uintptr_t PortNumber) const
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
void __fastcall TTerminal::OpenTunnel()
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
      SCOPE_EXIT
      {
        FTunnelOpening = false;
      };
      FTunnel->Open();
    }
    __finally__removed
    ({
      FTunnelOpening = false;
    })

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
void __fastcall TTerminal::CloseTunnel()
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
void __fastcall TTerminal::Closed()
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
void __fastcall TTerminal::ProcessGUI()
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
//---------------------------------------------------------------------------
void __fastcall TTerminal::Progress(TFileOperationProgressType *OperationProgress)
{
  if (FNesting == 0)
  {
    TAutoNestingCounter NestingCounter(FNesting);
    OperationProgress->Progress();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::Reopen(intptr_t Params)
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
  __finally__removed
  ({
    SessionData->RemoteDirectory = PrevRemoteDirectory;
    SessionData->FSProtocol = OrigFSProtocol;
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
  })
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::PromptUser(TSessionData *Data, TPromptKind Kind,
  const UnicodeString AName, const UnicodeString Instructions, const UnicodeString Prompt,
  bool Echo, intptr_t MaxLen, UnicodeString &AResult)
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
  __finally__removed
  ({
    delete Prompts;
    delete Results;
  })
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::PromptUser(TSessionData *Data, TPromptKind Kind,
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
TTerminal * __fastcall TTerminal::GetPasswordSource()
{
  return this;
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  const UnicodeString AName, const UnicodeString Instructions, TStrings *Prompts, TStrings *Response)
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
        GetOnPromptUser()(this, Kind, AName, Instructions, Prompts, Response, Result, nullptr);
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
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
uint32_t __fastcall TTerminal::QueryUser(const UnicodeString AQuery,
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
uint32_t __fastcall TTerminal::QueryUserException(const UnicodeString AQuery,
  Exception *E, uint32_t Answers, const TQueryParams *Params,
  TQueryType QueryType)
{
  uint32_t Result = 0;
  UnicodeString ExMessage;
  if (DebugAlwaysTrue(ExceptionMessage(E, ExMessage) || !AQuery.IsEmpty()))
  {
    std::unique_ptr<TStrings> MoreMessages(new TStringList());
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
    }
    __finally__removed
    ({
      delete MoreMessages;
    })
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DisplayBanner(const UnicodeString ABanner)
{
  if (GetOnDisplayBanner() != nullptr)
  {
    uintptr_t OrigParams, Params;
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
void __fastcall TTerminal::HandleExtendedException(Exception *E)
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
void __fastcall TTerminal::ShowExtendedException(Exception *E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    GetOnShowExtendedException()(this, E, nullptr);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoInformation(const UnicodeString AStr, bool Status,
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
void __fastcall TTerminal::Information(const UnicodeString AStr, bool Status)
{
  DoInformation(AStr, Status);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoProgress(TFileOperationProgressType &ProgressData)
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
void __fastcall TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString &AFileName, bool Success, TOnceDoneOperation &OnceDoneOperation)
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
void __fastcall TTerminal::SaveCapabilities(TFileSystemInfo &FileSystemInfo)
{
  for (intptr_t Index = 0; Index < fcCount; ++Index)
  {
    FileSystemInfo.IsCapable[Index] = GetIsCapable(static_cast<TFSCapability>(Index));
  }
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::GetIsCapableProtected(TFSCapability Capability) const
{
  DebugAssert(FFileSystem);
  return FFileSystem->IsCapable(Capability);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::GetAbsolutePath(const UnicodeString APath, bool Local) const
{
  return FFileSystem->GetAbsolutePath(APath, Local);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ReactOnCommand(intptr_t ACmd)
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
void __fastcall TTerminal::TerminalError(const UnicodeString Msg)
{
  TerminalError(nullptr, Msg);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::TerminalError(
  Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword)
{
  throw ETerminal(E, AMsg, AHelpKeyword);
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::DoQueryReopen(Exception *E)
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
bool __fastcall TTerminal::ContinueReopen(TDateTime Start) const
{
  return
    (FConfiguration->GetSessionReopenTimeout() == 0) ||
    (intptr_t(double(Now() - Start) * MSecsPerDay) < FConfiguration->GetSessionReopenTimeout());
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::QueryReopen(Exception *E, intptr_t AParams,
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
void __fastcall TTerminal::FileOperationLoopEnd(Exception &E,
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
      std::unique_ptr<Exception> E2(new EFatal(&E, AMessage, AHelpKeyword));
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
intptr_t __fastcall TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
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
UnicodeString __fastcall TTerminal::TranslateLockedPath(const UnicodeString APath, bool Lock)
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
void __fastcall TTerminal::ClearCaches()
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
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ClearCachedFileList(const UnicodeString APath,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(APath, SubDirs);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::AddCachedFileList(TRemoteFileList *FileList)
{
  FDirectoryCache->AddFileList(FileList);
}
//---------------------------------------------------------------------------
TRemoteFileList * __fastcall TTerminal::DirectoryFileList(const UnicodeString APath, TDateTime Timestamp, bool CanLoad)
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
void __fastcall TTerminal::TerminalSetCurrentDirectory(const UnicodeString AValue)
{
  DebugAssert(FFileSystem);
  UnicodeString Value = TranslateLockedPath(AValue, false);
  if (AValue != FFileSystem->RemoteCurrentDirectory)
  {
    RemoteChangeDirectory(Value);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::RemoteGetCurrentDirectory()
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
UnicodeString __fastcall TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->RemoteCurrentDirectory();
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}
//---------------------------------------------------------------------------
TRemoteTokenList * __fastcall TTerminal::GetGroups()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}
//---------------------------------------------------------------------------
TRemoteTokenList * __fastcall TTerminal::GetUsers()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}
//---------------------------------------------------------------------------
TRemoteTokenList * __fastcall TTerminal::GetMembership()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::TerminalGetUserName() const
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
bool __fastcall TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == nullptr) || FDirectoryChangesCache->GetIsEmpty());
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoInitializeLog()
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
void __fastcall TTerminal::DoChangeDirectory()
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
void __fastcall TTerminal::DoReadDirectory(bool ReloadOnly)
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
void __fastcall TTerminal::DoStartReadDirectory()
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
void __fastcall TTerminal::DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool &Cancel)
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
bool __fastcall TTerminal::InTransaction() const
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::BeginTransaction()
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
void __fastcall TTerminal::EndTransaction()
{
  DoEndTransaction(false);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoEndTransaction(bool Inform)
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
      __finally__removed
      ({
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
      })
    }
  }

  if (FCommandSession != nullptr)
  {
    FCommandSession->EndTransaction();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SetExceptionOnFail(bool Value)
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
bool __fastcall TTerminal::GetExceptionOnFail() const
{
  return static_cast<bool>(FExceptionOnFail > 0);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::FatalAbort()
{
  FatalError(nullptr, L"");
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::FatalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword)
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
void __fastcall TTerminal::CommandError(Exception *E, const UnicodeString AMsg)
{
  CommandError(E, AMsg, 0);
}
//---------------------------------------------------------------------------
uintptr_t __fastcall TTerminal::CommandError(Exception *E, const UnicodeString AMsg,
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
    }
    __finally__removed
    ({
      delete ECmd;
    })
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
bool __fastcall TTerminal::HandleException(Exception *E)
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
void __fastcall TTerminal::CloseOnCompletion(
  TOnceDoneOperation Operation, const UnicodeString AMessage,
  const UnicodeString ATargetLocalPath, const UnicodeString ADestLocalFileName)
{
  __removed Configuration->Usage->Inc(L"ClosesOnCompletion");
  LogEvent("Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(nullptr,
    AMessage.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : AMessage,
    Operation, ATargetLocalPath, ADestLocalFileName);
}
//---------------------------------------------------------------------------
TBatchOverwrite __fastcall TTerminal::EffectiveBatchOverwrite(
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
bool __fastcall TTerminal::CheckRemoteFile(
  const UnicodeString AFileName, const TCopyParamType *CopyParam, intptr_t Params, TFileOperationProgressType *OperationProgress) const
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
  __finally__removed
  ({
    OperationProgress->UnlockUserSelections();
  })
  return Result;
}
//---------------------------------------------------------------------------
uint32_t __fastcall TTerminal::ConfirmFileOverwrite(
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
  }
  __finally__removed
  ({
    OperationProgress->UnlockUserSelections();
  })

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
void __fastcall TTerminal::FileModified(const TRemoteFile *AFile,
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
void __fastcall TTerminal::DirectoryModified(const UnicodeString APath, bool SubDirs)
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
void __fastcall TTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
  AddCachedFileList(FileList);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ReloadDirectory()
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
void __fastcall TTerminal::RefreshDirectory()
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
void __fastcall TTerminal::EnsureNonExistence(const UnicodeString AFileName)
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
void __fastcall TTerminal::LogEvent(const UnicodeString AStr)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, AStr);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::RollbackAction(TSessionAction &Action,
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
void __fastcall TTerminal::DoStartup()
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
  __finally__removed
  ({
    DoEndTransaction(true);
  })
  LogEvent("Startup conversation with host finished.");
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ReadCurrentDirectory()
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
void __fastcall TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
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
      __finally__removed
      ({
        DoReadDirectory(ReloadOnly);
      })

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
      __finally__removed
      ({
        DoReadDirectoryProgress(-1, 0, Cancel);
        FReadingCurrentDirectory = false;
        TRemoteDirectory *OldFiles = FFiles;
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
      })
    }
    catch (Exception &E)
    {
      CommandError(&E, FMTLOAD(LIST_DIR_ERROR, FFiles->GetDirectory()));
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::GetRemoteFileInfo(TRemoteFile *AFile) const
{
  return
    FORMAT("%s;%s;%lld;%s;%d;%s;%s;%s;%d",
      AFile->GetFileName(), AFile->GetType(), AFile->GetSize(), StandardTimestamp(AFile->GetModification()), int(AFile->GetModificationFmt()),
      AFile->GetFileOwner().GetLogText(), AFile->GetFileGroup().GetLogText(), AFile->GetRights()->GetText(),
      AFile->GetAttr());
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogRemoteFile(TRemoteFile *AFile)
{
  // optimization
  if (GetLog()->GetLogging() && AFile)
  {
    LogEvent(GetRemoteFileInfo(AFile));
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::FormatFileDetailsForLog(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size)
{
  UnicodeString Result;
  // optimization
  if (GetLog()->GetLogging())
  {
    Result = FORMAT("'%s' [%s] [%s]", AFileName, UnicodeString(AModification != TDateTime() ? StandardTimestamp(AModification) : UnicodeString(L"n/a")), ::Int64ToStr(Size));
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogFileDetails(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size)
{
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT("File: %s", FormatFileDetailsForLog(AFileName, AModification, Size)));
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogFileDone(TFileOperationProgressType *OperationProgress, const UnicodeString DestFileName)
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
void __fastcall TTerminal::CustomReadDirectory(TRemoteFileList *AFileList)
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
  {
    for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      LogRemoteFile(AFileList->GetFile(Index));
    }
  }

  ReactOnCommand(fsListDirectory);
}
//---------------------------------------------------------------------------
TRemoteFileList * __fastcall TTerminal::ReadDirectoryListing(UnicodeString Directory, const TFileMasks &Mask)
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
TRemoteFile * __fastcall TTerminal::ReadFileListing(UnicodeString APath)
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
TRemoteFileList * __fastcall TTerminal::CustomReadDirectoryListing(UnicodeString Directory, bool UseCache)
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
TRemoteFileList * __fastcall TTerminal::DoReadDirectoryListing(UnicodeString ADirectory, bool UseCache)
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
      __finally__removed
      ({
        ExceptionOnFail = false;
      })

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
bool __fastcall TTerminal::DeleteContentsIfDirectory(
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
  return Dir;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ProcessDirectory(const UnicodeString ADirName,
  TProcessFileEvent CallBackFunc, void *AParam, bool UseCache, bool IgnoreErrors)
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
    __finally__removed
    ({
      ExceptionOnFail = false;
    })
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
        if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
        {
          CallBackFunc(Directory + File->GetFileName(), File, AParam);
          // We should catch ESkipFile here as we do in ProcessFiles.
          // Now we have to handle ESkipFile in every callback implementation.
        }
      }
    }
    __finally__removed
    ({
      delete FileList;
    })
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ReadDirectory(TRemoteFileList *AFileList)
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
void __fastcall TTerminal::ReadSymlink(TRemoteFile *SymlinkFile,
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
void __fastcall TTerminal::ReadFile(UnicodeString AFileName,
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
bool __fastcall TTerminal::FileExists(const UnicodeString AFileName, TRemoteFile **AFile)
{
  bool Result;
  TRemoteFile *File = nullptr;
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
    __finally__removed
    ({
      ExceptionOnFail = false;
    })

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
void __fastcall TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::ProcessFiles(TStrings *AFileList,
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
    Progress.Start(Operation, Side, AFileList->GetCount());

    TFileOperationProgressType *OperationProgress(&Progress);

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
                TRemoteFile *RemoteFile = AFileList->GetAs<TRemoteFile>(Index);
                ProcessFile(FileName, RemoteFile, Param);
              }
              else
              {
                // not used anymore
                __removed TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                __removed ProcessFileEx(FileName, (TRemoteFile *)FileList->GetObj(Index), Param, Index);
              }
              Success = true;
            }
            __finally__removed
            ({
              Progress.Finish(FileName, Success, OnceDoneOperation);
            })
          }
          catch (ESkipFile &E)
          {
            DEBUG_PRINTF("before HandleException");
            volatile TSuspendFileOperationProgress Suspend(OperationProgress);
            if (!HandleException(&E))
            {
              throw;
            }
          }
          ++Index;
        }
      }
      __finally__removed
      ({
        if (Side == osRemote)
        {
          EndTransaction();
        }
      })

      if (Progress.GetCancel() == csContinue)
      {
        Result = true;
      }
    }
    __finally__removed
    ({
      FOperationProgress = nullptr;
      Progress.Stop();
    })
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
//---------------------------------------------------------------------------
#if 0
bool __fastcall TTerminal::ProcessFilesEx(TStrings *FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void *Param, TOperationSide Side)
{
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
}
#endif
//---------------------------------------------------------------------------
TStrings * __fastcall TTerminal::GetFixedPaths() const
{
  DebugAssert(FFileSystem != nullptr);
  return FFileSystem ? FFileSystem->GetFixedPaths() : nullptr;
}
//---------------------------------------------------------------------------
bool __fastcall  TTerminal::GetResolvingSymlinks() const
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}
//---------------------------------------------------------------------------
TUsableCopyParamAttrs __fastcall  TTerminal::UsableCopyParamAttrs(intptr_t Params) const
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
//---------------------------------------------------------------------------
bool __fastcall TTerminal::IsRecycledFile(const UnicodeString AFileName)
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
void __fastcall TTerminal::RecycleFile(UnicodeString AFileName,
  const TRemoteFile *AFile)
{
  if (AFileName.IsEmpty())
  {
    DebugAssert(AFile != nullptr);
    if (AFile)
    {
      AFileName = AFile->GetFileName();
    }
  }

  if (!IsRecycledFile(AFileName))
  {
    LogEvent(FORMAT("Moving file \"%s\" to remote recycle bin '%s'.",
        AFileName, GetSessionData()->GetRecycleBinPath()));

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
      UnicodeString dt = FORMAT("%04d%02d%02d-%02d%02d%02d", Y, M, D, H, N, S);
      // Params.FileMask = FORMAT("*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()));
      Params.FileMask = FORMAT("*-%s.*", dt);
    }
#endif
    TerminalMoveFile(AFileName, AFile, &Params);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::TryStartOperationWithFile(
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
void __fastcall TTerminal::StartOperationWithFile(
  const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  if (!TryStartOperationWithFile(AFileName, Operation1, Operation2))
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::RemoteDeleteFile(const UnicodeString &AFileName,
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
    ReactOnCommand(fsDeleteFile);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoDeleteFile(const UnicodeString AFileName,
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
    }
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(DELETE_FILE_ERROR, AFileName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::RemoteDeleteFiles(TStrings *AFilesToDelete, intptr_t Params)
{
  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TODO("avoid resolving symlinks while reading subdirectories.");
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return this->ProcessFiles(AFilesToDelete, foDelete, nb::bind(&TTerminal::RemoteDeleteFile, this), &Params);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DeleteLocalFile(const UnicodeString &AFileName,
  const TRemoteFile * /*AFile*/, void *Params)
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
//---------------------------------------------------------------------------
bool __fastcall TTerminal::DeleteLocalFiles(TStrings *AFileList, intptr_t Params)
{
  return ProcessFiles(AFileList, foDelete, nb::bind(&TTerminal::DeleteLocalFile, this), &Params, osLocal);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::CustomCommandOnFile(const UnicodeString &AFileName,
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
void __fastcall TTerminal::DoCustomCommandOnFile(UnicodeString AFileName,
  const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams,
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
void __fastcall TTerminal::CustomCommandOnFiles(const UnicodeString ACommand,
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
bool __fastcall TTerminal::DoOnCustomCommand(const UnicodeString Command)
{
  bool Result = false;
  if (FOnCustomCommand != nullptr)
  {
    FOnCustomCommand(this, Command, Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ChangeFileProperties(const UnicodeString &AFileName,
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
void __fastcall TTerminal::DoChangeFileProperties(const UnicodeString AFileName,
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
void __fastcall TTerminal::ChangeFilesProperties(TStrings *AFileList,
  const TRemoteProperties *Properties)
{
  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  AnnounceFileListOperation();
  ProcessFiles(AFileList, foSetProperties, nb::bind(&TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::LoadFilesProperties(TStrings *AFileList)
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
void __fastcall TTerminal::DoCalculateFileSize(const UnicodeString &AFileName,
  const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *AParam)
{
  TCalculateSizeParams *AParams = get_as<TCalculateSizeParams>(AParam);

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
//---------------------------------------------------------------------------
void __fastcall TTerminal::CalculateFileSize(const UnicodeString &AFileName,
  const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *AParam)
{
  DebugAssert(AParam);
  DebugAssert(AFile);
  UnicodeString FileName = AFileName;
  TCalculateSizeParams *Params = get_as<TCalculateSizeParams>(AParam);
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

  bool AllowTransfer = (Params->CopyParam == nullptr);
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->GetSize();
    MaskParams.Modification = AFile->GetModification();

    UnicodeString BaseFileName =
      GetBaseFileName(base::UnixExcludeTrailingBackslash(AFile->GetFullFileName()));
    AllowTransfer = Params->CopyParam->AllowTransfer(
        BaseFileName, osRemote, AFile->GetIsDirectory(), MaskParams);
  }

  if (AllowTransfer)
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
        else
        {
          LogEvent(FORMAT("Getting size of directory \"%s\"", FileName));
          // pass in full path so we get it back in file list for AllowTransfer() exclusion
          if (!DoCalculateDirectorySize(AFile->GetFullFileName(), AFile, Params))
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
bool __fastcall TTerminal::DoCalculateDirectorySize(const UnicodeString AFileName,
  const TRemoteFile * /*AFile*/, TCalculateSizeParams *Params)
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
    catch (Exception &E)
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
//---------------------------------------------------------------------------
bool __fastcall TTerminal::CalculateFilesSize(TStrings *AFileList,
  int64_t &Size, intptr_t Params, const TCopyParamType *CopyParam,
  bool AllowDirs, TCalculateSizeStats &Stats)
{
  // With FTP protocol, we may use DSIZ command from
  // draft-peterson-streamlined-ftp-command-extensions-10
  // Implemented by Serv-U FTP.

  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TCalculateSizeParams Param;
  Param.Size = 0;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = &Stats;
  Param.AllowDirs = AllowDirs;
  Param.Result = true;
  Param.Files = nullptr;
  ProcessFiles(AFileList, foCalculateSize, nb::bind(&TTerminal::DoCalculateFileSize, this), &Param);
  Size = Param.Size;
  return Param.Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::CalculateFilesChecksum(UnicodeString Alg,
  TStrings *AFileList, TStrings *Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, AFileList, Checksums, OnCalculatedChecksum);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::TerminalRenameFile(const TRemoteFile *AFile,
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
void __fastcall TTerminal::DoRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
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
void __fastcall TTerminal::TerminalMoveFile(const UnicodeString &AFileName,
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
bool __fastcall TTerminal::TerminalMoveFiles(TStrings *AFileList, const UnicodeString ATarget,
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
    SCOPE_EXIT
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
          const TRemoteFile *File = AFileList->GetAs<TRemoteFile>(Index);
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
    };
    Result = ProcessFiles(AFileList, foRemoteMove, nb::bind(&TTerminal::TerminalMoveFile, this), &Params);
  }
  __finally__removed
  ({
    if (Active)
    {
      UnicodeString WithTrailing = UnixIncludeTrailingBackslash(CurrentDirectory);
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      for (int Index = 0; !PossiblyMoved && (Index < FileList->Count); Index++)
      {
        const TRemoteFile *File =
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
  })
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
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
void __fastcall TTerminal::TerminalCopyFile(const UnicodeString &AFileName,
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
bool __fastcall TTerminal::TerminalCopyFiles(TStrings *AFileList, const UnicodeString ATarget,
  const UnicodeString AFileMask)
{
  TMoveFileParams Params;
  Params.Target = ATarget;
  Params.FileMask = AFileMask;
  DirectoryModified(ATarget, true);
  return ProcessFiles(AFileList, foRemoteCopy, nb::bind(&TTerminal::TerminalCopyFile, this), &Params);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::RemoteCreateDirectory(const UnicodeString ADirName,
  const TRemoteProperties *Properties)
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
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoCreateDirectory(const UnicodeString ADirName)
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
    catch (Exception &E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CREATE_DIR_ERROR, ADirName));
    }
  }
  while (RetryLoop.Retry());
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::RemoteCreateLink(const UnicodeString AFileName,
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
void __fastcall TTerminal::DoCreateLink(const UnicodeString AFileName,
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
void __fastcall TTerminal::HomeDirectory()
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
void __fastcall TTerminal::RemoteChangeDirectory(const UnicodeString ADirectory)
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
void __fastcall TTerminal::LookupUsersGroups()
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
bool __fastcall TTerminal::AllowedAnyCommand(const UnicodeString Command) const
{
  return !Command.Trim().IsEmpty();
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::GetCommandSessionOpened() const
{
  // consider secondary terminal open in "ready" state only
  // so we never do keepalives on it until it is completely initialized
  return (FCommandSession != nullptr) &&
    (FCommandSession->GetStatus() == ssOpened);
}
//---------------------------------------------------------------------------
TTerminal * __fastcall TTerminal::CreateSecondarySession(const UnicodeString Name, TSessionData *SessionData)
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
//---------------------------------------------------------------------------
void __fastcall TTerminal::FillSessionDataForCode(TSessionData *Data) const
{
  const TSessionInfo &SessionInfo = GetSessionInfo();
  Data->SetHostKey(SessionInfo.HostKeyFingerprintSHA256);
}
//---------------------------------------------------------------------------
TTerminal * __fastcall TTerminal::GetCommandSession()
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

    TTerminal *CommandSession = CreateSecondarySession(L"Shell", CommandSessionData.get());
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
  TCallSessionAction &FAction;
  TCaptureOutputEvent FOutputEvent;
};

#pragma warning(pop)
//---------------------------------------------------------------------------
void __fastcall TTerminal::AnyCommand(const UnicodeString Command,
  TCaptureOutputEvent OutputEvent)
{
#if 0
#pragma warn -inl
  class TOutputProxy
  {
  public:
    __fastcall TOutputProxy(TCallSessionAction &Action, TCaptureOutputEvent OutputEvent) :
      FAction(Action),
      FOutputEvent(OutputEvent)
    {
    }

    void __fastcall Output(const UnicodeString AStr, TCaptureOutputType OutputType)
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
void __fastcall TTerminal::DoAnyCommand(const UnicodeString ACommand,
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
bool __fastcall TTerminal::DoCreateLocalFile(const UnicodeString AFileName,
  TFileOperationProgressType *OperationProgress,
  bool Resume,
  bool NoConfirmation,
  HANDLE *AHandle)
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
    *AHandle = this->TerminalCreateLocalFile(ApiPath(AFileName), DesiredAccess, ShareMode,
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
          }
          __finally__removed
          ({
            OperationProgress->UnlockUserSelections();
          })
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
bool __fastcall TTerminal::TerminalCreateLocalFile(const UnicodeString ATargetFileName,
  TFileOperationProgressType *OperationProgress,
  bool Resume,
  bool NoConfirmation,
  HANDLE *AHandle)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(CREATE_FILE_ERROR, ATargetFileName), "",
  [&]()
  {
    Result = DoCreateLocalFile(ATargetFileName, OperationProgress, Resume, NoConfirmation,
        AHandle);
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_FILE_ERROR, (FileName)));

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::TerminalOpenLocalFile(const UnicodeString ATargetFileName,
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
          if ((LSize == static_cast<uint32_t>(-1)) && (::GetLastError() != NO_ERROR))
          {
            ::RaiseLastOSError();
          }
          *ASize = (static_cast<int64_t>(HSize) << 32) + LSize;
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
void __fastcall TTerminal::TerminalOpenLocalFile(
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
bool __fastcall TTerminal::AllowLocalFileTransfer(const UnicodeString AFileName,
  const TCopyParamType *CopyParam, TFileOperationProgressType *OperationProgress)
{
  bool Result = true;
  // optimization
  if (GetLog()->GetLogging() || !CopyParam->AllowAnyTransfer())
  {
    WIN32_FIND_DATA FindData = {};
    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
    FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
      FMTLOAD(FILE_NOT_EXISTS, AFileName), "",
    [&]()
    {
      LocalFileHandle = ::FindFirstFileW(ApiPath(::ExcludeTrailingBackslash(AFileName)).c_str(), &FindData);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        ::RaiseLastOSError();
      }
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(FILE_NOT_EXISTS, (FileName)));
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
//---------------------------------------------------------------------------
void __fastcall TTerminal::MakeLocalFileList(const UnicodeString AFileName,
  const TSearchRec &Rec, void *Param)
{
  TMakeLocalFileListParams &Params = *get_as<TMakeLocalFileListParams>(Param);

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
//---------------------------------------------------------------------------
void __fastcall TTerminal::CalculateLocalFileSize(const UnicodeString AFileName,
  const TSearchRec &Rec, /*int64_t*/ void *AParams)
{
  TCalculateSizeParams *Params = get_as<TCalculateSizeParams>(AParams);

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
//---------------------------------------------------------------------------
bool __fastcall TTerminal::CalculateLocalFilesSize(TStrings *AFileList,
  const TCopyParamType *CopyParam, bool AllowDirs, TStrings *Files,
  int64_t &Size)
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
  __finally__removed
  ({
    FOperationProgress = nullptr;
    OperationProgress.Stop();
  })

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
  return Result;
}
//---------------------------------------------------------------------------
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
  TTerminal::TSynchronizeMode Mode;
  intptr_t Params;
  TSynchronizeDirectoryEvent OnSynchronizeDirectory;
  TSynchronizeOptions *Options;
  intptr_t Flags;
  TStringList *LocalFileList;
  const TCopyParamType *CopyParam;
  TSynchronizeChecklist *Checklist;

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
TSynchronizeChecklist * __fastcall TTerminal::SynchronizeCollect(const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType *CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
  TSynchronizeOptions *Options)
{
  volatile TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
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
static void __fastcall AddFlagName(UnicodeString &ParamsStr, intptr_t &Params, intptr_t Param, UnicodeString Name)
{
  if (FLAGSET(Params, Param))
  {
    AddToList(ParamsStr, Name, L", ");
  }
  Params &= ~Param;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::SynchronizeModeStr(TSynchronizeMode Mode)
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
UnicodeString __fastcall TTerminal::SynchronizeParamsStr(intptr_t Params)
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
void __fastcall TTerminal::DoSynchronizeCollectDirectory(const UnicodeString ALocalDirectory,
  const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType *CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions *Options,
  intptr_t AFlags, TSynchronizeChecklist *Checklist)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  TSynchronizeData Data;

  Data.LocalDirectory = ::IncludeTrailingBackslash(ALocalDirectory);
  Data.RemoteDirectory = base::UnixIncludeTrailingBackslash(ARemoteDirectory);
  Data.Mode = Mode;
  Data.Params = Params;
  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;
  Data.LocalFileList = nullptr;
  Data.CopyParam = CopyParam;
  Data.Options = Options;
  Data.Flags = AFlags;
  Data.Checklist = Checklist;

  LogEvent(FORMAT("Collecting synchronization list for local directory '%s' and remote directory '%s', "
    "mode = %s, params = 0x%x (%s), file mask = '%s'", ALocalDirectory, ARemoteDirectory,
    SynchronizeModeStr(Mode), int(Params), SynchronizeParamsStr(Params), CopyParam->GetIncludeFileMask().GetMasks()));

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

    FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
      FMTLOAD(LIST_DIR_ERROR, ALocalDirectory), "",
    [&]()
    {
      DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      Found = ::FindFirstChecked(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0;
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, (LocalDirectory)));

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
              (FLAGCLEAR(AFlags, sfFirstLevel) ||
               (Options == nullptr) ||
               Options->MatchesFilter(FileName) ||
               Options->MatchesFilter(RemoteFileName)))
          {
            TSynchronizeFileData *FileData = new TSynchronizeFileData();

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

          FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
            FMTLOAD(LIST_DIR_ERROR, ALocalDirectory), "",
          [&]()
          {
            Found = (::FindNextChecked(SearchRec) == 0);
          });
          __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, (LocalDirectory)));
        }
      }
      __finally__removed
      ({
        FindClose(SearchRec);
      })

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
          __finally__removed
          ({
            delete ChecklistItem;
          })
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
  __finally__removed
  ({
    if (Data.LocalFileList != nullptr)
    {
      for (int Index = 0; Index < Data.LocalFileList->Count; Index++)
      {
        TSynchronizeFileData *FileData = reinterpret_cast<TSynchronizeFileData *>
          (Data.LocalFileList->Objects[Index]);
        delete FileData;
      }
      delete Data.LocalFileList;
    }
  })
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SynchronizeCollectFile(const UnicodeString &AFileName,
  const TRemoteFile *AFile, /*TSynchronizeData*/ void *Param)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  try
  {
    DoSynchronizeCollectFile(AFileName, AFile, Param);
  }
  catch (ESkipFile &E)
  {
    volatile TSuspendFileOperationProgress Suspend(OperationProgress);
    if (!HandleException(&E))
    {
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoSynchronizeCollectFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, /*TSynchronizeData*/ void *Param)
{
  TSynchronizeData *Data = get_as<TSynchronizeData>(Param);

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
    __finally__removed
    ({
      delete ChecklistItem;
    })
  }
  else
  {
    LogEvent(FORMAT("Remote file %s excluded from synchronization",
      FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize())));
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SynchronizeApply(TSynchronizeChecklist *Checklist,
  const UnicodeString ALocalDirectory, const UnicodeString ARemoteDirectory,
  const TCopyParamType *CopyParam, intptr_t Params,
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
      __removed const TSynchronizeChecklist::TItem * ChecklistItem;

      DownloadList->Clear();
      DeleteRemoteList->Clear();
      UploadList->Clear();
      DeleteLocalList->Clear();

      const TChecklistItem *ChecklistItem = Checklist->GetItem(IIndex);

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
  __finally__removed
  ({
    delete DownloadList;
    delete DeleteRemoteList;
    delete UploadList;
    delete DeleteLocalList;

    EndTransaction();
  })
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoSynchronizeProgress(const TSynchronizeData &Data,
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
void __fastcall TTerminal::SynchronizeLocalTimestamp(const UnicodeString & /*AFileName*/,
  const TRemoteFile *AFile, void * /*Param*/)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();

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
    bool Result = ::SetFileTime(Handle, nullptr, nullptr, &WrTime);
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
void __fastcall TTerminal::SynchronizeRemoteTimestamp(const UnicodeString & /*AFileName*/,
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
void __fastcall TTerminal::FileFind(const UnicodeString &AFileName,
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
void __fastcall TTerminal::DoFilesFind(const UnicodeString ADirectory, TFilesFindParams &Params, const UnicodeString ARealDirectory)
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
      SCOPE_EXIT
      {
        Params.RealDirectory = PrevRealDirectory;
        FOnFindingFile = nullptr;
      };
      Params.RealDirectory = ARealDirectory;
      ProcessDirectory(ADirectory, nb::bind(&TTerminal::FileFind, this), &Params, false, true);
    }
    __finally__removed
    ({
      Params.RealDirectory = PrevRealDirectory;
      FOnFindingFile = nullptr;
    })
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::FilesFind(const UnicodeString ADirectory, const TFileMasks &FileMask,
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
void __fastcall TTerminal::SpaceAvailable(const UnicodeString APath,
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
void __fastcall TTerminal::LockFile(const UnicodeString &AFileName,
  const TRemoteFile *AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foLock);

  LogEvent(FORMAT("Locking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoLockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoLockFile(const UnicodeString &AFileName, const TRemoteFile *AFile)
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
void __fastcall TTerminal::UnlockFile(const UnicodeString &AFileName,
  const TRemoteFile *AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foUnlock);

  LogEvent(FORMAT("Unlocking file \"%s\".", AFileName));
  FileModified(AFile, AFileName, true);

  DoUnlockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DoUnlockFile(const UnicodeString &AFileName, const TRemoteFile *AFile)
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
void __fastcall TTerminal::LockFiles(TStrings *AFileList)
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
  __finally__removed
  ({
    EndTransaction();
  })
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::UnlockFiles(TStrings *AFileList)
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
  __finally__removed
  ({
    EndTransaction();
  })
}
//---------------------------------------------------------------------------
const TSessionInfo & __fastcall TTerminal::GetSessionInfo() const
{
  return FFileSystem->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & __fastcall TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::GetSupportedChecksumAlgs(TStrings *Algs) const
{
  FFileSystem->GetSupportedChecksumAlgs(Algs);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::GetPassword() const
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
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::GetRememberedPassword() const
{
  return DecryptPassword(FRememberedPassword);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::GetRememberedTunnelPassword() const
{
  return DecryptPassword(FRememberedTunnelPassword);
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::GetStoredCredentialsTried() const
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
intptr_t __fastcall TTerminal::CopyToParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress)
{
  UnicodeString FileName;
  TObject *Object;
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
    __finally__removed
    ({
      bool Success = (Prev < OperationProgress->FilesFinishedSuccessfully);
      ParallelOperation->Done(FileName, Dir, Success);
      FOperationProgress = nullptr;
    })
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::CanParallel(
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
void __fastcall TTerminal::CopyParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress)
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
  __finally__removed
  ({
    OperationProgress->SetDone();
    ParallelOperation->WaitFor();
  })
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogParallelTransfer(TParallelOperation *ParallelOperation)
{
  LogEvent(
    FORMAT("Adding a parallel transfer to the transfer started on the connection \"%s\"",
      ParallelOperation->GetMainName()));
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogTotalTransferDetails(
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
    LogEvent(CopyParam->GetLogStr());
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogTotalTransferDone(TFileOperationProgressType *OperationProgress)
{
  LogEvent(L"Copying finished: " + OperationProgress->GetLogStr(true));
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::CopyToRemote(TStrings *AFilesToCopy,
  const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams, TParallelOperation *ParallelOperation)
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
    if (CanParallel(CopyParam, AParams, ParallelOperation) &&
      !CopyParam->GetClearArchive())
    {
      Files.reset(new TStringList());
      Files->SetOwnsObjects(true);
    }
    // dirty trick: when moving, do not pass copy param to avoid exclude mask
    bool CalculatedSize =
      CalculateLocalFilesSize(
        AFilesToCopy,
        (FLAGCLEAR(AParams, cpDelete) ? CopyParam : nullptr),
        CopyParam->GetCalculateSize(), Files.get(), Size);

    FLastProgressLogged = GetTickCount();
    OperationProgress.Start((AParams & cpDelete) ? foMove : foCopy, osLocal,
      AFilesToCopy->GetCount(), (AParams & cpTemporary) > 0, ATargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = &OperationProgress; //-V506
    __removed bool CollectingUsage = false;
    try__finally
    {
      SCOPE_EXIT
      {
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

      UnicodeString UnlockedTargetDir = TranslateLockedPath(ATargetDir, false);
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
      }
      __finally__removed
      ({
        if (Active)
        {
          ReactOnCommand(fsCopyToRemote);
        }
        EndTransaction();
      })

      if (OperationProgress.GetCancel() == csContinue)
      {
        Result = true;
      }
    }
    __finally__removed
    ({
      if (CollectingUsage)
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"UploadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxUploadTime", CounterTime);
      }
      OperationProgress.Stop();
      FOperationProgress = nullptr;
    })
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
void __fastcall TTerminal::DoCopyToRemote(
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

    try__finally
    {
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      try
      {
        if (GetSessionData()->GetCacheDirectories())
        {
          DirectoryModified(TargetDir, false);

          if (::DirectoryExists(ApiPath(FileName)))
          {
            UnicodeString FileNameOnly = base::ExtractFileName(FileName, false);
            DirectoryModified(FullTargetDir + FileNameOnly, true);
          }
        }
        SourceRobust(FileName, FullTargetDir, CopyParam, AParams, OperationProgress, AFlags | tfFirstLevel);
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
    }
    __finally__removed
    ({
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    })
    Index++;
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SourceRobust(
  const UnicodeString AFileName, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
  TFileOperationProgressType *OperationProgress, uintptr_t AFlags)
{
  TUploadSessionAction Action(GetActionLog());
  bool * AFileTransferAny = FLAGSET(AFlags, tfUseFileTransferAny) ? &FFileTransferAny : nullptr;
  TRobustOperationLoop RobustLoop(this, OperationProgress, AFileTransferAny);

  do
  {
    bool ChildError = false;
    try
    {
      Source(AFileName, ATargetDir, CopyParam, AParams, OperationProgress, AFlags, Action, ChildError);
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
void __fastcall TTerminal::CreateTargetDirectory(
  const UnicodeString ADirectoryPath, uintptr_t Attrs, const TCopyParamType *CopyParam)
{
  TRemoteProperties Properties;
  if (CopyParam->GetPreserveRights())
  {
    Properties.Valid = TValidProperties() << vpRights;
    Properties.Rights = CopyParam->RemoteFileRights(Attrs);
  }
  RemoteCreateDirectory(ADirectoryPath, &Properties);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DirectorySource(
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
    DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    TSearchRecChecked SearchRec;
    bool FindOK;

    FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
      FMTLOAD(LIST_DIR_ERROR, ADirectoryName), "",
    [&]()
    {
      FindOK =
        ::FindFirstChecked(ADirectoryName + L"*.*", FindAttrs, SearchRec) == 0;
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, (ADirectoryName)));

    try__finally
    {
      SCOPE_EXIT
      {
        base::FindClose(SearchRec);
      };
      while (FindOK && !OperationProgress->GetCancel())
      {
        UnicodeString FileName = ADirectoryName + SearchRec.Name;
        try
        {
          if ((SearchRec.Name != L".") && (SearchRec.Name != L".."))
          {
            SourceRobust(FileName, DestFullName, CopyParam, AParams, OperationProgress, (AFlags & ~(tfFirstLevel | tfAutoResume)));
            // FTP: if any file got uploaded (i.e. there were any file in the directory and at least one was not skipped),
            // do not try to create the directory, as it should be already created by FZAPI during upload
            PostCreateDir = false;
          }
        }
        catch (ESkipFile &E)
        {
          // If ESkipFile occurs, just log it and continue with next file
          volatile TSuspendFileOperationProgress Suspend(OperationProgress);
          // here a message to user was displayed, which was not appropriate
          // when user refused to overwrite the file in subdirectory.
          // hopefully it won't be missing in other situations.
          if (!HandleException(&E))
          {
            throw;
          }
        }

        FileOperationLoopCustom(this, OperationProgress, True,
          FMTLOAD(LIST_DIR_ERROR, ADirectoryName), "",
        [&]()
        {
          FindOK = (::FindNextChecked(SearchRec) == 0);
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(LIST_DIR_ERROR, (ADirectoryName)));
      }
    }
    __finally__removed 
    ({
      FindClose(SearchRec);
    })

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
        RemoveDir(ApiPath(ADirectoryName));
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
      {
        FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
          FMTLOAD(CANT_SET_ATTRS, ADirectoryName), "",
        [&]()
        {
          THROWOSIFFALSE(::FileSetAttr(ApiPath(ADirectoryName), (DWORD)(Attrs & ~faArchive)) == 0);
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (DirectoryName)));
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SelectTransferMode(
  const UnicodeString ABaseFileName, TOperationSide Side, const TCopyParamType *CopyParam,
  const TFileMasks::TParams &MaskParams)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  OperationProgress->SetAsciiTransfer(CopyParam->UseAsciiTransfer(ABaseFileName, Side, MaskParams));
  UnicodeString ModeName = (OperationProgress->GetAsciiTransfer() ? L"Ascii" : L"Binary");
  LogEvent(FORMAT("%s transfer mode selected.", ModeName));
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SelectSourceTransferMode(const TLocalFileHandle &Handle, const TCopyParamType *CopyParam)
{
  // Will we use ASCII or BINARY file transfer?
  TFileMasks::TParams MaskParams;
  MaskParams.Size = Handle.Size;
  MaskParams.Modification = Handle.Modification;
  UnicodeString BaseFileName = GetBaseFileName(Handle.FileName);
  SelectTransferMode(BaseFileName, osLocal, CopyParam, MaskParams);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::UpdateSource(const TLocalFileHandle &AHandle, const TCopyParamType *CopyParam, intptr_t AParams)
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
        THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(AHandle.FileName)));
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
      THROWOSIFFALSE(::FileSetAttr(ApiPath(AHandle.FileName), (AHandle.Attrs & ~faArchive)) == 0);
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (Handle.FileName)));
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::Source(
  const UnicodeString AFileName, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
  TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TUploadSessionAction & Action, bool &ChildError)
{
  Action.SetFileName(::ExpandUNCFileName(AFileName));

  OperationProgress->SetFile(AFileName, false);

  if (!AllowLocalFileTransfer(AFileName, CopyParam, OperationProgress))
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
  }

  Handle.Release();

  UpdateSource(Handle, CopyParam, AParams);
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::CopyToLocal(TStrings *AFilesToCopy,
  const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
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
    if (CanParallel(CopyParam, AParams, ParallelOperation))
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
          (FLAGCLEAR(AParams, cpDelete) ? CopyParam : nullptr),
          CopyParam->GetCalculateSize(), Stats))
      {
        TotalSizeKnown = true;
      }
    }
    __finally__removed
    ({
      ExceptionOnFail = false;
    })

    OperationProgress.Start(((AParams & cpDelete) != 0 ? foMove : foCopy), osRemote,
      AFilesToCopy ? AFilesToCopy->GetCount() : 0, (AParams & cpTemporary) != 0, ATargetDir, CopyParam->GetCPSLimit());

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
        }
        __finally__removed
        ({
          if (Active)
          {
            ReactOnCommand(fsCopyToLocal);
          }
        })
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
    }
    __finally__removed
    ({
      if (CollectingUsage)
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"DownloadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxDownloadTime", CounterTime);
      }
      FOperationProgress = nullptr;
      OperationProgress.Stop();
    })
  }
  __finally__removed
  ({
    // If session is still active (no fatal error) we reload directory
    // by calling EndTransaction
    EndTransaction();
  })

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
void __fastcall TTerminal::DoCopyToLocal(
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

    try__finally
    {
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      try
      {
        UnicodeString AbsoluteFileName = GetAbsolutePath(FileName, true);
        const TRemoteFile *File = AFilesToCopy->GetAs<const TRemoteFile>(Index);
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
    }
    __finally__removed
    ({
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    })
    Index++;
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SinkRobust(
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
        int Params = dfNoRecursive;
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
  intptr_t Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  unsigned int Flags;
};
#endif // #if 0
//---------------------------------------------------------------------------
void __fastcall TTerminal::Sink(
  const UnicodeString FileName, const TRemoteFile *File, const UnicodeString TargetDir,
  const TCopyParamType *CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction &Action)
{
  Action.SetFileName(FileName);

  TFileMasks::TParams MaskParams;
  DebugAssert(File);
  MaskParams.Size = File->GetSize();
  MaskParams.Modification = File->GetModification();

  UnicodeString BaseFileName = GetBaseFileName(FileName);
  if (!CopyParam->AllowTransfer(BaseFileName, osRemote, File->GetIsDirectory(), MaskParams))
  {
    LogEvent(FORMAT("File \"%s\" excluded from transfer", FileName));
    throw ESkipFile();
  }

  if (CopyParam->SkipTransfer(FileName, File->GetIsDirectory()))
  {
    OperationProgress->AddSkippedFileSize(File->GetSize());
    throw ESkipFile();
  }

  LogFileDetails(FileName, File->GetModification(), File->GetSize());

  OperationProgress->SetFile(FileName);

  UnicodeString OnlyFileName = base::UnixExtractFileName(FileName);
  UnicodeString DestFileName = ChangeFileName(CopyParam, OnlyFileName, osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (File->GetIsDirectory())
  {
    Action.Cancel();
    if (CanRecurseToDirectory(File))
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
        THROWOSIFFALSE(::ForceDirectories(ApiPath(DestFullName)));
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_DIR_ERROR, (DestFullName)));

      if (FLAGCLEAR(Params, cpNoRecurse))
      {
        TSinkFileParams SinkFileParams;
        SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
        SinkFileParams.CopyParam = CopyParam;
        SinkFileParams.Params = Params;
        SinkFileParams.OperationProgress = OperationProgress;
        SinkFileParams.Skipped = false;
        SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

        ProcessDirectory(FileName, nb::bind(&TTerminal::SinkFile, this), &SinkFileParams);

        FFileSystem->DirectorySunk(DestFullName, File, CopyParam);

        // Do not delete directory if some of its files were skip.
        // Throw "skip file" for the directory to avoid attempt to deletion
        // of any parent directory
        if (FLAGSET(Params, cpDelete) && SinkFileParams.Skipped)
        {
          throw ESkipFile();
        }
      }
    }
    else
    {
      LogEvent(FORMAT("Skipping symlink to directory \"%s\".", FileName));
    }
  }
  else
  {
    LogEvent(FORMAT("Copying \"%s\" to local directory started.", FileName));

    // Will we use ASCII of BINARY file transfer?
    SelectTransferMode(BaseFileName, osRemote, CopyParam, MaskParams);

    // Suppose same data size to transfer as to write
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(File->GetSize());
    OperationProgress->SetLocalSize(OperationProgress->GetTransferSize());

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
      FileName, File, TargetDir, DestFileName, Attrs, CopyParam, Params, OperationProgress, Flags, Action);

    LogFileDone(OperationProgress, ::ExpandUNCFileName(DestFullName));
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::UpdateTargetAttrs(
  const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType *CopyParam, uintptr_t Attrs)
{
  if (Attrs == -1)
  {
    Attrs = faArchive;
  }
  intptr_t NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
  if ((NewAttrs & Attrs) != NewAttrs)
  {
    FileOperationLoopCustom(this, FOperationProgress, folAllowSkip,
      FMTLOAD(CANT_SET_ATTRS, ADestFullName), "",
    [&]()
    {
      THROWOSIFFALSE(::FileSetAttr(ApiPath(ADestFullName), (DWORD)(Attrs | NewAttrs)) == 0);
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (ADestFullName)));
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::UpdateTargetTime(HANDLE Handle, TDateTime Modification, TDSTMode DSTMode)
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
void __fastcall TTerminal::SinkFile(const UnicodeString &AFileName, const TRemoteFile *AFile, void *AParam)
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
void __fastcall TTerminal::ReflectSettings() const
{
  DebugAssert(FLog != nullptr);
  FLog->ReflectSettings();
  DebugAssert(FActionLog != nullptr);
  FActionLog->ReflectSettings();
  // also FTunnelLog ?
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::CollectUsage()
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
  {
    return FOnCheckForEsc();
  }
  return (FOperationProgress && FOperationProgress->GetCancel() == csCancel);
}
//---------------------------------------------------------------------------
static UnicodeString __fastcall FormatCertificateData(const UnicodeString Fingerprint, intptr_t Failures)
{
  return FORMAT("%s;%2.2X", Fingerprint, Failures);
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::VerifyCertificate(
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
bool __fastcall TTerminal::ConfirmCertificate(
  TSessionInfo & SessionInfo, intptr_t Failures, const UnicodeString CertificateStorageKey, bool CanRemember)
{
  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text = SessionInfo.CertificateFingerprint;

  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
  Aliases[0].ActionAlias = LoadStr(COPY_CERTIFICATE_ACTION);
  Aliases[0].OnClick = nb::bind(&TClipboardHandler::Copy, &ClipboardHandler);

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
void __fastcall TTerminal::CacheCertificate(const UnicodeString CertificateStorageKey,
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
void __fastcall TTerminal::CollectTlsUsage(const UnicodeString /*TlsVersionStr*/)
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
bool __fastcall TTerminal::LoadTlsCertificate(X509 *&Certificate, EVP_PKEY *&PrivateKey)
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
UnicodeString __fastcall TTerminal::GetBaseFileName(const UnicodeString AFileName) const
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
UnicodeString __fastcall TTerminal::ChangeFileName(const TCopyParamType *CopyParam,
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
bool __fastcall TTerminal::CanRecurseToDirectory(const TRemoteFile *AFile) const
{
  return !AFile->GetIsSymLink() || FSessionData->GetFollowDirectorySymlinks();
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::IsThisOrChild(TTerminal *Terminal) const
{
  return
    (this == Terminal) ||
    ((FCommandSession != nullptr) && (FCommandSession == Terminal));
}

void TTerminal::SetLocalFileTime(const UnicodeString LocalFileName,
  FILETIME *AcTime, FILETIME *WrTime)
{
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  FileOperationLoopCustom(this, OperationProgress, folAllowSkip,
    FMTLOAD(CANT_SET_ATTRS, LocalFileName), "",
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
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, (FileName)));
}

HANDLE TTerminal::TerminalCreateLocalFile(const UnicodeString LocalFileName, DWORD DesiredAccess,
  DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (GetOnCreateLocalFile())
  {
    return GetOnCreateLocalFile()(ApiPath(LocalFileName), DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes);
  }
  return ::CreateFile(ApiPath(LocalFileName).c_str(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, nullptr);
}

DWORD TTerminal::GetLocalFileAttributes(const UnicodeString LocalFileName) const
{
  if (GetOnGetLocalFileAttributes())
  {
    return GetOnGetLocalFileAttributes()(ApiPath(LocalFileName));
  }
  return ::FileGetAttrFix(LocalFileName);
}

bool TTerminal::SetLocalFileAttributes(const UnicodeString LocalFileName, DWORD FileAttributes)
{
  if (GetOnSetLocalFileAttributes())
  {
    return GetOnSetLocalFileAttributes()(ApiPath(LocalFileName), FileAttributes);
  }
  return ::FileSetAttr(LocalFileName, FileAttributes);
}

bool TTerminal::MoveLocalFile(const UnicodeString LocalFileName, const UnicodeString NewLocalFileName, DWORD Flags)
{
  if (GetOnMoveLocalFile())
  {
    return GetOnMoveLocalFile()(LocalFileName, NewLocalFileName, Flags);
  }
  return ::MoveFileEx(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), Flags) != FALSE;
}

bool __fastcall TTerminal::RemoveLocalDirectory(const UnicodeString LocalDirName)
{
  if (GetOnRemoveLocalDirectory())
  {
    return GetOnRemoveLocalDirectory()(LocalDirName);
  }
  return RemoveDir(LocalDirName);
}

bool __fastcall TTerminal::CreateLocalDirectory(const UnicodeString LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (GetOnCreateLocalDirectory())
  {
    return GetOnCreateLocalDirectory()(LocalDirName, SecurityAttributes);
  }
  return ::CreateDir(LocalDirName, SecurityAttributes);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall TSecondaryTerminal::TSecondaryTerminal(TTerminal *MainTerminal) :
  TTerminal(OBJECT_CLASS_TSecondaryTerminal),
  FMainTerminal(MainTerminal)
{
}

__fastcall TSecondaryTerminal::TSecondaryTerminal(TObjectClassId Kind, TTerminal *MainTerminal) :
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
    GetSessionData()->SetUserName(FMainTerminal->TerminalGetUserName());
  }
}
//---------------------------------------------------------------------------
void __fastcall TSecondaryTerminal::UpdateFromMain()
{
  if ((FFileSystem != nullptr) && (FMainTerminal->FFileSystem != nullptr))
  {
    FFileSystem->UpdateFromMain(FMainTerminal->FFileSystem);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSecondaryTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  DebugAssert(FileList != nullptr);
}
//---------------------------------------------------------------------------
void __fastcall TSecondaryTerminal::DirectoryModified(const UnicodeString APath,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(APath, SubDirs);
}
//---------------------------------------------------------------------------
TTerminal * __fastcall TSecondaryTerminal::GetPasswordSource()
{
  return FMainTerminal;
}
//---------------------------------------------------------------------------
__fastcall TTerminalList::TTerminalList(TConfiguration *AConfiguration) :
  TObjectList(OBJECT_CLASS_TTerminalList),
  FConfiguration(AConfiguration)
{
  DebugAssert(FConfiguration);
}
//---------------------------------------------------------------------------
__fastcall TTerminalList::~TTerminalList()
{
  DebugAssert(GetCount() == 0);
}
//---------------------------------------------------------------------------
TTerminal * __fastcall TTerminalList::CreateTerminal(TSessionData *Data)
{
  TTerminal *Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}
//---------------------------------------------------------------------------
TTerminal * __fastcall TTerminalList::NewTerminal(TSessionData *Data)
{
  TTerminal *Result = CreateTerminal(Data);
  Add(Result);
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalList::FreeTerminal(TTerminal *Terminal)
{
  DebugAssert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}
//---------------------------------------------------------------------------
void __fastcall TTerminalList::FreeAndNullTerminal(TTerminal *& Terminal)
{
  TTerminal *T = Terminal;
  Terminal = nullptr;
  FreeTerminal(T);
}
//---------------------------------------------------------------------------
TTerminal * __fastcall TTerminalList::GetTerminal(intptr_t Index)
{
  return GetAs<TTerminal>(Index);
}
//---------------------------------------------------------------------------
void __fastcall TTerminalList::RecryptPasswords()
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

