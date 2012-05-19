//---------------------------------------------------------------------------
#ifndef SynchronizeControllerH
#define SynchronizeControllerH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"

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
typedef fastdelegate::FastDelegate2<void,
  TObject * /* Sender */, bool /* Close */> TSynchronizeAbortEvent;
typedef fastdelegate::FastDelegate2<void,
  TObject* /* Sender */, TThreadMethodEvent /* Method */ > TSynchronizeThreadsEvent;
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef fastdelegate::FastDelegate3<void,
  TSynchronizeController * /* Controller */, TSynchronizeLogEntry /* Entry */, const UnicodeString & /* Message */ > TSynchronizeLogEvent;
typedef fastdelegate::FastDelegate8<void,
  TObject * /* Sender */, bool /* Start */, const TSynchronizeParamType & /* Params */,
  const TCopyParamType & /* CopyParam */, TSynchronizeOptions * /* Options */,
  TSynchronizeAbortEvent /* OnAbort */, TSynchronizeThreadsEvent /* OnSynchronizeThreads */,
  TSynchronizeLogEvent /* OnSynchronizeLog */ > TSynchronizeStartStopEvent;
typedef fastdelegate::FastDelegate8<void,
  TSynchronizeController * /* Sender */, const UnicodeString & /* LocalDirectory */,
  const UnicodeString & /* RemoteDirectory */, const TCopyParamType & /* CopyParam */,
  const TSynchronizeParamType & /* Params */, TSynchronizeChecklist ** /* Checklist */,
  TSynchronizeOptions * /* Options */, bool /* Full */ > TSynchronizeEvent;
typedef fastdelegate::FastDelegate3<void,
  TSynchronizeController * /* Sender */, const UnicodeString & /* Directory */, const UnicodeString & /* ErrorStr */ > TSynchronizeInvalidEvent;
typedef fastdelegate::FastDelegate2<void,
  TSynchronizeController * /* Sender */, int & /* MaxDirectories */ > TSynchronizeTooManyDirectoriesEvent;
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
  TSynchronizeEvent FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions;
  TSynchronizeThreadsEvent FOnSynchronizeThreads;
  Discmon::TDiscMonitor * FSynchronizeMonitor;
  TSynchronizeAbortEvent FSynchronizeAbort;
  TSynchronizeInvalidEvent FOnSynchronizeInvalid;
  TSynchronizeTooManyDirectoriesEvent FOnTooManyDirectories;
  TSynchronizeLogEvent FSynchronizeLog;
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
