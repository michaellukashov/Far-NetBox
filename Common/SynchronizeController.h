//---------------------------------------------------------------------------
#ifndef SynchronizeControllerH
#define SynchronizeControllerH
//---------------------------------------------------------------------------
#include <CopyParam.h>
//---------------------------------------------------------------------------
struct TSynchronizeParamType
{
  wstring LocalDirectory;
  wstring RemoteDirectory;
  int Params;
  int Options;
};
//---------------------------------------------------------------------------
class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
typedef void (TObject::* TSynchronizeAbortEvent)
  (TObject * Sender, bool Close);
typedef void (TObject::* TSynchronizeThreadsEvent)
  (TObject* Sender, TThreadMethod Method);
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef void (TObject::* TSynchronizeLog)
  (TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const wstring Message);
typedef void (TObject::* TSynchronizeStartStopEvent)
  (TObject * Sender, bool Start, const TSynchronizeParamType & Params,
   const TCopyParamType & CopyParam, TSynchronizeOptions * Options,
   TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
   TSynchronizeLog OnSynchronizeLog);
typedef void (TObject::* TSynchronizeEvent)
  (TSynchronizeController * Sender, const wstring LocalDirectory,
   const wstring RemoteDirectory, const TCopyParamType & CopyParam,
   const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
   TSynchronizeOptions * Options, bool Full);
typedef void (TObject::* TSynchronizeInvalidEvent)
  (TSynchronizeController * Sender, const wstring Directory, const wstring ErrorStr);
typedef void (TObject::* TSynchronizeTooManyDirectories)
  (TSynchronizeController * Sender, int & MaxDirectories);
//---------------------------------------------------------------------------
namespace Discmon
{
class TDiscMonitor;
}
//---------------------------------------------------------------------------
enum TSynchronizeOperation { soUpload, soDelete };
//---------------------------------------------------------------------------
class TSynchronizeController
{
public:
  TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectories AOnTooManyDirectories);
  ~TSynchronizeController();

  void StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLog OnSynchronizeLog);
  void LogOperation(TSynchronizeOperation Operation, const wstring FileName);

private:
  TSynchronizeEvent FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions;
  TSynchronizeThreadsEvent FOnSynchronizeThreads;
  Discmon::TDiscMonitor * FSynchronizeMonitor;
  TSynchronizeAbortEvent FSynchronizeAbort;
  TSynchronizeInvalidEvent FOnSynchronizeInvalid;
  TSynchronizeTooManyDirectories FOnTooManyDirectories;
  TSynchronizeLog FSynchronizeLog;
  TCopyParamType FCopyParam;

  void SynchronizeChange(TObject * Sender, const wstring Directory,
    bool & SubdirsChanged);
  void SynchronizeAbort(bool Close);
  void SynchronizeLog(TSynchronizeLogEntry Entry, const wstring Message);
  void SynchronizeInvalid(TObject * Sender, const wstring Directory,
    const wstring ErrorStr);
  void SynchronizeFilter(TObject * Sender, const wstring DirectoryName,
    bool & Add);
  void SynchronizeTooManyDirectories(TObject * Sender, int & MaxDirectories);
  void SynchronizeDirectoriesChange(TObject * Sender, int Directories);
};
//---------------------------------------------------------------------------
#endif
