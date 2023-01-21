#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <nbutils.h>
#include "PuttyIntf.h"
#include <Exceptions.h>

#include "Interface.h"
#include "SecureShell.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include <StrUtils.hpp>

#ifndef AUTO_WINSOCK
#include <WinSock2.h>
#endif
//#include <ws2ipdef.h>

__removed #pragma package(smart_init)

constexpr int MAX_BUFSIZE = 32 * 1024;

const wchar_t HostKeyDelimiter = L';';
static std::unique_ptr<TCriticalSection> PuttyStorageSection(TraceInitPtr(new TCriticalSection()));

struct TPuttyTranslation
{
  CUSTOM_MEM_ALLOCATION_IMPL
  const char *Original{nullptr};
  int Translation{0};
  UnicodeString HelpKeyword;
};

struct ScpLogPolicy : public LogPolicy
{
  TSecureShell * SecureShell;
  struct Seat * Seat;
};

Seat * get_log_seat(LogContext * logctx)
{
  ScpLogPolicy * ALogPolicy = static_cast<ScpLogPolicy *>(log_get_logpolicy(logctx));
  return ALogPolicy->Seat;
}

struct callback_set * get_log_callback_set(LogContext * logctx)
{
  ScpLogPolicy * ALogPolicy = static_cast<ScpLogPolicy *>(log_get_logpolicy(logctx));
  return ALogPolicy->SecureShell->GetCallbackSet();
}

TSecureShell::TSecureShell(TSessionUI * UI,
  TSessionData * SessionData, TSessionLog * Log, TConfiguration * Configuration) noexcept :
  TObject(OBJECT_CLASS_TSecureShell)
{
  FUI = UI;
  FSessionData = SessionData;
  FLog = Log;
  FConfiguration = Configuration;
  FActive = false;
  FWaiting = 0;
  FOpened = false;
  FAuthenticating = false;
  FAuthenticated = false;
  FUtfStrings = false;
  FLastSendBufferUpdate = 0;
  FSendBuf = 0;
  FSessionInfoValid = false;
  FBackend = nullptr;
  FSshImplementation = sshiUnknown;
  PendLen = 0;
  PendSize = 0;
  OutLen = 0;
  OutPtr = nullptr;
  Pending = nullptr;
  FBackendHandle = nullptr;
  FLogPolicy = nullptr;
  FSeat = nullptr;
  FLogCtx = nullptr;
  ResetConnection();
  FOnCaptureOutput = nullptr;
  FOnReceive = nullptr;
  FSocket = INVALID_SOCKET;
  FSocketEvent = ::CreateEvent(nullptr, false, false, nullptr);
  FFrozen = false;
  FDataWhileFrozen = false;
  FSshVersion = 0;
  FOpened = false;
  FWaiting = 0;
  FSimple = false;
  FNoConnectionResponse = false;
  FCollectPrivateKeyUsage = false;
  FWaitingForData = 0;
  FCallbackSet.reset(new callback_set());
  memset(FCallbackSet.get(), 0, sizeof(callback_set));
  FCallbackSet->ready_event = INVALID_HANDLE_VALUE;
}

TSecureShell::~TSecureShell() noexcept
{
  DebugAssert(FWaiting == 0);
  SetActive(false);
  if (Active)
  {
    Close();
  }
  ResetConnection();
  SAFE_CLOSE_HANDLE(FSocketEvent);
}

void TSecureShell::ResetConnection()
{
  FreeBackend();
  ClearStdError();
  PendLen = 0;
  PendSize = 0;
  sfree(Pending);
  Pending = nullptr;
  FCWriteTemp.Clear();
  ResetSessionInfo();
  FAuthenticating = false;
  FAuthenticated = false;
  FStoredPasswordTried = false;
  FStoredPasswordTriedForKI = false;
  FStoredPassphraseTried = false;
  delete FLogPolicy;
  FLogPolicy = nullptr;
  delete FSeat;
  FSeat = nullptr;
  if (FLogCtx != nullptr)
  {
    log_free(FLogCtx);
  }
  FLogCtx = nullptr;
  FClosed = false;
  FLoggedKnownHostKeys.clear();
}

void TSecureShell::ResetSessionInfo()
{
  FSessionInfoValid = false;
  FMinPacketSize = nullptr;
  FMaxPacketSize = nullptr;
}

void TSecureShell::UpdateSessionInfo() const
{
  if (!FSessionInfoValid)
  {
    DebugAssert(get_ssh_version(FBackendHandle) == 2);
    FSessionInfo.ProtocolBaseName = L"SSH";
    FSessionInfo.ProtocolName =
      FORMAT("%s-%d", FSessionInfo.ProtocolBaseName, get_ssh_version(FBackendHandle));
    FSessionInfo.SecurityProtocolName = FSessionInfo.ProtocolName;

    FSessionInfo.CSCipher = GetCipherName(get_cscipher(FBackendHandle));
    FSessionInfo.SCCipher = GetCipherName(get_sccipher(FBackendHandle));
    FSessionInfo.CSCompression = GetCompressorName(get_cscomp(FBackendHandle));
    FSessionInfo.SCCompression = GetDecompressorName(get_sccomp(FBackendHandle));

    FSessionInfoValid = true;
  }
}

const TSessionInfo &TSecureShell::GetSessionInfo() const
{
  if (!FSessionInfoValid)
  {
    UpdateSessionInfo();
  }
  return FSessionInfo;
}

void TSecureShell::GetHostKeyFingerprint(UnicodeString &SHA256, UnicodeString &MD5) const
{
  SHA256 = FSessionInfo.HostKeyFingerprintSHA256;
  MD5 = FSessionInfo.HostKeyFingerprintMD5;
}

Conf * TSecureShell::StoreToConfig(TSessionData * Data, bool Simple)
{
  Conf * conf = conf_new();

  PuttyDefaults(conf);

  // user-configurable settings
  conf_set_str(conf, CONF_host, AnsiString(Data->GetHostNameExpanded()).c_str());
  conf_set_str(conf, CONF_username, UTF8String(Data->GetUserNameExpanded()).c_str());
  conf_set_int(conf, CONF_port, nb::ToInt(Data->GetPortNumber()));
  conf_set_int(conf, CONF_protocol, PROT_SSH);
  conf_set_bool(conf, CONF_change_password, Data->GetChangePassword());
  // always set 0, as we will handle keepalives ourselves to avoid
  // multi-threaded issues in putty timer list
  conf_set_int(conf, CONF_ping_interval, 0);
  conf_set_bool(conf, CONF_compression, Data->GetCompression());
  conf_set_bool(conf, CONF_tryagent, Data->GetTryAgent());
  conf_set_bool (conf, CONF_agentfwd, Data->GetAgentFwd());
  conf_set_int(conf, CONF_addressfamily, Data->GetAddressFamily());
  conf_set_str(conf, CONF_ssh_rekey_data, AnsiString(Data->GetRekeyData()).c_str());
  conf_set_int(conf, CONF_ssh_rekey_time, nb::ToInt(Data->GetRekeyTime()));

  DebugAssert(CIPHER_MAX == CIPHER_COUNT);
  for (int c = 0; c < CIPHER_COUNT; c++)
  {
    int pcipher;
    switch (Data->Cipher[c]) {
      case cipWarn: pcipher = CIPHER_WARN; break;
      case cip3DES: pcipher = CIPHER_3DES; break;
      case cipBlowfish: pcipher = CIPHER_BLOWFISH; break;
      case cipAES: pcipher = CIPHER_AES; break;
      case cipDES: pcipher = CIPHER_DES; break;
      case cipArcfour: pcipher = CIPHER_ARCFOUR; break;
      case cipChaCha20: pcipher = CIPHER_CHACHA20; break;
      default: DebugFail();
    }
    conf_set_int_int(conf, CONF_ssh_cipherlist, c, pcipher);
  }

  DebugAssert(KEX_MAX == KEX_COUNT);
  for (int k = 0; k < KEX_COUNT; k++)
  {
    int pkex;
    switch (Data->Kex[k]) {
      case kexWarn: pkex = KEX_WARN; break;
      case kexDHGroup1: pkex = KEX_DHGROUP1; break;
      case kexDHGroup14: pkex = KEX_DHGROUP14; break;
      case kexDHGEx: pkex = KEX_DHGEX; break;
      case kexRSA: pkex = KEX_RSA; break;
      case kexECDH: pkex = KEX_ECDH; break;
      default: DebugFail();
    }
    conf_set_int_int(conf, CONF_ssh_kexlist, k, pkex);
  }

  DebugAssert(static_cast<THostKey>(HK_MAX) == HOSTKEY_COUNT);
  for (int h = 0; h < HOSTKEY_COUNT; h++)
  {
    int phk = 0;
    switch (Data->GetHostKeys(h)) {
      case hkWarn: phk = HK_WARN; break;
      case hkRSA: phk = HK_RSA; break;
      case hkDSA: phk = hkDSA; break;
      case hkECDSA: phk = HK_ECDSA; break;
      case hkED25519: phk = HK_ED25519; break;
      case hkED448: phk = HK_ED448; break;
      default: DebugFail();
    }
    conf_set_int_int(conf, CONF_ssh_hklist, h, phk);
  }

  DebugAssert(ngsslibs == GSSLIB_COUNT);
  for (int g = 0; g < GSSLIB_COUNT; g++)
  {
    int pgsslib = 0;
    switch (Data->GetGssLib(g)) {
      case gssGssApi32: pgsslib = 0; break;
      case gssSspi: pgsslib = 1; break;
      case gssCustom: pgsslib = 2; break;
      default: DebugFail();
    }
    conf_set_int_int(conf, CONF_ssh_gsslist, g, pgsslib);
  }
  Filename * GssLibCustomFileName = filename_from_str(UTF8String(Data->GetGssLibCustom()).c_str());
  conf_set_filename(conf, CONF_ssh_gss_custom, GssLibCustomFileName);
  filename_free(GssLibCustomFileName);

  UnicodeString SPublicKeyFile = Data->GetPublicKeyFile();
  if (SPublicKeyFile.IsEmpty()) { SPublicKeyFile = GetConfiguration()->GetDefaultKeyFile(); }
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  SPublicKeyFile = StripPathQuotes(::ExpandEnvironmentVariables(SPublicKeyFile));
  Filename *KeyFileFileName = filename_from_str(UTF8String(SPublicKeyFile).c_str());
  conf_set_filename(conf, CONF_keyfile, KeyFileFileName);
  filename_free(KeyFileFileName);

  conf_set_bool(conf, CONF_ssh2_des_cbc, Data->GetSsh2DES());
  conf_set_bool(conf, CONF_ssh_no_userauth, Data->GetSshNoUserAuth());
  conf_set_bool(conf, CONF_try_ki_auth, Data->AuthKI);
  conf_set_bool(conf, CONF_try_gssapi_auth, Data->GetAuthGSSAPI());
  conf_set_bool(conf, CONF_try_gssapi_kex, Data->AuthGSSAPIKEX);
  conf_set_bool(conf, CONF_gssapifwd, Data->GSSAPIFwdTGT);
  conf_set_bool(conf, CONF_change_username, Data->GetChangeUsername());

  conf_set_int(conf, CONF_proxy_type, Data->GetActualProxyMethod());
  conf_set_str(conf, CONF_proxy_host, AnsiString(Data->GetProxyHost()).c_str());
  conf_set_int(conf, CONF_proxy_port, nb::ToInt(Data->GetProxyPort()));
  conf_set_str(conf, CONF_proxy_username, UTF8String(Data->GetProxyUsername()).c_str());
  conf_set_str(conf, CONF_proxy_password, UTF8String(Data->GetProxyPassword()).c_str());
  if (Data->GetProxyMethod() == pmCmd)
  {
    conf_set_str(conf, CONF_proxy_telnet_command, AnsiString(Data->GetProxyLocalCommand()).c_str());
  }
  else
  {
    conf_set_str(conf, CONF_proxy_telnet_command, AnsiString(Data->GetProxyTelnetCommand()).c_str());
  }
  conf_set_int(conf, CONF_proxy_dns, Data->GetProxyDNS());
  conf_set_bool(conf, CONF_even_proxy_localhost, Data->GetProxyLocalhost());

  conf_set_int(conf, CONF_sshbug_hmac2, Data->GetBug(sbHMAC2));
  conf_set_int(conf, CONF_sshbug_derivekey2, Data->GetBug(sbDeriveKey2));
  conf_set_int(conf, CONF_sshbug_rsapad2, Data->Bug[sbRSAPad2]);
  conf_set_int(conf, CONF_sshbug_rekey2, Data->Bug[sbRekey2]);
  conf_set_int(conf, CONF_sshbug_pksessid2, Data->Bug[sbPKSessID2]);
  conf_set_int(conf, CONF_sshbug_maxpkt2, Data->Bug[sbMaxPkt2]);
  conf_set_int(conf, CONF_sshbug_ignore2, Data->Bug[sbIgnore2]);
  conf_set_int(conf, CONF_sshbug_winadj, Data->Bug[sbWinAdj]);
  conf_set_int(conf, CONF_sshbug_oldgex2, Data->Bug[sbOldGex2]);
  conf_set_int(conf, CONF_sshbug_chanreq, Data->Bug[sbChanReq]);

  if (!Data->GetTunnelPortFwd().IsEmpty())
  {
    DebugAssert(!Simple);
    UnicodeString TunnelPortFwd = Data->GetTunnelPortFwd();
    while (!TunnelPortFwd.IsEmpty())
    {
      UnicodeString Buf = CutToChar(TunnelPortFwd, L',', true);
      AnsiString Key = AnsiString(CutToChar(Buf, L'\t', true));
      AnsiString Value = AnsiString(Buf);
      conf_set_str_str(conf, CONF_portfwd, Key.c_str(), Value.c_str());
    }

    // when setting up a tunnel, do not open shell/sftp
    conf_set_bool(conf, CONF_ssh_no_shell, TRUE);
  }
  else
  {
    DebugAssert(Simple);
    conf_set_bool(conf, CONF_ssh_simple, Data->GetSshSimple() && Simple);

    if (Data->GetFSProtocol() == fsSCPonly)
    {
      conf_set_bool(conf, CONF_ssh_subsys, FALSE);
      if (Data->GetShell().IsEmpty())
      {
        // Following forces Putty to open default shell
        // see ssh.c: do_ssh2_authconn() and ssh1_protocol()
        conf_set_str(conf, CONF_remote_cmd, "");
      }
      else
      {
        conf_set_str(conf, CONF_remote_cmd, AnsiString(Data->GetShell()).c_str());
      }
      conf_set_bool(conf, CONF_force_remote_cmd2, 0);
    }
    else
    {
      if (Data->GetSftpServer().IsEmpty())
      {
        conf_set_bool(conf, CONF_ssh_subsys, TRUE);
        conf_set_str(conf, CONF_remote_cmd, "sftp");
      }
      else
      {
        conf_set_bool(conf, CONF_ssh_subsys, FALSE);
        conf_set_str(conf, CONF_remote_cmd, AnsiString(Data->GetSftpServer()).c_str());
      }

      if (Data->GetFSProtocol() != fsSFTPonly)
      {
        conf_set_bool(conf, CONF_ssh_subsys2, FALSE);
        if (Data->GetShell().IsEmpty())
        {
          // Following forces Putty to open default shell
          // see ssh.c: do_ssh2_authconn() and ssh1_protocol()
          conf_set_str(conf, CONF_remote_cmd2, "");
          // PuTTY ignores CONF_remote_cmd2 set to "",
          // so we have to enforce it
          // (CONF_force_remote_cmd2 is our config option)
          conf_set_bool(conf, CONF_force_remote_cmd2, 1);
        }
        else
        {
          conf_set_str(conf, CONF_remote_cmd2, AnsiString(Data->GetShell()).c_str());
        }
      }
      else
      {
        if (Data->SftpServer.IsEmpty())
        {
          // see psftp_connect() from psftp.c
          conf_set_bool(conf, CONF_ssh_subsys2, FALSE);
          conf_set_str(conf, CONF_remote_cmd2,
            "test -x /usr/lib/sftp-server && exec /usr/lib/sftp-server\n"
            "test -x /usr/local/lib/sftp-server && exec /usr/local/lib/sftp-server\n"
            "exec sftp-server");
        }
        else
        {
          conf_set_bool(conf, CONF_force_remote_cmd2, 0);
        }
      }
    }
  }

  conf_set_int(conf, CONF_connect_timeout, nb::ToInt(Data->GetTimeout() * MSecsPerSec));
  conf_set_int(conf, CONF_sndbuf, nb::ToInt(Data->GetSendBuf()));
  conf_set_str(conf, CONF_srcaddr, AnsiString(Data->SourceAddress).c_str());

  // permanent settings
  conf_set_bool(conf, CONF_nopty, TRUE);
  conf_set_bool(conf, CONF_tcp_keepalives, 1);
  conf_set_bool(conf, CONF_ssh_show_banner, TRUE);
  conf_set_int(conf, CONF_proxy_log_to_term, FORCE_OFF);

  conf_set_str(conf, CONF_loghost, AnsiString(Data->GetLogicalHostName()).c_str());

  return conf;
}

