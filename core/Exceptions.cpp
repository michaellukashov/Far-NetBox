//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Exceptions.h"
#include "Common.h"
#include "TextsCore.h"
#include "Terminal.h"

//---------------------------------------------------------------------------
bool ExceptionMessage(const std::exception *E, std::wstring & Message)
{
  bool Result = true;
  if (dynamic_cast<const EAbort *>(E) != NULL)
  {
    Result = false;
  }
  else if (dynamic_cast<const EAccessViolation *>(E) != NULL)
  {
    Message = LoadStr(ACCESS_VIOLATION_ERROR);
  }
  else if (std::string(E->what()).empty())
  {
    Result = false;
  }
  else
  {
    Message = ::MB2W(E->what());
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings *ExceptionToMoreMessages(const std::exception *E)
{
  TStrings *Result = NULL;
  std::wstring Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    const ExtException *ExtE = dynamic_cast<const ExtException *>(E);
    if (ExtE != NULL)
    {
      Result->AddStrings(ExtE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
ExtException::ExtException(const std::wstring Msg, int AHelpContext) :
    parent(::W2MB(Msg.c_str()).c_str(), AHelpContext),
    FMoreMessages(NULL)
{
}
//---------------------------------------------------------------------------
ExtException::ExtException(const std::exception *E) :
  parent(""),
  FMoreMessages(NULL)
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(const std::exception *E, std::wstring Msg) :
  parent(::W2MB(Msg.c_str()).c_str()),
  FMoreMessages(NULL)
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(std::wstring Msg) :
  parent(::W2MB(Msg.c_str()).c_str()),
  FMoreMessages(NULL)
{
  // append message to the end to more messages
  // DEBUG_PRINTF(L"ExtException");
  if (!Msg.empty())
  {
    if (GetMessage().empty())
    {
      SetMessage(Msg);
    }
    else
    {
      if (FMoreMessages == NULL)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(Msg);
    }
  }
}

//---------------------------------------------------------------------------
ExtException::ExtException(std::wstring Msg, const std::exception *E) :
  parent(::W2MB(Msg.c_str()).c_str()),
  FMoreMessages(NULL)
{
  // "copy std::exception"
  AddMoreMessages(E);
  // and append message to the end to more messages
  if (!Msg.empty())
  {
    if (GetMessage().empty())
    {
      SetMessage(Msg);
    }
    else
    {
      if (FMoreMessages == NULL)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(Msg);
    }
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(std::wstring Msg, std::wstring MoreMessages,
    std::wstring HelpKeyword) :
  parent(::W2MB(Msg.c_str()).c_str()),
  FMoreMessages(NULL),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.empty())
  {
    FMoreMessages = new TStringList();
    FMoreMessages->SetText(MoreMessages);
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(std::wstring Msg, TStrings *MoreMessages,
  bool Own) :
  parent(::W2MB(Msg.c_str()).c_str()),
  FMoreMessages(NULL)
{
  if (Own)
  {
    FMoreMessages = MoreMessages;
  }
  else
  {
    FMoreMessages = new TStringList();
    FMoreMessages->Assign(MoreMessages);
  }
}
//---------------------------------------------------------------------------
void ExtException::AddMoreMessages(const std::exception *E)
{
  if (E != NULL)
  {
    if (FMoreMessages == NULL)
    {
      FMoreMessages = new TStringList();
    }

    const ExtException *ExtE = dynamic_cast<const ExtException *>(E);
    if (ExtE != NULL)
    {
      if (!ExtE->GetHelpKeyword().empty())
      {
        // we have to yet decide what to do now
        assert(GetHelpKeyword().empty());

        FHelpKeyword = ExtE->GetHelpKeyword();
      }

      if (ExtE->GetMoreMessages() != NULL)
      {
        FMoreMessages->Assign(ExtE->GetMoreMessages());
      }
    }

    std::wstring Msg;
    ExceptionMessage(E, Msg);

    // new std::exception does not have own message, this is in fact duplication of
    // the std::exception data, but the std::exception class may being changed
    if (GetMessage().empty())
    {
      SetMessage(Msg);
    }
    else if (!Msg.empty())
    {
      FMoreMessages->Insert(0, Msg);
    }

    if (FMoreMessages->GetCount() == 0)
    {
      delete FMoreMessages;
      FMoreMessages = NULL;
    }
  }
}
//---------------------------------------------------------------------------
ExtException::~ExtException()
{
    DEBUG_PRINTF(L"~ExtException");
  delete FMoreMessages;
}
//---------------------------------------------------------------------------
std::wstring LastSysErrorMessage()
{
  int LastError = GetLastError();
  std::wstring Result;
  if (LastError != 0)
  {
    Result = FORMAT(L"System Error.  Code: %d.\r\n%s", LastError, SysErrorMessage(LastError).c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
EOSExtException::EOSExtException(std::wstring Msg) :
  parent(Msg, LastSysErrorMessage())
{
}
