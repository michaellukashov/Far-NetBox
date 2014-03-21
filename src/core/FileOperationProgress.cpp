//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
#define TRANSFER_BUF_SIZE 64 * 1024
//---------------------------------------------------------------------------
TFileOperationProgressType::TFileOperationProgressType()
{
  FOnProgress = nullptr;
  FOnFinished = nullptr;
  Clear();
}
//---------------------------------------------------------------------------
TFileOperationProgressType::TFileOperationProgressType(
  TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished)
{
  FOnProgress = AOnProgress;
  FOnFinished = AOnFinished;
  FReset = false;
  Clear();
}
//---------------------------------------------------------------------------
TFileOperationProgressType::~TFileOperationProgressType()
{
  assert(!InProgress || FReset);
  assert(!Suspended || FReset);
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Clear()
{
  FSuspendTime = 0,
  FFileStartTime = 0.0;
  FFilesFinished = 0;
  FReset = false;
  FLastSecond = 0;
  FRemainingCPS = 0;
  FTicks.clear();
  FTotalTransferredThen.clear();
  FCounterSet = false;
  Operation = foNone;
  Side = osLocal;
  FileName.Clear();
  Directory.Clear();
  AsciiTransfer = false;
  TransferingFile = false;
  Temp = false;
  LocalSize = 0;
  LocallyUsed = 0;
  // to bypass check in ClearTransfer()
  TransferSize = 0;
  TransferedSize = 0;
  SkippedSize = 0;
  ResumeStatus = rsNotAvailable;
  InProgress = false;
  FileInProgress = false;
  Cancel = csContinue;
  Count = 0;
  StartTime = Now();
  TotalTransfered = 0;
  TotalSkipped = 0;
  TotalSize = 0;
  BatchOverwrite = boNo;
  SkipToAll = false;
  CPSLimit = 0;
  TotalSizeSet = false;
  Suspended = false;

  ClearTransfer();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::ClearTransfer()
{
  if ((TransferSize > 0) && (TransferedSize < TransferSize))
  {
    int64_t RemainingSize = (TransferSize - TransferedSize);
    TotalSkipped += RemainingSize;
  }
  LocalSize = 0;
  TransferSize = 0;
  LocallyUsed = 0;
  SkippedSize = 0;
  TransferedSize = 0;
  TransferingFile = false;
  FLastSecond = 0;
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Start(TFileOperation AOperation,
  TOperationSide ASide, intptr_t ACount)
{
  Start(AOperation, ASide, ACount, false, L"", 0);
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Start(TFileOperation AOperation,
  TOperationSide ASide, intptr_t ACount, bool ATemp,
  const UnicodeString & ADirectory, uintptr_t ACPSLimit)
{
  Clear();
  Operation = AOperation;
  Side = ASide;
  Count = ACount;
  InProgress = true;
  Cancel = csContinue;
  Directory = ADirectory;
  Temp = ATemp;
  CPSLimit = ACPSLimit;
  try
  {
    DoProgress();
  }
  catch (...)
  {
    // connection can be lost during progress callbacks
    ClearTransfer();
    InProgress = false;
    throw;
  }
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Reset()
{
  FReset = true;
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Stop()
{
  // added to include remaining bytes to TotalSkipped, in case
  // the progress happens to update before closing
  ClearTransfer();
  InProgress = false;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Suspend()
{
  assert(!Suspended);
  Suspended = true;
  FSuspendTime = GetTickCount();
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Resume()
{
  assert(Suspended);
  Suspended = false;

  // shift timestamps for CPS calculation in advance
  // by the time the progress was suspended
  uint32_t Stopped = (uint32_t)(GetTickCount() - FSuspendTime);
  size_t I = 0;
  while (I < FTicks.size())
  {
    FTicks[I] += Stopped;
    ++I;
  }

  DoProgress();
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::OperationProgress() const
{
  assert(Count);
  intptr_t Result = (FFilesFinished * 100)/Count;
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::TransferProgress() const
{
  intptr_t Result;
  if (TransferSize)
  {
    Result = static_cast<intptr_t>((TransferedSize * 100) / TransferSize);
  }
  else
  {
    Result = 0;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::TotalTransferProgress() const
{
  assert(TotalSizeSet);
  intptr_t Result = TotalSize > 0 ? static_cast<intptr_t>(((TotalTransfered + TotalSkipped) * 100) / TotalSize) : 0;
  return Result < 100 ? Result : 100;
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::OverallProgress() const
{
  if (TotalSizeSet)
  {
    assert((Operation == foCopy) || (Operation == foMove));
    return TotalTransferProgress();
  }
  else
  {
    return OperationProgress();
  }
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::DoProgress()
{
  SetThreadExecutionState(ES_SYSTEM_REQUIRED);
  FOnProgress(*this, Cancel);
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Finish(const UnicodeString & FileName,
  bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  assert(InProgress);

  FOnFinished(Operation, Side, Temp, FileName,
    /* TODO : There wasn't 'Success' condition, was it by mistake or by purpose? */
    Success && (Cancel == csContinue), OnceDoneOperation);
  FFilesFinished++;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetFile(const UnicodeString & AFileName, bool AFileInProgress)
{
  FileName = AFileName;
  FileInProgress = AFileInProgress;
  ClearTransfer();
  FFileStartTime = Now();
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetFileInProgress()
{
  assert(!FileInProgress);
  FileInProgress = true;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetLocalSize(int64_t ASize)
{
  LocalSize = ASize;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::AddLocallyUsed(int64_t ASize)
{
  LocallyUsed += ASize;
  if (LocallyUsed > LocalSize)
  {
    LocalSize = LocallyUsed;
  }
  DoProgress();
}
//---------------------------------------------------------------------------
bool TFileOperationProgressType::IsLocallyDone() const
{
  assert(LocallyUsed <= LocalSize);
  return (LocallyUsed == LocalSize);
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetSpeedCounters()
{
  if ((CPSLimit > 0) && !FCounterSet)
  {
    FCounterSet = true;
    // Configuration->Usage->Inc(L"SpeedLimitUses");
  }
}
//---------------------------------------------------------------------------
uintptr_t TFileOperationProgressType::AdjustToCPSLimit(
  uintptr_t Size)
{
  SetSpeedCounters();

  if (CPSLimit > 0)
  {
    // we must not return 0, hence, if we reach zero,
    // we wait until the next second
    do
    {
      uintptr_t Second = (GetTickCount() / MSecsPerSec);

      if (Second != FLastSecond)
      {
        FRemainingCPS = CPSLimit;
        FLastSecond = Second;
      }

      if (FRemainingCPS == 0)
      {
        SleepEx(100, true);
        DoProgress();
      }
    }
    while ((CPSLimit > 0) && (FRemainingCPS == 0));

    // CPSLimit may have been dropped in DoProgress
    if (CPSLimit > 0)
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
//---------------------------------------------------------------------------
uintptr_t TFileOperationProgressType::LocalBlockSize()
{
  intptr_t Result = TRANSFER_BUF_SIZE;
  if (LocallyUsed + Result > LocalSize)
  {
    Result = static_cast<uintptr_t>(LocalSize - LocallyUsed);
  }
  Result = AdjustToCPSLimit(Result);
  return Result;
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetTotalSize(int64_t ASize)
{
  TotalSize = ASize;
  TotalSizeSet = true;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetTransferSize(int64_t ASize)
{
  TransferSize = ASize;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::ChangeTransferSize(int64_t ASize)
{
  // reflect change on file size (due to text transfer mode conversion particularly)
  // on total transfer size
  if (TotalSizeSet)
  {
    TotalSize += (ASize - TransferSize);
  }
  TransferSize = ASize;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::RollbackTransfer()
{
  TransferedSize -= SkippedSize;
  assert(TransferedSize <= TotalTransfered);
  TotalTransfered -= TransferedSize;
  assert(SkippedSize <= TotalSkipped);
  FTicks.clear();
  FTotalTransferredThen.clear();
  TotalSkipped -= SkippedSize;
  SkippedSize = 0;
  TransferedSize = 0;
  TransferSize = 0;
  LocallyUsed = 0;
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::AddTransfered(int64_t ASize,
  bool AddToTotals)
{
  TransferedSize += ASize;
  if (TransferedSize > TransferSize)
  {
    // this can happen with SFTP when downloading file that
    // grows while being downloaded
    if (TotalSizeSet)
    {
      TotalSize += (TransferedSize - TransferSize);
    }
    TransferSize = TransferedSize;
  }
  if (AddToTotals)
  {
    TotalTransfered += ASize;
    uint32_t Ticks = static_cast<uint32_t>(GetTickCount());
    if (FTicks.empty() ||
        (FTicks.back() > Ticks) || // ticks wrap after 49.7 days
        ((Ticks - FTicks.back()) >= MSecsPerSec))
    {
      FTicks.push_back(Ticks);
      FTotalTransferredThen.push_back(TotalTransfered);
    }

    if (FTicks.size() > 10)
    {
      FTicks.erase(FTicks.begin());
      FTotalTransferredThen.erase(FTotalTransferredThen.begin());
    }
  }
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::AddResumed(int64_t ASize)
{
  TotalSkipped += ASize;
  SkippedSize += ASize;
  AddTransfered(ASize, false);
  AddLocallyUsed(ASize);
}
//---------------------------------------------------------------------------
uintptr_t TFileOperationProgressType::TransferBlockSize()
{
  uintptr_t Result = TRANSFER_BUF_SIZE;
  if (TransferedSize + (int64_t)Result > TransferSize)
  {
    Result = static_cast<uintptr_t>(TransferSize - TransferedSize);
  }
  Result = AdjustToCPSLimit(Result);
  return Result;
}
//---------------------------------------------------------------------------
uintptr_t TFileOperationProgressType::StaticBlockSize()
{
  return TRANSFER_BUF_SIZE;
}
//---------------------------------------------------------------------------
bool TFileOperationProgressType::IsTransferDone() const
{
  assert(TransferedSize <= TransferSize);
  return (TransferedSize == TransferSize);
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetAsciiTransfer(bool AAsciiTransfer)
{
  AsciiTransfer = AAsciiTransfer;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetResumeStatus(TResumeStatus AResumeStatus)
{
  ResumeStatus = AResumeStatus;
  DoProgress();
}
//---------------------------------------------------------------------------
TDateTime TFileOperationProgressType::TimeElapsed() const
{
  return Now() - StartTime;
}
//---------------------------------------------------------------------------
uintptr_t TFileOperationProgressType::CPS()
{
  uintptr_t Result;
  if (FTicks.empty())
  {
    Result = 0;
  }
  else
  {
    uintptr_t Ticks = (Suspended ? FSuspendTime : GetTickCount());
    uintptr_t TimeSpan;
    if (Ticks < FTicks.front())
    {
      // clocks has wrapped, guess 10 seconds difference
      TimeSpan = 10000;
    }
    else
    {
      TimeSpan = (Ticks - FTicks.front());
    }

    if (TimeSpan == 0)
    {
      Result = 0;
    }
    else
    {
      int64_t Transferred = (TotalTransfered - FTotalTransferredThen.front());
      Result = static_cast<uintptr_t>(Transferred * MSecsPerSec / TimeSpan);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TFileOperationProgressType::TimeExpected()
{
  uintptr_t CurCps = CPS();
  if (CurCps)
  {
    return TDateTime(ToDouble((ToDouble(TransferSize - TransferedSize)) / CurCps) / SecsPerDay);
  }
  else
  {
    return TDateTime(0.0);
  }
}
//---------------------------------------------------------------------------
TDateTime TFileOperationProgressType::TotalTimeExpected()
{
  assert(TotalSizeSet);
  uintptr_t CurCps = CPS();
  // sanity check
  if ((CurCps > 0) && (TotalSize > TotalSkipped))
  {
    return TDateTime(ToDouble(ToDouble(TotalSize - TotalSkipped) / CurCps) /
      SecsPerDay);
  }
  else
  {
    return TDateTime(0.0);
  }
}
//---------------------------------------------------------------------------
TDateTime TFileOperationProgressType::TotalTimeLeft()
{
  assert(TotalSizeSet);
  uintptr_t CurCps = CPS();
  // sanity check
  if ((CurCps > 0) && (TotalSize > TotalSkipped + TotalTransfered))
  {
    return TDateTime(ToDouble(ToDouble(TotalSize - TotalSkipped - TotalTransfered) / CurCps) /
      SecsPerDay);
  }
  else
  {
    return TDateTime(0.0);
  }
}
