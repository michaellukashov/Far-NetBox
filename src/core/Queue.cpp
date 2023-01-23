
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include <System.DateUtils.hpp>

#include "Queue.h"

__removed #pragma package(smart_init)

class TBackgroundTerminal;

NB_DEFINE_CLASS_ID(TParallelTransferQueueItem);
class TParallelTransferQueueItem : public TLocatedQueueItem
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TParallelTransferQueueItem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TParallelTransferQueueItem) || TLocatedQueueItem::is(Kind); }
public:
  explicit TParallelTransferQueueItem(const TLocatedQueueItem *ParentItem, TParallelOperation *ParallelOperation) noexcept;

protected:
  virtual void DoExecute(TTerminal *Terminal) override;

private:
  TParallelOperation *FParallelOperation{nullptr};
};

class TUserAction : public TObject
{
  NB_DISABLE_COPY(TUserAction)
public:
  explicit TUserAction() = default;
  virtual ~TUserAction() = default;

  virtual void Execute(void *Arg) = 0;
  virtual bool Force() const { return false; }
};

class TNotifyAction : public TUserAction
{
  NB_DISABLE_COPY(TNotifyAction)
public:
  explicit TNotifyAction(TNotifyEvent AOnNotify) : TUserAction(),
    OnNotify(AOnNotify)
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
  TObject *Sender{nullptr};
};

class TInformationUserAction : public TUserAction
{
  NB_DISABLE_COPY(TInformationUserAction)
public:
  explicit TInformationUserAction(TInformationEvent AOnInformation) :
    OnInformation(AOnInformation)
  {
  }

  virtual void Execute(void * /*Arg*/) override
  {
    if (OnInformation != nullptr)
    {
      OnInformation(Terminal, Str, Status, Phase, Additional);
    }
  }

  virtual bool Force() const override
  {
    // we need to propagate mainly the end-phase event even, when user cancels
    // the connection, so that authentication window is closed
    return TUserAction::Force() || (Phase >= 0);
  }

  TInformationEvent OnInformation;
  TTerminal *Terminal{nullptr};
  UnicodeString Str;
  bool Status{false};
  int32_t Phase{0};
  UnicodeString Additional;
};

class TQueryUserAction : public TUserAction
{
  NB_DISABLE_COPY(TQueryUserAction)
public:
  explicit TQueryUserAction(TQueryUserEvent AOnQueryUser) :
    OnQueryUser(AOnQueryUser)
  {
  }

  virtual void Execute(void *Arg) override
  {
    if (OnQueryUser != nullptr)
    {
      OnQueryUser(Sender, Query, MoreMessages, Answers, Params, Answer, Type, Arg);
    }
  }

  TQueryUserEvent OnQueryUser;
  TObject *Sender{nullptr};
  UnicodeString Query;
  TStrings *MoreMessages{nullptr};
  uint32_t Answers{0};
  const TQueryParams *Params{nullptr};
  uint32_t Answer{0};
  TQueryType Type{qtConfirmation};
};

class TPromptUserAction : public TUserAction
{
  NB_DISABLE_COPY(TPromptUserAction)
public:
  explicit TPromptUserAction(TPromptUserEvent AOnPromptUser) noexcept :
    OnPromptUser(AOnPromptUser)
  {
  }

  virtual ~TPromptUserAction() = default;

  void Execute(void *Arg) override
  {
    if (OnPromptUser != nullptr)
    {
      OnPromptUser(Terminal, Kind, Name, Instructions, Prompts, Results, Result, Arg);
    }
  }

  TPromptUserEvent OnPromptUser;
  TTerminal *Terminal{nullptr};
  TPromptKind Kind{pkPrompt};
  UnicodeString Name;
  UnicodeString Instructions;
  TStrings* Prompts{nullptr};
  TStrings* Results{nullptr};
  bool Result{false};
};

class TShowExtendedExceptionAction : public TUserAction
{
  NB_DISABLE_COPY(TShowExtendedExceptionAction)
public:
  explicit TShowExtendedExceptionAction(TExtendedExceptionEvent AOnShowExtendedException) :
    OnShowExtendedException(AOnShowExtendedException)
  {
  }

  void Execute(void *Arg) override
  {
    if (OnShowExtendedException != nullptr)
    {
      OnShowExtendedException(Terminal, E, Arg);
    }
  }

  TExtendedExceptionEvent OnShowExtendedException;
  TTerminal *Terminal{nullptr};
  Exception *E{nullptr};
};

class TDisplayBannerAction : public TUserAction
{
  NB_DISABLE_COPY(TDisplayBannerAction)
public:
  explicit TDisplayBannerAction(TDisplayBannerEvent AOnDisplayBanner) :
    OnDisplayBanner(AOnDisplayBanner)
  {
  }

  void Execute(void * /*Arg*/) override
  {
    if (OnDisplayBanner != nullptr)
    {
      OnDisplayBanner(Terminal, SessionName, Banner, NeverShowAgain, Options, Params);
    }
  }

  TDisplayBannerEvent OnDisplayBanner;
  TTerminal *Terminal{nullptr};
  UnicodeString SessionName;
  UnicodeString Banner;
  bool NeverShowAgain{false};
  int32_t Options{0};
  uint32_t Params{0};
};

class TReadDirectoryAction : public TUserAction
{
  NB_DISABLE_COPY(TReadDirectoryAction)
public:
  explicit TReadDirectoryAction(TReadDirectoryEvent AOnReadDirectory) :
    OnReadDirectory(AOnReadDirectory)
  {
  }

  void Execute(void * /*Arg*/) override
  {
    if (OnReadDirectory != nullptr)
    {
      OnReadDirectory(Sender, ReloadOnly);
    }
  }

  TReadDirectoryEvent OnReadDirectory;
  TObject *Sender{nullptr};
  bool ReloadOnly{false};
};

class TReadDirectoryProgressAction : public TUserAction
{
  NB_DISABLE_COPY(TReadDirectoryProgressAction)
public:
  explicit TReadDirectoryProgressAction(TReadDirectoryProgressEvent AOnReadDirectoryProgress) :
    OnReadDirectoryProgress(AOnReadDirectoryProgress)
  {
  }

