#include "stdafx.h"

#include "Exceptions.h"
#include "Common.h"
#include "TextsCore.h"
#include "Terminal.h"

//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(const std::exception *E, UnicodeString &Message)
{
    bool Result = true;
    if (dynamic_cast<const System::EAbort *>(E) != NULL)
    {
        Result = false;
    }
    else if (dynamic_cast<const System::EAccessViolation *>(E) != NULL)
    {
        Message = LoadStr(ACCESS_VIOLATION_ERROR);
    }
    else if (std::string(E->what()).IsEmpty())
    {
        Result = false;
    }
    else
    {
        Message = System::MB2W(E->what());
    }
    return Result;
}
//---------------------------------------------------------------------------
System::TStrings * __fastcall ExceptionToMoreMessages(const std::exception *E)
{
    System::TStrings *Result = NULL;
    UnicodeString Message;
    if (ExceptionMessage(E, Message))
    {
        Result = new System::TStringList();
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
Exception::Exception(const UnicodeString Msg) :
    parent(System::W2MB(Msg.c_str()).c_str())
{
}
Exception::Exception(const Exception &E) throw() :
    parent(E.what())
{
}
Exception::Exception(const std::exception *E) :
    parent(E->what())
{
}
//---------------------------------------------------------------------------
ExtException::ExtException(const std::exception *E) :
    parent(L""),
    FMoreMessages(NULL)
{
    AddMoreMessages(E);
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString Msg) :
    parent(Msg),
    FMoreMessages(NULL)
{
    // append message to the end to more messages
    // DEBUG_PRINTF(L"begin");
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
                FMoreMessages = new System::TStringList();
            }
            FMoreMessages->Append(Msg);
        }
    }
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}

//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString Msg, const std::exception *E) :
    parent(Msg),
    FMoreMessages(NULL)
{
    DEBUG_PRINTF(L"Msg = %s, E = %x", Msg.c_str(), E);
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
                FMoreMessages = new System::TStringList();
            }
            FMoreMessages->Append(GetMessage());
        }
    }
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString Msg, System::TStrings *MoreMessages,
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
        FMoreMessages = new System::TStringList();
        FMoreMessages->Assign(MoreMessages);
    }
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}
//---------------------------------------------------------------------------
ExtException::ExtException(const ExtException &E) throw() :
    parent(E),
    FMoreMessages(NULL)
{
    AddMoreMessages(&E);
}

ExtException &ExtException::operator =(const ExtException &E) throw()
{
    AddMoreMessages(&E);
    return *this;
}

//---------------------------------------------------------------------------
void __fastcall ExtException::AddMoreMessages(const std::exception *E)
{
    if (E != NULL)
    {
        if (FMoreMessages == NULL)
        {
            FMoreMessages = new System::TStringList();
        }

        const ExtException *ExtE = dynamic_cast<const ExtException *>(E);
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

        // new std::exception does not have own message, this is in fact duplication of
        // the std::exception data, but the std::exception class may being changed
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
ExtException::~ExtException()
{
    DEBUG_PRINTF(L"~ExtException");
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
EOSExtException::EOSExtException(const UnicodeString Msg) :
  parent(Msg, LastSysErrorMessage())
{
}
*/

//---------------------------------------------------------------------------
EFatal::EFatal(const UnicodeString Msg, const std::exception *E) :
    parent(Msg, E),
    FReopenQueried(false)
{
    const EFatal *F = dynamic_cast<const EFatal *>(E);
    if (F != NULL)
    {
        FReopenQueried = F->FReopenQueried;
    }
}
//---------------------------------------------------------------------------
