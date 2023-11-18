
#include <rdestl/map.h>
#include <vcl.h>
#pragma hdrstop

#include <Exceptions.h>
#include <StrUtils.hpp>
#include <Sysutils.hpp>

#include "PuttyIntf.h"
#include "Interface.h"
#include "SecureShell.h"
#include "Exceptions.h"
#include "CoreMain.h"
#include "TextsCore.h"
__removed #include <Soap.EncdDecd.hpp>

char sshver[50]{};
extern const char commitid[] = "";
const bool platform_uses_x11_unix_by_default = true;
CRITICAL_SECTION putty_section;
bool SaveRandomSeed;
bool HadRandomSeed;
char appname_[50]{};
const char * const appname = appname_;
extern "C" const bool share_can_be_downstream = false;
extern "C" const bool share_can_be_upstream = false;
THierarchicalStorage * PuttyStorage = nullptr;

extern "C"
{
#include <windows/platform.h>
//#include <winstuff.h>
}
const UnicodeString OriginalPuttyRegistryStorageKey(PUTTY_REG_POS);
const UnicodeString KittyRegistryStorageKey("Software\\9bis.com\\KiTTY");
const UnicodeString OriginalPuttyExecutable("putty.exe");
const UnicodeString KittyExecutable("kitty.exe");
const UnicodeString PuttyKeyExt("ppk");

void PuttyInitialize()
{
  SaveRandomSeed = true;

  InitializeCriticalSection(&putty_section);

  HadRandomSeed = SysUtulsFileExists(ApiPath(GetConfiguration()->GetRandomSeedFileName()));
  if (HadRandomSeed)
  {
    AppLog(L"Random seed file exists");
  }
  // make sure random generator is initialized, so random_save_seed()
  // in destructor can proceed
  random_ref();

  sk_init();

  AnsiString VersionString = AnsiString(GetSshVersionString());
  DebugAssert(!VersionString.IsEmpty() && (nb::ToSizeT(VersionString.Length()) < _countof(sshver)));
  strcpy_s(sshver, sizeof(sshver), VersionString.c_str());
  AnsiString AppName = AnsiString(GetAppNameString());
  DebugAssert(!AppName.IsEmpty() && (nb::ToSizeT(AppName.Length()) < _countof(appname_)));
  strcpy_s(appname_, sizeof(appname_), AppName.c_str());
}

static bool DeleteRandomSeedOnExit()
{
  return !HadRandomSeed && !SaveRandomSeed;
}

void PuttyFinalize()
{
  if (SaveRandomSeed)
  {
    AppLog(L"Saving random seed file");
    random_save_seed();
  }
  random_unref();
  // random_ref in PuttyInitialize creates the seed file. Delete it, if we didn't want to create it.
  if (DeleteRandomSeedOnExit())
  {
    AppLog(L"Deleting unwanted random seed file");
    SysUtulsRemoveFile(ApiPath(GetConfiguration()->GetRandomSeedFileName()));
  }

  sk_cleanup();
  win_misc_cleanup();
  win_secur_cleanup();
  ec_cleanup();
  wingss_cleanup();
  DeleteCriticalSection(&putty_section);
}

void DontSaveRandomSeed()
{
  SaveRandomSeed = false;
}

bool RandomSeedExists()
{
  return
    !DeleteRandomSeedOnExit() &&
    SysUtulsFileExists(ApiPath(GetConfiguration()->GetRandomSeedFileName()));
}

TSecureShell * GetSeatSecureShell(Seat * seat)
{
  DebugAssert(seat != nullptr);
  if (is_tempseat(seat))
  {
    seat = tempseat_get_real(seat);
  }

  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  return SecureShell;
}

TSecureShell * GetSecureShell(Plug * plug, bool & pfwd)
{
  if (!is_ssh(plug) && !is_pfwd(plug))
  {
    // If it is not SSH/PFwd plug, then it must be Proxy plug.
    // Get SSH/PFwd plug which it wraps.
    ProxySocket * AProxySocket = get_proxy_plug_socket(plug);
    plug = AProxySocket->plug;
  }

  pfwd = is_pfwd(plug);
  Seat * seat;
  if (pfwd)
  {
    seat = get_pfwd_seat(plug);
  }
  else
  {
    seat = get_ssh_seat(plug);
  }
  DebugAssert(seat != nullptr);
  return GetSeatSecureShell(seat);
}

struct callback_set * get_callback_set(Plug * plug)
{
  bool pfwd;
  TSecureShell * SecureShell = GetSecureShell(plug, pfwd);
  return SecureShell->GetCallbackSet();
}

struct callback_set * get_seat_callback_set(Seat * seat)
{
  TSecureShell * SecureShell = GetSeatSecureShell(seat);
  return SecureShell->GetCallbackSet();
}

extern "C" const char * do_select(Plug * plug, SOCKET skt, bool enable)
{
  bool pfwd;
  TSecureShell * SecureShell = GetSecureShell(plug, pfwd);
  if (!pfwd)
  {
    SecureShell->UpdateSocket(skt, enable);
  }
  else
  {
    SecureShell->UpdatePortFwdSocket(skt, enable);
  }

  return nullptr;
}

static size_t output(Seat * seat, SeatOutputType type, const void * data, size_t len)
{
  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  if (nb::ToInt32(static_cast<char>(type)) == -1)
  {
    SecureShell->CWrite(reinterpret_cast<const char *>(data), len);
  }
  else if (type != SEAT_OUTPUT_STDERR)
  {
    SecureShell->FromBackend(reinterpret_cast<const uint8_t *>(data), len);
  }
  else
  {
    SecureShell->AddStdError(reinterpret_cast<const uint8_t *>(data), len);
  }
  return 0;
}

static bool eof(Seat *)
{
  return false;
}

