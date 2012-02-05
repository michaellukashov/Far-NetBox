//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Terminal.h"
#include "Queue.h"
#include "Exceptions.h"
#include "CoreMain.h"
//---------------------------------------------------------------------------
class TBackgroundTerminal;
//---------------------------------------------------------------------------
class TUserAction
{
public:
    virtual ~TUserAction() {}
    virtual void Execute(TTerminalQueue *Queue, void *Arg) = 0;
};
//---------------------------------------------------------------------------
class TQueryUserAction : public TUserAction
{
public:
    virtual void Execute(TTerminalQueue *Queue, void *Arg)
    {
        Queue->DoQueryUser(Sender, Query, MoreMessages, Answers, Params, Answer, Type, Arg);
    }

    nb::TObject *Sender;
    std::wstring Query;
    nb::TStrings *MoreMessages;
    int Answers;
    const TQueryParams *Params;
    int Answer;
    TQueryType Type;
};
//---------------------------------------------------------------------------
class TPromptUserAction : public TUserAction
{
public:
    TPromptUserAction() :
        Results(new nb::TStringList())
    {
    }

    virtual ~TPromptUserAction()
    {
        delete Results;
    }

    virtual void Execute(TTerminalQueue *Queue, void *Arg)
    {
        Queue->DoPromptUser(Terminal, Kind, Name, Instructions, Prompts, Results, Result, Arg);
    }

    TTerminal *Terminal;
    TPromptKind Kind;
    std::wstring Name;
    std::wstring Instructions;
    nb::TStrings *Prompts;
    nb::TStrings *Results;
    bool Result;
};
//---------------------------------------------------------------------------
class TShowExtendedExceptionAction : public TUserAction
{
public:
    virtual void Execute(TTerminalQueue *Queue, void *Arg)
    {
        Queue->DoShowExtendedException(Terminal, E, Arg);
    }

    TTerminal *Terminal;
    const std::exception *E;
};
//---------------------------------------------------------------------------
class TTerminalItem : public TSignalThread
{
    friend class TQueueItem;
    friend class TBackgroundTerminal;

public:
    explicit TTerminalItem(TTerminalQueue *Queue, size_t Index);
    virtual ~TTerminalItem();

    virtual void Init();
    void Process(TQueueItem *Item);
    bool ProcessUserAction(void *Arg);
    void Cancel();
    void Idle();
    bool Pause();
    bool Resume();

protected:
    TTerminalQueue *FQueue;
    TBackgroundTerminal *FTerminal;
    TQueueItem *FItem;
    TCriticalSection *FCriticalSection;
    TUserAction *FUserAction;
    bool FCancel;
    bool FPause;
    size_t FIndex;
    TTerminalItem *Self;

    virtual void ProcessEvent();
    virtual void Finished();
    bool WaitForUserAction(TQueueItem::TStatus ItemStatus, TUserAction *UserAction);
    bool OverrideItemStatus(TQueueItem::TStatus &ItemStatus);

    void TerminalQueryUser(nb::TObject *Sender,
                           const std::wstring Query, nb::TStrings *MoreMessages, int Answers,
                           const TQueryParams *Params, int &Answer, TQueryType Type, void *Arg);
    void TerminalPromptUser(TTerminal *Terminal, TPromptKind Kind,
                            std::wstring Name, std::wstring Instructions,
                            nb::TStrings *Prompts, nb::TStrings *Results, bool &Result, void *Arg);
    void TerminalShowExtendedException(TTerminal *Terminal,
                                       const std::exception *E, void *Arg);
    void OperationFinished(TFileOperation Operation, TOperationSide Side,
                           bool Temp, const std::wstring FileName, bool Success,
                           TOnceDoneOperation &OnceDoneOperation);
    void OperationProgress(TFileOperationProgressType &ProgressData,
                           TCancelStatus &Cancel);
};
//---------------------------------------------------------------------------
// TSimpleThread
//---------------------------------------------------------------------------
int TSimpleThread::ThreadProc(void *Thread)
{
    DEBUG_PRINTF(L"begin");
    TSimpleThread *SimpleThread = reinterpret_cast<TSimpleThread *>(Thread);
    assert(SimpleThread != NULL);
    try
    {
        SimpleThread->Execute();
    }
    catch(...)
    {
        // we do not expect thread to be terminated with std::exception
        assert(false);
    }
    SimpleThread->FFinished = true;
    SimpleThread->Finished();
    DEBUG_PRINTF(L"end");
    return 0;
}
//---------------------------------------------------------------------------
TSimpleThread::TSimpleThread() :
    nb::TObject(),
    FThread(NULL), FFinished(true)
{
    // DEBUG_PRINTF(L"begin");
}
//---------------------------------------------------------------------------
TSimpleThread::~TSimpleThread()
{
    Close();

    if (FThread != NULL)
    {
        ::CloseHandle(FThread);
    }
}

