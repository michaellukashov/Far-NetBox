//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread
{
public:
  TSimpleThread();
  virtual ~TSimpleThread();

  virtual void Start();
  void WaitFor(unsigned int Milliseconds = INFINITE);
  virtual void Terminate() = 0;
  void Close();
  bool IsFinished();

protected:
  HANDLE FThread;
  bool FFinished;

  virtual void Execute() = 0;
  virtual void Finished();

  static int ThreadProc(void * Thread);
};
//---------------------------------------------------------------------------
class TSignalThread : public TSimpleThread
{
public:
  virtual void Start();
  virtual void Terminate();
  void TriggerEvent();

protected:
  HANDLE FEvent;
  bool FTerminated;

  TSignalThread();
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
typedef void (TObject::* TQueueListUpdate)
  (TTerminalQueue * Queue);
typedef void (TObject::* TQueueItemUpdateEvent)
  (TTerminalQueue * Queue, TQueueItem * Item);
enum TQueueEvent { qeEmpty, qePendingUserAction };
typedef void (TObject::* TQueueEventEvent)
  (TTerminalQueue * Queue, TQueueEvent Event);
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;

public:
  TTerminalQueue(TTerminal * Terminal, TConfiguration * Configuration);
  virtual ~TTerminalQueue();

  void AddItem(TQueueItem * Item);
  TTerminalQueueStatus * CreateStatus(TTerminalQueueStatus * Current);
  void Idle();

  // __property bool IsEmpty = { read = GetIsEmpty };
  bool GetIsEmpty();
  // __property int TransfersLimit = { read = FTransfersLimit, write = SetTransfersLimit };
  int GetTransfersLimit() { return FTransfersLimit; }
  void SetTransfersLimit(int value);
  // __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  TQueryUserEvent GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent value) { FOnQueryUser = value; }
  // __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  TPromptUserEvent GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent value) { FOnPromptUser = value; }
  // __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  TExtendedExceptionEvent GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent value) { FOnShowExtendedException = value; }
  // __property TQueueListUpdate OnListUpdate = { read = FOnListUpdate, write = FOnListUpdate };
  TQueueListUpdate GetOnListUpdate() { return FOnListUpdate; }
  void SetOnListUpdate(TQueueListUpdate value) { FOnListUpdate = value; }
  // __property TQueueItemUpdateEvent OnQueueItemUpdate = { read = FOnQueueItemUpdate, write = FOnQueueItemUpdate };
  TQueueItemUpdateEvent GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void SetOnQueueItemUpdate(TQueueItemUpdateEvent value) { FOnQueueItemUpdate = value; }
  // __property TQueueEventEvent OnEvent = { read = FOnEvent, write = FOnEvent };
  TQueueEventEvent GetOnEvent() { return FOnEvent; }
  void SetOnEvent(TQueueEventEvent value) { FOnEvent = value; }

protected:
  friend class TTerminalItem;
  friend class TQueryUserAction;
  friend class TPromptUserAction;
  friend class TShowExtendedExceptionAction;

  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TQueueItemUpdateEvent FOnQueueItemUpdate;
  TQueueListUpdate FOnListUpdate;
  TQueueEventEvent FOnEvent;
  TTerminal * FTerminal;
  TConfiguration * FConfiguration;
  TSessionData * FSessionData;
  TList * FItems;
  int FItemsInProcess;
  TCriticalSection * FItemsSection;
  int FFreeTerminals;
  TList * FTerminals;
  int FTemporaryTerminals;
  int FOverallTerminals;
  int FTransfersLimit;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;

  TQueueItem * GetItem(int Index);
  bool ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy);
  bool ItemProcessUserAction(TQueueItem * Item, void * Arg);
  bool ItemMove(TQueueItem * Item, TQueueItem * BeforeItem);
  bool ItemExecuteNow(TQueueItem * Item);
  bool ItemDelete(TQueueItem * Item);
  bool ItemPause(TQueueItem * Item, bool Pause);
  bool ItemSetCPSLimit(TQueueItem * Item, unsigned long CPSLimit);

  void RetryItem(TQueueItem * Item);
  void DeleteItem(TQueueItem * Item);

  virtual void ProcessEvent();
  void TerminalFinished(TTerminalItem * TerminalItem);
  bool TerminalFree(TTerminalItem * TerminalItem);

  void DoQueryUser(TObject * Sender, const std::wstring Query,
    TStrings * MoreMessages, int Answers, const TQueryParams * Params, int & Answer,
    TQueryType Type, void * Arg);
  void DoPromptUser(TTerminal * Terminal, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, TStrings * Prompts,
    TStrings * Results, bool & Result, void * Arg);
  void DoShowExtendedException(TTerminal * Terminal,
    exception * E, void * Arg);
  void DoQueueItemUpdate(TQueueItem * Item);
  void DoListUpdate();
  void DoEvent(TQueueEvent Event);
};
//---------------------------------------------------------------------------
class TQueueItem
{
friend class TTerminalQueue;
friend class TTerminalItem;

public:
  enum TStatus {
    qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
    qsPaused, qsDone };
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