static SeatPromptResult get_userpass_input(Seat * seat, prompts_t * p)
{
  DebugAssert(p != nullptr);
  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  DebugAssert(SecureShell != nullptr);

  SeatPromptResult Result;
  std::unique_ptr<TStrings> Prompts(std::make_unique<TStringList>());
  std::unique_ptr<TStrings> Results(std::make_unique<TStringList>());
  try__finally
  {
    UnicodeString Name = UTF8ToString(p->name);
    UnicodeString AName = Name;
    TPromptKind PromptKind = SecureShell->IdentifyPromptKind(AName);
    bool UTF8Prompt = (PromptKind != pkPassphrase);

    for (int32_t Index = 0; Index < (int32_t)(p->n_prompts); Index++)
    {
      prompt_t * Prompt = p->prompts[Index];
      UnicodeString S;
      if (UTF8Prompt)
      {
        S = UTF8ToString(Prompt->prompt);
      }
      else
      {
        S = UnicodeString(AnsiString(Prompt->prompt));
      }
      Prompts->AddObject(S, (TObject *)(FLAGMASK(Prompt->echo, pupEcho)));
      // this fails, when new passwords do not match on change password prompt,
      // and putty retries the prompt
      DebugAssert(strlen(prompt_get_result_ref(Prompt)) == 0);
      Results->Add(L"");
    }

    UnicodeString Instructions = UTF8ToString(p->instruction);
    if (SecureShell->PromptUser(p->to_server, Name, p->name_reqd,
          Instructions, p->instr_reqd, Prompts.get(), Results.get()))
    {
      for (int32_t Index = 0; Index < nb::ToInt32(p->n_prompts); Index++)
      {
        prompt_t * Prompt = p->prompts[Index];
        RawByteString S;
        if (UTF8Prompt)
        {
          S = RawByteString(UTF8String(Results->GetString(Index)));
        }
        else
        {
          S = RawByteString(AnsiString(Results->GetString(Index)));
        }
        prompt_set_result(Prompt, S.c_str());
      }
      Result = SPR_OK;
    }
    else
    {
      Result = SPR_USER_ABORT;
    }
  },
  __finally__removed
  ({
    delete Prompts;
    delete Results;
  }) end_try__finally

  return Result;
}

static void connection_fatal(Seat * seat, const char * message)
{

  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  SecureShell->PuttyFatalError(UnicodeString(AnsiString(message)));
}

SeatPromptResult confirm_ssh_host_key(Seat * seat, const char * host, int32_t port, const char * keytype,
  char * keystr, SeatDialogText *, HelpCtx,
  void (*DebugUsedArg(callback))(void *ctx, SeatPromptResult result), void * DebugUsedArg(ctx),
  char **key_fingerprints, bool is_certificate, int32_t ca_count, bool already_verified)
{
  UnicodeString FingerprintSHA256, FingerprintMD5;
  if (key_fingerprints[SSH_FPTYPE_SHA256] != nullptr)
  {
    FingerprintSHA256 = key_fingerprints[SSH_FPTYPE_SHA256];
  }
  if (DebugAlwaysTrue(key_fingerprints[SSH_FPTYPE_MD5] != nullptr))
  {
    FingerprintMD5 = key_fingerprints[SSH_FPTYPE_MD5];
  }
  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  SecureShell->VerifyHostKey(
    host, port, keytype, keystr, FingerprintSHA256, FingerprintMD5, is_certificate, ca_count, already_verified);

  // We should return 0 when key was not confirmed, we throw exception instead.
  return SPR_OK;
}

bool have_ssh_host_key(Seat * seat, const char * hostname, int32_t port,
  const char * keytype)
{
  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  return SecureShell->HaveHostKey(hostname, port, keytype) ? 1 : 0;
}

SeatPromptResult confirm_weak_crypto_primitive(Seat * seat, const char * algtype, const char * algname,
  void (*/*callback*/)(void * ctx, SeatPromptResult result), void * /*ctx*/)
{
  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  SecureShell->AskAlg(algtype, algname);

  // We should return 0 when alg was not confirmed, we throw exception instead.
  return SPR_OK;
}

SeatPromptResult confirm_weak_cached_hostkey(Seat *, const char * /*algname*/, const char * /*betteralgs*/,
  void (*/*callback*/)(void *ctx, SeatPromptResult result), void * /*ctx*/)
{
  return SPR_OK;
}

const SeatDialogPromptDescriptions * prompt_descriptions(Seat *)
{
    static const SeatDialogPromptDescriptions descs = {
        /*.hk_accept_action =*/ "",
        /*.hk_connect_once_action =*/ "",
        /*.hk_cancel_action =*/ "",
        /*.hk_cancel_action_Participle =*/ "",
    };
    return &descs;
}

void old_keyfile_warning(void)
{
  // no reference to TSecureShell instance available - and we already warn on Login dialog
}

size_t banner(Seat * seat, const void * data, size_t len)
{
  TSecureShell * SecureShell = static_cast<ScpSeat *>(seat)->SecureShell;
  UnicodeString Banner(UTF8String(static_cast<const char *>(data), len));
  SecureShell->DisplayBanner(Banner);
  return 0; // PuTTY never uses the value
}

uintmax_t strtoumax_(const char *nptr, char **endptr, int32_t base)
{
  if (DebugAlwaysFalse(endptr != nullptr) ||
      DebugAlwaysFalse(base != 10))
  {
    Abort();
  }
  return StrToInt64(UnicodeString(AnsiString(nptr)));
}

static void SSHFatalError(const char * Format, va_list Param)
{
  char Buf[200];
  vsnprintf(Buf, LENOF(Buf), Format, Param);
  Buf[LENOF(Buf) - 1] = '\0';

  // Only few calls from putty\winnet.c might be connected with specific
  // TSecureShell. Otherwise called only for really fatal errors
  // like 'out of memory' from putty\ssh.c.
  throw ESshFatal(nullptr, Buf);
}

void modalfatalbox(const char * fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}

void nonfatal(const char * fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}

void ldisc_echoedit_update(Ldisc * /*handle*/)
{
  DebugFail();
}

uint32_t schedule_timer(int32_t ticks, timer_fn_t /*fn*/, void * /*ctx*/)
{
  return ticks + GetTickCount();
}

void expire_timer_context(void * /*ctx*/)
{
  // nothing
}

Pinger * pinger_new(Conf * /*conf*/, Backend * /*back*/)
{
  return nullptr;
}

void pinger_reconfig(Pinger * /*pinger*/, Conf * /*oldconf*/, Conf * /*newconf*/)
{
  // nothing
}

void pinger_free(Pinger * /*pinger*/)
{
  // nothing
}

void platform_get_x11_auth(struct X11Display * /*display*/, Conf * /*conf*/)
{
  // nothing, therefore no auth.
}

// Based on PuTTY's settings.c
char * get_remote_username(Conf * conf)
{
  char * username = conf_get_str(conf, CONF_username);
  char * result;
  if (*username)
  {
    result = dupstr(username);
  }
  else
  {
    result = nullptr;
  }
  return result;
}