  void Execute(void * /*Arg*/) override
  {
    if (OnReadDirectoryProgress != nullptr)
    {
      OnReadDirectoryProgress(Sender, Progress, ResolvedLinks, Cancel);
    }
  }

  TReadDirectoryProgressEvent OnReadDirectoryProgress;
  TObject *Sender{nullptr};
  int32_t Progress{0};
  int32_t ResolvedLinks{0};
  bool Cancel{false};
};

NB_DEFINE_CLASS_ID(TTerminalItem);
class TTerminalItem : public TSignalThread
{
  friend class TQueueItem;
  friend class TBackgroundTerminal;
  NB_DISABLE_COPY(TTerminalItem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTerminalItem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTerminalItem) || TSignalThread::is(Kind); }
public:
  TTerminalItem() = delete;
  explicit TTerminalItem(TTerminalQueue *Queue) noexcept;
  virtual ~TTerminalItem() noexcept;
  void InitTerminalItem(int32_t Index);

  void Process(TQueueItem *Item);
  bool ProcessUserAction(void *Arg);
  void Cancel();
  void Idle();
  bool Pause();
  bool Resume();

protected:
  TTerminalQueue *FQueue{nullptr};
  TBackgroundTerminal *FTerminal{nullptr};
  TQueueItem *FItem{nullptr};
  TCriticalSection FCriticalSection;
  TUserAction *FUserAction{nullptr};
  bool FCancel{false};
  bool FPause{false};

  void ProcessEvent() override;
  bool Finished() override;
  bool WaitForUserAction(TQueueItem::TStatus ItemStatus, TUserAction *UserAction);
  bool OverrideItemStatus(TQueueItem::TStatus &ItemStatus) const;

  void TerminalQueryUser(TObject *Sender,
    const UnicodeString AQuery, TStrings *MoreMessages, uint32_t Answers,
    const TQueryParams *Params, uint32_t &Answer, TQueryType Type, void *Arg);
  void TerminalPromptUser(TTerminal *Terminal, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions,
    TStrings *Prompts, TStrings *Results, bool &Result, void *Arg);
  void TerminalShowExtendedException(TTerminal *Terminal,
    Exception *E, void *Arg);
  void OperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString AFileName, bool Success,
    TOnceDoneOperation &OnceDoneOperation);
  void OperationProgress(TFileOperationProgressType &ProgressData);
};

// TSignalThread

int TSimpleThread::ThreadProc(void *Thread)
{
  TSimpleThread *SimpleThread = get_as<TSimpleThread>(Thread);
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
  if (SimpleThread->Finished())
  {
    delete SimpleThread;
  }
  return 0;
}

TSimpleThread::TSimpleThread(TObjectClassId Kind) noexcept :
  TObject(Kind)
{
}

void TSimpleThread::InitSimpleThread()
{
  FThread = StartThread(nullptr, 0, this, CREATE_SUSPENDED, FThreadId);
}

TSimpleThread::~TSimpleThread() noexcept
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

bool TSimpleThread::Finished()
{
  return false;
}

void TSimpleThread::Close()
{
  if (!FFinished)
  {
    Terminate();
    WaitFor();
  }
}

void TSimpleThread::WaitFor(uint32_t Milliseconds) const
{
  ::WaitForSingleObject(FThread, nb::ToDWord(Milliseconds));
}

// TSignalThread

TSignalThread::TSignalThread(TObjectClassId Kind) noexcept :
  TSimpleThread(Kind),
  FTerminated(true)
{
}

void TSignalThread::InitSignalThread(bool LowPriority, HANDLE Event)
{
  TSimpleThread::InitSimpleThread();
  if (Event == nullptr)
  {
    FEvent = ::CreateEvent(nullptr, false, false, nullptr);
  }
  else
  {
    FEvent = Event;
  }
  DebugAssert(FEvent != nullptr);

  if (LowPriority)
  {
    ::SetThreadPriority(FThread, THREAD_PRIORITY_BELOW_NORMAL);
  }
}

TSignalThread::~TSignalThread() noexcept
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

uint32_t TSignalThread::WaitForEvent(uint32_t Timeout) const
{
  uint32_t Res = ::WaitForSingleObject(FEvent, Timeout);
  uint32_t Result;
  if ((Res == WAIT_TIMEOUT) && !FTerminated)
  {
    Result = nb::ToUIntPtr(-1);
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

TTerminalQueue::TTerminalQueue(TTerminal *ATerminal,
  TConfiguration *AConfiguration) noexcept :
  TSignalThread(OBJECT_CLASS_TTerminalQueue),
  FTerminal(ATerminal),
  FConfiguration(AConfiguration),
  FSessionData(std::make_unique<TSessionData>(L"")),
  FItems(std::make_unique<TList>()),
  FDoneItems(std::make_unique<TList>()),
  FTerminals(std::make_unique<TList>()),
  FForcedItems(std::make_unique<TList>())
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

TTerminalQueue::~TTerminalQueue() noexcept
{
  Close();

  {
    TGuard Guard(FItemsSection); nb::used(Guard);

    while (FTerminals->GetCount() > 0)
    {
      TTerminalItem *TerminalItem = FTerminals->GetAs<TTerminalItem>(0);
      FTerminals->Delete(0);
      TerminalItem->Terminate();
      TerminalItem->WaitFor();
      SAFE_DESTROY(TerminalItem);
    }
//    SAFE_DESTROY(FTerminals);
//    SAFE_DESTROY(FForcedItems);

    FreeItemsList(FItems.get());
    FreeItemsList(FDoneItems.get());
  }

//  SAFE_DESTROY_EX(TSessionData, FSessionData);
}

void TTerminalQueue::FreeItemsList(TList *List) const
{
  for (int32_t Index = 0; Index < List->GetCount(); ++Index)
  {
    TQueueItem *Item = GetItem(List, Index);
    SAFE_DESTROY(Item);
  }
//  SAFE_DESTROY(List);
}

void TTerminalQueue::TerminalFinished(TTerminalItem *TerminalItem)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      int32_t Index = FTerminals->IndexOf(TerminalItem);
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

bool TTerminalQueue::TerminalFree(TTerminalItem *TerminalItem)
{
  bool Result = true;

  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      int32_t Index = FTerminals->IndexOf(TerminalItem);
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

int32_t TTerminalQueue::GetParallelDurationThreshold() const
{
  return FConfiguration->GetParallelDurationThreshold();
}

void TTerminalQueue::AddItem(TQueueItem *Item)
{
  DebugAssert(!FTerminated);

  Item->SetStatus(TQueueItem::qsPending);

  {
    TGuard Guard(FItemsSection); nb::used(Guard);

    FItems->Add(Item);
    Item->FQueue = this;
  }

  DoListUpdate();

  TriggerEvent();
}

void TTerminalQueue::RetryItem(TQueueItem *Item)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      int32_t Index = FItems->Remove(Item);
      DebugAssert(Index < FItemsInProcess);
      DebugUsedParam(Index);
      FItemsInProcess--;
      FItems->Add(Item);
    }

    DoListUpdate();

    TriggerEvent();
  }
}

