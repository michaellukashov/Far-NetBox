
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include <System.DateUtils.hpp>

#include "Terminal.h"
#include "Queue.h"

class TBackgroundTerminal;

class TParallelTransferQueueItem : public TLocatedQueueItem
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TParallelTransferQueueItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TParallelTransferQueueItem) || TLocatedQueueItem::is(Kind); }
public:
  TParallelTransferQueueItem(const TLocatedQueueItem * ParentItem, TParallelOperation * ParallelOperation);

protected:
  virtual void DoExecute(TTerminal * Terminal) override;

private:
  TParallelOperation * FParallelOperation;
};

class TUserAction : public TObject
{
NB_DISABLE_COPY(TUserAction)
public:
  explicit TUserAction()
  {
  }

  virtual ~TUserAction()
  {
  }

  virtual void Execute(void * Arg) = 0;
  virtual bool Force() const { return false; }
};

class TNotifyAction : public TUserAction
{
NB_DISABLE_COPY(TNotifyAction)
public:
  explicit TNotifyAction(TNotifyEvent AOnNotify) :
    TUserAction(),
    OnNotify(AOnNotify),
    Sender(nullptr)
  {
  }

  virtual void Execute(void * /*Arg*/) override
  {
    if (OnNotify != nullptr)
    {
      OnNotify(Sender);
    }
  }

  TNotifyEvent OnNotify;
  TObject * Sender;
};

class TInformationUserAction : public TUserAction
{
NB_DISABLE_COPY(TInformationUserAction)
public:
  explicit TInformationUserAction(TInformationEvent AOnInformation) :
    OnInformation(AOnInformation),
    Terminal(nullptr),
    Status(false),
    Phase(0)
  {
  }

  virtual void Execute(void * /*Arg*/) override
  {
    if (OnInformation != nullptr)
    {
      OnInformation(Terminal, Str, Status, Phase);
    }
  }

  virtual bool Force() const override
  {
    // we need to propagate mainly the end-phase event even, when user cancels
    // the connection, so that authentication window is closed
    return TUserAction::Force() || (Phase >= 0);
  }

  TInformationEvent OnInformation;
  TTerminal * Terminal;
  UnicodeString Str;
  bool Status;
  intptr_t Phase;
};

class TQueryUserAction : public TUserAction
{
NB_DISABLE_COPY(TQueryUserAction)
public:
  explicit TQueryUserAction(TQueryUserEvent AOnQueryUser) :
    OnQueryUser(AOnQueryUser),
    Sender(nullptr),
    MoreMessages(nullptr),
    Answers(0),
    Params(nullptr),
    Answer(0),
    Type(qtConfirmation)
  {
  }

  virtual void Execute(void * Arg) override
  {
    if (OnQueryUser != nullptr)
    {
      OnQueryUser(Sender, Query, MoreMessages, Answers, Params, Answer, Type, Arg);
    }
  }

  TQueryUserEvent OnQueryUser;
  TObject * Sender;
  UnicodeString Query;
  TStrings * MoreMessages;
  uintptr_t Answers;
  const TQueryParams * Params;
  uintptr_t Answer;
  TQueryType Type;
};

class TPromptUserAction : public TUserAction
{
NB_DISABLE_COPY(TPromptUserAction)
public:
  explicit TPromptUserAction(TPromptUserEvent AOnPromptUser) :
    OnPromptUser(AOnPromptUser),
    Terminal(nullptr),
    Kind(pkPrompt),
    Prompts(nullptr),
    Results(new TStringList()),
    Result(false)
  {
  }

  virtual ~TPromptUserAction()
  {
    SAFE_DESTROY(Results);
  }

  virtual void Execute(void * Arg) override
  {
    if (OnPromptUser != nullptr)
    {
      OnPromptUser(Terminal, Kind, Name, Instructions, Prompts, Results, Result, Arg);
    }
  }

  TPromptUserEvent OnPromptUser;
  TTerminal * Terminal;
  TPromptKind Kind;
  UnicodeString Name;
  UnicodeString Instructions;
  TStrings * Prompts;
  TStrings * Results;
  bool Result;
};

class TShowExtendedExceptionAction : public TUserAction
{
NB_DISABLE_COPY(TShowExtendedExceptionAction)
public:
  explicit TShowExtendedExceptionAction(TExtendedExceptionEvent AOnShowExtendedException) :
    OnShowExtendedException(AOnShowExtendedException),
    Terminal(nullptr),
    E(nullptr)
  {
  }

  virtual void Execute(void * Arg) override
  {
    if (OnShowExtendedException != nullptr)
    {
      OnShowExtendedException(Terminal, E, Arg);
    }
  }

  TExtendedExceptionEvent OnShowExtendedException;
  TTerminal * Terminal;
  Exception * E;
};

class TDisplayBannerAction : public TUserAction
{
NB_DISABLE_COPY(TDisplayBannerAction)
public:
  explicit TDisplayBannerAction(TDisplayBannerEvent AOnDisplayBanner) :
    OnDisplayBanner(AOnDisplayBanner),
    Terminal(nullptr),
    NeverShowAgain(false),
    Options(0)
  {
  }

  virtual void Execute(void * /*Arg*/) override
  {
    if (OnDisplayBanner != nullptr)
    {
      OnDisplayBanner(Terminal, SessionName, Banner, NeverShowAgain, Options);
    }
  }

  TDisplayBannerEvent OnDisplayBanner;
  TTerminal * Terminal;
  UnicodeString SessionName;
  UnicodeString Banner;
  bool NeverShowAgain;
  intptr_t Options;
};

class TReadDirectoryAction : public TUserAction
{
NB_DISABLE_COPY(TReadDirectoryAction)
public:
  explicit TReadDirectoryAction(TReadDirectoryEvent AOnReadDirectory) :
    OnReadDirectory(AOnReadDirectory),
    Sender(nullptr),
    ReloadOnly(false)
  {
  }

  virtual void Execute(void * /*Arg*/) override
  {
    if (OnReadDirectory != nullptr)
    {
      OnReadDirectory(Sender, ReloadOnly);
    }
  }

  TReadDirectoryEvent OnReadDirectory;
  TObject * Sender;
  bool ReloadOnly;
};

class TReadDirectoryProgressAction : public TUserAction
{
NB_DISABLE_COPY(TReadDirectoryProgressAction)
public:
  explicit TReadDirectoryProgressAction(TReadDirectoryProgressEvent AOnReadDirectoryProgress) :
    OnReadDirectoryProgress(AOnReadDirectoryProgress),
    Sender(nullptr),
    Progress(0),
    ResolvedLinks(0),
    Cancel(false)
  {
  }

  virtual void Execute(void * /*Arg*/) override
  {
    if (OnReadDirectoryProgress != nullptr)
    {
      OnReadDirectoryProgress(Sender, Progress, ResolvedLinks, Cancel);
    }
  }

  TReadDirectoryProgressEvent OnReadDirectoryProgress;
  TObject * Sender;
  intptr_t Progress;
  intptr_t ResolvedLinks;
  bool Cancel;
};

class TTerminalItem : public TSignalThread
{
friend class TQueueItem;
friend class TBackgroundTerminal;
NB_DISABLE_COPY(TTerminalItem)
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TTerminalItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTerminalItem) || TSignalThread::is(Kind); }
public:
  explicit TTerminalItem(TTerminalQueue * Queue);
  virtual ~TTerminalItem();
  void InitTerminalItem(intptr_t Index);

  void Process(TQueueItem * Item);
  bool ProcessUserAction(void * Arg);
  void Cancel();
  void Idle();
  bool Pause();
  bool Resume();

protected:
  TTerminalQueue * FQueue;
  TBackgroundTerminal * FTerminal;
  TQueueItem * FItem;
  TCriticalSection FCriticalSection;
  TUserAction * FUserAction;
  bool FCancel;
  bool FPause;

  virtual void ProcessEvent() override;
  virtual void Finished() override;
  bool WaitForUserAction(TQueueItem::TStatus ItemStatus, TUserAction * UserAction);
  bool OverrideItemStatus(TQueueItem::TStatus & ItemStatus) const;

  void TerminalQueryUser(TObject * Sender,
    UnicodeString AQuery, TStrings * MoreMessages, uintptr_t Answers,
    const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void OperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, UnicodeString AFileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void OperationProgress(TFileOperationProgressType & ProgressData);
};

// TSignalThread

int TSimpleThread::ThreadProc(void * Thread)
{
  TSimpleThread * SimpleThread = get_as<TSimpleThread>(Thread);
  DebugAssert(SimpleThread != nullptr);
  if (!SimpleThread)
    return 0;
  try
  {
    SimpleThread->Execute();
  }
  catch (...)
  {
    // we do not expect thread to be terminated with exception
    DebugFail();
  }
  SimpleThread->FFinished = true;
  SimpleThread->Finished();
  return 0;
}

TSimpleThread::TSimpleThread(TObjectClassId Kind) :
  TObject(Kind),
  FThread(nullptr),
  FThreadId(0),
  FFinished(true)
{
}

void TSimpleThread::InitSimpleThread()
{
  FThread = StartThread(nullptr, 0, this, CREATE_SUSPENDED, FThreadId);
}

TSimpleThread::~TSimpleThread()
{
  Close();

  if (FThread != nullptr)
  {
    SAFE_CLOSE_HANDLE(FThread);
  }
}

bool TSimpleThread::IsFinished() const
{
  return FFinished;
}

void TSimpleThread::Start()
{
  if (ResumeThread(FThread) == 1)
  {
    FFinished = false;
  }
}

void TSimpleThread::Finished()
{
}

void TSimpleThread::Close()
{
  if (!FFinished)
  {
    Terminate();
    WaitFor();
  }
}

void TSimpleThread::WaitFor(uintptr_t Milliseconds) const
{
  ::WaitForSingleObject(FThread, static_cast<DWORD>(Milliseconds));
}

// TSignalThread

TSignalThread::TSignalThread(TObjectClassId Kind, bool /*LowPriority*/) :
  TSimpleThread(Kind),
  FEvent(nullptr),
  FTerminated(true)
{
}

