
#pragma once
//---------------------------------------------------------------------------
#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TSimpleThread : public TObject
{
  NB_DISABLE_COPY(TSimpleThread)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSimpleThread); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSimpleThread) || TObject::is(Kind); }
public:
  explicit TSimpleThread(TObjectClassId Kind);
  virtual ~TSimpleThread();
  void InitSimpleThread();

  virtual void Start();
  void WaitFor(uintptr_t Milliseconds = INFINITE) const;
  virtual void Terminate() = 0;
  void Close();
  bool IsFinished() const;

protected:
  HANDLE FThread;
  TThreadID FThreadId;
  bool FFinished;

  virtual void Execute() = 0;
  virtual bool Finished();

public:
  static int ThreadProc(void *Thread);
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TSignalThread : public TSimpleThread
{
  NB_DISABLE_COPY(TSignalThread)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSignalThread); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSignalThread) || TSimpleThread::is(Kind); }
public:
  void InitSignalThread(bool LowPriority, HANDLE Event = nullptr);
  virtual void Start() override;
  virtual void Terminate() override;
  void TriggerEvent() const;

protected:
  HANDLE FEvent;
  bool FTerminated;

  explicit TSignalThread(TObjectClassId Kind);
  virtual ~TSignalThread();

  virtual bool WaitForEvent();
  uintptr_t WaitForEvent(uint32_t Timeout) const;
  virtual void Execute() override;
  virtual void ProcessEvent() = 0;
};
//---------------------------------------------------------------------------
class TTerminal;
class TQueueItem;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;
//---------------------------------------------------------------------------
#if 0
typedef void (__closure *TQueueListUpdate)
(TTerminalQueue *Queue);
#endif // #if 0
typedef nb::FastDelegate1<void,
  TTerminalQueue * /*Queue*/> TQueueListUpdateEvent;
#if 0
typedef void (__closure *TQueueItemUpdateEvent)
(TTerminalQueue *Queue, TQueueItem *Item);
enum TQueueEvent { qeEmpty, qeEmptyButMonitored, qePendingUserAction };
#endif // #if 0
typedef nb::FastDelegate2<void,
  TTerminalQueue * /*Queue*/, TQueueItem * /*Item*/> TQueueItemUpdateEvent;

enum TQueueEvent
{
  qeEmpty,
  qeEmptyButMonitored,
  qePendingUserAction,
};

#if 0
typedef void (__closure *TQueueEventEvent)
(TTerminalQueue *Queue, TQueueEvent Event);
#endif // #if 0
typedef nb::FastDelegate2<void,
  TTerminalQueue * /*Queue*/, TQueueEvent /*Event*/> TQueueEventEvent;

class TTerminalItem;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TTerminalQueue : public TSignalThread
{
  friend class TQueueItem;
  friend class TQueueItemProxy;
  friend class TTransferQueueItem;
  friend class TParallelTransferQueueItem;
  NB_DISABLE_COPY(TTerminalQueue)
public:
  explicit TTerminalQueue(TTerminal *ATerminal, TConfiguration *AConfiguration);
  virtual ~TTerminalQueue();

  void InitTerminalQueue();
  void AddItem(TQueueItem *Item);
  TTerminalQueueStatus *CreateStatus(TTerminalQueueStatus *Current);
  void Idle();

