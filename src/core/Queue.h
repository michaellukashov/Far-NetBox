//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread : public TObject
{
public:
  explicit TSimpleThread();
  void Init();
  virtual ~TSimpleThread();

  virtual void Start();
  void WaitFor(unsigned int Milliseconds = INFINITE);
  virtual void Terminate() = 0;
  void Close();
  bool IsFinished();

protected:
  HANDLE FThread;
  TThreadID FThreadId;
  bool FFinished;

  virtual void Execute() = 0;
  virtual void Finished();

public:
  static int ThreadProc(void * Thread);

private:
  TSimpleThread(const TSimpleThread &);
  TSimpleThread & operator = (const TSimpleThread &);
};
//---------------------------------------------------------------------------
class TSignalThread : public TSimpleThread
{
public:
  void Init(bool LowPriority);
  virtual void Start();
  virtual void Terminate();
  void TriggerEvent();

protected:
  HANDLE FEvent;
  bool FTerminated;

  explicit TSignalThread();
  virtual ~TSignalThread();

  virtual bool WaitForEvent();
  int WaitForEvent(unsigned int Timeout);
  virtual void Execute();
  virtual void ProcessEvent() = 0;
};
//---------------------------------------------------------------------------
class TTerminal;
class TQueueItem;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE1(TQueueListUpdateEvent, void,
  TTerminalQueue * /* Queue */);
DEFINE_CALLBACK_TYPE2(TQueueItemUpdateEvent, void,
  TTerminalQueue * /* Queue */, TQueueItem * /* Item */);
enum TQueueEvent { qeEmpty, qePendingUserAction };
DEFINE_CALLBACK_TYPE2(TQueueEventEvent, void,
  TTerminalQueue * /* Queue */, TQueueEvent /* Event */);
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;

public:
  explicit TTerminalQueue(TTerminal * Terminal, TConfiguration * Configuration);
  virtual ~TTerminalQueue();

  void Init();
  void AddItem(TQueueItem * Item);
  TTerminalQueueStatus * CreateStatus(TTerminalQueueStatus * Current);
  void Idle();

  bool GetIsEmpty();
  intptr_t GetTransfersLimit() { return FTransfersLimit; }
  intptr_t GetKeepDoneItemsFor() { return FKeepDoneItemsFor; }
  bool GetEnabled() const { return FEnabled; }
  TQueryUserEvent & GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent & GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TExtendedExceptionEvent & GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TQueueListUpdateEvent & GetOnListUpdate() { return FOnListUpdate; }
  void SetOnListUpdate(TQueueListUpdateEvent Value) { FOnListUpdate = Value; }
  TQueueItemUpdateEvent & GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void SetOnQueueItemUpdate(TQueueItemUpdateEvent Value) { FOnQueueItemUpdate = Value; }
  TQueueEventEvent & GetOnEvent() { return FOnEvent; }
  void SetOnEvent(TQueueEventEvent Value) { FOnEvent = Value; }

protected:
  friend class TTerminalItem;
  friend class TQueryUserAction;
  friend class TPromptUserAction;
  friend class TShowExtendedExceptionAction;

  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TQueueItemUpdateEvent FOnQueueItemUpdate;
  TQueueListUpdateEvent FOnListUpdate;
  TQueueEventEvent FOnEvent;
  TTerminal * FTerminal;
  TConfiguration * FConfiguration;
  TSessionData * FSessionData;
  TList * FItems;
  TList * FDoneItems;
  intptr_t FItemsInProcess;
  TCriticalSection * FItemsSection;
  intptr_t FFreeTerminals;
  TList * FTerminals;
  TList * FForcedItems;
  intptr_t FTemporaryTerminals;
  intptr_t FOverallTerminals;
  intptr_t FTransfersLimit;
  intptr_t FKeepDoneItemsFor;
  bool FEnabled;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;

