//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define TRACE_PROGRESS TRACING

#include "Common.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
#define TRANSFER_BUF_SIZE 4096
//---------------------------------------------------------------------------
TFileOperationProgressType::TFileOperationProgressType()
{
  FOnProgress = NULL;
  FOnFinished = NULL;
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
    __int64 RemainingSize = (TransferSize - TransferedSize);
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
  const UnicodeString & ADirectory, unsigned long ACPSLimit)
{
  CALLSTACK;
  Clear();
  Operation = AOperation;
  Side = ASide;
  Count = ACount;
  InProgress = true;
  Cancel = csContinue;
  Directory = ADirectory;
  Temp = ATemp;
  CPSLimit = ACPSLimit;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Reset()
{
  CALLSTACK;
  FReset = true;
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Stop()
{
  CALLSTACK;
  // added to include remaining bytes to TotalSkipped, in case
  // the progress happes to update before closing
  ClearTransfer();
  InProgress = false;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Suspend()
{
  CALLSTACK;
  assert(!Suspended);
  Suspended = true;
  FSuspendTime = GetTickCount();
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Resume()
{
  CALLSTACK;
  assert(Suspended);
  Suspended = false;

  // shift timestamps for CPS calculation in advance
  // by the time the progress was suspended
  unsigned long Stopped = (GetTickCount() - FSuspendTime);
  size_t i = 0;
  while (i < FTicks.size())
  {
    FTicks[i] += Stopped;
    ++i;
  }

  DoProgress();
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::OperationProgress() const
{
  CCALLSTACK(TRACE_PROGRESS);
  assert(Count);
  intptr_t Result = (FFilesFinished * 100)/Count;
  CTRACEFMT(TRACE_PROGRESS, "[%d]", Result);
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::TransferProgress() const
{
  CCALLSTACK(TRACE_PROGRESS);
  intptr_t Result;
  if (TransferSize)
  {
    Result = static_cast<intptr_t>((TransferedSize * 100) / TransferSize);
  }
  else
  {
    Result = 0;
  }
  CTRACEFMT(TRACE_PROGRESS, "[%d]", Result);
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::TotalTransferProgress() const
{
  CCALLSTACK(TRACE_PROGRESS);
  assert(TotalSizeSet);
  intptr_t Result = TotalSize > 0 ? static_cast<intptr_t>(((TotalTransfered + TotalSkipped) * 100) / TotalSize) : 0;
  CTRACEFMT(TRACE_PROGRESS, "[%d]", Result);
  return Result < 100 ? Result : 100;
}
//---------------------------------------------------------------------------
intptr_t TFileOperationProgressType::OverallProgress() const
{
  CCALLSTACK(TRACE_PROGRESS);
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
void TFileOperationProgressType::Finish(UnicodeString FileName,
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
void TFileOperationProgressType::SetFile(UnicodeString AFileName, bool AFileInProgress)
{
  CALLSTACK;
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
void TFileOperationProgressType::SetLocalSize(__int64 ASize)
{
  LocalSize = ASize;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::AddLocallyUsed(__int64 ASize)
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
unsigned long TFileOperationProgressType::AdjustToCPSLimit(
  unsigned long Size)
{
  if (CPSLimit > 0)
  {
    // we must not return 0, hence, if we reach zero,
    // we wait until the next second
    do
    {
      unsigned int Second = (GetTickCount() / MSecsPerSec);

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
unsigned long TFileOperationProgressType::LocalBlockSize()
{
  unsigned long Result = TRANSFER_BUF_SIZE;
  if (LocallyUsed + Result > LocalSize)
  {
    Result = static_cast<unsigned long>(LocalSize - LocallyUsed);
  }
  Result = AdjustToCPSLimit(Result);
  return Result;
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetTotalSize(__int64 ASize)
{
  TotalSize = ASize;
  TotalSizeSet = true;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetTransferSize(__int64 ASize)
{
  TransferSize = ASize;
  DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::ChangeTransferSize(__int64 ASize)
{
  // reflect change on file size (due to text transfer mode conversion particulary)
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
void TFileOperationProgressType::AddTransfered(__int64 ASize,
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
    unsigned long Ticks = static_cast<unsigned long>(GetTickCount());
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
void TFileOperationProgressType::AddResumed(__int64 ASize)
{
  TotalSkipped += ASize;
  SkippedSize += ASize;
  AddTransfered(ASize, false);
  AddLocallyUsed(ASize);
}
//---------------------------------------------------------------------------
unsigned long TFileOperationProgressType::TransferBlockSize()
{
  unsigned long Result = TRANSFER_BUF_SIZE;
  if (TransferedSize + Result > TransferSize)
  {
    Result = static_cast<unsigned long>(TransferSize - TransferedSize);
  }
  Result = AdjustToCPSLimit(Result);
  return Result;
}
//---------------------------------------------------------------------------
unsigned long TFileOperationProgressType::StaticBlockSize()
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
unsigned int TFileOperationProgressType::CPS()
{
  unsigned int Result;
  if (FTicks.empty())
  {
    Result = 0;
  }
  else
  {
    unsigned long Ticks = (Suspended ? FSuspendTime : GetTickCount());
    unsigned long TimeSpan;
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
      __int64 Transferred = (TotalTransfered - FTotalTransferredThen.front());
      Result = static_cast<unsigned int>(Transferred * MSecsPerSec / TimeSpan);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TFileOperationProgressType::TimeExpected()
{
  unsigned int CurCps = CPS();
  if (CurCps)
  {
    return TDateTime(static_cast<double>((static_cast<double>(TransferSize - TransferedSize)) / CurCps) / SecsPerDay);
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
  unsigned int CurCps = CPS();
  // sanity check
  if ((CurCps > 0) && (TotalSize > TotalSkipped))
  {
    return TDateTime(static_cast<double>(static_cast<double>(TotalSize - TotalSkipped) / CurCps) /
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
  unsigned int CurCps = CPS();
  // sanity check
  if ((CurCps > 0) && (TotalSize > TotalSkipped + TotalTransfered))
  {
    return TDateTime(static_cast<double>(static_cast<double>(TotalSize - TotalSkipped - TotalTransfered) / CurCps) /
      SecsPerDay);
  }
  else
  {
    return TDateTime(0.0);
  }
}