static void eventlog(LogPolicy * ALogPolicy, const char * string)
{
  static_cast<ScpLogPolicy *>(ALogPolicy)->SecureShell->PuttyLogEvent(string);
}

static const LogPolicyVtable ScpLogPolicyVTable =
  {
    eventlog,
    nullptr, // Should never be called
    nullptr, // Should never be called
    null_lp_verbose_no, // Should never be called
  };

void TSecureShell::Open()
{
  ResetConnection();

  FAuthenticating = false;
  FAuthenticated = false;
  FLastSendBufferUpdate = 0;

  // do not use UTF-8 until decided otherwise (see TSCPFileSystem::DetectUtf())
  FUtfStrings = false;

  SetActive(false);

  FAuthenticationLog.Clear();
  FNoConnectionResponse = false;
  FUI->Information(LoadStr(STATUS_LOOKUPHOST), true);

  try
  {
    char * RealHost = nullptr;
    FreeBackend(); // in case we are reconnecting
    const char * InitError{nullptr};
    Conf * conf = StoreToConfig(FSessionData, GetSimple());
    FSendBuf = FSessionData->GetSendBuf();
    FSeat = new ScpSeat(this);
    FLogPolicy = new ScpLogPolicy();
    FLogPolicy->vt = &ScpLogPolicyVTable;
    FLogPolicy->SecureShell = this;
    FLogPolicy->Seat = FSeat;
    try__finally
    {
      FLogCtx = log_init(FLogPolicy, conf);
      InitError = backend_init(&ssh_backend, FSeat, &FBackendHandle, FLogCtx, conf,
        AnsiString(FSessionData->GetHostNameExpanded()).c_str(), nb::ToInt(FSessionData->GetPortNumber()), &RealHost,
        FSessionData->GetTcpNoDelay() ? 1 : 0,
        conf_get_bool(conf, CONF_tcp_keepalives));
    },
    __finally
    {
      conf_free(conf);
    } end_try__finally

    sfree(RealHost);
    if (InitError)
    {
      PuttyFatalError(UnicodeString(InitError));
    }
    FUI->Information(LoadStr(STATUS_CONNECT), true);
    if (!Active && DebugAlwaysTrue(HasLocalProxy()))
    {
      FActive = true;
    }
    Init();

    CheckConnection(CONNECTION_FAILED);
  }
  catch (Exception & E)
  {
    if (FNoConnectionResponse && TryFtp())
    {
      // GetConfiguration()->Usage->Inc("ProtocolSuggestions");
      // HELP_FTP_SUGGESTION won't be used as all errors that set
      // FNoConnectionResponse have already their own help keyword
      FUI->FatalError(&E, LoadStr(FTP_SUGGESTION));
    }
    else
    {
      throw;
    }
  }
  FLastDataSent = Now();

  FSessionInfo.LoginTime = Now();

  FAuthenticating = false;
  FAuthenticated = true;
  FUI->Information(LoadStr(STATUS_AUTHENTICATED), true);

  ResetSessionInfo();

  DebugAssert(!FSessionInfo.SshImplementation.IsEmpty());
  FOpened = true;

  UnicodeString SshImplementation = GetSessionInfo().SshImplementation;
  if (IsOpenSSH(SshImplementation))
  {
    FSshImplementation = sshiOpenSSH;
  }
  // e.g. "mod_sftp/0.9.8"
  else if (SshImplementation.Pos("mod_sftp") == 1)
  {
    FSshImplementation = sshiProFTPD;
  }
  // e.g. "5.25 FlowSsh: Bitvise SSH Server (WinSSHD) 6.07: free only for personal non-commercial use"
  else if (SshImplementation.Pos("FlowSsh") > 0)
  {
    FSshImplementation = sshiBitvise;
  }
  // e.g. "srtSSHServer_10.00"
  else if (::ContainsText(SshImplementation, "srtSSHServer"))
  {
    FSshImplementation = sshiTitan;
  }
  else if (::ContainsText(FSessionInfo.SshImplementation, "OpenVMS"))
  {
    FSshImplementation = sshiOpenVMS;
  }
  else if (::ContainsText(FSessionInfo.SshImplementation, "CerberusFTPServer"))
  {
    FSshImplementation = sshiCerberus;
  }
  else
  {
    FSshImplementation = sshiUnknown;
  }
}

bool TSecureShell::TryFtp()
{
  bool Result;
  if (!FConfiguration->GetTryFtpWhenSshFails())
  {
    Result = false;
  }
  else
  {
    if (((FSessionData->GetFSProtocol() != fsSFTP) && (FSessionData->GetFSProtocol() != fsSFTPonly)) ||
        (FSessionData->GetPortNumber() != SshPortNumber) ||
        FSessionData->GetTunnel() || (FSessionData->GetProxyMethod() != ::pmNone))
    {
      LogEvent("Using non-standard protocol or port, tunnel or proxy, will not knock FTP port.");
      Result = false;
    }
    else
    {
      LogEvent("Knocking FTP port.");

      SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
      Result = (Socket != INVALID_SOCKET);
      if (Result)
      {
        LPHOSTENT HostEntry = gethostbyname(AnsiString(FSessionData->GetHostNameExpanded()).c_str());
        Result = (HostEntry != nullptr);
        if (Result)
        {
          SOCKADDR_IN Address;

          nb::ClearStruct(Address);
          Address.sin_family = AF_INET;
          int32_t Port = FtpPortNumber;
          Address.sin_port = htons(static_cast<short>(Port));
          Address.sin_addr.s_addr = *(reinterpret_cast<uint32_t *>(*HostEntry->h_addr_list));

          HANDLE Event = ::CreateEvent(nullptr, false, false, nullptr);
          Result = (::WSAEventSelect(Socket, static_cast<WSAEVENT>(Event), FD_CONNECT | FD_CLOSE) != SOCKET_ERROR);

          if (Result)
          {
            Result =
              (connect(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) != SOCKET_ERROR) ||
              (::WSAGetLastError() == WSAEWOULDBLOCK);
            if (Result)
            {
              Result = (::WaitForSingleObject(Event, 2000) == WAIT_OBJECT_0);
            }
          }
          SAFE_CLOSE_HANDLE(Event);
        }
        closesocket(Socket);
      }

      if (Result)
      {
        LogEvent("FTP port opened, will suggest using FTP protocol.");
      }
      else
      {
        LogEvent("FTP port did not open.");
      }
    }
  }
  return Result;
}

