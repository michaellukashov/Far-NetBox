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
  DebugAssert(MainThread != 0);
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

void WinFinalize()
{
  // JclRemoveExceptNotifier(DoExceptNotify);
}


static void ConvertKey(UnicodeString & FileName, TKeyType Type)
{
  UnicodeString Passphrase;

  UnicodeString Comment;
  if (IsKeyEncrypted(Type, FileName, Comment))
  {
//    TODO: implement
//    if (!InputDialog(
//          LoadStr(PASSPHRASE_TITLE),
//          FORMAT(LoadStr(PROMPT_KEY_PASSPHRASE).c_str(), Comment.c_str()),
//          Passphrase, HELP_NONE, nullptr, false, nullptr, false))
    {
      Abort();
    }
  }

  TPrivateKey * PrivateKey = LoadKey(Type, FileName, Passphrase);

  // try
  {
    SCOPE_EXIT
    {
      FreeKey(PrivateKey);
    };
    FileName = ChangeFileExt(FileName, ".ppk");

//    TODO: implement
//    if (!SaveDialog(LoadStr(CONVERTKEY_SAVE_TITLE), LoadStr(CONVERTKEY_SAVE_FILTER), L"ppk", FileName))
    {
      Abort();
    }

    SaveKey(ktSSH2, FileName, Passphrase, PrivateKey);

//    TODO: implement
//    MessageDialog(MainInstructions(FMTLOAD(CONVERTKEY_SAVED, FileName.c_str())), qtInformation, qaOK);
  }
  __finally
  {
    FreeKey(PrivateKey);
  };
}

static void DoVerifyKey(
  const UnicodeString & AFileName, bool TypeOnly, TSshProt SshProt, bool Convert)
{
  if (!AFileName.Trim().IsEmpty())
  {
    UnicodeString FileName = ExpandEnvironmentVariables(AFileName);
    TKeyType Type = GetKeyType(FileName);
    // reason _wfopen failed
    int Error = errno;
    UnicodeString Message;
    UnicodeString HelpKeyword; // = HELP_LOGIN_KEY_TYPE;
    UnicodeString PuttygenPath;
    std::unique_ptr<TStrings> MoreMessages;
    switch (Type)
    {
      case ktOpenSSHAuto:
      case ktSSHCom:
        {
          UnicodeString TypeName = (Type == ktOpenSSHAuto) ? L"OpenSSH SSH-2" : L"ssh.com SSH-2";
          Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName.c_str(), TypeName.c_str());

          if (Convert)
          {
            // Configuration->Usage->Inc(L"PrivateKeyConvertSuggestionsNative");
            UnicodeString ConvertMessage = FMTLOAD(KEY_TYPE_CONVERT3, TypeName.c_str(), RemoveMainInstructionsTag(Message).c_str());
            Message = UnicodeString();
//          TODO: implement
//          if (MoreMessageDialog(ConvertMessage, nullptr, qtConfirmation, qaOK | qaCancel, HelpKeyword) == qaOK)
//            {
//              ConvertKey(FileName, Type);
//              // Configuration->Usage->Inc(L"PrivateKeyConverted");
//            }
//            else
            {
              Abort();
            }
          }
          else
          {
            HelpKeyword = ""; // HELP_KEY_TYPE_UNSUPPORTED;
          }
        }
        break;

      case ktSSH1:
      case ktSSH2:
        // on file select do not check for SSH version as user may
        // intend to change it only after he/she selects key file
        if (!TypeOnly)
        {
          if ((Type == ktSSH1) !=
                ((SshProt == ssh1only) || (SshProt == ssh1)))
          {
            Message =
              MainInstructions(
                FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
                  (AFileName, (Type == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2"))));
          }
        }
        break;

      case ktUnopenable:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNOPENABLE, AFileName.c_str()));
        if (Error != ERROR_SUCCESS)
        {
          MoreMessages.reset(TextToStringList(SysErrorMessageForError(Error)));
        }
        break;

      default:
        DebugFail();
        // fallthru
      case ktUnknown:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNKNOWN2, AFileName.c_str()));
        break;
    }

    if (!Message.IsEmpty())
    {
      // Configuration->Usage->Inc(L"PrivateKeySelectErrors");
//      TODO: implement
//      if (MoreMessageDialog(Message, MoreMessages.get(), qtWarning, qaIgnore | qaAbort,
//           HelpKeyword) == qaAbort)
      {
        Abort();
      }
    }
  }
}

void VerifyAndConvertKey(UnicodeString & FileName)
{
  DoVerifyKey(FileName, true, TSshProt(0), true);
}

void VerifyKey(const UnicodeString & AFileName)
{
  DoVerifyKey(AFileName, true, TSshProt(0), false);
}

void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt)
{
  DoVerifyKey(AFileName, false, SshProt, false);
}

void VerifyCertificate(const UnicodeString & AFileName)
{
  if (!AFileName.Trim().IsEmpty())
  {
    try
    {
      CheckCertificate(AFileName);
    }
    catch (Exception & E)
    {
//      TODO: implement
//      if (ExceptionMessageDialog(&E, qtWarning, L"", qaIgnore | qaAbort) == qaAbort)
      {
        Abort();
      }
    }
  }
}
