//---------------------------------------------------------------------------
#include "stdafx.h"
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#endif

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
    const TFileOperationProgressEvent &AOnProgress,
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
void __fastcall TFileOperationProgressType::Clear()
{
    FileName = L"";
    AsciiTransfer = false;
    ResumeStatus = rsNotAvailable;
    Count = 0;
    FFilesFinished = 0;
    StartTime = System::Now();
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
void __fastcall TFileOperationProgressType::ClearTransfer()
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
void __fastcall TFileOperationProgressType::Start(TFileOperation AOperation,
                                       TOperationSide ASide, size_t ACount)
{
    Start(AOperation, ASide, ACount, false, L"", 0);
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::Start(TFileOperation AOperation,
                                       TOperationSide ASide, size_t ACount, bool ATemp,
                                       const UnicodeString ADirectory, size_t ACPSLimit)
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
void __fastcall TFileOperationProgressType::Reset()
{
    FReset = true;
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::Stop()
{
    // added to include remaining bytes to TotalSkipped, in case
    // the progress happes to update before closing
    ClearTransfer();
    InProgress = false;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::Suspend()
{
    assert(!Suspended);
    Suspended = true;
    FSuspendTime = GetTickCount();
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::Resume()
{
    assert(Suspended);
    Suspended = false;

    // shift timestamps for CPS calculation in advance
    // by the time the progress was suspended
    unsigned long Stopped = (GetTickCount() - FSuspendTime);
    size_t i = 0;
    while (i < FTicks.Length())
    {
        FTicks[i] += Stopped;
        ++i;
    }

    DoProgress();
}
//---------------------------------------------------------------------------
size_t __fastcall TFileOperationProgressType::OperationProgress()
{
    assert(Count);
    size_t Result = (FFilesFinished * 100) / Count;
    return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TFileOperationProgressType::TransferProgress()
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
size_t __fastcall TFileOperationProgressType::TotalTransferProgress()
{
    assert(TotalSizeSet);
    size_t Result = TotalSize > 0 ? static_cast<int>(((TotalTransfered + TotalSkipped) * 100) / TotalSize) : 0;
    return Result < 100 ? Result : 100;
}
//---------------------------------------------------------------------------
size_t __fastcall TFileOperationProgressType::OverallProgress()
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
void __fastcall TFileOperationProgressType::DoProgress()
{
    SetThreadExecutionState(ES_SYSTEM_REQUIRED);
    if (!FOnProgress.IsEmpty())
    {
        FOnProgress(*this, Cancel);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::Finish(const UnicodeString FileName,
                                        bool Success, TOnceDoneOperation &OnceDoneOperation)
{
    assert(InProgress);
    if (!FOnFinished.IsEmpty())
    {
        FOnFinished(Operation, Side, Temp, FileName,
                    // TODO : There wasn't 'Success' condition, was it by mistake or by purpose?
                    Success && (Cancel == csContinue), OnceDoneOperation);
    }
    FFilesFinished++;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::SetFile(const UnicodeString AFileName, bool AFileInProgress)
{
    FileName = AFileName;
    FileInProgress = AFileInProgress;
    ClearTransfer();
    FFileStartTime = System::Now();
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::SetFileInProgress()
{
    assert(!FileInProgress);
    FileInProgress = true;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::SetLocalSize(__int64 ASize)
{
    LocalSize = ASize;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::AddLocallyUsed(__int64 ASize)
{
    LocallyUsed += ASize;
    if (LocallyUsed > LocalSize)
    {
        LocalSize = LocallyUsed;
    }
    DoProgress();
}
//---------------------------------------------------------------------------
bool __fastcall TFileOperationProgressType::IsLocallyDone()
{
    assert(LocallyUsed <= LocalSize);
    return (LocallyUsed == LocalSize);
}
//---------------------------------------------------------------------------
size_t __fastcall TFileOperationProgressType::AdjustToCPSLimit(
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
size_t __fastcall TFileOperationProgressType::LocalBlockSize()
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
void __fastcall TFileOperationProgressType::SetTotalSize(__int64 ASize)
{
    TotalSize = ASize;
    TotalSizeSet = true;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::SetTransferSize(__int64 ASize)
{
    TransferSize = ASize;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::ChangeTransferSize(__int64 ASize)
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
void __fastcall TFileOperationProgressType::RollbackTransfer()
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
void __fastcall TFileOperationProgressType::AddTransfered(__int64 ASize,
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
        size_t Ticks = static_cast<size_t>(GetTickCount());
        if (FTicks.IsEmpty() ||
                (FTicks.back() > Ticks) || // ticks wrap after 49.7 days
                ((Ticks - FTicks.back()) >= 1000))
        {
            FTicks.push_back(Ticks);
            FTotalTransferredThen.push_back(TotalTransfered);
        }

        if (FTicks.Length() > 10)
        {
            FTicks.Delete(FTicks.begin());
            FTotalTransferredThen.Delete(FTotalTransferredThen.begin());
        }
    }
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::AddResumed(__int64 ASize)
{
    TotalSkipped += ASize;
    SkippedSize += ASize;
    AddTransfered(ASize, false);
    AddLocallyUsed(ASize);
}
//---------------------------------------------------------------------------
size_t __fastcall TFileOperationProgressType::TransferBlockSize()
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
size_t __fastcall TFileOperationProgressType::StaticBlockSize()
{
    return TRANSFER_BUF_SIZE;
}
//---------------------------------------------------------------------------
bool __fastcall TFileOperationProgressType::IsTransferDone()
{
    assert(TransferedSize <= TransferSize);
    return (TransferedSize == TransferSize);
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::SetAsciiTransfer(bool AAsciiTransfer)
{
    AsciiTransfer = AAsciiTransfer;
    DoProgress();
}
//---------------------------------------------------------------------------
void __fastcall TFileOperationProgressType::SetResumeStatus(TResumeStatus AResumeStatus)
{
    ResumeStatus = AResumeStatus;
    DoProgress();
}
//---------------------------------------------------------------------------
System::TDateTime __fastcall TFileOperationProgressType::TimeElapsed()
{
    return System::Now() - StartTime;
}
//---------------------------------------------------------------------------
size_t __fastcall TFileOperationProgressType::CPS()
{
    size_t Result;
    if (FTicks.IsEmpty())
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
System::TDateTime __fastcall TFileOperationProgressType::TimeExpected()
{
    size_t CurCps = CPS();
    if (CurCps)
    {
        return System::TDateTime(static_cast<double>((static_cast<double>(TransferSize - TransferedSize)) / CurCps) / (24 * 60 * 60));
    }
    else
    {
        return System::TDateTime(0);
    }
}
//---------------------------------------------------------------------------
System::TDateTime __fastcall TFileOperationProgressType::TotalTimeExpected()
{
    assert(TotalSizeSet);
    size_t CurCps = CPS();
    // sanity check
    if ((CurCps > 0) && (TotalSize > TotalSkipped))
    {
        return System::TDateTime(static_cast<double>(static_cast<double>(TotalSize - TotalSkipped) / CurCps) /
                             (24 * 60 * 60));
    }
    else
    {
        return System::TDateTime(0);
    }
}
//---------------------------------------------------------------------------
System::TDateTime __fastcall TFileOperationProgressType::TotalTimeLeft()
{
    assert(TotalSizeSet);
    size_t CurCps = CPS();
    // sanity check
    if ((CurCps > 0) && (TotalSize > TotalSkipped + TotalTransfered))
    {
        return System::TDateTime(static_cast<double>(static_cast<double>(TotalSize - TotalSkipped - TotalTransfered) / CurCps) /
                             (24 * 60 * 60));
    }
    else
    {
        return System::TDateTime(0);
    }
}