  __property bool IsEmpty = { read = GetIsEmpty };
  __property intptr_t TransfersLimit = { read = FTransfersLimit, write = SetTransfersLimit };
  __property intptr_t KeepDoneItemsFor = { read = FKeepDoneItemsFor, write = SetKeepDoneItemsFor };
  __property int ParallelDurationThreshold = { read = GetParallelDurationThreshold };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TQueueListUpdate OnListUpdate = { read = FOnListUpdate, write = FOnListUpdate };
  __property TQueueItemUpdateEvent OnQueueItemUpdate = { read = FOnQueueItemUpdate, write = FOnQueueItemUpdate };
  __property TQueueEventEvent OnEvent = { read = FOnEvent, write = FOnEvent };

protected:
  friend class TTerminalItem;
  friend class TQueryUserAction;
  friend class TPromptUserAction;
  friend class TShowExtendedExceptionAction;

public:
  intptr_t GetTransfersLimit() const { return FTransfersLimit; }
  intptr_t GetKeepDoneItemsFor() const { return FKeepDoneItemsFor; }
  bool GetEnabled() const { return FEnabled; }
  TQueryUserEvent GetOnQueryUser() const { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent GetOnPromptUser() const { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TExtendedExceptionEvent GetOnShowExtendedException() const { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TQueueListUpdateEvent GetOnListUpdate() const { return FOnListUpdate; }
  void SetOnListUpdate(TQueueListUpdateEvent Value) { FOnListUpdate = Value; }
  TQueueItemUpdateEvent GetOnQueueItemUpdate() const { return FOnQueueItemUpdate; }
  void SetOnQueueItemUpdate(TQueueItemUpdateEvent Value) { FOnQueueItemUpdate = Value; }
  TQueueEventEvent GetOnEvent() const { return FOnEvent; }
  void SetOnEvent(TQueueEventEvent Value) { FOnEvent = Value; }

protected:
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TQueueItemUpdateEvent FOnQueueItemUpdate;
  TQueueListUpdateEvent FOnListUpdate;
  TQueueEventEvent FOnEvent;
  TTerminal *FTerminal;
  TConfiguration *FConfiguration;
  TSessionData *FSessionData;
  TList *FItems;
  TList *FDoneItems;
  intptr_t FItemsInProcess;
  TCriticalSection FItemsSection;
  intptr_t FFreeTerminals;
  TList *FTerminals;
  TList *FForcedItems;
  intptr_t FTemporaryTerminals;
  intptr_t FOverallTerminals;
  intptr_t FTransfersLimit;
  intptr_t FKeepDoneItemsFor;
  bool FEnabled;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;

  static TQueueItem *GetItem(TList *List, intptr_t Index);
  TQueueItem *GetItem(intptr_t Index) const;
  void FreeItemsList(TList *&List) const;
  void UpdateStatusForList(
    TTerminalQueueStatus *Status, TList *List, TTerminalQueueStatus *Current);
  bool ItemGetData(TQueueItem *Item, TQueueItemProxy *Proxy);
  bool ItemProcessUserAction(TQueueItem *Item, void *Arg);
  bool ItemMove(TQueueItem *Item, TQueueItem *BeforeItem);
  bool ItemExecuteNow(TQueueItem *Item);
  bool ItemDelete(TQueueItem *Item);
  bool ItemPause(TQueueItem *Item, bool Pause);
  bool ItemSetCPSLimit(TQueueItem *Item, intptr_t CPSLimit);
  bool ItemGetCPSLimit(TQueueItem *Item, intptr_t &CPSLimit) const;

  void RetryItem(TQueueItem *Item);
  void DeleteItem(TQueueItem *Item, bool CanKeep);

  virtual bool WaitForEvent();
  virtual void ProcessEvent();
  void TerminalFinished(TTerminalItem *TerminalItem);
  bool TerminalFree(TTerminalItem *TerminalItem);
  intptr_t GetParallelDurationThreshold() const;

  void DoQueueItemUpdate(TQueueItem *Item);
  void DoListUpdate();
  void DoEvent(TQueueEvent Event);

public:
  void SetTransfersLimit(intptr_t Value);
  void SetKeepDoneItemsFor(intptr_t Value);
  void SetEnabled(bool Value);
  bool GetIsEmpty() const;

  bool TryAddParallelOperation(TQueueItem *Item, bool Force);
  bool ContinueParallelOperation() const;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TQueueItem : public TObject
{
  friend class TTerminalQueue;
  friend class TTerminalItem;
  friend class TParallelTransferQueueItem;
  NB_DISABLE_COPY(TQueueItem)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TQueueItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TQueueItem) || TObject::is(Kind); }

public:
  enum TStatus
  {
    qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
    qsPaused, qsDone,
  };

  struct TInfo : public TObject
  {
    TInfo() :
      Operation(foNone),
      Side(osLocal),
      SingleFile(false),
      Primary(false),
      GroupToken(nullptr)
    {
    }
    TFileOperation Operation;
    TOperationSide Side;
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
    bool SingleFile;
    bool Primary;
    void *GroupToken;
  };

  static bool IsUserActionStatus(TQueueItem::TStatus Status);

  __property TStatus Status = { read = GetStatus };
  __property HANDLE CompleteEvent = { read = FCompleteEvent, write = FCompleteEvent };

  HANDLE GetCompleteEvent() const { return FCompleteEvent; }
  void SetCompleteEvent(HANDLE Value) { FCompleteEvent = Value; }

protected:
  TStatus FStatus;
  TCriticalSection FSection;
  TTerminalItem *FTerminalItem;
  TFileOperationProgressType *FProgressData;
  TQueueItem::TInfo *FInfo;
  TTerminalQueue *FQueue;
  HANDLE FCompleteEvent;
  intptr_t FCPSLimit;
  TDateTime FDoneAt;

  explicit TQueueItem(TObjectClassId Kind);
  virtual ~TQueueItem();

public:
  void SetMasks(const UnicodeString Value);
  void SetStatus(TStatus Status);
  TStatus GetStatus() const;
  void Execute(TTerminalItem *TerminalItem);
  virtual void DoExecute(TTerminal *Terminal) = 0;
  void SetProgress(TFileOperationProgressType &ProgressData);
  void GetData(TQueueItemProxy *Proxy) const;
  void SetCPSLimit(intptr_t CPSLimit);
  intptr_t GetCPSLimit() const;
  virtual intptr_t DefaultCPSLimit() const;
  virtual UnicodeString GetStartupDirectory() const = 0;
  virtual void ProgressUpdated();
  virtual TQueueItem *CreateParallelOperation();
  bool Complete();
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TQueueItemProxy : public TObject
{
  friend class TQueueItem;
  friend class TTerminalQueueStatus;
  friend class TTerminalQueue;
  NB_DISABLE_COPY(TQueueItemProxy)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TQueueItemProxy); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TQueueItemProxy) || TObject::is(Kind); }
public:
  bool Update();
  bool ProcessUserAction();
  bool Move(bool Sooner);
  bool Move(TQueueItemProxy *BeforeItem);
  bool ExecuteNow();
  bool Delete();
  bool Pause();
  bool Resume();
  bool SetCPSLimit(intptr_t CPSLimit);
  bool GetCPSLimit(intptr_t &CPSLimit) const;

