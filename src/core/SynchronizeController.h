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
  (TObject* Sender, TThreadMethod Method);
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef void __fastcall (__closure * TSynchronizeLog)
  (TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const UnicodeString Message);
typedef void __fastcall (__closure * TSynchronizeStartStopEvent)
  (System::TObject * Sender, bool Start, const TSynchronizeParamType & Params,
   const TCopyParamType & CopyParam, TSynchronizeOptions * Options,
   TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
   TSynchronizeLog OnSynchronizeLog);
typedef void __fastcall (__closure * TSynchronizeEvent)
  (TSynchronizeController * Sender, const UnicodeString LocalDirectory,
   const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
   const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
   TSynchronizeOptions * Options, bool Full);
typedef void __fastcall (__closure * TSynchronizeInvalidEvent)
  (TSynchronizeController * Sender, const UnicodeString Directory, const UnicodeString ErrorStr);
typedef void __fastcall (__closure * TSynchronizeTooManyDirectories)
  (TSynchronizeController * Sender, int & MaxDirectories);
#endif
typedef boost::signal2<void, TObject * /* Sender */, bool /* Close */> synchronizeabort_signal_type;
typedef synchronizeabort_signal_type::slot_type TSynchronizeAbortEvent;
typedef boost::signal2<void, TObject* /* Sender */, TThreadMethod /* Method */ > synchronizethreads_signal_type;
typedef synchronizethreads_signal_type::slot_type TSynchronizeThreadsEvent;
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef boost::signal3<void, TSynchronizeController * /* Controller */, TSynchronizeLogEntry /* Entry */, const UnicodeString /* Message */ > synchronizelog_signal_type;
typedef synchronizelog_signal_type::slot_type TSynchronizeLog;
typedef boost::signal8<void, TObject * /* Sender */, bool /* Start */, const TSynchronizeParamType & /* Params */,
   const TCopyParamType & /* CopyParam */, TSynchronizeOptions * /* Options */,
   TSynchronizeAbortEvent /* OnAbort */, TSynchronizeThreadsEvent /* OnSynchronizeThreads */,
   TSynchronizeLog /* OnSynchronizeLog */ > synchronizestartstop_signal_type;
typedef synchronizestartstop_signal_type::slot_type TSynchronizeStartStopEvent;
typedef boost::signal8<void, TSynchronizeController * /* Sender */, const UnicodeString /* LocalDirectory */,
   const UnicodeString /* RemoteDirectory */, const TCopyParamType & /* CopyParam */,
   const TSynchronizeParamType & /* Params */, TSynchronizeChecklist ** /* Checklist */,
   TSynchronizeOptions * /* Options */, bool /* Full */ > synchronize_signal_type;
typedef synchronize_signal_type::slot_type TSynchronizeEvent;
typedef boost::signal3<void, TSynchronizeController * /* Sender */, const UnicodeString /* Directory */, const UnicodeString /* ErrorStr */ > synchronizeinvalid_signal_type;
typedef synchronizeinvalid_signal_type::slot_type TSynchronizeInvalidEvent;
typedef boost::signal2<void, TSynchronizeController * /* Sender */, int & /* MaxDirectories */ > synchronizetoomanydirectories_signal_type;
typedef synchronizetoomanydirectories_signal_type::slot_type TSynchronizeTooManyDirectories;
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
    TSynchronizeTooManyDirectories AOnTooManyDirectories);
  /* __fastcall */ ~TSynchronizeController();

  void __fastcall StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLog OnSynchronizeLog);
  void __fastcall LogOperation(TSynchronizeOperation Operation, const UnicodeString FileName);

private:
  synchronize_signal_type FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions;
  synchronizethreads_signal_type FOnSynchronizeThreads;
  Discmon::TDiscMonitor * FSynchronizeMonitor;
  synchronizeabort_signal_type FSynchronizeAbort;
  synchronizeinvalid_signal_type FOnSynchronizeInvalid;
  synchronizetoomanydirectories_signal_type FOnTooManyDirectories;
  synchronizelog_signal_type FSynchronizeLog;
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
