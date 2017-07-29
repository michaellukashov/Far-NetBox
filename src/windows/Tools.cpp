#include <vcl.h>
#pragma hdrstop

#include <winhttp.h>
#include <Common.h>
#include <TextsCore.h>
#include <Exceptions.h>
//#include <FileMasks.h>
//#include <CoreMain.h>
#include <RemoteFiles.h>
#include <Interface.h>
#include <LibraryLoader.hpp>

#include "WinInterface.h"
//#include "GUITools.h"
#include "PuttyTools.h"
#include "Tools.h"

TOptions * GetGlobalOptions()
{
  return nullptr; // TProgramParams::Instance();
}

bool SaveDialog(const UnicodeString & ATitle, const UnicodeString & Filter,
  const UnicodeString & ADefaultExt, UnicodeString & AFileName)
{
  bool Result = false;
  DebugUsedParam(Filter);
  DebugUsedParam(ADefaultExt);

  Result = InputDialog(ATitle, L""/*LoadStr(LOGIN_PRIVATE_KEY)*/, AFileName, L"", nullptr, true, nullptr, true);
  return Result;
}

#if 0
// implemented in FarInterface.cpp
void CopyToClipboard(const UnicodeString & Text)
{
  HANDLE Data;
  void * DataPtr;

  if (OpenClipboard(0))
  {
    try__finally
    {
      SCOPE_EXIT
      {
        CloseClipboard();
      };
      size_t Size = (Text.Length() + 1) * sizeof(wchar_t);
      Data = GlobalAlloc(GMEM_MOVEABLE + GMEM_DDESHARE, Size);
      try
      {
        SCOPE_EXIT
        {
          GlobalUnlock(Data);
        };
        DataPtr = GlobalLock(Data);
        try__finally
        {
          SCOPE_EXIT
          {
            GlobalUnlock(Data);
          };
          memcpy(DataPtr, Text.c_str(), Size);
          EmptyClipboard();
          SetClipboardData(CF_UNICODETEXT, Data);
        }
        __finally
        {
          GlobalUnlock(Data);
        };
      }
      catch (...)
      {
        GlobalFree(Data);
        throw;
      }
    }
    __finally
    {
      CloseClipboard();
    };
  }
  else
  {
    throw Exception(SCannotOpenClipboard);
  }
}
#endif

void CopyToClipboard(TStrings * Strings)
{
  if (Strings->GetCount() > 0)
  {
    CopyToClipboard(StringsToText(Strings));
  }
}

bool IsWin64()
{
  static int Result = -1;
  if (Result < 0)
  {
    Result = 0;
    BOOL Wow64Process = FALSE;
    if (::IsWow64Process(::GetCurrentProcess(), &Wow64Process))
    {
      if (Wow64Process)
      {
        Result = 1;
      }
    }
  }

  return (Result > 0);
}

static void ConvertKey(UnicodeString & FileName, TKeyType Type)
{
  UnicodeString Passphrase;

  UnicodeString Comment;
  if (IsKeyEncrypted(Type, FileName, Comment))
  {
    if (!InputDialog(
      LoadStr(PASSPHRASE_TITLE),
      FORMAT(LoadStr(PROMPT_KEY_PASSPHRASE), Comment),
      Passphrase, HELP_NONE, nullptr, false, nullptr, false))
    {
      Abort();
    }
  }

  TPrivateKey * PrivateKey = LoadKey(Type, FileName, Passphrase);

  try__finally
  {
    SCOPE_EXIT
    {
      FreeKey(PrivateKey);
    };
    FileName = ::ChangeFileExt(FileName, ".ppk", L'\\');

    if (!SaveDialog(LoadStr(CONVERTKEY_SAVE_TITLE), LoadStr(CONVERTKEY_SAVE_FILTER), L"ppk", FileName))
    {
      Abort();
    }

    SaveKey(ktSSH2, FileName, Passphrase, PrivateKey);

    MessageDialog(MainInstructions(FMTLOAD(CONVERTKEY_SAVED, FileName)), qtInformation, qaOK);
  }
  __finally
  {
/*
    FreeKey(PrivateKey);
*/
  };
}