  __property TFileOperationProgressType *ProgressData = { read = GetProgressData };
  __property int64_t TotalTransferred = { read = GetTotalTransferred };
  __property TQueueItem::TInfo *Info = { read = FInfo };
  __property TQueueItem::TStatus Status = { read = FStatus };
  __property bool ProcessingUserAction = { read = FProcessingUserAction };
  __property int Index = { read = GetIndex };
  __property void *UserData = { read = FUserData, write = FUserData };

  TQueueItem::TInfo *GetInfo() const { return FInfo; }
  TQueueItem::TStatus GetStatus() const { return FStatus; }
  bool GetProcessingUserAction() const { return FProcessingUserAction; }
  void *GetUserData() const { return FUserData; }
  void *GetUserData() { return FUserData; }
  void SetUserData(void *Value) { FUserData = Value; }
  void SetMasks(const UnicodeString Value);

private:
  TFileOperationProgressType *FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue *FQueue;
  TQueueItem *FQueueItem;
  TTerminalQueueStatus *FQueueStatus;
  TQueueItem::TInfo *FInfo;
  bool FProcessingUserAction;
  void *FUserData;

  explicit TQueueItemProxy(TTerminalQueue *Queue, TQueueItem *QueueItem);
  virtual ~TQueueItemProxy();

public:
  intptr_t GetIndex() const;
  TFileOperationProgressType *GetProgressData() const;
  int64_t GetTotalTransferred() const;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TTerminalQueueStatus : public TObject
{
  friend class TTerminalQueue;
  friend class TQueueItemProxy;
  NB_DISABLE_COPY(TTerminalQueueStatus)
public:
  virtual ~TTerminalQueueStatus();

  TQueueItemProxy *FindByQueueItem(TQueueItem *QueueItem);

  __property int Count = { read = GetCount };
  __property int DoneCount = { read = FDoneCount };
  __property int ActiveCount = { read = GetActiveCount };
  __property int DoneAndActiveCount = { read = GetDoneAndActiveCount };
  __property int ActivePrimaryCount = { read = GetActivePrimaryCount };
  __property int ActiveAndPendingPrimaryCount = { read = GetActiveAndPendingPrimaryCount };
  __property TQueueItemProxy * Items[int Index] = { read = GetItem };

  bool IsOnlyOneActiveAndNoPending() const;

protected:
  TTerminalQueueStatus();

  void Add(TQueueItemProxy *ItemProxy);
  void Delete(TQueueItemProxy *ItemProxy);
  void ResetStats() const;
  void NeedStats() const;

private:
  TList *FList;
  intptr_t FDoneCount;
  mutable intptr_t FActiveCount;
  mutable intptr_t FActivePrimaryCount;
  mutable intptr_t FActiveAndPendingPrimaryCount;

public:
  intptr_t GetCount() const;
  intptr_t GetActiveCount() const;
  intptr_t GetDoneAndActiveCount() const;
  intptr_t GetActivePrimaryCount() const;
  intptr_t GetActiveAndPendingPrimaryCount() const;
  intptr_t GetDoneCount() const { return FDoneCount; }
  void SetDoneCount(intptr_t Value);
  TQueueItemProxy *GetItem(intptr_t Index) const;
  TQueueItemProxy *GetItem(intptr_t Index);
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TLocatedQueueItem : public TQueueItem
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TLocatedQueueItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TLocatedQueueItem) || TQueueItem::is(Kind); }
protected:
  explicit TLocatedQueueItem(TObjectClassId Kind, TTerminal *Terminal);
  TLocatedQueueItem(const TLocatedQueueItem &Source);
  virtual ~TLocatedQueueItem()
  {
  }

  virtual void DoExecute(TTerminal *Terminal) override;
  virtual UnicodeString GetStartupDirectory() const override;

