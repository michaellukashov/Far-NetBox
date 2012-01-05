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
  std::wstring LocalDirectory;
  std::wstring RemoteDirectory;
  int Params;
  int Options;
};
//---------------------------------------------------------------------------
class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
// typedef void (TObject::* TSynchronizeAbortEvent)
  // (TObject * Sender, bool Close);
typedef boost::signal2<void, TObject *, bool> synchronizeabort_signal_type;
typedef synchronizeabort_signal_type::slot_type synchronizeabort_slot_type;
// typedef void (TObject::* TSynchronizeThreadsEvent)
  // (TObject* Sender, const threadmethod_slot_type &slot);
typedef boost::signal2<void, TObject *, const threadmethod_slot_type &> synchronizethreads_signal_type;
typedef synchronizethreads_signal_type::slot_type synchronizethreads_slot_type;
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
// typedef void (TObject::* TSynchronizeLog)
  // (TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const std::wstring Message);
typedef boost::signal3<void, TSynchronizeController *, TSynchronizeLogEntry, const std::wstring &> synchronizelog_signal_type;
typedef synchronizelog_signal_type::slot_type synchronizelog_slot_type;
// typedef void (TObject::* TSynchronizeStartStopEvent)
  // (TObject * Sender, bool Start, const TSynchronizeParamType & Params,
   // const TCopyParamType & CopyParam, TSynchronizeOptions * Options,
   // TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
   // TSynchronizeLog OnSynchronizeLog);
typedef boost::signal8<void, TObject *, bool, const TSynchronizeParamType &,
   const TCopyParamType &, TSynchronizeOptions *,
   const synchronizeabort_slot_type &, const synchronizethreads_slot_type &,
   const synchronizelog_slot_type &> synchronizestartstop_signal_type;
typedef synchronizestartstop_signal_type::slot_type synchronizestartstop_slot_type;
// typedef void (TObject::* TSynchronizeEvent)
  // (TSynchronizeController * Sender, const std::wstring LocalDirectory,
   // const std::wstring RemoteDirectory, const TCopyParamType & CopyParam,
   // const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
   // TSynchronizeOptions * Options, bool Full);
typedef boost::signal8<void, TSynchronizeController *, const std::wstring &,
   const std::wstring &, const TCopyParamType &,
   const TSynchronizeParamType &, TSynchronizeChecklist **,
   TSynchronizeOptions *, bool> synchronize_signal_type;
typedef synchronize_signal_type::slot_type synchronize_slot_type;
// typedef void (TObject::* TSynchronizeInvalidEvent)
  // (TSynchronizeController * Sender, const std::wstring Directory, const std::wstring ErrorStr);
typedef boost::signal3<void, TSynchronizeController *, const std::wstring &, const std::wstring &> synchronizeinvalid_signal_type;
typedef synchronizeinvalid_signal_type::slot_type synchronizeinvalid_slot_type;
// typedef void (TObject::* TSynchronizeTooManyDirectories)
  // (TSynchronizeController * Sender, int & MaxDirectories);
typedef boost::signal2<void, TSynchronizeController *, int &> synchronizetoomanydirectories_signal_type;
typedef synchronizetoomanydirectories_signal_type::slot_type synchronizetoomanydirectories_slot_type;
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
  explicit TSynchronizeController(const synchronize_slot_type &AOnSynchronize,
    const synchronizeinvalid_slot_type &AOnSynchronizeInvalid,
    const synchronizetoomanydirectories_slot_type &AOnTooManyDirectories);
  ~TSynchronizeController();

  void StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    const synchronizeabort_slot_type &OnAbort, const synchronizethreads_slot_type &OnSynchronizeThreads,
    const synchronizelog_slot_type &OnSynchronizeLog);
  void LogOperation(TSynchronizeOperation Operation, const std::wstring &FileName);

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

  void SynchronizeChange(TObject * Sender, const std::wstring &Directory,
    bool &SubdirsChanged);
  void SynchronizeAbort(bool Close);
  void SynchronizeLog(TSynchronizeLogEntry Entry, const std::wstring &Message);
  void SynchronizeInvalid(TObject * Sender, const std::wstring &Directory,
    const std::wstring &ErrorStr);
  void SynchronizeFilter(TObject * Sender, const std::wstring &DirectoryName,
    bool & Add);
  void SynchronizeTooManyDirectories(TObject * Sender, int & MaxDirectories);
  void SynchronizeDirectoriesChange(TObject * Sender, int Directories);
};
//---------------------------------------------------------------------------
#endif