void TSignalThread::InitSignalThread(bool LowPriority)
{
  TSimpleThread::InitSimpleThread();
  FEvent = ::CreateEvent(nullptr, false, false, nullptr);
  DebugAssert(FEvent != nullptr);

  if (LowPriority)
  {
    ::SetThreadPriority(FThread, THREAD_PRIORITY_BELOW_NORMAL);
  }
}

TSignalThread::~TSignalThread()
{
  // cannot leave closing to TSimpleThread as we need to close it before
  // destroying the event
  Close();

  if (FEvent)
  {
    SAFE_CLOSE_HANDLE(FEvent);
  }
}

void TSignalThread::Start()
{
  FTerminated = false;
  TSimpleThread::Start();
}

void TSignalThread::TriggerEvent() const
{
  if (FEvent && FEvent != INVALID_HANDLE_VALUE)
  {
    ::SetEvent(FEvent);
  }
}

bool TSignalThread::WaitForEvent()
{
  // should never return -1, so it is only about 0 or 1
  return WaitForEvent(INFINITE) > 0;
}

uintptr_t TSignalThread::WaitForEvent(uint32_t Timeout) const
{
  uintptr_t Res = ::WaitForSingleObject(FEvent, Timeout);
  uintptr_t Result;
  if ((Res == WAIT_TIMEOUT) && !FTerminated)
  {
    Result = static_cast<uintptr_t>(-1);
  }
  else
  {
    Result = ((Res == WAIT_OBJECT_0) && !FTerminated) ? 1 : 0;
  }
  return Result;
}

void TSignalThread::Execute()
{
  while (!FTerminated)
  {
    if (WaitForEvent())
    {
      ProcessEvent();
    }
  }
}

void TSignalThread::Terminate()
{
  FTerminated = true;
  TriggerEvent();
}

// TTerminalQueue

TTerminalQueue::TTerminalQueue(TTerminal * ATerminal,
  TConfiguration * AConfiguration) :
  TSignalThread(OBJECT_CLASS_TTerminalQueue, true),
  FOnQueryUser(nullptr),
  FOnPromptUser(nullptr),
  FOnShowExtendedException(nullptr),
  FOnQueueItemUpdate(nullptr),
  FOnListUpdate(nullptr),
  FOnEvent(nullptr),
  FTerminal(ATerminal),
  FConfiguration(AConfiguration),
  FSessionData(new TSessionData(L"")),
  FItems(new TList()),
  FDoneItems(new TList()),
  FItemsInProcess(0),
  FFreeTerminals(0),
  FTerminals(new TList()),
  FForcedItems(new TList()),
  FTemporaryTerminals(0),
  FOverallTerminals(0),
  FTransfersLimit(2),
  FKeepDoneItemsFor(0),
  FEnabled(true)
{
}

void TTerminalQueue::InitTerminalQueue()
{
  TSignalThread::InitSignalThread(true);
  FOnQueryUser = nullptr;
  FOnPromptUser = nullptr;
  FOnShowExtendedException = nullptr;
  FOnQueueItemUpdate = nullptr;
  FOnListUpdate = nullptr;
  FOnEvent = nullptr;
  FLastIdle = Now();
  FIdleInterval = EncodeTimeVerbose(0, 0, 2, 0);

  DebugAssert(FTerminal != nullptr);
  FSessionData->Assign(FTerminal->GetSessionData());

#if 0
  FItems = new TList();
  FDoneItems = new TList();
  FTerminals = new TList();
  FForcedItems = new TList();

  FItemsSection = new TCriticalSection();
#endif // #if 0

  DebugAssert(FItems);
  DebugAssert(FDoneItems);
  DebugAssert(FTerminals);
  DebugAssert(FForcedItems);

  Start();
}

TTerminalQueue::~TTerminalQueue()
{
  Close();

  {
    TGuard Guard(FItemsSection);

    while (FTerminals->GetCount() > 0)
    {
      TTerminalItem * TerminalItem = FTerminals->GetAs<TTerminalItem>(0);
      FTerminals->Delete(0);
      TerminalItem->Terminate();
      TerminalItem->WaitFor();
      SAFE_DESTROY(TerminalItem);
    }
    SAFE_DESTROY(FTerminals);
    SAFE_DESTROY(FForcedItems);

    FreeItemsList(FItems);
    FreeItemsList(FDoneItems);
  }

  SAFE_DESTROY_EX(TSessionData, FSessionData);
}

void TTerminalQueue::FreeItemsList(TList *& List) const
{
  for (intptr_t Index = 0; Index < List->GetCount(); ++Index)
  {
    TQueueItem * Item = GetItem(List, Index);
    SAFE_DESTROY(Item);
  }
  SAFE_DESTROY(List);
}

void TTerminalQueue::TerminalFinished(TTerminalItem * TerminalItem)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FTerminals->IndexOf(TerminalItem);
      DebugAssert(Index >= 0);

      if (Index < FFreeTerminals)
      {
        FFreeTerminals--;
      }

      // Index may be >= FTransfersLimit also when the transfer limit was
      // recently decreased, then
      // FTemporaryTerminals < FTerminals->Count - FTransfersLimit
      if ((FTransfersLimit >= 0) && (Index >= FTransfersLimit) && (FTemporaryTerminals > 0))
      {
        FTemporaryTerminals--;
      }

      FTerminals->Extract(TerminalItem);

      SAFE_DESTROY(TerminalItem);
    }

    TriggerEvent();
  }
}

bool TTerminalQueue::TerminalFree(TTerminalItem * TerminalItem)
{
  bool Result = true;

  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FTerminals->IndexOf(TerminalItem);
      DebugAssert(Index >= 0);
      DebugAssert(Index >= FFreeTerminals);

      Result = (FTransfersLimit < 0) || (Index < FTransfersLimit);
      if (Result)
      {
        FTerminals->Move(Index, 0);
        FFreeTerminals++;
      }
    }

    TriggerEvent();
  }

  return Result;
}

intptr_t TTerminalQueue::GetParallelDurationThreshold() const
{
  return FConfiguration->GetParallelDurationThreshold();
}

void TTerminalQueue::AddItem(TQueueItem * Item)
{
  DebugAssert(!FTerminated);

  Item->SetStatus(TQueueItem::qsPending);

  {
    TGuard Guard(FItemsSection);

    FItems->Add(Item);
    Item->FQueue = this;
  }

  DoListUpdate();

  TriggerEvent();
}

void TTerminalQueue::RetryItem(TQueueItem * Item)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FItems->Remove(Item);
      DebugAssert(Index < FItemsInProcess);
      DebugUsedParam(Index);
      FItemsInProcess--;
      FItems->Add(Item);
    }

    DoListUpdate();

    TriggerEvent();
  }
}

void TTerminalQueue::DeleteItem(TQueueItem * Item, bool CanKeep)
{
  if (!FTerminated)
  {
    bool Empty;
    bool EmptyButMonitored;
    bool Monitored;
    {
      TGuard Guard(FItemsSection);

      // does this need to be within guard?
      Monitored = (Item->GetCompleteEvent() != INVALID_HANDLE_VALUE);
      intptr_t Index = FItems->Remove(Item);
      DebugAssert(Index < FItemsInProcess);
      DebugUsedParam(Index);
      FItemsInProcess--;
      FForcedItems->Remove(Item);
      // =0  do not keep
      // <0  infinity
      if ((FKeepDoneItemsFor != 0) && CanKeep && Item->Complete())
      {
        DebugAssert(Item->GetStatus() == TQueueItem::qsDone);
        FDoneItems->Add(Item);
      }
      else
      {
        SAFE_DESTROY(Item);
      }

      EmptyButMonitored = true;
      Index = 0;
      while (EmptyButMonitored && (Index < FItems->GetCount()))
      {
        EmptyButMonitored = (GetItem(FItems, Index)->GetCompleteEvent() != INVALID_HANDLE_VALUE);
        ++Index;
      }
      Empty = (FItems->GetCount() == 0);
    }

    DoListUpdate();
    // report empty but/except for monitored, if queue is empty or only monitored items are pending.
    // do not report if current item was the last, but was monitored.
    if (!Monitored && EmptyButMonitored)
    {
      DoEvent(qeEmptyButMonitored);
    }
    if (Empty)
    {
      DoEvent(qeEmpty);
    }
  }
}

TQueueItem * TTerminalQueue::GetItem(TList * List, intptr_t Index)
{
  return List->GetAs<TQueueItem>(Index);
}

TQueueItem * TTerminalQueue::GetItem(intptr_t Index) const
{
  return FItems->GetAs<TQueueItem>(Index);
}

void TTerminalQueue::UpdateStatusForList(
  TTerminalQueueStatus * Status, TList * List, TTerminalQueueStatus * Current)
{
  for (intptr_t Index = 0; Index < List->GetCount(); ++Index)
  {
    TQueueItem * Item = GetItem(List, Index);
    TQueueItemProxy * ItemProxy = nullptr;
    if (Current)
    {
      ItemProxy = Current->FindByQueueItem(Item);
    }

    if (Current && ItemProxy)
    {
      Current->Delete(ItemProxy);
      Status->Add(ItemProxy);
      ItemProxy->Update();
    }
    else
    {
      Status->Add(new TQueueItemProxy(this, Item));
    }
  }
}

TTerminalQueueStatus * TTerminalQueue::CreateStatus(TTerminalQueueStatus * Current)
{
  std::unique_ptr<TTerminalQueueStatus> Status(new TTerminalQueueStatus());
  try__catch
  {
    try__finally
    {
      SCOPE_EXIT
      {
        if (Current != nullptr)
        {
          SAFE_DESTROY(Current);
        }
      };

      TGuard Guard(FItemsSection);

      UpdateStatusForList(Status.get(), FDoneItems, Current);
      Status->SetDoneCount(Status->GetCount());
      UpdateStatusForList(Status.get(), FItems, Current);
    }
    __finally
    {
#if 0
      if (Current != nullptr)
      {
        delete Current;
      }
#endif // #if 0
    };
  }
#if 0
  catch (...)
  {
    delete Status;
    throw;
  }
#endif // #if 0
  return Status.release();
}

