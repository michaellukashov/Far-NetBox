
#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FileOperationProgress.h"
#include "CoreMain.h"
#include "Interface.h"

constexpr const uint64_t TRANSFER_BUF_SIZE = 32 * 1024;

TFileOperationStatistics::TFileOperationStatistics() noexcept
{
  __removed memset(this, 0, sizeof(*this));
}


TFileOperationProgressType::TPersistence::TPersistence() noexcept
{
  FStatistics = nullptr;
  Clear(true, true);
}

bool TFileOperationProgressType::IsIndeterminateOperation(TFileOperation Operation)
{
  return (Operation == foCalculateSize);
}

bool TFileOperationProgressType::IsTransferOperation(TFileOperation Operation)
{
  return (Operation == foCopy) || (Operation == foMove);
}

void TFileOperationProgressType::TPersistence::Clear(bool Batch, bool Speed)
{
  if (Batch)
  {
    TotalTransferred = 0;
    StartTime = Now();
    SkipToAll = false;
    BatchOverwrite = boNo;
    CPSLimit = 0;
    CounterSet = false;
  }
  if (Speed)
  {
    Ticks.clear();
    TotalTransferredThen.clear();
  }
}

TFileOperationProgressType::TFileOperationProgressType() noexcept
{
  FOnProgress = nullptr;
  FOnFinished = nullptr;
  FParent = nullptr;
  Init();
  Clear();
}

TFileOperationProgressType::TFileOperationProgressType(
  TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished,
  TFileOperationProgressType * Parent) noexcept :
  FParent(Parent),
  FOnProgress(AOnProgress),
  FOnFinished(AOnFinished)
{
  FOnProgress = AOnProgress;
  FOnFinished = AOnFinished;
  FParent = Parent;
  FReset = false;
  Init();
  Clear();
}

TFileOperationProgressType::~TFileOperationProgressType() noexcept
{
  DebugAssert(!GetInProgress() || FReset);
  DebugAssert(!GetSuspended() || FReset);
  SAFE_DESTROY_EX(TCriticalSection, FSection);
  SAFE_DESTROY_EX(TCriticalSection, FUserSelectionsSection);
}

void TFileOperationProgressType::Init()
{
  FSection = new TCriticalSection();
  FUserSelectionsSection = new TCriticalSection();
  FRestored = false;
  FPersistence.Side = osCurrent; // = undefined value
}

void TFileOperationProgressType::Assign(const TFileOperationProgressType & Other)
{
  TValueRestorer<TCriticalSection *> SectionRestorer(FSection); nb::used(SectionRestorer);
  TValueRestorer<TCriticalSection *> UserSelectionsSectionRestorer(FUserSelectionsSection); nb::used(UserSelectionsSectionRestorer);
  TGuard Guard(*FSection); nb::used(Guard);
  TGuard OtherGuard(*Other.FSection); nb::used(OtherGuard);

  *this = Other;
}

void TFileOperationProgressType::AssignButKeepSuspendState(const TFileOperationProgressType & Other)
{
  TGuard Guard(*FSection); nb::used(Guard);
  TValueRestorer<uint64_t> SuspendTimeRestorer(FSuspendTime); nb::used(SuspendTimeRestorer);
  TValueRestorer<bool> SuspendedRestorer(FSuspended); nb::used(SuspendedRestorer);

  Assign(Other);
}

void TFileOperationProgressType::DoClear(bool Batch, bool Speed)
{
  FFileName = "";
  FFullFileName = "";
  FDirectory = "";
  FAsciiTransfer = false;
  FCount = -1;
  FFilesFinished = 0;
  FFilesFinishedSuccessfully = 0;
  FSuspended = false;
  FSuspendTime = 0;
  FInProgress = false;
  FDone = false;
  FFileInProgress = false;
  FTotalSkipped = 0;
  FTotalSize = 0;
  FSkippedSize = 0;
  FTotalSizeSet = false;
  FFileStartTime = 0.0;
  FFilesFinished = 0;
  FReset = false;
  FLastSecond = 0;
  FRemainingCPS = 0;
  FCounterSet = false;
  FOperation = foNone;
  FFileName.Clear();
  FDirectory.Clear();
  FAsciiTransfer = false;
  FLocalSize = 0;
  FLocallyUsed = 0;
  FOperation = foNone;
  FTemp = false;
  FPersistence.Clear(Batch, Speed);
  // to bypass check in ClearTransfer()
  FTransferSize = 0;
  ClearTransfer();
  FTransferredSize = 0;
  FCancel = csContinue;
  FCount = 0;
  FPersistence.StartTime = Now();
  FCPSLimit = 0;
  FSuspended = false;

  ClearTransfer();
}