void TSecureShell::Init()
{
  try
  {
    try
    {
      // Recent pscp checks backend_exitcode(FBackendHandle) in the loop
      // (see comment in putty revision 8110)
      // It seems that we do not need to do it.

      while (!winscp_query(FBackendHandle, WINSCP_QUERY_MAIN_CHANNEL))
      {
        if (GetConfiguration()->GetActualLogProtocol() >= 1)
        {
          LogEvent("Waiting for the server to continue with the initialization");
        }
        WaitForData();
      }
    }
    catch(Exception & E)
    {
      if (FAuthenticating && !FAuthenticationLog.IsEmpty())
      {
        FUI->FatalError(&E, FMTLOAD(AUTHENTICATION_LOG, FAuthenticationLog));
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

struct callback_set * TSecureShell::GetCallbackSet()
{
  return FCallbackSet.get();
}

UnicodeString TSecureShell::ConvertFromPutty(const char *Str, int32_t Length) const
{
  int32_t BomLength = NBChTraitsCRT<char>::SafeStringLen(WINSCP_BOM);
  if ((Length >= BomLength) &&
      (strncmp(Str, WINSCP_BOM, BomLength) == 0))
  {
    return UTF8ToString(Str + BomLength, Length - BomLength);
  }
  else
  {
    return AnsiToString(Str, Length);
  }
}

const UnicodeString ServerVersionMsg(L"Remote version: ");
const UnicodeString ForwardingFailureMsg(L"Forwarded connection refused by remote");
const UnicodeString LocalPortMsg(L"Local port ");
const UnicodeString ForwadingToMsg(L" forwarding to ");
const UnicodeString FailedMsg(L" failed:");

void TSecureShell::PuttyLogEvent(const char * AStr)
{
  UnicodeString Str = ConvertFromPutty(AStr, nb::ToIntPtr(NBChTraitsCRT<char>::SafeStringLen(AStr)));
  // Gross hack
  if (StartsStr(ServerVersionMsg, Str))
  {
    FSessionInfo.SshVersionString = RightStr(Str, Str.Length() - ServerVersionMsg.Length());

    const wchar_t * Ptr = wcschr(FSessionInfo.SshVersionString.c_str(), L'-');
    if (Ptr != nullptr)
    {
      Ptr = wcschr(Ptr + 1, L'-');
    }
    FSessionInfo.SshImplementation = (Ptr != nullptr) ? Ptr + 1 : L"";
  }
  else if (StartsStr(ForwardingFailureMsg, Str))
  {
    if (ForwardingFailureMsg == Str)
    {
      FLastTunnelError = Str;
    }
    else
    {
      FLastTunnelError = RightStr(Str, Str.Length() - ForwardingFailureMsg.Length());
      UnicodeString Prefix(L": ");
      if (StartsStr(Prefix, FLastTunnelError))
      {
        FLastTunnelError.Delete(1, Prefix.Length());
      }
      static const TPuttyTranslation Translation[] = {
        { L"Administratively prohibited [%]", PFWD_TRANSL_ADMIN },
        { L"Connect failed [%]", PFWD_TRANSL_CONNECT },
      };
      TranslatePuttyMessage(Translation, LENOF(Translation), FLastTunnelError);
    }
  }
  else if (StartsStr(LocalPortMsg, Str) && ContainsStr(Str, ForwadingToMsg) && ContainsStr(Str, FailedMsg))
  {
    FLastTunnelError = Str;
  }
  LogEvent(Str);
}

TPromptKind TSecureShell::IdentifyPromptKind(UnicodeString & AName) const
{
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
    { L"SOCKS proxy authentication", PROXY_AUTH_TITLE },
  };

  int Index = TranslatePuttyMessage(NameTranslation, _countof(NameTranslation), AName);

  TPromptKind PromptKind;
  if (Index == 0) // username
  {
    PromptKind = pkUserName;
  }
  else if (Index == 1) // passphrase
  {
    PromptKind = pkPassphrase;
  }
  else if (Index == 2) // TIS
  {
    PromptKind = pkTIS;
  }
  else if (Index == 3) // CryptoCard
  {
    PromptKind = pkCryptoCard;
  }
  else if ((Index == 4) || (Index == 5))
  {
    PromptKind = pkKeybInteractive;
  }
  else if (Index == 6)
  {
    PromptKind = pkPassword;
  }
  else if (Index == 7)
  {
    PromptKind = pkNewPassword;
  }
  else if (Index == 8)
  {
    PromptKind = pkProxyAuth;
  }
  else
  {
    PromptKind = pkPrompt;
    DebugFail();
  }

  return PromptKind;
}


bool TSecureShell::PromptUser(bool /*ToServer*/,
  UnicodeString AName, bool /*NameRequired*/,
  UnicodeString AInstructions, bool InstructionsRequired,
  TStrings * Prompts, TStrings * Results)
{
  // there can be zero prompts!

  DebugAssert(Results->GetCount() == Prompts->GetCount());

  UnicodeString Name = AName;
  TPromptKind PromptKind = IdentifyPromptKind(Name);

  const TPuttyTranslation * InstructionTranslation = nullptr;
  const TPuttyTranslation * PromptTranslation = nullptr;
  size_t PromptTranslationCount = 1;
  UnicodeString PromptDesc;

  if (PromptKind == pkUserName)
  {
    static const TPuttyTranslation UsernamePromptTranslation[] = {
      { "login as: ", USERNAME_PROMPT2 },
    };

    PromptTranslation = UsernamePromptTranslation;
    PromptDesc = "username";
  }
  else if (PromptKind == pkPassphrase)
  {
    static const TPuttyTranslation PassphrasePromptTranslation[] = {
      { "Passphrase for key \"%\": ", PROMPT_KEY_PASSPHRASE },
    };

    PromptTranslation = PassphrasePromptTranslation;
    PromptDesc = "passphrase";
  }
  else if (PromptKind == pkTIS)
  {
    static const TPuttyTranslation TISInstructionTranslation[] = {
      { "Using TIS authentication.%", TIS_INSTRUCTION },
    };

    static const TPuttyTranslation TISPromptTranslation[] = {
      { "Response: ", PROMPT_PROMPT },
    };

    InstructionTranslation = TISInstructionTranslation;
    PromptTranslation = TISPromptTranslation;
    PromptDesc = "tis";
  }
  else if (PromptKind == pkCryptoCard)
  {
    static const TPuttyTranslation CryptoCardInstructionTranslation[] = {
      { "Using CryptoCard authentication.%", CRYPTOCARD_INSTRUCTION },
    };
    static const TPuttyTranslation CryptoCardPromptTranslation[] = {
      { "Response: ", PROMPT_PROMPT },
    };

    InstructionTranslation = CryptoCardInstructionTranslation;
    PromptTranslation = CryptoCardPromptTranslation;
    PromptDesc = "cryptocard";
  }
  else if (PromptKind == pkKeybInteractive)
  {
    static const TPuttyTranslation KeybInteractiveInstructionTranslation[] =
    {
      { "Using keyboard-interactive authentication.%", KEYBINTER_INSTRUCTION },
    };
    static const TPuttyTranslation KeybInteractivePromptTranslation[] = {
      // as used by Linux-PAM (pam_exec/pam_exec.c, libpam/pam_get_authtok.c,
      // pam_unix/pam_unix_auth.c, pam_userdb/pam_userdb.c)
      { "Password: ", PASSWORD_PROMPT },
    };

    InstructionTranslation = KeybInteractiveInstructionTranslation;
    PromptTranslation = KeybInteractivePromptTranslation;
    PromptDesc = "keyboard interactive";
  }
  else if (PromptKind == pkPassword)
  {
    DebugAssert(Prompts->GetCount() == 1);
    Prompts->SetString(0, LoadStr(PASSWORD_PROMPT));
    PromptDesc = "password";
  }
  else if (PromptKind == pkNewPassword)
  {
    // Can be tested with WS_FTP server
    static const TPuttyTranslation NewPasswordPromptTranslation[] =
    {
      { "Current password (blank for previously entered password): ", NEW_PASSWORD_CURRENT_PROMPT },
      { "Enter new password: ", NEW_PASSWORD_NEW_PROMPT },
      { "Confirm new password: ", NEW_PASSWORD_CONFIRM_PROMPT },
    };
    PromptTranslation = NewPasswordPromptTranslation;
    PromptTranslationCount = _countof(NewPasswordPromptTranslation);
    PromptDesc = "new password";
  }
  else if (PromptKind == pkProxyAuth)
  {
    static const TPuttyTranslation ProxyAuthPromptTranslation[] = {
      { L"Proxy username: ", PROXY_AUTH_USERNAME_PROMPT },
      { L"Proxy password: ", PROXY_AUTH_PASSWORD_PROMPT },
    };
    PromptTranslation = ProxyAuthPromptTranslation;
    PromptTranslationCount = LENOF(ProxyAuthPromptTranslation);
    PromptDesc = L"proxy authentication";
  }
  else
  {
    PromptDesc = "unknown";
    DebugFail();
  }

  UnicodeString InstructionsLog =
    (AInstructions.IsEmpty() ? UnicodeString("<no instructions>") : FORMAT("\"%s\"", AInstructions));
  UnicodeString PromptsLog =
    (Prompts->GetCount() > 0 ? FORMAT("\"%s\"", Prompts->GetString(0)) : UnicodeString(L"<no prompt>")) +
    (Prompts->GetCount() > 1 ? FORMAT(" + %d more", Prompts->GetCount() - 1) : UnicodeString());
  LogEvent(FORMAT("Prompt (%s, \"%s\", %s, %s)", PromptDesc, AName, InstructionsLog, PromptsLog));

  Name = Name.Trim();

  UnicodeString Instructions = ::ReplaceStrAll(AInstructions, L"\x0D\x0A", L"\x01");
  Instructions = ::ReplaceStrAll(Instructions, L"\x0A\x0D", L"\x01");
  Instructions = ::ReplaceStrAll(Instructions, L"\x0A", L"\x01");
  Instructions = ::ReplaceStrAll(Instructions, L"\x0D", L"\x01");
  Instructions = ::ReplaceStrAll(Instructions, L"\x01", L"\x0D\x0A");
  if (InstructionTranslation != nullptr)
  {
    TranslatePuttyMessage(InstructionTranslation, 1, Instructions);
  }

  // some servers add leading or trailing blank line to make the prompt look prettier
  // on terminal console
  Instructions = Instructions.Trim();

  for (int32_t Index2 = 0; Index2 < Prompts->GetCount(); ++Index2)
  {
    UnicodeString Prompt = Prompts->GetString(Index2);
    if (PromptTranslation != nullptr)
    {
      TranslatePuttyMessage(PromptTranslation, PromptTranslationCount, Prompt);
    }
    // some servers add leading blank line to make the prompt look prettier
    // on terminal console
    Prompts->SetString(Index2, Prompt.Trim());
  }

  bool Result = false;
  if ((PromptKind == pkTIS) || (PromptKind == pkCryptoCard) ||
    (PromptKind == pkKeybInteractive))
  {
    if (FSessionData->GetAuthKIPassword() && !FSessionData->GetPassword().IsEmpty() &&
        !FStoredPasswordTriedForKI && (Prompts->GetCount() == 1) &&
        FLAGCLEAR(nb::ToIntPtr(Prompts->GetObj(0)), pupEcho))
    {
      LogEvent("Using stored password.");
      FUI->Information(LoadStr(AUTH_PASSWORD), false);
      Result = true;
      Results->SetString(0, NormalizeString(FSessionData->GetPassword()));
      FStoredPasswordTriedForKI = true;
    }
    else if (Instructions.IsEmpty() && !InstructionsRequired && (Prompts->GetCount() == 0))
    {
      LogEvent("Ignoring empty SSH server authentication request");
      Result = true;
    }
  }
  else if (PromptKind == pkPassword)
  {
    if (!FSessionData->GetPassword().IsEmpty() && !FStoredPasswordTried)
    {
      LogEvent("Using stored password.");
      FUI->Information(LoadStr(AUTH_PASSWORD), false);
      Result = true;
      Results->SetString(0, NormalizeString(FSessionData->GetPassword()));
      FStoredPasswordTried = true;
    }
  }
  else if (PromptKind == pkPassphrase)
  {
    if (!FSessionData->GetPassphrase().IsEmpty() && !FStoredPassphraseTried)
    {
      LogEvent("Using configured passphrase.");
      Result = true;
      Results->SetString(0, FSessionData->GetPassphrase());
      FStoredPassphraseTried = true;
    }
  }
  else if (PromptKind == pkNewPassword)
  {
    if (FSessionData->GetChangePassword())
    {
      FUI->Information(LoadStr(AUTH_CHANGING_PASSWORD), false);

      if (!FSessionData->GetPassword().IsEmpty() && !FSessionData->GetNewPassword().IsEmpty() && !FStoredPasswordTried)
      {
        LogEvent("Using stored password and new password.");
        Result = true;
        DebugAssert(Results->GetCount() == 3);
        Results->SetString(0, NormalizeString(FSessionData->GetPassword()));
        Results->SetString(1, NormalizeString(FSessionData->GetNewPassword()));
        Results->SetString(2, NormalizeString(FSessionData->GetNewPassword()));
        FStoredPasswordTried = true;
      }
    }
  }

  if (!Result)
  {
    LogEvent(L"Prompting user for the credentials.");
    Result = FUI->PromptUser(FSessionData,
        PromptKind, Name, Instructions, Prompts, Results);

    if (Result)
    {
      LogEvent(L"Prompt cancelled.");
    }
    else
    {
      if ((Prompts->GetCount() >= 1) &&
          (FLAGSET(nb::ToIntPtr(Prompts->GetObj(0)), pupEcho) || GetConfiguration()->GetLogSensitive()))
      {
        LogEvent(FORMAT("Response: \"%s\"", Results->GetString(0)));
      }
      else
      {
        LogEvent("Prompt responded.");
      }

      if ((PromptKind == pkUserName) && (Prompts->GetCount() == 1))
      {
        FUserName = Results->GetString(0);
      }
    }
  }

  return Result;
}

void TSecureShell::GotHostKey()
{
  // due to re-key GotHostKey() may be called again later during session
  if (!FAuthenticating && !FAuthenticated)
  {
    FAuthenticating = true;
    if (!FSessionData->GetChangePassword())
    {
      FUI->Information(LoadStr(STATUS_AUTHENTICATE), true);
    }
  }
}

void TSecureShell::CWrite(const char * Data, size_t Length)
{
  // some messages to stderr may indicate that something has changed with the
  // session, so reset the session info
  ResetSessionInfo();

  // We send only whole line at once, so we have to cache incoming data
  FCWriteTemp += DeleteChar(ConvertFromPutty(Data, Length), L'\r');

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

void TSecureShell::RegisterReceiveHandler(TNotifyEvent Handler)
{
  DebugAssert(FOnReceive == nullptr);
  FOnReceive = Handler;
}

void TSecureShell::UnregisterReceiveHandler(TNotifyEvent Handler)
{
  DebugAssert(FOnReceive == Handler);
  DebugUsedParam(Handler);
  FOnReceive = nullptr;
}

void TSecureShell::FromBackend(const uint8_t * Data, size_t Length)
{
  // Note that we do not apply ConvertFromPutty to Data yet (as opposite to CWrite).
  // as there's no use for this atm.
  CheckConnection();

  if (GetConfiguration()->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT("Received %u bytes", Length));
  }

  // Following is taken from scp.c from_backend() and modified

  const unsigned char *p = Data;
  unsigned Len = Length;

  // with event-select mechanism we can now receive data even before we
  // actually expect them (OutPtr can be nullptr)

  if ((OutPtr != nullptr) && (OutLen > 0) && (Len > 0))
  {
    unsigned Used = OutLen;
    if (Used > Len) Used = Len;
    memmove(OutPtr, p, Used);
    OutPtr += Used; OutLen -= Used;
    p += Used; Len -= Used;
  }

  if (Len > 0)
  {
    if (PendSize < PendLen + Len)
    {
      PendSize = PendLen + Len + 4096;
      Pending = (unsigned char *)
        (Pending ? srealloc(Pending, PendSize) : smalloc(PendSize));
      if (!Pending) FatalError(L"Out of memory");
    }
    memmove(Pending + PendLen, p, Len);
    PendLen += Len;
  }

  if (FOnReceive != nullptr)
  {
    if (!FFrozen)
    {
      FFrozen = true;
      try
      {
        do
        {
          FDataWhileFrozen = false;
          FOnReceive(nullptr);
        }
        while (FDataWhileFrozen);
      }
      __finally
      {
        FFrozen = false;
      }
    }
    else
    {
      FDataWhileFrozen = true;
    }
  }
}

bool TSecureShell::Peek(uint8_t *& Buf, int32_t Len) const
{
  bool Result = (PendLen >= Len);

  if (Result)
  {
    Buf = Pending;
  }

  return Result;
}

int32_t TSecureShell::Receive(uint8_t * Buf, int32_t Len)
{
  CheckConnection();

  if (Length > 0)
  {
    // Following is taken from scp.c ssh_scp_recv() and modified

    OutPtr = Buf;
    OutLen = Length;

    try__finally
    {
      /*
       * See if the pending-input block contains some of what we
       * need.
       */
      if (PendLen > 0)
      {
        int32_t PendUsed = PendLen;
        if (PendUsed > OutLen)
        {
          PendUsed = OutLen;
        }
        memmove(OutPtr, Pending, PendUsed); //-V575
        memmove(Pending, Pending + PendUsed, PendLen - PendUsed);
        OutPtr += PendUsed;
        OutLen -= PendUsed;
        PendLen -= PendUsed;
        if (PendLen == 0)
        {
          PendSize = 0;
          sfree(Pending);
          Pending = nullptr;
        }
      }

      while (OutLen > 0)
      {
        if (GetConfiguration()->GetActualLogProtocol() >= 1)
        {
          LogEvent(FORMAT("Waiting for another %u bytes", nb::ToInt(OutLen)));
        }
        WaitForData();
      }

      // This seems ambiguous
#if 0
      if (Length <= 0) { FatalError(LoadStr(LOST_CONNECTION)); }
#endif // #if 0
    },
    __finally
    {
      OutPtr = nullptr;
    } end_try__finally
  }
  if (GetConfiguration()->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT("Read %d bytes (%d pending)",
        nb::ToInt(Length), nb::ToInt(PendLen)));
  }
  return Length;
}

UnicodeString TSecureShell::ReceiveLine()
{
  RawByteString Line;
  Boolean EOL = False;

  do
  {
    // If there is any buffer of received chars
    if (PendLen > 0)
    {
      uint32_t Index = 0;
      // Repeat until we walk thru whole buffer or reach end-of-line
      while ((Index < PendLen) && (!Index || (Pending[Index - 1] != '\n')))
      {
        ++Index;
      }
      EOL = static_cast<Boolean>(Index && (Pending[Index - 1] == '\n'));
      int32_t PrevLen = Line.Length();
      char *Buf = Line.SetLength(PrevLen + Index);
      Receive(reinterpret_cast<uint8_t *>(Buf + PrevLen), Index);
    }

    // If buffer don't contain end-of-line character
    // we read one more which causes receiving new buffer of chars
    if (!EOL)
    {
      uint8_t Ch;
      Receive(&Ch, 1);
      Line += Ch;
      EOL = (Ch == '\n');
    }
  }
  while (!EOL);

  // We don't want end-of-line character
  Line.SetLength(Line.Length() - 1);

  UnicodeString Result = ConvertInput(Line, FSessionData->GetCodePageAsNumber());
  CaptureOutput(llOutput, Result);

  return Result;
}

UnicodeString TSecureShell::ConvertInput(const RawByteString Input, uint32_t CodePage) const
{
  UnicodeString Result;
  if (GetUtfStrings())
  {
    Result = UnicodeString(UTF8String(Input.c_str(), Input.GetLength()));
  }
  else
  {
//    Result = UnicodeString(AnsiString(Input.c_str(), Input.GetLength()));
    Result = ::MB2W(Input.c_str(), static_cast<UINT>(CodePage));
  }
  return Result;
}

void TSecureShell::SendSpecial(int32_t Code)
{
  if (GetConfiguration()->ActualLogProtocol >= 0)
  {
    LogEvent(FORMAT("Sending special code: %d", Code));
  }
  CheckConnection();
  backend_special(FBackendHandle, static_cast<SessionSpecialCode>(Code), 0);
  CheckConnection();
  FLastDataSent = Now();
}

uint32_t TSecureShell::TimeoutPrompt(TQueryParamsTimerEvent PoolEvent)
{
  ++FWaiting;

  uint32_t Answer;
  try__finally
  {
    TQueryParams Params(qpFatalAbort | qpAllowContinueOnError | qpIgnoreAbort);
    Params.HelpKeyword = HELP_MESSAGE_HOST_IS_NOT_COMMUNICATING;
    Params.Timer = 500;
    Params.TimerEvent = PoolEvent;
    Params.TimerMessage = MainInstructionsFirstParagraph(FMTLOAD(TIMEOUT_STILL_WAITING3, FSessionData->GetTimeout()));
    Params.TimerAnswers = qaAbort;
    Params.TimerQueryType = qtInformation;
    if (FConfiguration->GetSessionReopenAutoStall() > 0)
    {
      Params.Timeout = FConfiguration->GetSessionReopenAutoStall();
      Params.TimeoutAnswer = qaAbort;
      Params.TimeoutResponse = qaNo;
    }
    Answer = FUI->QueryUser(MainInstructions(FMTLOAD(CONFIRM_PROLONG_TIMEOUT3, FSessionData->GetTimeout(), FSessionData->GetTimeout())),
      nullptr, qaRetry | qaAbort, &Params);
  },
  __finally
  {
    FWaiting--;
  } end_try__finally
  return Answer;
}

void TSecureShell::SendBuffer(uint32_t & Result)
{
  // for comments see PoolForData
  if (!GetActive())
  {
    Result = qaRetry;
  }
  else
  {
    try
    {
      if (backend_sendbuffer(FBackendHandle) <= MAX_BUFSIZE)
      {
        Result = qaOK;
      }
    }
    catch (...)
    {
      Result = qaRetry;
    }
  }
}

void TSecureShell::TimeoutAbort(uint32_t Answer)
{
  FatalError(MainInstructions(LoadStr(Answer == qaAbort ? USER_TERMINATED : TIMEOUT_ERROR)));
}

void TSecureShell::DispatchSendBuffer(int32_t BufSize)
{
  TDateTime Start = Now();
  do
  {
    CheckConnection();
    if (GetConfiguration()->GetActualLogProtocol() >= 1)
    {
      LogEvent(FORMAT("There are %u bytes remaining in the send buffer, "
        "need to send at least another %u bytes",
        BufSize, BufSize - MAX_BUFSIZE));
    }
    EventSelectLoop(100, false, nullptr);
    BufSize = backend_sendbuffer(FBackendHandle);
    if (GetConfiguration()->GetActualLogProtocol() >= 1)
    {
      LogEvent(FORMAT("There are %u bytes remaining in the send buffer", BufSize));
    }

    if (Now() - Start > FSessionData->GetTimeoutDT())
    {
      LogEvent("Waiting for dispatching send buffer timed out, asking user what to do.");
      uint32_t Answer = TimeoutPrompt(nb::bind(&TSecureShell::SendBuffer, this));
      switch (Answer)
      {
        case qaRetry:
          Start = Now();
          break;

        case qaOK:
          BufSize = 0;
          break;

        default:
          DebugFail();
          // fallthru

        case qaAbort:
        case qaNo:
          TimeoutAbort(Answer);
          break;
      }
    }
  }
  while (BufSize > MAX_BUFSIZE);
}

void TSecureShell::Send(const uint8_t *Buf, int32_t Length)
{
  CheckConnection();
  backend_send(FBackendHandle, const_cast<char *>(reinterpret_cast<const char *>(Buf)), nb::ToInt(Length));
  int32_t BufSize = backend_sendbuffer(FBackendHandle);
  if (GetConfiguration()->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT("Sent %d bytes", nb::ToInt(Length)));
    LogEvent(FORMAT("There are %u bytes remaining in the send buffer", BufSize));
  }
  FLastDataSent = Now();
  // among other forces receive of pending data to free the servers's send buffer
  EventSelectLoop(0, false, nullptr);

  if (BufSize > MAX_BUFSIZE)
  {
    DispatchSendBuffer(BufSize);
  }
  CheckConnection();
}

void TSecureShell::SendNull()
{
  LogEvent("Sending nullptr.");
  uint8_t Null = 0;
  Send(&Null, 1);
}

void TSecureShell::SendLine(const UnicodeString & Line)
{
  CheckConnection();
  RawByteString Str;
  if (GetUtfStrings())
  {
    Str = RawByteString(UTF8String(Line));
  }
  else
  {
    Str = RawByteString(AnsiString(::W2MB(Line.c_str(), static_cast<UINT>(FSessionData->GetCodePageAsNumber()))));
  }
  Str += "\n";

  // FLog->Add(llInput, Line);
  Send(reinterpret_cast<const uint8_t *>(Str.c_str()), Str.Length());
}

int TSecureShell::TranslatePuttyMessage(
  const TPuttyTranslation *Translation, int32_t Count, UnicodeString &Message,
  UnicodeString *HelpKeyword) const
{
  int Result = -1;
  for (int32_t Index = 0; Index < Count; ++Index)
  {
    const char * Original = Translation[Index].Original;
    const char * Div = strchr(Original, '%');
    AnsiString AnsiMessage = AnsiString(Message);
    if (Div == nullptr)
    {
      if (strcmp(AnsiMessage.c_str(), Original) == 0)
      {
        Message = LoadStr(Translation[Index].Translation);
        Result = nb::ToInt(Index);
        break;
      }
    }
    else
    {
      size_t OriginalLen = nb::StrLength(Original);
      size_t PrefixLen = Div - Original;
      size_t SuffixLen = OriginalLen - PrefixLen - 1;
      if ((nb::ToSizeT(Message.Length()) >= OriginalLen - 1) &&
          (strncmp(AnsiMessage.c_str(), Original, PrefixLen) == 0) &&
          (strncmp(AnsiMessage.c_str() + AnsiMessage.Length() - SuffixLen, Div + 1, SuffixLen) == 0))
      {
        Message = FMTLOAD(Translation[Index].Translation,
          Message.SubString(PrefixLen + 1, Message.Length() - PrefixLen - SuffixLen).TrimRight());
        Result = nb::ToInt(Index);
        break;
      }
    }
  }

  if ((HelpKeyword != nullptr) && (Result >= 0))
  {
    *HelpKeyword = Translation[Result].HelpKeyword;
  }

  return Result;
}

int TSecureShell::TranslateAuthenticationMessage(
  UnicodeString & Message, UnicodeString * HelpKeyword)
{
  static const TPuttyTranslation Translation[] = {
    { "Using username \"%\".", AUTH_TRANSL_USERNAME },
    { "Using keyboard-interactive authentication.", AUTH_TRANSL_KEYB_INTER }, // not used anymore
    { "Authenticating with public key \"%\" from agent", AUTH_TRANSL_PUBLIC_KEY_AGENT },
    { "Authenticating with public key \"%\"", AUTH_TRANSL_PUBLIC_KEY },
    { "Authenticated using RSA key \"%\" from agent", AUTH_TRANSL_PUBLIC_KEY_AGENT },
    { "Wrong passphrase", AUTH_TRANSL_WRONG_PASSPHRASE },
    { "Wrong passphrase.", AUTH_TRANSL_WRONG_PASSPHRASE },
    { "Access denied", AUTH_TRANSL_ACCESS_DENIED },
    { "Trying public key authentication.", AUTH_TRANSL_TRY_PUBLIC_KEY },
    { "Server refused our public key.", AUTH_TRANSL_KEY_REFUSED, HELP_AUTH_TRANSL_KEY_REFUSED },
    { "Server refused our key", AUTH_TRANSL_KEY_REFUSED, HELP_AUTH_TRANSL_KEY_REFUSED },
  };

  int Result = TranslatePuttyMessage(Translation, _countof(Translation), Message, HelpKeyword);

  if ((Result == 2) || (Result == 3) || (Result == 4))
  {
    // GetConfiguration()->GetUsage()->Inc("OpenedSessionsPrivateKey2");
    FCollectPrivateKeyUsage = true;
  }

  return Result;
}

void TSecureShell::AddStdError(const char * Data, size_t Length)
{
  UnicodeString Str = ConvertInput(RawByteString(Data, Length));
  FStdError += Str;

  int32_t P;
  Str = DeleteChar(Str, L'\r');
  // We send only whole line at once to log, so we have to cache
  // incoming std error data
  FStdErrorTemp += Str;
  // Do we have at least one complete line in std error cache?
  while ((P = FStdErrorTemp.Pos(L"\n")) > 0)
  {
    UnicodeString Line = FStdErrorTemp.SubString(1, P - 1);
    FStdErrorTemp.Delete(1, P);
    AddStdErrorLine(Line);
  }
}

void TSecureShell::AddStdErrorLine(const UnicodeString & AStr)
{
  UnicodeString Str = AStr.Trim();
  if (FAuthenticating)
  {
    FAuthenticationLog += (FAuthenticationLog.IsEmpty() ? L"" : L"\n") + Str;
  }
  if (!Str.IsEmpty())
  {
    CaptureOutput(llStdError, Str);
  }
}

const UnicodeString & TSecureShell::GetStdError() const
{
  return FStdError;
}

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
    FStdErrorTemp.Clear();
  }
  FStdError.Clear();
}