bool TTerminalQueue::ItemGetData(TQueueItem * Item,
  TQueueItemProxy * Proxy)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection);

    Result = (FDoneItems->IndexOf(Item) >= 0) || (FItems->IndexOf(Item) >= 0);
    if (Result)
    {
      Item->GetData(Proxy);
    }
  }

  return Result;
}

bool TTerminalQueue::ItemProcessUserAction(TQueueItem * Item, void * Arg)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TTerminalItem * TerminalItem = nullptr;

    {
      TGuard Guard(FItemsSection);

      Result = (FItems->IndexOf(Item) >= 0) &&
        TQueueItem::IsUserActionStatus(Item->GetStatus());
      if (Result)
      {
        TerminalItem = Item->FTerminalItem;
      }
    }

    if (Result)
    {
      Result = TerminalItem->ProcessUserAction(Arg);
    }
  }

  return Result;
}

bool TTerminalQueue::ItemMove(TQueueItem * Item, TQueueItem * BeforeItem)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FItems->IndexOf(Item);
      intptr_t IndexDest = FItems->IndexOf(BeforeItem);
      Result = (Index >= 0) && (IndexDest >= 0) &&
        (Item->GetStatus() == TQueueItem::qsPending) &&
        (BeforeItem->GetStatus() == TQueueItem::qsPending);
      if (Result)
      {
        FItems->Move(Index, IndexDest);
      }
    }

    if (Result)
    {
      DoListUpdate();
      TriggerEvent();
    }
  }

  return Result;
}

bool TTerminalQueue::ItemExecuteNow(TQueueItem * Item)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FItems->IndexOf(Item);
      Result = (Index >= 0) && (Item->GetStatus() == TQueueItem::qsPending) &&
        // prevent double-initiation when "execute" is clicked twice too fast
        (Index >= FItemsInProcess);
      if (Result)
      {
        if (Index > FItemsInProcess)
        {
          FItems->Move(Index, FItemsInProcess);
        }

        if ((FTransfersLimit >= 0) && (FTerminals->GetCount() >= FTransfersLimit) &&
          // when queue is disabled, we may have idle terminals,
          // even when there are pending queue items
          (FFreeTerminals == 0))
        {
          FTemporaryTerminals++;
        }

        FForcedItems->Add(Item);
      }
    }

    if (Result)
    {
      DoListUpdate();
      TriggerEvent();
    }
  }

  return Result;
}

bool TTerminalQueue::ItemDelete(TQueueItem * Item)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    bool UpdateList = false;

    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FItems->IndexOf(Item);
      Result = (Index >= 0);
      if (Result)
      {
        if (Item->GetStatus() == TQueueItem::qsPending)
        {
          FItems->Delete(Index);
          FForcedItems->Remove(Item);
          SAFE_DESTROY(Item);
          UpdateList = true;
        }
        else
        {
          Item->FTerminalItem->Cancel();
        }
      }
      else
      {
        Index = FDoneItems->IndexOf(Item);
        Result = (Index >= 0);
        if (Result)
        {
          FDoneItems->Delete(Index);
          UpdateList = true;
        }
      }
    }

    if (UpdateList)
    {
      DoListUpdate();
      TriggerEvent();
    }
  }

  return Result;
}

bool TTerminalQueue::ItemPause(TQueueItem * Item, bool Pause)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TTerminalItem * TerminalItem = nullptr;

    {
      TGuard Guard(FItemsSection);

      Result = (FItems->IndexOf(Item) >= 0) &&
      ((Pause && (Item->GetStatus() == TQueueItem::qsProcessing)) ||
        (!Pause && (Item->GetStatus() == TQueueItem::qsPaused)));
      if (Result)
      {
        TerminalItem = Item->FTerminalItem;
      }
    }

    if (Result)
    {
      if (Pause)
      {
        Result = TerminalItem->Pause();
      }
      else
      {
        Result = TerminalItem->Resume();
      }
    }
  }

  return Result;
}

bool TTerminalQueue::ItemSetCPSLimit(TQueueItem * Item, intptr_t CPSLimit)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection);

    Result = (FItems->IndexOf(Item) >= 0);
    if (Result)
    {
      Item->SetCPSLimit(CPSLimit);
    }
  }

  return Result;
}

bool TTerminalQueue::ItemGetCPSLimit(TQueueItem * Item, intptr_t & CPSLimit) const
{
  CPSLimit = 0;
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection);

    Result = (FItems->IndexOf(Item) >= 0);
    if (Result)
    {
      CPSLimit = Item->GetCPSLimit();
    }
  }

  return Result;
}

void TTerminalQueue::Idle()
{
  TDateTime N = Now();
  if (N - FLastIdle > FIdleInterval)
  {
    FLastIdle = N;
    TTerminalItem * TerminalItem = nullptr;

    if (FFreeTerminals > 0)
    {
      TGuard Guard(FItemsSection);

      if (FFreeTerminals > 0)
      {
        // take the last free terminal, because TerminalFree() puts it to the
        // front, this ensures we cycle thru all free terminals
        TerminalItem = FTerminals->GetAs<TTerminalItem>(FFreeTerminals - 1);
        FTerminals->Move(FFreeTerminals - 1, FTerminals->GetCount() - 1);
        FFreeTerminals--;
      }
    }

    if (TerminalItem != nullptr)
    {
      TerminalItem->Idle();
    }
  }
}

bool TTerminalQueue::WaitForEvent()
{
  // terminate loop regularly, so that we can check for expired done items
  bool Result = (TSignalThread::WaitForEvent(1000) != 0);
  return Result;
}

void TTerminalQueue::ProcessEvent()
{
  TTerminalItem * TerminalItem;
  do
  {
    TerminalItem = nullptr;
    TQueueItem * Item1 = nullptr;

    {
      TGuard Guard(FItemsSection);

      // =0  do not keep
      // <0  infinity
      if (FKeepDoneItemsFor >= 0)
      {
        TDateTime RemoveDoneItemsBefore = Now();
        if (FKeepDoneItemsFor > 0)
        {
          RemoveDoneItemsBefore = ::IncSecond(RemoveDoneItemsBefore, -FKeepDoneItemsFor);
        }
        for (intptr_t Index = 0; Index < FDoneItems->GetCount(); ++Index)
        {
          TQueueItem * Item2 = GetItem(FDoneItems, Index);
          if (Item2->FDoneAt <= RemoveDoneItemsBefore)
          {
            FDoneItems->Delete(Index);
            SAFE_DESTROY(Item2);
            Index--;
            DoListUpdate();
          }
        }
      }

      if (FItems->GetCount() > FItemsInProcess)
      {
        Item1 = GetItem(FItemsInProcess);
        intptr_t ForcedIndex = FForcedItems->IndexOf(Item1);

        if (FEnabled || (ForcedIndex >= 0))
        {
          if ((FFreeTerminals == 0) &&
            ((FTransfersLimit <= 0) ||
              (FTerminals->GetCount() < FTransfersLimit + FTemporaryTerminals)))
          {
            FOverallTerminals++;
            TerminalItem = new TTerminalItem(this);
            TerminalItem->InitTerminalItem(FOverallTerminals);
            FTerminals->Add(TerminalItem);
          }
          else if (FFreeTerminals > 0)
          {
            TerminalItem = FTerminals->GetAs<TTerminalItem>(0);
            FTerminals->Move(0, FTerminals->GetCount() - 1);
            FFreeTerminals--;
          }

          if (TerminalItem != nullptr)
          {
            if (ForcedIndex >= 0)
            {
              FForcedItems->Delete(ForcedIndex);
            }
            FItemsInProcess++;
          }
        }
      }
    }

    if (TerminalItem != nullptr)
    {
      TerminalItem->Process(Item1);
    }
  }
  while (!FTerminated && (TerminalItem != nullptr));
}

void TTerminalQueue::DoQueueItemUpdate(TQueueItem * Item)
{
  if (GetOnQueueItemUpdate() != nullptr)
  {
    GetOnQueueItemUpdate()(this, Item);
  }
}

void TTerminalQueue::DoListUpdate()
{
  if (GetOnListUpdate() != nullptr)
  {
    GetOnListUpdate()(this);
  }
}

void TTerminalQueue::DoEvent(TQueueEvent Event)
{
  if (GetOnEvent() != nullptr)
  {
    GetOnEvent()(this, Event);
  }
}

void TTerminalQueue::SetTransfersLimit(intptr_t Value)
{
  if (FTransfersLimit != Value)
  {
    {
      TGuard Guard(FItemsSection);

      if ((Value >= 0) && (Value < FItemsInProcess))
      {
        FTemporaryTerminals = (FItemsInProcess - Value);
      }
      else
      {
        FTemporaryTerminals = 0;
      }
      FTransfersLimit = Value;
    }

    TriggerEvent();
  }
}

void TTerminalQueue::SetKeepDoneItemsFor(intptr_t Value)
{
  if (FKeepDoneItemsFor != Value)
  {
    {
      TGuard Guard(FItemsSection);

      FKeepDoneItemsFor = Value;
    }
  }
}

void TTerminalQueue::SetEnabled(bool Value)
{
  if (FEnabled != Value)
  {
    {
      TGuard Guard(FItemsSection);

      FEnabled = Value;
    }

    TriggerEvent();
  }
}

bool TTerminalQueue::GetIsEmpty() const
{
  TGuard Guard(FItemsSection);
  return (FItems->GetCount() == 0);
}

bool TTerminalQueue::TryAddParallelOperation(TQueueItem * Item, bool Force)
{
  TGuard Guard(FItemsSection);

  bool Result =
    (FFreeTerminals > 0) ||
    (Force && (FItemsInProcess < FTransfersLimit));

  if (Result)
  {
    AddItem(DebugNotNull(Item->CreateParallelOperation()));
  }

  return Result;
}

bool TTerminalQueue::ContinueParallelOperation() const
{
  TGuard Guard(FItemsSection);

  return (FItems->GetCount() <= FItemsInProcess);
}

// TBackgroundItem

class TBackgroundTerminal : public TSecondaryTerminal
{
friend class TTerminalItem;
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TBackgroundTerminal); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TBackgroundTerminal) || TSecondaryTerminal::is(Kind); }