void TFileOperationProgressType::Clear()
{
  DoClear(true, true);
}

void TFileOperationProgressType::ClearTransfer()
{
  if ((FTransferSize > 0) && (FTransferredSize < FTransferSize))
  {
    TGuard Guard(*FSection); nb::used(Guard);
    int64_t RemainingSize = (FTransferSize - FTransferredSize);
    AddSkipped(RemainingSize);
  }
  FLocalSize = 0;
  FTransferSize = 0;
  FLocallyUsed = 0;
  FSkippedSize = 0;
  FTransferredSize = 0;
  FTransferringFile = false;
  FLastSecond = 0;
}

void TFileOperationProgressType::Start(TFileOperation AOperation,
  TOperationSide ASide, int32_t ACount)
{
  Start(AOperation, ASide, ACount, false, L"", 0, odoIdle);
}

void TFileOperationProgressType::Start(TFileOperation AOperation,
  TOperationSide ASide, int32_t ACount, bool ATemp,
  const UnicodeString & ADirectory, uint64_t ACPSLimit, TOnceDoneOperation InitialOnceDoneOperation)
{

  {
    TGuard Guard(*FSection); nb::used(Guard); // not really needed, just for consistency
    DoClear(!FRestored, (FPersistence.Side != osCurrent) && (FPersistence.Side != ASide));
    FTotalTransferBase = FPersistence.TotalTransferred;
    FOperation = AOperation;
    FPersistence.Side = ASide;
    FCount = ACount;
    FInProgress = true;
    FCancel = csContinue;
    FDirectory = ADirectory;
    FTemp = ATemp;
    FInitialOnceDoneOperation = InitialOnceDoneOperation;
    FPersistence.CPSLimit = ACPSLimit;
  }

  try
  {
    DoProgress();
  }
  catch (...)
  {
    // connection can be lost during progress callbacks
    ClearTransfer();
    FInProgress = false;
    throw;
  }
}

void TFileOperationProgressType::Reset()
{
  FReset = true;
}

void TFileOperationProgressType::Stop()
{
  // added to include remaining bytes to TotalSkipped, in case
  // the progress happens to update before closing
  ClearTransfer();
  FInProgress = false;
  DoProgress();
}

void TFileOperationProgressType::SetDone()
{
  FDone = true;
  DoProgress();
}

void TFileOperationProgressType::Suspend()
{

  {
    TGuard Guard(*FSection); nb::used(Guard);
    DebugAssert(!FSuspended);
    FSuspended = true;
    FSuspendTime = ::GetTickCount();
  }

  DoProgress();
}

void TFileOperationProgressType::Resume()
{

  {
    TGuard Guard(*FSection); nb::used(Guard);
    DebugAssert(FSuspended);
    FSuspended = false;

    // shift timestamps for CPS calculation in advance
    // by the time the progress was suspended
    int32_t Stopped = nb::ToIntPtr(::GetTickCount() - FSuspendTime);
    size_t Index = 0;
    while (Index < FPersistence.Ticks.size())
    {
      FPersistence.Ticks[Index] += Stopped;
      ++Index;
    }
  }

  DoProgress();
}

int32_t TFileOperationProgressType::OperationProgress() const
{
  int32_t Result;
  if (FCount > 0)
  {
    Result = (FFilesFinished * 100) / FCount;
  }
  else
  {
    Result = 0;
  }
  return Result;
}

int32_t TFileOperationProgressType::TransferProgress() const
{
  int32_t Result;
  if (FTransferSize)
  {
    Result = nb::ToInt((FTransferredSize * 100) / FTransferSize);
  }
  else
  {
    Result = 0;
  }
  return Result;
}

int32_t TFileOperationProgressType::TotalTransferProgress() const
{
  TGuard Guard(*FSection); nb::used(Guard);
  DebugAssert(FTotalSizeSet);
  int32_t Result;
  if (FTotalSize > 0)
  {
    Result = nb::ToInt(((GetTotalTransferred() - FTotalTransferBase + FTotalSkipped) * 100) / FTotalSize);
  }
  else
  {
    Result = 0;
  }
  return Result < 100 ? Result : 100;
}