static bool DoVerifyKey(
  const UnicodeString & AFileName, bool TypeOnly, TSshProt SshProt, bool Convert)
{
  bool Result = true;
  if (!AFileName.Trim().IsEmpty())
  {
    UnicodeString FileName = ::ExpandEnvironmentVariables(AFileName);
    TKeyType Type = GetKeyType(FileName);
    // reason _wfopen failed
    int Error = errno;
    UnicodeString Message;
    UnicodeString HelpKeyword; // = HELP_LOGIN_KEY_TYPE;
    UnicodeString PuttygenPath;
    std::unique_ptr<TStrings> MoreMessages;
    switch (Type)
    {
      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        {
          UnicodeString TypeName = ((Type == ktOpenSSHPEM) || (Type == ktOpenSSHNew)) ? L"OpenSSH" : L"ssh.com";
          Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName, TypeName);

          if (Convert)
          {
            // Configuration->Usage->Inc(L"PrivateKeyConvertSuggestionsNative");
            UnicodeString ConvertMessage = FMTLOAD(KEY_TYPE_CONVERT3, TypeName, RemoveMainInstructionsTag(Message));
            Message = UnicodeString();
            if (MoreMessageDialog(ConvertMessage, nullptr, qtConfirmation, qaOK | qaCancel, HelpKeyword) == qaOK)
            {
              ConvertKey(FileName, Type);
              // Configuration->Usage->Inc(L"PrivateKeyConverted");
              Result = true;
            }
            else
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
                ((SshProt == ssh1only) || (SshProt == ssh1deprecated)))
          {
            Message =
              MainInstructions(
                FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
                  AFileName, (Type == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2")));
          }
        }
        break;

      case ktSSH1Public:
      case ktSSH2PublicRFC4716:
      case ktSSH2PublicOpenSSH:
        // noop
        // Do not even bother checking SSH protocol version
        Result = true;
        break;

      case ktUnopenable:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNOPENABLE, AFileName));
        if (Error != ERROR_SUCCESS)
        {
          MoreMessages.reset(TextToStringList(SysErrorMessageForError(Error)));
        }
        break;

      default:
        DebugFail();
        // fallthru
      case ktUnknown:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNKNOWN2, AFileName));
        break;
    }

    if (!Message.IsEmpty())
    {
      // Configuration->Usage->Inc(L"PrivateKeySelectErrors");
      if (MoreMessageDialog(Message, MoreMessages.get(), qtWarning, qaIgnore | qaAbort, HelpKeyword) == qaAbort)
      {
        Abort();
      }
      Result = true;
    }
  }
  return Result;
}

bool VerifyAndConvertKey(const UnicodeString & AFileName, bool TypeOnly)
{
  return DoVerifyKey(AFileName, TypeOnly, TSshProt(0), true);
}

bool VerifyKey(const UnicodeString & AFileName, bool TypeOnly)
{
  return DoVerifyKey(AFileName, TypeOnly, TSshProt(0), false);
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
      if (ExceptionMessageDialog(&E, qtWarning, L"", qaIgnore | qaAbort) == qaAbort)
      {
        Abort();
      }
    }
  }
}