static const SeatVtable ScpSeatVtable =
  {
    output,
    eof,
    nullseat_sent,
    banner,
    get_userpass_input,
    nullseat_notify_session_started,
    nullseat_notify_remote_exit,
    nullseat_notify_remote_disconnect,
    connection_fatal,
    nullseat_update_specials_menu,
    nullseat_get_ttymode,
    nullseat_set_busy_status,
    confirm_ssh_host_key,
    confirm_weak_crypto_primitive,
    confirm_weak_cached_hostkey,
    prompt_descriptions,
    nullseat_is_always_utf8,
    nullseat_echoedit_update,
    nullseat_get_x_display,
    nullseat_get_windowid,
    nullseat_get_window_pixel_size,
    nullseat_stripctrl_new,
    nullseat_set_trust_status,
    nullseat_can_set_trust_status_yes,
    nullseat_has_mixed_input_stream_yes,
    nullseat_verbose_yes,
    nullseat_interactive_no,
    nullseat_get_cursor_position,
  };

ScpSeat::ScpSeat(TSecureShell * ASecureShell)
{
  SecureShell = ASecureShell;
  vt = &ScpSeatVtable;
}

//static std::unique_ptr<TCriticalSection> PuttyRegistrySection(TraceInitPtr(new TCriticalSection()));
static TCriticalSection PuttyRegistrySection;
enum TPuttyRegistryMode { prmPass, prmRedirect, prmCollect, prmFail };
static TPuttyRegistryMode PuttyRegistryMode = prmRedirect;
using TPuttyRegistryTypes = rde::map<UnicodeString, uint32_t>;
TPuttyRegistryTypes PuttyRegistryTypes;
HKEY RandSeedFileStorage = reinterpret_cast<HKEY>(1);

int32_t reg_override_winscp()
{
  return (PuttyRegistryMode != prmPass);
}

HKEY open_regkey_fn_winscp(bool Create, HKEY Key, const char * Path, ...)
{
  HKEY Result;
  if (PuttyRegistryMode == prmCollect)
  {
    Result = reinterpret_cast<HKEY>(1);
  }
  else if (PuttyRegistryMode == prmFail)
  {
    Result = false;
  }
  else if (PuttyRegistryMode == prmRedirect)
  {
    DebugAssert(Key == HKEY_CURRENT_USER);
    DebugUsedParam(Key);

    UnicodeString SubKey;
    va_list ap;
    va_start(ap, Path);

    for (; Path; Path = va_arg(ap, const char *))
    {
      if (!SubKey.IsEmpty())
      {
        SubKey = IncludeTrailingBackslash(SubKey);
      }
      SubKey += UnicodeString(UTF8String(Path));
    }

    int32_t PuttyKeyLen = OriginalPuttyRegistryStorageKey.Length();
    DebugAssert(SubKey.SubString(1, PuttyKeyLen) == OriginalPuttyRegistryStorageKey);
    UnicodeString RegKey = SubKey.SubString(PuttyKeyLen + 1, SubKey.Length() - PuttyKeyLen);
    if (!RegKey.IsEmpty())
    {
      DebugAssert(RegKey[1] == L'\\');
      RegKey.Delete(1, 1);
    }

    if (RegKey.IsEmpty())
    {
      // Called from access_random_seed()
      Result = RandSeedFileStorage;
    }
    else
    {
      // we expect this to be called only from retrieve_host_key() or store_host_key()
      DebugAssert(RegKey == L"SshHostKeys");

      DebugAssert(PuttyStorage != nullptr);
      DebugAssert(PuttyStorage->AccessMode == (Create ? smReadWrite : smRead));
      if (PuttyStorage->OpenSubKey(RegKey, Create))
      {
        Result = reinterpret_cast<HKEY>(PuttyStorage);
      }
      else
      {
        Result = nullptr;
      }
    }
  }
  else
  {
    DebugFail();
    Result = nullptr;
  }
  return Result;
}

bool get_reg_dword_winscp(HKEY, const char * DebugUsedArg(Name), DWORD * DebugUsedArg(Out))
{
  bool Result;
  if (PuttyRegistryMode == prmFail)
  {
    Result = false;
  }
  else if (PuttyRegistryMode == prmCollect)
  {
    Result = false;
  }
  else
  {
    DebugFail();
    Result = false;
  }
  return Result;
}

char * get_reg_sz_winscp(HKEY Key, const char * Name)
{
  char * Result;
  if (PuttyRegistryMode == prmCollect)
  {
    Result = nullptr;
  }
  else if (DebugAlwaysTrue(PuttyRegistryMode == prmRedirect))
  {
    DebugAssert(GetConfiguration() != nullptr);

    UnicodeString ValueName = UTF8String(Name);
    bool Success;
    UnicodeString Value;
    if (Key == RandSeedFileStorage)
    {
      if (ValueName == L"RandSeedFile")
      {
        Value = GetConfiguration()->GetRandomSeedFileName();
        Success = true;
      }
      else
      {
        DebugFail();
        Success = false;
      }
    }
    else
    {
      THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
      if (Storage->ValueExists(ValueName))
      {
        Value = Storage->ReadStringRaw(ValueName, L"");
        Success = true;
      }
      else
      {
        Success = false;
      }
    }

    if (!Success)
    {
      Result = nullptr;
    }
    else
    {
      AnsiString ValueAnsi = AnsiString(Value);
      Result = snewn(ValueAnsi.Length() + 1, char);
      strcpy(Result, ValueAnsi.c_str());
    }
  }
  else
  {
    Result = nullptr;
  }
  return Result;
}

bool put_reg_dword_winscp(HKEY DebugUsedArg(Key), const char * Name, DWORD DebugUsedArg(Value))
{
  bool Result;
  if (PuttyRegistryMode == prmCollect)
  {
    UnicodeString ValueName = UTF8String(Name);
    PuttyRegistryTypes[ValueName] = REG_DWORD;
    Result = true;
  }
  else if (PuttyRegistryMode == prmRedirect)
  {
    // Might need to implement this for CA
    DebugFail();
    Result = false;
  }
  else
  {
    DebugFail();
    return false;
  }
  return Result;
}

bool put_reg_sz_winscp(HKEY Key, const char * Name, const char * Str)
{
  UnicodeString ValueName = UTF8String(Name);
  bool Result;
  if (PuttyRegistryMode == prmCollect)
  {
    PuttyRegistryTypes[ValueName] = REG_SZ;
    Result = true;
  }
  else if (PuttyRegistryMode == prmRedirect)
  {
    UnicodeString Value = UTF8String(Str);
    DebugAssert(Key != RandSeedFileStorage);
    THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
    DebugAssert(Storage != nullptr);
    if (Storage != nullptr)
    {
      Storage->WriteStringRaw(ValueName, Value);
    }

    Result = true;
  }
  else
  {
    DebugFail();
    Result = false;
  }
  return Result;
}

