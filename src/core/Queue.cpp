//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Terminal.h"
#include "Queue.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
class TBackgroundTerminal;
//---------------------------------------------------------------------------
class TUserAction : public TObject
{
public:
  explicit TUserAction() {}
  virtual ~TUserAction() {}
  virtual void Execute(void * Arg) = 0;
  virtual bool Force() { return false; }

private:
  NB_DISABLE_COPY(TUserAction)
};
//---------------------------------------------------------------------------
class TNotifyAction : public TUserAction
{
public:
  explicit TNotifyAction(TNotifyEvent AOnNotify) :
    OnNotify(AOnNotify),
    Sender(nullptr)
  {
  }

  virtual void Execute(void * /*Arg*/)
  {
    if (OnNotify != nullptr)
    {
      OnNotify(Sender);
    }
  }

  TNotifyEvent OnNotify;
  TObject * Sender;

private:
  NB_DISABLE_COPY(TNotifyAction)
};
//---------------------------------------------------------------------------
class TInformationUserAction : public TUserAction
{
public:
  explicit TInformationUserAction(TInformationEvent AOnInformation) :
    OnInformation(AOnInformation),
    Terminal(nullptr),
    Status(false),
    Phase(0)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnInformation != nullptr)
    {
      OnInformation(Terminal, Str, Status, Phase);
    }
  }

  virtual bool Force()
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

private:
  NB_DISABLE_COPY(TInformationUserAction)
};
//---------------------------------------------------------------------------
class TQueryUserAction : public TUserAction
{
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

  virtual void Execute(void * Arg)
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

private:
  NB_DISABLE_COPY(TQueryUserAction)
};
//---------------------------------------------------------------------------
class TPromptUserAction : public TUserAction
{
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
    delete Results;
  }

  virtual void Execute(void * Arg)
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

private:
  NB_DISABLE_COPY(TPromptUserAction)
};
//---------------------------------------------------------------------------
class TShowExtendedExceptionAction : public TUserAction
{
public:
  explicit TShowExtendedExceptionAction(TExtendedExceptionEvent AOnShowExtendedException) :
    OnShowExtendedException(AOnShowExtendedException),
    Terminal(nullptr),
    E(nullptr)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnShowExtendedException != nullptr)
    {
      OnShowExtendedException(Terminal, E, Arg);
    }
  }

  TExtendedExceptionEvent OnShowExtendedException;
  TTerminal * Terminal;
  Exception * E;

private:
  NB_DISABLE_COPY(TShowExtendedExceptionAction)
};
//---------------------------------------------------------------------------
class TDisplayBannerAction : public TUserAction
{
public:
  explicit TDisplayBannerAction(TDisplayBannerEvent AOnDisplayBanner) :
    OnDisplayBanner(AOnDisplayBanner),
    Terminal(nullptr),
    NeverShowAgain(false),
    Options(0)
  {
  }

  virtual void Execute(void * /*Arg*/)
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

private:
  NB_DISABLE_COPY(TDisplayBannerAction)
};
//---------------------------------------------------------------------------
class TReadDirectoryAction : public TUserAction
{
public:
  explicit TReadDirectoryAction(TReadDirectoryEvent AOnReadDirectory) :
    OnReadDirectory(AOnReadDirectory),
    Sender(nullptr),
    ReloadOnly(false)
  {
  }

  virtual void Execute(void * /*Arg*/)
  {
    if (OnReadDirectory != nullptr)
    {
      OnReadDirectory(Sender, ReloadOnly);
    }
  }

  TReadDirectoryEvent OnReadDirectory;
  TObject * Sender;
  bool ReloadOnly;

private:
  NB_DISABLE_COPY(TReadDirectoryAction)
};
//---------------------------------------------------------------------------
class TReadDirectoryProgressAction : public TUserAction
{
public:
  explicit TReadDirectoryProgressAction(TReadDirectoryProgressEvent AOnReadDirectoryProgress) :
    OnReadDirectoryProgress(AOnReadDirectoryProgress),
    Sender(nullptr),
    Progress(0),
    Cancel(false)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnReadDirectoryProgress != nullptr)
    {
      OnReadDirectoryProgress(Sender, Progress, Cancel);
    }
  }

  TReadDirectoryProgressEvent OnReadDirectoryProgress;
  TObject * Sender;
  intptr_t Progress;
  bool Cancel;

private:
  NB_DISABLE_COPY(TReadDirectoryProgressAction)
};
//---------------------------------------------------------------------------
class TTerminalItem : public TSignalThread
{
friend class TQueueItem;
friend class TBackgroundTerminal;

public:
  explicit TTerminalItem(TTerminalQueue * Queue);
  virtual ~TTerminalItem();
  virtual void Init(intptr_t Index);

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
  TCriticalSection * FCriticalSection;
  TUserAction * FUserAction;
  bool FCancel;
  bool FPause;

