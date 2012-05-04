//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>

#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread : public TObject
{
public:
  explicit TSimpleThread();
  virtual ~TSimpleThread();
  virtual void __fastcall Init();

  virtual void __fastcall Start();
  void __fastcall WaitFor(unsigned int Milliseconds = INFINITE);
  virtual void __fastcall Terminate() = 0;
  void __fastcall Close();
  bool __fastcall IsFinished();

  static int __fastcall ThreadProc(void * Thread);
protected:
  HANDLE FThread;
  bool FFinished;

  virtual void __fastcall Execute() = 0;
  virtual void __fastcall Finished();

private:
  TSimpleThread(const TSimpleThread &);
  TSimpleThread & operator = (const TSimpleThread &);
};
//---------------------------------------------------------------------------
class TSignalThread : public TSimpleThread
{
public:
  virtual void __fastcall Init(bool LowPriority);
  virtual void __fastcall Start();
  virtual void __fastcall Terminate();
  void __fastcall TriggerEvent();

protected:
  HANDLE FEvent;
  bool FTerminated;

  explicit TSignalThread();
  virtual ~TSignalThread();

  bool __fastcall WaitForEvent();
  virtual void __fastcall Execute();
  virtual void __fastcall ProcessEvent() = 0;
};
//---------------------------------------------------------------------------
class TTerminal;
class TQueueItem;
class TCriticalSection;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure * TQueueListUpdate)
  (TTerminalQueue * Queue);
typedef void __fastcall (__closure * TQueueItemUpdateEvent)
  (TTerminalQueue * Queue, TQueueItem * Item);
enum TQueueEvent { qeEmpty, qePendingUserAction };
typedef void __fastcall (__closure * TQueueEventEvent)
  (TTerminalQueue * Queue, TQueueEvent Event);
#else
typedef boost::signal1<void, TTerminalQueue *> queuelistupdate_signal_type;
typedef queuelistupdate_signal_type::slot_type TQueueListUpdate;
typedef boost::signal2<void, TTerminalQueue *, TQueueItem *> queueitemupdate_signal_type;
typedef queueitemupdate_signal_type::slot_type TQueueItemUpdateEvent;
enum TQueueEvent { qeEmpty, qePendingUserAction };
typedef boost::signal2<void, TTerminalQueue *, TQueueEvent> queueevent_signal_type;
typedef queueevent_signal_type::slot_type TQueueEventEvent;
#endif
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;

public:
  explicit TTerminalQueue(TTerminal * Terminal, TConfiguration * Configuration);
  virtual ~TTerminalQueue();

  virtual void __fastcall Init();
  void __fastcall AddItem(TQueueItem * Item);
  TTerminalQueueStatus * __fastcall CreateStatus(TTerminalQueueStatus * Current);
  void __fastcall Idle();

#ifndef _MSC_VER
  __property bool IsEmpty = { read = GetIsEmpty };
  __property int TransfersLimit = { read = FTransfersLimit, write = SetTransfersLimit };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TQueueListUpdate OnListUpdate = { read = FOnListUpdate, write = FOnListUpdate };
  __property TQueueItemUpdateEvent OnQueueItemUpdate = { read = FOnQueueItemUpdate, write = FOnQueueItemUpdate };
  __property TQueueEventEvent OnEvent = { read = FOnEvent, write = FOnEvent };
#else
  bool __fastcall GetIsEmpty();
  int __fastcall GetTransfersLimit() { return FTransfersLimit; }
  // void __fastcall SetTransfersLimit(int value);
  queryuser_signal_type & __fastcall GetOnQueryUser() { return FOnQueryUser; }
  void __fastcall SetOnQueryUser(const TQueryUserEvent & value) { FOnQueryUser.connect(value); }
  promptuser_signal_type & __fastcall GetOnPromptUser() { return FOnPromptUser; }
  void __fastcall SetOnPromptUser(const TPromptUserEvent & value) { FOnPromptUser.connect(value); }
  extendedexception_signal_type & __fastcall GetOnShowExtendedException() { return FOnShowExtendedException; }
  void __fastcall SetOnShowExtendedException(const TExtendedExceptionEvent & value) { FOnShowExtendedException.connect(value); }
  queuelistupdate_signal_type & __fastcall GetOnListUpdate() { return FOnListUpdate; }
  void __fastcall SetOnListUpdate(const TQueueListUpdate & value) { FOnListUpdate.connect(value); }
  queueitemupdate_signal_type & __fastcall GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void __fastcall SetOnQueueItemUpdate(const TQueueItemUpdateEvent & value) { FOnQueueItemUpdate.connect(value); }
  queueevent_signal_type & __fastcall GetOnEvent() { return FOnEvent; }
  void __fastcall SetOnEvent(const TQueueEventEvent & value) { FOnEvent.connect(value); }