private:
  UnicodeString FCurrentDir;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TTransferQueueItem : public TLocatedQueueItem
{
  NB_DISABLE_COPY(TTransferQueueItem)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTransferQueueItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTransferQueueItem) || TLocatedQueueItem::is(Kind); }
public:
  explicit TTransferQueueItem(TObjectClassId Kind, TTerminal *Terminal,
    const TStrings *AFilesToCopy, const UnicodeString TargetDir,
    const TCopyParamType *CopyParam, intptr_t Params, TOperationSide Side,
    bool SingleFile, bool Parallel);
  virtual ~TTransferQueueItem();

protected:
  TStrings *FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType *FCopyParam;
  intptr_t FParams;
  bool FParallel;
  DWORD FLastParallelOperationAdded;
  TParallelOperation *FParallelOperation;

  virtual intptr_t DefaultCPSLimit() const override;
  virtual void DoExecute(TTerminal *Terminal) override;
  virtual void DoTransferExecute(TTerminal *Terminal, TParallelOperation *ParallelOperation) = 0;
  virtual void ProgressUpdated() override;
  virtual TQueueItem *CreateParallelOperation() override;

public:
  TParallelOperation *GetParallelOperation() const { return FParallelOperation; }
  TParallelOperation *GetParallelOperation() { return FParallelOperation; }
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TUploadQueueItem : public TTransferQueueItem
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TUploadQueueItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TUploadQueueItem) || TTransferQueueItem::is(Kind); }
public:
  explicit TUploadQueueItem(TTerminal *Terminal,
    const TStrings *AFilesToCopy, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, intptr_t Params, bool SingleFile, bool Parallel);
  virtual ~TUploadQueueItem()
  {
  }