  virtual void ProcessEvent();
  virtual void Finished();
  bool WaitForUserAction(TQueueItem::TStatus ItemStatus, TUserAction * UserAction);
  bool OverrideItemStatus(TQueueItem::TStatus & ItemStatus);

  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & Query, TStrings * MoreMessages, uintptr_t Answers,
    const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void OperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void OperationProgress(TFileOperationProgressType & ProgressData,
    TCancelStatus & Cancel);
private:
  NB_DISABLE_COPY(TTerminalItem)
};
//---------------------------------------------------------------------------
// TSignalThread
//---------------------------------------------------------------------------
int TSimpleThread::ThreadProc(void * Thread)
{
  TSimpleThread * SimpleThread = reinterpret_cast<TSimpleThread*>(Thread);
  assert(SimpleThread != nullptr);
  try
  {
    SimpleThread->Execute();
  }
  catch (...)
  {
    // we do not expect thread to be terminated with exception
    assert(false);
  }
  SimpleThread->FFinished = true;
  SimpleThread->Finished();
  return 0;
}
//---------------------------------------------------------------------------
TSimpleThread::TSimpleThread() :
  FThread(nullptr),
  FThreadId(0),
  FFinished(true)
{
}
//---------------------------------------------------------------------------
void TSimpleThread::Init()
{
  FThread = StartThread(nullptr, 0, this, CREATE_SUSPENDED, FThreadId);
}
//---------------------------------------------------------------------------
TSimpleThread::~TSimpleThread()
{
  Close();

  if (FThread != nullptr)
  {
    ::CloseHandle(FThread);
  }
}
//---------------------------------------------------------------------------
bool TSimpleThread::IsFinished()
{
  return FFinished;
}
//---------------------------------------------------------------------------
void TSimpleThread::Start()
{
  if (ResumeThread(FThread) == 1)
  {
    FFinished = false;
  }
}
//---------------------------------------------------------------------------
void TSimpleThread::Finished()
{
}
//---------------------------------------------------------------------------
void TSimpleThread::Close()
{
  if (!FFinished)
  {
    Terminate();
    WaitFor();
  }
}
//---------------------------------------------------------------------------
void TSimpleThread::WaitFor(unsigned int Milliseconds)
{
  WaitForSingleObject(FThread, Milliseconds);
}
//---------------------------------------------------------------------------
// TSignalThread
//---------------------------------------------------------------------------
TSignalThread::TSignalThread() :
  TSimpleThread(),
  FEvent(nullptr),
  FTerminated(true)
{
}
//---------------------------------------------------------------------------
void TSignalThread::Init(bool LowPriority)
{
  TSimpleThread::Init();
  FEvent = CreateEvent(nullptr, false, false, nullptr);
  assert(FEvent != nullptr);

  if (LowPriority)
  {
    ::SetThreadPriority(FThread, THREAD_PRIORITY_BELOW_NORMAL);
  }
}
//---------------------------------------------------------------------------
TSignalThread::~TSignalThread()
{
  // cannot leave closing to TSimpleThread as we need to close it before
  // destroying the event
  Close();

  if (FEvent)
  {
    ::CloseHandle(FEvent);
  }
}
//---------------------------------------------------------------------------
void TSignalThread::Start()
{
  FTerminated = false;
  TSimpleThread::Start();
}
//---------------------------------------------------------------------------
void TSignalThread::TriggerEvent()
{
  SetEvent(FEvent);
}
//---------------------------------------------------------------------------
bool TSignalThread::WaitForEvent()
{
  // should never return -1, so it is only about 0 or 1
  return WaitForEvent(INFINITE) > 0;
}
//---------------------------------------------------------------------------
int TSignalThread::WaitForEvent(unsigned int Timeout)
{
  unsigned int Result = WaitForSingleObject(FEvent, Timeout);
  int Return;
  if ((Result == WAIT_TIMEOUT) && !FTerminated)
  {
    Return = -1;
  }
  else
  {
    Return = ((Result == WAIT_OBJECT_0) && !FTerminated) ? 1 : 0;
  }
  return Return;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TSignalThread::Terminate()
{
  FTerminated = true;
  TriggerEvent();
}
//---------------------------------------------------------------------------
// TTerminalQueue
//---------------------------------------------------------------------------
TTerminalQueue::TTerminalQueue(TTerminal * Terminal,
  TConfiguration * Configuration) :
  TSignalThread(),
  FTerminal(Terminal), FConfiguration(Configuration), FSessionData(nullptr),
  FItems(nullptr), FDoneItems(nullptr), FItemsInProcess(0), FItemsSection(nullptr),
  FFreeTerminals(0), FTerminals(nullptr), FForcedItems(nullptr), FTemporaryTerminals(0),
  FOverallTerminals(0), FTransfersLimit(2), FKeepDoneItemsFor(0), FEnabled(true)
{
}
//---------------------------------------------------------------------------
void TTerminalQueue::Init()
{
  TSignalThread::Init(true);
  FOnQueryUser = nullptr;
  FOnPromptUser = nullptr;
  FOnShowExtendedException = nullptr;
  FOnQueueItemUpdate = nullptr;
  FOnListUpdate = nullptr;
  FOnEvent = nullptr;
  FLastIdle = Now();
  FIdleInterval = EncodeTimeVerbose(0, 0, 2, 0);

  assert(FTerminal != nullptr);
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(FTerminal->GetSessionData());

  FItems = new TList();
  FDoneItems = new TList();
  FTerminals = new TList();
  FForcedItems = new TList();

  FItemsSection = new TCriticalSection();

  Start();
}
//---------------------------------------------------------------------------
TTerminalQueue::~TTerminalQueue()
{
  Close();

  {
    TGuard Guard(FItemsSection);

    while (FTerminals->GetCount() > 0)
    {
      TTerminalItem * TerminalItem = reinterpret_cast<TTerminalItem *>(FTerminals->GetItem(0));
      FTerminals->Delete(0);
      TerminalItem->Terminate();
      TerminalItem->WaitFor();
      delete TerminalItem;
    }
    delete FTerminals;
    delete FForcedItems;

    FreeItemsList(FItems);
    FreeItemsList(FDoneItems);
  }

  delete FItemsSection;
  delete FSessionData;
}
//---------------------------------------------------------------------------
void TTerminalQueue::FreeItemsList(TList * List)
{
  for (intptr_t Index = 0; Index < List->GetCount(); ++Index)
  {
    delete GetItem(List, Index);
  }
  delete List;
}
//---------------------------------------------------------------------------
void TTerminalQueue::TerminalFinished(TTerminalItem * TerminalItem)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FTerminals->IndexOf(TerminalItem);
      assert(Index >= 0);

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

      delete TerminalItem;
    }

    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