public:
  inline static TQueueItem * GetItem(TList * List, intptr_t Index);
  inline TQueueItem * GetItem(intptr_t Index);
  void FreeItemsList(TList * List);
  static bool EmptyButMonitoredItems(TList * List);
  void UpdateStatusForList(
    TTerminalQueueStatus * Status, TList * List, TTerminalQueueStatus * Current);
  bool ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy);
  bool ItemProcessUserAction(TQueueItem * Item, void * Arg);
  bool ItemMove(TQueueItem * Item, TQueueItem * BeforeItem);
  bool ItemExecuteNow(TQueueItem * Item);
  bool ItemDelete(TQueueItem * Item);
  bool ItemPause(TQueueItem * Item, bool Pause);
  bool ItemSetCPSLimit(TQueueItem * Item, unsigned long CPSLimit);

  void RetryItem(TQueueItem * Item);
  void DeleteItem(TQueueItem * Item, bool CanKeep);

  virtual bool WaitForEvent();
  virtual void ProcessEvent();
  void TerminalFinished(TTerminalItem * TerminalItem);
  bool TerminalFree(TTerminalItem * TerminalItem);

  void DoQueueItemUpdate(TQueueItem * Item);
  void DoListUpdate();
  void DoEvent(TQueueEvent Event);

public:
  void SetMasks(const UnicodeString & Value);
  void SetTransfersLimit(intptr_t Value);
  void SetKeepDoneItemsFor(intptr_t Value);
  void SetEnabled(bool Value);
  void SetEnabled(bool Value);

private:
  TTerminalQueue(const TTerminalQueue &);
  TTerminalQueue & operator = (const TTerminalQueue &);
};
//---------------------------------------------------------------------------
class TQueueItem : public TObject
{
friend class TTerminalQueue;
friend class TTerminalItem;

public:
  enum TStatus {
    qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
    qsPaused, qsDone };
  struct TInfo : public TObject
  {
    TFileOperation Operation;
    TOperationSide Side;
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
    bool SingleFile;
  };

  static bool IsUserActionStatus(TStatus Status);

  TStatus GetStatus();
  HANDLE GetCompleteEvent() { return FCompleteEvent; }
  void SetCompleteEvent(HANDLE Value) { FCompleteEvent = Value; }

protected:
  TStatus FStatus;
  TCriticalSection * FSection;
  TTerminalItem * FTerminalItem;
  TFileOperationProgressType * FProgressData;
  TQueueItem::TInfo * FInfo;
  TTerminalQueue * FQueue;
  HANDLE FCompleteEvent;
  long FCPSLimit;
  TDateTime FDoneAt;

  explicit TQueueItem();
  virtual ~TQueueItem();

public:
  void SetMasks(const UnicodeString & Value);
  void SetStatus(TStatus Status);
  void SetProgress(TFileOperationProgressType & ProgressData);
  void GetData(TQueueItemProxy * Proxy);
  void SetCPSLimit(unsigned long CPSLimit);
private:
  void Execute(TTerminalItem * TerminalItem);
  virtual void DoExecute(TTerminal * Terminal) = 0;
  virtual UnicodeString StartupDirectory() = 0;
  void Complete();
};
//---------------------------------------------------------------------------
class TQueueItemProxy : public TObject
{
friend class TQueueItem;
friend class TTerminalQueueStatus;
friend class TTerminalQueue;

public:
  bool Update();
  bool ProcessUserAction();
  bool Move(bool Sooner);
  bool Move(TQueueItemProxy * BeforeItem);
  bool ExecuteNow();
  bool Delete();
  bool Pause();
  bool Resume();
  bool SetCPSLimit(unsigned long CPSLimit);

  TFileOperationProgressType * GetProgressData();
  __int64 GetTotalTransferred();
  TQueueItem::TInfo * GetInfo() { return FInfo; }
  TQueueItem::TStatus GetStatus() const { return FStatus; }
  bool GetProcessingUserAction() const { return FProcessingUserAction; }
  void * GetUserData() { return FUserData; }
  void SetUserData(void * Value) { FUserData = Value; }

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;
  void * FUserData;

