//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Common.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
#define TRANSFER_BUF_SIZE 4096
//---------------------------------------------------------------------------
TFileOperationProgressType::TFileOperationProgressType()
{
    Clear();
}
//---------------------------------------------------------------------------
TFileOperationProgressType::TFileOperationProgressType(
    const fileoperationprogress_slot_type &AOnProgress,
    const fileoperationfinished_slot_type &AOnFinished)
{
    FOnProgress.connect(AOnProgress);
    FOnFinished.connect(AOnFinished);
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
    FileName = L"";
    AsciiTransfer = false;
    ResumeStatus = rsNotAvailable;
    Count = 0;
    FFilesFinished = 0;
    StartTime = nb::Now();
    Suspended = false;
    FSuspendTime = 0;
    InProgress = false;
    FileInProgress = false;
    TotalTransfered = 0;
    TotalSkipped = 0;
    TotalSize = 0;
    SkippedSize = 0;
    TotalSizeSet = false;
    Operation = foNone;
    Temp = false;
    SkipToAll = false;
    BatchOverwrite = boNo;
    // to bypass check in ClearTransfer()
    TransferSize = 0;
    CPSLimit = 0;
    FTicks.clear();
    FTotalTransferredThen.clear();
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
                                       TOperationSide ASide, size_t ACount)
{
    Start(AOperation, ASide, ACount, false, L"", 0);
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Start(TFileOperation AOperation,
                                       TOperationSide ASide, size_t ACount, bool ATemp,
                                       const std::wstring ADirectory, size_t ACPSLimit)
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
    DoProgress();
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
    // the progress happes to update before closing
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
size_t TFileOperationProgressType::OperationProgress()
{
    assert(Count);
    size_t Result = (FFilesFinished * 100) / Count;
    return Result;
}
//---------------------------------------------------------------------------
size_t TFileOperationProgressType::TransferProgress()
{
    size_t Result;
    if (TransferSize)
    {
        Result = static_cast<size_t>((TransferedSize * 100) / TransferSize);
    }
    else
    {
        Result = 0;
    }
    return Result;
}
//---------------------------------------------------------------------------
size_t TFileOperationProgressType::TotalTransferProgress()
{
    assert(TotalSizeSet);
    size_t Result = TotalSize > 0 ? static_cast<int>(((TotalTransfered + TotalSkipped) * 100) / TotalSize) : 0;
    return Result < 100 ? Result : 100;
}
//---------------------------------------------------------------------------
size_t TFileOperationProgressType::OverallProgress()
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
    if (!FOnProgress.empty())
    {
        FOnProgress(*this, Cancel);
    }
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::Finish(const std::wstring FileName,
                                        bool Success, TOnceDoneOperation &OnceDoneOperation)
{
    assert(InProgress);
    if (!FOnFinished.empty())
    {
        FOnFinished(Operation, Side, Temp, FileName,
                    // TODO : There wasn't 'Success' condition, was it by mistake or by purpose?
                    Success && (Cancel == csContinue), OnceDoneOperation);
    }
    FFilesFinished++;
    DoProgress();
}
//---------------------------------------------------------------------------
void TFileOperationProgressType::SetFile(const std::wstring AFileName, bool AFileInProgress)
{
    FileName = AFileName;
    FileInProgress = AFileInProgress;
    ClearTransfer();
    FFileStartTime = nb::Now();
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
bool TFileOperationProgressType::IsLocallyDone()
{
    assert(LocallyUsed <= LocalSize);
    return (LocallyUsed == LocalSize);
}
//---------------------------------------------------------------------------
size_t TFileOperationProgressType::AdjustToCPSLimit(
    size_t Size)
{
    if (CPSLimit > 0)
    {
        // we must not return 0, hence, if we reach zero,
        // we wait until the next second
        do
        {
            size_t Second = (GetTickCount() / 1000);

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
size_t TFileOperationProgressType::LocalBlockSize()
{
    size_t Result = TRANSFER_BUF_SIZE;
    if (LocallyUsed + Result > LocalSize)
    {
        Result = static_cast<size_t>(LocalSize - LocallyUsed);
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
        unsigned long Ticks = GetTickCount();
        if (FTicks.empty() ||
                (FTicks.back() > Ticks) || // ticks wrap after 49.7 days
                ((Ticks - FTicks.back()) >= 1000))
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
size_t TFileOperationProgressType::TransferBlockSize()
{
    size_t Result = TRANSFER_BUF_SIZE;
    if (TransferedSize + Result > TransferSize)
    {
        Result = (TransferSize - TransferedSize);
    }
    Result = AdjustToCPSLimit(Result);
    return Result;
}
//---------------------------------------------------------------------------
size_t TFileOperationProgressType::StaticBlockSize()
{
    return TRANSFER_BUF_SIZE;
}
//---------------------------------------------------------------------------
bool TFileOperationProgressType::IsTransferDone()
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
nb::TDateTime TFileOperationProgressType::TimeElapsed()
{
    return nb::Now() - StartTime;
}
//---------------------------------------------------------------------------
size_t TFileOperationProgressType::CPS()
{
    size_t Result;
    if (FTicks.empty())
    {
        Result = 0;
    }
    else
    {
        size_t Ticks = (Suspended ? FSuspendTime : GetTickCount());
        size_t TimeSpan;
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
            Result = static_cast<size_t>(Transferred * 1000 / TimeSpan);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
nb::TDateTime TFileOperationProgressType::TimeExpected()
{
    unsigned int CurCps = CPS();
    if (CurCps)
    {
        return nb::TDateTime(static_cast<double>((static_cast<double>(TransferSize - TransferedSize)) / CurCps) / (24 * 60 * 60));
    }
    else
    {
        return nb::TDateTime(0);
    }
}
//---------------------------------------------------------------------------
nb::TDateTime TFileOperationProgressType::TotalTimeExpected()
{
    assert(TotalSizeSet);
    size_t CurCps = CPS();
    // sanity check
    if ((CurCps > 0) && (TotalSize > TotalSkipped))
    {
        return nb::TDateTime(static_cast<double>(static_cast<double>(TotalSize - TotalSkipped) / CurCps) /
                             (24 * 60 * 60));
    }
    else
    {
        return nb::TDateTime(0);
    }
}
//---------------------------------------------------------------------------
nb::TDateTime TFileOperationProgressType::TotalTimeLeft()
{
    assert(TotalSizeSet);
    size_t CurCps = CPS();
    // sanity check
    if ((CurCps > 0) && (TotalSize > TotalSkipped + TotalTransfered))
    {
        return nb::TDateTime(static_cast<double>(static_cast<double>(TotalSize - TotalSkipped - TotalTransfered) / CurCps) /
                             (24 * 60 * 60));
    }
    else
    {
        return nb::TDateTime(0);
    }
}