void close_regkey_winscp(HKEY)
{
  DebugAssert((PuttyRegistryMode == prmCollect) || (PuttyRegistryMode == prmRedirect));
}

strbuf * get_reg_multi_sz_winscp(HKEY, const char * DebugUsedArg(name))
{
  // Needed for CA
  DebugFail();
  return nullptr;
}

TKeyType GetKeyType(const UnicodeString & FileName)
{
  DebugAssert(ktUnopenable == SSH_KEYTYPE_UNOPENABLE);
  DebugAssert(ktSSHCom == SSH_KEYTYPE_SSHCOM);
  DebugAssert(ktSSH2PublicOpenSSH == SSH_KEYTYPE_SSH2_PUBLIC_OPENSSH);
  UTF8String UtfFileName = UTF8String(FileName);
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  TKeyType Result = (TKeyType)key_type(KeyFile);
  filename_free(KeyFile);
  return Result;
}

bool IsKeyEncrypted(TKeyType KeyType, const UnicodeString & FileName, UnicodeString & Comment)
{
  UTF8String UtfFileName = UTF8String(FileName);
  bool Result;
  char * CommentStr = nullptr;
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  try__finally
  {
    switch (KeyType)
    {
      case ktSSH2:
        Result = (ppk_encrypted_f(KeyFile, &CommentStr) != 0);
        break;

      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        Result = (import_encrypted(KeyFile, KeyType, &CommentStr) != false);
        break;

      default:
        DebugFail();
        Result = false;
        break;
    }
  },
  __finally
  {
    filename_free(KeyFile);
  } end_try__finally

  if (CommentStr != nullptr)
  {
    Comment = UnicodeString(AnsiString(CommentStr));
    // ktOpenSSH has no comment, PuTTY defaults to file path
    if (Comment == FileName)
    {
      Comment = base::ExtractFileName(FileName, false);
    }
    sfree(CommentStr);
  }

  return Result;
}

TPrivateKey * LoadKey(TKeyType KeyType, const UnicodeString & FileName, const UnicodeString & Passphrase, UnicodeString & Error)
{
  UTF8String UtfFileName = UTF8String(FileName);
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  struct ssh2_userkey * Ssh2Key = nullptr;
  const char * ErrorStr = nullptr;
  AnsiString AnsiPassphrase = AnsiString(Passphrase);
  try__finally
  {
    switch (KeyType)
    {
      case ktSSH2:
        Ssh2Key = ppk_load_f(KeyFile, AnsiPassphrase.c_str(), &ErrorStr);
        break;

      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        Ssh2Key = import_ssh2(KeyFile, KeyType, ToChar(AnsiPassphrase), &ErrorStr);
        break;

      default:
        DebugFail();
        break;
    }
  },
  __finally
  {
    Shred(AnsiPassphrase);
    filename_free(KeyFile);
  } end_try__finally

  if (Ssh2Key == nullptr)
  {
    // While theoretically we may get "unable to open key file" and
    // so we should check system error code,
    // we actully never get here unless we call KeyType previously
    // and handle ktUnopenable accordingly.
    Error = AnsiString(ErrorStr);
  }
  else if (Ssh2Key == SSH2_WRONG_PASSPHRASE)
  {
    Error = EmptyStr;
    Ssh2Key = nullptr;
  }

  return reinterpret_cast<TPrivateKey *>(Ssh2Key);
}

TPrivateKey * LoadKey(TKeyType KeyType, const UnicodeString & FileName, const UnicodeString & Passphrase)
{
  UnicodeString Error;
  TPrivateKey * Result = LoadKey(KeyType, FileName, Passphrase, Error);
  if (Result == nullptr)
  {
    if (!Error.IsEmpty())
    {
      throw Exception(Error);
    }
    else
    {
      throw Exception(LoadStr(AUTH_TRANSL_WRONG_PASSPHRASE));
    }
  }
  return Result;
}

UnicodeString TestKey(TKeyType KeyType, const UnicodeString & FileName)
{
  UnicodeString Result;
  TPrivateKey * Key = LoadKey(KeyType, FileName, EmptyStr, Result);
  if (Key != nullptr)
  {
    FreeKey(Key);
  }
  return Result;
}

void ChangeKeyComment(TPrivateKey * PrivateKey, const UnicodeString & Comment)
{
  AnsiString AnsiComment(Comment);
  struct ssh2_userkey * Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);
  sfree(Ssh2Key->comment);
  Ssh2Key->comment = dupstr(AnsiComment.c_str());
}

