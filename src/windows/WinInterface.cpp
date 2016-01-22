#include <Interface.h>

bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages)
{
  bool Result = false;

  TGuard Guard(StackTraceCriticalSection.get());

  TStackTraceMap::iterator Iterator = StackTraceMap.find(GetCurrentThreadId());
  if (Iterator != StackTraceMap.end())
  {
    std::unique_ptr<TStrings> OwnedMoreMessages;
    if (MoreMessages == NULL)
    {
      OwnedMoreMessages.reset(new TStringList());
      MoreMessages = OwnedMoreMessages.get();
      Result = true;
    }
    if (!MoreMessages->Text.IsEmpty())
    {
      MoreMessages->Text = MoreMessages->Text + "\n";
    }
    MoreMessages->Text = MoreMessages->Text + LoadStr(STACK_TRACE) + "\n";
    MoreMessages->AddStrings(Iterator->second);

    delete Iterator->second;
    StackTraceMap.erase(Iterator);

    OwnedMoreMessages.release();
  }
  return Result;
}

