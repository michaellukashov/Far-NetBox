//---------------------------------------------------------------------------
#ifndef FileOperationProgressH
#define FileOperationProgressH
//---------------------------------------------------------------------------

#include <vector>

#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal6.hpp>

#include "Configuration.h"
#include "CopyParam.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
class TFileOperationProgressType;
enum TFileOperation { foNone, foCopy, foMove, foDelete, foSetProperties,
  foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
  foGetProperties, foCalculateChecksum };
enum TCancelStatus { csContinue = 0, csCancel, csCancelTransfer, csRemoteAbort };
enum TResumeStatus { rsNotAvailable, rsEnabled, rsDisabled };
enum TBatchOverwrite { boNo, boAll, boNone, boOlder, boAlternateResume, boAppend, boResume };
// typedef void (nb::TObject::*TFileOperationProgressEvent)
  // (TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
typedef boost::signal2<void, TFileOperationProgressType &, TCancelStatus &> fileoperationprogress_signal_type;
typedef fileoperationprogress_signal_type::slot_type fileoperationprogress_slot_type;

// typedef void (nb::TObject::*TFileOperationFinished)
  // (TFileOperation Operation, TOperationSide Side, bool Temp,
    // const std::wstring FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
typedef boost::signal6<void, TFileOperation, TOperationSide, bool,
    const std::wstring, bool, TOnceDoneOperation &> fileoperationfinished_signal_type;
typedef fileoperationfinished_signal_type::slot_type fileoperationfinished_slot_type;
//---------------------------------------------------------------------------
class TFileOperationProgressType
{
private:
  // when it was last time suspended (to calculate suspend time in Resume())
  unsigned int FSuspendTime;
  // when current file was started being transfered
  nb::TDateTime FFileStartTime;
  int FFilesFinished;
  fileoperationprogress_signal_type FOnProgress;
  fileoperationfinished_signal_type FOnFinished;
  bool FReset;
  unsigned int FLastSecond;
  unsigned long FRemainingCPS;
  std::vector<unsigned long> FTicks;
  std::vector<__int64> FTotalTransferredThen;

protected:
  void ClearTransfer();
  inline void DoProgress();

public:
  // common data
  TFileOperation Operation;
  // on what side if operation being processed (local/remote), source of copy
  TOperationSide Side;
  std::wstring FileName;
  std::wstring Directory;
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
  nb::TDateTime StartTime;
  // bytes transfered
  __int64 TotalTransfered;
  __int64 TotalSkipped;
  __int64 TotalSize;

  TBatchOverwrite BatchOverwrite;
  bool SkipToAll;
  unsigned long CPSLimit;

  bool TotalSizeSet;

  bool Suspended;

  explicit TFileOperationProgressType();
  explicit TFileOperationProgressType(
    const fileoperationprogress_slot_type &AOnProgress,
    const fileoperationfinished_slot_type &AOnFinished);
  ~TFileOperationProgressType();
  void AddLocallyUsed(__int64 ASize);
  void AddTransfered(__int64 ASize, bool AddToTotals = true);
  void AddResumed(__int64 ASize);
  void Clear();
  unsigned int CPS();
  void Finish(const std::wstring FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  unsigned long LocalBlockSize();
  bool IsLocallyDone();
  bool IsTransferDone();
  void SetFile(const std::wstring AFileName, bool AFileInProgress = true);
  void SetFileInProgress();
  size_t OperationProgress();
  size_t TransferBlockSize();
  size_t AdjustToCPSLimit(size_t Size);
  static size_t StaticBlockSize();
  void Reset();
  void Resume();
  void SetLocalSize(__int64 ASize);
  void SetAsciiTransfer(bool AAsciiTransfer);
  void SetResumeStatus(TResumeStatus AResumeStatus);
  void SetTransferSize(__int64 ASize);
  void ChangeTransferSize(__int64 ASize);
  void RollbackTransfer();
  void SetTotalSize(__int64 ASize);
  void Start(TFileOperation AOperation, TOperationSide ASide, size_t ACount);
  void Start(TFileOperation AOperation,
    TOperationSide ASide, size_t ACount, bool ATemp, const std::wstring ADirectory,
    unsigned long ACPSLimit);
  void Stop();
  void Suspend();
  // whole operation
  nb::TDateTime TimeElapsed();
  // only current file
  nb::TDateTime TimeExpected();
  nb::TDateTime TotalTimeExpected();
  nb::TDateTime TotalTimeLeft();
  int TransferProgress();
  int OverallProgress();
  int TotalTransferProgress();
private:
  TFileOperationProgressType(const TFileOperationProgressType &rhs);
  void operator=(const TFileOperationProgressType &rhs);
};
//---------------------------------------------------------------------------
class TSuspendFileOperationProgress
{
public:
  TSuspendFileOperationProgress(TFileOperationProgressType * OperationProgress)
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
