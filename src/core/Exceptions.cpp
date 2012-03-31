#include "stdafx.h"

#include "Exceptions.h"
#include "Common.h"
#include "TextsCore.h"
#include "Terminal.h"

//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(const std::exception *E, std::wstring &Message)
{
    bool Result = true;
    if (dynamic_cast<const nb::EAbort *>(E) != NULL)
    {
        Result = false;
    }
    else if (dynamic_cast<const nb::EAccessViolation *>(E) != NULL)
    {
        Message = LoadStr(ACCESS_VIOLATION_ERROR);
    }
    else if (std::string(E->what()).empty())
    {
        Result = false;
    }
    else
    {
        Message = nb::MB2W(E->what());
    }
    return Result;
}
//---------------------------------------------------------------------------
nb::TStrings * __fastcall ExceptionToMoreMessages(const std::exception *E)
{
    nb::TStrings *Result = NULL;
    std::wstring Message;
    if (ExceptionMessage(E, Message))
    {
        Result = new nb::TStringList();
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
ExtException::ExtException(const std::exception *E) :
    parent(""),
    FMoreMessages(NULL)
{
    AddMoreMessages(E);
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}
//---------------------------------------------------------------------------
ExtException::ExtException(const std::wstring Msg) :
    parent(nb::W2MB(Msg.c_str()).c_str()),
    FMoreMessages(NULL)
{
    // append message to the end to more messages
    // DEBUG_PRINTF(L"begin");
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
                FMoreMessages = new nb::TStringList();
            }
            FMoreMessages->Append(Msg);
        }
    }
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}

//---------------------------------------------------------------------------
ExtException::ExtException(const std::wstring Msg, const std::exception *E) :
    parent(nb::W2MB(Msg.c_str()).c_str()),
    FMoreMessages(NULL)
{
    DEBUG_PRINTF(L"Msg = %s, E = %x", Msg.c_str(), E);
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
                FMoreMessages = new nb::TStringList();
            }
            FMoreMessages->Append(GetMessage());
        }
    }
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}
//---------------------------------------------------------------------------
ExtException::ExtException(const std::wstring Msg, nb::TStrings *MoreMessages,
                           bool Own) :
    parent(nb::W2MB(Msg.c_str()).c_str()),
    FMoreMessages(NULL)
{
    if (Own)
    {
        FMoreMessages = MoreMessages;
    }
    else
    {
        FMoreMessages = new nb::TStringList();
        FMoreMessages->Assign(MoreMessages);
    }
    DEBUG_PRINTF(L"FMessage = %s", FMessage.c_str());
}
//---------------------------------------------------------------------------
ExtException::ExtException(const ExtException &E) throw() :
    parent(E.what()),
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
            FMoreMessages = new nb::TStringList();
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
    FMoreMessages = NULL;
}
//---------------------------------------------------------------------------
std::wstring __fastcall LastSysErrorMessage()
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
/*
EOSExtException::EOSExtException(const std::wstring Msg) :
  parent(Msg, LastSysErrorMessage())
{
}
*/

//---------------------------------------------------------------------------
EFatal::EFatal(const std::wstring Msg, const std::exception *E) :
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
