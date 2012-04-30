//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>

#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread : public System::TObject
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
  virtual void __fastcall Init();
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
  explicit TTerminalQueue(TTerminal * Terminal, TConfiguration * Configuration);
  virtual ~TTerminalQueue();

  virtual void __fastcall Init();
  void __fastcall AddItem(TQueueItem * Item);
  TTerminalQueueStatus * __fastcall CreateStatus(TTerminalQueueStatus * Current);
  void __fastcall Idle();

  bool __fastcall GetIsEmpty();
  size_t __fastcall GetTransfersLimit() { return FTransfersLimit; }
  void __fastcall SetTransfersLimit(size_t value);
  queryuser_signal_type & __fastcall GetOnQueryUser() { return FOnQueryUser; }
  void __fastcall SetOnQueryUser(const TQueryUserEvent & value) { FOnQueryUser.connect(value); }
  promptuser_signal_type & __fastcall GetOnPromptUser() { return FOnPromptUser; }
  void __fastcall SetOnPromptUser(const TPromptUserEvent & value) { FOnPromptUser.connect(value); }
  extendedexception_signal_type & __fastcall GetOnShowExtendedException() { return FOnShowExtendedException; }
  void __fastcall SetOnShowExtendedException(const TExtendedExceptionEvent & value) { FOnShowExtendedException.connect(value); }
  queuelistupdate_signal_type & __fastcall GetOnListUpdate() { return FOnListUpdate; }
  void __fastcall SetOnListUpdate(const queuelistupdate_slot_type & value) { FOnListUpdate.connect(value); }
  queueitemupdate_signal_type & __fastcall GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void __fastcall SetOnQueueItemUpdate(const queueitemupdate_slot_type & value) { FOnQueueItemUpdate.connect(value); }
  queueevent_signal_type & __fastcall GetOnEvent() { return FOnEvent; }
  void __fastcall SetOnEvent(const queueevent_slot_type & value) { FOnEvent.connect(value); }

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
  System::TList * FItems;
  size_t FItemsInProcess;
  TCriticalSection * FItemsSection;
  size_t FFreeTerminals;
  System::TList * FTerminals;
  size_t FTemporaryTerminals;
  size_t FOverallTerminals;
  size_t FTransfersLimit;
  System::TDateTime FIdleInterval;
  System::TDateTime FLastIdle;
  TTerminalQueue * Self;

  TQueueItem * __fastcall GetItem(size_t Index);
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

  void __fastcall DoQueryUser(System::TObject * Sender, const UnicodeString Query,
    System::TStrings * MoreMessages, int Answers, const TQueryParams * Params, unsigned int & Answer,
    TQueryType Type, void * Arg);
  void __fastcall DoPromptUser(TTerminal * Terminal, TPromptKind Kind,
    const UnicodeString Name, const UnicodeString Instructions, System::TStrings * Prompts,
    System::TStrings * Results, bool & Result, void * Arg);
  void __fastcall DoShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void __fastcall DoQueueItemUpdate(TQueueItem * Item);
  void __fastcall DoListUpdate();
  void __fastcall DoEvent(TQueueEvent Event);
};
//---------------------------------------------------------------------------
class TQueueItem : public System::TObject
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
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
  };

  static bool __fastcall IsUserActionStatus(TStatus Status);

  TStatus __fastcall GetStatus();
  HANDLE __fastcall GetCompleteEvent() { return FCompleteEvent; }
  void __fastcall SetCompleteEvent(HANDLE value) { FCompleteEvent = value; }

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
  bool FOwnsProgressData;

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
  bool __fastcall ProcessUserAction(void * Arg = NULL);
  bool __fastcall Move(bool Sooner);
  bool __fastcall Move(TQueueItemProxy * BeforeItem);
  bool __fastcall ExecuteNow();
  bool __fastcall Delete();
  bool __fastcall Pause();
  bool __fastcall Resume();
  bool __fastcall SetCPSLimit(unsigned long CPSLimit);

  TFileOperationProgressType * __fastcall GetProgressData();
  TQueueItem::TInfo * __fastcall GetInfo() { return FInfo; }
  TQueueItem::TStatus __fastcall GetStatus() { return FStatus; }
  bool __fastcall GetProcessingUserAction() { return FProcessingUserAction; }
  size_t __fastcall GetIndex();
  void * __fastcall GetUserData() { return FUserData; }
  void __fastcall SetUserData(void * value) { FUserData = value; }

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;
  void * FUserData;
  bool FOwnsProgressData;
  TQueueItemProxy * Self;

  explicit TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual ~TQueueItemProxy();
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus
{
  friend class TTerminalQueue;
  friend class TQueueItemProxy;

public:
  virtual ~TTerminalQueueStatus();

  TQueueItemProxy * __fastcall FindByQueueItem(TQueueItem * QueueItem);

  size_t __fastcall GetCount();
  size_t __fastcall GetActiveCount();
  TQueueItemProxy * __fastcall GetItem(size_t Index);

protected:
  TTerminalQueueStatus();

  void __fastcall Add(TQueueItemProxy * ItemProxy);
  void __fastcall Delete(TQueueItemProxy * ItemProxy);
  void __fastcall ResetStats();

private:
  System::TList * FList;
  size_t FActiveCount;

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
    System::TStrings * FilesToCopy, const UnicodeString TargetDir,
    const TCopyParamType * CopyParam, int Params, TOperationSide Side);
  virtual ~TTransferQueueItem();

protected:
  System::TStrings * FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType * FCopyParam;
  int FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  explicit TUploadQueueItem(TTerminal * Terminal,
    System::TStrings * FilesToCopy, const UnicodeString TargetDir,
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
    System::TStrings * FilesToCopy, const UnicodeString TargetDir,
    const TCopyParamType * CopyParam, int Params);
  virtual ~TDownloadQueueItem()
  {}
protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
#endif
