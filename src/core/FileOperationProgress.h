
#pragma once

#include "Configuration.h"
#include "CopyParam.h"
#include "Exceptions.h"

class TFileOperationProgressType;
enum TFileOperation
{
  foNone, foCopy, foMove, foDelete, foSetProperties,
  foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
  foGetProperties, foCalculateChecksum
};

// csCancelTransfer and csRemoteAbort are used with SCP only
enum TCancelStatus
{
  csContinue = 0, csCancel, csCancelTransfer, csRemoteAbort
};

enum TResumeStatus
{
  rsNotAvailable,
  rsEnabled,
  rsDisabled
};

enum TBatchOverwrite
{
  boNo,
  boAll,
  boNone,
  boOlder,
  boAlternateResume,
  boAppend,
  boResume
};

DEFINE_CALLBACK_TYPE1(TFileOperationProgressEvent, void,
  TFileOperationProgressType & /* ProgressData */);
DEFINE_CALLBACK_TYPE6(TFileOperationFinishedEvent, void,
  TFileOperation /* Operation */, TOperationSide /* Side */, bool /* Temp */,
  const UnicodeString & /* FileName */, bool /* Success */, TOnceDoneOperation & /* OnceDoneOperation */);

class TFileOperationProgressType : public Classes::TObject
{
public:
  // common data
  TFileOperation Operation;
  // on what side if operation being processed (local/remote), source of copy
  TOperationSide Side;
  UnicodeString FileName;
  UnicodeString FullFileName;
  UnicodeString Directory;
  bool AsciiTransfer;
  // Can be true with SCP protocol only
  bool TransferingFile;
  bool Temp;

  // file size to read/write
  int64_t LocalSize;
  int64_t LocallyUsed;
  int64_t TransferSize;
  int64_t TransferedSize;
  int64_t SkippedSize;
  TResumeStatus ResumeStatus;
  bool InProgress;
  bool FileInProgress;
  TCancelStatus Cancel;
  intptr_t Count;
  // when operation started
  Classes::TDateTime StartTime;
  // bytes transfered
  int64_t TotalTransfered;
  int64_t TotalSkipped;
  int64_t TotalSize;

  TBatchOverwrite BatchOverwrite;
  bool SkipToAll;
  uintptr_t CPSLimit;

  bool TotalSizeSet;
  bool Suspended;

  explicit TFileOperationProgressType();
  explicit TFileOperationProgressType(
    TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished);
  virtual ~TFileOperationProgressType();
  void AddLocallyUsed(int64_t ASize);
  void AddTransfered(int64_t ASize, bool AddToTotals = true);
  void AddResumed(int64_t ASize);
  void AddSkippedFileSize(int64_t ASize);
  void Clear();
  uintptr_t CPS() const;
  void Finish(const UnicodeString & AFileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  uintptr_t LocalBlockSize();
  bool IsLocallyDone() const;
  bool IsTransferDone() const;
  void SetFile(const UnicodeString & AFileName, bool AFileInProgress = true);
  void SetFileInProgress();
  intptr_t OperationProgress() const;
  uintptr_t TransferBlockSize();
  uintptr_t AdjustToCPSLimit(uintptr_t Size);
  void ThrottleToCPSLimit(uintptr_t Size);
  static uintptr_t StaticBlockSize();
  void Reset();
  void Resume();
  void SetLocalSize(int64_t ASize);
  void SetAsciiTransfer(bool AAsciiTransfer);
  void SetResumeStatus(TResumeStatus AResumeStatus);
  void SetTransferSize(int64_t ASize);
  void ChangeTransferSize(int64_t ASize);
  void RollbackTransfer();
  void SetTotalSize(int64_t ASize);
  void Start(TFileOperation AOperation, TOperationSide ASide, intptr_t ACount);
  void Start(TFileOperation AOperation,
    TOperationSide ASide, intptr_t ACount, bool ATemp, const UnicodeString & ADirectory,
    uintptr_t ACPSLimit);
  void Stop();
  void Suspend();
  // whole operation
  Classes::TDateTime TimeElapsed() const;
  // only current file
  Classes::TDateTime TimeExpected() const;
  Classes::TDateTime TotalTimeExpected() const;
  Classes::TDateTime TotalTimeLeft() const;
  intptr_t TransferProgress() const;
  intptr_t OverallProgress() const;
  intptr_t TotalTransferProgress() const;
  void SetSpeedCounters();

protected:
  void ClearTransfer();
  inline void DoProgress();

private:
  // when it was last time suspended (to calculate suspend time in Resume())
  uintptr_t FSuspendTime;
  // when current file was started being transfered
  Classes::TDateTime FFileStartTime;
  intptr_t FFilesFinished;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinishedEvent FOnFinished;
  bool FReset;
  uintptr_t FLastSecond;
  uintptr_t FRemainingCPS;
  bool FCounterSet;
  rde::vector<uint32_t> FTicks;
  rde::vector<int64_t> FTotalTransferredThen;
};

class TSuspendFileOperationProgress : public Classes::TObject
{
NB_DISABLE_COPY(TSuspendFileOperationProgress)
public:
  explicit TSuspendFileOperationProgress(TFileOperationProgressType * OperationProgress)
  {
    FOperationProgress = OperationProgress;
    if (FOperationProgress != nullptr)
    {
      FOperationProgress->Suspend();
    }
  }

  virtual ~TSuspendFileOperationProgress()
  {
    if (FOperationProgress != nullptr)
    {
      FOperationProgress->Resume();
    }
  }

private:
  TFileOperationProgressType * FOperationProgress;
};