protected:
  virtual void DoTransferExecute(TTerminal *Terminal, TParallelOperation *ParallelOperation) override;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TDownloadQueueItem : public TTransferQueueItem
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TDownloadQueueItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TDownloadQueueItem) || TTransferQueueItem::is(Kind); }
public:
  explicit TDownloadQueueItem(TTerminal *Terminal,
    const TStrings *AFilesToCopy, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, intptr_t Params, bool SingleFile, bool Parallel);
  virtual ~TDownloadQueueItem()
  {
  }

protected:
  virtual void DoTransferExecute(TTerminal *Terminal, TParallelOperation *ParallelOperation) override;
};
//---------------------------------------------------------------------------
class TUserAction;
class NB_CORE_EXPORT TTerminalThread : public TSignalThread
{
  NB_DISABLE_COPY(TTerminalThread)
public:
  explicit TTerminalThread(TTerminal *Terminal);
  void InitTerminalThread();
  virtual ~TTerminalThread();

  void TerminalOpen();
  void TerminalReopen();

  void Cancel();
  bool Release();
  void Idle();

  __property TNotifyEvent OnIdle = { read = FOnIdle, write = FOnIdle };
  __property bool Cancelling = { read = FCancel };
  __property bool AllowAbandon = { read = FAllowAbandon, write = FAllowAbandon };

  TNotifyEvent GetOnIdle() const { return FOnIdle; }
  void SetOnIdle(TNotifyEvent Value) { FOnIdle = Value; }
  bool GetCancelling() const { return FCancel; }

protected:
  virtual void ProcessEvent();
  virtual bool Finished();

private:
  TTerminal *FTerminal;

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
  TUserAction *FUserAction;

  Exception *FException;
  Exception *FIdleException;
  bool FCancel;
  TDateTime FCancelAfter;
  bool FAbandoned;
  bool FCancelled;
  bool FPendingIdle;
  bool FAllowAbandon;

  DWORD FMainThread;
  TCriticalSection FSection;

  void WaitForUserAction(TUserAction *UserAction);
  void RunAction(TNotifyEvent Action);

  static void SaveException(Exception &E, Exception *&Exception);
  static void Rethrow(Exception *&AException);
  void FatalAbort();
  void CheckCancel();

  void TerminalOpenEvent(TObject *Sender);
  void TerminalReopenEvent(TObject *Sender);

  void TerminalInformation(
    TTerminal *Terminal, const UnicodeString &Str, bool Status, intptr_t Phase);
  void TerminalQueryUser(TObject *Sender,
    const UnicodeString &AQuery, TStrings *MoreMessages, uint32_t Answers,
    const TQueryParams *Params, uint32_t &Answer, TQueryType Type, void *Arg);
  void TerminalPromptUser(TTerminal *Terminal, TPromptKind Kind,
    const UnicodeString &Name, const UnicodeString &Instructions,
    TStrings *Prompts, TStrings *Results, bool &Result, void *Arg);
  void TerminalShowExtendedException(TTerminal *Terminal,
    Exception *E, void *Arg);
  void TerminalDisplayBanner(TTerminal *Terminal,
    const UnicodeString &SessionName, const UnicodeString &Banner,
    bool &NeverShowAgain, intptr_t Options, uintptr_t &Params);
  void TerminalChangeDirectory(TObject *Sender);
  void TerminalReadDirectory(TObject *Sender, Boolean ReloadOnly);
  void TerminalStartReadDirectory(TObject *Sender);
  void TerminalReadDirectoryProgress(TObject *Sender, intptr_t Progress, intptr_t ResolvedLinks, bool &Cancel);
  void TerminalInitializeLog(TObject *Sender);
};
//---------------------------------------------------------------------------