// Based on cmdgen.c
void AddCertificateToKey(TPrivateKey * PrivateKey, const UnicodeString & CertificateFileName)
{
  struct ssh2_userkey * Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);

  TKeyType Type = GetKeyType(CertificateFileName);
  int Error = errno;
  if ((Type != SSH_KEYTYPE_SSH2_PUBLIC_RFC4716) &&
      (Type != SSH_KEYTYPE_SSH2_PUBLIC_OPENSSH))
  {
    if (Type == ktUnopenable)
    {
      throw EOSExtException(FMTLOAD(CERTIFICATE_UNOPENABLE, CertificateFileName), Error);
    }
    else
    {
      throw Exception(FMTLOAD(KEYGEN_NOT_PUBLIC, CertificateFileName));
    }
  }

  UTF8String UtfCertificateFileName = UTF8String(CertificateFileName);
  Filename * CertFilename = filename_from_str(UtfCertificateFileName.c_str());

  LoadedFile * CertLoadedFile;
  try__finally
  {
    const char * ErrorStr = nullptr;
    CertLoadedFile = lf_load_keyfile(CertFilename, &ErrorStr);
    if (CertLoadedFile == nullptr)
    {
      // not capturing errno, as this in unlikely file access error, after we have passed KeyType above
      throw ExtException(FMTLOAD(CERTIFICATE_UNOPENABLE, CertificateFileName), Error);
    }
  },
  __finally
  {
    filename_free(CertFilename);
  } end_try__finally

  strbuf * Pub = strbuf_new();
  char * AlgorithmName = nullptr;
  try__finally
  {
    const char * ErrorStr = nullptr;
    char * CommentStr = nullptr;
    if (!ppk_loadpub_s(BinarySource_UPCAST(CertLoadedFile), &AlgorithmName,
                       BinarySink_UPCAST(Pub), &CommentStr, &ErrorStr))
    {
      UnicodeString Error = ErrorStr;
      throw ExtException(FMTLOAD(CERTIFICATE_LOAD_ERROR, CertificateFileName), Error);
    }
    sfree(CommentStr);
  },
  __finally
  {
    lf_free(CertLoadedFile);
  } end_try__finally

  const ssh_keyalg * KeyAlg;
  try__finally
  {
    KeyAlg = find_pubkey_alg(AlgorithmName);
    if (KeyAlg == nullptr)
    {
      throw Exception(FMTLOAD(PUB_KEY_UNKNOWN, AlgorithmName));
    }

    // Check the two public keys match apart from certificates
    strbuf * OldBasePub = strbuf_new();
    ssh_key_public_blob(ssh_key_base_key(Ssh2Key->key), BinarySink_UPCAST(OldBasePub));

    ssh_key * NewPubKey = ssh_key_new_pub(KeyAlg, ptrlen_from_strbuf(Pub));
    strbuf * NewBasePub = strbuf_new();
    ssh_key_public_blob(ssh_key_base_key(NewPubKey), BinarySink_UPCAST(NewBasePub));
    ssh_key_free(NewPubKey);

    bool Match = ptrlen_eq_ptrlen(ptrlen_from_strbuf(OldBasePub), ptrlen_from_strbuf(NewBasePub));
    strbuf_free(OldBasePub);
    strbuf_free(NewBasePub);

    if (!Match)
    {
      throw Exception(FMTLOAD(CERTIFICATE_NOT_MATCH, CertificateFileName));
    }

    strbuf * Priv = strbuf_new_nm();
    ssh_key_private_blob(Ssh2Key->key, BinarySink_UPCAST(Priv));
    ssh_key * NewKey = ssh_key_new_priv(KeyAlg, ptrlen_from_strbuf(Pub), ptrlen_from_strbuf(Priv));
    strbuf_free(Priv);

    if (NewKey == nullptr)
    {
      throw Exception(FMTLOAD(CERTIFICATE_CANNOT_COMBINE, CertificateFileName));
    }

    ssh_key_free(Ssh2Key->key);
    Ssh2Key->key = NewKey;
  },
  __finally
  {
    strbuf_free(Pub);
    sfree(AlgorithmName);
  } end_try__finally
}

void SaveKey(TKeyType KeyType, const UnicodeString & FileName,
  const UnicodeString & Passphrase, TPrivateKey * PrivateKey)
{
  UTF8String UtfFileName = UTF8String(FileName);
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  try__finally
  {
    struct ssh2_userkey * Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);
    AnsiString AnsiPassphrase = AnsiString(Passphrase);
    char * PassphrasePtr = (AnsiPassphrase.IsEmpty() ? nullptr : ToChar(AnsiPassphrase));
    switch (KeyType)
    {
      case ktSSH2:
        {
          ppk_save_parameters Params = ppk_save_default_parameters;
          if (GetConfiguration()->KeyVersion != 0)
          {
            Params.fmt_version = GetConfiguration()->KeyVersion;
          }
          if (!ppk_save_f(KeyFile, Ssh2Key, PassphrasePtr, &Params))
          {
            int Error = errno;
            throw EOSExtException(FMTLOAD(KEY_SAVE_ERROR, FileName), Error);
          }
        }
        break;

      default:
        DebugFail();
        break;
    }
  },
  __finally
  {
    filename_free(KeyFile);
  } end_try__finally
}

void FreeKey(TPrivateKey * PrivateKey)
{
  struct ssh2_userkey * Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);
  ssh_key_free(Ssh2Key->key);
  sfree(Ssh2Key->comment);
  sfree(Ssh2Key);
}

RawByteString StrBufToString(strbuf * StrBuf)
{
  return RawByteString(reinterpret_cast<char *>(StrBuf->s), StrBuf->len);
}

RawByteString LoadPublicKey(
  const UnicodeString & FileName, UnicodeString & Algorithm, UnicodeString & Comment, bool & HasCertificate)
{
  RawByteString Result;
  UTF8String UtfFileName = UTF8String(FileName);
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  try__finally
  {
    char * AlgorithmStr = nullptr;
    char * CommentStr = nullptr;
    const char * ErrorStr = nullptr;
    strbuf * PublicKeyBuf = strbuf_new();
    if (!ppk_loadpub_f(KeyFile, &AlgorithmStr, BinarySink_UPCAST(PublicKeyBuf), &CommentStr, &ErrorStr))
    {
      UnicodeString Error = UnicodeString(AnsiString(ErrorStr));
      throw Exception(Error);
    }
    Algorithm = UnicodeString(AnsiString(AlgorithmStr));
    const ssh_keyalg * KeyAlg = find_pubkey_alg(AlgorithmStr);
    HasCertificate = (KeyAlg != nullptr) && KeyAlg->is_certificate;
    sfree(AlgorithmStr);
    Comment = UnicodeString(AnsiString(CommentStr));
    sfree(CommentStr);
    Result = StrBufToString(PublicKeyBuf);
    strbuf_free(PublicKeyBuf);
  },
  __finally
  {
    filename_free(KeyFile);
  } end_try__finally
  return Result;
}

UnicodeString GetPublicKeyLine(const UnicodeString & FileName, UnicodeString & Comment, bool & HasCertificate)
{
  UnicodeString Algorithm;
  RawByteString PublicKey = LoadPublicKey(FileName, Algorithm, Comment, HasCertificate);
  UnicodeString PublicKeyBase64 = EncodeBase64(PublicKey.c_str(), PublicKey.Length());
  PublicKeyBase64 = ReplaceStr(PublicKeyBase64, L"\r", L"");
  PublicKeyBase64 = ReplaceStr(PublicKeyBase64, L"\n", L"");
  UnicodeString Result = FORMAT(L"%s %s %s", Algorithm, PublicKeyBase64, Comment);
  return Result;
}

bool HasGSSAPI(const UnicodeString & CustomPath)
{
  static int32_t has = -1;
  if (has < 0)
  {
    Conf * conf = conf_new();
    ssh_gss_liblist * List = nullptr;
    try__finally
    {
      Filename * filename = filename_from_str(UTF8String(CustomPath).c_str());
      conf_set_filename(conf, CONF_ssh_gss_custom, filename);
      filename_free(filename);
      List = ssh_gss_setup(conf, nullptr);
      for (int32_t Index = 0; (has <= 0) && (Index < List->nlibraries); Index++)
      {
        ssh_gss_library * library = &List->libraries[Index];
        Ssh_gss_ctx ctx;
        memset(&ctx, 0, sizeof(ctx));
        has =
          ((library->acquire_cred(library, &ctx, nullptr) == SSH_GSS_OK) &&
           (library->release_cred(library, &ctx) == SSH_GSS_OK)) ? 1 : 0;
      }
    },
    __finally
    {
      ssh_gss_cleanup(List);
      conf_free(conf);
    } end_try__finally

    if (has < 0)
    {
      has = 0;
    }
  }
  return (has > 0);
}