void TTerminalQueue::DeleteItem(TQueueItem *Item, bool CanKeep)
{
  if (!FTerminated)
  {
    bool Empty;
    bool EmptyButMonitored;
    bool Monitored;
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      // does this need to be within guard?
      Monitored = (Item->GetCompleteEvent() != INVALID_HANDLE_VALUE);
      int32_t Index = FItems->Remove(Item);
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
        EmptyButMonitored = (GetItem(FItems.get(), Index)->GetCompleteEvent() != INVALID_HANDLE_VALUE);
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

TQueueItem *TTerminalQueue::GetItem(TList *List, int32_t Index)
{
  return List->GetAs<TQueueItem>(Index);
}

TQueueItem *TTerminalQueue::GetItem(int32_t Index) const
{
  return FItems->GetAs<TQueueItem>(Index);
}

void TTerminalQueue::UpdateStatusForList(
  TTerminalQueueStatus *Status, TList *List, TTerminalQueueStatus *Current)
{
  for (int32_t Index = 0; Index < List->GetCount(); ++Index)
  {
    TQueueItem *Item = GetItem(List, Index);
    TQueueItemProxy *ItemProxy = nullptr;
    if (Current != nullptr)
    {
      ItemProxy = Current->FindByQueueItem(Item);
    }

    if (ItemProxy != nullptr)
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

TTerminalQueueStatus *TTerminalQueue::CreateStatus(TTerminalQueueStatus *&Current)
{
  std::unique_ptr<TTerminalQueueStatus> Status(std::make_unique<TTerminalQueueStatus>());
  try__catch
  {
    try__finally
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      UpdateStatusForList(Status.get(), FDoneItems.get(), Current);
      Status->SetDoneCount(Status->GetCount());
      UpdateStatusForList(Status.get(), FItems.get(), Current);
    },
    __finally
    {
      if (Current != nullptr)
      {
        SAFE_DESTROY(Current);
      }
    } end_try__finally
  }
  catch__removed
  ({
    delete Status;
    throw;
  })
  return Status.release();
}

bool TTerminalQueue::ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy, TQueueFileList * FileList)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection); nb::used(Guard);

    Result = (FDoneItems->IndexOf(Item) >= 0) || (FItems->IndexOf(Item) >= 0);
    if (Result)
    {
      if (FileList != nullptr)
      {
        Result = Item->UpdateFileList(FileList);
      }
      else
      {
        Item->GetData(Proxy);
      }
    }
  }

  return Result;
}

bool TTerminalQueue::ItemProcessUserAction(TQueueItem *Item, void *Arg)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TTerminalItem *TerminalItem = nullptr;

    {
      TGuard Guard(FItemsSection); nb::used(Guard);

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

bool TTerminalQueue::ItemMove(TQueueItem *Item, TQueueItem *BeforeItem)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      int32_t Index = FItems->IndexOf(Item);
      int32_t IndexDest = FItems->IndexOf(BeforeItem);
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

bool TTerminalQueue::ItemExecuteNow(TQueueItem *Item)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      int32_t Index = FItems->IndexOf(Item);
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

bool TTerminalQueue::ItemDelete(TQueueItem *Item)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    bool UpdateList = false;

    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      int32_t Index = FItems->IndexOf(Item);
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
          SAFE_DESTROY(Item);
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

bool TTerminalQueue::ItemPause(TQueueItem *Item, bool Pause)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TTerminalItem *TerminalItem = nullptr;

    {
      TGuard Guard(FItemsSection); nb::used(Guard);

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

bool TTerminalQueue::ItemSetCPSLimit(TQueueItem *Item, int32_t CPSLimit) const
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection); nb::used(Guard);

    Result = (FItems->IndexOf(Item) >= 0);
    if (Result)
    {
      Item->SetCPSLimit(CPSLimit);
    }
  }

  return Result;
}

