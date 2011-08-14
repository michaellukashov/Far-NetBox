//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
bool ExceptionMessage(exception * E, wstring & Message)
{
  bool Result = true;
  if (dynamic_cast<EAbort *>(E) != NULL)
  {
    Result = false;
  }
  else if (dynamic_cast<EAccessViolation*>(E) != NULL)
  {
    Message = LoadStr(ACCESS_VIOLATION_ERROR);
  }
  else if (E->Message.IsEmpty())
  {
    Result = false;
  }
  else
  {
    Message = E->Message;
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * ExceptionToMoreMessages(exception * E)
{
  TStrings * Result = NULL;
  wstring Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * ExtE = dynamic_cast<ExtException *>(E);
    if (ExtE != NULL)
    {
      Result->AddStrings(ExtE->MoreMessages);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
ExtException::ExtException(exception * E) :
  exception("")
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(exception* E, wstring Msg):
  exception(Msg)
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(wstring Msg, exception* E) :
  exception("")
{
  // "copy exception"
  AddMoreMessages(E);
  // and append message to the end to more messages
  if (!Msg.IsEmpty())
  {
    if (Message.IsEmpty())
    {
      Message = Msg;
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
ExtException::ExtException(wstring Msg, wstring MoreMessages,
    wstring HelpKeyword) :
  exception(Msg),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.IsEmpty())
  {
    FMoreMessages = new TStringList();
    FMoreMessages->Text = MoreMessages;
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(wstring Msg, TStrings* MoreMessages,
  bool Own) :
  exception(Msg)
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
void ExtException::AddMoreMessages(exception* E)
{
  if (E != NULL)
  {
    if (FMoreMessages == NULL)
    {
      FMoreMessages = new TStringList();
    }

    ExtException * ExtE = dynamic_cast<ExtException *>(E);
    if (ExtE != NULL)
    {
      if (!ExtE->HelpKeyword.IsEmpty())
      {
        // we have to yet decide what to do now
        assert(HelpKeyword.IsEmpty());

        FHelpKeyword = ExtE->HelpKeyword;
      }

      if (ExtE->MoreMessages != NULL)
      {
        FMoreMessages->Assign(ExtE->MoreMessages);
      }
    }

    wstring Msg;
    ExceptionMessage(E, Msg);

    // new exception does not have own message, this is in fact duplication of
    // the exception data, but the exception class may being changed
    if (Message.IsEmpty())
    {
      Message = Msg;
    }
    else if (!Msg.IsEmpty())
    {
      FMoreMessages->Insert(0, Msg);
    }

    if (FMoreMessages->Count == 0)
    {
      delete FMoreMessages;
      FMoreMessages = NULL;
    }
  }
}
//---------------------------------------------------------------------------
ExtException::~ExtException()
{
  delete FMoreMessages;
}
//---------------------------------------------------------------------------
wstring LastSysErrorMessage()
{
  int LastError = GetLastError();
  wstring Result;
  if (LastError != 0)
  {
    Result = FORMAT(Sysconst_SOSError, (LastError, SysErrorMessage(LastError)));
  }
  return Result;
}
//---------------------------------------------------------------------------
EOSExtException::EOSExtException(wstring Msg) :
  ExtException(Msg, LastSysErrorMessage())
{
}
