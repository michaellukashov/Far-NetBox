//---------------------------------------------------------------------------
#ifndef FileOperationProgressH
#define FileOperationProgressH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
#include "Exceptions.h"
#include <vector>
//---------------------------------------------------------------------------
class TFileOperationProgressType;
enum TFileOperation { foNone, foCopy, foMove, foDelete, foSetProperties,
  foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
  foGetProperties, foCalculateChecksum };
enum TCancelStatus { csContinue = 0, csCancel, csCancelTransfer, csRemoteAbort };
enum TResumeStatus { rsNotAvailable, rsEnabled, rsDisabled };
enum TBatchOverwrite { boNo, boAll, boNone, boOlder, boAlternateResume, boAppend, boResume };
DEFINE_CALLBACK_TYPE2(TFileOperationProgressEvent, void,
  TFileOperationProgressType & /* ProgressData */, TCancelStatus & /* Cancel */);
DEFINE_CALLBACK_TYPE6(TFileOperationFinishedEvent, void,
  TFileOperation /* Operation */, TOperationSide /* Side */, bool /* Temp */,
  const UnicodeString & /* FileName */, bool /* Success */, TOnceDoneOperation & /* OnceDoneOperation */);
//---------------------------------------------------------------------------
class TFileOperationProgressType
{
private:
  // when it was last time suspended (to calculate suspend time in Resume())
  unsigned int FSuspendTime;
  // when current file was started being transfered
  TDateTime FFileStartTime;
  intptr_t FFilesFinished;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinishedEvent FOnFinished;
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
  intptr_t Count;
  // when operation started
  TDateTime StartTime;
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
    TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished);
  virtual ~TFileOperationProgressType();
  void AddLocallyUsed(__int64 ASize);
  void AddTransfered(__int64 ASize, bool AddToTotals = true);
  void AddResumed(__int64 ASize);
  void Clear();
  unsigned int CPS();
  void Finish(UnicodeString FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  unsigned long LocalBlockSize();
  bool IsLocallyDone() const;
  bool IsTransferDone() const;
  void SetFile(UnicodeString AFileName, bool AFileInProgress = true);
  void SetFileInProgress();
  intptr_t OperationProgress() const;
  unsigned long TransferBlockSize();
  unsigned long AdjustToCPSLimit(unsigned long Size);
  static unsigned long StaticBlockSize();
  void Reset();
  void Resume();
  void SetLocalSize(__int64 ASize);
  void SetAsciiTransfer(bool AAsciiTransfer);
  void SetResumeStatus(TResumeStatus AResumeStatus);
  void SetTransferSize(__int64 ASize);
  void ChangeTransferSize(__int64 ASize);
  void RollbackTransfer();
  void SetTotalSize(__int64 ASize);
  void Start(TFileOperation AOperation, TOperationSide ASide, intptr_t ACount);
  void Start(TFileOperation AOperation,
    TOperationSide ASide, intptr_t ACount, bool ATemp, const UnicodeString & ADirectory,
    unsigned long ACPSLimit);
  void Stop();
  void Suspend();
  // whole operation
  TDateTime TimeElapsed() const;
  // only current file
  TDateTime TimeExpected();
  TDateTime TotalTimeExpected();
  TDateTime TotalTimeLeft();
  intptr_t TransferProgress() const;
  intptr_t OverallProgress() const;
  intptr_t TotalTransferProgress() const;
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

  virtual ~TSuspendFileOperationProgress()
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
