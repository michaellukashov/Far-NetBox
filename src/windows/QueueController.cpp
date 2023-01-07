
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <CoreMain.h>
#include <Queue.h>
#include <TextsWin.h>
#include <GUITools.h>
#include <WinConfiguration.h>
#include "QueueController.h"
#include <BaseUtils.hpp>

#pragma package(smart_init)

TQueueController::TQueueController(TListView * ListView)
{
  FListView = ListView;
  DebugAssert(FListView != nullptr);
  DebugAssert(FListView->OnDblClick == nullptr);
  FListView->OnDblClick = QueueViewDblClick;
  DebugAssert(FListView->OnKeyDown == nullptr);
  FListView->OnKeyDown = QueueViewKeyDown;
  DebugAssert(FListView->OnCustomDrawItem == nullptr);
  FListView->OnCustomDrawItem = QueueViewCustomDrawItem;

  FQueueStatus = nullptr;
  FOnChange = nullptr;

  RememberConfiguration();
}

TQueueController::~TQueueController()
{
  DebugAssert(FListView->OnDblClick == QueueViewDblClick);
  FListView->OnDblClick = nullptr;
  DebugAssert(FListView->OnKeyDown == QueueViewKeyDown);
  FListView->OnKeyDown = nullptr;
  DebugAssert(FListView->OnCustomDrawItem == QueueViewCustomDrawItem);
  FListView->OnCustomDrawItem = nullptr;
}

TQueueItemProxy * TQueueController::QueueViewItemToQueueItem(TListItem * Item)
{
  // previously this method was based on ActiveCount and DoneCount,
  // as if we were inconfident about validity of Item->Data pointers,
  // not sure why
  return static_cast<TQueueItemProxy *>(Item->Data);
}

TQueueOperation TQueueController::DefaultOperation()
{
  TQueueItemProxy * QueueItem;

  if (FListView->ItemFocused != nullptr)
  {
    QueueItem = QueueViewItemToQueueItem(FListView->ItemFocused);

    switch (QueueItem->Status)
    {
      case TQueueItem::qsPending:
        return qoItemExecute;

      case TQueueItem::qsQuery:
        return qoItemQuery;

      case TQueueItem::qsError:
        return qoItemError;

      case TQueueItem::qsPrompt:
        return qoItemPrompt;

      case TQueueItem::qsProcessing:
        return qoItemPause;

      case TQueueItem::qsPaused:
        return qoItemResume;
    }
  }

  return qoNone;
}

bool TQueueController::AllowOperation(
  TQueueOperation Operation, void ** Param)
{
  TQueueItemProxy * QueueItem = nullptr;

  if (FListView->ItemFocused != nullptr)
  {
    QueueItem = QueueViewItemToQueueItem(FListView->ItemFocused);
  }

  switch (Operation)
  {
    case qoItemUserAction:
      return (QueueItem != nullptr) && TQueueItem::IsUserActionStatus(QueueItem->Status);

    case qoItemQuery:
      return (QueueItem != nullptr) && (QueueItem->Status == TQueueItem::qsQuery);

    case qoItemError:
      return (QueueItem != nullptr) && (QueueItem->Status == TQueueItem::qsError);

    case qoItemPrompt:
      return (QueueItem != nullptr) && (QueueItem->Status == TQueueItem::qsPrompt);

    case qoItemDelete:
      return (QueueItem != nullptr);

    case qoItemExecute:
      return (QueueItem != nullptr) && (QueueItem->Status == TQueueItem::qsPending);

    case qoItemUp:
      return (QueueItem != nullptr) &&
        (QueueItem->Status == TQueueItem::qsPending) &&
        // it's not first pending item,
        // this is based on assumption that pending items occupy single line always
        (FListView->ItemFocused->Index > 0) &&
        (QueueViewItemToQueueItem(FListView->Items->Item[FListView->ItemFocused->Index - 1])->Status == TQueueItem::qsPending);

    case qoItemDown:
      return (QueueItem != nullptr) &&
        (QueueItem->Status == TQueueItem::qsPending) &&
        (FListView->ItemFocused->Index < (FListView->Items->Count - 1));

    case qoItemPause:
      return (QueueItem != nullptr) &&
        (QueueItem->Status == TQueueItem::qsProcessing);

    case qoItemResume:
      return (QueueItem != nullptr) &&
        (QueueItem->Status == TQueueItem::qsPaused);

    case qoItemSpeed:
      {
        bool Result = (QueueItem != nullptr) && (QueueItem->Status != TQueueItem::qsDone);
        if (Result && (Param != nullptr))
        {
          Result = QueueItem->GetCPSLimit(*reinterpret_cast<unsigned long *>(Param));
        }
        return Result;
      }

    case qoPauseAll:
    case qoResumeAll:
      {
        TQueueItem::TStatus Status =
          (Operation == qoPauseAll) ? TQueueItem::qsProcessing : TQueueItem::qsPaused;
        bool Result = false;
        // can be nullptr when action update is triggered while disconnecting
        if (FQueueStatus != nullptr)
        {
          for (int i = FQueueStatus->DoneCount; !Result && (i < FQueueStatus->DoneAndActiveCount); i++)
          {
            QueueItem = FQueueStatus->Items[i];
            Result = (QueueItem->Status == Status);
          }
        }
        return Result;
      }

    case qoDeleteAllDone:
      return (FQueueStatus != nullptr) && (FQueueStatus->DoneCount > 0);

    case qoDeleteAll:
      return (FQueueStatus != nullptr) && (FQueueStatus->Count > 0);

    default:
      DebugFail();
      return false;
  }
}