#endif

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
  TTerminal * FTerminal;
  TConfiguration * FConfiguration;
  TSessionData * FSessionData;
  TList * FItems;
  int FItemsInProcess;
  TCriticalSection * FItemsSection;
  int FFreeTerminals;
  TList * FTerminals;
  TList * FForcedItems;
  int FTemporaryTerminals;
  int FOverallTerminals;
  int FTransfersLimit;
  bool FEnabled;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;
  TTerminalQueue * Self;

public:
  TQueueItem * __fastcall GetItem(int Index);
  bool __fastcall ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy);
  bool __fastcall ItemProcessUserAction(TQueueItem * Item, void * Arg);
  bool __fastcall ItemMove(TQueueItem * Item, TQueueItem * BeforeItem);
  bool __fastcall ItemExecuteNow(TQueueItem * Item);
  bool __fastcall ItemDelete(TQueueItem * Item);
  bool __fastcall ItemPause(TQueueItem * Item, bool Pause);
  bool __fastcall ItemSetCPSLimit(TQueueItem * Item, unsigned long CPSLimit);

  void __fastcall RetryItem(TQueueItem * Item);
  void __fastcall DeleteItem(TQueueItem * Item);

  virtual void __fastcall ProcessEvent();
  void __fastcall TerminalFinished(TTerminalItem * TerminalItem);
  bool __fastcall TerminalFree(TTerminalItem * TerminalItem);

  void __fastcall DoQueueItemUpdate(TQueueItem * Item);
  void __fastcall DoListUpdate();
  void __fastcall DoEvent(TQueueEvent Event);

  void __fastcall SetTransfersLimit(int value);
  void __fastcall SetEnabled(bool value);
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
  struct TInfo
  {
    TFileOperation Operation;
    TOperationSide Side;
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
  };

  static bool __fastcall IsUserActionStatus(TStatus Status);

#ifndef _MSC_VER
  __property TStatus Status = { read = GetStatus };
  __property HANDLE CompleteEvent = { read = FCompleteEvent, write = FCompleteEvent };
#else
  TStatus __fastcall GetStatus();
  HANDLE __fastcall GetCompleteEvent() { return FCompleteEvent; }
  void __fastcall SetCompleteEvent(HANDLE value) { FCompleteEvent = value; }
#endif

protected:
  TStatus FStatus;
  TCriticalSection * FSection;
  TTerminalItem * FTerminalItem;
  TFileOperationProgressType * FProgressData;
  TQueueItem::TInfo * FInfo;
  TTerminalQueue * FQueue;
  HANDLE FCompleteEvent;
  long FCPSLimit;
  TQueueItem * Self;

  explicit TQueueItem();
  virtual ~TQueueItem();

  void __fastcall SetStatus(TStatus Status);
  void __fastcall Execute(TTerminalItem * TerminalItem);
  virtual void __fastcall DoExecute(TTerminal * Terminal) = 0;
  void __fastcall SetProgress(TFileOperationProgressType & ProgressData);
  void __fastcall GetData(TQueueItemProxy * Proxy);
  void __fastcall SetCPSLimit(unsigned long CPSLimit);
  virtual UnicodeString __fastcall StartupDirectory() = 0;
};
//---------------------------------------------------------------------------
class TQueueItemProxy
{
friend class TQueueItem;
friend class TTerminalQueueStatus;
friend class TTerminalQueue;

public:
  bool __fastcall Update();
  bool __fastcall ProcessUserAction();
  bool __fastcall Move(bool Sooner);
  bool __fastcall Move(TQueueItemProxy * BeforeItem);
  bool __fastcall ExecuteNow();
  bool __fastcall Delete();
  bool __fastcall Pause();
  bool __fastcall Resume();
  bool __fastcall SetCPSLimit(unsigned long CPSLimit);

#ifndef _MSC_VER
  __property TFileOperationProgressType * ProgressData = { read = GetProgressData };
  __property TQueueItem::TInfo * Info = { read = FInfo };
  __property TQueueItem::TStatus Status = { read = FStatus };
  __property bool ProcessingUserAction = { read = FProcessingUserAction };
  __property int Index = { read = GetIndex };
  __property void * UserData = { read = FUserData, write = FUserData };
#else
  TFileOperationProgressType * __fastcall GetProgressData();
  TQueueItem::TInfo * __fastcall GetInfo() { return FInfo; }
  TQueueItem::TStatus __fastcall GetStatus() { return FStatus; }
  bool __fastcall GetProcessingUserAction() { return FProcessingUserAction; }
  int __fastcall GetIndex();
  void * __fastcall GetUserData() { return FUserData; }
  void __fastcall SetUserData(void * value) { FUserData = value; }
#endif

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;
  void * FUserData;
  TQueueItemProxy * Self;

