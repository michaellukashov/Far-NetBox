//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
bool ExceptionMessage(const Exception * E, UnicodeString & Message)
{
  bool Result = true;
  if (dynamic_cast<const EAbort *>(E) != nullptr)
  {
    Result = false;
  }
  else if (dynamic_cast<const EAccessViolation*>(E) != nullptr)
  {
    Message = LoadStr(ACCESS_VIOLATION_ERROR2);
  }
  else if (E && E->Message.IsEmpty())
  {
    Result = false;
  }
  else if (E)
  {
    Message = E->Message;
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * ExceptionToMoreMessages(Exception * E)
{
  TStrings * Result = nullptr;
  UnicodeString Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * ExtE = dynamic_cast<ExtException *>(E);
    if (ExtE != nullptr)
    {
      Result->AddStrings(ExtE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
ExtException::ExtException(Exception * E) :
  Exception(L""),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(Exception* E, const UnicodeString & Msg):
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
  AddMoreMessages(E);
}
ExtException::ExtException(ExtException* E, const UnicodeString & Msg):
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(Exception * E, int Ident) :
  Exception(E, Ident),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString & Msg, Exception* E) :
  Exception(L""),
  FMoreMessages(nullptr),
  FHelpKeyword()
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
      if (FMoreMessages == nullptr)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(Msg);
    }
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages,
  const UnicodeString & HelpKeyword) :
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.IsEmpty())
  {
    FMoreMessages = new TStringList();
    FMoreMessages->SetText(MoreMessages);
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString & Msg, TStrings * MoreMessages,
  bool Own, const UnicodeString & HelpKeyword) :
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
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
void ExtException::AddMoreMessages(const Exception * E)
{
  if (E != nullptr)
  {
    if (FMoreMessages == nullptr)
    {
      FMoreMessages = new TStringList();
    }

    const ExtException * ExtE = dynamic_cast<const ExtException *>(E);
    if (ExtE != nullptr)
    {
      if (!ExtE->GetHelpKeyword().IsEmpty())
      {
        // we have to yet decide what to do now
        assert(GetHelpKeyword().IsEmpty());

        FHelpKeyword = ExtE->GetHelpKeyword();
      }

      if (ExtE->GetMoreMessages() != nullptr)
      {
        FMoreMessages->Assign(ExtE->GetMoreMessages());
      }
    }

    UnicodeString Msg;
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

    if (FMoreMessages->GetCount() == 0)
    {
      delete FMoreMessages;
      FMoreMessages = nullptr;
    }
  }
}
//---------------------------------------------------------------------------
ExtException::~ExtException() throw()
{
  delete FMoreMessages;
  FMoreMessages = nullptr;
}
//---------------------------------------------------------------------------
ExtException * ExtException::Clone()
{
  return new ExtException(this, L"");
}
//---------------------------------------------------------------------------
UnicodeString LastSysErrorMessage()
{
  DWORD LastError = GetLastError();
  UnicodeString Result;
  if (LastError != 0)
  {
    Result = FORMAT(L"System Error.  Code: %d.\r\n%s", LastError, SysErrorMessage(LastError).c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
EOSExtException::EOSExtException(const UnicodeString & Msg) :
  ExtException(Msg, LastSysErrorMessage())
{
}
//---------------------------------------------------------------------------
EFatal::EFatal(Exception * E, const UnicodeString & Msg) :
  ExtException(Msg, E),
  FReopenQueried(false)
{
  EFatal * F = dynamic_cast<EFatal *>(E);
  if (F != nullptr)
  {
    FReopenQueried = F->GetReopenQueried();
  }
}
//---------------------------------------------------------------------------
ExtException * EFatal::Clone()
{
  return new EFatal(this, L"");
}
//---------------------------------------------------------------------------
ExtException * ESshTerminate::Clone()
{
  return new ESshTerminate(this, L"", Operation);
}
//---------------------------------------------------------------------------
ECallbackGuardAbort::ECallbackGuardAbort() : EAbort(L"callback abort")
{
}
//---------------------------------------------------------------------------
Exception * CloneException(Exception * E)
{
  ExtException * Ext = dynamic_cast<ExtException *>(E);
  if (Ext != nullptr)
  {
    return Ext->Clone();
  }
  else if (dynamic_cast<ECallbackGuardAbort *>(E) != nullptr)
  {
    return new ECallbackGuardAbort();
  }
  else if (dynamic_cast<EAbort *>(E) != nullptr)
  {
    return new EAbort(E->Message);
  }
  else
  {
    return new Exception(E->Message);
  }
}
//---------------------------------------------------------------------------
void RethrowException(Exception * E)
{
  if (dynamic_cast<EFatal *>(E) != nullptr)
  {
    throw EFatal(E, L"");
  }
  else if (dynamic_cast<ECallbackGuardAbort *>(E) != nullptr)
  {
    throw ECallbackGuardAbort();
  }
  else if (dynamic_cast<EAbort *>(E) != nullptr)
  {
    throw EAbort(E->Message);
  }
  else
  {
    throw ExtException(E, L"");
  }
}
