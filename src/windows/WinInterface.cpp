#include <Interface.h>
#include <FarTexts.h>

static TCriticalSection StackTraceCriticalSection;
typedef rde::map<DWORD, TStrings *> TStackTraceMap;
static TStackTraceMap StackTraceMap;

bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages)
{
  bool Result = false;

  TGuard Guard(StackTraceCriticalSection);

  DWORD Id = ::GetCurrentThreadId();
  TStackTraceMap::iterator Iterator = StackTraceMap.find(Id);
  if (Iterator != StackTraceMap.end())
  {
    std::unique_ptr<TStrings> OwnedMoreMessages;
    if (MoreMessages == nullptr)
    {
      OwnedMoreMessages.reset(new TStringList());
      MoreMessages = OwnedMoreMessages.get();
      Result = true;
    }
    if (!MoreMessages->GetText().IsEmpty())
    {
      MoreMessages->SetText(MoreMessages->GetText() + "\n");
    }
    MoreMessages->SetText(MoreMessages->GetText() + LoadStr(MSG_STACK_TRACE) + "\n");
    MoreMessages->AddStrings(Iterator->second);

    delete Iterator->second;
    StackTraceMap.erase(Id);

    OwnedMoreMessages.release();
  }
  return Result;
}

