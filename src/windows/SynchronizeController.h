#pragma once

#include <CopyParam.h>

struct NB_CORE_EXPORT TSynchronizeParamType : public TObject
{
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  int32_t Params{0};
  int32_t Options{0};
};

class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;

using TSynchronizeAbortEvent = nb::FastDelegate2<void,
  TObject * /*Sender*/, bool /*Close*/>;
using TSynchronizeThreadsEvent = nb::FastDelegate2<void,
  TObject * /*Sender*/, TThreadMethod /*Method*/>;
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange, slContinuedError, };
using TSynchronizeLogEvent = nb::FastDelegate3<void,
  TSynchronizeController * /*Controller*/, TSynchronizeLogEntry /*Entry*/,
  UnicodeString /*Message*/>;
using TSynchronizeStartStopEvent = nb::FastDelegate8<void,
  TObject * /*Sender*/, bool /*Start*/, const TSynchronizeParamType & /*Params*/,
  const TCopyParamType & /*CopyParam*/, TSynchronizeOptions * /*Options*/,
  TSynchronizeAbortEvent /*OnAbort*/, TSynchronizeThreadsEvent /*OnSynchronizeThreads*/,
  TSynchronizeLogEvent /*OnSynchronizeLog*/>;
using TSynchronizeEvent = nb::FastDelegate8<void,
  TSynchronizeController * /*Sender*/, UnicodeString /*LocalDirectory*/,
  UnicodeString /*RemoteDirectory*/, const TCopyParamType & /*CopyParam*/,
  const TSynchronizeParamType & /*Params*/, TSynchronizeChecklist ** /*Checklist*/,
  TSynchronizeOptions * /*Options*/, bool /*Full*/>;
using TSynchronizeInvalidEvent = nb::FastDelegate3<void,
  TSynchronizeController * /*Sender*/, UnicodeString /*Directory*/,
  UnicodeString /*ErrorStr*/>;
using TSynchronizeTooManyDirectoriesEvent = nb::FastDelegate2<void,
  TSynchronizeController * /*Sender*/,
  int32_t & /*MaxDirectories*/>;

namespace Discmon
{
class TDiscMonitor;
}

enum TSynchronizeOperation { soUpload, soDelete };

class NB_CORE_EXPORT TSynchronizeController : public TObject
{
  NB_DISABLE_COPY(TSynchronizeController)
public:
  explicit TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories) noexcept;
  virtual ~TSynchronizeController() noexcept;

  void StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType &CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLogEvent OnSynchronizeLog);
  void LogOperation(TSynchronizeOperation Operation, const UnicodeString AFileName);

private:
  TSynchronizeEvent FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions{nullptr};
  TSynchronizeThreadsEvent FOnSynchronizeThreads;
  Discmon::TDiscMonitor * FSynchronizeMonitor{nullptr};
  TSynchronizeAbortEvent FSynchronizeAbort;
  TSynchronizeInvalidEvent FOnSynchronizeInvalid;
  TSynchronizeTooManyDirectoriesEvent FOnTooManyDirectories;
  TSynchronizeLogEvent FSynchronizeLog;
  TCopyParamType FCopyParam;

  void SynchronizeChange(TObject * Sender, const UnicodeString Directory,
    bool & SubdirsChanged);
  void SynchronizeAbort(bool Close);
  void SynchronizeLog(TSynchronizeLogEntry Entry, const UnicodeString Message);
  void SynchronizeInvalid(TObject * Sender, const UnicodeString Directory,
    const UnicodeString ErrorStr);
  void SynchronizeFilter(TObject * Sender, const UnicodeString DirectoryName,
    bool & Add);
  void SynchronizeTooManyDirectories(TObject * Sender, int32_t & MaxDirectories);
  void SynchronizeDirectoriesChange(TObject * Sender, int32_t Directories);
};

void LogSynchronizeEvent(TTerminal * Terminal, const UnicodeString Message);