bool TTerminalQueue::ItemGetCPSLimit(TQueueItem *Item, int32_t &CPSLimit) const
{
  CPSLimit = 0;
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection); nb::used(Guard);

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
    TTerminalItem *TerminalItem = nullptr;

    if (FFreeTerminals > 0)
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

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
  TTerminalItem *TerminalItem;
  do
  {
    TerminalItem = nullptr;
    TQueueItem *Item1 = nullptr;

    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      // =0  do not keep
      // <0  infinity
      if (FKeepDoneItemsFor >= 0)
      {
        TDateTime RemoveDoneItemsBefore = Now();
        if (FKeepDoneItemsFor > 0)
        {
          RemoveDoneItemsBefore = ::IncSecond(RemoveDoneItemsBefore, -FKeepDoneItemsFor);
        }
        for (int32_t Index = 0; Index < FDoneItems->GetCount(); ++Index)
        {
          TQueueItem *Item2 = GetItem(FDoneItems.get(), Index);
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
        int32_t ForcedIndex = FForcedItems->IndexOf(Item1);

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

void TTerminalQueue::DoQueueItemUpdate(TQueueItem *Item)
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

void TTerminalQueue::SetTransfersLimit(int32_t Value)
{
  if (FTransfersLimit != Value)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

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

void TTerminalQueue::SetKeepDoneItemsFor(int32_t Value)
{
  if (FKeepDoneItemsFor != Value)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      FKeepDoneItemsFor = Value;
    }
  }
}

void TTerminalQueue::SetEnabled(bool Value)
{
  if (FEnabled != Value)
  {
    {
      TGuard Guard(FItemsSection); nb::used(Guard);

      FEnabled = Value;
    }

    TriggerEvent();
  }
}

bool TTerminalQueue::GetIsEmpty() const
{
  TGuard Guard(FItemsSection); nb::used(Guard);
  return (FItems->GetCount() == 0);
}

bool TTerminalQueue::TryAddParallelOperation(TQueueItem *Item, bool Force)
{
  TGuard Guard(FItemsSection); nb::used(Guard);

  bool Result =
    (FFreeTerminals > 0) ||
    (Force && (FItemsInProcess < FTransfersLimit));

  if (Result)
  {
    TQueueItem * ParallelItem = DebugNotNull(Item->CreateParallelOperation());
    if (!FEnabled)
    {
      FForcedItems->Add(ParallelItem);
    }
    AddItem(ParallelItem);
  }

  return Result;
}

bool TTerminalQueue::ContinueParallelOperation() const
{
  TGuard Guard(FItemsSection); nb::used(Guard);

  return
    (FItems->Count <= FItemsInProcess) ||
    // When queue auto processing is not enabled, keep using all connections for the transfers that were manually started
    !FEnabled;
}

// TBackgroundItem

NB_DEFINE_CLASS_ID(TBackgroundTerminal);
class TBackgroundTerminal : public TSecondaryTerminal
{
  friend class TTerminalItem;
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TBackgroundTerminal); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TBackgroundTerminal) || TSecondaryTerminal::is(Kind); }

public:
  explicit TBackgroundTerminal(TTerminal *MainTerminal) noexcept;
  virtual ~TBackgroundTerminal() = default;

  void Init(
    TSessionData *SessionData, TConfiguration *Configuration,
    TTerminalItem *Item, const UnicodeString Name);

protected:
  bool DoQueryReopen(Exception *E) override;

private:
  TTerminalItem *FItem{nullptr};
};

TBackgroundTerminal::TBackgroundTerminal(TTerminal *MainTerminal) noexcept :
  TSecondaryTerminal(OBJECT_CLASS_TBackgroundTerminal, MainTerminal)
{
}

void TBackgroundTerminal::Init(
  TSessionData *ASessionData, TConfiguration *AConfiguration, TTerminalItem *Item,
  const UnicodeString Name)
{
  TSecondaryTerminal::Init(ASessionData, AConfiguration, Name);
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
    ::Sleep(nb::ToDWord(GetConfiguration()->GetSessionReopenBackground()));
    Result = true;
  }
  return Result;
}

// TTerminalItem

TTerminalItem::TTerminalItem(TTerminalQueue *Queue) noexcept :
  TSignalThread(OBJECT_CLASS_TTerminalItem),
  FQueue(Queue)
{
}

void TTerminalItem::InitTerminalItem(int32_t Index)
{
  TSignalThread::InitSignalThread(true);

  std::unique_ptr<TBackgroundTerminal> Terminal(std::make_unique<TBackgroundTerminal>(FQueue->FTerminal));
  try__catch
  {
    Terminal->Init(FQueue->FSessionData.get(), FQueue->FConfiguration, this, FORMAT("Background %d", Index));
    Terminal->SetUseBusyCursor(false);

    Terminal->SetOnQueryUser(nb::bind(&TTerminalItem::TerminalQueryUser, this));
    Terminal->SetOnPromptUser(nb::bind(&TTerminalItem::TerminalPromptUser, this));
    Terminal->SetOnShowExtendedException(nb::bind(&TTerminalItem::TerminalShowExtendedException, this));
    Terminal->SetOnProgress(nb::bind(&TTerminalItem::OperationProgress, this));
    Terminal->SetOnFinished(nb::bind(&TTerminalItem::OperationFinished, this));

    FTerminal = Terminal.release();
  }
  catch__removed
  ({
    delete FTerminal;
    throw;
  })

  Start();
}

TTerminalItem::~TTerminalItem() noexcept
{
  Close();

  DebugAssert(FItem == nullptr);
  SAFE_DESTROY(FTerminal);
}

void TTerminalItem::Process(TQueueItem *Item)
{
  {
    TGuard Guard(FCriticalSection); nb::used(Guard);

    DebugAssert(FItem == nullptr);
    FItem = Item;
  }

  TriggerEvent();
}

void TTerminalItem::ProcessEvent()
{
  if (!FItem)
    return;
  TGuard Guard(FCriticalSection); nb::used(Guard);

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
  catch (Exception &E)
  {
    UnicodeString Message;
    if (ExceptionMessageFormatted(&E, Message))
    {
      // do not show error messages, if task was canceled anyway
      // (for example if transfer is canceled during reconnection attempts)
      if (!FCancel &&
        (FTerminal->QueryUserException("", &E, qaOK | qaCancel, nullptr, qtError) == qaCancel))
      {
        FCancel = true;
      }
    }
  }

  FItem->SetStatus(TQueueItem::qsDone);

  FItem->FTerminalItem = nullptr;

  TQueueItem *Item = FItem;
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
  TGuard Guard(FCriticalSection); nb::used(Guard);

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

bool TTerminalItem::ProcessUserAction(void *Arg)
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
  TQueueItem::TStatus ItemStatus, TUserAction *UserAction)
{
  DebugAssert(FItem != nullptr);
  if (!FItem)
    return false;
  DebugAssert((FItem->GetStatus() == TQueueItem::qsProcessing) ||
    (FItem->GetStatus() == TQueueItem::qsConnecting));

  bool Result;

  TQueueItem::TStatus PrevStatus = FItem->GetStatus();

  try__finally
  {
    FUserAction = UserAction;

    FItem->SetStatus(ItemStatus);
    FQueue->DoEvent(qePendingUserAction);

    Result = !FTerminated && WaitForEvent() && !FCancel;
  },
  __finally
  {
    FUserAction = nullptr;
    FItem->SetStatus(PrevStatus);
  } end_try__finally

  return Result;
}