public:
  explicit TBackgroundTerminal(TTerminal * MainTerminal);

  virtual ~TBackgroundTerminal()
  {
  }

  void Init(
    TSessionData * SessionData, TConfiguration * Configuration,
    TTerminalItem * Item, UnicodeString Name);

protected:
  virtual bool DoQueryReopen(Exception * E) override;

private:
  TTerminalItem * FItem;
};

TBackgroundTerminal::TBackgroundTerminal(TTerminal * MainTerminal) :
  TSecondaryTerminal(OBJECT_CLASS_TBackgroundTerminal, MainTerminal),
  FItem(nullptr)
{
}

void TBackgroundTerminal::Init(
    TSessionData * SessionData, TConfiguration * Configuration, TTerminalItem * Item,
    UnicodeString Name)
{
  TSecondaryTerminal::Init(SessionData, Configuration, Name);
  FItem = Item;
}

bool TBackgroundTerminal::DoQueryReopen(Exception * /*E*/)
{
  bool Result;
  if (FItem->FTerminated || FItem->FCancel)
  {
    // avoid reconnection if we are closing
    Result = false;
  }
  else
  {
    ::Sleep(static_cast<DWORD>(GetConfiguration()->GetSessionReopenBackground()));
    Result = true;
  }
  return Result;
}

// TTerminalItem

TTerminalItem::TTerminalItem(TTerminalQueue * Queue) :
  TSignalThread(OBJECT_CLASS_TTerminalItem, true),
  FQueue(Queue),
  FTerminal(nullptr),
  FItem(nullptr),
  FUserAction(nullptr),
  FCancel(false),
  FPause(false)
{
}

void TTerminalItem::InitTerminalItem(intptr_t Index)
{
  TSignalThread::InitSignalThread(true);

  std::unique_ptr<TBackgroundTerminal> Terminal(new TBackgroundTerminal(FQueue->FTerminal));
  try
  {
    Terminal->Init(FQueue->FSessionData, FQueue->FConfiguration, this, FORMAT("Background %d", Index));
    Terminal->SetUseBusyCursor(false);

    Terminal->SetOnQueryUser(nb::bind(&TTerminalItem::TerminalQueryUser, this));
    Terminal->SetOnPromptUser(nb::bind(&TTerminalItem::TerminalPromptUser, this));
    Terminal->SetOnShowExtendedException(nb::bind(&TTerminalItem::TerminalShowExtendedException, this));
    Terminal->SetOnProgress(nb::bind(&TTerminalItem::OperationProgress, this));
    Terminal->SetOnFinished(nb::bind(&TTerminalItem::OperationFinished, this));

    FTerminal = Terminal.release();
  }
  catch (...)
  {
#if 0
    delete FTerminal;
    throw;
#endif
  }

  Start();
}

TTerminalItem::~TTerminalItem()
{
  Close();

  DebugAssert(FItem == nullptr);
  SAFE_DESTROY(FTerminal);
}

void TTerminalItem::Process(TQueueItem * Item)
{
  {
    TGuard Guard(FCriticalSection);

    DebugAssert(FItem == nullptr);
    FItem = Item;
  }

  TriggerEvent();
}

void TTerminalItem::ProcessEvent()
{
  if (!FItem)
    return;
  TGuard Guard(FCriticalSection);

  bool Retry = true;

  FCancel = false;
  FPause = false;

  try
  {
    DebugAssert(FItem != nullptr);

    FItem->FTerminalItem = this;

    if (!FTerminal->GetActive())
    {
      FItem->SetStatus(TQueueItem::qsConnecting);

      FTerminal->GetSessionData()->SetRemoteDirectory(FItem->GetStartupDirectory());
      FTerminal->Open();
    }

    Retry = false;

    if (!FCancel)
    {
      FTerminal->UpdateFromMain();

      FItem->SetStatus(TQueueItem::qsProcessing);

      FItem->Execute(this);
    }
  }
  catch (Exception & E)
  {
    UnicodeString Message;
    if (ExceptionMessageFormatted(&E, Message))
    {
      // do not show error messages, if task was canceled anyway
      // (for example if transfer is canceled during reconnection attempts)
      if (!FCancel &&
          (FTerminal->QueryUserException(L"", &E, qaOK | qaCancel, nullptr, qtError) == qaCancel))
      {
        FCancel = true;
      }
    }
  }

  FItem->SetStatus(TQueueItem::qsDone);

  FItem->FTerminalItem = nullptr;

  TQueueItem * Item = FItem;
  FItem = nullptr;

  if (Retry && !FCancel)
  {
    FQueue->RetryItem(Item);
  }
  else
  {
    FQueue->DeleteItem(Item, !FCancel);
  }

  if (!FTerminal->GetActive() ||
      !FQueue->TerminalFree(this))
  {
    Terminate();
  }
}

void TTerminalItem::Idle()
{
  TGuard Guard(FCriticalSection);

  DebugAssert(FTerminal->GetActive());

  try
  {
    FTerminal->Idle();
  }
  catch (...)
  {
  }

  if (!FTerminal->GetActive() ||
    !FQueue->TerminalFree(this))
  {
    Terminate();
  }
}

void TTerminalItem::Cancel()
{
  FCancel = true;
  if ((FItem->GetStatus() == TQueueItem::qsPaused) ||
      TQueueItem::IsUserActionStatus(FItem->GetStatus()))
  {
    TriggerEvent();
  }
}

bool TTerminalItem::Pause()
{
  DebugAssert(FItem != nullptr);
  bool Result = FItem && (FItem->GetStatus() == TQueueItem::qsProcessing) && !FPause;
  if (Result)
  {
    FPause = true;
  }
  return Result;
}

bool TTerminalItem::Resume()
{
  DebugAssert(FItem != nullptr);
  bool Result = FItem && (FItem->GetStatus() == TQueueItem::qsPaused);
  if (Result)
  {
    TriggerEvent();
  }
  return Result;
}

bool TTerminalItem::ProcessUserAction(void * Arg)
{
  // When status is changed twice quickly, the controller when responding
  // to the first change (non-user-action) can be so slow to check only after
  // the second (user-action) change occurs. Thus it responds it.
  // Then as reaction to the second (user-action) change there will not be
  // any outstanding user-action.
  bool Result = (FUserAction != nullptr);
  if (Result)
  {
    DebugAssert(FItem != nullptr);

    FUserAction->Execute(Arg);
    FUserAction = nullptr;

    TriggerEvent();
  }
  return Result;
}

bool TTerminalItem::WaitForUserAction(
  TQueueItem::TStatus ItemStatus, TUserAction * UserAction)
{
  DebugAssert(FItem != nullptr);
  DebugAssert((FItem->GetStatus() == TQueueItem::qsProcessing) ||
    (FItem->GetStatus() == TQueueItem::qsConnecting));
  if (!FItem)
    return false;

  bool Result;

  TQueueItem::TStatus PrevStatus = FItem->GetStatus();

  try__finally
  {
    SCOPE_EXIT
    {
      FUserAction = nullptr;
      FItem->SetStatus(PrevStatus);
    };
    FUserAction = UserAction;

    FItem->SetStatus(ItemStatus);
    FQueue->DoEvent(qePendingUserAction);

    Result = !FTerminated && WaitForEvent() && !FCancel;
  }
  __finally
  {
#if 0
    FUserAction = nullptr;
    FItem->SetStatus(PrevStatus);
#endif // #if 0
  };

  return Result;
}

void TTerminalItem::Finished()
{
  TSignalThread::Finished();

  FQueue->TerminalFinished(this);
}

void TTerminalItem::TerminalQueryUser(TObject * Sender,
  UnicodeString AQuery, TStrings * MoreMessages, uintptr_t Answers,
  const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg)
{
  // so far query without queue item can occur only for key confirmation
  // on re-key with non-cached host key. make it fail.
  if (FItem != nullptr)
  {
    DebugUsedParam(Arg);
    DebugAssert(Arg == nullptr);

    TQueryUserAction Action(FQueue->GetOnQueryUser());
    Action.Sender = Sender;
    Action.Query = AQuery;
    Action.MoreMessages = MoreMessages;
    Action.Answers = Answers;
    Action.Params = Params;
    Action.Answer = Answer;
    Action.Type = Type;

    // if the query is "error", present it as an "error" state in UI,
    // however it is still handled as query by the action.

    TQueueItem::TStatus ItemStatus =
      (Action.Type == qtError ? TQueueItem::qsError : TQueueItem::qsQuery);

    if (WaitForUserAction(ItemStatus, &Action))
    {
      Answer = Action.Answer;
    }
  }
}

void TTerminalItem::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
  TStrings * Results, bool & Result, void * Arg)
{
  if (FItem == nullptr)
  {
    // sanity, should not occur
    DebugFail();
    Result = false;
  }
  else
  {
    DebugUsedParam(Arg);
    DebugAssert(Arg == nullptr);

    TPromptUserAction Action(FQueue->GetOnPromptUser());
    Action.Terminal = Terminal;
    Action.Kind = Kind;
    Action.Name = Name;
    Action.Instructions = Instructions;
    Action.Prompts = Prompts;
    Action.Results->AddStrings(Results);

    if (WaitForUserAction(TQueueItem::qsPrompt, &Action))
    {
      Results->Clear();
      Results->AddStrings(Action.Results);
      Result = Action.Result;
    }
  }
}

void TTerminalItem::TerminalShowExtendedException(
  TTerminal * Terminal, Exception * E, void * Arg)
{
  DebugUsedParam(Arg);
  DebugAssert(Arg == nullptr);

  if ((FItem != nullptr) &&
      ShouldDisplayException(E))
  {
    TShowExtendedExceptionAction Action(FQueue->GetOnShowExtendedException());
    Action.Terminal = Terminal;
    Action.E = E;

    WaitForUserAction(TQueueItem::qsError, &Action);
  }
}

void TTerminalItem::OperationFinished(TFileOperation /*Operation*/,
  TOperationSide /*Side*/, bool /*Temp*/, UnicodeString /*AFileName*/,
  bool /*Success*/, TOnceDoneOperation & /*OnceDoneOperation*/)
{
  // nothing
}