void TSecureShell::CaptureOutput(TLogLineType Type,
  const UnicodeString & Line)
{
  if (FOnCaptureOutput != nullptr)
  {
    FOnCaptureOutput(Line, (Type == llStdError) ? cotError : cotOutput);
  }
  FLog->Add(Type, Line);
}

int TSecureShell::TranslateErrorMessage(
  UnicodeString &Message, UnicodeString *HelpKeyword)
{
  static const TPuttyTranslation Translation[] = {
    { "Remote side unexpectedly closed network connection", UNEXPECTED_CLOSE_ERROR, HELP_UNEXPECTED_CLOSE_ERROR },
    { "Network error: Connection refused", NET_TRANSL_REFUSED2, HELP_NET_TRANSL_REFUSED },
    { "Network error: Connection reset by peer", NET_TRANSL_RESET, HELP_NET_TRANSL_RESET },
    { "Network error: Connection timed out", NET_TRANSL_TIMEOUT2, HELP_NET_TRANSL_TIMEOUT },
    { "Network error: No route to host", NET_TRANSL_NO_ROUTE2, HELP_NET_TRANSL_NO_ROUTE },
    { "Network error: Software caused connection abort", NET_TRANSL_CONN_ABORTED, HELP_NET_TRANSL_CONN_ABORTED },
    { "Host does not exist", NET_TRANSL_HOST_NOT_EXIST2, HELP_NET_TRANSL_HOST_NOT_EXIST },
    { "Incoming packet was garbled on decryption", NET_TRANSL_PACKET_GARBLED, HELP_NET_TRANSL_PACKET_GARBLED },
  };

  int Index = TranslatePuttyMessage(Translation, _countof(Translation), Message, HelpKeyword);

  if ((Index == 0) || (Index == 1) || (Index == 2) || (Index == 3))
  {
    FNoConnectionResponse = true;
  }

  Message = ReplaceStr(Message, "%HOST%", FSessionData->GetHostNameExpanded());

  return Index;
}

