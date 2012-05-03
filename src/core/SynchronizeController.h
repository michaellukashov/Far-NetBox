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
typedef boost::signal2<void, TObject *, bool> synchronizeabort_signal_type;
typedef synchronizeabort_signal_type::slot_type synchronizeabort_slot_type;
typedef boost::signal2<void, TObject *, const threadmethod_slot_type &> synchronizethreads_signal_type;
typedef synchronizethreads_signal_type::slot_type synchronizethreads_slot_type;
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
typedef boost::signal3<void, TSynchronizeController *, TSynchronizeLogEntry, const UnicodeString > synchronizelog_signal_type;
typedef synchronizelog_signal_type::slot_type synchronizelog_slot_type;
typedef boost::signal8<void, TObject *, bool, const TSynchronizeParamType &,
        const TCopyParamType &, TSynchronizeOptions *,
        const synchronizeabort_slot_type &, const synchronizethreads_slot_type &,
        const synchronizelog_slot_type &> synchronizestartstop_signal_type;
typedef synchronizestartstop_signal_type::slot_type synchronizestartstop_slot_type;
typedef boost::signal8<void, TSynchronizeController *, const UnicodeString,
        const UnicodeString, const TCopyParamType &,
        const TSynchronizeParamType &, TSynchronizeChecklist **,
        TSynchronizeOptions *, bool> synchronize_signal_type;
typedef synchronize_signal_type::slot_type synchronize_slot_type;
typedef boost::signal3<void, TSynchronizeController *, const UnicodeString, const UnicodeString > synchronizeinvalid_signal_type;
typedef synchronizeinvalid_signal_type::slot_type synchronizeinvalid_slot_type;
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
  explicit TSynchronizeController(const synchronize_slot_type & AOnSynchronize,
                                  const synchronizeinvalid_slot_type & AOnSynchronizeInvalid,
                                  const synchronizetoomanydirectories_slot_type & AOnTooManyDirectories);
  ~TSynchronizeController();

  void StartStop(TObject * Sender, bool Start,
                 const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
                 TSynchronizeOptions * Options,
                 const synchronizeabort_slot_type & OnAbort, const synchronizethreads_slot_type & OnSynchronizeThreads,
                 const synchronizelog_slot_type & OnSynchronizeLog);
  void LogOperation(TSynchronizeOperation Operation, const UnicodeString FileName);

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

  void SynchronizeChange(TObject * Sender, const UnicodeString Directory,
                         bool & SubdirsChanged);
  void SynchronizeAbort(bool Close);
  void SynchronizeLog(TSynchronizeLogEntry Entry, const UnicodeString Message);
  void SynchronizeInvalid(TObject * Sender, const UnicodeString Directory,
                          const UnicodeString ErrorStr);
  void SynchronizeFilter(TObject * Sender, const UnicodeString DirectoryName,
                         bool & Add);
  void SynchronizeTooManyDirectories(TObject * Sender, int & MaxDirectories);
  void SynchronizeDirectoriesChange(TObject * Sender, int Directories);
};
//---------------------------------------------------------------------------
#endif
