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
private:
  TUserAction(const TUserAction &);
  TUserAction & operator = (const TUserAction &);
};
//---------------------------------------------------------------------------
class TNotifyAction : public TUserAction
{
public:
  explicit TNotifyAction(TNotifyEvent AOnNotify) :
    OnNotify(AOnNotify),
    Sender(NULL)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnNotify != NULL)
    {
      OnNotify(Sender);
    }
  }

  TNotifyEvent OnNotify;
  TObject * Sender;

private:
  TNotifyAction(const TNotifyAction &);
  TNotifyAction & operator = (const TNotifyAction &);
};
//---------------------------------------------------------------------------
class TInformationUserAction : public TUserAction
{
public:
  explicit TInformationUserAction(TInformationEvent AOnInformation) :
    OnInformation(AOnInformation),
    Terminal(NULL),
    Status(false),
    Phase(0)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnInformation != NULL)
    {
      OnInformation(Terminal, Str, Status, Phase);
    }
  }

  TInformationEvent OnInformation;
  TTerminal * Terminal;
  UnicodeString Str;
  bool Status;
  int Phase;
private:
  TInformationUserAction(const TInformationUserAction &);
  TInformationUserAction & operator = (const TInformationUserAction &);
};
//---------------------------------------------------------------------------
class TQueryUserAction : public TUserAction
{
public:
  explicit TQueryUserAction(TQueryUserEvent AOnQueryUser) :
    OnQueryUser(AOnQueryUser),
    Sender(NULL),
    MoreMessages(NULL),
    Answers(0),
    Params(NULL),
    Answer(0),
    Type(qtConfirmation)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnQueryUser != NULL)
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
  TQueryUserAction(const TQueryUserAction &);
  TQueryUserAction & operator = (const TQueryUserAction &);
};
//---------------------------------------------------------------------------
class TPromptUserAction : public TUserAction
{
public:
  explicit TPromptUserAction(TPromptUserEvent AOnPromptUser) :
    OnPromptUser(AOnPromptUser),
    Terminal(NULL),
    Kind(pkPrompt),
    Prompts(NULL),
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
    if (OnPromptUser != NULL)
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
  TPromptUserAction(const TPromptUserAction &);
  TPromptUserAction & operator = (const TPromptUserAction &);
};
//---------------------------------------------------------------------------
class TShowExtendedExceptionAction : public TUserAction
{
public:
  explicit TShowExtendedExceptionAction(TExtendedExceptionEvent AOnShowExtendedException) :
    OnShowExtendedException(AOnShowExtendedException),
    Terminal(NULL),
    E(NULL)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnShowExtendedException != NULL)
    {
      OnShowExtendedException(Terminal, E, Arg);
    }
  }

  TExtendedExceptionEvent OnShowExtendedException;
  TTerminal * Terminal;
  Exception * E;

private:
  TShowExtendedExceptionAction(const TShowExtendedExceptionAction &);
  TShowExtendedExceptionAction & operator = (const TShowExtendedExceptionAction &);
};
//---------------------------------------------------------------------------
class TDisplayBannerAction : public TUserAction
{
public:
  explicit TDisplayBannerAction(TDisplayBannerEvent AOnDisplayBanner) :
    OnDisplayBanner(AOnDisplayBanner),
    Terminal(NULL),
    NeverShowAgain(false),
    Options(0)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnDisplayBanner != NULL)
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
  TDisplayBannerAction(const TDisplayBannerAction &);
  TDisplayBannerAction & operator = (const TDisplayBannerAction &);
};
//---------------------------------------------------------------------------
class TReadDirectoryAction : public TUserAction
{
public:
  explicit TReadDirectoryAction(TReadDirectoryEvent AOnReadDirectory) :
    OnReadDirectory(AOnReadDirectory),
    Sender(NULL),
    ReloadOnly(false)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnReadDirectory != NULL)
    {
      OnReadDirectory(Sender, ReloadOnly);
    }
  }

  TReadDirectoryEvent OnReadDirectory;
  TObject * Sender;
  bool ReloadOnly;

private:
  TReadDirectoryAction(const TReadDirectoryAction &);
  TReadDirectoryAction & operator = (const TReadDirectoryAction &);
};
//---------------------------------------------------------------------------
class TReadDirectoryProgressAction : public TUserAction
{
public:
  explicit TReadDirectoryProgressAction(TReadDirectoryProgressEvent AOnReadDirectoryProgress) :
    OnReadDirectoryProgress(AOnReadDirectoryProgress),
    Sender(NULL),
    Progress(0),
    Cancel(false)
  {
  }