void TSecureShell::PuttyFatalError(UnicodeString AError)
{
  UnicodeString Error = AError;
  UnicodeString HelpKeyword;
  TranslateErrorMessage(Error, &HelpKeyword);
  if (!FClosed)
  {
    FatalError(Error, HelpKeyword);
  }
  else
  {
    LogEvent(FORMAT(L"Ignoring an error from the server while closing: %s", Error));
  }
}

void TSecureShell::FatalError(UnicodeString Error, UnicodeString HelpKeyword)
{
  FUI->FatalError(nullptr, Error, HelpKeyword);
}

void TSecureShell::LogEvent(const UnicodeString & AStr)
{
  if (FLog->GetLogging())
  {
    FLog->Add(llMessage, AStr);
  }
}

void TSecureShell::SocketEventSelect(SOCKET Socket, HANDLE Event, bool Enable)
{
  int Events;

  if (Enable)
  {
    Events = (FD_CONNECT | FD_READ | FD_WRITE | FD_OOB | FD_CLOSE | FD_ACCEPT);
  }
  else
  {
    Events = 0;
  }

  if (GetConfiguration()->GetActualLogProtocol() >= 2)
  {
    LogEvent(FORMAT("Selecting events %d for socket %d", nb::ToInt(Events), nb::ToInt(Socket)));
  }

  if (::WSAEventSelect(Socket, static_cast<WSAEVENT>(Event), Events) == SOCKET_ERROR)
  {
    if (GetConfiguration()->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT("Error selecting events %d for socket %d", nb::ToInt(Events), nb::ToInt(Socket)));
    }

    if (Enable)
    {
      FatalError(FMTLOAD(EVENT_SELECT_ERROR, ::WSAGetLastError()));
    }
  }
}

bool TSecureShell::HasLocalProxy() const
{
  return (FSessionData->ProxyMethod == pmCmd);
}

void TSecureShell::UpdateSocket(SOCKET Value, bool Enable)
{
  if (!FActive && !Enable)
  {
    // no-op
    // Remove the branch eventually:
    // When TCP connection fails, PuTTY does not release the memory allocated for
    // socket. As a simple hack we call sk_tcp_close() in ssh.c to release the memory,
    // until they fix it better. Unfortunately sk_tcp_close calls do_select,
    // so we must filter that out.
  }
  else
  {
    DebugAssert(Value);
    DebugAssert((FActive && (FSocket == Value)) || (!FActive && Enable));

    // filter our "local proxy" connection, which have no socket
    if (Value != INVALID_SOCKET)
    {
      SocketEventSelect(Value, FSocketEvent, Enable);
    }
    else
    {
      DebugAssert(HasLocalProxy());
    }

    if (Enable)
    {
      FSocket = Value;
      FActive = true;
    }
    else
    {
      FSocket = INVALID_SOCKET;
      Discard();
    }
  }
}

void TSecureShell::UpdatePortFwdSocket(SOCKET Value, bool Enable)
{
  if (value != INVALID_SOCKET)
  {
    if (GetConfiguration()->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT("Updating forwarding socket %d (%d)", nb::ToInt(Value), nb::ToInt(Startup)));
    }

    SocketEventSelect(Value, FSocketEvent, Startup);

    if (Enable)
    {
      FPortFwdSockets.push_back(Value);
    }
    else
    {
      TSockets::iterator it = FPortFwdSockets.find(Value);
      if (it != FPortFwdSockets.end())
        FPortFwdSockets.erase(it);
    }
  }
  else
  {
    DebugAssert(FSessionData->AgentFwd);
  }
}

void TSecureShell::FreeBackend()
{
  if (FBackendHandle != nullptr)
  {
    backend_free(FBackendHandle);
    FBackendHandle = nullptr;

    // After destroying backend, ic_pktin_free should be the only remaining callback.
    if (is_idempotent_callback_pending(FCallbackSet.get(), FCallbackSet->ic_pktin_free))
    {
      // This releases the callback and should be noop otherwise.
      run_toplevel_callbacks(FCallbackSet.get());
    }

    sfree(FCallbackSet->ic_pktin_free);
    FCallbackSet->ic_pktin_free = nullptr;

    // Not checking that cbcurr is nullptr. It may be non-null, when (fatal?) exception occurs, while the callback is called.
    FCallbackSet->cbcurr = nullptr;
    DebugAssert(FCallbackSet->cbhead == nullptr);
    DebugAssert(FCallbackSet->cbtail == nullptr);

    if (FCallbackSet->pktin_freeq_head != nullptr)
    {
      DebugAssert(FCallbackSet->pktin_freeq_head->next == FCallbackSet->pktin_freeq_head);
      sfree(FCallbackSet->pktin_freeq_head);
      FCallbackSet->pktin_freeq_head = nullptr;
    }

    if (FCallbackSet->handlewaits_tree_real != nullptr)
    {
      DebugAssert(count234(FCallbackSet->handlewaits_tree_real) <= 1);
      while (count234(FCallbackSet->handlewaits_tree_real) > 0)
      {
        HandleWait * AHandleWait = static_cast<HandleWait *>(index234(FCallbackSet->handlewaits_tree_real, 0));
        delete_handle_wait(FCallbackSet.get(), AHandleWait);
      }

      freetree234(FCallbackSet->handlewaits_tree_real);
      FCallbackSet->handlewaits_tree_real = nullptr;
    }

    if (FCallbackSet->ready_event != INVALID_HANDLE_VALUE)
    {
      CloseHandle(FCallbackSet->ready_event);
      FCallbackSet->ready_event = INVALID_HANDLE_VALUE;
    }
  }
}

void TSecureShell::Discard()
{
  bool WasActive = FActive;
  FActive = false;
  FOpened = false;

  if (WasActive)
  {
    FUI->Closed();
  }
}