bool TTerminalItem::Finished()
{
  bool Result = TSignalThread::Finished();

  FQueue->TerminalFinished(this);

  return Result;
}

void TTerminalItem::TerminalQueryUser(TObject *Sender,
  const UnicodeString AQuery, TStrings *MoreMessages, uint32_t Answers,
  const TQueryParams *Params, uint32_t &Answer, TQueryType Type, void *Arg)
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

void TTerminalItem::TerminalPromptUser(TTerminal *Terminal,
  TPromptKind Kind, const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts,
  TStrings *Results, bool &Result, void *Arg)
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
    Action.Name = AName;
    Action.Instructions = AInstructions;
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
  TTerminal *Terminal, Exception *E, void *Arg)
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
  TOperationSide /*Side*/, bool /*Temp*/, const UnicodeString /*AFileName*/,
  bool /*Success*/, TOnceDoneOperation & /*OnceDoneOperation*/)
{
  // nothing
}

void TTerminalItem::OperationProgress(
  TFileOperationProgressType &ProgressData)
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
      FItem->SetStatus(TQueueItem::qsPaused);

      WaitForEvent();
    },
    __finally
    {
      FItem->SetStatus(PrevStatus);
      ProgressData.Resume();
    } end_try__finally
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

bool TTerminalItem::OverrideItemStatus(TQueueItem::TStatus &ItemStatus) const
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

TQueueItem::TQueueItem(TObjectClassId Kind) noexcept :
  TObject(Kind),
  FInfo(std::make_unique<TInfo>())
{
  FInfo->SingleFile = false;
  FInfo->Primary = true;
  FInfo->GroupToken = this;
}

TQueueItem::~TQueueItem() noexcept
{
  // we need to keep the total transfer size even after transfer completes
  SAFE_DESTROY(FProgressData);

  Complete();

//  SAFE_DESTROY(FInfo);
}

bool TQueueItem::Complete()
{
  TGuard Guard(FSection); nb::used(Guard);

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
  TGuard Guard(FSection); nb::used(Guard);

  return FStatus;
}