static void DoNormalizeFingerprint(UnicodeString & Fingerprint, UnicodeString & KeyName, UnicodeString & KeyType)
{
  cp_ssh_keyalg * SignKeys;
  int32_t Count;
  // We may use find_pubkey_alg, but it gets complicated with normalized fingerprint
  // as the names have different number of dashes
  get_hostkey_algs(-1, &Count, &SignKeys);
  try__finally
  {
    for (int32_t Index = 0; Index < Count; Index++)
    {
      cp_ssh_keyalg SignKey = SignKeys[Index];
      UnicodeString Name = UnicodeString(SignKey->ssh_id);
      if (StartsStr(Name + L" ", Fingerprint))
      {
        UnicodeString Rest = Fingerprint.SubString(Name.Length() + 2, Fingerprint.Length() - Name.Length() - 1);
        int Space = Rest.Pos(L" ");
        // If not a number, it's an invalid input,
        // either something completely wrong, or it can be OpenSSH base64 public key,
        // that got here from TPasteKeyHandler::Paste
        if (IsNumber(Rest.SubString(1, Space - 1)))
        {
          KeyName = Name;
          Fingerprint = Rest.SubString(Space + 1, Fingerprint.Length() - Space);
          Fingerprint = Base64ToUrlSafe(Fingerprint);
          Fingerprint = MD5ToUrlSafe(Fingerprint);
          KeyType = UnicodeString(SignKey->cache_id);
          return;
        }
      }
      else if (StartsStr(Name + NormalizedFingerprintSeparator, Fingerprint))
      {
        KeyType = UnicodeString(SignKey->cache_id);
        KeyName = Name;
        Fingerprint.Delete(1, Name.Length() + 1);
        return;
      }
    }
  },
  __finally
  {
    sfree(SignKeys);
  } end_try__finally
}

void NormalizeFingerprint(UnicodeString & Fingerprint, UnicodeString & KeyName)
{
  UnicodeString KeyType;
  DoNormalizeFingerprint(Fingerprint, KeyName, KeyType);
}

UnicodeString KeyTypeFromFingerprint(const UnicodeString & AFingerprint)
{
  UnicodeString KeyType;
  UnicodeString KeyName; // unused
  UnicodeString Fingerprint = AFingerprint;
  DoNormalizeFingerprint(Fingerprint, KeyName, KeyType);
  return KeyType;
}

UnicodeString GetPuTTYVersion()
{
  // "Release 0.64"
  // "Pre-release 0.65:2015-07-20.95501a1"
  // "Development snapshot 2015-12-22.51465fa"
  UnicodeString Result = get_putty_version();
  // Skip "Release", "Pre-release", "Development snapshot"
  int32_t P = Result.LastDelimiter(L" ");
  Result.Delete(1, P);
  return Result;
}

UnicodeString Sha256(const char * Data, size_t Size)
{
  unsigned char Digest[32];
  hash_simple(&ssh_sha256, make_ptrlen(Data, Size), Digest);
  UnicodeString Result(BytesToHex(Digest, LENOF(Digest)));
  return Result;
}

UnicodeString CalculateFileChecksum(TStream * Stream, const UnicodeString & Alg)
{
  const ssh_hashalg * HashAlg;
  if (SameIdent(Alg, Sha256ChecksumAlg))
  {
    HashAlg = &ssh_sha256;
  }
  else if (SameIdent(Alg, Sha1ChecksumAlg))
  {
    HashAlg = &ssh_sha1;
  }
  else if (SameIdent(Alg, Md5ChecksumAlg))
  {
    HashAlg = &ssh_md5;
  }
  else
  {
    throw Exception(FMTLOAD(UNKNOWN_CHECKSUM, (Alg)));
  }

  UnicodeString Result;
  ssh_hash * Hash = ssh_hash_new(HashAlg);
  try__finally
  {
    const int32_t BlockSize = 32 * 1024;
    TFileBuffer Buffer;
    DWORD Read;
    do
    {
      Buffer.Reset();
      Read = Buffer.LoadStream(Stream, BlockSize, false);
      if (Read > 0)
      {
        put_datapl(Hash, make_ptrlen(Buffer.Data, Read));
      }
    }
    while (Read > 0);
  },
  __finally
  {
    RawByteString Buf;
    Buf.SetLength(ssh_hash_alg(Hash)->hlen);
    ssh_hash_final(Hash, nb::ToUInt8Ptr(nb::ToPtr(Buf.c_str())));
    Result = BytesToHex(Buf);
  } end_try__finally

  return Result;
}

UnicodeString ParseOpenSshPubLine(const UnicodeString & Line, const struct ssh_keyalg *& Algorithm)
{
  UTF8String UtfLine = UTF8String(Line);
  char * AlgorithmName = nullptr;
  char * CommentPtr = nullptr;
  const char * ErrorStr = nullptr;
  strbuf * PubBlobBuf = strbuf_new();
  BinarySource Source[1];
  BinarySource_BARE_INIT(Source, UtfLine.c_str(), UtfLine.Length());
  UnicodeString Result;
  if (!openssh_loadpub(Source, &AlgorithmName, BinarySink_UPCAST(PubBlobBuf), &CommentPtr, &ErrorStr))
  {
    throw Exception(UnicodeString(ErrorStr));
  }
  else
  {
    try__finally
    {
      Algorithm = find_pubkey_alg(AlgorithmName);
      if (Algorithm == nullptr)
      {
        throw Exception(FMTLOAD(PUB_KEY_UNKNOWN, AlgorithmName));
      }

      ptrlen PtrLen = { PubBlobBuf->s, PubBlobBuf->len };
      ssh_key * Key = Algorithm->new_pub(Algorithm, PtrLen);
      if (Key == nullptr)
      {
        throw Exception(L"Invalid public key.");
      }
      char * FmtKey = Algorithm->cache_str(Key);
      Result = UnicodeString(FmtKey);
      sfree(FmtKey);
      Algorithm->freekey(Key);
    },
    __finally
    {
      strbuf_free(PubBlobBuf);
      sfree(AlgorithmName);
      sfree(CommentPtr);
    } end_try__finally
  }
  return Result;
}