int32_t TFileOperationProgressType::OverallProgress() const
{
  if (FTotalSizeSet)
  {
    DebugAssert(IsTransfer());
    return TotalTransferProgress();
  }
  return OperationProgress();
}

void TFileOperationProgressType::Progress()
{
  DoProgress();
}

void TFileOperationProgressType::DoProgress()
{
  SystemRequired();
  FOnProgress(*this);
}

void TFileOperationProgressType::Finish(const UnicodeString & AFileName,
  bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert(FInProgress);

  // Cancel reader is guarded
  FOnFinished(FOperation, Side(), FTemp, AFileName,
    Success && (FCancel == csContinue), OnceDoneOperation);
  FFilesFinished++;
  if (Success)
  {
    FFilesFinishedSuccessfully++;
  }
  DoProgress();
}

void TFileOperationProgressType::Succeeded(int32_t Count)
{
  if (FPersistence.Statistics != nullptr)
  {
    if (IsTransfer())
    {
      int64_t Transferred = FTransferredSize - FSkippedSize;
      if (Side() == osLocal)
      {
        FPersistence.Statistics->FilesUploaded += Count;
        FPersistence.Statistics->TotalUploaded += Transferred;
      }
      else
      {
        FPersistence.Statistics->FilesDownloaded += Count;
        FPersistence.Statistics->TotalDownloaded += Transferred;
      }
    }
    else if (Operation() == foDelete)
    {
      if (Side() == osLocal)
      {
        FPersistence.Statistics->FilesDeletedLocal += Count;
      }
      else
      {
        FPersistence.Statistics->FilesDeletedRemote += Count;
      }
    }
  }
}

void TFileOperationProgressType::SetFile(const UnicodeString & AFileName, bool AFileInProgress)
{
  UnicodeString FileName = AFileName;
  FFullFileName = AFileName;
  if (Side() == osRemote)
  {
    // historically set were passing filename-only for remote site operations,
    // now we need to collect a full paths, so we pass in full path,
    // but still want to have filename-only in FileName
    FileName = base::UnixExtractFileName(FileName);
  }
  FFileName = FileName;
  FFileInProgress = AFileInProgress;
  ClearTransfer();
  FFileStartTime = Now();
  DoProgress();
}

void TFileOperationProgressType::SetFileInProgress()
{
  DebugAssert(!FFileInProgress);
  FFileInProgress = true;
  DoProgress();
}

void TFileOperationProgressType::SetLocalSize(int64_t ASize)
{
  FLocalSize = ASize;
  DoProgress();
}

void TFileOperationProgressType::AddLocallyUsed(int64_t ASize)
{
  FLocallyUsed += ASize;
  if (FLocallyUsed > FLocalSize)
  {
    FLocalSize = FLocallyUsed;
  }
  DoProgress();
}

bool TFileOperationProgressType::IsLocallyDone() const
{
  DebugAssert(FLocallyUsed <= FLocalSize);
  return (FLocallyUsed == FLocalSize);
}

void TFileOperationProgressType::SetSpeedCounters()
{
  if ((FCPSLimit > 0) && !FPersistence.CounterSet)
  {
    FPersistence.CounterSet = true;
    __removed Configuration->Usage->Inc(L"SpeedLimitUses");
  }
}

// Used in WebDAV and S3 protocols
void TFileOperationProgressType::ThrottleToCPSLimit(
  uint64_t Size)
{
  uint64_t  Remaining = Size;
  while (Remaining > 0)
  {
    Remaining -= AdjustToCPSLimit(Remaining);
  }
}

uint64_t TFileOperationProgressType::AdjustToCPSLimit(
  uint64_t Size)
{
  SetSpeedCounters();

  // CPSLimit reader is guarded, we cannot block whole method as it can last long.
  if (FCPSLimit > 0)
  {
    // we must not return 0, hence, if we reach zero,
    // we wait until the next second
    do
    {
      uint32_t Second = (::GetTickCount() / MSecsPerSec);

      if (Second != FLastSecond)
      {
        FRemainingCPS = FCPSLimit;
        FLastSecond = Second;
      }

      if (FRemainingCPS == 0)
      {
        SleepEx(100, true);
        DoProgress();
      }
    }
    while ((FCPSLimit > 0) && (FRemainingCPS == 0));

    // CPSLimit may have been dropped in DoProgress
    if (FCPSLimit > 0)
    {
      if (FRemainingCPS < Size)
      {
        Size = FRemainingCPS;
      }

      FRemainingCPS -= Size;
    }
  }
  return Size;
}