void TQueueItem::SetStatus(TStatus Status)
{
  {
    TGuard Guard(FSection); nb::used(Guard);

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
  TFileOperationProgressType &ProgressData)
{
  {
    TGuard Guard(FSection); nb::used(Guard);

    // do not lose CPS limit override on "calculate size" operation,
    // wait until the real transfer operation starts
    if ((FCPSLimit >= 0) && ProgressData.IsTransfer())
    {
      ProgressData.SetCPSLimit(static_cast<uint32_t>(FCPSLimit));
      FCPSLimit = -1;
    }

    DebugAssert(FProgressData != nullptr);
    FProgressData->Assign(ProgressData);
    FProgressData->Reset();
  }
  ProgressUpdated();
  FQueue->DoQueueItemUpdate(this);
}

void TQueueItem::GetData(TQueueItemProxy *Proxy) const
{
  TGuard Guard(FSection); nb::used(Guard);

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

bool TQueueItem::UpdateFileList(TQueueFileList *)
{
  // noop - implemented in TTransferQueueItem
  return false;
}

void TQueueItem::Execute(TTerminalItem * TerminalItem)
{
  {
    DebugAssert(FProgressData == nullptr);
    TGuard Guard(FSection); nb::used(Guard);
    FProgressData = new TFileOperationProgressType();
  }
  DoExecute(TerminalItem->FTerminal);
}

void TQueueItem::SetCPSLimit(int32_t CPSLimit)
{
  FCPSLimit = CPSLimit;
}

int32_t TQueueItem::DefaultCPSLimit() const
{
  return 0;
}

int32_t TQueueItem::GetCPSLimit() const
{
  int32_t Result;
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

TQueueItem *TQueueItem::CreateParallelOperation()
{
  return nullptr;
}

// TQueueItemProxy

TQueueItemProxy::TQueueItemProxy(TTerminalQueue *Queue,
  TQueueItem *QueueItem) noexcept :
  TObject(OBJECT_CLASS_TQueueItemProxy),
  FProgressData(std::make_unique<TFileOperationProgressType>()),
  FQueue(Queue),
  FQueueItem(QueueItem),
  FInfo(std::make_unique<TQueueItem::TInfo>())
{
  Update();
}

TQueueItemProxy::~TQueueItemProxy() noexcept
{
//  SAFE_DESTROY(FProgressData);
//  SAFE_DESTROY(FInfo);
}

TFileOperationProgressType *TQueueItemProxy::GetProgressData() const
{
  return (FProgressData->GetOperation() == foNone) ? nullptr : FProgressData.get();
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

  bool Result = FQueue->ItemGetData(FQueueItem, this, nullptr);

  if ((FQueueStatus != nullptr) && (PrevStatus != GetStatus()))
  {
    FQueueStatus->ResetStats();
  }

  return Result;
}

bool TQueueItemProxy::UpdateFileList(TQueueFileList * FileList)
{
  return FQueue->ItemGetData(FQueueItem, nullptr, FileList);
}

bool TQueueItemProxy::ExecuteNow()
{
  return FQueue->ItemExecuteNow(FQueueItem);
}

bool TQueueItemProxy::Move(bool Sooner)
{
  bool Result = false;
  int32_t Index = GetIndex();
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

bool TQueueItemProxy::Move(TQueueItemProxy *BeforeItem)
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
    Result = FQueue->ItemProcessUserAction(FQueueItem, nullptr);
  },
  __finally
  {
    FProcessingUserAction = false;
  } end_try__finally
  return Result;
}

bool TQueueItemProxy::GetCPSLimit(int32_t &CPSLimit) const
{
  return FQueue->ItemGetCPSLimit(FQueueItem, CPSLimit);
}

bool TQueueItemProxy::SetCPSLimit(int32_t CPSLimit)
{
  return FQueue->ItemSetCPSLimit(FQueueItem, CPSLimit);
}

int32_t TQueueItemProxy::GetIndex() const
{
  DebugAssert(FQueueStatus != nullptr);
  int32_t Index = FQueueStatus ? FQueueStatus->FList->IndexOf(this) : 0;
  DebugAssert(Index >= 0);
  return Index;
}

// TTerminalQueueStatus

TTerminalQueueStatus::TTerminalQueueStatus() noexcept :
  FList(std::make_unique<TList>())
{
  ResetStats();
}

TTerminalQueueStatus::~TTerminalQueueStatus() noexcept
{
  for (int32_t Index = 0; Index < FList->GetCount(); ++Index)
  {
    TQueueItemProxy *Item = GetItem(Index);
    SAFE_DESTROY(Item);
  }
//  SAFE_DESTROY(FList);
}

void TTerminalQueueStatus::ResetStats() const
{
  FActiveCount = -1;
  FActivePrimaryCount = -1;
  FActiveAndPendingPrimaryCount = -1;
}

void TTerminalQueueStatus::SetDoneCount(int32_t Value)
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
    for (int32_t Index = GetDoneCount(); Index < GetCount(); ++Index)
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

int32_t TTerminalQueueStatus::GetActiveCount() const
{
  NeedStats();
  return FActiveCount;
}

int32_t TTerminalQueueStatus::GetDoneAndActiveCount() const
{
  return GetDoneCount() + GetActiveCount();
}

int32_t TTerminalQueueStatus::GetActivePrimaryCount() const
{
  NeedStats();
  return FActivePrimaryCount;
}

bool TTerminalQueueStatus::IsOnlyOneActiveAndNoPending() const
{
  return (FActivePrimaryCount == 1) && (FActiveAndPendingPrimaryCount == 1);
}

int32_t TTerminalQueueStatus::GetActiveAndPendingPrimaryCount() const
{
  NeedStats();
  return FActiveAndPendingPrimaryCount;
}

void TTerminalQueueStatus::Add(TQueueItemProxy *ItemProxy)
{
  ItemProxy->FQueueStatus = this;

  int32_t Index = FList->GetCount();
  if (!ItemProxy->GetInfo()->Primary)
  {
    for (int32_t I = 0; I < FList->GetCount(); I++)
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

void TTerminalQueueStatus::Delete(TQueueItemProxy *ItemProxy)
{
  FList->Extract(ItemProxy);
  ItemProxy->FQueueStatus = nullptr;
  ResetStats();
}

int32_t TTerminalQueueStatus::GetCount() const
{
  return FList->GetCount();
}

TQueueItemProxy *TTerminalQueueStatus::GetItem(int32_t Index) const
{
  return const_cast<TTerminalQueueStatus *>(this)->GetItem(Index);
}

TQueueItemProxy * TTerminalQueueStatus::GetItem(int32_t Index)
{
  return FList->GetAs<TQueueItemProxy>(Index);
}

TQueueItemProxy * TTerminalQueueStatus::FindByQueueItem(
  TQueueItem * QueueItem)
{
  for (int32_t Index = 0; Index < FList->GetCount(); ++Index)
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

bool TTerminalQueueStatus::UpdateFileList(TQueueItemProxy * ItemProxy, TQueueFileList * FileList)
{
  bool Result;
  if (ItemProxy != nullptr)
  {
    Result = ItemProxy->UpdateFileList(FileList);
  }
  else
  {
    Result = (FileList->GetCount() > 0);
    if (Result)
    {
      FileList->Clear();
    }
  }
  return Result;
}

// TBootstrapQueueItem

TBootstrapQueueItem::TBootstrapQueueItem() noexcept :
  TQueueItem(OBJECT_CLASS_TBootstrapQueueItem)
{
  FInfo->SingleFile = true;
}

TBootstrapQueueItem::TBootstrapQueueItem(TObjectClassId Kind) noexcept :
  TQueueItem(Kind)
{
  FInfo->SingleFile = true;
}

TBootstrapQueueItem::~TBootstrapQueueItem()
{
  TBootstrapQueueItem::Complete();
}

void TBootstrapQueueItem::DoExecute(TTerminal * DebugUsedArg(Terminal))
{
  // noop
}

UnicodeString TBootstrapQueueItem::GetStartupDirectory() const
{
  return UnicodeString();
}

bool TBootstrapQueueItem::Complete()
{
  TQueueItem::Complete();
  // To hide the item, even if "keep done items" is on
  return false;
}

// TLocatedQueueItem

TLocatedQueueItem::TLocatedQueueItem(TObjectClassId Kind, TTerminal *Terminal) noexcept :
  TQueueItem(Kind)
{
  DebugAssert(Terminal != nullptr);
  FCurrentDir = Terminal->RemoteGetCurrentDirectory();
}

TLocatedQueueItem::TLocatedQueueItem(const TLocatedQueueItem &Source) noexcept :
  TQueueItem(OBJECT_CLASS_TLocatedQueueItem)
{
  FCurrentDir = Source.FCurrentDir;
}

UnicodeString TLocatedQueueItem::GetStartupDirectory() const
{
  return FCurrentDir;
}

void TLocatedQueueItem::DoExecute(TTerminal *Terminal)
{
  DebugAssert(Terminal != nullptr);
  if (Terminal)
    Terminal->TerminalSetCurrentDirectory(FCurrentDir);
}

// TTransferQueueItem

TTransferQueueItem::TTransferQueueItem(TObjectClassId Kind, TTerminal *Terminal,
  const TStrings *AFilesToCopy, const UnicodeString TargetDir,
  const TCopyParamType *CopyParam, int32_t Params, TOperationSide Side,
  bool SingleFile, bool Parallel) noexcept :
  TLocatedQueueItem(Kind, Terminal),
  FFilesToCopy(std::unique_ptr<TStringList>())
{
  FInfo->Operation = (Params & cpDelete) ? foMove : foCopy;
  FInfo->Side = Side;
  FInfo->SingleFile = SingleFile;

  DebugAssert(AFilesToCopy != nullptr);
  if (!AFilesToCopy)
    return;
  FFilesToCopy = std::make_unique<TStringList>();
  for (int32_t Index = 0; Index < AFilesToCopy->GetCount(); ++Index)
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

TTransferQueueItem::~TTransferQueueItem() noexcept
{
  for (int32_t Index = 0; Index < FFilesToCopy->GetCount(); ++Index)
  {
    TObject *Object = FFilesToCopy->GetObj(Index);
    SAFE_DESTROY(Object);
  }
//  SAFE_DESTROY(FFilesToCopy);
  SAFE_DESTROY(FCopyParam);
}

int32_t TTransferQueueItem::DefaultCPSLimit() const
{
  return FCopyParam->GetCPSLimit();
}

void TTransferQueueItem::DoExecute(TTerminal *Terminal)
{
  TLocatedQueueItem::DoExecute(Terminal);

  DebugAssert(Terminal != nullptr);
  FParallelOperation.reset(new TParallelOperation(FInfo->Side));
  try__finally
  {
    DoTransferExecute(Terminal, FParallelOperation.get());
  },
  __finally
  {
    FParallelOperation->WaitFor();
  } end_try__finally
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
      TGuard Guard(FSection); nb::used(Guard);
      DebugAssert(FParallelOperation.get() != nullptr);
      // Won't be initialized, if the operation is not eligible for parallel transfers (like cpDelete).
      // We can probably move the check outside of the guard.
      if (FParallelOperation && FParallelOperation->IsInitialized())
      {
        DebugAssert(FProgressData->IsTransfer());
        if (FProgressData->Operation == foCopy)
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
        TGuard Guard(FSection); nb::used(Guard);
        FLastParallelOperationAdded = LastParallelOperationAddedPrev;
      }
    }
  }
}

TQueueItem *TTransferQueueItem::CreateParallelOperation()
{
  DebugAssert(FParallelOperation.get() != nullptr);

  FParallelOperation->AddClient();
  return new TParallelTransferQueueItem(this, FParallelOperation.get());
}

bool TTransferQueueItem::UpdateFileList(TQueueFileList * FileList)
{
  TGuard Guard(FSection);
  bool Result;
  if ((FParallelOperation.get() != nullptr) && FParallelOperation->IsInitialized())
  {
    Result = FParallelOperation->UpdateFileList(FileList);
  }
  else
  {
    Result = false;
  }
  return Result;
}

// TUploadQueueItem

TUploadQueueItem::TUploadQueueItem(TTerminal *Terminal,
  const TStrings *AFilesToCopy, const UnicodeString TargetDir,
  const TCopyParamType *CopyParam, int32_t Params, bool SingleFile, bool Parallel) noexcept :
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

void TUploadQueueItem::DoTransferExecute(TTerminal *Terminal, TParallelOperation *ParallelOperation)
{
  Terminal->CopyToRemote(FFilesToCopy.get(), FTargetDir, FCopyParam, FParams, ParallelOperation);
}

// TParallelTransferQueueItem

TParallelTransferQueueItem::TParallelTransferQueueItem(
  const TLocatedQueueItem *ParentItem, TParallelOperation *ParallelOperation) noexcept :
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

void TParallelTransferQueueItem::DoExecute(TTerminal *Terminal)
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
    Operation, FParallelOperation->GetSide(), -1, Temp, FParallelOperation->GetTargetDir(), 0, odoIdle);

  try__finally
  {
    bool Continue = true;
    do
    {
      int32_t GotNext = Terminal->CopyToParallel(FParallelOperation, &OperationProgress);
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
  },
  __finally
  {
    OperationProgress.Stop();
    FParallelOperation->RemoveClient();
  } end_try__finally
}

// TDownloadQueueItem

TDownloadQueueItem::TDownloadQueueItem(TTerminal *Terminal,
  const TStrings *AFilesToCopy, const UnicodeString TargetDir,
  const TCopyParamType *CopyParam, int32_t Params, bool SingleFile, bool Parallel) noexcept :
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

void TDownloadQueueItem::DoTransferExecute(TTerminal *Terminal, TParallelOperation *ParallelOperation)
{
  DebugAssert(Terminal != nullptr);
  Terminal->CopyToLocal(FFilesToCopy.get(), FTargetDir, FCopyParam, FParams, ParallelOperation);
}

// TTerminalThread

TTerminalThread::TTerminalThread(TTerminal *Terminal) noexcept :
  TSignalThread(OBJECT_CLASS_TTerminalThread),
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
  FAbandoned = false;
  FAllowAbandon = false;
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

TTerminalThread::~TTerminalThread() noexcept
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

  __removed delete FSection;
  if (FAbandoned)
  {
    SAFE_DESTROY(FTerminal);
  }
}

void TTerminalThread::Cancel()
{
  FCancel = true;
  FCancelAfter = IncMilliSecond(Now(), 1000);
}

void TTerminalThread::Idle()
{
  TGuard Guard(FSection); nb::used(Guard);
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
              catch (Exception &E)
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
              Wait = nb::Min(Wait + 10, MaxWait);
            }
          }
          break;

          default:
            throw Exception("Error waiting for background session task to complete");
        }

        if (FAllowAbandon && !Done && FCancel && (Now() >= FCancelAfter))
        {
          TGuard Guard(FSection); nb::used(Guard);
          if (WaitForSingleObject(FActionEvent, 0) != WAIT_OBJECT_0)
          {
            FAbandoned = true;
            FCancelled = true;
            FatalAbort();
          }
        }
      }
      while (!Done);


      if (Done)
      {
        Rethrow(FException);
      }
    },
    __finally
    {
      FAction = nullptr;
      SAFE_DESTROY_EX(Exception, FException);
    } end_try__finally
  }
  catch(...)
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
  catch(Exception & E)
  {
    SaveException(E, FException);
  }

  {
    TGuard Guard(FSection); nb::used(Guard);
    if (!FAbandoned)
    {
      ::SetEvent(FActionEvent);
    }
  }
}

