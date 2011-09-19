//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>

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

  static int ThreadProc(void *Thread);
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
// typedef void (TObject::*TQueueListUpdate)
  // (TTerminalQueue *Queue);
typedef boost::signal1<void, TTerminalQueue *> queuelistupdate_signal_type;
typedef queuelistupdate_signal_type::slot_type queuelistupdate_slot_type;
// typedef void (TObject::*TQueueItemUpdateEvent)
  // (TTerminalQueue *Queue, TQueueItem *Item);
typedef boost::signal2<void, TTerminalQueue *, TQueueItem *> queueitemupdate_signal_type;
typedef queueitemupdate_signal_type::slot_type queueitemupdate_slot_type;
enum TQueueEvent { qeEmpty, qePendingUserAction };
// typedef void (TObject::*TQueueEventEvent)
  // (TTerminalQueue *Queue, TQueueEvent Event);
typedef boost::signal2<void, TTerminalQueue *, TQueueEvent> queueevent_signal_type;
typedef queueevent_signal_type::slot_type queueevent_slot_type;
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;

public:
  TTerminalQueue(TTerminal *Terminal, TConfiguration *Configuration);
  virtual ~TTerminalQueue();

  void AddItem(TQueueItem *Item);
  TTerminalQueueStatus *CreateStatus(TTerminalQueueStatus *Current);
  void Idle();

  // __property bool IsEmpty = { read = GetIsEmpty };
  bool GetIsEmpty();
  // __property int TransfersLimit = { read = FTransfersLimit, write = SetTransfersLimit };
  int GetTransfersLimit() { return FTransfersLimit; }
  void SetTransfersLimit(int value);
  // __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  queryuser_signal_type &GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(const queryuser_slot_type &value) { FOnQueryUser.connect(value); }
  // __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  promptuser_signal_type &GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(const promptuser_slot_type &value) { FOnPromptUser.connect(value); }
  // __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  extendedexception_signal_type &GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(const extendedexception_slot_type &value) { FOnShowExtendedException.connect(value); }
  // __property TQueueListUpdate OnListUpdate = { read = FOnListUpdate, write = FOnListUpdate };
  queuelistupdate_signal_type &GetOnListUpdate() { return FOnListUpdate; }
  void SetOnListUpdate(const queuelistupdate_signal_type &value) { FOnListUpdate.connect(value); }
  // __property TQueueItemUpdateEvent OnQueueItemUpdate = { read = FOnQueueItemUpdate, write = FOnQueueItemUpdate };
  queueitemupdate_signal_type &GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void SetOnQueueItemUpdate(const queueitemupdate_slot_type &value) { FOnQueueItemUpdate.connect(value); }
  // __property TQueueEventEvent OnEvent = { read = FOnEvent, write = FOnEvent };
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
  TList *FItems;
  int FItemsInProcess;
  TCriticalSection *FItemsSection;
  int FFreeTerminals;
  TList *FTerminals;
  int FTemporaryTerminals;
  int FOverallTerminals;
  int FTransfersLimit;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;
  TTerminalQueue *Self;

  TQueueItem *GetItem(int Index);
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

  void DoQueryUser(TObject *Sender, const std::wstring Query,
    TStrings *MoreMessages, int Answers, const TQueryParams *Params, int & Answer,
    TQueryType Type, void *Arg);
  void DoPromptUser(TTerminal *Terminal, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, TStrings *Prompts,
    TStrings *Results, bool & Result, void *Arg);
  void DoShowExtendedException(TTerminal *Terminal,
    const std::exception *E, void *Arg);
  void DoQueueItemUpdate(TQueueItem *Item);
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
  TCriticalSection *FSection;
  TTerminalItem *FTerminalItem;
  TFileOperationProgressType *FProgressData;
  TQueueItem::TInfo *FInfo;
  TTerminalQueue *FQueue;
  HANDLE FCompleteEvent;
  long FCPSLimit;
  TQueueItem *Self;

  TQueueItem();
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

  // __property TFileOperationProgressType *ProgressData = { read = GetProgressData };
  TFileOperationProgressType *GetProgressData();
  // __property TQueueItem::TInfo *Info = { read = FInfo };
  TQueueItem::TInfo *GetInfo() { return FInfo; }
  // __property TQueueItem::TStatus Status = { read = FStatus };
  TQueueItem::TStatus GetStatus() { return FStatus; }
  // __property bool ProcessingUserAction = { read = FProcessingUserAction };
  bool GetProcessingUserAction() { return FProcessingUserAction; }
  // __property int Index = { read = GetIndex };
  int GetIndex();
  // __property void *UserData = { read = FUserData, write = FUserData };
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
  TQueueItemProxy *Self;

  TQueueItemProxy(TTerminalQueue *Queue, TQueueItem *QueueItem);
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

  // __property int Count = { read = GetCount };
  int GetCount();
  // __property int ActiveCount = { read = GetActiveCount };
  int GetActiveCount();
  // __property TQueueItemProxy *Items[int Index] = { read = GetItem };
  TQueueItemProxy *GetItem(int Index);

protected:
  TTerminalQueueStatus();

  void Add(TQueueItemProxy *ItemProxy);
  void Delete(TQueueItemProxy *ItemProxy);
  void ResetStats();

private:
  TList *FList;
  int FActiveCount;

};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
  TLocatedQueueItem(TTerminal *Terminal);

  virtual void DoExecute(TTerminal *Terminal);
  virtual std::wstring StartupDirectory();

private:
  std::wstring FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
  TTransferQueueItem(TTerminal *Terminal,
    TStrings *FilesToCopy, const std::wstring & TargetDir,
    const TCopyParamType *CopyParam, int Params, TOperationSide Side);
  virtual ~TTransferQueueItem();

protected:
  TStrings *FFilesToCopy;
  std::wstring FTargetDir;
  TCopyParamType *FCopyParam;
  int FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  TUploadQueueItem(TTerminal *Terminal,
    TStrings *FilesToCopy, const std::wstring & TargetDir,
    const TCopyParamType *CopyParam, int Params);

protected:
  virtual void DoExecute(TTerminal *Terminal);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
  TDownloadQueueItem(TTerminal *Terminal,
    TStrings *FilesToCopy, const std::wstring &TargetDir,
    const TCopyParamType *CopyParam, int Params);

protected:
  virtual void DoExecute(TTerminal *Terminal);
};
//---------------------------------------------------------------------------
#endif
