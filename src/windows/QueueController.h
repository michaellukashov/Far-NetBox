
#ifndef QueueControllerH
#define QueueControllerH

#include <ComCtrls.hpp>

enum TQueueOperation { qoNone, qoGoTo, qoPreferences, qoItemUserAction,
  qoItemQuery, qoItemError, qoItemPrompt, qoItemDelete, qoItemExecute,
  qoItemUp, qoItemDown, qoItemPause, qoItemResume, qoItemSpeed, qoPauseAll, qoResumeAll,
  qoOnceEmpty, qoDeleteAllDone, qoDeleteAll };
class TQueueItemProxy;
class TTerminalQueueStatus;

class TQueueController
{
public:
  TQueueController(TListView * ListView);
  virtual ~TQueueController();

  TQueueOperation DefaultOperation();
  bool AllowOperation(TQueueOperation Operation, void ** Param = nullptr);
  void ExecuteOperation(TQueueOperation Operation, void * Param = nullptr);

  void UpdateQueueStatus(TTerminalQueueStatus * QueueStatus);
  void RefreshQueueItem(TQueueItemProxy * QueueItem);
  static bool QueueItemNeedsFrequentRefresh(TQueueItemProxy * QueueItem);

  bool NeedRefresh();

  TQueueItemProxy * GetFocusedPrimaryItem();

  __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  __property bool Empty = { read = GetEmpty };

private:
  TListView * FListView;
  TTerminalQueueStatus * FQueueStatus;
  TNotifyEvent FOnChange;
  TFormatBytesStyle FFormatSizeBytes;

  TQueueItemProxy * QueueViewItemToQueueItem(TListItem * Item);
  void QueueViewDblClick(TObject * Sender);
  void QueueViewKeyDown(TObject * Sender, WORD & Key, TShiftState Shift);
  void QueueViewCustomDrawItem(TCustomListView * Sender, TListItem * Item,
    TCustomDrawState State, bool & DefaultDraw);
  virtual void DoChange();
  bool GetEmpty();
  void RememberConfiguration();

  static void FillQueueViewItem(TListItem * Item,
    TQueueItemProxy * QueueItem, bool Detail, bool OnlyLine);
  TListItem * InsertItemFor(TQueueItemProxy * QueueItem, int Index);
  bool UseDetailsLine(int ItemIndex, TQueueItemProxy * QueueItem);
};

#endif