void TTerminalThread::Rethrow(Exception *&AException)
{
  if (AException != nullptr)
  {
    try__finally
    {
      RethrowException(AException);
    },
    __finally
    {
      SAFE_DESTROY_EX(Exception, AException);
    } end_try__finally
  }
}

void TTerminalThread::SaveException(Exception &E, Exception *&Exception)
{
  DebugAssert(Exception == nullptr);

  Exception = CloneException(&E);
}

void TTerminalThread::FatalAbort()
{
  if (FAbandoned)
  {
    // We cannot use TTerminal::FatalError as the terminal still runs on a backgroud thread,
    // may have its TCallbackGuard armed right now.
    throw ESshFatal(nullptr, "");
  }
  else
  {
    FTerminal->FatalAbort();
  }
}

void TTerminalThread::CheckCancel()
{
  if (FCancel && !FCancelled)
  {
    FCancelled = true;
    FatalAbort();
  }
}

void TTerminalThread::WaitForUserAction(TUserAction *UserAction)
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
      DebugAlwaysFalse(UserAction == nullptr) || !UserAction->Force() || FAbandoned;
    if (DoCheckCancel)
    {
      CheckCancel();
    }

    // have to save it as we can go recursive via TQueryParams::TimerEvent,
    // see TTerminalThread::TerminalQueryUser
    TUserAction *PrevUserAction = FUserAction;
    try__finally
    {
      FUserAction = UserAction;

      while (true)
      {

        {
          TGuard Guard(FSection); nb::used(Guard);
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
            catch (Exception &E)
            {
              SaveException(E, FIdleException);
            }
          }
        }

        int32_t WaitResult = nb::ToIntPtr(WaitForEvent(1000));
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
    },
    __finally
    {
      FUserAction = PrevUserAction;
      SAFE_DESTROY_EX(Exception, FException);
    } end_try__finally

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
  TTerminal *Terminal, const UnicodeString AStr, bool Status, int32_t Phase, const UnicodeString & Additional)
{
  TInformationUserAction Action(FOnInformation);
  Action.Terminal = Terminal;
  Action.Str = AStr;
  Action.Status = Status;
  Action.Phase = Phase;
  Action.Additional = Additional;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalQueryUser(TObject *Sender,
  const UnicodeString AQuery, TStrings *MoreMessages, uint32_t Answers,
  const TQueryParams *Params, uint32_t &Answer, TQueryType Type, void *Arg)
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

void TTerminalThread::TerminalInitializeLog(TObject *Sender)
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

void TTerminalThread::TerminalPromptUser(TTerminal *Terminal,
  TPromptKind Kind, const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts,
  TStrings *Results, bool &Result, void *Arg)
{
  DebugUsedParam(Arg);
  DebugAssert(Arg == nullptr);

  TPromptUserAction Action(FOnPromptUser);
  Action.Terminal = Terminal;
  Action.Kind = Kind;
  Action.Name = AName;
  Action.Instructions = AInstructions;
  Action.Prompts = Prompts;
  Action.Results->AddStrings(Results);

  WaitForUserAction(&Action);

  Results->Clear();
  Results->AddStrings(Action.Results);
  Result = Action.Result;
}

void TTerminalThread::TerminalShowExtendedException(
  TTerminal *Terminal, Exception *E, void *Arg)
{
  DebugUsedParam(Arg);
  DebugAssert(Arg == nullptr);

  TShowExtendedExceptionAction Action(FOnShowExtendedException);
  Action.Terminal = Terminal;
  Action.E = E;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalDisplayBanner(TTerminal *Terminal,
  const UnicodeString ASessionName, const UnicodeString ABanner,
  bool &NeverShowAgain, int32_t Options, uint32_t &Params)
{
  TDisplayBannerAction Action(FOnDisplayBanner);
  Action.Terminal = Terminal;
  Action.SessionName = ASessionName;
  Action.Banner = ABanner;
  Action.NeverShowAgain = NeverShowAgain;
  Action.Options = Options;
  Action.Params = Params;

  WaitForUserAction(&Action);

  NeverShowAgain = Action.NeverShowAgain;
  Params = Action.Params;
}

void TTerminalThread::TerminalChangeDirectory(TObject *Sender)
{
  TNotifyAction Action(FOnChangeDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalReadDirectory(TObject *Sender, Boolean ReloadOnly)
{
  TReadDirectoryAction Action(FOnReadDirectory);
  Action.Sender = Sender;
  Action.ReloadOnly = ReloadOnly;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalStartReadDirectory(TObject *Sender)
{
  TNotifyAction Action(FOnStartReadDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalReadDirectoryProgress(
  TObject *Sender, int32_t Progress, int32_t ResolvedLinks, bool &Cancel)
{
  TReadDirectoryProgressAction Action(FOnReadDirectoryProgress);
  Action.Sender = Sender;
  Action.Progress = Progress;
  Action.ResolvedLinks = ResolvedLinks;
  Action.Cancel = Cancel;

  WaitForUserAction(&Action);

  Cancel = Action.Cancel;
}

bool TTerminalThread::Release()
{
  bool Result = !FAbandoned;
  if (Result)
  {
    delete this;
  }
  return Result;
}

bool TTerminalThread::Finished()
{
  return TSimpleThread::Finished() || FAbandoned;
}


TQueueFileList::TQueueFileList() :
  FList(new TStringList())
{
  Clear();
}

void TQueueFileList::Clear()
{
  FList->Clear();
  FLastParallelOperation = nullptr;
  FLastParallelOperationVersion = -1;
}

void TQueueFileList::Add(const UnicodeString & FileName, int32_t State)
{
  FList->AddObject(FileName, reinterpret_cast<TObject *>(State));
}

UnicodeString TQueueFileList::GetFileName(int32_t Index) const
{
  return FList->GetString(Index);
}

int32_t TQueueFileList::GetState(int32_t Index) const
{
  return reinterpret_cast<int32_t>(FList->GetObj(Index));
}

void TQueueFileList::SetState(int32_t Index, int32_t State)
{
  FList->SetObj(Index, reinterpret_cast<TObject *>(State));
}

int32_t TQueueFileList::GetCount() const
{
  return FList->Count();
}