void TSecureShell::Close()
{
  LogEvent("Closing connection.");
  DebugAssert(FActive);
  FClosed = true;

  // Without main channel SS_EOF is ignored and would get stuck waiting for exit code.
  if ((backend_exitcode(FBackendHandle) < 0) && winscp_query(FBackendHandle, WINSCP_QUERY_MAIN_CHANNEL))
  {
    // this is particularly necessary when using local proxy command
    // (e.g. plink), otherwise it hangs in sk_localproxy_close
    SendSpecial(SS_EOF);
    // Try waiting for the EOF exchange to complete (among other to avoid packet queue memory leaks)
    int Timeout = 500;
    while ((backend_exitcode(FBackendHandle) < 0) && (Timeout > 0))
    {
      const int Step = 100;
      if (!EventSelectLoop(Step, false, nullptr))
      {
        Timeout -= Step;
      }
    }
  }

  FreeBackend();

  Discard();
}

void inline TSecureShell::CheckConnection(int Message)
{
  if (!FActive || (backend_exitcode(FBackendHandle) >= 0))
  {
    UnicodeString Str;
    UnicodeString HelpKeyword;

    if (Message >= 0)
    {
      Str = LoadStr(Message);
    }
    else
    {
      Str = LoadStr(NOT_CONNECTED);
      HelpKeyword = HELP_NOT_CONNECTED;
    }

    Str = MainInstructions(Str);

    int ExitCode = backend_exitcode(FBackendHandle);
    if (ExitCode >= 0)
    {
      Str += L" " + FMTLOAD(SSH_EXITCODE, ExitCode);
    }
    FatalError(Str, HelpKeyword);
  }
}

void TSecureShell::PoolForData(WSANETWORKEVENTS & Events, uint32_t &Result)
{
  if (!GetActive())
  {
    // see comment below
    Result = qaRetry;
  }
  else
  {
    try
    {
      if (GetConfiguration()->GetActualLogProtocol() >= 2)
      {
        LogEvent("Pooling for data in case they finally arrives");
      }

      // in extreme condition it may happen that send buffer is full, but there
      // will be no data coming and we may not empty the send buffer because we
      // do not process FD_WRITE until we receive any FD_READ
      if (EventSelectLoop(0, false, &Events))
      {
        LogEvent("Data has arrived, closing query to user.");
        Result = qaOK;
      }
    }
    catch (...)
    {
      // if we let the exception out, it may popup another message dialog
      // in whole event loop, another call to PoolForData from original dialog
      // would be invoked, leading to an infinite loop.
      // by retrying we hope (that probably fatal) error would repeat in WaitForData.
      // anyway now once no actual work is done in EventSelectLoop,
      // hardly any exception can occur actually
      Result = qaRetry;
    }
  }
}

class TPoolForDataEvent : public TObject
{
  NB_DISABLE_COPY(TPoolForDataEvent)
public:
  TPoolForDataEvent(TSecureShell *SecureShell, WSANETWORKEVENTS &Events) noexcept :
    FSecureShell(SecureShell),
    FEvents(Events)
  {
  }

  void PoolForData(uint32_t & Result)
  {
    FSecureShell->PoolForData(FEvents, Result);
  }

private:
  TSecureShell * FSecureShell{nullptr};
  WSANETWORKEVENTS & FEvents;
};

void TSecureShell::WaitForData()
{
  // see winsftp.c
  bool IncomingData;

  do
  {
    if (GetConfiguration()->GetActualLogProtocol() >= 2)
    {
      LogEvent("Looking for incoming data");
    }

    IncomingData = EventSelectLoop(FSessionData->GetTimeout() * MSecsPerSec, true, nullptr);
    if (!IncomingData)
    {
      DebugAssert(FWaitingForData == 0);
      TAutoNestingCounter NestingCounter(FWaitingForData); nb::used(NestingCounter);

      WSANETWORKEVENTS Events;
      nb::ClearStruct(Events);
      TPoolForDataEvent Event(this, Events);

      LogEvent("Waiting for data timed out, asking user what to do.");
      uint32_t Answer = TimeoutPrompt(nb::bind(&TPoolForDataEvent::PoolForData, &Event));
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
          DebugFail();
          // fallthru

        case qaAbort:
        case qaNo:
          TimeoutAbort(Answer);
          break;
      }
    }
  }
  while (!IncomingData);
}

bool TSecureShell::SshFallbackCmd() const
{
  return ssh_fallback_cmd(FBackendHandle) != 0;
}

bool TSecureShell::EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS &Events)
{
  if (GetConfiguration()->GetActualLogProtocol() >= 2)
  {
    LogEvent(FORMAT("Enumerating network events for socket %d", nb::ToInt(Socket)));
  }

  // see winplink.c
  WSANETWORKEVENTS AEvents;
  if (::WSAEnumNetworkEvents(Socket, nullptr, &AEvents) == 0)
  {
    noise_ultralight(NOISE_SOURCE_IOID, nb::ToUInt32(Socket));
    noise_ultralight(NOISE_SOURCE_IOID, AEvents.lNetworkEvents);

    Events.lNetworkEvents |= AEvents.lNetworkEvents;
    for (int32_t Index = 0; Index < FD_MAX_EVENTS; ++Index)
    {
      if (AEvents.iErrorCode[Index] != 0)
      {
        Events.iErrorCode[Index] = AEvents.iErrorCode[Index];
      }
    }

    if (GetConfiguration()->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT("Enumerated %d network events making %d cumulative events for socket %d",
        nb::ToInt(AEvents.lNetworkEvents), nb::ToInt(Events.lNetworkEvents), nb::ToInt(Socket)));
    }
  }
  else
  {
    if (GetConfiguration()->GetActualLogProtocol() >= 2)
    {
      LogEvent(FORMAT("Error enumerating network events for socket %d", nb::ToInt(Socket)));
    }
  }

  bool Result =
    FLAGSET(Events.lNetworkEvents, FD_READ) ||
    FLAGSET(Events.lNetworkEvents, FD_CLOSE);
  return Result;
}

void TSecureShell::HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS &Events)
{
  static const struct { int Bit, Mask; const wchar_t * Desc; } EventTypes[] =
  {
    { FD_WRITE_BIT, FD_WRITE, L"write" },
    { FD_OOB_BIT, FD_OOB, L"oob" },
    { FD_ACCEPT_BIT, FD_ACCEPT, L"accept" },
    { FD_CONNECT_BIT, FD_CONNECT, L"connect" },
    { FD_CLOSE_BIT, FD_CLOSE, L"close" },
    // Read goes last, as it can cause an exception.
    // Though a correct solution would be to process all events, even if one causes exception
    { FD_READ_BIT, FD_READ, L"read" },
  };

  for (uint32_t Event = 0; Event < _countof(EventTypes); Event++)
  {
    if (FLAGSET(Events.lNetworkEvents, EventTypes[Event].Mask))
    {
      int Err = Events.iErrorCode[EventTypes[Event].Bit];

      if (GetConfiguration()->GetActualLogProtocol() >= 2)
      {
        LogEvent(FORMAT("Handling network %s event on socket %d with error %d",
          EventTypes[Event].Desc, int(Socket), Err));
      }
      __removed #pragma option push -w-prc
      LPARAM SelectEvent = WSAMAKESELECTREPLY(EventTypes[Event].Mask, Err);
      __removed #pragma option pop
      select_result(static_cast<WPARAM>(Socket), SelectEvent);
      CheckConnection();
    }
  }
}

bool TSecureShell::ProcessNetworkEvents(SOCKET Socket)
{
  WSANETWORKEVENTS Events;
  nb::ClearStruct(Events);
  bool Result = EnumNetworkEvents(Socket, Events);
  HandleNetworkEvents(Socket, Events);
  return Result;
}

bool TSecureShell::EventSelectLoop(uint32_t MSec, bool ReadEventRequired,
  WSANETWORKEVENTS * Events)
{
  bool Result = false;

  do
  {
    if (GetConfiguration()->GetActualLogProtocol() >= 2)
    {
#if 0
      LogEvent("Looking for network events");
#endif // #if 0
    }
    uint32_t TicksBefore = ::GetTickCount();
    HandleWaitList * WaitList = nullptr;

    try__finally
    {
      unsigned int Timeout = MSec;

      unsigned int WaitResult;
      do
      {
        CheckConnection();
        unsigned int TimeoutStep = std::min(GUIUpdateInterval, Timeout);
        if (toplevel_callback_pending(GetCallbackSet()))
        {
          TimeoutStep = 0;
        }
        Timeout -= TimeoutStep;
        if (WaitList != nullptr)
        {
          handle_wait_list_free(WaitList);
        }
        // It returns only busy handles, so the set can change with every call to run_toplevel_callbacks.
        WaitList = get_handle_wait_list(FCallbackSet.get());
        DebugAssert(WaitList->nhandles < MAXIMUM_WAIT_OBJECTS);
        WaitList->handles[WaitList->nhandles] = FSocketEvent;
        WaitResult = WaitForMultipleObjects(WaitList->nhandles + 1, WaitList->handles, FALSE, TimeoutStep);
        FUI->ProcessGUI();
        // run_toplevel_callbacks can cause processing of pending raw data, so:
        // 1) Check for changes in our pending buffer - wait criteria in Receive()
        int PrevDataLen = (-static_cast<int>(OutLen) + static_cast<int>(PendLen));
        // 2) Changes in session state - wait criteria in Init()
        unsigned int HadMainChannel = winscp_query(FBackendHandle, WINSCP_QUERY_MAIN_CHANNEL);
        if (run_toplevel_callbacks(GetCallbackSet()) &&
            (((-static_cast<int>(OutLen) + static_cast<int>(PendLen)) > PrevDataLen) ||
             (HadMainChannel != winscp_query(FBackendHandle, WINSCP_QUERY_MAIN_CHANNEL))))
        {
          // Note that we still may process new network event now
          Result = true;
        }
      } while ((WaitResult == WAIT_TIMEOUT) && (Timeout > 0) && !Result);

      if (WaitResult < WAIT_OBJECT_0 + WaitList->nhandles)
      {
        if (handle_wait_activate(FCallbackSet.get(), WaitList, WaitResult - WAIT_OBJECT_0))
        {
          Result = true;
        }
      }
      else if (WaitResult == WAIT_OBJECT_0 + WaitList->nhandles)
      {
        if (GetConfiguration()->GetActualLogProtocol() >= 1)
        {
          LogEvent("Detected network event");
        }

        DebugAssert(FSocket != INVALID_SOCKET);
        if (Events == nullptr)
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
          TSockets::iterator it = FPortFwdSockets.begin();
          while (it != FPortFwdSockets.end())
          {
            ProcessNetworkEvents(*it);
            ++it;
          }
        }
      }
      else if (WaitResult == WAIT_TIMEOUT)
      {
        if (GetConfiguration()->GetActualLogProtocol() >= 2)
        {
#if 0
          LogEvent("Timeout waiting for network events");
#endif // #if 0
        }

        MSec = 0;
      }
      else
      {
        if (GetConfiguration()->GetActualLogProtocol() >= 2)
        {
          LogEvent(FORMAT("Unknown waiting result %d", nb::ToInt(WaitResult)));
        }

        MSec = 0;
      }
    },
    __finally
    {
      if (WaitList != nullptr)
      {
        handle_wait_list_free(WaitList);
      }
    } end_try__finally


    uint32_t TicksAfter = ::GetTickCount();
    // ticks wraps once in 49.7 days
    if (TicksBefore < TicksAfter)
    {
      uint32_t Ticks = TicksAfter - TicksBefore;
      if (Ticks > MSec)
      {
        MSec = 0;
      }
      else
      {
        MSec -= Ticks;
      }
    }

    if ((FSocket != INVALID_SOCKET) &&
        (FSendBuf > 0) && (TicksAfter - FLastSendBufferUpdate >= 1000))
    {
      DWORD BufferLen = 0;
      DWORD OutBuffLen = 0;
      if (::WSAIoctl(FSocket, SIO_IDEAL_SEND_BACKLOG_QUERY, nullptr, 0, &BufferLen, sizeof(BufferLen), &OutBuffLen, nullptr, nullptr) == 0)
      {
        DebugAssert(OutBuffLen == sizeof(BufferLen));
        if (FSendBuf < nb::ToIntPtr(BufferLen))
        {
          LogEvent(FORMAT("Increasing send buffer from %d to %d", FSendBuf, nb::ToInt(BufferLen)));
          FSendBuf = BufferLen;
          setsockopt(FSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char *>(&BufferLen), sizeof(BufferLen));
        }
      }
      FLastSendBufferUpdate = nb::ToDWord(TicksAfter);
    }
  }
  while (ReadEventRequired && (MSec > 0) && !Result);

  return Result;
}

