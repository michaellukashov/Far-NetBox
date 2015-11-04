#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <TextsCore.h>
#include <Exceptions.h>
#include <FileMasks.h>
#include <CoreMain.h>
#include <RemoteFiles.h>
#include <Interface.h>

#include "GUITools.h"
#include "PuttyTools.h"
#include "Tools.h"

void * BusyStart()
{
  void * Token = nullptr; // ToPtr(Screen->Cursor);
  // Screen->Cursor = crHourGlass;
  return Token;
}

void BusyEnd(void * /*Token*/)
{
  // Screen->Cursor = reinterpret_cast<TCursor>(Token);
}

static DWORD MainThread = 0;
static TDateTime LastGUIUpdate(0.0);
static double GUIUpdateIntervalFrac = static_cast<double>(MSecsPerSec / 1000 * GUIUpdateInterval);  // 1/5 sec

bool ProcessGUI(bool Force)
{
  assert(MainThread != 0);
  bool Result = false;
  if (MainThread == ::GetCurrentThreadId())
  {
    TDateTime N = Now();
    if (Force ||
        (double(N) - double(LastGUIUpdate) > GUIUpdateIntervalFrac))
    {
      LastGUIUpdate = N;
//      Application->ProcessMessages();
      Result = true;
    }
  }
  return Result;
}

TOptions * GetGlobalOptions()
{
  return nullptr; // TProgramParams::Instance();
}

void WinInitialize()
{
//  if (JclHookExceptions())
//  {
//    JclStackTrackingOptions << stAllModules;
//    JclAddExceptNotifier(DoExceptNotify, npFirstChain);
//  }

  SetErrorMode(SEM_FAILCRITICALERRORS);
//  OnApiPath = ::ApiPath;
  MainThread = GetCurrentThreadId();

}
//---------------------------------------------------------------------------
void WinFinalize()
{
  // JclRemoveExceptNotifier(DoExceptNotify);
}

//---------------------------------------------------------------------------
static void DoVerifyKey(
  const UnicodeString & AFileName, bool TypeOnly, TSshProt SshProt)
{
  if (!AFileName.Trim().IsEmpty())
  {
    TRACE("1");
    UnicodeString FileName = ExpandEnvironmentVariables(AFileName);
    TKeyType Type = GetKeyType(FileName);
    // reason _wfopen failed
    int Error = errno;
    UnicodeString Message;
    UnicodeString HelpKeyword = HELP_LOGIN_KEY_TYPE;
    UnicodeString PuttygenPath;
    std::unique_ptr<TStrings> MoreMessages;
    bool TryPuttygen = false;
    switch (Type)
    {
      case ktOpenSSH:
      case ktSSHCom:
        TRACE("2");
        {
          UnicodeString TypeName = (Type == ktOpenSSH) ? L"OpenSSH SSH-2" : L"ssh.com SSH-2";
          TryPuttygen = FindTool(PuttygenTool, PuttygenPath);
          Message = FMTLOAD(KEY_TYPE_UNSUPPORTED, (AFileName, TypeName));
          if (TryPuttygen)
          {
            TRACE("3");
            Message = FMTLOAD(KEY_TYPE_CONVERT2, (TypeName, RemoveMainInstructionsTag(Message)));
          }
        }
        HelpKeyword = HELP_KEY_TYPE_UNSUPPORTED;
        break;

      case ktSSH1:
      case ktSSH2:
        TRACE("4");
        // on file select do not check for SSH version as user may
        // intend to change it only after he/she selects key file
        if (!TypeOnly)
        {
          TRACE("5");
          if ((Type == ktSSH1) !=
                ((SshProt == ssh1only) || (SshProt == ssh1)))
          {
            TRACE("6");
            Message =
              MainInstructions(
                FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
                  (AFileName, (Type == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2"))));
          }
        }
        break;

      case ktUnopenable:
        TRACE("8");
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNOPENABLE, (AFileName)));
        if (Error != ERROR_SUCCESS)
        {
          MoreMessages.reset(TextToStringList(SysErrorMessageForError(Error)));
        }
        break;

      default:
        TRACE("7");
        FAIL;
        // fallthru
      case ktUnknown:
        TRACE("9");
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNKNOWN2, (AFileName)));
        break;
    }

    if (!Message.IsEmpty())
    {
      if (TryPuttygen)
      {
        TRACE("10");
        Configuration->Usage->Inc(L"PrivateKeyConvertSuggestions");
        if (MoreMessageDialog(Message, MoreMessages.get(), qtConfirmation, qaOK | qaCancel, HelpKeyword) == qaOK)
        {
          TRACE("11");
          if (!ExecuteShell(PuttygenPath, AddPathQuotes(AFileName)))
          {
            throw Exception(FMTLOAD(EXECUTE_APP_ERROR, (PuttygenPath)));
          }
        }
        Abort();
      }
      else
      {
        TRACE("12");
        Configuration->Usage->Inc(L"PrivateKeySelectErrors");
        if (MoreMessageDialog(Message, MoreMessages.get(), qtWarning, qaIgnore | qaAbort,
             HelpKeyword) == qaAbort)
        {
          TRACE("13");
          Abort();
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void VerifyKey(const UnicodeString & AFileName)
{
  DoVerifyKey(AFileName, true, TSshProt(0));
}
//---------------------------------------------------------------------------
void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt)
{
  DoVerifyKey(AFileName, false, SshProt);
}
//---------------------------------------------------------------------------
void VerifyCertificate(const UnicodeString & AFileName)
{
  CALLSTACK;
  if (!AFileName.Trim().IsEmpty())
  {
    try
    {
      CheckCertificate(AFileName);
    }
    catch (Exception & E)
    {
      if (ExceptionMessageDialog(&E, qtWarning, L"", qaIgnore | qaAbort) == qaAbort)
      {
        Abort();
      }
    }
  }
}