// Use in SCP protocol only
uint64_t TFileOperationProgressType::LocalBlockSize()
{
  uint64_t Result = TRANSFER_BUF_SIZE;
  if (FLocallyUsed + Result > FLocalSize)
  {
    Result = (uint64_t)(FLocalSize - FLocallyUsed);
  }
  Result = AdjustToCPSLimit(Result);
  return Result;
}

void TFileOperationProgressType::SetTotalSize(int64_t ASize)
{
  TGuard Guard(*FSection); nb::used(Guard); // not really needed, just for consistency

  FTotalSize = ASize;
  FTotalSizeSet = true;
  // parent has its own totals
  if (FParent != nullptr)
  {
    DebugAssert(FParent->GetTotalSizeSet());
  }
  DoProgress();
}

void TFileOperationProgressType::SetTransferSize(int64_t ASize)
{
  FTransferSize = ASize;
  DoProgress();
}

void TFileOperationProgressType::SetTransferringFile(bool ATransferringFile)
{
  FTransferringFile = ATransferringFile;
}

bool TFileOperationProgressType::PassCancelToParent(TCancelStatus ACancel)
{
  bool Result;
  if (ACancel < csCancel)
  {
    // do not propagate csCancelFile,
    // though it's not supported for queue atm, so we do not expect it here
    DebugFail();
    Result = false;
  }
  else if (ACancel == csCancel)
  {
    Result = true;
  }
  else
  {
    // csCancelTransfer and csRemoteAbort are used with SCP only, which does not use parallel transfers
    DebugFail();
    Result = false;
  }
  return Result;
}

void TFileOperationProgressType::SetCancel(TCancelStatus ACancel)
{
  TGuard Guard(*FSection); nb::used(Guard);
  FCancel = ACancel;

  if ((FParent != nullptr) && PassCancelToParent(ACancel))
  {
    FParent->SetCancel(ACancel);
  }
}

void TFileOperationProgressType::SetCancelAtLeast(TCancelStatus ACancel)
{
  TGuard Guard(*FSection); nb::used(Guard);
  if (FCancel < ACancel)
  {
    FCancel = ACancel;
  }

  if ((FParent != nullptr) && PassCancelToParent(ACancel))
  {
    FParent->SetCancelAtLeast(ACancel);
  }
}

TCancelStatus TFileOperationProgressType::GetCancel() const
{
  TCancelStatus Result = FCancel;
  if (FParent != nullptr)
  {
    TGuard Guard(*FSection); nb::used(Guard);
    TCancelStatus ParentCancel = FParent->GetCancel();
    if (ParentCancel > Result)
    {
      Result = ParentCancel;
    }
  }
  return Result;
}

bool TFileOperationProgressType::ClearCancelFile()
{
  TGuard Guard(*FSection); nb::used(Guard);
  // Not propagated to parent, as this is local flag, see also PassCancelToParent
  bool Result = (GetCancel() == csCancelFile);
  if (Result)
  {
    FCancel = csContinue;
  }
  return Result;
}

uint64_t TFileOperationProgressType::GetCPSLimit() const
{
  uint64_t Result;
  if (FParent != nullptr)
  {
    Result = FParent->GetCPSLimit();
  }
  else
  {
    TGuard Guard(*FSection); nb::used(Guard);
    Result = FPersistence.CPSLimit;
  }
  return Result;
}

void TFileOperationProgressType::SetCPSLimit(uint64_t ACPSLimit)
{
  if (FParent != nullptr)
  {
    FParent->SetCPSLimit(ACPSLimit);
  }
  else
  {
    TGuard Guard(*FSection); nb::used(Guard);
    FPersistence.CPSLimit = ACPSLimit;
  }
}

TBatchOverwrite TFileOperationProgressType::GetBatchOverwrite() const
{
  TBatchOverwrite Result;
  if (FParent != nullptr)
  {
    Result = FParent->GetBatchOverwrite();
  }
  else
  {
    TGuard Guard(*FSection); nb::used(Guard); // not really needed
    Result = FPersistence.BatchOverwrite;
  }
  return Result;
}

