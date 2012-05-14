//---------------------------------------------------------------------------
#ifndef SynchronizeControllerH
#define SynchronizeControllerH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"
#include <boost/signals/signal3.hpp>
#include <boost/signals/signal8.hpp>

#include <CopyParam.h>
//---------------------------------------------------------------------------
struct TSynchronizeParamType
{
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  int Params;
  int Options;
};
//---------------------------------------------------------------------------
class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
#ifndef _MSC_VER
typedef void __fastcall (__closure * TSynchronizeAbortEvent)
  (System::TObject * Sender, bool Close);
typedef void __fastcall (__closure * TSynchronizeThreadsEvent)
  (TObject* Sender, TThreadMethodEvent Method);
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef void __fastcall (__closure * TSynchronizeLogEvent)
  (TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const UnicodeString Message);
typedef void __fastcall (__closure * TSynchronizeStartStopEvent)
  (System::TObject * Sender, bool Start, const TSynchronizeParamType & Params,
   const TCopyParamType & CopyParam, TSynchronizeOptions * Options,
   TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
   TSynchronizeLogEvent OnSynchronizeLog);
typedef void __fastcall (__closure * TSynchronizeEvent)
  (TSynchronizeController * Sender, const UnicodeString LocalDirectory,
   const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
   const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
   TSynchronizeOptions * Options, bool Full);
typedef void __fastcall (__closure * TSynchronizeInvalidEvent)
  (TSynchronizeController * Sender, const UnicodeString Directory, const UnicodeString ErrorStr);
typedef void __fastcall (__closure * TSynchronizeTooManyDirectoriesEvent)
  (TSynchronizeController * Sender, int & MaxDirectories);
#else
typedef boost::signal2<void, TObject * /* Sender */, bool /* Close */> TSynchronizeAbortSignal;
typedef TSynchronizeAbortSignal::slot_type TSynchronizeAbortEvent;
typedef boost::signal2<void, TObject* /* Sender */, TThreadMethodEvent /* Method */ > TSynchronizeThreadsSignal;
typedef TSynchronizeThreadsSignal::slot_type TSynchronizeThreadsEvent;
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef boost::signal3<void, TSynchronizeController * /* Controller */, TSynchronizeLogEntry /* Entry */, const UnicodeString /* Message */ > TSynchronizeLogSignal;
typedef TSynchronizeLogSignal::slot_type TSynchronizeLogEvent;
typedef boost::signal8<void, TObject * /* Sender */, bool /* Start */, const TSynchronizeParamType & /* Params */,
   const TCopyParamType & /* CopyParam */, TSynchronizeOptions * /* Options */,
   TSynchronizeAbortEvent /* OnAbort */, TSynchronizeThreadsEvent /* OnSynchronizeThreads */,
   TSynchronizeLogEvent /* OnSynchronizeLog */ > TSynchronizeStartStopSignal;
typedef TSynchronizeStartStopSignal::slot_type TSynchronizeStartStopEvent;
typedef boost::signal8<void, TSynchronizeController * /* Sender */, const UnicodeString /* LocalDirectory */,
   const UnicodeString /* RemoteDirectory */, const TCopyParamType & /* CopyParam */,
   const TSynchronizeParamType & /* Params */, TSynchronizeChecklist ** /* Checklist */,
   TSynchronizeOptions * /* Options */, bool /* Full */ > TSynchronizeSignal;
typedef TSynchronizeSignal::slot_type TSynchronizeEvent;
typedef boost::signal3<void, TSynchronizeController * /* Sender */, const UnicodeString /* Directory */, const UnicodeString /* ErrorStr */ > TSynchronizeInvalidSignal;
typedef TSynchronizeInvalidSignal::slot_type TSynchronizeInvalidEvent;
typedef boost::signal2<void, TSynchronizeController * /* Sender */, int & /* MaxDirectories */ > TSynchronizeTooManyDirectoriesSignal;
typedef TSynchronizeTooManyDirectoriesSignal::slot_type TSynchronizeTooManyDirectoriesEvent;
#endif
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
  explicit /* __fastcall */ TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories);
  /* __fastcall */ ~TSynchronizeController();

  void /* __fastcall */ StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLogEvent OnSynchronizeLog);
  void __fastcall LogOperation(TSynchronizeOperation Operation, const UnicodeString FileName);

private:
  TSynchronizeSignal FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions;
  TSynchronizeThreadsSignal FOnSynchronizeThreads;
  Discmon::TDiscMonitor * FSynchronizeMonitor;
  TSynchronizeAbortSignal FSynchronizeAbort;
  TSynchronizeInvalidSignal FOnSynchronizeInvalid;
  TSynchronizeTooManyDirectoriesSignal FOnTooManyDirectories;
  TSynchronizeLogSignal FSynchronizeLog;
  TCopyParamType FCopyParam;

  void __fastcall SynchronizeChange(TObject * Sender, const UnicodeString Directory,
    bool & SubdirsChanged);
  void __fastcall SynchronizeAbort(bool Close);
  void __fastcall SynchronizeLog(TSynchronizeLogEntry Entry, const UnicodeString Message);
  void __fastcall SynchronizeInvalid(TObject * Sender, const UnicodeString Directory,
    const UnicodeString ErrorStr);
  void __fastcall SynchronizeFilter(TObject * Sender, const UnicodeString DirectoryName,
    bool & Add);
  void __fastcall SynchronizeTooManyDirectories(TObject * Sender, int & MaxDirectories);
  void __fastcall SynchronizeDirectoriesChange(TObject * Sender, int Directories);
};
//---------------------------------------------------------------------------
#endif