  explicit TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual ~TQueueItemProxy();
public:
  void SetMasks(const UnicodeString & Value);
  intptr_t GetIndex();
  TFileOperationProgressType * GetProgressData();
  __int64 GetTotalTransferred();
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus : public TObject
{
friend class TTerminalQueue;
friend class TQueueItemProxy;

public:
  virtual ~TTerminalQueueStatus();

  TQueueItemProxy * FindByQueueItem(TQueueItem * QueueItem);

protected:
  TTerminalQueueStatus();

  void Add(TQueueItemProxy * ItemProxy);
  void Delete(TQueueItemProxy * ItemProxy);
  void ResetStats();

private:
  TList * FList;
  intptr_t FActiveCount;
  intptr_t FDoneCount;
  intptr_t FActiveCount;

public:
  void SetMasks(const UnicodeString & Value);
  intptr_t GetCount() const;
  intptr_t GetDoneCount() const { return FDoneCount; }
  intptr_t GetActiveCount();
  intptr_t GetDoneAndActiveCount() const;
  void SetDoneCount(intptr_t Value);
  TQueueItemProxy * GetItem(intptr_t Index);

private:
  TTerminalQueueStatus(const TTerminalQueueStatus &);
  TTerminalQueueStatus & operator = (const TTerminalQueueStatus &);
};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
  explicit TLocatedQueueItem(TTerminal * Terminal);
  virtual ~TLocatedQueueItem() {}

  virtual void DoExecute(TTerminal * Terminal);
  virtual UnicodeString StartupDirectory();

private:
  UnicodeString FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
  explicit TTransferQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params, TOperationSide Side,
    bool SingleFile);
  virtual ~TTransferQueueItem();

protected:
  TStrings * FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType * FCopyParam;
  intptr_t FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  explicit TUploadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile);
  virtual ~TUploadQueueItem() {}
protected:
  virtual void DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
  explicit TDownloadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile);
  virtual ~TDownloadQueueItem() {}
protected:
  virtual void DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TUserAction;
class TTerminalThread : public TSignalThread
{
public:
  explicit TTerminalThread(TTerminal * Terminal);
  void Init();
  virtual ~TTerminalThread();

  void TerminalOpen();
  void TerminalReopen();

  void Cancel();
  void Idle();

  TNotifyEvent & GetOnIdle() { return FOnIdle; }
  void SetOnIdle(TNotifyEvent Value) { FOnIdle = Value; }
  bool GetCancelling() const { return FCancel; };

protected:
  virtual void ProcessEvent();

private:
  TTerminal * FTerminal;

  TInformationEvent FOnInformation;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TDisplayBannerEvent FOnDisplayBanner;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TNotifyEvent FOnInitializeLog;

  TNotifyEvent FOnIdle;

  TNotifyEvent FAction;
  HANDLE FActionEvent;
  TUserAction * FUserAction;

  Exception * FException;
  Exception * FIdleException;
  bool FCancel;
  bool FCancelled;
  bool FPendingIdle;

  DWORD FMainThread;
  TCriticalSection * FSection;

  void WaitForUserAction(TUserAction * UserAction);
  void RunAction(TNotifyEvent Action);

  static void SaveException(Exception & E, Exception *& Exception);
  static void Rethrow(Exception *& Exception);
  void FatalAbort();
  void CheckCancel();

  void TerminalOpenEvent(TObject * Sender);
  void TerminalReopenEvent(TObject * Sender);

  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & Query, TStrings * MoreMessages, uintptr_t Answers,
    const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    const UnicodeString & SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, intptr_t Options);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, int Progress, bool & Cancel);
  void TerminalInitializeLog(TObject * Sender);

private:
  TTerminalThread(const TTerminalThread &);
  TTerminalThread & operator = (const TTerminalThread &);
};
//---------------------------------------------------------------------------
#endif