void TFileOperationProgressType::SetBatchOverwrite(TBatchOverwrite ABatchOverwrite)
{
  if (FParent != nullptr)
  {
    FParent->SetBatchOverwrite(ABatchOverwrite);
  }
  else
  {
    TGuard Guard(*FSection); nb::used(Guard); // not really needed
    FPersistence.BatchOverwrite = ABatchOverwrite;;
  }
}

bool TFileOperationProgressType::GetSkipToAll() const
{
  bool Result;
  if (FParent != nullptr)
  {
    Result = FParent->GetSkipToAll();
  }
  else
  {
    TGuard Guard(*FSection); nb::used(Guard); // not really needed
    Result = FPersistence.SkipToAll;
  }
  return Result;
}

void TFileOperationProgressType::SetSkipToAll()
{
  if (FParent != nullptr)
  {
    FParent->SetSkipToAll();
  }
  else
  {
    TGuard Guard(*FSection); nb::used(Guard); // not really needed
    FPersistence.SkipToAll = true;
  }
}

void TFileOperationProgressType::ChangeTransferSize(int64_t ASize)
{
  // reflect change on file size (due to text transfer mode conversion particularly)
  // on total transfer size
  if (GetTotalSizeSet())
  {
    AddTotalSize(ASize - FTransferSize);
  }
  FTransferSize = ASize;
  DoProgress();
}

void TFileOperationProgressType::RollbackTransferFromTotals(int64_t ATransferredSize, int64_t ASkippedSize)
{
  TGuard Guard(*FSection); nb::used(Guard);

  DebugAssert(ATransferredSize <= FPersistence.TotalTransferred - FTotalTransferBase);
  DebugAssert(ASkippedSize <= FTotalSkipped);
  FPersistence.TotalTransferred -= ATransferredSize;
  FPersistence.Ticks.clear();
  FPersistence.TotalTransferredThen.clear();
  FTotalSkipped -= ASkippedSize;

  if (FParent != nullptr)
  {
    FParent->RollbackTransferFromTotals(ATransferredSize, ASkippedSize);
  }
}

void TFileOperationProgressType::RollbackTransfer()
{
  FTransferredSize -= FSkippedSize;
  RollbackTransferFromTotals(FTransferredSize, FSkippedSize);
  FSkippedSize = 0;
  FTransferredSize = 0;
  FTransferSize = 0;
  FLocallyUsed = 0;
}

void TFileOperationProgressType::AddTransferredToTotals(int64_t ASize)
{
  TGuard Guard(*FSection); nb::used(Guard);

  FPersistence.TotalTransferred += ASize;
  if (ASize >= 0)
  {
    uint64_t Ticks = nb::ToUInt64(::GetTickCount());
    if (FPersistence.Ticks.empty() ||
        (FPersistence.Ticks.back() > Ticks) || // ticks wrap after 49.7 days
        ((Ticks - FPersistence.Ticks.back()) >= nb::ToUInt64(MSecsPerSec)))
    {
      FPersistence.Ticks.push_back(Ticks);
      FPersistence.TotalTransferredThen.push_back(FPersistence.TotalTransferred);
    }

    if (FPersistence.Ticks.size() > 10)
    {
      FPersistence.Ticks.erase(FPersistence.Ticks.begin());
      FPersistence.TotalTransferredThen.erase(FPersistence.TotalTransferredThen.begin());
    }
  }
  else
  {
    FPersistence.Ticks.clear();
  }

  if (FParent != nullptr)
  {
    FParent->AddTransferredToTotals(ASize);
  }
}

void TFileOperationProgressType::AddTotalSize(int64_t ASize)
{
  if (ASize != 0)
  {
    TGuard Guard(*FSection); nb::used(Guard);
    FTotalSize += ASize;

    if (FParent != nullptr)
    {
      FParent->AddTotalSize(ASize);
    }
  }
}

void TFileOperationProgressType::AddTransferred(int64_t ASize,
  bool AddToTotals)
{
  FTransferredSize += ASize;
  if (FTransferredSize > FTransferSize)
  {
    // this can happen with SFTP when downloading file that
    // grows while being downloaded
    if (FTotalSizeSet)
    {
      // we should probably guard this with AddToTotals
      AddTotalSize(FTransferredSize - FTransferSize);
    }
    FTransferSize = FTransferredSize;
  }
  if (AddToTotals)
  {
    AddTransferredToTotals(ASize);
  }
  DoProgress();
}