void TSecureShell::Idle(uint32_t MSec)
{
  noise_regular();

  winscp_query(FBackendHandle, WINSCP_QUERY_TIMER);

  // if we are actively waiting for data in WaitForData,
  // do not read here, otherwise we swallow read event and never wake
  if (FWaitingForData <= 0)
  {
    EventSelectLoop(MSec, false, nullptr);
  }
}

void TSecureShell::KeepAlive()
{
  if (FActive && (FWaiting == 0))
  {
    LogEvent("Sending null packet to keep session alive.");
    SendSpecial(SS_PING);
  }
  else
  {
    // defer next keepalive attempt
    FLastDataSent = Now();
  }
}

constexpr uint32_t minPacketSize = 0;

uint32_t TSecureShell::MinPacketSize() const
{
  if (!FSessionInfoValid)
  {
    UpdateSessionInfo();
  }

  return winscp_query(FBackendHandle, WINSCP_QUERY_REMMAXPKT);
}

UnicodeString TSecureShell::FormatKeyStr(UnicodeString KeyStr)
{
  int Index = 1;
  int Digits = 0;
  while (Index <= KeyStr.Length())
  {
    if (IsHex(KeyStr[Index]))
    {
      Digits++;
      if (Digits >= 16)
      {
        KeyStr.Insert(L" ", Index + 1);
        Index++;
        Digits = 0;
      }
    }
    else
    {
      Digits = 0;
    }
    Index++;
  }
  return KeyStr;
}

void TSecureShell::GetRealHost(UnicodeString & Host, int & Port)
{
  if (FSessionData->Tunnel)
  {
    // Now that we set the CONF_loghost, the hostname is correct already
    Host = FSessionData->OrigHostName;
    Port = FSessionData->OrigPortNumber;
  }
}

bool TSecureShell::HaveAcceptNewHostKeyPolicy()
{
  return SameText(FSessionData->HostKey.Trim(), L"acceptnew");
}

THierarchicalStorage * TSecureShell::GetHostKeyStorage()
{
  if (!Configuration->Persistent && HaveAcceptNewHostKeyPolicy())
  {
    return Configuration->CreateConfigRegistryStorage();
  }
  else
  {
    return Configuration->CreateConfigStorage();
  }
}

UnicodeString TSecureShell::RetrieveHostKey(const UnicodeString & Host, int Port, const UnicodeString & KeyType)
{
  std::unique_ptr<THierarchicalStorage> Storage(GetHostKeyStorage());
  Storage->AccessMode = smRead;
  TGuard Guard(PuttyStorageSection.get());
  DebugAssert(PuttyStorage == nullptr);
  TValueRestorer<THierarchicalStorage *> StorageRestorer(PuttyStorage);
  PuttyStorage = Storage.get();

  AnsiString AnsiStoredKeys;
  AnsiStoredKeys.SetLength(10240);
  UnicodeString Result;
  if (retrieve_host_key(AnsiString(Host).c_str(), Port, AnsiString(KeyType).c_str(),
        AnsiStoredKeys.c_str(), AnsiStoredKeys.Length()) == 0)
  {
    PackStr(AnsiStoredKeys);
    Result = UnicodeString(AnsiStoredKeys);
  }
  return Result;
}

static bool DoVerifyFingerprint(const UnicodeString & AFingerprintFromUser, const UnicodeString & AFingerprintFromHost, bool Base64)
{
  UnicodeString FingerprintFromUser = AFingerprintFromUser;
  UnicodeString FingerprintFromHost = AFingerprintFromHost;
  UnicodeString FingerprintFromUserName, FingerprintFromHostName;
  NormalizeFingerprint(FingerprintFromUser, FingerprintFromUserName);
  NormalizeFingerprint(FingerprintFromHost, FingerprintFromHostName);
  bool Result;
  if (DebugAlwaysFalse(FingerprintFromHostName.IsEmpty()))
  {
    Result = false;
  }
  else
  {
    // In all below three formats, the checksum can be any of these formats:
    // MD5 (case insensitive):
    // xx:xx:xx
    // xx-xx-xx
    // SHA-256 (case sensitive):
    // xxxx+xx/xxx=
    // xxxx+xx/xxx
    // xxxx-xx_xxx=
    // xxxx-xx_xxx

    // Full fingerprint format "type bits checksum"
    if (!FingerprintFromUserName.IsEmpty())
    {
      Result =
        SameText(FingerprintFromUserName, FingerprintFromHostName) &&
        SameChecksum(FingerprintFromUser, FingerprintFromHost, Base64);
    }
    else
    {
      // normalized format "type-checksum"
      UnicodeString NormalizedPrefix = FingerprintFromHost + NormalizedFingerprintSeparator;
      if (StartsText(NormalizedPrefix, FingerprintFromUser))
      {
        FingerprintFromUser.Delete(1, NormalizedPrefix.Length());
        Result = SameChecksum(FingerprintFromUser, FingerprintFromHost, Base64);
      }
      else
      {
        // last resort: "checksum" only
        Result = SameChecksum(FingerprintFromUser, FingerprintFromHost, Base64);
      }
    }
  }
  return Result;
}

static bool VerifyFingerprint(
  const UnicodeString & FingerprintFromUser, const UnicodeString & FingerprintFromHostMD5, const UnicodeString & FingerprintFromHostSHA256)
{
  return
    DoVerifyFingerprint(FingerprintFromUser, FingerprintFromHostMD5, false) ||
    DoVerifyFingerprint(FingerprintFromUser, FingerprintFromHostSHA256, true);
}

struct TPasteKeyHandler
{
  UnicodeString KeyStr;
  UnicodeString FingerprintMD5;
  UnicodeString FingerprintSHA256;
  TSessionUI * UI;

  void Paste(TObject * Sender, uint32_t & Answer);
};

void TPasteKeyHandler::Paste(TObject * /*Sender*/, uint32_t & Answer)
{
  UnicodeString ClipboardText;
  if (TextFromClipboard(ClipboardText, true))
  {
    if (VerifyFingerprint(ClipboardText, FingerprintMD5, FingerprintSHA256) ||
        SameText(ClipboardText, KeyStr))
    {
      Answer = qaYes;
    }
    else
    {
      const struct ssh_keyalg * Algorithm;
      try
      {
        UnicodeString Key = ParseOpenSshPubLine(ClipboardText, Algorithm);
        if (Key == KeyStr)
        {
          Answer = qaYes;
        }
      }
      catch (...)
      {
        // swallow
      }
    }
  }

  if (Answer == 0)
  {
    UI->QueryUser(LoadStr(HOSTKEY_NOT_MATCH_CLIPBOARD), nullptr, qaOK, nullptr, qtError);
  }
}

bool TSecureShell::VerifyCachedHostKey(
  const UnicodeString & StoredKeys, const UnicodeString & KeyStr, const UnicodeString & FingerprintMD5, const UnicodeString & FingerprintSHA256)
{
  bool Result = false;
  UnicodeString Buf = StoredKeys;
  while (!Result && !Buf.IsEmpty())
  {
    UnicodeString StoredKey = CutToChar(Buf, HostKeyDelimiter, false);
    // skip leading ECDH subtype identification
    int P = StoredKey.Pos(L",");
    // Start from beginning or after the comma, if there's any.
    // If it does not start with 0x, it's probably a fingerprint (stored by TSessionData::CacheHostKey).
    bool Fingerprint = (StoredKey.SubString(P + 1, 2) != L"0x");
    if (!Fingerprint && (StoredKey == KeyStr))
    {
      LogEvent(L"Host key matches cached key");
      Result = true;
    }
    else if (Fingerprint && VerifyFingerprint(StoredKey, FingerprintMD5, FingerprintSHA256))
    {
      LogEvent(L"Host key matches cached key fingerprint");
      Result = true;
    }
    else
    {
      if (Configuration->ActualLogProtocol >= 1)
      {
        UnicodeString FormattedKey = Fingerprint ? StoredKey : FormatKeyStr(StoredKey);
        LogEvent(FORMAT(L"Host key does not match cached key %s", (FormattedKey)));
      }
      else
      {
        LogEvent(L"Host key does not match cached key");
      }
    }
  }
  return Result;
}

UnicodeString TSecureShell::StoreHostKey(
  const UnicodeString & Host, int Port, const UnicodeString & KeyType, const UnicodeString & KeyStr)
{
  TGuard Guard(PuttyStorageSection.get());
  DebugAssert(PuttyStorage == nullptr);
  TValueRestorer<THierarchicalStorage *> StorageRestorer(PuttyStorage);
  std::unique_ptr<THierarchicalStorage> Storage(GetHostKeyStorage());
  Storage->AccessMode = smReadWrite;
  PuttyStorage = Storage.get();
  store_host_key(AnsiString(Host).c_str(), Port, AnsiString(KeyType).c_str(), AnsiString(KeyStr).c_str());
  return Storage->Source;
}

void TSecureShell::ParseFingerprint(const UnicodeString & Fingerprint, UnicodeString & SignKeyType, UnicodeString & Hash)
{
  UnicodeString Buf = Fingerprint;
  UnicodeString SignKeyAlg = CutToChar(Buf, L' ', false);
  UnicodeString SignKeySize = CutToChar(Buf, L' ', false);
  SignKeyType = SignKeyAlg + L' ' + SignKeySize;
  Hash = Buf;
}

