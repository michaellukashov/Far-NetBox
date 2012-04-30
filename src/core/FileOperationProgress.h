//---------------------------------------------------------------------------
#ifndef FileOperationProgressH
#define FileOperationProgressH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
#include "Exceptions.h"
#include <vector>

#ifdef _MSC_VER
#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal6.hpp>
#endif

//---------------------------------------------------------------------------
class TFileOperationProgressType;
enum TFileOperation { foNone, foCopy, foMove, foDelete, foSetProperties,
                      foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
                      foGetProperties, foCalculateChecksum
                    };
enum TCancelStatus { csContinue = 0, csCancel, csCancelTransfer, csRemoteAbort };
enum TResumeStatus { rsNotAvailable, rsEnabled, rsDisabled };
enum TBatchOverwrite { boNo, boAll, boNone, boOlder, boAlternateResume, boAppend, boResume };
#ifndef _MSC_VER
typedef void __fastcall (__closure * TFileOperationProgressEvent)
(TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
typedef void __fastcall (__closure * TFileOperationFinished)
(TFileOperation Operation, TOperationSide Side, bool Temp,
 const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
#else
typedef boost::signal2<void, TFileOperationProgressType & /* ProgressData */, TCancelStatus & /* Cancel */> fileoperationprogress_signal_type;
typedef fileoperationprogress_signal_type::slot_type TFileOperationProgressEvent;
typedef boost::signal6<void, TFileOperation /* Operation */, TOperationSide /* Side */, bool /* Temp */,
        const UnicodeString & /* FileName */, bool /* Success */, TOnceDoneOperation & /* OnceDoneOperation */> fileoperationfinished_signal_type;
typedef fileoperationfinished_signal_type::slot_type TFileOperationFinishedEvent;
#endif
//---------------------------------------------------------------------------
class TFileOperationProgressType
{
private:
  // when it was last time suspended (to calculate suspend time in Resume())
  unsigned int FSuspendTime;
  // when current file was started being transfered
  System::TDateTime FFileStartTime;
  size_t FFilesFinished;
  TFileOperationProgressEvent * FOnProgress;
  TFileOperationFinishedEvent * FOnFinished;
  bool FReset;
  size_t FLastSecond;
  size_t FRemainingCPS;
  std::vector<size_t> FTicks;
  std::vector<__int64> FTotalTransferredThen;

protected:
  void __fastcall ClearTransfer();
  inline void __fastcall DoProgress();

public:
  // common data
  TFileOperation Operation;
  // on what side if operation being processed (local/remote), source of copy
  TOperationSide Side;
  UnicodeString FileName;
  UnicodeString Directory;
  bool AsciiTransfer;
  bool TransferingFile;
  bool Temp;

  // file size to read/write
  __int64 LocalSize;
  __int64 LocallyUsed;
  __int64 TransferSize;
  __int64 TransferedSize;
  __int64 SkippedSize;
  TResumeStatus ResumeStatus;
  bool InProgress;
  bool FileInProgress;
  TCancelStatus Cancel;
  size_t Count;
  // when operation started
  System::TDateTime StartTime;
  // bytes transfered
  __int64 TotalTransfered;
  __int64 TotalSkipped;
  __int64 TotalSize;

  TBatchOverwrite BatchOverwrite;
  bool SkipToAll;
  size_t CPSLimit;

  bool TotalSizeSet;

  bool Suspended;

  explicit TFileOperationProgressType();
  explicit TFileOperationProgressType(
    const TFileOperationProgressEvent & AOnProgress,
    const TFileOperationFinishedEvent & AOnFinished);
  ~TFileOperationProgressType();
  void __fastcall AddLocallyUsed(__int64 ASize);
  void __fastcall AddTransfered(__int64 ASize, bool AddToTotals = true);
  void __fastcall AddResumed(__int64 ASize);
  void __fastcall Clear();
  size_t __fastcall CPS();
  void __fastcall Finish(const UnicodeString FileName, bool Success,
                         TOnceDoneOperation & OnceDoneOperation);
  size_t __fastcall LocalBlockSize();
  bool __fastcall IsLocallyDone();
  bool __fastcall IsTransferDone();
  void __fastcall SetFile(const UnicodeString AFileName, bool AFileInProgress = true);
  void __fastcall SetFileInProgress();
  size_t __fastcall OperationProgress();
  size_t __fastcall TransferBlockSize();
  size_t __fastcall AdjustToCPSLimit(size_t Size);
  static size_t __fastcall StaticBlockSize();
  void __fastcall Reset();
  void __fastcall Resume();
  void __fastcall SetLocalSize(__int64 ASize);
  void __fastcall SetAsciiTransfer(bool AAsciiTransfer);
  void __fastcall SetResumeStatus(TResumeStatus AResumeStatus);
  void __fastcall SetTransferSize(__int64 ASize);
  void __fastcall ChangeTransferSize(__int64 ASize);
  void __fastcall RollbackTransfer();
  void __fastcall SetTotalSize(__int64 ASize);
  void __fastcall Start(TFileOperation AOperation, TOperationSide ASide, size_t ACount);
  void __fastcall Start(TFileOperation AOperation,
                        TOperationSide ASide, size_t ACount, bool ATemp, const UnicodeString ADirectory,
                        size_t ACPSLimit);
  void __fastcall Stop();
  void __fastcall Suspend();
  // whole operation
  System::TDateTime __fastcall TimeElapsed();
  // only current file
  System::TDateTime __fastcall TimeExpected();
  System::TDateTime __fastcall TotalTimeExpected();
  System::TDateTime __fastcall TotalTimeLeft();
  size_t __fastcall TransferProgress();
  size_t __fastcall OverallProgress();
  size_t __fastcall TotalTransferProgress();
private:
  TFileOperationProgressType(const TFileOperationProgressType & rhs);
  void operator=(const TFileOperationProgressType & rhs);
};
//---------------------------------------------------------------------------
class TSuspendFileOperationProgress
{
public:
  explicit TSuspendFileOperationProgress(TFileOperationProgressType * OperationProgress)
  {
    FOperationProgress = OperationProgress;
    if (FOperationProgress != NULL)
    {
      FOperationProgress->Suspend();
    }
  }

  ~TSuspendFileOperationProgress()
  {
    if (FOperationProgress != NULL)
    {
      FOperationProgress->Resume();
    }
  }

private:
  TFileOperationProgressType * FOperationProgress;
};
//---------------------------------------------------------------------------
#endif