void TFileOperationProgressType::AddSkipped(int64_t ASize)
{
  TGuard Guard(*FSection); nb::used(Guard);

  FTotalSkipped += ASize;

  if (FParent != nullptr)
  {
    FParent->AddSkipped(ASize);
  }
}

void TFileOperationProgressType::AddResumed(int64_t ASize)
{
  AddSkipped(ASize);
  FSkippedSize += ASize;
  AddTransferred(ASize, false);
  AddLocallyUsed(ASize);
}

void TFileOperationProgressType::AddSkippedFileSize(int64_t ASize)
{
  AddSkipped(ASize);
  DoProgress();
}

// Use in SCP protocol only
uint64_t TFileOperationProgressType::TransferBlockSize()
{
  uint64_t Result = TRANSFER_BUF_SIZE;
  if (FTransferredSize + Result > FTransferSize)
  {
    Result = (uint64_t)(FTransferSize - FTransferredSize);
  }
  Result = AdjustToCPSLimit(Result);
  return Result;
}

uint64_t TFileOperationProgressType::StaticBlockSize()
{
  return TRANSFER_BUF_SIZE;
}

bool TFileOperationProgressType::IsTransferDoneChecked() const
{
  DebugAssert(TransferredSize <= TransferSize);
  return IsTransferDone();
}

// Note that this does not work correctly, if the file is larger than expected (at least with the FTP protocol)
bool TFileOperationProgressType::IsTransferDone() const
{
  return (TransferredSize == TransferSize);
}

void TFileOperationProgressType::SetAsciiTransfer(bool AAsciiTransfer)
{
  FAsciiTransfer = AAsciiTransfer;
  DoProgress();
}

TDateTime TFileOperationProgressType::TimeElapsed() const
{
  return Now() - GetStartTime();
}

uint64_t TFileOperationProgressType::CPS() const
{
  TGuard Guard(*FSection); nb::used(Guard);
  return nb::ToUInt64(GetCPS());
}

inline static uint32_t CalculateCPS(int64_t Transferred, uint32_t MSecElapsed)
{
  uint32_t Result;
  if (MSecElapsed == 0)
  {
    Result = 0;
  }
  else
  {
    Result = (uint32_t)(Transferred * MSecsPerSec / MSecElapsed);
  }
  return Result;
}

// Has to be called from a guarded method
uint64_t TFileOperationProgressType::GetCPS() const
{
  uint64_t Result;
  if (FPersistence.Ticks.empty())
  {
    Result = 0;
  }
  else
  {
    uint64_t Ticks = (GetSuspended() ? FSuspendTime : ::GetTickCount());
    uint32_t TimeSpan;
    if (Ticks < FPersistence.Ticks.front())
    {
      // clocks has wrapped, guess 10 seconds difference
      TimeSpan = 10000;
    }
    else
    {
      TimeSpan = (Ticks - FPersistence.Ticks.front());
    }

    int64_t Transferred = (FPersistence.TotalTransferred - FPersistence.TotalTransferredThen.front());
    Result = CalculateCPS(Transferred, TimeSpan);
  }
  return nb::ToUIntPtr(Result);
}

TDateTime TFileOperationProgressType::TimeExpected() const
{
  uint32_t CurCps = CPS();
  if (CurCps)
  {
    return TDateTime(nb::ToDouble((nb::ToDouble(FTransferSize - FTransferredSize)) / CurCps) / SecsPerDay);
  }
  return TDateTime(0.0);
}

TDateTime TFileOperationProgressType::TotalTimeLeft() const
{
  TGuard Guard(*FSection); nb::used(Guard);
  DebugAssert(FTotalSizeSet);
  uint32_t CurCps = GetCPS();
  // sanity check
  int64_t Processed = FTotalSkipped + FPersistence.TotalTransferred - FTotalTransferBase;
  if ((CurCps > 0) && (FTotalSize > Processed))
  {
    return TDateTime(nb::ToDouble(nb::ToDouble(FTotalSize - Processed) / CurCps) / SecsPerDay);
  }
  return TDateTime(0.0);
}

int64_t TFileOperationProgressType::GetTotalTransferred() const
{
  TGuard Guard(*FSection); nb::used(Guard);
  return FPersistence.TotalTransferred;
}

int64_t TFileOperationProgressType::GetOperationTransferred() const
{
  TGuard Guard(*FSection);
  return FPersistence.TotalTransferred - FTotalTransferBase + FTotalSkipped;
}

