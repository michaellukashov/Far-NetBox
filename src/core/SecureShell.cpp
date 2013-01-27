//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define TRACE_TRANSMIT TRACING

#include "PuttyIntf.h"
#include "Exceptions.h"
#include "Interface.h"
#include "SecureShell.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "Common.h"
#include "CoreMain.h"

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define MAX_BUFSIZE 32768
//---------------------------------------------------------------------------
struct TPuttyTranslation
{
  const wchar_t * Original;
  int Translation;
};
//---------------------------------------------------------------------------
char * AnsiStrNew(const wchar_t * S)
{
  AnsiString Buf = S;
  char * Result = new char[Buf.Length() + 1];
  memcpy(Result, Buf.c_str(), Buf.Length() + 1);
  return Result;
}
//---------------------------------------------------------------------------
void AnsiStrDispose(char * S)
{
  delete [] S;
}
//---------------------------------------------------------------------------
TSecureShell::TSecureShell(TSessionUI* UI,
  TSessionData * SessionData, TSessionLog * Log, TConfiguration * Configuration) :
  PendLen(0)
{
  CALLSTACK;
  TRACEFMT("1 [%x]", int(this));
  FUI = UI;
  FSessionData = SessionData;
  FLog = Log;
  FConfiguration = Configuration;
  FActive = false;
  FWaiting = 0;
  FOpened = false;
  OutPtr = NULL;
  Pending = NULL;
  FBackendHandle = NULL;
  ResetConnection();
  FOnCaptureOutput = NULL;
  FOnReceive = NULL;
  FConfig = new Config();
  memset(FConfig, 0, sizeof(*FConfig));
  FSocket = INVALID_SOCKET;
  FSocketEvent = CreateEvent(NULL, false, false, NULL);
  FFrozen = false;
  FSimple = false;
}
//---------------------------------------------------------------------------
TSecureShell::~TSecureShell()
{
  CALLSTACK;
  TRACEFMT("1 [%x]", int(this));
  assert(FWaiting == 0);
  SetActive(false);
  ResetConnection();
  CloseHandle(FSocketEvent);
  ClearConfig(FConfig);
  delete FConfig;
  FConfig = NULL;
  TRACE("/");
}
//---------------------------------------------------------------------------
void TSecureShell::ResetConnection()
{
  CALLSTACK;
  FreeBackend();
  TRACE("1");
  ClearStdError();
  PendLen = 0;
  PendSize = 0;
  TRACE("2");
  sfree(Pending);
  TRACE("3");
  Pending = NULL;
  FCWriteTemp = L"";
  ResetSessionInfo();
  FAuthenticating = false;
  FAuthenticated = false;
  FStoredPasswordTried = false;
  FStoredPasswordTriedForKI = false;
  TRACE("/");
}
//---------------------------------------------------------------------------
void TSecureShell::ResetSessionInfo()
{
  FSessionInfoValid = false;
  FMinPacketSize = NULL;
  FMaxPacketSize = NULL;
}
//---------------------------------------------------------------------------
inline void TSecureShell::UpdateSessionInfo()
{
  if (!FSessionInfoValid)
  {
    FSshVersion = get_ssh_version(FBackendHandle);
    FSessionInfo.ProtocolBaseName = L"SSH";
    FSessionInfo.ProtocolName =
      FORMAT(L"%s-%d", FSessionInfo.ProtocolBaseName.c_str(), get_ssh_version(FBackendHandle));
    FSessionInfo.SecurityProtocolName = FSessionInfo.ProtocolName;

    FSessionInfo.CSCompression =
      FuncToCompression(FSshVersion, get_cscomp(FBackendHandle));
    FSessionInfo.SCCompression =
      FuncToCompression(FSshVersion, get_sccomp(FBackendHandle));

    if (FSshVersion == 1)
    {
      FSessionInfo.CSCipher = CipherNames[FuncToSsh1Cipher(get_cipher(FBackendHandle))];
      FSessionInfo.SCCipher = CipherNames[FuncToSsh1Cipher(get_cipher(FBackendHandle))];
    }
    else
    {
      FSessionInfo.CSCipher = CipherNames[FuncToSsh2Cipher(get_cscipher(FBackendHandle))];
      FSessionInfo.SCCipher = CipherNames[FuncToSsh2Cipher(get_sccipher(FBackendHandle))];
    }

    FSessionInfoValid = true;
  }
}
//---------------------------------------------------------------------------
const TSessionInfo & TSecureShell::GetSessionInfo()
{
  if (!FSessionInfoValid)
  {
    UpdateSessionInfo();
  }
  return FSessionInfo;
}
//---------------------------------------------------------------------
void TSecureShell::ClearConfig(Config * cfg)
{
  AnsiStrDispose(cfg->remote_cmd_ptr);
  AnsiStrDispose(cfg->remote_cmd_ptr2);
  // clear all
  memset(cfg, 0, sizeof(*cfg));
}
//---------------------------------------------------------------------
void TSecureShell::StoreToConfig(TSessionData * Data, Config * cfg, bool Simple)
{
  ClearConfig(cfg);

  // user-configurable settings
  ASCOPY(cfg->host, Data->GetHostNameExpanded());
  ASCOPY(cfg->username, Data->GetUserNameExpanded());
  cfg->port = Data->GetPortNumber();
  cfg->protocol = PROT_SSH;
  // always set 0, as we will handle keepalives ourselves to avoid
  // multi-threaded issues in putty timer list
  cfg->ping_interval = 0;
  cfg->compression = Data->GetCompression();
  cfg->tryagent = Data->GetTryAgent();
  cfg->agentfwd = Data->GetAgentFwd();
  cfg->addressfamily = Data->GetAddressFamily();
  ASCOPY(cfg->ssh_rekey_data, Data->GetRekeyData());
  cfg->ssh_rekey_time = Data->GetRekeyTime();

  for (int c = 0; c < CIPHER_COUNT; c++)
  {
    int pcipher = 0;
    switch (Data->GetCipher(c)) {
      case cipWarn: pcipher = CIPHER_WARN; break;
      case cip3DES: pcipher = CIPHER_3DES; break;
      case cipBlowfish: pcipher = CIPHER_BLOWFISH; break;
      case cipAES: pcipher = CIPHER_AES; break;
      case cipDES: pcipher = CIPHER_DES; break;
      case cipArcfour: pcipher = CIPHER_ARCFOUR; break;
      default: assert(false);
    }
    cfg->ssh_cipherlist[c] = pcipher;
  }

  for (int k = 0; k < KEX_COUNT; k++)
  {
    int pkex = 0;
    switch (Data->GetKex(k)) {
      case kexWarn: pkex = KEX_WARN; break;
      case kexDHGroup1: pkex = KEX_DHGROUP1; break;
      case kexDHGroup14: pkex = KEX_DHGROUP14; break;
      case kexDHGEx: pkex = KEX_DHGEX; break;
      case kexRSA: pkex = KEX_RSA; break;
      default: assert(false);
    }
    cfg->ssh_kexlist[k] = pkex;
  }

  UnicodeString SPublicKeyFile = Data->GetPublicKeyFile();
  if (SPublicKeyFile.IsEmpty()) { SPublicKeyFile = Configuration->GetDefaultKeyFile(); }
  SPublicKeyFile = StripPathQuotes(ExpandEnvironmentVariables(SPublicKeyFile));
  ASCOPY(cfg->keyfile.path, SPublicKeyFile);
  cfg->sshprot = Data->GetSshProt();
  cfg->ssh2_des_cbc = Data->GetSsh2DES();
  cfg->ssh_no_userauth = Data->GetSshNoUserAuth();
  cfg->try_tis_auth = Data->GetAuthTIS();
  cfg->try_ki_auth = Data->GetAuthKI();
  cfg->try_gssapi_auth = Data->GetAuthGSSAPI();
  cfg->gssapifwd = Data->GetGSSAPIFwdTGT();
  cfg->change_username = Data->GetChangeUsername();

  cfg->proxy_type = Data->GetActualProxyMethod();
  ASCOPY(cfg->proxy_host, Data->GetProxyHost());
  cfg->proxy_port = Data->GetProxyPort();
  ASCOPY(cfg->proxy_username, Data->GetProxyUsername());
  ASCOPY(cfg->proxy_password, Data->GetProxyPassword());
  if (Data->GetProxyMethod() == pmCmd)
  {
    ASCOPY(cfg->proxy_telnet_command, Data->GetProxyLocalCommand());
  }
  else
  {
    ASCOPY(cfg->proxy_telnet_command, Data->GetProxyTelnetCommand());
  }
  cfg->proxy_dns = Data->GetProxyDNS();
  cfg->even_proxy_localhost = Data->GetProxyLocalhost();

  #pragma option push -w-eas
  // after 0.53b values were reversed, however putty still stores
  // settings to registry in same way as before
  cfg->sshbug_ignore1 = Data->GetBug(sbIgnore1);
  cfg->sshbug_plainpw1 = Data->GetBug(sbPlainPW1);
  cfg->sshbug_rsa1 = Data->GetBug(sbRSA1);
  cfg->sshbug_hmac2 = Data->GetBug(sbHMAC2);
  cfg->sshbug_derivekey2 = Data->GetBug(sbDeriveKey2);
  cfg->sshbug_rsapad2 = Data->GetBug(sbRSAPad2);
  cfg->sshbug_rekey2 = Data->GetBug(sbRekey2);
  // new after 0.53b
  cfg->sshbug_pksessid2 = Data->GetBug(sbPKSessID2);
  cfg->sshbug_maxpkt2 = Data->GetBug(sbMaxPkt2);
  cfg->sshbug_ignore2 = Data->GetBug(sbIgnore2);
  #pragma option pop

  if (!Data->GetTunnelPortFwd().IsEmpty())
  {
    assert(!Simple);
    ASCOPY(cfg->portfwd, Data->GetTunnelPortFwd());
    // when setting up a tunnel, do not open shell/sftp
    cfg->ssh_no_shell = TRUE;
  }
  else
  {
    assert(Simple);
    cfg->ssh_simple = Data->GetSshSimple() && Simple;

    if (Data->GetFSProtocol() == fsSCPonly)
    {
      cfg->ssh_subsys = FALSE;
      if (Data->GetShell().IsEmpty())
      {
        // Following forces Putty to open default shell
        // see ssh.c: do_ssh2_authconn() and ssh1_protocol()
        cfg->remote_cmd[0] = L'\0';
      }
      else
      {
        cfg->remote_cmd_ptr = AnsiStrNew(Data->GetShell().c_str());
      }
    }
    else
    {
      if (Data->GetSftpServer().IsEmpty())
      {
        cfg->ssh_subsys = TRUE;
        strcpy_s(cfg->remote_cmd, sizeof(cfg->remote_cmd), "sftp");
      }
      else
      {
        cfg->ssh_subsys = FALSE;
        cfg->remote_cmd_ptr = AnsiStrNew(Data->GetSftpServer().c_str());
      }

      if (Data->GetFSProtocol() != fsSFTPonly)
      {
        cfg->ssh_subsys2 = FALSE;
        if (Data->GetShell().IsEmpty())
        {
          // Following forces Putty to open default shell
          // see ssh.c: do_ssh2_authconn() and ssh1_protocol()
          cfg->remote_cmd_ptr2 = AnsiStrNew(L"\0");
        }
        else
        {
          cfg->remote_cmd_ptr2 = AnsiStrNew(Data->GetShell().c_str());
        }
      }

      if ((Data->GetFSProtocol() == fsSFTPonly) && Data->GetSftpServer().IsEmpty())
      {
        // see psftp_connect() from psftp.c
        cfg->ssh_subsys2 = FALSE;
        cfg->remote_cmd_ptr2 = AnsiStrNew(
          L"test -x /usr/lib/sftp-server && exec /usr/lib/sftp-server\n"
          L"test -x /usr/local/lib/sftp-server && exec /usr/local/lib/sftp-server\n"
          L"exec sftp-server");
      }
    }
  }

  cfg->connect_timeout = Data->GetTimeout() * MSecsPerSec;
  cfg->sndbuf = Data->GetSendBuf();

  // permanent settings
  cfg->nopty = TRUE;
  cfg->tcp_keepalives = 0;
  cfg->ssh_show_banner = TRUE;
  for (intptr_t Index = 0; Index < ngsslibs; ++Index)
  {
    cfg->ssh_gsslist[Index] = gsslibkeywords[Index].v;
  }
}
//---------------------------------------------------------------------------
void TSecureShell::Open()
{
  CALLSTACK;
  TRACE("0");
  FBackend = &ssh_backend;
  TRACE("0a");
  ResetConnection();

  FAuthenticating = false;
  FAuthenticated = false;

  SetActive(false);
  TRACE("0a");

  TRACE("1");
  FAuthenticationLog = L"";
  FUI->Information(LoadStr(STATUS_LOOKUPHOST), true);
  StoreToConfig(FSessionData, FConfig, GetSimple());

  char * RealHost = NULL;
  FreeBackend(); // in case we are reconnecting
  TRACEFMT("2 [%x]", int(FBackendHandle));
  const char * InitError = FBackend->init(this, &FBackendHandle, FConfig,
    const_cast<char *>(W2MB(FSessionData->GetHostNameExpanded().c_str(), FSessionData->GetCodePageAsNumber()).c_str()), FSessionData->GetPortNumber(), &RealHost, 0,
    FConfig->tcp_keepalives);
  TRACEFMT("2b [%x]", int(FBackendHandle));
  sfree(RealHost);
  if (InitError)
  {
    TRACE("3");
    PuttyFatalError(UnicodeString(InitError));
  }
  TRACE("4");
  FUI->Information(LoadStr(STATUS_CONNECT), true);
  Init();

  TRACE("5");
  CheckConnection(CONNECTION_FAILED);
  FLastDataSent = Now();

  FSessionInfo.LoginTime = Now();

  FAuthenticating = false;
  FAuthenticated = true;
  FUI->Information(LoadStr(STATUS_AUTHENTICATED), true);

  TRACE("6");
  ResetSessionInfo();

  assert(!FSessionInfo.SshImplementation.IsEmpty());
  TRACE("7");
  FOpened = true;
}
//---------------------------------------------------------------------------
void TSecureShell::Init()
{
  CALLSTACK;
  try
  {
    try
    {
      // Recent pscp checks FBackend->exitcode(FBackendHandle) in the loop
      // (see comment in putty revision 8110)
      // It seems that we do not need to do it.

      while (!get_ssh_state_session(FBackendHandle))
      {
        TRACE("1");
        if (Configuration->GetActualLogProtocol() >= 1)
        {
          LogEvent(L"Waiting for the server to continue with the initialisation");
        }
        WaitForData();
      }

      // unless this is tunnel session, it must be safe to send now
      assert(FBackend->sendok(FBackendHandle) || !FSessionData->GetTunnelPortFwd().IsEmpty());
    }
    catch(Exception & E)
    {
      if (FAuthenticating && !FAuthenticationLog.IsEmpty())
      {
        FUI->FatalError(&E, FMTLOAD(AUTHENTICATION_LOG, FAuthenticationLog.c_str()));
      }
      else
      {
        throw;
      }
    }
  }
  catch(Exception & E)
  {
    if (FAuthenticating)
    {
      FUI->FatalError(&E, LoadStr(AUTHENTICATION_FAILED));
    }
    else
    {
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void TSecureShell::PuttyLogEvent(const UnicodeString & Str)
{
  #define SERVER_VERSION_MSG L"Server version: "
  // Gross hack
  if (Str.Pos(SERVER_VERSION_MSG) == 1)
  {
    FSessionInfo.SshVersionString = Str.SubString(wcslen(SERVER_VERSION_MSG) + 1,
      Str.Length() - wcslen(SERVER_VERSION_MSG));

    const wchar_t * Ptr = wcschr(FSessionInfo.SshVersionString.c_str(), L'-');
    if (Ptr != NULL)
    {
      Ptr = wcschr(Ptr + 1, L'-');
    }
    FSessionInfo.SshImplementation = (Ptr != NULL) ? Ptr + 1 : L"";
  }
  #define FORWARDING_FAILURE_MSG L"Forwarded connection refused by server: "
  else if (Str.Pos(FORWARDING_FAILURE_MSG) == 1)
  {
    FLastTunnelError = Str.SubString(wcslen(FORWARDING_FAILURE_MSG) + 1,
      Str.Length() - wcslen(FORWARDING_FAILURE_MSG));

    static const TPuttyTranslation Translation[] = {
      { L"Administratively prohibited [%]", PFWD_TRANSL_ADMIN },
      { L"Connect failed [%]", PFWD_TRANSL_CONNECT },
    };
    TranslatePuttyMessage(Translation, LENOF(Translation), FLastTunnelError);
  }
  LogEvent(Str);
}
//---------------------------------------------------------------------------
bool TSecureShell::PromptUser(bool /*ToServer*/,
  const UnicodeString & AName, bool /*NameRequired*/,
  const UnicodeString & Instructions, bool InstructionsRequired,
  TStrings * Prompts, TStrings * Results)
{
  CALLSTACK;
  // there can be zero prompts!

  assert(Results->GetCount() == Prompts->GetCount());

  TPromptKind PromptKind;
  // beware of changing order
  static const TPuttyTranslation NameTranslation[] = {
    { L"SSH login name", USERNAME_TITLE },
    { L"SSH key passphrase", PASSPHRASE_TITLE },
    { L"SSH TIS authentication", SERVER_PROMPT_TITLE },
    { L"SSH CryptoCard authentication", SERVER_PROMPT_TITLE },
    { L"SSH server: %", SERVER_PROMPT_TITLE2 },
    { L"SSH server authentication", SERVER_PROMPT_TITLE },
    { L"SSH password", PASSWORD_TITLE },
    { L"New SSH password", NEW_PASSWORD_TITLE },
  };

  TRACE("1");
  UnicodeString Name = AName;
  int Index = TranslatePuttyMessage(NameTranslation, LENOF(NameTranslation), Name);

  const TPuttyTranslation * InstructionTranslation = NULL;
  const TPuttyTranslation * PromptTranslation = NULL;
  size_t PromptTranslationCount = 1;

  if (Index == 0) // username
  {
    TRACE("2");
    static const TPuttyTranslation UsernamePromptTranslation[] = {
      { L"login as: ", USERNAME_PROMPT2 },
    };

    PromptTranslation = UsernamePromptTranslation;
    PromptKind = pkUserName;
  }
  else if (Index == 1) // passhrase
  {
    TRACE("3");
    static const TPuttyTranslation PassphrasePromptTranslation[] = {
      { L"Passphrase for key \"%\": ", PROMPT_KEY_PASSPHRASE },
    };

    PromptTranslation = PassphrasePromptTranslation;
    PromptKind = pkPassphrase;
  }
  else if (Index == 2) // TIS
  {
    TRACE("4");
    static const TPuttyTranslation TISInstructionTranslation[] = {
      { L"Using TIS authentication.%", TIS_INSTRUCTION },
    };
    static const TPuttyTranslation TISPromptTranslation[] = {
      { L"Response: ", PROMPT_PROMPT },
    };

    InstructionTranslation = TISInstructionTranslation;
    PromptTranslation = TISPromptTranslation;
    PromptKind = pkTIS;
  }
  else if (Index == 3) // CryptoCard
  {
    TRACE("5");
    static const TPuttyTranslation CryptoCardInstructionTranslation[] = {
      { L"Using CryptoCard authentication.%", CRYPTOCARD_INSTRUCTION },
    };
    static const TPuttyTranslation CryptoCardPromptTranslation[] = {
      { L"Response: ", PROMPT_PROMPT },
    };

    InstructionTranslation = CryptoCardInstructionTranslation;
    PromptTranslation = CryptoCardPromptTranslation;
    PromptKind = pkCryptoCard;
  }
  else if ((Index == 4) || (Index == 5))
  {
    TRACE("6");
    static const TPuttyTranslation KeybInteractiveInstructionTranslation[] = {
      { L"Using keyboard-interactive authentication.%", KEYBINTER_INSTRUCTION },
    };

    InstructionTranslation = KeybInteractiveInstructionTranslation;
    PromptKind = pkKeybInteractive;
  }
  else if (Index == 6)
  {
    TRACE("7");
    assert(Prompts->GetCount() == 1);
    Prompts->Strings[0] = LoadStr(PASSWORD_PROMPT);
    PromptKind = pkPassword;
  }
  else if (Index == 7)
  {
    TRACE("8");
    static const TPuttyTranslation NewPasswordPromptTranslation[] = {
      { L"Current password (blank for previously entered password): ", NEW_PASSWORD_CURRENT_PROMPT },
      { L"Enter new password: ", NEW_PASSWORD_NEW_PROMPT },
      { L"Confirm new password: ", NEW_PASSWORD_CONFIRM_PROMPT },
    };
    PromptTranslation = NewPasswordPromptTranslation;
    PromptTranslationCount = LENOF(NewPasswordPromptTranslation);
    PromptKind = pkNewPassword;
  }
  else
  {
    PromptKind = pkPrompt;
    assert(false);
  }

  LogEvent(FORMAT(L"Prompt (%d, %s, %s, %s)", PromptKind, AName.c_str(), Instructions.c_str(), (Prompts->GetCount() > 0 ? Prompts->Strings[0].c_str() : UnicodeString(L"<no prompt>").c_str())).c_str());

  Name = Name.Trim();

  UnicodeString Instructions2 = Instructions;
  if (InstructionTranslation != NULL)
  {
    TranslatePuttyMessage(InstructionTranslation, 1, Instructions2);
  }

  // some servers add leading blank line to make the prompt look prettier
  // on terminal console
  Instructions2 = Instructions2.Trim();

  TRACEFMT("8a [%d]", Prompts->GetCount());
  for (intptr_t Index = 0; Index < Prompts->GetCount(); ++Index)
  {
    UnicodeString Prompt = Prompts->Strings[Index];
    TRACEFMT("8b [%s]", Prompt.c_str());
    if (PromptTranslation != NULL)
    {
      TranslatePuttyMessage(PromptTranslation, PromptTranslationCount, Prompt);
    }
    // some servers add leading blank line to make the prompt look prettier
    // on terminal console
    Prompts->Strings[Index] = Prompt.Trim();
  }

//!CLEANBEGIN
/*
  PromptKind = pkKeybInteractive;
  Name = "Server prompt";
  //Instructions2 = "Using keyboard-interactive authentication.";
  Instructions2 = "Using keyboard-interactive authentication.\nYour Kerberos password will expire in 16 days.";//"Using keyboard-interactive authentication.";
  Prompts = new TStringList();
*/
//  Prompts->AddObject("The challenge is '14315716'", (TObject *)true);
/*  Prompts->AddObject("&Current password:", (TObject *)false);
  Prompts->AddObject("&New password:", (TObject *)false);
  Prompts->AddObject("Confirm new &password:", (TObject *)false);*/
//  Results = new TStringList();
/*  Results->Add("");
  Results->Add("");
  Results->Add("");*/
//  Prompts->Strings[0] = "The challenge is '14315716'";
//  Prompts->Objects[0] = (TObject *)true;
//!CLEANEND
  bool Result = false;
  TRACE("9");
  if (PromptKind == pkUserName)
  {
    TRACE("10");
    if (FSessionData->GetAuthGSSAPI())
    {
      TRACE("11");
      // use empty username if no username was filled on login dialog
      // and GSSAPI auth is enabled, hence there's chance that the server can
      // deduce the username otherwise
      Results->Strings[0] = L"";
      Result = true;
    }
  }
  else if ((PromptKind == pkTIS) || (PromptKind == pkCryptoCard) ||
      (PromptKind == pkKeybInteractive))
  {
    TRACE("12");
    if (FSessionData->GetAuthKIPassword() && !FSessionData->GetPassword().IsEmpty() &&
        !FStoredPasswordTriedForKI && (Prompts->GetCount() == 1) &&
        !(Prompts->Objects[0]))
    {
      TRACE("13");
      LogEvent(L"Using stored password.");
      FUI->Information(LoadStr(AUTH_PASSWORD), false);
      Result = true;
      Results->Strings[0] = FSessionData->GetPassword();
      FStoredPasswordTriedForKI = true;
    }
    else if (Instructions2.IsEmpty() && !InstructionsRequired && (Prompts->GetCount() == 0))
    {
      TRACE("14");
      LogEvent(L"Ignoring empty SSH server authentication request");
      Result = true;
    }
  }
  else if (PromptKind == pkPassword)
  {
    TRACEFMT("15 [%d] [%d]", int(!FSessionData->GetPassword().IsEmpty()), int(!FStoredPasswordTried));
    if (!FSessionData->GetPassword().IsEmpty() && !FStoredPasswordTried)
    {
      TRACE("16");
      LogEvent(L"Using stored password.");
      FUI->Information(LoadStr(AUTH_PASSWORD), false);
      Result = true;
      Results->Strings[0] = FSessionData->GetPassword();
      FStoredPasswordTried = true;
    }
  }

  if (!Result)
  {
    TRACE("17");
    Result = FUI->PromptUser(FSessionData,
      PromptKind, Name, Instructions2, Prompts, Results);

    if (Result)
    {
      TRACE("18");
      if ((PromptKind == pkUserName) && (Prompts->GetCount() == 1))
      {
        TRACE("19");
        FUserName = Results->Strings[0];
      }
    }
  }

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
void TSecureShell::GotHostKey()
{
  // due to re-key GotHostKey() may be called again later during session
  if (!FAuthenticating && !FAuthenticated)
  {
    FAuthenticating = true;
    FUI->Information(LoadStr(STATUS_AUTHENTICATE), true);
  }
}
//---------------------------------------------------------------------------
void TSecureShell::CWrite(const char * Data, intptr_t Length)
{
  CALLSTACK;
  // some messages to stderr may indicate that something has changed with the
  // session, so reset the session info
  ResetSessionInfo();

  // We send only whole line at once, so we have to cache incoming data
  FCWriteTemp += DeleteChar(UnicodeString(Data, Length), L'\r');

  UnicodeString Line;
  // Do we have at least one complete line in std error cache?
  while (FCWriteTemp.Pos(L"\n") > 0)
  {
    UnicodeString Line = CutToChar(FCWriteTemp, L'\n', false);

    FLog->Add(llStdError, Line);

    if (FAuthenticating)
    {
      TranslateAuthenticationMessage(Line);
      FAuthenticationLog += (FAuthenticationLog.IsEmpty() ? L"" : L"\n") + Line;
    }

    FUI->Information(Line, false);
  }
}
//---------------------------------------------------------------------------
void TSecureShell::RegisterReceiveHandler(TNotifyEvent Handler)
{
  assert(FOnReceive == NULL);
  FOnReceive = Handler;
}
//---------------------------------------------------------------------------
void TSecureShell::UnregisterReceiveHandler(TNotifyEvent Handler)
{
  assert(FOnReceive == Handler);
  USEDPARAM(Handler);
  FOnReceive = NULL;
}
//---------------------------------------------------------------------------
void TSecureShell::FromBackend(bool IsStdErr, const unsigned char * Data, intptr_t Length)
{
  CCALLSTACK(TRACE_TRANSMIT);
  CheckConnection();

  CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", int(Length), int(IsStdErr));
  if (Configuration->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT(L"Received %u bytes (%d)", Length, static_cast<int>(IsStdErr)));
  }

  // Following is taken from scp.c from_backend() and modified

  if (IsStdErr)
  {
    CTRACE(TRACE_TRANSMIT, "2");
    AddStdError(UnicodeString(reinterpret_cast<const char *>(Data), Length));
  }
  else
  {
    CTRACE(TRACE_TRANSMIT, "3");
    const unsigned char *p = Data;
    intptr_t Len = Length;

    // with event-select mechanism we can now receive data even before we
    // actually expect them (OutPtr can be NULL)

    if ((OutPtr != NULL) && (OutLen > 0) && (Len > 0))
    {
      CTRACE(TRACE_TRANSMIT, "4");
      intptr_t Used = OutLen;
      if (Used > Len) { Used = Len; }
      memmove(OutPtr, p, Used);
      OutPtr += Used; OutLen -= Used;
      p += Used; Len -= Used;
    }

    if (Len > 0)
    {
      CTRACE(TRACE_TRANSMIT, "5");
      if (PendSize < PendLen + Len)
      {
        CTRACE(TRACE_TRANSMIT, "6");
        PendSize = PendLen + Len + 4096;
        Pending = static_cast<unsigned char *>
          (Pending ? srealloc(Pending, PendSize) : smalloc(PendSize));
        if (!Pending) { FatalError(L"Out of memory"); }
      }
      memmove(Pending + PendLen, p, Len);
      PendLen += Len;
    }

    if (FOnReceive != NULL)
    {
      CTRACE(TRACE_TRANSMIT, "7");
      if (!FFrozen)
      {
        CTRACE(TRACE_TRANSMIT, "8");
        FFrozen = true;
        TRY_FINALLY (
        {
          do
          {
            FDataWhileFrozen = false;
            FOnReceive(NULL);
          }
          while (FDataWhileFrozen);
        }
        ,
        {
          CTRACE(TRACE_TRANSMIT, "9");
          FFrozen = false;
        }
        );
      }
      else
      {
        CTRACE(TRACE_TRANSMIT, "10");
        FDataWhileFrozen = true;
      }
    }
  }
  CTRACE(TRACE_TRANSMIT, "/");
}
//---------------------------------------------------------------------------
bool TSecureShell::Peek(unsigned char *& Buf, intptr_t Len) const
{
  bool Result = (int(PendLen) >= Len);

  if (Result)
  {
    Buf = Pending;
  }

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TSecureShell::Receive(unsigned char * Buf, intptr_t Len)
{
  CCALLSTACK(TRACE_TRANSMIT);
  CheckConnection();
  CTRACE(TRACE_TRANSMIT, "1");

  if (Len > 0)
  {
    CTRACE(TRACE_TRANSMIT, "2");
    // Following is taken from scp.c ssh_scp_recv() and modified

    OutPtr = Buf;
    OutLen = Len;

    TRY_FINALLY (
    {
      /*
       * See if the pending-input block contains some of what we
       * need.
       */
      if (PendLen > 0)
      {
        CTRACE(TRACE_TRANSMIT, "3");
        intptr_t PendUsed = PendLen;
        if (PendUsed > OutLen)
        {
          PendUsed = OutLen;
        }
        memmove(OutPtr, Pending, PendUsed);
        memmove(Pending, Pending + PendUsed, PendLen - PendUsed);
        OutPtr += PendUsed;
        OutLen -= PendUsed;
        PendLen -= PendUsed;
        if (PendLen == 0)
        {
          PendSize = 0;
          sfree(Pending);
          Pending = NULL;
        }
        CTRACE(TRACE_TRANSMIT, "4");
      }

      while (OutLen > 0)
      {
        CTRACEFMT(TRACE_TRANSMIT, "4a [%d]", int(OutLen));
        if (Configuration->GetActualLogProtocol() >= 1)
        {
          LogEvent(FORMAT(L"Waiting for another %u bytes", static_cast<int>(OutLen)));
        }
        WaitForData();
      }

      // This seems ambiguous
      if (Len <= 0) { FatalError(LoadStr(LOST_CONNECTION)); }
      CTRACE(TRACE_TRANSMIT, "5");
    }
    ,
    {
      CTRACE(TRACE_TRANSMIT, "6");
      OutPtr = NULL;
    }
    );
  }
  CTRACEFMT(TRACE_TRANSMIT, "7 [%d] [%d]", int(Len), int(PendLen));
  if (Configuration->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT(L"Read %u bytes (%d pending)",
      static_cast<int>(Len), static_cast<int>(PendLen)));
  }
  CTRACE(TRACE_TRANSMIT, "/");
  return Len;
}
//---------------------------------------------------------------------------
UnicodeString TSecureShell::ReceiveLine()
{
  CCALLSTACK(TRACE_TRANSMIT);
  intptr_t Index = 0;
  AnsiString Line;
  Boolean EOL = False;

  do
  {
    // If there is any buffer of received chars
    if (PendLen > 0)
    {
      CTRACE(TRACE_TRANSMIT, "1");
      Index = 0;
      // Repeat until we walk thru whole buffer or reach end-of-line
      while ((Index < PendLen) && (!Index || (Pending[Index-1] != '\n')))
      {
        ++Index;
      }
      EOL = static_cast<Boolean>(Index && (Pending[Index-1] == '\n'));
      intptr_t PrevLen = Line.Length();
      Line.SetLength(PrevLen + Index);
      Receive(reinterpret_cast<unsigned char *>(const_cast<char *>(Line.c_str()) + PrevLen), Index);
    }

    // If buffer don't contain end-of-line character
    // we read one more which causes receiving new buffer of chars
    if (!EOL)
    {
      CTRACE(TRACE_TRANSMIT, "2");
      unsigned char Ch;
      Receive(&Ch, 1);
      Line += static_cast<char>(Ch);
      EOL = (static_cast<char>(Ch) == '\n');
    }
  }
  while (!EOL);

  CTRACE(TRACE_TRANSMIT, "3");
  // We don't want end-of-line character
  Line.SetLength(Line.Length()-1);

  // UnicodeString UnicodeLine = Line;
  UnicodeString UnicodeLine = ::TrimRight(MB2W(Line.c_str(), FSessionData->GetCodePageAsNumber()));
  CaptureOutput(llOutput, UnicodeLine);
  CTRACE(TRACE_TRANSMIT, "/");
  return UnicodeLine;
}
//---------------------------------------------------------------------------
void TSecureShell::SendSpecial(int Code)
{
  CCALLSTACK(TRACE_TRANSMIT);
  LogEvent(FORMAT(L"Sending special code: %d", (Code)));
  CheckConnection();
  FBackend->special(FBackendHandle, static_cast<Telnet_Special>(Code));
  CheckConnection();
  FLastDataSent = Now();
}
//---------------------------------------------------------------------------
void TSecureShell::SendEOF()
{
  CCALLSTACK(TRACE_TRANSMIT);
  SendSpecial(TS_EOF);
}
//---------------------------------------------------------------------------
unsigned int TSecureShell::TimeoutPrompt(TQueryParamsTimerEvent PoolEvent)
{
  CALLSTACK;
  FWaiting++;

  unsigned int Answer;
  TRY_FINALLY (
  {
    TQueryParams Params(qpFatalAbort | qpAllowContinueOnError | qpIgnoreAbort);
    Params.HelpKeyword = HELP_MESSAGE_HOST_IS_NOT_COMMUNICATING;
    Params.Timer = 500;
    Params.TimerEvent = PoolEvent;
    Params.TimerMessage = FMTLOAD(TIMEOUT_STILL_WAITING3, FSessionData->GetTimeout());
    Params.TimerAnswers = qaAbort;
    if (FConfiguration->GetSessionReopenAutoStall() > 0)
    {
      Params.Timeout = FConfiguration->GetSessionReopenAutoStall();
      Params.TimeoutAnswer = qaAbort;
    }
    Answer = FUI->QueryUser(FMTLOAD(CONFIRM_PROLONG_TIMEOUT3, FSessionData->GetTimeout(), FSessionData->GetTimeout()),
      NULL, qaRetry | qaAbort, &Params);
  }
  ,
  {
    FWaiting--;
  }
  );
  return Answer;
}
//---------------------------------------------------------------------------
void TSecureShell::SendBuffer(unsigned int & Result)
{
  CCALLSTACK(TRACE_TRANSMIT);
  // for comments see PoolForData
  if (!GetActive())
  {
    Result = qaRetry;
  }
  else
  {
    try
    {
      if (FBackend->sendbuffer(FBackendHandle) <= MAX_BUFSIZE)
      {
        Result = qaOK;
      }
    }
    catch(...)
    {
      Result = qaRetry;
    }
  }
}
//---------------------------------------------------------------------------
void TSecureShell::DispatchSendBuffer(int BufSize)
{
  CCALLSTACK(TRACE_TRANSMIT);
  TDateTime Start = Now();
  do
  {
    CheckConnection();
    CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", BufSize, BufSize - MAX_BUFSIZE);
    if (Configuration->GetActualLogProtocol() >= 1)
    {
      LogEvent(FORMAT(L"There are %u bytes remaining in the send buffer, "
        L"need to send at least another %u bytes",
        BufSize, BufSize - MAX_BUFSIZE));
    }
    EventSelectLoop(100, false, NULL);
    BufSize = FBackend->sendbuffer(FBackendHandle);
    CTRACEFMT(TRACE_TRANSMIT, "2 [%d]", BufSize);
    if (Configuration->GetActualLogProtocol() >= 1)
    {
      LogEvent(FORMAT(L"There are %u bytes remaining in the send buffer", BufSize));
    }

    if (Now() - Start > FSessionData->GetTimeoutDT())
    {
      LogEvent(L"Waiting for dispatching send buffer timed out, asking user what to do.");
      unsigned int Answer = TimeoutPrompt(MAKE_CALLBACK(TSecureShell::SendBuffer, this));
      switch (Answer)
      {
        case qaRetry:
          Start = Now();
          break;

        case qaOK:
          BufSize = 0;
          break;

        default:
          assert(false);
          // fallthru

        case qaAbort:
          FatalError(LoadStr(USER_TERMINATED));
          break;
      }
    }
  }
  while (BufSize > MAX_BUFSIZE);
}
//---------------------------------------------------------------------------
void TSecureShell::Send(const unsigned char * Buf, intptr_t Len)
{
  CCALLSTACK(TRACE_TRANSMIT);
  CheckConnection();
  int BufSize = FBackend->send(FBackendHandle, const_cast<char *>(reinterpret_cast<const char *>(Buf)), Len);
  CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", int(Len), int(BufSize));
  if (Configuration->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT(L"Sent %u bytes", (static_cast<int>(Len))));
    LogEvent(FORMAT(L"There are %u bytes remaining in the send buffer", BufSize));
  }
  FLastDataSent = Now();
  // among other forces receive of pending data to free the servers's send buffer
  EventSelectLoop(0, false, NULL);

  if (BufSize > MAX_BUFSIZE)
  {
    DispatchSendBuffer(BufSize);
  }
  CheckConnection();
}
//---------------------------------------------------------------------------
void TSecureShell::SendNull()
{
  CCALLSTACK(TRACE_TRANSMIT);
  LogEvent(L"Sending NULL.");
  unsigned char Null = 0;
  Send(&Null, 1);
}
//---------------------------------------------------------------------------
void TSecureShell::SendStr(const UnicodeString & Str)
{
  CheckConnection();
  std::string AnsiStr = W2MB(Str.c_str(), FSessionData->GetCodePageAsNumber());
  Send(reinterpret_cast<const unsigned char *>(AnsiStr.c_str()), (int)AnsiStr.size());
}
//---------------------------------------------------------------------------
void TSecureShell::SendLine(const UnicodeString & Line)
{
  SendStr(Line);
  Send(reinterpret_cast<const unsigned char *>("\n"), 1);
  FLog->Add(llInput, Line);
}
//---------------------------------------------------------------------------
int TSecureShell::TranslatePuttyMessage(
  const TPuttyTranslation * Translation, intptr_t Count, UnicodeString & Message) const
{
  int Result = -1;
  for (intptr_t Index = 0; Index < Count; ++Index)
  {
    const wchar_t * Original = Translation[Index].Original;
    const wchar_t * Div = wcschr(Original, L'%');
    if (Div == NULL)
    {
      if (wcscmp(Message.c_str(), Original) == 0)
      {
        Message = LoadStr(Translation[Index].Translation);
        Result = static_cast<int>(Index);
        break;
      }
    }
    else
    {
      size_t OriginalLen = wcslen(Original);
      size_t PrefixLen = Div - Original;
      size_t SuffixLen = OriginalLen - PrefixLen - 1;
      if ((static_cast<size_t>(Message.Length()) >= OriginalLen - 1) &&
          (wcsncmp(Message.c_str(), Original, PrefixLen) == 0) &&
          (wcsncmp(Message.c_str() + Message.Length() - SuffixLen, Div + 1, SuffixLen) == 0))
      {
        Message = FMTLOAD(Translation[Index].Translation,
          Message.SubString(PrefixLen + 1, Message.Length() - PrefixLen - SuffixLen).TrimRight().c_str());
        Result = static_cast<int>(Index);
        break;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int TSecureShell::TranslateAuthenticationMessage(UnicodeString & Message) const
{
  static const TPuttyTranslation Translation[] = {
    { L"Using username \"%\".", AUTH_TRANSL_USERNAME },
    { L"Using keyboard-interactive authentication.", AUTH_TRANSL_KEYB_INTER }, // not used anymore
    { L"Authenticating with public key \"%\" from agent", AUTH_TRANSL_PUBLIC_KEY_AGENT },
    { L"Authenticating with public key \"%\"", AUTH_TRANSL_PUBLIC_KEY },
    { L"Authenticated using RSA key \"%\" from agent", AUTH_TRANSL_PUBLIC_KEY_AGENT },
    { L"Wrong passphrase", AUTH_TRANSL_WRONG_PASSPHRASE },
    { L"Wrong passphrase.", AUTH_TRANSL_WRONG_PASSPHRASE },
    { L"Access denied", AUTH_TRANSL_ACCESS_DENIED },
    { L"Trying public key authentication.", AUTH_TRANSL_TRY_PUBLIC_KEY },
    { L"Server refused our public key.", AUTH_TRANSL_KEY_REFUSED },
    { L"Server refused our key", AUTH_TRANSL_KEY_REFUSED }
  };

  int Result = TranslatePuttyMessage(Translation, LENOF(Translation), Message);

  if ((Result == 2) || (Result == 3) || (Result == 4))
  {
    // Configuration->GetUsage()->Inc(L"OpenedSessionsPrivateKey");
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSecureShell::AddStdError(const UnicodeString & Str)
{
  FStdError += Str;

  intptr_t P = 0;
  UnicodeString Str2 = DeleteChar(Str, L'\r');
  // We send only whole line at once to log, so we have to cache
  // incoming std error data
  FStdErrorTemp += Str2;
  UnicodeString Line;
  // Do we have at least one complete line in std error cache?
  while ((P = FStdErrorTemp.Pos(L"\n")) > 0)
  {
    Line = FStdErrorTemp.SubString(1, P-1);
    FStdErrorTemp.Delete(1, P);
    AddStdErrorLine(Line);
  }
}
//---------------------------------------------------------------------------
void TSecureShell::AddStdErrorLine(const UnicodeString & Str)
{
  if (FAuthenticating)
  {
    FAuthenticationLog += (FAuthenticationLog.IsEmpty() ? L"" : L"\n") + Str;
  }
  if (!Str.Trim().IsEmpty())
  {
    CaptureOutput(llStdError, Str);
  }
}
//---------------------------------------------------------------------------
const UnicodeString & TSecureShell::GetStdError() const
{
  return FStdError;
}
//---------------------------------------------------------------------------
void TSecureShell::ClearStdError()
{
  // Flush std error cache
  if (!FStdErrorTemp.IsEmpty())
  {
    if (FAuthenticating)
    {
      FAuthenticationLog +=
        (FAuthenticationLog.IsEmpty() ? L"" : L"\n") + FStdErrorTemp;
    }
    CaptureOutput(llStdError, FStdErrorTemp);
    FStdErrorTemp = L"";
  }
  FStdError = L"";
}
//---------------------------------------------------------------------------
void TSecureShell::CaptureOutput(TLogLineType Type,
  const UnicodeString & Line)
{
  if (FOnCaptureOutput != NULL)
  {
    FOnCaptureOutput(Line, (Type == llStdError));
  }
  FLog->Add(Type, Line);
}
//---------------------------------------------------------------------------
int TSecureShell::TranslateErrorMessage(UnicodeString & Message) const
{
  static const TPuttyTranslation Translation[] = {
    { L"Server unexpectedly closed network connection", UNEXPECTED_CLOSE_ERROR },
    { L"Network error: Connection refused", NET_TRANSL_REFUSED },
    { L"Network error: Connection reset by peer", NET_TRANSL_RESET },
    { L"Network error: Connection timed out", NET_TRANSL_TIMEOUT },
  };

  return TranslatePuttyMessage(Translation, LENOF(Translation), Message);
}
//---------------------------------------------------------------------------
void TSecureShell::PuttyFatalError(const UnicodeString & Error)
{
  CALLSTACK;
  TRACEFMT("[%s]", Error.c_str());
  UnicodeString Error2 = Error;
  TranslateErrorMessage(Error2);

  FatalError(Error2);
}
//---------------------------------------------------------------------------
void TSecureShell::FatalError(const UnicodeString & Error)
{
  CALLSTACK;
  TRACEFMT("[%s]", Error.c_str());
  FUI->FatalError(NULL, Error);
}
//---------------------------------------------------------------------------
void TSecureShell::LogEvent(const UnicodeString & Str)
{
  TRACEFMT("[%s]", Str.c_str());
  if (FLog->GetLogging())
  {
    FLog->Add(llMessage, Str);
  }
}
//---------------------------------------------------------------------------
void TSecureShell::SocketEventSelect(SOCKET Socket, HANDLE Event, bool Startup)
{
  CCALLSTACK(TRACE_TRANSMIT);
  int Events;

  if (Startup)
  {
    Events = (FD_CONNECT | FD_READ | FD_WRITE | FD_OOB | FD_CLOSE | FD_ACCEPT);
  }
  else
  {
    Events = 0;
  }

  CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", int(Events), int(Socket));
  if (Configuration->GetActualLogProtocol() >= 2)
  {
    LogEvent(FORMAT(L"Selecting events %d for socket %d", static_cast<int>(Events), static_cast<int>(Socket)));
  }

  if (WSAEventSelect(Socket, (WSAEVENT)Event, Events) == SOCKET_ERROR)
  {
    CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", int(Events), int(Socket));
    if (Configuration->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT(L"Error selecting events %d for socket %d", static_cast<int>(Events), static_cast<int>(Socket)));
    }

    if (Startup)
    {
      FatalError(FMTLOAD(EVENT_SELECT_ERROR, WSAGetLastError()));
    }
  }
}
//---------------------------------------------------------------------------
void TSecureShell::UpdateSocket(SOCKET value, bool Startup)
{
  CCALLSTACK(TRACE_TRANSMIT);
  if (!FActive && !Startup)
  {
    // no-op
    // Remove the branch eventualy:
    // When TCP connection fails, PuTTY does not release the memory allocated for
    // socket. As a simple hack we call sk_tcp_close() in ssh.c to release the memory,
    // until they fix it better. Unfortunately sk_tcp_close calls do_select,
    // so we must filter that out.
  }
  else
  {
    CCALLSTACK(TRACE_TRANSMIT);
    CTRACE(TRACE_TRANSMIT, "2");
    assert(value);
    assert((FActive && (FSocket == value)) || (!FActive && Startup));

    // filter our "local proxy" connection, which have no socket
    if (value != INVALID_SOCKET)
    {
      CTRACE(TRACE_TRANSMIT, "3");
      SocketEventSelect(value, FSocketEvent, Startup);
    }
    else
    {
      CTRACE(TRACE_TRANSMIT, "4");
      assert(FSessionData->GetProxyMethod() == pmCmd);
    }

    if (Startup)
    {
      CTRACE(TRACE_TRANSMIT, "5");
      FSocket = value;
      FActive = true;
    }
    else
    {
      CTRACE(TRACE_TRANSMIT, "6");
      FSocket = INVALID_SOCKET;
      Discard();
    }
  }
}
//---------------------------------------------------------------------------
void TSecureShell::UpdatePortFwdSocket(SOCKET value, bool Startup)
{
  CCALLSTACK(TRACE_TRANSMIT);
  CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", int(value), int(Startup));
  if (Configuration->GetActualLogProtocol() >= 2)
  {
    LogEvent(FORMAT(L"Updating forwarding socket %d (%d)", static_cast<int>(value), static_cast<int>(Startup)));
  }

  SocketEventSelect(value, FSocketEvent, Startup);

  if (Startup)
  {
    FPortFwdSockets.insert(value);
  }
  else
  {
    FPortFwdSockets.erase(value);
  }
}
//---------------------------------------------------------------------------
void TSecureShell::SetActive(bool value)
{
  if (FActive != value)
  {
    if (value)
    {
      Open();
    }
    else
    {
      Close();
    }
  }
}
//---------------------------------------------------------------------------
void TSecureShell::FreeBackend()
{
  CALLSTACK;
  TRACEFMT("1 [%x]", int(FBackendHandle));
  if (FBackendHandle != NULL)
  {
    TRACE("1");
    FBackend->bfree(FBackendHandle);
    TRACE("2");
    FBackendHandle = NULL;
    TRACEFMT("3 [%x]", int(FBackendHandle));
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TSecureShell::Discard()
{
  CALLSTACK;
  bool WasActive = FActive;
  FActive = false;
  FOpened = false;

  if (WasActive)
  {
    FUI->Closed();
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TSecureShell::Close()
{
  CALLSTACK;
  LogEvent(L"Closing connection.");
  assert(FActive);

  // this is particularly necessary when using local proxy command
  // (e.g. plink), otherwise it hangs in sk_localproxy_close
  SendEOF();

  FreeBackend();

  Discard();
  TRACE("/");
}
//---------------------------------------------------------------------------
void inline TSecureShell::CheckConnection(int Message)
{
  CCALLSTACK(TRACE_TRANSMIT);
  if (!FActive || get_ssh_state_closed(FBackendHandle))
  {
    CTRACE(TRACE_TRANSMIT, "2");
    UnicodeString Str = LoadStr(Message >= 0 ? Message : NOT_CONNECTED);
    int ExitCode = get_ssh_exitcode(FBackendHandle);
    if (ExitCode >= 0)
    {
      Str += L" " + FMTLOAD(SSH_EXITCODE, ExitCode);
    }
    CTRACE(TRACE_TRANSMIT, "3");
    FatalError(Str);
  }
  CTRACE(TRACE_TRANSMIT, "/");
}
//---------------------------------------------------------------------------
void TSecureShell::PoolForData(WSANETWORKEVENTS & Events, unsigned int & Result)
{
  CCALLSTACK(TRACE_TRANSMIT);
  if (!GetActive())
  {
    // see comment below
    Result = qaRetry;
  }
  else
  {
    try
    {
      CTRACE(TRACE_TRANSMIT, "1");
      if (Configuration->GetActualLogProtocol() >= 2)
      {
        LogEvent(L"Pooling for data in case they finally arrives");
      }

      // in extreme condition it may happen that send buffer is full, but there
      // will be no data comming and we may not empty the send buffer because we
      // do not process FD_WRITE until we receive any FD_READ
      if (EventSelectLoop(0, false, &Events))
      {
        LogEvent(L"Data has arrived, closing query to user.");
        Result = qaOK;
      }
      CTRACE(TRACE_TRANSMIT, "2");
    }
    TRACE_CATCH_ALL
    {
      // TRACE_EXCEPT;
      // if we let the exception out, it may popup another message dialog
      // in whole event loop, another call to PoolForData from original dialog
      // would be invoked, leading to an infinite loop.
      // by retrying we hope (that probably fatal) error would repeat in WaitForData.
      // anyway now once no actual work is done in EventSelectLoop,
      // hardly any exception can occur actually
      Result = qaRetry;
    }
  }
  CTRACE(TRACE_TRANSMIT, "/");
}
//---------------------------------------------------------------------------
class TPoolForDataEvent
{
public:
  TPoolForDataEvent(TSecureShell * SecureShell, WSANETWORKEVENTS & Events) :
    FSecureShell(SecureShell),
    FEvents(Events)
  {
  }

  void PoolForData(unsigned int & Result)
  {
    FSecureShell->PoolForData(FEvents, Result);
  }

private:
  TSecureShell * FSecureShell;
  WSANETWORKEVENTS & FEvents;
};
//---------------------------------------------------------------------------
void TSecureShell::WaitForData()
{
  CCALLSTACK(TRACE_TRANSMIT);
  // see winsftp.c
  bool IncomingData;

  do
  {
    CTRACE(TRACE_TRANSMIT, "1");
    if (Configuration->GetActualLogProtocol() >= 2)
    {
      LogEvent(L"Looking for incoming data");
    }

    IncomingData = EventSelectLoop(FSessionData->GetTimeout() * MSecsPerSec, true, NULL);
    if (!IncomingData)
    {
      WSANETWORKEVENTS Events;
      memset(&Events, 0, sizeof(Events));
      TPoolForDataEvent Event(this, Events);

      CTRACE(TRACE_TRANSMIT, "1");
      LogEvent(L"Waiting for data timed out, asking user what to do.");
      unsigned int Answer = TimeoutPrompt(MAKE_CALLBACK(TPoolForDataEvent::PoolForData, &Event));
      CTRACE(TRACE_TRANSMIT, "2");
      switch (Answer)
      {
        case qaRetry:
          // noop
          break;

        case qaOK:
          // read event was already captured in PoolForData(),
          // make sure we do not try to select it again as it would timeout
          // unless another read event occurs
          IncomingData = true;
          HandleNetworkEvents(FSocket, Events);
          break;

        default:
          assert(false);
          // fallthru

        case qaAbort:
          FatalError(LoadStr(USER_TERMINATED));
          break;
      }
      CTRACE(TRACE_TRANSMIT, "3");
    }
  }
  while (!IncomingData);
  CTRACE(TRACE_TRANSMIT, "/");
}
//---------------------------------------------------------------------------
bool TSecureShell::SshFallbackCmd() const
{
  return ssh_fallback_cmd(FBackendHandle) != 0;
}
//---------------------------------------------------------------------------
bool TSecureShell::EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events)
{
  CCALLSTACK(TRACE_TRANSMIT);
  CTRACEFMT(TRACE_TRANSMIT, "1 [%d]", int(Socket));
  if (Configuration->GetActualLogProtocol() >= 2)
  {
    LogEvent(FORMAT(L"Enumerating network events for socket %d", static_cast<int>(Socket)));
  }

  // see winplink.c
  WSANETWORKEVENTS AEvents;
  if (WSAEnumNetworkEvents(Socket, NULL, &AEvents) == 0)
  {
    noise_ultralight(static_cast<unsigned long>(Socket));
    noise_ultralight(AEvents.lNetworkEvents);

    Events.lNetworkEvents |= AEvents.lNetworkEvents;
    for (intptr_t Index = 0; Index < FD_MAX_EVENTS; ++Index)
    {
      if (AEvents.iErrorCode[Index] != 0)
      {
        Events.iErrorCode[Index] = AEvents.iErrorCode[Index];
      }
    }

    CTRACEFMT(TRACE_TRANSMIT, "2 [%d] [%d] [%d]", int(AEvents.lNetworkEvents), int(Events.lNetworkEvents), int(Socket));
    if (Configuration->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT(L"Enumerated %d network events making %d cumulative events for socket %d",
        static_cast<int>(AEvents.lNetworkEvents), static_cast<int>(Events.lNetworkEvents), static_cast<int>(Socket)));
    }
  }
  else
  {
    CTRACEFMT(TRACE_TRANSMIT, "3 [%d]", int(Socket));
    if (Configuration->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT(L"Error enumerating network events for socket %d", static_cast<int>(Socket)));
    }
  }

  return
    FLAGSET(Events.lNetworkEvents, FD_READ) ||
    FLAGSET(Events.lNetworkEvents, FD_CLOSE);
}
//---------------------------------------------------------------------------
void TSecureShell::HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events)
{
  CCALLSTACK(TRACE_TRANSMIT);
  static const struct { int Bit, Mask; const wchar_t * Desc; } EventTypes[] =
  {
    { FD_READ_BIT, FD_READ, L"read" },
    { FD_WRITE_BIT, FD_WRITE, L"write" },
    { FD_OOB_BIT, FD_OOB, L"oob" },
    { FD_ACCEPT_BIT, FD_ACCEPT, L"accept" },
    { FD_CONNECT_BIT, FD_CONNECT, L"connect" },
    { FD_CLOSE_BIT, FD_CLOSE, L"close" },
  };

  CTRACE(TRACE_TRANSMIT, "1");
  for (unsigned int Event = 0; Event < LENOF(EventTypes); Event++)
  {
    if (FLAGSET(Events.lNetworkEvents, EventTypes[Event].Mask))
    {
      int Err = Events.iErrorCode[EventTypes[Event].Bit];
      CTRACEFMT(TRACE_TRANSMIT, "2 [%s] [%d] [%d]", EventTypes[Event].Desc, int(Socket), Err);
      if (Configuration->GetActualLogProtocol() >= 2)
      {
        LogEvent(FORMAT(L"Handling network %s event on socket %d with error %d",
          EventTypes[Event].Desc, int(Socket), Err));
      }
      #pragma option push -w-prc
      LPARAM SelectEvent = WSAMAKESELECTREPLY(EventTypes[Event].Mask, Err);
      #pragma option pop
      if (!select_result(static_cast<WPARAM>(Socket), SelectEvent))
      {
        // note that connection was closed definitely,
        // so "check" is actually not required
        CheckConnection();
      }
    }
  }
  CTRACE(TRACE_TRANSMIT, "/");
}
//---------------------------------------------------------------------------
bool TSecureShell::ProcessNetworkEvents(SOCKET Socket)
{
  CCALLSTACK(TRACE_TRANSMIT);
  WSANETWORKEVENTS Events;
  memset(&Events, 0, sizeof(Events));
  bool Result = EnumNetworkEvents(Socket, Events);
  HandleNetworkEvents(Socket, Events);
  CTRACE(TRACE_TRANSMIT, "/");
  return Result;
}
//---------------------------------------------------------------------------
bool TSecureShell::EventSelectLoop(unsigned int MSec, bool ReadEventRequired,
  WSANETWORKEVENTS * Events)
{
  CCALLSTACK(TRACE_TRANSMIT);
  CheckConnection();

  bool Result = false;

  do
  {
    CTRACEFMT(TRACE_TRANSMIT, "1 [%d] [%d]", int(MSec), int(ReadEventRequired));
    if (Configuration->GetActualLogProtocol() >= 2)
    {
      // LogEvent(L"Looking for network events");
    }
    unsigned int TicksBefore = GetTickCount();
    int HandleCount;
    // note that this returns all handles, not only the session-related handles
    HANDLE * Handles = handle_get_events(&HandleCount);
    TRY_FINALLY (
    {
      CTRACE(TRACE_TRANSMIT, "1a");
      Handles = sresize(Handles, static_cast<size_t>(HandleCount + 1), HANDLE);
      Handles[HandleCount] = FSocketEvent;
      unsigned int WaitResult = WaitForMultipleObjects(HandleCount + 1, Handles, FALSE, MSec);
      if (WaitResult < WAIT_OBJECT_0 + HandleCount)
      {
        if (handle_got_event(Handles[WaitResult - WAIT_OBJECT_0]))
        {
          Result = true;
        }
      }
      else if (WaitResult == WAIT_OBJECT_0 + HandleCount)
      {
        CTRACE(TRACE_TRANSMIT, "2");
        if (Configuration->GetActualLogProtocol() >= 1)
        {
          LogEvent(L"Detected network event");
        }

        if (Events == NULL)
        {
          if (ProcessNetworkEvents(FSocket))
          {
            Result = true;
          }
        }
        else
        {
          if (EnumNetworkEvents(FSocket, *Events))
          {
            Result = true;
          }
        }

        {
          TSockets::iterator i = FPortFwdSockets.begin();
          while (i != FPortFwdSockets.end())
          {
            ProcessNetworkEvents(*i);
            ++i;
          }
        }
      }
      else if (WaitResult == WAIT_TIMEOUT)
      {
        CTRACE(TRACE_TRANSMIT, "3");
        if (Configuration->GetActualLogProtocol() >= 2)
        {
          // LogEvent(L"Timeout waiting for network events");
        }

        MSec = 0;
      }
      else
      {
        CTRACEFMT(TRACE_TRANSMIT, "4 [%d]", int(WaitResult));
        if (Configuration->GetActualLogProtocol() >= 2)
        {
          LogEvent(FORMAT(L"Unknown waiting result %d", static_cast<int>(WaitResult)));
        }

        MSec = 0;
      }
    }
    ,
    {
      sfree(Handles);
    }
    );

    unsigned int TicksAfter = GetTickCount();
    // ticks wraps once in 49.7 days
    if (TicksBefore < TicksAfter)
    {
      unsigned int Ticks = TicksAfter - TicksBefore;
      if (Ticks > MSec)
      {
        MSec = 0;
      }
      else
      {
        MSec -= Ticks;
      }
    }
  }
  while (ReadEventRequired && (MSec > 0) && !Result);

  CTRACE(TRACE_TRANSMIT, "/");
  return Result;
}
//---------------------------------------------------------------------------
void TSecureShell::Idle(unsigned int MSec)
{
  CALLSTACK;
  TRACE_EXCEPT_BEGIN
  noise_regular();

  call_ssh_timer(FBackendHandle);

  EventSelectLoop(MSec, false, NULL);
  TRACE_EXCEPT_END
  TRACE("/");
}
//---------------------------------------------------------------------------
void TSecureShell::KeepAlive()
{
  if (FActive && (FWaiting == 0))
  {
    LogEvent(L"Sending null packet to keep session alive.");
    SendSpecial(TS_PING);
  }
  else
  {
    // defer next keepalive attempt
    FLastDataSent = Now();
  }
}
//---------------------------------------------------------------------------
static unsigned int minPacketSize = 0;

unsigned long TSecureShell::MinPacketSize()
{
  if (!FSessionInfoValid)
  {
    UpdateSessionInfo();
  }

  if (FSshVersion == 1)
  {
    return 0;
  }
  else
  {
    if (FMinPacketSize == NULL)
    {
      FMinPacketSize = &minPacketSize;
    }
    return *FMinPacketSize;
  }
}
//---------------------------------------------------------------------------
unsigned long TSecureShell::MaxPacketSize()
{
  if (!FSessionInfoValid)
  {
    UpdateSessionInfo();
  }

  if (FSshVersion == 1)
  {
    return 0;
  }
  else
  {
    if (FMaxPacketSize == NULL)
    {
      FMaxPacketSize = ssh2_remmaxpkt(FBackendHandle);
    }
    return *FMaxPacketSize;
  }
}
//---------------------------------------------------------------------------
UnicodeString TSecureShell::FuncToCompression(
  int SshVersion, const void * Compress) const
{
  enum TCompressionType { ctNone, ctZLib };
  if (SshVersion == 1)
  {
    return get_ssh1_compressing(FBackendHandle) ? L"ZLib" : L"";
  }
  else
  {
    return reinterpret_cast<ssh_compress *>(const_cast<void *>(Compress)) == &ssh_zlib ? L"ZLib" : L"";
  }
}
//---------------------------------------------------------------------------
TCipher TSecureShell::FuncToSsh1Cipher(const void * Cipher)
{
  const ssh_cipher *CipherFuncs[] =
    {&ssh_3des, &ssh_des, &ssh_blowfish_ssh1};
  const TCipher TCiphers[] = {cip3DES, cipDES, cipBlowfish};
  assert(LENOF(CipherFuncs) == LENOF(TCiphers));
  TCipher Result = cipWarn;

  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(TCiphers)); ++Index)
  {
    if (static_cast<ssh_cipher *>(const_cast<void *>(Cipher)) == CipherFuncs[Index])
    {
      Result = TCiphers[Index];
    }
  }

  assert(Result != cipWarn);
  return Result;
}
//---------------------------------------------------------------------------
TCipher TSecureShell::FuncToSsh2Cipher(const void * Cipher)
{
  const ssh2_ciphers *CipherFuncs[] =
    {&ssh2_3des, &ssh2_des, &ssh2_aes, &ssh2_blowfish, &ssh2_arcfour};
  const TCipher TCiphers[] = {cip3DES, cipDES, cipAES, cipBlowfish, cipArcfour};
  assert(LENOF(CipherFuncs) == LENOF(TCiphers));
  TCipher Result = cipWarn;

  for (unsigned int C = 0; C < LENOF(TCiphers); C++)
  {
    for (int F = 0; F < CipherFuncs[C]->nciphers; F++)
    {
      if (reinterpret_cast<ssh2_cipher *>(const_cast<void *>(Cipher)) == CipherFuncs[C]->list[F])
      {
        Result = TCiphers[C];
      }
    }
  }

  assert(Result != cipWarn);
  return Result;
}
//---------------------------------------------------------------------------
#ifndef _MSC_VER
struct TClipboardHandler
{
  UnicodeString Text;

  void Copy(TObject * /*Sender*/)
  {
    CopyToClipboard(Text.c_str();
  }
};
#endif
//---------------------------------------------------------------------------
void TSecureShell::VerifyHostKey(const UnicodeString & Host, int Port,
  const UnicodeString & KeyType, const UnicodeString & KeyStr, const UnicodeString & Fingerprint)
{
  LogEvent(FORMAT(L"Verifying host key %s %s with fingerprint %s", KeyType.c_str(), KeyStr.c_str(), Fingerprint.c_str()));

  UnicodeString Host2 = Host;
  UnicodeString KeyStr2 = KeyStr;
  GotHostKey();

  wchar_t Delimiter = L';';
  assert(KeyStr.Pos(Delimiter) == 0);

  if (FSessionData->GetTunnel())
  {
    Host2 = FSessionData->GetOrigHostName();
    Port = FSessionData->GetOrigPortNumber();
  }

  FSessionInfo.HostKeyFingerprint = Fingerprint;

  bool Result = false;

  UnicodeString Buf = FSessionData->GetHostKey();
  while (!Result && !Buf.IsEmpty())
  {
    UnicodeString ExpectedKey = CutToChar(Buf, Delimiter, false);
    if (ExpectedKey == Fingerprint)
    {
      LogEvent(L"Host key matches configured key");
      Result = true;
    }
    else
    {
      LogEvent(FORMAT(L"Host key does not match configured key %s", ExpectedKey.c_str()));
    }
  }

  UnicodeString StoredKeys;
  if (!Result)
  {
    AnsiString AnsiStoredKeys;
    AnsiStoredKeys.SetLength(10240);
    if (retrieve_host_key(W2MB(Host2.c_str(), FSessionData->GetCodePageAsNumber()).c_str(), Port, W2MB(KeyType.c_str(), FSessionData->GetCodePageAsNumber()).c_str(),
          const_cast<char *>(AnsiStoredKeys.c_str()), static_cast<int>(AnsiStoredKeys.Length())) == 0)
    {
      StoredKeys = AnsiStoredKeys.c_str();
      UnicodeString Buf2 = StoredKeys;
      while (!Result && !Buf2.IsEmpty())
      {
        UnicodeString StoredKey = CutToChar(Buf2, Delimiter, false);
        if (StoredKey == KeyStr)
        {
          LogEvent(L"Host key matches cached key");
          Result = true;
        }
        else
        {
          LogEvent(FORMAT(L"Host key does not match cached key %s", StoredKey.c_str()));
        }
      }
    }
    else
    {
      StoredKeys = L"";
    }
  }

  if (!Result)
  {
    bool Verified;
    if (Configuration->GetDisableAcceptingHostKeys())
    {
      Verified = false;
    }
    else
    {
      TClipboardHandler ClipboardHandler;
      ClipboardHandler.Text = Fingerprint;

      bool Unknown = StoredKeys.IsEmpty();

      int Answers;
      int AliasesCount;
      TQueryButtonAlias Aliases[3];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
      Aliases[0].OnClick = MAKE_CALLBACK(TClipboardHandler::Copy, &ClipboardHandler);
      Answers = qaYes | qaCancel | qaRetry;
      AliasesCount = 1;
      if (!Unknown)
      {
        Aliases[1].Button = qaYes;
        Aliases[1].Alias = LoadStr(UPDATE_KEY_BUTTON);
        Aliases[2].Button = qaOK;
        Aliases[2].Alias = LoadStr(ADD_KEY_BUTTON);
        AliasesCount += 2;
        Answers |= qaSkip | qaOK;
      }
      else
      {
        Answers |= qaNo;
      }

      TQueryParams Params;
      Params.NoBatchAnswers = qaYes | qaRetry | qaSkip | qaOK;
      Params.HelpKeyword = (Unknown ? HELP_UNKNOWN_KEY : HELP_DIFFERENT_KEY);
      Params.Aliases = Aliases;
      Params.AliasesCount = AliasesCount;
      unsigned int R = FUI->QueryUser(
        FMTLOAD((Unknown ? UNKNOWN_KEY2 : DIFFERENT_KEY3), KeyType.c_str(), Fingerprint.c_str()),
        NULL, Answers, &Params, qtWarning);

      switch (R) {
        case qaOK:
          assert(!Unknown);
          KeyStr2 = (StoredKeys + Delimiter + KeyStr);
          // fall thru
        case qaYes:
          store_host_key(AnsiString(Host2).c_str(), Port, AnsiString(KeyType).c_str(), AnsiString(KeyStr2).c_str());
          Verified = true;
          break;

        case qaCancel:
          Verified = false;
          break;

        default:
          Verified = true;
          break;
      }
    }

    if (!Verified)
    {
      Exception * E = new Exception(LoadStr(KEY_NOT_VERIFIED));
      TRY_FINALLY (
      {
        FUI->FatalError(E, FMTLOAD(HOSTKEY, Fingerprint.c_str()));
      }
      ,
      {
        delete E;
      }
      );
    }
  }
}
//---------------------------------------------------------------------------
void TSecureShell::AskAlg(const UnicodeString & AlgType,
  const UnicodeString & AlgName)
{
  UnicodeString Msg;
  if (AlgType == L"key-exchange algorithm")
  {
    Msg = FMTLOAD(KEX_BELOW_TRESHOLD, AlgName.c_str());
  }
  else
  {
    int CipherType = 0;
    if (AlgType == L"cipher")
    {
      CipherType = CIPHER_TYPE_BOTH;
    }
    else if (AlgType == L"client-to-server cipher")
    {
      CipherType = CIPHER_TYPE_CS;
    }
    else if (AlgType == L"server-to-client cipher")
    {
      CipherType = CIPHER_TYPE_SC;
    }
    else
    {
      assert(false);
    }

    Msg = FMTLOAD(CIPHER_BELOW_TRESHOLD, LoadStr(CipherType).c_str(), AlgName.c_str());
  }

  if (FUI->QueryUser(Msg, NULL, qaYes | qaNo, NULL, qtWarning) == qaNo)
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
void TSecureShell::DisplayBanner(const UnicodeString & Banner)
{
  FUI->DisplayBanner(Banner);
}
//---------------------------------------------------------------------------
void TSecureShell::OldKeyfileWarning()
{
  // actually never called, see Net.cpp
  FUI->QueryUser(LoadStr(OLD_KEY), NULL, qaOK, NULL, qtWarning);
}
//---------------------------------------------------------------------------
bool TSecureShell::GetStoredCredentialsTried() const
{
  return FStoredPasswordTried || FStoredPasswordTriedForKI;
}
//---------------------------------------------------------------------------
bool TSecureShell::GetReady() const
{
  return FOpened && (FWaiting == 0);
}