void TQueueController::ExecuteOperation(TQueueOperation Operation,
  void * Param)
{
  TQueueItemProxy * QueueItem = nullptr;

  if (FListView->ItemFocused != nullptr)
  {
    QueueItem = QueueViewItemToQueueItem(FListView->ItemFocused);
  }

  switch (Operation)
  {
    case qoItemUserAction:
    case qoItemQuery:
    case qoItemError:
    case qoItemPrompt:
      if (QueueItem != nullptr)
      {
        QueueItem->ProcessUserAction();
      }
      break;

    case qoItemExecute:
      if (QueueItem != nullptr)
      {
        QueueItem->ExecuteNow();
      }
      break;

    case qoItemUp:
    case qoItemDown:
      if (QueueItem != nullptr)
      {
        QueueItem->Move(Operation == qoItemUp);
      }
      break;

    case qoItemDelete:
      if (QueueItem != nullptr)
      {
        QueueItem->Delete();
      }
      break;

    case qoItemPause:
      if (QueueItem != nullptr)
      {
        QueueItem->Pause();
      }
      break;

    case qoItemResume:
      if (QueueItem != nullptr)
      {
        QueueItem->Resume();
      }
      break;

    case qoItemSpeed:
      if (QueueItem != nullptr)
      {
        QueueItem->SetCPSLimit(reinterpret_cast<unsigned long>(Param));
      }
      break;

    case qoPauseAll:
    case qoResumeAll:
      {
        for (int i = FQueueStatus->DoneCount; i < FQueueStatus->DoneAndActiveCount; i++)
        {
          QueueItem = FQueueStatus->Items[i];
          if ((Operation == qoPauseAll) && (QueueItem->Status == TQueueItem::qsProcessing))
          {
            QueueItem->Pause();
          }
          else if ((Operation == qoResumeAll) && (QueueItem->Status == TQueueItem::qsPaused))
          {
            QueueItem->Resume();
          }
        }
      }
      break;

    case qoDeleteAllDone:
    case qoDeleteAll:
      {
        int Count = (Operation == qoDeleteAll) ? FQueueStatus->Count : FQueueStatus->DoneCount;
        for (int i = 0; i < Count; i++)
        {
          QueueItem = FQueueStatus->Items[i];
          QueueItem->Delete();
        }
      }
      break;

    default:
      DebugFail();
      break;
  }
}

