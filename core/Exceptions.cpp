//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Exceptions.h"
#include "Common.h"
#include "TextsCore.h"
#include "Terminal.h"

//---------------------------------------------------------------------------
bool ExceptionMessage(std::exception * E, std::wstring & Message)
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
TStrings *ExceptionToMoreMessages(exception * E)
{
  TStrings *Result = NULL;
  std::wstring Message;
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
ExtException::ExtException(const std::wstring Msg, int AHelpContext) :
    exception(::W2MB(Msg.c_str()).c_str(), AHelpContext)
{
}
//---------------------------------------------------------------------------
ExtException::ExtException(exception * E) :
  exception("")
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(exception* E, std::wstring Msg):
  std::exception(::W2MB(Msg.c_str()).c_str())
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(std::wstring Msg) :
  exception("")
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
ExtException::ExtException(std::wstring Msg, exception* E) :
  exception("")
{
  // "copy exception"
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
  std::exception(::W2MB(Msg.c_str()).c_str()),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.empty())
  {
    FMoreMessages = new TStringList();
    FMoreMessages->SetText(MoreMessages);
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(std::wstring Msg, TStrings* MoreMessages,
  bool Own) :
  exception(::W2MB(Msg.c_str()).c_str())
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

    // new exception does not have own message, this is in fact duplication of
    // the exception data, but the exception class may being changed
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
  ExtException(Msg, LastSysErrorMessage())
{
}

//---------------------------------------------------------------------------
void ShowExtendedExceptionEx(TTerminal * Terminal,
  std::exception * E)
{
/* FIXME
  std::wstring Message; // not used
  if (ExceptionMessage(E, Message))
  {
    TTerminalManager * Manager = TTerminalManager::Instance(false);

    TQueryType Type;
    ESshTerminate * Terminate = dynamic_cast<ESshTerminate*>(E);
    bool CloseOnCompletion = (Terminate != NULL);
    Type = CloseOnCompletion ? qtInformation : qtError;

    if (E->InheritsFrom(__classid(EFatal)) && (Terminal != NULL) &&
        (Manager != NULL) && (Manager->ActiveTerminal == Terminal))
    {
      if (CloseOnCompletion)
      {
        Manager->DisconnectActiveTerminal();
      }

      int SessionReopenTimeout = 0;
      TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Terminal);
      if ((ManagedTerminal != NULL) &&
          ((Configuration->SessionReopenTimeout == 0) ||
           ((double)ManagedTerminal->ReopenStart == 0) ||
           (int(double(Now() - ManagedTerminal->ReopenStart) * 24*60*60*1000) < Configuration->SessionReopenTimeout)))
      {
        SessionReopenTimeout = GUIConfiguration->SessionReopenAutoIdle;
      }

      int Result;
      if (CloseOnCompletion)
      {
        if (WinConfiguration->ConfirmExitOnCompletion)
        {
          TMessageParams Params(mpNeverAskAgainCheck);
          Result = FatalExceptionMessageDialog(E, Type, 0,
            (Manager->GetCount() > 1) ?
              FMTLOAD(DISCONNECT_ON_COMPLETION, Manager->GetCount() - 1) :
              LoadStr(EXIT_ON_COMPLETION),
            qaYes | qaNo, HELP_NONE, &Params);

          if (Result == qaNeverAskAgain)
          {
            Result = qaYes;
            WinConfiguration->ConfirmExitOnCompletion = false;
          }
        }
        else
        {
          Result = qaYes;
        }
      }
      else
      {
        Result = FatalExceptionMessageDialog(E, Type, SessionReopenTimeout);
      }

      if (Result == qaYes)
      {
        assert(Terminate != NULL);
        assert(Terminate->Operation != odoIdle);
        Application->Terminate();

        switch (Terminate->Operation)
        {
          case odoDisconnect:
            break;

          case odoShutDown:
            ShutDownWindows();
            break;

          default:
            assert(false);
        }
      }
      else if (Result == qaRetry)
      {
        Manager->ReconnectActiveTerminal();
      }
      else
      {
        Manager->FreeActiveTerminal();
      }
    }
    else
    {
      if (CloseOnCompletion)
      {
        if (WinConfiguration->ConfirmExitOnCompletion)
        {
          TMessageParams Params(mpNeverAskAgainCheck);
          if (ExceptionMessageDialog(E, Type, "", qaOK, HELP_NONE, &Params) ==
                qaNeverAskAgain)
          {
            WinConfiguration->ConfirmExitOnCompletion = false;
          }
        }
      }
      else
      {
        ExceptionMessageDialog(E, Type);
      }
    }
  }
  */
}

//---------------------------------------------------------------------------
// void ShowExtendedException(std::exception * E)
// {
  // ShowExtendedExceptionEx(NULL, E);
// }