static bool GetProxyUrlFromIE(UnicodeString & Proxy)
{
  bool Result = false;
  // Code from http://gentoo.osuosl.org/distfiles/cl331.zip/io/
  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IEProxyInfo;
  ClearStruct(IEProxyInfo);
  TLibraryLoader LibraryLoader(L"winhttp.dll", true);
  if (LibraryLoader.Loaded())
  {
    typedef BOOL (WINAPI *FWinHttpGetIEProxyConfigForCurrentUser)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *);
    FWinHttpGetIEProxyConfigForCurrentUser GetIEProxyConfig = reinterpret_cast<FWinHttpGetIEProxyConfigForCurrentUser>(
      LibraryLoader.GetProcAddress("WinHttpGetIEProxyConfigForCurrentUser"));
    if (GetIEProxyConfig && GetIEProxyConfig(&IEProxyInfo))
    {
      if (IEProxyInfo.lpszProxy != nullptr)
      {
        UnicodeString IEProxy = IEProxyInfo.lpszProxy;
        Proxy = L"";
        while (Proxy.IsEmpty() && !IEProxy.IsEmpty())
        {
          UnicodeString Str = CutToChar(IEProxy, L';', true);
          if (Str.Pos(L"=") == 0)
          {
            Proxy = Str;
          }
          else
          {
            UnicodeString Protocol = CutToChar(Str, L'=', true);
            if (SameText(Protocol, L"http"))
            {
              Proxy = Str;
            }
          }
        }

        GlobalFree(IEProxyInfo.lpszProxy);
        Result = true;
      }
      if (IEProxyInfo.lpszAutoConfigUrl != nullptr)
      {
        GlobalFree(IEProxyInfo.lpszAutoConfigUrl);
      }
      if (IEProxyInfo.lpszProxyBypass != nullptr)
      {
        GlobalFree(IEProxyInfo.lpszProxyBypass);
      }
    }
  }
  return Result;
}

bool AutodetectProxy(UnicodeString & AHostName, intptr_t & APortNumber)
{
  bool Result = false;

  /* First we try for proxy info direct from the registry if
     it's available. */
  UnicodeString Proxy;
  WINHTTP_PROXY_INFO ProxyInfo;
  //memset(&ProxyInfo, 0, sizeof(ProxyInfo));
  ClearStruct(ProxyInfo);
  // if (WinHttpGetDefaultProxyConfiguration(&ProxyInfo))
  TLibraryLoader LibraryLoader(L"winhttp.dll", true);
  if (LibraryLoader.Loaded())
  {
    typedef BOOL (WINAPI *FWinHttpGetDefaultProxyConfiguration)(WINHTTP_PROXY_INFO*);
    FWinHttpGetDefaultProxyConfiguration GetDefaultProxyConfiguration = reinterpret_cast<FWinHttpGetDefaultProxyConfiguration>(
      LibraryLoader.GetProcAddress("WinHttpGetDefaultProxyConfiguration"));
    if (GetDefaultProxyConfiguration)
    {
      if (GetDefaultProxyConfiguration(&ProxyInfo))
      {
        if (ProxyInfo.lpszProxy != nullptr)
        {
          Proxy = ProxyInfo.lpszProxy;
          GlobalFree(ProxyInfo.lpszProxy);
          Result = true;
        }
        if (ProxyInfo.lpszProxyBypass != nullptr)
        {
          GlobalFree(ProxyInfo.lpszProxyBypass);
        }
      }
    }
  }

  /* The next fallback is to get the proxy info from MSIE.  This is also
     usually much quicker than WinHttpGetProxyForUrl(), although sometimes
     it seems to fall back to that, based on the longish delay involved.
     Another issue with this is that it won't work in a service process
     that isn't impersonating an interactive user (since there isn't a
     current user), but in that case we just fall back to
     WinHttpGetProxyForUrl() */
  if (!Result)
  {
    Result = GetProxyUrlFromIE(Proxy);
  }

  if (Result)
  {
    if (Proxy.Trim().IsEmpty())
    {
      Result = false;
    }
    else
    {
      AHostName = CutToChar(Proxy, L':', true);
      APortNumber = StrToIntDef(Proxy, ProxyPortNumber);
    }
  }

  // We can also use WinHttpGetProxyForUrl, but it is lengthy
  // See the source address of the code for example

  return Result;
}
