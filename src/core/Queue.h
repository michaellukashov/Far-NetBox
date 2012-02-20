//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>

#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread : public nb::TObject
{
public:
    explicit TSimpleThread();
    virtual ~TSimpleThread();
    virtual void Init();
    virtual void Start();
    void WaitFor(unsigned int Milliseconds = INFINITE);
    virtual void Terminate() = 0;
    void Close();
    bool IsFinished();

    static int ThreadProc(void *Thread);
protected:
    HANDLE FThread;
    bool FFinished;

    virtual void Execute() = 0;
    virtual void Finished();

private:
    TSimpleThread(const TSimpleThread &);
    TSimpleThread &operator = (const TSimpleThread &);
};
//---------------------------------------------------------------------------
class TSignalThread : public TSimpleThread
{
public:
    virtual void Init();
    virtual void Start();
    virtual void Terminate();
    void TriggerEvent();

protected:
    HANDLE FEvent;
    bool FTerminated;

    explicit TSignalThread();
    virtual ~TSignalThread();

    bool WaitForEvent();
    virtual void Execute();
    virtual void ProcessEvent() = 0;
};
//---------------------------------------------------------------------------
class TTerminal;
class TQueueItem;
class TCriticalSection;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;
//---------------------------------------------------------------------------
typedef boost::signal1<void, TTerminalQueue *> queuelistupdate_signal_type;
typedef queuelistupdate_signal_type::slot_type queuelistupdate_slot_type;
typedef boost::signal2<void, TTerminalQueue *, TQueueItem *> queueitemupdate_signal_type;
typedef queueitemupdate_signal_type::slot_type queueitemupdate_slot_type;
enum TQueueEvent { qeEmpty, qePendingUserAction };
typedef boost::signal2<void, TTerminalQueue *, TQueueEvent> queueevent_signal_type;
typedef queueevent_signal_type::slot_type queueevent_slot_type;
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
    friend class TQueueItem;
    friend class TQueueItemProxy;

public:
    explicit TTerminalQueue(TTerminal *Terminal, TConfiguration *Configuration);
    virtual ~TTerminalQueue();

    virtual void Init();
    void AddItem(TQueueItem *Item);
    TTerminalQueueStatus *CreateStatus(TTerminalQueueStatus *Current);
    void Idle();

    bool GetIsEmpty();
    size_t GetTransfersLimit() { return FTransfersLimit; }
    void SetTransfersLimit(size_t value);
    queryuser_signal_type &GetOnQueryUser() { return FOnQueryUser; }
    void SetOnQueryUser(const queryuser_slot_type &value) { FOnQueryUser.connect(value); }
    promptuser_signal_type &GetOnPromptUser() { return FOnPromptUser; }
    void SetOnPromptUser(const promptuser_slot_type &value) { FOnPromptUser.connect(value); }
    extendedexception_signal_type &GetOnShowExtendedException() { return FOnShowExtendedException; }
    void SetOnShowExtendedException(const extendedexception_slot_type &value) { FOnShowExtendedException.connect(value); }
    queuelistupdate_signal_type &GetOnListUpdate() { return FOnListUpdate; }
    void SetOnListUpdate(const queuelistupdate_slot_type &value) { FOnListUpdate.connect(value); }
    queueitemupdate_signal_type &GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
    void SetOnQueueItemUpdate(const queueitemupdate_slot_type &value) { FOnQueueItemUpdate.connect(value); }
    queueevent_signal_type &GetOnEvent() { return FOnEvent; }
    void SetOnEvent(const queueevent_slot_type &value) { FOnEvent.connect(value); }

protected:
    friend class TTerminalItem;
    friend class TQueryUserAction;
    friend class TPromptUserAction;
    friend class TShowExtendedExceptionAction;

    queryuser_signal_type FOnQueryUser;
    promptuser_signal_type FOnPromptUser;
    extendedexception_signal_type FOnShowExtendedException;
    queueitemupdate_signal_type FOnQueueItemUpdate;
    queuelistupdate_signal_type FOnListUpdate;
    queueevent_signal_type FOnEvent;
    TTerminal *FTerminal;
    TConfiguration *FConfiguration;
    TSessionData *FSessionData;
    nb::TList *FItems;
    size_t FItemsInProcess;
    TCriticalSection *FItemsSection;
    size_t FFreeTerminals;
    nb::TList *FTerminals;
    size_t FTemporaryTerminals;
    size_t FOverallTerminals;
    size_t FTransfersLimit;
    nb::TDateTime FIdleInterval;
    nb::TDateTime FLastIdle;
    TTerminalQueue *Self;

    TQueueItem *GetItem(size_t Index);
    bool ItemGetData(TQueueItem *Item, TQueueItemProxy *Proxy);
    bool ItemProcessUserAction(TQueueItem *Item, void *Arg);
    bool ItemMove(TQueueItem *Item, TQueueItem *BeforeItem);
    bool ItemExecuteNow(TQueueItem *Item);
    bool ItemDelete(TQueueItem *Item);
    bool ItemPause(TQueueItem *Item, bool Pause);
    bool ItemSetCPSLimit(TQueueItem *Item, unsigned long CPSLimit);

    void RetryItem(TQueueItem *Item);
    void DeleteItem(TQueueItem *Item);

    virtual void ProcessEvent();
    void TerminalFinished(TTerminalItem *TerminalItem);
    bool TerminalFree(TTerminalItem *TerminalItem);

    void DoQueryUser(nb::TObject *Sender, const std::wstring Query,
                     nb::TStrings *MoreMessages, int Answers, const TQueryParams *Params, int &Answer,
                     TQueryType Type, void *Arg);
    void DoPromptUser(TTerminal *Terminal, TPromptKind Kind,
                      const std::wstring Name, const std::wstring Instructions, nb::TStrings *Prompts,
                      nb::TStrings *Results, bool &Result, void *Arg);
    void DoShowExtendedException(TTerminal *Terminal,
                                 const std::exception *E, void *Arg);
    void DoQueueItemUpdate(TQueueItem *Item);
    void DoListUpdate();
    void DoEvent(TQueueEvent Event);
};
//---------------------------------------------------------------------------
class TQueueItem : public nb::TObject
{
    friend class TTerminalQueue;
    friend class TTerminalItem;

public:
    enum TStatus
    {
        qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
        qsPaused, qsDone
    };
    struct TInfo
    {
        TFileOperation Operation;
        TOperationSide Side;
        std::wstring Source;
        std::wstring Destination;
        std::wstring ModifiedLocal;
        std::wstring ModifiedRemote;
    };

