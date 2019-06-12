//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#ifndef PUTTY_DO_GLOBALS
#define PUTTY_DO_GLOBALS
#endif
#include <Exceptions.h>
#include <StrUtils.hpp>

#include "PuttyIntf.h"
#include "Interface.h"
#include "SecureShell.h"
#include "CoreMain.h"
#include "TextsCore.h"
__removed #include <Soap.EncdDecd.hpp>
//---------------------------------------------------------------------------
char sshver[50]{};
extern const char commitid[] = "";
const int platform_uses_x11_unix_by_default = TRUE;
CRITICAL_SECTION putty_section;
bool SaveRandomSeed;
char appname_[50]{};
const char *const appname = appname_;
extern "C" const int share_can_be_downstream = FALSE;
extern "C" const int share_can_be_upstream = FALSE;
//---------------------------------------------------------------------------
extern "C"
{
#include <winstuff.h>
}
const UnicodeString OriginalPuttyRegistryStorageKey(PUTTY_REG_POS);
const UnicodeString KittyRegistryStorageKey("Software\\9bis.com\\KiTTY");
const UnicodeString OriginalPuttyExecutable("putty.exe");
const UnicodeString KittyExecutable("kitty.exe");
const UnicodeString PuttyKeyExt("ppk");
//---------------------------------------------------------------------------
void PuttyInitialize()
{
  SaveRandomSeed = true;

  InitializeCriticalSection(&putty_section);

  // make sure random generator is initialised, so random_save_seed()
  // in destructor can proceed
  random_ref();

  flags = FLAG_VERBOSE | FLAG_SYNCAGENT; // verbose log

  sk_init();

  AnsiString VersionString = AnsiString(GetSshVersionString());
  DebugAssert(!VersionString.IsEmpty() && (nb::ToSizeT(VersionString.Length()) < _countof(sshver)));
  strcpy_s(sshver, sizeof(sshver), VersionString.c_str());
  AnsiString AppName = AnsiString(GetAppNameString());
  DebugAssert(!AppName.IsEmpty() && (nb::ToSizeT(AppName.Length()) < _countof(appname_)));
  strcpy_s(appname_, sizeof(appname_), AppName.c_str());
}
//---------------------------------------------------------------------------
void PuttyFinalize()
{
  if (SaveRandomSeed)
  {
    random_save_seed();
  }
  random_unref();

  sk_cleanup();
  win_misc_cleanup();
  win_secur_cleanup();
  ec_cleanup();
  DeleteCriticalSection(&putty_section);
}
//---------------------------------------------------------------------------
void DontSaveRandomSeed()
{
  SaveRandomSeed = false;
}
//---------------------------------------------------------------------------
extern "C" char *do_select(Plug plug, SOCKET skt, int startup)
{
  void *frontend;

  if (!is_ssh(plug) && !is_pfwd(plug))
  {
    // If it is not SSH/PFwd plug, then it must be Proxy plug.
    // Get SSH/PFwd plug which it wraps.
    Proxy_Socket ProxySocket = (reinterpret_cast<Proxy_Plug>(plug))->proxy_socket;
    plug = ProxySocket->plug;
  }

  bool pfwd = is_pfwd(plug) != 0;
  if (pfwd)
  {
    plug = static_cast<Plug>(get_pfwd_backend(plug));
  }

  frontend = get_ssh_frontend(plug);
  DebugAssert(frontend);

  TSecureShell *SecureShell = get_as<TSecureShell>(frontend);
  if (!pfwd)
  {
    SecureShell->UpdateSocket(skt, startup != 0);
  }
  else
  {
    SecureShell->UpdatePortFwdSocket(skt, startup != 0);
  }

  return nullptr;
}
//---------------------------------------------------------------------------
int from_backend(void *frontend, int is_stderr, const char *data, int datalen)
{
  DebugAssert(frontend);
  TSecureShell *SecureShell = get_as<TSecureShell>(frontend);
  DebugAssert(SecureShell);
  if (is_stderr >= 0)
  {
    DebugAssert((is_stderr == 0) || (is_stderr == 1));
    SecureShell->FromBackend((is_stderr == 1), reinterpret_cast<const uint8_t *>(data), datalen);
  }
  else
  {
    DebugAssert(is_stderr == -1);
    SecureShell->CWrite(data, datalen);
  }
  return 0;
}
//---------------------------------------------------------------------------
int from_backend_untrusted(void * /*frontend*/, const char * /*data*/, int /*len*/)
{
  // currently used with authentication banner only,
  // for which we have own interface display_banner
  return 0;
}
//---------------------------------------------------------------------------
int from_backend_eof(void * /*frontend*/)
{
  return FALSE;
}