int64_t TFileOperationProgressType::GetTotalSize() const
{
  TGuard Guard(*FSection); nb::used(Guard);
  return FTotalSize;
}

void TFileOperationProgressType::LockUserSelections()
{
  if (FParent != nullptr)
  {
    FParent->LockUserSelections();
  }
  else
  {
    FUserSelectionsSection->Enter();
  }
}

void TFileOperationProgressType::UnlockUserSelections()
{
  if (FParent != nullptr)
  {
    FParent->UnlockUserSelections();
  }
  else
  {
    FUserSelectionsSection->Leave();
  }
}

UnicodeString TFileOperationProgressType::GetLogStr(bool Done) const
{
  UnicodeString Transferred = FormatSize(GetTotalTransferred());
  TDateTime Time;
  UnicodeString TimeLabel;
  if (!Done && FTotalSizeSet)
  {
    Time = TotalTimeLeft();
    TimeLabel = "Left";
  }
  else
  {
    Time = TimeElapsed();
    TimeLabel = "Elapsed";
  }
  UnicodeString TimeStr = FormatDateTimeSpan(Time);

  uint32_t ACPS;
  if (!Done)
  {
    ACPS = CPS();
  }
  else
  {
    uint32_t Elapsed = TimeToMSec(TimeElapsed());
    ACPS = CalculateCPS(TotalTransferred, Elapsed);
  }
  UnicodeString CPSStr = FormatSize(ACPS);

  return FORMAT("Transferred: %s, %s: %s, CPS: %s/s", Transferred, TimeLabel, TimeStr, CPSStr);
}

void TFileOperationProgressType::Store(TPersistence & Persistence)
{
  // in this current use, we do not need this to be guarded actually, as it's used only for a single-threaded synchronization
  TGuard Guard(*FSection);
  Persistence = FPersistence;
}

void TFileOperationProgressType::Restore(TPersistence & Persistence)
{
  TGuard Guard(*FSection);
  FPersistence = Persistence;
  FRestored = true;
}

bool TFileOperationProgressType::IsIndeterminate() const
{
  return
    IsIndeterminateOperation(FOperation) ||
    (!TotalSizeSet && (FCount == 1));
}

bool TFileOperationProgressType::IsTransfer() const
{
  return IsTransferOperation(Operation);
}

TFileOperationProgressType & TFileOperationProgressType::operator=(const TFileOperationProgressType & rhs)
{
  FFileName = rhs.FFileName;
  FFullFileName = rhs.FFullFileName;
  FDirectory = rhs.FDirectory;
  FAsciiTransfer = rhs.FAsciiTransfer;
  FCount = rhs.FCount;
  FFilesFinished = rhs.FFilesFinished;
  FFilesFinishedSuccessfully = rhs.FFilesFinishedSuccessfully;
  FSuspended = rhs.FSuspended;
  FSuspendTime = rhs.FSuspendTime;
  FInProgress = rhs.FInProgress;
  FDone = rhs.FDone;
  FFileInProgress = rhs.FFileInProgress;
  FTotalSkipped = rhs.FTotalSkipped;
  FTotalSize = rhs.FTotalSize;
  FSkippedSize = rhs.FSkippedSize;
  FTotalSizeSet = rhs.FTotalSizeSet;
  FFileStartTime = rhs.FFileStartTime;
  FFilesFinished = rhs.FFilesFinished;
  FReset = rhs.FReset;
  FLastSecond = rhs.FLastSecond;
  FRemainingCPS = rhs.FRemainingCPS;
  FCounterSet = rhs.FCounterSet;
  FOperation = rhs.FOperation;
  FPersistence.Side = rhs.Side();
  FLocalSize = rhs.FLocalSize;
  FLocallyUsed = rhs.FLocallyUsed;
  FOperation = rhs.FOperation;
  FTemp = rhs.FTemp;
  FPersistence = rhs.FPersistence;
  FTransferSize = rhs.FTransferSize;

  FTransferredSize = rhs.FTransferredSize;
  FCancel = rhs.FCancel;
  FCount = rhs.FCount;

  FCPSLimit = rhs.FCPSLimit;
  FSuspended = rhs.FSuspended;

  FTransferringFile = rhs.FTransferringFile;
  FLastSecond = rhs.FLastSecond;

  return *this;
}
