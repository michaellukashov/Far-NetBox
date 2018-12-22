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
typedef void (__closure *TFileOperationProgressEvent)
(TFileOperationProgressType &ProgressData);
#endif
typedef nb::FastDelegate1<void,
  TFileOperationProgressType & /*ProgressData*/> TFileOperationProgressEvent;
#if 0
typedef void (__closure *TFileOperationFinished)
(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString &FileName, bool Success, TOnceDoneOperation &OnceDoneOperation);
#endif
typedef nb::FastDelegate6<void,
  TFileOperation /*Operation*/, TOperationSide /*Side*/, bool /*Temp*/,
  UnicodeString /*FileName*/, bool /*Success*/,
  TOnceDoneOperation & /*OnceDoneOperation*/> TFileOperationFinishedEvent;
//---------------------------------------------------------------------------
class TFileOperationStatistics
{
public:
  TFileOperationStatistics();

  int FilesUploaded;
  int FilesDownloaded;
  int FilesDeletedLocal;
  int FilesDeletedRemote;
  __int64 TotalUploaded;
  __int64 TotalDownloaded;
};
//---------------------------------------------------------------------------
{
public:
  class TPersistence
  {
public:
  class TPersistence
  {
  friend class TFileOperationProgressType;
  public:
    TPersistence();
    __property TFileOperationStatistics * Statistics = { read = FStatistics, write = FStatistics };

    TDateTime StartTime;
    TBatchOverwrite BatchOverwrite;
    bool SkipToAll;
    unsigned long CPSLimit;
    bool CounterSet;
    std::vector<unsigned long> Ticks;
    std::vector<__int64> TotalTransferredThen;
    TOperationSide Side;
    __int64 TotalTransferred;
    TFileOperationStatistics * FStatistics;
  };

  private:
    void Clear(bool Batch, bool Speed);

    TDateTime StartTime;
    TBatchOverwrite BatchOverwrite;
    bool SkipToAll;
    unsigned long CPSLimit;
    bool CounterSet;
    std::vector<unsigned long> Ticks;
    std::vector<__int64> TotalTransferredThen;
    TOperationSide Side;
    __int64 TotalTransferred;
    TFileOperationStatistics * FStatistics;
  };

private:
  TFileOperation FOperation{foNone};
  TOperationSide FSide{osLocal};
  UnicodeString FFileName;
  UnicodeString FFullFileName;
  UnicodeString FDirectory;
  bool FAsciiTransfer{false};
  bool FTransferringFile{false};
  bool FTemp{false};
  int64_t FLocalSize{0};
  int64_t FLocallyUsed{0};
  int64_t FTransferSize{0};
  int64_t FTransferredSize{0};
  int64_t FSkippedSize{0};
  bool FInProgress{false};
  bool FDone{false};
  bool FFileInProgress{false};
  TCancelStatus FCancel;
  intptr_t FCount{0};
  int64_t FTotalTransferBase{0};
  int64_t FTotalSkipped{0};
  int64_t FTotalSize{0};
  bool FSkipToAll{false};
  intptr_t FCPSLimit{0};
  bool FTotalSizeSet{false};
  bool FSuspended{false};
  bool FRestored;
  TFileOperationProgressType *FParent{nullptr};
  bool FRestored;

  // when it was last time suspended (to calculate suspend time in Resume())
  uintptr_t FSuspendTime{0};
  // when current file was started being transferred
  TDateTime FFileStartTime;
  intptr_t FFilesFinished{0};
  intptr_t FFilesFinishedSuccessfully{0};
  TFileOperationProgressEvent FOnProgress{nullptr};
  TFileOperationFinishedEvent FOnFinished{nullptr};
  bool FReset{false};
  uintptr_t FLastSecond{0};
  int64_t FRemainingCPS{0};
  bool FCounterSet{false};
  TPersistence FPersistence;
  TCriticalSection *FSection{nullptr};
  TCriticalSection *FUserSelectionsSection{nullptr};

public:
  int64_t  GetOperationTransferred() const;
  int64_t GetTotalSize() const;
  intptr_t GetCPSLimit() const;
  TBatchOverwrite GetBatchOverwrite() const;
  bool GetSkipToAll() const;
  TDateTime GetStartTime() const { return FPersistence.StartTime; };
  TOperationSide GetSide() const { return FPersistence.Side; };

protected:
  void ClearTransfer();
  inline void DoProgress();
  int __fastcall OperationProgress() const;
  void AddTransferredToTotals(int64_t ASize);
  void AddSkipped(int64_t ASize);
  void AddTotalSize(int64_t ASize);
  void RollbackTransferFromTotals(int64_t ATransferredSize, int64_t ASkippedSize);
  uintptr_t GetCPS() const;
  void Init();
  static bool PassCancelToParent(TCancelStatus ACancel);
  void DoClear(bool Batch, bool Speed);

public:
  // common data
  __property TFileOperation Operation = { read = FOperation };
  ROProperty<TFileOperation> Operation{nb::bind(&TFileOperationProgressType::GetOperation, this)};
  // on what side if operation being processed (local/remote), source of copy
  __property TOperationSide Side = { read = GetSide };
  __property int Count =  { read = FCount };
  ROProperty<intptr_t> Count{nb::bind(&TFileOperationProgressType::GetCount, this)};
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
  ROProperty<int64_t> TransferredSize{nb::bind(&TFileOperationProgressType::GetTransferredSize, this)};
  __property int64_t SkippedSize = { read = FSkippedSize };
  __property bool InProgress = { read = FInProgress };
  __property bool Done = { read = FDone };
  __property bool FileInProgress = { read = FFileInProgress };
  __property TCancelStatus Cancel = { read = GetCancel };
  ROProperty<TCancelStatus> Cancel{nb::bind(&TFileOperationProgressType::GetCancel, this)};
  // when operation started
  __property TDateTime StartTime = { read = GetStartTime };
  // bytes transferred
  __property int64_t TotalTransferred = { read = GetTotalTransferred };
  __property __int64 OperationTransferred = { read = GetOperationTransferred };
  __property __int64 TotalSize = { read = GetTotalSize };
  __property int FilesFinishedSuccessfully = { read = FFilesFinishedSuccessfully };

  __property TBatchOverwrite BatchOverwrite = { read = GetBatchOverwrite };
  __property bool SkipToAll = { read = GetSkipToAll };
  __property uintptr_t CPSLimit = { read = GetCPSLimit };

  __property bool TotalSizeSet = { read = FTotalSizeSet };

  __property bool Suspended = { read = FSuspended };

  TFileOperationProgressType();
  explicit TFileOperationProgressType(
    TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished,
    TFileOperationProgressType *Parent = nullptr);
  virtual ~TFileOperationProgressType();
  void Assign(const TFileOperationProgressType &Other);
  void AssignButKeepSuspendState(const TFileOperationProgressType &Other);
  void AddLocallyUsed(int64_t ASize);
  void AddTransferred(int64_t ASize, bool AddToTotals = true);
  void AddResumed(int64_t ASize);
  void AddSkippedFileSize(int64_t ASize);
  void Clear();
  uintptr_t CPS() const;
  void Finish(const UnicodeString AFileName, bool Success,
    TOnceDoneOperation &OnceDoneOperation);
  void Succeeded(int Count = 1);
  int64_t LocalBlockSize();
  bool IsLocallyDone() const;
  bool IsTransferDone() const;
  void SetFile(const UnicodeString AFileName, bool AFileInProgress = true);
  void SetFileInProgress();
  uintptr_t TransferBlockSize();
  int64_t AdjustToCPSLimit(int64_t Size);
  void ThrottleToCPSLimit(int64_t Size);
  static uintptr_t StaticBlockSize();
  void Reset();
  void Resume();
  void SetLocalSize(int64_t ASize);
  void SetAsciiTransfer(bool AAsciiTransfer);
  void SetTransferSize(int64_t ASize);
  void ChangeTransferSize(int64_t ASize);
  void RollbackTransfer();
  void SetTotalSize(int64_t ASize);
  void Start(TFileOperation AOperation, TOperationSide ASide, intptr_t ACount);
  void Start(TFileOperation AOperation,
    TOperationSide ASide, intptr_t ACount, bool ATemp, const UnicodeString ADirectory,
    uintptr_t ACPSLimit);
  void Stop();
  void SetDone();
  void Suspend();
  void LockUserSelections();
  void UnlockUserSelections();
  // whole operation
  TDateTime TimeElapsed() const;
  // only current file
  TDateTime TimeExpected() const;
  TDateTime TotalTimeLeft() const;
  intptr_t TransferProgress() const;
  intptr_t OverallProgress() const;
  intptr_t TotalTransferProgress() const;
  void SetSpeedCounters();
  void SetTransferringFile(bool ATransferringFile);
  TCancelStatus GetCancel() const;
  void SetCancel(TCancelStatus ACancel);
  void SetCancelAtLeast(TCancelStatus ACancel);
  bool ClearCancelFile();
  void SetCPSLimit(intptr_t ACPSLimit);
  void SetBatchOverwrite(TBatchOverwrite ABatchOverwrite);
  void SetSkipToAll();
  UnicodeString GetLogStr(bool Done) const;
  void Store(TPersistence & Persistence);
  void Restore(TPersistence & Persistence);

  static bool IsIndeterminateOperation(TFileOperation Operation);

  TFileOperation GetOperation() const { return FOperation; }
  // on what side if operation being processed (local/remote), source of copy
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
  explicit TSuspendFileOperationProgress(TFileOperationProgressType *OperationProgress) :
    FOperationProgress(OperationProgress)
  {
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
  TFileOperationProgressType *FOperationProgress;
};
//---------------------------------------------------------------------------