void TQueueController::FillQueueViewItem(TListItem * Item,
  TQueueItemProxy * QueueItem, bool Detail, bool OnlyLine)
{
  DebugAssert(!Detail || (QueueItem->Status != TQueueItem::qsPending));

  DebugAssert((Item->Data == nullptr) || (Item->Data == QueueItem));
  Item->Data = QueueItem;

  UnicodeString ProgressStr;
  int Image = -1;

  switch (QueueItem->Status)
  {
    case TQueueItem::qsDone:
      ProgressStr = LoadStr(QUEUE_DONE);
      break;

    case TQueueItem::qsPending:
      ProgressStr = LoadStr(QUEUE_PENDING);
      break;

    case TQueueItem::qsConnecting:
      ProgressStr = LoadStr(QUEUE_CONNECTING);
      break;

    case TQueueItem::qsQuery:
      ProgressStr = LoadStr(QUEUE_QUERY);
      Image = 4;
      break;

    case TQueueItem::qsError:
      ProgressStr = LoadStr(QUEUE_ERROR);
      Image = 5;
      break;

    case TQueueItem::qsPrompt:
      ProgressStr = LoadStr(QUEUE_PROMPT);
      Image = 6;
      break;

    case TQueueItem::qsPaused:
      ProgressStr = LoadStr(QUEUE_PAUSED);
      Image = 7;
      break;
  }

  bool BlinkHide = QueueItemNeedsFrequentRefresh(QueueItem) &&
    !QueueItem->ProcessingUserAction &&
    ((GetTickCount() % MSecsPerSec) >= (MSecsPerSec/2));

  int State = -1;
  UnicodeString Values[6];
  TFileOperationProgressType * ProgressData = QueueItem->ProgressData;
  TQueueItem::TInfo * Info = QueueItem->Info;

  if (!Detail && Info->Primary)
  {
    switch (Info->Operation)
    {
      case foCopy:
        State = ((Info->Side == osLocal) ? 2 : 0);
        break;

      case foMove:
        State = ((Info->Side == osLocal) ? 3 : 1);
        break;
    }

    if (!OnlyLine)
    {
      Image = -1;
      ProgressStr = L"";
    }

    // If both are empty, it's bootstrap item => do not show anything
    if (!Info->Source.IsEmpty() || !Info->Destination.IsEmpty())
    {
      // cannot use ProgressData->Temp as it is set only after the transfer actually starts
      Values[0] = Info->Source.IsEmpty() ? LoadStr(PROGRESS_TEMP_DIR) : Info->Source;
      Values[1] = Info->Destination.IsEmpty() ? LoadStr(PROGRESS_TEMP_DIR) : Info->Destination;
    }

    __int64 TotalTransferred = QueueItem->TotalTransferred;
    if (TotalTransferred >= 0)
    {
      Values[2] =
        FormatPanelBytes(TotalTransferred, WinConfiguration->FormatSizeBytes);
    }

    if (ProgressData != nullptr)
    {
      if (ProgressData->Operation == Info->Operation)
      {
        if (QueueItem->Status != TQueueItem::qsDone)
        {
          if (ProgressData->TotalSizeSet)
          {
            Values[3] = FormatDateTimeSpan(Configuration->TimeFormat, ProgressData->TotalTimeLeft());
          }
          else
          {
            Values[3] = FormatDateTimeSpan(Configuration->TimeFormat, ProgressData->TimeElapsed());
          }

          Values[4] = FORMAT("%s/s", FormatBytes(ProgressData->CPS()));
        }

        if (ProgressStr.IsEmpty())
        {
          ProgressStr = FORMAT("%d%%", ProgressData->OverallProgress());
        }
      }
      else if (ProgressData->Operation == foCalculateSize)
      {
        ProgressStr = LoadStr(QUEUE_LISTING);
      }
    }
    Values[5] = ProgressStr;
  }
  else
  {
    if (ProgressData != nullptr)
    {
      if ((Info->Side == osRemote) || !ProgressData->Temp)
      {
        Values[0] = ProgressData->FileName;
      }
      else
      {
        Values[0] = ExtractFileName(ProgressData->FileName);
      }

      if (ProgressData->Operation == Info->Operation)
      {
        Values[2] =
          FormatPanelBytes(ProgressData->TransferredSize, WinConfiguration->FormatSizeBytes);

        if (ProgressStr.IsEmpty())
        {
          ProgressStr = FORMAT("%d%%", ProgressData->TransferProgress());
        }
      }
    }
    Values[5] = ProgressStr;
  }

  Item->StateIndex = (!BlinkHide ? State : -1);
  Item->ImageIndex = (!BlinkHide ? Image : -1);
  for (size_t Index = 0; Index < LENOF(Values); Index++)
  {
    if (Index < static_cast<size_t>(Item->SubItems->Count))
    {
      Item->SubItems->Strings[Index] = Values[Index];
    }
    else
    {
      Item->SubItems->Add(Values[Index]);
    }
  }
}

TListItem * TQueueController::InsertItemFor(TQueueItemProxy * QueueItem, int Index)
{
  TListItem * Item;
  if (Index == FListView->Items->Count)
  {
    Item = FListView->Items->Add();
  }
  else if (FListView->Items->Item[Index]->Data != QueueItem)
  {
    Item = FListView->Items->Insert(Index);
  }
  else
  {
    Item = FListView->Items->Item[Index];
    DebugAssert(Item->Data == QueueItem);
  }
  return Item;
}

void TQueueController::UpdateQueueStatus(
  TTerminalQueueStatus * QueueStatus)
{
  FQueueStatus = QueueStatus;

  if (FQueueStatus != nullptr)
  {
    TQueueItemProxy * QueueItem;
    TListItem * Item;
    int Index = 0;
    for (int ItemIndex = 0; ItemIndex < FQueueStatus->Count; ItemIndex++)
    {
      QueueItem = FQueueStatus->Items[ItemIndex];

      int Index2 = Index;
      while ((Index2 < FListView->Items->Count) &&
             (FListView->Items->Item[Index2]->Data != QueueItem))
      {
        Index2++;
      }

      if (Index2 < FListView->Items->Count)
      {
        while (Index < Index2)
        {
          FListView->Items->Delete(Index);
          Index2--;
        }
      }

      Item = InsertItemFor(QueueItem, Index);
      bool HasDetailsLine = UseDetailsLine(ItemIndex, QueueItem);
      FillQueueViewItem(Item, QueueItem, false, !HasDetailsLine);
      Index++;

      DebugAssert((QueueItem->Status != TQueueItem::qsPending) ==
        (ItemIndex < FQueueStatus->DoneAndActiveCount));

      if (HasDetailsLine)
      {
        Item = InsertItemFor(QueueItem, Index);
        FillQueueViewItem(Item, QueueItem, true, false);
        Index++;
      }
    }

    while (Index < FListView->Items->Count)
    {
      FListView->Items->Delete(Index);
    }
  }
  else
  {
    FListView->Items->Clear();
  }

  DoChange();
}

