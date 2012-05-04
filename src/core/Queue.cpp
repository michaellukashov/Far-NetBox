//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/noncopyable.hpp>
#endif

#include "Common.h"
#include "Terminal.h"
#include "Queue.h"
#include "Exceptions.h"
#include "CoreMain.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
class TBackgroundTerminal;
//---------------------------------------------------------------------------
class TUserAction : private boost::noncopyable
{
public:
  explicit /* __fastcall */ TUserAction() {}
  virtual /* __fastcall */ ~TUserAction() {}
  virtual void __fastcall Execute(void * Arg) = 0;
};
//---------------------------------------------------------------------------
class TNotifyAction : public TUserAction
{
public:
  explicit /* __fastcall */ TNotifyAction(TNotifyEvent AOnNotify) :
    Sender(NULL)
  {
    OnNotify.connect(AOnNotify);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnNotify.empty())
    {
      OnNotify(Sender);
    }
  }

  notify_signal_type OnNotify;
  TObject * Sender;
private:
  TNotifyAction(const TNotifyAction &);
  TNotifyAction & operator = (const TNotifyAction &);
};
//---------------------------------------------------------------------------
class TInformationUserAction : public TUserAction
{
public:
  explicit /* __fastcall */ TInformationUserAction(TInformationEvent AOnInformation) :
    Terminal(NULL),
    Status(false),
    Phase(0)
  {
    OnInformation.connect(AOnInformation);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnInformation.empty())
    {
      OnInformation(Terminal, Str, Status, Phase);
    }
  }

  informationevent_signal_type OnInformation;
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
  explicit /* __fastcall */ TQueryUserAction(TQueryUserEvent AOnQueryUser) :
    Sender(NULL),
    MoreMessages(NULL),
    Params(NULL),
    Answer(0),
    Type(qtConfirmation)
  {
    OnQueryUser.connect(AOnQueryUser);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnQueryUser.empty())
    {
      OnQueryUser(Sender, Query, MoreMessages, Answers, Params, Answer, Type, Arg);
    }
  }

  queryuser_signal_type OnQueryUser;
  TObject * Sender;
  UnicodeString Query;
  TStrings * MoreMessages;
  unsigned int Answers;
  const TQueryParams * Params;
  unsigned int Answer;
  TQueryType Type;
private:
  TQueryUserAction(const TQueryUserAction &);
  TQueryUserAction & operator = (const TQueryUserAction &);
};
//---------------------------------------------------------------------------
class TPromptUserAction : public TUserAction
{
public:
  explicit /* __fastcall */ TPromptUserAction(TPromptUserEvent AOnPromptUser) :
    Terminal(NULL),
    Kind(pkPrompt),
    Prompts(NULL),
    Results(new TStringList())
  {
    OnPromptUser.connect(AOnPromptUser);
  }

  virtual /* __fastcall */ ~TPromptUserAction()
  {
    delete Results;
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnPromptUser.empty())
    {
      OnPromptUser(Terminal, Kind, Name, Instructions, Prompts, Results, Result, Arg);
    }
  }

  promptuser_signal_type OnPromptUser;
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
  explicit /* __fastcall */ TShowExtendedExceptionAction(TExtendedExceptionEvent AOnShowExtendedException) :
    Terminal(NULL),
    E(NULL)
  {
    OnShowExtendedException.connect(AOnShowExtendedException);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnShowExtendedException.empty())
    {
      OnShowExtendedException(Terminal, E, Arg);
    }
  }

  extendedexception_signal_type OnShowExtendedException;
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
  explicit /* __fastcall */ TDisplayBannerAction(TDisplayBannerEvent AOnDisplayBanner) :
    Terminal(NULL),
    NeverShowAgain(false),
    Options(0)
  {
    OnDisplayBanner.connect(AOnDisplayBanner);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnDisplayBanner.empty())
    {
      OnDisplayBanner(Terminal, SessionName, Banner, NeverShowAgain, Options);
    }
  }

  displaybanner_signal_type OnDisplayBanner;
  TTerminal * Terminal;
  UnicodeString SessionName;
  UnicodeString Banner;
  bool NeverShowAgain;
  int Options;
private:
  TDisplayBannerAction(const TDisplayBannerAction &);
  TDisplayBannerAction & operator = (const TDisplayBannerAction &);
};
//---------------------------------------------------------------------------
class TReadDirectoryAction : public TUserAction
{
public:
  explicit /* __fastcall */ TReadDirectoryAction(TReadDirectoryEvent AOnReadDirectory) :
    Sender(NULL),
    ReloadOnly(false)
  {
    OnReadDirectory.connect(AOnReadDirectory);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnReadDirectory.empty())
    {
      OnReadDirectory(Sender, ReloadOnly);
    }
  }

  readdirectory_signal_type OnReadDirectory;
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
  /* __fastcall */ TReadDirectoryProgressAction(TReadDirectoryProgressEvent AOnReadDirectoryProgress) :
    Sender(NULL),
    Progress(0),
    Cancel(false)
  {
    OnReadDirectoryProgress.connect(AOnReadDirectoryProgress);
  }

  virtual void __fastcall Execute(void * Arg)
  {
    if (!OnReadDirectoryProgress.empty())
    {
      OnReadDirectoryProgress(Sender, Progress, Cancel);
    }
  }

  readdirectoryprogress_signal_type OnReadDirectoryProgress;
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
  explicit /* __fastcall */ TTerminalItem(TTerminalQueue * Queue);
  virtual void __fastcall Init(int Index);
  virtual /* __fastcall */ ~TTerminalItem();

  void __fastcall Process(TQueueItem * Item);
  bool __fastcall ProcessUserAction(void * Arg);
  void __fastcall Cancel();
  void __fastcall Idle();
  bool __fastcall Pause();
  bool __fastcall Resume();