  virtual void Execute(void * Arg)
  {
    if (OnReadDirectoryProgress != NULL)
    {
      OnReadDirectoryProgress(Sender, Progress, Cancel);
    }
  }

  TReadDirectoryProgressEvent OnReadDirectoryProgress;
  TObject * Sender;
  int Progress;
  bool Cancel;

private:
  TReadDirectoryProgressAction(const TReadDirectoryProgressAction &);
  TReadDirectoryProgressAction & operator = (const TReadDirectoryProgressAction &);
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
};
//---------------------------------------------------------------------------
// TSignalThread
//---------------------------------------------------------------------------
int TSimpleThread::ThreadProc(void * Thread)
{
  TSimpleThread * SimpleThread = reinterpret_cast<TSimpleThread*>(Thread);
  assert(SimpleThread != NULL);
  try
  {
    SimpleThread->Execute();
  }
  catch(...)
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
  FThread(NULL), FFinished(true)
{
}
//---------------------------------------------------------------------------
void TSimpleThread::Init()
{
  FThread = StartThread(NULL, 0, this, CREATE_SUSPENDED, FThreadId);
}
//---------------------------------------------------------------------------
TSimpleThread::~TSimpleThread()
{
  Close();

  if (FThread != NULL)
  {
    CloseHandle(FThread);
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
  FEvent(NULL),
  FTerminated(true)
{
}
//---------------------------------------------------------------------------
void TSignalThread::Init(bool LowPriority)
{
  TSimpleThread::Init();
  FEvent = CreateEvent(NULL, false, false, NULL);
  assert(FEvent != NULL);

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
    CloseHandle(FEvent);
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
  if ((Result == WAIT_TIMEOUT) && !FTerminated)
  {
    return -1;
  }
  else
  {
    return ((Result == WAIT_OBJECT_0) && !FTerminated) ? 1 : 0;
  }
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
  FTerminal(Terminal), FConfiguration(Configuration), FSessionData(NULL),
  FItems(NULL), FItemsInProcess(0), FItemsSection(NULL),
  FFreeTerminals(0), FTerminals(NULL), FForcedItems(NULL), FTemporaryTerminals(0),
  FOverallTerminals(0), FTransfersLimit(2), FEnabled(true)
{
}
//---------------------------------------------------------------------------
void TTerminalQueue::Init()
{
  TSignalThread::Init(true);
  FOnQueryUser = NULL;
  FOnPromptUser = NULL;
  FOnShowExtendedException = NULL;
  FOnQueueItemUpdate = NULL;
  FOnListUpdate = NULL;
  FOnEvent = NULL;
  FLastIdle = Now();
  FIdleInterval = EncodeTimeVerbose(0, 0, 2, 0);

  assert(FTerminal != NULL);
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(FTerminal->GetSessionData());

  FItems = new TList();
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
      TTerminalItem * TerminalItem = reinterpret_cast<TTerminalItem *>(FTerminals->Items[0]);
      FTerminals->Delete(0);
      TerminalItem->Terminate();
      TerminalItem->WaitFor();
      delete TerminalItem;
    }
    delete FTerminals;
    delete FForcedItems;

    for (intptr_t Index = 0; Index < FItems->GetCount(); ++Index)
    {
      delete GetItem(Index);
    }
    delete FItems;
  }

  delete FItemsSection;
  delete FSessionData;
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
void TTerminalQueue::DeleteItem(TQueueItem * Item)
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
      delete Item;

      Empty = true;
      Index = 0;
      while (Empty && (Index < FItems->GetCount()))
      {
        Empty = (GetItem(Index)->GetCompleteEvent() != INVALID_HANDLE_VALUE);
        ++Index;
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
TQueueItem * TTerminalQueue::GetItem(intptr_t Index)
{
  return reinterpret_cast<TQueueItem*>(FItems->Items[Index]);
}
//---------------------------------------------------------------------------
TTerminalQueueStatus * TTerminalQueue::CreateStatus(TTerminalQueueStatus * Current)
{
  TTerminalQueueStatus * Status = new TTerminalQueueStatus();
  try
  {
    TRY_FINALLY (
    {
      TGuard Guard(FItemsSection);

      TQueueItemProxy * ItemProxy;
      for (intptr_t Index = 0; Index < FItems->GetCount(); ++Index)
      {
        TQueueItem * Item = GetItem(Index);
        if (Current != NULL)
        {
          ItemProxy = Current->FindByQueueItem(Item);
        }
        else
        {
          ItemProxy = NULL;
        }

        if (ItemProxy != NULL)
        {
          if (Current != NULL)
          {
            Current->Delete(ItemProxy);
          }
          Status->Add(ItemProxy);
          ItemProxy->Update();
        }
        else
        {
          Status->Add(new TQueueItemProxy(this, Item));
        }
      }
    }
    ,
    {
      if (Current != NULL)
      {
        delete Current;
      }
    }
    );
  }
  catch(...)
  {
    delete Status;
    throw;
  }

  return Status;
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

    Result = (FItems->IndexOf(Item) >= 0);
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
    TTerminalItem * TerminalItem = NULL;

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
    TTerminalItem * TerminalItem = NULL;

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
  if (Now() - FLastIdle > FIdleInterval)
  {
    FLastIdle = FIdleInterval;
    TTerminalItem * TerminalItem = NULL;

    if (FFreeTerminals > 0)
    {
      TGuard Guard(FItemsSection);

      if (FFreeTerminals > 0)
      {
        // take the last free terminal, because TerminalFree() puts it to the
        // front, this ensures we cycle thru all free terminals
        TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->Items[FFreeTerminals - 1]);
        FTerminals->Move(FFreeTerminals - 1, FTerminals->GetCount() - 1);
        FFreeTerminals--;
      }
    }

    if (TerminalItem != NULL)
    {
      TerminalItem->Idle();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalQueue::ProcessEvent()
{
  TTerminalItem * TerminalItem;
  do
  {
    TerminalItem = NULL;
    TQueueItem * Item = NULL;

    if (FItems->GetCount() > FItemsInProcess)
    {
      TGuard Guard(FItemsSection);

      Item = GetItem(FItemsInProcess);
      intptr_t ForcedIndex = FForcedItems->IndexOf(Item);

      if (FEnabled || (ForcedIndex >= 0))
      {
        if ((FFreeTerminals == 0) &&
            ((FTransfersLimit < 0) ||
             (FTerminals->GetCount() < FTransfersLimit + FTemporaryTerminals)))
        {
          FOverallTerminals++;
          TerminalItem = new TTerminalItem(this);
          TerminalItem->Init(FOverallTerminals);
          FTerminals->Add(TerminalItem);
        }
        else if (FFreeTerminals > 0)
        {
          TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->Items[0]);
          FTerminals->Move(0, FTerminals->GetCount() - 1);
          FFreeTerminals--;
        }

        if (TerminalItem != NULL)
        {
          if (ForcedIndex >= 0)
          {
            FForcedItems->Delete(ForcedIndex);
          }
          FItemsInProcess++;
        }
      }
    }

    if (TerminalItem != NULL)
    {
      TerminalItem->Process(Item);
    }
  }
  while (!FTerminated && (TerminalItem != NULL));
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoQueueItemUpdate(TQueueItem * Item)
{
  if (GetOnQueueItemUpdate() != NULL)
  {
    GetOnQueueItemUpdate()(this, Item);
  }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoListUpdate()
{
  if (GetOnListUpdate() != NULL)
  {
    GetOnListUpdate()(this);
  }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoEvent(TQueueEvent Event)
{
  if (GetOnEvent() != NULL)
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
  FItem(NULL)
{
}
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
    Sleep(GetConfiguration()->GetSessionReopenBackground());
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
// TTerminalItem
//---------------------------------------------------------------------------
TTerminalItem::TTerminalItem(TTerminalQueue * Queue) :
  TSignalThread(), FQueue(Queue), FTerminal(NULL), FItem(NULL),
  FCriticalSection(NULL), FUserAction(NULL), FCancel(false), FPause(false)
{
}
//---------------------------------------------------------------------------
void TTerminalItem::Init(intptr_t Index)
{
  TSignalThread::Init(true);

  FCriticalSection = new TCriticalSection();

  FTerminal = new TBackgroundTerminal(FQueue->FTerminal);
  FTerminal->Init(FQueue->FSessionData, FQueue->FConfiguration, this, FORMAT(L"Background %d", Index));
  try
  {
    FTerminal->SetUseBusyCursor(false);

    FTerminal->SetOnQueryUser(MAKE_CALLBACK(TTerminalItem::TerminalQueryUser, this));
    FTerminal->SetOnPromptUser(MAKE_CALLBACK(TTerminalItem::TerminalPromptUser, this));
    FTerminal->SetOnShowExtendedException(MAKE_CALLBACK(TTerminalItem::TerminalShowExtendedException, this));
    FTerminal->SetOnProgress(MAKE_CALLBACK(TTerminalItem::OperationProgress, this));
    FTerminal->SetOnFinished(MAKE_CALLBACK(TTerminalItem::OperationFinished, this));
  }
  catch(...)
  {
    delete FTerminal;
    throw;
  }

  Start();
}
//---------------------------------------------------------------------------
TTerminalItem::~TTerminalItem()
{
  Close();

  assert(FItem == NULL);
  delete FTerminal;
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TTerminalItem::Process(TQueueItem * Item)
{
  {
    TGuard Guard(FCriticalSection);

    assert(FItem == NULL);
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
    assert(FItem != NULL);

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
          (FTerminal->QueryUserException(L"", &E, qaOK | qaCancel, NULL, qtError) == qaCancel))
      {
        FCancel = true;
      }
    }
  }

  FItem->SetStatus(TQueueItem::qsDone);

  FItem->FTerminalItem = NULL;

  TQueueItem * Item = FItem;
  FItem = NULL;

  if (Retry && !FCancel)
  {
    FQueue->RetryItem(Item);
  }
  else
  {
    FQueue->DeleteItem(Item);
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
  catch(...)
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
  assert(FItem != NULL);
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
  assert(FItem != NULL);
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
  bool Result = (FUserAction != NULL);
  if (Result)
  {
    assert(FItem != NULL);

    FUserAction->Execute(Arg);
    FUserAction = NULL;

    TriggerEvent();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminalItem::WaitForUserAction(
  TQueueItem::TStatus ItemStatus, TUserAction * UserAction)
{
  assert(FItem != NULL);
  assert((FItem->GetStatus() == TQueueItem::qsProcessing) ||
    (FItem->GetStatus() == TQueueItem::qsConnecting));

  bool Result;

  TQueueItem::TStatus PrevStatus = FItem->GetStatus();

  TRY_FINALLY (
  {
    FUserAction = UserAction;

    FItem->SetStatus(ItemStatus);
    FQueue->DoEvent(qePendingUserAction);

    Result = !FTerminated && WaitForEvent() && !FCancel;
  }
  ,
  {
    FUserAction = NULL;
    FItem->SetStatus(PrevStatus);
  }
  );

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
  if (FItem != NULL)
  {
    USEDPARAM(Arg);
    assert(Arg == NULL);

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
  if (FItem == NULL)
  {
    // sanity, should not occur
    assert(false);
    Result = false;
  }
  else
  {
    USEDPARAM(Arg);
    assert(Arg == NULL);

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
  assert(Arg == NULL);

  UnicodeString Message; // not used
  if ((FItem != NULL) &&
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
    assert(FItem != NULL);
    TQueueItem::TStatus PrevStatus = FItem->GetStatus();
    assert(PrevStatus == TQueueItem::qsProcessing);
    // must be set before TFileOperationProgressType::Suspend(), because
    // it invokes this method back
    FPause = false;
    ProgressData.Suspend();

    TRY_FINALLY (
    {
      FItem->SetStatus(TQueueItem::qsPaused);

      WaitForEvent();
    }
    ,
    {
      FItem->SetStatus(PrevStatus);
      ProgressData.Resume();
    }
    );
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

  assert(FItem != NULL);
  FItem->SetProgress(ProgressData);
}
//---------------------------------------------------------------------------
bool TTerminalItem::OverrideItemStatus(TQueueItem::TStatus & ItemStatus)
{
  assert(FTerminal != NULL);
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
  FStatus(qsPending), FSection(NULL), FTerminalItem(NULL), FProgressData(NULL),
  FInfo(NULL),
  FQueue(NULL), FCompleteEvent(INVALID_HANDLE_VALUE),
  FCPSLimit(-1)
{
  FSection = new TCriticalSection();
  FInfo = new TInfo();
}
//---------------------------------------------------------------------------
TQueueItem::~TQueueItem()
{
  if (FCompleteEvent != INVALID_HANDLE_VALUE)
  {
    SetEvent(FCompleteEvent);
  }

  delete FSection;
  delete FInfo;
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
  }

  assert((FQueue != NULL) || (Status == qsPending));
  if (FQueue != NULL)
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

    assert(FProgressData != NULL);
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

  assert(Proxy->FProgressData != NULL);
  if (FProgressData != NULL)
  {
    *Proxy->FProgressData = *FProgressData;
  }
  else
  {
    Proxy->FProgressData->Clear();
  }
  *Proxy->FInfo = *FInfo;
  Proxy->FStatus = FStatus;
  if (FTerminalItem != NULL)
  {
    FTerminalItem->OverrideItemStatus(Proxy->FStatus);
  }
}
//---------------------------------------------------------------------------
void TQueueItem::Execute(TTerminalItem * TerminalItem)
{
  TRY_FINALLY (
  {
    {
      assert(FProgressData == NULL);
      TGuard Guard(FSection);
      FProgressData = new TFileOperationProgressType();
    }
    DoExecute(TerminalItem->FTerminal);
  }
  ,
  {
    {
      TGuard Guard(FSection);
      delete FProgressData;
      FProgressData = NULL;
    }
  }
  );
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
  FProgressData(NULL), FStatus(TQueueItem::qsPending), FQueue(Queue), FQueueItem(QueueItem),
  FQueueStatus(NULL), FInfo(NULL),
  FProcessingUserAction(false), FUserData(NULL)
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
  return (FProgressData->Operation == foNone) ? NULL : FProgressData;
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::Update()
{
  assert(FQueueItem != NULL);

  TQueueItem::TStatus PrevStatus = GetStatus();

  bool Result = FQueue->ItemGetData(FQueueItem, this);

  if ((FQueueStatus != NULL) && (PrevStatus != GetStatus()))
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
  assert(FQueueItem != NULL);

  bool Result = false;
  FProcessingUserAction = true;
  TRY_FINALLY (
  {
    Result = FQueue->ItemProcessUserAction(FQueueItem, NULL);
  }
  ,
  {
    FProcessingUserAction = false;
  }
  );
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
  assert(FQueueStatus != NULL);
  intptr_t Index = FQueueStatus->FList->IndexOf(this);
  assert(Index >= 0);
  return Index;
}
//---------------------------------------------------------------------------
// TTerminalQueueStatus
//---------------------------------------------------------------------------
TTerminalQueueStatus::TTerminalQueueStatus() :
  FList(NULL)
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
  FList = NULL;
}
//---------------------------------------------------------------------------
void TTerminalQueueStatus::ResetStats()
{
  FActiveCount = -1;
}
//---------------------------------------------------------------------------
intptr_t TTerminalQueueStatus::GetActiveCount()
{
  if (FActiveCount < 0)
  {
    FActiveCount = 0;

    while ((FActiveCount < FList->GetCount()) &&
      (GetItem(FActiveCount)->GetStatus() != TQueueItem::qsPending))
    {
      FActiveCount++;
    }
  }

  return FActiveCount;
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
  ItemProxy->FQueueStatus = NULL;
  ResetStats();
}
//---------------------------------------------------------------------------
intptr_t TTerminalQueueStatus::GetCount() const
{
  return FList->GetCount();
}
//---------------------------------------------------------------------------
TQueueItemProxy * TTerminalQueueStatus::GetItem(intptr_t Index)
{
  return reinterpret_cast<TQueueItemProxy *>(FList->Items[Index]);
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
  return NULL;
}
//---------------------------------------------------------------------------
// TLocatedQueueItem
//---------------------------------------------------------------------------
TLocatedQueueItem::TLocatedQueueItem(TTerminal * Terminal) :
  TQueueItem()
{
  assert(Terminal != NULL);
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
  assert(Terminal != NULL);
  Terminal->SetCurrentDirectory(FCurrentDir);
}
//---------------------------------------------------------------------------
// TTransferQueueItem
//---------------------------------------------------------------------------
TTransferQueueItem::TTransferQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params, TOperationSide Side) :
  TLocatedQueueItem(Terminal), FFilesToCopy(NULL), FCopyParam(NULL)
{
  FInfo->Operation = (Params & cpDelete ? foMove : foCopy);
  FInfo->Side = Side;

  assert(FilesToCopy != NULL);
  FFilesToCopy = new TStringList();
  for (intptr_t Index = 0; Index < FilesToCopy->GetCount(); ++Index)
  {
    FFilesToCopy->AddObject(FilesToCopy->GetString(Index),
      ((FilesToCopy->Objects[Index] == NULL) || (Side == osLocal)) ? NULL :
        dynamic_cast<TRemoteFile *>(FilesToCopy->Objects[Index])->Duplicate());
  }

  FTargetDir = TargetDir;

  assert(CopyParam != NULL);
  FCopyParam = new TCopyParamType(*CopyParam);

  FParams = Params;
}
//---------------------------------------------------------------------------
TTransferQueueItem::~TTransferQueueItem()
{
  for (intptr_t Index = 0; Index < FFilesToCopy->GetCount(); ++Index)
  {
    delete FFilesToCopy->Objects[Index];
  }
  delete FFilesToCopy;
  delete FCopyParam;
}
//---------------------------------------------------------------------------
// TUploadQueueItem
//---------------------------------------------------------------------------
TUploadQueueItem::TUploadQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params) :
  TTransferQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, osLocal)
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

  assert(Terminal != NULL);
  Terminal->CopyToRemote(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
//---------------------------------------------------------------------------
// TDownloadQueueItem
//---------------------------------------------------------------------------
TDownloadQueueItem::TDownloadQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params) :
  TTransferQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, osRemote)
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
    if (UnixExtractFilePath(FInfo->Source).IsEmpty())
    {
      FInfo->Source = UnixIncludeTrailingBackslash(Terminal->GetCurrentDirectory()) +
        FInfo->Source;
      FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        UnixIncludeTrailingBackslash(Terminal->GetCurrentDirectory());
    }
    else
    {
      FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        UnixExtractFilePath(FInfo->Source);
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

  assert(Terminal != NULL);
  Terminal->CopyToLocal(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
//---------------------------------------------------------------------------
// TTerminalThread
//---------------------------------------------------------------------------
TTerminalThread::TTerminalThread(TTerminal * Terminal) :
  TSignalThread(), FTerminal(Terminal)
{
  FAction = NULL;
  FActionEvent = CreateEvent(NULL, false, false, NULL);
  FException = NULL;
  FIdleException = NULL;
  FOnIdle = NULL;
  FUserAction = NULL;
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

  FTerminal->SetOnInformation(MAKE_CALLBACK(TTerminalThread::TerminalInformation, this));
  FTerminal->SetOnQueryUser(MAKE_CALLBACK(TTerminalThread::TerminalQueryUser, this));
  FTerminal->SetOnPromptUser(MAKE_CALLBACK(TTerminalThread::TerminalPromptUser, this));
  FTerminal->SetOnShowExtendedException(MAKE_CALLBACK(TTerminalThread::TerminalShowExtendedException, this));
  FTerminal->SetOnDisplayBanner(MAKE_CALLBACK(TTerminalThread::TerminalDisplayBanner, this));
  FTerminal->SetOnChangeDirectory(MAKE_CALLBACK(TTerminalThread::TerminalChangeDirectory, this));
  FTerminal->SetOnReadDirectory(MAKE_CALLBACK(TTerminalThread::TerminalReadDirectory, this));
  FTerminal->SetOnStartReadDirectory(MAKE_CALLBACK(TTerminalThread::TerminalStartReadDirectory, this));
  FTerminal->SetOnReadDirectoryProgress(MAKE_CALLBACK(TTerminalThread::TerminalReadDirectoryProgress, this));

  Start();
}
//---------------------------------------------------------------------------
TTerminalThread::~TTerminalThread()
{
  Close();

  CloseHandle(FActionEvent);

  assert(FTerminal->GetOnInformation() == MAKE_CALLBACK(TTerminalThread::TerminalInformation, this));
  assert(FTerminal->GetOnQueryUser() == MAKE_CALLBACK(TTerminalThread::TerminalQueryUser, this));
  assert(FTerminal->GetOnPromptUser() == MAKE_CALLBACK(TTerminalThread::TerminalPromptUser, this));
  assert(FTerminal->GetOnShowExtendedException() == MAKE_CALLBACK(TTerminalThread::TerminalShowExtendedException, this));
  assert(FTerminal->GetOnDisplayBanner() == MAKE_CALLBACK(TTerminalThread::TerminalDisplayBanner, this));
  assert(FTerminal->GetOnChangeDirectory() == MAKE_CALLBACK(TTerminalThread::TerminalChangeDirectory, this));
  assert(FTerminal->GetOnReadDirectory() == MAKE_CALLBACK(TTerminalThread::TerminalReadDirectory, this));
  assert(FTerminal->GetOnStartReadDirectory() == MAKE_CALLBACK(TTerminalThread::TerminalStartReadDirectory, this));
  assert(FTerminal->GetOnReadDirectoryProgress() == MAKE_CALLBACK(TTerminalThread::TerminalReadDirectoryProgress, this));

  FTerminal->SetOnInformation(FOnInformation);
  FTerminal->SetOnQueryUser(FOnQueryUser);
  FTerminal->SetOnPromptUser(FOnPromptUser);
  FTerminal->SetOnShowExtendedException(FOnShowExtendedException);
  FTerminal->SetOnDisplayBanner(FOnDisplayBanner);
  FTerminal->SetOnChangeDirectory(FOnChangeDirectory);
  FTerminal->SetOnReadDirectory(FOnReadDirectory);
  FTerminal->SetOnStartReadDirectory(FOnStartReadDirectory);
  FTerminal->SetOnReadDirectoryProgress(FOnReadDirectoryProgress);

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
  if ((FUserAction != NULL) && (FIdleException != NULL))
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
  assert(FAction == NULL);
  assert(FException == NULL);
  assert(FIdleException == NULL);
  assert(FOnIdle != NULL);

  FCancelled = false;
  FAction = Action;
  try
  {
    TRY_FINALLY (
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
            if (FUserAction != NULL)
            {
              try
              {
                FUserAction->Execute(NULL);
              }
              catch (Exception & E)
              {
                SaveException(E, FException);
              }

              FUserAction = NULL;
              TriggerEvent();
            }
            else
            {
              if (FOnIdle != NULL)
              {
                FOnIdle(NULL);
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
    ,
    {
      FAction = NULL;
      SAFE_DESTROY(FException);
    }
    );
  }
  TRACE_CATCH_ALL
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
  assert(FEvent != NULL);
  assert(FException == NULL);

  try
  {
    FAction(NULL);
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
  if (Exception != NULL)
  {
    TRY_FINALLY (
    {
      RethrowException(Exception);
    }
    ,
    {
      SAFE_DESTROY(Exception);
    }
    );
  }
}
//---------------------------------------------------------------------------
void TTerminalThread::SaveException(Exception & E, Exception *& Exception)
{
  assert(Exception == NULL);

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
    if (UserAction != NULL)
    {
      UserAction->Execute(NULL);
    }
  }
  else
  {
    // we should be called from our thread only,
    // with exception noted above
    assert(Thread == FThreadId);
    CheckCancel();

    // have to save it as we can go recursive via TQueryParams::TimerEvent,
    // see TTerminalThread::TerminalQueryUser
    TUserAction * PrevUserAction = FUserAction;
    TRY_FINALLY (
    {
      FUserAction = UserAction;

      while (true)
      {
        TGuard Guard(FSection);
        // If idle exception is already set, we are only waiting
        // for the main thread to pick it up
        // (or at least to finish handling the user action, so
        // that we rethrow the idle exception below)
        // Also if idle exception is set, it is probable that terminal
        // is not active anyway.
        if (FTerminal->GetActive() && FPendingIdle && (FIdleException == NULL))
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

      if (FIdleException != NULL)
      {
        // idle exception was not used to cancel the user action
        // (if it where it would be already cloned into the FException above
        // and rethrown)
        Rethrow(FIdleException);
      }
    }
    ,
    {
      FUserAction = PrevUserAction;
      SAFE_DESTROY(FException);
    }
    );
    CheckCancel();
  }
}
//---------------------------------------------------------------------------
void TTerminalThread::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase)
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
  assert(Arg == NULL);

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
void TTerminalThread::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
  TStrings * Results, bool & Result, void * Arg)
{
  USEDPARAM(Arg);
  assert(Arg == NULL);

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
  assert(Arg == NULL);

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
  TObject * Sender, int Progress, bool & Cancel)
{
  TReadDirectoryProgressAction Action(FOnReadDirectoryProgress);
  Action.Sender = Sender;
  Action.Progress = Progress;
  Action.Cancel = Cancel;

  WaitForUserAction(&Action);

  Cancel = Action.Cancel;
}
//---------------------------------------------------------------------------