  explicit TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual ~TQueueItemProxy();
#ifndef _MSC_VER
  int __fastcall GetIndex();
  TFileOperationProgressType * __fastcall GetProgressData();
#endif
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus
{
friend class TTerminalQueue;
friend class TQueueItemProxy;

public:
  virtual ~TTerminalQueueStatus();

  TQueueItemProxy * __fastcall FindByQueueItem(TQueueItem * QueueItem);

#ifndef _MSC_VER
  __property int Count = { read = GetCount };
  __property int ActiveCount = { read = GetActiveCount };
  __property TQueueItemProxy * Items[int Index] = { read = GetItem };
#else
  int __fastcall GetCount();
  int __fastcall GetActiveCount();
  TQueueItemProxy * __fastcall GetItem(int Index);
#endif

protected:
  TTerminalQueueStatus();

  void __fastcall Add(TQueueItemProxy * ItemProxy);
  void __fastcall Delete(TQueueItemProxy * ItemProxy);
  void __fastcall ResetStats();

private:
  TList * FList;
  int FActiveCount;

#ifndef _MSC_VER
  int __fastcall GetCount();
  int __fastcall GetActiveCount();
  TQueueItemProxy * __fastcall GetItem(int Index);
#endif
};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
  explicit TLocatedQueueItem(TTerminal * Terminal);
  virtual ~TLocatedQueueItem()
  {}

  virtual void __fastcall DoExecute(TTerminal * Terminal);
  virtual UnicodeString __fastcall StartupDirectory();

private:
  UnicodeString FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
  explicit TTransferQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, TOperationSide Side);
  virtual ~TTransferQueueItem();

protected:
  TStrings * FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType * FCopyParam;
  int FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  explicit TUploadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params);
  virtual ~TUploadQueueItem()
  {}
protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
  explicit TDownloadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params);
  virtual ~TDownloadQueueItem()
  {}
protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TUserAction;
class TTerminalThread : public TSignalThread
{
public:
  explicit TTerminalThread(TTerminal * Terminal);
  virtual ~TTerminalThread();
  virtual void __fastcall Init();

  void __fastcall TerminalOpen();
  void __fastcall TerminalReopen();

  void __fastcall Cancel();

#ifndef _MSC_VER
  __property TNotifyEvent OnIdle = { read = FOnIdle, write = FOnIdle };
  __property bool Cancelling = { read = FCancel };
#else
  notify_signal_type & GetOnIdle() { return FOnIdle; }
  void SetOnIdle(const TNotifyEvent & Value) { FOnIdle.connect(Value); }
  bool GetCancelling() const { return FCancel; };
#endif

protected:
  virtual void __fastcall ProcessEvent();

private:
  TTerminal * FTerminal;

  informationevent_signal_type FOnInformation;
  queryuser_signal_type FOnQueryUser;
  promptuser_signal_type FOnPromptUser;
  extendedexception_signal_type FOnShowExtendedException;
  displaybanner_signal_type FOnDisplayBanner;
  notify_signal_type FOnChangeDirectory;
  readdirectory_signal_type FOnReadDirectory;
  notify_signal_type FOnStartReadDirectory;
  readdirectoryprogress_signal_type FOnReadDirectoryProgress;

  notify_signal_type FOnIdle;

  notify_signal_type FAction;
  HANDLE FActionEvent;
  TUserAction * FUserAction;

  Exception * FException;
  bool FCancel;
  bool FCancelled;
  TTerminalThread * Self;

  void __fastcall WaitForUserAction(TUserAction * UserAction);
  void __fastcall RunAction(const TNotifyEvent & Action);

  void __fastcall SaveException(Exception & E);
  void __fastcall Rethrow();
  void __fastcall FatalAbort();
  void __fastcall CheckCancel();

  void TerminalOpenEvent(TObject * Sender);
  void TerminalReopenEvent(TObject * Sender);

  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, int Options);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, int Progress, bool & Cancel);
};
//---------------------------------------------------------------------------
#endif