bool TTerminalQueue::TerminalFree(TTerminalItem * TerminalItem)
{
  bool Result = true;

  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FTerminals->IndexOf(TerminalItem);
      assert(Index >= 0);
      assert(Index >= FFreeTerminals);

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
//---------------------------------------------------------------------------
void TTerminalQueue::AddItem(TQueueItem * Item)
{
  assert(!FTerminated);

  Item->SetStatus(TQueueItem::qsPending);

  {
    TGuard Guard(FItemsSection);

    FItems->Add(Item);
    Item->FQueue = this;
  }

  DoListUpdate();

  TriggerEvent();
}
//---------------------------------------------------------------------------
void TTerminalQueue::RetryItem(TQueueItem * Item)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      intptr_t Index = FItems->Remove(Item);
      assert(Index < FItemsInProcess);
      USEDPARAM(Index);
      FItemsInProcess--;
      FItems->Add(Item);
    }

    DoListUpdate();

    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DeleteItem(TQueueItem * Item, bool CanKeep)
{
  if (!FTerminated)
  {
    bool Empty;
    bool Monitored;
    {
      TGuard Guard(FItemsSection);

      // does this need to be within guard?
      Monitored = (Item->GetCompleteEvent() != INVALID_HANDLE_VALUE);
      intptr_t Index = FItems->Remove(Item);
      assert(Index < FItemsInProcess);
      USEDPARAM(Index);
      FItemsInProcess--;
      FForcedItems->Remove(Item);
      // =0  do not keep
      // <0  infinity
      if ((FKeepDoneItemsFor != 0) && CanKeep)
      {
        assert(Item->GetStatus() == TQueueItem::qsDone);
        Item->Complete();
        FDoneItems->Add(Item);
      }
      else
      {
        delete Item;
      }

      Empty = true;
      Index = 0;
      while (Empty && (Index < FItems->GetCount()))
      {
        Empty = (GetItem(FItems, Index)->GetCompleteEvent() != INVALID_HANDLE_VALUE);
        Index++;
      }
    }

    DoListUpdate();
    // report empty, if queue is empty or only monitored items are pending.
    // do not report if current item was the last, but was monitored.
    if (!Monitored && Empty)
    {
      DoEvent(qeEmpty);
    }
  }
}
//---------------------------------------------------------------------------
TQueueItem * TTerminalQueue::GetItem(TList * List, intptr_t Index)
{
  return reinterpret_cast<TQueueItem*>(List->GetItem(Index));
}
//---------------------------------------------------------------------------
TQueueItem * TTerminalQueue::GetItem(intptr_t Index)
{
  return reinterpret_cast<TQueueItem *>(FItems->GetItem(Index));
}
//---------------------------------------------------------------------------
void TTerminalQueue::UpdateStatusForList(
  TTerminalQueueStatus * Status, TList * List, TTerminalQueueStatus * Current)
{
  for (intptr_t Index = 0; Index < List->GetCount(); Index++)
  {
    TQueueItem * Item = GetItem(List, Index);
    TQueueItemProxy * ItemProxy;
    if (Current != nullptr)
    {
      ItemProxy = Current->FindByQueueItem(Item);
    }
    else
    {
      ItemProxy = nullptr;
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
//---------------------------------------------------------------------------
TTerminalQueueStatus * TTerminalQueue::CreateStatus(TTerminalQueueStatus * Current)
{
  std::auto_ptr<TTerminalQueueStatus> Status(new TTerminalQueueStatus());
  SCOPE_EXIT
  {
    if (Current != nullptr)
    {
      delete Current;
    }
  };
  {
    TGuard Guard(FItemsSection);

    UpdateStatusForList(Status.get(), FDoneItems, Current);
    Status->SetDoneCount(Status->GetCount());
    UpdateStatusForList(Status.get(), FItems, Current);
  }

  return Status.release();
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
          delete Item;
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
bool TTerminalQueue::ItemSetCPSLimit(TQueueItem * Item, unsigned long CPSLimit)
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
//---------------------------------------------------------------------------
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
        TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->GetItem(FFreeTerminals - 1));
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
//---------------------------------------------------------------------------
bool TTerminalQueue::WaitForEvent()
{
  // terminate loop regularly, so that we can check for expired done items
  bool Result = (TSignalThread::WaitForEvent(1000) != 0);
  return Result;
}
//---------------------------------------------------------------------------
void TTerminalQueue::ProcessEvent()
{
  TTerminalItem * TerminalItem;
  do
  {
    TerminalItem = nullptr;
    TQueueItem * Item = nullptr;

    {
      TGuard Guard(FItemsSection);

      // =0  do not keep
      // <0  infinity
      if (FKeepDoneItemsFor >= 0)
      {
        TDateTime RemoveDoneItemsBefore = Now();
        if (FKeepDoneItemsFor > 0)
        {
          RemoveDoneItemsBefore = IncSecond(RemoveDoneItemsBefore, -FKeepDoneItemsFor);
        }
        for (intptr_t Index = 0; Index < FDoneItems->GetCount(); Index++)
        {
          TQueueItem * Item = GetItem(FDoneItems, Index);
          if (Item->FDoneAt <= RemoveDoneItemsBefore)
          {
            FDoneItems->Delete(Index);
            delete Item;
            Index--;
            DoListUpdate();
          }
        }
      }

      if (FItems->GetCount() > FItemsInProcess)
      {
        Item = GetItem(FItemsInProcess);
        intptr_t ForcedIndex = FForcedItems->IndexOf(Item);

        if (FEnabled || (ForcedIndex >= 0))
        {
          if ((FFreeTerminals == 0) &&
              ((FTransfersLimit <= 0) ||
               (FTerminals->GetCount() < FTransfersLimit + FTemporaryTerminals)))
          {
            FOverallTerminals++;
            TerminalItem = new TTerminalItem(this);
            TerminalItem->Init(FOverallTerminals);
            FTerminals->Add(TerminalItem);
          }
          else if (FFreeTerminals > 0)
          {
            TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->GetItem(0));
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
      TerminalItem->Process(Item);
    }
  }
  while (!FTerminated && (TerminalItem != nullptr));
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoQueueItemUpdate(TQueueItem * Item)
{
  if (GetOnQueueItemUpdate() != nullptr)
  {
    GetOnQueueItemUpdate()(this, Item);
  }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoListUpdate()
{
  if (GetOnListUpdate() != nullptr)
  {
    GetOnListUpdate()(this);
  }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoEvent(TQueueEvent Event)
{
  if (GetOnEvent() != nullptr)
  {
    GetOnEvent()(this, Event);
  }
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
bool TTerminalQueue::GetIsEmpty()
{
  TGuard Guard(FItemsSection);
  return (FItems->GetCount() == 0);
}
//---------------------------------------------------------------------------
// TBackgroundItem
//---------------------------------------------------------------------------
class TBackgroundTerminal : public TSecondaryTerminal
{
  friend class TTerminalItem;
public:
  explicit TBackgroundTerminal(TTerminal * MainTerminal);
  virtual ~TBackgroundTerminal() {}
  void Init(
    TSessionData * SessionData, TConfiguration * Configuration,
    TTerminalItem * Item, const UnicodeString & Name);
protected:
  virtual bool DoQueryReopen(Exception * E);

private:
  TTerminalItem * FItem;
};
//---------------------------------------------------------------------------
TBackgroundTerminal::TBackgroundTerminal(TTerminal * MainTerminal) :
  TSecondaryTerminal(MainTerminal),
  FItem(nullptr)
{
}
//---------------------------------------------------------------------------
void TBackgroundTerminal::Init(TSessionData * SessionData, TConfiguration * Configuration, TTerminalItem * Item,
    const UnicodeString & Name)
{
  TSecondaryTerminal::Init(SessionData, Configuration, Name);
}
//---------------------------------------------------------------------------
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
    Sleep((DWORD)GetConfiguration()->GetSessionReopenBackground());
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
// TTerminalItem
//---------------------------------------------------------------------------
TTerminalItem::TTerminalItem(TTerminalQueue * Queue) :
  TSignalThread(), FQueue(Queue), FTerminal(nullptr), FItem(nullptr),
  FCriticalSection(nullptr), FUserAction(nullptr), FCancel(false), FPause(false)
{
}
//---------------------------------------------------------------------------
void TTerminalItem::Init(intptr_t Index)
{
  TSignalThread::Init(true);

  FCriticalSection = new TCriticalSection();

  std::auto_ptr<TBackgroundTerminal> Terminal(new TBackgroundTerminal(FQueue->FTerminal));
  Terminal->Init(FQueue->FSessionData, FQueue->FConfiguration, this, FORMAT(L"Background %d", Index));
  Terminal->SetUseBusyCursor(false);

  Terminal->SetOnQueryUser(MAKE_CALLBACK(TTerminalItem::TerminalQueryUser, this));
  Terminal->SetOnPromptUser(MAKE_CALLBACK(TTerminalItem::TerminalPromptUser, this));
  Terminal->SetOnShowExtendedException(MAKE_CALLBACK(TTerminalItem::TerminalShowExtendedException, this));
  Terminal->SetOnProgress(MAKE_CALLBACK(TTerminalItem::OperationProgress, this));
  Terminal->SetOnFinished(MAKE_CALLBACK(TTerminalItem::OperationFinished, this));
  FTerminal = Terminal.release();

  Start();
}
//---------------------------------------------------------------------------
TTerminalItem::~TTerminalItem()
{
  Close();

  assert(FItem == nullptr);
  delete FTerminal;
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TTerminalItem::Process(TQueueItem * Item)
{
  {
    TGuard Guard(FCriticalSection);

    assert(FItem == nullptr);
    FItem = Item;
  }

  TriggerEvent();
}
//---------------------------------------------------------------------------
void TTerminalItem::ProcessEvent()
{
  TGuard Guard(FCriticalSection);

  bool Retry = true;

  FCancel = false;
  FPause = false;

  try
  {
    assert(FItem != nullptr);

    FItem->FTerminalItem = this;
    if (!FTerminal->GetActive())
    {
      FItem->SetStatus(TQueueItem::qsConnecting);

      FTerminal->GetSessionData()->SetRemoteDirectory(FItem->StartupDirectory());
      FTerminal->Open();
    }

    Retry = false;

    if (!FCancel)
    {
      FItem->SetStatus(TQueueItem::qsProcessing);

      FItem->Execute(this);
    }
  }
  catch(Exception & E)
  {
    UnicodeString Message;
    if (ExceptionMessage(&E, Message))
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
//---------------------------------------------------------------------------
void TTerminalItem::Idle()
{
  TGuard Guard(FCriticalSection);

  assert(FTerminal->GetActive());

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
//---------------------------------------------------------------------------
void TTerminalItem::Cancel()
{
  FCancel = true;
  if ((FItem->GetStatus() == TQueueItem::qsPaused) ||
      TQueueItem::IsUserActionStatus(FItem->GetStatus()))
  {
    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
bool TTerminalItem::Pause()
{
  assert(FItem != nullptr);
  bool Result = (FItem->GetStatus() == TQueueItem::qsProcessing) && !FPause;
  if (Result)
  {
    FPause = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminalItem::Resume()
{
  assert(FItem != nullptr);
  bool Result = (FItem->GetStatus() == TQueueItem::qsPaused);
  if (Result)
  {
    TriggerEvent();
  }
  return Result;
}
//---------------------------------------------------------------------------
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
    assert(FItem != nullptr);

    FUserAction->Execute(Arg);
    FUserAction = nullptr;

    TriggerEvent();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminalItem::WaitForUserAction(
  TQueueItem::TStatus ItemStatus, TUserAction * UserAction)
{
  assert(FItem != nullptr);
  assert((FItem->GetStatus() == TQueueItem::qsProcessing) ||
    (FItem->GetStatus() == TQueueItem::qsConnecting));

  bool Result;

  TQueueItem::TStatus PrevStatus = FItem->GetStatus();

  SCOPE_EXIT
  {
    FUserAction = nullptr;
    FItem->SetStatus(PrevStatus);
  };
  {
    FUserAction = UserAction;

    FItem->SetStatus(ItemStatus);
    FQueue->DoEvent(qePendingUserAction);

    Result = !FTerminated && WaitForEvent() && !FCancel;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminalItem::Finished()
{
  TSignalThread::Finished();

  FQueue->TerminalFinished(this);
}
//---------------------------------------------------------------------------
void TTerminalItem::TerminalQueryUser(TObject * Sender,
  const UnicodeString & Query, TStrings * MoreMessages, uintptr_t Answers,
  const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg)
{
  // so far query without queue item can occur only for key confirmation
  // on re-key with non-cached host key. make it fail.
  if (FItem != nullptr)
  {
    USEDPARAM(Arg);
    assert(Arg == nullptr);

    TQueryUserAction Action(FQueue->GetOnQueryUser());
    Action.Sender = Sender;
    Action.Query = Query;
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
//---------------------------------------------------------------------------
void TTerminalItem::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
  TStrings * Results, bool & Result, void * Arg)
{
  if (FItem == nullptr)
  {
    // sanity, should not occur
    assert(false);
    Result = false;
  }
  else
  {
    USEDPARAM(Arg);
    assert(Arg == nullptr);

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
//---------------------------------------------------------------------------
void TTerminalItem::TerminalShowExtendedException(
  TTerminal * Terminal, Exception * E, void * Arg)
{
  USEDPARAM(Arg);
  assert(Arg == nullptr);

  UnicodeString Message; // not used
  if ((FItem != nullptr) &&
      ExceptionMessage(E, Message))
  {
    TShowExtendedExceptionAction Action(FQueue->GetOnShowExtendedException());
    Action.Terminal = Terminal;
    Action.E = E;

    WaitForUserAction(TQueueItem::qsError, &Action);
  }
}
//---------------------------------------------------------------------------
void TTerminalItem::OperationFinished(TFileOperation /*Operation*/,
  TOperationSide /*Side*/, bool /*Temp*/, const UnicodeString & /*FileName*/,
  bool /*Success*/, TOnceDoneOperation & /*OnceDoneOperation*/)
{
  // nothing
}
//---------------------------------------------------------------------------
void TTerminalItem::OperationProgress(
  TFileOperationProgressType & ProgressData, TCancelStatus & Cancel)
{
  if (FPause && !FTerminated && !FCancel)
  {
    assert(FItem != nullptr);
    TQueueItem::TStatus PrevStatus = FItem->GetStatus();
    assert(PrevStatus == TQueueItem::qsProcessing);
    // must be set before TFileOperationProgressType::Suspend(), because
    // it invokes this method back
    FPause = false;
    ProgressData.Suspend();

    SCOPE_EXIT
    {
      FItem->SetStatus(PrevStatus);
      ProgressData.Resume();
    };
    {
      FItem->SetStatus(TQueueItem::qsPaused);

      WaitForEvent();
    }
  }

  if (FTerminated || FCancel)
  {
    if (ProgressData.TransferingFile)
    {
      Cancel = csCancelTransfer;
    }
    else
    {
      Cancel = csCancel;
    }
  }

  assert(FItem != nullptr);
  FItem->SetProgress(ProgressData);
}
//---------------------------------------------------------------------------
bool TTerminalItem::OverrideItemStatus(TQueueItem::TStatus & ItemStatus)
{
  assert(FTerminal != nullptr);
  bool Result = (FTerminal->GetStatus() < ssOpened) && (ItemStatus == TQueueItem::qsProcessing);
  if (Result)
  {
    ItemStatus = TQueueItem::qsConnecting;
  }
  return Result;
}
//---------------------------------------------------------------------------
// TQueueItem
//---------------------------------------------------------------------------
TQueueItem::TQueueItem() :
  FStatus(qsPending), FSection(nullptr), FTerminalItem(nullptr), FProgressData(nullptr),
  FInfo(nullptr),
  FQueue(nullptr), FCompleteEvent(INVALID_HANDLE_VALUE),
  FCPSLimit(-1)
{
  FSection = new TCriticalSection();
  FInfo = new TInfo();
  FInfo->SingleFile = false;
}
//---------------------------------------------------------------------------
TQueueItem::~TQueueItem()
{
  // we need to keep the total transfer size even after transfer completes
  delete FProgressData;

  Complete();

  delete FSection;
  delete FInfo;
}
//---------------------------------------------------------------------------
void TQueueItem::Complete()
{
  TGuard Guard(FSection);

  if (FCompleteEvent != INVALID_HANDLE_VALUE)
  {
    SetEvent(FCompleteEvent);
    FCompleteEvent = INVALID_HANDLE_VALUE;
  }
}
//---------------------------------------------------------------------------
bool TQueueItem::IsUserActionStatus(TStatus Status)
{
  return (Status == qsQuery) || (Status == qsError) || (Status == qsPrompt);
}
//---------------------------------------------------------------------------
TQueueItem::TStatus TQueueItem::GetStatus()
{
  TGuard Guard(FSection);

  return FStatus;
}
//---------------------------------------------------------------------------
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

  assert((FQueue != nullptr) || (Status == qsPending));
  if (FQueue != nullptr)
  {
    FQueue->DoQueueItemUpdate(this);
  }
}
//---------------------------------------------------------------------------
void TQueueItem::SetProgress(
  TFileOperationProgressType & ProgressData)
{
  {
    TGuard Guard(FSection);

    assert(FProgressData != nullptr);
    *FProgressData = ProgressData;
    FProgressData->Reset();

    if (FCPSLimit >= 0)
    {
      ProgressData.CPSLimit = static_cast<unsigned long>(FCPSLimit);
      FCPSLimit = -1;
    }
  }
  FQueue->DoQueueItemUpdate(this);
}
//---------------------------------------------------------------------------
void TQueueItem::GetData(TQueueItemProxy * Proxy)
{
  TGuard Guard(FSection);

  assert(Proxy->FProgressData != nullptr);
  if (FProgressData != nullptr)
  {
    *Proxy->FProgressData = *FProgressData;
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
//---------------------------------------------------------------------------
void TQueueItem::Execute(TTerminalItem * TerminalItem)
{
  {
    assert(FProgressData == nullptr);
    TGuard Guard(FSection);
    FProgressData = new TFileOperationProgressType();
  }
  DoExecute(TerminalItem->FTerminal);
}
//---------------------------------------------------------------------------
void TQueueItem::SetCPSLimit(unsigned long CPSLimit)
{
  FCPSLimit = static_cast<long>(CPSLimit);
}
//---------------------------------------------------------------------------
// TQueueItemProxy
//---------------------------------------------------------------------------
TQueueItemProxy::TQueueItemProxy(TTerminalQueue * Queue,
  TQueueItem * QueueItem) :
  FProgressData(nullptr), FStatus(TQueueItem::qsPending), FQueue(Queue), FQueueItem(QueueItem),
  FQueueStatus(nullptr), FInfo(nullptr),
  FProcessingUserAction(false), FUserData(nullptr)
{
  FProgressData = new TFileOperationProgressType();
  FInfo = new TQueueItem::TInfo();

  Update();
}
//---------------------------------------------------------------------------
TQueueItemProxy::~TQueueItemProxy()
{
  delete FProgressData;
  delete FInfo;
}
//---------------------------------------------------------------------------
TFileOperationProgressType * TQueueItemProxy::GetProgressData()
{
  return (FProgressData->Operation == foNone) ? nullptr : FProgressData;
}
//---------------------------------------------------------------------------
__int64 TQueueItemProxy::GetTotalTransferred()
{
  // want to show total transferred also for "completed" items,
  // for which GetProgressData() is nullptr
  return
    (FProgressData->Operation == GetInfo()->Operation) || (GetStatus() == TQueueItem::qsDone) ?
      FProgressData->TotalTransfered : -1;
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Update()
{
  assert(FQueueItem != nullptr);

  TQueueItem::TStatus PrevStatus = GetStatus();

  bool Result = FQueue->ItemGetData(FQueueItem, this);

  if ((FQueueStatus != nullptr) && (PrevStatus != GetStatus()))
  {
    FQueueStatus->ResetStats();
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::ExecuteNow()
{
  return FQueue->ItemExecuteNow(FQueueItem);
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Move(bool Sooner)
{
  bool Result = false;
  intptr_t I = GetIndex();
  if (Sooner)
  {
    if (I > 0)
    {
      Result = Move(FQueueStatus->GetItem(I - 1));
    }
  }
  else
  {
    if (I < FQueueStatus->GetCount() - 1)
    {
      Result = FQueueStatus->GetItem(I + 1)->Move(this);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Move(TQueueItemProxy * BeforeItem)
{
  return FQueue->ItemMove(FQueueItem, BeforeItem->FQueueItem);
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Delete()
{
  return FQueue->ItemDelete(FQueueItem);
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Pause()
{
  return FQueue->ItemPause(FQueueItem, true);
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Resume()
{
  return FQueue->ItemPause(FQueueItem, false);
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::ProcessUserAction()
{
  assert(FQueueItem != nullptr);

  bool Result = false;
  FProcessingUserAction = true;
  SCOPE_EXIT
  {
    FProcessingUserAction = false;
  };
  {
    Result = FQueue->ItemProcessUserAction(FQueueItem, nullptr);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::SetCPSLimit(unsigned long CPSLimit)
{
  return FQueue->ItemSetCPSLimit(FQueueItem, CPSLimit);
}
//---------------------------------------------------------------------------
intptr_t TQueueItemProxy::GetIndex()
{
  assert(FQueueStatus != nullptr);
  intptr_t Index = FQueueStatus->FList->IndexOf(this);
  assert(Index >= 0);
  return Index;
}
//---------------------------------------------------------------------------
// TTerminalQueueStatus
//---------------------------------------------------------------------------
TTerminalQueueStatus::TTerminalQueueStatus() :
  FList(nullptr),
  FDoneCount(0),
  FActiveCount(0)
{
  FList = new TList();
  ResetStats();
}
//---------------------------------------------------------------------------
TTerminalQueueStatus::~TTerminalQueueStatus()
{
  for (intptr_t Index = 0; Index < FList->GetCount(); ++Index)
  {
    delete GetItem(Index);
  }
  delete FList;
  FList = nullptr;
}
//---------------------------------------------------------------------------
void TTerminalQueueStatus::ResetStats()
{
  FActiveCount = -1;
}
//---------------------------------------------------------------------------
void TTerminalQueueStatus::SetDoneCount(intptr_t Value)
{
  FDoneCount = Value;
  ResetStats();
}
//---------------------------------------------------------------------------
intptr_t TTerminalQueueStatus::GetActiveCount() const
{
  if (FActiveCount < 0)
  {
    FActiveCount = 0;

    intptr_t Index = FDoneCount;
    while ((Index < FList->GetCount()) &&
      (GetItem(Index)->GetStatus() != TQueueItem::qsPending))
    {
      FActiveCount++;
      Index++;
    }
  }

  return FActiveCount;
}
//---------------------------------------------------------------------------
intptr_t TTerminalQueueStatus::GetDoneAndActiveCount() const
{
  return GetDoneCount() + GetActiveCount();
}
//---------------------------------------------------------------------------
intptr_t TTerminalQueueStatus::GetActiveAndPendingCount() const
{
  return GetCount() - GetDoneCount();
}
//---------------------------------------------------------------------------
void TTerminalQueueStatus::Add(TQueueItemProxy * ItemProxy)
{
  ItemProxy->FQueueStatus = this;
  FList->Add(ItemProxy);
  ResetStats();
}
//---------------------------------------------------------------------------
void TTerminalQueueStatus::Delete(TQueueItemProxy * ItemProxy)
{
  FList->Extract(ItemProxy);
  ItemProxy->FQueueStatus = nullptr;
  ResetStats();
}
//---------------------------------------------------------------------------
intptr_t TTerminalQueueStatus::GetCount() const
{
  return FList->GetCount();
}
//---------------------------------------------------------------------------
TQueueItemProxy * TTerminalQueueStatus::GetItem(intptr_t Index) const
{
  return const_cast<TTerminalQueueStatus *>(this)->GetItem(Index);
}
//---------------------------------------------------------------------------
TQueueItemProxy * TTerminalQueueStatus::GetItem(intptr_t Index)
{
  return reinterpret_cast<TQueueItemProxy *>(FList->GetItem(Index));
}
//---------------------------------------------------------------------------
TQueueItemProxy * TTerminalQueueStatus::FindByQueueItem(
  TQueueItem * QueueItem)
{
  for (intptr_t Index = 0; Index < FList->GetCount(); ++Index)
  {
    TQueueItemProxy * Item = GetItem(Index);
    if (Item->FQueueItem == QueueItem)
    {
      return Item;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
// TLocatedQueueItem
//---------------------------------------------------------------------------
TLocatedQueueItem::TLocatedQueueItem(TTerminal * Terminal) :
  TQueueItem()
{
  assert(Terminal != nullptr);
  FCurrentDir = Terminal->GetCurrentDirectory();
}
//---------------------------------------------------------------------------
UnicodeString TLocatedQueueItem::StartupDirectory()
{
  return FCurrentDir;
}
//---------------------------------------------------------------------------
void TLocatedQueueItem::DoExecute(TTerminal * Terminal)
{
  assert(Terminal != nullptr);
  Terminal->SetCurrentDirectory(FCurrentDir);
}
//---------------------------------------------------------------------------
// TTransferQueueItem
//---------------------------------------------------------------------------
TTransferQueueItem::TTransferQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, TOperationSide Side,
  bool SingleFile) :
  TLocatedQueueItem(Terminal), FFilesToCopy(nullptr), FCopyParam(nullptr)
{
  FInfo->Operation = (Params & cpDelete ? foMove : foCopy);
  FInfo->Side = Side;
  FInfo->SingleFile = SingleFile;

  assert(FilesToCopy != nullptr);
  FFilesToCopy = new TStringList();
  for (intptr_t Index = 0; Index < FilesToCopy->GetCount(); ++Index)
  {
    FFilesToCopy->AddObject(FilesToCopy->GetString(Index),
      ((FilesToCopy->GetObject(Index) == nullptr) || (Side == osLocal)) ? nullptr :
        dynamic_cast<TRemoteFile *>(FilesToCopy->GetObject(Index))->Duplicate());
  }

  FTargetDir = TargetDir;

  assert(CopyParam != nullptr);
  FCopyParam = new TCopyParamType(*CopyParam);

  FParams = Params;
}
//---------------------------------------------------------------------------
TTransferQueueItem::~TTransferQueueItem()
{
  for (intptr_t Index = 0; Index < FFilesToCopy->GetCount(); ++Index)
  {
    delete FFilesToCopy->GetObject(Index);
  }
  delete FFilesToCopy;
  delete FCopyParam;
}
//---------------------------------------------------------------------------
// TUploadQueueItem
//---------------------------------------------------------------------------
TUploadQueueItem::TUploadQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile) :
  TTransferQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, osLocal, SingleFile)
{
  if (FilesToCopy->GetCount() > 1)
  {
    if (FLAGSET(Params, cpTemporary))
    {
      FInfo->Source = "";
      FInfo->ModifiedLocal = "";
    }
    else
    {
      ExtractCommonPath(FilesToCopy, FInfo->Source);
      // this way the trailing backslash is preserved for root directories like "D:\\"
      FInfo->Source = ExtractFileDir(IncludeTrailingBackslash(FInfo->Source));
      FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        IncludeTrailingBackslash(FInfo->Source);
    }
  }
  else
  {
    if (FLAGSET(Params, cpTemporary))
    {
      FInfo->Source = ::ExtractFileName(FilesToCopy->GetString(0), true);
      FInfo->ModifiedLocal = L"";
    }
    else
    {
      assert(FilesToCopy->GetCount() > 0);
      FInfo->Source = FilesToCopy->GetString(0);
      FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        IncludeTrailingBackslash(ExtractFilePath(FInfo->Source));
    }
  }

  FInfo->Destination =
    UnixIncludeTrailingBackslash(TargetDir) + CopyParam->GetFileMask();
  FInfo->ModifiedRemote = UnixIncludeTrailingBackslash(TargetDir);
}
//---------------------------------------------------------------------------
void TUploadQueueItem::DoExecute(TTerminal * Terminal)
{
  TTransferQueueItem::DoExecute(Terminal);

  assert(Terminal != nullptr);
  Terminal->CopyToRemote(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
//---------------------------------------------------------------------------
// TDownloadQueueItem
//---------------------------------------------------------------------------
TDownloadQueueItem::TDownloadQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile) :
  TTransferQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, osRemote, SingleFile)
{
  if (FilesToCopy->GetCount() > 1)
  {
    if (!UnixExtractCommonPath(FilesToCopy, FInfo->Source))
    {
      FInfo->Source = Terminal->GetCurrentDirectory();
    }
    FInfo->Source = UnixExcludeTrailingBackslash(FInfo->Source);
    FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
      UnixIncludeTrailingBackslash(FInfo->Source);
  }
  else
  {
    assert(FilesToCopy->GetCount() > 0);
    FInfo->Source = FilesToCopy->GetString(0);
    if (::UnixExtractFilePath(FInfo->Source).IsEmpty())
    {
      FInfo->Source = UnixIncludeTrailingBackslash(Terminal->GetCurrentDirectory()) +
        FInfo->Source;
      FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        UnixIncludeTrailingBackslash(Terminal->GetCurrentDirectory());
    }
    else
    {
      FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        ::UnixExtractFilePath(FInfo->Source);
    }
  }

  if (FLAGSET(Params, cpTemporary))
  {
    FInfo->Destination = L"";
  }
  else
  {
    FInfo->Destination =
      IncludeTrailingBackslash(TargetDir) + CopyParam->GetFileMask();
  }
  FInfo->ModifiedLocal = IncludeTrailingBackslash(TargetDir);
}
//---------------------------------------------------------------------------
void TDownloadQueueItem::DoExecute(TTerminal * Terminal)
{
  TTransferQueueItem::DoExecute(Terminal);

  assert(Terminal != nullptr);
  Terminal->CopyToLocal(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
//---------------------------------------------------------------------------
// TTerminalThread
//---------------------------------------------------------------------------
TTerminalThread::TTerminalThread(TTerminal * Terminal) :
  TSignalThread(), FTerminal(Terminal)
{
  FAction = nullptr;
  FActionEvent = CreateEvent(nullptr, false, false, nullptr);
  FException = nullptr;
  FIdleException = nullptr;
  FOnIdle = nullptr;
  FUserAction = nullptr;
  FCancel = false;
  FCancelled = false;
  FPendingIdle = false;
  FMainThread = GetCurrentThreadId();
  FSection = new TCriticalSection();
}

void TTerminalThread::Init()
{
  TSignalThread::Init(false);

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

  FTerminal->SetOnInformation(MAKE_CALLBACK(TTerminalThread::TerminalInformation, this));
  FTerminal->SetOnQueryUser(MAKE_CALLBACK(TTerminalThread::TerminalQueryUser, this));
  FTerminal->SetOnPromptUser(MAKE_CALLBACK(TTerminalThread::TerminalPromptUser, this));
  FTerminal->SetOnShowExtendedException(MAKE_CALLBACK(TTerminalThread::TerminalShowExtendedException, this));
  FTerminal->SetOnDisplayBanner(MAKE_CALLBACK(TTerminalThread::TerminalDisplayBanner, this));
  FTerminal->SetOnChangeDirectory(MAKE_CALLBACK(TTerminalThread::TerminalChangeDirectory, this));
  FTerminal->SetOnReadDirectory(MAKE_CALLBACK(TTerminalThread::TerminalReadDirectory, this));
  FTerminal->SetOnStartReadDirectory(MAKE_CALLBACK(TTerminalThread::TerminalStartReadDirectory, this));
  FTerminal->SetOnReadDirectoryProgress(MAKE_CALLBACK(TTerminalThread::TerminalReadDirectoryProgress, this));
  FTerminal->SetOnInitializeLog(MAKE_CALLBACK(TTerminalThread::TerminalInitializeLog, this));

  Start();
}
//---------------------------------------------------------------------------
TTerminalThread::~TTerminalThread()
{
  Close();

  ::CloseHandle(FActionEvent);

  assert(FTerminal->GetOnInformation() == MAKE_CALLBACK(TTerminalThread::TerminalInformation, this));
  assert(FTerminal->GetOnQueryUser() == MAKE_CALLBACK(TTerminalThread::TerminalQueryUser, this));
  assert(FTerminal->GetOnPromptUser() == MAKE_CALLBACK(TTerminalThread::TerminalPromptUser, this));
  assert(FTerminal->GetOnShowExtendedException() == MAKE_CALLBACK(TTerminalThread::TerminalShowExtendedException, this));
  assert(FTerminal->GetOnDisplayBanner() == MAKE_CALLBACK(TTerminalThread::TerminalDisplayBanner, this));
  assert(FTerminal->GetOnChangeDirectory() == MAKE_CALLBACK(TTerminalThread::TerminalChangeDirectory, this));
  assert(FTerminal->GetOnReadDirectory() == MAKE_CALLBACK(TTerminalThread::TerminalReadDirectory, this));
  assert(FTerminal->GetOnStartReadDirectory() == MAKE_CALLBACK(TTerminalThread::TerminalStartReadDirectory, this));
  assert(FTerminal->GetOnReadDirectoryProgress() == MAKE_CALLBACK(TTerminalThread::TerminalReadDirectoryProgress, this));
  assert(FTerminal->GetOnInitializeLog() == MAKE_CALLBACK(TTerminalThread::TerminalInitializeLog, this));

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

  delete FSection;
}
//---------------------------------------------------------------------------
void TTerminalThread::Cancel()
{
  FCancel = true;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TTerminalThread::TerminalOpen()
{
  RunAction(MAKE_CALLBACK(TTerminalThread::TerminalOpenEvent, this));
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalReopen()
{
  RunAction(MAKE_CALLBACK(TTerminalThread::TerminalReopenEvent, this));
}
//---------------------------------------------------------------------------
void TTerminalThread::RunAction(TNotifyEvent Action)
{
  assert(FAction == nullptr);
  assert(FException == nullptr);
  assert(FIdleException == nullptr);
  assert(FOnIdle != nullptr);

  FCancelled = false;
  FAction = Action;
  try
  {
    SCOPE_EXIT
    {
      FAction = nullptr;
      SAFE_DESTROY(FException);
    };
    {
      TriggerEvent();

      bool Done = false;

      do
      {
        switch (WaitForSingleObject(FActionEvent, 50))
        {
          case WAIT_OBJECT_0:
            Done = true;
            break;

          case WAIT_TIMEOUT:
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
            }
            else
            {
              if (FOnIdle != nullptr)
              {
                FOnIdle(nullptr);
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
  }
  catch (...)
  {
    if (FCancelled)
    {
      // even if the abort thrown as result of Cancel() was wrapper into
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
//---------------------------------------------------------------------------
void TTerminalThread::TerminalOpenEvent(TObject * /*Sender*/)
{
  FTerminal->Open();
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalReopenEvent(TObject * /*Sender*/)
{
  FTerminal->Reopen(0);
}
//---------------------------------------------------------------------------
void TTerminalThread::ProcessEvent()
{
  assert(FEvent != nullptr);
  assert(FException == nullptr);

  try
  {
    FAction(nullptr);
  }
  catch(Exception & E)
  {
    SaveException(E, FException);
  }

  SetEvent(FActionEvent);
}
//---------------------------------------------------------------------------
void TTerminalThread::Rethrow(Exception *& Exception)
{
  if (Exception != nullptr)
  {
    SCOPE_EXIT
    {
      SAFE_DESTROY(Exception);
    };
    {
      RethrowException(Exception);
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalThread::SaveException(Exception & E, Exception *& Exception)
{
  assert(Exception == nullptr);

  Exception = CloneException(&E);
}
//---------------------------------------------------------------------------
void TTerminalThread::FatalAbort()
{
  FTerminal->FatalAbort();
}
//---------------------------------------------------------------------------
void TTerminalThread::CheckCancel()
{
  if (FCancel && !FCancelled)
  {
    FCancelled = true;
    FatalAbort();
  }
}
//---------------------------------------------------------------------------
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
    assert(Thread == FThreadId);

    bool DoCheckCancel =
      ALWAYS_FALSE(UserAction == nullptr) || !UserAction->Force();
    if (DoCheckCancel)
    {
      CheckCancel();
    }

    // have to save it as we can go recursive via TQueryParams::TimerEvent,
    // see TTerminalThread::TerminalQueryUser
    TUserAction * PrevUserAction = FUserAction;
    SCOPE_EXIT
    {
      FUserAction = PrevUserAction;
      SAFE_DESTROY(FException);
    };
    {
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

        int WaitResult = WaitForEvent(1000);
        if (WaitResult == 0)
        {
          SAFE_DESTROY(FIdleException);
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

    if (DoCheckCancel)
    {
      CheckCancel();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & Str, bool Status, intptr_t Phase)
{
  TInformationUserAction Action(FOnInformation);
  Action.Terminal = Terminal;
  Action.Str = Str;
  Action.Status = Status;
  Action.Phase = Phase;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalQueryUser(TObject * Sender,
  const UnicodeString & Query, TStrings * MoreMessages, uintptr_t Answers,
  const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg)
{
  USEDPARAM(Arg);
  assert(Arg == nullptr);

  // note about TQueryParams::TimerEvent
  // So far there is only one use for this, the TSecureShell::SendBuffer,
  // which should be thread-safe, as long as the terminal thread,
  // is stopped waiting for OnQueryUser to finish.

  // note about TQueryButtonAlias::OnClick
  // So far there is only one use for this, the TClipboardHandler,
  // which is thread-safe.

  TQueryUserAction Action(FOnQueryUser);
  Action.Sender = Sender;
  Action.Query = Query;
  Action.MoreMessages = MoreMessages;
  Action.Answers = Answers;
  Action.Params = Params;
  Action.Answer = Answer;
  Action.Type = Type;

  WaitForUserAction(&Action);

  Answer = Action.Answer;
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalInitializeLog(TObject * Sender)
{
  if (FOnInitializeLog != nullptr)
  {
    // never used, so not tested either
    FAIL;
    TNotifyAction Action(FOnInitializeLog);
    Action.Sender = Sender;

    WaitForUserAction(&Action);
  }
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
  TStrings * Results, bool & Result, void * Arg)
{
  USEDPARAM(Arg);
  assert(Arg == nullptr);

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
//---------------------------------------------------------------------------
void TTerminalThread::TerminalShowExtendedException(
  TTerminal * Terminal, Exception * E, void * Arg)
{
  USEDPARAM(Arg);
  assert(Arg == nullptr);

  TShowExtendedExceptionAction Action(FOnShowExtendedException);
  Action.Terminal = Terminal;
  Action.E = E;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalDisplayBanner(TTerminal * Terminal,
  const UnicodeString & SessionName, const UnicodeString & Banner,
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
//---------------------------------------------------------------------------
void TTerminalThread::TerminalChangeDirectory(TObject * Sender)
{
  TNotifyAction Action(FOnChangeDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly)
{
  TReadDirectoryAction Action(FOnReadDirectory);
  Action.Sender = Sender;
  Action.ReloadOnly = ReloadOnly;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalStartReadDirectory(TObject * Sender)
{
  TNotifyAction Action(FOnStartReadDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalReadDirectoryProgress(
  TObject * Sender, intptr_t Progress, bool & Cancel)
{
  TReadDirectoryProgressAction Action(FOnReadDirectoryProgress);
  Action.Sender = Sender;
  Action.Progress = Progress;
  Action.Cancel = Cancel;

  WaitForUserAction(&Action);

  Cancel = Action.Cancel;
}
//---------------------------------------------------------------------------