// Based on ca_refresh_pubkey_info
void ParseCertificatePublicKey(const UnicodeString & Str, RawByteString & PublicKey, UnicodeString & Fingerprint)
{
  AnsiString AnsiStr = AnsiString(Str);
  ptrlen Data = ptrlen_from_asciz(AnsiStr.c_str());
  strbuf * Blob = strbuf_new();
  try__finally
  {
    // See if we have a plain base64-encoded public key blob.
    if (base64_valid(Data))
    {
      base64_decode_bs(BinarySink_UPCAST(Blob), Data);
    }
    else
    {
      // Otherwise, try to decode as if it was a public key _file_.
      BinarySource Src[1];
      BinarySource_BARE_INIT_PL(Src, Data);
      const char * Error;
      if (!ppk_loadpub_s(Src, nullptr, BinarySink_UPCAST(Blob), nullptr, &Error))
      {
        throw Exception(FMTLOAD(SSH_HOST_CA_DECODE_ERROR, Error));
      }
    }

    ptrlen AlgNamePtrLen = pubkey_blob_to_alg_name(ptrlen_from_strbuf(Blob));
    if (!AlgNamePtrLen.len)
    {
      throw Exception(LoadStr(SSH_HOST_CA_NO_KEY_TYPE));
    }

    UnicodeString AlgName = UnicodeString(AnsiString(static_cast<const char *>(AlgNamePtrLen.ptr), AlgNamePtrLen.len));
    const ssh_keyalg * Alg = find_pubkey_alg_len(AlgNamePtrLen);
    if (Alg == nullptr)
    {
      throw Exception(FMTLOAD(PUB_KEY_UNKNOWN, AlgName));
    }
    if (Alg->is_certificate)
    {
      throw Exception(FMTLOAD(SSH_HOST_CA_CERTIFICATE, AlgName));
    }

    ssh_key * Key = ssh_key_new_pub(Alg, ptrlen_from_strbuf(Blob));
    if (Key == nullptr)
    {
      throw Exception(FMTLOAD(SSH_HOST_CA_INVALID, AlgName));
    }

    char * FingerprintPtr = ssh2_fingerprint(Key, SSH_FPTYPE_DEFAULT);
    Fingerprint = UnicodeString(FingerprintPtr);
    sfree(FingerprintPtr);
    ssh_key_free(Key);

    PublicKey = StrBufToString(Blob);
  },
  __finally
  {
    strbuf_free(Blob);
  } end_try__finally
}

bool IsCertificateValidityExpressionValid(
  const UnicodeString & Str, UnicodeString & Error, int32_t & ErrorStart, int32_t & ErrorLen)
{
  char * ErrorMsg;
  ptrlen ErrorLoc;
  AnsiString StrAnsi(Str);
  const char * StrPtr = StrAnsi.c_str();
  bool Result = cert_expr_valid(StrPtr, &ErrorMsg, &ErrorLoc);
  if (!Result)
  {
    Error = UnicodeString(ErrorMsg);
    sfree(ErrorMsg);
    ErrorStart = static_cast<const char *>(ErrorLoc.ptr) - StrPtr;
    ErrorLen = ErrorLoc.len;
  }
  return Result;
}

bool IsOpenSSH(const UnicodeString & SshImplementation)
{
  return
    // e.g. "OpenSSH_5.3"
    (SshImplementation.Pos(L"OpenSSH") == 1) ||
    // Sun SSH is based on OpenSSH (suffers the same bugs)
    (SshImplementation.Pos(L"Sun_SSH") == 1);
}

// Same order as DefaultCipherList
struct TCipherGroup
{
  int32_t CipherGroup;
  const ssh2_ciphers * Cipher;
};
TCipherGroup Ciphers[] =
  {
    { CIPHER_AES, &ssh2_aes },
    { CIPHER_CHACHA20, &ssh2_ccp },
    { CIPHER_AESGCM, &ssh2_aesgcm },
    { CIPHER_3DES, &ssh2_3des },
    { CIPHER_DES, &ssh2_des },
    { CIPHER_BLOWFISH, &ssh2_blowfish },
    { CIPHER_ARCFOUR, &ssh2_arcfour },
  };

TStrings * SshCipherList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  for (uint32_t Index = 0; Index < LENOF(Ciphers); Index++)
  {
    const ssh2_ciphers * Cipher = Ciphers[Index].Cipher;
    for (int32_t Index2 = 0; Index2 < Cipher->nciphers; Index2++)
    {
      UnicodeString Name = UnicodeString(Cipher->list[Index2]->ssh2_id);
      Result->Add(Name);
    }
  }
  return Result.release();
}

int32_t GetCipherGroup(const ssh_cipher * TheCipher)
{
  DebugAssert(strlen(TheCipher->vt->ssh2_id) > 0);
  for (uint32_t Index = 0; Index < LENOF(Ciphers); Index++)
  {
    TCipherGroup & CipherGroup = Ciphers[Index];
    const ssh2_ciphers * Cipher = CipherGroup.Cipher;
    for (int32_t Index2 = 0; Index2 < Cipher->nciphers; Index2++)
    {
      if (strcmp(TheCipher->vt->ssh2_id, Cipher->list[Index2]->ssh2_id) == 0)
      {
        return CipherGroup.CipherGroup;
      }
    }
  }
  DebugFail();
  return -1;
}

TStrings * SshKexList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  // Same order as DefaultKexList
  const ssh_kexes * Kexes[] = {
    &ssh_gssk5_ecdh_kex, &ssh_gssk5_sha2_kex, &ssh_gssk5_sha1_kex,
    &ssh_ntru_hybrid_kex, &ssh_ecdh_kex, &ssh_diffiehellman_gex,
    &ssh_diffiehellman_group18, &ssh_diffiehellman_group17, &ssh_diffiehellman_group16, &ssh_diffiehellman_group15, &ssh_diffiehellman_group14,
    &ssh_rsa_kex, &ssh_diffiehellman_group1 };
  for (uint32_t Index = 0; Index < LENOF(Kexes); Index++)
  {
    for (int32_t Index2 = 0; Index2 < Kexes[Index]->nkexes; Index2++)
    {
      UnicodeString Name = UnicodeString(Kexes[Index]->list[Index2]->name);
      Result->Add(Name);
    }
  }
  return Result.release();
}

