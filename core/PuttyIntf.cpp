//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#define PUTTY_DO_GLOBALS
#include "PuttyIntf.h"
#include "Interface.h"
#include "SecureShell.h"
#include "Exceptions.h"
#include "CoreMain.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
char sshver[50];
const int platform_uses_x11_unix_by_default = TRUE;
CRITICAL_SECTION noise_section;
bool SaveRandomSeed;
char appname_[50];
const char *const appname = appname_;
//---------------------------------------------------------------------------
void PuttyInitialize()
{
  SaveRandomSeed = true;

  InitializeCriticalSection(&noise_section);
  DEBUG_PRINTF(L"1");

  // make sure random generator is initialised, so random_save_seed()
  // in destructor can proceed
  random_ref();
  DEBUG_PRINTF(L"2");

  flags = FLAG_VERBOSE | FLAG_SYNCAGENT; // verbose log

  sk_init();
  DEBUG_PRINTF(L"3");

  std::wstring VersionString = SshVersionString();
  DEBUG_PRINTF(L"4");
  assert(!VersionString.empty() && (VersionString.size() < sizeof(sshver)));
  strcpy(sshver, ::W2MB(VersionString.c_str()).c_str());
  std::wstring AppName = AppNameString();
  DEBUG_PRINTF(L"5");
  assert(!AppName.empty() && (AppName.size() < sizeof(appname_)));
  strcpy(appname_, ::W2MB(AppName.c_str()).c_str());
  DEBUG_PRINTF(L"6");
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
  DeleteCriticalSection(&noise_section);
}
//---------------------------------------------------------------------------
void DontSaveRandomSeed()
{
  SaveRandomSeed = false;
}
//---------------------------------------------------------------------------
extern "C" char * do_select(Plug plug, SOCKET skt, int startup)
{
  void * frontend;

  if (!is_ssh(plug) && !is_pfwd(plug))
  {
    // If it is not SSH/PFwd plug, them it must be Proxy plug.
    // Get SSH/PFwd plug which it wraps.
    Proxy_Socket ProxySocket = ((Proxy_Plug)plug)->proxy_socket;
    plug = ProxySocket->plug;
  }

  bool pfwd = is_pfwd(plug);
  if (pfwd)
  {
    plug = (Plug)get_pfwd_backend(plug);
  }

  frontend = get_ssh_frontend(plug);
  assert(frontend);

  TSecureShell * SecureShell = reinterpret_cast<TSecureShell*>(frontend);
  if (!pfwd)
  {
    SecureShell->UpdateSocket(skt, startup);
  }
  else
  {
    SecureShell->UpdatePortFwdSocket(skt, startup);
  }

  return NULL;
}
//---------------------------------------------------------------------------
int from_backend(void * frontend, int is_stderr, const char * data, int datalen)
{
  assert(frontend);
  if (is_stderr >= 0)
  {
    assert((is_stderr == 0) || (is_stderr == 1));
    ((TSecureShell *)frontend)->FromBackend((is_stderr == 1), data, datalen);
  }
  else
  {
    assert(is_stderr == -1);
    ((TSecureShell *)frontend)->CWrite(data, datalen);
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
int get_userpass_input(prompts_t * p, unsigned char * /*in*/, int /*inlen*/)
{
  assert(p != NULL);
  TSecureShell * SecureShell = reinterpret_cast<TSecureShell *>(p->frontend);
  assert(SecureShell != NULL);

  int Result;
  TStringList Prompts;
  TStringList Results;
  {
    for (int Index = 0; Index < int(p->n_prompts); Index++)
    {
      prompt_t * Prompt = p->prompts[Index];
      Prompts.AddObject(::MB2W(Prompt->prompt), (TObject *)Prompt->echo);
      Results.AddObject(L"", (TObject *)Prompt->result_len);
    }

    if (SecureShell->PromptUser(p->to_server, ::MB2W(p->name), p->name_reqd,
          ::MB2W(p->instruction), p->instr_reqd, &Prompts, &Results))
    {
      for (int Index = 0; Index < int(p->n_prompts); Index++)
      {
        prompt_t * Prompt = p->prompts[Index];
        wcsncpy((wchar_t *)::MB2W(Prompt->result).c_str(), Results.GetString(Index).c_str(), Prompt->result_len);
        Prompt->result[Prompt->result_len - 1] = '\0';
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
//---------------------------------------------------------------------------
char * get_ttymode(void * /*frontend*/, const char * /*mode*/)
{
  // should never happen when Config.nopty == TRUE
  assert(false);
  return NULL;
}
//---------------------------------------------------------------------------
void logevent(void * frontend, const char * string)
{
  // Frontend maybe NULL here
  if (frontend != NULL)
  {
    ((TSecureShell *)frontend)->PuttyLogEvent(::MB2W(string));
  }
}
//---------------------------------------------------------------------------
void connection_fatal(void * frontend, char * fmt, ...)
{
  va_list Param;
  char Buf[200];
  va_start(Param, fmt);
  vsnprintf(Buf, sizeof(Buf), fmt, Param); \
  Buf[sizeof(Buf) - 1] = '\0'; \
  va_end(Param);

  assert(frontend != NULL);
  ((TSecureShell *)frontend)->PuttyFatalError(::MB2W(Buf));
}
//---------------------------------------------------------------------------
int verify_ssh_host_key(void * frontend, char * host, int port, char * keytype,
  char * keystr, char * fingerprint, void (* /*callback*/)(void * ctx, int result),
  void * /*ctx*/)
{
  assert(frontend != NULL);
  ((TSecureShell *)frontend)->VerifyHostKey(::MB2W(host), port, ::MB2W(keytype), ::MB2W(keystr), ::MB2W(fingerprint));

  // We should return 0 when key was not confirmed, we throw exception instead.
  return 1;
}
//---------------------------------------------------------------------------
int askalg(void * frontend, const char * algtype, const char * algname,
  void (* /*callback*/)(void * ctx, int result), void * /*ctx*/)
{
  assert(frontend != NULL);
  ((TSecureShell *)frontend)->AskAlg(::MB2W(algtype), ::MB2W(algname));

  // We should return 0 when alg was not confirmed, we throw exception instead.
  return 1;
}
//---------------------------------------------------------------------------
void old_keyfile_warning(void)
{
  // no reference to TSecureShell instace available
}
//---------------------------------------------------------------------------
void display_banner(void * frontend, const char * banner, int size)
{
  assert(frontend);
  std::wstring Banner(::MB2W(banner), size);
  ((TSecureShell *)frontend)->DisplayBanner(Banner);
}
//---------------------------------------------------------------------------
static void SSHFatalError(const char * Format, va_list Param)
{
  char Buf[200];
  vsnprintf(Buf, sizeof(Buf), Format, Param);
  Buf[sizeof(Buf) - 1] = '\0';

  // Only few calls from putty\winnet.c might be connected with specific
  // TSecureShell. Otherwise called only for really fatal errors
  // like 'out of memory' from putty\ssh.c.
  throw ESshFatal(NULL, ::MB2W(Buf));
}
//---------------------------------------------------------------------------
void fatalbox(char * fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}
//---------------------------------------------------------------------------
void modalfatalbox(char * fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  SSHFatalError(fmt, Param);
  va_end(Param);
}
//---------------------------------------------------------------------------
void cleanup_exit(int /*code*/)
{
  throw ESshFatal(NULL, L"");
}
//---------------------------------------------------------------------------
int askappend(void * /*frontend*/, Filename /*filename*/,
  void (* /*callback*/)(void * ctx, int result), void * /*ctx*/)
{
  // this is called from logging.c of putty, which is never used with WinSCP
  assert(false);
  return 0;
}
//---------------------------------------------------------------------------
void ldisc_send(void * /*handle*/, char * /*buf*/, int len, int /*interactive*/)
{
  // This is only here because of the calls to ldisc_send(NULL,
  // 0) in ssh.c. Nothing in PSCP actually needs to use the ldisc
  // as an ldisc. So if we get called with any real data, I want
  // to know about it.
  assert(len == 0);
  USEDPARAM(len);
}
//---------------------------------------------------------------------------
void agent_schedule_callback(void (* /*callback*/)(void *, void *, int),
  void * /*callback_ctx*/, void * /*data*/, int /*len*/)
{
  assert(false);
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
typedef void (*timer_fn_t)(void *ctx, long now);
long schedule_timer(int ticks, timer_fn_t /*fn*/, void * /*ctx*/)
{
  return ticks + GetTickCount();
}
//---------------------------------------------------------------------------
void expire_timer_context(void * /*ctx*/)
{
  // nothing
}
//---------------------------------------------------------------------------
Pinger pinger_new(Config * /*cfg*/, Backend * /*back*/, void * /*backhandle*/)
{
  return NULL;
}
//---------------------------------------------------------------------------
void pinger_reconfig(Pinger /*pinger*/, Config * /*oldcfg*/, Config * /*newcfg*/)
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
void platform_get_x11_auth(struct X11Display * /*display*/, const Config * /*cfg*/)
{
  // nothing, therefore no auth.
}
//---------------------------------------------------------------------------
int get_remote_username(Config * cfg, char *user, size_t len)
{
  if (*cfg->username)
  {
    strncpy(user, cfg->username, len);
    user[len-1] = '\0';
  }
  else
  {
    *user = '\0';
  }
  return (*user != '\0');
}
//---------------------------------------------------------------------------
static long OpenWinSCPKey(HKEY Key, const char * SubKey, HKEY * Result, bool CanCreate)
{
  long R;
  assert(Configuration != NULL);

  assert(Key == HKEY_CURRENT_USER);
  USEDPARAM(Key);

  std::wstring RegKey = ::MB2W(SubKey);
  int PuttyKeyLen = Configuration->GetPuttyRegistryStorageKey().size();
  assert(RegKey.substr(0, PuttyKeyLen) == Configuration->GetPuttyRegistryStorageKey());
  RegKey = RegKey.substr(PuttyKeyLen + 1, RegKey.size() - PuttyKeyLen);
  // DEBUG_PRINTF(L"RegKey = %s", RegKey.c_str());
  if (!RegKey.empty())
  {
    assert(RegKey[0] == '\\');
    RegKey.erase(0, 1);
  }

  if (RegKey.empty())
  {
    *Result = static_cast<HKEY>(NULL);
    R = ERROR_SUCCESS;
  }
  else
  {
    // we expect this to be called only from verify_host_key() or store_host_key()
    assert(RegKey == L"SshHostKeys");

    THierarchicalStorage * Storage = Configuration->CreateScpStorage(false);
    Storage->SetAccessMode((CanCreate ? smReadWrite : smRead));
    if (Storage->OpenSubKey(RegKey, CanCreate))
    {
      *Result = reinterpret_cast<HKEY>(Storage);
      R = ERROR_SUCCESS;
    }
    else
    {
      delete Storage;
      R = ERROR_CANTOPEN;
    }
  }

  return R;
}
//---------------------------------------------------------------------------
long reg_open_winscp_key(HKEY Key, const char * SubKey, HKEY * Result)
{
  return OpenWinSCPKey(Key, SubKey, Result, false);
}
//---------------------------------------------------------------------------
long reg_create_winscp_key(HKEY Key, const char * SubKey, HKEY * Result)
{
  return OpenWinSCPKey(Key, SubKey, Result, true);
}
//---------------------------------------------------------------------------
long reg_query_winscp_value_ex(HKEY Key, const char * ValueName, unsigned long * /*Reserved*/,
  unsigned long * Type, unsigned char * Data, unsigned long * DataSize)
{
  long R;
  assert(Configuration != NULL);

  THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  std::wstring Value;
  if (Storage == NULL)
  {
    if (std::wstring(::MB2W(ValueName)) == L"RandSeedFile")
    {
      Value = Configuration->GetRandomSeedFileName();
      R = ERROR_SUCCESS;
    }
    else
    {
      assert(false);
      R = ERROR_READ_FAULT;
    }
  }
  else
  {
    if (Storage->ValueExists(::MB2W(ValueName)))
    {
      Value = Storage->ReadStringRaw(::MB2W(ValueName), L"");
      R = ERROR_SUCCESS;
    }
    else
    {
      R = ERROR_READ_FAULT;
    }
  }

  if (R == ERROR_SUCCESS)
  {
    assert(Type != NULL);
    *Type = REG_SZ;
    char * DataStr = reinterpret_cast<char *>(Data);
    strncpy(DataStr, ::W2MB(Value.c_str()).c_str(), *DataSize);
    DataStr[*DataSize - 1] = '\0';
    *DataSize = strlen(DataStr);
  }

  return R;
}
//---------------------------------------------------------------------------
long reg_set_winscp_value_ex(HKEY Key, const char * ValueName, unsigned long /*Reserved*/,
  unsigned long Type, const unsigned char * Data, unsigned long DataSize)
{
  assert(Configuration != NULL);

  assert(Type == REG_SZ);
  USEDPARAM(Type);
  THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  assert(Storage != NULL);
  if (Storage != NULL)
  {
    std::string Value(reinterpret_cast<const char*>(Data), DataSize - 1);
    Storage->WriteStringRaw(::MB2W(ValueName), ::MB2W(Value.c_str()));
  }

  return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
long reg_close_winscp_key(HKEY Key)
{
  assert(Configuration != NULL);

  THierarchicalStorage * Storage = reinterpret_cast<THierarchicalStorage *>(Key);
  if (Storage != NULL)
  {
    delete Storage;
  }

  return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
TKeyType KeyType(std::wstring FileName)
{
  assert(ktUnopenable == SSH_KEYTYPE_UNOPENABLE);
  assert(ktSSHCom == SSH_KEYTYPE_SSHCOM);
  Filename KeyFile;
  ASCOPY(KeyFile.path, ::W2MB(FileName.c_str()));
  return (TKeyType)key_type(&KeyFile);
}
//---------------------------------------------------------------------------
std::wstring KeyTypeName(TKeyType KeyType)
{
  return ::MB2W(key_type_to_str(KeyType));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
struct TUnicodeEmitParams
{
  std::wstring Buffer;
  int Pos;
  int Len;
};
//---------------------------------------------------------------------------
extern "C" void UnicodeEmit(void * AParams, long int Output)
{
  if (Output == 0xFFFFL) // see Putty's charset\internal.h
  {
    throw ExtException(LoadStr(DECODE_UTF_ERROR));
  }
  TUnicodeEmitParams * Params = (TUnicodeEmitParams *)AParams;
  if (Params->Pos >= Params->Len)
  {
    Params->Len += 50;
    Params->Buffer.resize(Params->Len);
  }
  Params->Pos++;
  Params->Buffer[Params->Pos] = (wchar_t)Output;
}
//---------------------------------------------------------------------------
std::wstring DecodeUTF(const std::wstring UTF)
{
  charset_state State;
  char * Str;
  TUnicodeEmitParams Params;
  std::wstring Result;

  State.s0 = 0;
  Str = (char *)::W2MB(UTF.c_str()).c_str();
  Params.Pos = 0;
  Params.Len = UTF.size();
  Params.Buffer.resize(Params.Len);

  while (*Str)
  {
    read_utf8(NULL, (unsigned char)*Str, &State, UnicodeEmit, &Params);
    Str++;
  }
  Params.Buffer.resize(Params.Pos);

  return Params.Buffer;
}
//---------------------------------------------------------------------------
struct TUnicodeEmitParams2
{
  std::wstring Buffer;
  int Pos;
  int Len;
};
//---------------------------------------------------------------------------
extern "C" void UnicodeEmit2(void * AParams, long int Output)
{
  if (Output == 0xFFFFL) // see Putty's charset\internal.h
  {
    throw ExtException(LoadStr(DECODE_UTF_ERROR));
  }
  TUnicodeEmitParams2 * Params = (TUnicodeEmitParams2 *)AParams;
  if (Params->Pos >= Params->Len)
  {
    Params->Len += 50;
    Params->Buffer.resize(Params->Len);
  }
  Params->Pos++;
  Params->Buffer[Params->Pos] = (unsigned char)Output;
}
//---------------------------------------------------------------------------
std::wstring EncodeUTF(const std::wstring Source)
{
  // std::wstring::c_bstr() returns NULL for empty strings
  // (as opposite to std::wstring::c_str() which returns "")
  if (Source.empty())
  {
    return L"";
  }
  else
  {
    charset_state State;
    wchar_t * Str;
    TUnicodeEmitParams2 Params;
    std::wstring Result;

    State.s0 = 0;
    Str = (wchar_t *)Source.c_str();
    Params.Pos = 0;
    Params.Len = Source.size();
    Params.Buffer.resize(Params.Len);

    while (*Str)
    {
      write_utf8(NULL, (wchar_t)*Str, &State, UnicodeEmit2, &Params);
      Str++;
    }
    Params.Buffer.resize(Params.Pos);

    return Params.Buffer;
  }
}
//---------------------------------------------------------------------------
__int64 ParseSize(std::wstring SizeStr)
{
  return parse_blocksize(::W2MB(SizeStr.c_str()).c_str());
}
//---------------------------------------------------------------------------
bool HasGSSAPI()
{
  static int has = -1;
  if (has < 0)
  {
    Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    ssh_gss_liblist * List = ssh_gss_setup(&cfg);
    {
      BOOST_SCOPE_EXIT ( (&List) )
      {
        ssh_gss_cleanup(List);
      } BOOST_SCOPE_EXIT_END
      for (int Index = 0; (has <= 0) && (Index < List->nlibraries); Index++)
      {
        ssh_gss_library * library = &List->libraries[Index];
        Ssh_gss_ctx ctx;
        memset(&ctx, 0, sizeof(ctx));
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
//---------------------------------------------------------------------------