void TTerminalItem::OperationProgress(
  TFileOperationProgressType & ProgressData)
{
  if (FPause && !FTerminated && !FCancel && FItem)
  {
    DebugAssert(FItem != nullptr);
    TQueueItem::TStatus PrevStatus = FItem->GetStatus();
    DebugAssert(PrevStatus == TQueueItem::qsProcessing);
    // must be set before TFileOperationProgressType::Suspend(), because
    // it invokes this method back
    FPause = false;
    ProgressData.Suspend();

    try__finally
    {
      SCOPE_EXIT
      {
        FItem->SetStatus(PrevStatus);
        ProgressData.Resume();
      };
      FItem->SetStatus(TQueueItem::qsPaused);

      WaitForEvent();
    }
    __finally
    {
#if 0
      FItem->SetStatus(PrevStatus);
      ProgressData.Resume();
#endif // #if 0
    };
  }

  if (FTerminated || FCancel)
  {
    if (ProgressData.GetTransferringFile())
    {
      ProgressData.SetCancel(csCancelTransfer);
    }
    else
    {
      ProgressData.SetCancel(csCancel);
    }
  }

  DebugAssert(FItem != nullptr);
  FItem->SetProgress(ProgressData);
}

bool TTerminalItem::OverrideItemStatus(TQueueItem::TStatus & ItemStatus) const
{
  DebugAssert(FTerminal != nullptr);
  bool Result = (FTerminal && FTerminal->GetStatus() < ssOpened) && (ItemStatus == TQueueItem::qsProcessing);
  if (Result)
  {
    ItemStatus = TQueueItem::qsConnecting;
  }
  return Result;
}

// TQueueItem

TQueueItem::TQueueItem(TObjectClassId Kind) :
  TObject(Kind),
  FStatus(qsPending),
  FTerminalItem(nullptr),
  FProgressData(nullptr),
  FInfo(new TInfo()),
  FQueue(nullptr),
  FCompleteEvent(INVALID_HANDLE_VALUE),
  FCPSLimit(static_cast<uintptr_t>(-1))
{
  FInfo->SingleFile = false;
  FInfo->Primary = true;
  FInfo->GroupToken = this;
}

TQueueItem::~TQueueItem()
{
  // we need to keep the total transfer size even after transfer completes
  SAFE_DESTROY(FProgressData);

  Complete();

  SAFE_DESTROY(FInfo);
}

bool TQueueItem::Complete()
{
  TGuard Guard(FSection);

  if (FCompleteEvent != INVALID_HANDLE_VALUE)
  {
    ::SetEvent(FCompleteEvent);
    FCompleteEvent = INVALID_HANDLE_VALUE;
  }

  return FInfo->Primary;
}

bool TQueueItem::IsUserActionStatus(TStatus Status)
{
  return (Status == qsQuery) || (Status == qsError) || (Status == qsPrompt);
}

TQueueItem::TStatus TQueueItem::GetStatus() const
{
  TGuard Guard(FSection);

  return FStatus;
}

void TQueueItem::SetStatus(TStatus Status)
{
  {
    TGuard Guard(FSection);

    FStatus = Status;
    if (FStatus == qsDone)
    {
      FDoneAt = Now();
    }
  }

  DebugAssert((FQueue != nullptr) || (Status == qsPending));
  if (FQueue != nullptr)
  {
    FQueue->DoQueueItemUpdate(this);
  }
}

void TQueueItem::ProgressUpdated()
{
  // noop
}

void TQueueItem::SetProgress(
  TFileOperationProgressType & ProgressData)
{
  {
    TGuard Guard(FSection);

    // do not lose CPS limit override on "calculate size" operation,
    // wait until the real transfer operation starts
    if ((FCPSLimit >= 0) && ((ProgressData.GetOperation() == foMove) || (ProgressData.GetOperation() == foCopy)))
    {
      ProgressData.SetCPSLimit(FCPSLimit);
      FCPSLimit = -1;
    }

    DebugAssert(FProgressData != nullptr);
    FProgressData->Assign(ProgressData);
    FProgressData->Reset();
  }
  ProgressUpdated();
  FQueue->DoQueueItemUpdate(this);
}

void TQueueItem::GetData(TQueueItemProxy * Proxy) const
{
  TGuard Guard(FSection);

  DebugAssert(Proxy->FProgressData != nullptr);
  if (FProgressData != nullptr)
  {
    Proxy->FProgressData->Assign(*FProgressData);
  }
  else
  {
    Proxy->FProgressData->Clear();
  }
  *Proxy->FInfo = *FInfo;
  Proxy->FStatus = FStatus;
  if (FTerminalItem != nullptr)
  {
    FTerminalItem->OverrideItemStatus(Proxy->FStatus);
  }
}

void TQueueItem::Execute(TTerminalItem * TerminalItem)
{
  {
    DebugAssert(FProgressData == nullptr);
    TGuard Guard(FSection);
    FProgressData = new TFileOperationProgressType();
  }
  DoExecute(TerminalItem->FTerminal);
}

void TQueueItem::SetCPSLimit(intptr_t CPSLimit)
{
  FCPSLimit = CPSLimit;
}

intptr_t TQueueItem::DefaultCPSLimit() const
{
  return 0;
}

intptr_t TQueueItem::GetCPSLimit() const
{
  intptr_t Result;
  if (FCPSLimit >= 0)
  {
    Result = FCPSLimit;
  }
  else if (FProgressData != nullptr)
  {
    Result = FProgressData->GetCPSLimit();
  }
  else
  {
    Result = DefaultCPSLimit();
  }
  return Result;
}

TQueueItem * TQueueItem::CreateParallelOperation()
{
  return nullptr;
}

// TQueueItemProxy

TQueueItemProxy::TQueueItemProxy(TTerminalQueue * Queue,
  TQueueItem * QueueItem) :
  TObject(OBJECT_CLASS_TQueueItemProxy),
  FProgressData(new TFileOperationProgressType()),
  FStatus(TQueueItem::qsPending),
  FQueue(Queue),
  FQueueItem(QueueItem),
  FQueueStatus(nullptr),
  FInfo(new TQueueItem::TInfo()),
  FProcessingUserAction(false),
  FUserData(nullptr)
{
  Update();
}

TQueueItemProxy::~TQueueItemProxy()
{
  SAFE_DESTROY(FProgressData);
  SAFE_DESTROY(FInfo);
}

TFileOperationProgressType * TQueueItemProxy::GetProgressData() const
{
  return (FProgressData->GetOperation() == foNone) ? nullptr : FProgressData;
}

int64_t TQueueItemProxy::GetTotalTransferred() const
{
  // want to show total transferred also for "completed" items,
  // for which GetProgressData() is nullptr
  return
    (FProgressData->GetOperation() == GetInfo()->Operation) || (GetStatus() == TQueueItem::qsDone) ?
      FProgressData->GetTotalTransferred() : -1;
}

bool TQueueItemProxy::Update()
{
  DebugAssert(FQueueItem != nullptr);

  TQueueItem::TStatus PrevStatus = GetStatus();

  bool Result = FQueue->ItemGetData(FQueueItem, this);

  if ((FQueueStatus != nullptr) && (PrevStatus != GetStatus()))
  {
    FQueueStatus->ResetStats();
  }

  return Result;
}

bool TQueueItemProxy::ExecuteNow()
{
  return FQueue->ItemExecuteNow(FQueueItem);
}

bool TQueueItemProxy::Move(bool Sooner)
{
  bool Result = false;
  intptr_t Index = GetIndex();
  if (Sooner)
  {
    if (Index > 0)
    {
      Result = Move(FQueueStatus->GetItem(Index - 1));
    }
  }
  else
  {
    if (Index < FQueueStatus->GetCount() - 1)
    {
      Result = FQueueStatus->GetItem(Index + 1)->Move(this);
    }
  }
  return Result;
}

bool TQueueItemProxy::Move(TQueueItemProxy * BeforeItem)
{
  return FQueue->ItemMove(FQueueItem, BeforeItem->FQueueItem);
}

bool TQueueItemProxy::Delete()
{
  return FQueue->ItemDelete(FQueueItem);
}

bool TQueueItemProxy::Pause()
{
  return FQueue->ItemPause(FQueueItem, true);
}

bool TQueueItemProxy::Resume()
{
  return FQueue->ItemPause(FQueueItem, false);
}

bool TQueueItemProxy::ProcessUserAction()
{
  DebugAssert(FQueueItem != nullptr);

  bool Result;
  FProcessingUserAction = true;
  try__finally
  {
    SCOPE_EXIT
    {
      FProcessingUserAction = false;
    };
    Result = FQueue->ItemProcessUserAction(FQueueItem, nullptr);
  }
  __finally
  {
#if 0
    FProcessingUserAction = false;
#endif // #if 0
  };
  return Result;
}

bool TQueueItemProxy::GetCPSLimit(intptr_t & CPSLimit) const
{
  return FQueue->ItemGetCPSLimit(FQueueItem, CPSLimit);
}

bool TQueueItemProxy::SetCPSLimit(intptr_t CPSLimit)
{
  return FQueue->ItemSetCPSLimit(FQueueItem, CPSLimit);
}

intptr_t TQueueItemProxy::GetIndex() const
{
  DebugAssert(FQueueStatus != nullptr);
  intptr_t Index = FQueueStatus ? FQueueStatus->FList->IndexOf(this) : 0;
  DebugAssert(Index >= 0);
  return Index;
}

// TTerminalQueueStatus

TTerminalQueueStatus::TTerminalQueueStatus() :
  FList(new TList()),
  FDoneCount(0),
  FActiveCount(0),
  FActivePrimaryCount(0),
  FActiveAndPendingPrimaryCount(0)
{
  ResetStats();
}

TTerminalQueueStatus::~TTerminalQueueStatus()
{
  for (intptr_t Index = 0; Index < FList->GetCount(); ++Index)
  {
    TQueueItemProxy * Item = GetItem(Index);
    SAFE_DESTROY(Item);
  }
  SAFE_DESTROY(FList);
}

void TTerminalQueueStatus::ResetStats() const
{
  FActiveCount = -1;
  FActivePrimaryCount = -1;
  FActiveAndPendingPrimaryCount = -1;
}