void TSecureShell::VerifyHostKey(
  const UnicodeString AHost, int32_t Port, const UnicodeString AKeyType, const UnicodeString AKeyStr,
  const UnicodeString & AFingerprintSHA256, const UnicodeString & AFingerprintMD5)
{
  if (GetConfiguration()->GetActualLogProtocol() >= 1)
  {
    LogEvent(FORMAT("Verifying host key %s %s with fingerprints %s, %s", AKeyType, FormatKeyStr(AKeyStr), AFingerprintSHA256, AFingerprintMD5));
  }

  GotHostKey();

  DebugAssert(AKeyStr.Pos(HostKeyDelimiter) == 0);

  UnicodeString Host = AHost;
  GetRealHost(Host, Port);

  UnicodeString SignKeyType, MD5, SHA256;
  ParseFingerprint(AFingerprintMD5, SignKeyType, MD5);

  DebugAssert(get_ssh_version(FBackendHandle) == 2);
  UnicodeString SignKeyTypeSHA256;
  ParseFingerprint(AFingerprintSHA256, SignKeyTypeSHA256, SHA256);
  DebugAssert(SignKeyTypeSHA256 == SignKeyType);
  if (DebugAlwaysTrue(StartsText(L"SHA256:", SHA256)))
  {
    CutToChar(SHA256, L':', false);
  }

  UnicodeString FingerprintMD5 = SignKeyType + L' ' + MD5;
  DebugAssert(AFingerprintMD5 == FingerprintMD5);
  UnicodeString FingerprintSHA256 = SignKeyType + L' ' + SHA256;
  DebugAssert(ReplaceStr(AFingerprintSHA256, L"SHA256:", EmptyStr) == FingerprintSHA256);

  FSessionInfo.HostKeyFingerprintSHA256 = FingerprintSHA256;
  FSessionInfo.HostKeyFingerprintMD5 = FingerprintMD5;

  if (FSessionData->GetFingerprintScan())
  {
    Abort();
  }

  bool AcceptNew = HaveAcceptNewHostKeyPolicy();
  UnicodeString ConfigHostKey;
  if (!AcceptNew)
  {
    ConfigHostKey = FSessionData->HostKey;
  }

  UnicodeString StoredKeys = RetrieveHostKey(Host, Port, KeyType);
  bool Result = VerifyCachedHostKey(StoredKeys, KeyStr, FingerprintMD5, FingerprintSHA256);
  if (!Result && AcceptNew)
  {
    if (!StoredKeys.IsEmpty()) // optimization + avoiding the log message
    {
      AcceptNew = false;
    }
    else if (have_any_ssh2_hostkey(FSeat, AnsiString(Host).c_str(), Port))
    {
      LogEvent(L"Host key not found in the cache, but other key types found, cannot accept new key");
      AcceptNew = false;
    }
  }

  bool ConfiguredKeyNotMatch = false;

  if (!Result && !ConfigHostKey.IsEmpty() &&
      // Should test have_any_ssh2_hostkey + No need to bother with AcceptNew, as we never get here
      (StoredKeys.IsEmpty() || FSessionData->OverrideCachedHostKey))
  {
    UnicodeString Buf = ConfigHostKey;
    while (!Result && !Buf.IsEmpty())
    {
      UnicodeString ExpectedKey = CutToChar(Buf, HostKeyDelimiter, false);
      if (ExpectedKey == L"*")
      {
        UnicodeString Message = LoadStr(ANY_HOSTKEY);
        FUI->Information(Message, true);
        FLog->Add(llException, Message);
        Result = true;
      }
      else if (VerifyFingerprint(ExpectedKey, FingerprintMD5, FingerprintSHA256))
      {
        LogEvent(L"Host key matches configured key fingerprint");
        Result = true;
      }
      else
      {
        LogEvent(FORMAT(L"Host key does not match configured key fingerprint %s", (ExpectedKey)));
      }
    }

    if (!Result)
    {
      ConfiguredKeyNotMatch = true;
    }
  }

  if (!Result && AcceptNew && DebugAlwaysTrue(ConfigHostKey.IsEmpty()))
  {
    try
    {
      UnicodeString StorageSource = StoreHostKey(Host, Port, KeyType, KeyStr);
      UnicodeString StoredKeys = RetrieveHostKey(Host, Port, KeyType);
      if (StoredKeys != KeyStr)
      {
        throw Exception(UnicodeString());
      }
      Configuration->Usage->Inc(L"HostKeyNewAccepted");
      LogEvent(FORMAT(L"Warning: Stored new host key to %s - This should occur only on the first connection", (StorageSource)));
      Result = true;
    }
    catch (Exception & E)
    {
      FUI->FatalError(&E, LoadStr(STORE_NEW_HOSTKEY_ERROR));
    }
  }

  if (!Result)
  {
    bool Verified;
    if (ConfiguredKeyNotMatch || GetConfiguration()->GetDisableAcceptingHostKeys())
    {
      Verified = false;
    }
    // no point offering manual verification, if we cannot persist the verified key
    else if (!GetConfiguration()->GetPersistent() && GetConfiguration()->GetScripting())
    {
      Verified = false;
    }
    else
    {
      // We should not offer caching if !Configuration->Persistent,
      // but as scripting mode is handled earlier and in GUI it hardly happens,
      // it's a small issue.
      TClipboardHandler ClipboardHandler;
      ClipboardHandler.Text = FingerprintSHA256 + L"\n" + FingerprintMD5;
      TPasteKeyHandler PasteKeyHandler;
      PasteKeyHandler.KeyStr = KeyStr;
      PasteKeyHandler.FingerprintMD5 = FingerprintMD5;
      PasteKeyHandler.FingerprintSHA256 = FingerprintSHA256;
      PasteKeyHandler.UI = FUI;

      bool Unknown = StoredKeys.IsEmpty();

      uint32_t Answers;
      int32_t AliasesCount;
      TQueryButtonAlias Aliases[4];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
      Aliases[0].ActionAlias = LoadStr(COPY_KEY_ACTION);
      Aliases[0].OnSubmit = nb::bind(&TClipboardHandler::Copy, &ClipboardHandler);
      Aliases[1].Button = qaIgnore;
      Aliases[1].Alias = LoadStr(PASTE_KEY_BUTTON);
      Aliases[1].OnSubmit = nb::bind(&TPasteKeyHandler::Paste, &PasteKeyHandler);
      Aliases[1].GroupWith = qaYes;
      Answers = qaYes | qaCancel | qaRetry | qaIgnore;
      AliasesCount = 2;
      if (!Unknown)
      {
        Aliases[2].Button = qaYes;
        Aliases[2].Alias = LoadStr(UPDATE_KEY_BUTTON);
        Aliases[3].Button = qaOK;
        Aliases[3].Alias = LoadStr(ADD_KEY_BUTTON);
        AliasesCount += 2;
        Answers |= qaSkip | qaOK;
      }
      else
      {
        Answers |= qaNo;
      }

      TQueryParams Params(qpWaitInBatch);
      Params.NoBatchAnswers = qaYes | qaRetry | qaSkip | qaOK;
      Params.HelpKeyword = (Unknown ? HELP_UNKNOWN_KEY : HELP_DIFFERENT_KEY);
      Params.Aliases = Aliases;
      Params.AliasesCount = AliasesCount;

      UnicodeString KeyTypeHuman = GetKeyTypeHuman(AKeyType);
      UnicodeString KeyDetails = FMTLOAD(KEY_DETAILS, SignKeyType, SHA256, MD5);
      UnicodeString Message = FMTLOAD((Unknown ? UNKNOWN_KEY4 : DIFFERENT_KEY5), KeyTypeHuman, KeyDetails);
      if (Configuration->Scripting)
      {
        AddToList(Message, LoadStr(SCRIPTING_USE_HOSTKEY), L"\n");
      }

      uint32_t R =
        FUI->QueryUser(Message, nullptr, Answers, &Params, qtWarning);
      UnicodeString StoreKeyStr = AKeyStr;

      switch (R) {
        case qaOK:
          DebugAssert(!Unknown);
          StoreKeyStr = (StoredKeys + HostKeyDelimiter + StoreKeyStr);
          // fall thru
        case qaYes:
          StoreHostKey(Host, nb::ToInt(Port), AKeyType, StoreKeyStr);
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
      __removed Configuration->Usage->Inc("HostNotVerified");

      UnicodeString Message;
      if (ConfiguredKeyNotMatch)
      {
        Message = FMTLOAD(CONFIGURED_KEY_NOT_MATCH, ConfigHostKey);
      }
      else if (!GetConfiguration()->GetPersistent() && GetConfiguration()->GetScripting())
      {
        Message = LoadStr(HOSTKEY_NOT_CONFIGURED);
      }
      else
      {
        Message = LoadStr(KEY_NOT_VERIFIED);
      }

      std::unique_ptr<Exception> E(std::make_unique<Exception>(MainInstructions(Message)));
      try__finally
      {
        FUI->FatalError(E.get(), FMTLOAD(HOSTKEY, FingerprintSHA256));
      },
      __finally__removed
      ({
        delete E;
      }) end_try__finally
    }
  }

  GetConfiguration()->RememberLastFingerprint(FSessionData->GetSiteKey(), SshFingerprintType, FingerprintSHA256);
}

bool TSecureShell::HaveHostKey(UnicodeString AHost, int32_t Port, UnicodeString KeyType)
{
  // Return true, if we have any host key fingerprint of a particular type

  UnicodeString Host = AHost;
  GetRealHost(Host, Port);

  UnicodeString StoredKeys = RetrieveHostKey(Host, Port, KeyType);
  bool Result = !StoredKeys.IsEmpty();

  if (!FSessionData->GetHostKey().IsEmpty())
  {
    UnicodeString Buf = FSessionData->GetHostKey();
    while (!Result && !Buf.IsEmpty())
    {
      UnicodeString ExpectedKey = CutToChar(Buf, HostKeyDelimiter, false);
      UnicodeString ExpectedKeyType = GetKeyTypeFromFingerprint(ExpectedKey);
      Result = SameText(ExpectedKeyType, KeyType);
    }
  }

  if (Result &&
      (FLoggedKnownHostKeys.find(KeyType) == FLoggedKnownHostKeys.end()))
  {
    LogEvent(FORMAT("Have a known host key of type %s", KeyType));
    // This is called multiple times for the same cached key since PuTTY 0.76 (support for rsa-sha2*?).
    // Avoid repeated log entries.
    FLoggedKnownHostKeys.insert(KeyType);
  }

  return Result;
}

void TSecureShell::AskAlg(UnicodeString AAlgType, UnicodeString AlgName)
{
  // beware of changing order
  static const TPuttyTranslation AlgTranslation[] = {
    { "cipher", CIPHER_TYPE_BOTH2 },
    { "client-to-server cipher", CIPHER_TYPE_CS2 },
    { "server-to-client cipher", CIPHER_TYPE_SC2 },
    { "key-exchange algorithm", KEY_EXCHANGE_ALG },
    { "hostkey type", KEYKEY_TYPE },
  };

  UnicodeString AlgType = AAlgType;
  TranslatePuttyMessage(AlgTranslation, _countof(AlgTranslation), AlgType);

  UnicodeString Msg = FMTLOAD(ALG_BELOW_TRESHOLD, AlgType, AlgName);

  if (FUI->QueryUser(Msg, nullptr, qaYes | qaNo, nullptr, qtWarning) == qaNo)
  {
    UnicodeString Error = FMTLOAD(ALG_NOT_VERIFIED, AlgType, AlgName);
    FUI->FatalError(nullptr, Error);
  }
}

void TSecureShell::DisplayBanner(const UnicodeString Banner)
{
  // Since 0.77 PuTTY calls this again with CRLF if the actual banner does not end with one.
  if (!Banner.Trim().IsEmpty())
  {
    FUI->DisplayBanner(Banner);
  }
}

void TSecureShell::OldKeyfileWarning()
{
  // actually never called, see Net.cpp
  FUI->QueryUser(LoadStr(OLD_KEY), nullptr, qaOK, nullptr, qtWarning);
}

bool TSecureShell::GetStoredCredentialsTried() const
{
  return FStoredPasswordTried || FStoredPasswordTriedForKI || FStoredPassphraseTried;
}

bool TSecureShell::GetReady() const
{
  return FOpened && (FWaiting == 0);
}

void TSecureShell::CollectUsage()
{
#if 0
  if (FCollectPrivateKeyUsage)
  {
    Configuration->Usage->Inc("OpenedSessionsPrivateKey2");
  }

  Configuration->Usage->Inc("OpenedSessionsSSH2");

  if (SshImplementation == sshiOpenSSH)
  {
    Configuration->Usage->Inc("OpenedSessionsSSHOpenSSH");
  }
  else if (SshImplementation == sshiProFTPD)
  {
    Configuration->Usage->Inc("OpenedSessionsSSHProFTPD");
  }
  else if (SshImplementation == sshiBitvise)
  {
    Configuration->Usage->Inc("OpenedSessionsSSHBitvise");
  }
  else if (SshImplementation == sshiTitan)
  {
    Configuration->Usage->Inc("OpenedSessionsSSHTitan");
  }
  else if (SshImplementation == sshiOpenVMS)
  {
    Configuration->Usage->Inc("OpenedSessionsSSHOpenVMS");
  }
  else if (ContainsText(FSessionInfo.SshImplementation, "Serv-U"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHServU");
  }
  else if (SshImplementation == sshiCerberus)
  {
    // Ntb, Cerberus can also be detected using vendor-id extension
    // Cerberus FTP Server 7.0.5.3 (70005003) by Cerberus, LLC
    Configuration->Usage->Inc("OpenedSessionsSSHCerberus");
  }
  else if (ContainsText(FSessionInfo.SshImplementation, "WS_FTP"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHWSFTP");
  }
  // SSH-2.0-1.36_sshlib GlobalSCAPE
  else if (ContainsText(FSessionInfo.SshImplementation, "GlobalSCAPE"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHGlobalScape");
  }
  // SSH-2.0-CompleteFTP-8.1.3
  else if (ContainsText(FSessionInfo.SshImplementation, "CompleteFTP"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHComplete");
  }
  // SSH-2.0-CoreFTP-0.3.3
  else if (ContainsText(FSessionInfo.SshImplementation, "CoreFTP"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHCore");
  }
  // SSH-2.0-SSHD-CORE-0.11.0 (value is configurable, this is a default)
  // (Apache Mina SSHD, e.g. on brickftp.com)
  else if (ContainsText(FSessionInfo.SshImplementation, "SSHD-CORE"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHApache");
  }
  // SSH-2.0-Syncplify_Me_Server
  else if (ContainsText(FSessionInfo.SshImplementation, "Syncplify"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHSyncplify");
  }
  else if (ContainsText(FSessionInfo.SshImplementation, "zFTPServer"))
  {
    Configuration->Usage->Inc("OpenedSessionsSSHzFTP");
  }
  else
  {
    Configuration->Usage->Inc("OpenedSessionsSSHOther");
  }
#endif // #if 0
}

bool TSecureShell::CanChangePassword() const
{
  return
    // These major SSH servers explicitly do not support password change.
    (GetSshImplementation() != sshiOpenSSH) && // See userauth_passwd
    (GetSshImplementation() != sshiProFTPD); // See sftp_auth_password
}

