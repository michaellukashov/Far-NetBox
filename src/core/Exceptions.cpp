//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"
#endif

#include "Common.h"
#include "Exceptions.h"
#include "Common.h"
#include "TextsCore.h"
#include "Terminal.h"

//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(Exception * E, UnicodeString & Message)
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
  else if (E->GetMessage().IsEmpty())
  {
    Result = false;
  }
  else
  {
    Message = E->GetMessage();
  }
  /*else
  {
    Message = MB2W(E->what());
  }*/
  return Result;
}
//---------------------------------------------------------------------------
TStrings * ExceptionToMoreMessages(Exception * E)
{
  TStrings * Result = NULL;
  UnicodeString Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * ExtE = dynamic_cast<ExtException *>(E);
    if (ExtE != NULL)
    {
      Result->AddStrings(ExtE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(Exception * E) :
  parent(L""),
  FMoreMessages(NULL)
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(UnicodeString Msg) :
  parent(Msg),
  FMoreMessages(NULL)
{
  // append message to the end to more messages
  if (!Msg.IsEmpty())
  {
    if (GetMessage().IsEmpty())
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
/* __fastcall */ ExtException::ExtException(UnicodeString Msg, Exception * E) :
  parent(Msg),
  FMoreMessages(NULL)
{
  // "copy std::exception"
  AddMoreMessages(E);
  // and append message to the end to more messages
  if (!Msg.IsEmpty())
  {
    if (GetMessage().IsEmpty())
    {
      SetMessage(Msg);
    }
    else
    {
      if (FMoreMessages == NULL)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(GetMessage());
    }
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(UnicodeString Msg, TStrings * MoreMessages,
  bool Own) :
  parent(Msg),
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
/* __fastcall */ ExtException::ExtException(ExtException & E) throw() :
  parent(E),
  FMoreMessages(NULL)
{
  AddMoreMessages(&E);
}

ExtException & /* __fastcall */ ExtException::operator =(const ExtException & E) throw()
{
  AddMoreMessages(&E);
  return *this;
}

//---------------------------------------------------------------------------
void __fastcall ExtException::AddMoreMessages(const Exception* E)
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
      if (!ExtE->GetHelpKeyword().IsEmpty())
      {
        // we have to yet decide what to do now
        assert(GetHelpKeyword().IsEmpty());

        FHelpKeyword = ExtE->GetHelpKeyword();
      }

      if (ExtE->GetMoreMessages() != NULL)
      {
        FMoreMessages->Assign(ExtE->GetMoreMessages());
      }
    }

    UnicodeString Msg;
    ExceptionMessage(E, Msg);

    // new exception does not have own message, this is in fact duplication of
    // the exception data, but the exception class may being changed
    if (GetMessage().IsEmpty())
    {
      SetMessage(Msg);
    }
    else if (!Msg.IsEmpty())
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
__fastcall ExtException::~ExtException()
{
  delete FMoreMessages;
  FMoreMessages = NULL;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall LastSysErrorMessage()
{
  int LastError = GetLastError();
  UnicodeString Result;
  if (LastError != 0)
  {
    Result = FORMAT(L"System Error.  Code: %d.\r\n%s", LastError, SysErrorMessage(LastError).c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
/*
__fastcall EOSExtException::EOSExtException(UnicodeString Msg) :
  parent(Msg, LastSysErrorMessage())
{
}
*/

//---------------------------------------------------------------------------
/* __fastcall */ EFatal::EFatal(UnicodeString Msg, Exception * E) :
  parent(Msg, E),
  FReopenQueried(false)
{
  EFatal * F = dynamic_cast<EFatal *>(E);
  if (F != NULL)
  {
    FReopenQueried = F->FReopenQueried;
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
Exception::Exception(Exception * E) :
  Exception(L"")
{
  // AddMoreMessages(E);
}
//---------------------------------------------------------------------------
Exception::Exception(UnicodeString Msg) :
  parent(W2MB(Msg.c_str()).c_str())
{
}
Exception::Exception(Exception & E) throw() :
  parent(E.what())
{
}
Exception::Exception(const std::exception * E) :
  parent(E->what())
{
}
//---------------------------------------------------------------------------
