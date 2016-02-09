#include <vcl.h>
#pragma hdrstop

#define PUTTY_DO_GLOBALS
#include <Exceptions.h>
#include <StrUtils.hpp>

#include "PuttyIntf.h"
#include "Interface.h"
#include "SecureShell.h"
#include "CoreMain.h"
#include "TextsCore.h"

char sshver[50];
const int platform_uses_x11_unix_by_default = TRUE;
CRITICAL_SECTION putty_section;
bool SaveRandomSeed;
char appname_[50];
const char * const appname = appname_;

extern "C"
{
#include <winstuff.h>
}

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
  assert(!VersionString.IsEmpty() && (static_cast<size_t>(VersionString.Length()) < _countof(sshver)));
  strcpy_s(sshver, sizeof(sshver), VersionString.c_str());
  AnsiString AppName = AnsiString(GetAppNameString());
  assert(!AppName.IsEmpty() && (static_cast<size_t>(AppName.Length()) < _countof(appname_)));
  strcpy_s(appname_, sizeof(appname_), AppName.c_str());
}

void PuttyFinalize()
{
  if (SaveRandomSeed)
  {
    random_save_seed();
  }
  random_unref();

  sk_cleanup();
  win_misc_cleanup();
  DeleteCriticalSection(&putty_section);
}

void DontSaveRandomSeed()
{
  SaveRandomSeed = false;
}