void TTerminalQueueStatus::SetDoneCount(intptr_t Value)
{
  FDoneCount = Value;
  ResetStats();
}

void TTerminalQueueStatus::NeedStats() const
{
  if (FActiveCount < 0)
  {
    FActiveCount = 0;
    FActivePrimaryCount = 0;
    FActiveAndPendingPrimaryCount = 0;
    for (intptr_t Index = GetDoneCount(); Index < GetCount(); ++Index)
    {
      bool Primary = GetItem(Index)->GetInfo()->Primary;

      if (GetItem(Index)->GetStatus() != TQueueItem::qsPending)
      {
        FActiveCount++;
        if (Primary)
        {
          ++FActivePrimaryCount;
        }
      }
      if (Primary)
      {
        ++FActiveAndPendingPrimaryCount;
      }
    }
  }
}

intptr_t TTerminalQueueStatus::GetActiveCount() const
{
  NeedStats();
  return FActiveCount;
}

intptr_t TTerminalQueueStatus::GetDoneAndActiveCount() const
{
  return GetDoneCount() + GetActiveCount();
}

intptr_t TTerminalQueueStatus::GetActivePrimaryCount() const
{
  NeedStats();
  return FActivePrimaryCount;
}

bool TTerminalQueueStatus::IsOnlyOneActiveAndNoPending() const
{
  return (FActivePrimaryCount == 1) && (FActiveAndPendingPrimaryCount == 1);
}

intptr_t TTerminalQueueStatus::GetActiveAndPendingPrimaryCount() const
{
  NeedStats();
  return FActiveAndPendingPrimaryCount;
}

void TTerminalQueueStatus::Add(TQueueItemProxy * ItemProxy)
{
  ItemProxy->FQueueStatus = this;

  intptr_t Index = FList->GetCount();
  if (!ItemProxy->GetInfo()->Primary)
  {
    for (intptr_t I = 0; I < FList->GetCount(); I++)
    {
      if (GetItem(I)->GetInfo()->GroupToken == ItemProxy->GetInfo()->GroupToken)
      {
        Index = I + 1;
      }
    }

    DebugAssert(Index >= 0);
  }

  FList->Insert(Index, ItemProxy);
  ResetStats();
}

void TTerminalQueueStatus::Delete(TQueueItemProxy * ItemProxy)
{
  FList->Extract(ItemProxy);
  ItemProxy->FQueueStatus = nullptr;
  ResetStats();
}

intptr_t TTerminalQueueStatus::GetCount() const
{
  return FList->GetCount();
}

TQueueItemProxy * TTerminalQueueStatus::GetItem(intptr_t Index) const
{
  return const_cast<TTerminalQueueStatus *>(this)->GetItem(Index);
}

TQueueItemProxy * TTerminalQueueStatus::GetItem(intptr_t Index)
{
  return FList->GetAs<TQueueItemProxy>(Index);
}

TQueueItemProxy * TTerminalQueueStatus::FindByQueueItem(
  TQueueItem * QueueItem)
{
  for (intptr_t Index = 0; Index < FList->GetCount(); ++Index)
  {
    TQueueItemProxy * Item = GetItem(Index);
    DebugAssert(Item);
    if (Item->FQueueItem == QueueItem)
    {
      return Item;
    }
  }
  return nullptr;
}

// TLocatedQueueItem

TLocatedQueueItem::TLocatedQueueItem(TObjectClassId Kind, TTerminal * Terminal) :
  TQueueItem(Kind)
{
  DebugAssert(Terminal != nullptr);
  FCurrentDir = Terminal->RemoteGetCurrentDirectory();
}

TLocatedQueueItem::TLocatedQueueItem(const TLocatedQueueItem & Source) :
  TQueueItem(OBJECT_CLASS_TLocatedQueueItem)
{
  FCurrentDir = Source.FCurrentDir;
}

UnicodeString TLocatedQueueItem::GetStartupDirectory() const
{
  return FCurrentDir;
}

void TLocatedQueueItem::DoExecute(TTerminal * Terminal)
{
  DebugAssert(Terminal != nullptr);
  if (Terminal)
    Terminal->TerminalSetCurrentDirectory(FCurrentDir);
}

// TTransferQueueItem

TTransferQueueItem::TTransferQueueItem(TObjectClassId Kind, TTerminal * Terminal,
  const TStrings * AFilesToCopy, UnicodeString TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, TOperationSide Side,
  bool SingleFile, bool Parallel) :
  TLocatedQueueItem(Kind, Terminal),
  FFilesToCopy(new TStringList()),
  FCopyParam(nullptr),
  FParams(0),
  FParallel(false),
  FLastParallelOperationAdded(0),
  FParallelOperation(nullptr)
{
  FInfo->Operation = (Params & cpDelete) ? foMove : foCopy;
  FInfo->Side = Side;
  FInfo->SingleFile = SingleFile;

  DebugAssert(AFilesToCopy != nullptr);
  if (!AFilesToCopy)
    return;
  FFilesToCopy = new TStringList();
  for (intptr_t Index = 0; Index < AFilesToCopy->GetCount(); ++Index)
  {
    FFilesToCopy->AddObject(AFilesToCopy->GetString(Index),
      ((AFilesToCopy->GetObj(Index) == nullptr) || (Side == osLocal)) ? nullptr :
        AFilesToCopy->GetAs<TRemoteFile>(Index)->Duplicate());
  }

  FTargetDir = TargetDir;

  DebugAssert(CopyParam != nullptr);
  if (CopyParam)
  {
    FCopyParam = new TCopyParamType(*CopyParam);

    FParams = Params;

    FParallel = Parallel;
    FLastParallelOperationAdded = GetTickCount();
  }
}

TTransferQueueItem::~TTransferQueueItem()
{
  for (intptr_t Index = 0; Index < FFilesToCopy->GetCount(); ++Index)
  {
    TObject * Object = FFilesToCopy->GetObj(Index);
    SAFE_DESTROY(Object);
  }
  SAFE_DESTROY(FFilesToCopy);
  SAFE_DESTROY(FCopyParam);
}

intptr_t TTransferQueueItem::DefaultCPSLimit() const
{
  return FCopyParam->GetCPSLimit();
}

void TTransferQueueItem::DoExecute(TTerminal * Terminal)
{
  TLocatedQueueItem::DoExecute(Terminal);

  DebugAssert(Terminal != nullptr);
  TParallelOperation ParallelOperation(FInfo->Side);
  FParallelOperation = &ParallelOperation;
  try__finally
  {
    SCOPE_EXIT
    {
      FParallelOperation = nullptr;
    };
    DoTransferExecute(Terminal, FParallelOperation);
  }
  __finally
  {
#if 0
    FParallelOperation = nullptr;
#endif // #if 0
  };
}

void TTransferQueueItem::ProgressUpdated()
{
  TLocatedQueueItem::ProgressUpdated();

  if (FParallel)
  {
    bool Add = false;
    bool Force = false;
    DWORD LastParallelOperationAddedPrev = 0;

    {
      TGuard Guard(FSection);
      DebugAssert(FParallelOperation != nullptr);
      // Won't be initialized, if the operation is not eligible for parallel transfers (like cpDelete).
      // We can probably move the check outside of the guard.
      if (FParallelOperation && FParallelOperation->IsInitialized())
      {
        DebugAssert((FProgressData->GetOperation() == foCopy) || (FProgressData->GetOperation() == foCalculateSize));
        if (FProgressData->GetOperation() == foCopy)
        {
          Add = FParallelOperation->ShouldAddClient();
          if (Add)
          {
            DWORD Now = GetTickCount();
            Force =
              (Now - FLastParallelOperationAdded >= 5 * 1000) &&
              (TimeToSeconds(FProgressData->TotalTimeLeft()) >= FQueue->GetParallelDurationThreshold());
            LastParallelOperationAddedPrev = FLastParallelOperationAdded;
            // update now already to prevent race condition, but we will have to rollback it back,
            // if we actually do not add the parallel operation
            FLastParallelOperationAdded = Now;
          }
        }
      }
    }

    if (Add)
    {
      if (!FQueue->TryAddParallelOperation(this, Force))
      {
        TGuard Guard(FSection);
        FLastParallelOperationAdded = LastParallelOperationAddedPrev;
      }
    }
  }
}

TQueueItem * TTransferQueueItem::CreateParallelOperation()
{
  DebugAssert(FParallelOperation != nullptr);

  FParallelOperation->AddClient();
  return new TParallelTransferQueueItem(this, FParallelOperation);
}

// TUploadQueueItem

TUploadQueueItem::TUploadQueueItem(TTerminal * Terminal,
  const TStrings * AFilesToCopy, UnicodeString TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile, bool Parallel) :
  TTransferQueueItem(OBJECT_CLASS_TUploadQueueItem, Terminal, AFilesToCopy, TargetDir, CopyParam, Params, osLocal, SingleFile, Parallel)
{
  if (AFilesToCopy->GetCount() > 1)
  {
    if (FLAGSET(Params, cpTemporary))
    {
      FInfo->Source.Clear();
      FInfo->ModifiedLocal.Clear();
    }
    else
    {
      base::ExtractCommonPath(AFilesToCopy, FInfo->Source);
      // this way the trailing backslash is preserved for root directories like "D:\\"
      FInfo->Source = ::ExtractFileDir(::IncludeTrailingBackslash(FInfo->Source));
      FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        ::IncludeTrailingBackslash(FInfo->Source);
    }
  }
  else
  {
    if (FLAGSET(Params, cpTemporary))
    {
      FInfo->Source = base::ExtractFileName(AFilesToCopy->GetString(0), true);
      FInfo->ModifiedLocal.Clear();
    }
    else
    {
      DebugAssert(AFilesToCopy->GetCount() > 0);
      FInfo->Source = AFilesToCopy->GetString(0);
      FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        ::IncludeTrailingBackslash(::ExtractFilePath(FInfo->Source));
    }
  }

  FInfo->Destination =
    base::UnixIncludeTrailingBackslash(TargetDir) + CopyParam->GetFileMask();
  FInfo->ModifiedRemote = base::UnixIncludeTrailingBackslash(TargetDir);
}

