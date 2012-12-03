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

  explicit /* __fastcall */ TFileOperationProgressType();
  explicit /* __fastcall */ TFileOperationProgressType(
    TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished);
  virtual /* __fastcall */ ~TFileOperationProgressType();
  void __fastcall AddLocallyUsed(__int64 ASize);
  void __fastcall AddTransfered(__int64 ASize, bool AddToTotals = true);
  void __fastcall AddResumed(__int64 ASize);
  void __fastcall Clear();
  unsigned int __fastcall CPS();
  void __fastcall Finish(UnicodeString FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  unsigned long __fastcall LocalBlockSize();
  bool __fastcall IsLocallyDone() const;
  bool __fastcall IsTransferDone() const;
  void __fastcall SetFile(UnicodeString AFileName, bool AFileInProgress = true);
  void __fastcall SetFileInProgress();
  intptr_t __fastcall OperationProgress() const;
  unsigned long __fastcall TransferBlockSize();
  unsigned long __fastcall AdjustToCPSLimit(unsigned long Size);
  static unsigned long __fastcall StaticBlockSize();
  void __fastcall Reset();
  void __fastcall Resume();
  void __fastcall SetLocalSize(__int64 ASize);
  void __fastcall SetAsciiTransfer(bool AAsciiTransfer);
  void __fastcall SetResumeStatus(TResumeStatus AResumeStatus);
  void __fastcall SetTransferSize(__int64 ASize);
  void __fastcall ChangeTransferSize(__int64 ASize);
  void __fastcall RollbackTransfer();
  void __fastcall SetTotalSize(__int64 ASize);
  void __fastcall Start(TFileOperation AOperation, TOperationSide ASide, intptr_t ACount);
  void __fastcall Start(TFileOperation AOperation,
    TOperationSide ASide, intptr_t ACount, bool ATemp, const UnicodeString ADirectory,
    unsigned long ACPSLimit);
  void __fastcall Stop();
  void __fastcall Suspend();
  // whole operation
  TDateTime __fastcall TimeElapsed() const;
  // only current file
  TDateTime __fastcall TimeExpected();
  TDateTime __fastcall TotalTimeExpected();
  TDateTime __fastcall TotalTimeLeft();
  intptr_t __fastcall TransferProgress() const;
  intptr_t __fastcall OverallProgress() const;
  intptr_t __fastcall TotalTransferProgress() const;
};
//---------------------------------------------------------------------------
class TSuspendFileOperationProgress
{
public:
  explicit /* __fastcall */ TSuspendFileOperationProgress(TFileOperationProgressType * OperationProgress)
  {
    FOperationProgress = OperationProgress;
    if (FOperationProgress != NULL)
    {
      FOperationProgress->Suspend();
    }
  }

  virtual /* __fastcall */ ~TSuspendFileOperationProgress()
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