int HostKeyToPutty(THostKey HostKey)
{
  int Result;
  switch (HostKey)
  {
    case hkWarn: Result = HK_WARN; break;
    case hkRSA: Result = HK_RSA; break;
    case hkDSA: Result = hkDSA; break;
    case hkECDSA: Result = HK_ECDSA; break;
    case hkED25519: Result = HK_ED25519; break;
    case hkED448: Result = HK_ED448; break;
    default: Result = -1; DebugFail();
  }
  return Result;
}

TStrings * SshHostKeyList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  for (int32_t DefaultIndex = 0; DefaultIndex < HOSTKEY_COUNT; DefaultIndex++)
  {
    int32_t Type = HostKeyToPutty(DefaultHostKeyList[DefaultIndex]);
    cp_ssh_keyalg * SignKeys;
    int32_t Count;
    get_hostkey_algs(Type, &Count, &SignKeys);
    try__finally
    {
      for (int32_t Index = 0; Index < Count; Index++)
      {
        cp_ssh_keyalg SignKey = SignKeys[Index];
        UnicodeString Name = UnicodeString(SignKey->ssh_id);
        Result->Add(Name);
      }
    },
    __finally
    {
      sfree(SignKeys);
    } end_try__finally
  }
  return Result.release();
}

TStrings * SshMacList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  const struct ssh2_macalg ** Macs = nullptr;
  int32_t Count = 0;
  get_macs(&Count, &Macs);

  for (int32_t Index = 0; Index < Count; Index++)
  {
    UnicodeString Name = UnicodeString(Macs[Index]->name);
    UnicodeString S = Name;
    UnicodeString ETMName = UnicodeString(Macs[Index]->etm_name);
    if (!ETMName.IsEmpty())
    {
      S = FORMAT(L"%s (%s)", S, ETMName);
    }
    Result->Add(S);
  }
  return Result.release();
}

UnicodeString GetCipherName(const ssh_cipher * Cipher)
{
  return UnicodeString(UTF8String(Cipher->vt->text_name));
}

UnicodeString GetCompressorName(const ssh_compressor * Compressor)
{
  UnicodeString Result;
  if (Compressor != nullptr)
  {
    Result = UnicodeString(UTF8String(Compressor->vt->name));
  }
  return Result;
}

UnicodeString GetDecompressorName(const ssh_decompressor * Decompressor)
{
  UnicodeString Result;
  if (Decompressor != nullptr)
  {
    Result = UnicodeString(UTF8String(Decompressor->vt->name));
  }
  return Result;
}

void WritePuttySettings(THierarchicalStorage * Storage, const UnicodeString & ASettings)
{
  if (PuttyRegistryTypes.empty())
  {
    TGuard Guard(PuttyRegistrySection);
    TValueRestorer<TPuttyRegistryMode> PuttyRegistryModeRestorer(PuttyRegistryMode);
    PuttyRegistryMode = prmCollect;
    Conf * conf = conf_new();
    try__finally
    {
      do_defaults(nullptr, conf);
      save_settings(nullptr, conf);
    },
    __finally
    {
      conf_free(conf);
    } end_try__finally
  }

  std::unique_ptr<TStrings> Settings(std::make_unique<TStringList>());
  UnicodeString Buf = ASettings;
  UnicodeString Setting;
  while (CutToken(Buf, Setting))
  {
    Settings->Add(Setting);
  }

  for (int32_t Index = 0; Index < Settings->Count; Index++)
  {
    UnicodeString Name = Settings->GetName(Index);
    TPuttyRegistryTypes::const_iterator IType = PuttyRegistryTypes.find(Name);
    if (IType != PuttyRegistryTypes.end())
    {
      UnicodeString Value = Settings->GetValueFromIndex(Index);
      int32_t I{0};
      if (IType->second == REG_SZ)
      {
        Storage->WriteStringRaw(Name, Value);
      }
      else if (DebugAlwaysTrue(IType->second == REG_DWORD) &&
               TryStrToInt(Value, I))
      {
        Storage->WriteInteger(Name, I);
      }
    }
  }
}

void PuttyDefaults(Conf * conf)
{
  TGuard Guard(PuttyRegistrySection);
  TValueRestorer<TPuttyRegistryMode> PuttyRegistryModeRestorer(PuttyRegistryMode);
  PuttyRegistryMode = prmFail;
  do_defaults(nullptr, conf);
}

void SavePuttyDefaults(const UnicodeString & Name)
{
  TGuard Guard(PuttyRegistrySection);
  TValueRestorer<TPuttyRegistryMode> PuttyRegistryModeRestorer(PuttyRegistryMode);
  PuttyRegistryMode = prmPass;
  Conf * conf = conf_new();
  try__finally
  {
    PuttyDefaults(conf);
    AnsiString PuttyName = PuttyStr(Name);
    save_settings(PuttyName.c_str(), conf);
  },
  __finally
  {
    conf_free(conf);
  } end_try__finally
}

struct host_ca_enum
{
  int Index;
};

host_ca_enum * enum_host_ca_start()
{
  GetConfiguration()->RefreshPuttySshHostCAList();
  host_ca_enum * Result = new host_ca_enum();
  Result->Index = 0;
  return Result;
}

bool enum_host_ca_next(host_ca_enum * Enum, strbuf * StrBuf)
{
  const TSshHostCAList * SshHostCAList = GetConfiguration()->GetActiveSshHostCAList();
  bool Result = (Enum->Index < SshHostCAList->GetCount());
  if (Result)
  {
    put_asciz(StrBuf, UTF8String(SshHostCAList->Get(Enum->Index)->Name).c_str());
    Enum->Index++;
  }
  return Result;
}

void enum_host_ca_finish(host_ca_enum * Enum)
{
  delete Enum;
}

host_ca * host_ca_load(const char * NameStr)
{
  host_ca * Result = nullptr;
  UnicodeString Name = UTF8String(NameStr);
  const TSshHostCA * SshHostCA = GetConfiguration()->GetActiveSshHostCAList()->Find(Name);
  if (DebugAlwaysTrue(SshHostCA != nullptr))
  {
    Result = host_ca_new();
    Result->name = dupstr(NameStr);
    Result->ca_public_key = strbuf_dup(make_ptrlen(SshHostCA->PublicKey.c_str(), SshHostCA->PublicKey.Length()));
    Result->validity_expression = dupstr(UTF8String(SshHostCA->ValidityExpression).c_str());
    Result->opts.permit_rsa_sha1 = SshHostCA->PermitRsaSha1;
    Result->opts.permit_rsa_sha256 = SshHostCA->PermitRsaSha256;
    Result->opts.permit_rsa_sha512 = SshHostCA->PermitRsaSha512;
  }
  return Result;
}