int GetUserpassInput(prompts_t *p, const uint8_t * /*in*/, int /*inlen*/);
//---------------------------------------------------------------------------
int get_userpass_input(prompts_t *p, const uint8_t *in, int inlen)
{
  return GetUserpassInput(p, in, inlen);
}

int GetUserpassInput(prompts_t *p, const uint8_t * /*in*/, int /*inlen*/)
{
  DebugAssert(p != nullptr);
  if (!p)
    return -1;
  TSecureShell *SecureShell = get_as<TSecureShell>(p->frontend);
  DebugAssert(SecureShell != nullptr);
  if (!SecureShell)
    return -1;

  int Result;
  std::unique_ptr<TStrings> Prompts(std::make_unique<TStringList>());
  std::unique_ptr<TStrings> Results(std::make_unique<TStringList>());
  try__finally
  {
    UnicodeString Name = UTF8ToString(p->name);
    UnicodeString AName = Name;
    TPromptKind PromptKind = SecureShell->IdentifyPromptKind(AName);
    bool UTF8Prompt = (PromptKind != pkPassphrase);

    for (size_t Index = 0; Index < p->n_prompts; ++Index)
    {
      prompt_t *Prompt = p->prompts[Index];
      UnicodeString S;
      if (UTF8Prompt)
      {
        S = UTF8ToString(Prompt->prompt);
      }
      else
      {
        S = UnicodeString(AnsiString(Prompt->prompt));
      }
      Prompts->AddObject(S, ToObj(FLAGMASK(Prompt->echo, pupEcho)));
      // this fails, when new passwords do not match on change password prompt,
      // and putty retries the prompt
      DebugAssert(Prompt->resultsize == 0);
      Results->Add(L"");
    }

    UnicodeString Instructions = UTF8ToString(p->instruction);
    if (SecureShell->PromptUser(p->to_server != 0, Name, p->name_reqd != 0,
        Instructions, p->instr_reqd != 0, Prompts.get(), Results.get()))
    {
      for (size_t Index = 0; Index < p->n_prompts; ++Index)
      {
        prompt_t *Prompt = p->prompts[Index];
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
      Result = 1;
    }
    else
    {
      Result = 0;
    }
  },
  __finally__removed
  ({
    delete Prompts;
    delete Results;
  }) end_try__finally

  return Result;
}
//---------------------------------------------------------------------------
char *get_ttymode(void * /*frontend*/, const char * /*mode*/)
{
  // should never happen when Config.nopty == TRUE
  DebugFail();
  return nullptr;
}
//---------------------------------------------------------------------------
void logevent(void *frontend, const char *str)
{
  // Frontend maybe NULL here
  if (frontend != nullptr)
  {
    get_as<TSecureShell>(frontend)->PuttyLogEvent(str);
  }
}
//---------------------------------------------------------------------------
void connection_fatal(void *frontend, const char *fmt, ...)
{
  va_list Param;
  AnsiString Str;
  char *Buf = Str.SetLength(32 * 1024);
  va_start(Param, fmt);
  vsnprintf_s(Buf, Str.GetLength(), _TRUNCATE, fmt, Param);
  Str[Str.GetLength() - 1] = '\0';
  va_end(Param);

  DebugAssert(frontend != nullptr);
  get_as<TSecureShell>(frontend)->PuttyFatalError(UnicodeString(Str));
}
//---------------------------------------------------------------------------
int verify_ssh_host_key(void *frontend, char *host, int port, const char *keytype,
  char *keystr, char *fingerprint, void ( * /*callback*/)(void *ctx, int result),
  void * /*ctx*/)
{
  DebugAssert(frontend != nullptr);
  get_as<TSecureShell>(frontend)->VerifyHostKey(UnicodeString(host), port, keytype, keystr, fingerprint);

  // We should return 0 when key was not confirmed, we throw exception instead.
  return 1;
}
//---------------------------------------------------------------------------
int have_ssh_host_key(void *frontend, const char *hostname, int port,
  const char *keytype)
{
  DebugAssert(frontend != nullptr);
  return static_cast<TSecureShell *>(frontend)->HaveHostKey(hostname, port, keytype) ? 1 : 0;
}
//---------------------------------------------------------------------------
int askalg(void *frontend, const char *algtype, const char *algname,
  void ( * /*callback*/)(void *ctx, int result), void * /*ctx*/)
{
  DebugAssert(frontend != nullptr);
  get_as<TSecureShell>(frontend)->AskAlg(algtype, algname);

  // We should return 0 when alg was not confirmed, we throw exception instead.
  return 1;
}
//---------------------------------------------------------------------------
int askhk(void * /*frontend*/, const char * /*algname*/, const char * /*betteralgs*/,
  void ( * /*callback*/)(void *ctx, int result), void * /*ctx*/)
{
  return 1;
}
//---------------------------------------------------------------------------
void old_keyfile_warning()
{
  // no reference to TSecureShell instance available
}
//---------------------------------------------------------------------------
void display_banner(void *frontend, const char *banner, int size)
{
  DebugAssert(frontend);
  UnicodeString Banner(banner, size);
  get_as<TSecureShell>(frontend)->DisplayBanner(Banner);
}
//---------------------------------------------------------------------------
static void SSHFatalError(const char *Format, va_list Param)
{
  AnsiString Str;
  char *Buf = Str.SetLength(32 * 1024);
  vsnprintf_s(Buf, Str.GetLength(), _TRUNCATE, Format, Param);
  Str[Str.GetLength() - 1] = '\0';

  // Only few calls from putty\winnet.c might be connected with specific
  // TSecureShell. Otherwise called only for really fatal errors
  // like 'out of memory' from putty\ssh.c.
  throw ESshFatal(nullptr, Str.c_str());
}
//---------------------------------------------------------------------------
void fatalbox(const char *fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}
//---------------------------------------------------------------------------
void modalfatalbox(const char *fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}
//---------------------------------------------------------------------------
void nonfatal(const char *fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}

void CleanupExit(int /*code*/)
{
  throw ESshFatal(nullptr, L"");
}
//---------------------------------------------------------------------------
void cleanup_exit(int code)
{
  CleanupExit(code);
}
//---------------------------------------------------------------------------
int askappend(void * /*frontend*/, Filename * /*filename*/,
  void ( * /*callback*/)(void *ctx, int result), void * /*ctx*/)
{
  // this is called from logging.c of putty, which is never used with WinSCP
  DebugFail();
  return 0;
}
//---------------------------------------------------------------------------
void ldisc_echoedit_update(void * /*handle*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void agent_schedule_callback(void ( * /*callback*/)(void *, void *, int),
  void * /*callback_ctx*/, void * /*data*/, int /*len*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void notify_remote_exit(void * /*frontend*/)
{
  // nothing
}
//---------------------------------------------------------------------------
void update_specials_menu(void * /*frontend*/)
{
  // nothing
}
//---------------------------------------------------------------------------
unsigned long schedule_timer(int ticks, timer_fn_t /*fn*/, void * /*ctx*/)
{
  return ticks + ::GetTickCount();
}
//---------------------------------------------------------------------------
void expire_timer_context(void * /*ctx*/)
{
  // nothing
}
//---------------------------------------------------------------------------
Pinger pinger_new(Conf * /*conf*/, Backend * /*back*/, void * /*backhandle*/)
{
  return nullptr;
}
//---------------------------------------------------------------------------
void pinger_reconfig(Pinger /*pinger*/, Conf * /*oldconf*/, Conf * /*newconf*/)
{
  // nothing
}
//---------------------------------------------------------------------------
void pinger_free(Pinger /*pinger*/)
{
  // nothing
}
//---------------------------------------------------------------------------
void set_busy_status(void * /*frontend*/, int /*status*/)
{
  // nothing
}
//---------------------------------------------------------------------------
void platform_get_x11_auth(struct X11Display * /*display*/, Conf * /*conf*/)
{
  // nothing, therefore no auth.
}
//---------------------------------------------------------------------------
// Based on PuTTY's settings.c
char *get_remote_username(Conf *conf)
{
  char *username = conf_get_str(conf, CONF_username);
  char *result = nullptr;
  if (*username)
  {
    result = dupstr(username);
  }
  return result;
}
//---------------------------------------------------------------------------
static long OpenWinSCPKey(HKEY Key, const char *SubKey, HKEY *Result, bool CanCreate)
{
  long R;
  DebugAssert(GetConfiguration() != nullptr);

  DebugAssert(Key == HKEY_CURRENT_USER);
  DebugUsedParam(Key);

  UnicodeString RegKey = SubKey;
  intptr_t PuttyKeyLen = OriginalPuttyRegistryStorageKey.Length();
  DebugAssert(RegKey.SubString(1, PuttyKeyLen) == OriginalPuttyRegistryStorageKey);
  RegKey = RegKey.SubString(PuttyKeyLen + 1, RegKey.Length() - PuttyKeyLen);
  if (!RegKey.IsEmpty())
  {
    DebugAssert(RegKey[1] == L'\\');
    RegKey.Delete(1, 1);
  }

  if (RegKey.IsEmpty())
  {
    *Result = static_cast<HKEY>(nullptr);
    R = ERROR_SUCCESS;
  }
  else
  {
    // we expect this to be called only from verify_host_key() or store_host_key()
    DebugAssert(RegKey == L"SshHostKeys");

    std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
    Storage->SetAccessMode((CanCreate ? smReadWrite : smRead));
    if (Storage->OpenSubKey(RegKey, CanCreate))
    {
      *Result = reinterpret_cast<HKEY>(Storage.release());
      R = ERROR_SUCCESS;
    }
    else
    {
      R = ERROR_CANTOPEN;
    }
  }

  return R;
}
//---------------------------------------------------------------------------
long reg_open_winscp_key(HKEY Key, const char *SubKey, HKEY *Result)
{
  return OpenWinSCPKey(Key, SubKey, Result, false);
}
//---------------------------------------------------------------------------
long reg_create_winscp_key(HKEY Key, const char *SubKey, HKEY *Result)
{
  return OpenWinSCPKey(Key, SubKey, Result, true);
}
//---------------------------------------------------------------------------
long reg_query_winscp_value_ex(HKEY Key, const char *ValueName, unsigned long * /*Reserved*/,
  unsigned long *Type, uint8_t *Data, unsigned long *DataSize)
{
  long R;
  DebugAssert(GetConfiguration() != nullptr);

  THierarchicalStorage *Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  AnsiString Value;
  if (Storage == nullptr)
  {
    if (UnicodeString(ValueName) == L"RandSeedFile")
    {
      Value = AnsiString(GetConfiguration()->GetRandomSeedFileName());
      R = ERROR_SUCCESS;
    }
    else
    {
      DebugFail();
      R = ERROR_READ_FAULT;
    }
  }
  else
  {
    if (Storage->ValueExists(ValueName))
    {
      Value = AnsiString(Storage->ReadStringRaw(ValueName, L""));
      R = ERROR_SUCCESS;
    }
    else
    {
      R = ERROR_READ_FAULT;
    }
  }

  if ((R == ERROR_SUCCESS) && Type)
  {
    DebugAssert(Type != nullptr);
    *Type = REG_SZ;
    char *DataStr = reinterpret_cast<char *>(Data);
    int sz = nb::ToInt(*DataSize);
    if (sz > 0)
    {
      strncpy(DataStr, Value.c_str(), sz);
      DataStr[sz - 1] = '\0';
    }
    *DataSize = nb::ToUInt32(NBChTraitsCRT<char>::SafeStringLen(DataStr));
  }

  return R;
}
//---------------------------------------------------------------------------
long reg_set_winscp_value_ex(HKEY Key, const char *ValueName, unsigned long /*Reserved*/,
  unsigned long Type, const uint8_t *Data, unsigned long DataSize)
{
  DebugAssert(GetConfiguration() != nullptr);

  DebugAssert(Type == REG_SZ);
  DebugUsedParam(Type);
  THierarchicalStorage *Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  DebugAssert(Storage != nullptr);
  if (Storage != nullptr)
  {
    UnicodeString Value(reinterpret_cast<const char *>(Data), DataSize - 1);
    Storage->WriteStringRaw(ValueName, Value);
  }

  return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
long reg_close_winscp_key(HKEY Key)
{
  DebugAssert(GetConfiguration() != nullptr);

  THierarchicalStorage *Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  if (Storage != nullptr)
  {
    SAFE_DESTROY_EX(THierarchicalStorage, Storage);
  }

  return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
TKeyType GetKeyType(UnicodeString AFileName)
{
  DebugAssert(ktUnopenable == static_cast<TKeyType>(SSH_KEYTYPE_UNOPENABLE));
  DebugAssert(ktSSHCom == static_cast<TKeyType>(SSH_KEYTYPE_SSHCOM));
  DebugAssert(ktSSH2PublicOpenSSH == static_cast<TKeyType>(SSH_KEYTYPE_SSH2_PUBLIC_OPENSSH));
  UTF8String UtfFileName = UTF8String(::ExpandEnvironmentVariables(AFileName));
  Filename *KeyFile = filename_from_str(UtfFileName.c_str());
  TKeyType Result = static_cast<TKeyType>(key_type(KeyFile));
  filename_free(KeyFile);
  return Result;
}
//---------------------------------------------------------------------------
bool IsKeyEncrypted(TKeyType KeyType, UnicodeString FileName, UnicodeString &Comment)
{
  UTF8String UtfFileName = UTF8String(::ExpandEnvironmentVariables(FileName));
  bool Result;
  char *CommentStr = nullptr;
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  try__finally
  {
    switch (KeyType)
    {
      case ktSSH2:
        Result = (ssh2_userkey_encrypted(KeyFile, &CommentStr) != 0);
        break;

      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        Result = (import_encrypted(KeyFile, KeyType, &CommentStr) != NULL);
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
  filename_free(KeyFile);
  return Result;
}
//---------------------------------------------------------------------------
TPrivateKey *LoadKey(TKeyType KeyType, UnicodeString FileName, UnicodeString Passphrase)
{
  UTF8String UtfFileName = UTF8String(::ExpandEnvironmentVariables(FileName));
  Filename *KeyFile = filename_from_str(UtfFileName.c_str());
  struct ssh2_userkey *Ssh2Key = nullptr;
  const char *ErrorStr = nullptr;
  AnsiString AnsiPassphrase = Passphrase;
  try__finally
  {
    switch (KeyType)
    {
      case ktSSH2:
        Ssh2Key = ssh2_load_userkey(KeyFile, AnsiPassphrase.c_str(), &ErrorStr);
        break;

      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        Ssh2Key = import_ssh2(KeyFile, KeyType, AnsiPassphrase.c_str(), &ErrorStr);
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
    UnicodeString Error = UnicodeString(ErrorStr);
    // While theoretically we may get "unable to open key file" and
    // so we should check system error code,
    // we actually never get here unless we call KeyType previously
    // and handle ktUnopenable accordingly.
    throw Exception(Error);
  }
  if (Ssh2Key == SSH2_WRONG_PASSPHRASE)
  {
    throw Exception(LoadStr(AUTH_TRANSL_WRONG_PASSPHRASE));
  }
  filename_free(KeyFile);
  return reinterpret_cast<TPrivateKey *>(Ssh2Key);
}
//---------------------------------------------------------------------------
void ChangeKeyComment(TPrivateKey *PrivateKey, UnicodeString Comment)
{
  AnsiString AnsiComment(Comment);
  struct ssh2_userkey *Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);
  sfree(Ssh2Key->comment);
  Ssh2Key->comment = dupstr(AnsiComment.c_str());
}
//---------------------------------------------------------------------------
void SaveKey(TKeyType KeyType, UnicodeString FileName,
  UnicodeString Passphrase, TPrivateKey *PrivateKey)
{
  UTF8String UtfFileName = UTF8String(::ExpandEnvironmentVariables(FileName));
  Filename *KeyFile = filename_from_str(UtfFileName.c_str());
  try__finally
  {
    struct ssh2_userkey * Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);
    AnsiString AnsiPassphrase = Passphrase;
    char * PassphrasePtr = const_cast<char *>(AnsiPassphrase.IsEmpty() ? nullptr : AnsiPassphrase.c_str());
    switch (KeyType)
    {
      case ktSSH2:
        if (!ssh2_save_userkey(KeyFile, Ssh2Key, PassphrasePtr))
        {
          int Error = errno;
          throw EOSExtException(FMTLOAD(KEY_SAVE_ERROR, (FileName)), Error);
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
//---------------------------------------------------------------------------
void FreeKey(TPrivateKey *PrivateKey)
{
  struct ssh2_userkey *Ssh2Key = reinterpret_cast<struct ssh2_userkey *>(PrivateKey);
  Ssh2Key->alg->freekey(Ssh2Key->data);
  sfree(Ssh2Key->comment);
  sfree(Ssh2Key);
}
//---------------------------------------------------------------------------
RawByteString LoadPublicKey(UnicodeString & FileName, UnicodeString & Algorithm, UnicodeString & Comment)
{
  RawByteString Result;
  UTF8String UtfFileName = UTF8String(FileName);
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  try__finally
  {
    char * AlgorithmStr = nullptr;
    int PublicKeyLen = 0;
    char * CommentStr = nullptr;
    const char * ErrorStr = nullptr;
    unsigned char * PublicKeyPtr =
      ssh2_userkey_loadpub(KeyFile, &AlgorithmStr, &PublicKeyLen, &CommentStr, &ErrorStr);
    if (PublicKeyPtr == nullptr)
    {
      UnicodeString Error = UnicodeString(AnsiString(ErrorStr));
      throw Exception(Error);
    }
    Algorithm = UnicodeString(AnsiString(AlgorithmStr));
    sfree(AlgorithmStr);
    Comment = UnicodeString(AnsiString(CommentStr));
    sfree(CommentStr);
    Result = RawByteString(reinterpret_cast<char *>(PublicKeyPtr), PublicKeyLen);
    free(PublicKeyPtr);
  },
  __finally
  {
    filename_free(KeyFile);
  } end_try__finally
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString GetPublicKeyLine(UnicodeString & FileName, UnicodeString & Comment)
{
  UnicodeString Algorithm;
  RawByteString PublicKey = LoadPublicKey(FileName, Algorithm, Comment);
  UnicodeString PublicKeyBase64 = EncodeBase64(PublicKey.c_str(), PublicKey.Length());
  PublicKeyBase64 = ReplaceStr(PublicKeyBase64, L"\r", L"");
  PublicKeyBase64 = ReplaceStr(PublicKeyBase64, L"\n", L"");
  UnicodeString Result = FORMAT("%s %s %s", Algorithm, PublicKeyBase64, Comment);
  return Result;
}
//---------------------------------------------------------------------------
bool HasGSSAPI(UnicodeString CustomPath)
{
  static int has = -1;
  if (has < 0)
  {
    Conf *conf = conf_new();
    ssh_gss_liblist *List = nullptr;
    try__finally
    {
      Filename *filename = filename_from_str(UTF8String(CustomPath).c_str());
      conf_set_filename(conf, CONF_ssh_gss_custom, filename);
      filename_free(filename);
      List = ssh_gss_setup(conf, nullptr);
      for (intptr_t Index = 0; (has <= 0) && (Index < List->nlibraries); ++Index)
      {
        ssh_gss_library *library = &List->libraries[Index];
        Ssh_gss_ctx ctx;
        ::ZeroMemory(&ctx, sizeof(ctx));
        has =
          ((library->acquire_cred(library, &ctx) == SSH_GSS_OK) &&
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
//---------------------------------------------------------------------------
static void DoNormalizeFingerprint(UnicodeString &Fingerprint, UnicodeString &KeyType)
{
  const wchar_t NormalizedSeparator = L'-';
  const int MaxCount = 10;
  const ssh_signkey *SignKeys[MaxCount];
  int Count = _countof(SignKeys);
  // We may use find_pubkey_alg, but it gets complicated with normalized fingerprint
  // as the names have different number of dashes
  get_hostkey_algs(&Count, SignKeys);

  for (intptr_t Index = 0; Index < Count; Index++)
  {
    const ssh_signkey *SignKey = SignKeys[Index];
    UnicodeString Name = UnicodeString(SignKey->name);
    if (::StartsStr(Name + L" ", Fingerprint))
    {
      intptr_t LenStart = Name.Length() + 1;
      Fingerprint[LenStart] = NormalizedSeparator;
      intptr_t Space = Fingerprint.Pos(L" ");
      // If not a number, it's an invalid input,
      // either something completely wrong, or it can be OpenSSH base64 public key,
      // that got here from TPasteKeyHandler::Paste
      if (IsNumber(Fingerprint.SubString(LenStart + 1, Space - LenStart - 1)))
      {
        Fingerprint.Delete(LenStart + 1, Space - LenStart);
        // noop for SHA256 fingerprints
        Fingerprint = ReplaceChar(Fingerprint, L':', NormalizedSeparator);
        KeyType = UnicodeString(SignKey->keytype);
        return;
      }
    }
    else if (StartsStr(Name + NormalizedSeparator, Fingerprint))
    {
      KeyType = UnicodeString(SignKey->keytype);
      return;
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString NormalizeFingerprint(UnicodeString AFingerprint)
{
  UnicodeString Fingerprint = AFingerprint;
  UnicodeString KeyType; // unused
  DoNormalizeFingerprint(Fingerprint, KeyType);
  return Fingerprint;
}
//---------------------------------------------------------------------------
UnicodeString GetKeyTypeFromFingerprint(UnicodeString AFingerprint)
{
  UnicodeString KeyType;
  DoNormalizeFingerprint(AFingerprint, KeyType);
  return KeyType;
}
//---------------------------------------------------------------------------
UnicodeString GetPuTTYVersion()
{
  // "Release 0.64"
  // "Pre-release 0.65:2015-07-20.95501a1"
  // "Development snapshot 2015-12-22.51465fa"
  UnicodeString Result = get_putty_version();
  // Skip "Release", "Pre-release", "Development snapshot"
  intptr_t P = Result.LastDelimiter(L" ");
  Result.Delete(1, P);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString Sha256(const char *Data, size_t Size)
{
  uint8_t Digest[32]{};
  putty_SHA256_Simple(Data, nb::ToInt(Size), Digest);
  UnicodeString Result(BytesToHex(Digest, _countof(Digest)));
  return Result;
}
//---------------------------------------------------------------------------
void DllHijackingProtection()
{
  dll_hijacking_protection();
}
//---------------------------------------------------------------------------
UnicodeString ParseOpenSshPubLine(UnicodeString ALine, const struct ssh_signkey *& Algorithm)
{
  UTF8String UtfLine = UTF8String(ALine);
  char * AlgorithmName = nullptr;
  int PubBlobLen = 0;
  char * CommentPtr = nullptr;
  const char * ErrorStr = nullptr;
  uint8_t * PubBlob = openssh_loadpub_line(UtfLine.data(), &AlgorithmName, &PubBlobLen, &CommentPtr, &ErrorStr);
  UnicodeString Result;
  if (PubBlob == nullptr)
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
        throw Exception(FORMAT("Unknown public key algorithm \"%s\".", AlgorithmName));
      }

      void * Key = Algorithm->newkey(Algorithm, reinterpret_cast<const char*>(PubBlob), PubBlobLen);
      if (Key == nullptr)
      {
        throw Exception("Invalid public key.");
      }
      char * FmtKey = Algorithm->fmtkey(Key);
      Result = UnicodeString(FmtKey);
      sfree(FmtKey);
      Algorithm->freekey(Key);
    },
    __finally
    {
      sfree(PubBlob);
      sfree(AlgorithmName);
      sfree(CommentPtr);
    } end_try__finally
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString GetKeyTypeHuman(UnicodeString AKeyType)
{
  UnicodeString Result;
  if (AKeyType == ssh_dss.keytype)
  {
    Result = "DSA";
  }
  else if (AKeyType == ssh_rsa.keytype)
  {
    Result = "RSA";
  }
  else if (AKeyType == ssh_ecdsa_ed25519.keytype)
  {
    Result = "Ed25519";
  }
  else if (AKeyType == ssh_ecdsa_nistp256.keytype)
  {
    Result = "ECDSA/nistp256";
  }
  else if (AKeyType == ssh_ecdsa_nistp384.keytype)
  {
    Result = "ECDSA/nistp384";
  }
  else if (AKeyType == ssh_ecdsa_nistp521.keytype)
  {
    Result = "ECDSA/nistp521";
  }
  else
  {
    DebugFail();
    Result = AKeyType;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool IsOpenSSH(UnicodeString SshImplementation)
{
  return
    // e.g. "OpenSSH_5.3"
    (SshImplementation.Pos(L"OpenSSH") == 1) ||
    // Sun SSH is based on OpenSSH (suffers the same bugs)
    (SshImplementation.Pos(L"Sun_SSH") == 1);
}
//---------------------------------------------------------------------------
TStrings * SshCipherList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  // Same order as DefaultCipherList
  const ssh2_ciphers * Ciphers[] = { &ssh2_aes, &ssh2_ccp, &ssh2_blowfish, &ssh2_3des, &ssh2_arcfour, &ssh2_des };
  for (unsigned int Index = 0; Index < LENOF(Ciphers); Index++)
  {
    for (int Index2 = 0; Index2 < Ciphers[Index]->nciphers; Index2++)
    {
      UnicodeString Name = UnicodeString(Ciphers[Index]->list[Index2]->name);
      Result->Add(Name);
    }
  }
  return Result.release();
}
//---------------------------------------------------------------------------
TStrings * SshKexList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  // Same order as DefaultKexList
  const ssh_kexes * Kexes[] = { &ssh_ecdh_kex, &ssh_diffiehellman_gex, &ssh_diffiehellman_group14, &ssh_rsa_kex, &ssh_diffiehellman_group1 };
  for (unsigned int Index = 0; Index < LENOF(Kexes); Index++)
  {
    for (int Index2 = 0; Index2 < Kexes[Index]->nkexes; Index2++)
    {
      UnicodeString Name = UnicodeString(Kexes[Index]->list[Index2]->name);
      Result->Add(Name);
    }
  }
  return Result.release();
}
//---------------------------------------------------------------------------
TStrings * SshHostKeyList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  const int MaxCount = 10;
  const ssh_signkey * SignKeys[MaxCount];
  int Count = LENOF(SignKeys);
  get_hostkey_algs(&Count, SignKeys);

  for (int Index = 0; Index < Count; Index++)
  {
    const ssh_signkey * SignKey = SignKeys[Index];
    UnicodeString Name = UnicodeString(SignKey->name);
    Result->Add(Name);
  }
  return Result.release();
}
//---------------------------------------------------------------------------
TStrings * SshMacList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  const struct ssh_mac ** Macs = nullptr;
  int Count = 0;
  get_macs(&Count, &Macs);

  for (int Index = 0; Index < Count; Index++)
  {
    UnicodeString Name = UnicodeString(Macs[Index]->name);
    Result->Add(Name);
  }
  return Result.release();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