  // __property TStatus Status = { read = GetStatus };
  TStatus GetStatus();
  // __property HANDLE CompleteEvent = { read = FCompleteEvent, write = FCompleteEvent };
  HANDLE GetCompleteEvent() { return FCompleteEvent; }
  void SetCompleteEvent(HANDLE value) { FCompleteEvent = value; }

protected:
  TStatus FStatus;
  TCriticalSection * FSection;
  TTerminalItem * FTerminalItem;
  TFileOperationProgressType * FProgressData;
  TQueueItem::TInfo * FInfo;
  TTerminalQueue * FQueue;
  HANDLE FCompleteEvent;
  long FCPSLimit;

  TQueueItem();
  virtual ~TQueueItem();

  void SetStatus(TStatus Status);
  void Execute(TTerminalItem * TerminalItem);
  virtual void DoExecute(TTerminal * Terminal) = 0;
  void SetProgress(TFileOperationProgressType & ProgressData);
  void GetData(TQueueItemProxy * Proxy);
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
  bool ProcessUserAction(void * Arg = NULL);
  bool Move(bool Sooner);
  bool Move(TQueueItemProxy * BeforeItem);
  bool ExecuteNow();
  bool Delete();
  bool Pause();
  bool Resume();
  bool SetCPSLimit(unsigned long CPSLimit);

  // __property TFileOperationProgressType * ProgressData = { read = GetProgressData };
  TFileOperationProgressType * GetProgressData();
  // __property TQueueItem::TInfo * Info = { read = FInfo };
  TQueueItem::TInfo * GetInfo() { return FInfo; }
  // __property TQueueItem::TStatus Status = { read = FStatus };
  TQueueItem::TStatus GetStatus() { return FStatus; }
  // __property bool ProcessingUserAction = { read = FProcessingUserAction };
  bool GetProcessingUserAction() { return FProcessingUserAction; }
  // __property int Index = { read = GetIndex };
  int GetIndex();
  // __property void * UserData = { read = FUserData, write = FUserData };
  void * GetUserData() { return FUserData; }
  void SetUserData(void * value) { FUserData = value; }

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;
  void * FUserData;

  TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual ~TQueueItemProxy();
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus
{
friend class TTerminalQueue;
friend class TQueueItemProxy;

public:
  virtual ~TTerminalQueueStatus();

  TQueueItemProxy * FindByQueueItem(TQueueItem * QueueItem);

  // __property int Count = { read = GetCount };
  int GetCount();
  // __property int ActiveCount = { read = GetActiveCount };
  int GetActiveCount();
  // __property TQueueItemProxy * Items[int Index] = { read = GetItem };
  TQueueItemProxy * GetItem(int Index);

protected:
  TTerminalQueueStatus();

  void Add(TQueueItemProxy * ItemProxy);
  void Delete(TQueueItemProxy * ItemProxy);
  void ResetStats();

private:
  TList * FList;
  int FActiveCount;

};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
  TLocatedQueueItem(TTerminal * Terminal);

  virtual void DoExecute(TTerminal * Terminal);
  virtual std::wstring StartupDirectory();

private:
  std::wstring FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
  TTransferQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const std::wstring & TargetDir,
    const TCopyParamType * CopyParam, int Params, TOperationSide Side);
  virtual ~TTransferQueueItem();

protected:
  TStrings * FFilesToCopy;
  std::wstring FTargetDir;
  TCopyParamType * FCopyParam;
  int FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  TUploadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const std::wstring & TargetDir,
    const TCopyParamType * CopyParam, int Params);

protected:
  virtual void DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
  TDownloadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const std::wstring & TargetDir,
    const TCopyParamType * CopyParam, int Params);

protected:
  virtual void DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
#endif