void TUploadQueueItem::DoTransferExecute(TTerminal * Terminal, TParallelOperation * ParallelOperation)
{
  Terminal->CopyToRemote(FFilesToCopy, FTargetDir, FCopyParam, FParams, ParallelOperation);
}

// TParallelTransferQueueItem

TParallelTransferQueueItem::TParallelTransferQueueItem(
    const TLocatedQueueItem * ParentItem, TParallelOperation * ParallelOperation) :
  TLocatedQueueItem(*ParentItem),
  FParallelOperation(ParallelOperation)
{
  // deliberately not copying the ModifiedLocal and ModifiedRemote, not to trigger panel refresh, when sub-item completes
  FInfo->Operation = ParentItem->FInfo->Operation;
  FInfo->Side = ParentItem->FInfo->Side;
  FInfo->Source = ParentItem->FInfo->Source;
  FInfo->Destination = ParentItem->FInfo->Destination;
  FInfo->SingleFile = DebugAlwaysFalse(ParentItem->FInfo->SingleFile);
  FInfo->Primary = false;
  FInfo->GroupToken = ParentItem->FInfo->GroupToken;
}

void TParallelTransferQueueItem::DoExecute(TTerminal * Terminal)
{
  TLocatedQueueItem::DoExecute(Terminal);

  DebugAssert(Terminal != nullptr);
  if (!Terminal)
    return;

  Terminal->LogParallelTransfer(FParallelOperation);
  TFileOperationProgressType OperationProgress(Terminal->GetOnProgress(), Terminal->GetOnFinished(), FParallelOperation->GetMainOperationProgress());
  TFileOperation Operation = (FLAGSET(FParallelOperation->GetParams(), cpDelete) ? foMove : foCopy);
  bool Temp = FLAGSET(FParallelOperation->GetParams(), cpTemporary);

  OperationProgress.Start(
    // CPS limit inherited from parent OperationProgress.
    // Count not known and won't be needed as we will always have TotalSize as  we always transfer a single file at a time.
    Operation, FParallelOperation->GetSide(), -1, Temp, FParallelOperation->GetTargetDir(), 0);

  try__finally
  {
    SCOPE_EXIT
    {
      OperationProgress.Stop();
      FParallelOperation->RemoveClient();
    };
    bool Continue = true;
    do
    {
      intptr_t GotNext = Terminal->CopyToParallel(FParallelOperation, &OperationProgress);
      if (GotNext < 0)
      {
        Continue = false;
      }
      else if (!FQueue->ContinueParallelOperation())
      {
        Continue = false;
      }
    }
    while (Continue);
  }
  __finally
  {
#if 0
    OperationProgress.Stop();
    FParallelOperation->RemoveClient();
#endif // #if 0
  };
}

// TDownloadQueueItem

TDownloadQueueItem::TDownloadQueueItem(TTerminal * Terminal,
  const TStrings * AFilesToCopy, UnicodeString TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile, bool Parallel) :
  TTransferQueueItem(OBJECT_CLASS_TDownloadQueueItem, Terminal, AFilesToCopy, TargetDir, CopyParam, Params, osRemote, SingleFile, Parallel)
{
  if (AFilesToCopy->GetCount() > 1)
  {
    if (!base::UnixExtractCommonPath(AFilesToCopy, FInfo->Source))
    {
      FInfo->Source = Terminal->RemoteGetCurrentDirectory();
    }
    FInfo->Source = base::UnixExcludeTrailingBackslash(FInfo->Source);
    FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
      base::UnixIncludeTrailingBackslash(FInfo->Source);
  }
  else
  {
    DebugAssert(AFilesToCopy->GetCount() > 0);
    FInfo->Source = AFilesToCopy->GetString(0);
    if (base::UnixExtractFilePath(FInfo->Source).IsEmpty())
    {
      FInfo->Source = base::UnixIncludeTrailingBackslash(Terminal->RemoteGetCurrentDirectory()) +
        FInfo->Source;
      FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        base::UnixIncludeTrailingBackslash(Terminal->RemoteGetCurrentDirectory());
    }
    else
    {
      FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        base::UnixExtractFilePath(FInfo->Source);
    }
  }

  if (FLAGSET(Params, cpTemporary))
  {
    FInfo->Destination.Clear();
  }
  else
  {
    FInfo->Destination =
      ::IncludeTrailingBackslash(TargetDir) + CopyParam->GetFileMask();
  }
  FInfo->ModifiedLocal = ::IncludeTrailingBackslash(TargetDir);
}

void TDownloadQueueItem::DoTransferExecute(TTerminal * Terminal, TParallelOperation * ParallelOperation)
{
  DebugAssert(Terminal != nullptr);
  Terminal->CopyToLocal(FFilesToCopy, FTargetDir, FCopyParam, FParams, ParallelOperation);
}

// TTerminalThread

TTerminalThread::TTerminalThread(TTerminal * Terminal) :
  TSignalThread(OBJECT_CLASS_TTerminalThread, false),
  FTerminal(Terminal),
  FOnInformation(nullptr),
  FOnQueryUser(nullptr),
  FOnPromptUser(nullptr),
  FOnShowExtendedException(nullptr),
  FOnDisplayBanner(nullptr),
  FOnChangeDirectory(nullptr),
  FOnReadDirectory(nullptr),
  FOnStartReadDirectory(nullptr),
  FOnReadDirectoryProgress(nullptr),
  FOnInitializeLog(nullptr),
  FOnIdle(nullptr),
  FAction(nullptr)
{
  FAction = nullptr;
  FActionEvent = ::CreateEvent(nullptr, false, false, nullptr);
  FException = nullptr;
  FIdleException = nullptr;
  FOnIdle = nullptr;
  FUserAction = nullptr;
  FCancel = false;
  FCancelled = false;
  FPendingIdle = false;
  FMainThread = GetCurrentThreadId();
}

void TTerminalThread::InitTerminalThread()
{
  TSignalThread::InitSignalThread(false);

  FOnInformation = FTerminal->GetOnInformation();
  FOnQueryUser = FTerminal->GetOnQueryUser();
  FOnPromptUser = FTerminal->GetOnPromptUser();
  FOnShowExtendedException = FTerminal->GetOnShowExtendedException();
  FOnDisplayBanner = FTerminal->GetOnDisplayBanner();
  FOnChangeDirectory = FTerminal->GetOnChangeDirectory();
  FOnReadDirectory = FTerminal->GetOnReadDirectory();
  FOnStartReadDirectory = FTerminal->GetOnStartReadDirectory();
  FOnReadDirectoryProgress = FTerminal->GetOnReadDirectoryProgress();
  FOnInitializeLog = FTerminal->GetOnInitializeLog();

  FTerminal->SetOnInformation(nb::bind(&TTerminalThread::TerminalInformation, this));
  FTerminal->SetOnQueryUser(nb::bind(&TTerminalThread::TerminalQueryUser, this));
  FTerminal->SetOnPromptUser(nb::bind(&TTerminalThread::TerminalPromptUser, this));
  FTerminal->SetOnShowExtendedException(nb::bind(&TTerminalThread::TerminalShowExtendedException, this));
  FTerminal->SetOnDisplayBanner(nb::bind(&TTerminalThread::TerminalDisplayBanner, this));
  FTerminal->SetOnChangeDirectory(nb::bind(&TTerminalThread::TerminalChangeDirectory, this));
  FTerminal->SetOnReadDirectory(nb::bind(&TTerminalThread::TerminalReadDirectory, this));
  FTerminal->SetOnStartReadDirectory(nb::bind(&TTerminalThread::TerminalStartReadDirectory, this));
  FTerminal->SetOnReadDirectoryProgress(nb::bind(&TTerminalThread::TerminalReadDirectoryProgress, this));
  FTerminal->SetOnInitializeLog(nb::bind(&TTerminalThread::TerminalInitializeLog, this));

  Start();
}

TTerminalThread::~TTerminalThread()
{
  Close();

  SAFE_CLOSE_HANDLE(FActionEvent);

  DebugAssert(FTerminal->GetOnInformation() == nb::bind(&TTerminalThread::TerminalInformation, this));
  DebugAssert(FTerminal->GetOnQueryUser() == nb::bind(&TTerminalThread::TerminalQueryUser, this));
  DebugAssert(FTerminal->GetOnPromptUser() == nb::bind(&TTerminalThread::TerminalPromptUser, this));
  DebugAssert(FTerminal->GetOnShowExtendedException() == nb::bind(&TTerminalThread::TerminalShowExtendedException, this));
  DebugAssert(FTerminal->GetOnDisplayBanner() == nb::bind(&TTerminalThread::TerminalDisplayBanner, this));
  DebugAssert(FTerminal->GetOnChangeDirectory() == nb::bind(&TTerminalThread::TerminalChangeDirectory, this));
  DebugAssert(FTerminal->GetOnReadDirectory() == nb::bind(&TTerminalThread::TerminalReadDirectory, this));
  DebugAssert(FTerminal->GetOnStartReadDirectory() == nb::bind(&TTerminalThread::TerminalStartReadDirectory, this));
  DebugAssert(FTerminal->GetOnReadDirectoryProgress() == nb::bind(&TTerminalThread::TerminalReadDirectoryProgress, this));
  DebugAssert(FTerminal->GetOnInitializeLog() == nb::bind(&TTerminalThread::TerminalInitializeLog, this));

  FTerminal->SetOnInformation(FOnInformation);
  FTerminal->SetOnQueryUser(FOnQueryUser);
  FTerminal->SetOnPromptUser(FOnPromptUser);
  FTerminal->SetOnShowExtendedException(FOnShowExtendedException);
  FTerminal->SetOnDisplayBanner(FOnDisplayBanner);
  FTerminal->SetOnChangeDirectory(FOnChangeDirectory);
  FTerminal->SetOnReadDirectory(FOnReadDirectory);
  FTerminal->SetOnStartReadDirectory(FOnStartReadDirectory);
  FTerminal->SetOnReadDirectoryProgress(FOnReadDirectoryProgress);
  FTerminal->SetOnInitializeLog(FOnInitializeLog);
}

void TTerminalThread::Cancel()
{
  FCancel = true;
}