bool TQueueController::UseDetailsLine(int ItemIndex, TQueueItemProxy * QueueItem)
{
  return
    (ItemIndex >= FQueueStatus->DoneCount) &&
    (ItemIndex < FQueueStatus->DoneAndActiveCount) &&
    QueueItem->Info->Primary &&
    !QueueItem->Info->SingleFile &&
    ((QueueItem->ProgressData == nullptr) || !QueueItem->ProgressData->Done);
}

void TQueueController::RefreshQueueItem(TQueueItemProxy * QueueItem)
{
  TListItem * NextListItem = nullptr;
  TListItem * ListItem;

  ListItem = FListView->FindData(0, QueueItem, true, false);
  DebugAssert(ListItem != nullptr);

  int Index = ListItem->Index;
  if (Index + 1 < FListView->Items->Count)
  {
    NextListItem = FListView->Items->Item[Index + 1];
    if (NextListItem->Data != QueueItem)
    {
      NextListItem = nullptr;
    }
  }

  bool HasDetailsLine = UseDetailsLine(QueueItem->Index, QueueItem);
  FillQueueViewItem(ListItem, QueueItem, false, !HasDetailsLine);

  if (HasDetailsLine)
  {
    if (NextListItem == nullptr)
    {
      NextListItem = FListView->Items->Insert(Index + 1);
    }
    FillQueueViewItem(NextListItem, QueueItem, true, false);
  }
  else
  {
    if (NextListItem != nullptr)
    {
      NextListItem->Delete();
    }
  }

  DoChange();
}

bool TQueueController::QueueItemNeedsFrequentRefresh(
  TQueueItemProxy * QueueItem)
{
  return
    (TQueueItem::IsUserActionStatus(QueueItem->Status) ||
     (QueueItem->Status == TQueueItem::qsPaused));
}

void TQueueController::DoChange()
{
  if (FOnChange != nullptr)
  {
    FOnChange(nullptr);
  }
}

void TQueueController::QueueViewDblClick(TObject * /*Sender*/)
{
  TQueueOperation Operation = DefaultOperation();

  if (Operation != qoNone)
  {
    ExecuteOperation(Operation);
  }
}

void TQueueController::QueueViewKeyDown(TObject * /*Sender*/,
  WORD & Key, TShiftState /*Shift*/)
{
  if (Key == VK_RETURN)
  {
    TQueueOperation Operation = DefaultOperation();

    if (Operation != qoNone)
    {
      ExecuteOperation(Operation);
    }
    Key = 0;
  }
  else if (Key == VK_DELETE)
  {
    ExecuteOperation(qoItemDelete);
    Key = 0;
  }
}

void TQueueController::QueueViewCustomDrawItem(TCustomListView * Sender,
  TListItem * Item, TCustomDrawState /*State*/, bool & /*DefaultDraw*/)
{
  TQueueItemProxy * QueueItem = QueueViewItemToQueueItem(Item);
  if (QueueItem->Status == TQueueItem::qsDone)
  {
    Sender->Canvas->Font->Color = clGrayText;
  }
}

bool TQueueController::GetEmpty()
{
  return (FQueueStatus == nullptr) || (FQueueStatus->Count == 0);
}

void TQueueController::RememberConfiguration()
{
  FFormatSizeBytes = WinConfiguration->FormatSizeBytes;
}

bool TQueueController::NeedRefresh()
{
  bool Result = (WinConfiguration->FormatSizeBytes != FFormatSizeBytes);
  RememberConfiguration();
  return Result;
}

TQueueItemProxy * TQueueController::GetFocusedPrimaryItem()
{
  TQueueItemProxy * Result = nullptr;
  TListItem * PrimaryItemOfFocused = FListView->ItemFocused;
  if (PrimaryItemOfFocused != nullptr)
  {
    while (!QueueViewItemToQueueItem(PrimaryItemOfFocused)->Info->Primary &&
           DebugAlwaysTrue(PrimaryItemOfFocused->Index > 0))
    {
      PrimaryItemOfFocused = FListView->Items->Item[PrimaryItemOfFocused->Index - 1];
    }
    Result = QueueViewItemToQueueItem(PrimaryItemOfFocused);
  }

  return Result;
}
