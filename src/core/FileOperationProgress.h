//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <Common.h>
#include <Exceptions.h>

#include "Configuration.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
class TFileOperationProgressType;

enum TFileOperation
{
  foNone, foCopy, foMove, foDelete, foSetProperties,
  foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
  foGetProperties, foCalculateChecksum, foLock, foUnlock,
};

// csCancelTransfer and csRemoteAbort are used with SCP only
__removed enum TCancelStatus { csContinue = 0, csCancelFile, csCancel, csCancelTransfer, csRemoteAbort };
enum TCancelStatus
{
  csContinue = 0,
  csCancelFile,
  csCancel,
  csCancelTransfer,
  csRemoteAbort,
};

__removed enum TBatchOverwrite { boNo, boAll, boNone, boOlder, boAlternateResume, boAppend, boResume };
enum TBatchOverwrite
{
  boNo,
  boAll,
  boNone,
  boOlder,
  boAlternateResume,
  boAppend,
  boResume,
};

#if 0
typedef void __fastcall (__closure *TFileOperationProgressEvent)
  (TFileOperationProgressType & ProgressData);
#endif
typedef nb::FastDelegate1<void,
  TFileOperationProgressType & /*ProgressData*/> TFileOperationProgressEvent;
#if 0
typedef void __fastcall (__closure *TFileOperationFinished)
  (TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
#endif
typedef nb::FastDelegate6<void,
  TFileOperation /*Operation*/, TOperationSide /*Side*/, bool /*Temp*/,
  const UnicodeString & /*FileName*/, bool /*Success*/,
  TOnceDoneOperation & /*OnceDoneOperation*/> TFileOperationFinishedEvent;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TFileOperationProgressType : public TObject
{
private:
  TFileOperation FOperation;
  TOperationSide FSide;
  UnicodeString FFileName;
  UnicodeString FFullFileName;
  UnicodeString FDirectory;
  bool FAsciiTransfer;
  bool FTransferringFile;
  bool FTemp;
  int64_t FLocalSize;
  int64_t FLocallyUsed;
  int64_t FTransferSize;
  int64_t FTransferredSize;
  int64_t FSkippedSize;
  bool FInProgress;
  bool FDone;
  bool FFileInProgress;
  TCancelStatus FCancel;
  intptr_t FCount;
  TDateTime FStartTime;
  int64_t FTotalTransferred;
  int64_t FTotalSkipped;
  int64_t FTotalSize;
  TBatchOverwrite FBatchOverwrite;
  bool FSkipToAll;
  intptr_t FCPSLimit;
  bool FTotalSizeSet;
  bool FSuspended;
  TFileOperationProgressType *FParent;

  // when it was last time suspended (to calculate suspend time in Resume())
  uintptr_t FSuspendTime;
  // when current file was started being transferred
  TDateTime FFileStartTime;
  intptr_t FFilesFinished;
  intptr_t FFilesFinishedSuccessfully;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinishedEvent FOnFinished;
  bool FReset;
  uintptr_t FLastSecond;
  intptr_t FRemainingCPS;
  bool FCounterSet;
  rde::vector<intptr_t> FTicks;
  rde::vector<int64_t> FTotalTransferredThen;
  TCriticalSection *FSection;
  TCriticalSection *FUserSelectionsSection;

public:
  int64_t __fastcall GetTotalTransferred() const;
  int64_t __fastcall GetTotalSize() const;
  intptr_t __fastcall GetCPSLimit() const;
  TBatchOverwrite __fastcall GetBatchOverwrite() const;
  bool __fastcall GetSkipToAll() const;

protected:
  void __fastcall ClearTransfer();
  inline void __fastcall DoProgress();
  intptr_t __fastcall OperationProgress() const;
  void __fastcall AddTransferredToTotals(int64_t ASize);
  void __fastcall AddSkipped(int64_t ASize);
  void __fastcall AddTotalSize(int64_t ASize);
  void __fastcall RollbackTransferFromTotals(int64_t ATransferredSize, int64_t ASkippedSize);
  uintptr_t __fastcall GetCPS() const;
  void __fastcall Init();
  static bool __fastcall PassCancelToParent(TCancelStatus ACancel);

public:
  // common data
  __property TFileOperation Operation = { read = FOperation };
  ROProperty<TFileOperation, TFileOperationProgressType> Operation{this, &TFileOperationProgressType::GetOperation};
  // on what side if operation being processed (local/remote), source of copy
  __property TOperationSide Side = { read = FSide };
  __property int Count =  { read = FCount };
  ROProperty<intptr_t, TFileOperationProgressType> Count{this, &TFileOperationProgressType::GetCount};
  __property UnicodeString FileName =  { read = FFileName };
  __property UnicodeString FullFileName = { read = FFullFileName };
  __property UnicodeString Directory = { read = FDirectory };
  __property bool AsciiTransfer = { read = FAsciiTransfer };
  // Can be true with SCP protocol only
  __property bool TransferringFile = { read = FTransferringFile };
  __property bool Temp = { read = FTemp };

  // file size to read/write
  __property int64_t LocalSize = { read = FLocalSize };
  __property int64_t LocallyUsed = { read = FLocallyUsed };
  __property int64_t TransferSize = { read = FTransferSize };
  __property int64_t TransferredSize = { read = FTransferredSize };
  ROProperty<int64_t, TFileOperationProgressType> TransferredSize{this, &TFileOperationProgressType::GetTransferredSize};
  __property int64_t SkippedSize = { read = FSkippedSize };
  __property bool InProgress = { read = FInProgress };
  __property bool Done = { read = FDone };
  __property bool FileInProgress = { read = FFileInProgress };
  __property TCancelStatus Cancel = { read = GetCancel };
  ROProperty<TCancelStatus, TFileOperationProgressType> Cancel{this, &TFileOperationProgressType::GetCancel};
  // when operation started
  __property TDateTime StartTime = { read = FStartTime };
  // bytes transferred
  __property int64_t TotalTransferred = { read = GetTotalTransferred };
  __property int64_t TotalSize = { read = GetTotalSize };
  __property int FilesFinishedSuccessfully = { read = FFilesFinishedSuccessfully };

  __property TBatchOverwrite BatchOverwrite = { read = GetBatchOverwrite };
  __property bool SkipToAll = { read = GetSkipToAll };
  __property unsigned long CPSLimit = { read = GetCPSLimit };

  __property bool TotalSizeSet = { read = FTotalSizeSet };

  __property bool Suspended = { read = FSuspended };

  __fastcall TFileOperationProgressType();
  explicit __fastcall TFileOperationProgressType(
    TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished,
    TFileOperationProgressType *Parent = nullptr);
  virtual __fastcall ~TFileOperationProgressType();
  void __fastcall Assign(const TFileOperationProgressType &Other);
  void __fastcall AssignButKeepSuspendState(const TFileOperationProgressType &Other);
  void __fastcall AddLocallyUsed(int64_t ASize);
  void __fastcall AddTransferred(int64_t ASize, bool AddToTotals = true);
  void __fastcall AddResumed(int64_t ASize);
  void __fastcall AddSkippedFileSize(int64_t ASize);
  void __fastcall Clear();
  uintptr_t __fastcall CPS() const;
  void __fastcall Finish(UnicodeString AFileName, bool Success,
    TOnceDoneOperation &OnceDoneOperation);
  void __fastcall Progress();
  uintptr_t __fastcall LocalBlockSize();
  bool __fastcall IsLocallyDone() const;
  bool __fastcall IsTransferDone() const;
  void __fastcall SetFile(UnicodeString AFileName, bool AFileInProgress = true);
  void __fastcall SetFileInProgress();
  uintptr_t __fastcall TransferBlockSize();
  intptr_t __fastcall AdjustToCPSLimit(intptr_t Size);
  void __fastcall ThrottleToCPSLimit(intptr_t Size);
  static uintptr_t __fastcall StaticBlockSize();
  void __fastcall Reset();
  void __fastcall Resume();
  void __fastcall SetLocalSize(int64_t ASize);
  void __fastcall SetAsciiTransfer(bool AAsciiTransfer);
  void __fastcall SetTransferSize(int64_t ASize);
  void __fastcall ChangeTransferSize(int64_t ASize);
  void __fastcall RollbackTransfer();
  void __fastcall SetTotalSize(int64_t ASize);
  void __fastcall Start(TFileOperation AOperation, TOperationSide ASide, intptr_t ACount);
  void __fastcall Start(TFileOperation AOperation,
    TOperationSide ASide, intptr_t ACount, bool ATemp, const UnicodeString ADirectory,
    uintptr_t ACPSLimit);
  void __fastcall Stop();
  void __fastcall SetDone();
  void __fastcall Suspend();
  void __fastcall LockUserSelections();
  void __fastcall UnlockUserSelections();
  // whole operation
  TDateTime __fastcall TimeElapsed() const;
  // only current file
  TDateTime __fastcall TimeExpected() const;
  TDateTime __fastcall TotalTimeLeft() const;
  intptr_t __fastcall TransferProgress() const;
  intptr_t __fastcall OverallProgress() const;
  intptr_t __fastcall TotalTransferProgress() const;
  void __fastcall SetSpeedCounters();
  void __fastcall SetTransferringFile(bool ATransferringFile);
  TCancelStatus __fastcall GetCancel() const;
  void __fastcall SetCancel(TCancelStatus ACancel);
  void __fastcall SetCancelAtLeast(TCancelStatus ACancel);
  bool __fastcall ClearCancelFile();
  void __fastcall SetCPSLimit(intptr_t ACPSLimit);
  void __fastcall SetBatchOverwrite(TBatchOverwrite ABatchOverwrite);
  void __fastcall SetSkipToAll();
  UnicodeString __fastcall GetLogStr(bool Done) const;

  TFileOperation GetOperation() const { return FOperation; }
  // on what side if operation being processed (local/remote), source of copy
  TOperationSide GetSide() const { return FSide; }
  UnicodeString GetFileName() const { return FFileName; }
  UnicodeString GetFullFileName() const { return FFullFileName; }
  UnicodeString GetDirectory() const { return FDirectory; }
  bool GetAsciiTransfer() const { return FAsciiTransfer; }
  // Can be true with SCP protocol only
  bool GetTransferringFile() const { return FTransferringFile; }
  bool GetTemp() const { return FTemp; }

  intptr_t GetCount() const { return FCount; }
  int64_t GetLocalSize() const { return FLocalSize; }
  int64_t GetLocallyUsed() const { return FLocallyUsed; }
  int64_t GetTransferSize() const { return FTransferSize; }
  int64_t GetTransferredSize() const { return FTransferredSize; }
  int64_t GetSkippedSize() const { return FSkippedSize; }

  bool GetInProgress() const { return FInProgress; }
  bool GetDone() const { return FDone; }
  bool GetFileInProgress() const { return FFileInProgress; }

  TDateTime GetStartTime() const { return FStartTime; }
  int64_t GetTotalSkipped() const { return FTotalSkipped; }

  intptr_t GetFilesFinishedSuccessfully() const { return FFilesFinishedSuccessfully; }
  bool GetTotalSizeSet() const { return FTotalSizeSet; }
  bool GetSuspended() const { return FSuspended; }
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TSuspendFileOperationProgress : public TObject
{
  NB_DISABLE_COPY(TSuspendFileOperationProgress)
public:
  explicit __fastcall TSuspendFileOperationProgress(TFileOperationProgressType *OperationProgress) :
    FOperationProgress(OperationProgress)
  {
    if (FOperationProgress != nullptr)
    {
      FOperationProgress->Suspend();
    }
  }

  virtual __fastcall ~TSuspendFileOperationProgress()
  {
    if (FOperationProgress != nullptr)
    {
      FOperationProgress->Resume();
    }
  }

private:
  TFileOperationProgressType *FOperationProgress;
};
//---------------------------------------------------------------------------