void TTerminalThread::Idle()
{
  TGuard Guard(FSection);
  // only when running user action already,
  // so that the exception is caught, saved and actually
  // passed back into the terminal thread, saved again
  // and passed back to us
  if ((FUserAction != nullptr) && (FIdleException != nullptr))
  {
    Rethrow(FIdleException);
  }
  FPendingIdle = true;
}

void TTerminalThread::TerminalOpen()
{
  RunAction(nb::bind(&TTerminalThread::TerminalOpenEvent, this));
}

void TTerminalThread::TerminalReopen()
{
  RunAction(nb::bind(&TTerminalThread::TerminalReopenEvent, this));
}

void TTerminalThread::RunAction(TNotifyEvent Action)
{
  DebugAssert(FAction == nullptr);
  DebugAssert(FException == nullptr);
  DebugAssert(FIdleException == nullptr);
  DebugAssert(FOnIdle != nullptr);

  FCancelled = false;
  FAction = Action;
  try
  {
    try__finally
    {
      SCOPE_EXIT
      {
        FAction = nullptr;
        SAFE_DESTROY_EX(Exception, FException);
      };
      TriggerEvent();

      bool Done = false;
      const DWORD MaxWait = 50;
      DWORD Wait = MaxWait;

      do
      {
        switch (WaitForSingleObject(FActionEvent, Wait))
        {
        case WAIT_OBJECT_0:
          Done = true;
          break;

        case WAIT_TIMEOUT:
          {
            if (FUserAction != nullptr)
            {
              try
              {
                FUserAction->Execute(nullptr);
              }
              catch (Exception & E)
              {
                SaveException(E, FException);
              }

              FUserAction = nullptr;
              TriggerEvent();
              Wait = 0;
            }
            else
            {
              if (FOnIdle != nullptr)
              {
                FOnIdle(nullptr);
              }
              Wait = Min(Wait + 10, MaxWait);
            }
          }
          break;

        default:
          throw Exception(L"Error waiting for background session task to complete");
        }
      }
      while (!Done);


      Rethrow(FException);
    }
    __finally
    {
#if 0
      FAction = nullptr;
      SAFE_DESTROY_EX(Exception, FException);
#endif // #if 0
    };
  }
  catch (...)
  {
    if (FCancelled)
    {
      // even if the abort thrown as result of Cancel() was wrapped into
      // some higher-level exception, normalize back to message-less fatal
      // exception here
      FatalAbort();
    }
    else
    {
      throw;
    }
  }
}

void TTerminalThread::TerminalOpenEvent(TObject * /*Sender*/)
{
  FTerminal->Open();
}

void TTerminalThread::TerminalReopenEvent(TObject * /*Sender*/)
{
  FTerminal->Reopen(0);
}

void TTerminalThread::ProcessEvent()
{
  DebugAssert(FEvent != nullptr);
  DebugAssert(FException == nullptr);

  try
  {
    FAction(nullptr);
  }
  catch (Exception & E)
  {
    SaveException(E, FException);
  }

  ::SetEvent(FActionEvent);
}

void TTerminalThread::Rethrow(Exception *& AException)
{
  if (AException != nullptr)
  {
    try__finally
    {
      SCOPE_EXIT
      {
        SAFE_DESTROY_EX(Exception, AException);
      };
      RethrowException(AException);
    }
    __finally
    {
#if 0
      SAFE_DESTROY_EX(Exception, AException);
#endif // #if 0
    };
  }
}

void TTerminalThread::SaveException(Exception & E, Exception *& Exception)
{
  DebugAssert(Exception == nullptr);

  Exception = CloneException(&E);
}

void TTerminalThread::FatalAbort()
{
  FTerminal->FatalAbort();
}

void TTerminalThread::CheckCancel()
{
  if (FCancel && !FCancelled)
  {
    FCancelled = true;
    FatalAbort();
  }
}

void TTerminalThread::WaitForUserAction(TUserAction * UserAction)
{
  DWORD Thread = GetCurrentThreadId();
  // we can get called from the main thread from within Idle,
  // should be only to call HandleExtendedException
  if (Thread == FMainThread)
  {
    if (UserAction != nullptr)
    {
      UserAction->Execute(nullptr);
    }
  }
  else
  {
    // we should be called from our thread only,
    // with exception noted above
    DebugAssert(Thread == FThreadId);

    bool DoCheckCancel =
      DebugAlwaysFalse(UserAction == nullptr) || !UserAction->Force();
    if (DoCheckCancel)
    {
      CheckCancel();
    }

    // have to save it as we can go recursive via TQueryParams::TimerEvent,
    // see TTerminalThread::TerminalQueryUser
    TUserAction * PrevUserAction = FUserAction;
    try__finally
    {
      SCOPE_EXIT
      {
        FUserAction = PrevUserAction;
        SAFE_DESTROY_EX(Exception, FException);
      };
      FUserAction = UserAction;

      while (true)
      {
        {
          TGuard Guard(FSection);
          // If idle exception is already set, we are only waiting
          // for the main thread to pick it up
          // (or at least to finish handling the user action, so
          // that we rethrow the idle exception below)
          // Also if idle exception is set, it is probable that terminal
          // is not active anyway.
          if (FTerminal->GetActive() && FPendingIdle && (FIdleException == nullptr))
          {
            FPendingIdle = false;
            try
            {
              FTerminal->Idle();
            }
            catch (Exception & E)
            {
              SaveException(E, FIdleException);
            }
          }
        }

        intptr_t WaitResult = static_cast<intptr_t>(WaitForEvent(1000));
        if (WaitResult == 0)
        {
          SAFE_DESTROY_EX(Exception, FIdleException);
          FatalAbort();
        }
        else if (WaitResult > 0)
        {
          break;
        }
      }


      Rethrow(FException);

      if (FIdleException != nullptr)
      {
        // idle exception was not used to cancel the user action
        // (if it where it would be already cloned into the FException above
        // and rethrown)
        Rethrow(FIdleException);
      }
    }
    __finally
    {
#if 0
      FUserAction = PrevUserAction;
      SAFE_DESTROY_EX(Exception, FException);
#endif // #if 0
    };

    // Contrary to a call before, this is unconditional,
    // otherwise cancelling authentication won't work,
    // if it is tried only after the last user action
    // (what is common, when cancelling while waiting for
    // resolving of unresolvable host name, where the last user action is
    // "resolving hostname" information action)
    CheckCancel();
  }
}

void TTerminalThread::TerminalInformation(
  TTerminal * Terminal, UnicodeString Str, bool Status, intptr_t Phase)
{
  TInformationUserAction Action(FOnInformation);
  Action.Terminal = Terminal;
  Action.Str = Str;
  Action.Status = Status;
  Action.Phase = Phase;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalQueryUser(TObject * Sender,
  UnicodeString AQuery, TStrings * MoreMessages, uintptr_t Answers,
  const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg)
{
  DebugUsedParam(Arg);
  DebugAssert(Arg == nullptr);

  // note about TQueryParams::TimerEvent
  // So far there is only one use for this, the TSecureShell::SendBuffer,
  // which should be thread-safe, as long as the terminal thread,
  // is stopped waiting for OnQueryUser to finish.

  // note about TQueryButtonAlias::OnClick
  // So far there is only one use for this, the TClipboardHandler,
  // which is thread-safe.

  TQueryUserAction Action(FOnQueryUser);
  Action.Sender = Sender;
  Action.Query = AQuery;
  Action.MoreMessages = MoreMessages;
  Action.Answers = Answers;
  Action.Params = Params;
  Action.Answer = Answer;
  Action.Type = Type;

  WaitForUserAction(&Action);

  Answer = Action.Answer;
}

void TTerminalThread::TerminalInitializeLog(TObject * Sender)
{
  if (FOnInitializeLog != nullptr)
  {
    // never used, so not tested either
    DebugFail();
    TNotifyAction Action(FOnInitializeLog);
    Action.Sender = Sender;

    WaitForUserAction(&Action);
  }
}

void TTerminalThread::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
  TStrings * Results, bool & Result, void * Arg)
{
  DebugUsedParam(Arg);
  DebugAssert(Arg == nullptr);

  TPromptUserAction Action(FOnPromptUser);
  Action.Terminal = Terminal;
  Action.Kind = Kind;
  Action.Name = Name;
  Action.Instructions = Instructions;
  Action.Prompts = Prompts;
  Action.Results->AddStrings(Results);

  WaitForUserAction(&Action);

  Results->Clear();
  Results->AddStrings(Action.Results);
  Result = Action.Result;
}

void TTerminalThread::TerminalShowExtendedException(
  TTerminal * Terminal, Exception * E, void * Arg)
{
  DebugUsedParam(Arg);
  DebugAssert(Arg == nullptr);

  TShowExtendedExceptionAction Action(FOnShowExtendedException);
  Action.Terminal = Terminal;
  Action.E = E;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalDisplayBanner(TTerminal * Terminal,
  UnicodeString SessionName, UnicodeString Banner,
  bool & NeverShowAgain, intptr_t Options)
{
  TDisplayBannerAction Action(FOnDisplayBanner);
  Action.Terminal = Terminal;
  Action.SessionName = SessionName;
  Action.Banner = Banner;
  Action.NeverShowAgain = NeverShowAgain;
  Action.Options = Options;

  WaitForUserAction(&Action);

  NeverShowAgain = Action.NeverShowAgain;
}

void TTerminalThread::TerminalChangeDirectory(TObject * Sender)
{
  TNotifyAction Action(FOnChangeDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly)
{
  TReadDirectoryAction Action(FOnReadDirectory);
  Action.Sender = Sender;
  Action.ReloadOnly = ReloadOnly;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalStartReadDirectory(TObject * Sender)
{
  TNotifyAction Action(FOnStartReadDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalReadDirectoryProgress(
  TObject * Sender, intptr_t Progress, intptr_t ResolvedLinks, bool & Cancel)
{
  TReadDirectoryProgressAction Action(FOnReadDirectoryProgress);
  Action.Sender = Sender;
  Action.Progress = Progress;
  Action.ResolvedLinks = ResolvedLinks;
  Action.Cancel = Cancel;

  WaitForUserAction(&Action);

  Cancel = Action.Cancel;
}