extern "C" char * do_select(Plug plug, SOCKET skt, int startup)
{
  void * frontend = nullptr;

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
  assert(frontend);

  TSecureShell * SecureShell = NB_STATIC_DOWNCAST(TSecureShell, frontend);
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

int from_backend(void * frontend, int is_stderr, const char * data, int datalen)
{
  assert(frontend);
  if (is_stderr >= 0)
  {
    assert((is_stderr == 0) || (is_stderr == 1));
    (NB_STATIC_DOWNCAST(TSecureShell, frontend))->FromBackend((is_stderr == 1), reinterpret_cast<const uint8_t *>(data), datalen);
  }
  else
  {
    assert(is_stderr == -1);
    (NB_STATIC_DOWNCAST(TSecureShell, frontend))->CWrite(data, datalen);
  }
  return 0;
}

int from_backend_untrusted(void * /*frontend*/, const char * /*data*/, int /*len*/)
{
  // currently used with authentication banner only,
  // for which we have own interface display_banner
  return 0;
}

int from_backend_eof(void * /*frontend*/)
{
  return FALSE;
}

int GetUserpassInput(prompts_t * p, const uint8_t * /*in*/, int /*inlen*/)
{
  assert(p != nullptr);
  TSecureShell * SecureShell = NB_STATIC_DOWNCAST(TSecureShell, p->frontend);
  assert(SecureShell != nullptr);

  int Result;
  std::unique_ptr<TStrings> Prompts(new TStringList());
  std::unique_ptr<TStrings> Results(new TStringList());
  {
    for (size_t Index = 0; Index < p->n_prompts; ++Index)
    {
      prompt_t * Prompt = p->prompts[Index];
      Prompts->AddObject(Prompt->prompt, reinterpret_cast<TObject *>(static_cast<size_t>(FLAGMASK(Prompt->echo, pupEcho))));
      // this fails, when new passwords do not match on change password prompt,
      // and putty retries the prompt
      assert(Prompt->resultsize == 0);
      Results->Add(L"");
    }

    if (SecureShell->PromptUser(p->to_server != 0, p->name, p->name_reqd != 0,
          UnicodeString(p->instruction), p->instr_reqd != 0, Prompts.get(), Results.get()))
    {
      for (size_t Index = 0; Index < p->n_prompts; ++Index)
      {
        prompt_t * Prompt = p->prompts[Index];
        prompt_set_result(Prompt, AnsiString(Results->GetString(Index).c_str()).c_str());
      }
      Result = 1;
    }
    else
    {
      Result = 0;
    }
  }

  return Result;
}

int get_userpass_input(prompts_t * p, const uint8_t * in, int inlen)
{
  return GetUserpassInput(p, in, inlen);
}

char * get_ttymode(void * /*frontend*/, const char * /*mode*/)
{
  // should never happen when Config.nopty == TRUE
  FAIL;
  return nullptr;
}

void logevent(void * frontend, const char * string)
{
  // Frontend maybe nullptr here
  if (frontend != nullptr)
  {
    (NB_STATIC_DOWNCAST(TSecureShell, frontend))->PuttyLogEvent(string);
  }
}

void connection_fatal(void * frontend, const char * fmt, ...)
{
  va_list Param;
  std::string Buf;
  Buf.resize(32*1024);
  va_start(Param, fmt);
  vsnprintf_s((char *)Buf.c_str(), Buf.size(), _TRUNCATE, fmt, Param);
  Buf[Buf.size() - 1] = '\0';
  va_end(Param);

  assert(frontend != nullptr);
  (NB_STATIC_DOWNCAST(TSecureShell, frontend))->PuttyFatalError(UnicodeString(Buf.c_str()));
}

int verify_ssh_host_key(void * frontend, char * host, int port, const char * keytype,
  char * keystr, char * fingerprint, void (* /*callback*/)(void * ctx, int result),
  void * /*ctx*/)
{
  assert(frontend != nullptr);
  (NB_STATIC_DOWNCAST(TSecureShell, frontend))->VerifyHostKey(UnicodeString(host), port, keytype, keystr, fingerprint);

  // We should return 0 when key was not confirmed, we throw exception instead.
  return 1;
}

int askalg(void * frontend, const char * algtype, const char * algname,
  void (* /*callback*/)(void * ctx, int result), void * /*ctx*/)
{
  assert(frontend != nullptr);
  (NB_STATIC_DOWNCAST(TSecureShell, frontend))->AskAlg(algtype, algname);

  // We should return 0 when alg was not confirmed, we throw exception instead.
  return 1;
}

void old_keyfile_warning(void)
{
  // no reference to TSecureShell instance available
}

void display_banner(void * frontend, const char * banner, int size)
{
  assert(frontend);
  UnicodeString Banner(banner, size);
  (NB_STATIC_DOWNCAST(TSecureShell, frontend))->DisplayBanner(Banner);
}

static void SSHFatalError(const char * Format, va_list Param)
{
  std::string Buf;
  Buf.resize(32*1024);
  vsnprintf_s((char *)Buf.c_str(), Buf.size(), _TRUNCATE, Format, Param);
  Buf[Buf.size() - 1] = '\0';

  // Only few calls from putty\winnet.c might be connected with specific
  // TSecureShell. Otherwise called only for really fatal errors
  // like 'out of memory' from putty\ssh.c.
  throw ESshFatal(nullptr, Buf.c_str());
}

void fatalbox(const char * fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
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

void CleanupExit(int /*code*/)
{
  throw ESshFatal(nullptr, L"");
}

void cleanup_exit(int code)
{
  CleanupExit(code);
}

int askappend(void * /*frontend*/, Filename * /*filename*/,
  void (* /*callback*/)(void * ctx, int result), void * /*ctx*/)
{
  // this is called from logging.c of putty, which is never used with WinSCP
  FAIL;
  return 0;
}

void ldisc_send(void * /*handle*/, char * /*buf*/, int len, int /*interactive*/)
{
  // This is only here because of the calls to ldisc_send(nullptr,
  // 0) in ssh.c. Nothing in PSCP actually needs to use the ldisc
  // as an ldisc. So if we get called with any real data, I want
  // to know about it.
  assert(len == 0);
  USEDPARAM(len);
}

void agent_schedule_callback(void (* /*callback*/)(void *, void *, int),
  void * /*callback_ctx*/, void * /*data*/, int /*len*/)
{
  FAIL;
}

void notify_remote_exit(void * /*frontend*/)
{
  // nothing
}

void update_specials_menu(void * /*frontend*/)
{
  // nothing
}

unsigned long schedule_timer(int ticks, timer_fn_t /*fn*/, void * /*ctx*/)
{
  return ticks + ::GetTickCount();
}

void expire_timer_context(void * /*ctx*/)
{
  // nothing
}

Pinger pinger_new(Conf * /*conf*/, Backend * /*back*/, void * /*backhandle*/)
{
  return nullptr;
}

void pinger_reconfig(Pinger /*pinger*/, Conf * /*oldconf*/, Conf * /*newconf*/)
{
  // nothing
}

void pinger_free(Pinger /*pinger*/)
{
  // nothing
}

void set_busy_status(void * /*frontend*/, int /*status*/)
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

static long OpenWinSCPKey(HKEY Key, const char * SubKey, HKEY * Result, bool CanCreate)
{
  long R;
  assert(GetConfiguration() != nullptr);

  assert(Key == HKEY_CURRENT_USER);
  USEDPARAM(Key);

  UnicodeString RegKey = SubKey;
  UnicodeString OriginalPuttyRegistryStorageKey(PUTTY_REG_POS);
  intptr_t PuttyKeyLen = OriginalPuttyRegistryStorageKey.Length();
  assert(RegKey.SubString(1, PuttyKeyLen) == OriginalPuttyRegistryStorageKey);
  RegKey = RegKey.SubString(PuttyKeyLen + 1, RegKey.Length() - PuttyKeyLen);
  if (!RegKey.IsEmpty())
  {
    assert(RegKey[1] == L'\\');
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
    assert(RegKey == L"SshHostKeys");

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

long reg_open_winscp_key(HKEY Key, const char * SubKey, HKEY * Result)
{
  return OpenWinSCPKey(Key, SubKey, Result, false);
}

long reg_create_winscp_key(HKEY Key, const char * SubKey, HKEY * Result)
{
  return OpenWinSCPKey(Key, SubKey, Result, true);
}

long reg_query_winscp_value_ex(HKEY Key, const char * ValueName, unsigned long * /*Reserved*/,
  unsigned long * Type, uint8_t * Data, unsigned long * DataSize)
{
  long R;
  assert(GetConfiguration() != nullptr);

  THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  AnsiString Value;
  if (Storage == nullptr)
  {
    if (UnicodeString(ValueName) == L"RandSeedFile")
    {
      Value = GetConfiguration()->GetRandomSeedFileName();
      R = ERROR_SUCCESS;
    }
    else
    {
      FAIL;
      R = ERROR_READ_FAULT;
    }
  }
  else
  {
    if (Storage->ValueExists(ValueName))
    {
      Value = Storage->ReadStringRaw(ValueName, L"");
      R = ERROR_SUCCESS;
    }
    else
    {
      R = ERROR_READ_FAULT;
    }
  }

  if (R == ERROR_SUCCESS)
  {
    assert(Type != nullptr);
    *Type = REG_SZ;
    char * DataStr = reinterpret_cast<char *>(Data);
    int sz = static_cast<int>(*DataSize);
    if (sz > 0)
    {
        strncpy(DataStr, Value.c_str(), sz);
        DataStr[sz - 1] = '\0';
    }
    *DataSize = static_cast<uint32_t>(strlen(DataStr));
  }

  return R;
}

long reg_set_winscp_value_ex(HKEY Key, const char * ValueName, unsigned long /*Reserved*/,
  unsigned long Type, const uint8_t * Data, unsigned long DataSize)
{
  assert(Type == REG_SZ);
  USEDPARAM(Type);
  THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  assert(Storage != nullptr);
  if (Storage != nullptr)
  {
    UnicodeString Value(reinterpret_cast<const char *>(Data), DataSize - 1);
    Storage->WriteStringRaw(ValueName, Value);
  }

  return ERROR_SUCCESS;
}

long reg_close_winscp_key(HKEY Key)
{
  assert(GetConfiguration() != nullptr);

  THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  if (Storage != nullptr)
  {
    SAFE_DESTROY(Storage);
  }

  return ERROR_SUCCESS;
}

TKeyType GetKeyType(const UnicodeString & AFileName)
{
  assert(ktUnopenable == SSH_KEYTYPE_UNOPENABLE);
  assert(ktSSH2PublicOpenSSH == SSH_KEYTYPE_SSH2_PUBLIC_OPENSSH);
  UTF8String UtfFileName = UTF8String(::ExpandEnvironmentVariables(AFileName));
  Filename * KeyFile = filename_from_str(UtfFileName.c_str());
  TKeyType Result = static_cast<TKeyType>(key_type(KeyFile));
  filename_free(KeyFile);
  return Result;
}

UnicodeString GetKeyTypeName(TKeyType KeyType)
{
  return key_type_to_str(KeyType);
}

int64_t ParseSize(const UnicodeString & SizeStr)
{
  AnsiString AnsiSizeStr = AnsiString(SizeStr);
  return parse_blocksize64(AnsiSizeStr.c_str());
}

bool HasGSSAPI(const UnicodeString & CustomPath)
{
  static int has = -1;
  if (has < 0)
  {
    Conf * conf = conf_new();
    ssh_gss_liblist * List = nullptr;
    {
      SCOPE_EXIT
      {
        ssh_gss_cleanup(List);
        conf_free(conf);
      };
      Filename * filename = filename_from_str(UTF8String(CustomPath).c_str());
      conf_set_filename(conf, CONF_ssh_gss_custom, filename);
      filename_free(filename);
      List = ssh_gss_setup(conf);
      for (intptr_t Index = 0; (has <= 0) && (Index < List->nlibraries); ++Index)
      {
        ssh_gss_library * library = &List->libraries[Index];
        Ssh_gss_ctx ctx;
        ::ZeroMemory(&ctx, sizeof(ctx));
        has =
          ((library->acquire_cred(library, &ctx) == SSH_GSS_OK) &&
           (library->release_cred(library, &ctx) == SSH_GSS_OK)) ? 1 : 0;
      }
    }

    if (has < 0)
    {
      has = 0;
    }
  }
  return (has > 0);
}

UnicodeString NormalizeFingerprint(const UnicodeString & Fingerprint)
{
  UnicodeString Result = Fingerprint;
  UnicodeString DssName = UnicodeString(ssh_dss.name) + L" ";
  UnicodeString RsaName = UnicodeString(ssh_rsa.name) + L" ";
  // ssh_ecdsa_ed25519 ssh_ecdsa_nistp256 ssh_ecdsa_nistp384 ssh_ecdsa_nistp521
  UnicodeString ECDSAed25519Name = UnicodeString(ssh_ecdsa_ed25519.name) + L" ";
  UnicodeString ECDSAnistp256Name = UnicodeString(ssh_ecdsa_nistp256.name) + L" ";
  UnicodeString ECDSAnistp384Name = UnicodeString(ssh_ecdsa_nistp384.name) + L" ";
  UnicodeString ECDSAnistp521Name = UnicodeString(ssh_ecdsa_nistp521.name) + L" ";

  bool IsFingerprint = false;
  intptr_t LenStart = 0;
  if (StartsStr(DssName, Result))
  {
    LenStart = DssName.Length() + 1;
    IsFingerprint = true;
  }
  else if (StartsStr(RsaName, Result))
  {
    LenStart = RsaName.Length() + 1;
    IsFingerprint = true;
  }
  else if (StartsStr(ECDSAed25519Name, Result))
  {
	  LenStart = ECDSAed25519Name.Length() + 1;
	  IsFingerprint = true;
  }
  else if (StartsStr(ECDSAnistp256Name, Result))
  {
	  LenStart = ECDSAnistp256Name.Length() + 1;
	  IsFingerprint = true;
  }
  else if (StartsStr(ECDSAnistp384Name, Result))
  {
	  LenStart = ECDSAnistp384Name.Length() + 1;
	  IsFingerprint = true;
  }
  else if (StartsStr(ECDSAnistp521Name, Result))
  {
	  LenStart = ECDSAnistp521Name.Length() + 1;
	  IsFingerprint = true;
  }

  if (IsFingerprint)
  {
    Result[LenStart - 1] = L'-';
    intptr_t Space = Result.Pos(L" ");
    assert(IsNumber(Result.SubString(LenStart, Space - LenStart)));
    Result.Delete(LenStart, Space - LenStart + 1);
    Result = ReplaceChar(Result, L':', L'-');
  }
  return Result;
}

UnicodeString GetKeyTypeFromFingerprint(const UnicodeString & Fingerprint)
{
  UnicodeString Fingerprint2 = NormalizeFingerprint(Fingerprint);
  UnicodeString Result;
  if (StartsStr(UnicodeString(ssh_dss.name) + L"-", Fingerprint2))
  {
    Result = ssh_dss.keytype;
  }
  else if (StartsStr(UnicodeString(ssh_rsa.name) + L"-", Fingerprint2))
  {
    Result = ssh_rsa.keytype;
  }
  return Result;
}