void TSimpleThread::Init()
{
    DWORD ThreadID;
    FThread = reinterpret_cast<HANDLE>(StartThread(NULL, 0, this, CREATE_SUSPENDED, ThreadID));
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
    FTerminated(true), FEvent(NULL)
{
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
void TSignalThread::Init()
{
    TSimpleThread::Init();
    FEvent = CreateEvent(NULL, false, false, NULL);
    assert(FEvent != NULL);

    ::SetThreadPriority(FThread, THREAD_PRIORITY_BELOW_NORMAL);
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
    return (WaitForSingleObject(FEvent, INFINITE) == WAIT_OBJECT_0) &&
           !FTerminated;
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
TTerminalQueue::TTerminalQueue(TTerminal *Terminal,
                               TConfiguration *configuration) :
    FTerminal(Terminal), FTransfersLimit(2),
    FConfiguration(configuration), FSessionData(NULL), FItems(NULL),
    FTerminals(NULL), FItemsSection(NULL), FFreeTerminals(0),
    FItemsInProcess(0), FTemporaryTerminals(0), FOverallTerminals(0)
{
}
//---------------------------------------------------------------------------
TTerminalQueue::~TTerminalQueue()
{
    Close();

    {
        TGuard Guard(FItemsSection);

        TTerminalItem *TerminalItem;
        while (FTerminals->GetCount() > 0)
        {
            TerminalItem = reinterpret_cast<TTerminalItem *>(FTerminals->GetItem(0));
            FTerminals->Delete(0);
            TerminalItem->Terminate();
            TerminalItem->WaitFor();
            delete TerminalItem;
        }
        delete FTerminals;

        for (size_t Index = 0; Index < FItems->GetCount(); Index++)
        {
            delete GetItem(Index);
        }
        delete FItems;
    }

    delete FItemsSection;
    delete FSessionData;
}
//---------------------------------------------------------------------------
void TTerminalQueue::Init()
{
    TSignalThread::Init();

    FLastIdle = nb::Now();
    FIdleInterval = EncodeTimeVerbose(0, 0, 2, 0);

    assert(FTerminal != NULL);
    FSessionData = new TSessionData(L"");
    FSessionData->Assign(FTerminal->GetSessionData());

    FItems = new nb::TList();
    FTerminals = new nb::TList();

    FItemsSection = new TCriticalSection();
    Self = this;

    Start();
}
//---------------------------------------------------------------------------
void TTerminalQueue::TerminalFinished(TTerminalItem *TerminalItem)
{
    if (!FTerminated)
    {
        {
            TGuard Guard(FItemsSection);

            size_t Index = FTerminals->IndexOf(static_cast<nb::TObject *>(TerminalItem));
            assert(Index != -1);

            if (Index < FFreeTerminals)
            {
                FFreeTerminals--;
            }

            // Index may be >= FTransfersLimit also when the transfer limit was
            // recently decresed, then
            // FTemporaryTerminals < FTerminals->GetCount() - FTransfersLimit
            if ((FTransfersLimit > 0) && (Index >= FTransfersLimit) && (FTemporaryTerminals > 0))
            {
                FTemporaryTerminals--;
            }

            FTerminals->Extract(static_cast<nb::TObject *>(TerminalItem));

            delete TerminalItem;
        }

        TriggerEvent();
    }
}
//---------------------------------------------------------------------------
bool TTerminalQueue::TerminalFree(TTerminalItem *TerminalItem)
{
    bool Result = true;

    if (!FTerminated)
    {
        {
            TGuard Guard(FItemsSection);

            size_t Index = FTerminals->IndexOf(static_cast<nb::TObject *>(TerminalItem));
            assert(Index != -1);
            assert(Index >= FFreeTerminals);

            Result = (FTransfersLimit <= 0) || (Index < FTransfersLimit);
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
void TTerminalQueue::AddItem(TQueueItem *Item)
{
    assert(!FTerminated);

    Item->SetStatus(TQueueItem::qsPending);

    {
        TGuard Guard(FItemsSection);

        FItems->Add(static_cast<nb::TObject *>(Item));
        Item->FQueue = this;
    }

    DoListUpdate();

    TriggerEvent();
}
//---------------------------------------------------------------------------
void TTerminalQueue::RetryItem(TQueueItem *Item)
{
    if (!FTerminated)
    {
        {
            TGuard Guard(FItemsSection);

            size_t Index = FItems->Remove(static_cast<nb::TObject *>(Item));
            assert(Index < FItemsInProcess);
            USEDPARAM(Index);
            FItemsInProcess--;
            FItems->Add(static_cast<nb::TObject *>(Item));
        }

        DoListUpdate();

        TriggerEvent();
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DeleteItem(TQueueItem *Item)
{
    if (!FTerminated)
    {
        bool Empty;
        bool Monitored;
        {
            TGuard Guard(FItemsSection);

            // does this need to be within guard?
            Monitored = (Item->GetCompleteEvent() != INVALID_HANDLE_VALUE);
            size_t Index = FItems->Remove(static_cast<nb::TObject *>(Item));
            assert(Index < FItemsInProcess);
            USEDPARAM(Index);
            FItemsInProcess--;
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
TQueueItem *TTerminalQueue::GetItem(size_t Index)
{
    return reinterpret_cast<TQueueItem *>(FItems->GetItem(Index));
}
//---------------------------------------------------------------------------
TTerminalQueueStatus *TTerminalQueue::CreateStatus(TTerminalQueueStatus *Current)
{
    TTerminalQueueStatus *Status = new TTerminalQueueStatus();
    try
    {
        {
            BOOST_SCOPE_EXIT ( (&Current) )
            {
                if (Current != NULL)
                {
                    delete Current;
                }
            } BOOST_SCOPE_EXIT_END
            TGuard Guard(FItemsSection);

            TQueueItem *Item;
            TQueueItemProxy *ItemProxy;
            for (size_t Index = 0; Index < FItems->GetCount(); Index++)
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
    }
    catch(...)
    {
        delete Status;
        throw;
    }

    return Status;
}
//---------------------------------------------------------------------------
bool TTerminalQueue::ItemGetData(TQueueItem *Item,
                                 TQueueItemProxy *Proxy)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        TGuard Guard(FItemsSection);

        Result = (FItems->IndexOf(static_cast<nb::TObject *>(Item)) != -1);
        if (Result)
        {
            Item->GetData(Proxy);
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
bool TTerminalQueue::ItemProcessUserAction(TQueueItem *Item, void *Arg)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        TTerminalItem *TerminalItem = NULL;

        {
            TGuard Guard(FItemsSection);

            Result = (FItems->IndexOf(static_cast<nb::TObject *>(Item)) != -1) &&
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
bool TTerminalQueue::ItemMove(TQueueItem *Item, TQueueItem *BeforeItem)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        {
            TGuard Guard(FItemsSection);

            size_t Index = FItems->IndexOf(static_cast<nb::TObject *>(Item));
            size_t IndexDest = FItems->IndexOf(static_cast<nb::TObject *>(BeforeItem));
            Result = (Index != -1) && (IndexDest != -1) &&
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
bool TTerminalQueue::ItemExecuteNow(TQueueItem *Item)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        {
            TGuard Guard(FItemsSection);

            size_t Index = FItems->IndexOf(static_cast<nb::TObject *>(Item));
            Result = (Index != -1) && (Item->GetStatus() == TQueueItem::qsPending) &&
                     // prevent double-initiation when "execute" is clicked twice too fast
                     (Index >= FItemsInProcess);
            if (Result)
            {
                if (Index > FItemsInProcess)
                {
                    FItems->Move(Index, FItemsInProcess);
                }

                if ((FTransfersLimit > 0) && (FTerminals->GetCount() >= static_cast<size_t>(FTransfersLimit)))
                {
                    FTemporaryTerminals++;
                }
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
bool TTerminalQueue::ItemDelete(TQueueItem *Item)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        bool UpdateList = false;

        {
            TGuard Guard(FItemsSection);

            size_t Index = FItems->IndexOf(static_cast<nb::TObject *>(Item));
            Result = (Index != -1);
            if (Result)
            {
                if (Item->GetStatus() == TQueueItem::qsPending)
                {
                    FItems->Delete(Index);
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
bool TTerminalQueue::ItemPause(TQueueItem *Item, bool Pause)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        TTerminalItem *TerminalItem = NULL;

        {
            TGuard Guard(FItemsSection);

            Result = (FItems->IndexOf(static_cast<nb::TObject *>(Item)) != -1) &&
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
bool TTerminalQueue::ItemSetCPSLimit(TQueueItem *Item, unsigned long CPSLimit)
{
    // to prevent deadlocks when closing queue from other thread
    bool Result = !FFinished;
    if (Result)
    {
        TGuard Guard(FItemsSection);

        Result = (FItems->IndexOf(static_cast<nb::TObject *>(Item)) != -1);
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
    if (nb::Now() - FLastIdle > FIdleInterval)
    {
        FLastIdle = FIdleInterval;
        TTerminalItem *TerminalItem = NULL;

        if (FFreeTerminals > 0)
        {
            TGuard Guard(FItemsSection);

            if (FFreeTerminals > 0)
            {
                // take the last free terminal, because TerminalFree() puts it to the
                // front, this ensures we cycle thru all free terminals
                TerminalItem = reinterpret_cast<TTerminalItem *>(FTerminals->GetItem(FFreeTerminals - 1));
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
    TTerminalItem *TerminalItem;
    TQueueItem *Item;

    do
    {
        TerminalItem = NULL;
        Item = NULL;

        if (FItems->GetCount() > static_cast<size_t>(FItemsInProcess))
        {
            TGuard Guard(FItemsSection);

            if ((FFreeTerminals == 0) &&
                    ((static_cast<int>(FTransfersLimit) <= 0) ||
                     (FTerminals->GetCount() < FTransfersLimit + FTemporaryTerminals)))
            {
                FOverallTerminals++;
                TerminalItem = new TTerminalItem(this, FOverallTerminals);
                TerminalItem->Init();
                FTerminals->Add(static_cast<nb::TObject *>(TerminalItem));
            }
            else if (FFreeTerminals > 0)
            {
                TerminalItem = reinterpret_cast<TTerminalItem *>(FTerminals->GetItem(0));
                FTerminals->Move(0, FTerminals->GetCount() - 1);
                FFreeTerminals--;
            }

            if (TerminalItem != NULL)
            {
                Item = GetItem(FItemsInProcess);
                FItemsInProcess++;
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
void TTerminalQueue::DoQueueItemUpdate(TQueueItem *Item)
{
    if (!GetOnQueueItemUpdate().empty())
    {
        GetOnQueueItemUpdate()(this, Item);
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoListUpdate()
{
    if (!GetOnListUpdate().empty())
    {
        GetOnListUpdate()(this);
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoEvent(TQueueEvent Event)
{
    if (!GetOnEvent().empty())
    {
        GetOnEvent()(this, Event);
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoQueryUser(nb::TObject *Sender,
                                 const std::wstring Query, nb::TStrings *MoreMessages, int Answers,
                                 const TQueryParams *Params, int &Answer, TQueryType Type, void *Arg)
{
    if (!GetOnQueryUser().empty())
    {
        GetOnQueryUser()(Sender, Query, MoreMessages, Answers, Params, Answer, Type, Arg);
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoPromptUser(TTerminal *Terminal,
                                  TPromptKind Kind, const std::wstring Name, const std::wstring Instructions,
                                  nb::TStrings *Prompts, nb::TStrings *Results, bool &Result, void *Arg)
{
    if (!GetOnPromptUser().empty())
    {
        GetOnPromptUser()(Terminal, Kind, Name, Instructions, Prompts, Results, Result, Arg);
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::DoShowExtendedException(
    TTerminal *Terminal, const std::exception *E, void *Arg)
{
    if (!GetOnShowExtendedException().empty())
    {
        GetOnShowExtendedException()(Terminal, E, Arg);
    }
}
//---------------------------------------------------------------------------
void TTerminalQueue::SetTransfersLimit(size_t value)
{
    if (FTransfersLimit != value)
    {
        {
            TGuard Guard(FItemsSection);

            if ((value > 0) && (value < FItemsInProcess))
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
    explicit TBackgroundTerminal(TTerminal *MainTerminal,
                                 TTerminalItem *Item);
    virtual void Init(TSessionData *SessionData, TConfiguration *Configuration,
                      const std::wstring Name);
    virtual ~TBackgroundTerminal()
    {}
protected:
    virtual bool DoQueryReopen(std::exception *E);

private:
    TTerminalItem *FItem;
};
//---------------------------------------------------------------------------
TBackgroundTerminal::TBackgroundTerminal(TTerminal *MainTerminal,
        TTerminalItem *Item) :
    TSecondaryTerminal(MainTerminal),
    FItem(Item)
{
}
void TBackgroundTerminal::Init(TSessionData *SessionData, TConfiguration *configuration,
                               const std::wstring Name)
{
    TSecondaryTerminal::Init(SessionData, configuration, Name);
}

//---------------------------------------------------------------------------
bool TBackgroundTerminal::DoQueryReopen(std::exception * /*E*/)
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
TTerminalItem::TTerminalItem(TTerminalQueue *Queue, size_t Index) :
    TSignalThread(), FQueue(Queue), FTerminal(NULL), FItem(NULL),
    FCriticalSection(NULL), FUserAction(NULL), FIndex(Index)
{
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
void TTerminalItem::Init()
{
    TSignalThread::Init();

    FCriticalSection = new TCriticalSection();
    Self = this;

    FTerminal = new TBackgroundTerminal(FQueue->FTerminal, this);
    FTerminal->Init(FQueue->FSessionData, FQueue->FConfiguration, FORMAT(L"Background %d", FIndex));
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
void TTerminalItem::Process(TQueueItem *Item)
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
    catch (const std::exception &E)
    {
        // do not show error messages, if task was canceled anyway
        // (for example if transfer is cancelled during reconnection attempts)
        // if (!FCancel &&
        // (FTerminal->QueryUserException(L"", &E, qaOK | qaCancel, NULL, qtError) == qaCancel))
        // {
        // FCancel = true;
        // }
        std::wstring Message;
        if (ExceptionMessage(&E, Message))
        {
            // do not show error messages, if task was canceled anyway
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

    TQueueItem *Item = FItem;
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
bool TTerminalItem::ProcessUserAction(void *Arg)
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

        FUserAction->Execute(FQueue, Arg);
        FUserAction = NULL;

        TriggerEvent();
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TTerminalItem::WaitForUserAction(
    TQueueItem::TStatus ItemStatus, TUserAction *UserAction)
{
    assert(FItem != NULL);
    assert((FItem->GetStatus() == TQueueItem::qsProcessing) ||
           (FItem->GetStatus() == TQueueItem::qsConnecting));

    bool Result;

    TQueueItem::TStatus PrevStatus = FItem->GetStatus();

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
    return Result;
}
//---------------------------------------------------------------------------
void TTerminalItem::Finished()
{
    TSignalThread::Finished();

    FQueue->TerminalFinished(this);
}
//---------------------------------------------------------------------------
void TTerminalItem::TerminalQueryUser(nb::TObject *Sender,
                                      const std::wstring Query, nb::TStrings *MoreMessages, int Answers,
                                      const TQueryParams *Params, int &Answer, TQueryType Type, void *Arg)
{
    // so far query without queue item can occur only for key cofirmation
    // on re-key with non-cached host key. make it fail.
    if (FItem != NULL)
    {
        USEDPARAM(Arg);
        assert(Arg == NULL);

        TQueryUserAction Action;
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
void TTerminalItem::TerminalPromptUser(TTerminal *Terminal,
                                       TPromptKind Kind, std::wstring Name, std::wstring Instructions, nb::TStrings *Prompts,
                                       nb::TStrings *Results, bool &Result, void *Arg)
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

        TPromptUserAction Action;
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
    TTerminal *Terminal, const std::exception *E, void *Arg)
{
    USEDPARAM(Arg);
    assert(Arg == NULL);

    std::wstring Message; // not used
    if ((FItem != NULL) &&
            ExceptionMessage(E, Message))
    {
        TShowExtendedExceptionAction Action;
        Action.Terminal = Terminal;
        Action.E = E;

        WaitForUserAction(TQueueItem::qsError, &Action);
    }
}
//---------------------------------------------------------------------------
void TTerminalItem::OperationFinished(TFileOperation /*Operation*/,
                                      TOperationSide /*Side*/, bool /*Temp*/, const std::wstring /*FileName*/,
                                      bool /*Success*/, TOnceDoneOperation & /*OnceDoneOperation*/)
{
    // nothing
}
//---------------------------------------------------------------------------
void TTerminalItem::OperationProgress(
    TFileOperationProgressType &ProgressData, TCancelStatus &Cancel)
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

        {
            BOOST_SCOPE_EXIT ( (&Self) (&PrevStatus) (&ProgressData) )
            {
                Self->FItem->SetStatus(PrevStatus);
                ProgressData.Resume();
            } BOOST_SCOPE_EXIT_END
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

    FItem->SetProgress(ProgressData);
}
//---------------------------------------------------------------------------
bool TTerminalItem::OverrideItemStatus(TQueueItem::TStatus &ItemStatus)
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
    FStatus(qsPending), FTerminalItem(NULL), FSection(NULL), FProgressData(NULL),
    FQueue(NULL), FInfo(NULL), FCompleteEvent(INVALID_HANDLE_VALUE),
    FCPSLimit(-1),
    FOwnsProgressData(true)
{
    FSection = new TCriticalSection();
    FInfo = new TInfo();
    Self = this;
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
    TFileOperationProgressType &ProgressData)
{
    {
        TGuard Guard(FSection);

        assert(FProgressData != NULL);
        if ((FProgressData != &ProgressData) && FOwnsProgressData)
        {
            delete FProgressData;
        }
        FProgressData = &ProgressData;
        FOwnsProgressData = false;
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
void TQueueItem::GetData(TQueueItemProxy *Proxy)
{
    TGuard Guard(FSection);

    assert(Proxy->FProgressData != NULL);
    if (FProgressData != NULL)
    {
        Proxy->FProgressData = FProgressData;
        Proxy->FOwnsProgressData = false;
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
void TQueueItem::Execute(TTerminalItem *TerminalItem)
{
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            if (Self->FOwnsProgressData)
            {
                TGuard Guard(Self->FSection);
                delete Self->FProgressData;
                Self->FProgressData = NULL;
            }
        } BOOST_SCOPE_EXIT_END
        {
            assert(FProgressData == NULL);
            TGuard Guard(FSection);
            FProgressData = new TFileOperationProgressType();
        }
        DoExecute(TerminalItem->FTerminal);
    }
}
//---------------------------------------------------------------------------
void TQueueItem::SetCPSLimit(unsigned long CPSLimit)
{
    FCPSLimit = static_cast<long>(CPSLimit);
}
//---------------------------------------------------------------------------
// TQueueItemProxy
//---------------------------------------------------------------------------
TQueueItemProxy::TQueueItemProxy(TTerminalQueue *Queue,
                                 TQueueItem *QueueItem) :
    FQueue(Queue), FQueueItem(QueueItem), FProgressData(NULL),
    FQueueStatus(NULL), FInfo(NULL),
    FProcessingUserAction(false), FUserData(NULL),
    FOwnsProgressData(true)
{
    FProgressData = new TFileOperationProgressType();
    FInfo = new TQueueItem::TInfo();
    Self = this;

    Update();
}
//---------------------------------------------------------------------------
TQueueItemProxy::~TQueueItemProxy()
{
    if (FOwnsProgressData)
    {
        delete FProgressData;
    }
    delete FInfo;
}
//---------------------------------------------------------------------------
TFileOperationProgressType *TQueueItemProxy::GetProgressData()
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
    size_t I = GetIndex();
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
bool TQueueItemProxy::Move(TQueueItemProxy *BeforeItem)
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
bool TQueueItemProxy::ProcessUserAction(void *Arg)
{
    assert(FQueueItem != NULL);

    bool Result;
    FProcessingUserAction = true;
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FProcessingUserAction = false;
        } BOOST_SCOPE_EXIT_END
        Result = FQueue->ItemProcessUserAction(FQueueItem, Arg);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TQueueItemProxy::SetCPSLimit(unsigned long CPSLimit)
{
    return FQueue->ItemSetCPSLimit(FQueueItem, CPSLimit);
}
//---------------------------------------------------------------------------
size_t TQueueItemProxy::GetIndex()
{
    assert(FQueueStatus != NULL);
    size_t Index = FQueueStatus->FList->IndexOf(static_cast<nb::TObject *>(static_cast<void *>(this)));
    assert(Index != -1);
    return Index;
}
//---------------------------------------------------------------------------
// TTerminalQueueStatus
//---------------------------------------------------------------------------
TTerminalQueueStatus::TTerminalQueueStatus() :
    FList(NULL)
{
    FList = new nb::TList();
    ResetStats();
}
//---------------------------------------------------------------------------
TTerminalQueueStatus::~TTerminalQueueStatus()
{
    for (size_t Index = 0; Index < FList->GetCount(); Index++)
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
size_t TTerminalQueueStatus::GetActiveCount()
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
void TTerminalQueueStatus::Add(TQueueItemProxy *ItemProxy)
{
    ItemProxy->FQueueStatus = this;
    FList->Add(static_cast<nb::TObject *>(static_cast<void *>(ItemProxy)));
    ResetStats();
}
//---------------------------------------------------------------------------
void TTerminalQueueStatus::Delete(TQueueItemProxy *ItemProxy)
{
    FList->Extract(static_cast<nb::TObject *>(static_cast<void *>(ItemProxy)));
    ItemProxy->FQueueStatus = NULL;
    ResetStats();
}
//---------------------------------------------------------------------------
size_t TTerminalQueueStatus::GetCount()
{
    return FList->GetCount();
}
//---------------------------------------------------------------------------
TQueueItemProxy *TTerminalQueueStatus::GetItem(size_t Index)
{
    return reinterpret_cast<TQueueItemProxy *>(FList->GetItem(Index));
}
//---------------------------------------------------------------------------
TQueueItemProxy *TTerminalQueueStatus::FindByQueueItem(
    TQueueItem *QueueItem)
{
    TQueueItemProxy *Item;
    for (size_t Index = 0; Index < FList->GetCount(); Index++)
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
TLocatedQueueItem::TLocatedQueueItem(TTerminal *Terminal) :
    TQueueItem()
{
    assert(Terminal != NULL);
    FCurrentDir = Terminal->GetCurrentDirectory();
}
//---------------------------------------------------------------------------
std::wstring TLocatedQueueItem::StartupDirectory()
{
    return FCurrentDir;
}
//---------------------------------------------------------------------------
void TLocatedQueueItem::DoExecute(TTerminal *Terminal)
{
    assert(Terminal != NULL);
    Terminal->GetCurrentDirectory() = FCurrentDir;
}
//---------------------------------------------------------------------------
// TTransferQueueItem
//---------------------------------------------------------------------------
TTransferQueueItem::TTransferQueueItem(TTerminal *Terminal,
                                       nb::TStrings *FilesToCopy, const std::wstring TargetDir,
                                       const TCopyParamType *CopyParam, int Params, TOperationSide Side) :
    TLocatedQueueItem(Terminal), FFilesToCopy(NULL), FCopyParam(NULL)
{
    FInfo->Operation = (Params & cpDelete ? foMove : foCopy);
    FInfo->Side = Side;

    assert(FilesToCopy != NULL);
    FFilesToCopy = new nb::TStringList();
    for (size_t Index = 0; Index < FilesToCopy->GetCount(); Index++)
    {
        FFilesToCopy->AddObject(FilesToCopy->GetString(Index),
                                ((FilesToCopy->GetObject(Index) == NULL) || (Side == osLocal)) ? NULL :
                                reinterpret_cast<TRemoteFile *>(FilesToCopy->GetObject(Index))->Duplicate());
    }

    FTargetDir = TargetDir;

    assert(CopyParam != NULL);
    FCopyParam = new TCopyParamType(*CopyParam);

    FParams = Params;
}
//---------------------------------------------------------------------------
TTransferQueueItem::~TTransferQueueItem()
{
    for (size_t Index = 0; Index < FFilesToCopy->GetCount(); Index++)
    {
        delete FFilesToCopy->GetObject(Index);
    }
    delete FFilesToCopy;
    delete FCopyParam;
}
//---------------------------------------------------------------------------
// TUploadQueueItem
//---------------------------------------------------------------------------
TUploadQueueItem::TUploadQueueItem(TTerminal *Terminal,
                                   nb::TStrings *FilesToCopy, const std::wstring TargetDir,
                                   const TCopyParamType *CopyParam, int Params) :
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
            FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? std::wstring() :
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
            FInfo->ModifiedLocal = FLAGCLEAR(Params, cpDelete) ? std::wstring() :
                                   IncludeTrailingBackslash(ExtractFilePath(FInfo->Source));
        }
    }

    FInfo->Destination =
        UnixIncludeTrailingBackslash(TargetDir) + CopyParam->GetFileMask();
    FInfo->ModifiedRemote = UnixIncludeTrailingBackslash(TargetDir);
}
//---------------------------------------------------------------------------
void TUploadQueueItem::DoExecute(TTerminal *Terminal)
{
    TTransferQueueItem::DoExecute(Terminal);

    assert(Terminal != NULL);
    Terminal->CopyToRemote(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
//---------------------------------------------------------------------------
// TDownloadQueueItem
//---------------------------------------------------------------------------
TDownloadQueueItem::TDownloadQueueItem(TTerminal *Terminal,
                                       nb::TStrings *FilesToCopy, const std::wstring TargetDir,
                                       const TCopyParamType *CopyParam, int Params) :
    TTransferQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, osRemote)
{
    if (FilesToCopy->GetCount() > 1)
    {
        if (!UnixExtractCommonPath(FilesToCopy, FInfo->Source))
        {
            FInfo->Source = Terminal->GetCurrentDirectory();
        }
        FInfo->Source = UnixExcludeTrailingBackslash(FInfo->Source);
        FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? std::wstring() :
                                UnixIncludeTrailingBackslash(FInfo->Source);
    }
    else
    {
        assert(FilesToCopy->GetCount() > 0);
        FInfo->Source = FilesToCopy->GetString(0);
        if (UnixExtractFilePath(FInfo->Source).empty())
        {
            FInfo->Source = UnixIncludeTrailingBackslash(Terminal->GetCurrentDirectory()) +
                            FInfo->Source;
            FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? std::wstring() :
                                    UnixIncludeTrailingBackslash(Terminal->GetCurrentDirectory());
        }
        else
        {
            FInfo->ModifiedRemote = FLAGCLEAR(Params, cpDelete) ? std::wstring() :
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
void TDownloadQueueItem::DoExecute(TTerminal *Terminal)
{
    TTransferQueueItem::DoExecute(Terminal);

    assert(Terminal != NULL);
    Terminal->CopyToLocal(FFilesToCopy, FTargetDir, FCopyParam, FParams);
}