protected:
  TTerminalQueue * FQueue;
  TBackgroundTerminal * FTerminal;
  TQueueItem * FItem;
  TCriticalSection * FCriticalSection;
  TUserAction * FUserAction;
  bool FCancel;
  bool FPause;
  TTerminalItem * Self;

  virtual void __fastcall ProcessEvent();
  virtual void __fastcall Finished();
  bool __fastcall WaitForUserAction(TQueueItem::TStatus ItemStatus, TUserAction * UserAction);
  bool __fastcall OverrideItemStatus(TQueueItem::TStatus & ItemStatus);

  void /* __fastcall */ TerminalQueryUser(TObject * Sender,
    const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void /* __fastcall */ TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void /* __fastcall */ TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void /* __fastcall */ OperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void /* __fastcall */ OperationProgress(TFileOperationProgressType & ProgressData,
    TCancelStatus & Cancel);
};
//---------------------------------------------------------------------------
// TSignalThread
//---------------------------------------------------------------------------
int __fastcall TSimpleThread::ThreadProc(void * Thread)
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
/* __fastcall */ TSimpleThread::TSimpleThread() :
  FThread(NULL), FFinished(true)
{
#ifndef _MSC_VER
  unsigned ThreadID;
  FThread = reinterpret_cast<HANDLE>(
    StartThread(NULL, 0, ThreadProc, this, CREATE_SUSPENDED, ThreadID));
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSimpleThread::Init()
{
  unsigned int ThreadID;
  FThread = reinterpret_cast<HANDLE>(
    StartThread(NULL, 0, this, CREATE_SUSPENDED, ThreadID));
}

//---------------------------------------------------------------------------
/* __fastcall */ TSimpleThread::~TSimpleThread()
{
  Close();

  if (FThread != NULL)
  {
    CloseHandle(FThread);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSimpleThread::IsFinished()
{
  return FFinished;
}
//---------------------------------------------------------------------------
void __fastcall TSimpleThread::Start()
{
  if (ResumeThread(FThread) == 1)
  {
    FFinished = false;
  }
}
//---------------------------------------------------------------------------
void __fastcall TSimpleThread::Finished()
{
}
//---------------------------------------------------------------------------
void __fastcall TSimpleThread::Close()
{
  if (!FFinished)
  {
    Terminate();
    WaitFor();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSimpleThread::WaitFor(unsigned int Milliseconds)
{
  WaitForSingleObject(FThread, Milliseconds);
}
//---------------------------------------------------------------------------
// TSignalThread
//---------------------------------------------------------------------------
/* __fastcall */ TSignalThread::TSignalThread() :
  TSimpleThread(),
  FTerminated(true), FEvent(NULL)
{
#ifndef _MSC_VER
  FEvent = CreateEvent(NULL, false, false, NULL);
  assert(FEvent != NULL);

  if (LowPriority)
  {
    ::SetThreadPriority(FThread, THREAD_PRIORITY_BELOW_NORMAL);
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSignalThread::Init(bool LowPriority)
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
/* __fastcall */ TSignalThread::~TSignalThread()
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
void __fastcall TSignalThread::Start()
{
  FTerminated = false;
  TSimpleThread::Start();
}
//---------------------------------------------------------------------------
void __fastcall TSignalThread::TriggerEvent()
{
  SetEvent(FEvent);
}
//---------------------------------------------------------------------------
bool __fastcall TSignalThread::WaitForEvent()
{
  return (WaitForSingleObject(FEvent, INFINITE) == WAIT_OBJECT_0) &&
    !FTerminated;
}
//---------------------------------------------------------------------------
void __fastcall TSignalThread::Execute()
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
void __fastcall TSignalThread::Terminate()
{
  FTerminated = true;
  TriggerEvent();
}
//---------------------------------------------------------------------------
// TTerminalQueue
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalQueue::TTerminalQueue(TTerminal * Terminal,
  TConfiguration * Configuration) :
  TSignalThread(),
  FTerminal(Terminal), FTransfersLimit(2), FEnabled(true),
  FConfiguration(Configuration), FSessionData(NULL), FItems(NULL),
  FTerminals(NULL), FItemsSection(NULL), FFreeTerminals(0),
  FItemsInProcess(0), FTemporaryTerminals(0), FOverallTerminals(0)
{
#ifndef _MSC_VER
  FOnQueryUser = NULL;
  FOnPromptUser = NULL;
  FOnShowExtendedException = NULL;
  FOnQueueItemUpdate = NULL;
  FOnListUpdate = NULL;
  FOnEvent = NULL;
  FLastIdle = Now();
  FIdleInterval = EncodeTimeVerbose(0, 0, 2, 0);

  assert(Terminal != NULL);
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(Terminal->SessionData);

  FItems = new TList();
  FTerminals = new TList();
  FForcedItems = new TList();

  FItemsSection = new TCriticalSection();

  Start();
#endif
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::Init()
{
  TSignalThread::Init(true);
#ifndef _MSC_VER
  FOnQueryUser = NULL;
  FOnPromptUser = NULL;
  FOnShowExtendedException = NULL;
  FOnQueueItemUpdate = NULL;
  FOnListUpdate = NULL;
  FOnEvent = NULL;
#endif
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
/* __fastcall */ TTerminalQueue::~TTerminalQueue()
{
  Close();

  {
    TGuard Guard(FItemsSection);

    TTerminalItem * TerminalItem;
    while (FTerminals->GetCount() > 0)
    {
      TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->GetItem(0));
      FTerminals->Delete(0);
      TerminalItem->Terminate();
      TerminalItem->WaitFor();
      delete TerminalItem;
    }
    delete FTerminals;
    delete FForcedItems;

    for (int Index = 0; Index < FItems->GetCount(); Index++)
    {
      delete GetItem(Index);
    }
    delete FItems;
  }

  delete FItemsSection;
  delete FSessionData;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::TerminalFinished(TTerminalItem * TerminalItem)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      int Index = FTerminals->IndexOf(static_cast<TObject *>(TerminalItem));
      assert(Index >= 0);

      if (Index < FFreeTerminals)
      {
        FFreeTerminals--;
      }

      // Index may be >= FTransfersLimit also when the transfer limit was
      // recently decresed, then
      // FTemporaryTerminals < FTerminals->GetCount() - FTransfersLimit
      if ((FTransfersLimit >= 0) && (Index >= FTransfersLimit) && (FTemporaryTerminals > 0))
      {
        FTemporaryTerminals--;
      }

      FTerminals->Extract(static_cast<TObject *>(TerminalItem));

      delete TerminalItem;
    }

    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TTerminalQueue::TerminalFree(TTerminalItem * TerminalItem)
{
  bool Result = true;

  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      int Index = FTerminals->IndexOf(static_cast<TObject *>(TerminalItem));
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
void __fastcall TTerminalQueue::AddItem(TQueueItem * Item)
{
  assert(!FTerminated);

  Item->SetStatus(TQueueItem::qsPending);

  {
    TGuard Guard(FItemsSection);

    FItems->Add(static_cast<TObject *>(Item));
    Item->FQueue = this;
  }

  DoListUpdate();

  TriggerEvent();
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::RetryItem(TQueueItem * Item)
{
  if (!FTerminated)
  {
    {
      TGuard Guard(FItemsSection);

      int Index = FItems->Remove(static_cast<TObject *>(Item));
      assert(Index < FItemsInProcess);
      USEDPARAM(Index);
      FItemsInProcess--;
      FItems->Add(static_cast<TObject *>(Item));
    }

    DoListUpdate();

    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::DeleteItem(TQueueItem * Item)
{
  if (!FTerminated)
  {
    bool Empty;
    bool Monitored;
    {
      TGuard Guard(FItemsSection);

      // does this need to be within guard?
      Monitored = (Item->GetCompleteEvent() != INVALID_HANDLE_VALUE);
      int Index = FItems->Remove(static_cast<TObject *>(Item));
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
TQueueItem * __fastcall TTerminalQueue::GetItem(int Index)
{
  return reinterpret_cast<TQueueItem *>(FItems->GetItem(Index));
}
//---------------------------------------------------------------------------
TTerminalQueueStatus * __fastcall TTerminalQueue::CreateStatus(TTerminalQueueStatus * Current)
{
  TTerminalQueueStatus * Status = new TTerminalQueueStatus();
  try
  {
    // try
    {
      BOOST_SCOPE_EXIT ( (&Current) )
      {
        if (Current != NULL)
        {
          delete Current;
        }
      } BOOST_SCOPE_EXIT_END
      TGuard Guard(FItemsSection);

      TQueueItem * Item;
      TQueueItemProxy * ItemProxy;
      for (int Index = 0; Index < FItems->GetCount(); Index++)
      {
        Item = GetItem(Index);
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
#ifndef _MSC_VER
    __finally
    {
      if (Current != NULL)
      {
        delete Current;
      }
    }
#endif
  }
  catch(...)
  {
    delete Status;
    throw;
  }

  return Status;
}
//---------------------------------------------------------------------------
bool __fastcall TTerminalQueue::ItemGetData(TQueueItem * Item,
  TQueueItemProxy * Proxy)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection);

    Result = (FItems->IndexOf(static_cast<TObject *>(Item)) >= 0);
    if (Result)
    {
      Item->GetData(Proxy);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TTerminalQueue::ItemProcessUserAction(TQueueItem * Item, void * Arg)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TTerminalItem * TerminalItem = NULL;

    {
      TGuard Guard(FItemsSection);

      Result = (FItems->IndexOf(static_cast<TObject *>(Item)) >= 0) &&
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
bool __fastcall TTerminalQueue::ItemMove(TQueueItem * Item, TQueueItem * BeforeItem)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    {
      TGuard Guard(FItemsSection);

      int Index = FItems->IndexOf(static_cast<TObject *>(Item));
      int IndexDest = FItems->IndexOf(static_cast<TObject *>(BeforeItem));
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
bool __fastcall TTerminalQueue::ItemExecuteNow(TQueueItem * Item)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    {
      TGuard Guard(FItemsSection);

      int Index = FItems->IndexOf(static_cast<TObject *>(Item));
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
bool __fastcall TTerminalQueue::ItemDelete(TQueueItem * Item)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    bool UpdateList = false;

    {
      TGuard Guard(FItemsSection);

      int Index = FItems->IndexOf(static_cast<TObject *>(Item));
      Result = (Index >= 0);
      if (Result)
      {
        if (Item->GetStatus() == TQueueItem::qsPending)
        {
          FItems->Delete(Index);
          FForcedItems->Remove(Item);
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
bool __fastcall TTerminalQueue::ItemPause(TQueueItem * Item, bool Pause)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TTerminalItem * TerminalItem = NULL;

    {
      TGuard Guard(FItemsSection);

      Result = (FItems->IndexOf(static_cast<TObject *>(Item)) >= 0) &&
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
bool __fastcall TTerminalQueue::ItemSetCPSLimit(TQueueItem * Item, unsigned long CPSLimit)
{
  // to prevent deadlocks when closing queue from other thread
  bool Result = !FFinished;
  if (Result)
  {
    TGuard Guard(FItemsSection);

    Result = (FItems->IndexOf(static_cast<TObject *>(Item)) >= 0);
    if (Result)
    {
      Item->SetCPSLimit(CPSLimit);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::Idle()
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
        TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->GetItem(FFreeTerminals - 1));
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
void __fastcall TTerminalQueue::ProcessEvent()
{
  TTerminalItem * TerminalItem;
  TQueueItem * Item;

  do
  {
    TerminalItem = NULL;
    Item = NULL;

    if (FItems->GetCount() > FItemsInProcess)
    {
      TGuard Guard(FItemsSection);

      Item = GetItem(FItemsInProcess);
      int ForcedIndex = FForcedItems->IndexOf(Item);

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
          TerminalItem = reinterpret_cast<TTerminalItem*>(FTerminals->GetItem(0));
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
void __fastcall TTerminalQueue::DoQueueItemUpdate(TQueueItem * Item)
{
  if (!GetOnQueueItemUpdate().empty())
  {
    GetOnQueueItemUpdate()(this, Item);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::DoListUpdate()
{
  if (!GetOnListUpdate().empty())
  {
    GetOnListUpdate()(this);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::DoEvent(TQueueEvent Event)
{
  if (!GetOnEvent().empty())
  {
    GetOnEvent()(this, Event);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::SetTransfersLimit(int value)
{
  if (FTransfersLimit != value)
  {
    {
      TGuard Guard(FItemsSection);

      if ((value >= 0) && (value < FItemsInProcess))
      {
        FTemporaryTerminals = (FItemsInProcess - value);
      }
      else
      {
        FTemporaryTerminals = 0;
      }
      FTransfersLimit = value;
    }

    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueue::SetEnabled(bool value)
{
  if (FEnabled != value)
  {
    {
      TGuard Guard(FItemsSection);

      FEnabled = value;
    }

    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TTerminalQueue::GetIsEmpty()
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
  explicit /* __fastcall */ TBackgroundTerminal(TTerminal * MainTerminal);
  virtual void __fastcall Init(
    TSessionData * SessionData, TConfiguration * Configuration,
    TTerminalItem * Item, const UnicodeString & Name);
  virtual /* __fastcall */ ~TBackgroundTerminal() {}
protected:
  virtual bool __fastcall DoQueryReopen(Exception * E);

private:
  TTerminalItem * FItem;
};
//---------------------------------------------------------------------------
/* __fastcall */ TBackgroundTerminal::TBackgroundTerminal(TTerminal * MainTerminal) :
  TSecondaryTerminal(MainTerminal)
{
}
void __fastcall TBackgroundTerminal::Init(TSessionData * SessionData, TConfiguration * Configuration, TTerminalItem * Item,
    const UnicodeString & Name)
{
  TSecondaryTerminal::Init(SessionData, Configuration, Name);
}
//---------------------------------------------------------------------------
bool __fastcall TBackgroundTerminal::DoQueryReopen(Exception * /*E*/)
{
  bool Result;
  if (FItem->FTerminated || FItem->FCancel)
  {
    // avoid reconnection if we are closing
    Result = false;
  }
  else
  {
    Sleep(Configuration->GetSessionReopenBackground());
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
// TTerminalItem
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalItem::TTerminalItem(TTerminalQueue * Queue) :
  TSignalThread(), FQueue(Queue), FTerminal(NULL), FItem(NULL),
  FCriticalSection(NULL), FUserAction(NULL)
{
}
//---------------------------------------------------------------------------
void __fastcall TTerminalItem::Init(int Index)
{
  TSignalThread::Init(true);

  FCriticalSection = new TCriticalSection();
  Self = this;

  FTerminal = new TBackgroundTerminal(FQueue->FTerminal);
  FTerminal->Init(FQueue->FSessionData, FQueue->FConfiguration, this, FORMAT(L"Background %d", Index));
  try
  {
    FTerminal->SetUseBusyCursor(false);
    FTerminal->SetOnQueryUser(boost::bind(&TTerminalItem::TerminalQueryUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
    FTerminal->SetOnPromptUser(boost::bind(&TTerminalItem::TerminalPromptUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
    FTerminal->SetOnShowExtendedException(boost::bind(&TTerminalItem::TerminalShowExtendedException, this, _1, _2, _3));
    FTerminal->SetOnProgress(boost::bind(&TTerminalItem::OperationProgress, this, _1, _2));
    FTerminal->SetOnFinished(boost::bind(&TTerminalItem::OperationFinished, this, _1, _2, _3, _4, _5, _6));
  }
  catch(...)
  {
    delete FTerminal;
    throw;
  }

  Start();
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalItem::~TTerminalItem()
{
  Close();

  assert(FItem == NULL);
  delete FTerminal;
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalItem::Process(TQueueItem * Item)
{
  {
    TGuard Guard(FCriticalSection);

    assert(FItem == NULL);
    FItem = Item;
  }

  TriggerEvent();
}
//---------------------------------------------------------------------------
void __fastcall TTerminalItem::ProcessEvent()
{
  TGuard Guard(FCriticalSection);

  bool Retry = true;

  FCancel = false;
  FPause = false;
  FItem->FTerminalItem = this;

  try
  {
    assert(FItem != NULL);

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
      // do not show error messages, if task was cancelled anyway
      // (for example if transfer is cancelled during reconnection attempts)
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
void __fastcall TTerminalItem::Idle()
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
void __fastcall TTerminalItem::Cancel()
{
  FCancel = true;
  if ((FItem->GetStatus() == TQueueItem::qsPaused) ||
      TQueueItem::IsUserActionStatus(FItem->GetStatus()))
  {
    TriggerEvent();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TTerminalItem::Pause()
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
bool __fastcall TTerminalItem::Resume()
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
bool __fastcall TTerminalItem::ProcessUserAction(void * Arg)
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
bool __fastcall TTerminalItem::WaitForUserAction(
  TQueueItem::TStatus ItemStatus, TUserAction * UserAction)
{
  assert(FItem != NULL);
  assert((FItem->GetStatus() == TQueueItem::qsProcessing) ||
    (FItem->GetStatus() == TQueueItem::qsConnecting));

  bool Result;

  TQueueItem::TStatus PrevStatus = FItem->GetStatus();

  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&PrevStatus) )
    {
      Self->FUserAction = NULL;
      Self->FItem->SetStatus(PrevStatus);
    } BOOST_SCOPE_EXIT_END
    FUserAction = UserAction;

    FItem->SetStatus(ItemStatus);
    FQueue->DoEvent(qePendingUserAction);

    Result = !FTerminated && WaitForEvent() && !FCancel;
  }
#ifndef _MSC_VER
  __finally
  {
    FUserAction = NULL;
    FItem->SetStatus(PrevStatus);
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalItem::Finished()
{
  TSignalThread::Finished();

  FQueue->TerminalFinished(this);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalItem::TerminalQueryUser(TObject * Sender,
  const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
  const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg)
{
  // so far query without queue item can occur only for key cofirmation
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
void /* __fastcall */ TTerminalItem::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
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
void /* __fastcall */ TTerminalItem::TerminalShowExtendedException(
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
void /* __fastcall */ TTerminalItem::OperationFinished(TFileOperation /*Operation*/,
  TOperationSide /*Side*/, bool /*Temp*/, const UnicodeString & /*FileName*/,
  bool /*Success*/, TOnceDoneOperation & /*OnceDoneOperation*/)
{
  // nothing
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalItem::OperationProgress(
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

    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) (&PrevStatus) (&ProgressData) )
      {
        Self->FItem->SetStatus(PrevStatus);
        ProgressData.Resume();
      } BOOST_SCOPE_EXIT_END
      FItem->SetStatus(TQueueItem::qsPaused);

      WaitForEvent();
    }
#ifndef _MSC_VER
    __finally
    {
      FItem->SetStatus(PrevStatus);
      ProgressData.Resume();
    }
#endif
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
bool __fastcall TTerminalItem::OverrideItemStatus(TQueueItem::TStatus & ItemStatus)
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
/* __fastcall */ TQueueItem::TQueueItem() :
  FStatus(qsPending), FTerminalItem(NULL), FSection(NULL), FProgressData(NULL),
  FQueue(NULL), FInfo(NULL), FCompleteEvent(INVALID_HANDLE_VALUE),
  FCPSLimit(-1)
{
  FSection = new TCriticalSection();
  FInfo = new TInfo();
  Self = this;
}
//---------------------------------------------------------------------------
/* __fastcall */ TQueueItem::~TQueueItem()
{
  if (FCompleteEvent != INVALID_HANDLE_VALUE)
  {
    SetEvent(FCompleteEvent);
  }

  delete FSection;
  delete FInfo;
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItem::IsUserActionStatus(TStatus Status)
{
  return (Status == qsQuery) || (Status == qsError) || (Status == qsPrompt);
}
//---------------------------------------------------------------------------
TQueueItem::TStatus __fastcall TQueueItem::GetStatus()
{
  TGuard Guard(FSection);

  return FStatus;
}
//---------------------------------------------------------------------------
void __fastcall TQueueItem::SetStatus(TStatus Status)
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
void __fastcall TQueueItem::SetProgress(
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
void __fastcall TQueueItem::GetData(TQueueItemProxy * Proxy)
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
void __fastcall TQueueItem::Execute(TTerminalItem * TerminalItem)
{
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      TGuard Guard(Self->FSection);
      delete Self->FProgressData;
      Self->FProgressData = NULL;
    } BOOST_SCOPE_EXIT_END
    {
      assert(FProgressData == NULL);
      TGuard Guard(FSection);
      FProgressData = new TFileOperationProgressType();
    }
    DoExecute(TerminalItem->FTerminal);
  }
#ifndef _MSC_VER
  __finally
  {
    {
      TGuard Guard(FSection);
      delete FProgressData;
      FProgressData = NULL;
    }
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TQueueItem::SetCPSLimit(unsigned long CPSLimit)
{
  FCPSLimit = static_cast<long>(CPSLimit);
}
//---------------------------------------------------------------------------
// TQueueItemProxy
//---------------------------------------------------------------------------
/* __fastcall */ TQueueItemProxy::TQueueItemProxy(TTerminalQueue * Queue,
  TQueueItem * QueueItem) :
  FQueue(Queue), FQueueItem(QueueItem), FProgressData(NULL),
  FQueueStatus(NULL), FInfo(NULL),
  FProcessingUserAction(false), FUserData(NULL)
{
  FProgressData = new TFileOperationProgressType();
  FInfo = new TQueueItem::TInfo();
  Self = this;

  Update();
}
//---------------------------------------------------------------------------
/* __fastcall */ TQueueItemProxy::~TQueueItemProxy()
{
  delete FProgressData;
  delete FInfo;
}
//---------------------------------------------------------------------------
TFileOperationProgressType * __fastcall TQueueItemProxy::GetProgressData()
{
  return (FProgressData->Operation == foNone) ? NULL : FProgressData;
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::Update()
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
bool __fastcall TQueueItemProxy::ExecuteNow()
{
  return FQueue->ItemExecuteNow(FQueueItem);
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::Move(bool Sooner)
{
  bool Result = false;
  int I = GetIndex();
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
bool __fastcall TQueueItemProxy::Move(TQueueItemProxy * BeforeItem)
{
  return FQueue->ItemMove(FQueueItem, BeforeItem->FQueueItem);
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::Delete()
{
  return FQueue->ItemDelete(FQueueItem);
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::Pause()
{
  return FQueue->ItemPause(FQueueItem, true);
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::Resume()
{
  return FQueue->ItemPause(FQueueItem, false);
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::ProcessUserAction()
{
  assert(FQueueItem != NULL);

  bool Result;
  FProcessingUserAction = true;
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FProcessingUserAction = false;
    } BOOST_SCOPE_EXIT_END
    Result = FQueue->ItemProcessUserAction(FQueueItem, NULL);
  }
#ifndef _MSC_VER
  __finally
  {
    FProcessingUserAction = false;
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TQueueItemProxy::SetCPSLimit(unsigned long CPSLimit)
{
  return FQueue->ItemSetCPSLimit(FQueueItem, CPSLimit);
}
//---------------------------------------------------------------------------
int __fastcall TQueueItemProxy::GetIndex()
{
  assert(FQueueStatus != NULL);
  int Index = FQueueStatus->FList->IndexOf(static_cast<TObject *>(static_cast<void *>(this)));
  assert(Index >= 0);
  return Index;
}
//---------------------------------------------------------------------------
// TTerminalQueueStatus
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalQueueStatus::TTerminalQueueStatus() :
  FList(NULL)
{
  FList = new TList();
  ResetStats();
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalQueueStatus::~TTerminalQueueStatus()
{
  for (int Index = 0; Index < FList->GetCount(); Index++)
  {
    delete GetItem(Index);
  }
  delete FList;
  FList = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueueStatus::ResetStats()
{
  FActiveCount = NPOS;
}
//---------------------------------------------------------------------------
int __fastcall TTerminalQueueStatus::GetActiveCount()
{
  if (static_cast<int>(FActiveCount) < 0)
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
void __fastcall TTerminalQueueStatus::Add(TQueueItemProxy * ItemProxy)
{
  ItemProxy->FQueueStatus = this;
  FList->Add(static_cast<TObject *>(static_cast<void *>(ItemProxy)));
  ResetStats();
}
//---------------------------------------------------------------------------
void __fastcall TTerminalQueueStatus::Delete(TQueueItemProxy * ItemProxy)
{
  FList->Extract(static_cast<TObject *>(static_cast<void *>(ItemProxy)));
  ItemProxy->FQueueStatus = NULL;
  ResetStats();
}
//---------------------------------------------------------------------------
int __fastcall TTerminalQueueStatus::GetCount()
{
  return FList->GetCount();
}
//---------------------------------------------------------------------------
TQueueItemProxy * __fastcall TTerminalQueueStatus::GetItem(int Index)
{
  return reinterpret_cast<TQueueItemProxy *>(FList->GetItem(Index));
}
//---------------------------------------------------------------------------
TQueueItemProxy * __fastcall TTerminalQueueStatus::FindByQueueItem(
  TQueueItem * QueueItem)
{
  TQueueItemProxy * Item;
  for (int Index = 0; Index < FList->GetCount(); Index++)
  {
    Item = GetItem(Index);
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
/* __fastcall */ TLocatedQueueItem::TLocatedQueueItem(TTerminal * Terminal) :
  TQueueItem()
{
  assert(Terminal != NULL);
  FCurrentDir = Terminal->GetCurrentDirectory();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TLocatedQueueItem::StartupDirectory()
{
  return FCurrentDir;
}
//---------------------------------------------------------------------------
void __fastcall TLocatedQueueItem::DoExecute(TTerminal * Terminal)
{
  assert(Terminal != NULL);
  Terminal->GetCurrentDirectory() = FCurrentDir;
}
//---------------------------------------------------------------------------
// TTransferQueueItem
//---------------------------------------------------------------------------
/* __fastcall */ TTransferQueueItem::TTransferQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, int Params, TOperationSide Side) :
  TLocatedQueueItem(Terminal), FFilesToCopy(NULL), FCopyParam(NULL)
{
  FInfo->Operation = (Params & cpDelete ? foMove : foCopy);
  FInfo->Side = Side;

  assert(FilesToCopy != NULL);
  FFilesToCopy = new TStringList();
  for (int Index = 0; Index < FilesToCopy->GetCount(); Index++)
  {
    FFilesToCopy->AddObject(FilesToCopy->GetStrings(Index),
      ((FilesToCopy->GetObjects(Index) == NULL) || (Side == osLocal)) ? NULL :
        dynamic_cast<TRemoteFile *>(FilesToCopy->GetObjects(Index))->Duplicate());
  }

  FTargetDir = TargetDir;

  assert(CopyParam != NULL);
  FCopyParam = new TCopyParamType(*CopyParam);

  FParams = Params;
}
//---------------------------------------------------------------------------
/* __fastcall */ TTransferQueueItem::~TTransferQueueItem()
{
  for (int Index = 0; Index < FFilesToCopy->GetCount(); Index++)
  {
    delete FFilesToCopy->GetObjects(Index);
  }
  delete FFilesToCopy;
  delete FCopyParam;
}
//---------------------------------------------------------------------------
// TUploadQueueItem
//---------------------------------------------------------------------------
/* __fastcall */ TUploadQueueItem::TUploadQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, int Params) :
  TTransferQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, osLocal)
{
  if (FilesToCopy->GetCount() > 1)
  {
    if (FLAGSET(Params, cpTemporary))
    {
      FInfo->Source = L"";
      FInfo->ModifiedLocal = L"";
    }
    else
    {
      ExtractCommonPath(FilesToCopy, FInfo->Source);
      // this way the trailing backslash is preserved for root directories like D:
      FInfo->Source = ExtractFileDir(IncludeTrailingBackslash(FInfo->Source));
      FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        IncludeTrailingBackslash(FInfo->Source);
    }
  }
  else
  {
    if (FLAGSET(Params, cpTemporary))
    {
      FInfo->Source = ::ExtractFileName(FilesToCopy->GetStrings(0), true);
      FInfo->ModifiedLocal = L"";
    }
    else
    {
      assert(FilesToCopy->GetCount() > 0);
      FInfo->Source = FilesToCopy->GetStrings(0);
      FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? UnicodeString() :
        IncludeTrailingBackslash(ExtractFilePath(FInfo->Source));
    }
  }

  FInfo->Destination =
    UnixIncludeTrailingBackslash(TargetDir) + CopyParam->GetFileMask();
  FInfo->ModifiedRemote = UnixIncludeTrailingBackslash(TargetDir);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TUploadQueueItem::DoExecute(TTerminal * Terminal)
{
  TTransferQueueItem::DoExecute(Terminal);

  assert(Terminal != NULL);
  Terminal->CopyToRemote(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
//---------------------------------------------------------------------------
// TDownloadQueueItem
//---------------------------------------------------------------------------
/* __fastcall */ TDownloadQueueItem::TDownloadQueueItem(TTerminal * Terminal,
  TStrings * FilesToCopy, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, int Params) :
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
    FInfo->Source = FilesToCopy->GetStrings(0);
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
void __fastcall TDownloadQueueItem::DoExecute(TTerminal * Terminal)
{
  TTransferQueueItem::DoExecute(Terminal);

  assert(Terminal != NULL);
  Terminal->CopyToLocal(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}

//---------------------------------------------------------------------------
// TTerminalThread
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalThread::TTerminalThread(TTerminal * Terminal) :
  TSignalThread(), FTerminal(Terminal)
{
#ifndef _MSC_VER
  FAction = NULL;
  FActionEvent = CreateEvent(NULL, false, false, NULL);
  FException = NULL;
  FOnIdle = NULL;
  FUserAction = NULL;
  FCancel = false;

  FOnInformation = FTerminal->OnInformation;
  FOnQueryUser = FTerminal->OnQueryUser;
  FOnPromptUser = FTerminal->OnPromptUser;
  FOnShowExtendedException = FTerminal->OnShowExtendedException;
  FOnDisplayBanner = FTerminal->OnDisplayBanner;
  FOnChangeDirectory = FTerminal->OnChangeDirectory;
  FOnReadDirectory = FTerminal->OnReadDirectory;
  FOnStartReadDirectory = FTerminal->OnStartReadDirectory;
  FOnReadDirectoryProgress = FTerminal->OnReadDirectoryProgress;

  FTerminal->OnInformation = TerminalInformation;
  FTerminal->OnQueryUser = TerminalQueryUser;
  FTerminal->OnPromptUser = TerminalPromptUser;
  FTerminal->OnShowExtendedException = TerminalShowExtendedException;
  FTerminal->OnDisplayBanner = TerminalDisplayBanner;
  FTerminal->OnChangeDirectory = TerminalChangeDirectory;
  FTerminal->OnReadDirectory = TerminalReadDirectory;
  FTerminal->OnStartReadDirectory = TerminalStartReadDirectory;
  FTerminal->OnReadDirectoryProgress = TerminalReadDirectoryProgress;

  Start();
#endif
}

void __fastcall TTerminalThread::Init()
{
  Self = this;
  TSignalThread::Init(false);

#ifndef _MSC_VER
  FAction = NULL;
#endif
  FActionEvent = CreateEvent(NULL, false, false, NULL);
  FException = NULL;
#ifndef _MSC_VER
  FOnIdle = NULL;
#endif
  FUserAction = NULL;
  FCancel = false;

  FOnInformation = FTerminal->GetOnInformation();
  FOnQueryUser = FTerminal->GetOnQueryUser();
  FOnPromptUser = FTerminal->GetOnPromptUser();
  FOnShowExtendedException = FTerminal->GetOnShowExtendedException();
  FOnDisplayBanner = FTerminal->GetOnDisplayBanner();
  FOnChangeDirectory = FTerminal->GetOnChangeDirectory();
  FOnReadDirectory = FTerminal->GetOnReadDirectory();
  FOnStartReadDirectory = FTerminal->GetOnStartReadDirectory();
  FOnReadDirectoryProgress = FTerminal->GetOnReadDirectoryProgress();

  FTerminal->SetOnInformation(boost::bind(&TTerminalThread::TerminalInformation, this, _1, _2, _3, _4));
  FTerminal->SetOnQueryUser(boost::bind(&TTerminalThread::TerminalQueryUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
  FTerminal->SetOnPromptUser(boost::bind(&TTerminalThread::TerminalPromptUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
  FTerminal->SetOnShowExtendedException(boost::bind(&TTerminalThread::TerminalShowExtendedException, this, _1, _2, _3));
  FTerminal->SetOnDisplayBanner(boost::bind(&TTerminalThread::TerminalDisplayBanner, this, _1, _2, _3, _4, _5));
  FTerminal->SetOnChangeDirectory(boost::bind(&TTerminalThread::TerminalChangeDirectory, this, _1));
  FTerminal->SetOnReadDirectory(boost::bind(&TTerminalThread::TerminalReadDirectory, this, _1, _2));
  FTerminal->SetOnStartReadDirectory(boost::bind(&TTerminalThread::TerminalStartReadDirectory, this, _1));
  FTerminal->SetOnReadDirectoryProgress(boost::bind(&TTerminalThread::TerminalReadDirectoryProgress, this, _1, _2, _3));

  Start();
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalThread::~TTerminalThread()
{
  Close();

  CloseHandle(FActionEvent);

#ifndef _MSC_VER
  assert(FTerminal->OnInformation == TerminalInformation);
  assert(FTerminal->OnQueryUser == TerminalQueryUser);
  assert(FTerminal->OnPromptUser == TerminalPromptUser);
  assert(FTerminal->OnShowExtendedException == TerminalShowExtendedException);
  assert(FTerminal->OnDisplayBanner == TerminalDisplayBanner);
  assert(FTerminal->OnChangeDirectory == TerminalChangeDirectory);
  assert(FTerminal->OnReadDirectory == TerminalReadDirectory);
  assert(FTerminal->OnStartReadDirectory == TerminalStartReadDirectory);
  assert(FTerminal->OnReadDirectoryProgress == TerminalReadDirectoryProgress);

  FTerminal->OnInformation = FOnInformation;
  FTerminal->OnQueryUser = FOnQueryUser;
  FTerminal->OnPromptUser = FOnPromptUser;
  FTerminal->OnShowExtendedException = FOnShowExtendedException;
  FTerminal->OnDisplayBanner = FOnDisplayBanner;
  FTerminal->OnChangeDirectory = FOnChangeDirectory;
  FTerminal->OnReadDirectory = FOnReadDirectory;
  FTerminal->OnStartReadDirectory = FOnStartReadDirectory;
  FTerminal->OnReadDirectoryProgress = FOnReadDirectoryProgress;
#else
  FTerminal->SetOnInformation(FOnInformation);
  FTerminal->SetOnQueryUser(FOnQueryUser);
  FTerminal->SetOnPromptUser(FOnPromptUser);
  FTerminal->SetOnShowExtendedException(FOnShowExtendedException);
  FTerminal->SetOnDisplayBanner(FOnDisplayBanner);
  FTerminal->SetOnChangeDirectory(FOnChangeDirectory);
  FTerminal->SetOnReadDirectory(FOnReadDirectory);
  FTerminal->SetOnStartReadDirectory(FOnStartReadDirectory);
  FTerminal->SetOnReadDirectoryProgress(FOnReadDirectoryProgress);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::Cancel()
{
  FCancel = true;
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::TerminalOpen()
{
  RunAction(boost::bind(&TTerminalThread::TerminalOpenEvent, this, _1));
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::TerminalReopen()
{
  RunAction(boost::bind(&TTerminalThread::TerminalReopenEvent, this, _1));
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::RunAction(const TNotifyEvent & Action)
{
#ifndef _MSC_VER
  assert(FAction == NULL);
#endif
  assert(FException == NULL);
#ifndef _MSC_VER
  assert(FOnIdle != NULL);
#endif

  FCancelled = false;
  FAction.connect(Action);
  try
  {
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FAction.disconnect_all_slots();
        SAFE_DESTROY(Self->FException);
      } BOOST_SCOPE_EXIT_END
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
                SaveException(E);
              }

              FUserAction = NULL;
              TriggerEvent();
            }
            else
            {
              if (!FOnIdle.empty())
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


      Rethrow();
    }
#ifndef _MSC_VER
    __finally
    {
      FAction = NULL;
      SAFE_DESTROY(FException);
    }
#endif
  }
  catch(...)
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
void /* __fastcall */ TTerminalThread::TerminalOpenEvent(TObject * /*Sender*/)
{
  FTerminal->Open();
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalThread::TerminalReopenEvent(TObject * /*Sender*/)
{
  FTerminal->Reopen(0);
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::ProcessEvent()
{
  assert(FEvent != NULL);
  assert(FException == NULL);

  try
  {
    FAction(NULL);
  }
  catch(Exception & E)
  {
    SaveException(E);
  }

  SetEvent(FActionEvent);
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::Rethrow()
{
  if (FException != NULL)
  {
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        SAFE_DESTROY(Self->FException);
      } BOOST_SCOPE_EXIT_END
      if (dynamic_cast<EFatal *>(FException) != NULL)
      {
        throw EFatal(L"", FException);
      }
      else
      {
        throw ExtException(L"", FException);
      }
    }
#ifndef _MSC_VER
    __finally
    {
      SAFE_DESTROY(FException);
    }
#endif
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::SaveException(Exception & E)
{
  assert(FException == NULL);

  // should not happen
  assert(dynamic_cast<ESshTerminate *>(&E) == NULL);

  if (dynamic_cast<EFatal *>(&E) != NULL)
  {
    FException = new EFatal(L"", &E);
  }
  else
  {
    FException = new ExtException(L"", &E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::FatalAbort()
{
  FTerminal->FatalAbort();
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::CheckCancel()
{
  if (FCancel && !FCancelled)
  {
    FCancelled = true;
    FatalAbort();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminalThread::WaitForUserAction(TUserAction * UserAction)
{
  CheckCancel();

  // have to save it as we can go recursive via TQueryParams::TimerEvent,
  // see TTerminalThread::TerminalQueryUser
  TUserAction * PrevUserAction = FUserAction;
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&PrevUserAction) )
    {
      Self->FUserAction = PrevUserAction;
      SAFE_DESTROY(Self->FException);
    } BOOST_SCOPE_EXIT_END
    FUserAction = UserAction;

    if (!WaitForEvent())
    {
      FatalAbort();
    }


    Rethrow();
  }
#ifndef _MSC_VER
  __finally
  {
    FUserAction = PrevUserAction;
    SAFE_DESTROY(FException);
  }
#endif

  CheckCancel();
}
//---------------------------------------------------------------------------
void  /* __fastcall */ TTerminalThread::TerminalInformation(
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
void /* __fastcall */ TTerminalThread::TerminalQueryUser(TObject * Sender,
  const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
  const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg)
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
void /* __fastcall */ TTerminalThread::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
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
void /* __fastcall */ TTerminalThread::TerminalShowExtendedException(
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
void /* __fastcall */ TTerminalThread::TerminalDisplayBanner(TTerminal * Terminal,
  UnicodeString SessionName, const UnicodeString & Banner,
  bool & NeverShowAgain, int Options)
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
void /* __fastcall */ TTerminalThread::TerminalChangeDirectory(TObject * Sender)
{
  TNotifyAction Action(FOnChangeDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalThread::TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly)
{
  TReadDirectoryAction Action(FOnReadDirectory);
  Action.Sender = Sender;
  Action.ReloadOnly = ReloadOnly;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalThread::TerminalStartReadDirectory(TObject * Sender)
{
  TNotifyAction Action(FOnStartReadDirectory);
  Action.Sender = Sender;

  WaitForUserAction(&Action);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalThread::TerminalReadDirectoryProgress(
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