    static bool IsUserActionStatus(TStatus Status);

    TStatus GetStatus();
    HANDLE GetCompleteEvent() { return FCompleteEvent; }
    void SetCompleteEvent(HANDLE value) { FCompleteEvent = value; }

protected:
    TStatus FStatus;
    TCriticalSection *FSection;
    TTerminalItem *FTerminalItem;
    TFileOperationProgressType *FProgressData;
    TQueueItem::TInfo *FInfo;
    TTerminalQueue *FQueue;
    HANDLE FCompleteEvent;
    long FCPSLimit;
    TQueueItem *Self;
    bool FOwnsProgressData;

    explicit TQueueItem();
    virtual ~TQueueItem();

    void SetStatus(TStatus Status);
    void Execute(TTerminalItem *TerminalItem);
    virtual void DoExecute(TTerminal *Terminal) = 0;
    void SetProgress(TFileOperationProgressType &ProgressData);
    void GetData(TQueueItemProxy *Proxy);
    void SetCPSLimit(unsigned long CPSLimit);
    virtual std::wstring StartupDirectory() = 0;
};
//---------------------------------------------------------------------------
class TQueueItemProxy
{
    friend class TQueueItem;
    friend class TTerminalQueueStatus;
    friend class TTerminalQueue;

public:
    bool Update();
    bool ProcessUserAction(void *Arg = NULL);
    bool Move(bool Sooner);
    bool Move(TQueueItemProxy *BeforeItem);
    bool ExecuteNow();
    bool Delete();
    bool Pause();
    bool Resume();
    bool SetCPSLimit(unsigned long CPSLimit);

    TFileOperationProgressType *GetProgressData();
    TQueueItem::TInfo *GetInfo() { return FInfo; }
    TQueueItem::TStatus GetStatus() { return FStatus; }
    bool GetProcessingUserAction() { return FProcessingUserAction; }
    size_t GetIndex();
    void *GetUserData() { return FUserData; }
    void SetUserData(void *value) { FUserData = value; }

private:
    TFileOperationProgressType *FProgressData;
    TQueueItem::TStatus FStatus;
    TTerminalQueue *FQueue;
    TQueueItem *FQueueItem;
    TTerminalQueueStatus *FQueueStatus;
    TQueueItem::TInfo *FInfo;
    bool FProcessingUserAction;
    void *FUserData;
    bool FOwnsProgressData;
    TQueueItemProxy *Self;

    explicit TQueueItemProxy(TTerminalQueue *Queue, TQueueItem *QueueItem);
    virtual ~TQueueItemProxy();
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus
{
    friend class TTerminalQueue;
    friend class TQueueItemProxy;

public:
    virtual ~TTerminalQueueStatus();

    TQueueItemProxy *FindByQueueItem(TQueueItem *QueueItem);

    size_t GetCount();
    size_t GetActiveCount();
    TQueueItemProxy *GetItem(size_t Index);

protected:
    TTerminalQueueStatus();

    void Add(TQueueItemProxy *ItemProxy);
    void Delete(TQueueItemProxy *ItemProxy);
    void ResetStats();

private:
    nb::TList *FList;
    size_t FActiveCount;

};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
    explicit TLocatedQueueItem(TTerminal *Terminal);
    virtual ~TLocatedQueueItem()
    {}

    virtual void DoExecute(TTerminal *Terminal);
    virtual std::wstring StartupDirectory();

private:
    std::wstring FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
    explicit TTransferQueueItem(TTerminal *Terminal,
                                nb::TStrings *FilesToCopy, const std::wstring TargetDir,
                                const TCopyParamType *CopyParam, int Params, TOperationSide Side);
    virtual ~TTransferQueueItem();

protected:
    nb::TStrings *FFilesToCopy;
    std::wstring FTargetDir;
    TCopyParamType *FCopyParam;
    int FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
    explicit TUploadQueueItem(TTerminal *Terminal,
                              nb::TStrings *FilesToCopy, const std::wstring TargetDir,
                              const TCopyParamType *CopyParam, int Params);
    virtual ~TUploadQueueItem()
    {}
protected:
    virtual void DoExecute(TTerminal *Terminal);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
    explicit TDownloadQueueItem(TTerminal *Terminal,
                                nb::TStrings *FilesToCopy, const std::wstring TargetDir,
                                const TCopyParamType *CopyParam, int Params);
    virtual ~TDownloadQueueItem()
    {}
protected:
    virtual void DoExecute(TTerminal *Terminal);
};
//---------------------------------------------------------------------------
#endif
