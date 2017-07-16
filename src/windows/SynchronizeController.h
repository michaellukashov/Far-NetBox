#pragma once

#include <CopyParam.h>

struct TSynchronizeParamType : public TObject
{
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  intptr_t Params;
  intptr_t Options;
};

class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;

#if 0
typedef void (__closure * TSynchronizeAbortEvent)
  (System::TObject * Sender, bool Close);
#endif // #if 0
typedef nb::FastDelegate2<void,
  TObject * /*Sender*/, bool /*Close*/> TSynchronizeAbortEvent;
#if 0
typedef void (__closure * TSynchronizeThreadsEvent)
  (TObject* Sender, TThreadMethod Method);
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange, slContinuedError };
#endif // #if 0
typedef nb::FastDelegate2<void,
  TObject * /*Sender*/, TThreadMethod /*Method*/> TSynchronizeThreadsEvent;

enum TSynchronizeLogEntry
{
  slScan,
  slStart,
  slChange,
  slUpload,
  slDelete,
  slDirChange,
  slContinuedError,
};

#if 0
typedef void (__closure * TSynchronizeLog)
  (TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const UnicodeString Message);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TSynchronizeController * /*Controller*/, TSynchronizeLogEntry /*Entry*/,
  UnicodeString /*Message*/> TSynchronizeLogEvent;
#if 0
typedef void (__closure * TSynchronizeStartStopEvent)
  (System::TObject * Sender, bool Start, const TSynchronizeParamType & Params,
   const TCopyParamType & CopyParam, TSynchronizeOptions * Options,
   TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
   TSynchronizeLog OnSynchronizeLog);
#endif // #if 0
typedef nb::FastDelegate8<void,
  TObject * /*Sender*/, bool /*Start*/, const TSynchronizeParamType & /*Params*/,
  const TCopyParamType & /*CopyParam*/, TSynchronizeOptions * /*Options*/,
  TSynchronizeAbortEvent /*OnAbort*/, TSynchronizeThreadsEvent /*OnSynchronizeThreads*/,
  TSynchronizeLogEvent /*OnSynchronizeLog*/> TSynchronizeStartStopEvent;
#if 0
typedef void (__closure * TSynchronizeEvent)
  (TSynchronizeController * Sender, const UnicodeString LocalDirectory,
   const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
   const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
   TSynchronizeOptions * Options, bool Full);
#endif // #if 0
typedef nb::FastDelegate8<void,
  TSynchronizeController * /*Sender*/, UnicodeString /*LocalDirectory*/,
  UnicodeString /*RemoteDirectory*/, const TCopyParamType & /*CopyParam*/,
  const TSynchronizeParamType & /*Params*/, TSynchronizeChecklist ** /*Checklist*/,
  TSynchronizeOptions * /*Options*/, bool /*Full*/> TSynchronizeEvent;
#if 0
typedef void (__closure * TSynchronizeInvalidEvent)
  (TSynchronizeController * Sender, const UnicodeString Directory, const UnicodeString ErrorStr);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TSynchronizeController * /*Sender*/, UnicodeString /*Directory*/,
  UnicodeString /*ErrorStr*/> TSynchronizeInvalidEvent;
#if 0
typedef void (__closure * TSynchronizeTooManyDirectories)
  (TSynchronizeController * Sender, int & MaxDirectories);
#endif // #if 0
typedef nb::FastDelegate2<void,
  TSynchronizeController * /*Sender*/,
  intptr_t & /*MaxDirectories*/> TSynchronizeTooManyDirectoriesEvent;

namespace Discmon {
class TDiscMonitor;
}

enum TSynchronizeOperation
{
  soUpload,
  soDelete
};

class NB_CORE_EXPORT TSynchronizeController : public TObject
{
NB_DISABLE_COPY(TSynchronizeController)
public:
  explicit TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories);
  virtual ~TSynchronizeController();

  void StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLogEvent OnSynchronizeLog);
  void LogOperation(TSynchronizeOperation Operation, UnicodeString AFileName);

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

  void SynchronizeChange(TObject * Sender, UnicodeString Directory,
    bool & SubdirsChanged);
  void SynchronizeAbort(bool Close);
  void SynchronizeLog(TSynchronizeLogEntry Entry, UnicodeString Message);
  void SynchronizeInvalid(TObject * Sender, UnicodeString Directory,
    UnicodeString ErrorStr);
  void SynchronizeFilter(TObject * Sender, UnicodeString DirectoryName,
    bool & Add);
  void SynchronizeTooManyDirectories(TObject * Sender, intptr_t & MaxDirectories);
  void SynchronizeDirectoriesChange(TObject * Sender, intptr_t Directories);
};

