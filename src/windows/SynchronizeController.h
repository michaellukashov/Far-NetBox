//---------------------------------------------------------------------------
#ifndef SynchronizeControllerH
#define SynchronizeControllerH
//---------------------------------------------------------------------------
#include <CoreDefs.hpp>
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
DEFINE_CALLBACK_TYPE2(TSynchronizeAbortEvent, void,
  TObject * /* Sender */, bool /* Close */);
DEFINE_CALLBACK_TYPE2(TSynchronizeThreadsEvent, void,
  TObject * /* Sender */, TThreadMethod /* Method */);
enum TSynchronizeLogEntry { slScan, slStart, slChange, slUpload, slDelete, slDirChange };
DEFINE_CALLBACK_TYPE3(TSynchronizeLogEvent, void,
  TSynchronizeController * /* Controller */, TSynchronizeLogEntry /* Entry */, const UnicodeString & /* Message */);
DEFINE_CALLBACK_TYPE8(TSynchronizeStartStopEvent, void,
  TObject * /* Sender */, bool /* Start */, const TSynchronizeParamType & /* Params */,
  const TCopyParamType & /* CopyParam */, TSynchronizeOptions * /* Options */,
  TSynchronizeAbortEvent /* OnAbort */, TSynchronizeThreadsEvent /* OnSynchronizeThreads */,
  TSynchronizeLogEvent /* OnSynchronizeLog */);
DEFINE_CALLBACK_TYPE8(TSynchronizeEvent, void,
  TSynchronizeController * /* Sender */, const UnicodeString & /* LocalDirectory */,
  const UnicodeString & /* RemoteDirectory */, const TCopyParamType & /* CopyParam */,
  const TSynchronizeParamType & /* Params */, TSynchronizeChecklist ** /* Checklist */,
  TSynchronizeOptions * /* Options */, bool /* Full */);
DEFINE_CALLBACK_TYPE3(TSynchronizeInvalidEvent, void,
  TSynchronizeController * /* Sender */, const UnicodeString & /* Directory */, const UnicodeString & /* ErrorStr */);
DEFINE_CALLBACK_TYPE2(TSynchronizeTooManyDirectoriesEvent, void,
  TSynchronizeController * /* Sender */, int & /* MaxDirectories */);
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
  explicit TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories);
  ~TSynchronizeController();

  void StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLogEvent OnSynchronizeLog);
  void __fastcall LogOperation(TSynchronizeOperation Operation, const UnicodeString & FileName);

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

  void __fastcall SynchronizeChange(TObject * Sender, const UnicodeString & Directory,
    bool & SubdirsChanged);
  void __fastcall SynchronizeAbort(bool Close);
  void __fastcall SynchronizeLog(TSynchronizeLogEntry Entry, const UnicodeString & Message);
  void __fastcall SynchronizeInvalid(TObject * Sender, const UnicodeString & Directory,
    const UnicodeString & ErrorStr);
  void __fastcall SynchronizeFilter(TObject * Sender, const UnicodeString & DirectoryName,
    bool & Add);
  void __fastcall SynchronizeTooManyDirectories(TObject * Sender, int & MaxDirectories);
  void __fastcall SynchronizeDirectoriesChange(TObject * Sender, int Directories);
};
//---------------------------------------------------------------------------
#endif
